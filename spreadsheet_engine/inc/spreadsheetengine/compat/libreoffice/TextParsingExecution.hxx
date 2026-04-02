/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <document.hxx>
#include <interpretercontext.hxx>

#include <spreadsheetengine/api/Parsing.hxx>
#include <spreadsheetengine/api/Text.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice::textparsingexecution
{

class DirectTextParsingAdapter
{
    DocumentEvaluationHost maHost;
    bool mbEmptyStringAsZero = false;

public:
    DirectTextParsingAdapter(
        const ScDocument& rDoc, ScInterpreterContext& rContext, bool bEmptyStringAsZero = false)
        : maHost(rDoc, rContext)
        , mbEmptyStringAsZero(bEmptyStringAsZero)
    {
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateValue(
        const OUString& rInput) const
    {
        return spreadsheetengine::api::parsing::valueFromText(maHost, toApiString(rInput));
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateDateValue(
        const OUString& rInput) const
    {
        return spreadsheetengine::api::parsing::dateValueFromText(maHost, toApiString(rInput));
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateTimeValue(
        const OUString& rInput) const
    {
        return spreadsheetengine::api::parsing::timeValueFromText(maHost, toApiString(rInput));
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateNumberValue(
        const OUString& rInput, const std::optional<OUString>& roDecimalSeparator,
        const std::optional<OUString>& roGroupSeparator) const
    {
        return spreadsheetengine::api::text::parseNumberValue(toApiString(rInput),
            roDecimalSeparator ? std::optional(toApiString(*roDecimalSeparator)) : std::nullopt,
            roGroupSeparator ? std::optional(toApiString(*roGroupSeparator)) : std::nullopt,
            mbEmptyStringAsZero);
    }
};

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> evaluateValue(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const OUString& rInput)
{
    return DirectTextParsingAdapter(rDoc, rContext).evaluateValue(rInput);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> evaluateDateValue(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const OUString& rInput)
{
    return DirectTextParsingAdapter(rDoc, rContext).evaluateDateValue(rInput);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> evaluateTimeValue(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const OUString& rInput)
{
    return DirectTextParsingAdapter(rDoc, rContext).evaluateTimeValue(rInput);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> evaluateNumberValue(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const OUString& rInput,
    const std::optional<OUString>& roDecimalSeparator,
    const std::optional<OUString>& roGroupSeparator, bool bEmptyStringAsZero = false)
{
    return DirectTextParsingAdapter(rDoc, rContext, bEmptyStringAsZero)
        .evaluateNumberValue(rInput, roDecimalSeparator, roGroupSeparator);
}

} // namespace spreadsheetengine::compat::libreoffice::textparsingexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
