/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/FinancialRuntime.hxx>
#include <cstdint>

#include <spreadsheetengine/runtime/MathFinancial.hxx>

#include "DateAlgorithms.hxx"

#include <algorithm>
#include <cmath>
#include <optional>

#include <spreadsheetengine/runtime/FloatingPoint.hxx>
#include <spreadsheetengine/api/Types.hxx>

#include "CoreRuntimeUtils.hxx"

namespace spreadsheetengine::core::finance
{
namespace
{

using spreadsheetengine::core::util::toWholeNumber;
using spreadsheetengine::core::util::makeFiniteResult;

[[nodiscard]] bool isValidPaymentPeriod(double fPeriod, double fTotalPeriods)
{
    return fTotalPeriods > 0.0 && fPeriod >= 1.0 && fPeriod <= fTotalPeriods;
}

[[nodiscard]] bool isValidBasis(std::int32_t nBasis) { return nBasis >= 0 && nBasis <= 4; }

[[nodiscard]] bool isValidCouponFrequency(std::int32_t nFrequency)
{
    return nFrequency == 1 || nFrequency == 2 || nFrequency == 4;
}

[[nodiscard]] spreadsheetengine::api::DateParts dateFromSerial(
    const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nDateSerial)
{
    return spreadsheetengine::core::detail::date::fromAbsoluteDays(
        spreadsheetengine::core::detail::date::toAbsoluteDays(rNullDate) + nDateSerial);
}

[[nodiscard]] std::int32_t getDaysInYears(std::int16_t nYear1, std::int16_t nYear2)
{
    if (nYear1 > nYear2)
        return 0;

    std::int32_t nDayCount = 0;
    for (std::int16_t nYear = nYear1; nYear <= nYear2; ++nYear)
        nDayCount += spreadsheetengine::core::detail::date::isLeapYear(nYear) ? 366 : 365;
    return nDayCount;
}

class FinanceDate
{
private:
    std::uint16_t mnOrigDay = 0;
    std::uint16_t mnDay = 0;
    std::uint16_t mnMonth = 0;
    std::int16_t mnYear = 0;
    bool mbLastDayMode = false;
    bool mbLastDay = false;
    bool mb30Days = false;
    bool mbUSMode = false;

    void setDay()
    {
        if (mb30Days)
        {
            mnDay = std::min(mnOrigDay, static_cast<std::uint16_t>(30));
            if (mbLastDay
                || mnDay >= spreadsheetengine::core::detail::date::getDaysInMonth(mnMonth, mnYear))
            {
                mnDay = 30;
            }
            return;
        }

        const std::uint16_t nLastDay
            = spreadsheetengine::core::detail::date::getDaysInMonth(mnMonth, mnYear);
        mnDay = mbLastDay ? nLastDay : std::min(mnOrigDay, nLastDay);
    }

    [[nodiscard]] std::uint16_t getDaysInMonth() const
    {
        return getDaysInMonth(mnMonth);
    }

    [[nodiscard]] std::uint16_t getDaysInMonth(std::uint16_t nMonth) const
    {
        return mb30Days ? 30
                        : spreadsheetengine::core::detail::date::getDaysInMonth(nMonth, mnYear);
    }

    [[nodiscard]] std::int32_t getDaysInMonthRange(std::uint16_t nFrom, std::uint16_t nTo) const
    {
        if (nFrom > nTo)
            return 0;

        if (mb30Days)
            return static_cast<std::int32_t>(nTo - nFrom + 1) * 30;

        std::int32_t nDayCount = 0;
        for (std::uint16_t nMonth = nFrom; nMonth <= nTo; ++nMonth)
            nDayCount += getDaysInMonth(nMonth);
        return nDayCount;
    }

    [[nodiscard]] std::int32_t getDaysInYearRange(std::int16_t nFrom, std::int16_t nTo) const
    {
        if (nFrom > nTo)
            return 0;
        return mb30Days ? (static_cast<std::int32_t>(nTo - nFrom + 1) * 360)
                        : getDaysInYears(nFrom, nTo);
    }

    [[nodiscard]] bool doAddYears(std::int32_t nYearCount)
    {
        const std::int32_t nNewYear = static_cast<std::int32_t>(mnYear) + nYearCount;
        if (nNewYear < spreadsheetengine::core::detail::date::kYearMin
            || nNewYear > spreadsheetengine::core::detail::date::kYearMax || nNewYear == 0)
        {
            return false;
        }

        mnYear = static_cast<std::int16_t>(nNewYear);
        return true;
    }

public:
    FinanceDate() = default;

    FinanceDate(const spreadsheetengine::api::DateParts& rNullDate,
        spreadsheetengine::api::DateSerial nDate, std::int32_t nBasis)
    {
        const spreadsheetengine::api::DateParts aDate = dateFromSerial(rNullDate, nDate);
        mnOrigDay = static_cast<std::uint16_t>(aDate.mnDay);
        mnMonth = static_cast<std::uint16_t>(aDate.mnMonth);
        mnYear = static_cast<std::int16_t>(aDate.mnYear);
        mbLastDayMode = nBasis != 5;
        mbLastDay = mnOrigDay
                    >= spreadsheetengine::core::detail::date::getDaysInMonth(mnMonth, mnYear);
        mb30Days = nBasis == 0 || nBasis == 4;
        mbUSMode = nBasis == 0;
        setDay();
    }

    [[nodiscard]] std::uint16_t getMonth() const { return mnMonth; }
    [[nodiscard]] std::int16_t getYear() const { return mnYear; }

    [[nodiscard]] bool setYear(std::int16_t nNewYear)
    {
        if (nNewYear == 0)
            return false;
        mnYear = nNewYear;
        setDay();
        return true;
    }

    [[nodiscard]] bool addYears(std::int32_t nYearCount)
    {
        if (!doAddYears(nYearCount))
            return false;
        setDay();
        return true;
    }

    [[nodiscard]] bool addMonths(std::int32_t nMonthCount)
    {
        std::int32_t nNewMonth = nMonthCount + static_cast<std::int32_t>(mnMonth);
        if (nNewMonth > 12)
        {
            --nNewMonth;
            if (!doAddYears(nNewMonth / 12))
                return false;
            mnMonth = static_cast<std::uint16_t>(nNewMonth % 12) + 1;
        }
        else if (nNewMonth < 1)
        {
            if (!doAddYears(nNewMonth / 12 - 1))
                return false;
            mnMonth = static_cast<std::uint16_t>(nNewMonth % 12 + 12);
        }
        else
        {
            mnMonth = static_cast<std::uint16_t>(nNewMonth);
        }

        setDay();
        return true;
    }

    [[nodiscard]] spreadsheetengine::api::DateSerial
    getDate(const spreadsheetengine::api::DateParts& rNullDate) const
    {
        const std::uint16_t nLastDay
            = spreadsheetengine::core::detail::date::getDaysInMonth(mnMonth, mnYear);
        const std::uint16_t nRealDay = (mbLastDayMode && mbLastDay) ? nLastDay
                                                                 : std::min(nLastDay, mnOrigDay);
        const spreadsheetengine::api::DateParts aDate { mnYear, static_cast<std::int16_t>(mnMonth),
            static_cast<std::int16_t>(nRealDay) };
        return spreadsheetengine::core::detail::date::toAbsoluteDays(aDate)
               - spreadsheetengine::core::detail::date::toAbsoluteDays(rNullDate);
    }

    [[nodiscard]] bool operator<(const FinanceDate& rOther) const
    {
        if (mnYear != rOther.mnYear)
            return mnYear < rOther.mnYear;
        if (mnMonth != rOther.mnMonth)
            return mnMonth < rOther.mnMonth;
        if (mnDay != rOther.mnDay)
            return mnDay < rOther.mnDay;
        if (mbLastDay || rOther.mbLastDay)
            return !mbLastDay && rOther.mbLastDay;
        return mnOrigDay < rOther.mnOrigDay;
    }

    [[nodiscard]] bool operator>(const FinanceDate& rOther) const { return rOther < *this; }

    [[nodiscard]] static std::optional<std::int32_t> getDiff(FinanceDate aFrom, FinanceDate aTo)
    {
        if (aFrom > aTo)
            std::swap(aFrom, aTo);

        std::int32_t nDiff = 0;
        if (aTo.mb30Days)
        {
            if (aTo.mbUSMode)
            {
                if (((aFrom.mnMonth == 2) || (aFrom.mnDay < 30)) && (aTo.mnOrigDay == 31))
                    aTo.mnDay = 31;
                else if ((aTo.mnMonth == 2) && aTo.mbLastDay)
                    aTo.mnDay = spreadsheetengine::core::detail::date::getDaysInMonth(
                        2, aTo.mnYear);
            }
            else
            {
                if ((aFrom.mnMonth == 2) && (aFrom.mnDay == 30))
                    aFrom.mnDay = spreadsheetengine::core::detail::date::getDaysInMonth(
                        2, aFrom.mnYear);
                if ((aTo.mnMonth == 2) && (aTo.mnDay == 30))
                    aTo.mnDay = spreadsheetengine::core::detail::date::getDaysInMonth(
                        2, aTo.mnYear);
            }
        }

        if ((aFrom.mnYear < aTo.mnYear)
            || ((aFrom.mnYear == aTo.mnYear) && (aFrom.mnMonth < aTo.mnMonth)))
        {
            nDiff = aFrom.getDaysInMonth() - aFrom.mnDay + 1;
            aFrom.mnOrigDay = aFrom.mnDay = 1;
            aFrom.mbLastDay = false;
            if (!aFrom.addMonths(1))
                return std::nullopt;

            if (aFrom.mnYear < aTo.mnYear)
            {
                nDiff += aFrom.getDaysInMonthRange(aFrom.mnMonth, 12);
                if (!aFrom.addMonths(13 - static_cast<std::int32_t>(aFrom.mnMonth)))
                    return std::nullopt;

                nDiff += aFrom.getDaysInYearRange(aFrom.mnYear, aTo.mnYear - 1);
                if (!aFrom.addYears(aTo.mnYear - aFrom.mnYear))
                    return std::nullopt;
            }

            nDiff += aFrom.getDaysInMonthRange(aFrom.mnMonth, aTo.mnMonth - 1);
            if (!aFrom.addMonths(aTo.mnMonth - aFrom.mnMonth))
                return std::nullopt;
        }

        nDiff += aTo.mnDay - aFrom.mnDay;
        return std::max<std::int32_t>(nDiff, 0);
    }
};

[[nodiscard]] std::optional<FinanceDate> getPreviousCouponDate(
    const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nSettlement,
    spreadsheetengine::api::DateSerial nMaturity, std::int32_t nFrequency, std::int32_t nBasis)
{
    if (nSettlement >= nMaturity || !isValidCouponFrequency(nFrequency))
        return std::nullopt;

    const FinanceDate aSettlement(rNullDate, nSettlement, nBasis);
    const FinanceDate aMaturity(rNullDate, nMaturity, nBasis);
    FinanceDate aDate = aMaturity;
    if (!aDate.setYear(aSettlement.getYear()))
        return std::nullopt;
    if (aDate < aSettlement && !aDate.addYears(1))
        return std::nullopt;
    while (aDate > aSettlement)
    {
        if (!aDate.addMonths(-12 / nFrequency))
            return std::nullopt;
    }
    return aDate;
}

[[nodiscard]] std::optional<FinanceDate> getNextCouponDate(
    const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nSettlement,
    spreadsheetengine::api::DateSerial nMaturity, std::int32_t nFrequency, std::int32_t nBasis)
{
    if (nSettlement >= nMaturity || !isValidCouponFrequency(nFrequency))
        return std::nullopt;

    const FinanceDate aSettlement(rNullDate, nSettlement, nBasis);
    const FinanceDate aMaturity(rNullDate, nMaturity, nBasis);
    FinanceDate aDate = aMaturity;
    if (!aDate.setYear(aSettlement.getYear()))
        return std::nullopt;
    if (aDate > aSettlement && !aDate.addYears(-1))
        return std::nullopt;
    while (!(aDate > aSettlement))
    {
        if (!aDate.addMonths(12 / nFrequency))
            return std::nullopt;
    }
    return aDate;
}

[[nodiscard]] std::optional<double> getCouponDayBasis(
    const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nSettlement,
    spreadsheetengine::api::DateSerial nMaturity, std::int32_t nFrequency, std::int32_t nBasis)
{
    const auto oPrevious = getPreviousCouponDate(
        rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
    if (!oPrevious)
        return std::nullopt;

    const FinanceDate aSettlement(rNullDate, nSettlement, nBasis);
    const auto oDiff = FinanceDate::getDiff(*oPrevious, aSettlement);
    if (!oDiff)
        return std::nullopt;
    return static_cast<double>(*oDiff);
}

[[nodiscard]] std::optional<double> getCouponDays(
    const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nSettlement,
    spreadsheetengine::api::DateSerial nMaturity, std::int32_t nFrequency, std::int32_t nBasis)
{
    if (nSettlement >= nMaturity || !isValidCouponFrequency(nFrequency))
        return std::nullopt;

    if (nBasis == 1)
    {
        auto oPrevious = getPreviousCouponDate(
            rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
        if (!oPrevious)
            return std::nullopt;

        FinanceDate aNext = *oPrevious;
        if (!aNext.addMonths(12 / nFrequency))
            return std::nullopt;

        const auto oDiff = FinanceDate::getDiff(*oPrevious, aNext);
        if (!oDiff)
            return std::nullopt;
        return static_cast<double>(*oDiff);
    }

    switch (nBasis)
    {
        case 0:
        case 2:
        case 4:
            return 360.0 / static_cast<double>(nFrequency);
        case 3:
            return 365.0 / static_cast<double>(nFrequency);
        default:
            return std::nullopt;
    }
}

[[nodiscard]] std::optional<double> getCouponDaysNext(
    const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nSettlement,
    spreadsheetengine::api::DateSerial nMaturity, std::int32_t nFrequency, std::int32_t nBasis)
{
    if (nSettlement >= nMaturity || !isValidCouponFrequency(nFrequency))
        return std::nullopt;

    if (nBasis != 0 && nBasis != 4)
    {
        const auto oNext = getNextCouponDate(rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
        if (!oNext)
            return std::nullopt;

        const FinanceDate aSettlement(rNullDate, nSettlement, nBasis);
        const auto oDiff = FinanceDate::getDiff(aSettlement, *oNext);
        if (!oDiff)
            return std::nullopt;
        return static_cast<double>(*oDiff);
    }

    const auto oCouponDays = getCouponDays(rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
    const auto oCouponDayBasis = getCouponDayBasis(
        rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
    if (!oCouponDays || !oCouponDayBasis)
        return std::nullopt;
    return *oCouponDays - *oCouponDayBasis;
}

[[nodiscard]] std::optional<double> getCouponCount(
    const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nSettlement,
    spreadsheetengine::api::DateSerial nMaturity, std::int32_t nFrequency, std::int32_t nBasis)
{
    if (nSettlement >= nMaturity || !isValidCouponFrequency(nFrequency))
        return std::nullopt;

    const FinanceDate aMaturity(rNullDate, nMaturity, nBasis);
    const auto oPrevious = getPreviousCouponDate(
        rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
    if (!oPrevious)
        return std::nullopt;

    const std::int32_t nMonths
        = (static_cast<std::int32_t>(aMaturity.getYear()) - static_cast<std::int32_t>(oPrevious->getYear()))
              * 12
          + static_cast<std::int32_t>(aMaturity.getMonth())
          - static_cast<std::int32_t>(oPrevious->getMonth());
    return static_cast<double>(nMonths * nFrequency / 12);
}

[[nodiscard]] std::optional<double> computeYearFractionValue(
    const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nStartDate,
    spreadsheetengine::api::DateSerial nEndDate, std::int32_t nBasis)
{
    if (nStartDate == nEndDate)
        return 0.0;

    if (nStartDate > nEndDate)
        std::swap(nStartDate, nEndDate);

    const spreadsheetengine::api::DateParts aStart = dateFromSerial(rNullDate, nStartDate);
    const spreadsheetengine::api::DateParts aEnd = dateFromSerial(rNullDate, nEndDate);

    std::uint16_t nDay1 = static_cast<std::uint16_t>(aStart.mnDay);
    std::uint16_t nDay2 = static_cast<std::uint16_t>(aEnd.mnDay);
    std::uint16_t nMonth1 = static_cast<std::uint16_t>(aStart.mnMonth);
    std::uint16_t nMonth2 = static_cast<std::uint16_t>(aEnd.mnMonth);
    const std::int16_t nYear1 = static_cast<std::int16_t>(aStart.mnYear);
    const std::int16_t nYear2 = static_cast<std::int16_t>(aEnd.mnYear);

    std::int32_t nDayDiff = 0;
    switch (nBasis)
    {
        case 0:
            if (nDay1 == 31)
                --nDay1;
            if (nDay1 == 30 && nDay2 == 31)
            {
                --nDay2;
            }
            else if (nMonth1 == 2
                     && nDay1
                            == (spreadsheetengine::core::detail::date::isLeapYear(nYear1) ? 29
                                                                                          : 28))
            {
                nDay1 = 30;
                if (nMonth2 == 2
                    && nDay2
                           == (spreadsheetengine::core::detail::date::isLeapYear(nYear2) ? 29
                                                                                         : 28))
                {
                    nDay2 = 30;
                }
            }
            nDayDiff = (nYear2 - nYear1) * 360 + (nMonth2 - nMonth1) * 30 + (nDay2 - nDay1);
            break;
        case 1:
        case 2:
        case 3:
            nDayDiff = nEndDate - nStartDate;
            break;
        case 4:
            if (nDay1 == 31)
                --nDay1;
            if (nDay2 == 31)
                --nDay2;
            nDayDiff = (nYear2 - nYear1) * 360 + (nMonth2 - nMonth1) * 30 + (nDay2 - nDay1);
            break;
        default:
            return std::nullopt;
    }

    double fDaysInYear = 0.0;
    switch (nBasis)
    {
        case 0:
        case 2:
        case 4:
            fDaysInYear = 360.0;
            break;
        case 1:
        {
            const bool bDifferentYears = nYear1 != nYear2;
            if (bDifferentYears
                && ((nYear2 != nYear1 + 1) || (nMonth1 < nMonth2)
                    || (nMonth1 == nMonth2 && nDay1 < nDay2)))
            {
                fDaysInYear = static_cast<double>(getDaysInYears(nYear1, nYear2))
                              / static_cast<double>(nYear2 - nYear1 + 1);
            }
            else if (!bDifferentYears
                     && spreadsheetengine::core::detail::date::isLeapYear(nYear1))
            {
                fDaysInYear = 366.0;
            }
            else if (bDifferentYears
                     && ((spreadsheetengine::core::detail::date::isLeapYear(nYear1)
                             && ((nMonth1 < 2) || (nMonth1 == 2 && nDay1 <= 29)))
                         || (spreadsheetengine::core::detail::date::isLeapYear(nYear2)
                             && (nMonth2 > 2 || (nMonth2 == 2 && nDay2 == 29)))))
            {
                fDaysInYear = 366.0;
            }
            else
            {
                fDaysInYear = 365.0;
            }
            break;
        }
        case 3:
            fDaysInYear = 365.0;
            break;
        default:
            return std::nullopt;
    }

    return static_cast<double>(nDayDiff) / fDaysInYear;
}

} // namespace

api::ValueResult<double> evaluateFutureValue(
    double fRate, double fPeriods, double fPayment, double fPresentValue, bool bPayInAdvance)
{
    return makeFiniteResult(spreadsheetengine::core::math::computeFutureValue(
        fRate, fPeriods, fPayment, fPresentValue, bPayInAdvance));
}

api::ValueResult<double> evaluatePresentValue(
    double fRate, double fPeriods, double fPayment, double fFutureValue, bool bPayInAdvance)
{
    return makeFiniteResult(spreadsheetengine::core::math::computePresentValue(
        fRate, fPeriods, fPayment, fFutureValue, bPayInAdvance));
}

api::ValueResult<double> evaluatePayment(
    double fRate, double fPeriods, double fPresentValue, double fFutureValue,
    bool bPayInAdvance)
{
    if (fp::approxEqual(fPeriods, 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteResult(spreadsheetengine::core::math::computePayment(
        fRate, fPeriods, fPresentValue, fFutureValue, bPayInAdvance));
}

api::ValueResult<double> evaluatePeriodsForFutureValue(
    double fRate, double fPayment, double fPresentValue, double fFutureValue,
    bool bPayInAdvance)
{
    return makeFiniteResult(spreadsheetengine::core::math::computePeriodsForFutureValue(
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
    return makeFiniteResult(aRateResult.mfRate);
}

api::ValueResult<double> evaluateInterestSchedulePayment(
    double fRate, double fPeriod, double fTotalPeriods, double fInvestment)
{
    if (fp::approxEqual(fTotalPeriods, 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteResult(
        spreadsheetengine::core::math::computeInterestSchedulePayment(
            fRate, fPeriod, fTotalPeriods, fInvestment));
}

api::ValueResult<double> evaluateInterestPayment(
    double fRate, double fPeriod, double fTotalPeriods, double fPresentValue,
    double fFutureValue, bool bPayInAdvance)
{
    if (!isValidPaymentPeriod(fPeriod, fTotalPeriods))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return makeFiniteResult(spreadsheetengine::core::math::computeInterestPayment(
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

    return makeFiniteResult(spreadsheetengine::core::math::computePrincipalPayment(
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

    return makeFiniteResult(spreadsheetengine::core::math::computeCumulativeInterest(
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

    return makeFiniteResult(spreadsheetengine::core::math::computeCumulativePrincipal(
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

    return makeFiniteResult(spreadsheetengine::core::math::computeDoubleDecliningBalance(
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

    return makeFiniteResult(spreadsheetengine::core::math::computeVariableDecliningBalance(
        fCost, fSalvage, fLife, fStart, fEnd, fFactor, bNoSwitch));
}

api::ValueResult<double> evaluateYearFraction(
    const api::DateParts& rNullDate, api::DateSerial nStartDate, api::DateSerial nEndDate,
    std::int32_t nBasis)
{
    const auto oYearFraction = computeYearFractionValue(rNullDate, nStartDate, nEndDate, nBasis);
    if (!oYearFraction)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(*oYearFraction);
}

api::ValueResult<double> evaluatePrice(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    double fRate, double fYield, double fRedemption, std::int32_t nFrequency, std::int32_t nBasis)
{
    if (fYield < 0.0 || fRate < 0.0 || !(fRedemption > 0.0)
        || !isValidCouponFrequency(nFrequency) || nSettlement >= nMaturity)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    if (!isValidBasis(nBasis))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const auto oCouponDays = getCouponDays(rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
    const auto oCouponDaysNext
        = getCouponDaysNext(rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
    const auto oCouponCount = getCouponCount(rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
    const auto oCouponDayBasis
        = getCouponDayBasis(rNullDate, nSettlement, nMaturity, nFrequency, nBasis);
    if (!oCouponDays || !oCouponDaysNext || !oCouponCount || !oCouponDayBasis
        || fp::approxEqual(*oCouponDays, 0.0))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const double fFrequency = static_cast<double>(nFrequency);
    const double fDiscountFactor = *oCouponDaysNext / *oCouponDays;
    double fPrice = fRedemption
                    / std::pow(1.0 + fYield / fFrequency, *oCouponCount - 1.0 + fDiscountFactor);
    fPrice -= 100.0 * fRate / fFrequency * *oCouponDayBasis / *oCouponDays;

    const double fCouponAmount = 100.0 * fRate / fFrequency;
    const double fYieldFactor = 1.0 + fYield / fFrequency;
    for (double fCouponIndex = 0.0; fCouponIndex < *oCouponCount; ++fCouponIndex)
        fPrice += fCouponAmount / std::pow(fYieldFactor, fCouponIndex + fDiscountFactor);

    return makeFiniteResult(fPrice);
}

api::ValueResult<double> evaluateAmorlinc(
    const api::DateParts& rNullDate, double fCost, api::DateSerial nPurchaseDate,
    api::DateSerial nFirstPeriodEndDate, double fSalvage, double fPeriod, double fRate,
    std::int32_t nBasis)
{
    if (nPurchaseDate > nFirstPeriodEndDate || !(fRate > 0.0) || fSalvage > fCost
        || !(fCost > 0.0) || fSalvage < 0.0 || fPeriod < 0.0 || !isValidBasis(nBasis))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const auto oFirstPeriodFraction
        = computeYearFractionValue(rNullDate, nPurchaseDate, nFirstPeriodEndDate, nBasis);
    if (!oFirstPeriodFraction)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const std::uint32_t nPeriod = static_cast<std::uint32_t>(fPeriod);
    const double fDepreciationPerPeriod = fCost * fRate;
    const double fDepreciableCost = fCost - fSalvage;
    const double fInitialDepreciation = *oFirstPeriodFraction * fRate * fCost;
    const std::uint32_t nFullPeriods = static_cast<std::uint32_t>(
        (fCost - fSalvage - fInitialDepreciation) / fDepreciationPerPeriod);

    double fResult = 0.0;
    if (nPeriod == 0)
    {
        fResult = fInitialDepreciation;
    }
    else if (nPeriod <= nFullPeriods)
    {
        fResult = fDepreciationPerPeriod;
    }
    else if (nPeriod == nFullPeriods + 1)
    {
        fResult = fDepreciableCost - fDepreciationPerPeriod * nFullPeriods
                  - fInitialDepreciation;
    }

    return makeFiniteResult(std::max(fResult, 0.0));
}

api::ValueResult<double> evaluateOddlyield(
    const api::DateParts& rNullDate, api::DateSerial nSettlement, api::DateSerial nMaturity,
    api::DateSerial nLastInterest, double fRate, double fPrice, double fRedemption,
    std::int32_t nFrequency, std::int32_t nBasis)
{
    if (!(fRate > 0.0) || !(fPrice > 0.0) || !(fRedemption > 0.0)
        || !isValidCouponFrequency(nFrequency) || nMaturity <= nSettlement
        || nSettlement <= nLastInterest || !isValidBasis(nBasis))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const double fFrequency = static_cast<double>(nFrequency);
    const auto oLastToMaturity
        = computeYearFractionValue(rNullDate, nLastInterest, nMaturity, nBasis);
    const auto oSettlementToMaturity
        = computeYearFractionValue(rNullDate, nSettlement, nMaturity, nBasis);
    const auto oLastToSettlement
        = computeYearFractionValue(rNullDate, nLastInterest, nSettlement, nBasis);
    if (!oLastToMaturity || !oSettlementToMaturity || !oLastToSettlement)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const double fDCi = *oLastToMaturity * fFrequency;
    const double fDSCi = *oSettlementToMaturity * fFrequency;
    const double fAi = *oLastToSettlement * fFrequency;

    double fYield = fRedemption + fDCi * 100.0 * fRate / fFrequency;
    fYield /= fPrice + fAi * 100.0 * fRate / fFrequency;
    fYield -= 1.0;
    fYield *= fFrequency / fDSCi;
    return makeFiniteResult(fYield);
}

} // namespace spreadsheetengine::core::finance

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
