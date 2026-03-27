/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <autonamecache.hxx>
#include <compiler.hxx>
#include <dbdata.hxx>
#include <dociter.hxx>
#include <document.hxx>
#include <externalrefmgr.hxx>
#include <formula/FormulaCompiler.hxx>
#include <rangelst.hxx>

#include <com/sun/star/sheet/ExternalLinkInfo.hpp>

#include <optional>
#include <vector>

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Grammar.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/CompileHost.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::detail::compiler::CompileContext makeCompileContext(
    const ScAddress& rBaseAddress, formula::FormulaGrammar::Grammar eGrammar,
    bool bForPersistence = false, bool bAllowExternalReferences = true)
{
    spreadsheetengine::detail::compiler::CompileContext aContext;
    aContext.maGrammar = toApiGrammar(eGrammar);
    aContext.maBaseAddress = toApiCellAddress(rBaseAddress);
    aContext.mbForPersistence = bForPersistence;
    aContext.mbAllowExternalReferences = bAllowExternalReferences;
    return aContext;
}

namespace detail
{

inline formula::FormulaGrammar::Grammar getEffectiveGrammar(
    ScDocument& rDoc, const spreadsheetengine::detail::compiler::CompileContext& rContext)
{
    const formula::FormulaGrammar::Grammar eGrammar = toLibreOfficeGrammar(rContext.maGrammar);
    return eGrammar == formula::FormulaGrammar::GRAM_UNSPECIFIED ? rDoc.GetGrammar() : eGrammar;
}

inline ScAddress getBaseAddress(
    const spreadsheetengine::detail::compiler::CompileContext& rContext)
{
    return toLibreOfficeAddress(rContext.maBaseAddress);
}

inline OUString upperCaseName(spreadsheetengine::api::StringView rName)
{
    return ScGlobal::getCharClass().uppercase(
        toLibreOfficeString(spreadsheetengine::api::String(rName)));
}

inline std::optional<spreadsheetengine::detail::token::TableRefItem> mapTableRefItem(
    OpCode eOpCode)
{
    using spreadsheetengine::detail::token::TableRefItem;

    switch (eOpCode)
    {
        case ocTableRefItemAll:
            return TableRefItem::All;
        case ocTableRefItemHeaders:
            return TableRefItem::Headers;
        case ocTableRefItemData:
            return TableRefItem::Data;
        case ocTableRefItemTotals:
            return TableRefItem::Totals;
        case ocTableRefItemThisRow:
            return TableRefItem::ThisRow;
        default:
            return std::nullopt;
    }
}

inline std::optional<spreadsheetengine::detail::token::TableRefItem> parseTableRefItem(
    ScDocument& rDoc, spreadsheetengine::api::StringView rItemName,
    const spreadsheetengine::detail::compiler::CompileContext& rContext)
{
    if (rItemName.empty())
        return spreadsheetengine::detail::token::TableRefItem::Table;

    const formula::FormulaGrammar::Grammar eGrammar = getEffectiveGrammar(rDoc, rContext);
    ScCompiler aCompiler(rDoc, getBaseAddress(rContext), eGrammar);
    ScCompiler::OpCodeMapPtr xSymbols
        = aCompiler.GetOpCodeMap(formula::FormulaGrammar::extractFormulaLanguage(eGrammar));
    if (!xSymbols)
        return std::nullopt;

    const OUString aUpper = upperCaseName(rItemName);
    const formula::OpCodeHashMap& rHashMap = xSymbols->getHashMap();
    const auto it = rHashMap.find(aUpper);
    if (it == rHashMap.end())
        return std::nullopt;

    return mapTableRefItem(it->second);
}

inline std::optional<api::refdata::SingleRefData> lookupColRowNameInLabelRanges(
    ScDocument& rDoc, const OUString& rName, const ScAddress& rBaseAddress)
{
    bool bFound = false;
    ScSingleRefData aReference;
    const SCTAB nThisTab = rBaseAddress.Tab();

    for (short nThisSheetOnly = 1; nThisSheetOnly >= 0 && !bFound; --nThisSheetOnly)
    {
        for (short nRowName = 0; nRowName < 2 && !bFound; ++nRowName)
        {
            ScRangePairList* pRanges
                = nRowName ? rDoc.GetRowNameRanges() : rDoc.GetColNameRanges();
            if (!pRanges)
                continue;

            for (size_t nPair = 0; nPair < pRanges->size() && !bFound; ++nPair)
            {
                const ScRangePair& rPair = (*pRanges)[nPair];
                const ScRange& rNameRange = rPair.GetRange(0);
                if (nThisSheetOnly
                    && (rNameRange.aStart.Tab() > nThisTab || nThisTab > rNameRange.aEnd.Tab()))
                {
                    continue;
                }

                ScCellIterator aIter(rDoc, rNameRange);
                for (bool bHas = aIter.first(); bHas && !bFound; bHas = aIter.next())
                {
                    const CellType eType = aIter.getType();
                    bool bOk = false;
                    if (eType == CELLTYPE_FORMULA)
                    {
                        ScFormulaCell* pFormula = aIter.getFormulaCell();
                        bOk = pFormula && pFormula->GetCode()->GetCodeLen() > 0
                              && pFormula->aPos != rBaseAddress;
                    }
                    else
                    {
                        bOk = true;
                    }

                    if (!bOk || !aIter.hasString())
                        continue;

                    if (!ScGlobal::GetTransliteration().isEqual(aIter.getString(), rName))
                        continue;

                    aReference.InitFlags();
                    if (!nRowName)
                        aReference.SetColRel(true);
                    else
                        aReference.SetRowRel(true);
                    aReference.SetAddress(
                        rDoc.GetSheetLimits(), aIter.GetPos(), rBaseAddress);
                    bFound = true;
                }
            }
        }
    }

    if (!bFound)
        return std::nullopt;

    return aReference.toApiSingleRefData();
}

inline std::optional<api::refdata::SingleRefData> lookupColRowNameByAutoLookup(
    ScDocument& rDoc, const OUString& rName, const ScAddress& rBaseAddress)
{
    bool bFound = false;
    tools::Long nDistance = 0;
    tools::Long nMax = 0;
    bool bTwo = false;
    const tools::Long nBaseColumn = static_cast<tools::Long>(rBaseAddress.Col());
    const tools::Long nBaseRow = static_cast<tools::Long>(rBaseAddress.Row());
    ScAddress aFirst(0, 0, rBaseAddress.Tab());
    ScAddress aSecond(rDoc.MaxCol(), rDoc.MaxRow(), rBaseAddress.Tab());

    const auto handleCandidate = [&](const ScAddress& rAddress) {
        if (rAddress == rBaseAddress)
            return;

        const tools::Long nColumnDelta = nBaseColumn - rAddress.Col();
        const tools::Long nRowDelta = nBaseRow - rAddress.Row();
        const tools::Long nCandidateDistance = nColumnDelta * nColumnDelta + nRowDelta * nRowDelta;

        if (bFound)
        {
            if (nCandidateDistance >= nDistance)
                return;

            if (nColumnDelta < 0 || nRowDelta < 0)
            {
                bTwo = true;
                aSecond = rAddress;
                nMax = std::max(
                    nBaseColumn + std::abs(nColumnDelta), nBaseRow + std::abs(nRowDelta));
                nDistance = nCandidateDistance;
                return;
            }

            if (rAddress.Row() >= aFirst.Row() || nBaseRow < static_cast<tools::Long>(aFirst.Row()))
            {
                bTwo = false;
                aFirst = rAddress;
                nMax = std::max(nBaseColumn + nColumnDelta, nBaseRow + nRowDelta);
                nDistance = nCandidateDistance;
            }
            return;
        }

        aFirst = rAddress;
        nDistance = nCandidateDistance;
        nMax = std::max(
            nBaseColumn + std::abs(nColumnDelta), nBaseRow + std::abs(nRowDelta));
        bFound = true;
    };

    ScAutoNameCache* pNameCache = rDoc.GetAutoNameCache();
    if (pNameCache)
    {
        const ScAutoNameAddresses& rAddresses
            = pNameCache->GetNameOccurrences(rName, rBaseAddress.Tab());
        for (const ScAddress& rAddress : rAddresses)
        {
            if (bFound && nMax < static_cast<tools::Long>(rAddress.Col()))
                break;
            handleCandidate(rAddress);
        }
    }
    else
    {
        ScCellIterator aIter(rDoc, ScRange(aFirst, aSecond));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            if (bFound && nMax < static_cast<tools::Long>(aIter.GetPos().Col()))
                break;

            const CellType eType = aIter.getType();
            bool bOk = false;
            if (eType == CELLTYPE_FORMULA)
            {
                ScFormulaCell* pFormula = aIter.getFormulaCell();
                bOk = pFormula && pFormula->GetCode()->GetCodeLen() > 0
                      && pFormula->aPos != rBaseAddress;
            }
            else
            {
                bOk = true;
            }

            if (!bOk || !aIter.hasString())
                continue;

            if (!ScGlobal::GetTransliteration().isEqual(aIter.getString(), rName))
                continue;

            handleCandidate(aIter.GetPos());
        }
    }

    if (!bFound)
        return std::nullopt;

    ScAddress aChosen;
    if (bTwo)
    {
        if (nBaseColumn >= static_cast<tools::Long>(aFirst.Col())
            && nBaseRow >= static_cast<tools::Long>(aFirst.Row()))
        {
            aChosen = aFirst;
        }
        else if (nBaseColumn < static_cast<tools::Long>(aFirst.Col()))
        {
            if (nBaseRow >= static_cast<tools::Long>(aSecond.Row()))
                aChosen = aSecond;
            else
                aChosen = aFirst;
        }
        else
        {
            const tools::Long nFirstColumnDelta = nBaseColumn - aFirst.Col();
            const tools::Long nFirstRowDelta = nBaseRow - aFirst.Row();
            const tools::Long nSecondColumnDelta = nBaseColumn - aSecond.Col();
            const tools::Long nSecondRowDelta = nBaseRow - aSecond.Row();
            if (nFirstColumnDelta * nFirstColumnDelta + nFirstRowDelta * nFirstRowDelta
                <= nSecondColumnDelta * nSecondColumnDelta + nSecondRowDelta * nSecondRowDelta)
            {
                aChosen = aFirst;
            }
            else
            {
                aChosen = aSecond;
            }
        }
    }
    else
    {
        aChosen = aFirst;
    }

    ScSingleRefData aReference;
    aReference.InitAddress(aChosen);
    if ((aChosen.Row() < rDoc.MaxRow()
         && rDoc.HasStringData(aChosen.Col(), aChosen.Row() + 1, aChosen.Tab()))
        || (aChosen.Row() > 0 && rDoc.HasStringData(aChosen.Col(), aChosen.Row() - 1, aChosen.Tab()))
        || (aChosen.Row() < rDoc.MaxRow()
            && rDoc.GetRefCellValue(
                       ScAddress(aChosen.Col(), aChosen.Row() + 1, aChosen.Tab()))
                   .isEmpty()
            && aChosen.Col() < rDoc.MaxCol()
            && rDoc.GetRefCellValue(
                       ScAddress(aChosen.Col() + 1, aChosen.Row(), aChosen.Tab()))
                   .hasNumeric()))
    {
        aReference.SetRowRel(true);
    }
    else
    {
        aReference.SetColRel(true);
    }
    aReference.SetAddress(rDoc.GetSheetLimits(), aChosen, rBaseAddress);
    return aReference.toApiSingleRefData();
}

} // namespace detail

class DocumentCompileHost final : public spreadsheetengine::detail::compiler::NameResolver
    , public spreadsheetengine::detail::compiler::DatabaseRangeResolver
    , public spreadsheetengine::detail::compiler::TableRefResolver
    , public spreadsheetengine::detail::compiler::ColRowNameResolver
    , public spreadsheetengine::detail::compiler::ExternalNameResolver
{
public:
    explicit DocumentCompileHost(
        ScDocument& rDoc,
        css::uno::Sequence<css::sheet::ExternalLinkInfo> aExternalLinks = {})
        : mrDoc(rDoc)
        , maExternalLinks(std::move(aExternalLinks))
    {
    }

    [[nodiscard]] spreadsheetengine::detail::compiler::CompileHosts hosts() const
    {
        return { this, this, this, this, this };
    }

    [[nodiscard]] std::optional<spreadsheetengine::detail::token::NameData> lookupRangeName(
        spreadsheetengine::api::StringView rName,
        std::optional<spreadsheetengine::api::SheetId> onSheet,
        const spreadsheetengine::detail::compiler::CompileContext& rContext) const override
    {
        const OUString aUpper = detail::upperCaseName(rName);

        SCTAB nSheet = onSheet.has_value() ? static_cast<SCTAB>(*onSheet)
                                           : detail::getBaseAddress(rContext).Tab();
        const ScRangeName* pLocalNames = mrDoc.GetRangeName(nSheet);
        if (pLocalNames)
        {
            const ScRangeData* pData = pLocalNames->findByUpperName(aUpper);
            if (pData)
                return spreadsheetengine::detail::token::NameData { nSheet, pData->GetIndex() };
        }

        const ScRangeName* pGlobalNames = mrDoc.GetRangeName();
        if (!pGlobalNames)
            return std::nullopt;

        const ScRangeData* pGlobalData = pGlobalNames->findByUpperName(aUpper);
        if (!pGlobalData)
            return std::nullopt;

        return spreadsheetengine::detail::token::NameData { -1, pGlobalData->GetIndex() };
    }

    [[nodiscard]] std::optional<spreadsheetengine::detail::token::DatabaseRangeData>
    lookupDatabaseRange(spreadsheetengine::api::StringView rName,
        const spreadsheetengine::detail::compiler::CompileContext&) const override
    {
        ScDBData* pData = mrDoc.GetDBCollection()->getNamedDBs().findByUpperName(
            detail::upperCaseName(rName));
        if (!pData)
            return std::nullopt;

        return spreadsheetengine::detail::token::DatabaseRangeData { pData->GetIndex() };
    }

    [[nodiscard]] std::optional<spreadsheetengine::detail::token::TableRefData>
    lookupTableReference(spreadsheetengine::api::StringView rTableName,
        spreadsheetengine::api::StringView rItemName,
        const spreadsheetengine::detail::compiler::CompileContext& rContext) const override
    {
        ScDBData* pData = mrDoc.GetDBCollection()->getNamedDBs().findByUpperName(
            detail::upperCaseName(rTableName));
        if (!pData)
            return std::nullopt;

        const auto oItem = detail::parseTableRefItem(mrDoc, rItemName, rContext);
        if (!oItem)
            return std::nullopt;

        return spreadsheetengine::detail::token::TableRefData { pData->GetIndex(), *oItem };
    }

    [[nodiscard]] std::optional<spreadsheetengine::api::refdata::SingleRefData> lookupColRowName(
        spreadsheetengine::api::StringView rName,
        const spreadsheetengine::detail::compiler::CompileContext& rContext) const override
    {
        OUString aName(toLibreOfficeString(spreadsheetengine::api::String(rName)));
        formula::FormulaCompiler::DeQuote(aName);
        const ScAddress aBaseAddress = detail::getBaseAddress(rContext);

        if (const auto oInList = detail::lookupColRowNameInLabelRanges(mrDoc, aName, aBaseAddress))
            return oInList;

        if (!mrDoc.GetDocOptions().IsLookUpColRowNames())
            return std::nullopt;

        return detail::lookupColRowNameByAutoLookup(mrDoc, aName, aBaseAddress);
    }

    [[nodiscard]] std::optional<spreadsheetengine::detail::token::ExternalNameData>
    lookupExternalName(spreadsheetengine::api::StringView rSymbol,
        const spreadsheetengine::detail::compiler::CompileContext& rContext) const override
    {
        if (!rContext.mbAllowExternalReferences || !mrDoc.HasExternalRefManager())
            return std::nullopt;

        const formula::FormulaGrammar::AddressConvention eConvention
            = formula::FormulaGrammar::extractRefConvention(
                detail::getEffectiveGrammar(mrDoc, rContext));
        const ScCompiler::Convention* pConvention = ScCompiler::GetRefConvention(eConvention);
        if (!pConvention)
            return std::nullopt;

        OUString aFile;
        OUString aName;
        if (!pConvention->parseExternalName(
                toLibreOfficeString(spreadsheetengine::api::String(rSymbol)), aFile, aName, mrDoc,
                &maExternalLinks))
        {
            return std::nullopt;
        }

        if (aFile.getLength() > MAXSTRLEN || aName.getLength() > MAXSTRLEN)
            return std::nullopt;

        ScExternalRefManager* pRefMgr = mrDoc.GetExternalRefManager();
        if (!pRefMgr)
            return std::nullopt;

        pRefMgr->convertToAbsName(aFile);
        const sal_uInt16 nFileId = pRefMgr->getExternalFileId(aFile);
        if (!pRefMgr->isValidRangeName(nFileId, aName))
            return std::nullopt;

        const OUString* pRealName = pRefMgr->getRealRangeName(nFileId, aName);
        return spreadsheetengine::detail::token::ExternalNameData {
            nFileId, toApiString(pRealName ? *pRealName : aName)
        };
    }

private:
    ScDocument& mrDoc;
    css::uno::Sequence<css::sheet::ExternalLinkInfo> maExternalLinks;
};

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
