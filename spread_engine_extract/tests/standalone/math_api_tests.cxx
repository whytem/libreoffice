/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Math.hxx>

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

}

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::RoundingMode;
    using namespace spreadsheetengine::api::math;

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

    std::cout << "spreadsheetengine api tests passed\n";
    return EXIT_SUCCESS;
}
