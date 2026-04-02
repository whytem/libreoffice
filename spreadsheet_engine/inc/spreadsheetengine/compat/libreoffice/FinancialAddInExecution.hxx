/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <functional>
#include <optional>

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Types.hxx>
#include <spreadsheetengine/runtime/FinancialRuntime.hxx>

namespace spreadsheetengine::compat::libreoffice::financialaddinexecution
{

class DirectFinancialAddInAdapter
{
    std::optional<spreadsheetengine::api::DateParts> moNullDate;
    std::optional<sal_Int32> mnBasis;

public:
    DirectFinancialAddInAdapter() = default;

    explicit DirectFinancialAddInAdapter(const spreadsheetengine::api::DateParts& rNullDate)
        : moNullDate(rNullDate)
    {
    }

    DirectFinancialAddInAdapter(
        const spreadsheetengine::api::DateParts& rNullDate, sal_Int32 nBasis)
        : moNullDate(rNullDate)
        , mnBasis(nBasis)
    {
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double> evaluateEffect(
        double fNominalRate, double fPeriods)
    {
        return spreadsheetengine::core::finance::evaluateEffectiveAnnualRate(
            fNominalRate, fPeriods);
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double> evaluateNominal(
        double fEffectiveRate, double fPeriods)
    {
        return spreadsheetengine::core::finance::evaluateNominal(fEffectiveRate, fPeriods);
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double> evaluateDollarFraction(
        double fDollarDecimal, double fFractionDenominator)
    {
        return spreadsheetengine::core::finance::evaluateDollarFraction(
            fDollarDecimal, fFractionDenominator);
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double> evaluateDollarDecimal(
        double fDollarFraction, double fFractionDenominator)
    {
        return spreadsheetengine::core::finance::evaluateDollarDecimal(
            fDollarFraction, fFractionDenominator);
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double>
    evaluateCumulativePrincipal(double fRate, double fTotalPeriods, double fPresentValue,
        double fStart, double fEnd, bool bPayInAdvance)
    {
        return spreadsheetengine::core::finance::evaluateCumulativePrincipal(
            fRate, fTotalPeriods, fPresentValue, fStart, fEnd, bPayInAdvance);
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double>
    evaluateCumulativeInterest(double fRate, double fTotalPeriods, double fPresentValue,
        double fStart, double fEnd, bool bPayInAdvance)
    {
        return spreadsheetengine::core::finance::evaluateCumulativeInterest(
            fRate, fTotalPeriods, fPresentValue, fStart, fEnd, bPayInAdvance);
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double> evaluateFutureValueSchedule(
        double fPrincipal, const std::vector<double>& rSchedule)
    {
        return spreadsheetengine::core::finance::evaluateFutureValueSchedule(
            fPrincipal, rSchedule);
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double> evaluateXirrNumbers(
        const std::vector<double>& rValues,
        const std::vector<spreadsheetengine::api::DateSerial>& rDates, double fGuess)
    {
        return spreadsheetengine::core::finance::evaluateXirrNumbers(rValues, rDates, fGuess);
    }

    [[nodiscard]] static spreadsheetengine::api::ValueResult<double> evaluateXnpvNumbers(
        double fRate, const std::vector<double>& rValues,
        const std::vector<spreadsheetengine::api::DateSerial>& rDates)
    {
        return spreadsheetengine::core::finance::evaluateXnpvNumbers(fRate, rValues, rDates);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateAccrint(
        spreadsheetengine::api::DateSerial nIssue, spreadsheetengine::api::DateSerial nSettlement,
        double fRate, double fParValue, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(
            spreadsheetengine::core::finance::evaluateAccrint, nIssue, nSettlement, fRate,
            fParValue, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateAccrintm(
        spreadsheetengine::api::DateSerial nIssue, spreadsheetengine::api::DateSerial nSettlement,
        double fRate, double fParValue) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateAccrintm, nIssue,
            nSettlement, fRate, fParValue);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateAmordegrc(double fCost,
        spreadsheetengine::api::DateSerial nPurchaseDate,
        spreadsheetengine::api::DateSerial nFirstPeriodEndDate, double fSalvage, double fPeriod,
        double fRate) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateAmordegrc, fCost,
            nPurchaseDate, nFirstPeriodEndDate, fSalvage, fPeriod, fRate);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateAmorlinc(double fCost,
        spreadsheetengine::api::DateSerial nPurchaseDate,
        spreadsheetengine::api::DateSerial nFirstPeriodEndDate, double fSalvage, double fPeriod,
        double fRate) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateAmorlinc, fCost,
            nPurchaseDate, nFirstPeriodEndDate, fSalvage, fPeriod, fRate);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateDuration(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fCoupon, double fYield,
        sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(
            spreadsheetengine::core::finance::evaluateDuration, nSettlement, nMaturity, fCoupon,
            fYield, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateDisc(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fPrice,
        double fRedemption) const
    {
        return evaluateWithDateMode(
            spreadsheetengine::core::finance::evaluateDisc, nSettlement, nMaturity, fPrice,
            fRedemption);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateIntrate(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fInvestment,
        double fRedemption) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateIntrate,
            nSettlement, nMaturity, fInvestment, fRedemption);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateModifiedDuration(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fCoupon, double fYield,
        sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateModifiedDuration,
            nSettlement, nMaturity, fCoupon, fYield, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluatePrice(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fRate, double fYield,
        double fRedemption, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluatePrice, nSettlement,
            nMaturity, fRate, fYield, fRedemption, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluatePricedisc(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fDiscount,
        double fRedemption) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluatePricedisc,
            nSettlement, nMaturity, fDiscount, fRedemption);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluatePricemat(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, spreadsheetengine::api::DateSerial nIssue,
        double fRate, double fYield) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluatePricemat,
            nSettlement, nMaturity, nIssue, fRate, fYield);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateReceived(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fInvestment,
        double fDiscount) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateReceived,
            nSettlement, nMaturity, fInvestment, fDiscount);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateYieldmat(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity,
        spreadsheetengine::api::DateSerial nIssue, double fRate, double fPrice) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateYieldmat,
            nSettlement, nMaturity, nIssue, fRate, fPrice);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateTbillEq(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fDiscount) const
    {
        return evaluateWithNullDate(spreadsheetengine::core::finance::evaluateTbillEq, nSettlement,
            nMaturity, fDiscount);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateTbillPrice(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fDiscount) const
    {
        return evaluateWithNullDate(spreadsheetengine::core::finance::evaluateTbillPrice,
            nSettlement, nMaturity, fDiscount);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateTbillYield(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fPrice) const
    {
        return evaluateWithNullDate(spreadsheetengine::core::finance::evaluateTbillYield,
            nSettlement, nMaturity, fPrice);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateYield(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fCoupon, double fPrice,
        double fRedemption, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateYield,
            nSettlement, nMaturity, fCoupon, fPrice, fRedemption, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateYielddisc(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, double fPrice,
        double fRedemption) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateYielddisc,
            nSettlement, nMaturity, fPrice, fRedemption);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateOddlprice(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity,
        spreadsheetengine::api::DateSerial nLastInterest, double fRate, double fYield,
        double fRedemption, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateOddlprice,
            nSettlement, nMaturity, nLastInterest, fRate, fYield, fRedemption, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateOddlyield(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity,
        spreadsheetengine::api::DateSerial nLastInterest, double fRate, double fPrice,
        double fRedemption, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateOddlyield,
            nSettlement, nMaturity, nLastInterest, fRate, fPrice, fRedemption, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateCoupdaybs(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateCoupdaybs,
            nSettlement, nMaturity, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateCoupdays(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateCoupdays,
            nSettlement, nMaturity, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateCoupdaysnc(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateCoupdaysnc,
            nSettlement, nMaturity, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateCouppcd(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateCouppcd,
            nSettlement, nMaturity, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateCoupncd(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateCoupncd,
            nSettlement, nMaturity, nFrequency);
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateCoupnum(
        spreadsheetengine::api::DateSerial nSettlement,
        spreadsheetengine::api::DateSerial nMaturity, sal_Int32 nFrequency) const
    {
        return evaluateWithDateMode(spreadsheetengine::core::finance::evaluateCoupnum,
            nSettlement, nMaturity, nFrequency);
    }

private:
    template <typename Function, typename... Args>
    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateWithNullDate(
        Function&& rFunction, Args&&... rArgs) const
    {
        return std::invoke(std::forward<Function>(rFunction), *moNullDate,
            std::forward<Args>(rArgs)...);
    }

    template <typename Function, typename... Args>
    [[nodiscard]] spreadsheetengine::api::ValueResult<double> evaluateWithDateMode(
        Function&& rFunction, Args&&... rArgs) const
    {
        return std::invoke(std::forward<Function>(rFunction), *moNullDate,
            std::forward<Args>(rArgs)..., *mnBasis);
    }
};

} // namespace spreadsheetengine::compat::libreoffice::financialaddinexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
