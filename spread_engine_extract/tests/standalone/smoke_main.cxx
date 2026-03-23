/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/core/Phase0.hxx>

namespace
{

bool almostEqual(double fLeft, double fRight)
{
    const double fScale = std::max({ 1.0, std::fabs(fLeft), std::fabs(fRight) });
    return std::fabs(fLeft - fRight) <= (1.0e-9 * fScale);
}

}

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::math::Sign;
    using spreadsheetengine::api::math::arcTan2;
    using spreadsheetengine::api::math::bitAnd;
    using spreadsheetengine::api::math::bitRightShift;
    using spreadsheetengine::api::math::ceiling;
    using spreadsheetengine::api::math::degrees;
    using spreadsheetengine::api::math::futureValue;
    using spreadsheetengine::api::math::modulo;
    using spreadsheetengine::api::math::payment;
    using spreadsheetengine::api::math::presentValue;
    using spreadsheetengine::api::math::radians;
    using spreadsheetengine::api::math::roundToDecimals;
    using spreadsheetengine::api::math::sign;
    using spreadsheetengine::api::math::squareRoot;
    using spreadsheetengine::core::isLibraryLinked;

    auto fail = [](const char* pMessage) {
        std::cerr << "spreadsheetengine_smoke: " << pMessage << '\n';
        return EXIT_FAILURE;
    };

    if (!isLibraryLinked())
        return fail("Phase0 library probe failed");

    if (sign(-42.0) != Sign::Negative || sign(0.0) != Sign::Zero
        || sign(7.0) != Sign::Positive)
    {
        return fail("sign() returned unexpected results");
    }

    const auto aMod = modulo(10.0, 3.0);
    if (!aMod || !almostEqual(aMod.maValue, 1.0))
        return fail("modulo() returned unexpected result");

    const auto aBitAnd = bitAnd(6.0, 3.0);
    if (!aBitAnd || !almostEqual(aBitAnd.maValue, 2.0))
        return fail("bitAnd() returned unexpected result");

    const auto aBitShift = bitRightShift(16.0, 2.0);
    if (!aBitShift || !almostEqual(aBitShift.maValue, 4.0))
        return fail("bitRightShift() returned unexpected result");

    if (!almostEqual(arcTan2(0.0, 1.0), 0.0))
        return fail("arcTan2() returned unexpected result");

    if (!almostEqual(degrees(radians(45.0)), 45.0))
        return fail("degree/radian conversion returned unexpected result");

    if (!almostEqual(roundToDecimals(12.345, 2, spreadsheetengine::api::RoundingMode::Corrected), 12.35))
        return fail("roundToDecimals() returned unexpected result");

    const auto aCeiling = ceiling(4.1, 2.0, true, false);
    if (!aCeiling || !almostEqual(aCeiling.maValue, 6.0))
        return fail("ceiling() returned unexpected result");

    const auto aSqrt = squareRoot(81.0);
    if (!aSqrt || !almostEqual(aSqrt.maValue, 9.0))
        return fail("squareRoot() returned unexpected result");

    const auto aNegativeSqrt = squareRoot(-1.0);
    if (aNegativeSqrt || aNegativeSqrt.meError != Error::Domain)
        return fail("squareRoot() should reject negative input with Domain error");

    const double fPmt = payment(0.01, 12.0, 1000.0, 0.0, false);
    const double fFv = futureValue(0.01, 12.0, fPmt, 1000.0, false);
    if (!almostEqual(fFv, 0.0))
        return fail("futureValue()/payment() returned inconsistent result");

    const double fPv = presentValue(0.01, 12.0, fPmt, 0.0, false);
    if (!almostEqual(fPv, 1000.0))
        return fail("presentValue()/payment() returned inconsistent result");

    std::cout << "spreadsheetengine smoke test passed\n";
    return EXIT_SUCCESS;
}
