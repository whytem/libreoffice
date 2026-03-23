/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/DateTimeWorkday.hxx>

#include <algorithm>
#include <cstddef>

#include <tools/date.hxx>

namespace spreadsheetengine::core::datetime
{

namespace
{

sal_Int16 getNormalizedDayOfWeek(sal_Int32 nDate)
{
    return static_cast<sal_Int16>((nDate - 1) % 7);
}

}

void setDefaultWeekendMask(bool bWeekendMask[7])
{
    std::fill_n(bWeekendMask, 7, false);
    bWeekendMask[SATURDAY] = true;
    bWeekendMask[SUNDAY] = true;
}

bool applyWeekendMaskSequence(const std::vector<double>& rWeekendDays, bool bWeekendMask[7])
{
    if (rWeekendDays.size() != 7)
        return false;

    for (int i = 0; i < 7; ++i)
        bWeekendMask[i] = static_cast<bool>(rWeekendDays[(i == 6 ? 0 : i + 1)]);
    return true;
}

bool applyWeekendMaskMsSpec(const OUString& rWeekendDays, bool bWorkdayFunction, bool bWeekendMask[7])
{
    setDefaultWeekendMask(bWeekendMask);
    if (rWeekendDays.isEmpty())
        return true;

    if (bWorkdayFunction && rWeekendDays == "1111111")
        return false;

    std::fill_n(bWeekendMask, 7, false);
    switch (rWeekendDays.getLength())
    {
        case 1:
            switch (rWeekendDays[0])
            {
                case '1':
                    bWeekendMask[SATURDAY] = true;
                    bWeekendMask[SUNDAY] = true;
                    break;
                case '2':
                    bWeekendMask[SUNDAY] = true;
                    bWeekendMask[MONDAY] = true;
                    break;
                case '3':
                    bWeekendMask[MONDAY] = true;
                    bWeekendMask[TUESDAY] = true;
                    break;
                case '4':
                    bWeekendMask[TUESDAY] = true;
                    bWeekendMask[WEDNESDAY] = true;
                    break;
                case '5':
                    bWeekendMask[WEDNESDAY] = true;
                    bWeekendMask[THURSDAY] = true;
                    break;
                case '6':
                    bWeekendMask[THURSDAY] = true;
                    bWeekendMask[FRIDAY] = true;
                    break;
                case '7':
                    bWeekendMask[FRIDAY] = true;
                    bWeekendMask[SATURDAY] = true;
                    break;
                default:
                    return false;
            }
            return true;
        case 2:
            if (rWeekendDays[0] != '1')
                return false;
            switch (rWeekendDays[1])
            {
                case '1':
                    bWeekendMask[SUNDAY] = true;
                    break;
                case '2':
                    bWeekendMask[MONDAY] = true;
                    break;
                case '3':
                    bWeekendMask[TUESDAY] = true;
                    break;
                case '4':
                    bWeekendMask[WEDNESDAY] = true;
                    break;
                case '5':
                    bWeekendMask[THURSDAY] = true;
                    break;
                case '6':
                    bWeekendMask[FRIDAY] = true;
                    break;
                case '7':
                    bWeekendMask[SATURDAY] = true;
                    break;
                default:
                    return false;
            }
            return true;
        case 7:
            for (int i = 0; i < 7; ++i)
            {
                switch (rWeekendDays[i])
                {
                    case '0':
                        bWeekendMask[i] = false;
                        break;
                    case '1':
                        bWeekendMask[i] = true;
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

sal_Int32 countWorkdays(
    sal_Int32 nDate1, sal_Int32 nDate2, const std::vector<double>& rSortedHolidays,
    const bool bWeekendMask[7])
{
    sal_Int32 nCount = 0;
    std::size_t nRef = 0;
    const bool bReverse = nDate1 > nDate2;
    if (bReverse)
        std::swap(nDate1, nDate2);

    const std::size_t nMax = rSortedHolidays.size();
    while (nDate1 <= nDate2)
    {
        if (!bWeekendMask[getNormalizedDayOfWeek(nDate1)])
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

sal_Int32 advanceWorkday(
    sal_Int32 nDate, sal_Int32 nDays, const std::vector<double>& rSortedHolidays,
    const bool bWeekendMask[7])
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
            } while (bWeekendMask[getNormalizedDayOfWeek(nDate)]);

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
            } while (bWeekendMask[getNormalizedDayOfWeek(nDate)]);

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
