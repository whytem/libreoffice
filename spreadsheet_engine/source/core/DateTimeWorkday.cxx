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
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <string_view>

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

struct WorkdayRuntimeStatsStore
{
    std::atomic<sal_uInt64> mnWeekendMaskSequenceCalls { 0 };
    std::atomic<sal_uInt64> mnWeekendMaskMsSpecCalls { 0 };
    std::atomic<sal_uInt64> mnCountWorkdaysCalls { 0 };
    std::atomic<sal_uInt64> mnCountWorkdaysTotalSpanDays { 0 };
    std::atomic<sal_uInt64> mnCountWorkdaysMaxSpanDays { 0 };
    std::atomic<sal_uInt64> mnCountWorkdaysTotalHolidayCount { 0 };
    std::atomic<sal_uInt64> mnCountWorkdaysMaxHolidayCount { 0 };
    std::atomic<sal_uInt64> mnCountWorkdaysTotalLoopIterations { 0 };
    std::atomic<sal_uInt64> mnCountWorkdaysMaxLoopIterations { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayCalls { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayTotalRequestedDays { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayMaxRequestedDays { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayTotalHolidayCount { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayMaxHolidayCount { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayTotalCalendarSteps { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayMaxCalendarSteps { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayTotalWeekendSkips { 0 };
    std::atomic<sal_uInt64> mnAdvanceWorkdayTotalHolidaySkips { 0 };
};

WorkdayRuntimeStatsStore& workdayRuntimeStatsStore()
{
    static WorkdayRuntimeStatsStore aStore;
    return aStore;
}

bool workdayRuntimeStatsEnabled()
{
    static const bool bEnabled = [] {
        if (const char* pValue = std::getenv("SPREADSHEET_ENGINE_WORKDAY_RUNTIME_STATS"))
        {
            const std::string_view aValue(pValue);
            return !aValue.empty() && aValue != "0" && aValue != "off" && aValue != "false";
        }
        return false;
    }();
    return bEnabled;
}

void addStat(std::atomic<sal_uInt64>& rTarget, sal_uInt64 nDelta)
{
    rTarget.fetch_add(nDelta, std::memory_order_relaxed);
}

void updateMaxStat(std::atomic<sal_uInt64>& rTarget, sal_uInt64 nValue)
{
    sal_uInt64 nCurrent = rTarget.load(std::memory_order_relaxed);
    while (nCurrent < nValue
           && !rTarget.compare_exchange_weak(
               nCurrent, nValue, std::memory_order_relaxed, std::memory_order_relaxed))
    {
    }
}

std::size_t getNormalizedDayOfWeek(spreadsheetengine::api::DateSerial nDate)
{
    auto nDay = static_cast<int>((nDate + 5) % 7);
    if (nDay < 0)
        nDay += 7;
    return static_cast<std::size_t>(nDay);
}

}

void resetWorkdayRuntimeStats()
{
    auto& rStore = workdayRuntimeStatsStore();
    rStore.mnWeekendMaskSequenceCalls.store(0, std::memory_order_relaxed);
    rStore.mnWeekendMaskMsSpecCalls.store(0, std::memory_order_relaxed);
    rStore.mnCountWorkdaysCalls.store(0, std::memory_order_relaxed);
    rStore.mnCountWorkdaysTotalSpanDays.store(0, std::memory_order_relaxed);
    rStore.mnCountWorkdaysMaxSpanDays.store(0, std::memory_order_relaxed);
    rStore.mnCountWorkdaysTotalHolidayCount.store(0, std::memory_order_relaxed);
    rStore.mnCountWorkdaysMaxHolidayCount.store(0, std::memory_order_relaxed);
    rStore.mnCountWorkdaysTotalLoopIterations.store(0, std::memory_order_relaxed);
    rStore.mnCountWorkdaysMaxLoopIterations.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayCalls.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayTotalRequestedDays.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayMaxRequestedDays.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayTotalHolidayCount.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayMaxHolidayCount.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayTotalCalendarSteps.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayMaxCalendarSteps.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayTotalWeekendSkips.store(0, std::memory_order_relaxed);
    rStore.mnAdvanceWorkdayTotalHolidaySkips.store(0, std::memory_order_relaxed);
}

WorkdayRuntimeStatsSnapshot getWorkdayRuntimeStatsSnapshot()
{
    const auto& rStore = workdayRuntimeStatsStore();
    WorkdayRuntimeStatsSnapshot aSnapshot;
    aSnapshot.mnWeekendMaskSequenceCalls
        = rStore.mnWeekendMaskSequenceCalls.load(std::memory_order_relaxed);
    aSnapshot.mnWeekendMaskMsSpecCalls
        = rStore.mnWeekendMaskMsSpecCalls.load(std::memory_order_relaxed);
    aSnapshot.mnCountWorkdaysCalls
        = rStore.mnCountWorkdaysCalls.load(std::memory_order_relaxed);
    aSnapshot.mnCountWorkdaysTotalSpanDays
        = rStore.mnCountWorkdaysTotalSpanDays.load(std::memory_order_relaxed);
    aSnapshot.mnCountWorkdaysMaxSpanDays
        = rStore.mnCountWorkdaysMaxSpanDays.load(std::memory_order_relaxed);
    aSnapshot.mnCountWorkdaysTotalHolidayCount
        = rStore.mnCountWorkdaysTotalHolidayCount.load(std::memory_order_relaxed);
    aSnapshot.mnCountWorkdaysMaxHolidayCount
        = rStore.mnCountWorkdaysMaxHolidayCount.load(std::memory_order_relaxed);
    aSnapshot.mnCountWorkdaysTotalLoopIterations
        = rStore.mnCountWorkdaysTotalLoopIterations.load(std::memory_order_relaxed);
    aSnapshot.mnCountWorkdaysMaxLoopIterations
        = rStore.mnCountWorkdaysMaxLoopIterations.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayCalls
        = rStore.mnAdvanceWorkdayCalls.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayTotalRequestedDays
        = rStore.mnAdvanceWorkdayTotalRequestedDays.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayMaxRequestedDays
        = rStore.mnAdvanceWorkdayMaxRequestedDays.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayTotalHolidayCount
        = rStore.mnAdvanceWorkdayTotalHolidayCount.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayMaxHolidayCount
        = rStore.mnAdvanceWorkdayMaxHolidayCount.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayTotalCalendarSteps
        = rStore.mnAdvanceWorkdayTotalCalendarSteps.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayMaxCalendarSteps
        = rStore.mnAdvanceWorkdayMaxCalendarSteps.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayTotalWeekendSkips
        = rStore.mnAdvanceWorkdayTotalWeekendSkips.load(std::memory_order_relaxed);
    aSnapshot.mnAdvanceWorkdayTotalHolidaySkips
        = rStore.mnAdvanceWorkdayTotalHolidaySkips.load(std::memory_order_relaxed);
    return aSnapshot;
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
    if (workdayRuntimeStatsEnabled())
        addStat(workdayRuntimeStatsStore().mnWeekendMaskSequenceCalls, 1);

    if (rWeekendDays.size() != 7)
        return false;

    for (std::size_t i = 0; i < rWeekendMask.size(); ++i)
        rWeekendMask[i] = static_cast<bool>(rWeekendDays[(i == SUNDAY_INDEX) ? 0 : i + 1]);
    return true;
}

bool applyWeekendMaskMsSpec(spreadsheetengine::api::StringView rWeekendDays, bool bWorkdayFunction,
    spreadsheetengine::api::WeekendMask& rWeekendMask)
{
    if (workdayRuntimeStatsEnabled())
        addStat(workdayRuntimeStatsStore().mnWeekendMaskMsSpecCalls, 1);

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

bool hasAvailableWorkday(const spreadsheetengine::api::WeekendMask& rWeekendMask)
{
    return std::any_of(rWeekendMask.begin(), rWeekendMask.end(), [](bool bWeekend) {
        return !bWeekend;
    });
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

    const sal_uInt64 nSpanDays = nDate2 >= nDate1
                                     ? static_cast<sal_uInt64>(nDate2 - nDate1) + 1
                                     : 0;
    if (workdayRuntimeStatsEnabled())
    {
        auto& rStore = workdayRuntimeStatsStore();
        addStat(rStore.mnCountWorkdaysCalls, 1);
        addStat(rStore.mnCountWorkdaysTotalSpanDays, nSpanDays);
        updateMaxStat(rStore.mnCountWorkdaysMaxSpanDays, nSpanDays);
        addStat(rStore.mnCountWorkdaysTotalHolidayCount,
            static_cast<sal_uInt64>(rSortedHolidays.size()));
        updateMaxStat(rStore.mnCountWorkdaysMaxHolidayCount,
            static_cast<sal_uInt64>(rSortedHolidays.size()));
        addStat(rStore.mnCountWorkdaysTotalLoopIterations, nSpanDays);
        updateMaxStat(rStore.mnCountWorkdaysMaxLoopIterations, nSpanDays);
    }

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
    const sal_uInt64 nRequestedDays = static_cast<sal_uInt64>(
        nDays >= 0 ? nDays : -static_cast<long long>(nDays));
    sal_uInt64 nCalendarSteps = 0;
    sal_uInt64 nWeekendSkips = 0;
    sal_uInt64 nHolidaySkips = 0;

    if (workdayRuntimeStatsEnabled())
    {
        auto& rStore = workdayRuntimeStatsStore();
        addStat(rStore.mnAdvanceWorkdayCalls, 1);
        addStat(rStore.mnAdvanceWorkdayTotalRequestedDays, nRequestedDays);
        updateMaxStat(rStore.mnAdvanceWorkdayMaxRequestedDays, nRequestedDays);
        addStat(rStore.mnAdvanceWorkdayTotalHolidayCount,
            static_cast<sal_uInt64>(rSortedHolidays.size()));
        updateMaxStat(rStore.mnAdvanceWorkdayMaxHolidayCount,
            static_cast<sal_uInt64>(rSortedHolidays.size()));
    }

    if (!nDays)
        return nDate;

    const std::size_t nMax = rSortedHolidays.size();
    if (nDays > 0)
    {
        std::size_t nRef = 0;
        while (nDays)
        {
            for (;;)
            {
                ++nDate;
                ++nCalendarSteps;
                if (rWeekendMask[getNormalizedDayOfWeek(nDate)])
                {
                    ++nWeekendSkips;
                    continue;
                }
                break;
            }

            while (nRef < nMax && rSortedHolidays[nRef] < nDate)
                ++nRef;

            if (nRef >= nMax || rSortedHolidays[nRef] != nDate)
                --nDays;
            else
                ++nHolidaySkips;
        }
    }
    else
    {
        std::ptrdiff_t nRef = static_cast<std::ptrdiff_t>(nMax) - 1;
        while (nDays)
        {
            for (;;)
            {
                --nDate;
                ++nCalendarSteps;
                if (rWeekendMask[getNormalizedDayOfWeek(nDate)])
                {
                    ++nWeekendSkips;
                    continue;
                }
                break;
            }

            while (nRef >= 0 && rSortedHolidays[static_cast<std::size_t>(nRef)] > nDate)
                --nRef;

            if (nRef < 0 || rSortedHolidays[static_cast<std::size_t>(nRef)] != nDate)
                ++nDays;
            else
                ++nHolidaySkips;
        }
    }

    if (workdayRuntimeStatsEnabled())
    {
        auto& rStore = workdayRuntimeStatsStore();
        addStat(rStore.mnAdvanceWorkdayTotalCalendarSteps, nCalendarSteps);
        updateMaxStat(rStore.mnAdvanceWorkdayMaxCalendarSteps, nCalendarSteps);
        addStat(rStore.mnAdvanceWorkdayTotalWeekendSkips, nWeekendSkips);
        addStat(rStore.mnAdvanceWorkdayTotalHolidaySkips, nHolidaySkips);
    }

    return nDate;
}

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
