/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Rounding.hxx>
#include <spreadsheetengine/runtime/MathBitwise.hxx>
#include <spreadsheetengine/runtime/MathFinancial.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <spreadsheetengine/runtime/MathScalar.hxx>
#include <spreadsheetengine/runtime/MathTranscendental.hxx>

namespace spreadsheetengine::api::math
{

enum class Sign
{
    Negative = -1,
    Zero = 0,
    Positive = 1
};

struct FinancialInterestPayment
{
    double mfInterest = 0.0;
    double mfPayment = 0.0;
};

struct FinancialRateResult
{
    double mfRate = 0.0;
    bool mbConverged = false;
    api::Error meError = api::Error::None;

    [[nodiscard]] constexpr bool ok() const { return meError == api::Error::None; }

    constexpr explicit operator bool() const { return ok(); }
};

inline rtl_math_RoundingMode toCoreRoundingMode(api::RoundingMode eMode)
{
    switch (eMode)
    {
        case api::RoundingMode::Down:
            return rtl_math_RoundingMode_Down;
        case api::RoundingMode::Up:
            return rtl_math_RoundingMode_Up;
        default:
            return rtl_math_RoundingMode_Corrected;
    }
}

inline Sign sign(double fValue)
{
    switch (spreadsheetengine::core::math::computePlusMinus(fValue))
    {
        case -1:
            return Sign::Negative;
        case 1:
            return Sign::Positive;
        default:
            return Sign::Zero;
    }
}

inline double abs(double fValue) { return spreadsheetengine::core::math::computeAbs(fValue); }

inline double integerFloor(double fValue)
{
    return spreadsheetengine::core::math::computeInt(fValue);
}

inline double arcTan2(double fY, double fX)
{
    return spreadsheetengine::core::math::computeArcTan2(fY, fX);
}

inline api::ValueResult<double> bitAnd(double fLeft, double fRight)
{
    if (auto oValue = spreadsheetengine::core::math::computeBitAnd(fLeft, fRight))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> bitOr(double fLeft, double fRight)
{
    if (auto oValue = spreadsheetengine::core::math::computeBitOr(fLeft, fRight))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> bitXor(double fLeft, double fRight)
{
    if (auto oValue = spreadsheetengine::core::math::computeBitXor(fLeft, fRight))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> bitLeftShift(double fValue, double fShift)
{
    if (auto oValue = spreadsheetengine::core::math::computeBitLeftShift(fValue, fShift))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> bitRightShift(double fValue, double fShift)
{
    if (auto oValue = spreadsheetengine::core::math::computeBitRightShift(fValue, fShift))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> logarithm(double fValue, double fBase)
{
    if (auto oValue = spreadsheetengine::core::math::computeLog(fValue, fBase))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> naturalLogarithm(double fValue)
{
    if (auto oValue = spreadsheetengine::core::math::computeLn(fValue))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> logarithmBase10(double fValue)
{
    if (auto oValue = spreadsheetengine::core::math::computeLog10(fValue))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> modulo(double fNumerator, double fDenominator)
{
    if (auto oValue = spreadsheetengine::core::math::computeMod(fNumerator, fDenominator))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline double pi() { return spreadsheetengine::core::math::computePi(); }

inline double degrees(double fRadians)
{
    return spreadsheetengine::core::math::computeDegrees(fRadians);
}

inline double radians(double fDegrees)
{
    return spreadsheetengine::core::math::computeRadians(fDegrees);
}

inline double sine(double fValue) { return spreadsheetengine::core::math::computeSin(fValue); }

inline double cosine(double fValue)
{
    return spreadsheetengine::core::math::computeCos(fValue);
}

inline double tangent(double fValue)
{
    return spreadsheetengine::core::math::computeTan(fValue);
}

inline double cotangent(double fValue)
{
    return spreadsheetengine::core::math::computeCot(fValue);
}

inline double arcSine(double fValue)
{
    return spreadsheetengine::core::math::computeArcSin(fValue);
}

inline double arcCosine(double fValue)
{
    return spreadsheetengine::core::math::computeArcCos(fValue);
}

inline double arcTangent(double fValue)
{
    return spreadsheetengine::core::math::computeArcTan(fValue);
}

inline double arcCotangent(double fValue)
{
    return spreadsheetengine::core::math::computeArcCot(fValue);
}

inline double hyperbolicSine(double fValue)
{
    return spreadsheetengine::core::math::computeSinHyp(fValue);
}

inline double hyperbolicCosine(double fValue)
{
    return spreadsheetengine::core::math::computeCosHyp(fValue);
}

inline double hyperbolicTangent(double fValue)
{
    return spreadsheetengine::core::math::computeTanHyp(fValue);
}

inline double hyperbolicCotangent(double fValue)
{
    return spreadsheetengine::core::math::computeCotHyp(fValue);
}

inline double inverseHyperbolicSine(double fValue)
{
    return spreadsheetengine::core::math::computeArcSinHyp(fValue);
}

inline api::ValueResult<double> inverseHyperbolicCosine(double fValue)
{
    if (auto oValue = spreadsheetengine::core::math::computeArcCosHyp(fValue))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::Domain);
}

inline api::ValueResult<double> inverseHyperbolicTangent(double fValue)
{
    if (auto oValue = spreadsheetengine::core::math::computeArcTanHyp(fValue))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::Domain);
}

inline api::ValueResult<double> inverseHyperbolicCotangent(double fValue)
{
    if (auto oValue = spreadsheetengine::core::math::computeArcCotHyp(fValue))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::Domain);
}

inline double cosecant(double fValue)
{
    return spreadsheetengine::core::math::computeCosecant(fValue);
}

inline double secant(double fValue)
{
    return spreadsheetengine::core::math::computeSecant(fValue);
}

inline double hyperbolicCosecant(double fValue)
{
    return spreadsheetengine::core::math::computeCosecantHyp(fValue);
}

inline double hyperbolicSecant(double fValue)
{
    return spreadsheetengine::core::math::computeSecantHyp(fValue);
}

inline double exponential(double fValue)
{
    return spreadsheetengine::core::math::computeExp(fValue);
}

inline api::ValueResult<double> squareRoot(double fValue)
{
    if (auto oValue = spreadsheetengine::core::math::computeSqrt(fValue))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::Domain);
}

inline double roundToDecimals(double fValue, int nDecimals, api::RoundingMode eMode)
{
    return spreadsheetengine::core::math::roundToDecimals(
        fValue, static_cast<sal_Int16>(nDecimals), toCoreRoundingMode(eMode));
}

inline double roundToSignificantDigits(double fValue, double fDigits)
{
    return spreadsheetengine::core::math::roundToSignificantDigits(fValue, fDigits);
}

inline api::ValueResult<double> ceiling(
    double fValue, double fSignificance, bool bAbs, bool bOdfMode)
{
    if (auto oValue
        = spreadsheetengine::core::math::computeCeiling(fValue, fSignificance, bAbs, bOdfMode))
    {
        return api::ValueResult<double>::success(*oValue);
    }
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> ceilingMs(double fValue, double fSignificance)
{
    if (auto oValue = spreadsheetengine::core::math::computeCeilingMs(fValue, fSignificance))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline double ceilingPrecise(double fValue, double fSignificance)
{
    return spreadsheetengine::core::math::computeCeilingPrecise(fValue, fSignificance);
}

inline api::ValueResult<double> floor(
    double fValue, double fSignificance, bool bAbs, bool bOdfMode)
{
    if (auto oValue
        = spreadsheetengine::core::math::computeFloor(fValue, fSignificance, bAbs, bOdfMode))
    {
        return api::ValueResult<double>::success(*oValue);
    }
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> floorMs(double fValue, double fSignificance)
{
    if (auto oValue = spreadsheetengine::core::math::computeFloorMs(fValue, fSignificance))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline double floorPrecise(double fValue, double fSignificance)
{
    return spreadsheetengine::core::math::computeFloorPrecise(fValue, fSignificance);
}

inline double even(double fValue) { return spreadsheetengine::core::math::computeEven(fValue); }

inline double odd(double fValue) { return spreadsheetengine::core::math::computeOdd(fValue); }

inline double presentValue(
    double fRate, double fPeriods, double fPayment, double fFutureValue, bool bPayInAdvance)
{
    return spreadsheetengine::core::math::computePresentValue(
        fRate, fPeriods, fPayment, fFutureValue, bPayInAdvance);
}

inline double payment(
    double fRate, double fPeriods, double fPresentValue, double fFutureValue, bool bPayInAdvance)
{
    return spreadsheetengine::core::math::computePayment(
        fRate, fPeriods, fPresentValue, fFutureValue, bPayInAdvance);
}

inline double futureValue(
    double fRate, double fPeriods, double fPayment, double fPresentValue, bool bPayInAdvance)
{
    return spreadsheetengine::core::math::computeFutureValue(
        fRate, fPeriods, fPayment, fPresentValue, bPayInAdvance);
}

inline double interestSchedulePayment(
    double fRate, double fPeriod, double fTotalPeriods, double fInvestment)
{
    return spreadsheetengine::core::math::computeInterestSchedulePayment(
        fRate, fPeriod, fTotalPeriods, fInvestment);
}

inline double sumOfYearsDepreciation(
    double fCost, double fSalvage, double fLife, double fPeriod)
{
    return spreadsheetengine::core::math::computeSumOfYearsDepreciation(
        fCost, fSalvage, fLife, fPeriod);
}

inline double doubleDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fFactor)
{
    return spreadsheetengine::core::math::computeDoubleDecliningBalance(
        fCost, fSalvage, fLife, fPeriod, fFactor);
}

inline double fixedDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fMonths)
{
    return spreadsheetengine::core::math::computeFixedDecliningBalance(
        fCost, fSalvage, fLife, fPeriod, fMonths);
}

inline double variableDecliningBalanceSegment(
    double fCost, double fSalvage, double fLife, double fRemainingLife,
    double fPeriod, double fFactor)
{
    return spreadsheetengine::core::math::computeVariableDecliningBalanceSegment(
        fCost, fSalvage, fLife, fRemainingLife, fPeriod, fFactor);
}

inline double variableDecliningBalance(
    double fCost, double fSalvage, double fLife, double fStart,
    double fEnd, double fFactor, bool bNoSwitch)
{
    return spreadsheetengine::core::math::computeVariableDecliningBalance(
        fCost, fSalvage, fLife, fStart, fEnd, fFactor, bNoSwitch);
}

inline FinancialInterestPayment interestPayment(
    double fRate, double fPer, double fPeriods, double fPresentValue,
    double fFutureValue, bool bPayInAdvance)
{
    const auto aResult = spreadsheetengine::core::math::computeInterestPayment(
        fRate, fPer, fPeriods, fPresentValue, fFutureValue, bPayInAdvance);
    return { aResult.mfInterest, aResult.mfPayment };
}

inline double principalPayment(
    double fRate, double fPer, double fPeriods, double fPresentValue,
    double fFutureValue, bool bPayInAdvance)
{
    return spreadsheetengine::core::math::computePrincipalPayment(
        fRate, fPer, fPeriods, fPresentValue, fFutureValue, bPayInAdvance);
}

inline double cumulativeInterest(
    double fRate, double fStart, double fEnd, double fPeriods,
    double fPresentValue, double fFutureValue, bool bPayInAdvance)
{
    return spreadsheetengine::core::math::computeCumulativeInterest(
        fRate, fStart, fEnd, fPeriods, fPresentValue, fFutureValue, bPayInAdvance);
}

inline double cumulativePrincipal(
    double fRate, double fStart, double fEnd, double fPeriods,
    double fPresentValue, double fFutureValue, bool bPayInAdvance)
{
    return spreadsheetengine::core::math::computeCumulativePrincipal(
        fRate, fStart, fEnd, fPeriods, fPresentValue, fFutureValue, bPayInAdvance);
}

inline double paybackDuration(double fRate, double fPresentValue, double fFutureValue)
{
    return spreadsheetengine::core::math::computePaybackDuration(
        fRate, fPresentValue, fFutureValue);
}

inline double growthRateOverPeriods(
    double fPeriods, double fPresentValue, double fFutureValue)
{
    return spreadsheetengine::core::math::computeGrowthRateOverPeriods(
        fPeriods, fPresentValue, fFutureValue);
}

inline double periodsForFutureValue(
    double fRate, double fPayment, double fPresentValue,
    double fFutureValue, bool bPayInAdvance)
{
    return spreadsheetengine::core::math::computePeriodsForFutureValue(
        fRate, fPayment, fPresentValue, fFutureValue, bPayInAdvance);
}

inline FinancialRateResult solveRate(
    double fPeriods, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayType, double fGuess, bool bAllowAlternateGuesses)
{
    const auto aResult = spreadsheetengine::core::math::solveRate(
        fPeriods, fPayment, fPresentValue, fFutureValue, bPayType, fGuess,
        bAllowAlternateGuesses);
    return { aResult.mfRate,
             aResult.mbConverged,
             aResult.mbConverged ? api::Error::None : api::Error::NoConvergence };
}

inline double effectiveAnnualRate(double fNominalRate, double fPeriods)
{
    return spreadsheetengine::core::math::computeEffectiveAnnualRate(fNominalRate, fPeriods);
}

inline double nominalAnnualRate(double fEffectiveRate, double fPeriods)
{
    return spreadsheetengine::core::math::computeNominalAnnualRate(fEffectiveRate, fPeriods);
}

inline double straightLineDepreciation(double fCost, double fSalvage, double fLife)
{
    return spreadsheetengine::core::math::computeStraightLineDepreciation(
        fCost, fSalvage, fLife);
}

} // namespace spreadsheetengine::api::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
