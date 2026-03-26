/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/MathFinancial.hxx>

#include <algorithm>
#include <cstddef>
#include <cmath>

#include <kahan.hxx>
#include <o3tl/untaint.hxx>
#include <rtl/math.hxx>

namespace spreadsheetengine::core::math
{
namespace
{

constexpr double kRateEpsilon = 1.0E-7;
constexpr double kRateEpsilonSmall = 1.0E-14;
constexpr sal_uInt16 kRateIterationsMax = 150;

bool iterateRate(
    double fNper, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayType, double& fGuess)
{
    bool bValid = true;
    bool bFound = false;
    double fX;
    double fXnew;
    double fTerm;
    double fTermDerivation;
    double fGeoSeries;
    double fGeoSeriesDerivation;
    sal_uInt16 nCount = 0;

    if (bPayType)
    {
        fFutureValue -= fPayment;
        fPresentValue += fPayment;
    }

    if (fNper == ::rtl::math::round(fNper))
    {
        fX = fGuess;
        while (!bFound && nCount < kRateIterationsMax)
        {
            const double fPowNminus1 = std::pow(1.0 + fX, fNper - 1.0);
            const double fPowN = fPowNminus1 * (1.0 + fX);
            if (fX == 0.0)
            {
                fGeoSeries = fNper;
                fGeoSeriesDerivation = fNper * (fNper - 1.0) / 2.0;
            }
            else
            {
                fGeoSeries = (fPowN - 1.0) / fX;
                fGeoSeriesDerivation = fNper * fPowNminus1 / fX - fGeoSeries / fX;
            }

            fTerm = fFutureValue + fPresentValue * fPowN + fPayment * fGeoSeries;
            fTermDerivation
                = fPresentValue * fNper * fPowNminus1 + fPayment * fGeoSeriesDerivation;

            if (std::abs(fTerm) < kRateEpsilonSmall)
                bFound = true;
            else
            {
                if (fTermDerivation == 0.0)
                    fXnew = fX + 1.1 * kRateEpsilon;
                else
                    fXnew = fX - fTerm / fTermDerivation;

                ++nCount;
                bFound = (std::abs(fXnew - fX) < kRateEpsilon);
                fX = fXnew;
            }
        }

        bValid = (fX > -1.0);
    }
    else
    {
        fX = (fGuess < -1.0) ? -1.0 : fGuess;
        while (bValid && !bFound && nCount < kRateIterationsMax)
        {
            if (fX == 0.0)
            {
                fGeoSeries = fNper;
                fGeoSeriesDerivation = fNper * (fNper - 1.0) / 2.0;
            }
            else
            {
                fGeoSeries = (std::pow(1.0 + fX, fNper) - 1.0) / fX;
                fGeoSeriesDerivation
                    = fNper * std::pow(1.0 + fX, fNper - 1.0) / fX - fGeoSeries / fX;
            }

            fTerm = fFutureValue + fPresentValue * std::pow(1.0 + fX, fNper)
                    + fPayment * fGeoSeries;
            fTermDerivation = fPresentValue * fNper * std::pow(1.0 + fX, fNper - 1.0)
                              + fPayment * fGeoSeriesDerivation;

            if (std::abs(fTerm) < kRateEpsilonSmall)
                bFound = true;
            else
            {
                if (fTermDerivation == 0.0)
                    fXnew = fX + 1.1 * kRateEpsilon;
                else
                    fXnew = fX - fTerm / fTermDerivation;

                ++nCount;
                bFound = (std::abs(fXnew - fX) < kRateEpsilon);
                fX = fXnew;
                bValid = (fX >= -1.0);
            }
        }
    }

    fGuess = fX;
    return bValid && bFound;
}

} // namespace

double computeInterestSchedulePayment(
    double fRate, double fPeriod, double fTotalPeriods, double fInvestment)
{
    return fInvestment * fRate * (o3tl::div_allow_zero(fPeriod, fTotalPeriods) - 1.0);
}

double computeSumOfYearsDepreciation(
    double fCost, double fSalvage, double fLife, double fPeriod)
{
    return o3tl::div_allow_zero(
        (fCost - fSalvage) * (fLife - fPeriod + 1.0), (fLife * (fLife + 1.0)) / 2.0);
}

double computePresentValue(
    double fRate, double fNper, double fPayment, double fFutureValue, bool bPayInAdvance)
{
    double fPresentValue;
    if (fRate == 0.0)
        fPresentValue = fFutureValue + fPayment * fNper;
    else if (bPayInAdvance)
        fPresentValue = (fFutureValue * std::pow(1.0 + fRate, -fNper))
                        + (fPayment * (1.0 - std::pow(1.0 + fRate, -fNper + 1.0)) / fRate)
                        + fPayment;
    else
        fPresentValue = (fFutureValue * std::pow(1.0 + fRate, -fNper))
                        + (fPayment * (1.0 - std::pow(1.0 + fRate, -fNper)) / fRate);

    return -fPresentValue;
}

double computeDoubleDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fFactor)
{
    double fRate = o3tl::div_allow_zero(fFactor, fLife);
    double fOldValue;
    if (fRate >= 1.0)
    {
        fRate = 1.0;
        fOldValue = fPeriod == 1.0 ? fCost : 0.0;
    }
    else
        fOldValue = fCost * std::pow(1.0 - fRate, fPeriod - 1.0);

    const double fNewValue = fCost * std::pow(1.0 - fRate, fPeriod);
    const double fDdb = fNewValue < fSalvage ? fOldValue - fSalvage : fOldValue - fNewValue;
    return fDdb < 0.0 ? 0.0 : fDdb;
}

double computeFixedDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fMonths)
{
    double fOffRate = 1.0 - std::pow(fSalvage / fCost, 1.0 / fLife);
    fOffRate = ::rtl::math::approxFloor((fOffRate * 1000.0) + 0.5) / 1000.0;
    const double fFirstOffRate = fCost * fOffRate * fMonths / 12.0;

    double fDb = 0.0;
    if (::rtl::math::approxFloor(fPeriod) == 1.0)
        fDb = fFirstOffRate;
    else
    {
        KahanSum fSumOffRate = fFirstOffRate;
        double fMin = fLife;
        if (fMin > fPeriod)
            fMin = fPeriod;

        const sal_uInt16 iMax = static_cast<sal_uInt16>(::rtl::math::approxFloor(fMin));
        for (sal_uInt16 i = 2; i <= iMax; ++i)
        {
            fDb = -(fSumOffRate - fCost).get() * fOffRate;
            fSumOffRate += fDb;
        }

        if (fPeriod > fLife)
            fDb = -(fSumOffRate - fCost).get() * fOffRate * (12.0 - fMonths) / 12.0;
    }

    return fDb;
}

double computeVariableDecliningBalanceSegment(
    double fCost, double fSalvage, double fLife, double fRemainingLife,
    double fPeriod, double fFactor)
{
    KahanSum fVdb = 0.0;
    const double fIntEnd = ::rtl::math::approxCeil(fPeriod);
    const std::size_t nLoopEnd = static_cast<std::size_t>(fIntEnd);

    double fSln = 0.0;
    double fSalvageValue = fCost - fSalvage;
    bool bNowSln = false;

    for (std::size_t i = 1; i <= nLoopEnd; ++i)
    {
        double fTerm;
        if (!bNowSln)
        {
            const double fDdb = computeDoubleDecliningBalance(
                fCost, fSalvage, fLife, static_cast<double>(i), fFactor);
            fSln = fSalvageValue / (fRemainingLife - static_cast<double>(i - 1));

            if (fSln > fDdb)
            {
                fTerm = fSln;
                bNowSln = true;
            }
            else
            {
                fTerm = fDdb;
                fSalvageValue -= fDdb;
            }
        }
        else
            fTerm = fSln;

        if (i == nLoopEnd)
            fTerm *= (fPeriod + 1.0 - fIntEnd);

        fVdb += fTerm;
    }

    return fVdb.get();
}

double computeVariableDecliningBalance(
    double fCost, double fSalvage, double fLife, double fStart,
    double fEnd, double fFactor, bool bNoSwitch)
{
    KahanSum fVdb = 0.0;
    const double fIntStart = ::rtl::math::approxFloor(fStart);
    const double fIntEnd = ::rtl::math::approxCeil(fEnd);
    const std::size_t nLoopStart = static_cast<std::size_t>(fIntStart);
    const std::size_t nLoopEnd = static_cast<std::size_t>(fIntEnd);

    if (bNoSwitch)
    {
        for (std::size_t i = nLoopStart + 1; i <= nLoopEnd; ++i)
        {
            double fTerm = computeDoubleDecliningBalance(
                fCost, fSalvage, fLife, static_cast<double>(i), fFactor);

            if (i == nLoopStart + 1)
                fTerm *= (std::min(fEnd, fIntStart + 1.0) - fStart);
            else if (i == nLoopEnd)
                fTerm *= (fEnd + 1.0 - fIntEnd);

            fVdb += fTerm;
        }
    }
    else
    {
        double fPart = 0.0;
        if (!::rtl::math::approxEqual(fStart, fIntStart)
            || !::rtl::math::approxEqual(fEnd, fIntEnd))
        {
            if (!::rtl::math::approxEqual(fStart, fIntStart))
            {
                const double fTempIntEnd = fIntStart + 1.0;
                const double fTempValue = fCost
                                          - computeVariableDecliningBalanceSegment(
                                              fCost, fSalvage, fLife, fLife, fIntStart, fFactor);
                fPart += (fStart - fIntStart)
                         * computeVariableDecliningBalanceSegment(
                             fTempValue, fSalvage, fLife, fLife - fIntStart,
                             fTempIntEnd - fIntStart, fFactor);
            }
            if (!::rtl::math::approxEqual(fEnd, fIntEnd))
            {
                const double fTempIntStart = fIntEnd - 1.0;
                const double fTempValue = fCost
                                          - computeVariableDecliningBalanceSegment(
                                              fCost, fSalvage, fLife, fLife, fTempIntStart,
                                              fFactor);
                fPart += (fIntEnd - fEnd)
                         * computeVariableDecliningBalanceSegment(
                             fTempValue, fSalvage, fLife, fLife - fTempIntStart,
                             fIntEnd - fTempIntStart, fFactor);
            }
        }

        fCost -= computeVariableDecliningBalanceSegment(
            fCost, fSalvage, fLife, fLife, fIntStart, fFactor);
        fVdb = computeVariableDecliningBalanceSegment(
            fCost, fSalvage, fLife, fLife - fIntStart, fIntEnd - fIntStart, fFactor);
        fVdb -= fPart;
    }

    return fVdb.get();
}

double computePayment(
    double fRate, double fNper, double fPresentValue, double fFutureValue, bool bPayInAdvance)
{
    double fPayment;
    if (fRate == 0.0)
        fPayment = o3tl::div_allow_zero(fPresentValue + fFutureValue, fNper);
    else if (bPayInAdvance)
        fPayment = (fFutureValue + fPresentValue * std::exp(fNper * std::log1p(fRate))) * fRate
                   / (std::expm1((fNper + 1) * std::log1p(fRate)) - fRate);
    else
        fPayment = (fFutureValue + fPresentValue * std::exp(fNper * std::log1p(fRate))) * fRate
                   / std::expm1(fNper * std::log1p(fRate));

    return -fPayment;
}

double computeFutureValue(
    double fRate, double fNper, double fPayment, double fPresentValue, bool bPayInAdvance)
{
    double fFutureValue;
    if (fRate == 0.0)
        fFutureValue = fPresentValue + fPayment * fNper;
    else
    {
        const double fTerm = std::pow(1.0 + fRate, fNper);
        if (bPayInAdvance)
            fFutureValue = fPresentValue * fTerm
                           + fPayment * (1.0 + fRate) * (fTerm - 1.0) / fRate;
        else
            fFutureValue = fPresentValue * fTerm + fPayment * (fTerm - 1.0) / fRate;
    }

    return -fFutureValue;
}

FinancialInterestPayment computeInterestPayment(
    double fRate, double fPer, double fNper, double fPresentValue,
    double fFutureValue, bool bPayInAdvance)
{
    FinancialInterestPayment aResult;
    aResult.mfPayment = computePayment(
        fRate, fNper, fPresentValue, fFutureValue, bPayInAdvance);

    double fInterest;
    if (fPer == 1.0)
        fInterest = bPayInAdvance ? 0.0 : -fPresentValue;
    else if (bPayInAdvance)
        fInterest = computeFutureValue(
                        fRate, fPer - 2.0, aResult.mfPayment, fPresentValue, true)
                    - aResult.mfPayment;
    else
        fInterest = computeFutureValue(
            fRate, fPer - 1.0, aResult.mfPayment, fPresentValue, false);

    aResult.mfInterest = fInterest * fRate;
    return aResult;
}

double computePrincipalPayment(
    double fRate, double fPer, double fNper, double fPresentValue,
    double fFutureValue, bool bPayInAdvance)
{
    const FinancialInterestPayment aResult = computeInterestPayment(
        fRate, fPer, fNper, fPresentValue, fFutureValue, bPayInAdvance);
    return aResult.mfPayment - aResult.mfInterest;
}

double computeCumulativeInterest(
    double fRate, double fStart, double fEnd, double fNper,
    double fPresentValue, double fFutureValue, bool bPayInAdvance)
{
    KahanSum fInterest = 0.0;
    std::size_t nStart = static_cast<std::size_t>(fStart);
    const std::size_t nEnd = static_cast<std::size_t>(fEnd);
    const double fPayment = computePayment(
        fRate, fNper, fPresentValue, fFutureValue, bPayInAdvance);

    if (nStart == 1)
    {
        if (!bPayInAdvance)
            fInterest = -fPresentValue;
        ++nStart;
    }

    for (std::size_t i = nStart; i <= nEnd; ++i)
    {
        if (bPayInAdvance)
            fInterest += computeFutureValue(
                             fRate, static_cast<double>(i - 2), fPayment, fPresentValue, true)
                         - fPayment;
        else
            fInterest += computeFutureValue(
                fRate, static_cast<double>(i - 1), fPayment, fPresentValue, false);
    }

    fInterest *= fRate;
    return fInterest.get();
}

double computeCumulativePrincipal(
    double fRate, double fStart, double fEnd, double fNper,
    double fPresentValue, double fFutureValue, bool bPayInAdvance)
{
    KahanSum fPrincipal = 0.0;
    std::size_t nStart = static_cast<std::size_t>(fStart);
    const std::size_t nEnd = static_cast<std::size_t>(fEnd);
    const double fPayment = computePayment(
        fRate, fNper, fPresentValue, fFutureValue, bPayInAdvance);

    if (nStart == 1)
    {
        fPrincipal = bPayInAdvance ? fPayment : fPayment + fPresentValue * fRate;
        ++nStart;
    }

    for (std::size_t i = nStart; i <= nEnd; ++i)
    {
        if (bPayInAdvance)
            fPrincipal += fPayment
                          - (computeFutureValue(
                                 fRate, static_cast<double>(i - 2), fPayment, fPresentValue,
                                 true)
                             - fPayment)
                                * fRate;
        else
            fPrincipal += fPayment
                          - computeFutureValue(
                                fRate, static_cast<double>(i - 1), fPayment, fPresentValue,
                                false)
                                * fRate;
    }

    return fPrincipal.get();
}

double computePaybackDuration(double fRate, double fPresentValue, double fFutureValue)
{
    return std::log(fFutureValue / fPresentValue) / std::log1p(fRate);
}

double computeGrowthRateOverPeriods(double fPeriods, double fPresentValue, double fFutureValue)
{
    return std::pow(fFutureValue / fPresentValue, 1.0 / fPeriods) - 1.0;
}

double computePeriodsForFutureValue(
    double fRate, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayInAdvance)
{
    if (fPresentValue + fFutureValue == 0.0)
        return 0.0;

    if (fRate == 0.0)
        return -o3tl::div_allow_zero(fPresentValue + fFutureValue, fPayment);

    if (bPayInAdvance)
    {
        return std::log(-o3tl::div_allow_zero(
                            fRate * fFutureValue - fPayment * (1.0 + fRate),
                            (fRate * fPresentValue + fPayment * (1.0 + fRate))))
               / std::log1p(fRate);
    }

    return std::log(-(fRate * fFutureValue - fPayment) / (fRate * fPresentValue + fPayment))
           / std::log1p(fRate);
}

FinancialRateResult solveRate(
    double fNper, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayType, double fGuess, bool bAllowAlternateGuesses)
{
    FinancialRateResult aResult;
    aResult.mfRate = fGuess;
    const double fOrigGuess = fGuess;

    aResult.mbConverged = iterateRate(
        fNper, fPayment, fPresentValue, fFutureValue, bPayType, aResult.mfRate);

    if (!aResult.mbConverged && bAllowAlternateGuesses)
    {
        for (int nStep = 2; nStep <= 10 && !aResult.mbConverged; ++nStep)
        {
            aResult.mfRate = fOrigGuess * nStep;
            aResult.mbConverged = iterateRate(
                fNper, fPayment, fPresentValue, fFutureValue, bPayType, aResult.mfRate);
            if (!aResult.mbConverged)
            {
                aResult.mfRate = fOrigGuess / nStep;
                aResult.mbConverged = iterateRate(
                    fNper, fPayment, fPresentValue, fFutureValue, bPayType, aResult.mfRate);
            }
        }
    }

    return aResult;
}

double computeEffectiveAnnualRate(double fNominalRate, double fPeriods)
{
    return std::pow(1.0 + fNominalRate / fPeriods, fPeriods) - 1.0;
}

double computeNominalAnnualRate(double fEffectiveRate, double fPeriods)
{
    return (std::pow(fEffectiveRate + 1.0, 1.0 / fPeriods) - 1.0) * fPeriods;
}

double computeStraightLineDepreciation(double fCost, double fSalvage, double fLife)
{
    return o3tl::div_allow_zero(fCost - fSalvage, fLife);
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
