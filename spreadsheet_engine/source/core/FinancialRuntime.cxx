/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/FinancialRuntime.hxx>

#include <spreadsheetengine/runtime/MathFinancial.hxx>

#include <cmath>
#include <optional>

#include <rtl/math.hxx>
#include <sal/types.h>

namespace spreadsheetengine::core::finance
{
namespace
{

[[nodiscard]] api::ValueResult<double> makeFiniteNumberResult(double fValue)
{
    if (!std::isfinite(fValue))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(fValue);
}

[[nodiscard]] std::optional<sal_Int32> toWholeNumber(double fValue)
{
    if (!std::isfinite(fValue))
        return std::nullopt;

    const double fRounded = std::round(fValue);
    if (std::abs(fValue - fRounded) > 1e-9)
        return std::nullopt;

    return static_cast<sal_Int32>(fRounded);
}

[[nodiscard]] bool isValidPaymentPeriod(double fPeriod, double fTotalPeriods)
{
    return fTotalPeriods > 0.0 && fPeriod >= 1.0 && fPeriod <= fTotalPeriods;
}

} // namespace

api::ValueResult<double> evaluateFutureValue(
    double fRate, double fPeriods, double fPayment, double fPresentValue, bool bPayInAdvance)
{
    return makeFiniteNumberResult(spreadsheetengine::core::math::computeFutureValue(
        fRate, fPeriods, fPayment, fPresentValue, bPayInAdvance));
}

api::ValueResult<double> evaluatePresentValue(
    double fRate, double fPeriods, double fPayment, double fFutureValue, bool bPayInAdvance)
{
    return makeFiniteNumberResult(spreadsheetengine::core::math::computePresentValue(
        fRate, fPeriods, fPayment, fFutureValue, bPayInAdvance));
}

api::ValueResult<double> evaluatePayment(
    double fRate, double fPeriods, double fPresentValue, double fFutureValue,
    bool bPayInAdvance)
{
    if (::rtl::math::approxEqual(fPeriods, 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteNumberResult(spreadsheetengine::core::math::computePayment(
        fRate, fPeriods, fPresentValue, fFutureValue, bPayInAdvance));
}

api::ValueResult<double> evaluatePeriodsForFutureValue(
    double fRate, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayInAdvance)
{
    return makeFiniteNumberResult(spreadsheetengine::core::math::computePeriodsForFutureValue(
        fRate, fPayment, fPresentValue, fFutureValue, bPayInAdvance));
}

api::ValueResult<double> evaluateRate(
    double fPeriods, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayInAdvance, double fGuess)
{
    if (!(fPeriods > 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const auto aRateResult = spreadsheetengine::core::math::solveRate(
        fPeriods, fPayment, fPresentValue, fFutureValue, bPayInAdvance, fGuess, true);
    if (!aRateResult.mbConverged)
        return api::ValueResult<double>::failure(api::Error::NoConvergence);
    return makeFiniteNumberResult(aRateResult.mfRate);
}

api::ValueResult<double> evaluateInterestSchedulePayment(
    double fRate, double fPeriod, double fTotalPeriods, double fInvestment)
{
    if (::rtl::math::approxEqual(fTotalPeriods, 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteNumberResult(
        spreadsheetengine::core::math::computeInterestSchedulePayment(
            fRate, fPeriod, fTotalPeriods, fInvestment));
}

api::ValueResult<double> evaluateInterestPayment(
    double fRate, double fPeriod, double fTotalPeriods, double fPresentValue,
    double fFutureValue, bool bPayInAdvance)
{
    if (!isValidPaymentPeriod(fPeriod, fTotalPeriods))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteNumberResult(spreadsheetengine::core::math::computeInterestPayment(
                                       fRate, fPeriod, fTotalPeriods, fPresentValue,
                                       fFutureValue, bPayInAdvance)
                                       .mfInterest);
}

api::ValueResult<double> evaluatePrincipalPayment(
    double fRate, double fPeriod, double fTotalPeriods, double fPresentValue,
    double fFutureValue, bool bPayInAdvance)
{
    if (!isValidPaymentPeriod(fPeriod, fTotalPeriods))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteNumberResult(spreadsheetengine::core::math::computePrincipalPayment(
        fRate, fPeriod, fTotalPeriods, fPresentValue, fFutureValue, bPayInAdvance));
}

api::ValueResult<double> evaluateCumulativeInterest(
    double fRate, double fTotalPeriods, double fPresentValue, double fStart,
    double fEnd, bool bPayInAdvance)
{
    if (!(fRate > 0.0) || !(fTotalPeriods > 0.0) || !(fPresentValue > 0.0)
        || !(fStart >= 1.0) || !(fEnd >= fStart))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const auto oWholeStart = toWholeNumber(fStart);
    const auto oWholeEnd = toWholeNumber(fEnd);
    if (!oWholeStart || !oWholeEnd)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteNumberResult(spreadsheetengine::core::math::computeCumulativeInterest(
        fRate, static_cast<double>(*oWholeStart), static_cast<double>(*oWholeEnd),
        fTotalPeriods, fPresentValue, 0.0, bPayInAdvance));
}

api::ValueResult<double> evaluateCumulativePrincipal(
    double fRate, double fTotalPeriods, double fPresentValue, double fStart,
    double fEnd, bool bPayInAdvance)
{
    if (!(fRate > 0.0) || !(fTotalPeriods > 0.0) || !(fPresentValue > 0.0)
        || !(fStart >= 1.0) || !(fEnd >= fStart))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const auto oWholeStart = toWholeNumber(fStart);
    const auto oWholeEnd = toWholeNumber(fEnd);
    if (!oWholeStart || !oWholeEnd)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteNumberResult(spreadsheetengine::core::math::computeCumulativePrincipal(
        fRate, static_cast<double>(*oWholeStart), static_cast<double>(*oWholeEnd),
        fTotalPeriods, fPresentValue, 0.0, bPayInAdvance));
}

api::ValueResult<double> evaluateDoubleDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fFactor)
{
    if (!(fCost > 0.0) || fSalvage < 0.0 || fCost < fSalvage || !(fLife > 0.0)
        || !(fPeriod > 0.0) || fPeriod > fLife || !(fFactor > 0.0))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    return makeFiniteNumberResult(spreadsheetengine::core::math::computeDoubleDecliningBalance(
        fCost, fSalvage, fLife, fPeriod, fFactor));
}

api::ValueResult<double> evaluateVariableDecliningBalance(
    double fCost, double fSalvage, double fLife, double fStart,
    double fEnd, double fFactor, bool bNoSwitch)
{
    if (!(fCost > 0.0) || fSalvage < 0.0 || fCost < fSalvage || !(fLife > 0.0)
        || fStart < 0.0 || fEnd < fStart || fEnd > fLife || !(fFactor > 0.0))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    return makeFiniteNumberResult(spreadsheetengine::core::math::computeVariableDecliningBalance(
        fCost, fSalvage, fLife, fStart, fEnd, fFactor, bNoSwitch));
}

} // namespace spreadsheetengine::core::finance

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
