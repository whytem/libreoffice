/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::core::host
{

[[nodiscard]] inline api::ValueResult<api::CellValueView> readValueView(
    const api::EvaluationHost& rHost, const api::CellRange& rRange)
{
    if (!rRange.isNormalized())
        return api::ValueResult<api::CellValueView>::failure(api::Error::IllegalArgument);

    if (rRange.isSingleCell())
    {
        const auto aValue = rHost.getCellValue(rRange.maStart);
        if (!aValue)
            return api::ValueResult<api::CellValueView>::failure(aValue.meError);

        return api::ValueResult<api::CellValueView>::success(
            api::CellValueView::scalar(aValue.maValue));
    }

    const auto aReference = rHost.resolveReference(rRange);
    if (!aReference)
        return api::ValueResult<api::CellValueView>::failure(aReference.meError);

    return api::ValueResult<api::CellValueView>::success(
        api::CellValueView::matrixReference(aReference.maValue));
}

[[nodiscard]] inline api::ValueResult<api::CellValue> readValueViewElement(
    const api::EvaluationHost& rHost, const api::CellValueView& rView,
    api::ColumnIndex nColumnOffset = 0, api::RowIndex nRowOffset = 0)
{
    if (rView.isScalar())
    {
        if (nColumnOffset != 0 || nRowOffset != 0)
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);

        return api::ValueResult<api::CellValue>::success(rView.maValue);
    }

    if (!rView.maReference.containsOffset(nColumnOffset, nRowOffset))
        return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);

    return rHost.getRangeValue(rView.maReference.maRange, nColumnOffset, nRowOffset);
}

[[nodiscard]] inline api::ValueResult<api::NumberParseResult> coerceToNumber(
    const api::EvaluationHost& rHost, const api::CellValue& rValue)
{
    if (rValue.isNumber() || rValue.isBoolean())
        return api::ValueResult<api::NumberParseResult>::success({ rValue.mfNumber, 0 });

    if (rValue.isText())
        return rHost.parseNumber(rValue.maString);

    if (rValue.isError())
        return api::ValueResult<api::NumberParseResult>::failure(rValue.meError);

    return api::ValueResult<api::NumberParseResult>::failure(api::Error::NoValue);
}

[[nodiscard]] inline api::ValueResult<api::NumberParseResult> coerceValueViewElementToNumber(
    const api::EvaluationHost& rHost, const api::CellValueView& rView,
    api::ColumnIndex nColumnOffset = 0, api::RowIndex nRowOffset = 0)
{
    const auto aValue = readValueViewElement(rHost, rView, nColumnOffset, nRowOffset);
    if (!aValue)
        return api::ValueResult<api::NumberParseResult>::failure(aValue.meError);

    return coerceToNumber(rHost, aValue.maValue);
}

[[nodiscard]] inline api::ValueResult<api::String> formatValue(
    const api::EvaluationHost& rHost, const api::CellValue& rValue, api::FormatIndex nFormat = 0)
{
    if (rValue.isNumber() || rValue.isBoolean())
        return rHost.formatNumber(rValue.mfNumber, nFormat);

    if (rValue.isText())
        return api::ValueResult<api::String>::success(rValue.maString);

    if (rValue.isError())
        return api::ValueResult<api::String>::failure(rValue.meError);

    return api::ValueResult<api::String>::success({});
}

} // namespace spreadsheetengine::core::host

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
