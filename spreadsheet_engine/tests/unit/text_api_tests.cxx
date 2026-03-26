/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Text.hxx>

#include "SharedCaseSupport.hxx"
#include "TestSupport.hxx"
#include "TextTestDoubles.hxx"

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::standalone::test::AsciiCaseMappingService;
    using spreadsheetengine::standalone::test::Latin1EncodingService;
    using spreadsheetengine::standalone::test::MockWidthConversionService;
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::fail;
    const AsciiCaseMappingService aCaseService;
    const MockWidthConversionService aWidthService;
    const Latin1EncodingService aEncodingService;

    if (spreadsheetengine::api::text::trimRepeatedSpaces(u"  a   b  ") != u"a b")
        return fail("spreadsheetengine_text_tests", "trimRepeatedSpaces() mismatch");

    if (spreadsheetengine::api::text::countCodePoints(u"A\u00E9\U0001F600") != 3)
        return fail("spreadsheetengine_text_tests", "countCodePoints() mismatch");

    const auto aNumberValue = spreadsheetengine::api::text::parseNumberValue(u"1,234.5%",
        std::optional<spreadsheetengine::api::String>(u"."),
        std::optional<spreadsheetengine::api::String>(u","), false);
    if (!aNumberValue || !almostEqual(aNumberValue.maValue, 12.345))
        return fail("spreadsheetengine_text_tests", "parseNumberValue() mismatch");

    const auto aRejectedNumberValue
        = spreadsheetengine::api::text::parseNumberValue(u"3.5%", std::nullopt, std::nullopt, false);
    if (aRejectedNumberValue || aRejectedNumberValue.meError != Error::NoValue)
    {
        return fail("spreadsheetengine_text_tests",
            "parseNumberValue() decimal-separator handling mismatch");
    }

    if (spreadsheetengine::api::text::cleanPrintable(u"A\u0001B") != u"AB")
        return fail("spreadsheetengine_text_tests", "cleanPrintable() mismatch");

    if (spreadsheetengine::api::text::codeFromText(aEncodingService, u"Az") != 65)
        return fail("spreadsheetengine_text_tests", "codeFromText() mismatch");

    const auto aChar = spreadsheetengine::api::text::charFromValue(aEncodingService, 65.0);
    if (!aChar || aChar.maValue != u"A")
        return fail("spreadsheetengine_text_tests", "charFromValue() mismatch");

    const auto aUnicode = spreadsheetengine::api::text::unicodeFromText(u"\U0001F600");
    if (!aUnicode || !almostEqual(aUnicode.maValue, 128512.0))
        return fail("spreadsheetengine_text_tests", "unicodeFromText() mismatch");

    const auto aUnichar = spreadsheetengine::api::text::unicharFromCodePoint(0x1F600);
    if (!aUnichar || aUnichar.maValue != u"\U0001F600")
        return fail("spreadsheetengine_text_tests", "unicharFromCodePoint() mismatch");

    if (spreadsheetengine::api::text::uppercase(aCaseService, u"abc") != u"ABC"
        || spreadsheetengine::api::text::lowercase(aCaseService, u"ABC") != u"abc"
        || spreadsheetengine::api::text::propercase(aCaseService, u"hello world") != u"Hello World")
    {
        return fail("spreadsheetengine_text_tests", "case mapping mismatch");
    }

    if (spreadsheetengine::api::text::convertIntoFullWidth(aWidthService, u"A") != u"Ａ"
        || spreadsheetengine::api::text::convertIntoHalfWidth(aWidthService, u"Ａ") != u"A")
    {
        return fail("spreadsheetengine_text_tests", "width conversion mismatch");
    }

    for (const auto& rRow :
         spreadsheetengine::standalone::test::loadSharedCaseRows("text_cases.tsv"))
    {
        if (rRow.maColumns.size() < 7)
            return failSharedCase(
                "spreadsheetengine_text_tests", rRow, "text shared case column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const auto aInputA = spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[1]);
        const auto aInputB = spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[2]);
        const auto aInputC = spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[3]);
        const auto aExpected
            = spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[5]);
        const auto eExpectedError
            = spreadsheetengine::standalone::test::parseExpectedError(rRow.maColumns[6]);

        if (rFunction == "TRIM")
        {
            if (spreadsheetengine::api::text::trimRepeatedSpaces(aInputA) != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "TRIM mismatch");
        }
        else if (rFunction == "LEN")
        {
            if (spreadsheetengine::api::text::countCodePoints(aInputA)
                != static_cast<sal_Int32>(
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase("spreadsheetengine_text_tests", rRow, "LEN mismatch");
            }
        }
        else if (rFunction == "NUMBERVALUE")
        {
            const auto aResult = spreadsheetengine::api::text::parseNumberValue(
                aInputA,
                rRow.maColumns[2].empty() ? std::nullopt
                                          : std::optional<spreadsheetengine::api::String>(aInputB),
                rRow.maColumns[3].empty() ? std::nullopt
                                          : std::optional<spreadsheetengine::api::String>(aInputC),
                false);
            if (eExpectedError != Error::None)
            {
                if (aResult || aResult.meError != eExpectedError)
                {
                    return failSharedCase(
                        "spreadsheetengine_text_tests", rRow, "NUMBERVALUE error mismatch");
                }
            }
            else if (!aResult
                     || !almostEqual(
                         aResult.maValue,
                         spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase(
                    "spreadsheetengine_text_tests", rRow, "NUMBERVALUE value mismatch");
            }
        }
        else if (rFunction == "CLEAN")
        {
            if (spreadsheetengine::api::text::cleanPrintable(aInputA) != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "CLEAN mismatch");
        }
        else if (rFunction == "CODE")
        {
            if (spreadsheetengine::api::text::codeFromText(aEncodingService, aInputA)
                != static_cast<sal_Int32>(
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase("spreadsheetengine_text_tests", rRow, "CODE mismatch");
            }
        }
        else if (rFunction == "CHAR")
        {
            const auto aResult = spreadsheetengine::api::text::charFromValue(
                aEncodingService,
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]));
            if (!aResult || aResult.maValue != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "CHAR mismatch");
        }
        else if (rFunction == "UNICODE")
        {
            const auto aResult = spreadsheetengine::api::text::unicodeFromText(aInputA);
            if (!aResult
                || !almostEqual(
                    aResult.maValue,
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[5])))
            {
                return failSharedCase("spreadsheetengine_text_tests", rRow, "UNICODE mismatch");
            }
        }
        else if (rFunction == "UNICHAR")
        {
            const auto aResult = spreadsheetengine::api::text::unicharFromCodePoint(
                static_cast<sal_uInt32>(
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])));
            if (!aResult || aResult.maValue != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "UNICHAR mismatch");
        }
        else if (rFunction == "UPPER")
        {
            if (spreadsheetengine::api::text::uppercase(aCaseService, aInputA) != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "UPPER mismatch");
        }
        else if (rFunction == "LOWER")
        {
            if (spreadsheetengine::api::text::lowercase(aCaseService, aInputA) != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "LOWER mismatch");
        }
        else if (rFunction == "PROPER")
        {
            if (spreadsheetengine::api::text::propercase(aCaseService, aInputA) != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "PROPER mismatch");
        }
        else if (rFunction == "ASC")
        {
            if (spreadsheetengine::api::text::convertIntoHalfWidth(aWidthService, aInputA) != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "ASC mismatch");
        }
        else if (rFunction == "JIS")
        {
            if (spreadsheetengine::api::text::convertIntoFullWidth(aWidthService, aInputA) != aExpected)
                return failSharedCase("spreadsheetengine_text_tests", rRow, "JIS mismatch");
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_text_tests", rRow, "unknown text shared-case function");
        }
    }

    std::cout << "spreadsheetengine text api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
