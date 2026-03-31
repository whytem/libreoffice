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

std::optional<EvaluationResult> Evaluator::tryEvaluateConversionFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kConversionFunctions{
        api::StringView(u"EUROCONVERT"),
        api::StringView(u"CONVERT"),
        api::StringView(u"DECIMAL"),
        api::StringView(u"DEC2HEX"),
        api::StringView(u"BASE"),
        api::StringView(u"ROMAN"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kConversionFunctions))
        return std::nullopt;
    return evaluateConversionFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateConversionFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    FunctionEvalContext aContext { *this, rNode, rCurrentAddress };

if (aFunctionName == u"EUROCONVERT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;
        EvaluationResult aFromUnitArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aFromUnitArgument)
            return aFromUnitArgument;
        EvaluationResult aToUnitArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aToUnitArgument)
            return aToUnitArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeScalarResult(api::CellValue::error(aValueNumber.meError));
        const auto aFromUnit = coerceToString(aFromUnitArgument.maValue.maValue);
        if (!aFromUnit)
            return makeScalarResult(api::CellValue::error(aFromUnit.meError));
        const auto aToUnit = coerceToString(aToUnitArgument.maValue.maValue);
        if (!aToUnit)
            return makeScalarResult(api::CellValue::error(aToUnit.meError));

        bool bFullPrecision = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aFullPrecision = aContext.evaluateScalarArgumentValue(*rNode.maChildren[3]);
            if (!aFullPrecision)
                return makeFailure(aFullPrecision.meError);
            if (!aFullPrecision.maValue.isEmpty())
            {
                const auto aBool = coerceToBoolean(aFullPrecision.maValue);
                if (!aBool)
                    return makeScalarResult(api::CellValue::error(aBool.meError));
                bFullPrecision = aBool.maValue;
            }
        }

        if (rNode.maChildren.size() == 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

            const auto aTriangulationPrecision
                = aContext.evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aTriangulationPrecision)
                return makeScalarResult(api::CellValue::error(aTriangulationPrecision.meError));

            const auto oWholePrecision = toWholeNumber(aTriangulationPrecision.maValue);
            if (!oWholePrecision || *oWholePrecision < 3)
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        }

        const auto aConverted = seconvert::evaluateEuroConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue, true, !bFullPrecision);
        if (!aConverted)
            return makeScalarResult(api::CellValue::error(aConverted.meError));
        return makeScalarResult(api::CellValue::number(aConverted.maValue));
    }

    if (aFunctionName == u"CONVERT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        if (rNode.maChildren.size() > 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;
        EvaluationResult aFromUnitArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aFromUnitArgument)
            return aFromUnitArgument;
        EvaluationResult aToUnitArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aToUnitArgument)
            return aToUnitArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeScalarResult(api::CellValue::error(aValueNumber.meError));
        const auto aFromUnit = coerceToString(aFromUnitArgument.maValue.maValue);
        if (!aFromUnit)
            return makeScalarResult(api::CellValue::error(aFromUnit.meError));
        const auto aToUnit = coerceToString(aToUnitArgument.maValue.maValue);
        if (!aToUnit)
            return makeScalarResult(api::CellValue::error(aToUnit.meError));

        const auto aConverted = seconvert::evaluateConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue);
        if (aConverted)
            return makeScalarResult(api::CellValue::number(aConverted.maValue));

        const auto aEuroConverted = seconvert::evaluateEuroConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue, false, false);
        if (aEuroConverted)
        {
            return makeScalarResult(api::CellValue::number(aEuroConverted.maValue));
        }

        return makeScalarResult(api::CellValue::error(aConverted.meError));
    }

    if (aFunctionName == u"DECIMAL")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aTextArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aTextArgument)
            return aTextArgument;
        EvaluationResult aBaseArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aBaseArgument)
            return aBaseArgument;

        const auto aText = coerceToString(aTextArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        const auto aBaseNumber = coerceToNumber(aBaseArgument.maValue.maValue);
        if (!aBaseNumber)
            return makeFailure(aBaseNumber.meError);

        const auto aResult = seconvert::evaluateDecimalValue(aText.maValue, aBaseNumber.maValue);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"DEC2HEX")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        std::optional<double> oPlaces;
        if (rNode.maChildren.size() == 2)
        {
            EvaluationResult aPlacesArgument
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aPlacesArgument)
                return aPlacesArgument;

            const auto aPlacesNumber = coerceToNumber(aPlacesArgument.maValue.maValue);
            if (!aPlacesNumber)
                return makeFailure(aPlacesNumber.meError);
            oPlaces = aPlacesNumber.maValue;
        }

        const auto aResult = seconvert::evaluateBaseValue(aValueNumber.maValue, 16.0, oPlaces);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::text(aResult.maValue));
    }

    if (aFunctionName == u"BASE")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;
        EvaluationResult aBaseArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aBaseArgument)
            return aBaseArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aBaseNumber = coerceToNumber(aBaseArgument.maValue.maValue);
        if (!aBaseNumber)
            return makeFailure(aBaseNumber.meError);

        std::optional<double> ofMinLength;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aLengthArgument
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aLengthArgument)
                return aLengthArgument;

            const auto aLengthNumber = coerceToNumber(aLengthArgument.maValue.maValue);
            if (!aLengthNumber)
                return makeFailure(aLengthNumber.meError);
            ofMinLength = aLengthNumber.maValue;
        }

        const auto aResult = seconvert::evaluateBaseValue(
            aValueNumber.maValue, aBaseNumber.maValue, ofMinLength);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::text(aResult.maValue));
    }

    if (aFunctionName == u"ROMAN")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        std::optional<double> ofMode;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aModeArgument
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aModeArgument)
                return aModeArgument;

            const auto aModeNumber = coerceToNumber(aModeArgument.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            ofMode = aModeNumber.maValue;
        }

        const auto aResult = seconvert::evaluateRomanValue(aValueNumber.maValue, ofMode);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::text(aResult.maValue));
    }

        return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
