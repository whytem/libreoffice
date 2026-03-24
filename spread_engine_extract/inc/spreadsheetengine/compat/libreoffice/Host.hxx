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
#include <formula/errorcodes.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
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

inline spreadsheetengine::api::Error toApiError(FormulaError eError)
{
    switch (eError)
    {
        case FormulaError::IllegalArgument:
            return spreadsheetengine::api::Error::IllegalArgument;
        case FormulaError::DivisionByZero:
            return spreadsheetengine::api::Error::DivisionByZero;
        case FormulaError::IllegalFPOperation:
            return spreadsheetengine::api::Error::Domain;
        case FormulaError::StringOverflow:
            return spreadsheetengine::api::Error::StringOverflow;
        case FormulaError::NoValue:
            return spreadsheetengine::api::Error::NoValue;
        case FormulaError::NoConvergence:
            return spreadsheetengine::api::Error::NoConvergence;
        case FormulaError::NONE:
        default:
            return spreadsheetengine::api::Error::None;
    }
}

class DocumentEvaluationHost final : public spreadsheetengine::api::EvaluationHost
{
    const ScDocument& mrDoc;
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
    parseNumber(spreadsheetengine::api::StringView rValue) const override
    {
        sal_uInt32 nFormat = 0;
        double fValue = 0.0;
        if (!mrDoc.GetFormatTable()->IsNumberFormat(
                toLibreOfficeString(spreadsheetengine::api::String(rValue)), nFormat, fValue))
        {
            return spreadsheetengine::api::ValueResult<spreadsheetengine::api::NumberParseResult>::failure(
                spreadsheetengine::api::Error::NoValue);
        }

        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::NumberParseResult>::success(
            { fValue, nFormat });
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::String> formatNumber(
        double fValue, spreadsheetengine::api::FormatIndex nFormat = 0) const override
    {
        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::String>::success(
            toApiString(mrDoc.GetFormatTable()->GetInputLineString(fValue, nFormat)));
    }
};

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
