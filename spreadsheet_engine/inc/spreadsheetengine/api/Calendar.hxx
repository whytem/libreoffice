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

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/DateTimeWeek.hxx>

namespace spreadsheetengine::api::calendar
{

inline api::ValueResult<double> makeDateSerial(
    const api::DateParts& rNullDate, sal_Int16 nYear, sal_Int16 nMonth, sal_Int16 nDay, bool bStrict)
{
    if (auto oValue
        = spreadsheetengine::core::datetime::makeDateSerial(rNullDate, nYear, nMonth, nDay, bStrict))
    {
        return api::ValueResult<double>::success(*oValue);
    }
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline double yearFromSerial(const api::DateParts& rNullDate, api::DateSerial nDays)
{
    return spreadsheetengine::core::datetime::extractYear(rNullDate, nDays);
}

inline double monthFromSerial(const api::DateParts& rNullDate, api::DateSerial nDays)
{
    return spreadsheetengine::core::datetime::extractMonth(rNullDate, nDays);
}

inline api::ValueResult<double> dayFromSerial(const api::DateParts& rNullDate, api::DateSerial nDays)
{
    if (auto oValue = spreadsheetengine::core::datetime::extractDay(rNullDate, nDays))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline double hourFromTimeValue(double fTimeValue)
{
    return spreadsheetengine::core::datetime::extractHour(fTimeValue);
}

inline double minuteFromTimeValue(double fTimeValue)
{
    return spreadsheetengine::core::datetime::extractMinute(fTimeValue);
}

inline double secondFromTimeValue(double fTimeValue)
{
    return spreadsheetengine::core::datetime::extractSecond(fTimeValue);
}

inline api::ValueResult<double> makeTimeSerial(double fHour, double fMinute, double fSecond)
{
    if (auto oValue = spreadsheetengine::core::datetime::makeTimeSerial(fHour, fMinute, fSecond))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> easterSundaySerial(const api::DateParts& rNullDate, sal_Int16 nYear)
{
    if (auto oValue = spreadsheetengine::core::datetime::computeEasterSundaySerial(rNullDate, nYear))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline double diffDate(double fDate1, double fDate2)
{
    return spreadsheetengine::core::datetime::computeDiffDate(fDate1, fDate2);
}

inline double diffDate360(
    const api::DateParts& rNullDate, api::DateSerial nDate1, api::DateSerial nDate2, bool bEuropeanMethod)
{
    return spreadsheetengine::core::datetime::computeDiffDate360(
        rNullDate, nDate1, nDate2, bEuropeanMethod);
}

inline api::ValueResult<double> dateDif(const api::DateParts& rNullDate,
    api::DateSerial nDate1, api::DateSerial nDate2, api::StringView rInterval)
{
    if (auto oValue = spreadsheetengine::core::datetime::computeDateDif(rNullDate, nDate1, nDate2, rInterval))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<int> dayOfWeek(
    const api::DateParts& rNullDate, api::DateSerial nDays, sal_Int16 nFlag)
{
    const auto aResult = spreadsheetengine::core::datetime::computeDayOfWeek(rNullDate, nDays, nFlag);
    if (aResult.mbValid)
        return api::ValueResult<int>::success(aResult.mnValue);
    return api::ValueResult<int>::failure(api::Error::IllegalArgument);
}

inline int weeknumOOo(const api::DateParts& rNullDate, api::DateSerial nDays, sal_Int16 nFlag)
{
    return spreadsheetengine::core::datetime::computeWeeknumOOo(rNullDate, nDays, nFlag);
}

inline api::ValueResult<int> weekOfYear(
    const api::DateParts& rNullDate, api::DateSerial nDays, sal_Int16 nFlag)
{
    if (auto oValue = spreadsheetengine::core::datetime::computeWeekOfYear(rNullDate, nDays, nFlag))
        return api::ValueResult<int>::success(*oValue);
    return api::ValueResult<int>::failure(api::Error::IllegalArgument);
}

inline int isoWeekOfYear(const api::DateParts& rNullDate, api::DateSerial nDays)
{
    return spreadsheetengine::core::datetime::computeIsoWeekOfYear(rNullDate, nDays);
}

} // namespace spreadsheetengine::api::calendar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
