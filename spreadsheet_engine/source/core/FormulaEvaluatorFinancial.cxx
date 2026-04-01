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
        api::StringView(u"NOMINAL"),
        api::StringView(u"NOMINAL_ADD"),
        api::StringView(u"GETNOMINAL"),
        api::StringView(u"DOLLARFR"),
        api::StringView(u"GETDOLLARFR"),
        api::StringView(u"DOLLARDE"),
        api::StringView(u"GETDOLLARDE"),
        api::StringView(u"EFFECT"),
        api::StringView(u"NPV"),
        api::StringView(u"XNPV"),
        api::StringView(u"GETXNPV"),
        api::StringView(u"RRI"),
        api::StringView(u"ISPMT"),
        api::StringView(u"IPMT"),
        api::StringView(u"PPMT"),
        api::StringView(u"CUMIPMT"),
        api::StringView(u"CUMPRINC"),
        api::StringView(u"DDB"),
        api::StringView(u"DB"),
        api::StringView(u"VDB"),
        api::StringView(u"SLN"),
        api::StringView(u"SYD"),
        api::StringView(u"IRR"),
        api::StringView(u"MIRR"),
        api::StringView(u"YEARFRAC"),
        api::StringView(u"GETYEARFRAC"),
        api::StringView(u"PRICE"),
        api::StringView(u"GETPRICE"),
        api::StringView(u"PRICEMAT"),
        api::StringView(u"GETPRICEMAT"),
        api::StringView(u"ACCRINTM"),
        api::StringView(u"GETACCRINTM"),
        api::StringView(u"RECEIVED"),
        api::StringView(u"GETRECEIVED"),
        api::StringView(u"DISC"),
        api::StringView(u"GETDISC"),
        api::StringView(u"PRICEDISC"),
        api::StringView(u"GETPRICEDISC"),
        api::StringView(u"INTRATE"),
        api::StringView(u"GETINTRATE"),
        api::StringView(u"YIELDDISC"),
        api::StringView(u"GETYIELDDISC"),
        api::StringView(u"MDURATION"),
        api::StringView(u"GETMDURATION"),
        api::StringView(u"YIELD"),
        api::StringView(u"GETYIELD"),
        api::StringView(u"TBILLPRICE"),
        api::StringView(u"GETTBILLPRICE"),
        api::StringView(u"TBILLEQ"),
        api::StringView(u"GETTBILLEQ"),
        api::StringView(u"TBILLYIELD"),
        api::StringView(u"GETTBILLYIELD"),
        api::StringView(u"FVSCHEDULE"),
        api::StringView(u"GETFVSCHEDULE"),
        api::StringView(u"AMORLINC"),
        api::StringView(u"GETAMORLINC"),
        api::StringView(u"AMORDEGRC"),
        api::StringView(u"GETAMORDEGRC"),
        api::StringView(u"ODDLPRICE"),
        api::StringView(u"GETODDLPRICE"),
        api::StringView(u"ODDLYIELD"),
        api::StringView(u"GETODDLYIELD"),
        api::StringView(u"COUPNCD"),
        api::StringView(u"GETCOUPNCD"),
        api::StringView(u"COUPDAYS"),
        api::StringView(u"GETCOUPDAYS"),
        api::StringView(u"COUPDAYSNC"),
        api::StringView(u"GETCOUPDAYSNC"),
        api::StringView(u"COUPDAYBS"),
        api::StringView(u"GETCOUPDAYBS"),
        api::StringView(u"COUPPCD"),
        api::StringView(u"GETCOUPPCD"),
        api::StringView(u"COUPNUM"),
        api::StringView(u"GETCOUPNUM"),
        api::StringView(u"PDURATION"),
        api::StringView(u"XIRR"),
        api::StringView(u"GETXIRR"),
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
    const auto makeCellError = [&](api::Error eError) -> EvaluationResult {
        return makeScalarResult(api::CellValue::error(eError));
    };
    const auto replayStoredOrCellError = [&](api::Error eError) -> EvaluationResult {
        if (canUseStoredReplayValue())
        {
            if (const auto oStoredValue = tryGetStoredCellValue(rCurrentAddress))
                return makeScalarResult(*oStoredValue);
        }
        return makeCellError(eError);
    };
    const auto collectNumericSeries = [&](const formula::Node& rArgument)
        -> api::ValueResult<std::vector<double>> {
        std::vector<double> aValues;
        const auto aVisited = aContext.visitFlattenedValues(
            rArgument,
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isNumber())
                    aValues.push_back(rValue.mfNumber);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        return api::ValueResult<std::vector<double>>::success(std::move(aValues));
    };
    const auto collectDateSeries = [&](const formula::Node& rArgument)
        -> api::ValueResult<std::vector<api::DateSerial>> {
        std::vector<api::DateSerial> aDates;
        const auto aVisited = aContext.visitFlattenedValues(
            rArgument,
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isEmpty())
                    return api::ValueResult<bool>::success(true);
                const auto oDateSerial = sedatetime::coerceToDateSerial(rValue);
                if (!oDateSerial)
                    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
                aDates.push_back(*oDateSerial);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<std::vector<api::DateSerial>>::failure(aVisited.meError);
        return api::ValueResult<std::vector<api::DateSerial>>::success(std::move(aDates));
    };

    if (aFunctionName == u"XNPV" || aFunctionName == u"GETXNPV")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aValues = collectNumericSeries(*rNode.maChildren[1]);
        if (!aValues)
            return makeFailure(aValues.meError);
        const auto aDates = collectDateSeries(*rNode.maChildren[2]);
        if (!aDates)
            return makeFailure(aDates.meError);

        const auto aXnpv
            = sefinance::evaluateXnpvNumbers(aRate.maValue, aValues.maValue, aDates.maValue);
        if (!aXnpv)
            return makeFailure(aXnpv.meError);
        return makeScalarResult(api::CellValue::number(aXnpv.maValue));
    }

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
            return makeCellError(api::Error::IllegalArgument);

        const auto aNper = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aNper)
            return makeCellError(aNper.meError);
        const auto aPayment = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPayment)
            return makeCellError(aPayment.meError);
        const auto aPresentValue = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeCellError(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFutureValue = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aFutureValue)
                return makeCellError(aFutureValue.meError);
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
                    return makeCellError(aPayTypeValue.meError);
                if (aPayTypeValue.maValue.isEmpty())
                    return makeCellError(api::Error::IllegalArgument);

                const auto aPayType = coerceToBoolean(aPayTypeValue.maValue);
                if (!aPayType)
                    return makeCellError(aPayType.meError);
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
                    return makeCellError(aGuessValue.meError);
                if (aGuessValue.maValue.isEmpty())
                    return makeCellError(api::Error::IllegalArgument);

                const auto aGuess = coerceToNumber(aGuessValue.maValue);
                if (!aGuess)
                    return makeCellError(aGuess.meError);
                fGuess = aGuess.maValue;
            }
        }

        const auto aRateResult = sefinance::evaluateRate(
            aNper.maValue, aPayment.maValue, aPresentValue.maValue, fFutureValue,
            bPayInAdvance, fGuess);
        if (!aRateResult)
            return makeCellError(aRateResult.meError);
        return makeScalarResult(api::CellValue::number(aRateResult.maValue));
    }

    if (aFunctionName == u"NOMINAL" || aFunctionName == u"NOMINAL_ADD"
        || aFunctionName == u"GETNOMINAL")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aRate)
            return makeCellError(aRate.meError);
        const auto aPeriods = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[1]);
        if (!aPeriods)
            return makeCellError(aPeriods.meError);

        const auto aNominal = sefinance::evaluateNominal(aRate.maValue, aPeriods.maValue);
        if (!aNominal)
            return makeCellError(aNominal.meError);
        return makeScalarResult(api::CellValue::number(aNominal.maValue));
    }

    if (aFunctionName == u"EFFECT")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aNominalRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aNominalRate)
            return makeCellError(aNominalRate.meError);
        const auto aPeriods = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aPeriods)
            return makeCellError(aPeriods.meError);

        const auto aEffect
            = sefinance::evaluateEffectiveAnnualRate(aNominalRate.maValue, aPeriods.maValue);
        if (!aEffect)
            return makeCellError(aEffect.meError);
        return makeScalarResult(api::CellValue::number(aEffect.maValue));
    }

    if (aFunctionName == u"DOLLARFR" || aFunctionName == u"GETDOLLARFR")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aDollarDecimal = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aDollarDecimal)
            return makeCellError(aDollarDecimal.meError);
        const auto aFractionDenominator
            = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aFractionDenominator)
            return makeCellError(aFractionDenominator.meError);

        const auto aDollarFraction = sefinance::evaluateDollarFraction(
            aDollarDecimal.maValue, aFractionDenominator.maValue);
        if (!aDollarFraction)
            return makeCellError(aDollarFraction.meError);
        return makeScalarResult(api::CellValue::number(aDollarFraction.maValue));
    }

    if (aFunctionName == u"DOLLARDE" || aFunctionName == u"GETDOLLARDE")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aDollarFraction = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aDollarFraction)
            return makeCellError(aDollarFraction.meError);
        const auto aFractionDenominator
            = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aFractionDenominator)
            return makeCellError(aFractionDenominator.meError);

        const auto aDollarDecimal = sefinance::evaluateDollarDecimal(
            aDollarFraction.maValue, aFractionDenominator.maValue);
        if (!aDollarDecimal)
            return makeCellError(aDollarDecimal.meError);
        return makeScalarResult(api::CellValue::number(aDollarDecimal.maValue));
    }

    if (aFunctionName == u"NPV")
    {
        if (rNode.maChildren.size() < 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aRate)
            return makeFailure(aRate.meError);

        std::vector<double> aValues;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aSeries = collectNumericSeries(*rNode.maChildren[nIndex]);
            if (!aSeries)
                return makeFailure(aSeries.meError);
            aValues.insert(aValues.end(), aSeries.maValue.begin(), aSeries.maValue.end());
        }

        const auto aNpv = sefinance::evaluateNetPresentValueNumbers(aRate.maValue, aValues);
        if (!aNpv)
            return makeFailure(aNpv.meError);
        return makeScalarResult(api::CellValue::number(aNpv.maValue));
    }

    if (aFunctionName == u"RRI")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aPeriods = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aPeriods)
            return makeCellError(aPeriods.meError);
        const auto aPresentValue = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPresentValue)
            return makeCellError(aPresentValue.meError);
        const auto aFutureValue = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aFutureValue)
            return makeCellError(aFutureValue.meError);

        const auto aRri = sefinance::evaluateGrowthRateOverPeriods(
            aPeriods.maValue, aPresentValue.maValue, aFutureValue.maValue);
        if (!aRri)
            return makeCellError(aRri.meError);
        return makeScalarResult(api::CellValue::number(aRri.maValue));
    }

    if (aFunctionName == u"ISPMT")
    {
        if (rNode.maChildren.size() != 4)
            return makeCellError(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeCellError(aRate.meError);
        const auto aPeriod = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPeriod)
            return makeCellError(aPeriod.meError);
        const auto aTotalPeriods = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aTotalPeriods)
            return makeCellError(aTotalPeriods.meError);
        const auto aInvestment = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aInvestment)
            return makeCellError(aInvestment.meError);

        const auto aInterest = sefinance::evaluateInterestSchedulePayment(
            aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue, aInvestment.maValue);
        if (!aInterest)
            return makeCellError(aInterest.meError);
        return makeScalarResult(api::CellValue::number(aInterest.maValue));
    }

    if (aFunctionName == u"IPMT" || aFunctionName == u"PPMT")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6)
            return makeCellError(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeCellError(aRate.meError);
        const auto aPeriod = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPeriod)
            return makeCellError(aPeriod.meError);
        const auto aTotalPeriods = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aTotalPeriods)
            return makeCellError(aTotalPeriods.meError);
        const auto aPresentValue = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aPresentValue)
            return makeCellError(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 5)
        {
            const auto aFutureValue = aContext.evaluateNumericArgument(*rNode.maChildren[4], 0.0);
            if (!aFutureValue)
                return makeCellError(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 6)
        {
            const auto aPayType = aContext.evaluatePayTypeArgument(*rNode.maChildren[5], false);
            if (!aPayType)
                return makeCellError(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        if (aFunctionName == u"IPMT")
        {
            const auto aInterest = sefinance::evaluateInterestPayment(
                aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue,
                aPresentValue.maValue, fFutureValue, bPayInAdvance);
            if (!aInterest)
                return makeCellError(aInterest.meError);
            return makeScalarResult(api::CellValue::number(aInterest.maValue));
        }

        const auto aPrincipal = sefinance::evaluatePrincipalPayment(
            aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
            fFutureValue, bPayInAdvance);
        if (!aPrincipal)
            return makeCellError(aPrincipal.meError);
        return makeScalarResult(api::CellValue::number(aPrincipal.maValue));
    }

    if (aFunctionName == u"CUMIPMT" || aFunctionName == u"CUMPRINC")
    {
        if (rNode.maChildren.size() != 6)
            return makeCellError(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeCellError(aRate.meError);
        const auto aTotalPeriods = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aTotalPeriods)
            return makeCellError(aTotalPeriods.meError);
        const auto aPresentValue = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeCellError(aPresentValue.meError);
        const auto aStart = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aStart)
            return makeCellError(aStart.meError);
        const auto aEnd = aContext.evaluateNumericArgument(*rNode.maChildren[4], 0.0);
        if (!aEnd)
            return makeCellError(aEnd.meError);
        if (rNode.maChildren[5]->meKind == formula::NodeKind::EmptyArgument)
            return makeFailure(api::Error::IllegalArgument);
        const auto aPayType = aContext.evaluateStrictPaymentTypeArgument(*rNode.maChildren[5]);
        if (!aPayType)
            return makeCellError(aPayType.meError);

        if (aFunctionName == u"CUMIPMT")
        {
            const auto aInterest = sefinance::evaluateCumulativeInterest(
                aRate.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
                aStart.maValue, aEnd.maValue, aPayType.maValue);
            if (!aInterest)
                return makeCellError(aInterest.meError);
            return makeScalarResult(api::CellValue::number(aInterest.maValue));
        }

        const auto aPrincipal = sefinance::evaluateCumulativePrincipal(
            aRate.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
            aStart.maValue, aEnd.maValue, aPayType.maValue);
        if (!aPrincipal)
            return makeCellError(aPrincipal.meError);
        return makeScalarResult(api::CellValue::number(aPrincipal.maValue));
    }

    if (aFunctionName == u"DDB")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeCellError(api::Error::IllegalArgument);

        const auto aCost = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aCost)
            return makeCellError(aCost.meError);
        const auto aSalvage = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aSalvage)
            return makeCellError(aSalvage.meError);
        const auto aLife = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aLife)
            return makeCellError(aLife.meError);
        const auto aPeriod = aContext.evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
        if (!aPeriod)
            return makeCellError(aPeriod.meError);

        double fFactor = 2.0;
        if (rNode.maChildren.size() == 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                return makeCellError(api::Error::IllegalArgument);
            const auto aFactor = aContext.evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aFactor)
                return makeCellError(aFactor.meError);
            fFactor = aFactor.maValue;
        }

        const auto aDepreciation = sefinance::evaluateDoubleDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aPeriod.maValue, fFactor);
        if (!aDepreciation)
            return makeCellError(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"DB")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeCellError(api::Error::IllegalArgument);

        const auto aCost = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aCost)
            return makeCellError(aCost.meError);
        const auto aSalvage = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aSalvage)
            return makeCellError(aSalvage.meError);
        const auto aLife = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aLife)
            return makeCellError(aLife.meError);
        const auto aPeriod = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aPeriod)
            return makeCellError(aPeriod.meError);

        double fMonths = 12.0;
        if (rNode.maChildren.size() == 5)
        {
            const auto aMonths = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[4]);
            if (!aMonths)
                return makeCellError(aMonths.meError);
            fMonths = fp::approxFloor(aMonths.maValue);
        }

        const auto aDepreciation = sefinance::evaluateFixedDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aPeriod.maValue, fMonths);
        if (!aDepreciation)
            return makeCellError(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"VDB")
    {
        if (rNode.maChildren.size() < 5 || rNode.maChildren.size() > 7)
            return makeCellError(api::Error::IllegalArgument);

        const auto aCost = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aCost)
            return makeCellError(aCost.meError);
        const auto aSalvage = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aSalvage)
            return makeCellError(aSalvage.meError);
        const auto aLife = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aLife)
            return makeCellError(aLife.meError);
        const auto aStart = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aStart)
            return makeCellError(aStart.meError);
        if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
            return makeFailure(api::Error::IllegalArgument);
        const auto aEnd = aContext.evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
        if (!aEnd)
            return makeCellError(aEnd.meError);

        double fFactor = 2.0;
        if (rNode.maChildren.size() >= 6)
        {
            if (rNode.maChildren[5]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);
            const auto aFactor = aContext.evaluateNumericArgument(*rNode.maChildren[5], std::nullopt);
            if (!aFactor)
                return makeCellError(aFactor.meError);
            fFactor = aFactor.maValue;
        }

        bool bNoSwitch = false;
        if (rNode.maChildren.size() == 7)
        {
            const auto aNoSwitch = aContext.evaluatePayTypeArgument(*rNode.maChildren[6], false);
            if (!aNoSwitch)
                return makeCellError(aNoSwitch.meError);
            bNoSwitch = aNoSwitch.maValue;
        }

        const auto aDepreciation = sefinance::evaluateVariableDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aStart.maValue,
            aEnd.maValue, fFactor, bNoSwitch);
        if (!aDepreciation)
            return makeCellError(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"SLN")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCost = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aCost)
            return makeFailure(aCost.meError);
        const auto aSalvage = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aSalvage)
            return makeFailure(aSalvage.meError);
        const auto aLife = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aLife)
            return makeFailure(aLife.meError);

        const auto aDepreciation = sefinance::evaluateStraightLineDepreciation(
            aCost.maValue, aSalvage.maValue, aLife.maValue);
        if (!aDepreciation)
            return makeFailure(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"SYD")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCost = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aCost)
            return makeFailure(aCost.meError);
        const auto aSalvage = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aSalvage)
            return makeFailure(aSalvage.meError);
        const auto aLife = aContext.evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aLife)
            return makeFailure(aLife.meError);
        const auto aPeriod = aContext.evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);

        const auto aDepreciation = sefinance::evaluateSumOfYearsDepreciation(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aPeriod.maValue);
        if (!aDepreciation)
            return makeFailure(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"IRR")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValues = collectNumericSeries(*rNode.maChildren[0]);
        if (!aValues)
            return makeFailure(aValues.meError);

        double fGuess = 0.1;
        if (rNode.maChildren.size() == 2)
        {
            const auto aGuess = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aGuess)
                return makeFailure(aGuess.meError);
            fGuess = aGuess.maValue;
        }

        const auto aIrr = sefinance::evaluateIrrNumbers(aValues.maValue, fGuess);
        if (!aIrr)
            return makeFailure(aIrr.meError);
        return makeScalarResult(api::CellValue::number(aIrr.maValue));
    }

    if (aFunctionName == u"MIRR")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aValues = collectNumericSeries(*rNode.maChildren[0]);
        if (!aValues)
            return makeCellError(aValues.meError);
        const auto aFinanceRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aFinanceRate)
            return makeCellError(aFinanceRate.meError);
        const auto aReinvestRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aReinvestRate)
            return makeCellError(aReinvestRate.meError);

        const auto aMirr = sefinance::evaluateMirrNumbers(
            aValues.maValue, aFinanceRate.maValue, aReinvestRate.maValue);
        if (!aMirr)
            return makeCellError(aMirr.meError);
        return makeScalarResult(api::CellValue::number(aMirr.maValue));
    }

    if (aFunctionName == u"YEARFRAC" || aFunctionName == u"GETYEARFRAC")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aStartDate = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aStartDate)
            return makeCellError(aStartDate.meError);
        const auto aEndDate = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aEndDate)
            return makeCellError(aEndDate.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 3)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[2], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aYearFraction = sefinance::evaluateYearFraction(
            sedatetime::defaultNullDate(), aStartDate.maValue, aEndDate.maValue, nBasis);
        if (!aYearFraction)
            return makeCellError(aYearFraction.meError);
        return makeScalarResult(api::CellValue::number(aYearFraction.maValue));
    }

    if (aFunctionName == u"PRICE" || aFunctionName == u"GETPRICE")
    {
        if (rNode.maChildren.size() < 6 || rNode.maChildren.size() > 7)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aRate)
            return makeCellError(aRate.meError);
        const auto aYield = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aYield)
            return makeCellError(aYield.meError);
        const auto aRedemption = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[4]);
        if (!aRedemption)
            return makeCellError(aRedemption.meError);
        const auto aFrequency = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[5]);
        if (!aFrequency)
            return makeCellError(aFrequency.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 7)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[6], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aPrice = sefinance::evaluatePrice(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aRate.maValue, aYield.maValue, aRedemption.maValue, aFrequency.maValue, nBasis);
        if (!aPrice)
            return makeCellError(aPrice.meError);
        return makeScalarResult(api::CellValue::number(aPrice.maValue));
    }

    if (aFunctionName == u"PRICEMAT" || aFunctionName == u"GETPRICEMAT")
    {
        if (rNode.maChildren.size() < 5 || rNode.maChildren.size() > 6)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aIssue = aContext.evaluateRequiredDateArgument(*rNode.maChildren[2]);
        if (!aIssue)
            return makeCellError(aIssue.meError);
        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aRate)
            return makeCellError(aRate.meError);
        const auto aYield = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[4]);
        if (!aYield)
            return makeCellError(aYield.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 6)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[5], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aPricemat = sefinance::evaluatePricemat(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aIssue.maValue, aRate.maValue, aYield.maValue, nBasis);
        if (!aPricemat)
            return makeCellError(aPricemat.meError);
        return makeScalarResult(api::CellValue::number(aPricemat.maValue));
    }

    if (aFunctionName == u"ACCRINTM" || aFunctionName == u"GETACCRINTM")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aIssue = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aIssue)
            return makeFailure(aIssue.meError);
        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aSettlement)
            return makeFailure(aSettlement.meError);
        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aRate)
            return makeFailure(aRate.meError);

        double fParValue = 1000.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aParValue = aContext.evaluateNumericArgument(*rNode.maChildren[3], 1000.0);
            if (!aParValue)
                return makeFailure(aParValue.meError);
            fParValue = aParValue.maValue;
        }

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 5)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[4], 0);
            if (!aBasis)
                return makeFailure(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aAccrintm = sefinance::evaluateAccrintm(
            sedatetime::defaultNullDate(), aIssue.maValue, aSettlement.maValue,
            aRate.maValue, fParValue, nBasis);
        if (!aAccrintm)
            return makeFailure(aAccrintm.meError);
        return makeScalarResult(api::CellValue::number(aAccrintm.maValue));
    }

    if (aFunctionName == u"RECEIVED" || aFunctionName == u"GETRECEIVED")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aInvestment = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aInvestment)
            return makeCellError(aInvestment.meError);
        const auto aDiscount = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aDiscount)
            return makeCellError(aDiscount.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 5)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[4], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aReceived = sefinance::evaluateReceived(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aInvestment.maValue, aDiscount.maValue, nBasis);
        if (!aReceived)
            return makeCellError(aReceived.meError);
        return makeScalarResult(api::CellValue::number(aReceived.maValue));
    }

    if (aFunctionName == u"DISC" || aFunctionName == u"GETDISC")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aPrice = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aPrice)
            return makeCellError(aPrice.meError);
        const auto aRedemption = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aRedemption)
            return makeCellError(aRedemption.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 5)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[4], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aDisc = sefinance::evaluateDisc(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aPrice.maValue, aRedemption.maValue, nBasis);
        if (!aDisc)
            return makeCellError(aDisc.meError);
        return makeScalarResult(api::CellValue::number(aDisc.maValue));
    }

    if (aFunctionName == u"PRICEDISC" || aFunctionName == u"GETPRICEDISC")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aDiscount = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aDiscount)
            return makeCellError(aDiscount.meError);
        const auto aRedemption = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aRedemption)
            return makeCellError(aRedemption.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 5)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[4], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aPricedisc = sefinance::evaluatePricedisc(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aDiscount.maValue, aRedemption.maValue, nBasis);
        if (!aPricedisc)
            return makeCellError(aPricedisc.meError);
        return makeScalarResult(api::CellValue::number(aPricedisc.maValue));
    }

    if (aFunctionName == u"INTRATE" || aFunctionName == u"GETINTRATE")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aInvestment = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aInvestment)
            return makeCellError(aInvestment.meError);
        const auto aRedemption = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aRedemption)
            return makeCellError(aRedemption.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 5)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[4], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aIntrate = sefinance::evaluateIntrate(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aInvestment.maValue, aRedemption.maValue, nBasis);
        if (!aIntrate)
            return makeCellError(aIntrate.meError);
        return makeScalarResult(api::CellValue::number(aIntrate.maValue));
    }

    if (aFunctionName == u"YIELDDISC" || aFunctionName == u"GETYIELDDISC")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aPrice = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aPrice)
            return makeCellError(aPrice.meError);
        const auto aRedemption = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aRedemption)
            return makeCellError(aRedemption.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 5)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[4], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aYielddisc = sefinance::evaluateYielddisc(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aPrice.maValue, aRedemption.maValue, nBasis);
        if (!aYielddisc)
            return makeCellError(aYielddisc.meError);
        return makeScalarResult(api::CellValue::number(aYielddisc.maValue));
    }

    if (aFunctionName == u"MDURATION" || aFunctionName == u"GETMDURATION")
    {
        if (rNode.maChildren.size() < 5 || rNode.maChildren.size() > 6)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aCoupon = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aCoupon)
            return makeCellError(aCoupon.meError);
        const auto aYield = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aYield)
            return makeCellError(aYield.meError);
        const auto aFrequency = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[4]);
        if (!aFrequency)
            return makeCellError(aFrequency.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 6)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[5], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aDuration = sefinance::evaluateModifiedDuration(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aCoupon.maValue, aYield.maValue, aFrequency.maValue, nBasis);
        if (!aDuration)
            return makeCellError(aDuration.meError);
        return makeScalarResult(api::CellValue::number(aDuration.maValue));
    }

    if (aFunctionName == u"YIELD" || aFunctionName == u"GETYIELD")
    {
        if (rNode.maChildren.size() < 6 || rNode.maChildren.size() > 7)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aCoupon = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aCoupon)
            return makeCellError(aCoupon.meError);
        const auto aPrice = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aPrice)
            return makeCellError(aPrice.meError);
        const auto aRedemption = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[4]);
        if (!aRedemption)
            return makeCellError(aRedemption.meError);
        const auto aFrequency = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[5]);
        if (!aFrequency)
            return makeCellError(aFrequency.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 7)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[6], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aYield = sefinance::evaluateYield(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aCoupon.maValue, aPrice.maValue, aRedemption.maValue, aFrequency.maValue, nBasis);
        if (!aYield)
            return makeCellError(aYield.meError);
        return makeScalarResult(api::CellValue::number(aYield.maValue));
    }

    if (aFunctionName == u"TBILLPRICE" || aFunctionName == u"GETTBILLPRICE")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aDiscount = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aDiscount)
            return makeCellError(aDiscount.meError);

        const auto aPrice = sefinance::evaluateTbillPrice(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aDiscount.maValue);
        if (!aPrice)
            return makeCellError(aPrice.meError);
        return makeScalarResult(api::CellValue::number(aPrice.maValue));
    }

    if (aFunctionName == u"TBILLEQ" || aFunctionName == u"GETTBILLEQ")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aDiscount = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aDiscount)
            return makeCellError(aDiscount.meError);

        const auto aTbillEq = sefinance::evaluateTbillEq(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aDiscount.maValue);
        if (!aTbillEq)
            return makeCellError(aTbillEq.meError);
        return makeScalarResult(api::CellValue::number(aTbillEq.maValue));
    }

    if (aFunctionName == u"TBILLYIELD" || aFunctionName == u"GETTBILLYIELD")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aPrice = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aPrice)
            return makeCellError(aPrice.meError);

        const auto aYield = sefinance::evaluateTbillYield(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aPrice.maValue);
        if (!aYield)
            return makeCellError(aYield.meError);
        return makeScalarResult(api::CellValue::number(aYield.maValue));
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

    if (aFunctionName == u"AMORDEGRC" || aFunctionName == u"GETAMORDEGRC")
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

        const auto aDepreciation = sefinance::evaluateAmordegrc(
            sedatetime::defaultNullDate(), aCost.maValue, aPurchaseDate.maValue,
            aFirstPeriodEndDate.maValue, aSalvage.maValue, aPeriod.maValue, aRate.maValue,
            nBasis);
        if (!aDepreciation)
            return makeScalarResult(api::CellValue::error(aDepreciation.meError));
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"ODDLPRICE" || aFunctionName == u"GETODDLPRICE")
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
        const auto aYield = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[4]);
        if (!aYield)
            return makeScalarResult(api::CellValue::error(aYield.meError));
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

        const auto aPrice = sefinance::evaluateOddlprice(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aLastInterest.maValue, aRate.maValue, aYield.maValue, aRedemption.maValue,
            aFrequency.maValue, nBasis);
        if (!aPrice)
            return makeScalarResult(api::CellValue::error(aPrice.meError));
        return makeScalarResult(api::CellValue::number(aPrice.maValue));
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

    if (aFunctionName == u"COUPNCD" || aFunctionName == u"GETCOUPNCD"
        || aFunctionName == u"COUPDAYS" || aFunctionName == u"GETCOUPDAYS"
        || aFunctionName == u"COUPDAYSNC" || aFunctionName == u"GETCOUPDAYSNC"
        || aFunctionName == u"COUPDAYBS" || aFunctionName == u"GETCOUPDAYBS"
        || aFunctionName == u"COUPPCD" || aFunctionName == u"GETCOUPPCD"
        || aFunctionName == u"COUPNUM" || aFunctionName == u"GETCOUPNUM")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSettlement = aContext.evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeCellError(aSettlement.meError);
        const auto aMaturity = aContext.evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeCellError(aMaturity.meError);
        const auto aFrequency = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[2]);
        if (!aFrequency)
            return makeCellError(aFrequency.meError);

        std::int32_t nBasis = 0;
        if (rNode.maChildren.size() == 4)
        {
            const auto aBasis = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[3], 0);
            if (!aBasis)
                return makeCellError(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        api::ValueResult<double> aResult = api::ValueResult<double>::failure(api::Error::IllegalArgument);
        if (aFunctionName == u"COUPNCD" || aFunctionName == u"GETCOUPNCD")
        {
            aResult = sefinance::evaluateCoupncd(
                sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
                aFrequency.maValue, nBasis);
        }
        else if (aFunctionName == u"COUPDAYS" || aFunctionName == u"GETCOUPDAYS")
        {
            aResult = sefinance::evaluateCoupdays(
                sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
                aFrequency.maValue, nBasis);
        }
        else if (aFunctionName == u"COUPDAYSNC" || aFunctionName == u"GETCOUPDAYSNC")
        {
            aResult = sefinance::evaluateCoupdaysnc(
                sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
                aFrequency.maValue, nBasis);
        }
        else if (aFunctionName == u"COUPDAYBS" || aFunctionName == u"GETCOUPDAYBS")
        {
            aResult = sefinance::evaluateCoupdaybs(
                sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
                aFrequency.maValue, nBasis);
        }
        else if (aFunctionName == u"COUPPCD" || aFunctionName == u"GETCOUPPCD")
        {
            aResult = sefinance::evaluateCouppcd(
                sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
                aFrequency.maValue, nBasis);
        }
        else if (aFunctionName == u"COUPNUM" || aFunctionName == u"GETCOUPNUM")
        {
            aResult = sefinance::evaluateCoupnum(
                sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
                aFrequency.maValue, nBasis);
        }

        if (!aResult)
            return makeCellError(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"FVSCHEDULE" || aFunctionName == u"GETFVSCHEDULE")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aPrincipal = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aPrincipal)
            return makeCellError(aPrincipal.meError);
        if (rNode.maChildren[1]->meKind == formula::NodeKind::EmptyArgument)
            return makeCellError(api::Error::IllegalArgument);
        const auto aSchedule = collectNumericSeries(*rNode.maChildren[1]);
        if (!aSchedule)
            return makeCellError(aSchedule.meError);
        if (aSchedule.maValue.empty())
            return makeCellError(api::Error::IllegalArgument);

        const auto aResult
            = sefinance::evaluateFutureValueSchedule(aPrincipal.maValue, aSchedule.maValue);
        if (!aResult)
            return makeCellError(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"PDURATION")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aRate = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aRate)
            return makeCellError(aRate.meError);
        const auto aPresentValue = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aPresentValue)
            return makeCellError(aPresentValue.meError);
        const auto aFutureValue = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aFutureValue)
            return makeCellError(aFutureValue.meError);

        const auto aResult = sefinance::evaluatePaybackDuration(
            aRate.maValue, aPresentValue.maValue, aFutureValue.maValue);
        if (!aResult)
            return makeCellError(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"XIRR" || aFunctionName == u"GETXIRR")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValues = collectNumericSeries(*rNode.maChildren[0]);
        if (!aValues)
            return makeFailure(aValues.meError);
        const auto aDates = collectDateSeries(*rNode.maChildren[1]);
        if (!aDates)
            return makeFailure(aDates.meError);

        double fGuess = 0.1;
        if (rNode.maChildren.size() == 3)
        {
            const auto aGuess = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
            if (!aGuess)
                return makeFailure(aGuess.meError);
            fGuess = aGuess.maValue;
        }

        const auto aXirr = sefinance::evaluateXirrNumbers(
            aValues.maValue, aDates.maValue, fGuess);
        if (!aXirr)
            return makeFailure(aXirr.meError);
        return makeScalarResult(api::CellValue::number(aXirr.maValue));
    }

        return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
