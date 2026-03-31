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

std::optional<EvaluationResult> Evaluator::tryEvaluateMathFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kMathFunctions{
        api::StringView(u"ABS"),
        api::StringView(u"PI"),
        api::StringView(u"DEGREES"),
        api::StringView(u"RADIANS"),
        api::StringView(u"SIN"),
        api::StringView(u"COS"),
        api::StringView(u"TAN"),
        api::StringView(u"COT"),
        api::StringView(u"ASIN"),
        api::StringView(u"ACOS"),
        api::StringView(u"ATAN"),
        api::StringView(u"ACOT"),
        api::StringView(u"ATAN2"),
        api::StringView(u"SINH"),
        api::StringView(u"COSH"),
        api::StringView(u"TANH"),
        api::StringView(u"COTH"),
        api::StringView(u"ASINH"),
        api::StringView(u"ACOSH"),
        api::StringView(u"ATANH"),
        api::StringView(u"ACOTH"),
        api::StringView(u"GCD"),
        api::StringView(u"LCM"),
        api::StringView(u"SIGN"),
        api::StringView(u"ROUND"),
        api::StringView(u"ROUNDUP"),
        api::StringView(u"ROUNDDOWN"),
        api::StringView(u"CEILING"),
        api::StringView(u"FLOOR"),
        api::StringView(u"CEILING.XCL"),
        api::StringView(u"FLOOR.XCL"),
        api::StringView(u"CEILING.MATH"),
        api::StringView(u"FLOOR.MATH"),
        api::StringView(u"CEILING.PRECISE"),
        api::StringView(u"FLOOR.PRECISE"),
        api::StringView(u"ISO.CEILING"),
        api::StringView(u"ROUNDSIG"),
        api::StringView(u"BITAND"),
        api::StringView(u"BITOR"),
        api::StringView(u"BITXOR"),
        api::StringView(u"BITLSHIFT"),
        api::StringView(u"BITRSHIFT"),
        api::StringView(u"POWER"),
        api::StringView(u"LOG"),
        api::StringView(u"LOG10"),
        api::StringView(u"LN"),
        api::StringView(u"PRODUCT"),
        api::StringView(u"QUOTIENT"),
        api::StringView(u"MROUND"),
        api::StringView(u"COMBIN"),
        api::StringView(u"COMBINA"),
        api::StringView(u"FACT"),
        api::StringView(u"MULTINOMIAL"),
        api::StringView(u"SERIESSUM"),
        api::StringView(u"SEC"),
        api::StringView(u"SECH"),
        api::StringView(u"CSC"),
        api::StringView(u"CSCH"),
        api::StringView(u"EXP"),
        api::StringView(u"INT"),
        api::StringView(u"SQRT"),
        api::StringView(u"SUMSQ"),
        api::StringView(u"TRUNC"),
        api::StringView(u"MOD"),
        api::StringView(u"RAWSUBTRACT"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kMathFunctions))
        return std::nullopt;
    return evaluateMathFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateMathFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    FunctionEvalContext aContext { *this, rNode, rCurrentAddress };
    const auto evaluateUnaryNumericFinite = [&](auto aCompute) -> EvaluationResult {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        const auto aResult = seutil::makeFiniteResult(aCompute(aValue.maValue));
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    };
    const auto evaluateUnaryNumericFiniteWithError = [&](auto aCompute,
                                                         api::Error eError) -> EvaluationResult {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        const double fResult = aCompute(aValue.maValue);
        if (!std::isfinite(fResult))
            return makeFailure(eError);
        return makeScalarResult(api::CellValue::number(fResult));
    };
    const auto evaluateUnaryNumericOptional = [&](auto aCompute) -> EvaluationResult {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        const auto oResult = aCompute(aValue.maValue);
        if (!oResult)
            return makeFailure(api::Error::IllegalArgument);

        const auto aFinite = seutil::makeFiniteResult(*oResult);
        if (!aFinite)
            return makeFailure(aFinite.meError);
        return makeScalarResult(api::CellValue::number(aFinite.maValue));
    };
    const auto evaluateUnaryNumericOptionalWithError = [&](auto aCompute,
                                                           api::Error eError) -> EvaluationResult {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        const auto oResult = aCompute(aValue.maValue);
        if (!oResult)
            return makeFailure(eError);

        const auto aFinite = seutil::makeFiniteResult(*oResult);
        if (!aFinite)
            return makeFailure(aFinite.meError);
        return makeScalarResult(api::CellValue::number(aFinite.maValue));
    };

if (aFunctionName == u"ABS")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        return makeScalarResult(api::CellValue::number(api::math::abs(aNumber.maValue)));
    }

    if (aFunctionName == u"PI")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(api::math::pi()));
    }

    if (aFunctionName == u"SIN")
        return evaluateUnaryNumericFinite(semath::computeSin);

    if (aFunctionName == u"COS")
        return evaluateUnaryNumericFinite(semath::computeCos);

    if (aFunctionName == u"TAN")
        return evaluateUnaryNumericFinite(semath::computeTan);

    if (aFunctionName == u"COT")
        return evaluateUnaryNumericFinite(semath::computeCot);

    if (aFunctionName == u"ASIN")
        return evaluateUnaryNumericFiniteWithError(semath::computeArcSin, api::Error::Domain);

    if (aFunctionName == u"ACOS")
        return evaluateUnaryNumericFiniteWithError(semath::computeArcCos, api::Error::Domain);

    if (aFunctionName == u"ATAN")
        return evaluateUnaryNumericFinite(semath::computeArcTan);

    if (aFunctionName == u"ACOT")
        return evaluateUnaryNumericFinite(semath::computeArcCot);

    if (aFunctionName == u"DEGREES")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        return makeScalarResult(api::CellValue::number(api::math::degrees(aNumber.maValue)));
    }

    if (aFunctionName == u"RADIANS")
        return evaluateUnaryNumericFinite(semath::computeRadians);

    if (aFunctionName == u"ATAN2")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aY = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aY)
            return makeFailure(aY.meError);
        const auto aX = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aX)
            return makeFailure(aX.meError);

        const auto aResult = seutil::makeFiniteResult(std::atan2(aY.maValue, aX.maValue));
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"SINH")
        return evaluateUnaryNumericFinite(semath::computeSinHyp);

    if (aFunctionName == u"COSH")
        return evaluateUnaryNumericFinite(semath::computeCosHyp);

    if (aFunctionName == u"TANH")
        return evaluateUnaryNumericFinite(semath::computeTanHyp);

    if (aFunctionName == u"COTH")
        return evaluateUnaryNumericFinite(semath::computeCotHyp);

    if (aFunctionName == u"ASINH")
        return evaluateUnaryNumericFinite(semath::computeArcSinHyp);

    if (aFunctionName == u"ACOSH")
        return evaluateUnaryNumericOptionalWithError(semath::computeArcCosHyp, api::Error::Domain);

    if (aFunctionName == u"ATANH")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);

        const auto aResult = api::math::inverseHyperbolicTangent(aNumber.maValue);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"ACOTH")
        return evaluateUnaryNumericOptionalWithError(semath::computeArcCotHyp, api::Error::Domain);

    if (aFunctionName == u"SIGN")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);
        return makeScalarResult(api::CellValue::number(
            static_cast<double>(semath::computePlusMinus(aValue.maValue))));
    }

    if (aFunctionName == u"INT")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);
        return makeScalarResult(api::CellValue::number(semath::computeInt(aValue.maValue)));
    }

    if (aFunctionName == u"GCD" || aFunctionName == u"LCM")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = aContext.collectNumericArguments(true, true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        std::int64_t nResult = 0;
        bool bSawValue = false;
        for (const double fValue : aNumbers.maValue)
        {
            if (!std::isfinite(fValue) || fValue < 0.0)
                return makeFailure(api::Error::IllegalArgument);

            const auto fTruncated = std::trunc(fValue);
            if (fTruncated < static_cast<double>(std::numeric_limits<std::int64_t>::min())
                || fTruncated > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
            {
                return makeFailure(api::Error::IllegalArgument);
            }

            const std::int64_t nValue = static_cast<std::int64_t>(fTruncated);
            if (!bSawValue)
            {
                nResult = std::abs(nValue);
                bSawValue = true;
                continue;
            }

            if (aFunctionName == u"GCD")
                nResult = std::gcd(nResult, std::abs(nValue));
            else
                nResult = std::lcm(nResult, std::abs(nValue));
        }

        return makeScalarResult(api::CellValue::number(static_cast<double>(nResult)));
    }

    if (aFunctionName == u"ROUND" || aFunctionName == u"ROUNDUP" || aFunctionName == u"ROUNDDOWN")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        int nDecimals = 0;
        if (rNode.maChildren.size() == 2)
        {
            EvaluationResult aDecimals
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aDecimals)
                return aDecimals;

            const auto aDigitsNumber = coerceToNumber(aDecimals.maValue.maValue);
            if (!aDigitsNumber || !std::isfinite(aDigitsNumber.maValue))
                return makeFailure(api::Error::IllegalArgument);

            const double fTruncatedDigits = std::trunc(aDigitsNumber.maValue);
            if (fTruncatedDigits < static_cast<double>(std::numeric_limits<int>::min())
                || fTruncatedDigits > static_cast<double>(std::numeric_limits<int>::max()))
            {
                return makeFailure(api::Error::IllegalArgument);
            }

            nDecimals = static_cast<int>(fTruncatedDigits);
        }

        api::RoundingMode eMode = api::RoundingMode::Corrected;
        if (aFunctionName == u"ROUNDUP")
            eMode = api::RoundingMode::Up;
        else if (aFunctionName == u"ROUNDDOWN")
            eMode = api::RoundingMode::Down;

        const auto aRounded = semath::evaluateRoundValue(
            aValueNumber.maValue, nDecimals, eMode,
            aFunctionName == u"ROUNDUP" || aFunctionName == u"ROUNDDOWN");
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"CEILING" || aFunctionName == u"FLOOR" || aFunctionName == u"CEILING.XCL"
        || aFunctionName == u"FLOOR.XCL")
    {
        const bool bMicrosoftCompat = usesMicrosoftCompatibilityName(rNode.maPrimaryText)
                                      || aFunctionName == u"CEILING.XCL"
                                      || aFunctionName == u"FLOOR.XCL";
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3
            || (bMicrosoftCompat && rNode.maChildren.size() != 2))
        {
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        }

        double fValue = 0.0;
        if (rNode.maChildren[0]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aValue
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
            if (!aValue)
                return aValue;

            const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
            if (!aValueNumber)
                return makeScalarResult(api::CellValue::error(aValueNumber.meError));
            fValue = aValueNumber.maValue;
        }

        double fSignificance = 1.0;
        if (rNode.maChildren.size() >= 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aSignificance
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aSignificance)
                return aSignificance;

            const auto aSignificanceNumber = coerceToNumber(aSignificance.maValue.maValue);
            if (!aSignificanceNumber)
                return makeScalarResult(api::CellValue::error(aSignificanceNumber.meError));
            fSignificance = aSignificanceNumber.maValue;
        }

        const bool bMissingSignificance
            = rNode.maChildren.size() < 2
              || rNode.maChildren[1]->meKind == formula::NodeKind::EmptyArgument;

        bool bAbs = false;
        if (!bMicrosoftCompat && rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeScalarResult(api::CellValue::error(aModeNumber.meError));
            bAbs = !fp::approxEqual(aModeNumber.maValue, 0.0);
        }

        if (!bMicrosoftCompat && bMissingSignificance && fValue < 0.0)
            fSignificance = -1.0;

        const auto aRounded = semath::evaluateCeilingFloorValue(
            fValue, fSignificance, bAbs,
            aFunctionName == u"CEILING" || aFunctionName == u"CEILING.XCL",
            bMicrosoftCompat);
        if (!aRounded)
            return makeScalarResult(api::CellValue::error(aRounded.meError));
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"CEILING.MATH" || aFunctionName == u"FLOOR.MATH")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeScalarResult(api::CellValue::error(aValueNumber.meError));

        double fSignificance = 1.0;
        if (rNode.maChildren.size() >= 2)
        {
            EvaluationResult aSignificance
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aSignificance)
                return aSignificance;

            const auto aSignificanceNumber = coerceToNumber(aSignificance.maValue.maValue);
            if (!aSignificanceNumber)
                return makeScalarResult(api::CellValue::error(aSignificanceNumber.meError));
            fSignificance = aSignificanceNumber.maValue;
        }

        if (fSignificance == 0.0 || aValueNumber.maValue == 0.0)
            return makeScalarResult(api::CellValue::number(0.0));

        double fMode = 0.0;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeScalarResult(api::CellValue::error(aModeNumber.meError));
            fMode = aModeNumber.maValue;
        }

        const auto aRounded = semath::evaluateCeilingFloorMathValue(
            aValueNumber.maValue, fSignificance, fMode, aFunctionName == u"CEILING.MATH");
        if (!aRounded)
            return makeScalarResult(api::CellValue::error(aRounded.meError));
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"CEILING.PRECISE" || aFunctionName == u"FLOOR.PRECISE"
        || aFunctionName == u"ISO.CEILING")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeScalarResult(api::CellValue::error(aValueNumber.meError));

        double fSignificance = 1.0;
        if (rNode.maChildren.size() == 2)
        {
            EvaluationResult aSignificance
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aSignificance)
                return aSignificance;

            const auto aSignificanceNumber = coerceToNumber(aSignificance.maValue.maValue);
            if (!aSignificanceNumber)
                return makeScalarResult(api::CellValue::error(aSignificanceNumber.meError));
            fSignificance = aSignificanceNumber.maValue;
        }

        const auto aRounded = semath::evaluateCeilingFloorPreciseValue(
            aValueNumber.maValue, fSignificance, aFunctionName == u"FLOOR.PRECISE");
        if (!aRounded)
            return makeScalarResult(api::CellValue::error(aRounded.meError));
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"ROUNDSIG")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;
        EvaluationResult aDigits
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDigits)
            return aDigits;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aDigitsNumber = coerceToNumber(aDigits.maValue.maValue);
        if (!aDigitsNumber || !std::isfinite(aDigitsNumber.maValue))
            return makeFailure(api::Error::IllegalArgument);

        const auto aRounded
            = semath::evaluateRoundSigValue(aValueNumber.maValue, aDigitsNumber.maValue);
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"BITAND" || aFunctionName == u"BITOR" || aFunctionName == u"BITXOR")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aLeft = aContext.evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aLeft)
            return makeFailure(aLeft.meError);

        if (rNode.maChildren.size() == 1)
            return makeFailure(api::Error::NoValue);

        const auto aRight = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aRight)
            return makeFailure(aRight.meError);

        std::optional<double> oResult;
        if (aFunctionName == u"BITAND")
            oResult = semath::computeBitAnd(aLeft.maValue, aRight.maValue);
        else if (aFunctionName == u"BITOR")
            oResult = semath::computeBitOr(aLeft.maValue, aRight.maValue);
        else
            oResult = semath::computeBitXor(aLeft.maValue, aRight.maValue);

        if (!oResult)
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(*oResult));
    }

    if (aFunctionName == u"BITLSHIFT" || aFunctionName == u"BITRSHIFT")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValueNumber = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aShiftNumber = aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aShiftNumber)
            return makeFailure(aShiftNumber.meError);

        const auto oWholeValue = toWholeNumber(aValueNumber.maValue);
        const auto oWholeShift = toWholeNumber(aShiftNumber.maValue);
        if (!oWholeValue || !oWholeShift || *oWholeValue < 0)
            return makeFailure(api::Error::IllegalArgument);

        const std::int32_t nShift = *oWholeShift;
        const std::uint64_t nValue = static_cast<std::uint64_t>(*oWholeValue);

        if (nShift == 0)
            return makeScalarResult(api::CellValue::number(static_cast<double>(nValue)));

        if (aFunctionName == u"BITLSHIFT")
        {
            if (nShift < 0)
                return makeScalarResult(
                    api::CellValue::number(static_cast<double>(nValue >> (-nShift))));

            if (nShift >= 64)
                return makeFailure(api::Error::IllegalArgument);
            return makeScalarResult(
                api::CellValue::number(static_cast<double>(nValue << nShift)));
        }

        if (nShift < 0)
        {
            const std::int32_t nLeftShift = -nShift;
            if (nLeftShift >= 64)
                return makeFailure(api::Error::IllegalArgument);
            return makeScalarResult(
                api::CellValue::number(static_cast<double>(nValue << nLeftShift)));
        }

        if (nShift >= 64)
            return makeScalarResult(api::CellValue::number(0.0));
        return makeScalarResult(api::CellValue::number(static_cast<double>(nValue >> nShift)));
    }

    if (aFunctionName == u"LOG")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber || !(aValueNumber.maValue > 0.0))
            return makeFailure(api::Error::IllegalArgument);

        double fBase = 10.0;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aBaseArgument
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aBaseArgument)
                return aBaseArgument;

            const auto aBaseNumber = coerceToNumber(aBaseArgument.maValue.maValue);
            if (!aBaseNumber)
                return makeFailure(aBaseNumber.meError);
            fBase = aBaseNumber.maValue;
        }

        const auto aLogarithm = semath::evaluateLogValue(aValueNumber.maValue, fBase);
        if (!aLogarithm)
            return makeFailure(aLogarithm.meError);
        return makeScalarResult(api::CellValue::number(aLogarithm.maValue));
    }

    if (aFunctionName == u"LOG10")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        const auto oLogarithm = semath::computeLog10(aValue.maValue);
        if (!oLogarithm)
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(*oLogarithm));
    }

    if (aFunctionName == u"LN")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        const auto oLogarithm = semath::computeLn(aValue.maValue);
        if (!oLogarithm)
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(*oLogarithm));
    }

    if (aFunctionName == u"POWER")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aBase = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aBase)
            return makeFailure(aBase.meError);
        const auto aExponent = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aExponent)
            return makeFailure(aExponent.meError);

        const double fResult = std::pow(aBase.maValue, aExponent.maValue);
        if (!std::isfinite(fResult))
            return makeFailure(api::Error::Domain);
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"PRODUCT")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = aContext.collectNumericArguments(true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeScalarResult(api::CellValue::number(0.0));

        double fResult = 1.0;
        for (double fValue : aNumbers.maValue)
        {
            fResult *= fValue;
            if (!std::isfinite(fResult))
                return makeFailure(api::Error::IllegalArgument);
        }
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"SUMSQ")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = aContext.collectNumericArguments(true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeScalarResult(api::CellValue::number(0.0));

        double fResult = 0.0;
        for (double fValue : aNumbers.maValue)
        {
            fResult += fValue * fValue;
            if (!std::isfinite(fResult))
                return makeFailure(api::Error::IllegalArgument);
        }
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"QUOTIENT")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumerator = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aNumerator)
            return makeFailure(aNumerator.meError);
        const auto aDenominator = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aDenominator)
            return makeFailure(aDenominator.meError);
        if (fp::approxEqual(aDenominator.maValue, 0.0))
            return makeFailure(api::Error::DivisionByZero);

        return makeScalarResult(api::CellValue::number(
            std::trunc(aNumerator.maValue / aDenominator.maValue)));
    }

    if (aFunctionName == u"MROUND")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);
        if (rNode.maChildren[0]->meKind == formula::NodeKind::EmptyArgument
            || rNode.maChildren[1]->meKind == formula::NodeKind::EmptyArgument)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;
        EvaluationResult aMultipleArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMultipleArgument)
            return aMultipleArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aMultipleNumber = coerceToNumber(aMultipleArgument.maValue.maValue);
        if (!aMultipleNumber)
            return makeFailure(aMultipleNumber.meError);

        const auto aRounded
            = semath::evaluateMroundValue(aValueNumber.maValue, aMultipleNumber.maValue);
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"COMBIN")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aN = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aN)
            return makeFailure(aN.meError);
        const auto aK = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aK)
            return makeFailure(aK.meError);

        const auto aCombin = semath::evaluateCombinValue(aN.maValue, aK.maValue, false);
        if (!aCombin)
            return makeFailure(aCombin.meError);
        return makeScalarResult(api::CellValue::number(aCombin.maValue));
    }

    if (aFunctionName == u"COMBINA")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aN = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aN)
            return makeFailure(aN.meError);
        const auto aK = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aK)
            return makeFailure(aK.meError);

        const auto aCombina = semath::evaluateCombinValue(aN.maValue, aK.maValue, true);
        if (!aCombina)
            return makeFailure(aCombina.meError);
        return makeScalarResult(api::CellValue::number(aCombina.maValue));
    }

    if (aFunctionName == u"FACT")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);
        if (aValue.maValue < 0.0)
            return makeFailure(api::Error::IllegalArgument);

        const double fRounded = std::floor(aValue.maValue);
        const double fResult = std::tgamma(fRounded + 1.0);
        if (!std::isfinite(fResult))
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"MULTINOMIAL")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        std::vector<double> aValues;
        aValues.reserve(rNode.maChildren.size());
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aValue = aContext.evaluateNumericArgument(*pChild, std::nullopt);
            if (!aValue)
                return makeFailure(aValue.meError);
            aValues.push_back(aValue.maValue);
        }

        const auto aMultinomial = semath::evaluateMultinomialValue(aValues);
        if (!aMultinomial)
            return makeFailure(aMultinomial.meError);
        return makeScalarResult(api::CellValue::number(aMultinomial.maValue));
    }

    if (aFunctionName == u"SERIESSUM")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aX = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aX)
            return makeFailure(aX.meError);
        const auto aN = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aN)
            return makeFailure(aN.meError);
        const auto aM = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aM)
            return makeFailure(aM.meError);

        double fResult = 0.0;
        std::size_t nCoefficientIndex = 0;
        const auto aVisited = aContext.visitFlattenedValues(
            *rNode.maChildren[3],
            [&](const api::CellValue& rValue,
                bool bFromReference) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (bFromReference && (rValue.isEmpty() || rValue.isText()))
                    return api::ValueResult<bool>::success(true);
                if (rValue.isEmpty())
                    return api::ValueResult<bool>::success(true);

                const auto aCoefficient = coerceToNumber(rValue);
                if (!aCoefficient)
                    return api::ValueResult<bool>::failure(aCoefficient.meError);

                const double fTerm = aCoefficient.maValue
                                     * std::pow(aX.maValue,
                                         aN.maValue + aM.maValue * static_cast<double>(nCoefficientIndex));
                fResult += fTerm;
                if (!std::isfinite(fResult))
                    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
                ++nCoefficientIndex;
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return makeFailure(aVisited.meError);
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"CSC")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aValue)
            return makeFailure(aValue.meError);
        if (!aValue.maValue.isNumber())
            return makeFailure(api::Error::IllegalArgument);

        const auto aCsc = semath::evaluateCscValue(aValue.maValue.mfNumber);
        if (!aCsc)
            return makeFailure(aCsc.meError);
        return makeScalarResult(api::CellValue::number(aCsc.maValue));
    }

    if (aFunctionName == u"CSCH")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aValue)
            return makeFailure(aValue.meError);
        if (!aValue.maValue.isNumber())
            return makeFailure(api::Error::IllegalArgument);

        const auto aCsch = semath::evaluateCschValue(aValue.maValue.mfNumber);
        if (!aCsch)
            return makeFailure(aCsch.meError);
        return makeScalarResult(api::CellValue::number(aCsch.maValue));
    }

    if (aFunctionName == u"SEC")
        return evaluateUnaryNumericFinite(semath::computeSecant);

    if (aFunctionName == u"SECH")
        return evaluateUnaryNumericFinite(semath::computeSecantHyp);

    if (aFunctionName == u"EXP")
        return evaluateUnaryNumericFinite(semath::computeExp);

    if (aFunctionName == u"SQRT")
        return evaluateUnaryNumericOptional(semath::computeSqrt);

    if (aFunctionName == u"TRUNC")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        std::int32_t nDigits = 0;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aDigits = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aDigits)
                return makeFailure(aDigits.meError);
            const auto oWholeDigits = toWholeNumber(aDigits.maValue);
            if (!oWholeDigits)
                return makeFailure(api::Error::IllegalArgument);
            nDigits = *oWholeDigits;
        }

        const auto aTruncated = semath::evaluateTruncValue(aValue.maValue, nDigits);
        if (!aTruncated)
            return makeFailure(aTruncated.meError);
        return makeScalarResult(api::CellValue::number(aTruncated.maValue));
    }

    if (aFunctionName == u"MOD")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNumerator
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aNumerator)
            return aNumerator;

        EvaluationResult aDenominator
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDenominator)
            return aDenominator;

        const auto aLeftNumber = coerceToNumber(aNumerator.maValue.maValue);
        if (!aLeftNumber)
            return makeFailure(aLeftNumber.meError);
        const auto aRightNumber = coerceToNumber(aDenominator.maValue.maValue);
        if (!aRightNumber)
            return makeFailure(aRightNumber.meError);

        const auto aModResult = semath::evaluateModValue(
            aLeftNumber.maValue, aRightNumber.maValue);
        if (!aModResult)
            return makeFailure(aModResult.meError);
        return makeScalarResult(api::CellValue::number(aModResult.maValue));
    }

    if (aFunctionName == u"RAWSUBTRACT")
    {
        if (rNode.maChildren.size() < 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aFirst
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aFirst)
            return aFirst;
        const auto aFirstNumber = coerceToNumber(aFirst.maValue.maValue);
        if (!aFirstNumber)
            return makeFailure(aFirstNumber.meError);

        double fResult = aFirstNumber.maValue;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            EvaluationResult aNext
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress));
            if (!aNext)
                return aNext;
            const auto aNextNumber = coerceToNumber(aNext.maValue.maValue);
            if (!aNextNumber)
                return makeFailure(aNextNumber.meError);
            fResult -= aNextNumber.maValue;
        }

        return makeScalarResult(api::CellValue::number(fResult));
    }

        return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
