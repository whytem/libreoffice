/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/DateTimeWorkday.hxx>
#include <cstdint>

#include <algorithm>
#include <cstddef>

namespace spreadsheetengine::core::datetime
{

namespace
{

constexpr std::size_t MONDAY_INDEX = 0;
constexpr std::size_t TUESDAY_INDEX = 1;
constexpr std::size_t WEDNESDAY_INDEX = 2;
constexpr std::size_t THURSDAY_INDEX = 3;
constexpr std::size_t FRIDAY_INDEX = 4;
constexpr std::size_t SATURDAY_INDEX = 5;
constexpr std::size_t SUNDAY_INDEX = 6;

std::size_t getNormalizedDayOfWeek(spreadsheetengine::api::DateSerial nDate)
{
    auto nDay = static_cast<int>((nDate - 1) % 7);
    if (nDay < 0)
        nDay += 7;
    return static_cast<std::size_t>(nDay);
}

}

void setDefaultWeekendMask(spreadsheetengine::api::WeekendMask& rWeekendMask)
{
    rWeekendMask.fill(false);
    rWeekendMask[SATURDAY_INDEX] = true;
    rWeekendMask[SUNDAY_INDEX] = true;
}

bool applyWeekendMaskSequence(
    const std::vector<double>& rWeekendDays, spreadsheetengine::api::WeekendMask& rWeekendMask)
{
    if (rWeekendDays.size() != 7)
        return false;

    for (std::size_t i = 0; i < rWeekendMask.size(); ++i)
        rWeekendMask[i] = static_cast<bool>(rWeekendDays[(i == SUNDAY_INDEX) ? 0 : i + 1]);
    return true;
}

bool applyWeekendMaskMsSpec(spreadsheetengine::api::StringView rWeekendDays, bool bWorkdayFunction,
    spreadsheetengine::api::WeekendMask& rWeekendMask)
{
    setDefaultWeekendMask(rWeekendMask);
    if (rWeekendDays.empty())
        return true;

    if (bWorkdayFunction && rWeekendDays == u"1111111")
        return false;

    rWeekendMask.fill(false);
    switch (rWeekendDays.size())
    {
        case 1:
            switch (rWeekendDays[0])
            {
                case u'1':
                    rWeekendMask[SATURDAY_INDEX] = true;
                    rWeekendMask[SUNDAY_INDEX] = true;
                    break;
                case u'2':
                    rWeekendMask[SUNDAY_INDEX] = true;
                    rWeekendMask[MONDAY_INDEX] = true;
                    break;
                case u'3':
                    rWeekendMask[MONDAY_INDEX] = true;
                    rWeekendMask[TUESDAY_INDEX] = true;
                    break;
                case u'4':
                    rWeekendMask[TUESDAY_INDEX] = true;
                    rWeekendMask[WEDNESDAY_INDEX] = true;
                    break;
                case u'5':
                    rWeekendMask[WEDNESDAY_INDEX] = true;
                    rWeekendMask[THURSDAY_INDEX] = true;
                    break;
                case u'6':
                    rWeekendMask[THURSDAY_INDEX] = true;
                    rWeekendMask[FRIDAY_INDEX] = true;
                    break;
                case u'7':
                    rWeekendMask[FRIDAY_INDEX] = true;
                    rWeekendMask[SATURDAY_INDEX] = true;
                    break;
                default:
                    return false;
            }
            return true;
        case 2:
            if (rWeekendDays[0] != u'1')
                return false;
            switch (rWeekendDays[1])
            {
                case u'1':
                    rWeekendMask[SUNDAY_INDEX] = true;
                    break;
                case u'2':
                    rWeekendMask[MONDAY_INDEX] = true;
                    break;
                case u'3':
                    rWeekendMask[TUESDAY_INDEX] = true;
                    break;
                case u'4':
                    rWeekendMask[WEDNESDAY_INDEX] = true;
                    break;
                case u'5':
                    rWeekendMask[THURSDAY_INDEX] = true;
                    break;
                case u'6':
                    rWeekendMask[FRIDAY_INDEX] = true;
                    break;
                case u'7':
                    rWeekendMask[SATURDAY_INDEX] = true;
                    break;
                default:
                    return false;
            }
            return true;
        case 7:
            for (std::size_t i = 0; i < rWeekendMask.size(); ++i)
            {
                switch (rWeekendDays[i])
                {
                    case u'0':
                        rWeekendMask[i] = false;
                        break;
                    case u'1':
                        rWeekendMask[i] = true;
                        break;
                    default:
                        return false;
                }
            }
            return true;
        default:
            return false;
    }
}

std::int32_t countWorkdays(spreadsheetengine::api::DateSerial nDate1,
    spreadsheetengine::api::DateSerial nDate2,
    const std::vector<spreadsheetengine::api::DateSerial>& rSortedHolidays,
    const spreadsheetengine::api::WeekendMask& rWeekendMask)
{
    std::int32_t nCount = 0;
    std::size_t nRef = 0;
    const bool bReverse = nDate1 > nDate2;
    if (bReverse)
        std::swap(nDate1, nDate2);

    const std::size_t nMax = rSortedHolidays.size();
    while (nDate1 <= nDate2)
    {
        if (!rWeekendMask[getNormalizedDayOfWeek(nDate1)])
        {
            while (nRef < nMax && rSortedHolidays[nRef] < nDate1)
                ++nRef;
            if (nRef >= nMax || rSortedHolidays[nRef] != nDate1)
                ++nCount;
        }
        ++nDate1;
    }

    return bReverse ? -nCount : nCount;
}

spreadsheetengine::api::DateSerial advanceWorkday(spreadsheetengine::api::DateSerial nDate,
    spreadsheetengine::api::DateSerial nDays,
    const std::vector<spreadsheetengine::api::DateSerial>& rSortedHolidays,
    const spreadsheetengine::api::WeekendMask& rWeekendMask)
{
    if (!nDays)
        return nDate;

    const std::size_t nMax = rSortedHolidays.size();
    if (nDays > 0)
    {
        std::size_t nRef = 0;
        while (nDays)
        {
            do
            {
                ++nDate;
            } while (rWeekendMask[getNormalizedDayOfWeek(nDate)]);

            while (nRef < nMax && rSortedHolidays[nRef] < nDate)
                ++nRef;

            if (nRef >= nMax || rSortedHolidays[nRef] != nDate)
                --nDays;
        }
    }
    else
    {
        std::ptrdiff_t nRef = static_cast<std::ptrdiff_t>(nMax) - 1;
        while (nDays)
        {
            do
            {
                --nDate;
            } while (rWeekendMask[getNormalizedDayOfWeek(nDate)]);

            while (nRef >= 0 && rSortedHolidays[static_cast<std::size_t>(nRef)] > nDate)
                --nRef;

            if (nRef < 0 || rSortedHolidays[static_cast<std::size_t>(nRef)] != nDate)
                ++nDays;
        }
    }

    return nDate;
}

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
