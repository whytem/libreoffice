/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Numeral.hxx>
#include <spreadsheetengine/api/Text.hxx>
#include <spreadsheetengine/api/Workday.hxx>
#include <spreadsheetengine/core/TextServices.hxx>

namespace
{

bool almostEqual(double fLeft, double fRight)
{
    const double fScale = std::max({ 1.0, std::fabs(fLeft), std::fabs(fRight) });
    return std::fabs(fLeft - fRight) <= (1.0e-9 * fScale);
}

int fail(const char* pMessage)
{
    std::cerr << "spreadsheetengine_tests: " << pMessage << '\n';
    return EXIT_FAILURE;
}

class AsciiCaseMappingService final : public spreadsheetengine::core::text::CaseMappingService
{
public:
    spreadsheetengine::api::String uppercase(
        spreadsheetengine::api::StringView rInput) const override
    {
        spreadsheetengine::api::String aResult(rInput);
        for (auto& c : aResult)
        {
            if (u'a' <= c && c <= u'z')
                c -= (u'a' - u'A');
        }
        return aResult;
    }

    spreadsheetengine::api::String lowercase(
        spreadsheetengine::api::StringView rInput) const override
    {
        spreadsheetengine::api::String aResult(rInput);
        for (auto& c : aResult)
        {
            if (u'A' <= c && c <= u'Z')
                c += (u'a' - u'A');
        }
        return aResult;
    }

    bool isLetter(char32_t nCodePoint) const override
    {
        return (U'A' <= nCodePoint && nCodePoint <= U'Z')
               || (U'a' <= nCodePoint && nCodePoint <= U'z');
    }
};

class MockWidthConversionService final : public spreadsheetengine::core::text::WidthConversionService
{
public:
    spreadsheetengine::api::String toHalfWidth(
        spreadsheetengine::api::StringView rInput) const override
    {
        spreadsheetengine::api::String aResult(rInput);
        for (auto& c : aResult)
        {
            if (c == u'Ａ')
                c = u'A';
        }
        return aResult;
    }

    spreadsheetengine::api::String toFullWidth(
        spreadsheetengine::api::StringView rInput) const override
    {
        spreadsheetengine::api::String aResult(rInput);
        for (auto& c : aResult)
        {
            if (c == u'A')
                c = u'Ａ';
        }
        return aResult;
    }
};

class Latin1EncodingService final : public spreadsheetengine::core::text::SingleByteEncodingService
{
public:
    sal_Int32 encodeFirstCharacter(spreadsheetengine::api::StringView rInput) const override
    {
        if (rInput.empty())
            return 0;
        return static_cast<unsigned char>(rInput.front() & 0x00FF);
    }

    std::optional<spreadsheetengine::api::String> decodeSingleByte(
        unsigned char nValue) const override
    {
        return spreadsheetengine::api::String(1, static_cast<char16_t>(nValue));
    }
};

}

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::RoundingMode;
    using spreadsheetengine::api::DateParts;
    using spreadsheetengine::api::DateSerial;
    using namespace spreadsheetengine::api::calendar;
    using namespace spreadsheetengine::api::math;
    using namespace spreadsheetengine::api::numeral;
    using namespace spreadsheetengine::api::text;
    using namespace spreadsheetengine::api::workday;

    if (!almostEqual(abs(-7.25), 7.25))
        return fail("abs() mismatch");

    if (!almostEqual(integerFloor(-1.2), -2.0))
        return fail("integerFloor() mismatch");

    const auto aLog = logarithm(100.0, 10.0);
    if (!aLog || !almostEqual(aLog.maValue, 2.0))
        return fail("logarithm() mismatch");

    const auto aBadLog = logarithm(-1.0, 10.0);
    if (aBadLog || aBadLog.meError != Error::IllegalArgument)
        return fail("logarithm() domain handling mismatch");

    const auto aBitOr = bitOr(10.0, 6.0);
    if (!aBitOr || !almostEqual(aBitOr.maValue, 14.0))
        return fail("bitOr() mismatch");

    const auto aBitXor = bitXor(10.0, 6.0);
    if (!aBitXor || !almostEqual(aBitXor.maValue, 12.0))
        return fail("bitXor() mismatch");

    const auto aBitShift = bitLeftShift(3.0, 4.0);
    if (!aBitShift || !almostEqual(aBitShift.maValue, 48.0))
        return fail("bitLeftShift() mismatch");

    if (!almostEqual(pi(), 3.14159265358979323846))
        return fail("pi() mismatch");

    if (!almostEqual(cosine(0.0), 1.0))
        return fail("cosine() mismatch");

    const auto aBadArcCosh = inverseHyperbolicCosine(0.5);
    if (aBadArcCosh || aBadArcCosh.meError != Error::Domain)
        return fail("inverseHyperbolicCosine() domain handling mismatch");

    if (!almostEqual(roundToDecimals(1.2349, 3, RoundingMode::Corrected), 1.235))
        return fail("roundToDecimals() mismatch");

    if (!almostEqual(roundToSignificantDigits(1234.0, 2.0), 1200.0))
        return fail("roundToSignificantDigits() mismatch");

    const auto aFloorMs = floorMs(5.9, 2.0);
    if (!aFloorMs || !almostEqual(aFloorMs.maValue, 4.0))
        return fail("floorMs() mismatch");

    if (!almostEqual(even(3.2), 4.0) || !almostEqual(odd(-2.2), -3.0))
        return fail("even()/odd() mismatch");

    const double fPmt = payment(0.01, 12.0, 1000.0, 0.0, false);
    const auto aRate = solveRate(12.0, fPmt, 1000.0, 0.0, false, 0.05, true);
    if (!aRate || !aRate.mbConverged || !almostEqual(aRate.mfRate, 0.01))
        return fail("solveRate() mismatch");

    if (!almostEqual(straightLineDepreciation(1000.0, 100.0, 9.0), 100.0))
        return fail("straightLineDepreciation() mismatch");

    if (!almostEqual(sumOfYearsDepreciation(1000.0, 100.0, 9.0, 1.0), 180.0))
        return fail("sumOfYearsDepreciation() mismatch");

    if (!almostEqual(effectiveAnnualRate(0.12, 12.0), std::pow(1.01, 12.0) - 1.0))
        return fail("effectiveAnnualRate() mismatch");

    const auto aBase = toBase(255.0, 16.0, 4.0);
    if (!aBase || aBase.maValue != u"00FF")
        return fail("toBase() mismatch");

    const auto aOverflow = toBase(8.0, 2.0, 70000.0);
    if (aOverflow || aOverflow.meError != Error::IllegalArgument)
        return fail("toBase() invalid-min-length handling mismatch");

    const auto aDecimal = fromBase(u"FF", 16.0);
    if (!aDecimal || !almostEqual(aDecimal.maValue, 255.0))
        return fail("fromBase() mismatch");

    const auto aRoman = toRoman(1999.0);
    if (!aRoman || aRoman.maValue != u"MCMXCIX")
        return fail("toRoman() mismatch");

    const auto aArabic = fromRoman(u"mcmxcix");
    if (!aArabic || aArabic.maValue != 1999)
        return fail("fromRoman() mismatch");

    const auto aDefaultWeekendMask = defaultWeekendMask();
    if (aDefaultWeekendMask[0] || aDefaultWeekendMask[4] || !aDefaultWeekendMask[5]
        || !aDefaultWeekendMask[6])
    {
        return fail("defaultWeekendMask() mismatch");
    }

    const auto aSequenceWeekendMask
        = weekendMaskFromSequence({ 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 });
    if (!aSequenceWeekendMask || aSequenceWeekendMask.maValue != aDefaultWeekendMask)
        return fail("weekendMaskFromSequence() mismatch");

    const auto aMsWeekendMask = weekendMaskFromMsSpec(u"0000011", false);
    if (!aMsWeekendMask || aMsWeekendMask.maValue != aDefaultWeekendMask)
        return fail("weekendMaskFromMsSpec() mismatch");

    const auto aBadWeekendMask = weekendMaskFromMsSpec(u"1111111", true);
    if (aBadWeekendMask || aBadWeekendMask.meError != Error::IllegalArgument)
        return fail("weekendMaskFromMsSpec() invalid handling mismatch");

    const std::vector<DateSerial> aHolidaySerials { 8 };
    if (countWorkdays(1, 10, aHolidaySerials, aDefaultWeekendMask) != 7)
        return fail("countWorkdays() mismatch");

    if (advanceWorkday(5, 1, aHolidaySerials, aDefaultWeekendMask) != 9)
        return fail("advanceWorkday() mismatch");

    const DateParts aNullDate { 1899, 12, 30 };
    const auto aJan1 = makeDateSerial(aNullDate, 1900, 1, 1, true);
    if (!aJan1 || !almostEqual(aJan1.maValue, 2.0))
        return fail("makeDateSerial() mismatch");

    if (!almostEqual(yearFromSerial(aNullDate, 0), 1899.0)
        || !almostEqual(monthFromSerial(aNullDate, 0), 12.0))
    {
        return fail("yearFromSerial()/monthFromSerial() mismatch");
    }

    const auto aDay = dayFromSerial(aNullDate, 0);
    if (!aDay || !almostEqual(aDay.maValue, 30.0))
        return fail("dayFromSerial() mismatch");

    const auto aTime = makeTimeSerial(1.0, 30.0, 0.0);
    if (!aTime || !almostEqual(aTime.maValue, (1.5 / 24.0)))
        return fail("makeTimeSerial() mismatch");

    if (!almostEqual(hourFromTimeValue(1.5 / 24.0), 1.0)
        || !almostEqual(minuteFromTimeValue(1.5 / 24.0), 30.0)
        || !almostEqual(secondFromTimeValue(1.5 / 24.0), 0.0))
    {
        return fail("time extraction mismatch");
    }

    const auto aWeekday = dayOfWeek(aNullDate, 2, 2);
    if (!aWeekday || aWeekday.maValue != 1)
        return fail("dayOfWeek() mismatch");

    if (weeknumOOo(aNullDate, 2, 1) != 1)
        return fail("weeknumOOo() mismatch");

    const auto aJan1_2020 = makeDateSerial(aNullDate, 2020, 1, 1, true);
    if (!aJan1_2020)
        return fail("makeDateSerial() 2020-01-01 mismatch");
    const auto aIsoWeek = weekOfYear(aNullDate, static_cast<DateSerial>(aJan1_2020.maValue), 21);
    if (!aIsoWeek || aIsoWeek.maValue != 1
        || isoWeekOfYear(aNullDate, static_cast<DateSerial>(aJan1_2020.maValue)) != 1)
        return fail("weekOfYear()/isoWeekOfYear() mismatch");

    const auto aEaster = easterSundaySerial(aNullDate, 2024);
    if (!aEaster)
        return fail("easterSundaySerial() mismatch");
    const auto aEasterMonth = monthFromSerial(aNullDate, static_cast<DateSerial>(aEaster.maValue));
    const auto aEasterDay = dayFromSerial(aNullDate, static_cast<DateSerial>(aEaster.maValue));
    if (!almostEqual(aEasterMonth, 3.0) || !aEasterDay || !almostEqual(aEasterDay.maValue, 31.0))
        return fail("easterSundaySerial() round-trip mismatch");

    const auto aFeb1 = makeDateSerial(aNullDate, 2020, 2, 1, true);
    if (!aFeb1 || !almostEqual(diffDate360(aNullDate, static_cast<DateSerial>(aJan1_2020.maValue),
                           static_cast<DateSerial>(aFeb1.maValue), false), 30.0))
    {
        return fail("diffDate360() mismatch");
    }

    const auto aDate2 = makeDateSerial(aNullDate, 2021, 3, 15, true);
    if (!aDate2)
        return fail("makeDateSerial() second date mismatch");
    const auto aDateDifY = dateDif(
        aNullDate, static_cast<DateSerial>(aJan1_2020.maValue), static_cast<DateSerial>(aDate2.maValue), u"y");
    const auto aDateDifYM = dateDif(
        aNullDate, static_cast<DateSerial>(aJan1_2020.maValue), static_cast<DateSerial>(aDate2.maValue), u"ym");
    if (!aDateDifY || !almostEqual(aDateDifY.maValue, 1.0)
        || !aDateDifYM || !almostEqual(aDateDifYM.maValue, 2.0))
    {
        return fail("dateDif() mismatch");
    }

    const AsciiCaseMappingService aCaseService;
    const MockWidthConversionService aWidthService;
    const Latin1EncodingService aEncodingService;

    if (trimRepeatedSpaces(u"  a   b  ") != u"a b")
        return fail("trimRepeatedSpaces() mismatch");

    if (countCodePoints(u"A\u00E9\U0001F600") != 3)
        return fail("countCodePoints() mismatch");

    const auto aNumberValue = parseNumberValue(u"1,234.5%", std::optional<spreadsheetengine::api::String>(u"."), std::optional<spreadsheetengine::api::String>(u","), false);
    if (!aNumberValue || !almostEqual(aNumberValue.maValue, 12.345))
        return fail("parseNumberValue() mismatch");

    const auto aRejectedNumberValue = parseNumberValue(u"3.5%", std::nullopt, std::nullopt, false);
    if (aRejectedNumberValue || aRejectedNumberValue.meError != Error::NoValue)
        return fail("parseNumberValue() decimal-separator handling mismatch");

    if (cleanPrintable(u"A\u0001B") != u"AB")
        return fail("cleanPrintable() mismatch");

    if (spreadsheetengine::api::text::codeFromText(aEncodingService, u"Az") != 65)
        return fail("codeFromText() mismatch");

    const auto aChar = spreadsheetengine::api::text::charFromValue(aEncodingService, 65.0);
    if (!aChar || aChar.maValue != u"A")
        return fail("charFromValue() mismatch");

    const auto aUnicode = unicodeFromText(u"\U0001F600");
    if (!aUnicode || !almostEqual(aUnicode.maValue, 128512.0))
        return fail("unicodeFromText() mismatch");

    const auto aUnichar = unicharFromCodePoint(0x1F600);
    if (!aUnichar || aUnichar.maValue != u"\U0001F600")
        return fail("unicharFromCodePoint() mismatch");

    if (spreadsheetengine::api::text::uppercase(aCaseService, u"abc") != u"ABC"
        || spreadsheetengine::api::text::lowercase(aCaseService, u"ABC") != u"abc"
        || spreadsheetengine::api::text::propercase(aCaseService, u"hello world") != u"Hello World")
    {
        return fail("case mapping mismatch");
    }

    if (spreadsheetengine::api::text::convertIntoFullWidth(aWidthService, u"A") != u"Ａ"
        || spreadsheetengine::api::text::convertIntoHalfWidth(aWidthService, u"Ａ") != u"A")
    {
        return fail("width conversion mismatch");
    }

    std::cout << "spreadsheetengine api tests passed\n";
    return EXIT_SUCCESS;
}
