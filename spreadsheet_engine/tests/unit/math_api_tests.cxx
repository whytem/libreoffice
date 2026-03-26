/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cmath>
#include <iostream>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Numeral.hxx>

#include "SharedCaseSupport.hxx"
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

    for (const auto& rRow : spreadsheetengine::standalone::test::loadSharedCaseRows(
             "math_scalar_cases.tsv"))
    {
        if (rRow.maColumns.size() < 6)
            return failSharedCase(
                "spreadsheetengine_math_tests", rRow, "math shared case column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const auto eExpectedError
            = spreadsheetengine::standalone::test::parseExpectedError(rRow.maColumns[5]);

        auto checkValueResult = [&](const auto& rResult, const char* pMismatch) -> int {
            if (eExpectedError != Error::None)
            {
                if (rResult || rResult.meError != eExpectedError)
                    return failSharedCase("spreadsheetengine_math_tests", rRow, pMismatch);
            }
            else if (!rResult
                     || !almostEqual(
                         rResult.maValue,
                         spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4])))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, pMismatch);
            }
            return EXIT_SUCCESS;
        };

        if (rFunction == "MOD")
        {
            if (const int nFailure = checkValueResult(
                    modulo(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "MOD mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "LN")
        {
            if (const int nFailure = checkValueResult(
                    naturalLogarithm(
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                    "LN mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "LOG10")
        {
            if (const int nFailure = checkValueResult(
                    logarithmBase10(
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                    "LOG10 mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "LOG")
        {
            if (const int nFailure = checkValueResult(
                    logarithm(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "LOG mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "SQRT")
        {
            if (const int nFailure = checkValueResult(
                    squareRoot(
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                    "SQRT mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "BITAND")
        {
            if (const int nFailure = checkValueResult(
                    bitAnd(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "BITAND mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "BITOR")
        {
            if (const int nFailure = checkValueResult(
                    bitOr(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "BITOR mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "BITXOR")
        {
            if (const int nFailure = checkValueResult(
                    bitXor(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "BITXOR mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "BITLSHIFT")
        {
            if (const int nFailure = checkValueResult(
                    bitLeftShift(
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "BITLSHIFT mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "BITRSHIFT")
        {
            if (const int nFailure = checkValueResult(
                    bitRightShift(
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "BITRSHIFT mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "CEILING.MATH")
        {
            if (const int nFailure = checkValueResult(
                    ceilingMs(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "CEILING.MATH mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "FLOOR.MATH")
        {
            if (const int nFailure = checkValueResult(
                    floorMs(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                        spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    "FLOOR.MATH mismatch"))
            {
                return nFailure;
            }
        }
        else if (rFunction == "EVEN")
        {
            if (eExpectedError != Error::None
                || !almostEqual(even(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4])))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "EVEN mismatch");
            }
        }
        else if (rFunction == "ODD")
        {
            if (eExpectedError != Error::None
                || !almostEqual(odd(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1])),
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4])))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "ODD mismatch");
            }
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_math_tests", rRow, "unknown math shared-case function");
        }
    }

    for (const auto& rRow : spreadsheetengine::standalone::test::loadSharedCaseRows(
             "financial_cases.tsv"))
    {
        if (rRow.maColumns.size() < 9)
            return failSharedCase(
                "spreadsheetengine_math_tests", rRow, "financial shared case column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const double fExpected = spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[7]);

        if (rFunction == "PMT")
        {
            if (!almostEqual(payment(spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[3]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4]),
                                 spreadsheetengine::standalone::test::parseBool(rRow.maColumns[5])),
                    fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "PMT mismatch");
            }
        }
        else if (rFunction == "FV")
        {
            if (!almostEqual(futureValue(
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[3]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4]),
                                 spreadsheetengine::standalone::test::parseBool(rRow.maColumns[5])),
                    fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "FV mismatch");
            }
        }
        else if (rFunction == "PV")
        {
            if (!almostEqual(presentValue(
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[3]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4]),
                                 spreadsheetengine::standalone::test::parseBool(rRow.maColumns[5])),
                    fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "PV mismatch");
            }
        }
        else if (rFunction == "EFFECT")
        {
            if (!almostEqual(effectiveAnnualRate(
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "EFFECT mismatch");
            }
        }
        else if (rFunction == "NOMINAL")
        {
            if (!almostEqual(nominalAnnualRate(
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2])),
                    fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "NOMINAL mismatch");
            }
        }
        else if (rFunction == "SLN")
        {
            if (!almostEqual(straightLineDepreciation(
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[3])),
                    fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "SLN mismatch");
            }
        }
        else if (rFunction == "SYD")
        {
            if (!almostEqual(sumOfYearsDepreciation(
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[3]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4])),
                    fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "SYD mismatch");
            }
        }
        else if (rFunction == "RRI")
        {
            if (!almostEqual(growthRateOverPeriods(
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                                 spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[3])),
                    fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "RRI mismatch");
            }
        }
        else if (rFunction == "RATE")
        {
            const auto aRateResult = solveRate(
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[3]),
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4]),
                spreadsheetengine::standalone::test::parseBool(rRow.maColumns[5]),
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[6]), true);
            if (!aRateResult || !aRateResult.mbConverged || !almostEqual(aRateResult.mfRate, fExpected))
            {
                return failSharedCase("spreadsheetengine_math_tests", rRow, "RATE mismatch");
            }
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_math_tests", rRow, "unknown financial shared-case function");
        }
    }

    for (const auto& rRow : spreadsheetengine::standalone::test::loadSharedCaseRows(
             "numeral_conversion_cases.tsv"))
    {
        if (rRow.maColumns.size() < 6)
            return failSharedCase(
                "spreadsheetengine_math_tests", rRow, "numeral shared case column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const auto eExpectedError
            = spreadsheetengine::standalone::test::parseExpectedError(rRow.maColumns[5]);

        if (rFunction == "BASE")
        {
            const auto aResult = toBase(
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]),
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]),
                rRow.maColumns[3].empty()
                    ? std::nullopt
                    : std::optional<double>(spreadsheetengine::standalone::test::parseDouble(
                          rRow.maColumns[3])));
            if (eExpectedError != Error::None)
            {
                if (aResult || aResult.meError != eExpectedError)
                {
                    return failSharedCase(
                        "spreadsheetengine_math_tests", rRow, "BASE error mismatch");
                }
            }
            else if (!aResult
                     || aResult.maValue
                            != spreadsheetengine::standalone::test::decodeUtf8TestString(
                                rRow.maColumns[4]))
            {
                return failSharedCase(
                    "spreadsheetengine_math_tests", rRow, "BASE value mismatch");
            }
        }
        else if (rFunction == "DECIMAL")
        {
            const auto aResult = fromBase(
                spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[1]),
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[2]));
            if (eExpectedError != Error::None)
            {
                if (aResult || aResult.meError != eExpectedError)
                {
                    return failSharedCase(
                        "spreadsheetengine_math_tests", rRow, "DECIMAL error mismatch");
                }
            }
            else if (!aResult
                     || !almostEqual(
                         aResult.maValue,
                         spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4])))
            {
                return failSharedCase(
                    "spreadsheetengine_math_tests", rRow, "DECIMAL value mismatch");
            }
        }
        else if (rFunction == "ROMAN")
        {
            const auto aResult = toRoman(
                spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[1]));
            if (!aResult
                || aResult.maValue
                       != spreadsheetengine::standalone::test::decodeUtf8TestString(
                           rRow.maColumns[4]))
            {
                return failSharedCase(
                    "spreadsheetengine_math_tests", rRow, "ROMAN value mismatch");
            }
        }
        else if (rFunction == "ARABIC")
        {
            const auto aResult = fromRoman(
                spreadsheetengine::standalone::test::decodeUtf8TestString(rRow.maColumns[1]));
            if (!aResult
                || !almostEqual(
                    static_cast<double>(aResult.maValue),
                    spreadsheetengine::standalone::test::parseDouble(rRow.maColumns[4])))
            {
                return failSharedCase(
                    "spreadsheetengine_math_tests", rRow, "ARABIC value mismatch");
            }
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_math_tests", rRow, "unknown numeral shared-case function");
        }
    }

    std::cout << "spreadsheetengine math api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
