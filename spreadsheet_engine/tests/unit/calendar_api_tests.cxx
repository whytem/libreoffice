/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <cstdint>
#include <vector>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Workday.hxx>

#include "SharedCaseSupport.hxx"
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

    if (advanceWorkday(5, 1, aHolidaySerials, aDefaultWeekendMask) != 6)
        return fail("spreadsheetengine_calendar_tests", "advanceWorkday() mismatch");

    const std::vector<DateSerial> aNovemberHolidaySerials { 41945, 41946, 41947 };
    if (advanceWorkday(41944, 5, aNovemberHolidaySerials, aDefaultWeekendMask) != 41954)
    {
        return fail("spreadsheetengine_calendar_tests", "advanceWorkday() November mismatch");
    }
    if (countWorkdays(41944, 41973, aNovemberHolidaySerials, aDefaultWeekendMask) != 18)
    {
        return fail("spreadsheetengine_calendar_tests", "countWorkdays() November mismatch");
    }

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

    const auto aMarch31_2001 = makeDateSerial(aNullDate, 2001, 3, 31, true);
    if (!aMarch31_2001)
        return fail("spreadsheetengine_calendar_tests", "makeDateSerial() 2001-03-31 mismatch");
    const auto aShiftedEdate = shiftMonthSerial(static_cast<DateSerial>(aMarch31_2001.maValue), 1, false);
    if (!aShiftedEdate || !almostEqual(aShiftedEdate.maValue, 37011.0))
        return fail("spreadsheetengine_calendar_tests", "shiftMonthSerial() EDATE mismatch");

    const auto aJan11_2015 = makeDateSerial(aNullDate, 2015, 1, 11, true);
    if (!aJan11_2015)
        return fail("spreadsheetengine_calendar_tests", "makeDateSerial() 2015-01-11 mismatch");
    const auto aShiftedEomonth = shiftMonthSerial(static_cast<DateSerial>(aJan11_2015.maValue), 1, true);
    if (!aShiftedEomonth || !almostEqual(aShiftedEomonth.maValue, 42063.0))
        return fail("spreadsheetengine_calendar_tests", "shiftMonthSerial() EOMONTH mismatch");

    const auto maskToString = [](const spreadsheetengine::api::WeekendMask& rMask) {
        std::string aMask(7, '0');
        for (std::size_t i = 0; i < rMask.size(); ++i)
            aMask[i] = rMask[i] ? '1' : '0';
        return aMask;
    };

    const auto parseDateSerialToken = [&](std::string_view rToken) -> std::optional<DateSerial> {
        if (rToken.empty())
            return std::nullopt;
        if (rToken.find('-') == std::string_view::npos)
        {
            return static_cast<DateSerial>(
                spreadsheetengine::standalone::test::parseDouble(rToken));
        }

        const std::string aToken(rToken);
        const std::size_t nDash1 = aToken.find('-');
        const std::size_t nDash2 = aToken.find('-', nDash1 + 1);
        if (nDash1 == std::string::npos || nDash2 == std::string::npos)
            return std::nullopt;

        const auto aSerial = makeDateSerial(
            aNullDate, static_cast<std::int16_t>(std::stoi(aToken.substr(0, nDash1))),
            static_cast<std::int16_t>(std::stoi(aToken.substr(nDash1 + 1, nDash2 - nDash1 - 1))),
            static_cast<std::int16_t>(std::stoi(aToken.substr(nDash2 + 1))), true);
        if (!aSerial)
            return std::nullopt;
        return static_cast<DateSerial>(aSerial.maValue);
    };

    const auto parseHolidayList = [&](std::string_view rToken) {
        std::vector<DateSerial> aHolidays;
        std::size_t nStart = 0;
        while (nStart < rToken.size())
        {
            const std::size_t nComma = rToken.find(',', nStart);
            const auto aPart = rToken.substr(
                nStart, nComma == std::string_view::npos ? rToken.size() - nStart : nComma - nStart);
            if (!aPart.empty())
            {
                const auto oSerial = parseDateSerialToken(aPart);
                if (!oSerial)
                    return std::optional<std::vector<DateSerial>>();
                aHolidays.push_back(*oSerial);
            }

            if (nComma == std::string_view::npos)
                break;
            nStart = nComma + 1;
        }

        return std::optional<std::vector<DateSerial>>(aHolidays);
    };

    for (const auto& rRow :
         spreadsheetengine::standalone::test::loadSharedCaseRows("calendar_cases.tsv"))
    {
        if (rRow.maColumns.size() < 7)
        {
            return failSharedCase(
                "spreadsheetengine_calendar_tests", rRow, "calendar shared case column mismatch");
        }

        const auto& rFunction = rRow.maColumns[0];
        if (rFunction == "DATE")
        {
            const auto aResult = makeDateSerial(
                aNullDate, static_cast<std::int16_t>(spreadsheetengine::standalone::test::parseDouble(
                               rRow.maColumns[1])),
                static_cast<std::int16_t>(spreadsheetengine::standalone::test::parseDouble(
                    rRow.maColumns[2])),
                static_cast<std::int16_t>(spreadsheetengine::standalone::test::parseDouble(
                    rRow.maColumns[3])),
                rRow.maColumns[4] == "STRICT");
            if (!aResult
                || !almostEqual(
                    aResult.maValue,
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase("spreadsheetengine_calendar_tests", rRow, "DATE mismatch");
            }
        }
        else if (rFunction == "YEARFROM")
        {
            if (!almostEqual(yearFromSerial(
                                 aNullDate, static_cast<DateSerial>(spreadsheetengine::standalone::test::parseDouble(
                                                rRow.maColumns[1]))),
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "YEARFROM mismatch");
            }
        }
        else if (rFunction == "MONTHFROM")
        {
            if (!almostEqual(monthFromSerial(
                                 aNullDate, static_cast<DateSerial>(spreadsheetengine::standalone::test::parseDouble(
                                                rRow.maColumns[1]))),
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "MONTHFROM mismatch");
            }
        }
        else if (rFunction == "DAYFROM")
        {
            const auto aResult = dayFromSerial(
                aNullDate, static_cast<DateSerial>(
                               spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])));
            if (!aResult
                || !almostEqual(
                    aResult.maValue,
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "DAYFROM mismatch");
            }
        }
        else if (rFunction == "TIME")
        {
            const auto aResult = makeTimeSerial(
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[3]));
            if (!aResult
                || !almostEqual(
                    aResult.maValue,
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase("spreadsheetengine_calendar_tests", rRow, "TIME mismatch");
            }
        }
        else if (rFunction == "WEEKDAY")
        {
            const auto aResult = dayOfWeek(
                aNullDate, static_cast<DateSerial>(
                               spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                static_cast<std::int16_t>(
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])));
            if (!aResult
                || aResult.maValue
                       != static_cast<int>(
                           spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase("spreadsheetengine_calendar_tests", rRow, "WEEKDAY mismatch");
            }
        }
        else if (rFunction == "WEEKNUM_OOO")
        {
            if (weeknumOOo(
                    aNullDate,
                    static_cast<DateSerial>(
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                    static_cast<std::int16_t>(
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])))
                != static_cast<int>(
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "WEEKNUM_OOO mismatch");
            }
        }
        else if (rFunction == "ISOWEEKNUM")
        {
            const auto oDate = parseDateSerialToken(rRow.maColumns[1]);
            if (!oDate
                || isoWeekOfYear(aNullDate, *oDate)
                       != static_cast<int>(
                           spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "ISOWEEKNUM mismatch");
            }
        }
        else if (rFunction == "EASTERSUNDAY")
        {
            const auto aResult = easterSundaySerial(
                aNullDate, static_cast<std::int16_t>(
                               spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])));
            const auto oExpectedSerial = parseDateSerialToken(rRow.maColumns[5]);
            if (!aResult || !oExpectedSerial
                || !almostEqual(aResult.maValue, static_cast<double>(*oExpectedSerial)))
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "EASTERSUNDAY mismatch");
            }
        }
        else if (rFunction == "DAYS360")
        {
            const auto oDate1 = parseDateSerialToken(rRow.maColumns[1]);
            const auto oDate2 = parseDateSerialToken(rRow.maColumns[2]);
            if (!oDate1 || !oDate2
                || !almostEqual(
                    diffDate360(aNullDate, *oDate1, *oDate2, false),
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase("spreadsheetengine_calendar_tests", rRow, "DAYS360 mismatch");
            }
        }
        else if (rFunction == "DATEDIF")
        {
            const auto oDate1 = parseDateSerialToken(rRow.maColumns[1]);
            const auto oDate2 = parseDateSerialToken(rRow.maColumns[2]);
            const auto aResult = oDate1 && oDate2
                                     ? dateDif(aNullDate, *oDate1, *oDate2,
                                           spreadsheetengine::standalone::test::decodeUtf8TestString(
                                               rRow.maColumns[3]))
                                     : spreadsheetengine::api::ValueResult<double>::failure(
                                           Error::IllegalArgument);
            if (!aResult
                || !almostEqual(
                    aResult.maValue,
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase("spreadsheetengine_calendar_tests", rRow, "DATEDIF mismatch");
            }
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_calendar_tests", rRow, "unknown calendar shared-case function");
        }
    }

    for (const auto& rRow :
         spreadsheetengine::standalone::test::loadSharedCaseRows("workday_cases.tsv"))
    {
        if (rRow.maColumns.size() < 7)
        {
            return failSharedCase(
                "spreadsheetengine_calendar_tests", rRow, "workday shared case column mismatch");
        }

        const auto& rFunction = rRow.maColumns[0];
        const auto eExpectedError
            = spreadsheetengine::standalone::test::parseExpectedError(rRow.maColumns[6]);

        if (rFunction == "WEEKENDMASK.DEFAULT")
        {
            if (maskToString(defaultWeekendMask()) != rRow.maColumns[5])
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "WEEKENDMASK.DEFAULT mismatch");
            }
        }
        else if (rFunction == "WEEKENDMASK.MS")
        {
            const auto aResult = weekendMaskFromMsSpec(
                spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[1]),
                spreadsheetengine::standalone::test::parseBool(rRow.maColumns[2]));
            if (eExpectedError != Error::None)
            {
                if (aResult || aResult.meError != eExpectedError)
                {
                    return failSharedCase(
                        "spreadsheetengine_calendar_tests", rRow,
                        "WEEKENDMASK.MS error mismatch");
                }
            }
            else if (!aResult || maskToString(aResult.maValue) != rRow.maColumns[5])
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "WEEKENDMASK.MS mismatch");
            }
        }
        else if (rFunction == "NETWORKDAYS")
        {
            const auto oHolidays = parseHolidayList(rRow.maColumns[3]);
            const auto aWeekendMask = weekendMaskFromMsSpec(
                spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[4]),
                false);
            if (!oHolidays || !aWeekendMask
                || countWorkdays(
                       static_cast<DateSerial>(
                           spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                       static_cast<DateSerial>(
                           spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                       *oHolidays, aWeekendMask.maValue)
                       != static_cast<DateSerial>(
                           spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "NETWORKDAYS mismatch");
            }
        }
        else if (rFunction == "WORKDAY")
        {
            const auto oHolidays = parseHolidayList(rRow.maColumns[3]);
            const auto aWeekendMask = weekendMaskFromMsSpec(
                spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[4]),
                true);
            if (!oHolidays || !aWeekendMask
                || advanceWorkday(
                       static_cast<DateSerial>(
                           spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                       static_cast<DateSerial>(
                           spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                       *oHolidays, aWeekendMask.maValue)
                       != static_cast<DateSerial>(
                           spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase(
                    "spreadsheetengine_calendar_tests", rRow, "WORKDAY mismatch");
            }
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_calendar_tests", rRow, "unknown workday shared-case function");
        }
    }

    std::cout << "spreadsheetengine calendar api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
