/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cellvalue.hxx>
#include <compiler.hxx>
#include <document.hxx>
#include <formulacell.hxx>
#include <global.hxx>
#include <interpretercontext.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/runtime/DateTimeParse.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::NumberParseResult::Kind toApiParseKind(SvNumFormatType eType)
{
    const auto eMaskedType = eType & ~SvNumFormatType::DEFINED;
    if (eMaskedType == SvNumFormatType::DATE)
        return spreadsheetengine::api::NumberParseResult::Kind::Date;
    if (eMaskedType == SvNumFormatType::TIME)
        return spreadsheetengine::api::NumberParseResult::Kind::Time;
    if (eMaskedType == SvNumFormatType::DATETIME)
        return spreadsheetengine::api::NumberParseResult::Kind::DateTime;
    return spreadsheetengine::api::NumberParseResult::Kind::Number;
}

enum class HostCellStringKind : std::uint8_t
{
    Raw,
    Display
};

[[nodiscard]] inline std::optional<spreadsheetengine::api::Error>
tryParseHostImportedHybridFormulaErrorText(const ScDocument& rDoc, const ScAddress& rAddress,
    std::u16string_view aRawString)
{
    if (aRawString.empty())
        return std::nullopt;

    const auto tryParseDetailedError = [&]() -> std::optional<spreadsheetengine::api::Error> {
        sal_uInt16 nErrorValue = 0;
        bool bSawDigit = false;
        for (sal_Unicode cChar : aRawString)
        {
            if (cChar < u'0' || cChar > u'9')
                continue;
            bSawDigit = true;
            nErrorValue = static_cast<sal_uInt16>(nErrorValue * 10 + (cChar - u'0'));
        }

        if (!bSawDigit)
            return std::nullopt;

        const FormulaError eError = static_cast<FormulaError>(nErrorValue);
        if (!isPublishedFormulaError(eError) && eError != FormulaError::NotAvailable)
            return std::nullopt;

        return toApiError(eError);
    };

    if (const auto oDetailedError = tryParseDetailedError())
        return *oDetailedError;

    const OUString aErrorText(aRawString.data(), aRawString.size());
    ScCompiler aCompiler(
        const_cast<ScDocument&>(rDoc), rAddress, formula::FormulaGrammar::GRAM_ODFF);
    const FormulaError eError = aCompiler.GetErrorConstant(aErrorText);
    if (eError == FormulaError::NONE)
    {
        for (FormulaError eCandidate : { FormulaError::NoCode, FormulaError::DivisionByZero,
                 FormulaError::NoValue, FormulaError::NoRef, FormulaError::NoName,
                 FormulaError::IllegalFPOperation, FormulaError::NotAvailable })
        {
            if (aErrorText == ScGlobal::GetErrorString(eCandidate))
                return toApiError(eCandidate);
        }
        return std::nullopt;
    }

    return toApiError(eError);
}

[[nodiscard]] inline std::optional<spreadsheetengine::api::CellValue>
tryReadHostCachedFormulaCellValue(const ScDocument& rDoc, const ScAddress& rAddress,
    const ScFormulaCell& rFormula)
{
    const auto convertStoredStringValue = [&](const OUString& rString)
        -> spreadsheetengine::api::CellValue {
        if (const auto oError = tryParseHostImportedHybridFormulaErrorText(
                rDoc, rAddress,
                std::u16string_view(rString.getStr(), rString.getLength())))
        {
            return spreadsheetengine::api::CellValue::error(*oError);
        }

        if (const auto oParsed = spreadsheetengine::core::datetime::parseStandaloneNumberText(
                toApiString(rString)))
        {
            return spreadsheetengine::api::CellValue::number(oParsed->mfValue);
        }

        const OUString aUpperString = rString.toAsciiUpperCase();
        if (aUpperString == "TRUE")
            return spreadsheetengine::api::CellValue::boolean(true);
        if (aUpperString == "FALSE")
            return spreadsheetengine::api::CellValue::boolean(false);

        return spreadsheetengine::api::CellValue::text(toApiString(rString));
    };

    const auto convertStoredResultValue = [&](const sc::FormulaResultValue& rResult)
        -> std::optional<spreadsheetengine::api::CellValue> {
        switch (rResult.meType)
        {
            case sc::FormulaResultValue::Value:
                return spreadsheetengine::api::CellValue::number(rResult.mfValue);
            case sc::FormulaResultValue::Error:
                return spreadsheetengine::api::CellValue::error(toApiError(rResult.mnError));
            case sc::FormulaResultValue::String:
                return convertStoredStringValue(rResult.maString.getString());
            case sc::FormulaResultValue::Invalid:
                break;
        }

        return std::nullopt;
    };

    const bool bImportedCachedFormula = !rFormula.GetHybridFormula().isEmpty()
                                        || const_cast<ScFormulaCell&>(rFormula)
                                               .IsEmptyDisplayedAsString();
    const bool bAllowDirtyHybridResult = rFormula.HasHybridStringResult();
    if (rFormula.NeedsInterpret() && !bAllowDirtyHybridResult)
    {
        if (bImportedCachedFormula)
        {
            if (const auto oStoredResult = convertStoredResultValue(rFormula.GetResult()))
                return *oStoredResult;
        }
        return std::nullopt;
    }

    if (rFormula.HasHybridStringResult())
    {
        return convertStoredStringValue(rFormula.GetResultString().getString());
    }

    if (rFormula.NeedsInterpret())
        return std::nullopt;

    const sc::FormulaResultValue aResult = rFormula.GetResult();
    return convertStoredResultValue(aResult);
}

[[nodiscard]] inline spreadsheetengine::api::CellValue readHostDocumentCellValue(
    const ScDocument& rDoc, const ScAddress& rAddress, const ScRefCellValue& rCell,
    HostCellStringKind eStringKind = HostCellStringKind::Raw)
{
    if (rCell.isEmpty())
        return spreadsheetengine::api::CellValue::empty();

    if (rCell.getType() == CELLTYPE_FORMULA)
    {
        ScFormulaCell* pFormula = rCell.getFormula();
        if (!pFormula)
            return spreadsheetengine::api::CellValue::empty();

        if (const auto oCachedValue = tryReadHostCachedFormulaCellValue(rDoc, rAddress, *pFormula))
            return *oCachedValue;

        if (const FormulaError eError = pFormula->GetErrCode(); eError != FormulaError::NONE)
            return spreadsheetengine::api::CellValue::error(toApiError(eError));

        if (pFormula->IsValue())
            return spreadsheetengine::api::CellValue::number(pFormula->GetRawValue());

        const OUString aString = eStringKind == HostCellStringKind::Display
                                     ? pFormula->GetString().getString()
                                     : pFormula->GetRawString().getString();
        return spreadsheetengine::api::CellValue::text(toApiString(aString));
    }

    if (rCell.hasError())
        return spreadsheetengine::api::CellValue::error(toApiError(rDoc.GetErrCode(rAddress)));

    if (rCell.hasNumeric())
        return spreadsheetengine::api::CellValue::number(rCell.getRawValue());

    if (rCell.hasString())
    {
        const OUString aString = eStringKind == HostCellStringKind::Display
                                     ? rCell.getString(rDoc)
                                     : rCell.getRawString(rDoc);
        return spreadsheetengine::api::CellValue::text(toApiString(aString));
    }

    return spreadsheetengine::api::CellValue::empty();
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>
readHostDocumentCellValue(
    const ScDocument& rDoc, const ScAddress& rAddress,
    HostCellStringKind eStringKind = HostCellStringKind::Raw)
{
    if (!rDoc.ValidAddress(rAddress) || !rDoc.HasTable(rAddress.Tab()))
    {
        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::failure(
            spreadsheetengine::api::Error::IllegalArgument);
    }

    ScRefCellValue aCell(const_cast<ScDocument&>(rDoc), rAddress);
    return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
        readHostDocumentCellValue(rDoc, rAddress, aCell, eStringKind));
}

class DocumentEvaluationHost final : public spreadsheetengine::api::EvaluationHost
{
    const ScDocument& mrDoc;
    ScInterpreterContext* mpContext;
    spreadsheetengine::api::String maLocaleTag;

public:
    explicit DocumentEvaluationHost(
        const ScDocument& rDoc, spreadsheetengine::api::StringView rLocaleTag = {})
        : mrDoc(rDoc)
        , mpContext(nullptr)
        , maLocaleTag(rLocaleTag)
    {
    }

    explicit DocumentEvaluationHost(const ScDocument& rDoc, ScInterpreterContext& rContext,
        spreadsheetengine::api::StringView rLocaleTag = {})
        : mrDoc(rDoc)
        , mpContext(&rContext)
        , maLocaleTag(rLocaleTag)
    {
    }

    [[nodiscard]] bool hasSheet(spreadsheetengine::api::SheetId nSheet) const override
    {
        return mrDoc.HasTable(nSheet);
    }

    [[nodiscard]] sal_Int32 sheetCount() const override { return mrDoc.GetTableCount(); }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::String> getSheetName(
        spreadsheetengine::api::SheetId nSheet) const override
    {
        OUString aName;
        if (!mrDoc.GetName(nSheet, aName))
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::String>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
        }

        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::String>::success(
            toApiString(aName));
    }

    [[nodiscard]] spreadsheetengine::api::DateParts getNullDate() const override
    {
        return toApiDateParts(mrDoc.GetFormatTable()->GetNullDate());
    }

    [[nodiscard]] spreadsheetengine::api::String getLocaleTag() const override
    {
        return maLocaleTag;
    }

    [[nodiscard]] spreadsheetengine::api::query::SearchType getSearchType() const override
    {
        const ScDocOptions& rOptions = mrDoc.GetDocOptions();
        if (rOptions.IsFormulaRegexEnabled())
            return spreadsheetengine::api::query::SearchType::Regex;
        if (rOptions.IsFormulaWildcardsEnabled())
            return spreadsheetengine::api::query::SearchType::Wildcard;
        return spreadsheetengine::api::query::SearchType::Normal;
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>
    getCellValue(const spreadsheetengine::api::CellAddress& rAddress) const override
    {
        return spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(
            mrDoc, toLibreOfficeAddress(rAddress));
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>
    getRangeValue(const spreadsheetengine::api::CellRange& rRange,
        spreadsheetengine::api::ColumnIndex nColumnOffset,
        spreadsheetengine::api::RowIndex nRowOffset) const override
    {
        if (!rRange.isNormalized() || !rRange.containsOffset(nColumnOffset, nRowOffset))
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
        }

        return spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(mrDoc,
            ScAddress(rRange.maStart.mnColumn + nColumnOffset, rRange.maStart.mnRow + nRowOffset,
                rRange.maStart.mnSheet));
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::ResolvedReference>
    resolveReference(const spreadsheetengine::api::CellRange& rRange) const override
    {
        if (!rRange.isNormalized() || !mrDoc.HasTable(rRange.maStart.mnSheet))
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::ResolvedReference>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
        }

        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::ResolvedReference>::success(
            { rRange });
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::NumberParseResult>
    parseNumber(spreadsheetengine::api::StringView rValue,
        spreadsheetengine::api::NumberParseMode eMode = spreadsheetengine::api::NumberParseMode::General) const override
    {
        sal_uInt32 nFormat = 0;
        double fValue = 0.0;
        const OUString aString = toLibreOfficeString(spreadsheetengine::api::String(rValue));
        const SvNumInputOptions eInputOptions = eMode == spreadsheetengine::api::NumberParseMode::LaxTime
                                                    ? SvNumInputOptions::LAX_TIME
                                                    : SvNumInputOptions::NONE;
        const bool bParsed = mpContext
                                 ? mpContext->NFIsNumberFormat(aString, nFormat, fValue, eInputOptions)
                                 : mrDoc.GetFormatTable()->IsNumberFormat(
                                       aString, nFormat, fValue, eInputOptions);
        if (!bParsed)
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::NumberParseResult>::failure(
                spreadsheetengine::api::Error::NoValue);
        }

        const SvNumFormatType eType
            = mpContext ? mpContext->NFGetType(nFormat) : mrDoc.GetFormatTable()->GetType(nFormat);
        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::NumberParseResult>::success(
            { fValue, nFormat, toApiParseKind(eType) });
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::String> formatNumber(
        double fValue, spreadsheetengine::api::FormatIndex nFormat = 0) const override
    {
        const OUString aFormatted = mpContext
                                        ? mpContext->NFGetInputLineString(fValue, nFormat)
                                        : mrDoc.GetFormatTable()->GetInputLineString(fValue, nFormat);
        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::String>::success(
            toApiString(aFormatted));
    }
};

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
