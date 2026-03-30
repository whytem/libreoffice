/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/MathFunctionRuntime.hxx>

#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>

#include <algorithm>
#include <cmath>
#include <limits>

#include <rtl/math.hxx>

namespace spreadsheetengine::core::math
{
namespace
{
[[nodiscard]] double binomialCoefficient(double fN, double fK)
{
    if (fN < fK)
        return 0.0;
    if (fK == 0.0)
        return 1.0;

    double fValue = fN / fK;
    fN -= 1.0;
    fK -= 1.0;
    while (fK > 0.0)
    {
        fValue *= fN / fK;
        fK -= 1.0;
        fN -= 1.0;
    }
    return fValue;
}

[[nodiscard]] double roundMagnitudeDirectional(
    double fValue, sal_Int32 nDecimals, api::RoundingMode eMode)
{
    double fRoundedMagnitude = api::math::roundToDecimals(std::abs(fValue), nDecimals, eMode);

    const double fScale = std::pow(10.0, static_cast<double>(std::abs(nDecimals)));
    if (!std::isfinite(fScale) || fScale == 0.0)
        return fValue;

    const double fMagnitude = std::abs(fValue);
    const double fScaled = nDecimals >= 0 ? fMagnitude * fScale : fMagnitude / fScale;
    if (nDecimals < 12)
    {
        const double fRoundedInteger = ::rtl::math::round(fScaled);
        if (std::abs(fScaled - fRoundedInteger) <= 1e-12)
        {
            fRoundedMagnitude
                = nDecimals >= 0 ? fRoundedInteger / fScale : fRoundedInteger * fScale;
        }
    }

    return std::signbit(fValue) ? -fRoundedMagnitude : fRoundedMagnitude;
}

[[nodiscard]] api::ValueResult<double> makeFiniteResult(double fValue)
{
    if (!std::isfinite(fValue))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(fValue);
}

} // namespace

api::ValueResult<double> evaluateRoundValue(
    double fValue, sal_Int32 nDecimals, api::RoundingMode eMode, bool bDirectional)
{
    if (bDirectional)
        return api::ValueResult<double>::success(roundMagnitudeDirectional(fValue, nDecimals, eMode));

    if (nDecimals == 0)
    {
        return api::ValueResult<double>::success(::rtl::math::round(
            fValue, 0, api::math::toCoreRoundingMode(eMode)));
    }

    return api::ValueResult<double>::success(api::math::roundToDecimals(fValue, nDecimals, eMode));
}

api::ValueResult<double> evaluateCeilingFloorValue(
    double fValue, double fSignificance, bool bAbs, bool bCeiling, bool bMicrosoftCompat)
{
    if (bMicrosoftCompat)
    {
        if (bCeiling)
            return api::math::ceilingMs(fValue, fSignificance);
        return api::math::floorMs(fValue, fSignificance);
    }

    if (bCeiling)
        return api::math::ceiling(fValue, fSignificance, bAbs, true);
    return api::math::floor(fValue, fSignificance, bAbs, true);
}

api::ValueResult<double> evaluateCeilingFloorMathValue(
    double fValue, double fSignificance, double fMode, bool bCeiling)
{
    if (fSignificance == 0.0 || fValue == 0.0)
        return api::ValueResult<double>::success(0.0);

    const double fMagnitude = std::abs(fSignificance);
    double fResult = 0.0;
    if (bCeiling)
    {
        if (fValue < 0.0 && fMode != 0.0)
            fResult = ::rtl::math::approxFloor(fValue / fMagnitude) * fMagnitude;
        else
            fResult = computeCeilingPrecise(fValue, fMagnitude);
    }
    else
    {
        if (fValue < 0.0 && fMode != 0.0)
            fResult = ::rtl::math::approxCeil(fValue / fMagnitude) * fMagnitude;
        else
            fResult = computeFloorPrecise(fValue, fMagnitude);
    }

    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluateCeilingFloorPreciseValue(
    double fValue, double fSignificance, bool bFloor)
{
    const double fMagnitude = std::abs(fSignificance);
    return api::ValueResult<double>::success(
        bFloor ? api::math::floorPrecise(fValue, fMagnitude)
               : api::math::ceilingPrecise(fValue, fMagnitude));
}

api::ValueResult<double> evaluateRoundSigValue(double fValue, double fDigits)
{
    const double fWholeDigits = ::rtl::math::approxFloor(fDigits);
    if (fWholeDigits < 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (fValue == 0.0)
        return api::ValueResult<double>::success(0.0);
    return api::ValueResult<double>::success(
        api::math::roundToSignificantDigits(fValue, fWholeDigits));
}

api::ValueResult<double> evaluateLogValue(double fValue, double fBase)
{
    if (!(fValue > 0.0) || !(fBase > 0.0) || ::rtl::math::approxEqual(fBase, 1.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::math::logarithm(fValue, fBase);
}

api::ValueResult<double> evaluateMroundValue(double fValue, double fMultiple)
{
    if (::rtl::math::approxEqual(fMultiple, 0.0))
        return api::ValueResult<double>::success(0.0);

    const double fResult = fMultiple
                           * ::rtl::math::round(
                               ::rtl::math::approxValue(fValue / fMultiple));
    return makeFiniteResult(fResult);
}

api::ValueResult<double> evaluateModValue(double fNumerator, double fDenominator)
{
    if (fDenominator == 0.0)
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    const auto aModResult = api::math::modulo(fNumerator, fDenominator);
    if (!aModResult)
        return api::ValueResult<double>::failure(aModResult.meError);
    return api::ValueResult<double>::success(aModResult.maValue);
}

api::ValueResult<double> evaluateFactorialValue(double fValue)
{
    double fWhole = ::rtl::math::approxFloor(fValue);
    if (fWhole < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (fWhole == 0.0)
        return api::ValueResult<double>::success(1.0);
    if (fWhole > 170.0)
        return api::ValueResult<double>::failure(api::Error::NoValue);

    double fResult = fWhole;
    while (fWhole > 2.0)
    {
        fWhole -= 1.0;
        fResult *= fWhole;
    }
    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluateCombinValue(double fN, double fK, bool bAllowRepetition)
{
    const double fWholeN = ::rtl::math::approxFloor(fN);
    const double fWholeK = ::rtl::math::approxFloor(fK);
    if (fWholeN < 0.0 || fWholeK < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (bAllowRepetition)
    {
        if (fWholeN == 0.0 && fWholeK == 0.0)
            return api::ValueResult<double>::success(0.0);
        if (fWholeK == 0.0)
            return api::ValueResult<double>::success(1.0);
        if (fWholeN < fWholeK)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        return api::ValueResult<double>::success(
            binomialCoefficient(fWholeN + fWholeK - 1.0, fWholeK));
    }

    if (fWholeK > fWholeN)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return api::ValueResult<double>::success(binomialCoefficient(fWholeN, fWholeK));
}

api::ValueResult<double> evaluatePermutationValue(double fN, double fK)
{
    const double fWholeN = ::rtl::math::approxFloor(fN);
    const double fWholeK = ::rtl::math::approxFloor(fK);
    if (fWholeN < 0.0 || fWholeK < 0.0 || fWholeK > fWholeN)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (fWholeK == 0.0)
        return api::ValueResult<double>::success(1.0);

    double fResult = fWholeN;
    for (double fIndex = fWholeK - 1.0; fIndex >= 1.0; --fIndex)
        fResult *= fWholeN - fIndex;
    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluatePermutationAValue(double fN, double fK)
{
    const double fWholeN = ::rtl::math::approxFloor(fN);
    const double fWholeK = ::rtl::math::approxFloor(fK);
    if (fWholeN < 0.0 || fWholeK < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(std::pow(fWholeN, fWholeK));
}

api::ValueResult<double> evaluateMultinomialValue(const std::vector<double>& rValues)
{
    auto binomialCoefficient = [](double fN, double fK) {
        if (fN < fK)
            return 0.0;
        if (fK == 0.0)
            return 1.0;

        double fValue = fN / fK;
        fN -= 1.0;
        fK -= 1.0;
        while (fK > 0.0)
        {
            fValue *= fN / fK;
            fK -= 1.0;
            fN -= 1.0;
        }
        return fValue;
    };

    double fTotal = 0.0;
    double fResult = 1.0;
    for (const double fInput : rValues)
    {
        const double fRounded
            = fInput >= 0.0 ? ::rtl::math::approxFloor(fInput) : ::rtl::math::approxCeil(fInput);
        if (fRounded < 0.0)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        if (fRounded > 0.0)
        {
            fTotal += fRounded;
            fResult *= binomialCoefficient(fTotal, fRounded);
            if (!std::isfinite(fResult))
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        }
    }

    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluateCscValue(double fValue)
{
    const double fResult = api::math::cosecant(fValue);
    if (!std::isfinite(fResult))
    {
        if (::rtl::math::approxEqual(::rtl::math::sin(fValue), 0.0))
            return api::ValueResult<double>::failure(api::Error::DivisionByZero);
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluateCschValue(double fValue)
{
    const double fResult = api::math::hyperbolicCosecant(fValue);
    if (!std::isfinite(fResult))
    {
        if (::rtl::math::approxEqual(std::sinh(fValue), 0.0))
            return api::ValueResult<double>::failure(api::Error::DivisionByZero);
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluateTruncValue(double fValue, sal_Int32 nDigits)
{
    const double fScale = std::pow(10.0, std::abs(nDigits));
    double fResult = 0.0;
    if (nDigits >= 0)
        fResult = std::trunc(fValue * fScale) / fScale;
    else
        fResult = std::trunc(fValue / fScale) * fScale;
    return api::ValueResult<double>::success(fResult);
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
