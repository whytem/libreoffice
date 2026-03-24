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

#include "DateAlgorithms.hxx"

namespace spreadsheetengine::core::datetime
{

namespace sedate = spreadsheetengine::core::detail::date;

namespace
{

spreadsheetengine::api::DateParts getDateForSerial(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays)
{
    return sedate::fromAbsoluteDays(sedate::toAbsoluteDays(rNullDate) + nDays);
}

}

WeekdayResult computeDayOfWeek(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays,
    sal_Int16 nFlag)
{
    const auto aDate = getDateForSerial(rNullDate, nDays);
    int nValue = static_cast<int>(sedate::getDayOfWeekFromAbsoluteDays(sedate::toAbsoluteDays(aDate)));

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

int computeWeeknumOOo(const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nDays, sal_Int16 nFlag)
{
    const auto aDate = getDateForSerial(rNullDate, nDays);
    return static_cast<int>(sedate::getWeekOfYear(aDate, nFlag == 1 ? 6 : 0, 1));
}

std::optional<int> computeWeekOfYear(const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nDays, sal_Int16 nFlag)
{
    const auto aDate = getDateForSerial(rNullDate, nDays);

    sal_Int32 nMinimumNumberOfDaysInWeek;
    sal_Int16 nFirstDayOfWeek;
    switch (nFlag)
    {
        case 1:
            nFirstDayOfWeek = 6;
            nMinimumNumberOfDaysInWeek = 1;
            break;
        case 2:
            nFirstDayOfWeek = 0;
            nMinimumNumberOfDaysInWeek = 1;
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            nFirstDayOfWeek = nFlag - 11;
            nMinimumNumberOfDaysInWeek = 1;
            break;
        case 21:
        case 150:
            nFirstDayOfWeek = 0;
            nMinimumNumberOfDaysInWeek = 4;
            break;
        default:
            return std::nullopt;
    }

    return static_cast<int>(sedate::getWeekOfYear(aDate, nFirstDayOfWeek, nMinimumNumberOfDaysInWeek));
}

int computeIsoWeekOfYear(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays)
{
    const auto aDate = getDateForSerial(rNullDate, nDays);
    return static_cast<int>(sedate::getWeekOfYear(aDate, 0, 4));
}

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
