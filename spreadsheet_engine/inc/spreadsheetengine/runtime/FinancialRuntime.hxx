/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

#include <vector>

namespace spreadsheetengine::core::finance
{

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateFutureValue(
    double fRate, double fPeriods, double fPayment, double fPresentValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePresentValue(
    double fRate, double fPeriods, double fPayment, double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePayment(
    double fRate, double fPeriods, double fPresentValue, double fFutureValue,
    bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePeriodsForFutureValue(
    double fRate, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateRate(
    double fPeriods, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayInAdvance, double fGuess);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateNominal(
    double fEffectiveRate, double fPeriods);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateEffectiveAnnualRate(
    double fNominalRate, double fPeriods);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateDollarFraction(
    double fDollarDecimal, double fFractionDenominator);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateDollarDecimal(
    double fDollarFraction, double fFractionDenominator);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateInterestSchedulePayment(
    double fRate, double fPeriod, double fTotalPeriods, double fInvestment);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateInterestPayment(
    double fRate, double fPeriod, double fTotalPeriods, double fPresentValue,
    double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePrincipalPayment(
    double fRate, double fPeriod, double fTotalPeriods, double fPresentValue,
    double fFutureValue, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCumulativeInterest(
    double fRate, double fTotalPeriods, double fPresentValue, double fStart,
    double fEnd, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCumulativePrincipal(
    double fRate, double fTotalPeriods, double fPresentValue, double fStart,
    double fEnd, bool bPayInAdvance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateDoubleDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fFactor);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateVariableDecliningBalance(
    double fCost, double fSalvage, double fLife, double fStart,
    double fEnd, double fFactor, bool bNoSwitch);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateFixedDecliningBalance(
    double fCost, double fSalvage, double fLife, double fPeriod, double fMonths);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateStraightLineDepreciation(
    double fCost, double fSalvage, double fLife);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateSumOfYearsDepreciation(
    double fCost, double fSalvage, double fLife, double fPeriod);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateYearFraction(
    const api::DateParts& rNullDate, api::DateSerial nStartDate, api::DateSerial nEndDate,
    sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePrice(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fRate, double fYield, double fRedemption, sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePricemat(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    api::DateSerial nIssue, double fRate, double fYield, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateAmorlinc(
    const api::DateParts& rNullDate, double fCost, api::DateSerial nPurchaseDate,
    api::DateSerial nFirstPeriodEndDate, double fSalvage, double fPeriod, double fRate,
    sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateAmordegrc(
    const api::DateParts& rNullDate, double fCost, api::DateSerial nPurchaseDate,
    api::DateSerial nFirstPeriodEndDate, double fSalvage, double fPeriod, double fRate,
    sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateReceived(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fInvestment, double fDiscount, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateAccrintm(
    const api::DateParts& rNullDate, api::DateSerial nIssue, api::DateSerial nSettlement,
    double fRate, double fParValue, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateDisc(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fPrice, double fRedemption, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePricedisc(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fDiscount, double fRedemption, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateIntrate(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fInvestment, double fRedemption, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateYielddisc(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fPrice, double fRedemption, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateModifiedDuration(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fCoupon, double fYield, sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateYield(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fCoupon, double fPrice, double fRedemption, sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateTbillPrice(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fDiscount);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateTbillEq(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fDiscount);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateTbillYield(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fPrice);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateFutureValueSchedule(
    double fPrincipal, const std::vector<double>& rSchedule);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePaybackDuration(
    double fRate, double fPresentValue, double fFutureValue);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateOddlprice(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    api::DateSerial nLastInterest, double fRate, double fYield, double fRedemption,
    sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateOddlyield(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    api::DateSerial nLastInterest, double fRate, double fPrice, double fRedemption,
    sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCoupdaybs(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCoupdays(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCoupdaysnc(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCouppcd(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCoupncd(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCoupnum(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateIrrNumbers(
    const std::vector<double>& rValues, double fGuess);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateMirrNumbers(
    const std::vector<double>& rValues, double fFinanceRate, double fReinvestRate);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateXirrNumbers(
    const std::vector<double>& rValues, const std::vector<api::DateSerial>& rDates,
    double fGuess);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateNetPresentValueNumbers(
    double fRate, const std::vector<double>& rValues);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateGrowthRateOverPeriods(
    double fPeriods, double fPresentValue, double fFutureValue);

} // namespace spreadsheetengine::core::finance

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
