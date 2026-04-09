/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstdlib>
#include <optional>
#include <string_view>

#include <document.hxx>
#include <interpretercontext.hxx>
#include <svl/numformat.hxx>

#include <spreadsheetengine/api/FormulaResult.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx>
#include <spreadsheetengine/detail/OdfFormulaParser.hxx>

namespace spreadsheetengine::compat::libreoffice::interprettaileval
{

enum class RolloutMode : sal_uInt8
{
    Off,
    Observe,
    ShadowCompare,
    AuthoritativeWithFallback
};

enum class FallbackReason : sal_uInt8
{
    UnsupportedTailContext,
    UnsupportedFormulaShape,
    UnsupportedFunction,
    ParseFailure,
    ProjectionFailure,
    ShadowMismatch,
    Count
};

enum class MismatchReason : sal_uInt8
{
    Error,
    NumericValue,
    FormatType,
    Count
};

struct EvaluationAttempt
{
    bool mbSupported = false;
    api::formulavalue::FormulaResultValue maResult
        = api::formulavalue::makeInvalidResult();
    SvNumFormatType meFormatType = SvNumFormatType::ALL;
    FallbackReason meFallbackReason = FallbackReason::UnsupportedFormulaShape;
};

struct StatsSnapshot
{
    sal_uInt64 mnObserveCount = 0;
    sal_uInt64 mnShadowCompareCount = 0;
    sal_uInt64 mnAuthoritativeCount = 0;
    sal_uInt64 mnAuthoritativeFallbackCount = 0;
    sal_uInt64 mnShadowMatchCount = 0;
    std::array<sal_uInt64, static_cast<std::size_t>(FallbackReason::Count)> maFallbackReasons {};
    std::array<sal_uInt64, static_cast<std::size_t>(MismatchReason::Count)> maMismatchReasons {};
};

namespace detail
{

struct StatsStore
{
    std::atomic<sal_uInt64> mnObserveCount { 0 };
    std::atomic<sal_uInt64> mnShadowCompareCount { 0 };
    std::atomic<sal_uInt64> mnAuthoritativeCount { 0 };
    std::atomic<sal_uInt64> mnAuthoritativeFallbackCount { 0 };
    std::atomic<sal_uInt64> mnShadowMatchCount { 0 };
    std::array<std::atomic<sal_uInt64>, static_cast<std::size_t>(FallbackReason::Count)>
        maFallbackReasons {};
    std::array<std::atomic<sal_uInt64>, static_cast<std::size_t>(MismatchReason::Count)>
        maMismatchReasons {};
};

[[nodiscard]] inline StatsStore& statsStore()
{
    static StatsStore aStore;
    return aStore;
}

[[nodiscard]] constexpr std::size_t toIndex(FallbackReason eReason)
{
    return static_cast<std::size_t>(eReason);
}

[[nodiscard]] constexpr std::size_t toIndex(MismatchReason eReason)
{
    return static_cast<std::size_t>(eReason);
}

[[nodiscard]] inline bool envEnabled(std::string_view rValue)
{
    return !rValue.empty() && rValue != "0" && rValue != "off" && rValue != "false";
}

[[nodiscard]] inline api::String uppercaseAscii(api::StringView rValue)
{
    api::String aNormalized(rValue);
    for (auto& rChar : aNormalized)
    {
        if (rChar >= u'a' && rChar <= u'z')
            rChar = static_cast<char16_t>(rChar - u'a' + u'A');
    }
    return aNormalized;
}

[[nodiscard]] inline std::u16string_view trimFormulaEquals(std::u16string_view rSource)
{
    if (!rSource.empty() && rSource.front() == u'=')
    {
        rSource.remove_prefix(1);
        return rSource;
    }

    if (rSource.size() > 3 && rSource.front() == u'{' && rSource[1] == u'=' && rSource.back() == u'}')
    {
        rSource.remove_prefix(2);
        rSource.remove_suffix(1);
        return rSource;
    }

    return rSource;
}

[[nodiscard]] inline api::String normalizeFormulaSource(std::u16string_view rSource)
{
    if (rSource.rfind(u"of:=", 0) == 0)
        return api::String(rSource);

    const std::u16string_view aTrimmed = trimFormulaEquals(rSource);
    api::String aNormalized(u"of:=");
    aNormalized.append(aTrimmed.begin(), aTrimmed.end());
    return aNormalized;
}

[[nodiscard]] inline bool isSignedNumericLiteral(const core::formula::Node& rNode)
{
    if (rNode.meKind != core::formula::NodeKind::UnaryOperation || rNode.maChildren.size() != 1)
        return false;
    if (rNode.maPrimaryText != u"+" && rNode.maPrimaryText != u"-")
        return false;
    return rNode.maChildren[0]
           && rNode.maChildren[0]->meKind == core::formula::NodeKind::NumberLiteral;
}

[[nodiscard]] inline std::optional<double> extractNumericLiteral(
    const core::formula::Node& rNode)
{
    if (rNode.meKind == core::formula::NodeKind::NumberLiteral)
        return rNode.mfNumber;

    if (!isSignedNumericLiteral(rNode))
        return std::nullopt;

    const double fValue = rNode.maChildren[0]->mfNumber;
    return rNode.maPrimaryText == u"-" ? -fValue : fValue;
}

[[nodiscard]] inline std::optional<OUString> extractStringLiteral(
    const core::formula::Node& rNode)
{
    if (rNode.meKind == core::formula::NodeKind::StringLiteral)
        return toLibreOfficeString(api::String(rNode.maPrimaryText));

    if (rNode.meKind == core::formula::NodeKind::EmptyArgument)
        return OUString();

    return std::nullopt;
}

[[nodiscard]] inline api::formulavalue::FormulaResultValue makeErrorResultFromApi(
    api::Error eError)
{
    return api::formulavalue::makeErrorResult(eError);
}

[[nodiscard]] inline EvaluationAttempt makeUnsupported(FallbackReason eReason)
{
    EvaluationAttempt aAttempt;
    aAttempt.meFallbackReason = eReason;
    return aAttempt;
}

[[nodiscard]] inline EvaluationAttempt makeNumericResult(double fValue, SvNumFormatType eFormatType)
{
    EvaluationAttempt aAttempt;
    aAttempt.mbSupported = true;
    aAttempt.maResult = api::formulavalue::makeValueResult(fValue);
    aAttempt.meFormatType = eFormatType;
    return aAttempt;
}

[[nodiscard]] inline EvaluationAttempt makeErrorResult(api::Error eError)
{
    EvaluationAttempt aAttempt;
    aAttempt.mbSupported = true;
    aAttempt.maResult = makeErrorResultFromApi(eError);
    return aAttempt;
}

[[nodiscard]] inline EvaluationAttempt evaluateTextParsingFunction(
    const core::formula::Node& rNode, api::StringView rFunctionName, const ScDocument& rDoc,
    ScInterpreterContext& rContext, bool bEmptyStringAsZero)
{
    if (rFunctionName == u"VALUE")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(api::Error::IllegalArgument);

        if (const auto oNumeric = extractNumericLiteral(*rNode.maChildren[0]))
            return makeNumericResult(*oNumeric, SvNumFormatType::NUMBER);

        const auto oText = extractStringLiteral(*rNode.maChildren[0]);
        if (!oText)
            return makeUnsupported(FallbackReason::UnsupportedFormulaShape);

        const auto aResult = textparsingexecution::evaluateValue(rDoc, rContext, *oText);
        if (!aResult)
            return makeErrorResult(aResult.meError);
        return makeNumericResult(aResult.maValue, SvNumFormatType::NUMBER);
    }

    if (rFunctionName == u"DATEVALUE")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(api::Error::IllegalArgument);

        const auto oText = extractStringLiteral(*rNode.maChildren[0]);
        if (!oText)
            return makeUnsupported(FallbackReason::UnsupportedFormulaShape);

        const auto aResult = textparsingexecution::evaluateDateValue(rDoc, rContext, *oText);
        if (!aResult)
            return makeErrorResult(aResult.meError);
        return makeNumericResult(aResult.maValue, SvNumFormatType::DATE);
    }

    if (rFunctionName == u"TIMEVALUE")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(api::Error::IllegalArgument);

        const auto oText = extractStringLiteral(*rNode.maChildren[0]);
        if (!oText)
            return makeUnsupported(FallbackReason::UnsupportedFormulaShape);

        const auto aResult = textparsingexecution::evaluateTimeValue(rDoc, rContext, *oText);
        if (!aResult)
            return makeErrorResult(aResult.meError);
        return makeNumericResult(aResult.maValue, SvNumFormatType::TIME);
    }

    if (rFunctionName == u"NUMBERVALUE")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeErrorResult(api::Error::IllegalArgument);

        const auto oText = extractStringLiteral(*rNode.maChildren[0]);
        if (!oText)
            return makeUnsupported(FallbackReason::UnsupportedFormulaShape);

        std::optional<OUString> oDecimalSeparator;
        std::optional<OUString> oGroupSeparator;
        if (rNode.maChildren.size() >= 2)
        {
            oDecimalSeparator = extractStringLiteral(*rNode.maChildren[1]);
            if (!oDecimalSeparator)
                return makeUnsupported(FallbackReason::UnsupportedFormulaShape);
        }
        if (rNode.maChildren.size() == 3)
        {
            oGroupSeparator = extractStringLiteral(*rNode.maChildren[2]);
            if (!oGroupSeparator)
                return makeUnsupported(FallbackReason::UnsupportedFormulaShape);
        }

        const auto aResult = textparsingexecution::evaluateNumberValue(
            rDoc, rContext, *oText, oDecimalSeparator, oGroupSeparator, bEmptyStringAsZero);
        if (!aResult)
            return makeErrorResult(aResult.meError);
        return makeNumericResult(aResult.maValue, SvNumFormatType::NUMBER);
    }

    return makeUnsupported(FallbackReason::UnsupportedFunction);
}

} // namespace detail

[[nodiscard]] inline RolloutMode resolveRolloutMode()
{
    const char* pValue = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR");
    if (!pValue || !*pValue)
        return RolloutMode::Off;

    const std::string_view aValue(pValue);
    if (aValue == "observe")
        return RolloutMode::Observe;
    if (aValue == "shadow" || aValue == "shadowcompare")
        return RolloutMode::ShadowCompare;
    if (aValue == "authority" || aValue == "authoritative"
        || aValue == "authoritative_with_fallback")
    {
        return RolloutMode::AuthoritativeWithFallback;
    }

    return detail::envEnabled(aValue) ? RolloutMode::ShadowCompare : RolloutMode::Off;
}

[[nodiscard]] inline EvaluationAttempt tryEvaluateFormula(
    const ScDocument& rDoc, ScInterpreterContext& rContext, std::u16string_view rFormulaSource,
    bool bEmptyStringAsZero)
{
    const api::String aNormalized = detail::normalizeFormulaSource(rFormulaSource);
    const auto aParse = core::formula::parseFormula(aNormalized);
    if (!aParse || !aParse.mpRoot)
        return detail::makeUnsupported(FallbackReason::ParseFailure);

    const auto& rRoot = *aParse.mpRoot;
    if (rRoot.meKind != core::formula::NodeKind::FunctionCall)
        return detail::makeUnsupported(FallbackReason::UnsupportedFormulaShape);

    const api::String aFunctionName = detail::uppercaseAscii(rRoot.maPrimaryText);
    return detail::evaluateTextParsingFunction(
        rRoot, aFunctionName, rDoc, rContext, bEmptyStringAsZero);
}

inline void resetStats()
{
    auto& rStore = detail::statsStore();
    rStore.mnObserveCount.store(0);
    rStore.mnShadowCompareCount.store(0);
    rStore.mnAuthoritativeCount.store(0);
    rStore.mnAuthoritativeFallbackCount.store(0);
    rStore.mnShadowMatchCount.store(0);
    for (auto& rValue : rStore.maFallbackReasons)
        rValue.store(0);
    for (auto& rValue : rStore.maMismatchReasons)
        rValue.store(0);
}

[[nodiscard]] inline StatsSnapshot getStatsSnapshot()
{
    StatsSnapshot aSnapshot;
    auto& rStore = detail::statsStore();
    aSnapshot.mnObserveCount = rStore.mnObserveCount.load();
    aSnapshot.mnShadowCompareCount = rStore.mnShadowCompareCount.load();
    aSnapshot.mnAuthoritativeCount = rStore.mnAuthoritativeCount.load();
    aSnapshot.mnAuthoritativeFallbackCount = rStore.mnAuthoritativeFallbackCount.load();
    aSnapshot.mnShadowMatchCount = rStore.mnShadowMatchCount.load();
    for (std::size_t i = 0; i < aSnapshot.maFallbackReasons.size(); ++i)
        aSnapshot.maFallbackReasons[i] = rStore.maFallbackReasons[i].load();
    for (std::size_t i = 0; i < aSnapshot.maMismatchReasons.size(); ++i)
        aSnapshot.maMismatchReasons[i] = rStore.maMismatchReasons[i].load();
    return aSnapshot;
}

inline void recordObserveSupport()
{
    detail::statsStore().mnObserveCount.fetch_add(1);
}

inline void recordShadowCompareSupport()
{
    detail::statsStore().mnShadowCompareCount.fetch_add(1);
}

inline void recordAuthoritativeRoute()
{
    detail::statsStore().mnAuthoritativeCount.fetch_add(1);
}

inline void recordAuthoritativeFallback(FallbackReason eReason)
{
    auto& rStore = detail::statsStore();
    rStore.mnAuthoritativeFallbackCount.fetch_add(1);
    rStore.maFallbackReasons[detail::toIndex(eReason)].fetch_add(1);
}

inline void recordFallback(FallbackReason eReason)
{
    detail::statsStore().maFallbackReasons[detail::toIndex(eReason)].fetch_add(1);
}

inline void recordShadowMatch()
{
    detail::statsStore().mnShadowMatchCount.fetch_add(1);
}

inline void recordMismatch(MismatchReason eReason)
{
    detail::statsStore().maMismatchReasons[detail::toIndex(eReason)].fetch_add(1);
}

} // namespace spreadsheetengine::compat::libreoffice::interprettaileval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
