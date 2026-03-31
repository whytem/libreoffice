/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/types.h>

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

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

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateYearFraction(
    const api::DateParts& rNullDate, api::DateSerial nStartDate, api::DateSerial nEndDate,
    sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePrice(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fRate, double fYield, double fRedemption, sal_Int32 nFrequency, sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateAmorlinc(
    const api::DateParts& rNullDate, double fCost, api::DateSerial nPurchaseDate,
    api::DateSerial nFirstPeriodEndDate, double fSalvage, double fPeriod, double fRate,
    sal_Int32 nBasis);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateOddlyield(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    api::DateSerial nLastInterest, double fRate, double fPrice, double fRedemption,
    sal_Int32 nFrequency, sal_Int32 nBasis);

} // namespace spreadsheetengine::core::finance

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
