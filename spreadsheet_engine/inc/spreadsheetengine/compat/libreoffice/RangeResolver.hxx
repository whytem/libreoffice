/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>

#include <address.hxx>
#include <compiler.hxx>
#include <dbdata.hxx>
#include <document.hxx>
#include <formula/grammar.hxx>
#include <global.hxx>
#include <rangenam.hxx>
#include <rangeutl.hxx>
#include <tokenarray.hxx>

#include <spreadsheetengine/api/RangeResolver.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/Grammar.hxx>
#include <spreadsheetengine/compat/libreoffice/IndirectExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice
{

namespace detail
{

[[nodiscard]] inline OUString normalizeRangeResolverToken(api::StringView rValue)
{
    OUString aText = toLibreOfficeString(api::String(rValue));
    if (!aText.isEmpty() && aText[0] == '.')
        aText = aText.copy(1);

    OUString aExternalNameCandidate = aText;
    if (aExternalNameCandidate.getLength() >= 2 && aExternalNameCandidate.startsWith(u"[")
        && aExternalNameCandidate.endsWith(u"]"))
    {
        aExternalNameCandidate
            = aExternalNameCandidate.copy(1, aExternalNameCandidate.getLength() - 2);
    }
    const sal_Int32 nNameMarker = aExternalNameCandidate.indexOf(u"#$$'"_ustr);
    if (nNameMarker >= 0 && aExternalNameCandidate.endsWith(u"'"))
    {
        OUString aName = aExternalNameCandidate.copy(
            nNameMarker + 4, aExternalNameCandidate.getLength() - (nNameMarker + 5));
        aName = aName.replaceAll(u"''"_ustr, u"'"_ustr);
        return aExternalNameCandidate.copy(0, nNameMarker + 1) + aName;
    }

    return aText;
}

[[nodiscard]] inline api::CellAddress toApiCellAddress(const ScAddress& rAddress)
{
    return { static_cast<api::SheetId>(rAddress.Tab()),
             static_cast<api::ColumnIndex>(rAddress.Col()),
             static_cast<api::RowIndex>(rAddress.Row()) };
}

[[nodiscard]] inline api::CellAddress toApiCellAddress(const ScSingleRefData& rReference)
{
    return { static_cast<api::SheetId>(rReference.Tab()),
             static_cast<api::ColumnIndex>(rReference.Col()),
             static_cast<api::RowIndex>(rReference.Row()) };
}

[[nodiscard]] inline api::CellRange toApiCellRange(const ScRange& rRange)
{
    return { toApiCellAddress(rRange.aStart), toApiCellAddress(rRange.aEnd) };
}

[[nodiscard]] inline api::ResolvedRangeBinding makeLocalRangeBinding(const ScRange& rRange)
{
    api::ResolvedRangeBinding aBinding;
    aBinding.meKind = api::ResolvedRangeBindingKind::LocalRange;
    aBinding.maRange = toApiCellRange(rRange);
    return aBinding;
}

[[nodiscard]] inline api::ResolvedRangeBinding makeExternalRangeBinding(
    const ScRefAddress& rStart, const ScRefAddress& rEnd, sal_uInt16 nFileId, const OUString& rTabName)
{
    api::ResolvedRangeBinding aBinding;
    aBinding.meKind = api::ResolvedRangeBindingKind::ExternalRange;
    aBinding.maRange.maStart = { static_cast<api::SheetId>(rStart.Tab()),
                                 static_cast<api::ColumnIndex>(rStart.Col()),
                                 static_cast<api::RowIndex>(rStart.Row()) };
    aBinding.maRange.maEnd = { static_cast<api::SheetId>(rEnd.Tab()),
                               static_cast<api::ColumnIndex>(rEnd.Col()),
                               static_cast<api::RowIndex>(rEnd.Row()) };
    aBinding.mnFileId = nFileId;
    aBinding.maTabName = toApiString(rTabName);
    return aBinding;
}

[[nodiscard]] inline api::ResolvedRangeBinding makeExternalRangeBinding(
    const ScSingleRefData& rStart, const ScSingleRefData& rEnd, sal_uInt16 nFileId,
    const OUString& rTabName)
{
    api::ResolvedRangeBinding aBinding;
    aBinding.meKind = api::ResolvedRangeBindingKind::ExternalRange;
    aBinding.maRange.maStart = toApiCellAddress(rStart);
    aBinding.maRange.maEnd = toApiCellAddress(rEnd);
    aBinding.mnFileId = nFileId;
    aBinding.maTabName = toApiString(rTabName);
    return aBinding;
}

[[nodiscard]] inline api::ResolvedRangeBinding makeTokenBackedSymbolBinding(const OUString& rSymbol)
{
    api::ResolvedRangeBinding aBinding;
    aBinding.meKind = api::ResolvedRangeBindingKind::TokenBackedSymbol;
    aBinding.maSymbol = toApiString(rSymbol);
    return aBinding;
}

[[nodiscard]] inline std::optional<api::ResolvedRangeBinding> lookupDatabaseRangeBinding(
    const ScDocument& rDoc, const OUString& rUpperName)
{
    const ScDBData* pDbData = rDoc.GetDBCollection()->getNamedDBs().findByUpperName(rUpperName);
    if (!pDbData)
        return std::nullopt;

    ScRange aRange;
    pDbData->GetArea(aRange);
    if (pDbData->HasHeader())
        aRange.aStart.IncRow();
    if (pDbData->HasTotals())
        aRange.aEnd.IncRow(-1);
    if (aRange.aStart.Row() > aRange.aEnd.Row())
        return std::nullopt;
    return makeLocalRangeBinding(aRange);
}

[[nodiscard]] inline ScRangeData* findNamedRangeData(
    const OUString& rName, const ScDocument& rDoc, const ScAddress& rBaseAddress)
{
    if (ScRangeData* pRangeData = ScRangeStringConverter::GetRangeDataFromString(
            rName, rBaseAddress.Tab(), rDoc, formula::FormulaGrammar::CONV_OOO))
    {
        return pRangeData;
    }

    const OUString aUpperName = ScGlobal::getCharClass().uppercase(rName);
    if (ScRangeName* pLocalNames = rDoc.GetRangeName(rBaseAddress.Tab()))
    {
        if (ScRangeData* pLocalData = pLocalNames->findByUpperName(aUpperName))
            return pLocalData;
    }

    if (ScRangeName* pGlobalNames = rDoc.GetRangeName())
        return pGlobalNames->findByUpperName(aUpperName);
    return nullptr;
}

[[nodiscard]] inline std::optional<api::ResolvedRangeBinding> tryResolveCompiledSymbol(
    const ScDocument& rDoc, const ScAddress& rBaseAddress, const OUString& rSymbol,
    formula::FormulaGrammar::Grammar eGrammar)
{
    if (rSymbol.isEmpty())
        return std::nullopt;

    ScCompiler aCompiler(const_cast<ScDocument&>(rDoc), rBaseAddress, eGrammar);
    std::unique_ptr<ScTokenArray> pCode = aCompiler.CompileString(rSymbol);
    if (!pCode || pCode->GetCodeError() != FormulaError::NONE)
        return std::nullopt;

    if (pCode->GetLen() == 1)
    {
        if (const formula::FormulaToken* pToken = pCode->FirstToken();
            pToken && pToken->GetType() == formula::svExternalName)
        {
            return makeTokenBackedSymbolBinding(rSymbol);
        }
    }

    ScCompiler aRpnCompiler(const_cast<ScDocument&>(rDoc), rBaseAddress, *pCode, eGrammar);
    aRpnCompiler.CompileTokenArray();
    if (pCode->GetCodeError() != FormulaError::NONE)
        return std::nullopt;

    if (pCode->GetCodeLen() == 1)
    {
        if (formula::FormulaToken* pRpnToken = pCode->FirstRPNToken())
        {
            switch (pRpnToken->GetType())
            {
                case formula::svExternalSingleRef:
                    return makeExternalRangeBinding(
                        *pRpnToken->GetSingleRef(), *pRpnToken->GetSingleRef(),
                        pRpnToken->GetIndex(), pRpnToken->GetString().getString());
                case formula::svExternalDoubleRef:
                    return makeExternalRangeBinding(
                        pRpnToken->GetDoubleRef()->Ref1, pRpnToken->GetDoubleRef()->Ref2,
                        pRpnToken->GetIndex(), pRpnToken->GetString().getString());
                case formula::svExternalName:
                    return makeTokenBackedSymbolBinding(rSymbol);
                default:
                    break;
            }
        }
    }

    ScRange aRange;
    if (pCode->IsReference(aRange, rBaseAddress))
        return makeLocalRangeBinding(aRange);

    return std::nullopt;
}

[[nodiscard]] inline std::optional<api::ResolvedRangeBinding> resolveNamedRangeBinding(
    const ScDocument& rDoc, const ScAddress& rBaseAddress, const OUString& rName)
{
    if (ScRangeData* pRangeData = findNamedRangeData(rName, rDoc, rBaseAddress))
    {
        ScRange aRange;
        if (pRangeData->IsReference(aRange, rBaseAddress))
            return makeLocalRangeBinding(aRange);
        if (const ScTokenArray* pCode = pRangeData->GetCode(); pCode && pCode->IsReference(aRange, rBaseAddress))
            return makeLocalRangeBinding(aRange);

        const OUString aOdfSymbol = pRangeData->GetSymbol(rBaseAddress, formula::FormulaGrammar::GRAM_ODFF);
        if (const auto oResolved = tryResolveCompiledSymbol(
                rDoc, rBaseAddress, aOdfSymbol, formula::FormulaGrammar::GRAM_ODFF))
        {
            return oResolved;
        }

        const OUString aNativeSymbol
            = pRangeData->GetSymbol(rBaseAddress, formula::FormulaGrammar::GRAM_NATIVE);
        if (const auto oResolved = tryResolveCompiledSymbol(
                rDoc, rBaseAddress, aNativeSymbol, formula::FormulaGrammar::GRAM_NATIVE))
        {
            return oResolved;
        }
    }

    const OUString aUpperName = ScGlobal::getCharClass().uppercase(rName);
    if (const auto oDatabaseRange = lookupDatabaseRangeBinding(rDoc, aUpperName))
        return oDatabaseRange;

    if (const auto oResolved
        = tryResolveCompiledSymbol(rDoc, rBaseAddress, rName, formula::FormulaGrammar::GRAM_ODFF))
    {
        return oResolved;
    }
    return tryResolveCompiledSymbol(rDoc, rBaseAddress, rName, formula::FormulaGrammar::GRAM_NATIVE);
}

} // namespace detail

class DocumentRangeResolver final : public api::RangeResolver
{
    ScDocument& mrDoc;

public:
    explicit DocumentRangeResolver(ScDocument& rDoc)
        : mrDoc(rDoc)
    {
    }

    [[nodiscard]] api::ValueResult<api::ResolvedRangeBinding> resolveRange(
        const api::RangeResolutionRequest& rRequest) const override
    {
        const ScAddress aBaseAddress(static_cast<SCCOL>(rRequest.maBaseAddress.mnColumn),
            static_cast<SCROW>(rRequest.maBaseAddress.mnRow),
            static_cast<SCTAB>(rRequest.maBaseAddress.mnSheet));

        switch (rRequest.meKind)
        {
            case api::RangeResolutionKind::DirectReference:
            {
                const OUString aPrimary = detail::normalizeRangeResolverToken(rRequest.maPrimaryText);
                const auto eConvention = toLibreOfficeAddressConvention(rRequest.meConvention);
                const ScAddress::Details aDetails(eConvention, aBaseAddress);

                if (rRequest.maSecondaryText.empty())
                {
                    ScRefAddress aReference;
                    ScAddress::ExternalInfo aExternalInfo;
                    if (!ConvertSingleRef(mrDoc, aPrimary, aBaseAddress.Tab(), aReference, aDetails,
                            &aExternalInfo))
                    {
                        return api::ValueResult<api::ResolvedRangeBinding>::failure(
                            api::Error::IllegalArgument);
                    }

                    if (aExternalInfo.mbExternal)
                    {
                        return api::ValueResult<api::ResolvedRangeBinding>::success(
                            detail::makeExternalRangeBinding(aReference, aReference,
                                aExternalInfo.mnFileId, aExternalInfo.maTabName));
                    }

                    return api::ValueResult<api::ResolvedRangeBinding>::success(
                        detail::makeLocalRangeBinding(ScRange(aReference.GetAddress(),
                            aReference.GetAddress())));
                }

                OUString aRangeText = aPrimary;
                aRangeText += u":"_ustr;
                aRangeText += detail::normalizeRangeResolverToken(rRequest.maSecondaryText);

                ScRefAddress aStart;
                ScRefAddress aEnd;
                ScAddress::ExternalInfo aExternalInfo;
                if (!ConvertDoubleRef(mrDoc, aRangeText, aBaseAddress.Tab(), aStart, aEnd, aDetails,
                        &aExternalInfo))
                {
                    return api::ValueResult<api::ResolvedRangeBinding>::failure(
                        api::Error::IllegalArgument);
                }

                if (aExternalInfo.mbExternal)
                {
                    return api::ValueResult<api::ResolvedRangeBinding>::success(
                        detail::makeExternalRangeBinding(aStart, aEnd, aExternalInfo.mnFileId,
                            aExternalInfo.maTabName));
                }

                return api::ValueResult<api::ResolvedRangeBinding>::success(
                    detail::makeLocalRangeBinding(ScRange(aStart.GetAddress(), aEnd.GetAddress())));
            }
            case api::RangeResolutionKind::NamedReference:
            {
                const OUString aName = detail::normalizeRangeResolverToken(rRequest.maPrimaryText);
                if (const auto oResolved
                    = detail::resolveNamedRangeBinding(mrDoc, aBaseAddress, aName))
                {
                    return api::ValueResult<api::ResolvedRangeBinding>::success(*oResolved);
                }

                return api::ValueResult<api::ResolvedRangeBinding>::failure(api::Error::NoName);
            }
            case api::RangeResolutionKind::IndirectText:
            {
                const OUString aReferenceText
                    = detail::normalizeRangeResolverToken(rRequest.maPrimaryText);
                const auto oResolved = indirectexecution::resolveIndirectReference(
                    mrDoc, aBaseAddress, svl::SharedString(aReferenceText),
                    toLibreOfficeAddressConvention(rRequest.meConvention), rRequest.mbTryXlA1);
                if (!oResolved)
                {
                    return api::ValueResult<api::ResolvedRangeBinding>::failure(
                        api::Error::IllegalArgument);
                }

                switch (oResolved->meKind)
                {
                    case indirectexecution::IndirectExecutionResult::Kind::SingleRef:
                        return api::ValueResult<api::ResolvedRangeBinding>::success(
                            detail::makeLocalRangeBinding(
                                ScRange(oResolved->maRef1.GetAddress(), oResolved->maRef1.GetAddress())));
                    case indirectexecution::IndirectExecutionResult::Kind::DoubleRef:
                        return api::ValueResult<api::ResolvedRangeBinding>::success(
                            detail::makeLocalRangeBinding(
                                ScRange(oResolved->maRef1.GetAddress(), oResolved->maRef2.GetAddress())));
                    case indirectexecution::IndirectExecutionResult::Kind::ExternalSingleRef:
                        return api::ValueResult<api::ResolvedRangeBinding>::success(
                            detail::makeExternalRangeBinding(oResolved->maRef1, oResolved->maRef1,
                                oResolved->mnFileId, oResolved->maTabName));
                    case indirectexecution::IndirectExecutionResult::Kind::ExternalDoubleRef:
                        return api::ValueResult<api::ResolvedRangeBinding>::success(
                            detail::makeExternalRangeBinding(oResolved->maRef1, oResolved->maRef2,
                                oResolved->mnFileId, oResolved->maTabName));
                    case indirectexecution::IndirectExecutionResult::Kind::Token:
                        return api::ValueResult<api::ResolvedRangeBinding>::success(
                            detail::makeTokenBackedSymbolBinding(aReferenceText));
                }
                break;
            }
        }

        return api::ValueResult<api::ResolvedRangeBinding>::failure(api::Error::IllegalArgument);
    }
};

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
