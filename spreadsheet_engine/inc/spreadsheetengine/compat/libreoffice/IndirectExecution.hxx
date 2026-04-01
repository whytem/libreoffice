/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <optional>

#include <address.hxx>
#include <compiler.hxx>
#include <document.hxx>
#include <externalrefmgr.hxx>
#include <formula/types.hxx>
#include <global.hxx>
#include <rangeutl.hxx>
#include <rangenam.hxx>
#include <svl/sharedstring.hxx>
#include <tokenarray.hxx>

namespace spreadsheetengine::compat::libreoffice::indirectexecution
{

struct IndirectExecutionResult
{
    enum class Kind : std::uint8_t
    {
        SingleRef,
        DoubleRef,
        ExternalSingleRef,
        ExternalDoubleRef,
        Token
    };

    Kind meKind = Kind::SingleRef;
    ScRefAddress maRef1;
    ScRefAddress maRef2;
    sal_uInt16 mnFileId = 0;
    OUString maTabName;
    formula::FormulaConstTokenRef mxToken;
};

namespace detail
{

[[nodiscard]] inline bool isTableStructuredRef(const OUString& rReferenceText, sal_Int32& rIndex)
{
    rIndex = ScGlobal::FindUnquoted(rReferenceText, '[');
    return rIndex > 0 && ScGlobal::FindUnquoted(rReferenceText, ']', rIndex + 1) > rIndex;
}

[[nodiscard]] inline bool isQuotedExternalNameReference(const OUString& rReferenceText,
    formula::FormulaGrammar::AddressConvention eConvention, bool bTryXlA1, sal_Int32& rIndex)
{
    if (rReferenceText.isEmpty() || rReferenceText[0] != '\'')
        return false;

    if (bTryXlA1 || eConvention == formula::FormulaGrammar::CONV_OOO)
    {
        rIndex = ScGlobal::FindUnquoted(rReferenceText, '#');
        if (rIndex >= 3 && rReferenceText[rIndex - 1] == '\'')
            return true;
    }

    if (bTryXlA1 || eConvention != formula::FormulaGrammar::CONV_OOO)
    {
        rIndex = ScGlobal::FindUnquoted(rReferenceText, '!');
        if (rIndex >= 3 && rReferenceText[rIndex - 1] == '\'')
            return true;
    }

    return false;
}

[[nodiscard]] inline IndirectExecutionResult makeSingleRefResult(const ScAddress& rAddress)
{
    IndirectExecutionResult aResult;
    aResult.meKind = IndirectExecutionResult::Kind::SingleRef;
    aResult.maRef1 = ScRefAddress(rAddress.Col(), rAddress.Row(), rAddress.Tab());
    return aResult;
}

[[nodiscard]] inline IndirectExecutionResult makeDoubleRefResult(const ScRange& rRange)
{
    IndirectExecutionResult aResult;
    aResult.meKind = IndirectExecutionResult::Kind::DoubleRef;
    aResult.maRef1 = ScRefAddress(rRange.aStart.Col(), rRange.aStart.Row(), rRange.aStart.Tab());
    aResult.maRef2 = ScRefAddress(rRange.aEnd.Col(), rRange.aEnd.Row(), rRange.aEnd.Tab());
    return aResult;
}

[[nodiscard]] inline IndirectExecutionResult makeExternalSingleRefResult(
    const ScRefAddress& rReference, sal_uInt16 nFileId, const OUString& rTabName)
{
    IndirectExecutionResult aResult;
    aResult.meKind = IndirectExecutionResult::Kind::ExternalSingleRef;
    aResult.maRef1 = rReference;
    aResult.mnFileId = nFileId;
    aResult.maTabName = rTabName;
    return aResult;
}

[[nodiscard]] inline IndirectExecutionResult makeExternalDoubleRefResult(
    const ScRefAddress& rReference1, const ScRefAddress& rReference2, sal_uInt16 nFileId,
    const OUString& rTabName)
{
    IndirectExecutionResult aResult;
    aResult.meKind = IndirectExecutionResult::Kind::ExternalDoubleRef;
    aResult.maRef1 = rReference1;
    aResult.maRef2 = rReference2;
    aResult.mnFileId = nFileId;
    aResult.maTabName = rTabName;
    return aResult;
}

} // namespace detail

[[nodiscard]] inline std::optional<IndirectExecutionResult> resolveIndirectReference(
    ScDocument& rDocument, const ScAddress& rPosition, const svl::SharedString& rReferenceText,
    formula::FormulaGrammar::AddressConvention eConvention, bool bTryXlA1)
{
    const OUString& rRefText = rReferenceText.getString();
    if (rRefText.isEmpty())
        return std::nullopt;

    const ScAddress::Details aDetails(eConvention, rPosition);
    const ScAddress::Details aDetailsXlA1(formula::FormulaGrammar::CONV_XL_A1, rPosition);

    const SCTAB nTab = rPosition.Tab();
    bool bTableRefNamed = false;
    sal_Int32 nTableRefNamedIndex = -1;
    OUString aTableRefNamedSymbol;

    do
    {
        ScRangeData* pRangeData
            = ScRangeStringConverter::GetRangeDataFromString(rRefText, nTab, rDocument, eConvention);
        if (!pRangeData)
            break;

        pRangeData->ValidateTabRefs();

        ScRange aRange;
        if (!pRangeData->IsReference(aRange, rPosition))
        {
            aTableRefNamedSymbol = pRangeData->GetSymbol();
            bTableRefNamed = detail::isTableStructuredRef(aTableRefNamedSymbol, nTableRefNamedIndex);
            break;
        }

        if (aRange.aStart == aRange.aEnd)
            return detail::makeSingleRefResult(aRange.aStart);
        return detail::makeDoubleRefResult(aRange);
    } while (false);

    do
    {
        if (bTableRefNamed)
            break;

        const OUString aUpperReference = rReferenceText.getIgnoreCaseString();
        const ScDBData* pDbData
            = rDocument.GetDBCollection()->getNamedDBs().findByUpperName(aUpperReference);
        if (!pDbData)
            break;

        ScRange aRange;
        pDbData->GetArea(aRange);
        if (pDbData->HasHeader())
            aRange.aStart.IncRow();
        if (pDbData->HasTotals())
            aRange.aEnd.IncRow(-1);
        if (aRange.aStart.Row() > aRange.aEnd.Row())
            break;

        if (aRange.aStart == aRange.aEnd)
            return detail::makeSingleRefResult(aRange.aStart);
        return detail::makeDoubleRefResult(aRange);
    } while (false);

    ScRefAddress aReference1;
    ScRefAddress aReference2;
    ScAddress::ExternalInfo aExternalInfo;
    if (!bTableRefNamed
        && (ConvertDoubleRef(rDocument, rRefText, nTab, aReference1, aReference2, aDetails,
                &aExternalInfo)
            || (bTryXlA1 && ConvertDoubleRef(rDocument, rRefText, nTab, aReference1, aReference2,
                               aDetailsXlA1, &aExternalInfo))))
    {
        if (aExternalInfo.mbExternal)
        {
            return detail::makeExternalDoubleRefResult(
                aReference1, aReference2, aExternalInfo.mnFileId, aExternalInfo.maTabName);
        }

        IndirectExecutionResult aResult;
        aResult.meKind = IndirectExecutionResult::Kind::DoubleRef;
        aResult.maRef1 = aReference1;
        aResult.maRef2 = aReference2;
        return aResult;
    }

    if (!bTableRefNamed
        && (ConvertSingleRef(rDocument, rRefText, nTab, aReference1, aDetails, &aExternalInfo)
            || (bTryXlA1 && ConvertSingleRef(
                               rDocument, rRefText, nTab, aReference1, aDetailsXlA1, &aExternalInfo))))
    {
        if (aExternalInfo.mbExternal)
            return detail::makeExternalSingleRefResult(
                aReference1, aExternalInfo.mnFileId, aExternalInfo.maTabName);

        IndirectExecutionResult aResult;
        aResult.meKind = IndirectExecutionResult::Kind::SingleRef;
        aResult.maRef1 = aReference1;
        return aResult;
    }

    sal_Int32 nStructuredRefIndex = bTableRefNamed ? nTableRefNamedIndex : -1;
    bool bTableRef = bTableRefNamed;
    if (!bTableRefNamed)
        bTableRef = detail::isTableStructuredRef(rRefText, nStructuredRefIndex);

    sal_Int32 nExternalNameIndex = -1;
    const bool bExternalName = !bTableRef
                               && detail::isQuotedExternalNameReference(
                                   rRefText, eConvention, bTryXlA1, nExternalNameIndex);
    if (!bExternalName && !bTableRef)
        return std::nullopt;

    ScCompiler aCompiler(rDocument, rPosition, rDocument.GetGrammar());
    aCompiler.SetRefConvention(eConvention);
    std::unique_ptr<ScTokenArray> pTokenArray(
        aCompiler.CompileString(bTableRefNamed ? aTableRefNamedSymbol : rRefText));
    if (pTokenArray->GetCodeError() != FormulaError::NONE || !pTokenArray->GetLen())
        return std::nullopt;

    if (bExternalName)
    {
        const formula::FormulaToken* pToken = pTokenArray->FirstToken();
        if (!pToken || pToken->GetType() != formula::svExternalName)
            return std::nullopt;
    }
    else if (!pTokenArray->HasOpCode(ocTableRef))
    {
        return std::nullopt;
    }

    aCompiler.CompileTokenArray();
    if (pTokenArray->GetCodeLen() != 1)
        return std::nullopt;

    ScTokenRef xToken(pTokenArray->FirstRPNToken());
    if (!xToken)
        return std::nullopt;

    switch (xToken->GetType())
    {
        case formula::svSingleRef:
        case formula::svDoubleRef:
        case formula::svExternalSingleRef:
        case formula::svExternalDoubleRef:
        case formula::svError:
        {
            IndirectExecutionResult aResult;
            aResult.meKind = IndirectExecutionResult::Kind::Token;
            aResult.mxToken = xToken;
            return aResult;
        }
        default:
            break;
    }

    return std::nullopt;
}

} // namespace spreadsheetengine::compat::libreoffice::indirectexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
