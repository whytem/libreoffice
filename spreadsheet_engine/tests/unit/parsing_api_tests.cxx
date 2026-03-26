/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <stdexcept>

#include <spreadsheetengine/api/Parsing.hxx>
#include <spreadsheetengine/runtime/InMemoryHost.hxx>

#include "SharedCaseSupport.hxx"
#include "TestSupport.hxx"

namespace
{

spreadsheetengine::api::NumberParseResult::Kind parseNumberKind(std::string_view rValue)
{
    using Kind = spreadsheetengine::api::NumberParseResult::Kind;
    if (rValue == "Number")
        return Kind::Number;
    if (rValue == "Date")
        return Kind::Date;
    if (rValue == "Time")
        return Kind::Time;
    if (rValue == "DateTime")
        return Kind::DateTime;

    throw std::runtime_error("unknown number parse kind");
}

}

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::NumberParseMode;
    using spreadsheetengine::api::parsing::dateValueFromText;
    using spreadsheetengine::api::parsing::timeValueFromText;
    using spreadsheetengine::api::parsing::valueFromText;
    using spreadsheetengine::core::host::InMemoryEvaluationHost;
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::decodeUtf8TestString;
    using spreadsheetengine::standalone::test::fail;
    using spreadsheetengine::standalone::test::loadSharedCaseRows;
    using spreadsheetengine::standalone::test::parseDouble;
    using spreadsheetengine::standalone::test::parseExpectedError;

    InMemoryEvaluationHost aHost;
    aHost.setParsedNumber(u"42.5", 42.5, 11);
    aHost.setParsedNumber(u"1954-07-20", 19925.0, 21,
        spreadsheetengine::api::NumberParseResult::Kind::Date);
    aHost.setParsedNumber(u"16:30:01", 59401.0 / 86400.0, 22,
        spreadsheetengine::api::NumberParseResult::Kind::Time);
    aHost.setParsedNumber(u"1954-07-20 16:30:01", 19925.0 + (59401.0 / 86400.0), 23,
        spreadsheetengine::api::NumberParseResult::Kind::DateTime);

    const auto aNumber = valueFromText(aHost, u"42.5");
    if (!aNumber || !almostEqual(aNumber.maValue, 42.5))
        return fail("spreadsheetengine_parsing_tests", "valueFromText() number mismatch");

    const auto aDate = dateValueFromText(aHost, u"1954-07-20");
    if (!aDate || !almostEqual(aDate.maValue, 19925.0))
        return fail("spreadsheetengine_parsing_tests", "dateValueFromText() date mismatch");

    const auto aTime = timeValueFromText(aHost, u"16:30:01");
    if (!aTime || !almostEqual(aTime.maValue, 59401.0 / 86400.0))
        return fail("spreadsheetengine_parsing_tests", "timeValueFromText() time mismatch");

    const auto aDateTimeDate = dateValueFromText(aHost, u"1954-07-20 16:30:01");
    const auto aDateTimeTime = timeValueFromText(aHost, u"1954-07-20 16:30:01");
    if (!aDateTimeDate || !almostEqual(aDateTimeDate.maValue, 19925.0) || !aDateTimeTime
        || !almostEqual(aDateTimeTime.maValue, 59401.0 / 86400.0))
    {
        return fail(
            "spreadsheetengine_parsing_tests", "date/time split for datetime mismatch");
    }

    const auto aWrongKindDate = dateValueFromText(aHost, u"42.5");
    if (aWrongKindDate || aWrongKindDate.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_parsing_tests", "DATEVALUE wrong-kind mismatch");
    }

    const auto aWrongKindTime = timeValueFromText(aHost, u"42.5");
    if (aWrongKindTime || aWrongKindTime.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_parsing_tests", "TIMEVALUE wrong-kind mismatch");
    }

    for (const auto& rRow : loadSharedCaseRows("locale_parsing_cases.tsv"))
    {
        if (rRow.maColumns.size() < 6)
            return failSharedCase(
                "spreadsheetengine_parsing_tests", rRow, "locale parsing column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const auto aInput = decodeUtf8TestString(rRow.maColumns[1]);
        const auto eExpectedError = parseExpectedError(rRow.maColumns[5]);

        InMemoryEvaluationHost aRowHost;
        if (!rRow.maColumns[2].empty() && !rRow.maColumns[3].empty())
        {
            const auto eKind = parseNumberKind(rRow.maColumns[3]);
            aRowHost.setParsedNumber(aInput, parseDouble(rRow.maColumns[2]), 0, eKind);
            if (rFunction == "TIMEVALUE")
            {
                aRowHost.setParsedNumber(
                    aInput, parseDouble(rRow.maColumns[2]), 0, eKind, NumberParseMode::LaxTime);
            }
        }

        auto checkResult = [&](const spreadsheetengine::api::ValueResult<double>& rResult)
            -> std::string {
            if (eExpectedError != Error::None)
            {
                if (rResult || rResult.meError != eExpectedError)
                {
                    return "locale parsing error mismatch";
                }
                return std::string();
            }

            if (!rResult || !almostEqual(rResult.maValue, parseDouble(rRow.maColumns[4])))
            {
                return "locale parsing value mismatch";
            }
            return std::string();
        };

        std::string aFailure;
        if (rFunction == "VALUE")
            aFailure = checkResult(valueFromText(aRowHost, aInput));
        else if (rFunction == "DATEVALUE")
            aFailure = checkResult(dateValueFromText(aRowHost, aInput));
        else if (rFunction == "TIMEVALUE")
            aFailure = checkResult(timeValueFromText(aRowHost, aInput));
        else
        {
            return failSharedCase(
                "spreadsheetengine_parsing_tests", rRow, "unknown locale parsing function");
        }

        if (!aFailure.empty())
            return failSharedCase("spreadsheetengine_parsing_tests", rRow, aFailure.c_str());
    }

    std::cout << "spreadsheetengine parsing api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
