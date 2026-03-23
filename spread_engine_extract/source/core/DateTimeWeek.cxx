/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/DateTimeWeek.hxx>

#include <optional>

#include <tools/date.hxx>

namespace spreadsheetengine::core::datetime
{

WeekdayResult computeDayOfWeek(const Date& rNullDate, sal_Int32 nDays, sal_Int16 nFlag)
{
    Date aDate = rNullDate;
    aDate.AddDays(nDays);
    int nValue = static_cast<int>(aDate.GetDayOfWeek());

    switch (nFlag)
    {
        case 1:
            if (nValue == 6)
                nValue = 1;
            else
                nValue += 2;
            break;
        case 2:
            nValue += 1;
            break;
        case 3:
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            if (nValue < nFlag - 11)
                nValue += 19 - nFlag;
            else
                nValue -= nFlag - 12;
            break;
        default:
            return { nValue, false };
    }

    return { nValue, true };
}

int computeWeeknumOOo(const Date& rNullDate, sal_Int32 nDays, sal_Int16 nFlag)
{
    Date aDate = rNullDate;
    aDate.AddDays(nDays);
    return static_cast<int>(aDate.GetWeekOfYear(nFlag == 1 ? SUNDAY : MONDAY));
}

std::optional<int> computeWeekOfYear(const Date& rNullDate, sal_Int32 nDays, sal_Int16 nFlag)
{
    Date aDate = rNullDate;
    aDate.AddDays(nDays);

    sal_Int32 nMinimumNumberOfDaysInWeek;
    DayOfWeek eFirstDayOfWeek;
    switch (nFlag)
    {
        case 1:
            eFirstDayOfWeek = SUNDAY;
            nMinimumNumberOfDaysInWeek = 1;
            break;
        case 2:
            eFirstDayOfWeek = MONDAY;
            nMinimumNumberOfDaysInWeek = 1;
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            eFirstDayOfWeek = static_cast<DayOfWeek>(nFlag - 11);
            nMinimumNumberOfDaysInWeek = 1;
            break;
        case 21:
        case 150:
            eFirstDayOfWeek = MONDAY;
            nMinimumNumberOfDaysInWeek = 4;
            break;
        default:
            return std::nullopt;
    }

    return static_cast<int>(aDate.GetWeekOfYear(eFirstDayOfWeek, nMinimumNumberOfDaysInWeek));
}

int computeIsoWeekOfYear(const Date& rNullDate, sal_Int32 nDays)
{
    Date aDate = rNullDate;
    aDate.AddDays(nDays);
    return static_cast<int>(aDate.GetWeekOfYear());
}

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
