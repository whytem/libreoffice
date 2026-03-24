/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cmath>
#include <iostream>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Numeral.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::RoundingMode;
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::fail;
    using namespace spreadsheetengine::api::math;
    using namespace spreadsheetengine::api::numeral;

    if (!almostEqual(abs(-7.25), 7.25))
        return fail("spreadsheetengine_math_tests", "abs() mismatch");

    if (!almostEqual(integerFloor(-1.2), -2.0))
        return fail("spreadsheetengine_math_tests", "integerFloor() mismatch");

    const auto aLog = logarithm(100.0, 10.0);
    if (!aLog || !almostEqual(aLog.maValue, 2.0))
        return fail("spreadsheetengine_math_tests", "logarithm() mismatch");

    const auto aBadLog = logarithm(-1.0, 10.0);
    if (aBadLog || aBadLog.meError != Error::IllegalArgument)
        return fail("spreadsheetengine_math_tests", "logarithm() domain handling mismatch");

    const auto aBitOr = bitOr(10.0, 6.0);
    if (!aBitOr || !almostEqual(aBitOr.maValue, 14.0))
        return fail("spreadsheetengine_math_tests", "bitOr() mismatch");

    const auto aBitXor = bitXor(10.0, 6.0);
    if (!aBitXor || !almostEqual(aBitXor.maValue, 12.0))
        return fail("spreadsheetengine_math_tests", "bitXor() mismatch");

    const auto aBitShift = bitLeftShift(3.0, 4.0);
    if (!aBitShift || !almostEqual(aBitShift.maValue, 48.0))
        return fail("spreadsheetengine_math_tests", "bitLeftShift() mismatch");

    if (!almostEqual(pi(), 3.14159265358979323846))
        return fail("spreadsheetengine_math_tests", "pi() mismatch");

    if (!almostEqual(cosine(0.0), 1.0))
        return fail("spreadsheetengine_math_tests", "cosine() mismatch");

    const auto aBadArcCosh = inverseHyperbolicCosine(0.5);
    if (aBadArcCosh || aBadArcCosh.meError != Error::Domain)
        return fail("spreadsheetengine_math_tests", "inverseHyperbolicCosine() domain handling mismatch");

    if (!almostEqual(roundToDecimals(1.2349, 3, RoundingMode::Corrected), 1.235))
        return fail("spreadsheetengine_math_tests", "roundToDecimals() mismatch");

    if (!almostEqual(roundToSignificantDigits(1234.0, 2.0), 1200.0))
        return fail("spreadsheetengine_math_tests", "roundToSignificantDigits() mismatch");

    const auto aFloorMs = floorMs(5.9, 2.0);
    if (!aFloorMs || !almostEqual(aFloorMs.maValue, 4.0))
        return fail("spreadsheetengine_math_tests", "floorMs() mismatch");

    if (!almostEqual(even(3.2), 4.0) || !almostEqual(odd(-2.2), -3.0))
        return fail("spreadsheetengine_math_tests", "even()/odd() mismatch");

    const double fPmt = payment(0.01, 12.0, 1000.0, 0.0, false);
    const auto aRate = solveRate(12.0, fPmt, 1000.0, 0.0, false, 0.05, true);
    if (!aRate || !aRate.mbConverged || !almostEqual(aRate.mfRate, 0.01))
        return fail("spreadsheetengine_math_tests", "solveRate() mismatch");

    if (!almostEqual(straightLineDepreciation(1000.0, 100.0, 9.0), 100.0))
        return fail("spreadsheetengine_math_tests", "straightLineDepreciation() mismatch");

    if (!almostEqual(sumOfYearsDepreciation(1000.0, 100.0, 9.0, 1.0), 180.0))
        return fail("spreadsheetengine_math_tests", "sumOfYearsDepreciation() mismatch");

    if (!almostEqual(effectiveAnnualRate(0.12, 12.0), std::pow(1.01, 12.0) - 1.0))
        return fail("spreadsheetengine_math_tests", "effectiveAnnualRate() mismatch");

    const auto aBase = toBase(255.0, 16.0, 4.0);
    if (!aBase || aBase.maValue != u"00FF")
        return fail("spreadsheetengine_math_tests", "toBase() mismatch");

    const auto aOverflow = toBase(8.0, 2.0, 70000.0);
    if (aOverflow || aOverflow.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_math_tests", "toBase() invalid-min-length handling mismatch");
    }

    const auto aDecimal = fromBase(u"FF", 16.0);
    if (!aDecimal || !almostEqual(aDecimal.maValue, 255.0))
        return fail("spreadsheetengine_math_tests", "fromBase() mismatch");

    const auto aRoman = toRoman(1999.0);
    if (!aRoman || aRoman.maValue != u"MCMXCIX")
        return fail("spreadsheetengine_math_tests", "toRoman() mismatch");

    const auto aArabic = fromRoman(u"mcmxcix");
    if (!aArabic || aArabic.maValue != 1999)
        return fail("spreadsheetengine_math_tests", "fromRoman() mismatch");

    std::cout << "spreadsheetengine math api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
