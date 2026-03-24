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
#include <document.hxx>
#include <formulacell.hxx>
#include <interpretercontext.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::CellAddress toApiCellAddress(const ScAddress& rAddress)
{
    return { rAddress.Tab(), rAddress.Col(), rAddress.Row() };
}

inline ScAddress toLibreOfficeAddress(const spreadsheetengine::api::CellAddress& rAddress)
{
    return ScAddress(rAddress.mnColumn, rAddress.mnRow, rAddress.mnSheet);
}

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

class DocumentEvaluationHost final : public spreadsheetengine::api::EvaluationHost
{
    const ScDocument& mrDoc;
    ScInterpreterContext* mpContext;
    spreadsheetengine::api::String maLocaleTag;

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>
    readCellValue(const ScAddress& rAddress) const
    {
        if (!mrDoc.ValidAddress(rAddress) || !mrDoc.HasTable(rAddress.Tab()))
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
        }

        ScRefCellValue aCell(const_cast<ScDocument&>(mrDoc), rAddress);
        if (aCell.isEmpty())
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                spreadsheetengine::api::CellValue::empty());
        }

        if (aCell.hasError())
        {
            FormulaError eError = mrDoc.GetErrCode(rAddress);
            if (aCell.getType() == CELLTYPE_FORMULA && aCell.getFormula())
                eError = aCell.getFormula()->GetErrCode();

            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                spreadsheetengine::api::CellValue::error(toApiError(eError)));
        }

        if (aCell.hasNumeric())
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                spreadsheetengine::api::CellValue::number(aCell.getRawValue()));
        }

        if (aCell.hasString())
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
                spreadsheetengine::api::CellValue::text(toApiString(aCell.getRawString(mrDoc))));
        }

        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>::success(
            spreadsheetengine::api::CellValue::empty());
    }

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

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>
    getCellValue(const spreadsheetengine::api::CellAddress& rAddress) const override
    {
        return readCellValue(toLibreOfficeAddress(rAddress));
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

        return readCellValue(ScAddress(rRange.maStart.mnColumn + nColumnOffset,
            rRange.maStart.mnRow + nRowOffset, rRange.maStart.mnSheet));
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
