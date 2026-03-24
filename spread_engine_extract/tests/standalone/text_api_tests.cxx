/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Text.hxx>

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

    std::cout << "spreadsheetengine text api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
