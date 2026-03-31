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

#include <unicode/regex.h>

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

std::optional<EvaluationResult> Evaluator::tryEvaluateTextFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kTextFunctions{
        api::StringView(u"CONCATENATE"),
        api::StringView(u"CLEAN"),
        api::StringView(u"CHAR"),
        api::StringView(u"CODE"),
        api::StringView(u"UNICHAR"),
        api::StringView(u"UPPER"),
        api::StringView(u"LOWER"),
        api::StringView(u"PROPER"),
        api::StringView(u"ASC"),
        api::StringView(u"JIS"),
        api::StringView(u"LEN"),
        api::StringView(u"LENB"),
        api::StringView(u"FINDB"),
        api::StringView(u"SEARCHB"),
        api::StringView(u"REPLACEB"),
        api::StringView(u"SUBSTITUTE"),
        api::StringView(u"SEARCH"),
        api::StringView(u"FIND"),
        api::StringView(u"TEXTAFTER"),
        api::StringView(u"TEXTBEFORE"),
        api::StringView(u"MID"),
        api::StringView(u"REPLACE"),
        api::StringView(u"LEFT"),
        api::StringView(u"RIGHT"),
        api::StringView(u"TEXTJOIN"),
        api::StringView(u"CONCAT"),
        api::StringView(u"NUMBERVALUE"),
        api::StringView(u"REGEX"),
        api::StringView(u"T"),
        api::StringView(u"EXACT"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kTextFunctions))
        return std::nullopt;
    return evaluateTextFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateTextFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    FunctionEvalContext aContext { *this, rNode, rCurrentAddress };

if (aFunctionName == u"CONCATENATE")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        api::String aResult;
        for (const auto& pChild : rNode.maChildren)
        {
            EvaluationResult aArgument
                = ensureScalarValue(*this, evaluateNode(*pChild, rCurrentAddress));
            if (!aArgument)
                return aArgument;

            const auto aText = coerceToString(aArgument.maValue.maValue);
            if (!aText)
                return makeFailure(aText.meError);
            aResult += aText.maValue;
        }

        return makeScalarResult(api::CellValue::text(aResult));
    }

    if (aFunctionName == u"CLEAN")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        return makeScalarResult(api::CellValue::text(
            api::text::cleanPrintable(aText.maValue)));
    }

    if (aFunctionName == u"CHAR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aCode = coerceToNumber(aArgument.maValue.maValue);
        if (!aCode)
            return makeFailure(aCode.meError);

        const auto oWholeNumber = toWholeNumber(aCode.maValue);
        if (!oWholeNumber || *oWholeNumber < 1 || *oWholeNumber > 255)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCharacter = api::text::charFromValue(
            setext::defaultSingleByteEncodingService(), static_cast<double>(*oWholeNumber));
        if (!aCharacter)
            return makeFailure(aCharacter.meError);
        return makeScalarResult(api::CellValue::text(aCharacter.maValue));
    }

    if (aFunctionName == u"CODE")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        if (aText.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(static_cast<double>(
            api::text::codeFromText(
                setext::defaultSingleByteEncodingService(), aText.maValue))));
    }

    if (aFunctionName == u"UNICHAR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aCodePoint = coerceToNumber(aArgument.maValue.maValue);
        if (!aCodePoint)
            return makeFailure(aCodePoint.meError);

        const auto oWholeNumber = toWholeNumber(aCodePoint.maValue);
        if (!oWholeNumber || *oWholeNumber < 0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCharacter
            = api::text::unicharFromCodePoint(static_cast<std::uint32_t>(*oWholeNumber));
        if (!aCharacter)
            return makeFailure(aCharacter.meError);

        return makeScalarResult(api::CellValue::text(aCharacter.maValue));
    }

    if (aFunctionName == u"UPPER" || aFunctionName == u"LOWER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        const api::String aResult = aFunctionName == u"UPPER"
                                        ? api::text::uppercase(
                                              setext::defaultCaseMappingService(), aText.maValue)
                                        : api::text::lowercase(
                                              setext::defaultCaseMappingService(), aText.maValue);
        return makeScalarResult(api::CellValue::text(aResult));
    }

    if (aFunctionName == u"PROPER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        return makeScalarResult(api::CellValue::text(api::text::propercase(
            setext::defaultCaseMappingService(), aText.maValue)));
    }

    if (aFunctionName == u"ASC" || aFunctionName == u"JIS")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        const api::String aConverted = aFunctionName == u"ASC"
                                           ? api::text::convertIntoHalfWidth(
                                                 setext::defaultWidthConversionService(),
                                                 aText.maValue)
                                           : api::text::convertIntoFullWidth(
                                                 setext::defaultWidthConversionService(),
                                                 aText.maValue);
        return makeScalarResult(api::CellValue::text(aConverted));
    }

    if (aFunctionName == u"LEN")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(api::text::countCodePoints(aText.maValue))));
    }

    if (aFunctionName == u"LENB")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        return makeScalarResult(api::CellValue::number(
            static_cast<double>(setext::expandDbcsByteText(aText.maValue, false).size())));
    }

    if (aFunctionName == u"FINDB" || aFunctionName == u"SEARCHB")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNeedleArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aNeedleArgument)
            return aNeedleArgument;
        EvaluationResult aHaystackArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aHaystackArgument)
            return aHaystackArgument;

        const auto aNeedle = coerceToString(aNeedleArgument.maValue.maValue);
        if (!aNeedle)
            return makeFailure(aNeedle.meError);
        const auto aHaystack = coerceToString(aHaystackArgument.maValue.maValue);
        if (!aHaystack)
            return makeFailure(aHaystack.meError);

        std::size_t nStartIndex = 0;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aStart = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aStart)
                return makeFailure(aStart.meError);
            const auto oWholeStart = toWholeNumber(aStart.maValue);
            if (!oWholeStart || *oWholeStart < 1)
                return makeFailure(api::Error::IllegalArgument);
            nStartIndex = static_cast<std::size_t>(*oWholeStart - 1);
        }

        const auto oFoundIndex = setext::findByteText(aNeedle.maValue, aHaystack.maValue,
            nStartIndex, aFunctionName == u"SEARCHB");
        if (!oFoundIndex)
            return makeFailure(api::Error::NotAvailable);

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(*oFoundIndex + 1)));
    }

    if (aFunctionName == u"REPLACEB")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aSourceArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aSourceArgument)
            return aSourceArgument;
        EvaluationResult aStartArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aStartArgument)
            return aStartArgument;
        EvaluationResult aLengthArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aLengthArgument)
            return aLengthArgument;
        EvaluationResult aReplacementArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aReplacementArgument)
            return aReplacementArgument;

        const auto aSource = coerceToString(aSourceArgument.maValue.maValue);
        if (!aSource)
            return makeFailure(aSource.meError);
        const auto aStart = coerceToNumber(aStartArgument.maValue.maValue);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aLength = coerceToNumber(aLengthArgument.maValue.maValue);
        if (!aLength)
            return makeFailure(aLength.meError);
        const auto aReplacement = coerceToString(aReplacementArgument.maValue.maValue);
        if (!aReplacement)
            return makeFailure(aReplacement.meError);

        const auto oWholeStart = toWholeNumber(aStart.maValue);
        const auto oWholeLength = toWholeNumber(aLength.maValue);
        if (!oWholeStart || !oWholeLength || *oWholeStart < 1 || *oWholeLength < 0)
            return makeFailure(api::Error::IllegalArgument);

        const std::size_t nStartIndex = static_cast<std::size_t>(*oWholeStart - 1);
        const std::size_t nReplaceLength = static_cast<std::size_t>(*oWholeLength);
        const api::String aExpandedSource = setext::expandDbcsByteText(aSource.maValue, false);
        if (nStartIndex >= aExpandedSource.size()
            || nReplaceLength > aExpandedSource.size() - nStartIndex)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        return makeScalarResult(api::CellValue::text(setext::replaceByteText(
            aSource.maValue, nStartIndex, nReplaceLength, aReplacement.maValue)));
    }

    if (aFunctionName == u"SUBSTITUTE")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aSourceArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aSourceArgument)
            return aSourceArgument;
        EvaluationResult aOldArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aOldArgument)
            return aOldArgument;
        EvaluationResult aNewArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aNewArgument)
            return aNewArgument;

        const auto aSource = coerceToString(aSourceArgument.maValue.maValue);
        if (!aSource)
            return makeFailure(aSource.meError);
        const auto aOldText = coerceToString(aOldArgument.maValue.maValue);
        if (!aOldText)
            return makeFailure(aOldText.meError);
        const auto aNewText = coerceToString(aNewArgument.maValue.maValue);
        if (!aNewText)
            return makeFailure(aNewText.meError);

        if (aOldText.maValue.empty())
            return makeScalarResult(api::CellValue::text(aSource.maValue));

        std::optional<std::int32_t> oInstance;
        if (rNode.maChildren.size() == 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = aContext.evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aInstance)
                return makeFailure(aInstance.meError);
            const auto oWholeInstance = toWholeNumber(aInstance.maValue);
            if (!oWholeInstance || *oWholeInstance < 1)
                return makeFailure(api::Error::IllegalArgument);
            oInstance = *oWholeInstance;
        }

        return makeScalarResult(api::CellValue::text(
            setext::substituteText(aSource.maValue, aOldText.maValue, aNewText.maValue, oInstance)));
    }

    if (aFunctionName == u"SEARCH" || aFunctionName == u"FIND")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNeedleArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aNeedleArgument)
            return aNeedleArgument;
        EvaluationResult aHaystackArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aHaystackArgument)
            return aHaystackArgument;

        const auto aNeedle = coerceToString(aNeedleArgument.maValue.maValue);
        if (!aNeedle)
            return makeFailure(aNeedle.meError);
        const auto aHaystack = coerceToString(aHaystackArgument.maValue.maValue);
        if (!aHaystack)
            return makeFailure(aHaystack.meError);

        std::int32_t nStart = 1;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aStart = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aStart)
                return makeFailure(aStart.meError);
            const auto oWholeStart = toWholeNumber(aStart.maValue);
            if (!oWholeStart || *oWholeStart < 1)
                return makeFailure(api::Error::IllegalArgument);
            nStart = *oWholeStart;
        }

        const auto oFoundIndex = setext::findText(
            aNeedle.maValue, aHaystack.maValue, nStart - 1, aFunctionName == u"SEARCH");
        if (!oFoundIndex)
            return makeFailure(api::Error::NotAvailable);

        return makeScalarResult(api::CellValue::number(static_cast<double>(*oFoundIndex + 1)));
    }

    if (aFunctionName == u"TEXTAFTER")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTextValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTextValue)
            return makeFailure(aTextValue.meError);
        const auto aText = coerceToString(aTextValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        std::vector<api::String> aDelimiters;
        const auto aDelimiterVisit = aContext.visitFlattenedValues( *rNode.maChildren[1],
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                const auto aDelimiter = coerceToString(rValue);
                if (!aDelimiter)
                    return api::ValueResult<bool>::failure(aDelimiter.meError);
                aDelimiters.push_back(aDelimiter.maValue);
                return api::ValueResult<bool>::success(true);
            });
        if (!aDelimiterVisit)
            return makeFailure(aDelimiterVisit.meError);
        if (aDelimiters.empty())
            return makeFailure(api::Error::IllegalArgument);

        std::int32_t nInstance = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aInstance)
                return makeFailure(aInstance.meError);
            const auto oWholeInstance = toWholeNumber(aInstance.maValue);
            if (!oWholeInstance || *oWholeInstance == 0)
                return makeFailure(api::Error::IllegalArgument);
            nInstance = *oWholeInstance;
        }

        bool bCaseInsensitive = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchMode = aContext.evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aMatchMode)
                return makeFailure(aMatchMode.meError);
            const auto oWholeMatchMode = toWholeNumber(aMatchMode.maValue);
            if (!oWholeMatchMode || (*oWholeMatchMode != 0 && *oWholeMatchMode != 1))
                return makeFailure(api::Error::IllegalArgument);
            bCaseInsensitive = *oWholeMatchMode == 1;
        }

        bool bMatchEnd = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchEnd = aContext.evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aMatchEnd)
                return makeFailure(aMatchEnd.meError);
            const auto aMatchEndBool = coerceToBoolean(aMatchEnd.maValue);
            if (!aMatchEndBool)
                return makeFailure(aMatchEndBool.meError);
            bMatchEnd = aMatchEndBool.maValue;
        }

        const auto handleNotFound = [&]() -> EvaluationResult {
            if (rNode.maChildren.size() >= 6)
                return evaluateNode(*rNode.maChildren[5], rCurrentAddress);
            return makeFailure(api::Error::NotAvailable);
        };
        const auto oResult
            = setext::textAfter(aText.maValue, aDelimiters, nInstance, bCaseInsensitive, bMatchEnd);
        if (!oResult)
            return handleNotFound();
        return makeScalarResult(api::CellValue::text(*oResult));
    }

    if (aFunctionName == u"TEXTBEFORE")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTextValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTextValue)
            return makeFailure(aTextValue.meError);
        const auto aText = coerceToString(aTextValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        std::vector<api::String> aDelimiters;
        const auto aDelimiterVisit = aContext.visitFlattenedValues( *rNode.maChildren[1],
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                const auto aDelimiter = coerceToString(rValue);
                if (!aDelimiter)
                    return api::ValueResult<bool>::failure(aDelimiter.meError);
                aDelimiters.push_back(aDelimiter.maValue);
                return api::ValueResult<bool>::success(true);
            });
        if (!aDelimiterVisit)
            return makeFailure(aDelimiterVisit.meError);
        if (aDelimiters.empty())
            return makeFailure(api::Error::IllegalArgument);

        std::int32_t nInstance = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aInstance)
                return makeFailure(aInstance.meError);
            const auto oWholeInstance = toWholeNumber(aInstance.maValue);
            if (!oWholeInstance || *oWholeInstance == 0)
                return makeFailure(api::Error::IllegalArgument);
            nInstance = *oWholeInstance;
        }

        bool bCaseInsensitive = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchMode = aContext.evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aMatchMode)
                return makeFailure(aMatchMode.meError);
            const auto oWholeMatchMode = toWholeNumber(aMatchMode.maValue);
            if (!oWholeMatchMode || (*oWholeMatchMode != 0 && *oWholeMatchMode != 1))
                return makeFailure(api::Error::IllegalArgument);
            bCaseInsensitive = *oWholeMatchMode == 1;
        }

        bool bMatchEnd = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchEnd = aContext.evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aMatchEnd)
                return makeFailure(aMatchEnd.meError);
            const auto aMatchEndBool = coerceToBoolean(aMatchEnd.maValue);
            if (!aMatchEndBool)
                return makeFailure(aMatchEndBool.meError);
            bMatchEnd = aMatchEndBool.maValue;
        }

        const auto handleNotFound = [&]() -> EvaluationResult {
            if (rNode.maChildren.size() >= 6)
                return evaluateNode(*rNode.maChildren[5], rCurrentAddress);
            return makeFailure(api::Error::NotAvailable);
        };
        const auto oResult
            = setext::textBefore(aText.maValue, aDelimiters, nInstance, bCaseInsensitive, bMatchEnd);
        if (!oResult)
            return handleNotFound();
        return makeScalarResult(api::CellValue::text(*oResult));
    }

    if (aFunctionName == u"MID")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aTextArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aTextArgument)
            return aTextArgument;
        EvaluationResult aStartArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aStartArgument)
            return aStartArgument;
        EvaluationResult aLengthArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aLengthArgument)
            return aLengthArgument;

        const auto aText = coerceToString(aTextArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        const auto aStart = coerceToNumber(aStartArgument.maValue.maValue);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aLength = coerceToNumber(aLengthArgument.maValue.maValue);
        if (!aLength)
            return makeFailure(aLength.meError);

        const auto oWholeStart = toWholeNumber(aStart.maValue);
        const auto oWholeLength = toWholeNumber(aLength.maValue);
        if (!oWholeStart || !oWholeLength || *oWholeStart < 1 || *oWholeLength < 0)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::text(
            setext::sliceText(aText.maValue, *oWholeStart - 1, *oWholeLength)));
    }

    if (aFunctionName == u"REPLACE")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aSourceArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aSourceArgument)
            return aSourceArgument;
        EvaluationResult aStartArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aStartArgument)
            return aStartArgument;
        EvaluationResult aLengthArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aLengthArgument)
            return aLengthArgument;
        EvaluationResult aReplacementArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aReplacementArgument)
            return aReplacementArgument;

        const auto aSource = coerceToString(aSourceArgument.maValue.maValue);
        if (!aSource)
            return makeFailure(aSource.meError);
        const auto aStart = coerceToNumber(aStartArgument.maValue.maValue);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aLength = coerceToNumber(aLengthArgument.maValue.maValue);
        if (!aLength)
            return makeFailure(aLength.meError);
        const auto aReplacement = coerceToString(aReplacementArgument.maValue.maValue);
        if (!aReplacement)
            return makeFailure(aReplacement.meError);

        const auto oWholeStart = toWholeNumber(aStart.maValue);
        const auto oWholeLength = toWholeNumber(aLength.maValue);
        if (!oWholeStart || !oWholeLength || *oWholeStart < 1 || *oWholeLength < 0)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::text(setext::replaceText(
            aSource.maValue, *oWholeStart - 1, *oWholeLength, aReplacement.maValue)));
    }

    if (aFunctionName == u"LEFT" || aFunctionName == u"RIGHT")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aTextArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aTextArgument)
            return aTextArgument;

        const auto aText = coerceToString(aTextArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        std::int32_t nLength = 1;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aLength = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aLength)
                return makeFailure(aLength.meError);
            const auto oWholeLength = toWholeNumber(aLength.maValue);
            if (!oWholeLength || *oWholeLength < 0)
                return makeFailure(api::Error::IllegalArgument);
            nLength = *oWholeLength;
        }

        return makeScalarResult(api::CellValue::text(setext::sliceTextLeftRight(
            aText.maValue, nLength, aFunctionName == u"RIGHT")));
    }

    if (aFunctionName == u"TEXTJOIN")
    {
        if (rNode.maChildren.size() < 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDelimiterValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDelimiterValue)
            return makeFailure(aDelimiterValue.meError);
        const auto aDelimiter = coerceToString(aDelimiterValue.maValue);
        if (!aDelimiter)
            return makeFailure(aDelimiter.meError);

        const auto aIgnoreEmptyValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[1]);
        if (!aIgnoreEmptyValue)
            return makeFailure(aIgnoreEmptyValue.meError);
        const auto aIgnoreEmpty = coerceToBoolean(aIgnoreEmptyValue.maValue);
        if (!aIgnoreEmpty)
            return makeFailure(aIgnoreEmpty.meError);

        api::String aResult;
        bool bHaveAny = false;
        for (std::size_t nIndex = 2; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aVisited = aContext.visitFlattenedValues( *rNode.maChildren[nIndex],
                [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);
                    if (rValue.isEmpty() && aIgnoreEmpty.maValue)
                        return api::ValueResult<bool>::success(true);

                    const auto aTextValue = coerceToString(rValue);
                    if (!aTextValue)
                        return api::ValueResult<bool>::failure(aTextValue.meError);

                    if (aTextValue.maValue.empty() && aIgnoreEmpty.maValue)
                        return api::ValueResult<bool>::success(true);

                    if (bHaveAny)
                        aResult += aDelimiter.maValue;
                    aResult += aTextValue.maValue;
                    bHaveAny = true;
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return makeFailure(aVisited.meError);
        }

        return makeScalarResult(api::CellValue::text(aResult));
    }

    if (aFunctionName == u"CONCAT")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        api::String aResult;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = aContext.visitFlattenedValues( *pChild,
                [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);
                    const auto aTextValue = coerceToString(rValue);
                    if (!aTextValue)
                        return api::ValueResult<bool>::failure(aTextValue.meError);
                    aResult += aTextValue.maValue;
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return makeFailure(aVisited.meError);
        }

        return makeScalarResult(api::CellValue::text(aResult));
    }

    if (aFunctionName == u"NUMBERVALUE")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
        {
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        }

        EvaluationResult aInput
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aInput)
            return makeScalarResult(api::CellValue::error(aInput.meError));

        if (aInput.maValue.maValue.isNumber() || aInput.maValue.maValue.isBoolean())
            return makeScalarResult(api::CellValue::number(aInput.maValue.maValue.mfNumber));

        const auto aInputText = coerceToString(aInput.maValue.maValue);
        if (!aInputText)
            return makeScalarResult(api::CellValue::error(aInputText.meError));

        std::optional<api::String> oDecimalSeparator;
        std::optional<api::String> oGroupSeparator;
        if (rNode.maChildren.size() >= 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aDecimal
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aDecimal)
                return makeScalarResult(api::CellValue::error(aDecimal.meError));
            const auto aDecimalText = coerceToString(aDecimal.maValue.maValue);
            if (!aDecimalText)
                return makeScalarResult(api::CellValue::error(aDecimalText.meError));
            oDecimalSeparator = aDecimalText.maValue;
        }
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aGroup
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aGroup)
                return makeScalarResult(api::CellValue::error(aGroup.meError));
            const auto aGroupText = coerceToString(aGroup.maValue.maValue);
            if (!aGroupText)
                return makeScalarResult(api::CellValue::error(aGroupText.meError));
            oGroupSeparator = aGroupText.maValue;
        }

        const auto aParsed
            = api::text::parseNumberValue(aInputText.maValue, oDecimalSeparator, oGroupSeparator, false);
        if (!aParsed)
            return makeScalarResult(api::CellValue::error(aParsed.meError));
        return makeScalarResult(api::CellValue::number(aParsed.maValue));
    }

    if (aFunctionName == u"REGEX")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        auto evaluateTextArgument = [&](const formula::Node& rArgument)
            -> api::ValueResult<api::String> {
            EvaluationResult aValue
                = ensureScalarValue(*this, evaluateNode(rArgument, rCurrentAddress));
            if (!aValue)
                return api::ValueResult<api::String>::failure(aValue.meError);
            return coerceToString(aValue.maValue.maValue);
        };

        bool bGlobalReplacement = false;
        sal_Int32 nOccurrence = 1;
        if (rNode.maChildren.size() == 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aFlagsOrOccurrence
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aFlagsOrOccurrence)
            {
                return makeScalarResult(api::CellValue::error(aFlagsOrOccurrence.meError));
            }

            if (aFlagsOrOccurrence.maValue.maValue.isNumber()
                || aFlagsOrOccurrence.maValue.maValue.isBoolean())
            {
                const auto aOccurrence = coerceToNumber(aFlagsOrOccurrence.maValue.maValue);
                if (!aOccurrence)
                    return makeScalarResult(api::CellValue::error(aOccurrence.meError));
                const auto oWholeOccurrence = toWholeNumber(aOccurrence.maValue);
                if (!oWholeOccurrence)
                    return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
                nOccurrence = static_cast<sal_Int32>(*oWholeOccurrence);
            }
            else
            {
                const auto aFlags = coerceToString(aFlagsOrOccurrence.maValue.maValue);
                if (!aFlags)
                    return makeScalarResult(api::CellValue::error(aFlags.meError));
                if (aFlags.maValue.size() > 1)
                    return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
                if (!aFlags.maValue.empty())
                {
                    if (aFlags.maValue == u"g")
                        bGlobalReplacement = true;
                    else
                        return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
                }
            }
        }

        const bool bReplacementArgPresent
            = rNode.maChildren.size() >= 3
              && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument;
        api::String aReplacement;
        const bool bReplacement = bReplacementArgPresent && nOccurrence != 0;
        if (bReplacementArgPresent)
        {
            const auto aReplacementText = evaluateTextArgument(*rNode.maChildren[2]);
            if (!aReplacementText)
                return makeScalarResult(api::CellValue::error(aReplacementText.meError));
            aReplacement = aReplacementText.maValue;
        }

        const auto aExpression = evaluateTextArgument(*rNode.maChildren[1]);
        if (!aExpression)
            return makeScalarResult(api::CellValue::error(aExpression.meError));
        const auto aText = evaluateTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeScalarResult(api::CellValue::error(aText.meError));

        if (nOccurrence == 0)
            return makeScalarResult(api::CellValue::text(aText.maValue));

        UErrorCode eStatus = U_ZERO_ERROR;
        const icu::UnicodeString aPattern(
            false, reinterpret_cast<const UChar*>(aExpression.maValue.data()),
            static_cast<int32_t>(aExpression.maValue.size()));
        icu::RegexMatcher aMatcher(aPattern, 0, eStatus);
        if (U_FAILURE(eStatus))
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        aMatcher.setTimeLimit(23 * 1000, eStatus);

        const icu::UnicodeString aIcuText(
            false, reinterpret_cast<const UChar*>(aText.maValue.data()),
            static_cast<int32_t>(aText.maValue.size()));
        aMatcher.reset(aIcuText);

        if (!bReplacement)
        {
            sal_Int32 nCount = 0;
            while (aMatcher.find(eStatus) && U_SUCCESS(eStatus) && ++nCount < nOccurrence)
                ;
            if (U_FAILURE(eStatus))
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
            if (nCount != nOccurrence)
                return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

            const icu::UnicodeString aMatch(aMatcher.group(eStatus));
            if (U_FAILURE(eStatus))
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
            return makeScalarResult(api::CellValue::text(api::String(
                reinterpret_cast<const char16_t*>(aMatch.getBuffer()),
                static_cast<std::size_t>(aMatch.length()))));
        }

        const icu::UnicodeString aIcuReplacement(
            false, reinterpret_cast<const UChar*>(aReplacement.data()),
            static_cast<int32_t>(aReplacement.size()));
        icu::UnicodeString aReplaced;
        if (bGlobalReplacement)
            aReplaced = aMatcher.replaceAll(aIcuReplacement, eStatus);
        else if (nOccurrence == 1)
            aReplaced = aMatcher.replaceFirst(aIcuReplacement, eStatus);
        else
        {
            sal_Int32 nCount = 0;
            while (aMatcher.find(eStatus) && U_SUCCESS(eStatus))
            {
                if (++nCount == nOccurrence)
                {
                    aMatcher.appendReplacement(aReplaced, aIcuReplacement, eStatus);
                    break;
                }
            }
            aMatcher.appendTail(aReplaced);
        }
        if (U_FAILURE(eStatus))
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        return makeScalarResult(api::CellValue::text(api::String(
            reinterpret_cast<const char16_t*>(aReplaced.getBuffer()),
            static_cast<std::size_t>(aReplaced.length()))));
    }

    if (aFunctionName == u"T")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        if (aValue.maValue.maValue.isError())
            return makeFailure(aValue.maValue.maValue.meError);
        if (aValue.maValue.maValue.isText())
            return makeScalarResult(api::CellValue::text(aValue.maValue.maValue.maString));
        return makeScalarResult(api::CellValue::text({}));
    }

    if (aFunctionName == u"EXACT")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        auto materializeFirstValue = [&](const formula::Node& rArgument) -> EvaluationResult {
            EvaluationResult aValue = evaluateNode(rArgument, rCurrentAddress);
            if (!aValue)
                return aValue;
            if (aValue.maValue.isScalar())
                return aValue;
            return materializeReferenceValue(aValue.maValue.maReference, 0, 0);
        };

        EvaluationResult aLeft = materializeFirstValue(*rNode.maChildren[0]);
        if (!aLeft)
            return aLeft;

        EvaluationResult aRight = materializeFirstValue(*rNode.maChildren[1]);
        if (!aRight)
            return aRight;

        const auto aLeftText = coerceToString(aLeft.maValue.maValue);
        if (!aLeftText)
            return makeFailure(aLeftText.meError);

        const auto aRightText = coerceToString(aRight.maValue.maValue);
        if (!aRightText)
            return makeFailure(aRightText.meError);

        return makeScalarResult(api::CellValue::boolean(
            aLeftText.maValue == aRightText.maValue));
    }

        return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
