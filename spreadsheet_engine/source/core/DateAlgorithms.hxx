/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Date.hxx>

namespace spreadsheetengine::core::detail::date
{

constexpr std::int16_t kYearMax = std::numeric_limits<std::int16_t>::max();
constexpr std::int16_t kYearMin = std::numeric_limits<std::int16_t>::min();

constexpr std::int32_t yearToDays(std::int16_t nYear)
{
    assert(nYear != 0);
    auto val = [](int off, int y) { return off + y * 365 + y / 4 - y / 100 + y / 400; };
    return nYear < 0 ? val(-366, nYear + 1) : val(0, nYear - 1);
}

constexpr bool isLeapYear(std::int16_t nYear)
{
    assert(nYear != 0);
    if (nYear < 0)
        nYear = -nYear - 1;
    return (((nYear % 4) == 0) && ((nYear % 100) != 0)) || ((nYear % 400) == 0);
}

constexpr std::uint16_t getDaysInMonth(std::uint16_t nMonth, std::int16_t nYear)
{
    assert(1 <= nMonth && nMonth <= 12);
    if (nMonth < 1 || 12 < nMonth)
        return 0;

    constexpr std::uint16_t aDaysInMonth[12]
        = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    std::uint16_t nDays = aDaysInMonth[nMonth - 1];
    return nMonth == 2 && isLeapYear(nYear) ? nDays + 1 : nDays;
}

constexpr bool isValidDate(std::uint16_t nDay, std::uint16_t nMonth, std::int16_t nYear)
{
    if (nYear == 0)
        return false;
    if (nMonth < 1 || 12 < nMonth)
        return false;
    return 1 <= nDay && nDay <= getDaysInMonth(nMonth, nYear);
}

constexpr bool isValidAndGregorian(std::uint16_t nDay, std::uint16_t nMonth, std::int16_t nYear)
{
    if (!isValidDate(nDay, nMonth, nYear))
        return false;
    if (nYear < 1582)
        return false;
    if (nYear == 1582 && nMonth < 10)
        return false;
    return nYear != 1582 || nMonth != 10 || nDay >= 15;
}

constexpr std::int32_t convertDateToDays(std::uint16_t nDay, std::uint16_t nMonth, std::int16_t nYear)
{
    std::int32_t nDays = yearToDays(nYear);
    for (std::uint16_t i = 1; i < nMonth; ++i)
        nDays += getDaysInMonth(i, nYear);
    return nDays + nDay;
}

inline bool normalize(std::uint16_t& rDay, std::uint16_t& rMonth, std::int16_t& rYear)
{
    if (isValidDate(rDay, rMonth, rYear))
        return false;

    if (rDay == 0 && rMonth == 0 && rYear == 0)
        return false;

    if (rDay == 0)
    {
        if (rMonth != 0)
            --rMonth;
    }

    if (rMonth > 12)
    {
        rYear += rMonth / 12;
        rMonth = rMonth % 12;
        if (rYear == 0)
            rYear = 1;
    }
    if (rMonth == 0)
    {
        --rYear;
        if (rYear == 0)
            rYear = -1;
        rMonth = 12;
    }

    if (rYear < 0)
    {
        std::uint16_t nDays;
        while (rDay > (nDays = getDaysInMonth(rMonth, rYear)))
        {
            rDay -= nDays;
            if (rMonth > 1)
                --rMonth;
            else
            {
                if (rYear == kYearMin)
                {
                    rDay = 1;
                    rMonth = 1;
                    return true;
                }
                --rYear;
                rMonth = 12;
            }
        }
    }
    else
    {
        std::uint16_t nDays;
        while (rDay > (nDays = getDaysInMonth(rMonth, rYear)))
        {
            rDay -= nDays;
            if (rMonth < 12)
                ++rMonth;
            else
            {
                if (rYear == kYearMax)
                {
                    rDay = 31;
                    rMonth = 12;
                    return true;
                }
                ++rYear;
                if (rYear == 0)
                    rYear = 1;
                rMonth = 1;
            }
        }
    }

    if (rDay == 0)
        rDay = getDaysInMonth(rMonth, rYear);

    return true;
}

inline void convertDaysToDate(std::int32_t nDays, std::uint16_t& rDay, std::uint16_t& rMonth, std::int16_t& rYear)
{
    constexpr std::int32_t MIN_DAYS = convertDateToDays(1, 1, kYearMin);
    constexpr std::int32_t MAX_DAYS = convertDateToDays(31, 12, kYearMax);

    if (nDays <= MIN_DAYS)
    {
        rDay = 1;
        rMonth = 1;
        rYear = kYearMin;
        return;
    }
    if (nDays >= MAX_DAYS)
    {
        rDay = 31;
        rMonth = 12;
        rYear = kYearMax;
        return;
    }

    const std::int16_t nSign = (nDays <= 0 ? -1 : 1);
    std::int32_t nTempDays;
    std::int32_t i = 0;
    bool bCalc;

    do
    {
        rYear = static_cast<std::int16_t>((nDays / 365) - (i * nSign));
        if (rYear == 0)
            rYear = nSign;
        nTempDays = nDays - yearToDays(rYear);
        bCalc = false;
        if (nTempDays < 1)
        {
            i += nSign;
            bCalc = true;
        }
        else if (nTempDays > 365 && (nTempDays != 366 || !isLeapYear(rYear)))
        {
            i -= nSign;
            bCalc = true;
        }
    } while (bCalc);

    rMonth = 1;
    while (nTempDays > getDaysInMonth(rMonth, rYear))
    {
        nTempDays -= getDaysInMonth(rMonth, rYear);
        ++rMonth;
    }
    rDay = static_cast<std::uint16_t>(nTempDays);
}

inline spreadsheetengine::api::DateParts fromAbsoluteDays(std::int32_t nDays)
{
    std::uint16_t nDay;
    std::uint16_t nMonth;
    std::int16_t nYear;
    convertDaysToDate(nDays, nDay, nMonth, nYear);
    return { nYear, static_cast<std::int16_t>(nMonth), static_cast<std::int16_t>(nDay) };
}

inline std::int32_t toAbsoluteDays(const spreadsheetengine::api::DateParts& rDate)
{
    return convertDateToDays(static_cast<std::uint16_t>(rDate.mnDay),
        static_cast<std::uint16_t>(rDate.mnMonth), static_cast<std::int16_t>(rDate.mnYear));
}

inline std::uint16_t getDayOfYear(const spreadsheetengine::api::DateParts& rDate)
{
    std::uint16_t nDay = static_cast<std::uint16_t>(rDate.mnDay);
    for (std::uint16_t i = 1; i < static_cast<std::uint16_t>(rDate.mnMonth); ++i)
        nDay += getDaysInMonth(i, static_cast<std::int16_t>(rDate.mnYear));
    return nDay;
}

inline std::int16_t getPrevYear(std::int16_t nYear) { return nYear == 1 ? -1 : nYear - 1; }
inline std::int16_t getNextYear(std::int16_t nYear) { return nYear == -1 ? 1 : nYear + 1; }

inline std::int16_t getDayOfWeekFromAbsoluteDays(std::int32_t nDays)
{
    std::int32_t nWeekday = (nDays - 1) % 7;
    if (nWeekday < 0)
        nWeekday += 7;
    return static_cast<std::int16_t>(nWeekday);
}

inline std::uint16_t getWeekOfYear(
    const spreadsheetengine::api::DateParts& rDate, std::int16_t nStartDay, std::int16_t nMinimumNumberOfDaysInWeek)
{
    short n1WDay = static_cast<short>(getDayOfWeekFromAbsoluteDays(
        convertDateToDays(1, 1, static_cast<std::int16_t>(rDate.mnYear))));
    short nDayOfYear = static_cast<short>(getDayOfYear(rDate));

    nDayOfYear--;
    n1WDay = (n1WDay + (7 - nStartDay)) % 7;

    if (nMinimumNumberOfDaysInWeek < 1 || 7 < nMinimumNumberOfDaysInWeek)
        nMinimumNumberOfDaysInWeek = 4;

    short nWeek;
    if (nMinimumNumberOfDaysInWeek == 1)
    {
        nWeek = ((n1WDay + nDayOfYear) / 7) + 1;
        if (nWeek == 54)
            nWeek = 1;
        else if (nWeek == 53)
        {
            const short nDaysInYear
                = static_cast<short>(isLeapYear(static_cast<std::int16_t>(rDate.mnYear)) ? 366 : 365);
            short nDaysNextYear = static_cast<short>(getDayOfWeekFromAbsoluteDays(
                convertDateToDays(1, 1, getNextYear(static_cast<std::int16_t>(rDate.mnYear)))));
            nDaysNextYear = (nDaysNextYear + (7 - nStartDay)) % 7;
            if (nDayOfYear > (nDaysInYear - nDaysNextYear - 1))
                nWeek = 1;
        }
    }
    else if (nMinimumNumberOfDaysInWeek == 7)
    {
        nWeek = ((n1WDay + nDayOfYear) / 7);
        if (nWeek == 0)
        {
            const auto aPrevYearEnd
                = spreadsheetengine::api::DateParts{ getPrevYear(static_cast<std::int16_t>(rDate.mnYear)), 12, 31 };
            nWeek = static_cast<short>(getWeekOfYear(aPrevYearEnd, nStartDay, nMinimumNumberOfDaysInWeek));
        }
    }
    else
    {
        if (n1WDay < nMinimumNumberOfDaysInWeek)
            nWeek = 1;
        else if (n1WDay == nMinimumNumberOfDaysInWeek)
            nWeek = 53;
        else if (n1WDay == nMinimumNumberOfDaysInWeek + 1)
        {
            nWeek = isLeapYear(getPrevYear(static_cast<std::int16_t>(rDate.mnYear))) ? 53 : 52;
        }
        else
            nWeek = 52;

        if ((nWeek == 1) || (nDayOfYear + n1WDay > 6))
        {
            if (nWeek == 1)
                nWeek += (nDayOfYear + n1WDay) / 7;
            else
                nWeek = (nDayOfYear + n1WDay) / 7;

            if (nWeek == 53)
            {
                std::int32_t nTempDays = toAbsoluteDays(rDate);
                nTempDays += 6 - (getDayOfWeekFromAbsoluteDays(nTempDays) + (7 - nStartDay)) % 7;
                nWeek = static_cast<short>(getWeekOfYear(
                    fromAbsoluteDays(nTempDays), nStartDay, nMinimumNumberOfDaysInWeek));
            }
        }
    }

    return static_cast<std::uint16_t>(nWeek);
}

} // namespace spreadsheetengine::core::detail::date

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
