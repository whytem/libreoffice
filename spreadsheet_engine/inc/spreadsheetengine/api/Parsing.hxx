/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/math.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>

namespace spreadsheetengine::api::parsing
{

inline bool isDateLike(api::NumberParseResult::Kind eKind)
{
    return eKind == api::NumberParseResult::Kind::Date
           || eKind == api::NumberParseResult::Kind::DateTime;
}

inline bool isTimeLike(api::NumberParseResult::Kind eKind)
{
    return eKind == api::NumberParseResult::Kind::Time
           || eKind == api::NumberParseResult::Kind::DateTime;
}

inline api::ValueResult<double> valueFromText(
    const api::EvaluationHost& rHost, api::StringView rInput)
{
    const auto aParsed = rHost.parseNumber(rInput);
    if (!aParsed)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return api::ValueResult<double>::success(aParsed.maValue.mfValue);
}

inline api::ValueResult<double> dateValueFromText(
    const api::EvaluationHost& rHost, api::StringView rInput)
{
    const auto aParsed = rHost.parseNumber(rInput);
    if (!aParsed || !isDateLike(aParsed.maValue.meKind))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return api::ValueResult<double>::success(rtl::math::approxFloor(aParsed.maValue.mfValue));
}

inline api::ValueResult<double> timeValueFromText(
    const api::EvaluationHost& rHost, api::StringView rInput)
{
    const auto aParsed = rHost.parseNumber(rInput, api::NumberParseMode::LaxTime);
    if (!aParsed || !isTimeLike(aParsed.maValue.meKind))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return api::ValueResult<double>::success(
        spreadsheetengine::core::datetime::normalizeTimeFraction(aParsed.maValue.mfValue));
}

} // namespace spreadsheetengine::api::parsing

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
