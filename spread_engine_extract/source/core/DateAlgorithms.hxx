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
#include <limits>

#include <sal/types.h>

#include <spreadsheetengine/api/Date.hxx>

namespace spreadsheetengine::core::detail::date
{

constexpr sal_Int16 kYearMax = std::numeric_limits<sal_Int16>::max();
constexpr sal_Int16 kYearMin = std::numeric_limits<sal_Int16>::min();

constexpr sal_Int32 yearToDays(sal_Int16 nYear)
{
    assert(nYear != 0);
    auto val = [](int off, int y) { return off + y * 365 + y / 4 - y / 100 + y / 400; };
    return nYear < 0 ? val(-366, nYear + 1) : val(0, nYear - 1);
}

constexpr bool isLeapYear(sal_Int16 nYear)
{
    assert(nYear != 0);
    if (nYear < 0)
        nYear = -nYear - 1;
    return (((nYear % 4) == 0) && ((nYear % 100) != 0)) || ((nYear % 400) == 0);
}

constexpr sal_uInt16 getDaysInMonth(sal_uInt16 nMonth, sal_Int16 nYear)
{
    assert(1 <= nMonth && nMonth <= 12);
    if (nMonth < 1 || 12 < nMonth)
        return 0;

    constexpr sal_uInt16 aDaysInMonth[12]
        = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    sal_uInt16 nDays = aDaysInMonth[nMonth - 1];
    return nMonth == 2 && isLeapYear(nYear) ? nDays + 1 : nDays;
}

constexpr bool isValidDate(sal_uInt16 nDay, sal_uInt16 nMonth, sal_Int16 nYear)
{
    if (nYear == 0)
        return false;
    if (nMonth < 1 || 12 < nMonth)
        return false;
    return 1 <= nDay && nDay <= getDaysInMonth(nMonth, nYear);
}

constexpr bool isValidAndGregorian(sal_uInt16 nDay, sal_uInt16 nMonth, sal_Int16 nYear)
{
    if (!isValidDate(nDay, nMonth, nYear))
        return false;
    if (nYear < 1582)
        return false;
    if (nYear == 1582 && nMonth < 10)
        return false;
    return nYear != 1582 || nMonth != 10 || nDay >= 15;
}

constexpr sal_Int32 convertDateToDays(sal_uInt16 nDay, sal_uInt16 nMonth, sal_Int16 nYear)
{
    sal_Int32 nDays = yearToDays(nYear);
    for (sal_uInt16 i = 1; i < nMonth; ++i)
        nDays += getDaysInMonth(i, nYear);
    return nDays + nDay;
}

inline bool normalize(sal_uInt16& rDay, sal_uInt16& rMonth, sal_Int16& rYear)
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
        sal_uInt16 nDays;
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
        sal_uInt16 nDays;
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

inline void convertDaysToDate(sal_Int32 nDays, sal_uInt16& rDay, sal_uInt16& rMonth, sal_Int16& rYear)
{
    constexpr sal_Int32 MIN_DAYS = convertDateToDays(1, 1, kYearMin);
    constexpr sal_Int32 MAX_DAYS = convertDateToDays(31, 12, kYearMax);

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

    const sal_Int16 nSign = (nDays <= 0 ? -1 : 1);
    sal_Int32 nTempDays;
    sal_Int32 i = 0;
    bool bCalc;

    do
    {
        rYear = static_cast<sal_Int16>((nDays / 365) - (i * nSign));
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
    rDay = static_cast<sal_uInt16>(nTempDays);
}

inline spreadsheetengine::api::DateParts fromAbsoluteDays(sal_Int32 nDays)
{
    sal_uInt16 nDay;
    sal_uInt16 nMonth;
    sal_Int16 nYear;
    convertDaysToDate(nDays, nDay, nMonth, nYear);
    return { nYear, static_cast<std::int16_t>(nMonth), static_cast<std::int16_t>(nDay) };
}

inline sal_Int32 toAbsoluteDays(const spreadsheetengine::api::DateParts& rDate)
{
    return convertDateToDays(static_cast<sal_uInt16>(rDate.mnDay),
        static_cast<sal_uInt16>(rDate.mnMonth), static_cast<sal_Int16>(rDate.mnYear));
}

inline sal_uInt16 getDayOfYear(const spreadsheetengine::api::DateParts& rDate)
{
    sal_uInt16 nDay = static_cast<sal_uInt16>(rDate.mnDay);
    for (sal_uInt16 i = 1; i < static_cast<sal_uInt16>(rDate.mnMonth); ++i)
        nDay += getDaysInMonth(i, static_cast<sal_Int16>(rDate.mnYear));
    return nDay;
}

inline sal_Int16 getPrevYear(sal_Int16 nYear) { return nYear == 1 ? -1 : nYear - 1; }
inline sal_Int16 getNextYear(sal_Int16 nYear) { return nYear == -1 ? 1 : nYear + 1; }

inline sal_Int16 getDayOfWeekFromAbsoluteDays(sal_Int32 nDays)
{
    sal_Int32 nWeekday = (nDays - 1) % 7;
    if (nWeekday < 0)
        nWeekday += 7;
    return static_cast<sal_Int16>(nWeekday);
}

inline sal_uInt16 getWeekOfYear(
    const spreadsheetengine::api::DateParts& rDate, sal_Int16 nStartDay, sal_Int16 nMinimumNumberOfDaysInWeek)
{
    short n1WDay = static_cast<short>(getDayOfWeekFromAbsoluteDays(
        convertDateToDays(1, 1, static_cast<sal_Int16>(rDate.mnYear))));
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
                = static_cast<short>(isLeapYear(static_cast<sal_Int16>(rDate.mnYear)) ? 366 : 365);
            short nDaysNextYear = static_cast<short>(getDayOfWeekFromAbsoluteDays(
                convertDateToDays(1, 1, getNextYear(static_cast<sal_Int16>(rDate.mnYear)))));
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
                = spreadsheetengine::api::DateParts{ getPrevYear(static_cast<sal_Int16>(rDate.mnYear)), 12, 31 };
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
            nWeek = isLeapYear(getPrevYear(static_cast<sal_Int16>(rDate.mnYear))) ? 53 : 52;
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
                sal_Int32 nTempDays = toAbsoluteDays(rDate);
                nTempDays += 6 - (getDayOfWeekFromAbsoluteDays(nTempDays) + (7 - nStartDay)) % 7;
                nWeek = static_cast<short>(getWeekOfYear(
                    fromAbsoluteDays(nTempDays), nStartDay, nMinimumNumberOfDaysInWeek));
            }
        }
    }

    return static_cast<sal_uInt16>(nWeek);
}

} // namespace spreadsheetengine::core::detail::date

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
