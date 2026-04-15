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

#include <spreadsheetengine/api/Types.hxx>
#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::datetime
{

struct WorkdayRuntimeStatsSnapshot
{
    sal_uInt64 mnWeekendMaskSequenceCalls = 0;
    sal_uInt64 mnWeekendMaskMsSpecCalls = 0;
    sal_uInt64 mnCountWorkdaysCalls = 0;
    sal_uInt64 mnCountWorkdaysTotalSpanDays = 0;
    sal_uInt64 mnCountWorkdaysMaxSpanDays = 0;
    sal_uInt64 mnCountWorkdaysTotalHolidayCount = 0;
    sal_uInt64 mnCountWorkdaysMaxHolidayCount = 0;
    sal_uInt64 mnCountWorkdaysTotalLoopIterations = 0;
    sal_uInt64 mnCountWorkdaysMaxLoopIterations = 0;
    sal_uInt64 mnAdvanceWorkdayCalls = 0;
    sal_uInt64 mnAdvanceWorkdayTotalRequestedDays = 0;
    sal_uInt64 mnAdvanceWorkdayMaxRequestedDays = 0;
    sal_uInt64 mnAdvanceWorkdayTotalHolidayCount = 0;
    sal_uInt64 mnAdvanceWorkdayMaxHolidayCount = 0;
    sal_uInt64 mnAdvanceWorkdayTotalCalendarSteps = 0;
    sal_uInt64 mnAdvanceWorkdayMaxCalendarSteps = 0;
    sal_uInt64 mnAdvanceWorkdayTotalWeekendSkips = 0;
    sal_uInt64 mnAdvanceWorkdayTotalHolidaySkips = 0;
};

SPREADSHEETENGINE_DLLPUBLIC void resetWorkdayRuntimeStats();

SPREADSHEETENGINE_DLLPUBLIC WorkdayRuntimeStatsSnapshot getWorkdayRuntimeStatsSnapshot();

SPREADSHEETENGINE_DLLPUBLIC void setDefaultWeekendMask(
    spreadsheetengine::api::WeekendMask& rWeekendMask);

SPREADSHEETENGINE_DLLPUBLIC bool applyWeekendMaskSequence(
    const std::vector<double>& rWeekendDays, spreadsheetengine::api::WeekendMask& rWeekendMask);

SPREADSHEETENGINE_DLLPUBLIC bool applyWeekendMaskMsSpec(
    spreadsheetengine::api::StringView rWeekendDays, bool bWorkdayFunction,
    spreadsheetengine::api::WeekendMask& rWeekendMask);

SPREADSHEETENGINE_DLLPUBLIC bool hasAvailableWorkday(
    const spreadsheetengine::api::WeekendMask& rWeekendMask);

SPREADSHEETENGINE_DLLPUBLIC sal_Int32 countWorkdays(
    spreadsheetengine::api::DateSerial nDate1, spreadsheetengine::api::DateSerial nDate2,
    const std::vector<spreadsheetengine::api::DateSerial>& rSortedHolidays,
    const spreadsheetengine::api::WeekendMask& rWeekendMask);

SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::DateSerial advanceWorkday(
    spreadsheetengine::api::DateSerial nDate, spreadsheetengine::api::DateSerial nDays,
    const std::vector<spreadsheetengine::api::DateSerial>& rSortedHolidays,
    const spreadsheetengine::api::WeekendMask& rWeekendMask);

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
