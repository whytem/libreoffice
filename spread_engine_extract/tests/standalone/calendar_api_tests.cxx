/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <vector>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Workday.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::DateParts;
    using spreadsheetengine::api::DateSerial;
    using spreadsheetengine::api::Error;
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::fail;
    using namespace spreadsheetengine::api::calendar;
    using namespace spreadsheetengine::api::workday;

    const auto aDefaultWeekendMask = defaultWeekendMask();
    if (aDefaultWeekendMask[0] || aDefaultWeekendMask[4] || !aDefaultWeekendMask[5]
        || !aDefaultWeekendMask[6])
    {
        return fail("spreadsheetengine_calendar_tests", "defaultWeekendMask() mismatch");
    }

    const auto aSequenceWeekendMask
        = weekendMaskFromSequence({ 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 });
    if (!aSequenceWeekendMask || aSequenceWeekendMask.maValue != aDefaultWeekendMask)
        return fail("spreadsheetengine_calendar_tests", "weekendMaskFromSequence() mismatch");

    const auto aMsWeekendMask = weekendMaskFromMsSpec(u"0000011", false);
    if (!aMsWeekendMask || aMsWeekendMask.maValue != aDefaultWeekendMask)
        return fail("spreadsheetengine_calendar_tests", "weekendMaskFromMsSpec() mismatch");

    const auto aBadWeekendMask = weekendMaskFromMsSpec(u"1111111", true);
    if (aBadWeekendMask || aBadWeekendMask.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_calendar_tests",
            "weekendMaskFromMsSpec() invalid handling mismatch");
    }

    const std::vector<DateSerial> aHolidaySerials { 8 };
    if (countWorkdays(1, 10, aHolidaySerials, aDefaultWeekendMask) != 7)
        return fail("spreadsheetengine_calendar_tests", "countWorkdays() mismatch");

    if (advanceWorkday(5, 1, aHolidaySerials, aDefaultWeekendMask) != 9)
        return fail("spreadsheetengine_calendar_tests", "advanceWorkday() mismatch");

    const DateParts aNullDate { 1899, 12, 30 };
    const auto aJan1 = makeDateSerial(aNullDate, 1900, 1, 1, true);
    if (!aJan1 || !almostEqual(aJan1.maValue, 2.0))
        return fail("spreadsheetengine_calendar_tests", "makeDateSerial() mismatch");

    if (!almostEqual(yearFromSerial(aNullDate, 0), 1899.0)
        || !almostEqual(monthFromSerial(aNullDate, 0), 12.0))
    {
        return fail("spreadsheetengine_calendar_tests",
            "yearFromSerial()/monthFromSerial() mismatch");
    }

    const auto aDay = dayFromSerial(aNullDate, 0);
    if (!aDay || !almostEqual(aDay.maValue, 30.0))
        return fail("spreadsheetengine_calendar_tests", "dayFromSerial() mismatch");

    const auto aTime = makeTimeSerial(1.0, 30.0, 0.0);
    if (!aTime || !almostEqual(aTime.maValue, (1.5 / 24.0)))
        return fail("spreadsheetengine_calendar_tests", "makeTimeSerial() mismatch");

    if (!almostEqual(hourFromTimeValue(1.5 / 24.0), 1.0)
        || !almostEqual(minuteFromTimeValue(1.5 / 24.0), 30.0)
        || !almostEqual(secondFromTimeValue(1.5 / 24.0), 0.0))
    {
        return fail("spreadsheetengine_calendar_tests", "time extraction mismatch");
    }

    const auto aWeekday = dayOfWeek(aNullDate, 2, 2);
    if (!aWeekday || aWeekday.maValue != 1)
        return fail("spreadsheetengine_calendar_tests", "dayOfWeek() mismatch");

    if (weeknumOOo(aNullDate, 2, 1) != 1)
        return fail("spreadsheetengine_calendar_tests", "weeknumOOo() mismatch");

    const auto aJan1_2020 = makeDateSerial(aNullDate, 2020, 1, 1, true);
    if (!aJan1_2020)
        return fail("spreadsheetengine_calendar_tests", "makeDateSerial() 2020-01-01 mismatch");

    const auto aIsoWeek = weekOfYear(aNullDate, static_cast<DateSerial>(aJan1_2020.maValue), 21);
    if (!aIsoWeek || aIsoWeek.maValue != 1
        || isoWeekOfYear(aNullDate, static_cast<DateSerial>(aJan1_2020.maValue)) != 1)
    {
        return fail("spreadsheetengine_calendar_tests", "weekOfYear()/isoWeekOfYear() mismatch");
    }

    const auto aEaster = easterSundaySerial(aNullDate, 2024);
    if (!aEaster)
        return fail("spreadsheetengine_calendar_tests", "easterSundaySerial() mismatch");

    const auto aEasterMonth = monthFromSerial(aNullDate, static_cast<DateSerial>(aEaster.maValue));
    const auto aEasterDay = dayFromSerial(aNullDate, static_cast<DateSerial>(aEaster.maValue));
    if (!almostEqual(aEasterMonth, 3.0) || !aEasterDay || !almostEqual(aEasterDay.maValue, 31.0))
    {
        return fail("spreadsheetengine_calendar_tests",
            "easterSundaySerial() round-trip mismatch");
    }

    const auto aFeb1 = makeDateSerial(aNullDate, 2020, 2, 1, true);
    if (!aFeb1
        || !almostEqual(diffDate360(aNullDate, static_cast<DateSerial>(aJan1_2020.maValue),
                              static_cast<DateSerial>(aFeb1.maValue), false), 30.0))
    {
        return fail("spreadsheetengine_calendar_tests", "diffDate360() mismatch");
    }

    const auto aDate2 = makeDateSerial(aNullDate, 2021, 3, 15, true);
    if (!aDate2)
        return fail("spreadsheetengine_calendar_tests", "makeDateSerial() second date mismatch");

    const auto aDateDifY = dateDif(aNullDate, static_cast<DateSerial>(aJan1_2020.maValue),
        static_cast<DateSerial>(aDate2.maValue), u"y");
    const auto aDateDifYM = dateDif(aNullDate, static_cast<DateSerial>(aJan1_2020.maValue),
        static_cast<DateSerial>(aDate2.maValue), u"ym");
    if (!aDateDifY || !almostEqual(aDateDifY.maValue, 1.0) || !aDateDifYM
        || !almostEqual(aDateDifYM.maValue, 2.0))
    {
        return fail("spreadsheetengine_calendar_tests", "dateDif() mismatch");
    }

    std::cout << "spreadsheetengine calendar api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
