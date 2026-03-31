/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

#include <algorithm>
#include <array>

namespace spreadsheetengine::core::eval
{
namespace
{

template <std::size_t N>
[[nodiscard]] bool matchesFunctionRegistry(
    api::StringView rFunctionName, const std::array<api::StringView, N>& rRegistry)
{
    return std::find(rRegistry.begin(), rRegistry.end(), rFunctionName) != rRegistry.end();
}

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateFinancialFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kFinancialFunctions{
        api::StringView(u"FV"),
        api::StringView(u"PV"),
        api::StringView(u"PMT"),
        api::StringView(u"NPER"),
        api::StringView(u"RATE"),
        api::StringView(u"ISPMT"),
        api::StringView(u"IPMT"),
        api::StringView(u"PPMT"),
        api::StringView(u"CUMIPMT"),
        api::StringView(u"CUMPRINC"),
        api::StringView(u"DDB"),
        api::StringView(u"VDB"),
        api::StringView(u"YEARFRAC"),
        api::StringView(u"GETYEARFRAC"),
        api::StringView(u"PRICE"),
        api::StringView(u"AMORLINC"),
        api::StringView(u"GETAMORLINC"),
        api::StringView(u"ODDLYIELD"),
        api::StringView(u"GETODDLYIELD"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kFinancialFunctions))
        return std::nullopt;
    return evaluateFinancialFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateFinancialFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    FunctionEvalContext aContext { *this, rNode, rCurrentAddress };

if (aFunctionName == u"FV" || aFunctionName == u"PV" || aFunctionName == u"PMT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aNper = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aNper)
            return makeFailure(aNper.meError);
        const auto aPayment = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);

        double fOptionalEndpointValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aEndpointValue = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aEndpointValue)
                return makeFailure(aEndpointValue.meError);
            fOptionalEndpointValue = aEndpointValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 5)
        {
            const auto aPayType = aContext.evaluatePayTypeArgument(*rNode.maChildren[4], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        if (aFunctionName == u"FV")
        {
            const auto aFutureValue = sefinance::evaluateFutureValue(
                aRate.maValue, aNper.maValue, aPayment.maValue, fOptionalEndpointValue,
                bPayInAdvance);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            return makeScalarResult(api::CellValue::number(aFutureValue.maValue));
        }

        if (aFunctionName == u"PV")
        {
            const auto aPresentValue = sefinance::evaluatePresentValue(
                aRate.maValue, aNper.maValue, aPayment.maValue, fOptionalEndpointValue,
                bPayInAdvance);
            if (!aPresentValue)
                return makeFailure(aPresentValue.meError);
            return makeScalarResult(api::CellValue::number(aPresentValue.maValue));
        }

        if (aFunctionName == u"PMT")
        {
            const auto aPaymentValue = sefinance::evaluatePayment(
                aRate.maValue, aNper.maValue, aPayment.maValue, fOptionalEndpointValue,
                bPayInAdvance);
            if (!aPaymentValue)
                return makeFailure(aPaymentValue.meError);
            return makeScalarResult(api::CellValue::number(aPaymentValue.maValue));
        }

    }

    if (aFunctionName == u"NPER")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPayment = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);
        const auto aPresentValue = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFutureValue = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 5)
        {
            const auto aPayType = aContext.evaluatePayTypeArgument(*rNode.maChildren[4], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        const auto aPeriods = sefinance::evaluatePeriodsForFutureValue(
            aRate.maValue, aPayment.maValue, aPresentValue.maValue, fFutureValue,
            bPayInAdvance);
        if (!aPeriods)
            return makeFailure(aPeriods.meError);
        return makeScalarResult(api::CellValue::number(aPeriods.maValue));
    }

    if (aFunctionName == u"RATE")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aNper = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aNper)
            return makeFailure(aNper.meError);
        const auto aPayment = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);
        const auto aPresentValue = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFutureValue = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() >= 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                bPayInAdvance = false;
            else
            {
                const auto aPayTypeValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[4]);
                if (!aPayTypeValue)
                    return makeFailure(aPayTypeValue.meError);
                if (aPayTypeValue.maValue.isEmpty())
                    return makeFailure(api::Error::IllegalArgument);

                const auto aPayType = coerceToBoolean(aPayTypeValue.maValue);
                if (!aPayType)
                    return makeFailure(aPayType.meError);
                bPayInAdvance = aPayType.maValue;
            }
        }

        double fGuess = 0.1;
        if (rNode.maChildren.size() == 6)
        {
            if (rNode.maChildren[5]->meKind == formula::NodeKind::EmptyArgument)
                fGuess = 0.1;
            else
            {
                const auto aGuessValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[5]);
                if (!aGuessValue)
                    return makeFailure(aGuessValue.meError);
                if (aGuessValue.maValue.isEmpty())
                    return makeFailure(api::Error::IllegalArgument);

                const auto aGuess = coerceToNumber(aGuessValue.maValue);
                if (!aGuess)
                    return makeFailure(aGuess.meError);
                fGuess = aGuess.maValue;
            }
        }

        const auto aRateResult = sefinance::evaluateRate(
            aNper.maValue, aPayment.maValue, aPresentValue.maValue, fFutureValue,
            bPayInAdvance, fGuess);
        if (!aRateResult)
            return makeFailure(aRateResult.meError);
        return makeScalarResult(api::CellValue::number(aRateResult.maValue));
    }

    if (aFunctionName == u"ISPMT")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPeriod = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);
        const auto aTotalPeriods = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aInvestment = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aInvestment)
            return makeFailure(aInvestment.meError);

        const auto aInterest = sefinance::evaluateInterestSchedulePayment(
            aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue, aInvestment.maValue);
        if (!aInterest)
            return makeFailure(aInterest.meError);
        return makeScalarResult(api::CellValue::number(aInterest.maValue));
    }

    if (aFunctionName == u"IPMT" || aFunctionName == u"PPMT")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPeriod = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);
        const auto aTotalPeriods = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aPresentValue = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 5)
        {
            const auto aFutureValue = aContext.evaluateNumericArgument(*rNode.maChildren[4], 0.0);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 6)
        {
            const auto aPayType = aContext.evaluatePayTypeArgument(*rNode.maChildren[5], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        if (aFunctionName == u"IPMT")
        {
            const auto aInterest = sefinance::evaluateInterestPayment(
                aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue,
                aPresentValue.maValue, fFutureValue, bPayInAdvance);
            if (!aInterest)
                return makeFailure(aInterest.meError);
            return makeScalarResult(api::CellValue::number(aInterest.maValue));
        }

        const auto aPrincipal = sefinance::evaluatePrincipalPayment(
            aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
            fFutureValue, bPayInAdvance);
        if (!aPrincipal)
            return makeFailure(aPrincipal.meError);
        return makeScalarResult(api::CellValue::number(aPrincipal.maValue));
    }

    if (aFunctionName == u"CUMIPMT" || aFunctionName == u"CUMPRINC")
    {
        if (rNode.maChildren.size() != 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aTotalPeriods = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aPresentValue = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);
        const auto aStart = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aEnd = aContext.evaluateNumericArgument(*rNode.maChildren[4], 0.0);
        if (!aEnd)
            return makeFailure(aEnd.meError);
        const auto aPayType = aContext.evaluateStrictPaymentTypeArgument(*rNode.maChildren[5]);
        if (!aPayType)
            return makeFailure(aPayType.meError);

        if (aFunctionName == u"CUMIPMT")
        {
            const auto aInterest = sefinance::evaluateCumulativeInterest(
                aRate.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
                aStart.maValue, aEnd.maValue, aPayType.maValue);
            if (!aInterest)
                return makeFailure(aInterest.meError);
            return makeScalarResult(api::CellValue::number(aInterest.maValue));
        }

        const auto aPrincipal = sefinance::evaluateCumulativePrincipal(
            aRate.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
            aStart.maValue, aEnd.maValue, aPayType.maValue);
        if (!aPrincipal)
            return makeFailure(aPrincipal.meError);
        return makeScalarResult(api::CellValue::number(aPrincipal.maValue));
    }

    if (aFunctionName == u"DDB")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCost = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aCost)
            return makeFailure(aCost.meError);
        const auto aSalvage = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aSalvage)
            return makeFailure(aSalvage.meError);
        const auto aLife = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aLife)
            return makeFailure(aLife.meError);
        const auto aPeriod = aContext.evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);

        double fFactor = 2.0;
        if (rNode.maChildren.size() == 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);
            const auto aFactor = aContext.evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aFactor)
                return makeFailure(aFactor.meError);
            fFactor = aFactor.maValue;
        }

        const auto aDepreciation = sefinance::evaluateDoubleDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aPeriod.maValue, fFactor);
        if (!aDepreciation)
            return makeFailure(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"VDB")
    {
        if (rNode.maChildren.size() < 5 || rNode.maChildren.size() > 7)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCost = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aCost)
            return makeFailure(aCost.meError);
        const auto aSalvage = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aSalvage)
            return makeFailure(aSalvage.meError);
        const auto aLife = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aLife)
            return makeFailure(aLife.meError);
        const auto aStart = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aStart)
            return makeFailure(aStart.meError);
        if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
            return makeFailure(api::Error::IllegalArgument);
        const auto aEnd = aContext.evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
        if (!aEnd)
            return makeFailure(aEnd.meError);

        double fFactor = 2.0;
        if (rNode.maChildren.size() >= 6)
        {
            if (rNode.maChildren[5]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);
            const auto aFactor = aContext.evaluateNumericArgument(*rNode.maChildren[5], std::nullopt);
            if (!aFactor)
                return makeFailure(aFactor.meError);
            fFactor = aFactor.maValue;
        }

        bool bNoSwitch = false;
        if (rNode.maChildren.size() == 7)
        {
            const auto aNoSwitch = aContext.evaluatePayTypeArgument(*rNode.maChildren[6], false);
            if (!aNoSwitch)
                return makeFailure(aNoSwitch.meError);
            bNoSwitch = aNoSwitch.maValue;
        }

        const auto aDepreciation = sefinance::evaluateVariableDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aStart.maValue,
            aEnd.maValue, fFactor, bNoSwitch);
        if (!aDepreciation)
            return makeFailure(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"YEARFRAC" || aFunctionName == u"GETYEARFRAC")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aStartDate = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aStartDate)
            return makeFailure(aStartDate.meError);
        const auto aEndDate = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aEndDate)
            return makeFailure(aEndDate.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 3)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[2], 0);
            if (!aBasis)
                return makeFailure(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aYearFraction = sefinance::evaluateYearFraction(
            sedatetime::defaultNullDate(), aStartDate.maValue, aEndDate.maValue, nBasis);
        if (!aYearFraction)
            return makeFailure(aYearFraction.meError);
        return makeScalarResult(api::CellValue::number(aYearFraction.maValue));
    }

    if (aFunctionName == u"PRICE")
    {
        if (rNode.maChildren.size() < 6 || rNode.maChildren.size() > 7)
            return makeFailure(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeFailure(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeFailure(aMaturity.meError);
        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aYield = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aYield)
            return makeFailure(aYield.meError);
        const auto aRedemption = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[4]);
        if (!aRedemption)
            return makeFailure(aRedemption.meError);
        const auto aFrequency = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[5]);
        if (!aFrequency)
            return makeFailure(aFrequency.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 7)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[6], 0);
            if (!aBasis)
                return makeFailure(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aPrice = sefinance::evaluatePrice(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aRate.maValue, aYield.maValue, aRedemption.maValue, aFrequency.maValue, nBasis);
        if (!aPrice)
            return makeFailure(aPrice.meError);
        return makeScalarResult(api::CellValue::number(aPrice.maValue));
    }

    if (aFunctionName == u"AMORLINC" || aFunctionName == u"GETAMORLINC")
    {
        if (rNode.maChildren.size() < 6 || rNode.maChildren.size() > 7)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aCost = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aCost)
            return makeScalarResult(api::CellValue::error(aCost.meError));
        const auto aPurchaseDate = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aPurchaseDate)
            return makeScalarResult(api::CellValue::error(aPurchaseDate.meError));
        const auto aFirstPeriodEndDate = aContext.evaluateRequiredDateArgument(*rNode.maChildren[2]);
        if (!aFirstPeriodEndDate)
            return makeScalarResult(api::CellValue::error(aFirstPeriodEndDate.meError));
        const auto aSalvage = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aSalvage)
            return makeScalarResult(api::CellValue::error(aSalvage.meError));
        const auto aPeriod = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[4]);
        if (!aPeriod)
            return makeScalarResult(api::CellValue::error(aPeriod.meError));
        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[5]);
        if (!aRate)
            return makeScalarResult(api::CellValue::error(aRate.meError));

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 7)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[6], 0);
            if (!aBasis)
                return makeScalarResult(api::CellValue::error(aBasis.meError));
            nBasis = aBasis.maValue;
        }

        const auto aDepreciation = sefinance::evaluateAmorlinc(
            sedatetime::defaultNullDate(), aCost.maValue, aPurchaseDate.maValue,
            aFirstPeriodEndDate.maValue, aSalvage.maValue, aPeriod.maValue, aRate.maValue,
            nBasis);
        if (!aDepreciation)
            return makeScalarResult(api::CellValue::error(aDepreciation.meError));
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"ODDLYIELD" || aFunctionName == u"GETODDLYIELD")
    {
        if (rNode.maChildren.size() < 7 || rNode.maChildren.size() > 8)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeScalarResult(api::CellValue::error(aSettlement.meError));
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeScalarResult(api::CellValue::error(aMaturity.meError));
        const auto aLastInterest = aContext.evaluateRequiredDateArgument(*rNode.maChildren[2]);
        if (!aLastInterest)
            return makeScalarResult(api::CellValue::error(aLastInterest.meError));
        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aRate)
            return makeScalarResult(api::CellValue::error(aRate.meError));
        const auto aPrice = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[4]);
        if (!aPrice)
            return makeScalarResult(api::CellValue::error(aPrice.meError));
        const auto aRedemption = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[5]);
        if (!aRedemption)
            return makeScalarResult(api::CellValue::error(aRedemption.meError));
        const auto aFrequency = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[6]);
        if (!aFrequency)
            return makeScalarResult(api::CellValue::error(aFrequency.meError));

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 8)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[7], 0);
            if (!aBasis)
                return makeScalarResult(api::CellValue::error(aBasis.meError));
            nBasis = aBasis.maValue;
        }

        const auto aYield = sefinance::evaluateOddlyield(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aLastInterest.maValue, aRate.maValue, aPrice.maValue, aRedemption.maValue,
            aFrequency.maValue, nBasis);
        if (!aYield)
            return makeScalarResult(api::CellValue::error(aYield.meError));
        return makeScalarResult(api::CellValue::number(aYield.maValue));
    }

        return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
