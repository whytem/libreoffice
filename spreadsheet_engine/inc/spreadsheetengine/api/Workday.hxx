/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/runtime/DateTimeWorkday.hxx>

namespace spreadsheetengine::api::workday
{

inline api::WeekendMask defaultWeekendMask()
{
    api::WeekendMask aMask {};
    spreadsheetengine::core::datetime::setDefaultWeekendMask(aMask);
    return aMask;
}

inline api::ValueResult<api::WeekendMask> weekendMaskFromSequence(
    const std::vector<double>& rWeekendDays)
{
    api::WeekendMask aMask {};
    if (spreadsheetengine::core::datetime::applyWeekendMaskSequence(rWeekendDays, aMask))
        return api::ValueResult<api::WeekendMask>::success(aMask);
    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<api::WeekendMask> weekendMaskFromMsSpec(
    api::StringView rWeekendDays, bool bWorkdayFunction)
{
    api::WeekendMask aMask {};
    if (spreadsheetengine::core::datetime::applyWeekendMaskMsSpec(
            rWeekendDays, bWorkdayFunction, aMask))
    {
        return api::ValueResult<api::WeekendMask>::success(aMask);
    }
    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
}

inline bool hasAvailableWorkday(const api::WeekendMask& rWeekendMask)
{
    return spreadsheetengine::core::datetime::hasAvailableWorkday(rWeekendMask);
}

inline api::DateSerial countWorkdays(api::DateSerial nDate1, api::DateSerial nDate2,
    const std::vector<api::DateSerial>& rSortedHolidays, const api::WeekendMask& rWeekendMask)
{
    return spreadsheetengine::core::datetime::countWorkdays(
        nDate1, nDate2, rSortedHolidays, rWeekendMask);
}

inline api::DateSerial advanceWorkday(api::DateSerial nDate, api::DateSerial nDays,
    const std::vector<api::DateSerial>& rSortedHolidays, const api::WeekendMask& rWeekendMask)
{
    return spreadsheetengine::core::datetime::advanceWorkday(
        nDate, nDays, rSortedHolidays, rWeekendMask);
}

} // namespace spreadsheetengine::api::workday

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
