/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::math
{

struct FinancialInterestPayment
{
    double mfInterest = 0.0;
    double mfPayment = 0.0;
};

struct FinancialRateResult
{
    bool mbConverged = false;
    double mfRate = 0.0;
};

SPREADSHEETENGINE_DLLPUBLIC double computePresentValue(
    double fRate, double fNper, double fPayment, double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC double computeInterestSchedulePayment(
    double fRate, double fPeriod, double fTotalPeriods, double fInvestment);

SPREADSHEETENGINE_DLLPUBLIC double computeSumOfYearsDepreciation(
    double fCost, double fSalvage, double fLife, double fPeriod);

SPREADSHEETENGINE_DLLPUBLIC double computeDoubleDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fFactor);

SPREADSHEETENGINE_DLLPUBLIC double computeFixedDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fMonths);

SPREADSHEETENGINE_DLLPUBLIC double computeVariableDecliningBalanceSegment(
    double fCost, double fSalvage, double fLife, double fRemainingLife,
    double fPeriod, double fFactor);

SPREADSHEETENGINE_DLLPUBLIC double computeVariableDecliningBalance(
    double fCost, double fSalvage, double fLife, double fStart,
    double fEnd, double fFactor, bool bNoSwitch);

SPREADSHEETENGINE_DLLPUBLIC double computePayment(
    double fRate, double fNper, double fPresentValue, double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC double computeFutureValue(
    double fRate, double fNper, double fPayment, double fPresentValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC FinancialInterestPayment computeInterestPayment(
    double fRate, double fPer, double fNper, double fPresentValue,
    double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC double computePrincipalPayment(
    double fRate, double fPer, double fNper, double fPresentValue,
    double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC double computeCumulativeInterest(
    double fRate, double fStart, double fEnd, double fNper,
    double fPresentValue, double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC double computeCumulativePrincipal(
    double fRate, double fStart, double fEnd, double fNper,
    double fPresentValue, double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC double computePaybackDuration(
    double fRate, double fPresentValue, double fFutureValue);

SPREADSHEETENGINE_DLLPUBLIC double computeGrowthRateOverPeriods(
    double fPeriods, double fPresentValue, double fFutureValue);

SPREADSHEETENGINE_DLLPUBLIC double computePeriodsForFutureValue(
    double fRate, double fPayment, double fPresentValue,
    double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC FinancialRateResult solveRate(
    double fNper, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayType, double fGuess, bool bAllowAlternateGuesses);

SPREADSHEETENGINE_DLLPUBLIC double computeEffectiveAnnualRate(
    double fNominalRate, double fPeriods);

SPREADSHEETENGINE_DLLPUBLIC double computeNominalAnnualRate(
    double fEffectiveRate, double fPeriods);

SPREADSHEETENGINE_DLLPUBLIC double computeStraightLineDepreciation(
    double fCost, double fSalvage, double fLife);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
