/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <address.hxx>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <document.hxx>
#include <docoptio.hxx>
#include <formula/grammar.hxx>
#include <interpretercontext.hxx>
#include <rangeutl.hxx>
#include <svl/numformat.hxx>

#include <spreadsheetengine/api/FormulaResult.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/LookupExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx>
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
    UnsupportedHostSurface,
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
    ResultType,
    StringValue,
    Count
};

enum class FunctionKind : sal_uInt8
{
    Unknown,
    Value,
    DateValue,
    TimeValue,
    NumberValue,
    Match,
    XMatch,
    Lookup,
    VLookup,
    HLookup,
    XLookup,
    Index,
    Count
};

struct EvaluationAttempt
{
    bool mbSupported = false;
    api::formulavalue::FormulaResultValue maResult
        = api::formulavalue::makeInvalidResult();
    SvNumFormatType meFormatType = SvNumFormatType::ALL;
    FallbackReason meFallbackReason = FallbackReason::UnsupportedFormulaShape;
    FunctionKind meFunction = FunctionKind::Unknown;
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
    std::array<sal_uInt64, static_cast<std::size_t>(FunctionKind::Count)> maFunctionObserveCount {};
    std::array<sal_uInt64, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionShadowCompareCount {};
    std::array<sal_uInt64, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionAuthoritativeCount {};
    std::array<sal_uInt64, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionFallbackCount {};
    std::array<std::array<sal_uInt64, static_cast<std::size_t>(FallbackReason::Count)>,
        static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionFallbackReasons {};
};

struct DiagnosticSample
{
    FallbackReason meReason = FallbackReason::UnsupportedFormulaShape;
    OUString maWorkbookLabel;
    OUString maCellAddress;
    OUString maRawFormula;
    OUString maNormalizedFormula;
    OUString maRootKind;
};

namespace detail
{

template <typename T> struct Materialization
{
    bool mbSupported = false;
    std::optional<T> moValue;
    api::Error meError = api::Error::None;
    FallbackReason meFallbackReason = FallbackReason::UnsupportedFormulaShape;
};

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
    std::array<std::atomic<sal_uInt64>, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionObserveCount {};
    std::array<std::atomic<sal_uInt64>, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionShadowCompareCount {};
    std::array<std::atomic<sal_uInt64>, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionAuthoritativeCount {};
    std::array<std::atomic<sal_uInt64>, static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionFallbackCount {};
    std::array<std::array<std::atomic<sal_uInt64>, static_cast<std::size_t>(FallbackReason::Count)>,
        static_cast<std::size_t>(FunctionKind::Count)>
        maFunctionFallbackReasons {};
};

struct DiagnosticStore
{
    std::mutex maMutex;
    std::vector<DiagnosticSample> maSamples;
    OUString maWorkbookLabel;
};

[[nodiscard]] inline StatsStore& statsStore()
{
    static StatsStore aStore;
    return aStore;
}

[[nodiscard]] inline DiagnosticStore& diagnosticStore()
{
    static DiagnosticStore aStore;
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

[[nodiscard]] constexpr std::size_t toIndex(FunctionKind eFunction)
{
    return static_cast<std::size_t>(eFunction);
}

[[nodiscard]] inline bool envEnabled(std::string_view rValue)
{
    return !rValue.empty() && rValue != "0" && rValue != "off" && rValue != "false";
}

[[nodiscard]] inline bool diagnosticsEnabled()
{
    if (const char* pValue = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_DIAGNOSTICS"))
        return envEnabled(pValue);
    return false;
}

[[nodiscard]] inline std::size_t diagnosticSampleLimit()
{
    if (const char* pValue = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_DIAGNOSTIC_LIMIT"))
    {
        try
        {
            const int nLimit = std::stoi(pValue);
            if (nLimit > 0)
                return static_cast<std::size_t>(nLimit);
        }
        catch (...)
        {
        }
    }

    return 24;
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

[[nodiscard]] inline bool isAsciiNamespaceChar(char16_t c)
{
    return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z')
           || (c >= u'0' && c <= u'9') || c == u'.' || c == u'_' || c == u'-';
}

[[nodiscard]] inline std::size_t leadingNamespacePrefixLength(std::u16string_view rSource)
{
    if (rSource.empty())
        return 0;

    if (!((rSource.front() >= u'a' && rSource.front() <= u'z')
          || (rSource.front() >= u'A' && rSource.front() <= u'Z')))
    {
        return 0;
    }

    for (std::size_t i = 1; i < rSource.size(); ++i)
    {
        const char16_t c = rSource[i];
        if (c == u':')
            return i + 1;
        if (!isAsciiNamespaceChar(c))
            return 0;
    }

    return 0;
}

[[nodiscard]] inline OUString normalizeReferenceToken(api::StringView rValue)
{
    OUString aText = toLibreOfficeString(api::String(rValue));
    if (!aText.isEmpty() && aText[0] == '.')
        aText = aText.copy(1);
    return aText;
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
    if (rSource.rfind(u"of:", 0) == 0)
        return api::String(rSource);

    const std::u16string_view aTrimmed = trimFormulaEquals(rSource);
    if (aTrimmed.rfind(u"of:=", 0) == 0 || aTrimmed.rfind(u"of:", 0) == 0)
        return api::String(aTrimmed);

    if (const std::size_t nPrefixLen = leadingNamespacePrefixLength(aTrimmed))
    {
        api::String aCanonical(u"of:");
        aCanonical.append(aTrimmed.begin() + static_cast<std::ptrdiff_t>(nPrefixLen),
            aTrimmed.end());
        return aCanonical;
    }

    api::String aNormalized(u"of:=");
    aNormalized.append(aTrimmed.begin(), aTrimmed.end());
    return aNormalized;
}

[[nodiscard]] inline FunctionKind classifyFunction(api::StringView rFunctionName)
{
    if (rFunctionName == u"VALUE")
        return FunctionKind::Value;
    if (rFunctionName == u"DATEVALUE")
        return FunctionKind::DateValue;
    if (rFunctionName == u"TIMEVALUE")
        return FunctionKind::TimeValue;
    if (rFunctionName == u"NUMBERVALUE")
        return FunctionKind::NumberValue;
    if (rFunctionName == u"MATCH")
        return FunctionKind::Match;
    if (rFunctionName == u"XMATCH" || rFunctionName == u"COM.MICROSOFT.XMATCH")
        return FunctionKind::XMatch;
    if (rFunctionName == u"LOOKUP")
        return FunctionKind::Lookup;
    if (rFunctionName == u"VLOOKUP")
        return FunctionKind::VLookup;
    if (rFunctionName == u"HLOOKUP")
        return FunctionKind::HLookup;
    if (rFunctionName == u"XLOOKUP" || rFunctionName == u"COM.MICROSOFT.XLOOKUP")
        return FunctionKind::XLookup;
    if (rFunctionName == u"INDEX")
        return FunctionKind::Index;
    return FunctionKind::Unknown;
}

[[nodiscard]] inline api::query::SearchType searchTypeFromDocument(const ScDocument& rDoc)
{
    const ScDocOptions& rOptions = rDoc.GetDocOptions();
    if (rOptions.IsFormulaRegexEnabled())
        return api::query::SearchType::Regex;
    if (rOptions.IsFormulaWildcardsEnabled())
        return api::query::SearchType::Wildcard;
    return api::query::SearchType::Normal;
}

[[nodiscard]] inline OUString rootKindName(core::formula::NodeKind eKind)
{
    switch (eKind)
    {
        case core::formula::NodeKind::FunctionCall:
            return u"FunctionCall"_ustr;
        case core::formula::NodeKind::NumberLiteral:
            return u"NumberLiteral"_ustr;
        case core::formula::NodeKind::StringLiteral:
            return u"StringLiteral"_ustr;
        case core::formula::NodeKind::BooleanLiteral:
            return u"BooleanLiteral"_ustr;
        case core::formula::NodeKind::ErrorLiteral:
            return u"ErrorLiteral"_ustr;
        case core::formula::NodeKind::CellReference:
            return u"CellReference"_ustr;
        case core::formula::NodeKind::RangeReference:
            return u"RangeReference"_ustr;
        case core::formula::NodeKind::NamedReference:
            return u"NamedReference"_ustr;
        case core::formula::NodeKind::RangeConstructor:
            return u"RangeConstructor"_ustr;
        case core::formula::NodeKind::ReferenceList:
            return u"ReferenceList"_ustr;
        case core::formula::NodeKind::ArrayConstant:
            return u"ArrayConstant"_ustr;
        case core::formula::NodeKind::UnaryOperation:
            return u"UnaryOperation"_ustr;
        case core::formula::NodeKind::BinaryOperation:
            return u"BinaryOperation"_ustr;
        case core::formula::NodeKind::EmptyArgument:
            return u"EmptyArgument"_ustr;
    }

    return u"Unknown"_ustr;
}

inline void recordDiagnosticSample(FallbackReason eReason, const ScDocument& rDoc,
    const ScAddress& rFormulaPos, std::u16string_view rRawFormula,
    std::u16string_view rNormalizedFormula, std::optional<core::formula::NodeKind> oRootKind)
{
    if (!diagnosticsEnabled())
        return;

    auto& rStore = diagnosticStore();
    std::scoped_lock aGuard(rStore.maMutex);
    if (rStore.maSamples.size() >= diagnosticSampleLimit())
        return;

    DiagnosticSample aSample;
    aSample.meReason = eReason;
    aSample.maWorkbookLabel = rStore.maWorkbookLabel;
    aSample.maCellAddress = rFormulaPos.Format(ScRefFlags::ADDR_ABS_3D, &rDoc, rDoc.GetAddressConvention());
    aSample.maRawFormula = toLibreOfficeString(api::String(rRawFormula));
    aSample.maNormalizedFormula = toLibreOfficeString(api::String(rNormalizedFormula));
    if (oRootKind)
        aSample.maRootKind = rootKindName(*oRootKind);
    rStore.maSamples.push_back(std::move(aSample));
}

template <typename T>
[[nodiscard]] inline Materialization<T> makeUnsupportedMaterialization(FallbackReason eReason)
{
    Materialization<T> aResult;
    aResult.meFallbackReason = eReason;
    return aResult;
}

template <typename T>
[[nodiscard]] inline Materialization<T> makeMaterializedValue(T aValue)
{
    Materialization<T> aResult;
    aResult.mbSupported = true;
    aResult.moValue = std::move(aValue);
    return aResult;
}

template <typename T>
[[nodiscard]] inline Materialization<T> makeMaterializedError(api::Error eError)
{
    Materialization<T> aResult;
    aResult.mbSupported = true;
    aResult.meError = eError;
    return aResult;
}

[[nodiscard]] inline api::formulavalue::FormulaResultValue makeErrorResultFromApi(api::Error eError)
{
    return api::formulavalue::makeErrorResult(eError);
}

[[nodiscard]] inline EvaluationAttempt makeUnsupported(
    FunctionKind eFunction, FallbackReason eReason)
{
    EvaluationAttempt aAttempt;
    aAttempt.meFunction = eFunction;
    aAttempt.meFallbackReason = eReason;
    return aAttempt;
}

[[nodiscard]] inline EvaluationAttempt makeNumericResult(
    FunctionKind eFunction, double fValue, SvNumFormatType eFormatType)
{
    EvaluationAttempt aAttempt;
    aAttempt.mbSupported = true;
    aAttempt.meFunction = eFunction;
    aAttempt.maResult = api::formulavalue::makeValueResult(fValue);
    aAttempt.meFormatType = eFormatType;
    return aAttempt;
}

[[nodiscard]] inline EvaluationAttempt makeStringResult(
    FunctionKind eFunction, const OUString& rValue, bool bMultiLine = false)
{
    EvaluationAttempt aAttempt;
    aAttempt.mbSupported = true;
    aAttempt.meFunction = eFunction;
    aAttempt.maResult
        = api::formulavalue::makeStringResult(toApiString(rValue), bMultiLine);
    return aAttempt;
}

[[nodiscard]] inline EvaluationAttempt makeErrorResult(FunctionKind eFunction, api::Error eError)
{
    EvaluationAttempt aAttempt;
    aAttempt.mbSupported = true;
    aAttempt.meFunction = eFunction;
    aAttempt.maResult = makeErrorResultFromApi(eError);
    return aAttempt;
}

[[nodiscard]] inline EvaluationAttempt makeScalarAttempt(
    FunctionKind eFunction, const api::CellValue& rValue)
{
    if (rValue.isError())
        return makeErrorResult(eFunction, rValue.meError);
    if (rValue.isText())
        return makeStringResult(eFunction, toLibreOfficeString(rValue.maString));
    if (rValue.isNumber() || rValue.isBoolean())
        return makeNumericResult(eFunction, rValue.mfNumber, SvNumFormatType::NUMBER);
    return makeNumericResult(eFunction, 0.0, SvNumFormatType::NUMBER);
}

[[nodiscard]] inline FunctionKind classifyDelegatedFunctionNode(
    const core::formula::Node& rNode);

[[nodiscard]] inline EvaluationAttempt evaluateDelegatedNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos, bool bEmptyStringAsZero, std::size_t nDepth = 0);

[[nodiscard]] inline EvaluationAttempt evaluateScalarOrDelegatedNode(
    const core::formula::Node& rNode, FunctionKind ePreferredFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    std::size_t nDepth = 0);

[[nodiscard]] inline std::optional<double> extractNumericLiteral(
    const core::formula::Node& rNode)
{
    if (rNode.meKind == core::formula::NodeKind::NumberLiteral)
        return rNode.mfNumber;

    if (rNode.meKind != core::formula::NodeKind::UnaryOperation || rNode.maChildren.size() != 1
        || rNode.maChildren[0]->meKind != core::formula::NodeKind::NumberLiteral)
    {
        return std::nullopt;
    }

    const double fValue = rNode.maChildren[0]->mfNumber;
    return rNode.meUnaryOperator == core::formula::UnaryOperator::Minus ? -fValue : fValue;
}

[[nodiscard]] inline OUString formatScalarNumber(
    const ScDocument& rDoc, ScInterpreterContext& rContext, double fValue)
{
    DocumentEvaluationHost aHost(rDoc, rContext);
    const auto aFormatted = aHost.formatNumber(fValue);
    return aFormatted ? toLibreOfficeString(aFormatted.maValue) : OUString::number(fValue);
}

[[nodiscard]] inline api::ValueResult<OUString> coerceScalarToText(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const api::CellValue& rValue)
{
    if (rValue.isError())
        return api::ValueResult<OUString>::failure(rValue.meError);

    if (rValue.isEmpty())
        return api::ValueResult<OUString>::success(OUString());

    if (rValue.isText())
    {
        return api::ValueResult<OUString>::success(
            toLibreOfficeString(rValue.maString));
    }

    if (rValue.isBoolean())
    {
        return api::ValueResult<OUString>::success(
            rValue.mfNumber != 0.0 ? u"TRUE"_ustr : u"FALSE"_ustr);
    }

    return api::ValueResult<OUString>::success(formatScalarNumber(rDoc, rContext, rValue.mfNumber));
}

[[nodiscard]] inline bool isAsciiDigit(char16_t c)
{
    return c >= u'0' && c <= u'9';
}

[[nodiscard]] inline std::optional<std::int16_t> parseAsciiFixedInt(
    std::u16string_view rValue)
{
    if (rValue.empty())
        return std::nullopt;

    sal_Int32 nValue = 0;
    for (const char16_t c : rValue)
    {
        if (!isAsciiDigit(c))
            return std::nullopt;
        nValue = nValue * 10 + (c - u'0');
        if (nValue > std::numeric_limits<std::int16_t>::max())
            return std::nullopt;
    }

    return static_cast<std::int16_t>(nValue);
}

[[nodiscard]] inline std::optional<double> tryIsoDateValueFallback(
    const ScDocument& rDoc, const OUString& rInput)
{
    const std::u16string_view aInput = rInput;
    if (aInput.size() != 10 && aInput.size() != 19)
        return std::nullopt;
    if (aInput[4] != u'-' || aInput[7] != u'-')
        return std::nullopt;

    const auto oYear = parseAsciiFixedInt(aInput.substr(0, 4));
    const auto oMonth = parseAsciiFixedInt(aInput.substr(5, 2));
    const auto oDay = parseAsciiFixedInt(aInput.substr(8, 2));
    if (!oYear || !oMonth || !oDay)
        return std::nullopt;

    if (aInput.size() == 19)
    {
        if ((aInput[10] != u' ' && aInput[10] != u'T') || aInput[13] != u':' || aInput[16] != u':'
            || !parseAsciiFixedInt(aInput.substr(11, 2))
            || !parseAsciiFixedInt(aInput.substr(14, 2))
            || !parseAsciiFixedInt(aInput.substr(17, 2)))
        {
            return std::nullopt;
        }
    }

    const auto aSerial = spreadsheetengine::api::calendar::makeDateSerial(
        toApiDateParts(rDoc.GetFormatTable()->GetNullDate()), *oYear, *oMonth, *oDay, true);
    if (!aSerial)
        return std::nullopt;

    return std::trunc(aSerial.maValue);
}

[[nodiscard]] inline api::ValueResult<double> coerceScalarToNumber(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const api::CellValue& rValue)
{
    if (rValue.isError())
        return api::ValueResult<double>::failure(rValue.meError);

    if (rValue.isEmpty())
        return api::ValueResult<double>::success(0.0);

    if (rValue.isNumber() || rValue.isBoolean())
        return api::ValueResult<double>::success(rValue.mfNumber);

    if (!rValue.isText())
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    DocumentEvaluationHost aHost(rDoc, rContext);
    const auto aParsed = aHost.parseNumber(rValue.maString);
    if (!aParsed)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(aParsed.maValue.mfValue);
}

[[nodiscard]] inline api::ValueResult<bool> coerceScalarToBool(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const api::CellValue& rValue)
{
    if (rValue.isError())
        return api::ValueResult<bool>::failure(rValue.meError);

    if (rValue.isEmpty())
        return api::ValueResult<bool>::success(false);

    if (rValue.isBoolean() || rValue.isNumber())
        return api::ValueResult<bool>::success(rValue.mfNumber != 0.0);

    if (!rValue.isText())
        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

    DocumentEvaluationHost aHost(rDoc, rContext);
    const auto aParsed = aHost.parseNumber(rValue.maString);
    if (!aParsed)
        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
    return api::ValueResult<bool>::success(aParsed.maValue.mfValue != 0.0);
}

[[nodiscard]] inline std::optional<sal_Int32> coerceWholeNumber(double fValue)
{
    if (!std::isfinite(fValue) || std::trunc(fValue) != fValue
        || fValue < static_cast<double>(std::numeric_limits<sal_Int32>::min())
        || fValue > static_cast<double>(std::numeric_limits<sal_Int32>::max()))
    {
        return std::nullopt;
    }
    return static_cast<sal_Int32>(fValue);
}

[[nodiscard]] inline Materialization<ScRange> resolveReferenceRangeNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, const ScAddress& rFormulaPos)
{
    switch (rNode.meKind)
    {
        case core::formula::NodeKind::CellReference:
        {
            ScRefAddress aReference;
            ScAddress::ExternalInfo aExternalInfo;
            const OUString aText = normalizeReferenceToken(rNode.maPrimaryText);
            const ScAddress::Details aDetails(formula::FormulaGrammar::CONV_OOO, rFormulaPos);
            if (!ConvertSingleRef(rDoc, aText, rFormulaPos.Tab(), aReference, aDetails,
                    &aExternalInfo)
                || aExternalInfo.mbExternal)
            {
                return makeUnsupportedMaterialization<ScRange>(
                    FallbackReason::UnsupportedHostSurface);
            }

            const ScAddress aAddress = aReference.GetAddress();
            return makeMaterializedValue(ScRange(aAddress, aAddress));
        }
        case core::formula::NodeKind::RangeReference:
        {
            ScRefAddress aStart;
            ScRefAddress aEnd;
            ScAddress::ExternalInfo aExternalInfo;
            OUString aText = normalizeReferenceToken(rNode.maPrimaryText);
            aText += u":"_ustr;
            aText += normalizeReferenceToken(rNode.maSecondaryText);
            const ScAddress::Details aDetails(formula::FormulaGrammar::CONV_OOO, rFormulaPos);
            if (!ConvertDoubleRef(rDoc, aText, rFormulaPos.Tab(), aStart, aEnd, aDetails,
                    &aExternalInfo)
                || aExternalInfo.mbExternal)
            {
                return makeUnsupportedMaterialization<ScRange>(
                    FallbackReason::UnsupportedHostSurface);
            }

            return makeMaterializedValue(ScRange(aStart.GetAddress(), aEnd.GetAddress()));
        }
        case core::formula::NodeKind::NamedReference:
        {
            ScRangeData* pRangeData = ScRangeStringConverter::GetRangeDataFromString(
                toLibreOfficeString(rNode.maPrimaryText), rFormulaPos.Tab(), rDoc,
                formula::FormulaGrammar::CONV_OOO);
            if (!pRangeData)
            {
                return makeUnsupportedMaterialization<ScRange>(
                    FallbackReason::UnsupportedHostSurface);
            }

            ScRange aRange;
            if (!pRangeData->IsReference(aRange, rFormulaPos))
            {
                return makeUnsupportedMaterialization<ScRange>(
                    FallbackReason::UnsupportedHostSurface);
            }
            return makeMaterializedValue(aRange);
        }
        default:
            return makeUnsupportedMaterialization<ScRange>(
                FallbackReason::UnsupportedFormulaShape);
    }
}

[[nodiscard]] inline std::optional<double> referencedDateTimeSerial(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.meKind != core::formula::NodeKind::CellReference
        && rNode.meKind != core::formula::NodeKind::NamedReference)
    {
        return std::nullopt;
    }

    const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
    if (!aRange.mbSupported || !aRange.moValue || aRange.moValue->aStart != aRange.moValue->aEnd
        || aRange.moValue->aStart == rFormulaPos)
    {
        return std::nullopt;
    }

    const ScAddress aAddress = aRange.moValue->aStart;
    ScRefCellValue aCell(const_cast<ScDocument&>(rDoc), aAddress);
    if (!aCell.hasNumeric())
        return std::nullopt;

    const sal_uInt32 nFormat = rDoc.GetNumberFormat(rContext, aAddress);
    const SvNumFormatType eFormatType
        = rContext.GetFormatTable()->GetType(nFormat) & ~SvNumFormatType::DEFINED;
    if (eFormatType != SvNumFormatType::DATE && eFormatType != SvNumFormatType::TIME
        && eFormatType != SvNumFormatType::DATETIME)
    {
        return std::nullopt;
    }

    return aCell.getRawValue();
}

inline void putScalarIntoMatrix(
    const api::CellValue& rValue, const ScMatrixRef& pMatrix, SCSIZE nColumn, SCSIZE nRow)
{
    if (rValue.isError())
        pMatrix->PutError(toFormulaError(rValue.meError), nColumn, nRow);
    else if (rValue.isText())
        pMatrix->PutString(svl::SharedString(toLibreOfficeString(rValue.maString)), nColumn, nRow);
    else if (rValue.isBoolean())
        pMatrix->PutBoolean(rValue.mfNumber != 0.0, nColumn, nRow);
    else if (rValue.isNumber())
        pMatrix->PutDouble(rValue.mfNumber, nColumn, nRow);
    else
        pMatrix->PutEmpty(nColumn, nRow);
}

[[nodiscard]] inline Materialization<api::CellValue> materializeScalarNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    switch (rNode.meKind)
    {
        case core::formula::NodeKind::NumberLiteral:
            return makeMaterializedValue(api::CellValue::number(rNode.mfNumber));
        case core::formula::NodeKind::StringLiteral:
            return makeMaterializedValue(api::CellValue::text(rNode.maPrimaryText));
        case core::formula::NodeKind::BooleanLiteral:
            return makeMaterializedValue(api::CellValue::boolean(rNode.mbBoolean));
        case core::formula::NodeKind::ErrorLiteral:
            return makeMaterializedValue(api::CellValue::error(api::Error::IllegalArgument));
        case core::formula::NodeKind::EmptyArgument:
            return makeMaterializedValue(api::CellValue::empty());
        case core::formula::NodeKind::CellReference:
        case core::formula::NodeKind::NamedReference:
        {
            const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
            if (!aRange.mbSupported)
                return makeUnsupportedMaterialization<api::CellValue>(aRange.meFallbackReason);
            if (!aRange.moValue)
                return makeMaterializedError<api::CellValue>(aRange.meError);
            if (aRange.moValue->aStart != aRange.moValue->aEnd
                || aRange.moValue->aStart == rFormulaPos)
            {
                return makeUnsupportedMaterialization<api::CellValue>(
                    FallbackReason::UnsupportedHostSurface);
            }
            return makeMaterializedValue(
                readHostDocumentCellValue(rDoc, aRange.moValue->aStart).maValue);
        }
        case core::formula::NodeKind::UnaryOperation:
        {
            if (rNode.maChildren.size() != 1)
                return makeUnsupportedMaterialization<api::CellValue>(
                    FallbackReason::UnsupportedFormulaShape);

            const auto aChild
                = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
            if (!aChild.mbSupported)
                return makeUnsupportedMaterialization<api::CellValue>(aChild.meFallbackReason);
            if (!aChild.moValue)
                return makeMaterializedError<api::CellValue>(aChild.meError);

            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aChild.moValue);
            if (!aNumber)
                return makeMaterializedError<api::CellValue>(aNumber.meError);

            return makeMaterializedValue(api::CellValue::number(
                rNode.meUnaryOperator == core::formula::UnaryOperator::Minus
                    ? -aNumber.maValue
                    : aNumber.maValue));
        }
        case core::formula::NodeKind::BinaryOperation:
        {
            if (rNode.maChildren.size() != 2)
                return makeUnsupportedMaterialization<api::CellValue>(
                    FallbackReason::UnsupportedFormulaShape);

            const auto aLeft
                = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
            if (!aLeft.mbSupported)
                return makeUnsupportedMaterialization<api::CellValue>(aLeft.meFallbackReason);
            if (!aLeft.moValue)
                return makeMaterializedError<api::CellValue>(aLeft.meError);

            const auto aRight
                = materializeScalarNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
            if (!aRight.mbSupported)
                return makeUnsupportedMaterialization<api::CellValue>(aRight.meFallbackReason);
            if (!aRight.moValue)
                return makeMaterializedError<api::CellValue>(aRight.meError);

            switch (rNode.meBinaryOperator)
            {
                case core::formula::BinaryOperator::Concat:
                {
                    const auto aLeftText = coerceScalarToText(rDoc, rContext, *aLeft.moValue);
                    if (!aLeftText)
                        return makeMaterializedError<api::CellValue>(aLeftText.meError);
                    const auto aRightText = coerceScalarToText(rDoc, rContext, *aRight.moValue);
                    if (!aRightText)
                        return makeMaterializedError<api::CellValue>(aRightText.meError);
                    return makeMaterializedValue(
                        api::CellValue::text(toApiString(aLeftText.maValue + aRightText.maValue)));
                }
                case core::formula::BinaryOperator::Add:
                case core::formula::BinaryOperator::Subtract:
                case core::formula::BinaryOperator::Multiply:
                case core::formula::BinaryOperator::Divide:
                case core::formula::BinaryOperator::Power:
                {
                    const auto aLeftNumber = coerceScalarToNumber(rDoc, rContext, *aLeft.moValue);
                    if (!aLeftNumber)
                        return makeMaterializedError<api::CellValue>(aLeftNumber.meError);
                    const auto aRightNumber = coerceScalarToNumber(rDoc, rContext, *aRight.moValue);
                    if (!aRightNumber)
                        return makeMaterializedError<api::CellValue>(aRightNumber.meError);

                    double fResult = 0.0;
                    switch (rNode.meBinaryOperator)
                    {
                        case core::formula::BinaryOperator::Add:
                            fResult = aLeftNumber.maValue + aRightNumber.maValue;
                            break;
                        case core::formula::BinaryOperator::Subtract:
                            fResult = aLeftNumber.maValue - aRightNumber.maValue;
                            break;
                        case core::formula::BinaryOperator::Multiply:
                            fResult = aLeftNumber.maValue * aRightNumber.maValue;
                            break;
                        case core::formula::BinaryOperator::Divide:
                            if (aRightNumber.maValue == 0.0)
                                return makeMaterializedError<api::CellValue>(
                                    api::Error::DivisionByZero);
                            fResult = aLeftNumber.maValue / aRightNumber.maValue;
                            break;
                        case core::formula::BinaryOperator::Power:
                            fResult = std::pow(aLeftNumber.maValue, aRightNumber.maValue);
                            if (!std::isfinite(fResult))
                                return makeMaterializedError<api::CellValue>(api::Error::Domain);
                            break;
                        default:
                            break;
                    }
                    return makeMaterializedValue(api::CellValue::number(fResult));
                }
                default:
                    return makeUnsupportedMaterialization<api::CellValue>(
                        FallbackReason::UnsupportedFormulaShape);
            }
        }
        default:
            return makeUnsupportedMaterialization<api::CellValue>(
                FallbackReason::UnsupportedFormulaShape);
    }
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeMatrixNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.meKind == core::formula::NodeKind::ArrayConstant)
    {
        if (rNode.mnArrayColumns < 1 || rNode.mnArrayRows < 1
            || static_cast<sal_Int32>(rNode.maChildren.size())
                   != rNode.mnArrayColumns * rNode.mnArrayRows)
        {
            return makeMaterializedError<ScMatrixRef>(api::Error::IllegalArgument);
        }

        ScMatrixRef xMatrix(
            new ScMatrix(static_cast<SCSIZE>(rNode.mnArrayColumns), static_cast<SCSIZE>(rNode.mnArrayRows)));
        std::size_t nIndex = 0;
        for (sal_Int32 nRow = 0; nRow < rNode.mnArrayRows; ++nRow)
        {
            for (sal_Int32 nColumn = 0; nColumn < rNode.mnArrayColumns; ++nColumn)
            {
                const auto aElement
                    = materializeScalarNode(*rNode.maChildren[nIndex++], rDoc, rContext, rFormulaPos);
                if (!aElement.mbSupported)
                    return makeUnsupportedMaterialization<ScMatrixRef>(aElement.meFallbackReason);
                if (!aElement.moValue)
                    return makeMaterializedError<ScMatrixRef>(aElement.meError);

                putScalarIntoMatrix(*aElement.moValue, xMatrix, static_cast<SCSIZE>(nColumn),
                    static_cast<SCSIZE>(nRow));
            }
        }

        return makeMaterializedValue(xMatrix);
    }

    const auto aScalar = materializeScalarNode(rNode, rDoc, rContext, rFormulaPos);
    if (!aScalar.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aScalar.meFallbackReason);
    if (!aScalar.moValue)
        return makeMaterializedError<ScMatrixRef>(aScalar.meError);

    ScMatrixRef xMatrix(new ScMatrix(1, 1));
    putScalarIntoMatrix(*aScalar.moValue, xMatrix, 0, 0);
    return makeMaterializedValue(xMatrix);
}

[[nodiscard]] inline Materialization<lookupexecution::LookupInputSource> materializeLookupInputSourceNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.meKind == core::formula::NodeKind::CellReference
        || rNode.meKind == core::formula::NodeKind::RangeReference
        || rNode.meKind == core::formula::NodeKind::NamedReference)
    {
        const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
        if (!aRange.mbSupported)
        {
            return makeUnsupportedMaterialization<lookupexecution::LookupInputSource>(
                aRange.meFallbackReason);
        }
        if (!aRange.moValue)
            return makeMaterializedError<lookupexecution::LookupInputSource>(aRange.meError);

        if (aRange.moValue->aStart.Tab() != aRange.moValue->aEnd.Tab())
        {
            return makeUnsupportedMaterialization<lookupexecution::LookupInputSource>(
                FallbackReason::UnsupportedHostSurface);
        }

        lookupexecution::LookupInputSource aSource;
        aSource.moRange = *aRange.moValue;
        return makeMaterializedValue(aSource);
    }

    const auto aMatrix = materializeMatrixNode(rNode, rDoc, rContext, rFormulaPos);
    if (!aMatrix.mbSupported)
    {
        return makeUnsupportedMaterialization<lookupexecution::LookupInputSource>(
            aMatrix.meFallbackReason);
    }
    if (!aMatrix.moValue)
        return makeMaterializedError<lookupexecution::LookupInputSource>(aMatrix.meError);

    lookupexecution::LookupInputSource aSource;
    aSource.mpMatrix = *aMatrix.moValue;
    return makeMaterializedValue(aSource);
}

[[nodiscard]] inline EvaluationAttempt materializeLookupResult(
    FunctionKind eFunction, const ScDocument& rDoc, const lookupexecution::LookupExecutionResult& rResult)
{
    const auto makeLookupScalarAttempt = [&](const api::CellValue& rValue) {
        if (rValue.isEmpty())
            return makeUnsupported(eFunction, FallbackReason::UnsupportedHostSurface);
        return makeScalarAttempt(eFunction, rValue);
    };

    if (rResult.meKind == lookupexecution::LookupExecutionResult::Kind::Scalar)
        return makeLookupScalarAttempt(rResult.maScalar);

    if (rResult.isSingleCellReference())
        return makeLookupScalarAttempt(readHostDocumentCellValue(rDoc, rResult.maRange.aStart).maValue);

    if (rResult.meKind == lookupexecution::LookupExecutionResult::Kind::Matrix && rResult.mpMatrix)
    {
        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        rResult.mpMatrix->GetDimensions(nColumns, nRows);
        if (nColumns == 1 && nRows == 1)
            return makeLookupScalarAttempt(lookupexecution::detail::toApiCellValue(rResult.mpMatrix->Get(0, 0)));
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedHostSurface);
}

[[nodiscard]] inline FunctionKind classifyDelegatedFunctionNode(
    const core::formula::Node& rNode)
{
    if (rNode.meKind != core::formula::NodeKind::FunctionCall)
        return FunctionKind::Unknown;

    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if (aFunctionName == u"IFERROR" || aFunctionName == u"IFNA")
    {
        if (rNode.maChildren.empty())
            return FunctionKind::Unknown;
        return classifyDelegatedFunctionNode(*rNode.maChildren[0]);
    }

    return classifyFunction(aFunctionName);
}

[[nodiscard]] inline EvaluationAttempt evaluateTextParsingFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero)
{
    auto materializeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        return materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
    };

    if (eFunction == FunctionKind::Value || eFunction == FunctionKind::DateValue
        || eFunction == FunctionKind::TimeValue)
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aArgument = materializeArgument(*rNode.maChildren[0]);
        if (!aArgument.mbSupported)
            return makeUnsupported(eFunction, aArgument.meFallbackReason);
        if (!aArgument.moValue)
            return makeErrorResult(eFunction, aArgument.meError);

        if (eFunction == FunctionKind::Value
            && (aArgument.moValue->isNumber() || aArgument.moValue->isBoolean()))
        {
            return makeNumericResult(eFunction, aArgument.moValue->mfNumber,
                SvNumFormatType::NUMBER);
        }
        if (eFunction == FunctionKind::Value && aArgument.moValue->isEmpty())
            return makeNumericResult(eFunction, 0.0, SvNumFormatType::NUMBER);

        if (eFunction == FunctionKind::DateValue)
        {
            if (const auto oSerial
                = referencedDateTimeSerial(*rNode.maChildren[0], rDoc, rContext, rFormulaPos))
            {
                return makeNumericResult(
                    eFunction, std::trunc(*oSerial), SvNumFormatType::DATE);
            }
        }

        const auto aText = coerceScalarToText(rDoc, rContext, *aArgument.moValue);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);

        if (eFunction == FunctionKind::Value)
        {
            const auto aResult = textparsingexecution::evaluateValue(rDoc, rContext, aText.maValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::NUMBER);
        }

        if (eFunction == FunctionKind::DateValue)
        {
            const auto aResult
                = textparsingexecution::evaluateDateValue(rDoc, rContext, aText.maValue);
            if (!aResult)
            {
                if (const auto oIsoFallback = tryIsoDateValueFallback(rDoc, aText.maValue))
                    return makeNumericResult(eFunction, *oIsoFallback, SvNumFormatType::DATE);
                return makeErrorResult(eFunction, aResult.meError);
            }
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::DATE);
        }

        const auto aResult = textparsingexecution::evaluateTimeValue(rDoc, rContext, aText.maValue);
        if (!aResult)
            return makeErrorResult(eFunction, aResult.meError);
        return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::TIME);
    }

    if (eFunction == FunctionKind::NumberValue)
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aTextArg = materializeArgument(*rNode.maChildren[0]);
        if (!aTextArg.mbSupported)
            return makeUnsupported(eFunction, aTextArg.meFallbackReason);
        if (!aTextArg.moValue)
            return makeErrorResult(eFunction, aTextArg.meError);
        const auto aText = coerceScalarToText(rDoc, rContext, *aTextArg.moValue);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);

        std::optional<OUString> oDecimalSeparator;
        std::optional<OUString> oGroupSeparator;
        if (rNode.maChildren.size() >= 2)
        {
            const auto aDecimalArg = materializeArgument(*rNode.maChildren[1]);
            if (!aDecimalArg.mbSupported)
                return makeUnsupported(eFunction, aDecimalArg.meFallbackReason);
            if (!aDecimalArg.moValue)
                return makeErrorResult(eFunction, aDecimalArg.meError);
            const auto aDecimal = coerceScalarToText(rDoc, rContext, *aDecimalArg.moValue);
            if (!aDecimal)
                return makeErrorResult(eFunction, aDecimal.meError);
            oDecimalSeparator = aDecimal.maValue;
        }
        if (rNode.maChildren.size() == 3)
        {
            const auto aGroupArg = materializeArgument(*rNode.maChildren[2]);
            if (!aGroupArg.mbSupported)
                return makeUnsupported(eFunction, aGroupArg.meFallbackReason);
            if (!aGroupArg.moValue)
                return makeErrorResult(eFunction, aGroupArg.meError);
            const auto aGroup = coerceScalarToText(rDoc, rContext, *aGroupArg.moValue);
            if (!aGroup)
                return makeErrorResult(eFunction, aGroup.meError);
            oGroupSeparator = aGroup.maValue;
        }

        const auto aResult = textparsingexecution::evaluateNumberValue(
            rDoc, rContext, aText.maValue, oDecimalSeparator, oGroupSeparator,
            bEmptyStringAsZero);
        if (!aResult)
            return makeErrorResult(eFunction, aResult.meError);
        return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::NUMBER);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateLookupFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const api::query::SearchType eSearchType = searchTypeFromDocument(rDoc);
    auto materializeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        return materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
    };
    auto materializeSource = [&](const core::formula::Node& rArgument)
        -> Materialization<lookupexecution::LookupInputSource> {
        return materializeLookupInputSourceNode(rArgument, rDoc, rContext, rFormulaPos);
    };
    auto normalizeWholeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<sal_Int32> {
        const auto aArgument = materializeArgument(rArgument);
        if (!aArgument.mbSupported)
            return makeUnsupportedMaterialization<sal_Int32>(aArgument.meFallbackReason);
        if (!aArgument.moValue)
            return makeMaterializedError<sal_Int32>(aArgument.meError);
        if (aArgument.moValue->isEmpty())
            return makeMaterializedError<sal_Int32>(api::Error::IllegalArgument);
        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aArgument.moValue);
        if (!aNumber)
            return makeMaterializedError<sal_Int32>(aNumber.meError);
        const auto oWhole = coerceWholeNumber(aNumber.maValue);
        if (!oWhole)
            return makeMaterializedError<sal_Int32>(api::Error::IllegalArgument);
        return makeMaterializedValue(*oWhole);
    };

    if (eFunction == FunctionKind::Match)
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aLookup = materializeArgument(*rNode.maChildren[0]);
        if (!aLookup.mbSupported)
            return makeUnsupported(eFunction, aLookup.meFallbackReason);
        if (!aLookup.moValue)
            return makeErrorResult(eFunction, aLookup.meError);
        if (aLookup.moValue->isError())
            return makeErrorResult(eFunction, aLookup.moValue->meError);

        const auto aSearch = materializeSource(*rNode.maChildren[1]);
        if (!aSearch.mbSupported)
            return makeUnsupported(eFunction, aSearch.meFallbackReason);
        if (!aSearch.moValue)
            return makeErrorResult(eFunction, aSearch.meError);

        api::lookup::MatchSearchMode aModes;
        if (rNode.maChildren.size() == 3)
        {
            const auto aMode = normalizeWholeArgument(*rNode.maChildren[2]);
            if (!aMode.mbSupported)
                return makeUnsupported(eFunction, aMode.meFallbackReason);
            if (!aMode.moValue)
                return makeErrorResult(eFunction, aMode.meError);
            const auto aNormalized = api::lookup::normalizeMatchType(*aMode.moValue);
            if (!aNormalized)
                return makeErrorResult(eFunction, aNormalized.meError);
            aModes = aNormalized.maValue;
        }
        else
        {
            aModes.meMatchMode = api::lookup::MatchMode::ExactOrNextSmaller;
            aModes.meSearchMode = api::lookup::SearchMode::BinaryAscending;
        }

        lookupexecution::MatchExecutionRequest aRequest;
        aRequest.maLookupValue = *aLookup.moValue;
        aRequest.maSearchSource = *aSearch.moValue;
        aRequest.maLegacyModes = aModes;
        aRequest.meSearchType = eSearchType;
        const auto aResolved = lookupexecution::resolveMatchIndex(rDoc, rContext, aRequest);
        if (!aResolved)
            return makeErrorResult(eFunction, aResolved.meError);
        return makeNumericResult(eFunction, static_cast<double>(aResolved.maValue + 1),
            SvNumFormatType::NUMBER);
    }

    if (eFunction == FunctionKind::XMatch)
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aLookup = materializeArgument(*rNode.maChildren[0]);
        if (!aLookup.mbSupported)
            return makeUnsupported(eFunction, aLookup.meFallbackReason);
        if (!aLookup.moValue)
            return makeErrorResult(eFunction, aLookup.meError);
        if (aLookup.moValue->isError())
            return makeErrorResult(eFunction, aLookup.moValue->meError);

        const auto aSearch = materializeSource(*rNode.maChildren[1]);
        if (!aSearch.mbSupported)
            return makeUnsupported(eFunction, aSearch.meFallbackReason);
        if (!aSearch.moValue)
            return makeErrorResult(eFunction, aSearch.meError);

        lookupexecution::MatchExecutionRequest aRequest;
        aRequest.mbExtended = true;
        aRequest.mbAllowPatternMatch = true;
        aRequest.maLookupValue = *aLookup.moValue;
        aRequest.maSearchSource = *aSearch.moValue;
        aRequest.meSearchType = eSearchType;

        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMode = normalizeWholeArgument(*rNode.maChildren[2]);
            if (!aMode.mbSupported)
                return makeUnsupported(eFunction, aMode.meFallbackReason);
            if (!aMode.moValue)
                return makeErrorResult(eFunction, aMode.meError);
            const auto aNormalized = api::lookup::normalizeExtendedMatchMode(
                static_cast<std::int16_t>(*aMode.moValue));
            if (!aNormalized)
                return makeErrorResult(eFunction, aNormalized.meError);
            aRequest.meMatchMode = aNormalized.maValue;
        }

        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMode = normalizeWholeArgument(*rNode.maChildren[3]);
            if (!aMode.mbSupported)
                return makeUnsupported(eFunction, aMode.meFallbackReason);
            if (!aMode.moValue)
                return makeErrorResult(eFunction, aMode.meError);
            const auto aNormalized = api::lookup::normalizeSearchMode(
                static_cast<std::int16_t>(*aMode.moValue));
            if (!aNormalized)
                return makeErrorResult(eFunction, aNormalized.meError);
            aRequest.meSearchMode = aNormalized.maValue;
        }

        const auto aResolved = lookupexecution::resolveMatchIndex(rDoc, rContext, aRequest);
        if (!aResolved)
            return makeErrorResult(eFunction, aResolved.meError);
        return makeNumericResult(eFunction, static_cast<double>(aResolved.maValue + 1),
            SvNumFormatType::NUMBER);
    }

    if (eFunction == FunctionKind::Lookup)
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aLookup = materializeArgument(*rNode.maChildren[0]);
        if (!aLookup.mbSupported)
            return makeUnsupported(eFunction, aLookup.meFallbackReason);
        if (!aLookup.moValue)
            return makeErrorResult(eFunction, aLookup.meError);
        if (aLookup.moValue->isError())
            return makeErrorResult(eFunction, aLookup.moValue->meError);

        const auto aData = materializeSource(*rNode.maChildren[1]);
        if (!aData.mbSupported)
            return makeUnsupported(eFunction, aData.meFallbackReason);
        if (!aData.moValue)
            return makeErrorResult(eFunction, aData.meError);

        lookupexecution::LegacyLookupRequest aRequest;
        aRequest.maLookupValue = *aLookup.moValue;
        aRequest.meSearchType = eSearchType;
        const auto aDataInput = lookupexecution::detail::buildLookupInput(*aData.moValue);
        if (!aDataInput)
            return makeErrorResult(eFunction, aDataInput.meError);
        aRequest.maDataInput = aDataInput.maValue;

        if (rNode.maChildren.size() == 3)
        {
            const auto aResult = materializeSource(*rNode.maChildren[2]);
            if (!aResult.mbSupported)
                return makeUnsupported(eFunction, aResult.meFallbackReason);
            if (!aResult.moValue)
                return makeErrorResult(eFunction, aResult.meError);
            const auto aResultInput = lookupexecution::detail::buildLookupInput(*aResult.moValue);
            if (!aResultInput)
                return makeErrorResult(eFunction, aResultInput.meError);
            aRequest.moResultInput = aResultInput.maValue;
        }

        const auto aResolved = lookupexecution::resolveLookupResult(rDoc, rContext, aRequest);
        if (!aResolved)
            return makeErrorResult(eFunction, aResolved.meError);
        return materializeLookupResult(eFunction, rDoc, aResolved.maValue);
    }

    if (eFunction == FunctionKind::VLookup || eFunction == FunctionKind::HLookup)
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aLookup = materializeArgument(*rNode.maChildren[0]);
        if (!aLookup.mbSupported)
            return makeUnsupported(eFunction, aLookup.meFallbackReason);
        if (!aLookup.moValue)
            return makeErrorResult(eFunction, aLookup.meError);
        if (aLookup.moValue->isError())
            return makeErrorResult(eFunction, aLookup.moValue->meError);

        const auto aTable = materializeSource(*rNode.maChildren[1]);
        if (!aTable.mbSupported)
            return makeUnsupported(eFunction, aTable.meFallbackReason);
        if (!aTable.moValue)
            return makeErrorResult(eFunction, aTable.meError);
        const auto aTableInput = lookupexecution::detail::buildLookupInput(*aTable.moValue);
        if (!aTableInput)
            return makeErrorResult(eFunction, aTableInput.meError);

        const auto aIndex = normalizeWholeArgument(*rNode.maChildren[2]);
        if (!aIndex.mbSupported)
            return makeUnsupported(eFunction, aIndex.meFallbackReason);
        if (!aIndex.moValue || *aIndex.moValue < 1)
            return makeErrorResult(eFunction, aIndex.moValue ? api::Error::IllegalArgument
                                                             : aIndex.meError);

        bool bApproximate = true;
        if (rNode.maChildren.size() == 4)
        {
            const auto aApprox = materializeArgument(*rNode.maChildren[3]);
            if (!aApprox.mbSupported)
                return makeUnsupported(eFunction, aApprox.meFallbackReason);
            if (!aApprox.moValue)
                return makeErrorResult(eFunction, aApprox.meError);
            const auto aApproxBool = coerceScalarToBool(rDoc, rContext, *aApprox.moValue);
            if (!aApproxBool)
                return makeErrorResult(eFunction, aApproxBool.meError);
            bApproximate = aApproxBool.maValue;
        }

        lookupexecution::TabularLookupRequest aRequest;
        aRequest.maLookupValue = *aLookup.moValue;
        aRequest.maTableInput = aTableInput.maValue;
        aRequest.meSearchOrientation
            = eFunction == FunctionKind::HLookup ? api::lookup::VectorOrientation::Row
                                                 : api::lookup::VectorOrientation::Column;
        aRequest.mnResultIndex = *aIndex.moValue - 1;
        aRequest.mbApproximate = bApproximate;
        aRequest.meSearchType = eSearchType;

        const auto aResolved
            = lookupexecution::resolveTabularLookupResult(rDoc, rContext, aRequest);
        if (!aResolved)
            return makeErrorResult(eFunction, aResolved.meError);
        return materializeLookupResult(eFunction, rDoc, aResolved.maValue);
    }

    if (eFunction == FunctionKind::XLookup)
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aLookup = materializeArgument(*rNode.maChildren[0]);
        if (!aLookup.mbSupported)
            return makeUnsupported(eFunction, aLookup.meFallbackReason);
        if (!aLookup.moValue)
            return makeErrorResult(eFunction, aLookup.meError);
        if (aLookup.moValue->isError())
            return makeErrorResult(eFunction, aLookup.moValue->meError);

        const auto aSearch = materializeSource(*rNode.maChildren[1]);
        if (!aSearch.mbSupported)
            return makeUnsupported(eFunction, aSearch.meFallbackReason);
        if (!aSearch.moValue)
            return makeErrorResult(eFunction, aSearch.meError);
        const auto aSearchInput = lookupexecution::detail::buildLookupInput(*aSearch.moValue);
        if (!aSearchInput)
            return makeErrorResult(eFunction, aSearchInput.meError);

        const auto aResult = materializeSource(*rNode.maChildren[2]);
        if (!aResult.mbSupported)
            return makeUnsupported(eFunction, aResult.meFallbackReason);
        if (!aResult.moValue)
            return makeErrorResult(eFunction, aResult.meError);
        const auto aResultInput = lookupexecution::detail::buildLookupInput(*aResult.moValue);
        if (!aResultInput)
            return makeErrorResult(eFunction, aResultInput.meError);

        lookupexecution::XLookupExecutionRequest aRequest;
        aRequest.maLookupValue = *aLookup.moValue;
        aRequest.maSearchInput = aSearchInput.maValue;
        aRequest.maResultInput = aResultInput.maValue;
        aRequest.meSearchType = eSearchType;
        aRequest.mbAllowPatternMatch = true;

        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMode = normalizeWholeArgument(*rNode.maChildren[4]);
            if (!aMode.mbSupported)
                return makeUnsupported(eFunction, aMode.meFallbackReason);
            if (!aMode.moValue)
                return makeErrorResult(eFunction, aMode.meError);
            const auto aNormalized = api::lookup::normalizeExtendedMatchMode(
                static_cast<std::int16_t>(*aMode.moValue));
            if (!aNormalized)
                return makeErrorResult(eFunction, aNormalized.meError);
            aRequest.meMatchMode = aNormalized.maValue;
        }

        if (rNode.maChildren.size() >= 6
            && rNode.maChildren[5]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMode = normalizeWholeArgument(*rNode.maChildren[5]);
            if (!aMode.mbSupported)
                return makeUnsupported(eFunction, aMode.meFallbackReason);
            if (!aMode.moValue)
                return makeErrorResult(eFunction, aMode.meError);
            const auto aNormalized = api::lookup::normalizeSearchMode(
                static_cast<std::int16_t>(*aMode.moValue));
            if (!aNormalized)
                return makeErrorResult(eFunction, aNormalized.meError);
            aRequest.meSearchMode = aNormalized.maValue;
        }

        const auto aResolved = lookupexecution::resolveXLookupResult(rDoc, rContext, aRequest);
        if (!aResolved)
        {
            if (aResolved.meError == api::Error::NotAvailable && rNode.maChildren.size() >= 4
                && rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument)
            {
                auto aFallback = evaluateScalarOrDelegatedNode(*rNode.maChildren[3], eFunction, rDoc,
                    rContext, rFormulaPos, rDoc.GetCalcConfig().mbEmptyStringAsZero, 1);
                if (aFallback.meFunction == FunctionKind::Unknown)
                    aFallback.meFunction = eFunction;
                return aFallback;
            }
            return makeErrorResult(eFunction, aResolved.meError);
        }
        return materializeLookupResult(eFunction, rDoc, aResolved.maValue);
    }

    if (eFunction == FunctionKind::Index)
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aRow = normalizeWholeArgument(*rNode.maChildren[1]);
        if (!aRow.mbSupported)
            return makeUnsupported(eFunction, aRow.meFallbackReason);
        if (!aRow.moValue || *aRow.moValue < 0)
            return makeErrorResult(eFunction, aRow.moValue ? api::Error::IllegalArgument
                                                           : aRow.meError);

        sal_Int32 nColumn = 0;
        if (rNode.maChildren.size() == 3)
        {
            const auto aColumn = normalizeWholeArgument(*rNode.maChildren[2]);
            if (!aColumn.mbSupported)
                return makeUnsupported(eFunction, aColumn.meFallbackReason);
            if (!aColumn.moValue || *aColumn.moValue < 0)
            {
                return makeErrorResult(eFunction, aColumn.moValue ? api::Error::IllegalArgument
                                                                  : aColumn.meError);
            }
            nColumn = *aColumn.moValue;
        }

        const auto aReferenceSource = resolveReferenceRangeNode(*rNode.maChildren[0], rDoc, rFormulaPos);
        if (aReferenceSource.mbSupported && aReferenceSource.moValue)
        {
            const auto aSelection = referenceexecution::planIndexReferenceSelection(
                *aReferenceSource.moValue, *aRow.moValue, nColumn,
                static_cast<std::uint8_t>(rNode.maChildren.size()));
            if (!aSelection)
                return makeErrorResult(eFunction, aSelection.meError);
            if (!aSelection.maValue.maRange.isSingleCell())
                return makeUnsupported(eFunction, FallbackReason::UnsupportedHostSurface);

            lookupexecution::LookupExecutionResult aResult;
            aResult.meKind = lookupexecution::LookupExecutionResult::Kind::Reference;
            aResult.maRange = toLibreOfficeRange(aSelection.maValue.maRange);
            return materializeLookupResult(eFunction, rDoc, aResult);
        }
        if (!aReferenceSource.mbSupported
            && aReferenceSource.meFallbackReason != FallbackReason::UnsupportedFormulaShape)
        {
            return makeUnsupported(eFunction, aReferenceSource.meFallbackReason);
        }
        if (!aReferenceSource.mbSupported
            && aReferenceSource.meFallbackReason == FallbackReason::UnsupportedFormulaShape)
        {
            const auto aMatrixSource
                = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
            if (!aMatrixSource.mbSupported)
                return makeUnsupported(eFunction, aMatrixSource.meFallbackReason);
            if (!aMatrixSource.moValue)
                return makeErrorResult(eFunction, aMatrixSource.meError);

            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aMatrixSource.moValue)->GetDimensions(nColumns, nRows);
            const auto aSelection = api::reference::planIndexMatrixSelection(
                { static_cast<api::MatrixSize>(nColumns), static_cast<api::MatrixSize>(nRows) },
                *aRow.moValue, nColumn, rNode.maChildren.size() < 3,
                static_cast<std::uint8_t>(rNode.maChildren.size()));
            if (!aSelection)
                return makeErrorResult(eFunction, aSelection.meError);
            if (aSelection.maValue.maDimensions.mnColumns != 1
                || aSelection.maValue.maDimensions.mnRows != 1)
            {
                return makeUnsupported(eFunction, FallbackReason::UnsupportedHostSurface);
            }

            const api::CellValue aScalar = lookupexecution::detail::toApiCellValue(
                (*aMatrixSource.moValue)->Get(static_cast<SCSIZE>(aSelection.maValue.maStart.mnColumn),
                    static_cast<SCSIZE>(aSelection.maValue.maStart.mnRow)));
            if (aScalar.isError())
                return makeErrorResult(eFunction, aScalar.meError);
            if (aScalar.isText())
                return makeStringResult(eFunction, toLibreOfficeString(aScalar.maString));
            if (aScalar.isNumber() || aScalar.isBoolean())
                return makeNumericResult(eFunction, aScalar.mfNumber, SvNumFormatType::NUMBER);
            return makeUnsupported(eFunction, FallbackReason::UnsupportedHostSurface);
        }

        return makeErrorResult(eFunction, aReferenceSource.meError);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateFunctionNode(
    const core::formula::Node& rRoot, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos, bool bEmptyStringAsZero)
{
    const api::String aFunctionName = uppercaseAscii(rRoot.maPrimaryText);
    const FunctionKind eFunction = classifyFunction(aFunctionName);
    switch (eFunction)
    {
        case FunctionKind::Value:
        case FunctionKind::DateValue:
        case FunctionKind::TimeValue:
        case FunctionKind::NumberValue:
            return evaluateTextParsingFunction(
                rRoot, eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero);
        case FunctionKind::Match:
        case FunctionKind::XMatch:
        case FunctionKind::Lookup:
        case FunctionKind::VLookup:
        case FunctionKind::HLookup:
        case FunctionKind::XLookup:
        case FunctionKind::Index:
            return evaluateLookupFunction(rRoot, eFunction, rDoc, rContext, rFormulaPos);
        case FunctionKind::Unknown:
        case FunctionKind::Count:
            return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateScalarOrDelegatedNode(
    const core::formula::Node& rNode, FunctionKind ePreferredFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    std::size_t nDepth)
{
    if (rNode.meKind == core::formula::NodeKind::FunctionCall)
    {
        auto aAttempt
            = evaluateDelegatedNode(rNode, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, nDepth);
        if (aAttempt.meFunction == FunctionKind::Unknown)
            aAttempt.meFunction = ePreferredFunction;
        return aAttempt;
    }

    const auto aScalar = materializeScalarNode(rNode, rDoc, rContext, rFormulaPos);
    if (!aScalar.mbSupported)
        return makeUnsupported(ePreferredFunction, aScalar.meFallbackReason);
    if (!aScalar.moValue)
        return makeErrorResult(ePreferredFunction, aScalar.meError);
    return makeScalarAttempt(ePreferredFunction, *aScalar.moValue);
}

[[nodiscard]] inline EvaluationAttempt evaluateDelegatedNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos, bool bEmptyStringAsZero, std::size_t nDepth)
{
    if (nDepth > 8)
        return makeUnsupported(classifyDelegatedFunctionNode(rNode),
            FallbackReason::UnsupportedFormulaShape);

    if (rNode.meKind != core::formula::NodeKind::FunctionCall)
        return makeUnsupported(classifyDelegatedFunctionNode(rNode),
            FallbackReason::UnsupportedFormulaShape);

    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if (aFunctionName == u"IFERROR" || aFunctionName == u"IFNA")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorResult(FunctionKind::Unknown, api::Error::IllegalArgument);

        const FunctionKind ePrimaryFunction = classifyDelegatedFunctionNode(*rNode.maChildren[0]);
        auto aPrimary = evaluateDelegatedNode(
            *rNode.maChildren[0], rDoc, rContext, rFormulaPos, bEmptyStringAsZero, nDepth + 1);
        if (!aPrimary.mbSupported)
        {
            if (aPrimary.meFunction == FunctionKind::Unknown)
                aPrimary.meFunction = ePrimaryFunction;
            return aPrimary;
        }

        const bool bUseFallback
            = aPrimary.maResult.meType == api::formulavalue::ValueType::Error
              && (aFunctionName == u"IFERROR"
                  || aPrimary.maResult.meError == api::Error::NotAvailable);
        if (!bUseFallback)
        {
            if (aPrimary.meFunction == FunctionKind::Unknown)
                aPrimary.meFunction = ePrimaryFunction;
            return aPrimary;
        }

        auto aFallback = evaluateScalarOrDelegatedNode(*rNode.maChildren[1],
            ePrimaryFunction != FunctionKind::Unknown ? ePrimaryFunction : FunctionKind::Unknown,
            rDoc, rContext, rFormulaPos, bEmptyStringAsZero, nDepth + 1);
        if (aFallback.meFunction == FunctionKind::Unknown)
            aFallback.meFunction = ePrimaryFunction;
        return aFallback;
    }

    return evaluateFunctionNode(rNode, rDoc, rContext, rFormulaPos, bEmptyStringAsZero);
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
    const ScDocument& rDoc, ScInterpreterContext& rContext, const ScAddress& rFormulaPos,
    std::u16string_view rFormulaSource, bool bEmptyStringAsZero)
{
    const api::String aNormalized = detail::normalizeFormulaSource(rFormulaSource);
    const auto aParse = core::formula::parseFormula(aNormalized);
    if (!aParse || !aParse.mpRoot)
    {
        detail::recordDiagnosticSample(FallbackReason::ParseFailure, rDoc, rFormulaPos,
            rFormulaSource, aNormalized, std::nullopt);
        return detail::makeUnsupported(FunctionKind::Unknown, FallbackReason::ParseFailure);
    }

    const auto& rRoot = *aParse.mpRoot;
    if (rRoot.meKind != core::formula::NodeKind::FunctionCall)
    {
        detail::recordDiagnosticSample(FallbackReason::UnsupportedFormulaShape, rDoc, rFormulaPos,
            rFormulaSource, aNormalized, rRoot.meKind);
        return detail::makeUnsupported(
            FunctionKind::Unknown, FallbackReason::UnsupportedFormulaShape);
    }

    return detail::evaluateDelegatedNode(
        rRoot, rDoc, rContext, rFormulaPos, bEmptyStringAsZero);
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
    for (auto& rValue : rStore.maFunctionObserveCount)
        rValue.store(0);
    for (auto& rValue : rStore.maFunctionShadowCompareCount)
        rValue.store(0);
    for (auto& rValue : rStore.maFunctionAuthoritativeCount)
        rValue.store(0);
    for (auto& rValue : rStore.maFunctionFallbackCount)
        rValue.store(0);
    for (auto& rFunctionReasons : rStore.maFunctionFallbackReasons)
    {
        for (auto& rValue : rFunctionReasons)
            rValue.store(0);
    }

    auto& rDiagnostics = detail::diagnosticStore();
    std::scoped_lock aGuard(rDiagnostics.maMutex);
    rDiagnostics.maSamples.clear();
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
    for (std::size_t i = 0; i < aSnapshot.maFunctionObserveCount.size(); ++i)
        aSnapshot.maFunctionObserveCount[i] = rStore.maFunctionObserveCount[i].load();
    for (std::size_t i = 0; i < aSnapshot.maFunctionShadowCompareCount.size(); ++i)
        aSnapshot.maFunctionShadowCompareCount[i] = rStore.maFunctionShadowCompareCount[i].load();
    for (std::size_t i = 0; i < aSnapshot.maFunctionAuthoritativeCount.size(); ++i)
        aSnapshot.maFunctionAuthoritativeCount[i]
            = rStore.maFunctionAuthoritativeCount[i].load();
    for (std::size_t i = 0; i < aSnapshot.maFunctionFallbackCount.size(); ++i)
        aSnapshot.maFunctionFallbackCount[i] = rStore.maFunctionFallbackCount[i].load();
    for (std::size_t i = 0; i < aSnapshot.maFunctionFallbackReasons.size(); ++i)
    {
        for (std::size_t j = 0; j < aSnapshot.maFunctionFallbackReasons[i].size(); ++j)
        {
            aSnapshot.maFunctionFallbackReasons[i][j]
                = rStore.maFunctionFallbackReasons[i][j].load();
        }
    }
    return aSnapshot;
}

inline void recordObserveSupport(FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.mnObserveCount.fetch_add(1);
    rStore.maFunctionObserveCount[detail::toIndex(eFunction)].fetch_add(1);
}

inline void recordShadowCompareSupport(FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.mnShadowCompareCount.fetch_add(1);
    rStore.maFunctionShadowCompareCount[detail::toIndex(eFunction)].fetch_add(1);
}

inline void recordAuthoritativeRoute(FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.mnAuthoritativeCount.fetch_add(1);
    rStore.maFunctionAuthoritativeCount[detail::toIndex(eFunction)].fetch_add(1);
}

inline void recordAuthoritativeFallback(FallbackReason eReason, FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.mnAuthoritativeFallbackCount.fetch_add(1);
    rStore.maFallbackReasons[detail::toIndex(eReason)].fetch_add(1);
    rStore.maFunctionFallbackCount[detail::toIndex(eFunction)].fetch_add(1);
    rStore.maFunctionFallbackReasons[detail::toIndex(eFunction)][detail::toIndex(eReason)].fetch_add(1);
}

inline void recordFallback(FallbackReason eReason, FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.maFallbackReasons[detail::toIndex(eReason)].fetch_add(1);
    rStore.maFunctionFallbackCount[detail::toIndex(eFunction)].fetch_add(1);
    rStore.maFunctionFallbackReasons[detail::toIndex(eFunction)][detail::toIndex(eReason)].fetch_add(1);
}

inline void recordShadowMatch()
{
    detail::statsStore().mnShadowMatchCount.fetch_add(1);
}

inline void recordMismatch(MismatchReason eReason)
{
    detail::statsStore().maMismatchReasons[detail::toIndex(eReason)].fetch_add(1);
}

inline void setDiagnosticWorkbookLabel(const OUString& rWorkbookLabel)
{
    auto& rStore = detail::diagnosticStore();
    std::scoped_lock aGuard(rStore.maMutex);
    rStore.maWorkbookLabel = rWorkbookLabel;
}

inline std::vector<DiagnosticSample> getDiagnosticSamples()
{
    auto& rStore = detail::diagnosticStore();
    std::scoped_lock aGuard(rStore.maMutex);
    return rStore.maSamples;
}

} // namespace spreadsheetengine::compat::libreoffice::interprettaileval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
