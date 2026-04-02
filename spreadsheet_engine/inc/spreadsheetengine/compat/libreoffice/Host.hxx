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
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

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

[[nodiscard]] inline spreadsheetengine::api::CellValue readHostDocumentCellValue(
    const ScDocument& rDoc, const ScAddress& rAddress, const ScRefCellValue& rCell,
    HostCellStringKind eStringKind = HostCellStringKind::Raw)
{
    if (rCell.isEmpty())
        return spreadsheetengine::api::CellValue::empty();

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
