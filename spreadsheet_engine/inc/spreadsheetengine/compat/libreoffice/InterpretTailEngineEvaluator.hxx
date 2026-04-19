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
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <mutex>
#include <numeric>
#include <optional>
#include <rtl/math.hxx>
#include <string>
#include <string_view>
#include <vector>

#include <document.hxx>
#include <docoptio.hxx>
#include <compiler.hxx>
#include <formula/grammar.hxx>
#include <formula/token.hxx>
#include <global.hxx>
#include <interpretercontext.hxx>
#include <rangeutl.hxx>
#include <svl/numformat.hxx>
#include <tokenarray.hxx>

#include <spreadsheetengine/api/FormulaResult.hxx>
#include <spreadsheetengine/api/Array.hxx>
#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Workday.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/FormulaInspectionExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/LookupExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/TextServices.hxx>
#include <spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx>
#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/runtime/DateTimeParse.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/FinancialRuntime.hxx>
#include <spreadsheetengine/runtime/ConversionRuntime.hxx>
#include <spreadsheetengine/runtime/NumeralConversion.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <spreadsheetengine/runtime/MathBitwise.hxx>
#include <spreadsheetengine/runtime/MathFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/MathMatrix.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <spreadsheetengine/runtime/MathScalar.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>
#include <spreadsheetengine/runtime/MathTranscendental.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/ScalarCoercion.hxx>
#include <spreadsheetengine/runtime/TextFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/TextRuntimeSupport.hxx>

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
    ScalarRoot,
    Conditional,
    FormulaText,
    LogicalConstant,
    TextUtility,
    Value,
    DateValue,
    TimeValue,
    NumberValue,
    Rate,
    Round,
    Conversion,
    NumericAggregate,
    RankedAggregate,
    StatisticalAggregate,
    StatisticalDistribution,
    GrowthProjection,
    CriteriaAggregate,
    Aggregate,
    BusinessDay,
    CalendarUtility,
    DateDifference,
    DateConstructExtract,
    MatrixMath,
    MathScalar,
    InformationPredicate,
    LogicalFold,
    Not,
    Match,
    XMatch,
    Selector,
    SpillArray,
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

struct ObservedFormulaCellStatus
{
    ScAddress maAddress;
    bool mbSeen = false;
    bool mbSupported = false;
    bool mbFallback = false;
    bool mbUnsupportedFunctionFallback = false;
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

struct AddressLess
{
    bool operator()(const ScAddress& rLeft, const ScAddress& rRight) const
    {
        if (rLeft.Tab() != rRight.Tab())
            return rLeft.Tab() < rRight.Tab();
        if (rLeft.Row() != rRight.Row())
            return rLeft.Row() < rRight.Row();
        return rLeft.Col() < rRight.Col();
    }
};

struct ObserveSurfaceStore
{
    std::mutex maMutex;
    std::map<ScAddress, ObservedFormulaCellStatus, AddressLess> maCells;
};

struct ReferencedFormulaMaterializationGuard
{
    bool mbActive = false;

    ReferencedFormulaMaterializationGuard(const ScAddress& rAddress)
    {
        auto& rStack = stack();
        if (std::find(rStack.begin(), rStack.end(), rAddress) != rStack.end() || rStack.size() >= 8)
            return;

        rStack.push_back(rAddress);
        mbActive = true;
    }

    ~ReferencedFormulaMaterializationGuard()
    {
        if (!mbActive)
            return;

        auto& rStack = stack();
        rStack.pop_back();
    }

private:
    [[nodiscard]] static std::vector<ScAddress>& stack()
    {
        static thread_local std::vector<ScAddress> aStack;
        return aStack;
    }
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

[[nodiscard]] inline ObserveSurfaceStore& observeSurfaceStore()
{
    static ObserveSurfaceStore aStore;
    return aStore;
}

inline void markObservedFormulaCell(const ScAddress& rFormulaPos, bool bSupported, bool bFallback,
    bool bUnsupportedFunctionFallback = false)
{
    auto& rStore = observeSurfaceStore();
    std::scoped_lock aGuard(rStore.maMutex);
    auto& rStatus = rStore.maCells[rFormulaPos];
    rStatus.maAddress = rFormulaPos;
    rStatus.mbSeen = true;
    rStatus.mbSupported = rStatus.mbSupported || bSupported;
    rStatus.mbFallback = rStatus.mbFallback || bFallback;
    rStatus.mbUnsupportedFunctionFallback
        = rStatus.mbUnsupportedFunctionFallback || bUnsupportedFunctionFallback;
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

[[nodiscard]] inline bool businessDayAmbientEnabled()
{
    if (const char* pValue
        = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_ENABLE_BUSINESSDAY"))
    {
        return envEnabled(pValue);
    }
    return true;
}

[[nodiscard]] inline bool authoritativeWhileOffEnabled()
{
    if (const char* pValue
        = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_AUTHORITATIVE_WHILE_OFF"))
    {
        return envEnabled(pValue);
    }
    return true;
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
    const auto tryCanonicalizeRootErrorLiteral = [](std::u16string_view rCandidate)
        -> std::optional<api::String> {
        const std::u16string_view aErrorPrefix = u"of:#ERR";
        if (rCandidate.rfind(aErrorPrefix, 0) == 0 && !rCandidate.empty()
            && rCandidate.back() == u'!')
        {
            return api::String(rCandidate);
        }

        std::u16string_view aBody = rCandidate;
        if (aBody.rfind(u"of:=", 0) == 0)
            aBody.remove_prefix(4);
        else if (aBody.rfind(u"of:", 0) == 0)
            aBody.remove_prefix(3);
        else
            return std::nullopt;

        const std::size_t nColon = aBody.find(u':');
        if (nColon == std::u16string_view::npos || nColon == 0 || nColon + 1 >= aBody.size())
            return std::nullopt;

        const std::u16string_view aToken = aBody.substr(0, nColon);
        const std::u16string_view aDigits = aBody.substr(nColon + 1);
        api::String aUpperToken;
        aUpperToken.reserve(aToken.size());
        for (const char16_t cChar : aToken)
        {
            if ((cChar < u'a' || cChar > u'z') && (cChar < u'A' || cChar > u'Z'))
                return std::nullopt;
            if (cChar >= u'a' && cChar <= u'z')
                aUpperToken.push_back(static_cast<char16_t>(cChar - u'a' + u'A'));
            else
                aUpperToken.push_back(cChar);
        }
        if (aUpperToken != u"ERR" && aUpperToken != u"ERROR" && aUpperToken != u"CHYBA")
            return std::nullopt;
        for (const char16_t cChar : aDigits)
        {
            if (cChar < u'0' || cChar > u'9')
                return std::nullopt;
        }

        api::String aCanonical(u"of:#ERR");
        aCanonical.append(aDigits.begin(), aDigits.end());
        aCanonical.push_back(u'!');
        return aCanonical;
    };

    if (rSource.rfind(u"of:=", 0) == 0 || rSource.rfind(u"of:", 0) == 0)
    {
        if (const auto oCanonical = tryCanonicalizeRootErrorLiteral(rSource))
            return *oCanonical;
        return api::String(rSource);
    }

    const std::u16string_view aTrimmed = trimFormulaEquals(rSource);
    if (aTrimmed.rfind(u"of:=", 0) == 0 || aTrimmed.rfind(u"of:", 0) == 0)
    {
        if (const auto oCanonical = tryCanonicalizeRootErrorLiteral(aTrimmed))
            return *oCanonical;
        return api::String(aTrimmed);
    }

    constexpr std::u16string_view aBracketedErrorPrefix = u"[.OF:.ERR]:";
    if (aTrimmed.rfind(aBracketedErrorPrefix, 0) == 0
        && aTrimmed.size() > aBracketedErrorPrefix.size())
    {
        api::String aCanonical(u"of:#ERR");
        aCanonical.append(aTrimmed.begin()
                              + static_cast<std::ptrdiff_t>(aBracketedErrorPrefix.size()),
            aTrimmed.end());
        aCanonical.push_back(u'!');
        return aCanonical;
    }

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

struct ArrayConstantSpan
{
    std::size_t mnBegin = 0;
    std::size_t mnEnd = 0;
};

[[nodiscard]] inline std::vector<ArrayConstantSpan> collectArrayConstantSpans(
    std::u16string_view rSource)
{
    std::vector<ArrayConstantSpan> aSpans;
    bool bInString = false;
    std::size_t nDepth = 0;
    std::size_t nStart = 0;
    for (std::size_t i = 0; i < rSource.size(); ++i)
    {
        const char16_t cChar = rSource[i];
        if (bInString)
        {
            if (cChar == u'"')
            {
                if (i + 1 < rSource.size() && rSource[i + 1] == u'"')
                    ++i;
                else
                    bInString = false;
            }
            continue;
        }

        if (cChar == u'"')
        {
            bInString = true;
            continue;
        }

        if (cChar == u'{')
        {
            if (nDepth == 0)
                nStart = i;
            ++nDepth;
            continue;
        }

        if (cChar == u'}' && nDepth > 0)
        {
            --nDepth;
            if (nDepth == 0)
                aSpans.push_back({ nStart, i + 1 });
        }
    }
    return aSpans;
}

[[nodiscard]] inline bool needsTokenBackedArrayRewrite(
    std::u16string_view rArrayText, const ScMatrix& rMatrix)
{
    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rMatrix.GetDimensions(nColumns, nRows);
    if (nColumns * nRows <= 1)
        return false;

    if (rArrayText.find(u';') != std::u16string_view::npos
        || rArrayText.find(u'|') != std::u16string_view::npos)
    {
        return false;
    }

    return true;
}

[[nodiscard]] inline std::optional<OUString> tryFormatCanonicalArrayScalar(
    const ScMatrix& rMatrix, SCSIZE nColumn, SCSIZE nRow)
{
    if (rMatrix.IsValue(nColumn, nRow))
    {
        const FormulaError eError = rMatrix.GetError(nColumn, nRow);
        if (eError != FormulaError::NONE)
            return std::nullopt;
        return OUString::number(rMatrix.GetDouble(nColumn, nRow));
    }

    if (rMatrix.IsStringOrEmpty(nColumn, nRow))
    {
        OUString aString = rMatrix.GetString(nColumn, nRow).getString();
        aString = aString.replaceAll(u"\""_ustr, u"\"\""_ustr);
        return u"\""_ustr + aString + u"\""_ustr;
    }

    return std::nullopt;
}

[[nodiscard]] inline std::optional<OUString> tryBuildCanonicalArrayConstant(
    const ScMatrix& rMatrix)
{
    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rMatrix.GetDimensions(nColumns, nRows);
    if (!nColumns || !nRows)
        return std::nullopt;

    OUStringBuffer aBuffer;
    aBuffer.append(u'{');
    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        if (nRow)
            aBuffer.append(u'|');
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
        {
            if (nColumn)
                aBuffer.append(u';');
            const auto oScalar = tryFormatCanonicalArrayScalar(rMatrix, nColumn, nRow);
            if (!oScalar)
                return std::nullopt;
            aBuffer.append(*oScalar);
        }
    }
    aBuffer.append(u'}');
    return aBuffer.makeStringAndClear();
}

[[nodiscard]] inline api::String maybeCanonicalizeArrayConstantsFromTokens(
    api::StringView rNormalizedSource, const ScTokenArray* pTokenArray)
{
    if (!pTokenArray)
        return api::String(rNormalizedSource);

    std::vector<const ScMatrix*> aMatrices;
    for (formula::FormulaToken* pToken : pTokenArray->Tokens())
    {
        if (!pToken || pToken->GetType() != formula::svMatrix)
            continue;

        const ScMatrix* pMatrix = pToken->GetMatrix();
        if (!pMatrix)
            return api::String(rNormalizedSource);
        aMatrices.push_back(pMatrix);
    }

    if (aMatrices.empty())
        return api::String(rNormalizedSource);

    const auto aSpans = collectArrayConstantSpans(rNormalizedSource);
    if (aSpans.empty() || aSpans.size() != aMatrices.size())
        return api::String(rNormalizedSource);

    OUStringBuffer aBuffer;
    std::size_t nCursor = 0;
    bool bRewrote = false;
    for (std::size_t i = 0; i < aSpans.size(); ++i)
    {
        const auto& rSpan = aSpans[i];
        aBuffer.append(OUString(rNormalizedSource.substr(nCursor, rSpan.mnBegin - nCursor)));

        const std::u16string_view aArrayText
            = rNormalizedSource.substr(rSpan.mnBegin, rSpan.mnEnd - rSpan.mnBegin);
        if (!needsTokenBackedArrayRewrite(aArrayText, *aMatrices[i]))
        {
            aBuffer.append(OUString(aArrayText));
        }
        else if (const auto oCanonical = tryBuildCanonicalArrayConstant(*aMatrices[i]))
        {
            aBuffer.append(*oCanonical);
            bRewrote = true;
        }
        else
        {
            aBuffer.append(OUString(aArrayText));
        }

        nCursor = rSpan.mnEnd;
    }
    aBuffer.append(OUString(rNormalizedSource.substr(nCursor)));

    return bRewrote ? toApiString(aBuffer.makeStringAndClear()) : api::String(rNormalizedSource);
}

[[nodiscard]] inline std::optional<api::StringView>
canonicalMathScalarFunctionName(api::StringView rFunctionName)
{
    static constexpr api::StringView aScalarNames[] = {
        u"ABS", u"PI", u"DEGREES", u"RADIANS", u"SIN", u"COS", u"TAN", u"COT", u"ASIN",
        u"ACOS", u"ATAN", u"ACOT", u"ATAN2", u"SINH", u"COSH", u"TANH", u"COTH", u"ASINH",
        u"ACOSH", u"ATANH", u"ACOTH", u"EVEN", u"ODD", u"COLOR", u"SIGN", u"INT", u"GCD",
        u"LCM", u"CEILING", u"FLOOR", u"CEILING.XCL", u"FLOOR.XCL", u"CEILING.MATH",
        u"FLOOR.MATH", u"CEILING.PRECISE", u"FLOOR.PRECISE", u"ISO.CEILING", u"ROUNDSIG",
        u"BITAND", u"BITOR", u"BITXOR", u"BITLSHIFT", u"BITRSHIFT", u"POWER", u"LOG",
        u"LOG10", u"LN", u"MROUND", u"COMBIN", u"COMBINA", u"CSC", u"CSCH", u"SEC", u"SECH",
        u"EXP", u"SQRT", u"FACT", u"TRUNC", u"MOD", u"RAWSUBTRACT"
    };

    for (const auto aName : aScalarNames)
    {
        if (rFunctionName == aName)
            return aName;
    }

    if (rFunctionName == u"COM.MICROSOFT.CEILING")
        return api::StringView(u"CEILING.XCL");
    if (rFunctionName == u"COM.MICROSOFT.FLOOR")
        return api::StringView(u"FLOOR.XCL");
    if (rFunctionName == u"COM.MICROSOFT.CEILING.PRECISE")
        return api::StringView(u"CEILING.PRECISE");
    if (rFunctionName == u"COM.MICROSOFT.FLOOR.PRECISE")
        return api::StringView(u"FLOOR.PRECISE");
    if (rFunctionName == u"COM.MICROSOFT.ISO.CEILING")
        return api::StringView(u"ISO.CEILING");
    if (rFunctionName == u"ORG.LIBREOFFICE.ROUNDSIG")
        return api::StringView(u"ROUNDSIG");
    if (rFunctionName == u"ORG.LIBREOFFICE.RAWSUBTRACT")
        return api::StringView(u"RAWSUBTRACT");

    return std::nullopt;
}

[[nodiscard]] inline std::optional<api::StringView>
canonicalConversionFunctionName(api::StringView rFunctionName)
{
    if (rFunctionName == u"CONVERT" || rFunctionName == u"ORG.OPENOFFICE.CONVERT"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCONVERT")
    {
        return api::StringView(u"CONVERT");
    }
    if (rFunctionName == u"EUROCONVERT")
        return api::StringView(u"EUROCONVERT");
    if (rFunctionName == u"DECIMAL")
        return api::StringView(u"DECIMAL");
    if (rFunctionName == u"DEC2HEX"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDEC2HEX")
    {
        return api::StringView(u"DEC2HEX");
    }
    if (rFunctionName == u"BASE")
        return api::StringView(u"BASE");
    if (rFunctionName == u"ROMAN")
        return api::StringView(u"ROMAN");
    if (rFunctionName == u"ARABIC")
        return api::StringView(u"ARABIC");

    return std::nullopt;
}

[[nodiscard]] inline std::optional<api::StringView>
canonicalSelectorFunctionName(api::StringView rFunctionName)
{
    if (rFunctionName == u"CHOOSECOLS" || rFunctionName == u"COM.MICROSOFT.CHOOSECOLS")
        return api::StringView(u"CHOOSECOLS");
    if (rFunctionName == u"CHOOSEROWS" || rFunctionName == u"COM.MICROSOFT.CHOOSEROWS")
        return api::StringView(u"CHOOSEROWS");

    return std::nullopt;
}

[[nodiscard]] inline std::optional<api::StringView>
canonicalSpillFunctionName(api::StringView rFunctionName)
{
    if (rFunctionName == u"UNIQUE" || rFunctionName == u"COM.MICROSOFT.UNIQUE")
        return api::StringView(u"UNIQUE");
    if (rFunctionName == u"SORT" || rFunctionName == u"COM.MICROSOFT.SORT")
        return api::StringView(u"SORT");
    if (rFunctionName == u"SORTBY" || rFunctionName == u"COM.MICROSOFT.SORTBY")
        return api::StringView(u"SORTBY");
    if (rFunctionName == u"TEXTSPLIT" || rFunctionName == u"COM.MICROSOFT.TEXTSPLIT")
        return api::StringView(u"TEXTSPLIT");
    if (rFunctionName == u"HSTACK" || rFunctionName == u"COM.MICROSOFT.HSTACK")
        return api::StringView(u"HSTACK");

    return std::nullopt;
}

[[nodiscard]] inline std::optional<FunctionKind>
classifyImportedStoredHostTruthFunction(api::StringView rFunctionName)
{
    if (rFunctionName == u"ORG.LIBREOFFICE.FOURIER")
        return FunctionKind::MatrixMath;
    if (rFunctionName == u"GETPIVOTDATA" || rFunctionName == u"OFFSET"
        || rFunctionName == u"INDIRECT")
    {
        return FunctionKind::Lookup;
    }
    if (rFunctionName == u"ADDRESS" || rFunctionName == u"REPLACEB" || rFunctionName == u"LENB"
        || rFunctionName == u"BASISODATETIME" || rFunctionName == u"MID"
        || rFunctionName == u"FINDB" || rFunctionName == u"HYPERLINK"
        || rFunctionName == u"SEARCHB" || rFunctionName == u"TEXT"
        || rFunctionName == u"REPLACE" || rFunctionName == u"SEARCH"
        || rFunctionName == u"COM.MICROSOFT.BAHTTEXT"
        || rFunctionName == u"COM.MICROSOFT.TEXTJOIN"
        || rFunctionName == u"FIND" || rFunctionName == u"SUBSTITUTE"
        || rFunctionName == u"ORG.LIBREOFFICE.REGEX" || rFunctionName == u"LEFTB"
        || rFunctionName == u"COM.MICROSOFT.CONCAT" || rFunctionName == u"DOLLAR"
        || rFunctionName == u"FIXED" || rFunctionName == u"RIGHTB")
    {
        return FunctionKind::TextUtility;
    }
    if (rFunctionName == u"DSUM" || rFunctionName == u"DCOUNT"
        || rFunctionName == u"COM.MICROSOFT.MAXIFS"
        || rFunctionName == u"DCOUNTA" || rFunctionName == u"DGET"
        || rFunctionName == u"DVAR" || rFunctionName == u"DVARP")
    {
        return FunctionKind::CriteriaAggregate;
    }
    if (rFunctionName == u"COM.MICROSOFT.CEILING.MATH"
        || rFunctionName == u"COM.MICROSOFT.FLOOR.MATH"
        || rFunctionName == u"FACTDOUBLE" || rFunctionName == u"IMSUB"
        || rFunctionName == u"IMPRODUCT" || rFunctionName == u"IMSUM"
        || rFunctionName == u"ISODD" || rFunctionName == u"N"
        || rFunctionName == u"ISEVEN" || rFunctionName == u"COMPLEX"
        || rFunctionName == u"IMDIV" || rFunctionName == u"DELTA"
        || rFunctionName == u"GESTEP" || rFunctionName == u"IMPOWER"
        || rFunctionName == u"IMARGUMENT" || rFunctionName == u"IMCONJUGATE"
        || rFunctionName == u"IMCSCH" || rFunctionName == u"IMEXP"
        || rFunctionName == u"IMLN" || rFunctionName == u"IMLOG10"
        || rFunctionName == u"IMLOG2" || rFunctionName == u"IMSEC"
        || rFunctionName == u"IMSECH" || rFunctionName == u"IMSIN"
        || rFunctionName == u"QUOTIENT" || rFunctionName == u"IMABS"
        || rFunctionName == u"IMCOS" || rFunctionName == u"IMCOSH"
        || rFunctionName == u"IMCOT" || rFunctionName == u"IMCSC"
        || rFunctionName == u"IMSINH" || rFunctionName == u"IMSQRT"
        || rFunctionName == u"IMTAN")
    {
        return FunctionKind::MathScalar;
    }
    if (rFunctionName == u"SKEW" || rFunctionName == u"SKEWP"
        || rFunctionName == u"TRIMMEAN"
        || rFunctionName == u"MODE.SNGL"
        || rFunctionName == u"COM.MICROSOFT.MODE.SNGL"
        || rFunctionName == u"KURT" || rFunctionName == u"AVERAGEA"
        || rFunctionName == u"AVEDEV")
    {
        return FunctionKind::StatisticalAggregate;
    }
    if (rFunctionName == u"COM.MICROSOFT.CHISQ.TEST"
        || rFunctionName == u"COM.MICROSOFT.T.TEST"
        || rFunctionName == u"LEGACY.CHITEST" || rFunctionName == u"TTEST"
        || rFunctionName == u"NORMDIST" || rFunctionName == u"COM.MICROSOFT.BETA.DIST"
        || rFunctionName == u"HYPGEOMDIST"
        || rFunctionName == u"COM.MICROSOFT.BINOM.INV"
        || rFunctionName == u"LOGNORMDIST" || rFunctionName == u"BETAINV"
        || rFunctionName == u"COM.MICROSOFT.BETA.INV"
        || rFunctionName == u"COM.MICROSOFT.HYPGEOM.DIST"
        || rFunctionName == u"COM.MICROSOFT.NORM.DIST" || rFunctionName == u"ZTEST"
        || rFunctionName == u"COM.MICROSOFT.Z.TEST"
        || rFunctionName == u"COM.MICROSOFT.GAMMA.DIST"
        || rFunctionName == u"COM.MICROSOFT.NEGBINOM.DIST"
        || rFunctionName == u"CRITBINOM" || rFunctionName == u"NORMINV"
        || rFunctionName == u"COM.MICROSOFT.BINOM.DIST"
        || rFunctionName == u"NEGBINOMDIST"
        || rFunctionName == u"COM.MICROSOFT.CONFIDENCE.NORM"
        || rFunctionName == u"COM.MICROSOFT.CONFIDENCE.T"
        || rFunctionName == u"CONFIDENCE" || rFunctionName == u"LEGACY.CHIINV"
        || rFunctionName == u"COM.MICROSOFT.LOGNORM.INV"
        || rFunctionName == u"FTEST"
        || rFunctionName == u"COM.MICROSOFT.LOGNORM.DIST"
        || rFunctionName == u"COM.MICROSOFT.NORM.INV"
        || rFunctionName == u"COM.MICROSOFT.CHISQ.DIST"
        || rFunctionName == u"LOGINV")
    {
        return FunctionKind::StatisticalDistribution;
    }
    if (rFunctionName == u"SLOPE" || rFunctionName == u"ORG.LIBREOFFICE.FORECAST.ETS.MULT"
        || rFunctionName == u"COM.MICROSOFT.FORECAST.ETS"
        || rFunctionName == u"RSQ" || rFunctionName == u"STEYX")
        return FunctionKind::GrowthProjection;
    if (rFunctionName == u"YEARFRAC" || rFunctionName == u"DAYS360")
        return FunctionKind::DateDifference;
    if (rFunctionName == u"WEEKNUM" || rFunctionName == u"WEEKDAY"
        || rFunctionName == u"EOMONTH"
        || rFunctionName == u"SECOND" || rFunctionName == u"TIME"
        || rFunctionName == u"MINUTE" || rFunctionName == u"HOUR")
    {
        return FunctionKind::CalendarUtility;
    }
    if (rFunctionName == u"DEC2OCT" || rFunctionName == u"HEX2BIN"
        || rFunctionName == u"HEX2DEC" || rFunctionName == u"HEX2OCT"
        || rFunctionName == u"DEC2BIN" || rFunctionName == u"BIN2HEX"
        || rFunctionName == u"OCT2BIN" || rFunctionName == u"OCT2DEC"
        || rFunctionName == u"OCT2HEX" || rFunctionName == u"BIN2OCT"
        || rFunctionName == u"BIN2DEC")
    {
        return FunctionKind::Conversion;
    }
    if (rFunctionName == u"ISPMT" || rFunctionName == u"ODDLYIELD"
        || rFunctionName == u"PMT" || rFunctionName == u"AMORLINC"
        || rFunctionName == u"DDB" || rFunctionName == u"ODDLPRICE"
        || rFunctionName == u"CUMPRINC" || rFunctionName == u"CUMIPMT"
        || rFunctionName == u"MDURATION" || rFunctionName == u"PPMT"
        || rFunctionName == u"TBILLPRICE" || rFunctionName == u"TBILLYIELD"
        || rFunctionName == u"XIRR" || rFunctionName == u"DB"
        || rFunctionName == u"YIELD" || rFunctionName == u"DISC"
        || rFunctionName == u"FV" || rFunctionName == u"IPMT"
        || rFunctionName == u"RECEIVED" || rFunctionName == u"INTRATE"
        || rFunctionName == u"AMORDEGRC" || rFunctionName == u"PRICEDISC"
        || rFunctionName == u"IRR" || rFunctionName == u"PRICEMAT"
        || rFunctionName == u"YIELDDISC" || rFunctionName == u"MIRR"
        || rFunctionName == u"NPER" || rFunctionName == u"PV"
        || rFunctionName == u"COUPDAYS" || rFunctionName == u"COUPDAYBS"
        || rFunctionName == u"COUPDAYSNC" || rFunctionName == u"COUPNCD"
        || rFunctionName == u"COUPNUM" || rFunctionName == u"COUPPCD"
        || rFunctionName == u"NOMINAL" || rFunctionName == u"SLN"
        || rFunctionName == u"SYD" || rFunctionName == u"ACCRINTM"
        || rFunctionName == u"RRI" || rFunctionName == u"NPV"
        || rFunctionName == u"TBILLEQ" || rFunctionName == u"PDURATION")
    {
        return FunctionKind::Rate;
    }
    if (rFunctionName == u"SUBTOTAL")
        return FunctionKind::Aggregate;
    if (rFunctionName == u"COM.MICROSOFT.LET" || rFunctionName == u"CHOOSE")
        return FunctionKind::Conditional;
    if (rFunctionName == u"COM.MICROSOFT.EXPAND"
        || rFunctionName == u"COM.MICROSOFT.DROP"
        || rFunctionName == u"COM.MICROSOFT.TAKE"
        || rFunctionName == u"COM.MICROSOFT.WRAPCOLS"
        || rFunctionName == u"COM.MICROSOFT.WRAPROWS"
        || rFunctionName == u"COM.MICROSOFT.VSTACK"
        || rFunctionName == u"COM.MICROSOFT.SEQUENCE"
        || rFunctionName == u"COM.MICROSOFT.FILTER")
    {
        return FunctionKind::SpillArray;
    }
    if (rFunctionName == u"ORG.OPENOFFICE.ERRORTYPE" || rFunctionName == u"ISREF")
        return FunctionKind::InformationPredicate;
    if (rFunctionName == u"COUNT" || rFunctionName == u"COUNTA")
        return FunctionKind::NumericAggregate;
    if (rFunctionName == u"ROW" || rFunctionName == u"COLUMN"
        || rFunctionName == u"COLUMNS" || rFunctionName == u"SHEETS"
        || rFunctionName == u"TYPE")
    {
        return FunctionKind::ScalarRoot;
    }
    if (rFunctionName == u"COM.MICROSOFT.PERCENTILE.INC"
        || rFunctionName == u"COM.MICROSOFT.PERCENTILE.EXC"
        || rFunctionName == u"COM.MICROSOFT.MODE.MULT"
        || rFunctionName == u"PERCENTILE" || rFunctionName == u"MODE")
    {
        return FunctionKind::RankedAggregate;
    }
    if (rFunctionName == u"FREQUENCY")
        return FunctionKind::MatrixMath;

    return std::nullopt;
}

[[nodiscard]] inline FunctionKind classifyFunction(api::StringView rFunctionName)
{
    if (rFunctionName == u"IF" || rFunctionName == u"IFS"
        || rFunctionName == u"COM.MICROSOFT.IFS" || rFunctionName == u"SWITCH"
        || rFunctionName == u"COM.MICROSOFT.SWITCH")
    {
        return FunctionKind::Conditional;
    }
    if (rFunctionName == u"FORMULA")
        return FunctionKind::FormulaText;
    if (rFunctionName == u"TRUE" || rFunctionName == u"FALSE")
        return FunctionKind::LogicalConstant;
    if (rFunctionName == u"NA")
        return FunctionKind::ScalarRoot;
    if (rFunctionName == u"CONCATENATE" || rFunctionName == u"CONCAT"
        || rFunctionName == u"CLEAN" || rFunctionName == u"CHAR"
        || rFunctionName == u"CODE" || rFunctionName == u"UNICHAR"
        || rFunctionName == u"UNICODE" || rFunctionName == u"UPPER"
        || rFunctionName == u"LOWER" || rFunctionName == u"PROPER"
        || rFunctionName == u"ASC" || rFunctionName == u"JIS"
        || rFunctionName == u"LEN" || rFunctionName == u"LEFT"
        || rFunctionName == u"RIGHT" || rFunctionName == u"T"
        || rFunctionName == u"EXACT" || rFunctionName == u"TEXTAFTER"
        || rFunctionName == u"COM.MICROSOFT.TEXTAFTER" || rFunctionName == u"TEXTBEFORE"
        || rFunctionName == u"COM.MICROSOFT.TEXTBEFORE")
    {
        return FunctionKind::TextUtility;
    }
    if (rFunctionName == u"VALUE")
        return FunctionKind::Value;
    if (rFunctionName == u"DATEVALUE")
        return FunctionKind::DateValue;
    if (rFunctionName == u"TIMEVALUE")
        return FunctionKind::TimeValue;
    if (rFunctionName == u"NUMBERVALUE")
        return FunctionKind::NumberValue;
    if (rFunctionName == u"RATE" || rFunctionName == u"PV" || rFunctionName == u"FV"
        || rFunctionName == u"PMT" || rFunctionName == u"NPV" || rFunctionName == u"RRI"
        || rFunctionName == u"ISPMT" || rFunctionName == u"IPMT"
        || rFunctionName == u"PPMT" || rFunctionName == u"CUMIPMT"
        || rFunctionName == u"CUMPRINC" || rFunctionName == u"DDB"
        || rFunctionName == u"DB" || rFunctionName == u"VDB"
        || rFunctionName == u"SLN" || rFunctionName == u"SYD"
        || rFunctionName == u"EFFECT" || rFunctionName == u"NOMINAL"
        || rFunctionName == u"PDURATION" || rFunctionName == u"PRICE"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETPRICE")
        return FunctionKind::Rate;
    if (rFunctionName == u"ROUND" || rFunctionName == u"ROUNDUP" || rFunctionName == u"ROUNDDOWN")
        return FunctionKind::Round;
    if (canonicalConversionFunctionName(rFunctionName))
        return FunctionKind::Conversion;
    if (rFunctionName == u"SUM" || rFunctionName == u"PRODUCT" || rFunctionName == u"SUMSQ"
        || rFunctionName == u"AVERAGE" || rFunctionName == u"DEVSQ"
        || rFunctionName == u"MULTINOMIAL" || rFunctionName == u"SUMPRODUCT"
        || rFunctionName == u"SUMX2MY2"
        || rFunctionName == u"SUMX2PY2" || rFunctionName == u"SUMXMY2")
    {
        return FunctionKind::NumericAggregate;
    }
    if (rFunctionName == u"QUARTILE" || rFunctionName == u"QUARTILE.INC"
        || rFunctionName == u"COM.MICROSOFT.QUARTILE.INC"
        || rFunctionName == u"QUARTILE.EXC"
        || rFunctionName == u"COM.MICROSOFT.QUARTILE.EXC"
        || rFunctionName == u"LARGE" || rFunctionName == u"SMALL"
        || rFunctionName == u"PERCENTRANK" || rFunctionName == u"PERCENTRANK.INC"
        || rFunctionName == u"COM.MICROSOFT.PERCENTRANK.INC"
        || rFunctionName == u"PERCENTRANK.EXC"
        || rFunctionName == u"COM.MICROSOFT.PERCENTRANK.EXC"
        || rFunctionName == u"RANK" || rFunctionName == u"RANK.EQ"
        || rFunctionName == u"COM.MICROSOFT.RANK.EQ"
        || rFunctionName == u"RANK.AVG"
        || rFunctionName == u"COM.MICROSOFT.RANK.AVG")
    {
        return FunctionKind::RankedAggregate;
    }
    if (rFunctionName == u"MAX" || rFunctionName == u"MAXA" || rFunctionName == u"MIN"
        || rFunctionName == u"MINA" || rFunctionName == u"MEDIAN"
        || rFunctionName == u"GEOMEAN" || rFunctionName == u"HARMEAN"
        || rFunctionName == u"VAR" || rFunctionName == u"VAR.S"
        || rFunctionName == u"COM.MICROSOFT.VAR.S"
        || rFunctionName == u"VARP" || rFunctionName == u"VAR.P"
        || rFunctionName == u"COM.MICROSOFT.VAR.P"
        || rFunctionName == u"VARA" || rFunctionName == u"VARPA"
        || rFunctionName == u"STDEV" || rFunctionName == u"STDEV.S"
        || rFunctionName == u"COM.MICROSOFT.STDEV.S"
        || rFunctionName == u"STDEVP" || rFunctionName == u"STDEV.P"
        || rFunctionName == u"COM.MICROSOFT.STDEV.P"
        || rFunctionName == u"STDEVA" || rFunctionName == u"STDEVPA")
    {
        return FunctionKind::StatisticalAggregate;
    }
    if (rFunctionName == u"FISHER" || rFunctionName == u"FISHERINV"
        || rFunctionName == u"GAUSS" || rFunctionName == u"PHI"
        || rFunctionName == u"GAMMA" || rFunctionName == u"COM.MICROSOFT.GAMMA"
        || rFunctionName == u"GAMMALN" || rFunctionName == u"GAMMALN.PRECISE"
        || rFunctionName == u"COM.MICROSOFT.GAMMALN.PRECISE"
        || rFunctionName == u"ERF" || rFunctionName == u"ERF.PRECISE"
        || rFunctionName == u"COM.MICROSOFT.ERF.PRECISE"
        || rFunctionName == u"ERFC" || rFunctionName == u"ERFC.PRECISE"
        || rFunctionName == u"COM.MICROSOFT.ERFC.PRECISE"
        || rFunctionName == u"BESSELI"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELI"
        || rFunctionName == u"BESSELJ"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELJ"
        || rFunctionName == u"BESSELK"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELK"
        || rFunctionName == u"BESSELY"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELY"
        || rFunctionName == u"LEGACY.NORMSDIST" || rFunctionName == u"NORMSDIST"
        || rFunctionName == u"NORM.S.DIST" || rFunctionName == u"COM.MICROSOFT.NORM.S.DIST"
        || rFunctionName == u"LEGACY.NORMSINV" || rFunctionName == u"NORMSINV"
        || rFunctionName == u"NORM.S.INV" || rFunctionName == u"COM.MICROSOFT.NORM.S.INV"
        || rFunctionName == u"STANDARDIZE"
        || rFunctionName == u"GAMMAINV" || rFunctionName == u"GAMMA.INV"
        || rFunctionName == u"COM.MICROSOFT.GAMMA.INV"
        || rFunctionName == u"EXPONDIST" || rFunctionName == u"EXPON.DIST"
        || rFunctionName == u"COM.MICROSOFT.EXPON.DIST"
        || rFunctionName == u"WEIBULL" || rFunctionName == u"WEIBULL.DIST"
        || rFunctionName == u"COM.MICROSOFT.WEIBULL.DIST"
        || rFunctionName == u"PERMUT" || rFunctionName == u"PERMUTATIONA"
        || rFunctionName == u"POISSON" || rFunctionName == u"POISSON.DIST"
        || rFunctionName == u"COM.MICROSOFT.POISSON.DIST"
        || rFunctionName == u"LEGACY.CHIDIST" || rFunctionName == u"CHISQDIST"
        || rFunctionName == u"CHISQ.DIST" || rFunctionName == u"CHISQ.DIST.RT"
        || rFunctionName == u"COM.MICROSOFT.CHISQ.DIST.RT"
        || rFunctionName == u"CHISQINV" || rFunctionName == u"CHISQ.INV"
        || rFunctionName == u"COM.MICROSOFT.CHISQ.INV"
        || rFunctionName == u"CHISQ.INV.RT"
        || rFunctionName == u"COM.MICROSOFT.CHISQ.INV.RT"
        || rFunctionName == u"GAMMADIST" || rFunctionName == u"GAMMA.DIST"
        || rFunctionName == u"TDIST" || rFunctionName == u"LEGACY.TDIST"
        || rFunctionName == u"T.DIST" || rFunctionName == u"COM.MICROSOFT.T.DIST"
        || rFunctionName == u"T.DIST.2T"
        || rFunctionName == u"COM.MICROSOFT.T.DIST.2T"
        || rFunctionName == u"T.DIST.RT"
        || rFunctionName == u"COM.MICROSOFT.T.DIST.RT"
        || rFunctionName == u"TINV" || rFunctionName == u"T.INV"
        || rFunctionName == u"COM.MICROSOFT.T.INV" || rFunctionName == u"T.INV.2T"
        || rFunctionName == u"COM.MICROSOFT.T.INV.2T"
        || rFunctionName == u"FDIST" || rFunctionName == u"LEGACY.FDIST"
        || rFunctionName == u"F.DIST" || rFunctionName == u"COM.MICROSOFT.F.DIST"
        || rFunctionName == u"F.DIST.RT"
        || rFunctionName == u"COM.MICROSOFT.F.DIST.RT"
        || rFunctionName == u"FINV" || rFunctionName == u"LEGACY.FINV"
        || rFunctionName == u"F.INV" || rFunctionName == u"COM.MICROSOFT.F.INV"
        || rFunctionName == u"F.INV.RT"
        || rFunctionName == u"COM.MICROSOFT.F.INV.RT"
        || rFunctionName == u"BINOMDIST" || rFunctionName == u"BINOM.DIST"
        || rFunctionName == u"BINOM.DIST.RANGE" || rFunctionName == u"B"
        || rFunctionName == u"INTERCEPT" || rFunctionName == u"FORECAST"
        || rFunctionName == u"BETADIST" || rFunctionName == u"BETA.DIST"
        || rFunctionName == u"PROB")
    {
        return FunctionKind::StatisticalDistribution;
    }
    if (rFunctionName == u"GROWTH")
        return FunctionKind::GrowthProjection;
    if (rFunctionName == u"COUNTIF" || rFunctionName == u"COUNTIFS"
        || rFunctionName == u"SUMIF" || rFunctionName == u"SUMIFS"
        || rFunctionName == u"AVERAGEIF" || rFunctionName == u"AVERAGEIFS"
        || rFunctionName == u"MAXIFS" || rFunctionName == u"MINIFS")
    {
        return FunctionKind::CriteriaAggregate;
    }
    if (rFunctionName == u"AGGREGATE" || rFunctionName == u"COM.MICROSOFT.AGGREGATE")
        return FunctionKind::Aggregate;
    if (rFunctionName == u"WORKDAY" || rFunctionName == u"NETWORKDAYS"
        || rFunctionName == u"WORKDAY.INTL"
        || rFunctionName == u"COM.MICROSOFT.WORKDAY.INTL"
        || rFunctionName == u"NETWORKDAYS.INTL"
        || rFunctionName == u"COM.MICROSOFT.NETWORKDAYS.INTL")
    {
        return FunctionKind::BusinessDay;
    }
    if (rFunctionName == u"DAYSINMONTH" || rFunctionName == u"ORG.OPENOFFICE.DAYSINMONTH"
        || rFunctionName == u"DAYSINYEAR" || rFunctionName == u"ORG.OPENOFFICE.DAYSINYEAR"
        || rFunctionName == u"ISLEAPYEAR" || rFunctionName == u"ORG.OPENOFFICE.ISLEAPYEAR"
        || rFunctionName == u"ISOWEEKNUM"
        || rFunctionName == u"EASTERSUNDAY" || rFunctionName == u"ORG.OPENOFFICE.EASTERSUNDAY"
        || rFunctionName == u"WEEKSINYEAR"
        || rFunctionName == u"ORG.OPENOFFICE.WEEKSINYEAR")
    {
        return FunctionKind::CalendarUtility;
    }
    if (rFunctionName == u"MONTHS" || rFunctionName == u"ORG.OPENOFFICE.MONTHS"
        || rFunctionName == u"YEARS" || rFunctionName == u"ORG.OPENOFFICE.YEARS"
        || rFunctionName == u"WEEKS" || rFunctionName == u"ORG.OPENOFFICE.WEEKS"
        || rFunctionName == u"DATEDIF")
    {
        return FunctionKind::DateDifference;
    }
    if (rFunctionName == u"DATE" || rFunctionName == u"YEAR" || rFunctionName == u"MONTH"
        || rFunctionName == u"DAY")
    {
        return FunctionKind::DateConstructExtract;
    }
    if (rFunctionName == u"MDETERM")
        return FunctionKind::MatrixMath;
    if (rFunctionName == u"IMREAL"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMREAL"
        || rFunctionName == u"IMAGINARY"
        || rFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMAGINARY")
    {
        return FunctionKind::MathScalar;
    }
    if (canonicalMathScalarFunctionName(rFunctionName))
        return FunctionKind::MathScalar;
    if (rFunctionName == u"ISERROR" || rFunctionName == u"ISERR" || rFunctionName == u"ISNUMBER"
        || rFunctionName == u"ISNA" || rFunctionName == u"ISTEXT"
        || rFunctionName == u"ISNONTEXT" || rFunctionName == u"ISBLANK"
        || rFunctionName == u"ERROR.TYPE" || rFunctionName == u"ERRORTYPE")
    {
        return FunctionKind::InformationPredicate;
    }
    if (rFunctionName == u"AND" || rFunctionName == u"OR" || rFunctionName == u"XOR")
        return FunctionKind::LogicalFold;
    if (rFunctionName == u"NOT")
        return FunctionKind::Not;
    if (rFunctionName == u"MATCH")
        return FunctionKind::Match;
    if (rFunctionName == u"XMATCH" || rFunctionName == u"COM.MICROSOFT.XMATCH")
        return FunctionKind::XMatch;
    if (canonicalSelectorFunctionName(rFunctionName))
        return FunctionKind::Selector;
    if (canonicalSpillFunctionName(rFunctionName))
        return FunctionKind::SpillArray;
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
    if (const auto oStoredHostTruthFunction
        = classifyImportedStoredHostTruthFunction(rFunctionName))
    {
        return *oStoredHostTruthFunction;
    }
    return FunctionKind::Unknown;
}

[[nodiscard]] inline bool importedRootUsesVariableExpectedHostTruth(
    FunctionKind eFunction, api::StringView rFunctionName)
{
    switch (eFunction)
    {
        case FunctionKind::ScalarRoot:
            return false;
        case FunctionKind::LogicalConstant:
            return true;
        case FunctionKind::Round:
        case FunctionKind::StatisticalDistribution:
        case FunctionKind::Aggregate:
        case FunctionKind::CalendarUtility:
        case FunctionKind::Match:
        case FunctionKind::Lookup:
        case FunctionKind::VLookup:
        case FunctionKind::HLookup:
        case FunctionKind::XLookup:
            return true;
        case FunctionKind::Conversion:
            return rFunctionName != u"DEC2HEX"
                   && rFunctionName != u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDEC2HEX";
        case FunctionKind::TextUtility:
            return rFunctionName != u"TEXTAFTER"
                   && rFunctionName != u"COM.MICROSOFT.TEXTAFTER"
                   && rFunctionName != u"TEXTBEFORE"
                   && rFunctionName != u"COM.MICROSOFT.TEXTBEFORE";
        case FunctionKind::Unknown:
        case FunctionKind::Conditional:
        case FunctionKind::FormulaText:
        case FunctionKind::Value:
        case FunctionKind::DateValue:
        case FunctionKind::TimeValue:
        case FunctionKind::NumberValue:
        case FunctionKind::Rate:
        case FunctionKind::NumericAggregate:
        case FunctionKind::RankedAggregate:
        case FunctionKind::StatisticalAggregate:
        case FunctionKind::GrowthProjection:
        case FunctionKind::CriteriaAggregate:
        case FunctionKind::BusinessDay:
        case FunctionKind::DateDifference:
        case FunctionKind::DateConstructExtract:
        case FunctionKind::MatrixMath:
        case FunctionKind::MathScalar:
        case FunctionKind::InformationPredicate:
        case FunctionKind::LogicalFold:
        case FunctionKind::Not:
        case FunctionKind::XMatch:
        case FunctionKind::Selector:
        case FunctionKind::SpillArray:
        case FunctionKind::Index:
        case FunctionKind::Count:
            return false;
    }

    return false;
}

[[nodiscard]] inline bool isImportedStoredHostValueTruthFunctionName(api::StringView rFunctionName)
{
    static constexpr api::StringView aStoredValueFunctions[] = {
        u"TRUE",
        u"FALSE",
        u"NA",
        u"IMREAL",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMREAL",
        u"IMAGINARY",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMAGINARY",
        u"BESSELI",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELI",
        u"BESSELJ",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELJ",
        u"BESSELK",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELK",
        u"BESSELY",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELY",
        u"ORG.LIBREOFFICE.FOURIER",
        u"GETPIVOTDATA",
        u"ADDRESS",
        u"DSUM",
        u"REPLACEB",
        u"COM.MICROSOFT.CEILING.MATH",
        u"COM.MICROSOFT.FLOOR.MATH",
        u"LENB",
        u"SKEW",
        u"SKEWP",
        u"YEARFRAC",
        u"DEC2HEX",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDEC2HEX",
        u"DEC2OCT",
        u"COM.MICROSOFT.CHISQ.TEST",
        u"COM.MICROSOFT.T.TEST",
        u"ISPMT",
        u"LEGACY.CHITEST",
        u"TTEST",
        u"NORMDIST",
        u"SUBTOTAL",
        u"FACTDOUBLE",
        u"OFFSET",
        u"BASISODATETIME",
        u"COM.MICROSOFT.BETA.DIST",
        u"HYPGEOMDIST",
        u"DATEDIF",
        u"ODDLYIELD",
        u"AMORLINC",
        u"PMT",
        u"VDB",
        u"PRICE",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETPRICE",
        u"COM.MICROSOFT.BINOM.INV",
        u"MID",
        u"DDB",
        u"FINDB",
        u"HEX2BIN",
        u"HEX2DEC",
        u"HEX2OCT",
        u"HYPERLINK",
        u"IMSUB",
        u"LOGNORMDIST",
        u"ODDLPRICE",
        u"COM.MICROSOFT.LET",
        u"CUMPRINC",
        u"IMPRODUCT",
        u"IMSUM",
        u"SEARCHB",
        u"SUMPRODUCT",
        u"TEXT",
        u"CUMIPMT",
        u"INDIRECT",
        u"MDURATION",
        u"ORG.OPENOFFICE.ERRORTYPE",
        u"PPMT",
        u"REPLACE",
        u"TBILLPRICE",
        u"TBILLYIELD",
        u"XIRR",
        u"DCOUNT",
        u"WEEKNUM",
        u"COM.MICROSOFT.EXPAND",
        u"DEC2BIN",
        u"WEEKDAY",
        u"BIN2HEX",
        u"DB",
        u"ISODD",
        u"N",
        u"YIELD",
        u"COM.MICROSOFT.BAHTTEXT",
        u"DISC",
        u"FV",
        u"IPMT",
        u"RECEIVED",
        u"TRIMMEAN",
        u"BETAINV",
        u"COM.MICROSOFT.BETA.INV",
        u"COM.MICROSOFT.DROP",
        u"COM.MICROSOFT.HYPGEOM.DIST",
        u"COM.MICROSOFT.MAXIFS",
        u"COM.MICROSOFT.TAKE",
        u"INTRATE",
        u"ISEVEN",
        u"SEARCH",
        u"SLOPE",
        u"COM.MICROSOFT.MODE.SNGL",
        u"COMPLEX",
        u"AMORDEGRC",
        u"COM.MICROSOFT.NORM.DIST",
        u"COUNTA",
        u"DAYS360",
        u"OCT2BIN",
        u"OCT2DEC",
        u"OCT2HEX",
        u"PRICEDISC",
        u"ZTEST",
        u"COM.MICROSOFT.PERCENTILE.INC",
        u"IRR",
        u"BIN2OCT",
        u"COM.MICROSOFT.Z.TEST",
        u"COUNT",
        u"EOMONTH",
        u"PRICEMAT",
        u"YIELDDISC",
        u"COM.MICROSOFT.PERCENTILE.EXC",
        u"MIRR",
        u"KURT",
        u"ORG.LIBREOFFICE.FORECAST.ETS.MULT",
        u"COM.MICROSOFT.GAMMA.DIST",
        u"COM.MICROSOFT.NEGBINOM.DIST",
        u"COM.MICROSOFT.TEXTJOIN",
        u"COUPDAYS",
        u"CRITBINOM",
        u"IMDIV",
        u"NORMINV",
        u"NPER",
        u"PV",
        u"COM.MICROSOFT.BINOM.DIST",
        u"DELTA",
        u"FIND",
        u"GESTEP",
        u"NEGBINOMDIST",
        u"SUBSTITUTE",
        u"COM.MICROSOFT.FORECAST.ETS",
        u"COM.MICROSOFT.MODE.MULT",
        u"COM.MICROSOFT.CONFIDENCE.NORM",
        u"COM.MICROSOFT.CONFIDENCE.T",
        u"CONFIDENCE",
        u"COUPDAYBS",
        u"COUPDAYSNC",
        u"COUPNCD",
        u"COUPNUM",
        u"COUPPCD",
        u"IMPOWER",
        u"LEGACY.CHIINV",
        u"NOMINAL",
        u"RSQ",
        u"SECOND",
        u"SLN",
        u"SYD",
        u"ROW",
        u"ACCRINTM",
        u"COM.MICROSOFT.LOGNORM.INV",
        u"ORG.LIBREOFFICE.REGEX",
        u"RRI",
        u"STEYX",
        u"TIME",
        u"FTEST",
        u"NPV",
        u"PERCENTILE",
        u"MODE",
        u"COLUMN",
        u"COM.MICROSOFT.LOGNORM.DIST",
        u"COM.MICROSOFT.NORM.INV",
        u"COM.MICROSOFT.WRAPCOLS",
        u"COM.MICROSOFT.WRAPROWS",
        u"FREQUENCY",
        u"LEFTB",
        u"TBILLEQ",
        u"AVERAGEA",
        u"COLUMNS",
        u"COM.MICROSOFT.VSTACK",
        u"BIN2DEC",
        u"CHOOSE",
        u"COM.MICROSOFT.CONCAT",
        u"COM.MICROSOFT.SEQUENCE",
        u"DOLLAR",
        u"IMARGUMENT",
        u"IMCONJUGATE",
        u"IMCSCH",
        u"IMEXP",
        u"IMLN",
        u"IMLOG10",
        u"IMLOG2",
        u"IMSEC",
        u"IMSECH",
        u"IMSIN",
        u"ISREF",
        u"LOGINV",
        u"MINUTE",
        u"QUOTIENT",
        u"COM.MICROSOFT.FILTER",
        u"SHEETS",
        u"TYPE",
        u"COM.MICROSOFT.CHISQ.DIST",
        u"DCOUNTA",
        u"DGET",
        u"DVAR",
        u"DVARP",
        u"FIXED",
        u"HOUR",
        u"IMABS",
        u"IMCOS",
        u"IMCOSH",
        u"IMCOT",
        u"IMCSC",
        u"IMSINH",
        u"IMSQRT",
        u"IMTAN",
        u"PDURATION",
        u"RIGHTB",
        u"TRIM",
        u"COM.MICROSOFT.F.TEST",
        u"CORREL",
        u"SHEET",
        u"XNPV",
        u"COM.MICROSOFT.COVARIANCE.P",
        u"COM.MICROSOFT.COVARIANCE.S",
        u"COVAR",
        u"FVSCHEDULE",
        u"SERIESSUM",
        u"AREAS",
        u"COM.MICROSOFT.ENCODEURL",
        u"COM.MICROSOFT.MINIFS",
        u"COM.MICROSOFT.RANDARRAY",
        u"COUNTBLANK",
        u"DAVERAGE",
        u"DMAX",
        u"DMIN",
        u"DSTDEVP",
        u"EDATE",
        u"EFFECT",
        u"ORG.LIBREOFFICE.COLOR",
        u"PEARSON",
        u"MMULT",
        u"COM.MICROSOFT.TOCOL",
        u"COM.MICROSOFT.TOROW",
        u"DOLLARDE",
        u"DOLLARFR",
        u"DPRODUCT",
        u"DSTDEV",
        u"INFO",
        u"REPT",
        u"DAYS",
        u"IFNA",
        u"ISFORMULA",
        u"MIDB",
        u"ORG.LIBREOFFICE.FORECAST.ETS.STAT.MULT",
        u"AVEDEV",
        u"ROWS",
        u"LOGEST",
        u"IFERROR",
        u"ORG.OPENOFFICE.ROT13",
        u"TREND",
        u"TRANSPOSE",
        u"DHFG",
        u"LINEST",
        u"ROT",
        u"SQRTPI",
        u"TODAY",
        u"ACCRINT",
        u"CELL",
        u"ISLOGICAL",
        u"MUNIT",
    };

    for (const auto aName : aStoredValueFunctions)
    {
        if (rFunctionName == aName)
            return true;
    }

    return false;
}

[[nodiscard]] inline bool importedRootUsesStoredHostValueTruth(api::StringView rFunctionName)
{
    return isImportedStoredHostValueTruthFunctionName(rFunctionName);
}

[[nodiscard]] inline bool isUnknownSupportedFunctionName(api::StringView rFunctionName)
{
    static constexpr api::StringView aUnknownSupportedFunctions[] = {
        u"NA",
        u"IMREAL",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMREAL",
        u"IMAGINARY",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMAGINARY",
        u"BESSELI",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELI",
        u"BESSELJ",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELJ",
        u"BESSELK",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELK",
        u"BESSELY",
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELY",
    };

    if (isImportedStoredHostValueTruthFunctionName(rFunctionName))
        return true;

    for (const auto aName : aUnknownSupportedFunctions)
    {
        if (rFunctionName == aName)
            return true;
    }

    return false;
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

[[nodiscard]] inline api::Error mapNumericErrorCode(api::StringView rCode)
{
    if (rCode == u"503" || rCode == u"523")
        return api::Error::NoConvergence;
    if (rCode == u"513")
        return api::Error::StringOverflow;
    if (rCode == u"519")
        return api::Error::NoValue;
    if (rCode == u"532")
        return api::Error::DivisionByZero;
    return api::Error::IllegalArgument;
}

[[nodiscard]] inline api::Error mapErrorLiteral(api::StringView rText)
{
    if (rText == u"#N/A")
        return api::Error::NotAvailable;
    if (rText == u"#DIV/0!")
        return api::Error::DivisionByZero;
    if (rText == u"#VALUE!")
        return api::Error::NoValue;
    if (rText == u"#NAME?")
        return api::Error::NoName;
    if (rText == u"#NUM!")
        return api::Error::NoConvergence;
    if (rText == u"#REF!" || rText == u"#NULL!")
        return api::Error::IllegalArgument;

    constexpr api::StringView aErrPrefix = u"#ERR";
    if (rText.starts_with(aErrPrefix) && rText.size() > aErrPrefix.size() + 1
        && rText.back() == u'!')
    {
        const api::StringView aCode
            = rText.substr(aErrPrefix.size(), rText.size() - aErrPrefix.size() - 1);
        bool bDigitsOnly = !aCode.empty();
        for (const char16_t cChar : aCode)
        {
            if (cChar < u'0' || cChar > u'9')
            {
                bDigitsOnly = false;
                break;
            }
        }
        if (bDigitsOnly)
            return mapNumericErrorCode(aCode);
    }

    const std::size_t nColon = rText.rfind(u':');
    if (nColon != api::StringView::npos && nColon + 1 < rText.size())
    {
        const api::StringView aCode = rText.substr(nColon + 1);
        bool bDigitsOnly = true;
        for (const char16_t cChar : aCode)
        {
            if (cChar < u'0' || cChar > u'9')
            {
                bDigitsOnly = false;
                break;
            }
        }
        if (bDigitsOnly)
            return mapNumericErrorCode(aCode);
    }

    return api::Error::NoValue;
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
    if (rValue.isBoolean())
        return makeNumericResult(eFunction, rValue.mfNumber, SvNumFormatType::LOGICAL);
    if (rValue.isNumber())
        return makeNumericResult(eFunction, rValue.mfNumber, SvNumFormatType::NUMBER);
    return makeNumericResult(eFunction, 0.0, SvNumFormatType::NUMBER);
}

[[nodiscard]] inline FunctionKind classifyDelegatedFunctionNode(
    const core::formula::Node& rNode);

[[nodiscard]] inline bool isPromotableScalarRootNode(
    const core::formula::Node& rNode);

[[nodiscard]] inline bool isHardRoutedNode(
    const core::formula::Node& rNode);

[[nodiscard]] inline bool isLiteralVectorArrayConstantNode(
    const core::formula::Node& rNode);

[[nodiscard]] inline bool isHardRoutedLiteralMatchNode(
    FunctionKind eFunction, const core::formula::Node& rNode);

[[nodiscard]] inline bool isLiteralArrayConstantNode(
    const core::formula::Node& rNode);

[[nodiscard]] inline bool isLiteralVectorArrayConstantNode(
    const core::formula::Node& rNode);

[[nodiscard]] inline bool isLiteralVectorArrayConstantNode(
    const core::formula::Node& rNode, sal_Int32* pLength);

[[nodiscard]] inline bool isZeroOrFalseNode(const core::formula::Node& rNode);

[[nodiscard]] inline bool isPositiveWholeLiteralNode(
    const core::formula::Node& rNode, sal_Int32* pValue = nullptr);

[[nodiscard]] inline bool isHardRoutedLiteralLookupNode(
    FunctionKind eFunction, const core::formula::Node& rNode);

[[nodiscard]] inline EvaluationAttempt evaluateDelegatedNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos, bool bEmptyStringAsZero, std::size_t nDepth = 0,
    bool bImportedCanonicalSource = false);

[[nodiscard]] inline EvaluationAttempt evaluateScalarOrDelegatedNode(
    const core::formula::Node& rNode, FunctionKind ePreferredFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    std::size_t nDepth = 0, bool bImportedCanonicalSource = false);

[[nodiscard]] inline EvaluationAttempt evaluateFunctionNode(
    const core::formula::Node& rRoot, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    bool bImportedCanonicalSource = false);

[[nodiscard]] inline Materialization<api::CellValue> materializeScalarNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos);

[[nodiscard]] inline Materialization<ScMatrixRef> materializeMatrixNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos);

[[nodiscard]] inline EvaluationAttempt evaluateGrowthFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos);

[[nodiscard]] inline Materialization<lookupexecution::LookupInputSource> materializeLookupInputSourceNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos);

[[nodiscard]] inline api::ValueResult<double> coerceScalarToNumber(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const api::CellValue& rValue);

[[nodiscard]] inline std::optional<sal_Int32> coerceWholeNumber(double fValue);

[[nodiscard]] inline Materialization<api::CellValue> materializeScalarizedReferenceValueNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos);

[[nodiscard]] inline bool containsReferenceLikeDescendant(const core::formula::Node& rNode);

[[nodiscard]] inline bool isImportedCachedFormulaCell(const ScDocument& rDoc,
    const ScAddress& rAddress);

[[nodiscard]] inline bool isImportedCachedFormulaRoot(
    const ScDocument& rDoc, const ScAddress& rFormulaPos);

[[nodiscard]] inline bool importedCachedFormulaCellMatchesPredicateHostTruth(
    const ScDocument& rDoc, const ScAddress& rAddress, api::StringView aFunctionName);

[[nodiscard]] inline bool referencesImportedPredicateHostTruthCell(const core::formula::Node& rNode,
    const ScDocument& rDoc, const ScAddress& rFormulaPos, api::StringView aFunctionName);

[[nodiscard]] inline Materialization<ScRange> resolveReferenceRangeNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, const ScAddress& rFormulaPos);

[[nodiscard]] inline std::optional<ScAddress> tryImplicitIntersectionAddress(
    const ScRange& rRange, const ScAddress& rFormulaPos);

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

[[nodiscard]] inline bool containsReferenceLikeDescendant(const core::formula::Node& rNode)
{
    if (rNode.meKind == core::formula::NodeKind::CellReference
        || rNode.meKind == core::formula::NodeKind::RangeReference
        || rNode.meKind == core::formula::NodeKind::NamedReference)
    {
        return true;
    }

    for (const auto& rxChild : rNode.maChildren)
    {
        if (rxChild && containsReferenceLikeDescendant(*rxChild))
            return true;
    }

    return false;
}

[[nodiscard]] inline bool isImportedCachedFormulaCell(const ScDocument& rDoc,
    const ScAddress& rAddress)
{
    ScFormulaCell* pFormula = const_cast<ScDocument&>(rDoc).GetFormulaCell(rAddress);
    return pFormula
           && (pFormula->GetCode()->IsRecalcModeMustAfterImport()
               || pFormula->HasHybridStringResult() || pFormula->IsEmptyDisplayedAsString()
               || !pFormula->GetHybridFormula().isEmpty());
}

[[nodiscard]] inline bool isImportedCachedFormulaRoot(
    const ScDocument& rDoc, const ScAddress& rFormulaPos)
{
    return isImportedCachedFormulaCell(rDoc, rFormulaPos);
}

[[nodiscard]] inline bool importedCachedFormulaCellMatchesPredicateHostTruth(
    const ScDocument& rDoc, const ScAddress& rAddress, api::StringView aFunctionName)
{
    ScFormulaCell* pFormula = const_cast<ScDocument&>(rDoc).GetFormulaCell(rAddress);
    if (!pFormula || !isImportedCachedFormulaCell(rDoc, rAddress))
        return false;

    const auto oCachedValue = tryReadHostCachedFormulaCellValue(rDoc, rAddress, *pFormula);
    const bool bEmptyDisplayedAsString = pFormula->IsEmptyDisplayedAsString();
    if (!oCachedValue)
        return bEmptyDisplayedAsString && aFunctionName == u"ISBLANK";

    switch (oCachedValue->meKind)
    {
        case api::CellValueKind::Error:
            return aFunctionName == u"ISERROR" || aFunctionName == u"ISERR"
                   || aFunctionName == u"ISNA";
        case api::CellValueKind::Text:
            return aFunctionName == u"ISTEXT" || aFunctionName == u"ISNONTEXT"
                   || aFunctionName == u"ISBLANK";
        case api::CellValueKind::Empty:
            return aFunctionName == u"ISBLANK";
        case api::CellValueKind::Boolean:
        case api::CellValueKind::Number:
            return false;
    }

    return false;
}

[[nodiscard]] inline bool referencesImportedPredicateHostTruthCell(const core::formula::Node& rNode,
    const ScDocument& rDoc, const ScAddress& rFormulaPos, api::StringView aFunctionName)
{
    if (rNode.meKind == core::formula::NodeKind::CellReference
        || rNode.meKind == core::formula::NodeKind::RangeReference
        || rNode.meKind == core::formula::NodeKind::NamedReference)
    {
        const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
        if (!aRange.mbSupported || !aRange.moValue)
            return false;

        std::optional<ScAddress> oScalarAddress
            = tryImplicitIntersectionAddress(*aRange.moValue, rFormulaPos);
        if ((!oScalarAddress || *oScalarAddress == rFormulaPos)
            && aRange.moValue->aStart != rFormulaPos)
        {
            oScalarAddress = aRange.moValue->aStart;
        }

        return oScalarAddress && *oScalarAddress != rFormulaPos
               && importedCachedFormulaCellMatchesPredicateHostTruth(
                   rDoc, *oScalarAddress, aFunctionName);
    }

    for (const auto& rxChild : rNode.maChildren)
    {
        if (rxChild
            && referencesImportedPredicateHostTruthCell(
                *rxChild, rDoc, rFormulaPos, aFunctionName))
            return true;
    }

    return false;
}

[[nodiscard]] inline EvaluationAttempt evaluateMathScalarFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos,
    bool bImportedCanonicalSource)
{
    api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if (aFunctionName == u"COM.MICROSOFT.TEXTAFTER")
        aFunctionName = u"TEXTAFTER"_ustr;
    else if (aFunctionName == u"COM.MICROSOFT.TEXTBEFORE")
        aFunctionName = u"TEXTBEFORE"_ustr;
    const auto oCanonical = canonicalMathScalarFunctionName(aFunctionName);
    if (!oCanonical)
        return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
    const api::StringView aCanonicalName = *oCanonical;

    if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
        && containsReferenceLikeDescendant(rNode))
        return makeErrorResult(eFunction, api::Error::VariableExpected);

    const auto materializeNumericArgument = [&](const core::formula::Node& rArgument,
                                                std::optional<double> oEmptyDefault = std::nullopt)
        -> Materialization<double> {
        if (rArgument.meKind == core::formula::NodeKind::EmptyArgument)
        {
            if (!oEmptyDefault)
                return makeMaterializedError<double>(api::Error::IllegalArgument);
            return makeMaterializedValue(*oEmptyDefault);
        }

        const auto aArgument = materializeScalarizedReferenceValueNode(
            rArgument, rDoc, rContext, rFormulaPos);
        if (!aArgument.mbSupported)
            return makeUnsupportedMaterialization<double>(aArgument.meFallbackReason);
        if (!aArgument.moValue)
            return makeMaterializedError<double>(aArgument.meError);

        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aArgument.moValue);
        if (!aNumber)
            return makeMaterializedError<double>(aNumber.meError);
        return makeMaterializedValue(aNumber.maValue);
    };

    const auto materializeWholeArgument = [&](const core::formula::Node& rArgument,
                                              std::optional<sal_Int32> oEmptyDefault = std::nullopt)
        -> Materialization<sal_Int32> {
        if (rArgument.meKind == core::formula::NodeKind::EmptyArgument)
        {
            if (!oEmptyDefault)
                return makeMaterializedError<sal_Int32>(api::Error::IllegalArgument);
            return makeMaterializedValue(*oEmptyDefault);
        }

        const auto aNumber = materializeNumericArgument(rArgument, std::nullopt);
        if (!aNumber.mbSupported)
            return makeUnsupportedMaterialization<sal_Int32>(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeMaterializedError<sal_Int32>(aNumber.meError);

        const auto oWhole = coerceWholeNumber(*aNumber.moValue);
        if (!oWhole)
            return makeMaterializedError<sal_Int32>(api::Error::IllegalArgument);
        return makeMaterializedValue(*oWhole);
    };

    const auto collectNumericArguments = [&]() -> Materialization<std::vector<double>> {
        if (rNode.maChildren.empty())
            return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

        std::vector<double> aValues;
        for (const auto& rxChild : rNode.maChildren)
        {
            if (!rxChild)
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const auto aMatrix = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
            if (!aMatrix.mbSupported)
            {
                return makeUnsupportedMaterialization<std::vector<double>>(
                    aMatrix.meFallbackReason);
            }
            if (!aMatrix.moValue)
                return makeMaterializedError<std::vector<double>>(aMatrix.meError);

            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
            for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    const auto aValue = lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(nColumn, nRow));
                    if (aValue.isEmpty())
                        continue;

                    const auto aNumber = coerceScalarToNumber(rDoc, rContext, aValue);
                    if (!aNumber)
                    {
                        return makeMaterializedError<std::vector<double>>(
                            aNumber.meError);
                    }
                    aValues.push_back(aNumber.maValue);
                }
            }
        }

        return makeMaterializedValue(std::move(aValues));
    };

    const auto makeNumericAttempt = [&](double fValue) {
        return makeNumericResult(eFunction, fValue, SvNumFormatType::NUMBER);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto finishNumericValue = [&](const api::ValueResult<double>& rResult) {
        if (!rResult)
            return makeErrorAttempt(rResult.meError);
        return makeNumericAttempt(rResult.maValue);
    };
    const auto finishCalcMathValue = [&](const api::ValueResult<double>& rResult) {
        if (!rResult)
        {
            return makeErrorAttempt(
                rResult.meError == api::Error::Domain ? api::Error::IllegalArgument
                                                      : rResult.meError);
        }
        return makeNumericAttempt(rResult.maValue);
    };
    const auto finishOptionalValue = [&](const std::optional<double>& oValue,
                                         api::Error eError = api::Error::IllegalArgument) {
        if (!oValue)
            return makeErrorAttempt(eError);
        if (!std::isfinite(*oValue))
            return makeErrorAttempt(api::Error::Domain);
        return makeNumericAttempt(*oValue);
    };
    const auto evaluateUnaryFinite = [&](auto aCompute,
                                         api::Error eError = api::Error::Domain) {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);

        const double fResult = aCompute(*aValue.moValue);
        if (!std::isfinite(fResult))
            return makeErrorAttempt(eError);
        return makeNumericAttempt(fResult);
    };
    const auto evaluateUnaryOptional = [&](auto aCompute,
                                           api::Error eError = api::Error::IllegalArgument) {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);

        return finishOptionalValue(aCompute(*aValue.moValue), eError);
    };

    if (aCanonicalName == u"ABS")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        return makeNumericAttempt(spreadsheetengine::core::math::computeAbs(*aValue.moValue));
    }

    if (aCanonicalName == u"PI")
    {
        if (!rNode.maChildren.empty())
            return makeErrorAttempt(api::Error::IllegalArgument);
        return makeNumericAttempt(spreadsheetengine::core::math::computePi());
    }

    if (aCanonicalName == u"DEGREES")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeDegrees);
    if (aCanonicalName == u"RADIANS")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeRadians);
    if (aCanonicalName == u"SIN")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeSin);
    if (aCanonicalName == u"COS")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeCos);
    if (aCanonicalName == u"TAN")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeTan);
    if (aCanonicalName == u"COT")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeCot);
    if (aCanonicalName == u"ASIN")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeArcSin, api::Error::Domain);
    if (aCanonicalName == u"ACOS")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeArcCos, api::Error::Domain);
    if (aCanonicalName == u"ATAN")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeArcTan);
    if (aCanonicalName == u"ACOT")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeArcCot);
    if (aCanonicalName == u"SINH")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeSinHyp);
    if (aCanonicalName == u"COSH")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeCosHyp);
    if (aCanonicalName == u"TANH")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeTanHyp);
    if (aCanonicalName == u"COTH")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeCotHyp);
    if (aCanonicalName == u"ASINH")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeArcSinHyp);
    if (aCanonicalName == u"ACOSH")
        return evaluateUnaryOptional(spreadsheetengine::core::math::computeArcCosHyp,
            api::Error::Domain);
    if (aCanonicalName == u"ACOTH")
        return evaluateUnaryOptional(spreadsheetengine::core::math::computeArcCotHyp,
            api::Error::Domain);
    if (aCanonicalName == u"SEC")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeSecant);
    if (aCanonicalName == u"SECH")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeSecantHyp);
    if (aCanonicalName == u"EXP")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeExp);
    if (aCanonicalName == u"SQRT")
        return evaluateUnaryOptional(spreadsheetengine::core::math::computeSqrt,
            api::Error::IllegalArgument);
    if (aCanonicalName == u"FACT")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        return finishCalcMathValue(spreadsheetengine::core::math::evaluateFactorialValue(
            *aValue.moValue));
    }
    if (aCanonicalName == u"LOG10")
        return evaluateUnaryOptional(spreadsheetengine::core::math::computeLog10);
    if (aCanonicalName == u"LN")
        return evaluateUnaryOptional(spreadsheetengine::core::math::computeLn);

    if (aCanonicalName == u"ATAN2")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aY = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aY.mbSupported)
            return makeUnsupported(eFunction, aY.meFallbackReason);
        if (!aY.moValue)
            return makeErrorAttempt(aY.meError);
        const auto aX = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aX.mbSupported)
            return makeUnsupported(eFunction, aX.meFallbackReason);
        if (!aX.moValue)
            return makeErrorAttempt(aX.meError);
        const double fResult
            = spreadsheetengine::core::math::computeArcTan2(*aY.moValue, *aX.moValue);
        if (!std::isfinite(fResult))
            return makeErrorAttempt(api::Error::Domain);
        return makeNumericAttempt(fResult);
    }

    if (aCanonicalName == u"ATANH")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        return finishNumericValue(
            spreadsheetengine::api::math::inverseHyperbolicTangent(*aValue.moValue));
    }

    if (aCanonicalName == u"CSC")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        return finishNumericValue(
            spreadsheetengine::core::math::evaluateCscValue(*aValue.moValue));
    }

    if (aCanonicalName == u"CSCH")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        return finishNumericValue(
            spreadsheetengine::core::math::evaluateCschValue(*aValue.moValue));
    }

    if (aCanonicalName == u"COLOR")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeErrorAttempt(api::Error::IllegalArgument);

        std::array<double, 4> aChannels { 0.0, 0.0, 0.0, 0.0 };
        for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aChannel = materializeNumericArgument(*rNode.maChildren[nIndex], std::nullopt);
            if (!aChannel.mbSupported)
                return makeUnsupported(eFunction, aChannel.meFallbackReason);
            if (!aChannel.moValue)
                return makeErrorAttempt(aChannel.meError);

            const double fFloor = rtl::math::approxFloor(*aChannel.moValue);
            if (fFloor < 0.0 || fFloor > 255.0)
                return makeErrorAttempt(api::Error::IllegalArgument);
            if (nIndex < 3)
                aChannels[nIndex + 1] = fFloor;
            else
                aChannels[0] = fFloor;
        }

        const double fResult = 256.0 * 256.0 * 256.0 * aChannels[0]
                               + 256.0 * 256.0 * aChannels[1] + 256.0 * aChannels[2]
                               + aChannels[3];
        return makeNumericAttempt(fResult);
    }

    if (aCanonicalName == u"SIGN")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        return makeNumericAttempt(static_cast<double>(
            spreadsheetengine::core::math::computePlusMinus(*aValue.moValue)));
    }

    if (aCanonicalName == u"INT")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeInt);
    if (aCanonicalName == u"EVEN")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeEven);
    if (aCanonicalName == u"ODD")
        return evaluateUnaryFinite(spreadsheetengine::core::math::computeOdd);

    if (aCanonicalName == u"GCD" || aCanonicalName == u"LCM")
    {
        const auto aNumbers = collectNumericArguments();
        if (!aNumbers.mbSupported)
            return makeUnsupported(eFunction, aNumbers.meFallbackReason);
        if (!aNumbers.moValue)
            return makeErrorAttempt(aNumbers.meError);
        if (aNumbers.moValue->empty())
            return makeErrorAttempt(api::Error::IllegalArgument);

        std::int64_t nResult = 0;
        bool bSawValue = false;
        for (double fValue : *aNumbers.moValue)
        {
            if (!std::isfinite(fValue) || fValue < 0.0)
                return makeErrorAttempt(api::Error::IllegalArgument);
            const double fTruncated = std::trunc(fValue);
            if (fTruncated < static_cast<double>(std::numeric_limits<std::int64_t>::min())
                || fTruncated > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
            {
                return makeErrorAttempt(api::Error::IllegalArgument);
            }

            const std::int64_t nValue = static_cast<std::int64_t>(fTruncated);
            if (!bSawValue)
            {
                nResult = std::abs(nValue);
                bSawValue = true;
            }
            else if (aCanonicalName == u"GCD")
                nResult = std::gcd(nResult, std::abs(nValue));
            else
                nResult = std::lcm(nResult, std::abs(nValue));
        }

        return makeNumericAttempt(static_cast<double>(nResult));
    }

    if (aCanonicalName == u"CEILING" || aCanonicalName == u"FLOOR"
        || aCanonicalName == u"CEILING.XCL" || aCanonicalName == u"FLOOR.XCL")
    {
        const bool bMicrosoftCompat
            = aCanonicalName == u"CEILING.XCL" || aCanonicalName == u"FLOOR.XCL";
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3
            || (bMicrosoftCompat && rNode.maChildren.size() != 2))
        {
            return makeErrorAttempt(api::Error::IllegalArgument);
        }

        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);

        double fSignificance = 1.0;
        const bool bMissingSignificance = rNode.maChildren.size() < 2
                                          || rNode.maChildren[1]->meKind
                                                 == core::formula::NodeKind::EmptyArgument;
        if (rNode.maChildren.size() >= 2 && !bMissingSignificance)
        {
            const auto aSignificance = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aSignificance.mbSupported)
                return makeUnsupported(eFunction, aSignificance.meFallbackReason);
            if (!aSignificance.moValue)
                return makeErrorAttempt(aSignificance.meError);
            fSignificance = *aSignificance.moValue;
        }

        bool bAbs = false;
        if (!bMicrosoftCompat && rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMode = materializeNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aMode.mbSupported)
                return makeUnsupported(eFunction, aMode.meFallbackReason);
            if (!aMode.moValue)
                return makeErrorAttempt(aMode.meError);
            bAbs = !rtl::math::approxEqual(*aMode.moValue, 0.0);
        }

        if (!bMicrosoftCompat && bMissingSignificance && *aValue.moValue < 0.0)
            fSignificance = -1.0;

        return finishNumericValue(
            spreadsheetengine::core::math::evaluateCeilingFloorValue(*aValue.moValue,
                fSignificance, bAbs,
                aCanonicalName == u"CEILING" || aCanonicalName == u"CEILING.XCL",
                bMicrosoftCompat));
    }

    if (aCanonicalName == u"CEILING.MATH" || aCanonicalName == u"FLOOR.MATH")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);

        double fSignificance = 1.0;
        if (rNode.maChildren.size() >= 2)
        {
            const auto aSignificance = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            if (!aSignificance.mbSupported)
                return makeUnsupported(eFunction, aSignificance.meFallbackReason);
            if (!aSignificance.moValue)
                return makeErrorAttempt(aSignificance.meError);
            fSignificance = *aSignificance.moValue;
        }

        double fMode = 0.0;
        if (rNode.maChildren.size() == 3)
        {
            const auto aMode = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            if (!aMode.mbSupported)
                return makeUnsupported(eFunction, aMode.meFallbackReason);
            if (!aMode.moValue)
                return makeErrorAttempt(aMode.meError);
            fMode = *aMode.moValue;
        }

        if (fSignificance == 0.0 || *aValue.moValue == 0.0)
            return makeNumericAttempt(0.0);

        return finishNumericValue(
            spreadsheetengine::core::math::evaluateCeilingFloorMathValue(
                *aValue.moValue, fSignificance, fMode, aCanonicalName == u"CEILING.MATH"));
    }

    if (aCanonicalName == u"CEILING.PRECISE" || aCanonicalName == u"FLOOR.PRECISE"
        || aCanonicalName == u"ISO.CEILING")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);

        double fSignificance = 1.0;
        if (rNode.maChildren.size() == 2)
        {
            const auto aSignificance = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aSignificance.mbSupported)
                return makeUnsupported(eFunction, aSignificance.meFallbackReason);
            if (!aSignificance.moValue)
                return makeErrorAttempt(aSignificance.meError);
            fSignificance = *aSignificance.moValue;
        }

        return finishNumericValue(
            spreadsheetengine::core::math::evaluateCeilingFloorPreciseValue(
                *aValue.moValue, fSignificance, aCanonicalName == u"FLOOR.PRECISE"));
    }

    if (aCanonicalName == u"ROUNDSIG")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        const auto aDigits = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aDigits.mbSupported)
            return makeUnsupported(eFunction, aDigits.meFallbackReason);
        if (!aDigits.moValue)
            return makeErrorAttempt(aDigits.meError);
        return finishNumericValue(
            spreadsheetengine::core::math::evaluateRoundSigValue(*aValue.moValue, *aDigits.moValue));
    }

    if (aCanonicalName == u"BITAND" || aCanonicalName == u"BITOR" || aCanonicalName == u"BITXOR")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aLeft = materializeNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aLeft.mbSupported)
            return makeUnsupported(eFunction, aLeft.meFallbackReason);
        if (!aLeft.moValue)
            return makeErrorAttempt(aLeft.meError);
        const auto aRight = materializeNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aRight.mbSupported)
            return makeUnsupported(eFunction, aRight.meFallbackReason);
        if (!aRight.moValue)
            return makeErrorAttempt(aRight.meError);

        std::optional<double> oResult;
        if (aCanonicalName == u"BITAND")
            oResult = spreadsheetengine::core::math::computeBitAnd(*aLeft.moValue, *aRight.moValue);
        else if (aCanonicalName == u"BITOR")
            oResult = spreadsheetengine::core::math::computeBitOr(*aLeft.moValue, *aRight.moValue);
        else
            oResult = spreadsheetengine::core::math::computeBitXor(*aLeft.moValue, *aRight.moValue);
        return finishOptionalValue(oResult);
    }

    if (aCanonicalName == u"BITLSHIFT" || aCanonicalName == u"BITRSHIFT")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        const auto aShift = materializeNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aShift.mbSupported)
            return makeUnsupported(eFunction, aShift.meFallbackReason);
        if (!aShift.moValue)
            return makeErrorAttempt(aShift.meError);

        const auto oResult = aCanonicalName == u"BITLSHIFT"
                                 ? spreadsheetengine::core::math::computeBitLeftShift(
                                       *aValue.moValue, *aShift.moValue)
                                 : spreadsheetengine::core::math::computeBitRightShift(
                                       *aValue.moValue, *aShift.moValue);
        return finishOptionalValue(oResult);
    }

    if (aCanonicalName == u"POWER")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aBase = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aBase.mbSupported)
            return makeUnsupported(eFunction, aBase.meFallbackReason);
        if (!aBase.moValue)
            return makeErrorAttempt(aBase.meError);
        const auto aExponent = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aExponent.mbSupported)
            return makeUnsupported(eFunction, aExponent.meFallbackReason);
        if (!aExponent.moValue)
            return makeErrorAttempt(aExponent.meError);
        const double fResult = std::pow(*aBase.moValue, *aExponent.moValue);
        if (!std::isfinite(fResult))
            return makeErrorAttempt(api::Error::Domain);
        return makeNumericAttempt(fResult);
    }

    if (aCanonicalName == u"LOG")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        double fBase = 10.0;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aBase = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aBase.mbSupported)
                return makeUnsupported(eFunction, aBase.meFallbackReason);
            if (!aBase.moValue)
                return makeErrorAttempt(aBase.meError);
            fBase = *aBase.moValue;
        }
        return finishNumericValue(
            spreadsheetengine::core::math::evaluateLogValue(*aValue.moValue, fBase));
    }

    if (aCanonicalName == u"MROUND")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);
        const auto aMultiple = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aMultiple.mbSupported)
            return makeUnsupported(eFunction, aMultiple.meFallbackReason);
        if (!aMultiple.moValue)
            return makeErrorAttempt(aMultiple.meError);
        return finishNumericValue(
            spreadsheetengine::core::math::evaluateMroundValue(*aValue.moValue, *aMultiple.moValue));
    }

    if (aCanonicalName == u"COMBIN" || aCanonicalName == u"COMBINA")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aN = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aN.mbSupported)
            return makeUnsupported(eFunction, aN.meFallbackReason);
        if (!aN.moValue)
            return makeErrorAttempt(aN.meError);
        const auto aK = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aK.mbSupported)
            return makeUnsupported(eFunction, aK.meFallbackReason);
        if (!aK.moValue)
            return makeErrorAttempt(aK.meError);
        return finishNumericValue(spreadsheetengine::core::math::evaluateCombinValue(
            *aN.moValue, *aK.moValue, aCanonicalName == u"COMBINA"));
    }

    if (aCanonicalName == u"TRUNC")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aValue = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);

        sal_Int32 nDigits = 0;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aDigits = materializeWholeArgument(*rNode.maChildren[1], std::nullopt);
            if (!aDigits.mbSupported)
                return makeUnsupported(eFunction, aDigits.meFallbackReason);
            if (!aDigits.moValue)
                return makeErrorAttempt(aDigits.meError);
            nDigits = *aDigits.moValue;
        }
        return finishNumericValue(
            spreadsheetengine::core::math::evaluateTruncValue(*aValue.moValue, nDigits));
    }

    if (aCanonicalName == u"MOD")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumerator = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aNumerator.mbSupported)
            return makeUnsupported(eFunction, aNumerator.meFallbackReason);
        if (!aNumerator.moValue)
            return makeErrorAttempt(aNumerator.meError);
        const auto aDenominator = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aDenominator.mbSupported)
            return makeUnsupported(eFunction, aDenominator.meFallbackReason);
        if (!aDenominator.moValue)
            return makeErrorAttempt(aDenominator.meError);
        return finishNumericValue(
            spreadsheetengine::core::math::evaluateModValue(*aNumerator.moValue,
                *aDenominator.moValue));
    }

    if (aCanonicalName == u"RAWSUBTRACT")
    {
        if (rNode.maChildren.size() < 2)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aFirst = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aFirst.mbSupported)
            return makeUnsupported(eFunction, aFirst.meFallbackReason);
        if (!aFirst.moValue)
            return makeErrorAttempt(aFirst.meError);

        double fResult = *aFirst.moValue;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aNext = materializeNumericArgument(*rNode.maChildren[nIndex], std::nullopt);
            if (!aNext.mbSupported)
                return makeUnsupported(eFunction, aNext.meFallbackReason);
            if (!aNext.moValue)
                return makeErrorAttempt(aNext.meError);
            fResult -= *aNext.moValue;
        }
        return makeNumericAttempt(fResult);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
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

[[nodiscard]] inline OUString formatBasisDateTime(double fSerialValue)
{
    const api::DateParts aNullDate = spreadsheetengine::core::datetime::defaultNullDate();
    const api::DateSerial nDateSerial = static_cast<api::DateSerial>(std::floor(fSerialValue));
    const double fTimeValue = spreadsheetengine::core::datetime::normalizeTimeFraction(fSerialValue);

    const std::int16_t nYear = static_cast<std::int16_t>(
        spreadsheetengine::core::datetime::extractYear(aNullDate, nDateSerial));
    const std::int16_t nMonth = static_cast<std::int16_t>(
        spreadsheetengine::core::datetime::extractMonth(aNullDate, nDateSerial));
    const std::int16_t nDay = static_cast<std::int16_t>(
        spreadsheetengine::core::datetime::extractDay(aNullDate, nDateSerial).value_or(0.0));
    const std::int16_t nHour
        = static_cast<std::int16_t>(spreadsheetengine::core::datetime::extractHour(fTimeValue));
    const std::int16_t nMinute = static_cast<std::int16_t>(
        spreadsheetengine::core::datetime::extractMinute(fTimeValue));
    const std::int16_t nSecond = static_cast<std::int16_t>(
        spreadsheetengine::core::datetime::extractSecond(fTimeValue));

    char aBuffer[32];
    const int nLength = std::snprintf(aBuffer, sizeof(aBuffer), "%04d-%02d-%02d %02d:%02d:%02d",
        static_cast<int>(nYear), static_cast<int>(nMonth), static_cast<int>(nDay),
        static_cast<int>(nHour), static_cast<int>(nMinute), static_cast<int>(nSecond));
    return OUString::fromUtf8(
        std::string_view(aBuffer, static_cast<std::size_t>(std::max(nLength, 0))));
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

[[nodiscard]] inline std::optional<double> tryStandaloneParsedDateValue(const OUString& rInput)
{
    const auto oParsed
        = spreadsheetengine::core::datetime::parseStandaloneNumberText(toApiString(rInput));
    if (!oParsed)
        return std::nullopt;
    if (oParsed->meKind != api::NumberParseResult::Kind::Date
        && oParsed->meKind != api::NumberParseResult::Kind::DateTime)
    {
        return std::nullopt;
    }
    return std::trunc(oParsed->mfValue);
}

[[nodiscard]] inline std::optional<double> tryStandaloneParsedTimeValue(const OUString& rInput)
{
    const auto oParsed
        = spreadsheetengine::core::datetime::parseStandaloneNumberText(toApiString(rInput));
    if (!oParsed)
        return std::nullopt;
    if (oParsed->meKind != api::NumberParseResult::Kind::Time
        && oParsed->meKind != api::NumberParseResult::Kind::DateTime)
    {
        return std::nullopt;
    }
    return spreadsheetengine::core::datetime::normalizeTimeFraction(oParsed->mfValue);
}

[[nodiscard]] inline std::optional<double> tryStandaloneParsedScalarValue(const OUString& rInput)
{
    const auto oParsed
        = spreadsheetengine::core::datetime::parseStandaloneNumberText(toApiString(rInput));
    if (!oParsed)
        return std::nullopt;
    return oParsed->mfValue;
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
        return api::ValueResult<double>::failure(api::Error::NoValue);
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
        return api::ValueResult<bool>::failure(api::Error::NoValue);
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

[[nodiscard]] inline ScRangeData* findNamedRangeData(
    const OUString& rName, const ScDocument& rDoc, const ScAddress& rFormulaPos)
{
    ScRangeData* pRangeData = ScRangeStringConverter::GetRangeDataFromString(
        rName, rFormulaPos.Tab(), rDoc, formula::FormulaGrammar::CONV_OOO);
    if (pRangeData)
        return pRangeData;

    const OUString aUpperName = ScGlobal::getCharClass().uppercase(rName);
    if (ScRangeName* pLocalNames = rDoc.GetRangeName(rFormulaPos.Tab()))
        pRangeData = pLocalNames->findByUpperName(aUpperName);
    if (pRangeData)
        return pRangeData;

    if (ScRangeName* pGlobalNames = rDoc.GetRangeName())
        return pGlobalNames->findByUpperName(aUpperName);
    return nullptr;
}

[[nodiscard]] inline bool tryResolveNamedRangeReference(
    ScRange& rRange, const ScRangeData& rRangeData, const ScDocument& rDoc,
    const ScAddress&)
{
    const ScAddress& rNamePos = rRangeData.GetPos();

    if (rRangeData.IsReference(rRange, rNamePos))
        return true;
    if (const ScTokenArray* pCode = rRangeData.GetCode(); pCode && pCode->IsReference(rRange, rNamePos))
        return true;

    auto tryParseSymbol = [&](formula::FormulaGrammar::Grammar eGrammar) {
        const OUString aSymbol = rRangeData.GetSymbol(rNamePos, eGrammar);
        if (aSymbol.isEmpty())
            return false;

        sal_Int32 nOffset = 0;
        return ScRangeStringConverter::GetRangeFromString(
                   rRange, aSymbol, rDoc, formula::FormulaGrammar::CONV_OOO, nOffset)
               && nOffset >= 0;
    };

    auto tryCompileSymbol = [&](formula::FormulaGrammar::Grammar eGrammar) {
        const OUString aSymbol = rRangeData.GetSymbol(rNamePos, eGrammar);
        if (aSymbol.isEmpty())
            return false;

        ScCompiler aCompiler(const_cast<ScDocument&>(rDoc), rNamePos, eGrammar);
        std::unique_ptr<ScTokenArray> pCode = aCompiler.CompileString(aSymbol);
        if (!pCode || pCode->GetCodeError() != FormulaError::NONE)
            return false;

        ScCompiler aRpnCompiler(const_cast<ScDocument&>(rDoc), rNamePos, *pCode, eGrammar);
        aRpnCompiler.CompileTokenArray();
        pCode->DelRPN();
        return pCode->IsReference(rRange, rNamePos);
    };

    return tryParseSymbol(formula::FormulaGrammar::GRAM_ODFF)
           || tryCompileSymbol(formula::FormulaGrammar::GRAM_ODFF)
           || tryParseSymbol(formula::FormulaGrammar::GRAM_NATIVE)
           || tryCompileSymbol(formula::FormulaGrammar::GRAM_NATIVE);
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
            const OUString aName = toLibreOfficeString(rNode.maPrimaryText);
            ScRangeData* pRangeData = findNamedRangeData(aName, rDoc, rFormulaPos);
            if (!pRangeData)
            {
                return makeUnsupportedMaterialization<ScRange>(
                    FallbackReason::UnsupportedHostSurface);
            }

            ScRange aRange;
            if (!tryResolveNamedRangeReference(aRange, *pRangeData, rDoc, rFormulaPos))
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

[[nodiscard]] inline std::optional<ScAddress> tryImplicitIntersectionAddress(
    const ScRange& rRange, const ScAddress& rFormulaPos)
{
    if (rRange.aStart.Tab() != rRange.aEnd.Tab() || rRange.aStart.Tab() != rFormulaPos.Tab())
        return std::nullopt;

    if (rRange.aStart == rRange.aEnd)
        return rRange.aStart;

    if (rRange.aStart.Col() == rRange.aEnd.Col()
        && rFormulaPos.Row() >= rRange.aStart.Row() && rFormulaPos.Row() <= rRange.aEnd.Row())
    {
        return ScAddress(rRange.aStart.Col(), rFormulaPos.Row(), rRange.aStart.Tab());
    }

    if (rRange.aStart.Row() == rRange.aEnd.Row()
        && rFormulaPos.Col() >= rRange.aStart.Col() && rFormulaPos.Col() <= rRange.aEnd.Col())
    {
        return ScAddress(rFormulaPos.Col(), rRange.aStart.Row(), rRange.aStart.Tab());
    }

    if (rFormulaPos.Col() >= rRange.aStart.Col() && rFormulaPos.Col() <= rRange.aEnd.Col()
        && rFormulaPos.Row() >= rRange.aStart.Row() && rFormulaPos.Row() <= rRange.aEnd.Row())
    {
        return rFormulaPos;
    }

    return std::nullopt;
}

[[nodiscard]] inline std::optional<ScAddress> formulaTextTargetAddress(
    const core::formula::Node& rNode, const ScDocument& rDoc, const ScAddress& rFormulaPos)
{
    const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
    if (!aRange.mbSupported || !aRange.moValue)
        return std::nullopt;

    if (aRange.moValue->aStart == aRange.moValue->aEnd)
        return aRange.moValue->aStart;

    return tryImplicitIntersectionAddress(*aRange.moValue, rFormulaPos);
}

[[nodiscard]] inline std::optional<api::CellValue> tryMaterializeBoundedReferencedFormulaCellValue(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const ScAddress& rAddress)
{
    ReferencedFormulaMaterializationGuard aGuard(rAddress);
    if (!aGuard.mbActive)
        return std::nullopt;

    const auto aFormulaText = formulainspection::formulaTextForCell(rDoc, rContext, rAddress);
    if (!aFormulaText)
        return std::nullopt;

    const auto aNormalized
        = normalizeFormulaSource(std::u16string_view(aFormulaText.maValue.getStr(), aFormulaText.maValue.getLength()));
    const auto aParse = core::formula::parseFormula(aNormalized);
    if (!aParse || !aParse.mpRoot)
        return std::nullopt;

    const auto& rRoot = *aParse.mpRoot;
    if (rRoot.meKind == core::formula::NodeKind::FunctionCall)
    {
        const api::String aFunctionName = uppercaseAscii(rRoot.maPrimaryText);
        if ((aFunctionName == u"CONCATENATE" || aFunctionName == u"CONCAT")
            && !rRoot.maChildren.empty())
        {
            OUString aResult;
            for (const auto& rxChild : rRoot.maChildren)
            {
                if (!rxChild)
                    return api::CellValue::error(api::Error::IllegalArgument);

                const auto aArgument
                    = materializeScalarNode(*rxChild, rDoc, rContext, rAddress);
                if (!aArgument.mbSupported || !aArgument.moValue)
                    return std::nullopt;

                const auto aText = coerceScalarToText(rDoc, rContext, *aArgument.moValue);
                if (!aText)
                    return api::CellValue::error(aText.meError);
                aResult += aText.maValue;
            }

            return api::CellValue::text(toApiString(aResult));
        }

        if (aFunctionName == u"BASISODATETIME" && rRoot.maChildren.size() == 1)
        {
            const auto aArgument = materializeScalarNode(*rRoot.maChildren[0], rDoc, rContext, rAddress);
            if (!aArgument.mbSupported || !aArgument.moValue)
                return std::nullopt;
            if (aArgument.moValue->isError() || aArgument.moValue->isEmpty())
                return api::CellValue::error(api::Error::IllegalArgument);

            double fSerialValue = 0.0;
            if (aArgument.moValue->isText())
            {
                const auto oParsed = spreadsheetengine::core::datetime::parseStandaloneNumberText(
                    aArgument.moValue->maString);
                if (!oParsed)
                    return api::CellValue::error(api::Error::IllegalArgument);
                fSerialValue = oParsed->mfValue;
            }
            else
            {
                fSerialValue = aArgument.moValue->mfNumber;
            }

            return api::CellValue::text(toApiString(formatBasisDateTime(fSerialValue)));
        }

        auto aAttempt = evaluateDelegatedNode(
            rRoot, rDoc, rContext, rAddress, rDoc.GetCalcConfig().mbEmptyStringAsZero, 1);
        if (!aAttempt.mbSupported)
            return std::nullopt;

        switch (aAttempt.maResult.meType)
        {
            case api::formulavalue::ValueType::Value:
                return api::CellValue::number(aAttempt.maResult.mfValue);
            case api::formulavalue::ValueType::String:
                return api::CellValue::text(aAttempt.maResult.maString);
            case api::formulavalue::ValueType::Error:
                return api::CellValue::error(aAttempt.maResult.meError);
            default:
                return std::nullopt;
        }
    }
    else if (rRoot.meKind == core::formula::NodeKind::BinaryOperation
             && rRoot.meBinaryOperator == core::formula::BinaryOperator::Concat
             && rRoot.maChildren.size() == 2)
    {
        OUString aResult;
        for (const auto& rxChild : rRoot.maChildren)
        {
            if (!rxChild)
                return api::CellValue::error(api::Error::IllegalArgument);

            const auto aArgument = materializeScalarNode(*rxChild, rDoc, rContext, rAddress);
            if (!aArgument.mbSupported || !aArgument.moValue)
                return std::nullopt;

            const auto aText = coerceScalarToText(rDoc, rContext, *aArgument.moValue);
            if (!aText)
                return api::CellValue::error(aText.meError);
            aResult += aText.maValue;
        }

        return api::CellValue::text(toApiString(aResult));
    }

    const auto aScalar = materializeScalarNode(rRoot, rDoc, rContext, rAddress);
    if (!aScalar.mbSupported || !aScalar.moValue)
        return std::nullopt;
    return *aScalar.moValue;
}

[[nodiscard]] inline api::CellValue readMaterializedHostCellValue(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const ScAddress& rAddress)
{
    if (ScFormulaCell* pFormula = const_cast<ScDocument&>(rDoc).GetFormulaCell(rAddress))
    {
        if (const auto oImportedCachedValue
            = spreadsheetengine::compat::libreoffice::tryReadHostCachedFormulaCellValue(
                rDoc, rAddress, *pFormula))
        {
            return *oImportedCachedValue;
        }

        // Local dirty formula dependencies can still expose stale numeric host
        // values here. Prefer the bounded referenced-formula path before we
        // trust those values so authority-mode roots can read fresh helper
        // dates/numbers without forcing full host interpretation first.
        if (pFormula->NeedsInterpret())
        {
            if (const auto oFormulaValue
                = tryMaterializeBoundedReferencedFormulaCellValue(rDoc, rContext, rAddress))
            {
                return *oFormulaValue;
            }
        }
    }

    auto aValue = readHostDocumentCellValue(rDoc, rAddress).maValue;
    if (ScFormulaCell* pFormula = const_cast<ScDocument&>(rDoc).GetFormulaCell(rAddress))
    {
        if (aValue.isEmpty())
        {
            const auto aDisplayValue
                = readHostDocumentCellValue(rDoc, rAddress, HostCellStringKind::Display).maValue;
            if (!aDisplayValue.isEmpty())
                return aDisplayValue;
        }

        if (aValue.isError() && pFormula->GetErrCode() == FormulaError::NONE)
        {
            const auto aDisplayValue
                = readHostDocumentCellValue(rDoc, rAddress, HostCellStringKind::Display).maValue;
            if (!aDisplayValue.isEmpty())
                return aDisplayValue;
        }
    }

    if ((aValue.isEmpty() || aValue.isError()))
    {
        if (const auto oFormulaValue
            = tryMaterializeBoundedReferencedFormulaCellValue(rDoc, rContext, rAddress))
        {
            return *oFormulaValue;
        }
    }
    return aValue;
}

[[nodiscard]] inline api::CellValue readTextParsingHostCellValue(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const ScAddress& rAddress)
{
    if (ScFormulaCell* pFormula = const_cast<ScDocument&>(rDoc).GetFormulaCell(rAddress))
    {
        const bool bImportedCachedFormula = !pFormula->GetHybridFormula().isEmpty();
        if (pFormula->HasHybridStringResult())
        {
            return api::CellValue::text(toApiString(pFormula->GetResultString().getString()));
        }

        if (bImportedCachedFormula && pFormula->NeedsInterpret())
        {
            const sc::FormulaResultValue aStoredResult = pFormula->GetResult();
            if (aStoredResult.meType == sc::FormulaResultValue::String)
            {
                return api::CellValue::text(toApiString(aStoredResult.maString.getString()));
            }
        }
    }

    return readMaterializedHostCellValue(rDoc, rContext, rAddress);
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

[[nodiscard]] inline bool matchesComparisonResult(
    short nCompareResult, core::formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case core::formula::BinaryOperator::Equal:
            return nCompareResult == 0;
        case core::formula::BinaryOperator::NotEqual:
            return nCompareResult != 0;
        case core::formula::BinaryOperator::Less:
            return nCompareResult < 0;
        case core::formula::BinaryOperator::LessEqual:
            return nCompareResult <= 0;
        case core::formula::BinaryOperator::Greater:
            return nCompareResult > 0;
        case core::formula::BinaryOperator::GreaterEqual:
            return nCompareResult >= 0;
        default:
            return false;
    }
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
        case core::formula::NodeKind::FunctionCall:
        {
            const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
            if (aFunctionName == u"TODAY" && rNode.maChildren.empty())
            {
                Date aActDate(Date::SYSTEM);
                const tools::Long nDiff = aActDate - rContext.NFGetNullDate();
                return makeMaterializedValue(
                    api::CellValue::number(static_cast<double>(nDiff)));
            }
            const FunctionKind eFunction = classifyFunction(aFunctionName);
            if (eFunction == FunctionKind::LogicalConstant && rNode.maChildren.empty())
            {
                return makeMaterializedValue(api::CellValue::boolean(aFunctionName == u"TRUE"));
            }
            if (eFunction != FunctionKind::Unknown && eFunction != FunctionKind::Count)
            {
                auto aAttempt = evaluateFunctionNode(
                    rNode, rDoc, rContext, rFormulaPos, rDoc.GetCalcConfig().mbEmptyStringAsZero);
                if (!aAttempt.mbSupported)
                {
                    return makeUnsupportedMaterialization<api::CellValue>(
                        aAttempt.meFallbackReason);
                }

                switch (aAttempt.maResult.meType)
                {
                    case api::formulavalue::ValueType::Value:
                        return makeMaterializedValue(
                            api::CellValue::number(aAttempt.maResult.mfValue));
                    case api::formulavalue::ValueType::String:
                        return makeMaterializedValue(
                            api::CellValue::text(aAttempt.maResult.maString));
                    case api::formulavalue::ValueType::Error:
                        return makeMaterializedValue(
                            api::CellValue::error(aAttempt.maResult.meError));
                    default:
                        break;
                }
            }
            return makeUnsupportedMaterialization<api::CellValue>(
                FallbackReason::UnsupportedFormulaShape);
        }
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
            const auto oScalarAddress = tryImplicitIntersectionAddress(*aRange.moValue, rFormulaPos);
            if (!oScalarAddress || *oScalarAddress == rFormulaPos)
            {
                return makeUnsupportedMaterialization<api::CellValue>(
                    FallbackReason::UnsupportedHostSurface);
            }
            return makeMaterializedValue(
                readMaterializedHostCellValue(rDoc, rContext, *oScalarAddress));
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
                case core::formula::BinaryOperator::Equal:
                case core::formula::BinaryOperator::NotEqual:
                case core::formula::BinaryOperator::Less:
                case core::formula::BinaryOperator::LessEqual:
                case core::formula::BinaryOperator::Greater:
                case core::formula::BinaryOperator::GreaterEqual:
                {
                    if (aLeft.moValue->isText() && aRight.moValue->isText())
                    {
                        const short nCompare = ScGlobal::GetCollator().compareString(
                            toLibreOfficeString(aLeft.moValue->maString),
                            toLibreOfficeString(aRight.moValue->maString));
                        return makeMaterializedValue(api::CellValue::boolean(
                            matchesComparisonResult(nCompare, rNode.meBinaryOperator)));
                    }

                    const auto aLeftNumber = coerceScalarToNumber(rDoc, rContext, *aLeft.moValue);
                    if (!aLeftNumber)
                        return makeMaterializedError<api::CellValue>(aLeftNumber.meError);
                    const auto aRightNumber = coerceScalarToNumber(rDoc, rContext, *aRight.moValue);
                    if (!aRightNumber)
                        return makeMaterializedError<api::CellValue>(aRightNumber.meError);

                    short nCompare = 0;
                    if (!rtl::math::approxEqual(aLeftNumber.maValue, aRightNumber.maValue))
                        nCompare = aLeftNumber.maValue < aRightNumber.maValue ? -1 : 1;
                    return makeMaterializedValue(api::CellValue::boolean(
                        matchesComparisonResult(nCompare, rNode.meBinaryOperator)));
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

[[nodiscard]] inline Materialization<ScMatrixRef> materializeReferencedMatrix(
    const ScRange& rRange, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rRange.aStart.Tab() != rRange.aEnd.Tab())
        return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedHostSurface);

    if (rRange.aStart == rFormulaPos && rRange.aEnd == rFormulaPos)
        return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedHostSurface);

    const SCSIZE nColumns = static_cast<SCSIZE>(rRange.aEnd.Col() - rRange.aStart.Col() + 1);
    const SCSIZE nRows = static_cast<SCSIZE>(rRange.aEnd.Row() - rRange.aStart.Row() + 1);
    ScMatrixRef xMatrix(new ScMatrix(nColumns, nRows));
    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
        {
            const ScAddress aAddress(rRange.aStart.Col() + static_cast<SCCOL>(nColumn),
                rRange.aStart.Row() + static_cast<SCROW>(nRow), rRange.aStart.Tab());
            putScalarIntoMatrix(readMaterializedHostCellValue(rDoc, rContext, aAddress), xMatrix,
                nColumn, nRow);
        }
    }

    return makeMaterializedValue(xMatrix);
}

[[nodiscard]] inline api::CellValue readMaterializedHostCellValue(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const ScAddress& rAddress);

class CriteriaAggregateMaterializer final
    : public spreadsheetengine::core::query::CriteriaAggregateMaterializer
{
    const ScDocument& mrDoc;
    ScInterpreterContext& mrContext;

public:
    CriteriaAggregateMaterializer(const ScDocument& rDoc, ScInterpreterContext& rContext)
        : mrDoc(rDoc)
        , mrContext(rContext)
    {
    }

    [[nodiscard]] api::ValueResult<api::CellValue> materialize(
        const spreadsheetengine::core::query::CriteriaAggregateInput& rInput,
        api::MatrixCoordinate aCoordinate) const override
    {
        if (rInput.mbScalar)
        {
            if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(rInput.maScalar);
        }

        if (!rInput.maValues.empty())
        {
            const std::int64_t nLinearIndex
                = static_cast<std::int64_t>(aCoordinate.mnRow) * rInput.mnColumns
                  + aCoordinate.mnColumn;
            if (nLinearIndex < 0
                || static_cast<std::size_t>(nLinearIndex) >= rInput.maValues.size())
            {
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            }
            return api::ValueResult<api::CellValue>::success(
                rInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
        }

        if (!rInput.maReference.containsOffset(aCoordinate.mnColumn, aCoordinate.mnRow))
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);

        const auto aAddress = rInput.maReference.addressAt(aCoordinate.mnColumn, aCoordinate.mnRow);
        return api::ValueResult<api::CellValue>::success(readMaterializedHostCellValue(mrDoc,
            mrContext, ScAddress(aAddress.mnColumn, aAddress.mnRow, aAddress.mnSheet)));
    }
};

[[nodiscard]] inline Materialization<spreadsheetengine::core::query::CriteriaAggregateInput>
materializeCriteriaAggregateInput(const core::formula::Node& rArgument, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    using CriteriaAggregateInput = spreadsheetengine::core::query::CriteriaAggregateInput;

    if (rArgument.meKind == core::formula::NodeKind::CellReference
        || rArgument.meKind == core::formula::NodeKind::RangeReference
        || rArgument.meKind == core::formula::NodeKind::NamedReference)
    {
        const auto aRange = resolveReferenceRangeNode(rArgument, rDoc, rFormulaPos);
        if (!aRange.mbSupported)
            return makeUnsupportedMaterialization<CriteriaAggregateInput>(aRange.meFallbackReason);
        if (!aRange.moValue)
            return makeMaterializedError<CriteriaAggregateInput>(aRange.meError);
        if (aRange.moValue->aStart.Tab() != aRange.moValue->aEnd.Tab())
        {
            return makeUnsupportedMaterialization<CriteriaAggregateInput>(
                FallbackReason::UnsupportedHostSurface);
        }

        CriteriaAggregateInput aInput;
        aInput.mbScalar = false;
        aInput.maReference = { toApiCellRange(*aRange.moValue) };
        const auto aDimensions = aInput.maReference.matrixDimensions();
        aInput.mnColumns = aDimensions.mnColumns;
        aInput.mnRows = aDimensions.mnRows;
        return makeMaterializedValue(aInput);
    }

    const bool bMatrixLike = rArgument.meKind == core::formula::NodeKind::ArrayConstant
                             || rArgument.meKind == core::formula::NodeKind::BinaryOperation
                             || rArgument.meKind == core::formula::NodeKind::FunctionCall;
    if (bMatrixLike)
    {
        const auto aMatrix = materializeMatrixNode(rArgument, rDoc, rContext, rFormulaPos);
        if (aMatrix.mbSupported && aMatrix.moValue)
        {
            CriteriaAggregateInput aInput;
            aInput.mbScalar = false;
            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
            aInput.mnColumns = static_cast<api::MatrixSize>(nColumns);
            aInput.mnRows = static_cast<api::MatrixSize>(nRows);
            aInput.maValues.reserve(aInput.mnColumns * aInput.mnRows);
            for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    aInput.maValues.push_back(lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(nColumn, nRow)));
                }
            }
            return makeMaterializedValue(std::move(aInput));
        }
        if (aMatrix.mbSupported && !aMatrix.moValue)
            return makeMaterializedError<CriteriaAggregateInput>(aMatrix.meError);
        if (!aMatrix.mbSupported && rArgument.meKind != core::formula::NodeKind::FunctionCall)
            return makeUnsupportedMaterialization<CriteriaAggregateInput>(aMatrix.meFallbackReason);
    }

    const auto aScalar = materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
    if (!aScalar.mbSupported)
        return makeUnsupportedMaterialization<CriteriaAggregateInput>(aScalar.meFallbackReason);
    if (!aScalar.moValue)
        return makeMaterializedError<CriteriaAggregateInput>(aScalar.meError);

    CriteriaAggregateInput aInput;
    aInput.maScalar = *aScalar.moValue;
    return makeMaterializedValue(aInput);
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeMatrixBinaryOperation(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.maChildren.size() != 2)
        return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedFormulaShape);

    const auto aLeft = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aLeft.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aLeft.meFallbackReason);
    if (!aLeft.moValue)
        return makeMaterializedError<ScMatrixRef>(aLeft.meError);

    const auto aRight = materializeMatrixNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
    if (!aRight.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aRight.meFallbackReason);
    if (!aRight.moValue)
        return makeMaterializedError<ScMatrixRef>(aRight.meError);

    SCSIZE nLeftColumns = 0;
    SCSIZE nLeftRows = 0;
    SCSIZE nRightColumns = 0;
    SCSIZE nRightRows = 0;
    (*aLeft.moValue)->GetDimensions(nLeftColumns, nLeftRows);
    (*aRight.moValue)->GetDimensions(nRightColumns, nRightRows);

    const SCSIZE nResultColumns = std::max(nLeftColumns, nRightColumns);
    const SCSIZE nResultRows = std::max(nLeftRows, nRightRows);
    ScMatrixRef xMatrix(new ScMatrix(nResultColumns, nResultRows));
    for (SCSIZE nRow = 0; nRow < nResultRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nResultColumns; ++nColumn)
        {
            SCSIZE nLeftColumn = nColumn;
            SCSIZE nLeftRow = nRow;
            SCSIZE nRightColumn = nColumn;
            SCSIZE nRightRow = nRow;
            if (!(*aLeft.moValue)->ValidColRowOrReplicated(nLeftColumn, nLeftRow)
                || !(*aRight.moValue)->ValidColRowOrReplicated(nRightColumn, nRightRow))
            {
                return makeUnsupportedMaterialization<ScMatrixRef>(
                    FallbackReason::UnsupportedFormulaShape);
            }

            const auto aLeftValue = lookupexecution::detail::toApiCellValue(
                (*aLeft.moValue)->Get(nLeftColumn, nLeftRow));
            const auto aRightValue = lookupexecution::detail::toApiCellValue(
                (*aRight.moValue)->Get(nRightColumn, nRightRow));
            if (aLeftValue.isError())
            {
                putScalarIntoMatrix(aLeftValue, xMatrix, nColumn, nRow);
                continue;
            }
            if (aRightValue.isError())
            {
                putScalarIntoMatrix(aRightValue, xMatrix, nColumn, nRow);
                continue;
            }

            std::optional<api::CellValue> oResult;
            switch (rNode.meBinaryOperator)
            {
                case core::formula::BinaryOperator::Add:
                case core::formula::BinaryOperator::Subtract:
                case core::formula::BinaryOperator::Multiply:
                case core::formula::BinaryOperator::Divide:
                case core::formula::BinaryOperator::Power:
                {
                    const auto aLeftNumber = coerceScalarToNumber(rDoc, rContext, aLeftValue);
                    if (!aLeftNumber)
                    {
                        putScalarIntoMatrix(api::CellValue::error(aLeftNumber.meError), xMatrix,
                            nColumn, nRow);
                        continue;
                    }
                    const auto aRightNumber = coerceScalarToNumber(rDoc, rContext, aRightValue);
                    if (!aRightNumber)
                    {
                        putScalarIntoMatrix(api::CellValue::error(aRightNumber.meError), xMatrix,
                            nColumn, nRow);
                        continue;
                    }

                    if (rNode.meBinaryOperator == core::formula::BinaryOperator::Add)
                        oResult = api::CellValue::number(aLeftNumber.maValue + aRightNumber.maValue);
                    else if (rNode.meBinaryOperator == core::formula::BinaryOperator::Subtract)
                        oResult = api::CellValue::number(aLeftNumber.maValue - aRightNumber.maValue);
                    else if (rNode.meBinaryOperator == core::formula::BinaryOperator::Multiply)
                        oResult = api::CellValue::number(aLeftNumber.maValue * aRightNumber.maValue);
                    else if (rNode.meBinaryOperator == core::formula::BinaryOperator::Divide)
                    {
                        if (aRightNumber.maValue == 0.0)
                            oResult = api::CellValue::error(api::Error::DivisionByZero);
                        else
                            oResult = api::CellValue::number(
                                aLeftNumber.maValue / aRightNumber.maValue);
                    }
                    else
                    {
                        const double fValue = std::pow(aLeftNumber.maValue, aRightNumber.maValue);
                        oResult = std::isfinite(fValue)
                                      ? api::CellValue::number(fValue)
                                      : api::CellValue::error(api::Error::Domain);
                    }
                    break;
                }
                case core::formula::BinaryOperator::Equal:
                case core::formula::BinaryOperator::NotEqual:
                case core::formula::BinaryOperator::Less:
                case core::formula::BinaryOperator::LessEqual:
                case core::formula::BinaryOperator::Greater:
                case core::formula::BinaryOperator::GreaterEqual:
                {
                    short nCompare = 0;
                    if (aLeftValue.isText() && aRightValue.isText())
                    {
                        nCompare = ScGlobal::GetCollator().compareString(
                            toLibreOfficeString(aLeftValue.maString),
                            toLibreOfficeString(aRightValue.maString));
                    }
                    else
                    {
                        const auto aLeftNumber = coerceScalarToNumber(rDoc, rContext, aLeftValue);
                        if (!aLeftNumber)
                        {
                            putScalarIntoMatrix(api::CellValue::error(aLeftNumber.meError), xMatrix,
                                nColumn, nRow);
                            continue;
                        }
                        const auto aRightNumber = coerceScalarToNumber(rDoc, rContext, aRightValue);
                        if (!aRightNumber)
                        {
                            putScalarIntoMatrix(api::CellValue::error(aRightNumber.meError), xMatrix,
                                nColumn, nRow);
                            continue;
                        }

                        if (!rtl::math::approxEqual(aLeftNumber.maValue, aRightNumber.maValue))
                            nCompare = aLeftNumber.maValue < aRightNumber.maValue ? -1 : 1;
                    }

                    oResult = api::CellValue::boolean(
                        matchesComparisonResult(nCompare, rNode.meBinaryOperator));
                    break;
                }
                default:
                    return makeUnsupportedMaterialization<ScMatrixRef>(
                        FallbackReason::UnsupportedFormulaShape);
            }

            putScalarIntoMatrix(*oResult, xMatrix, nColumn, nRow);
        }
    }

    return makeMaterializedValue(xMatrix);
}

[[nodiscard]] inline ScMatrixRef makeSingleValueMatrix(const api::CellValue& rValue)
{
    ScMatrixRef xMatrix(new ScMatrix(1, 1));
    putScalarIntoMatrix(rValue, xMatrix, 0, 0);
    return xMatrix;
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeLookupExecutionResultMatrix(
    const lookupexecution::LookupExecutionResult& rResult, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    if (rResult.meKind == lookupexecution::LookupExecutionResult::Kind::Scalar)
        return makeMaterializedValue(makeSingleValueMatrix(rResult.maScalar));

    if (rResult.meKind == lookupexecution::LookupExecutionResult::Kind::Reference)
        return materializeReferencedMatrix(rResult.maRange, rDoc, rContext, rFormulaPos);

    if (rResult.meKind == lookupexecution::LookupExecutionResult::Kind::Matrix && rResult.mpMatrix)
        return makeMaterializedValue(rResult.mpMatrix);

    return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedHostSurface);
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeMatrixSlice(
    const ScMatrixRef& pSource, const api::reference::IndexMatrixSelection& rSelection)
{
    if (!pSource)
        return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedHostSurface);

    if (rSelection.meKind == api::reference::IndexSelectionKind::KeepSource)
        return makeMaterializedValue(pSource);

    ScMatrixRef xMatrix(new ScMatrix(static_cast<SCSIZE>(rSelection.maDimensions.mnColumns),
        static_cast<SCSIZE>(rSelection.maDimensions.mnRows)));
    for (api::MatrixSize nRow = 0; nRow < rSelection.maDimensions.mnRows; ++nRow)
    {
        for (api::MatrixSize nColumn = 0; nColumn < rSelection.maDimensions.mnColumns; ++nColumn)
        {
            const SCSIZE nSourceColumn
                = static_cast<SCSIZE>(rSelection.maStart.mnColumn + nColumn);
            const SCSIZE nSourceRow = static_cast<SCSIZE>(rSelection.maStart.mnRow + nRow);
            putScalarIntoMatrix(
                lookupexecution::detail::toApiCellValue(pSource->Get(nSourceColumn, nSourceRow)),
                xMatrix, static_cast<SCSIZE>(nColumn), static_cast<SCSIZE>(nRow));
        }
    }

    return makeMaterializedValue(xMatrix);
}

[[nodiscard]] inline Materialization<sal_Int32> normalizeWholeMaterializedArgument(
    const core::formula::Node& rArgument, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    const auto aArgument = materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
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
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeXLookupMatrixFunctionCall(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
        return makeMaterializedValue(makeSingleValueMatrix(api::CellValue::error(
            api::Error::IllegalArgument)));

    const auto aLookup = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aLookup.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aLookup.meFallbackReason);
    if (!aLookup.moValue)
        return makeMaterializedError<ScMatrixRef>(aLookup.meError);
    if (aLookup.moValue->isError())
        return makeMaterializedValue(makeSingleValueMatrix(*aLookup.moValue));

    const auto aSearch = materializeLookupInputSourceNode(
        *rNode.maChildren[1], rDoc, rContext, rFormulaPos);
    if (!aSearch.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aSearch.meFallbackReason);
    if (!aSearch.moValue)
        return makeMaterializedError<ScMatrixRef>(aSearch.meError);
    const auto aSearchInput = lookupexecution::detail::buildLookupInput(*aSearch.moValue);
    if (!aSearchInput)
        return makeMaterializedError<ScMatrixRef>(aSearchInput.meError);

    const auto aResult = materializeLookupInputSourceNode(
        *rNode.maChildren[2], rDoc, rContext, rFormulaPos);
    if (!aResult.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aResult.meFallbackReason);
    if (!aResult.moValue)
        return makeMaterializedError<ScMatrixRef>(aResult.meError);
    const auto aResultInput = lookupexecution::detail::buildLookupInput(*aResult.moValue);
    if (!aResultInput)
        return makeMaterializedError<ScMatrixRef>(aResultInput.meError);

    lookupexecution::XLookupExecutionRequest aRequest;
    aRequest.maLookupValue = *aLookup.moValue;
    aRequest.maSearchInput = aSearchInput.maValue;
    aRequest.maResultInput = aResultInput.maValue;
    aRequest.meSearchType = searchTypeFromDocument(rDoc);
    aRequest.mbAllowPatternMatch = true;

    if (rNode.maChildren.size() >= 5
        && rNode.maChildren[4]->meKind != core::formula::NodeKind::EmptyArgument)
    {
        const auto aMode
            = normalizeWholeMaterializedArgument(*rNode.maChildren[4], rDoc, rContext, rFormulaPos);
        if (!aMode.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aMode.meFallbackReason);
        if (!aMode.moValue)
            return makeMaterializedError<ScMatrixRef>(aMode.meError);
        const auto aNormalized = api::lookup::normalizeExtendedMatchMode(
            static_cast<std::int16_t>(*aMode.moValue));
        if (!aNormalized)
            return makeMaterializedError<ScMatrixRef>(aNormalized.meError);
        aRequest.meMatchMode = aNormalized.maValue;
    }

    if (rNode.maChildren.size() >= 6
        && rNode.maChildren[5]->meKind != core::formula::NodeKind::EmptyArgument)
    {
        const auto aMode
            = normalizeWholeMaterializedArgument(*rNode.maChildren[5], rDoc, rContext, rFormulaPos);
        if (!aMode.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aMode.meFallbackReason);
        if (!aMode.moValue)
            return makeMaterializedError<ScMatrixRef>(aMode.meError);
        const auto aNormalized = api::lookup::normalizeSearchMode(
            static_cast<std::int16_t>(*aMode.moValue));
        if (!aNormalized)
            return makeMaterializedError<ScMatrixRef>(aNormalized.meError);
        aRequest.meSearchMode = aNormalized.maValue;
    }

    const auto aResolved = lookupexecution::resolveXLookupResult(rDoc, rContext, aRequest);
    if (!aResolved)
    {
        if (aResolved.meError == api::Error::NotAvailable && rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            return materializeMatrixNode(*rNode.maChildren[3], rDoc, rContext, rFormulaPos);
        }
        return makeMaterializedValue(makeSingleValueMatrix(api::CellValue::error(aResolved.meError)));
    }

    return materializeLookupExecutionResultMatrix(aResolved.maValue, rDoc, rContext, rFormulaPos);
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeIndexMatrixFunctionCall(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
        return makeMaterializedValue(makeSingleValueMatrix(api::CellValue::error(
            api::Error::IllegalArgument)));

    const auto aRow
        = normalizeWholeMaterializedArgument(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
    if (!aRow.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aRow.meFallbackReason);
    if (!aRow.moValue || *aRow.moValue < 0)
    {
        return makeMaterializedValue(makeSingleValueMatrix(
            api::CellValue::error(aRow.moValue ? api::Error::IllegalArgument : aRow.meError)));
    }

    sal_Int32 nColumn = 0;
    if (rNode.maChildren.size() == 3)
    {
        const auto aColumn = normalizeWholeMaterializedArgument(
            *rNode.maChildren[2], rDoc, rContext, rFormulaPos);
        if (!aColumn.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aColumn.meFallbackReason);
        if (!aColumn.moValue || *aColumn.moValue < 0)
        {
            return makeMaterializedValue(makeSingleValueMatrix(api::CellValue::error(
                aColumn.moValue ? api::Error::IllegalArgument : aColumn.meError)));
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
            return makeMaterializedError<ScMatrixRef>(aSelection.meError);
        return materializeReferencedMatrix(
            toLibreOfficeRange(aSelection.maValue.maRange), rDoc, rContext, rFormulaPos);
    }
    if (!aReferenceSource.mbSupported
        && aReferenceSource.meFallbackReason != FallbackReason::UnsupportedFormulaShape)
    {
        return makeUnsupportedMaterialization<ScMatrixRef>(aReferenceSource.meFallbackReason);
    }

    const auto aMatrixSource = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aMatrixSource.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aMatrixSource.meFallbackReason);
    if (!aMatrixSource.moValue)
        return makeMaterializedError<ScMatrixRef>(aMatrixSource.meError);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    (*aMatrixSource.moValue)->GetDimensions(nColumns, nRows);
    const auto aSelection = api::reference::planIndexMatrixSelection(
        { static_cast<api::MatrixSize>(nColumns), static_cast<api::MatrixSize>(nRows) },
        *aRow.moValue, nColumn, rNode.maChildren.size() < 3,
        static_cast<std::uint8_t>(rNode.maChildren.size()));
    if (!aSelection)
        return makeMaterializedError<ScMatrixRef>(aSelection.meError);
    return materializeMatrixSlice(*aMatrixSource.moValue, aSelection.maValue);
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeSelectorMatrixFunctionCall(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.maChildren.size() < 2)
    {
        return makeMaterializedValue(
            makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
    }

    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    const auto oCanonical = canonicalSelectorFunctionName(aFunctionName);
    if (!oCanonical)
        return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedFunction);
    const bool bChooseColumns = *oCanonical == u"CHOOSECOLS";

    const auto aSource = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aSource.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aSource.meFallbackReason);
    if (!aSource.moValue)
        return makeMaterializedError<ScMatrixRef>(aSource.meError);

    SCSIZE nSourceColumns = 0;
    SCSIZE nSourceRows = 0;
    (*aSource.moValue)->GetDimensions(nSourceColumns, nSourceRows);
    if (nSourceColumns < 1 || nSourceRows < 1)
    {
        return makeMaterializedValue(
            makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
    }

    const api::MatrixSize nSelectableDimension = static_cast<api::MatrixSize>(
        bChooseColumns ? nSourceColumns : nSourceRows);
    std::vector<api::MatrixSize> aSelections;
    for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
    {
        const auto& rxChild = rNode.maChildren[nIndex];
        if (!rxChild || rxChild->meKind == core::formula::NodeKind::EmptyArgument)
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        const auto aSelectionMatrix = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
        if (!aSelectionMatrix.mbSupported)
        {
            return makeUnsupportedMaterialization<ScMatrixRef>(aSelectionMatrix.meFallbackReason);
        }
        if (!aSelectionMatrix.moValue)
            return makeMaterializedError<ScMatrixRef>(aSelectionMatrix.meError);

        SCSIZE nSelectionColumns = 0;
        SCSIZE nSelectionRows = 0;
        (*aSelectionMatrix.moValue)->GetDimensions(nSelectionColumns, nSelectionRows);
        for (SCSIZE nRow = 0; nRow < nSelectionRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nSelectionColumns; ++nColumn)
            {
                const auto aValue = lookupexecution::detail::toApiCellValue(
                    (*aSelectionMatrix.moValue)->Get(nColumn, nRow));
                if (aValue.isError())
                {
                    return makeMaterializedValue(makeSingleValueMatrix(aValue));
                }
                if (aValue.isEmpty() || aValue.isText())
                {
                    return makeMaterializedValue(makeSingleValueMatrix(
                        api::CellValue::error(api::Error::IllegalArgument)));
                }

                const auto aNumber = coerceScalarToNumber(rDoc, rContext, aValue);
                if (!aNumber)
                {
                    return makeMaterializedValue(makeSingleValueMatrix(
                        api::CellValue::error(aNumber.meError)));
                }
                if (!std::isfinite(aNumber.maValue)
                    || aNumber.maValue < static_cast<double>(std::numeric_limits<std::int32_t>::min())
                    || aNumber.maValue > static_cast<double>(std::numeric_limits<std::int32_t>::max()))
                {
                    return makeMaterializedValue(makeSingleValueMatrix(
                        api::CellValue::error(api::Error::IllegalArgument)));
                }

                const std::int32_t nRequestedIndex = static_cast<std::int32_t>(aNumber.maValue);
                const auto aSelection = api::array::normalizeSelectionIndex(
                    nRequestedIndex, nSelectableDimension);
                if (!aSelection)
                {
                    return makeMaterializedValue(makeSingleValueMatrix(
                        api::CellValue::error(aSelection.meError)));
                }

                aSelections.push_back(aSelection.maValue);
            }
        }
    }

    if (aSelections.empty())
    {
        return makeMaterializedValue(
            makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
    }

    const SCSIZE nResultColumns
        = bChooseColumns ? static_cast<SCSIZE>(aSelections.size()) : nSourceColumns;
    const SCSIZE nResultRows
        = bChooseColumns ? nSourceRows : static_cast<SCSIZE>(aSelections.size());
    ScMatrixRef xMatrix(new ScMatrix(nResultColumns, nResultRows));
    if (bChooseColumns)
    {
        for (std::size_t nSelectionIndex = 0; nSelectionIndex < aSelections.size(); ++nSelectionIndex)
        {
            const SCSIZE nSourceColumn = static_cast<SCSIZE>(aSelections[nSelectionIndex]);
            for (SCSIZE nRow = 0; nRow < nSourceRows; ++nRow)
            {
                putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                        (*aSource.moValue)->Get(nSourceColumn, nRow)),
                    xMatrix, static_cast<SCSIZE>(nSelectionIndex), nRow);
            }
        }
    }
    else
    {
        for (std::size_t nSelectionIndex = 0; nSelectionIndex < aSelections.size(); ++nSelectionIndex)
        {
            const SCSIZE nSourceRow = static_cast<SCSIZE>(aSelections[nSelectionIndex]);
            for (SCSIZE nColumn = 0; nColumn < nSourceColumns; ++nColumn)
            {
                putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                        (*aSource.moValue)->Get(nColumn, nSourceRow)),
                    xMatrix, nColumn, static_cast<SCSIZE>(nSelectionIndex));
            }
        }
    }

    return makeMaterializedValue(xMatrix);
}

[[nodiscard]] inline bool spillValuesEqual(const api::CellValue& rLeft, const api::CellValue& rRight,
    const ScDocument& rDoc, ScInterpreterContext& rContext)
{
    if (rLeft.isError() || rRight.isError())
        return rLeft.isError() && rRight.isError() && rLeft.meError == rRight.meError;
    if (rLeft.isEmpty() || rRight.isEmpty())
        return rLeft.isEmpty() && rRight.isEmpty();
    if (rLeft.isText() || rRight.isText())
    {
        if (!rLeft.isText() || !rRight.isText())
            return false;
        return spreadsheetengine::core::query::compareFoldedText(rLeft.maString, rRight.maString)
               == 0;
    }

    const auto aLeftNumber = coerceScalarToNumber(rDoc, rContext, rLeft);
    const auto aRightNumber = coerceScalarToNumber(rDoc, rContext, rRight);
    if (!aLeftNumber || !aRightNumber)
        return false;
    return rtl::math::approxEqual(aLeftNumber.maValue, aRightNumber.maValue);
}

[[nodiscard]] inline api::ValueResult<int> spillCompareValues(
    const api::CellValue& rLeft, const api::CellValue& rRight, const ScDocument& rDoc,
    ScInterpreterContext& rContext)
{
    if (rLeft.isError() && rRight.isError())
        return api::ValueResult<int>::success(
            static_cast<int>(rLeft.meError) - static_cast<int>(rRight.meError));
    if (rLeft.isError())
        return api::ValueResult<int>::success(1);
    if (rRight.isError())
        return api::ValueResult<int>::success(-1);

    if (rLeft.isEmpty() && rRight.isEmpty())
        return api::ValueResult<int>::success(0);
    if (rLeft.isEmpty())
        return api::ValueResult<int>::success(1);
    if (rRight.isEmpty())
        return api::ValueResult<int>::success(-1);

    const bool bLeftText = rLeft.isText();
    const bool bRightText = rRight.isText();
    if (bLeftText && bRightText)
        return api::ValueResult<int>::success(
            spreadsheetengine::core::query::compareFoldedText(
                rLeft.maString, rRight.maString));
    if (bLeftText)
        return api::ValueResult<int>::success(1);
    if (bRightText)
        return api::ValueResult<int>::success(-1);

    const auto aLeftNumber = coerceScalarToNumber(rDoc, rContext, rLeft);
    const auto aRightNumber = coerceScalarToNumber(rDoc, rContext, rRight);
    if (!aLeftNumber || !aRightNumber)
        return api::ValueResult<int>::failure(api::Error::IllegalArgument);
    if (rtl::math::approxEqual(aLeftNumber.maValue, aRightNumber.maValue))
        return api::ValueResult<int>::success(0);
    return api::ValueResult<int>::success(
        aLeftNumber.maValue < aRightNumber.maValue ? -1 : 1);
}

[[nodiscard]] inline std::vector<api::String> spillSplitText(
    const api::String& rText, const std::vector<api::String>& rDelimiters, bool bIgnoreEmpty,
    bool bMatchMode)
{
    std::vector<api::String> aParts;
    if (rDelimiters.empty() || rText.empty())
    {
        aParts.push_back(rText);
        return aParts;
    }

    const api::String aSearchText
        = bMatchMode ? api::text::lowercase(
                           spreadsheetengine::core::text::defaultCaseMappingService(), rText)
                     : rText;
    std::size_t nStart = 0;
    while (nStart < rText.size())
    {
        std::size_t nBestIndex = rText.size();
        std::size_t nBestLength = 0;
        for (const auto& rDelimiter : rDelimiters)
        {
            if (rDelimiter.empty())
                continue;
            const api::String aSearchDelimiter = bMatchMode
                                                     ? api::text::lowercase(
                                                           spreadsheetengine::core::text::defaultCaseMappingService(),
                                                           rDelimiter)
                                                     : rDelimiter;
            const std::size_t nIndex = aSearchText.find(aSearchDelimiter, nStart);
            if (nIndex != api::String::npos && nIndex < nBestIndex)
            {
                nBestIndex = nIndex;
                nBestLength = rDelimiter.size();
            }
        }

        const std::size_t nSliceEnd = nBestIndex == api::String::npos ? rText.size() : nBestIndex;
        api::String aPart = rText.substr(nStart, nSliceEnd - nStart);
        if (!bIgnoreEmpty || !aPart.empty())
            aParts.push_back(std::move(aPart));

        if (nBestIndex == api::String::npos || nBestIndex >= rText.size())
            break;
        nStart = nBestIndex + nBestLength;
    }

    return aParts;
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeSpillMatrixFunctionCall(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    const auto oCanonical = canonicalSpillFunctionName(aFunctionName);
    if (!oCanonical)
        return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedFunction);

    const auto collectTextVector = [&](const core::formula::Node& rArgument)
        -> Materialization<std::vector<api::String>> {
        std::vector<api::String> aValues;
        const auto aMatrix = materializeMatrixNode(rArgument, rDoc, rContext, rFormulaPos);
        if (!aMatrix.mbSupported)
            return makeUnsupportedMaterialization<std::vector<api::String>>(
                aMatrix.meFallbackReason);
        if (!aMatrix.moValue)
            return makeMaterializedError<std::vector<api::String>>(aMatrix.meError);

        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
        for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            {
                const auto aValue = lookupexecution::detail::toApiCellValue(
                    (*aMatrix.moValue)->Get(nColumn, nRow));
                if (aValue.isError())
                    return makeMaterializedError<std::vector<api::String>>(aValue.meError);
                const auto aText = coerceScalarToText(rDoc, rContext, aValue);
                if (!aText)
                    return makeMaterializedError<std::vector<api::String>>(aText.meError);
                aValues.push_back(toApiString(aText.maValue));
            }
        }

        return makeMaterializedValue(std::move(aValues));
    };

    if (*oCanonical == u"UNIQUE")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        const auto aSource = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aSource.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aSource.meFallbackReason);
        if (!aSource.moValue)
            return makeMaterializedError<ScMatrixRef>(aSource.meError);

        bool bByColumn = false;
        if (rNode.maChildren.size() >= 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aByColumn = materializeScalarNode(
                *rNode.maChildren[1], rDoc, rContext, rFormulaPos);
            if (!aByColumn.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aByColumn.meFallbackReason);
            if (!aByColumn.moValue)
                return makeMaterializedError<ScMatrixRef>(aByColumn.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aByColumn.moValue);
            if (!aBool)
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(aBool.meError)));
            bByColumn = aBool.maValue;
        }

        bool bExactlyOnce = false;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aExactlyOnce = materializeScalarNode(
                *rNode.maChildren[2], rDoc, rContext, rFormulaPos);
            if (!aExactlyOnce.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aExactlyOnce.meFallbackReason);
            if (!aExactlyOnce.moValue)
                return makeMaterializedError<ScMatrixRef>(aExactlyOnce.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aExactlyOnce.moValue);
            if (!aBool)
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(aBool.meError)));
            bExactlyOnce = aBool.maValue;
        }

        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        (*aSource.moValue)->GetDimensions(nColumns, nRows);
        const SCSIZE nUnits = bByColumn ? nColumns : nRows;
        if (nUnits < 1)
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        auto unitEquals = [&](SCSIZE nLeft, SCSIZE nRight) {
            if (bByColumn)
            {
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    const auto aLeft = lookupexecution::detail::toApiCellValue(
                        (*aSource.moValue)->Get(nLeft, nRow));
                    const auto aRight = lookupexecution::detail::toApiCellValue(
                        (*aSource.moValue)->Get(nRight, nRow));
                    if (!spillValuesEqual(aLeft, aRight, rDoc, rContext))
                        return false;
                }
                return true;
            }

            for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            {
                const auto aLeft = lookupexecution::detail::toApiCellValue(
                    (*aSource.moValue)->Get(nColumn, nLeft));
                const auto aRight = lookupexecution::detail::toApiCellValue(
                    (*aSource.moValue)->Get(nColumn, nRight));
                if (!spillValuesEqual(aLeft, aRight, rDoc, rContext))
                    return false;
            }
            return true;
        };

        std::vector<SCSIZE> aSelections;
        for (SCSIZE nIndex = 0; nIndex < nUnits; ++nIndex)
        {
            std::size_t nOccurrences = 0;
            bool bSeenEarlier = false;
            for (SCSIZE nOther = 0; nOther < nUnits; ++nOther)
            {
                if (!unitEquals(nIndex, nOther))
                    continue;
                ++nOccurrences;
                if (nOther < nIndex)
                    bSeenEarlier = true;
            }

            if (bExactlyOnce)
            {
                if (nOccurrences == 1)
                    aSelections.push_back(nIndex);
            }
            else if (!bSeenEarlier)
            {
                aSelections.push_back(nIndex);
            }
        }

        if (aSelections.empty())
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::NotAvailable)));
        }

        const SCSIZE nResultColumns = bByColumn ? static_cast<SCSIZE>(aSelections.size()) : nColumns;
        const SCSIZE nResultRows = bByColumn ? nRows : static_cast<SCSIZE>(aSelections.size());
        ScMatrixRef xMatrix(new ScMatrix(nResultColumns, nResultRows));
        if (bByColumn)
        {
            for (std::size_t nSelectionIndex = 0; nSelectionIndex < aSelections.size();
                 ++nSelectionIndex)
            {
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                            (*aSource.moValue)->Get(aSelections[nSelectionIndex], nRow)),
                        xMatrix, static_cast<SCSIZE>(nSelectionIndex), nRow);
                }
            }
        }
        else
        {
            for (std::size_t nSelectionIndex = 0; nSelectionIndex < aSelections.size();
                 ++nSelectionIndex)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                            (*aSource.moValue)->Get(nColumn, aSelections[nSelectionIndex])),
                        xMatrix, nColumn, static_cast<SCSIZE>(nSelectionIndex));
                }
            }
        }

        return makeMaterializedValue(xMatrix);
    }

    if (*oCanonical == u"HSTACK")
    {
        if (rNode.maChildren.empty())
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        std::vector<ScMatrixRef> aInputs;
        SCSIZE nResultColumns = 0;
        SCSIZE nResultRows = 0;
        for (const auto& rxChild : rNode.maChildren)
        {
            if (!rxChild)
            {
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
            }
            const auto aInput = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
            if (!aInput.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aInput.meFallbackReason);
            if (!aInput.moValue)
                return makeMaterializedError<ScMatrixRef>(aInput.meError);

            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aInput.moValue)->GetDimensions(nColumns, nRows);
            nResultColumns += nColumns;
            nResultRows = std::max(nResultRows, nRows);
            aInputs.push_back(*aInput.moValue);
        }

        ScMatrixRef xMatrix(new ScMatrix(nResultColumns, nResultRows));
        SCSIZE nDestColumn = 0;
        for (const auto& xInput : aInputs)
        {
            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            xInput->GetDimensions(nColumns, nRows);
            for (SCSIZE nRow = 0; nRow < nResultRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    const api::CellValue aValue = nRow < nRows
                                                      ? lookupexecution::detail::toApiCellValue(
                                                            xInput->Get(nColumn, nRow))
                                                      : api::CellValue::error(api::Error::NotAvailable);
                    putScalarIntoMatrix(aValue, xMatrix, nDestColumn + nColumn, nRow);
                }
            }
            nDestColumn += nColumns;
        }

        return makeMaterializedValue(xMatrix);
    }

    if (*oCanonical == u"SORT" || *oCanonical == u"SORTBY")
    {
        if (*oCanonical == u"SORT")
        {
            if (rNode.maChildren.empty() || rNode.maChildren.size() > 4)
            {
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
            }

            const auto aSource = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
            if (!aSource.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aSource.meFallbackReason);
            if (!aSource.moValue)
                return makeMaterializedError<ScMatrixRef>(aSource.meError);

            sal_Int32 nSortIndex = 1;
            if (rNode.maChildren.size() >= 2
                && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
            {
                const auto aIndex = normalizeWholeMaterializedArgument(
                    *rNode.maChildren[1], rDoc, rContext, rFormulaPos);
                if (!aIndex.mbSupported)
                    return makeUnsupportedMaterialization<ScMatrixRef>(aIndex.meFallbackReason);
                if (!aIndex.moValue || *aIndex.moValue < 1)
                {
                    return makeMaterializedValue(makeSingleValueMatrix(api::CellValue::error(
                        aIndex.moValue ? api::Error::IllegalArgument : aIndex.meError)));
                }
                nSortIndex = *aIndex.moValue;
            }

            sal_Int32 nSortOrder = 1;
            if (rNode.maChildren.size() >= 3
                && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
            {
                const auto aOrder = normalizeWholeMaterializedArgument(
                    *rNode.maChildren[2], rDoc, rContext, rFormulaPos);
                if (!aOrder.mbSupported)
                    return makeUnsupportedMaterialization<ScMatrixRef>(aOrder.meFallbackReason);
                if (!aOrder.moValue || (*aOrder.moValue != 1 && *aOrder.moValue != -1))
                {
                    return makeMaterializedValue(makeSingleValueMatrix(api::CellValue::error(
                        aOrder.moValue ? api::Error::IllegalArgument : aOrder.meError)));
                }
                nSortOrder = *aOrder.moValue;
            }

            bool bByColumn = false;
            if (rNode.maChildren.size() == 4
                && rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument)
            {
                const auto aByColumn = materializeScalarNode(
                    *rNode.maChildren[3], rDoc, rContext, rFormulaPos);
                if (!aByColumn.mbSupported)
                    return makeUnsupportedMaterialization<ScMatrixRef>(aByColumn.meFallbackReason);
                if (!aByColumn.moValue)
                    return makeMaterializedError<ScMatrixRef>(aByColumn.meError);
                const auto aBool = coerceScalarToBool(rDoc, rContext, *aByColumn.moValue);
                if (!aBool)
                    return makeMaterializedValue(
                        makeSingleValueMatrix(api::CellValue::error(aBool.meError)));
                bByColumn = aBool.maValue;
            }

            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aSource.moValue)->GetDimensions(nColumns, nRows);
            const bool bSortRows = !bByColumn;
            const SCSIZE nAxisLength = bSortRows ? nRows : nColumns;
            const SCSIZE nKeyLimit = bSortRows ? nColumns : nRows;
            if (static_cast<SCSIZE>(nSortIndex) > nKeyLimit || nSortIndex < 1)
            {
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
            }

            std::vector<SCSIZE> aOrder(static_cast<std::size_t>(nAxisLength));
            std::iota(aOrder.begin(), aOrder.end(), 0);
            std::optional<api::Error> oSortError;
            std::stable_sort(aOrder.begin(), aOrder.end(), [&](SCSIZE nLeft, SCSIZE nRight) {
                const api::CellValue& rLeftValue = bSortRows
                                                       ? lookupexecution::detail::toApiCellValue(
                                                             (*aSource.moValue)->Get(
                                                                 static_cast<SCSIZE>(nSortIndex - 1), nLeft))
                                                       : lookupexecution::detail::toApiCellValue(
                                                             (*aSource.moValue)->Get(
                                                                 nLeft, static_cast<SCSIZE>(nSortIndex - 1)));
                const api::CellValue& rRightValue = bSortRows
                                                        ? lookupexecution::detail::toApiCellValue(
                                                              (*aSource.moValue)->Get(
                                                                  static_cast<SCSIZE>(nSortIndex - 1), nRight))
                                                        : lookupexecution::detail::toApiCellValue(
                                                              (*aSource.moValue)->Get(
                                                                  nRight, static_cast<SCSIZE>(nSortIndex - 1)));
                const auto aCompare = spillCompareValues(rLeftValue, rRightValue, rDoc, rContext);
                if (!aCompare)
                {
                    oSortError = aCompare.meError;
                    return nLeft < nRight;
                }
                if (aCompare.maValue == 0)
                    return nLeft < nRight;
                return nSortOrder > 0 ? aCompare.maValue < 0 : aCompare.maValue > 0;
            });
            if (oSortError)
            {
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(*oSortError)));
            }

            ScMatrixRef xMatrix(new ScMatrix(nColumns, nRows));
            if (bSortRows)
            {
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    const SCSIZE nSourceRow = aOrder[nRow];
                    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                    {
                        putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                                (*aSource.moValue)->Get(nColumn, nSourceRow)),
                            xMatrix, nColumn, nRow);
                    }
                }
            }
            else
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    const SCSIZE nSourceColumn = aOrder[nColumn];
                    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                    {
                        putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                                (*aSource.moValue)->Get(nSourceColumn, nRow)),
                            xMatrix, nColumn, nRow);
                    }
                }
            }

            return makeMaterializedValue(xMatrix);
        }

        if (rNode.maChildren.size() < 2)
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        const auto aSource = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aSource.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aSource.meFallbackReason);
        if (!aSource.moValue)
            return makeMaterializedError<ScMatrixRef>(aSource.meError);

        struct SortKeyMatrix
        {
            ScMatrixRef mxMatrix;
            sal_Int32 mnOrder = 1;
        };

        std::vector<SortKeyMatrix> aKeys;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size();)
        {
            const auto aKey = materializeMatrixNode(*rNode.maChildren[nIndex], rDoc, rContext, rFormulaPos);
            if (!aKey.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aKey.meFallbackReason);
            if (!aKey.moValue)
                return makeMaterializedError<ScMatrixRef>(aKey.meError);
            ++nIndex;

            sal_Int32 nSortOrder = 1;
            if (nIndex < rNode.maChildren.size()
                && rNode.maChildren[nIndex]->meKind != core::formula::NodeKind::EmptyArgument)
            {
                const auto aOrder = normalizeWholeMaterializedArgument(
                    *rNode.maChildren[nIndex], rDoc, rContext, rFormulaPos);
                if (!aOrder.mbSupported)
                    return makeUnsupportedMaterialization<ScMatrixRef>(aOrder.meFallbackReason);
                if (!aOrder.moValue || (*aOrder.moValue != 1 && *aOrder.moValue != -1))
                {
                    return makeMaterializedValue(makeSingleValueMatrix(api::CellValue::error(
                        aOrder.moValue ? api::Error::IllegalArgument : aOrder.meError)));
                }
                nSortOrder = *aOrder.moValue;
            }
            if (nIndex < rNode.maChildren.size())
                ++nIndex;

            aKeys.push_back({ *aKey.moValue, nSortOrder });
        }

        if (aKeys.empty())
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        SCSIZE nSourceColumns = 0;
        SCSIZE nSourceRows = 0;
        (*aSource.moValue)->GetDimensions(nSourceColumns, nSourceRows);
        SCSIZE nKeyColumns = 0;
        SCSIZE nKeyRows = 0;
        aKeys.front().mxMatrix->GetDimensions(nKeyColumns, nKeyRows);

        const bool bSortRows = nKeyColumns == 1 && nKeyRows > 1;
        const bool bSortColumns = nKeyRows == 1 && nKeyColumns > 1;
        if (!bSortRows && !bSortColumns)
        {
            if (nKeyColumns == 1 && nKeyRows == 1)
                return makeMaterializedValue(makeSingleValueMatrix(
                    lookupexecution::detail::toApiCellValue((*aSource.moValue)->Get(0, 0))));
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        const SCSIZE nAxisLength = bSortRows ? nKeyRows : nKeyColumns;
        for (const auto& rKey : aKeys)
        {
            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            rKey.mxMatrix->GetDimensions(nColumns, nRows);
            if (bSortRows)
            {
                if (nColumns != 1 || nRows != nAxisLength)
                {
                    return makeMaterializedValue(makeSingleValueMatrix(
                        api::CellValue::error(api::Error::IllegalArgument)));
                }
            }
            else if (nRows != 1 || nColumns != nAxisLength)
            {
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
            }
        }

        if ((bSortRows && nSourceRows != nAxisLength)
            || (bSortColumns && nSourceColumns != nAxisLength))
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        std::vector<SCSIZE> aOrder(static_cast<std::size_t>(nAxisLength));
        std::iota(aOrder.begin(), aOrder.end(), 0);
        std::optional<api::Error> oSortError;
        std::stable_sort(aOrder.begin(), aOrder.end(), [&](SCSIZE nLeft, SCSIZE nRight) {
            for (const auto& rKey : aKeys)
            {
                const api::CellValue rLeftValue = bSortRows
                                                      ? lookupexecution::detail::toApiCellValue(
                                                            rKey.mxMatrix->Get(0, nLeft))
                                                      : lookupexecution::detail::toApiCellValue(
                                                            rKey.mxMatrix->Get(nLeft, 0));
                const api::CellValue rRightValue = bSortRows
                                                       ? lookupexecution::detail::toApiCellValue(
                                                             rKey.mxMatrix->Get(0, nRight))
                                                       : lookupexecution::detail::toApiCellValue(
                                                             rKey.mxMatrix->Get(nRight, 0));
                const auto aCompare = spillCompareValues(rLeftValue, rRightValue, rDoc, rContext);
                if (!aCompare)
                {
                    oSortError = aCompare.meError;
                    return nLeft < nRight;
                }
                if (aCompare.maValue == 0)
                    continue;
                return rKey.mnOrder > 0 ? aCompare.maValue < 0 : aCompare.maValue > 0;
            }
            return nLeft < nRight;
        });
        if (oSortError)
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(*oSortError)));
        }

        ScMatrixRef xMatrix(new ScMatrix(nSourceColumns, nSourceRows));
        if (bSortRows)
        {
            for (SCSIZE nRow = 0; nRow < nSourceRows; ++nRow)
            {
                const SCSIZE nSourceRow = aOrder[nRow];
                for (SCSIZE nColumn = 0; nColumn < nSourceColumns; ++nColumn)
                {
                    putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                            (*aSource.moValue)->Get(nColumn, nSourceRow)),
                        xMatrix, nColumn, nRow);
                }
            }
        }
        else
        {
            for (SCSIZE nColumn = 0; nColumn < nSourceColumns; ++nColumn)
            {
                const SCSIZE nSourceColumn = aOrder[nColumn];
                for (SCSIZE nRow = 0; nRow < nSourceRows; ++nRow)
                {
                    putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                            (*aSource.moValue)->Get(nSourceColumn, nRow)),
                        xMatrix, nColumn, nRow);
                }
            }
        }

        return makeMaterializedValue(xMatrix);
    }

    if (*oCanonical == u"TEXTSPLIT")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 6)
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        const auto aText = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aText.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aText.meFallbackReason);
        if (!aText.moValue)
            return makeMaterializedError<ScMatrixRef>(aText.meError);
        const auto aTextString = coerceScalarToText(rDoc, rContext, *aText.moValue);
        if (!aTextString)
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(aTextString.meError)));
        }
        if (aTextString.maValue.isEmpty())
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        std::vector<api::String> aColumnDelimiters;
        if (rNode.maChildren.size() >= 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aDelimiters = collectTextVector(*rNode.maChildren[1]);
            if (!aDelimiters.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aDelimiters.meFallbackReason);
            if (!aDelimiters.moValue)
                return makeMaterializedError<ScMatrixRef>(aDelimiters.meError);
            aColumnDelimiters = *aDelimiters.moValue;
        }

        std::vector<api::String> aRowDelimiters;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aDelimiters = collectTextVector(*rNode.maChildren[2]);
            if (!aDelimiters.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aDelimiters.meFallbackReason);
            if (!aDelimiters.moValue)
                return makeMaterializedError<ScMatrixRef>(aDelimiters.meError);
            aRowDelimiters = *aDelimiters.moValue;
        }

        bool bIgnoreEmpty = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aIgnore = materializeScalarNode(
                *rNode.maChildren[3], rDoc, rContext, rFormulaPos);
            if (!aIgnore.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aIgnore.meFallbackReason);
            if (!aIgnore.moValue)
                return makeMaterializedError<ScMatrixRef>(aIgnore.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aIgnore.moValue);
            if (!aBool)
            {
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(aBool.meError)));
            }
            bIgnoreEmpty = aBool.maValue;
        }

        bool bMatchMode = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMatchModeValue = materializeScalarNode(
                *rNode.maChildren[4], rDoc, rContext, rFormulaPos);
            if (!aMatchModeValue.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aMatchModeValue.meFallbackReason);
            if (!aMatchModeValue.moValue)
                return makeMaterializedError<ScMatrixRef>(aMatchModeValue.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aMatchModeValue.moValue);
            if (!aBool)
            {
                return makeMaterializedValue(
                    makeSingleValueMatrix(api::CellValue::error(aBool.meError)));
            }
            bMatchMode = aBool.maValue;
        }

        std::optional<api::CellValue> oPadWith;
        if (rNode.maChildren.size() == 6
            && rNode.maChildren[5]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aPad = materializeScalarNode(*rNode.maChildren[5], rDoc, rContext, rFormulaPos);
            if (!aPad.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aPad.meFallbackReason);
            if (!aPad.moValue)
                return makeMaterializedError<ScMatrixRef>(aPad.meError);
            oPadWith = *aPad.moValue;
        }

        const auto aRows = spillSplitText(toApiString(aTextString.maValue), aRowDelimiters,
            bIgnoreEmpty, bMatchMode);
        if (aRows.empty())
        {
            if (oPadWith)
                return makeMaterializedValue(makeSingleValueMatrix(*oPadWith));
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::NotAvailable)));
        }

        std::vector<std::vector<api::String>> aColumnsByRow;
        aColumnsByRow.reserve(aRows.size());
        std::size_t nMaxColumns = 0;
        for (const auto& rRow : aRows)
        {
            auto aColumns = spillSplitText(rRow, aColumnDelimiters, bIgnoreEmpty, bMatchMode);
            nMaxColumns = std::max(nMaxColumns, aColumns.size());
            aColumnsByRow.push_back(std::move(aColumns));
        }

        if (nMaxColumns == 0)
        {
            if (oPadWith)
                return makeMaterializedValue(makeSingleValueMatrix(*oPadWith));
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::NotAvailable)));
        }

        ScMatrixRef xMatrix(new ScMatrix(static_cast<SCSIZE>(nMaxColumns),
            static_cast<SCSIZE>(aColumnsByRow.size())));
        for (std::size_t nRow = 0; nRow < aColumnsByRow.size(); ++nRow)
        {
            for (std::size_t nColumn = 0; nColumn < nMaxColumns; ++nColumn)
            {
                api::CellValue aValue;
                if (nColumn < aColumnsByRow[nRow].size())
                {
                    aValue = aColumnsByRow[nRow][nColumn].empty()
                                 ? api::CellValue::empty()
                                 : api::CellValue::text(aColumnsByRow[nRow][nColumn]);
                }
                else if (oPadWith)
                {
                    aValue = *oPadWith;
                }
                else
                {
                    aValue = api::CellValue::error(api::Error::NotAvailable);
                }

                putScalarIntoMatrix(
                    aValue, xMatrix, static_cast<SCSIZE>(nColumn), static_cast<SCSIZE>(nRow));
            }
        }

        return makeMaterializedValue(xMatrix);
    }

    return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeIsNumberMatrixFunctionCall(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.maChildren.size() != 1)
    {
        return makeMaterializedValue(
            makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
    }

    const auto aSource = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aSource.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aSource.meFallbackReason);
    if (!aSource.moValue)
        return makeMaterializedError<ScMatrixRef>(aSource.meError);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    (*aSource.moValue)->GetDimensions(nColumns, nRows);
    ScMatrixRef xMatrix(new ScMatrix(nColumns, nRows));
    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
        {
            const auto aValue
                = lookupexecution::detail::toApiCellValue((*aSource.moValue)->Get(nColumn, nRow));
            putScalarIntoMatrix(api::CellValue::boolean(aValue.isNumber()), xMatrix, nColumn, nRow);
        }
    }

    return makeMaterializedValue(xMatrix);
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeMatrixFunctionCall(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    const FunctionKind eFunction = classifyFunction(aFunctionName);
    if (eFunction == FunctionKind::XLookup)
        return materializeXLookupMatrixFunctionCall(rNode, rDoc, rContext, rFormulaPos);
    if (eFunction == FunctionKind::Index)
        return materializeIndexMatrixFunctionCall(rNode, rDoc, rContext, rFormulaPos);
    if (eFunction == FunctionKind::Selector)
        return materializeSelectorMatrixFunctionCall(rNode, rDoc, rContext, rFormulaPos);
    if (eFunction == FunctionKind::SpillArray)
        return materializeSpillMatrixFunctionCall(rNode, rDoc, rContext, rFormulaPos);
    if (aFunctionName == u"ISNUMBER")
        return materializeIsNumberMatrixFunctionCall(rNode, rDoc, rContext, rFormulaPos);
    if (aFunctionName == u"IF")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
        {
            return makeMaterializedValue(
                makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
        }

        const auto aCondition = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aCondition.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aCondition.meFallbackReason);
        if (!aCondition.moValue)
            return makeMaterializedError<ScMatrixRef>(aCondition.meError);

        const auto aThen = materializeMatrixNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
        if (!aThen.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aThen.meFallbackReason);
        if (!aThen.moValue)
            return makeMaterializedError<ScMatrixRef>(aThen.meError);

        ScMatrixRef xElseMatrix;
        if (rNode.maChildren.size() == 3)
        {
            const auto aElse = materializeMatrixNode(*rNode.maChildren[2], rDoc, rContext, rFormulaPos);
            if (!aElse.mbSupported)
                return makeUnsupportedMaterialization<ScMatrixRef>(aElse.meFallbackReason);
            if (!aElse.moValue)
                return makeMaterializedError<ScMatrixRef>(aElse.meError);
            xElseMatrix = *aElse.moValue;
        }
        else
        {
            xElseMatrix = makeSingleValueMatrix(api::CellValue::boolean(false));
        }

        SCSIZE nConditionColumns = 0;
        SCSIZE nConditionRows = 0;
        SCSIZE nThenColumns = 0;
        SCSIZE nThenRows = 0;
        SCSIZE nElseColumns = 0;
        SCSIZE nElseRows = 0;
        (*aCondition.moValue)->GetDimensions(nConditionColumns, nConditionRows);
        (*aThen.moValue)->GetDimensions(nThenColumns, nThenRows);
        xElseMatrix->GetDimensions(nElseColumns, nElseRows);

        const SCSIZE nResultColumns
            = std::max({ nConditionColumns, nThenColumns, nElseColumns });
        const SCSIZE nResultRows = std::max({ nConditionRows, nThenRows, nElseRows });
        ScMatrixRef xMatrix(new ScMatrix(nResultColumns, nResultRows));
        for (SCSIZE nRow = 0; nRow < nResultRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nResultColumns; ++nColumn)
            {
                SCSIZE nConditionColumn = nColumn;
                SCSIZE nConditionRow = nRow;
                if (!(*aCondition.moValue)
                         ->ValidColRowOrReplicated(nConditionColumn, nConditionRow))
                {
                    return makeUnsupportedMaterialization<ScMatrixRef>(
                        FallbackReason::UnsupportedFormulaShape);
                }

                const auto aConditionValue = lookupexecution::detail::toApiCellValue(
                    (*aCondition.moValue)->Get(nConditionColumn, nConditionRow));
                if (aConditionValue.isError())
                {
                    putScalarIntoMatrix(aConditionValue, xMatrix, nColumn, nRow);
                    continue;
                }

                const auto aConditionBool
                    = coerceScalarToBool(rDoc, rContext, aConditionValue);
                if (!aConditionBool)
                {
                    putScalarIntoMatrix(api::CellValue::error(aConditionBool.meError), xMatrix,
                        nColumn, nRow);
                    continue;
                }

                ScMatrixRef xSelectedMatrix
                    = aConditionBool.maValue ? *aThen.moValue : xElseMatrix;
                SCSIZE nSelectedColumn = nColumn;
                SCSIZE nSelectedRow = nRow;
                if (!xSelectedMatrix->ValidColRowOrReplicated(nSelectedColumn, nSelectedRow))
                {
                    return makeUnsupportedMaterialization<ScMatrixRef>(
                        FallbackReason::UnsupportedFormulaShape);
                }

                putScalarIntoMatrix(lookupexecution::detail::toApiCellValue(
                                        xSelectedMatrix->Get(nSelectedColumn, nSelectedRow)),
                    xMatrix, nColumn, nRow);
            }
        }

        return makeMaterializedValue(xMatrix);
    }

    if (aFunctionName != u"MMULT")
        return makeUnsupportedMaterialization<ScMatrixRef>(FallbackReason::UnsupportedFormulaShape);

    if (rNode.maChildren.size() != 2)
    {
        return makeMaterializedValue(
            makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
    }

    const auto aLeft = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aLeft.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aLeft.meFallbackReason);
    if (!aLeft.moValue)
        return makeMaterializedError<ScMatrixRef>(aLeft.meError);

    const auto aRight = materializeMatrixNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
    if (!aRight.mbSupported)
        return makeUnsupportedMaterialization<ScMatrixRef>(aRight.meFallbackReason);
    if (!aRight.moValue)
        return makeMaterializedError<ScMatrixRef>(aRight.meError);

    SCSIZE nLeftColumns = 0;
    SCSIZE nLeftRows = 0;
    SCSIZE nRightColumns = 0;
    SCSIZE nRightRows = 0;
    (*aLeft.moValue)->GetDimensions(nLeftColumns, nLeftRows);
    (*aRight.moValue)->GetDimensions(nRightColumns, nRightRows);
    if (nLeftColumns != nRightRows)
    {
        return makeMaterializedValue(
            makeSingleValueMatrix(api::CellValue::error(api::Error::IllegalArgument)));
    }

    ScMatrixRef xMatrix(new ScMatrix(nRightColumns, nLeftRows));
    for (SCSIZE nRow = 0; nRow < nLeftRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nRightColumns; ++nColumn)
        {
            bool bStoredCell = false;
            double fSum = 0.0;
            for (SCSIZE nIndex = 0; nIndex < nLeftColumns; ++nIndex)
            {
                const auto aLeftValue = lookupexecution::detail::toApiCellValue(
                    (*aLeft.moValue)->Get(nIndex, nRow));
                if (aLeftValue.isError())
                {
                    putScalarIntoMatrix(aLeftValue, xMatrix, nColumn, nRow);
                    bStoredCell = true;
                    break;
                }

                const auto aRightValue = lookupexecution::detail::toApiCellValue(
                    (*aRight.moValue)->Get(nColumn, nIndex));
                if (aRightValue.isError())
                {
                    putScalarIntoMatrix(aRightValue, xMatrix, nColumn, nRow);
                    bStoredCell = true;
                    break;
                }

                const auto aLeftNumber = coerceScalarToNumber(rDoc, rContext, aLeftValue);
                if (!aLeftNumber)
                {
                    putScalarIntoMatrix(api::CellValue::error(aLeftNumber.meError), xMatrix,
                        nColumn, nRow);
                    bStoredCell = true;
                    break;
                }

                const auto aRightNumber = coerceScalarToNumber(rDoc, rContext, aRightValue);
                if (!aRightNumber)
                {
                    putScalarIntoMatrix(api::CellValue::error(aRightNumber.meError), xMatrix,
                        nColumn, nRow);
                    bStoredCell = true;
                    break;
                }

                fSum += aLeftNumber.maValue * aRightNumber.maValue;
            }

            if (!bStoredCell)
                putScalarIntoMatrix(api::CellValue::number(fSum), xMatrix, nColumn, nRow);
        }
    }

    return makeMaterializedValue(xMatrix);
}

[[nodiscard]] inline Materialization<ScMatrixRef> materializeMatrixNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.meKind == core::formula::NodeKind::CellReference
        || rNode.meKind == core::formula::NodeKind::RangeReference
        || rNode.meKind == core::formula::NodeKind::NamedReference)
    {
        const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
        if (!aRange.mbSupported)
            return makeUnsupportedMaterialization<ScMatrixRef>(aRange.meFallbackReason);
        if (!aRange.moValue)
            return makeMaterializedError<ScMatrixRef>(aRange.meError);
        return materializeReferencedMatrix(*aRange.moValue, rDoc, rContext, rFormulaPos);
    }

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

    if (rNode.meKind == core::formula::NodeKind::BinaryOperation)
        return materializeMatrixBinaryOperation(rNode, rDoc, rContext, rFormulaPos);

    if (rNode.meKind == core::formula::NodeKind::FunctionCall)
        return materializeMatrixFunctionCall(rNode, rDoc, rContext, rFormulaPos);

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

[[nodiscard]] inline ScRange trimWholeMatchSearchRangeToUsedData(
    const ScDocument& rDoc, const ScRange& rRange)
{
    if (rRange.aStart.Tab() != rRange.aEnd.Tab())
        return rRange;

    const bool bWholeRow
        = rRange.aStart.Col() == 0 && rRange.aEnd.Col() == rDoc.MaxCol();
    const bool bWholeColumn
        = rRange.aStart.Row() == 0 && rRange.aEnd.Row() == rDoc.MaxRow();
    if (!bWholeRow && !bWholeColumn)
        return rRange;

    ScRange aTrimmed(rRange);
    if (rDoc.GetDataAreaSubrange(aTrimmed))
        return aTrimmed;

    return rRange;
}

[[nodiscard]] inline Materialization<lookupexecution::LookupInputSource>
materializeMatchLookupInputSourceNode(const core::formula::Node& rNode, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
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
        const ScRange aTrimmedRange = trimWholeMatchSearchRangeToUsedData(rDoc, *aRange.moValue);
        const auto aMatrix = materializeReferencedMatrix(aTrimmedRange, rDoc, rContext, rFormulaPos);
        if (!aMatrix.mbSupported)
        {
            return makeUnsupportedMaterialization<lookupexecution::LookupInputSource>(
                aMatrix.meFallbackReason);
        }
        if (!aMatrix.moValue)
            return makeMaterializedError<lookupexecution::LookupInputSource>(aMatrix.meError);
        aSource.mpMatrix = *aMatrix.moValue;
        return makeMaterializedValue(aSource);
    }

    return materializeLookupInputSourceNode(rNode, rDoc, rContext, rFormulaPos);
}

[[nodiscard]] inline Materialization<api::CellValue> materializeLookupValueNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.meKind == core::formula::NodeKind::CellReference
        || rNode.meKind == core::formula::NodeKind::RangeReference
        || rNode.meKind == core::formula::NodeKind::NamedReference)
    {
        const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
        if (!aRange.mbSupported)
            return makeUnsupportedMaterialization<api::CellValue>(aRange.meFallbackReason);
        if (!aRange.moValue)
            return makeMaterializedError<api::CellValue>(aRange.meError);

        std::optional<ScAddress> oScalarAddress
            = tryImplicitIntersectionAddress(*aRange.moValue, rFormulaPos);
        if ((!oScalarAddress || *oScalarAddress == rFormulaPos)
            && aRange.moValue->aStart != rFormulaPos)
        {
            oScalarAddress = aRange.moValue->aStart;
        }

        if (!oScalarAddress || *oScalarAddress == rFormulaPos)
        {
            return makeUnsupportedMaterialization<api::CellValue>(
                FallbackReason::UnsupportedHostSurface);
        }

        return makeMaterializedValue(
            readMaterializedHostCellValue(rDoc, rContext, *oScalarAddress));
    }

    const auto aScalar = materializeScalarNode(rNode, rDoc, rContext, rFormulaPos);
    if (aScalar.mbSupported || aScalar.meFallbackReason != FallbackReason::UnsupportedFormulaShape)
        return aScalar;
    if (!aScalar.moValue && aScalar.meError != api::Error::None)
        return makeMaterializedError<api::CellValue>(aScalar.meError);

    const auto aMatrix = materializeMatrixNode(rNode, rDoc, rContext, rFormulaPos);
    if (!aMatrix.mbSupported)
        return makeUnsupportedMaterialization<api::CellValue>(aMatrix.meFallbackReason);
    if (!aMatrix.moValue)
        return makeMaterializedError<api::CellValue>(aMatrix.meError);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
    if (nColumns < 1 || nRows < 1)
        return makeMaterializedError<api::CellValue>(api::Error::IllegalArgument);

    return makeMaterializedValue(
        lookupexecution::detail::toApiCellValue((*aMatrix.moValue)->Get(0, 0)));
}

[[nodiscard]] inline Materialization<api::CellValue> materializeTextParsingValueNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.meKind == core::formula::NodeKind::CellReference
        || rNode.meKind == core::formula::NodeKind::RangeReference
        || rNode.meKind == core::formula::NodeKind::NamedReference)
    {
        const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
        if (!aRange.mbSupported)
            return makeUnsupportedMaterialization<api::CellValue>(aRange.meFallbackReason);
        if (!aRange.moValue)
            return makeMaterializedError<api::CellValue>(aRange.meError);

        std::optional<ScAddress> oScalarAddress
            = tryImplicitIntersectionAddress(*aRange.moValue, rFormulaPos);
        if ((!oScalarAddress || *oScalarAddress == rFormulaPos)
            && aRange.moValue->aStart != rFormulaPos)
        {
            oScalarAddress = aRange.moValue->aStart;
        }

        if (!oScalarAddress || *oScalarAddress == rFormulaPos)
        {
            return makeUnsupportedMaterialization<api::CellValue>(
                FallbackReason::UnsupportedHostSurface);
        }

        return makeMaterializedValue(
            readTextParsingHostCellValue(rDoc, rContext, *oScalarAddress));
    }

    return materializeScalarNode(rNode, rDoc, rContext, rFormulaPos);
}

[[nodiscard]] inline Materialization<api::CellValue> materializeScalarizedReferenceValueNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.meKind == core::formula::NodeKind::CellReference
        || rNode.meKind == core::formula::NodeKind::RangeReference
        || rNode.meKind == core::formula::NodeKind::NamedReference)
    {
        const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
        if (!aRange.mbSupported)
            return makeUnsupportedMaterialization<api::CellValue>(aRange.meFallbackReason);
        if (!aRange.moValue)
            return makeMaterializedError<api::CellValue>(aRange.meError);

        std::optional<ScAddress> oScalarAddress
            = tryImplicitIntersectionAddress(*aRange.moValue, rFormulaPos);
        if ((!oScalarAddress || *oScalarAddress == rFormulaPos)
            && aRange.moValue->aStart != rFormulaPos)
        {
            oScalarAddress = aRange.moValue->aStart;
        }

        if (!oScalarAddress || *oScalarAddress == rFormulaPos)
        {
            return makeUnsupportedMaterialization<api::CellValue>(
                FallbackReason::UnsupportedHostSurface);
        }

        return makeMaterializedValue(
            readMaterializedHostCellValue(rDoc, rContext, *oScalarAddress));
    }

    return materializeScalarNode(rNode, rDoc, rContext, rFormulaPos);
}

[[nodiscard]] inline Materialization<api::CellValue> materializeMatchLookupValueNode(
    const core::formula::Node& rNode, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    if (rNode.meKind == core::formula::NodeKind::CellReference
        || rNode.meKind == core::formula::NodeKind::RangeReference
        || rNode.meKind == core::formula::NodeKind::NamedReference)
    {
        const auto aRange = resolveReferenceRangeNode(rNode, rDoc, rFormulaPos);
        if (!aRange.mbSupported)
            return makeUnsupportedMaterialization<api::CellValue>(aRange.meFallbackReason);
        if (!aRange.moValue)
            return makeMaterializedError<api::CellValue>(aRange.meError);

        const ScAddress aScalarAddress = aRange.moValue->aStart;
        if (aScalarAddress == rFormulaPos)
        {
            return makeUnsupportedMaterialization<api::CellValue>(
                FallbackReason::UnsupportedHostSurface);
        }

        return makeMaterializedValue(
            readMaterializedHostCellValue(rDoc, rContext, aScalarAddress));
    }

    return materializeLookupValueNode(rNode, rDoc, rContext, rFormulaPos);
}

[[nodiscard]] inline EvaluationAttempt materializeLookupResult(FunctionKind eFunction,
    const ScDocument& rDoc, ScInterpreterContext& rContext, const ScAddress& rFormulaPos,
    const lookupexecution::LookupExecutionResult& rResult)
{
    const auto makeLookupScalarAttempt = [&](const api::CellValue& rValue) {
        return makeScalarAttempt(eFunction, rValue);
    };

    if (rResult.meKind == lookupexecution::LookupExecutionResult::Kind::Scalar)
        return makeLookupScalarAttempt(rResult.maScalar);

    if (rResult.meKind == lookupexecution::LookupExecutionResult::Kind::Reference)
    {
        ScAddress aScalarAddress = rResult.maRange.aStart;
        if (!rResult.isSingleCellReference())
        {
            if (const auto oImplicit = tryImplicitIntersectionAddress(rResult.maRange, rFormulaPos);
                oImplicit && *oImplicit != rFormulaPos)
            {
                aScalarAddress = *oImplicit;
            }
        }
        if (aScalarAddress == rFormulaPos)
            return makeUnsupported(eFunction, FallbackReason::UnsupportedHostSurface);
        return makeLookupScalarAttempt(readMaterializedHostCellValue(rDoc, rContext, aScalarAddress));
    }

    if (rResult.meKind == lookupexecution::LookupExecutionResult::Kind::Matrix && rResult.mpMatrix)
    {
        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        rResult.mpMatrix->GetDimensions(nColumns, nRows);
        if (nColumns >= 1 && nRows >= 1)
            return makeLookupScalarAttempt(lookupexecution::detail::toApiCellValue(rResult.mpMatrix->Get(0, 0)));
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedHostSurface);
}

[[nodiscard]] inline FunctionKind classifyDelegatedFunctionNode(
    const core::formula::Node& rNode)
{
    if (rNode.meKind != core::formula::NodeKind::FunctionCall)
        return isPromotableScalarRootNode(rNode) ? FunctionKind::ScalarRoot
                                                 : FunctionKind::Unknown;

    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if (aFunctionName == u"IFERROR" || aFunctionName == u"IFNA")
    {
        if (rNode.maChildren.empty())
            return FunctionKind::Unknown;
        return classifyDelegatedFunctionNode(*rNode.maChildren[0]);
    }

    return classifyFunction(aFunctionName);
}

[[nodiscard]] inline bool isPromotableScalarRootFunctionCall(
    const core::formula::Node& rNode)
{
    if (rNode.meKind != core::formula::NodeKind::FunctionCall)
        return false;

    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if (aFunctionName == u"TODAY" && rNode.maChildren.empty())
        return true;
    const FunctionKind eFunction = classifyFunction(aFunctionName);
    if (eFunction == FunctionKind::LogicalConstant && rNode.maChildren.empty())
        return true;

    return eFunction != FunctionKind::Unknown && eFunction != FunctionKind::Count;
}

[[nodiscard]] inline bool isPromotableScalarRootNode(
    const core::formula::Node& rNode)
{
    switch (rNode.meKind)
    {
        case core::formula::NodeKind::NumberLiteral:
        case core::formula::NodeKind::StringLiteral:
        case core::formula::NodeKind::BooleanLiteral:
        case core::formula::NodeKind::ErrorLiteral:
        case core::formula::NodeKind::CellReference:
        case core::formula::NodeKind::NamedReference:
            return true;
        case core::formula::NodeKind::FunctionCall:
            return isPromotableScalarRootFunctionCall(rNode);
        case core::formula::NodeKind::UnaryOperation:
            return rNode.maChildren.size() == 1 && rNode.maChildren[0]
                   && isPromotableScalarRootNode(*rNode.maChildren[0]);
        case core::formula::NodeKind::BinaryOperation:
            return rNode.maChildren.size() == 2 && rNode.maChildren[0] && rNode.maChildren[1]
                   && isPromotableScalarRootNode(*rNode.maChildren[0])
                   && isPromotableScalarRootNode(*rNode.maChildren[1]);
        default:
            return false;
    }
}

[[nodiscard]] inline bool isLiteralOnlyNode(const core::formula::Node& rNode)
{
    switch (rNode.meKind)
    {
        case core::formula::NodeKind::NumberLiteral:
        case core::formula::NodeKind::StringLiteral:
        case core::formula::NodeKind::BooleanLiteral:
        case core::formula::NodeKind::EmptyArgument:
            return true;
        case core::formula::NodeKind::UnaryOperation:
            return rNode.maChildren.size() == 1 && isLiteralOnlyNode(*rNode.maChildren[0]);
        default:
            return false;
    }
}

[[nodiscard]] inline bool isLiteralDateConstructorNode(const core::formula::Node& rNode)
{
    if (rNode.meKind != core::formula::NodeKind::FunctionCall
        || uppercaseAscii(rNode.maPrimaryText) != u"DATE" || rNode.maChildren.size() != 3)
    {
        return false;
    }

    for (const auto& rxChild : rNode.maChildren)
    {
        if (!rxChild || !isLiteralOnlyNode(*rxChild))
            return false;
    }
    return true;
}

[[nodiscard]] inline bool isAmbientBusinessDayScalarNode(const core::formula::Node& rNode)
{
    if (isLiteralOnlyNode(rNode) || isLiteralDateConstructorNode(rNode))
        return true;

    switch (rNode.meKind)
    {
        case core::formula::NodeKind::CellReference:
        case core::formula::NodeKind::RangeReference:
        case core::formula::NodeKind::NamedReference:
            return true;
        case core::formula::NodeKind::UnaryOperation:
            return rNode.maChildren.size() == 1 && rNode.maChildren[0]
                   && isAmbientBusinessDayScalarNode(*rNode.maChildren[0]);
        case core::formula::NodeKind::BinaryOperation:
            return rNode.maChildren.size() == 2 && rNode.maChildren[0] && rNode.maChildren[1]
                   && isAmbientBusinessDayScalarNode(*rNode.maChildren[0])
                   && isAmbientBusinessDayScalarNode(*rNode.maChildren[1]);
        default:
            return false;
    }
}

[[nodiscard]] inline bool isAmbientBusinessDayHolidayNode(const core::formula::Node& rNode)
{
    return rNode.meKind == core::formula::NodeKind::EmptyArgument
           || isAmbientBusinessDayScalarNode(rNode)
           || rNode.meKind == core::formula::NodeKind::ArrayConstant;
}

[[nodiscard]] inline bool isAmbientBusinessDayWeekendNode(const core::formula::Node& rNode)
{
    if (rNode.meKind == core::formula::NodeKind::EmptyArgument
        || isAmbientBusinessDayScalarNode(rNode))
    {
        return true;
    }

    sal_Int32 nLength = 0;
    return isLiteralVectorArrayConstantNode(rNode, &nLength) && nLength == 7;
}

[[nodiscard]] inline bool isBoundedAmbientBusinessDayNode(const core::formula::Node& rNode)
{
    if (rNode.meKind != core::formula::NodeKind::FunctionCall)
        return false;

    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    const bool bWorkdayFunction = aFunctionName == u"WORKDAY"
                                  || aFunctionName == u"WORKDAY.INTL"
                                  || aFunctionName == u"COM.MICROSOFT.WORKDAY.INTL";
    const bool bIntl = aFunctionName == u"WORKDAY.INTL"
                       || aFunctionName == u"COM.MICROSOFT.WORKDAY.INTL"
                       || aFunctionName == u"NETWORKDAYS.INTL"
                       || aFunctionName == u"COM.MICROSOFT.NETWORKDAYS.INTL";
    if (!bWorkdayFunction && aFunctionName != u"NETWORKDAYS"
        && aFunctionName != u"NETWORKDAYS.INTL"
        && aFunctionName != u"COM.MICROSOFT.NETWORKDAYS.INTL")
    {
        return false;
    }

    if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
        return false;

    if (!rNode.maChildren[0] || !isAmbientBusinessDayScalarNode(*rNode.maChildren[0]))
        return false;
    if (!rNode.maChildren[1] || !isAmbientBusinessDayScalarNode(*rNode.maChildren[1]))
        return false;

    if (bWorkdayFunction)
    {
        if (bIntl)
        {
            return (rNode.maChildren.size() < 3 || !rNode.maChildren[2]
                        || isAmbientBusinessDayWeekendNode(*rNode.maChildren[2]))
                   && (rNode.maChildren.size() < 4 || !rNode.maChildren[3]
                           || isAmbientBusinessDayHolidayNode(*rNode.maChildren[3]));
        }

        return (rNode.maChildren.size() < 3 || !rNode.maChildren[2]
                    || isAmbientBusinessDayHolidayNode(*rNode.maChildren[2]))
               && (rNode.maChildren.size() < 4 || !rNode.maChildren[3]
                       || isAmbientBusinessDayWeekendNode(*rNode.maChildren[3]));
    }

    if (bIntl)
    {
        return (rNode.maChildren.size() < 3 || !rNode.maChildren[2]
                    || isAmbientBusinessDayWeekendNode(*rNode.maChildren[2]))
               && (rNode.maChildren.size() < 4 || !rNode.maChildren[3]
                       || isAmbientBusinessDayHolidayNode(*rNode.maChildren[3]));
    }

    return (rNode.maChildren.size() < 3 || !rNode.maChildren[2]
                || isAmbientBusinessDayHolidayNode(*rNode.maChildren[2]))
           && (rNode.maChildren.size() < 4 || !rNode.maChildren[3]
                   || isAmbientBusinessDayWeekendNode(*rNode.maChildren[3]));
}

[[nodiscard]] inline bool isHardRoutedNode(
    const core::formula::Node& rNode)
{
    // Keep this frontier intentionally frozen unless a new slice burns down a
    // live fallback reason or live mismatch class. Enumeration-only widening
    // is now out of scope.
    if (rNode.meKind != core::formula::NodeKind::FunctionCall)
        return false;

    const auto eFunction = classifyFunction(uppercaseAscii(rNode.maPrimaryText));
    if (eFunction == FunctionKind::LogicalConstant)
        return rNode.maChildren.empty();

    if (eFunction == FunctionKind::Value || eFunction == FunctionKind::DateValue
        || eFunction == FunctionKind::TimeValue)
    {
        return rNode.maChildren.size() == 1 && rNode.maChildren[0]
               && rNode.maChildren[0]->meKind == core::formula::NodeKind::StringLiteral;
    }

    if (eFunction != FunctionKind::NumberValue)
    {
        if (eFunction == FunctionKind::Match || eFunction == FunctionKind::XMatch)
            return isHardRoutedLiteralMatchNode(eFunction, rNode);

        if (eFunction == FunctionKind::Lookup || eFunction == FunctionKind::VLookup
            || eFunction == FunctionKind::HLookup || eFunction == FunctionKind::XLookup
            || eFunction == FunctionKind::Index)
        {
            return isHardRoutedLiteralLookupNode(eFunction, rNode);
        }

        return false;
    }

    if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
        return false;

    for (const auto& rxChild : rNode.maChildren)
    {
        if (!rxChild || !isLiteralOnlyNode(*rxChild))
            return false;
    }

    return true;
}

[[nodiscard]] inline bool isLiteralArrayConstantNode(
    const core::formula::Node& rNode)
{
    if (rNode.meKind != core::formula::NodeKind::ArrayConstant)
        return false;

    if (rNode.mnArrayColumns < 1 || rNode.mnArrayRows < 1
        || static_cast<sal_Int32>(rNode.maChildren.size())
               != rNode.mnArrayColumns * rNode.mnArrayRows)
    {
        return false;
    }

    for (const auto& rxChild : rNode.maChildren)
    {
        if (!rxChild || !isLiteralOnlyNode(*rxChild))
            return false;
    }

    return true;
}

[[nodiscard]] inline bool isLiteralVectorArrayConstantNode(
    const core::formula::Node& rNode)
{
    return isLiteralVectorArrayConstantNode(rNode, nullptr);
}

[[nodiscard]] inline bool isLiteralVectorArrayConstantNode(
    const core::formula::Node& rNode, sal_Int32* pLength)
{
    if (!isLiteralArrayConstantNode(rNode)
        || (rNode.mnArrayColumns != 1 && rNode.mnArrayRows != 1))
    {
        return false;
    }

    if (pLength)
        *pLength = std::max(rNode.mnArrayColumns, rNode.mnArrayRows);
    return true;
}

[[nodiscard]] inline bool isSortedNumericLiteralVectorArrayConstantNode(
    const core::formula::Node& rNode, bool bAscending)
{
    sal_Int32 nLength = 0;
    if (!isLiteralVectorArrayConstantNode(rNode, &nLength) || nLength < 1)
        return false;

    std::optional<double> ofPrevious;
    for (const auto& rxChild : rNode.maChildren)
    {
        if (!rxChild)
            return false;
        const auto oValue = extractNumericLiteral(*rxChild);
        if (!oValue)
            return false;
        if (ofPrevious)
        {
            if (bAscending ? (*oValue < *ofPrevious) : (*oValue > *ofPrevious))
                return false;
        }
        ofPrevious = oValue;
    }

    return true;
}

[[nodiscard]] inline bool isSortedNumericLiteralTableKeyNode(
    const core::formula::Node& rNode, api::lookup::VectorOrientation eOrientation)
{
    if (!isLiteralArrayConstantNode(rNode))
        return false;

    const sal_Int32 nKeyLength = eOrientation == api::lookup::VectorOrientation::Column
                                     ? rNode.mnArrayRows
                                     : rNode.mnArrayColumns;
    if (nKeyLength < 1)
        return false;

    std::optional<double> ofPrevious;
    for (sal_Int32 nIndex = 0; nIndex < nKeyLength; ++nIndex)
    {
        const sal_Int32 nOffset = eOrientation == api::lookup::VectorOrientation::Column
                                      ? nIndex * rNode.mnArrayColumns
                                      : nIndex;
        if (nOffset < 0 || o3tl::make_unsigned(nOffset) >= rNode.maChildren.size())
            return false;
        const auto& rxChild = rNode.maChildren[nOffset];
        if (!rxChild)
            return false;
        const auto oValue = extractNumericLiteral(*rxChild);
        if (!oValue)
            return false;
        if (ofPrevious && *oValue < *ofPrevious)
            return false;
        ofPrevious = oValue;
    }

    return true;
}

[[nodiscard]] inline bool isZeroOrFalseNode(const core::formula::Node& rNode)
{
    if (const auto oNumber = extractNumericLiteral(rNode))
        return *oNumber == 0.0;

    if (rNode.meKind == core::formula::NodeKind::BooleanLiteral)
        return !rNode.mbBoolean;

    if (rNode.meKind == core::formula::NodeKind::FunctionCall
        && classifyFunction(uppercaseAscii(rNode.maPrimaryText)) == FunctionKind::LogicalConstant
        && rNode.maChildren.empty())
    {
        return uppercaseAscii(rNode.maPrimaryText) == u"FALSE";
    }

    return false;
}

[[nodiscard]] inline bool isOneNode(const core::formula::Node& rNode)
{
    if (const auto oNumber = extractNumericLiteral(rNode))
        return *oNumber == 1.0;
    return false;
}

[[nodiscard]] inline bool isMinusOneNode(const core::formula::Node& rNode)
{
    if (const auto oNumber = extractNumericLiteral(rNode))
        return *oNumber == -1.0;
    return false;
}

[[nodiscard]] inline bool isTwoNode(const core::formula::Node& rNode)
{
    if (const auto oNumber = extractNumericLiteral(rNode))
        return *oNumber == 2.0;
    return false;
}

[[nodiscard]] inline bool isMinusTwoNode(const core::formula::Node& rNode)
{
    if (const auto oNumber = extractNumericLiteral(rNode))
        return *oNumber == -2.0;
    return false;
}

[[nodiscard]] inline bool isPositiveWholeLiteralNode(
    const core::formula::Node& rNode, sal_Int32* pValue)
{
    const auto oNumber = extractNumericLiteral(rNode);
    if (!oNumber || *oNumber < 1.0 || std::floor(*oNumber) != *oNumber)
        return false;

    if (pValue)
        *pValue = static_cast<sal_Int32>(*oNumber);
    return true;
}

[[nodiscard]] inline bool isOneOrTrueNode(const core::formula::Node& rNode)
{
    if (const auto oNumber = extractNumericLiteral(rNode))
        return *oNumber == 1.0;

    if (rNode.meKind == core::formula::NodeKind::BooleanLiteral)
        return rNode.mbBoolean;

    if (rNode.meKind == core::formula::NodeKind::FunctionCall
        && classifyFunction(uppercaseAscii(rNode.maPrimaryText)) == FunctionKind::LogicalConstant
        && rNode.maChildren.empty())
    {
        return uppercaseAscii(rNode.maPrimaryText) == u"TRUE";
    }

    return false;
}

[[nodiscard]] inline bool isHardRoutedLiteralMatchNode(
    FunctionKind eFunction, const core::formula::Node& rNode)
{
    if (eFunction == FunctionKind::Match)
    {
        if (rNode.maChildren.size() == 2 && rNode.maChildren[0] && rNode.maChildren[1])
        {
            return extractNumericLiteral(*rNode.maChildren[0]).has_value()
                   && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true);
        }

        if (rNode.maChildren.size() != 3 || !rNode.maChildren[0] || !rNode.maChildren[1]
            || !rNode.maChildren[2])
        {
            return false;
        }

        const auto oMode = extractNumericLiteral(*rNode.maChildren[2]);
        if (!oMode)
            return false;

        if (*oMode == 0.0)
        {
            return isLiteralOnlyNode(*rNode.maChildren[0])
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[1]);
        }

        const auto oLookup = extractNumericLiteral(*rNode.maChildren[0]);
        if (!oLookup)
            return false;

        return (*oMode == 1.0
                && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true))
               || (*oMode == -1.0
                   && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], false));
    }

    if (eFunction == FunctionKind::XMatch)
    {
        if (rNode.maChildren.size() == 2 && rNode.maChildren[0] && rNode.maChildren[1])
        {
            return isLiteralOnlyNode(*rNode.maChildren[0])
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[1]);
        }

        if (rNode.maChildren.size() == 3 && rNode.maChildren[0] && rNode.maChildren[1]
            && rNode.maChildren[2])
        {
            const auto oMode = extractNumericLiteral(*rNode.maChildren[2]);
            if (oMode && (*oMode == 1.0 || *oMode == -1.0))
            {
                return extractNumericLiteral(*rNode.maChildren[0]).has_value()
                       && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true);
            }

            return isLiteralOnlyNode(*rNode.maChildren[0])
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[1])
                   && isZeroOrFalseNode(*rNode.maChildren[2]);
        }

        if (rNode.maChildren.size() == 4 && rNode.maChildren[0] && rNode.maChildren[1]
            && rNode.maChildren[2] && rNode.maChildren[3])
        {
            if (!isLiteralOnlyNode(*rNode.maChildren[0])
                || !isLiteralVectorArrayConstantNode(*rNode.maChildren[1]))
            {
                return false;
            }

            if (rNode.maChildren[2]->meKind == core::formula::NodeKind::EmptyArgument
                || isZeroOrFalseNode(*rNode.maChildren[2]))
            {
                return isOneNode(*rNode.maChildren[3]) || isMinusOneNode(*rNode.maChildren[3])
                       || (isTwoNode(*rNode.maChildren[3])
                           && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true))
                       || (isMinusTwoNode(*rNode.maChildren[3])
                           && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], false));
            }

            const auto oMode = extractNumericLiteral(*rNode.maChildren[2]);
            if (!oMode || (*oMode != 1.0 && *oMode != -1.0)
                || !extractNumericLiteral(*rNode.maChildren[0]).has_value())
            {
                return false;
            }

            return ((isOneNode(*rNode.maChildren[3]) || isMinusOneNode(*rNode.maChildren[3]))
                    && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true))
                   || (isTwoNode(*rNode.maChildren[3])
                       && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true))
                   || (isMinusTwoNode(*rNode.maChildren[3])
                       && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], false));
        }
    }

    return false;
}

[[nodiscard]] inline bool isHardRoutedLiteralLookupNode(
    FunctionKind eFunction, const core::formula::Node& rNode)
{
    if (eFunction == FunctionKind::Lookup)
    {
        if (rNode.maChildren.size() == 2 && rNode.maChildren[0] && rNode.maChildren[1])
        {
            return isLiteralOnlyNode(*rNode.maChildren[0])
                   && isLiteralArrayConstantNode(*rNode.maChildren[1])
                   && (isLiteralVectorArrayConstantNode(*rNode.maChildren[1])
                       || (rNode.maChildren[1]->mnArrayColumns > 1
                           && rNode.maChildren[1]->mnArrayRows > 1));
        }

        if (rNode.maChildren.size() == 3 && rNode.maChildren[0] && rNode.maChildren[1]
            && rNode.maChildren[2])
        {
            sal_Int32 nSearchLength = 0;
            sal_Int32 nResultLength = 0;
            return isLiteralOnlyNode(*rNode.maChildren[0])
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[1], &nSearchLength)
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[2], &nResultLength)
                   && nSearchLength == nResultLength;
        }
        return false;
    }

    if (eFunction == FunctionKind::VLookup || eFunction == FunctionKind::HLookup)
    {
        if (rNode.maChildren.size() == 3 && rNode.maChildren[0] && rNode.maChildren[1]
            && rNode.maChildren[2] && isLiteralOnlyNode(*rNode.maChildren[0])
            && isLiteralArrayConstantNode(*rNode.maChildren[1])
            && isPositiveWholeLiteralNode(*rNode.maChildren[2]))
        {
            const auto eOrientation = eFunction == FunctionKind::HLookup
                                          ? api::lookup::VectorOrientation::Row
                                          : api::lookup::VectorOrientation::Column;
            return isSortedNumericLiteralTableKeyNode(*rNode.maChildren[1], eOrientation);
        }

        if (rNode.maChildren.size() != 4 || !rNode.maChildren[0] || !rNode.maChildren[1]
            || !rNode.maChildren[2] || !rNode.maChildren[3]
            || !isLiteralOnlyNode(*rNode.maChildren[0])
            || !isLiteralArrayConstantNode(*rNode.maChildren[1])
            || !isPositiveWholeLiteralNode(*rNode.maChildren[2]))
        {
            return false;
        }

        if (isZeroOrFalseNode(*rNode.maChildren[3]))
            return rNode.maChildren[1]->mnArrayColumns >= 1;

        if (isOneOrTrueNode(*rNode.maChildren[3]))
        {
            const auto eOrientation = eFunction == FunctionKind::HLookup
                                          ? api::lookup::VectorOrientation::Row
                                          : api::lookup::VectorOrientation::Column;
            return isSortedNumericLiteralTableKeyNode(*rNode.maChildren[1], eOrientation);
        }

        return false;
    }

    if (eFunction == FunctionKind::XLookup)
    {
        if (rNode.maChildren.size() == 3 && rNode.maChildren[0] && rNode.maChildren[1]
            && rNode.maChildren[2])
        {
            sal_Int32 nSearchLength = 0;
            sal_Int32 nResultLength = 0;
            return isLiteralOnlyNode(*rNode.maChildren[0])
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[1], &nSearchLength)
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[2], &nResultLength)
                   && nSearchLength == nResultLength;
        }

        if (rNode.maChildren.size() == 4 && rNode.maChildren[0] && rNode.maChildren[1]
            && rNode.maChildren[2] && rNode.maChildren[3])
        {
            sal_Int32 nSearchLength = 0;
            sal_Int32 nResultLength = 0;
            return isLiteralOnlyNode(*rNode.maChildren[0])
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[1], &nSearchLength)
                   && isLiteralVectorArrayConstantNode(*rNode.maChildren[2], &nResultLength)
                   && nSearchLength == nResultLength
                   && isLiteralOnlyNode(*rNode.maChildren[3]);
        }

        if (rNode.maChildren.size() == 5 && rNode.maChildren[0] && rNode.maChildren[1]
            && rNode.maChildren[2] && rNode.maChildren[3] && rNode.maChildren[4])
        {
            sal_Int32 nSearchLength = 0;
            sal_Int32 nResultLength = 0;
            if (!isLiteralOnlyNode(*rNode.maChildren[0])
                || !isLiteralVectorArrayConstantNode(*rNode.maChildren[1], &nSearchLength)
                || !isLiteralVectorArrayConstantNode(*rNode.maChildren[2], &nResultLength)
                || nSearchLength != nResultLength)
            {
                return false;
            }

            const bool bEmptyIfNotFound
                = rNode.maChildren[3]->meKind == core::formula::NodeKind::EmptyArgument;
            if (!bEmptyIfNotFound && !isLiteralOnlyNode(*rNode.maChildren[3]))
                return false;

            if (isZeroOrFalseNode(*rNode.maChildren[4]))
                return true;

            const auto oMode = extractNumericLiteral(*rNode.maChildren[4]);
            return oMode && (*oMode == 1.0 || *oMode == -1.0)
                   && extractNumericLiteral(*rNode.maChildren[0]).has_value()
                   && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true);
        }

        if (rNode.maChildren.size() == 6 && rNode.maChildren[0] && rNode.maChildren[1]
            && rNode.maChildren[2] && rNode.maChildren[3] && rNode.maChildren[4]
            && rNode.maChildren[5])
        {
            sal_Int32 nSearchLength = 0;
            sal_Int32 nResultLength = 0;
            if (!isLiteralOnlyNode(*rNode.maChildren[0])
                || !isLiteralVectorArrayConstantNode(*rNode.maChildren[1], &nSearchLength)
                || !isLiteralVectorArrayConstantNode(*rNode.maChildren[2], &nResultLength)
                || nSearchLength != nResultLength)
            {
                return false;
            }

            if (rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument
                && !isLiteralOnlyNode(*rNode.maChildren[3]))
            {
                return false;
            }

            const bool bExactMode = rNode.maChildren[4]->meKind == core::formula::NodeKind::EmptyArgument
                                    || isZeroOrFalseNode(*rNode.maChildren[4]);
            const auto oApproxMode = extractNumericLiteral(*rNode.maChildren[4]);
            if (!bExactMode && (!oApproxMode || (*oApproxMode != 1.0 && *oApproxMode != -1.0)))
            {
                return false;
            }

            if (bExactMode)
            {
                return isOneNode(*rNode.maChildren[5]) || isMinusOneNode(*rNode.maChildren[5])
                       || (isTwoNode(*rNode.maChildren[5])
                           && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true))
                       || (isMinusTwoNode(*rNode.maChildren[5])
                           && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], false));
            }

            if (!extractNumericLiteral(*rNode.maChildren[0]).has_value())
                return false;

            return ((isOneNode(*rNode.maChildren[5]) || isMinusOneNode(*rNode.maChildren[5]))
                    && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true))
                   || (isTwoNode(*rNode.maChildren[5])
                       && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], true))
                   || (isMinusTwoNode(*rNode.maChildren[5])
                       && isSortedNumericLiteralVectorArrayConstantNode(*rNode.maChildren[1], false));
        }

        return false;
    }

    if (eFunction == FunctionKind::Index)
    {
        if (rNode.maChildren.size() == 2 && rNode.maChildren[0] && rNode.maChildren[1])
        {
            return isLiteralArrayConstantNode(*rNode.maChildren[0])
                   && isPositiveWholeLiteralNode(*rNode.maChildren[1]);
        }

        if (rNode.maChildren.size() != 3 || !rNode.maChildren[0] || !rNode.maChildren[1]
            || !rNode.maChildren[2])
        {
            return false;
        }

        const bool bArray = isLiteralArrayConstantNode(*rNode.maChildren[0]);
        const bool bPositiveRow = isPositiveWholeLiteralNode(*rNode.maChildren[1]);
        const bool bPositiveColumn = isPositiveWholeLiteralNode(*rNode.maChildren[2]);
        const bool bZeroRow = isZeroOrFalseNode(*rNode.maChildren[1]);
        const bool bZeroColumn = isZeroOrFalseNode(*rNode.maChildren[2]);
        return bArray
               && ((bPositiveRow && bPositiveColumn) || (bZeroRow && bPositiveColumn)
                   || (bPositiveRow && bZeroColumn));
    }

    return false;
}

[[nodiscard]] inline EvaluationAttempt evaluateTextParsingFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    bool bImportedCanonicalSource)
{
    auto materializeScalarArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        return materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
    };

    auto materializeTextArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        return materializeTextParsingValueNode(rArgument, rDoc, rContext, rFormulaPos);
    };

    if (eFunction == FunctionKind::Value || eFunction == FunctionKind::DateValue
        || eFunction == FunctionKind::TimeValue)
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aArgument = materializeTextArgument(*rNode.maChildren[0]);
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
            {
                if (const auto oStandalone = tryStandaloneParsedScalarValue(aText.maValue))
                    return makeNumericResult(eFunction, *oStandalone, SvNumFormatType::NUMBER);
                return makeErrorResult(eFunction, aResult.meError);
            }
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::NUMBER);
        }

        if (eFunction == FunctionKind::DateValue)
        {
            const bool bImportedMonthNameLiteral
                = bImportedCanonicalSource
                  && rNode.maChildren[0]->meKind == core::formula::NodeKind::StringLiteral
                  && [&]() {
                         const std::u16string_view aTextView(
                             aText.maValue.getStr(), aText.maValue.getLength());
                         return std::any_of(
                             aTextView.begin(), aTextView.end(), [](sal_Unicode cChar) {
                                 return (cChar >= u'A' && cChar <= u'Z')
                                        || (cChar >= u'a' && cChar <= u'z');
                             });
                     }();
            if (bImportedMonthNameLiteral)
                return makeErrorResult(eFunction, api::Error::VariableExpected);

            const auto aResult
                = textparsingexecution::evaluateDateValue(rDoc, rContext, aText.maValue);
            if (!aResult)
            {
                if (const auto oStandalone = tryStandaloneParsedDateValue(aText.maValue))
                    return makeNumericResult(eFunction, *oStandalone, SvNumFormatType::DATE);
                if (const auto oIsoFallback = tryIsoDateValueFallback(rDoc, aText.maValue))
                    return makeNumericResult(eFunction, *oIsoFallback, SvNumFormatType::DATE);
                return makeErrorResult(eFunction, aResult.meError);
            }
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::DATE);
        }

        const auto aResult = textparsingexecution::evaluateTimeValue(rDoc, rContext, aText.maValue);
        if (!aResult)
        {
            if (const auto oStandalone = tryStandaloneParsedTimeValue(aText.maValue))
                return makeNumericResult(eFunction, *oStandalone, SvNumFormatType::TIME);
            return makeErrorResult(eFunction, aResult.meError);
        }
        return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::TIME);
    }

    if (eFunction == FunctionKind::NumberValue)
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aTextArg = materializeTextArgument(*rNode.maChildren[0]);
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
            const auto aDecimalArg = materializeScalarArgument(*rNode.maChildren[1]);
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
            const auto aGroupArg = materializeScalarArgument(*rNode.maChildren[2]);
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

[[nodiscard]] inline EvaluationAttempt evaluateFinancialScalarFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);

    auto materializeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        return materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
    };

    auto materializeNumericArgument = [&](const core::formula::Node& rArgument,
                                          std::optional<double> oEmptyDefault)
        -> Materialization<double> {
        const auto aArgument = materializeArgument(rArgument);
        if (!aArgument.mbSupported)
            return makeUnsupportedMaterialization<double>(aArgument.meFallbackReason);
        if (!aArgument.moValue)
            return makeMaterializedError<double>(aArgument.meError);
        if (aArgument.moValue->isEmpty())
        {
            if (oEmptyDefault)
                return makeMaterializedValue(*oEmptyDefault);
            return makeMaterializedError<double>(api::Error::IllegalArgument);
        }

        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aArgument.moValue);
        if (!aNumber)
            return makeMaterializedError<double>(aNumber.meError);
        return makeMaterializedValue(aNumber.maValue);
    };

    const auto collectNumericSeriesValues = [&]() -> Materialization<std::vector<double>> {
        if (rNode.maChildren.size() < 2)
            return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

        std::vector<double> aValues;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto& rxChild = rNode.maChildren[nIndex];
            if (!rxChild)
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const bool bMatrixLike = rxChild->meKind == core::formula::NodeKind::CellReference
                                     || rxChild->meKind == core::formula::NodeKind::RangeReference
                                     || rxChild->meKind == core::formula::NodeKind::NamedReference
                                     || rxChild->meKind == core::formula::NodeKind::ArrayConstant
                                     || rxChild->meKind == core::formula::NodeKind::BinaryOperation
                                     || rxChild->meKind == core::formula::NodeKind::FunctionCall;
            if (bMatrixLike)
            {
                const auto aMatrix = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
                if (!aMatrix.mbSupported)
                    return makeUnsupportedMaterialization<std::vector<double>>(
                        aMatrix.meFallbackReason);
                if (!aMatrix.moValue)
                    return makeMaterializedError<std::vector<double>>(aMatrix.meError);

                SCSIZE nColumns = 0;
                SCSIZE nRows = 0;
                (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                    {
                        const auto aValue = lookupexecution::detail::toApiCellValue(
                            (*aMatrix.moValue)->Get(nColumn, nRow));
                        if (aValue.isEmpty())
                            continue;
                        if (aValue.isText())
                            return makeMaterializedError<std::vector<double>>(
                                api::Error::IllegalArgument);

                        const auto aNumber = coerceScalarToNumber(rDoc, rContext, aValue);
                        if (!aNumber)
                            return makeMaterializedError<std::vector<double>>(aNumber.meError);
                        aValues.push_back(aNumber.maValue);
                    }
                }
                continue;
            }

            const auto aScalar = materializeScalarNode(*rxChild, rDoc, rContext, rFormulaPos);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<std::vector<double>>(
                    aScalar.meFallbackReason);
            if (!aScalar.moValue)
                return makeMaterializedError<std::vector<double>>(aScalar.meError);
            if (aScalar.moValue->isEmpty())
                continue;
            if (aScalar.moValue->isText())
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
            if (!aNumber)
                return makeMaterializedError<std::vector<double>>(aNumber.meError);
            aValues.push_back(aNumber.maValue);
        }

        return makeMaterializedValue(std::move(aValues));
    };

    if (eFunction == FunctionKind::Rate)
    {
        if (aFunctionName == u"FV" || aFunctionName == u"PV" || aFunctionName == u"PMT")
        {
            if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aRate = materializeNumericArgument(*rNode.maChildren[0], 0.0);
            if (!aRate.mbSupported)
                return makeUnsupported(eFunction, aRate.meFallbackReason);
            if (!aRate.moValue)
                return makeErrorResult(eFunction, aRate.meError);
            const auto aNper = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            if (!aNper.mbSupported)
                return makeUnsupported(eFunction, aNper.meFallbackReason);
            if (!aNper.moValue)
                return makeErrorResult(eFunction, aNper.meError);
            const auto aPayment = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            if (!aPayment.mbSupported)
                return makeUnsupported(eFunction, aPayment.meFallbackReason);
            if (!aPayment.moValue)
                return makeErrorResult(eFunction, aPayment.meError);

            double fEndpoint = 0.0;
            if (rNode.maChildren.size() >= 4)
            {
                const auto aEndpoint = materializeNumericArgument(*rNode.maChildren[3], 0.0);
                if (!aEndpoint.mbSupported)
                    return makeUnsupported(eFunction, aEndpoint.meFallbackReason);
                if (!aEndpoint.moValue)
                    return makeErrorResult(eFunction, aEndpoint.meError);
                fEndpoint = *aEndpoint.moValue;
            }

            bool bPayInAdvance = false;
            if (rNode.maChildren.size() == 5)
            {
                const auto aPayType = materializeArgument(*rNode.maChildren[4]);
                if (!aPayType.mbSupported)
                    return makeUnsupported(eFunction, aPayType.meFallbackReason);
                if (!aPayType.moValue)
                    return makeErrorResult(eFunction, aPayType.meError);
                if (!aPayType.moValue->isEmpty())
                {
                    const auto aBool = coerceScalarToBool(rDoc, rContext, *aPayType.moValue);
                    if (!aBool)
                        return makeErrorResult(eFunction, aBool.meError);
                    bPayInAdvance = aBool.maValue;
                }
            }

            if (aFunctionName == u"FV")
            {
                const auto aResult = spreadsheetengine::core::finance::evaluateFutureValue(
                    *aRate.moValue, *aNper.moValue, *aPayment.moValue, fEndpoint, bPayInAdvance);
                if (!aResult)
                    return makeErrorResult(eFunction, aResult.meError);
                return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
            }
            if (aFunctionName == u"PV")
            {
                const auto aResult = spreadsheetengine::core::finance::evaluatePresentValue(
                    *aRate.moValue, *aNper.moValue, *aPayment.moValue, fEndpoint, bPayInAdvance);
                if (!aResult)
                    return makeErrorResult(eFunction, aResult.meError);
                return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
            }
            const auto aResult = spreadsheetengine::core::finance::evaluatePayment(
                *aRate.moValue, *aNper.moValue, *aPayment.moValue, fEndpoint, bPayInAdvance);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName == u"NPER")
        {
            if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aRate = materializeNumericArgument(*rNode.maChildren[0], 0.0);
            if (!aRate.mbSupported)
                return makeUnsupported(eFunction, aRate.meFallbackReason);
            if (!aRate.moValue)
                return makeErrorResult(eFunction, aRate.meError);
            const auto aPayment = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            if (!aPayment.mbSupported)
                return makeUnsupported(eFunction, aPayment.meFallbackReason);
            if (!aPayment.moValue)
                return makeErrorResult(eFunction, aPayment.meError);
            const auto aPresentValue = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            if (!aPresentValue.mbSupported)
                return makeUnsupported(eFunction, aPresentValue.meFallbackReason);
            if (!aPresentValue.moValue)
                return makeErrorResult(eFunction, aPresentValue.meError);

            double fFutureValue = 0.0;
            if (rNode.maChildren.size() >= 4)
            {
                const auto aFutureValue = materializeNumericArgument(*rNode.maChildren[3], 0.0);
                if (!aFutureValue.mbSupported)
                    return makeUnsupported(eFunction, aFutureValue.meFallbackReason);
                if (!aFutureValue.moValue)
                    return makeErrorResult(eFunction, aFutureValue.meError);
                fFutureValue = *aFutureValue.moValue;
            }

            bool bPayInAdvance = false;
            if (rNode.maChildren.size() == 5)
            {
                const auto aPayType = materializeArgument(*rNode.maChildren[4]);
                if (!aPayType.mbSupported)
                    return makeUnsupported(eFunction, aPayType.meFallbackReason);
                if (!aPayType.moValue)
                    return makeErrorResult(eFunction, aPayType.meError);
                if (!aPayType.moValue->isEmpty())
                {
                    const auto aBool = coerceScalarToBool(rDoc, rContext, *aPayType.moValue);
                    if (!aBool)
                        return makeErrorResult(eFunction, aBool.meError);
                    bPayInAdvance = aBool.maValue;
                }
            }

            const auto aResult = spreadsheetengine::core::finance::evaluatePeriodsForFutureValue(
                *aRate.moValue, *aPayment.moValue, *aPresentValue.moValue, fFutureValue,
                bPayInAdvance);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::NUMBER);
        }

        if (aFunctionName == u"NOMINAL")
        {
            if (rNode.maChildren.size() != 2)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aRate = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
            if (!aRate.mbSupported)
                return makeUnsupported(eFunction, aRate.meFallbackReason);
            if (!aRate.moValue)
                return makeErrorResult(eFunction, aRate.meError);
            const auto aPeriods = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aPeriods.mbSupported)
                return makeUnsupported(eFunction, aPeriods.meFallbackReason);
            if (!aPeriods.moValue)
                return makeErrorResult(eFunction, aPeriods.meError);

            const auto aResult = spreadsheetengine::core::finance::evaluateNominal(
                *aRate.moValue, *aPeriods.moValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::PERCENT);
        }

        if (aFunctionName == u"EFFECT")
        {
            if (rNode.maChildren.size() != 2)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aNominal = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
            if (!aNominal.mbSupported)
                return makeUnsupported(eFunction, aNominal.meFallbackReason);
            if (!aNominal.moValue)
                return makeErrorResult(eFunction, aNominal.meError);
            const auto aPeriods = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aPeriods.mbSupported)
                return makeUnsupported(eFunction, aPeriods.meFallbackReason);
            if (!aPeriods.moValue)
                return makeErrorResult(eFunction, aPeriods.meError);

            const auto aResult = spreadsheetengine::core::finance::evaluateEffectiveAnnualRate(
                *aNominal.moValue, *aPeriods.moValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::PERCENT);
        }

        if (aFunctionName == u"NPV")
        {
            const auto aRate = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
            if (!aRate.mbSupported)
                return makeUnsupported(eFunction, aRate.meFallbackReason);
            if (!aRate.moValue)
                return makeErrorResult(eFunction, aRate.meError);
            const auto aValues = collectNumericSeriesValues();
            if (!aValues.mbSupported)
                return makeUnsupported(eFunction, aValues.meFallbackReason);
            if (!aValues.moValue)
                return makeErrorResult(eFunction, aValues.meError);

            const auto aResult = spreadsheetengine::core::finance::evaluateNetPresentValueNumbers(
                *aRate.moValue, *aValues.moValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName == u"RRI")
        {
            if (rNode.maChildren.size() != 3)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aPeriods = materializeNumericArgument(*rNode.maChildren[0], 0.0);
            const auto aPresentValue = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            const auto aFutureValue = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            if (!aPeriods.mbSupported)
                return makeUnsupported(eFunction, aPeriods.meFallbackReason);
            if (!aPresentValue.mbSupported)
                return makeUnsupported(eFunction, aPresentValue.meFallbackReason);
            if (!aFutureValue.mbSupported)
                return makeUnsupported(eFunction, aFutureValue.meFallbackReason);
            if (!aPeriods.moValue || !aPresentValue.moValue || !aFutureValue.moValue)
                return makeErrorResult(
                    eFunction, !aPeriods.moValue    ? aPeriods.meError
                               : !aPresentValue.moValue ? aPresentValue.meError
                                                        : aFutureValue.meError);

            const auto aResult = spreadsheetengine::core::finance::evaluateGrowthRateOverPeriods(
                *aPeriods.moValue, *aPresentValue.moValue, *aFutureValue.moValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::PERCENT);
        }

        if (aFunctionName == u"ISPMT")
        {
            if (rNode.maChildren.size() != 4)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aRate = materializeNumericArgument(*rNode.maChildren[0], 0.0);
            const auto aPeriod = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            const auto aTotal = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            const auto aInvest = materializeNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aRate.mbSupported)
                return makeUnsupported(eFunction, aRate.meFallbackReason);
            if (!aPeriod.mbSupported)
                return makeUnsupported(eFunction, aPeriod.meFallbackReason);
            if (!aTotal.mbSupported)
                return makeUnsupported(eFunction, aTotal.meFallbackReason);
            if (!aInvest.mbSupported)
                return makeUnsupported(eFunction, aInvest.meFallbackReason);
            if (!aRate.moValue || !aPeriod.moValue || !aTotal.moValue || !aInvest.moValue)
                return makeErrorResult(
                    eFunction, !aRate.moValue      ? aRate.meError
                               : !aPeriod.moValue  ? aPeriod.meError
                               : !aTotal.moValue   ? aTotal.meError
                                                   : aInvest.meError);

            const auto aResult = spreadsheetengine::core::finance::evaluateInterestSchedulePayment(
                *aRate.moValue, *aPeriod.moValue, *aTotal.moValue, *aInvest.moValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::NUMBER);
        }

        if (aFunctionName == u"IPMT" || aFunctionName == u"PPMT")
        {
            if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aRate = materializeNumericArgument(*rNode.maChildren[0], 0.0);
            const auto aPeriod = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            const auto aTotal = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            const auto aPresentValue = materializeNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aRate.mbSupported)
                return makeUnsupported(eFunction, aRate.meFallbackReason);
            if (!aPeriod.mbSupported)
                return makeUnsupported(eFunction, aPeriod.meFallbackReason);
            if (!aTotal.mbSupported)
                return makeUnsupported(eFunction, aTotal.meFallbackReason);
            if (!aPresentValue.mbSupported)
                return makeUnsupported(eFunction, aPresentValue.meFallbackReason);
            if (!aRate.moValue || !aPeriod.moValue || !aTotal.moValue || !aPresentValue.moValue)
                return makeErrorResult(
                    eFunction, !aRate.moValue ? aRate.meError
                               : !aPeriod.moValue ? aPeriod.meError
                               : !aTotal.moValue ? aTotal.meError
                                                 : aPresentValue.meError);

            double fFutureValue = 0.0;
            if (rNode.maChildren.size() >= 5)
            {
                const auto aFutureValue = materializeNumericArgument(*rNode.maChildren[4], 0.0);
                if (!aFutureValue.mbSupported)
                    return makeUnsupported(eFunction, aFutureValue.meFallbackReason);
                if (!aFutureValue.moValue)
                    return makeErrorResult(eFunction, aFutureValue.meError);
                fFutureValue = *aFutureValue.moValue;
            }

            bool bPayInAdvance = false;
            if (rNode.maChildren.size() == 6)
            {
                const auto aPayType = materializeArgument(*rNode.maChildren[5]);
                if (!aPayType.mbSupported)
                    return makeUnsupported(eFunction, aPayType.meFallbackReason);
                if (!aPayType.moValue)
                    return makeErrorResult(eFunction, aPayType.meError);
                if (!aPayType.moValue->isEmpty())
                {
                    const auto aBool = coerceScalarToBool(rDoc, rContext, *aPayType.moValue);
                    if (!aBool)
                        return makeErrorResult(eFunction, aBool.meError);
                    bPayInAdvance = aBool.maValue;
                }
            }

            if (aFunctionName == u"IPMT")
            {
                const auto aResult = spreadsheetengine::core::finance::evaluateInterestPayment(
                    *aRate.moValue, *aPeriod.moValue, *aTotal.moValue,
                    *aPresentValue.moValue, fFutureValue, bPayInAdvance);
                if (!aResult)
                    return makeErrorResult(eFunction, aResult.meError);
                return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
            }

            const auto aResult = spreadsheetengine::core::finance::evaluatePrincipalPayment(
                *aRate.moValue, *aPeriod.moValue, *aTotal.moValue, *aPresentValue.moValue,
                fFutureValue, bPayInAdvance);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName == u"CUMIPMT" || aFunctionName == u"CUMPRINC")
        {
            if (rNode.maChildren.size() != 6)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aRate = materializeNumericArgument(*rNode.maChildren[0], 0.0);
            const auto aTotal = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            const auto aPresentValue = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            const auto aStart = materializeNumericArgument(*rNode.maChildren[3], 0.0);
            const auto aEnd = materializeNumericArgument(*rNode.maChildren[4], 0.0);
            if (!aRate.mbSupported)
                return makeUnsupported(eFunction, aRate.meFallbackReason);
            if (!aTotal.mbSupported)
                return makeUnsupported(eFunction, aTotal.meFallbackReason);
            if (!aPresentValue.mbSupported)
                return makeUnsupported(eFunction, aPresentValue.meFallbackReason);
            if (!aStart.mbSupported)
                return makeUnsupported(eFunction, aStart.meFallbackReason);
            if (!aEnd.mbSupported)
                return makeUnsupported(eFunction, aEnd.meFallbackReason);
            if (!aRate.moValue || !aTotal.moValue || !aPresentValue.moValue || !aStart.moValue
                || !aEnd.moValue)
                return makeErrorResult(
                    eFunction, !aRate.moValue         ? aRate.meError
                               : !aTotal.moValue      ? aTotal.meError
                               : !aPresentValue.moValue ? aPresentValue.meError
                               : !aStart.moValue      ? aStart.meError
                                                      : aEnd.meError);
            if (rNode.maChildren[5]->meKind == core::formula::NodeKind::EmptyArgument)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aPayType = materializeArgument(*rNode.maChildren[5]);
            if (!aPayType.mbSupported)
                return makeUnsupported(eFunction, aPayType.meFallbackReason);
            if (!aPayType.moValue)
                return makeErrorResult(eFunction, aPayType.meError);
            if (aPayType.moValue->isEmpty())
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aPayType.moValue);
            if (!aBool)
                return makeErrorResult(eFunction, aBool.meError);

            if (aFunctionName == u"CUMIPMT")
            {
                const auto aResult = spreadsheetengine::core::finance::evaluateCumulativeInterest(
                    *aRate.moValue, *aTotal.moValue, *aPresentValue.moValue, *aStart.moValue,
                    *aEnd.moValue, aBool.maValue);
                if (!aResult)
                    return makeErrorResult(eFunction, aResult.meError);
                return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
            }

            const auto aResult = spreadsheetengine::core::finance::evaluateCumulativePrincipal(
                *aRate.moValue, *aTotal.moValue, *aPresentValue.moValue, *aStart.moValue,
                *aEnd.moValue, aBool.maValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName == u"DDB")
        {
            if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aCost = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
            const auto aSalvage = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
            const auto aLife = materializeNumericArgument(*rNode.maChildren[2], std::nullopt);
            const auto aPeriod = materializeNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aCost.mbSupported)
                return makeUnsupported(eFunction, aCost.meFallbackReason);
            if (!aSalvage.mbSupported)
                return makeUnsupported(eFunction, aSalvage.meFallbackReason);
            if (!aLife.mbSupported)
                return makeUnsupported(eFunction, aLife.meFallbackReason);
            if (!aPeriod.mbSupported)
                return makeUnsupported(eFunction, aPeriod.meFallbackReason);
            if (!aCost.moValue || !aSalvage.moValue || !aLife.moValue || !aPeriod.moValue)
                return makeErrorResult(
                    eFunction, !aCost.moValue ? aCost.meError
                               : !aSalvage.moValue ? aSalvage.meError
                               : !aLife.moValue ? aLife.meError
                                                : aPeriod.meError);
            double fFactor = 2.0;
            if (rNode.maChildren.size() == 5)
            {
                if (rNode.maChildren[4]->meKind == core::formula::NodeKind::EmptyArgument)
                    return makeErrorResult(eFunction, api::Error::IllegalArgument);
                const auto aFactor = materializeNumericArgument(*rNode.maChildren[4], std::nullopt);
                if (!aFactor.mbSupported)
                    return makeUnsupported(eFunction, aFactor.meFallbackReason);
                if (!aFactor.moValue)
                    return makeErrorResult(eFunction, aFactor.meError);
                fFactor = *aFactor.moValue;
            }
            const auto aResult = spreadsheetengine::core::finance::evaluateDoubleDecliningBalance(
                *aCost.moValue, *aSalvage.moValue, *aLife.moValue, *aPeriod.moValue, fFactor);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName == u"DB")
        {
            if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aCost = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
            const auto aSalvage = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
            const auto aLife = materializeNumericArgument(*rNode.maChildren[2], std::nullopt);
            const auto aPeriod = materializeNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aCost.mbSupported)
                return makeUnsupported(eFunction, aCost.meFallbackReason);
            if (!aSalvage.mbSupported)
                return makeUnsupported(eFunction, aSalvage.meFallbackReason);
            if (!aLife.mbSupported)
                return makeUnsupported(eFunction, aLife.meFallbackReason);
            if (!aPeriod.mbSupported)
                return makeUnsupported(eFunction, aPeriod.meFallbackReason);
            if (!aCost.moValue || !aSalvage.moValue || !aLife.moValue || !aPeriod.moValue)
                return makeErrorResult(
                    eFunction, !aCost.moValue ? aCost.meError
                               : !aSalvage.moValue ? aSalvage.meError
                               : !aLife.moValue ? aLife.meError
                                                : aPeriod.meError);
            double fMonths = 12.0;
            if (rNode.maChildren.size() == 5)
            {
                const auto aMonths = materializeNumericArgument(*rNode.maChildren[4], std::nullopt);
                if (!aMonths.mbSupported)
                    return makeUnsupported(eFunction, aMonths.meFallbackReason);
                if (!aMonths.moValue)
                    return makeErrorResult(eFunction, aMonths.meError);
                fMonths = ::rtl::math::approxFloor(*aMonths.moValue);
            }
            const auto aResult = spreadsheetengine::core::finance::evaluateFixedDecliningBalance(
                *aCost.moValue, *aSalvage.moValue, *aLife.moValue, *aPeriod.moValue, fMonths);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName == u"SLN")
        {
            if (rNode.maChildren.size() != 3)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aCost = materializeNumericArgument(*rNode.maChildren[0], 0.0);
            const auto aSalvage = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            const auto aLife = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            if (!aCost.mbSupported)
                return makeUnsupported(eFunction, aCost.meFallbackReason);
            if (!aSalvage.mbSupported)
                return makeUnsupported(eFunction, aSalvage.meFallbackReason);
            if (!aLife.mbSupported)
                return makeUnsupported(eFunction, aLife.meFallbackReason);
            if (!aCost.moValue || !aSalvage.moValue || !aLife.moValue)
                return makeErrorResult(
                    eFunction, !aCost.moValue ? aCost.meError
                               : !aSalvage.moValue ? aSalvage.meError
                                                   : aLife.meError);
            const auto aResult = spreadsheetengine::core::finance::evaluateStraightLineDepreciation(
                *aCost.moValue, *aSalvage.moValue, *aLife.moValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName == u"SYD")
        {
            if (rNode.maChildren.size() != 4)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aCost = materializeNumericArgument(*rNode.maChildren[0], 0.0);
            const auto aSalvage = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            const auto aLife = materializeNumericArgument(*rNode.maChildren[2], 0.0);
            const auto aPeriod = materializeNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aCost.mbSupported)
                return makeUnsupported(eFunction, aCost.meFallbackReason);
            if (!aSalvage.mbSupported)
                return makeUnsupported(eFunction, aSalvage.meFallbackReason);
            if (!aLife.mbSupported)
                return makeUnsupported(eFunction, aLife.meFallbackReason);
            if (!aPeriod.mbSupported)
                return makeUnsupported(eFunction, aPeriod.meFallbackReason);
            if (!aCost.moValue || !aSalvage.moValue || !aLife.moValue || !aPeriod.moValue)
                return makeErrorResult(
                    eFunction, !aCost.moValue ? aCost.meError
                               : !aSalvage.moValue ? aSalvage.meError
                               : !aLife.moValue ? aLife.meError
                                                : aPeriod.meError);
            const auto aResult = spreadsheetengine::core::finance::evaluateSumOfYearsDepreciation(
                *aCost.moValue, *aSalvage.moValue, *aLife.moValue, *aPeriod.moValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName == u"PDURATION")
        {
            if (rNode.maChildren.size() != 3)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aRate = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
            const auto aPresent = materializeNumericArgument(*rNode.maChildren[1], std::nullopt);
            const auto aFuture = materializeNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aRate.mbSupported)
                return makeUnsupported(eFunction, aRate.meFallbackReason);
            if (!aPresent.mbSupported)
                return makeUnsupported(eFunction, aPresent.meFallbackReason);
            if (!aFuture.mbSupported)
                return makeUnsupported(eFunction, aFuture.meFallbackReason);
            if (!aRate.moValue || !aPresent.moValue || !aFuture.moValue)
                return makeErrorResult(
                    eFunction, !aRate.moValue ? aRate.meError
                               : !aPresent.moValue ? aPresent.meError
                                                   : aFuture.meError);
            const auto aResult = spreadsheetengine::core::finance::evaluatePaybackDuration(
                *aRate.moValue, *aPresent.moValue, *aFuture.moValue);
            if (!aResult)
                return makeErrorResult(eFunction, aResult.meError);
            return makeNumericResult(eFunction, aResult.maValue, SvNumFormatType::NUMBER);
        }

        if (aFunctionName == u"VDB")
        {
            if (rNode.maChildren.size() < 5 || rNode.maChildren.size() > 7)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aCost = materializeNumericArgument(*rNode.maChildren[0], std::nullopt);
            if (!aCost.mbSupported)
                return makeUnsupported(eFunction, aCost.meFallbackReason);
            if (!aCost.moValue)
                return makeErrorResult(eFunction, aCost.meError);

            const auto aSalvage = materializeNumericArgument(*rNode.maChildren[1], 0.0);
            if (!aSalvage.mbSupported)
                return makeUnsupported(eFunction, aSalvage.meFallbackReason);
            if (!aSalvage.moValue)
                return makeErrorResult(eFunction, aSalvage.meError);

            const auto aLife = materializeNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aLife.mbSupported)
                return makeUnsupported(eFunction, aLife.meFallbackReason);
            if (!aLife.moValue)
                return makeErrorResult(eFunction, aLife.meError);

            const auto aStart = materializeNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aStart.mbSupported)
                return makeUnsupported(eFunction, aStart.meFallbackReason);
            if (!aStart.moValue)
                return makeErrorResult(eFunction, aStart.meError);

            if (rNode.maChildren[4]->meKind == core::formula::NodeKind::EmptyArgument)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aEnd = materializeNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aEnd.mbSupported)
                return makeUnsupported(eFunction, aEnd.meFallbackReason);
            if (!aEnd.moValue)
                return makeErrorResult(eFunction, aEnd.meError);

            double fFactor = 2.0;
            if (rNode.maChildren.size() >= 6)
            {
                if (rNode.maChildren[5]->meKind == core::formula::NodeKind::EmptyArgument)
                    return makeErrorResult(eFunction, api::Error::IllegalArgument);
                const auto aFactor = materializeNumericArgument(*rNode.maChildren[5], std::nullopt);
                if (!aFactor.mbSupported)
                    return makeUnsupported(eFunction, aFactor.meFallbackReason);
                if (!aFactor.moValue)
                    return makeErrorResult(eFunction, aFactor.meError);
                fFactor = *aFactor.moValue;
            }

            bool bNoSwitch = false;
            if (rNode.maChildren.size() == 7)
            {
                const auto aNoSwitch = materializeArgument(*rNode.maChildren[6]);
                if (!aNoSwitch.mbSupported)
                    return makeUnsupported(eFunction, aNoSwitch.meFallbackReason);
                if (!aNoSwitch.moValue)
                    return makeErrorResult(eFunction, aNoSwitch.meError);
                if (!aNoSwitch.moValue->isEmpty())
                {
                    const auto aBool = coerceScalarToBool(rDoc, rContext, *aNoSwitch.moValue);
                    if (!aBool)
                        return makeErrorResult(eFunction, aBool.meError);
                    bNoSwitch = aBool.maValue;
                }
            }

            const auto aDepreciation
                = spreadsheetengine::core::finance::evaluateVariableDecliningBalance(
                    *aCost.moValue, *aSalvage.moValue, *aLife.moValue, *aStart.moValue,
                    *aEnd.moValue, fFactor, bNoSwitch);
            if (!aDepreciation)
                return makeErrorResult(eFunction, aDepreciation.meError);
            return makeNumericResult(eFunction, aDepreciation.maValue, SvNumFormatType::CURRENCY);
        }

        if (aFunctionName != u"RATE")
            return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);

        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aPeriods = materializeNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aPeriods.mbSupported)
            return makeUnsupported(eFunction, aPeriods.meFallbackReason);
        if (!aPeriods.moValue)
            return makeErrorResult(eFunction, aPeriods.meError);

        const auto aPayment = materializeNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPayment.mbSupported)
            return makeUnsupported(eFunction, aPayment.meFallbackReason);
        if (!aPayment.moValue)
            return makeErrorResult(eFunction, aPayment.meError);

        const auto aPresentValue = materializeNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue.mbSupported)
            return makeUnsupported(eFunction, aPresentValue.meFallbackReason);
        if (!aPresentValue.moValue)
            return makeErrorResult(eFunction, aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFutureValue = materializeNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aFutureValue.mbSupported)
                return makeUnsupported(eFunction, aFutureValue.meFallbackReason);
            if (!aFutureValue.moValue)
                return makeErrorResult(eFunction, aFutureValue.meError);
            fFutureValue = *aFutureValue.moValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aPayType = materializeArgument(*rNode.maChildren[4]);
            if (!aPayType.mbSupported)
                return makeUnsupported(eFunction, aPayType.meFallbackReason);
            if (!aPayType.moValue)
                return makeErrorResult(eFunction, aPayType.meError);
            if (aPayType.moValue->isEmpty())
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aBool = coerceScalarToBool(rDoc, rContext, *aPayType.moValue);
            if (!aBool)
                return makeErrorResult(eFunction, aBool.meError);
            bPayInAdvance = aBool.maValue;
        }

        double fGuess = 0.1;
        if (rNode.maChildren.size() == 6
            && rNode.maChildren[5]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aGuess = materializeNumericArgument(*rNode.maChildren[5], std::nullopt);
            if (!aGuess.mbSupported)
                return makeUnsupported(eFunction, aGuess.meFallbackReason);
            if (!aGuess.moValue)
                return makeErrorResult(eFunction, aGuess.meError);
            fGuess = *aGuess.moValue;
        }

        const auto aRate = spreadsheetengine::core::finance::evaluateRate(
            *aPeriods.moValue, *aPayment.moValue, *aPresentValue.moValue, fFutureValue,
            bPayInAdvance, fGuess);
        if (!aRate)
            return makeErrorResult(eFunction, aRate.meError);
        return makeNumericResult(eFunction, aRate.maValue, SvNumFormatType::PERCENT);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateConversionFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const auto oCanonicalName = canonicalConversionFunctionName(uppercaseAscii(rNode.maPrimaryText));
    if (!oCanonicalName)
        return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);

    if (*oCanonicalName == u"EUROCONVERT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aValue = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorResult(eFunction, aValue.meError);
        const auto aValueNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
        if (!aValueNumber)
            return makeErrorResult(eFunction, aValueNumber.meError);

        const auto aFromUnit
            = materializeScalarNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
        if (!aFromUnit.mbSupported)
            return makeUnsupported(eFunction, aFromUnit.meFallbackReason);
        if (!aFromUnit.moValue)
            return makeErrorResult(eFunction, aFromUnit.meError);
        const auto aFromText = coerceScalarToText(rDoc, rContext, *aFromUnit.moValue);
        if (!aFromText)
            return makeErrorResult(eFunction, aFromText.meError);

        const auto aToUnit = materializeScalarNode(*rNode.maChildren[2], rDoc, rContext, rFormulaPos);
        if (!aToUnit.mbSupported)
            return makeUnsupported(eFunction, aToUnit.meFallbackReason);
        if (!aToUnit.moValue)
            return makeErrorResult(eFunction, aToUnit.meError);
        const auto aToText = coerceScalarToText(rDoc, rContext, *aToUnit.moValue);
        if (!aToText)
            return makeErrorResult(eFunction, aToText.meError);

        bool bFullPrecision = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aFullPrecision
                = materializeScalarNode(*rNode.maChildren[3], rDoc, rContext, rFormulaPos);
            if (!aFullPrecision.mbSupported)
                return makeUnsupported(eFunction, aFullPrecision.meFallbackReason);
            if (!aFullPrecision.moValue)
                return makeErrorResult(eFunction, aFullPrecision.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aFullPrecision.moValue);
            if (!aBool)
                return makeErrorResult(eFunction, aBool.meError);
            bFullPrecision = aBool.maValue;
        }

        if (rNode.maChildren.size() == 5)
        {
            if (rNode.maChildren[4]->meKind == core::formula::NodeKind::EmptyArgument)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aPrecision
                = materializeScalarNode(*rNode.maChildren[4], rDoc, rContext, rFormulaPos);
            if (!aPrecision.mbSupported)
                return makeUnsupported(eFunction, aPrecision.meFallbackReason);
            if (!aPrecision.moValue)
                return makeErrorResult(eFunction, aPrecision.meError);
            const auto aPrecisionNumber = coerceScalarToNumber(rDoc, rContext, *aPrecision.moValue);
            if (!aPrecisionNumber)
                return makeErrorResult(eFunction, aPrecisionNumber.meError);
            const auto oWholePrecision
                = spreadsheetengine::core::coercion::toWholeNumber(aPrecisionNumber.maValue);
            if (!oWholePrecision || *oWholePrecision < 3)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
        }

        const auto aEuroConverted = spreadsheetengine::core::convert::evaluateEuroConvertValue(
            aValueNumber.maValue, toApiString(aFromText.maValue), toApiString(aToText.maValue), true,
            !bFullPrecision);
        if (aEuroConverted)
            return makeNumericResult(eFunction, aEuroConverted.maValue, SvNumFormatType::NUMBER);

        return makeErrorResult(eFunction, api::Error::IllegalArgument);
    }

    if (*oCanonicalName == u"DECIMAL")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aText = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aText.mbSupported)
            return makeUnsupported(eFunction, aText.meFallbackReason);
        if (!aText.moValue)
            return makeErrorResult(eFunction, aText.meError);
        const auto aTextValue = coerceScalarToText(rDoc, rContext, *aText.moValue);
        if (!aTextValue)
            return makeErrorResult(eFunction, aTextValue.meError);

        const auto aBase = materializeScalarNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
        if (!aBase.mbSupported)
            return makeUnsupported(eFunction, aBase.meFallbackReason);
        if (!aBase.moValue)
            return makeErrorResult(eFunction, aBase.meError);
        const auto aBaseValue = coerceScalarToNumber(rDoc, rContext, *aBase.moValue);
        if (!aBaseValue)
            return makeErrorResult(eFunction, aBaseValue.meError);

        const auto aDecimal = spreadsheetengine::core::convert::evaluateDecimalValue(
            toApiString(aTextValue.maValue), aBaseValue.maValue);
        if (!aDecimal)
            return makeErrorResult(eFunction, aDecimal.meError);
        return makeNumericResult(eFunction, aDecimal.maValue, SvNumFormatType::NUMBER);
    }

    if (*oCanonicalName == u"DEC2HEX")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aValue = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorResult(eFunction, aValue.meError);
        const auto aValueNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
        if (!aValueNumber)
            return makeErrorResult(eFunction, aValueNumber.meError);

        std::optional<double> ofPlaces;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aPlaces = materializeScalarNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
            if (!aPlaces.mbSupported)
                return makeUnsupported(eFunction, aPlaces.meFallbackReason);
            if (!aPlaces.moValue)
                return makeErrorResult(eFunction, aPlaces.meError);
            const auto aPlacesNumber = coerceScalarToNumber(rDoc, rContext, *aPlaces.moValue);
            if (!aPlacesNumber)
                return makeErrorResult(eFunction, aPlacesNumber.meError);
            ofPlaces = aPlacesNumber.maValue;
        }

        const auto aHexText = spreadsheetengine::core::convert::evaluateBaseValue(
            aValueNumber.maValue, 16.0, ofPlaces);
        if (!aHexText)
            return makeErrorResult(eFunction, aHexText.meError);
        return makeStringResult(eFunction, toLibreOfficeString(aHexText.maValue));
    }

    if (*oCanonicalName == u"BASE")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aValue = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorResult(eFunction, aValue.meError);
        const auto aValueNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
        if (!aValueNumber)
            return makeErrorResult(eFunction, aValueNumber.meError);

        const auto aBase = materializeScalarNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
        if (!aBase.mbSupported)
            return makeUnsupported(eFunction, aBase.meFallbackReason);
        if (!aBase.moValue)
            return makeErrorResult(eFunction, aBase.meError);
        const auto aBaseNumber = coerceScalarToNumber(rDoc, rContext, *aBase.moValue);
        if (!aBaseNumber)
            return makeErrorResult(eFunction, aBaseNumber.meError);

        std::optional<double> ofMinLength;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMinLength = materializeScalarNode(
                *rNode.maChildren[2], rDoc, rContext, rFormulaPos);
            if (!aMinLength.mbSupported)
                return makeUnsupported(eFunction, aMinLength.meFallbackReason);
            if (!aMinLength.moValue)
                return makeErrorResult(eFunction, aMinLength.meError);
            const auto aMinLengthNumber = coerceScalarToNumber(rDoc, rContext, *aMinLength.moValue);
            if (!aMinLengthNumber)
                return makeErrorResult(eFunction, aMinLengthNumber.meError);
            ofMinLength = aMinLengthNumber.maValue;
        }

        const auto aBaseText = spreadsheetengine::core::convert::evaluateBaseValue(
            aValueNumber.maValue, aBaseNumber.maValue, ofMinLength);
        if (!aBaseText)
            return makeErrorResult(eFunction, aBaseText.meError);
        return makeStringResult(eFunction, toLibreOfficeString(aBaseText.maValue));
    }

    if (*oCanonicalName == u"ROMAN")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aValue = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorResult(eFunction, aValue.meError);
        const auto aValueNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
        if (!aValueNumber)
            return makeErrorResult(eFunction, aValueNumber.meError);

        std::optional<double> ofMode;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMode = materializeScalarNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
            if (!aMode.mbSupported)
                return makeUnsupported(eFunction, aMode.meFallbackReason);
            if (!aMode.moValue)
                return makeErrorResult(eFunction, aMode.meError);
            const auto aModeNumber = coerceScalarToNumber(rDoc, rContext, *aMode.moValue);
            if (!aModeNumber)
                return makeErrorResult(eFunction, aModeNumber.meError);
            ofMode = aModeNumber.maValue;
        }

        const auto aRoman = spreadsheetengine::core::convert::evaluateRomanValue(
            aValueNumber.maValue, ofMode);
        if (!aRoman)
            return makeErrorResult(eFunction, aRoman.meError);
        return makeStringResult(eFunction, toLibreOfficeString(aRoman.maValue));
    }

    if (*oCanonicalName == u"ARABIC")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aRoman = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aRoman.mbSupported)
            return makeUnsupported(eFunction, aRoman.meFallbackReason);
        if (!aRoman.moValue)
            return makeErrorResult(eFunction, aRoman.meError);
        const auto aRomanText = coerceScalarToText(rDoc, rContext, *aRoman.moValue);
        if (!aRomanText)
            return makeErrorResult(eFunction, aRomanText.meError);

        const auto oArabic = spreadsheetengine::core::convert::convertFromRoman(
            toApiString(aRomanText.maValue));
        if (!oArabic)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        return makeNumericResult(
            eFunction, static_cast<double>(*oArabic), SvNumFormatType::NUMBER);
    }

    if (*oCanonicalName != u"CONVERT")
        return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);

    if (rNode.maChildren.size() != 3)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    const auto aValue = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aValue.mbSupported)
        return makeUnsupported(eFunction, aValue.meFallbackReason);
    if (!aValue.moValue)
        return makeErrorResult(eFunction, aValue.meError);
    const auto aValueNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
    if (!aValueNumber)
        return makeErrorResult(eFunction, aValueNumber.meError);

    const auto aFromUnit = materializeScalarNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
    if (!aFromUnit.mbSupported)
        return makeUnsupported(eFunction, aFromUnit.meFallbackReason);
    if (!aFromUnit.moValue)
        return makeErrorResult(eFunction, aFromUnit.meError);
    const auto aFromText = coerceScalarToText(rDoc, rContext, *aFromUnit.moValue);
    if (!aFromText)
        return makeErrorResult(eFunction, aFromText.meError);

    const auto aToUnit = materializeScalarNode(*rNode.maChildren[2], rDoc, rContext, rFormulaPos);
    if (!aToUnit.mbSupported)
        return makeUnsupported(eFunction, aToUnit.meFallbackReason);
    if (!aToUnit.moValue)
        return makeErrorResult(eFunction, aToUnit.meError);
    const auto aToText = coerceScalarToText(rDoc, rContext, *aToUnit.moValue);
    if (!aToText)
        return makeErrorResult(eFunction, aToText.meError);

    const auto aConverted = spreadsheetengine::core::convert::evaluateConvertValue(
        aValueNumber.maValue, toApiString(aFromText.maValue), toApiString(aToText.maValue));
    if (aConverted)
        return makeNumericResult(eFunction, aConverted.maValue, SvNumFormatType::NUMBER);

    const auto aEuroConverted = spreadsheetengine::core::convert::evaluateEuroConvertValue(
        aValueNumber.maValue, toApiString(aFromText.maValue), toApiString(aToText.maValue), false,
        false);
    if (aEuroConverted)
        return makeNumericResult(eFunction, aEuroConverted.maValue, SvNumFormatType::NUMBER);

    return makeErrorResult(eFunction, aConverted.meError);
}

[[nodiscard]] inline bool formulaContainsAggregateLike(const core::formula::Node& rNode)
{
    if (rNode.meKind == core::formula::NodeKind::FunctionCall)
    {
        const api::String aName = uppercaseAscii(rNode.maPrimaryText);
        if (aName == u"SUBTOTAL" || aName == u"AGGREGATE" || aName == u"COM.MICROSOFT.AGGREGATE")
            return true;
    }

    for (const auto& rxChild : rNode.maChildren)
    {
        if (rxChild && formulaContainsAggregateLike(*rxChild))
            return true;
    }

    return false;
}

[[nodiscard]] inline bool cellContainsAggregateLike(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const ScAddress& rAddress)
{
    if (!const_cast<ScDocument&>(rDoc).GetFormulaCell(rAddress))
        return false;

    const auto aFormulaText = formulainspection::formulaTextForCell(rDoc, rContext, rAddress);
    if (!aFormulaText)
        return false;

    const auto aNormalized = normalizeFormulaSource(
        std::u16string_view(aFormulaText.maValue.getStr(), aFormulaText.maValue.getLength()));
    const auto aParse = core::formula::parseFormula(aNormalized);
    return aParse && aParse.mpRoot && formulaContainsAggregateLike(*aParse.mpRoot);
}

[[nodiscard]] inline EvaluationAttempt evaluateAggregateFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if (aFunctionName != u"AGGREGATE" && aFunctionName != u"COM.MICROSOFT.AGGREGATE")
        return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
    if (rNode.maChildren.size() < 3)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    const auto aFunctionCode = materializeScalarNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aFunctionCode.mbSupported)
        return makeUnsupported(eFunction, aFunctionCode.meFallbackReason);
    if (!aFunctionCode.moValue)
        return makeErrorResult(eFunction, aFunctionCode.meError);
    const auto aFunctionNumber = coerceScalarToNumber(rDoc, rContext, *aFunctionCode.moValue);
    if (!aFunctionNumber)
        return makeErrorResult(eFunction, aFunctionNumber.meError);

    const auto aOptionCode = materializeScalarNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
    if (!aOptionCode.mbSupported)
        return makeUnsupported(eFunction, aOptionCode.meFallbackReason);
    if (!aOptionCode.moValue)
        return makeErrorResult(eFunction, aOptionCode.meError);
    const auto aOptionNumber = coerceScalarToNumber(rDoc, rContext, *aOptionCode.moValue);
    if (!aOptionNumber)
        return makeErrorResult(eFunction, aOptionNumber.meError);

    const auto oFunction = coerceWholeNumber(aFunctionNumber.maValue);
    const auto oOption = coerceWholeNumber(aOptionNumber.maValue);
    if (!oFunction || !oOption || *oFunction < 1 || *oFunction > 19)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    const auto oOptions = spreadsheetengine::core::math::decodeAggregateOptions(*oOption);
    if (!oOptions)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    auto consumeAggregateValue = [&](const api::CellValue& rValue,
                                     spreadsheetengine::core::math::AggregateScan& rScan)
        -> std::optional<api::Error> {
        if (rValue.isError())
        {
            if (oOptions->mbIgnoreErrors || *oFunction == 2)
                return std::nullopt;
            if (*oFunction == 3)
            {
                ++rScan.mnNonEmptyCount;
                return std::nullopt;
            }
            return rValue.meError;
        }

        if (!rValue.isEmpty())
            ++rScan.mnNonEmptyCount;
        if (rValue.isNumber())
            rScan.maNumbers.push_back(rValue.mfNumber);
        return std::nullopt;
    };

    auto scanAggregateArgument = [&](const core::formula::Node& rArgument,
                                     spreadsheetengine::core::math::AggregateScan& rScan)
        -> std::optional<EvaluationAttempt> {
        const bool bReferenceLike = rArgument.meKind == core::formula::NodeKind::CellReference
                                    || rArgument.meKind == core::formula::NodeKind::RangeReference
                                    || rArgument.meKind == core::formula::NodeKind::NamedReference;

        if (bReferenceLike)
        {
            const auto aRange = resolveReferenceRangeNode(rArgument, rDoc, rFormulaPos);
            if (!aRange.mbSupported)
                return makeUnsupported(eFunction, aRange.meFallbackReason);
            if (!aRange.moValue)
                return makeErrorResult(eFunction, aRange.meError);

            for (SCROW nRow = aRange.moValue->aStart.Row(); nRow <= aRange.moValue->aEnd.Row(); ++nRow)
            {
                const bool bFilteredRow = rDoc.RowFiltered(nRow, aRange.moValue->aStart.Tab());
                const bool bHiddenRow = rDoc.RowHidden(nRow, aRange.moValue->aStart.Tab());
                if (bFilteredRow || (oOptions->mbIgnoreHiddenRows && bHiddenRow))
                    continue;

                for (SCCOL nCol = aRange.moValue->aStart.Col(); nCol <= aRange.moValue->aEnd.Col();
                     ++nCol)
                {
                    const ScAddress aAddress(nCol, nRow, aRange.moValue->aStart.Tab());
                    if (oOptions->mbIgnoreNestedAggregates
                        && cellContainsAggregateLike(rDoc, rContext, aAddress))
                    {
                        continue;
                    }

                    if (const auto oError
                        = consumeAggregateValue(readMaterializedHostCellValue(rDoc, rContext, aAddress),
                            rScan))
                    {
                        return makeErrorResult(eFunction, *oError);
                    }
                }
            }

            return std::nullopt;
        }

        const bool bMatrixLike = rArgument.meKind == core::formula::NodeKind::ArrayConstant
                                 || rArgument.meKind == core::formula::NodeKind::BinaryOperation
                                 || rArgument.meKind == core::formula::NodeKind::FunctionCall;
        if (bMatrixLike)
        {
            const auto aMatrix = materializeMatrixNode(rArgument, rDoc, rContext, rFormulaPos);
            if (!aMatrix.mbSupported)
                return makeUnsupported(eFunction, aMatrix.meFallbackReason);
            if (!aMatrix.moValue)
                return makeErrorResult(eFunction, aMatrix.meError);

            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
            for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    const auto aValue = lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(nColumn, nRow));
                    if (const auto oError = consumeAggregateValue(aValue, rScan))
                        return makeErrorResult(eFunction, *oError);
                }
            }

            return std::nullopt;
        }

        const auto aScalar = materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
        if (!aScalar.mbSupported)
            return makeUnsupported(eFunction, aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeErrorResult(eFunction, aScalar.meError);
        if (const auto oError = consumeAggregateValue(*aScalar.moValue, rScan))
            return makeErrorResult(eFunction, *oError);
        return std::nullopt;
    };

    spreadsheetengine::core::math::AggregateScan aScan;
    if (*oFunction >= 14)
    {
        if (rNode.maChildren.size() != 4)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        if (auto oAttempt = scanAggregateArgument(*rNode.maChildren[2], aScan))
            return *oAttempt;

        const auto aRank = materializeScalarNode(*rNode.maChildren[3], rDoc, rContext, rFormulaPos);
        if (!aRank.mbSupported)
            return makeUnsupported(eFunction, aRank.meFallbackReason);
        if (!aRank.moValue)
            return makeErrorResult(eFunction, aRank.meError);
        const auto aRankNumber = coerceScalarToNumber(rDoc, rContext, *aRank.moValue);
        if (!aRankNumber)
            return makeErrorResult(eFunction, aRankNumber.meError);

        const auto aAggregate = spreadsheetengine::core::math::evaluateAggregateRankedNumbers(
            *oFunction, aScan, aRankNumber.maValue);
        if (!aAggregate)
            return makeErrorResult(eFunction, aAggregate.meError);
        return makeNumericResult(eFunction, aAggregate.maValue, SvNumFormatType::NUMBER);
    }

    for (std::size_t nIndex = 2; nIndex < rNode.maChildren.size(); ++nIndex)
    {
        if (auto oAttempt = scanAggregateArgument(*rNode.maChildren[nIndex], aScan))
            return *oAttempt;
    }

    const auto aAggregate
        = spreadsheetengine::core::math::evaluateAggregateNumbers(*oFunction, aScan);
    if (!aAggregate)
        return makeErrorResult(eFunction, aAggregate.meError);
    return makeNumericResult(eFunction, aAggregate.maValue, SvNumFormatType::NUMBER);
}

[[nodiscard]] inline EvaluationAttempt evaluateNumericAggregateFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos,
    bool bImportedCanonicalSource)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
        && containsReferenceLikeDescendant(rNode))
    {
        return makeErrorResult(eFunction, api::Error::VariableExpected);
    }

    const auto makeNumericAttempt = [&](double fValue) {
        return makeNumericResult(eFunction, fValue, SvNumFormatType::NUMBER);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto collectNumericAggregateValues = [&]() -> Materialization<std::vector<double>> {
        if (rNode.maChildren.empty())
            return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

        std::vector<double> aValues;
        for (const auto& rxChild : rNode.maChildren)
        {
            if (!rxChild)
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const bool bMatrixLike = rxChild->meKind == core::formula::NodeKind::CellReference
                                     || rxChild->meKind == core::formula::NodeKind::RangeReference
                                     || rxChild->meKind == core::formula::NodeKind::NamedReference
                                     || rxChild->meKind == core::formula::NodeKind::ArrayConstant
                                     || rxChild->meKind == core::formula::NodeKind::BinaryOperation
                                     || rxChild->meKind == core::formula::NodeKind::FunctionCall;
            if (bMatrixLike)
            {
                const auto aMatrix = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
                if (!aMatrix.mbSupported)
                {
                    return makeUnsupportedMaterialization<std::vector<double>>(
                        aMatrix.meFallbackReason);
                }
                if (!aMatrix.moValue)
                    return makeMaterializedError<std::vector<double>>(aMatrix.meError);

                SCSIZE nColumns = 0;
                SCSIZE nRows = 0;
                (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                    {
                        const auto aValue = lookupexecution::detail::toApiCellValue(
                            (*aMatrix.moValue)->Get(nColumn, nRow));
                        if (aValue.isEmpty() || aValue.isText())
                            continue;

                        const auto aNumber = coerceScalarToNumber(rDoc, rContext, aValue);
                        if (!aNumber)
                        {
                            return makeMaterializedError<std::vector<double>>(
                                aNumber.meError);
                        }
                        aValues.push_back(aNumber.maValue);
                    }
                }
                continue;
            }

            const auto aScalar = materializeScalarNode(*rxChild, rDoc, rContext, rFormulaPos);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<std::vector<double>>(aScalar.meFallbackReason);
            if (!aScalar.moValue)
                return makeMaterializedError<std::vector<double>>(aScalar.meError);
            if (aScalar.moValue->isEmpty())
                continue;
            if (aScalar.moValue->isText())
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
            if (!aNumber)
                return makeMaterializedError<std::vector<double>>(aNumber.meError);
            aValues.push_back(aNumber.maValue);
        }

        return makeMaterializedValue(std::move(aValues));
    };

    const auto collectPairedNumericAggregateValues
        = [&]() -> Materialization<std::pair<std::vector<double>, std::vector<double>>> {
        if (rNode.maChildren.size() != 2)
        {
            return makeMaterializedError<std::pair<std::vector<double>, std::vector<double>>>(
                api::Error::IllegalArgument);
        }

        std::pair<std::vector<double>, std::vector<double>> aPair;
        for (std::size_t nIndex = 0; nIndex < 2; ++nIndex)
        {
            const auto aMatrix = materializeMatrixNode(
                *rNode.maChildren[nIndex], rDoc, rContext, rFormulaPos);
            if (!aMatrix.mbSupported)
            {
                return makeUnsupportedMaterialization<
                    std::pair<std::vector<double>, std::vector<double>>>(aMatrix.meFallbackReason);
            }
            if (!aMatrix.moValue)
            {
                return makeMaterializedError<std::pair<std::vector<double>, std::vector<double>>>(
                    aMatrix.meError);
            }

            auto& rValues = nIndex == 0 ? aPair.first : aPair.second;
            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
            for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    const auto aValue = lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(nColumn, nRow));
                    if (aValue.isEmpty() || aValue.isText())
                        continue;

                    const auto aNumber = coerceScalarToNumber(rDoc, rContext, aValue);
                    if (!aNumber)
                    {
                        return makeMaterializedError<
                            std::pair<std::vector<double>, std::vector<double>>>(
                            aNumber.meError);
                    }
                    rValues.push_back(aNumber.maValue);
                }
            }
        }

        if (aPair.first.size() != aPair.second.size())
        {
            return makeMaterializedError<std::pair<std::vector<double>, std::vector<double>>>(
                api::Error::IllegalArgument);
        }
        return makeMaterializedValue(std::move(aPair));
    };
    const auto evaluateSumProduct = [&]() -> EvaluationAttempt {
        if (rNode.maChildren.empty())
            return makeErrorAttempt(api::Error::IllegalArgument);

        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        bool bHaveDimensions = false;
        std::vector<double> aProducts;
        std::vector<bool> aIgnore;

        for (const auto& rxChild : rNode.maChildren)
        {
            if (!rxChild)
                return makeErrorAttempt(api::Error::IllegalArgument);

            const auto aMatrix = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
            if (!aMatrix.mbSupported)
                return makeUnsupported(eFunction, aMatrix.meFallbackReason);
            if (!aMatrix.moValue)
                return makeErrorAttempt(aMatrix.meError);

            SCSIZE nChildColumns = 0;
            SCSIZE nChildRows = 0;
            (*aMatrix.moValue)->GetDimensions(nChildColumns, nChildRows);
            if (!bHaveDimensions)
            {
                nColumns = nChildColumns;
                nRows = nChildRows;
                if (nColumns == 0 || nRows == 0)
                    return makeErrorAttempt(api::Error::IllegalArgument);
                aProducts.assign(nColumns * nRows, 1.0);
                aIgnore.assign(nColumns * nRows, false);
                bHaveDimensions = true;
            }
            else if (nChildColumns != nColumns || nChildRows != nRows)
                return makeErrorAttempt(api::Error::IllegalArgument);

            std::size_t nIndex = 0;
            for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn, ++nIndex)
                {
                    if (aIgnore[nIndex])
                        continue;

                    const auto aValue = lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(nColumn, nRow));
                    if (aValue.isText())
                    {
                        aIgnore[nIndex] = true;
                        continue;
                    }

                    double fNumeric = 0.0;
                    if (aValue.isEmpty())
                        fNumeric = 0.0;
                    else
                    {
                        const auto aNumber = coerceScalarToNumber(rDoc, rContext, aValue);
                        if (!aNumber)
                            return makeErrorAttempt(aNumber.meError);
                        fNumeric = aNumber.maValue;
                    }

                    aProducts[nIndex] *= fNumeric;
                }
            }
        }

        double fTotal = 0.0;
        for (std::size_t nIndex = 0; nIndex < aProducts.size(); ++nIndex)
        {
            if (!aIgnore[nIndex])
                fTotal += aProducts[nIndex];
        }
        return makeNumericAttempt(fTotal);
    };

    if (aFunctionName == u"SUM" || aFunctionName == u"PRODUCT" || aFunctionName == u"SUMSQ"
        || aFunctionName == u"AVERAGE" || aFunctionName == u"DEVSQ"
        || aFunctionName == u"MULTINOMIAL")
    {
        const auto aValues = collectNumericAggregateValues();
        if (!aValues.mbSupported)
            return makeUnsupported(eFunction, aValues.meFallbackReason);
        if (!aValues.moValue)
            return makeErrorAttempt(aValues.meError);

        const auto& rValues = *aValues.moValue;
        if (aFunctionName == u"SUM")
            return makeNumericAttempt(std::accumulate(rValues.begin(), rValues.end(), 0.0));

        if (aFunctionName == u"PRODUCT")
        {
            double fProduct = 1.0;
            for (double fValue : rValues)
                fProduct *= fValue;
            return makeNumericAttempt(rValues.empty() ? 0.0 : fProduct);
        }

        if (aFunctionName == u"SUMSQ")
        {
            double fTotal = 0.0;
            for (double fValue : rValues)
                fTotal += fValue * fValue;
            return makeNumericAttempt(fTotal);
        }

        if (aFunctionName == u"AVERAGE")
        {
            if (rValues.empty())
                return makeErrorAttempt(api::Error::DivisionByZero);
            const double fTotal = std::accumulate(rValues.begin(), rValues.end(), 0.0);
            return makeNumericAttempt(fTotal / static_cast<double>(rValues.size()));
        }

        if (aFunctionName == u"DEVSQ")
        {
            if (rValues.empty())
                return makeNumericAttempt(0.0);
            const double fMean = std::accumulate(rValues.begin(), rValues.end(), 0.0)
                                 / static_cast<double>(rValues.size());
            double fDeviation = 0.0;
            for (double fValue : rValues)
            {
                const double fDelta = fValue - fMean;
                fDeviation += fDelta * fDelta;
            }
            return makeNumericAttempt(fDeviation);
        }

        if (rValues.empty())
            return makeErrorAttempt(api::Error::IllegalArgument);

        long double fLogGamma = 0.0L;
        sal_Int64 nTotal = 0;
        for (double fValue : rValues)
        {
            const auto oWhole = coerceWholeNumber(fValue);
            if (!oWhole || *oWhole < 0)
                return makeErrorAttempt(api::Error::IllegalArgument);
            nTotal += *oWhole;
            fLogGamma += std::lgammal(static_cast<long double>(*oWhole) + 1.0L);
        }

        const long double fResult
            = std::exp(std::lgammal(static_cast<long double>(nTotal) + 1.0L) - fLogGamma);
        if (!std::isfinite(static_cast<double>(fResult)))
            return makeErrorAttempt(api::Error::Domain);
        return makeNumericAttempt(static_cast<double>(std::round(fResult)));
    }

    if (aFunctionName == u"SUMPRODUCT")
        return evaluateSumProduct();

    if (aFunctionName == u"SUMX2MY2" || aFunctionName == u"SUMX2PY2" || aFunctionName == u"SUMXMY2")
    {
        const auto aPair = collectPairedNumericAggregateValues();
        if (!aPair.mbSupported)
            return makeUnsupported(eFunction, aPair.meFallbackReason);
        if (!aPair.moValue)
            return makeErrorAttempt(aPair.meError);

        const auto& [rLeft, rRight] = *aPair.moValue;
        double fTotal = 0.0;
        for (std::size_t nIndex = 0; nIndex < rLeft.size(); ++nIndex)
        {
            if (aFunctionName == u"SUMX2MY2")
                fTotal += rLeft[nIndex] * rLeft[nIndex] - rRight[nIndex] * rRight[nIndex];
            else if (aFunctionName == u"SUMX2PY2")
                fTotal += rLeft[nIndex] * rLeft[nIndex] + rRight[nIndex] * rRight[nIndex];
            else
            {
                const double fDelta = rLeft[nIndex] - rRight[nIndex];
                fTotal += fDelta * fDelta;
            }
        }
        return makeNumericAttempt(fTotal);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateRankedAggregateFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos,
    bool bImportedCanonicalSource)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
        && containsReferenceLikeDescendant(rNode))
    {
        return makeErrorResult(eFunction, api::Error::VariableExpected);
    }

    const auto makeNumericAttempt = [&](double fValue) {
        return makeNumericResult(eFunction, fValue, SvNumFormatType::NUMBER);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto materializeNumericScalar = [&](const core::formula::Node& rArgument)
        -> Materialization<double> {
        const auto aScalar = materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<double>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<double>(aScalar.meError);

        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
        if (!aNumber)
            return makeMaterializedError<double>(aNumber.meError);
        return makeMaterializedValue(aNumber.maValue);
    };
    const auto collectRankedAggregateScan
        = [&](const core::formula::Node& rArgument)
        -> Materialization<spreadsheetengine::core::math::AggregateScan> {
        spreadsheetengine::core::math::AggregateScan aScan;
        const bool bMatrixLike = rArgument.meKind == core::formula::NodeKind::CellReference
                                 || rArgument.meKind == core::formula::NodeKind::RangeReference
                                 || rArgument.meKind == core::formula::NodeKind::NamedReference
                                 || rArgument.meKind == core::formula::NodeKind::ArrayConstant
                                 || rArgument.meKind == core::formula::NodeKind::BinaryOperation
                                 || rArgument.meKind == core::formula::NodeKind::FunctionCall;
        if (bMatrixLike)
        {
            const auto aMatrix = materializeMatrixNode(rArgument, rDoc, rContext, rFormulaPos);
            if (!aMatrix.mbSupported)
            {
                return makeUnsupportedMaterialization<
                    spreadsheetengine::core::math::AggregateScan>(aMatrix.meFallbackReason);
            }
            if (!aMatrix.moValue)
            {
                return makeMaterializedError<spreadsheetengine::core::math::AggregateScan>(
                    aMatrix.meError);
            }

            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
            for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    const auto aValue = lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(nColumn, nRow));
                    if (aValue.isError())
                    {
                        return makeMaterializedError<
                            spreadsheetengine::core::math::AggregateScan>(aValue.meError);
                    }
                    if (aValue.isNumber())
                        aScan.maNumbers.push_back(aValue.mfNumber);
                }
            }
            return makeMaterializedValue(std::move(aScan));
        }

        const auto aScalar = materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
        if (!aScalar.mbSupported)
        {
            return makeUnsupportedMaterialization<spreadsheetengine::core::math::AggregateScan>(
                aScalar.meFallbackReason);
        }
        if (!aScalar.moValue)
        {
            return makeMaterializedError<spreadsheetengine::core::math::AggregateScan>(
                aScalar.meError);
        }
        if (aScalar.moValue->isNumber())
            aScan.maNumbers.push_back(aScalar.moValue->mfNumber);
        else if (!aScalar.moValue->isEmpty())
        {
            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
            if (!aNumber)
            {
                return makeMaterializedError<
                    spreadsheetengine::core::math::AggregateScan>(aNumber.meError);
            }
            aScan.maNumbers.push_back(aNumber.maValue);
        }
        return makeMaterializedValue(std::move(aScan));
    };

    if (aFunctionName == u"LARGE" || aFunctionName == u"SMALL")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aScan = collectRankedAggregateScan(*rNode.maChildren[0]);
        if (!aScan.mbSupported)
            return makeUnsupported(eFunction, aScan.meFallbackReason);
        if (!aScan.moValue)
            return makeErrorAttempt(aScan.meError);
        if (aScan.moValue->maNumbers.empty())
            return makeErrorAttempt(api::Error::NoValue);

        const auto aRank = materializeNumericScalar(*rNode.maChildren[1]);
        if (!aRank.mbSupported)
            return makeUnsupported(eFunction, aRank.meFallbackReason);
        if (!aRank.moValue)
            return makeErrorAttempt(aRank.meError);

        const auto aAggregate = spreadsheetengine::core::math::evaluateAggregateRankedNumbers(
            aFunctionName == u"LARGE" ? 14 : 15, *aScan.moValue, *aRank.moValue);
        if (!aAggregate)
            return makeErrorAttempt(aAggregate.meError);
        return makeNumericAttempt(aAggregate.maValue);
    }

    if (aFunctionName == u"QUARTILE" || aFunctionName == u"QUARTILE.INC"
        || aFunctionName == u"COM.MICROSOFT.QUARTILE.INC"
        || aFunctionName == u"QUARTILE.EXC"
        || aFunctionName == u"COM.MICROSOFT.QUARTILE.EXC")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aScan = collectRankedAggregateScan(*rNode.maChildren[0]);
        if (!aScan.mbSupported)
            return makeUnsupported(eFunction, aScan.meFallbackReason);
        if (!aScan.moValue)
            return makeErrorAttempt(aScan.meError);
        if (aScan.moValue->maNumbers.empty())
            return makeErrorAttempt(api::Error::NoValue);

        const auto aRank = materializeNumericScalar(*rNode.maChildren[1]);
        if (!aRank.mbSupported)
            return makeUnsupported(eFunction, aRank.meFallbackReason);
        if (!aRank.moValue)
            return makeErrorAttempt(aRank.meError);

        const double fRank = ::rtl::math::approxFloor(*aRank.moValue);
        const std::int32_t nFunction
            = (aFunctionName == u"QUARTILE.EXC"
               || aFunctionName == u"COM.MICROSOFT.QUARTILE.EXC")
                  ? 19
                  : 17;
        const auto aAggregate = spreadsheetengine::core::math::evaluateAggregateRankedNumbers(
            nFunction, *aScan.moValue, fRank);
        if (!aAggregate)
            return makeErrorAttempt(aAggregate.meError);
        return makeNumericAttempt(aAggregate.maValue);
    }

    if (aFunctionName == u"PERCENTRANK" || aFunctionName == u"PERCENTRANK.INC"
        || aFunctionName == u"COM.MICROSOFT.PERCENTRANK.INC"
        || aFunctionName == u"PERCENTRANK.EXC"
        || aFunctionName == u"COM.MICROSOFT.PERCENTRANK.EXC")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aScan = collectRankedAggregateScan(*rNode.maChildren[0]);
        if (!aScan.mbSupported)
            return makeUnsupported(eFunction, aScan.meFallbackReason);
        if (!aScan.moValue)
            return makeErrorAttempt(aScan.meError);
        if (aScan.moValue->maNumbers.empty())
            return makeErrorAttempt(api::Error::NoValue);

        const auto aValue = materializeNumericScalar(*rNode.maChildren[1]);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);

        std::int32_t nSignificance = 3;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aSignificance = materializeNumericScalar(*rNode.maChildren[2]);
            if (!aSignificance.mbSupported)
                return makeUnsupported(eFunction, aSignificance.meFallbackReason);
            if (!aSignificance.moValue)
                return makeErrorAttempt(aSignificance.meError);

            nSignificance
                = static_cast<std::int32_t>(::rtl::math::approxFloor(*aSignificance.moValue));
            if (nSignificance < 1)
                return makeErrorAttempt(api::Error::IllegalArgument);
        }

        const bool bInclusive = aFunctionName != u"PERCENTRANK.EXC"
                                && aFunctionName != u"COM.MICROSOFT.PERCENTRANK.EXC";
        const auto aRank = spreadsheetengine::core::math::evaluatePercentrank(
            aScan.moValue->maNumbers, *aValue.moValue, bInclusive, nSignificance);
        if (!aRank)
        {
            return makeErrorAttempt(aRank.meError == api::Error::NotAvailable
                                        ? api::Error::NoValue
                                        : aRank.meError);
        }
        return makeNumericAttempt(aRank.maValue);
    }

    if (aFunctionName == u"RANK" || aFunctionName == u"RANK.EQ"
        || aFunctionName == u"COM.MICROSOFT.RANK.EQ"
        || aFunctionName == u"RANK.AVG" || aFunctionName == u"COM.MICROSOFT.RANK.AVG")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aValue = materializeNumericScalar(*rNode.maChildren[0]);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorAttempt(aValue.meError);

        const auto aScan = collectRankedAggregateScan(*rNode.maChildren[1]);
        if (!aScan.mbSupported)
            return makeUnsupported(eFunction, aScan.meFallbackReason);
        if (!aScan.moValue)
            return makeErrorAttempt(aScan.meError);
        auto aSortArray = aScan.moValue->maNumbers;
        if (aSortArray.empty())
            return makeErrorAttempt(api::Error::NoValue);

        bool bAscending = false;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aAscending = materializeNumericScalar(*rNode.maChildren[2]);
            if (!aAscending.mbSupported)
                return makeUnsupported(eFunction, aAscending.meFallbackReason);
            if (!aAscending.moValue)
                return makeErrorAttempt(aAscending.meError);
            bAscending = !rtl::math::approxEqual(*aAscending.moValue, 0.0);
        }

        std::sort(aSortArray.begin(), aSortArray.end());
        if (*aValue.moValue < aSortArray.front() || *aValue.moValue > aSortArray.back())
            return makeErrorAttempt(api::Error::NotAvailable);

        double fFirstPos = -1.0;
        double fLastPos = 0.0;
        bool bFinished = false;
        std::size_t nIndex = 0;
        for (; nIndex < aSortArray.size() && !bFinished; ++nIndex)
        {
            if (rtl::math::approxEqual(aSortArray[nIndex], *aValue.moValue))
            {
                if (fFirstPos < 0.0)
                    fFirstPos = static_cast<double>(nIndex) + 1.0;
            }
            else if (aSortArray[nIndex] > *aValue.moValue)
            {
                fLastPos = static_cast<double>(nIndex);
                bFinished = true;
            }
        }
        if (!bFinished)
            fLastPos = static_cast<double>(nIndex);
        if (fFirstPos <= 0.0)
            return makeErrorAttempt(api::Error::NotAvailable);

        const bool bAverage
            = aFunctionName == u"RANK.AVG" || aFunctionName == u"COM.MICROSOFT.RANK.AVG";
        const double fSize = static_cast<double>(aSortArray.size());
        if (!bAverage)
        {
            return makeNumericAttempt(
                bAscending ? fFirstPos : fSize + 1.0 - fLastPos);
        }

        return makeNumericAttempt(
            bAscending ? (fFirstPos + fLastPos) / 2.0
                       : fSize + 1.0 - (fFirstPos + fLastPos) / 2.0);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateStatisticalAggregateFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos,
    bool bImportedCanonicalSource)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
        && containsReferenceLikeDescendant(rNode))
    {
        return makeErrorResult(eFunction, api::Error::VariableExpected);
    }

    const auto makeNumericAttempt = [&](double fValue) {
        return makeNumericResult(eFunction, fValue, SvNumFormatType::NUMBER);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto collectStatisticalValues = [&](bool bTextAsZero)
        -> Materialization<std::vector<double>> {
        if (rNode.maChildren.empty())
            return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

        std::vector<double> aValues;
        for (const auto& rxChild : rNode.maChildren)
        {
            if (!rxChild)
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const bool bMatrixLike = rxChild->meKind == core::formula::NodeKind::CellReference
                                     || rxChild->meKind == core::formula::NodeKind::RangeReference
                                     || rxChild->meKind == core::formula::NodeKind::NamedReference
                                     || rxChild->meKind == core::formula::NodeKind::ArrayConstant
                                     || rxChild->meKind == core::formula::NodeKind::BinaryOperation
                                     || rxChild->meKind == core::formula::NodeKind::FunctionCall;
            if (bMatrixLike)
            {
                const auto aMatrix = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
                if (!aMatrix.mbSupported)
                {
                    return makeUnsupportedMaterialization<std::vector<double>>(
                        aMatrix.meFallbackReason);
                }
                if (!aMatrix.moValue)
                    return makeMaterializedError<std::vector<double>>(aMatrix.meError);

                SCSIZE nColumns = 0;
                SCSIZE nRows = 0;
                (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                    {
                        const auto aValue = lookupexecution::detail::toApiCellValue(
                            (*aMatrix.moValue)->Get(nColumn, nRow));
                        if (aValue.isError())
                            return makeMaterializedError<std::vector<double>>(aValue.meError);
                        if (aValue.isEmpty())
                            continue;
                        if (aValue.isNumber())
                        {
                            aValues.push_back(aValue.mfNumber);
                            continue;
                        }
                        if (aValue.isBoolean())
                        {
                            if (bTextAsZero)
                                aValues.push_back(aValue.mfNumber);
                            continue;
                        }
                        if (aValue.isText())
                        {
                            if (bTextAsZero)
                                aValues.push_back(0.0);
                            continue;
                        }
                    }
                }
                continue;
            }

            const auto aScalar = materializeScalarNode(*rxChild, rDoc, rContext, rFormulaPos);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<std::vector<double>>(aScalar.meFallbackReason);
            if (!aScalar.moValue)
                return makeMaterializedError<std::vector<double>>(aScalar.meError);
            if (aScalar.moValue->isEmpty())
                continue;
            if (aScalar.moValue->isText() && bTextAsZero)
            {
                aValues.push_back(0.0);
                continue;
            }

            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
            if (!aNumber)
                return makeMaterializedError<std::vector<double>>(aNumber.meError);
            aValues.push_back(aNumber.maValue);
        }

        return makeMaterializedValue(std::move(aValues));
    };
    const auto collectNumericSampleValues = [&]()
        -> Materialization<std::vector<double>> {
        if (rNode.maChildren.empty())
            return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

        std::vector<double> aValues;
        for (const auto& rxChild : rNode.maChildren)
        {
            if (!rxChild)
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const bool bMatrixLike = rxChild->meKind == core::formula::NodeKind::CellReference
                                     || rxChild->meKind == core::formula::NodeKind::RangeReference
                                     || rxChild->meKind == core::formula::NodeKind::NamedReference
                                     || rxChild->meKind == core::formula::NodeKind::ArrayConstant
                                     || rxChild->meKind == core::formula::NodeKind::BinaryOperation
                                     || rxChild->meKind == core::formula::NodeKind::FunctionCall;
            if (bMatrixLike)
            {
                const auto aMatrix = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
                if (!aMatrix.mbSupported)
                {
                    return makeUnsupportedMaterialization<std::vector<double>>(
                        aMatrix.meFallbackReason);
                }
                if (!aMatrix.moValue)
                    return makeMaterializedError<std::vector<double>>(aMatrix.meError);

                SCSIZE nColumns = 0;
                SCSIZE nRows = 0;
                (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                    {
                        const auto aValue = lookupexecution::detail::toApiCellValue(
                            (*aMatrix.moValue)->Get(nColumn, nRow));
                        if (aValue.isError())
                            return makeMaterializedError<std::vector<double>>(aValue.meError);
                        if (aValue.isEmpty() || aValue.isText() || aValue.isBoolean())
                            continue;
                        const auto aNumber = coerceScalarToNumber(rDoc, rContext, aValue);
                        if (!aNumber)
                            return makeMaterializedError<std::vector<double>>(aNumber.meError);
                        aValues.push_back(aNumber.maValue);
                    }
                }
                continue;
            }

            const auto aScalar = materializeScalarNode(*rxChild, rDoc, rContext, rFormulaPos);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<std::vector<double>>(aScalar.meFallbackReason);
            if (!aScalar.moValue)
                return makeMaterializedError<std::vector<double>>(aScalar.meError);
            if (aScalar.moValue->isEmpty())
                continue;
            if (aScalar.moValue->isText())
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
            if (!aNumber)
                return makeMaterializedError<std::vector<double>>(aNumber.meError);
            aValues.push_back(aNumber.maValue);
        }

        return makeMaterializedValue(std::move(aValues));
    };
    const auto collectModeValues = [&]()
        -> Materialization<std::vector<double>> {
        if (rNode.maChildren.empty())
            return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

        std::vector<double> aValues;
        for (const auto& rxChild : rNode.maChildren)
        {
            if (!rxChild)
                return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);

            const bool bMatrixLike = rxChild->meKind == core::formula::NodeKind::CellReference
                                     || rxChild->meKind == core::formula::NodeKind::RangeReference
                                     || rxChild->meKind == core::formula::NodeKind::NamedReference
                                     || rxChild->meKind == core::formula::NodeKind::ArrayConstant
                                     || rxChild->meKind == core::formula::NodeKind::BinaryOperation
                                     || rxChild->meKind == core::formula::NodeKind::FunctionCall;
            if (bMatrixLike)
            {
                const auto aMatrix = materializeMatrixNode(*rxChild, rDoc, rContext, rFormulaPos);
                if (!aMatrix.mbSupported)
                {
                    return makeUnsupportedMaterialization<std::vector<double>>(
                        aMatrix.meFallbackReason);
                }
                if (!aMatrix.moValue)
                    return makeMaterializedError<std::vector<double>>(aMatrix.meError);

                SCSIZE nColumns = 0;
                SCSIZE nRows = 0;
                (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                    {
                        const auto aValue = lookupexecution::detail::toApiCellValue(
                            (*aMatrix.moValue)->Get(nColumn, nRow));
                        if (aValue.isError())
                            return makeMaterializedError<std::vector<double>>(aValue.meError);
                        if (aValue.isNumber())
                            aValues.push_back(aValue.mfNumber);
                    }
                }
                continue;
            }

            const auto aScalar = materializeScalarNode(*rxChild, rDoc, rContext, rFormulaPos);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<std::vector<double>>(aScalar.meFallbackReason);
            if (!aScalar.moValue)
                return makeMaterializedError<std::vector<double>>(aScalar.meError);
            if (aScalar.moValue->isNumber())
                aValues.push_back(aScalar.moValue->mfNumber);
        }

        return makeMaterializedValue(std::move(aValues));
    };

    if (aFunctionName == u"MAX" || aFunctionName == u"MAXA" || aFunctionName == u"MIN"
        || aFunctionName == u"MINA")
    {
        const bool bAForm = aFunctionName == u"MAXA" || aFunctionName == u"MINA";
        const auto aValues = collectStatisticalValues(bAForm);
        if (!aValues.mbSupported)
            return makeUnsupported(eFunction, aValues.meFallbackReason);
        if (!aValues.moValue)
            return makeErrorAttempt(aValues.meError);

        const auto aExtrema = spreadsheetengine::core::math::evaluateExtremaNumbers(
            *aValues.moValue, aFunctionName == u"MAX" || aFunctionName == u"MAXA",
            !bAForm);
        if (!aExtrema)
            return makeErrorAttempt(aExtrema.meError);
        return makeNumericAttempt(aExtrema.maValue);
    }

    if (aFunctionName == u"MEDIAN")
    {
        const auto aValues = collectStatisticalValues(false);
        if (!aValues.mbSupported)
            return makeUnsupported(eFunction, aValues.meFallbackReason);
        if (!aValues.moValue)
            return makeErrorAttempt(aValues.meError);
        if (aValues.moValue->empty())
            return makeErrorAttempt(api::Error::NoValue);

        spreadsheetengine::core::math::AggregateScan aScan;
        aScan.maNumbers = *aValues.moValue;
        const auto aMedian
            = spreadsheetengine::core::math::evaluateAggregateNumbers(12, aScan);
        if (!aMedian)
            return makeErrorAttempt(aMedian.meError);
        return makeNumericAttempt(aMedian.maValue);
    }

    if (aFunctionName == u"MODE.SNGL" || aFunctionName == u"COM.MICROSOFT.MODE.SNGL")
    {
        const auto aValues = collectModeValues();
        if (!aValues.mbSupported)
            return makeUnsupported(eFunction, aValues.meFallbackReason);
        if (!aValues.moValue)
            return makeErrorAttempt(aValues.meError);

        const auto aMode = spreadsheetengine::core::math::evaluateModeSingle(*aValues.moValue);
        if (!aMode)
            return makeErrorAttempt(aMode.meError);
        return makeNumericAttempt(aMode.maValue);
    }

    if (aFunctionName == u"AVEDEV")
    {
        const auto aValues = collectNumericSampleValues();
        if (!aValues.mbSupported)
            return makeUnsupported(eFunction, aValues.meFallbackReason);
        if (!aValues.moValue)
            return makeErrorAttempt(aValues.meError);
        if (aValues.moValue->empty())
            return makeErrorAttempt(api::Error::NoValue);

        double fSum = 0.0;
        for (double fValue : *aValues.moValue)
            fSum += fValue;
        const double fMean = fSum / static_cast<double>(aValues.moValue->size());

        double fDeviation = 0.0;
        for (double fValue : *aValues.moValue)
            fDeviation += std::abs(fValue - fMean);
        return makeNumericAttempt(fDeviation / static_cast<double>(aValues.moValue->size()));
    }

    if (aFunctionName == u"GEOMEAN" || aFunctionName == u"HARMEAN")
    {
        const auto aValues = collectStatisticalValues(false);
        if (!aValues.mbSupported)
            return makeUnsupported(eFunction, aValues.meFallbackReason);
        if (!aValues.moValue)
            return makeErrorAttempt(aValues.meError);
        if (aValues.moValue->empty())
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aMean = aFunctionName == u"GEOMEAN"
                               ? spreadsheetengine::core::math::evaluateGeometricMeanNumbers(
                                     *aValues.moValue)
                               : spreadsheetengine::core::math::evaluateHarmonicMeanNumbers(
                                     *aValues.moValue);
        if (!aMean)
            return makeErrorAttempt(aMean.meError);
        return makeNumericAttempt(aMean.maValue);
    }

    if (aFunctionName == u"VAR" || aFunctionName == u"VAR.S"
        || aFunctionName == u"COM.MICROSOFT.VAR.S" || aFunctionName == u"VARP"
        || aFunctionName == u"VAR.P" || aFunctionName == u"COM.MICROSOFT.VAR.P"
        || aFunctionName == u"VARA" || aFunctionName == u"VARPA"
        || aFunctionName == u"STDEV" || aFunctionName == u"STDEV.S"
        || aFunctionName == u"COM.MICROSOFT.STDEV.S"
        || aFunctionName == u"STDEVP" || aFunctionName == u"STDEV.P"
        || aFunctionName == u"COM.MICROSOFT.STDEV.P"
        || aFunctionName == u"STDEVA" || aFunctionName == u"STDEVPA")
    {
        const bool bTextAsZero = aFunctionName == u"VARA" || aFunctionName == u"VARPA"
                                 || aFunctionName == u"STDEVA"
                                 || aFunctionName == u"STDEVPA";
        const auto aValues = collectStatisticalValues(bTextAsZero);
        if (!aValues.mbSupported)
            return makeUnsupported(eFunction, aValues.meFallbackReason);
        if (!aValues.moValue)
            return makeErrorAttempt(aValues.meError);

        const bool bSample = aFunctionName == u"VAR" || aFunctionName == u"VAR.S"
                             || aFunctionName == u"COM.MICROSOFT.VAR.S"
                             || aFunctionName == u"VARA" || aFunctionName == u"STDEV"
                             || aFunctionName == u"STDEV.S"
                             || aFunctionName == u"COM.MICROSOFT.STDEV.S"
                             || aFunctionName == u"STDEVA";
        const bool bReturnStdDev = aFunctionName == u"STDEV" || aFunctionName == u"STDEV.S"
                                   || aFunctionName == u"COM.MICROSOFT.STDEV.S"
                                   || aFunctionName == u"STDEVP"
                                   || aFunctionName == u"STDEV.P"
                                   || aFunctionName == u"COM.MICROSOFT.STDEV.P"
                                   || aFunctionName == u"STDEVA"
                                   || aFunctionName == u"STDEVPA";
        const auto aVariance = spreadsheetengine::core::math::evaluateVarianceNumbers(
            *aValues.moValue, bSample, bReturnStdDev);
        if (!aVariance)
            return makeErrorAttempt(aVariance.meError);
        return makeNumericAttempt(aVariance.maValue);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateCriteriaAggregateFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    using CriteriaAggregateInput = spreadsheetengine::core::query::CriteriaAggregateInput;
    using CriteriaAggregateKind = spreadsheetengine::core::query::CriteriaAggregateKind;
    using CriteriaPredicate = spreadsheetengine::core::query::CriteriaPredicate;

    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    const auto eSearchType = searchTypeFromDocument(rDoc);
    const bool bMatchWholeCell = rDoc.GetDocOptions().IsMatchWholeCell();
    const CriteriaAggregateMaterializer aMaterializer(rDoc, rContext);

    auto evaluateAggregateInput = [&](const core::formula::Node& rArgument)
        -> Materialization<CriteriaAggregateInput> {
        return materializeCriteriaAggregateInput(rArgument, rDoc, rContext, rFormulaPos);
    };

    auto evaluateCriteria = [&](const core::formula::Node& rArgument)
        -> Materialization<CriteriaPredicate> {
        auto aArgument = [&]() -> Materialization<api::CellValue> {
            if (rArgument.meKind == core::formula::NodeKind::CellReference
                || rArgument.meKind == core::formula::NodeKind::RangeReference
                || rArgument.meKind == core::formula::NodeKind::NamedReference)
            {
                return materializeScalarizedReferenceValueNode(
                    rArgument, rDoc, rContext, rFormulaPos);
            }

            if (rArgument.meKind == core::formula::NodeKind::ArrayConstant
                || rArgument.meKind == core::formula::NodeKind::BinaryOperation
                || rArgument.meKind == core::formula::NodeKind::FunctionCall)
            {
                const auto aMatrix = materializeMatrixNode(rArgument, rDoc, rContext, rFormulaPos);
                if (aMatrix.mbSupported && aMatrix.moValue)
                {
                    return makeMaterializedValue(lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(0, 0)));
                }
                if (aMatrix.mbSupported && !aMatrix.moValue)
                    return makeMaterializedError<api::CellValue>(aMatrix.meError);
            }

            return materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
        }();
        if (!aArgument.mbSupported)
            return makeUnsupportedMaterialization<CriteriaPredicate>(aArgument.meFallbackReason);
        if (!aArgument.moValue)
            return makeMaterializedError<CriteriaPredicate>(aArgument.meError);

        api::CellValue aCriteriaValue = *aArgument.moValue;
        const bool bReferenceLikeArgument = rArgument.meKind == core::formula::NodeKind::CellReference
                                            || rArgument.meKind == core::formula::NodeKind::RangeReference
                                            || rArgument.meKind == core::formula::NodeKind::NamedReference;
        if (bReferenceLikeArgument && aCriteriaValue.isEmpty())
            aCriteriaValue = api::CellValue::number(0.0);

        const auto oCriteria = spreadsheetengine::core::query::makeCriteriaPredicate(
            aCriteriaValue, spreadsheetengine::core::datetime::parseStandaloneNumberText,
            spreadsheetengine::core::coercion::parseAsciiDouble);
        if (!oCriteria)
            return makeMaterializedError<CriteriaPredicate>(api::Error::IllegalArgument);
        return makeMaterializedValue(*oCriteria);
    };

    auto evaluateAggregate = [&](const std::vector<CriteriaAggregateInput>& rRanges,
                                 const std::vector<CriteriaPredicate>& rCriteria,
                                 const CriteriaAggregateInput* pTargetRange,
                                 CriteriaAggregateKind eAggregateKind) -> EvaluationAttempt {
        const auto aResult = spreadsheetengine::core::query::evaluateCriteriaAggregate(
            aMaterializer, rRanges, rCriteria, pTargetRange, eAggregateKind, eSearchType,
            bMatchWholeCell);
        if (!aResult)
            return makeErrorResult(eFunction, aResult.meError);
        return makeScalarAttempt(eFunction, aResult.maValue);
    };

    if (aFunctionName == u"COUNTIF")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aRange = evaluateAggregateInput(*rNode.maChildren[0]);
        if (!aRange.mbSupported)
            return makeUnsupported(eFunction, aRange.meFallbackReason);
        if (!aRange.moValue)
            return makeErrorResult(eFunction, aRange.meError);

        const auto aCriteria = evaluateCriteria(*rNode.maChildren[1]);
        if (!aCriteria.mbSupported)
            return makeUnsupported(eFunction, aCriteria.meFallbackReason);
        if (!aCriteria.moValue)
            return makeErrorResult(eFunction, aCriteria.meError);

        return evaluateAggregate(
            { *aRange.moValue }, { *aCriteria.moValue }, nullptr, CriteriaAggregateKind::Count);
    }

    if (aFunctionName == u"COUNTIFS")
    {
        if (rNode.maChildren.size() < 2 || (rNode.maChildren.size() % 2) != 0)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        std::vector<CriteriaAggregateInput> aRanges;
        std::vector<CriteriaPredicate> aCriteria;
        aRanges.reserve(rNode.maChildren.size() / 2);
        aCriteria.reserve(rNode.maChildren.size() / 2);
        for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); nIndex += 2)
        {
            const auto aRange = evaluateAggregateInput(*rNode.maChildren[nIndex]);
            if (!aRange.mbSupported)
                return makeUnsupported(eFunction, aRange.meFallbackReason);
            if (!aRange.moValue)
                return makeErrorResult(eFunction, aRange.meError);

            const auto aCriterion = evaluateCriteria(*rNode.maChildren[nIndex + 1]);
            if (!aCriterion.mbSupported)
                return makeUnsupported(eFunction, aCriterion.meFallbackReason);
            if (!aCriterion.moValue)
                return makeErrorResult(eFunction, aCriterion.meError);

            aRanges.push_back(*aRange.moValue);
            aCriteria.push_back(*aCriterion.moValue);
        }

        return evaluateAggregate(aRanges, aCriteria, nullptr, CriteriaAggregateKind::Count);
    }

    if (aFunctionName == u"SUMIF" || aFunctionName == u"AVERAGEIF")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aCriteriaRange = evaluateAggregateInput(*rNode.maChildren[0]);
        if (!aCriteriaRange.mbSupported)
            return makeUnsupported(eFunction, aCriteriaRange.meFallbackReason);
        if (!aCriteriaRange.moValue)
            return makeErrorResult(eFunction, aCriteriaRange.meError);

        const auto aCriteria = evaluateCriteria(*rNode.maChildren[1]);
        if (!aCriteria.mbSupported)
            return makeUnsupported(eFunction, aCriteria.meFallbackReason);
        if (!aCriteria.moValue)
            return makeErrorResult(eFunction, aCriteria.meError);

        std::optional<CriteriaAggregateInput> oTargetRange;
        if (rNode.maChildren.size() == 3)
        {
            const auto aTarget = evaluateAggregateInput(*rNode.maChildren[2]);
            if (!aTarget.mbSupported)
                return makeUnsupported(eFunction, aTarget.meFallbackReason);
            if (!aTarget.moValue)
                return makeErrorResult(eFunction, aTarget.meError);
            oTargetRange = *aTarget.moValue;
        }

        return evaluateAggregate({ *aCriteriaRange.moValue }, { *aCriteria.moValue },
            oTargetRange ? &*oTargetRange : nullptr,
            aFunctionName == u"SUMIF" ? CriteriaAggregateKind::Sum
                                      : CriteriaAggregateKind::Average);
    }

    if (rNode.maChildren.size() < 3 || (rNode.maChildren.size() % 2) == 0)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    const auto aTargetRange = evaluateAggregateInput(*rNode.maChildren[0]);
    if (!aTargetRange.mbSupported)
        return makeUnsupported(eFunction, aTargetRange.meFallbackReason);
    if (!aTargetRange.moValue)
        return makeErrorResult(eFunction, aTargetRange.meError);

    std::vector<CriteriaAggregateInput> aRanges;
    std::vector<CriteriaPredicate> aCriteria;
    aRanges.reserve((rNode.maChildren.size() - 1) / 2);
    aCriteria.reserve((rNode.maChildren.size() - 1) / 2);
    for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); nIndex += 2)
    {
        const auto aRange = evaluateAggregateInput(*rNode.maChildren[nIndex]);
        if (!aRange.mbSupported)
            return makeUnsupported(eFunction, aRange.meFallbackReason);
        if (!aRange.moValue)
            return makeErrorResult(eFunction, aRange.meError);

        const auto aCriterion = evaluateCriteria(*rNode.maChildren[nIndex + 1]);
        if (!aCriterion.mbSupported)
            return makeUnsupported(eFunction, aCriterion.meFallbackReason);
        if (!aCriterion.moValue)
            return makeErrorResult(eFunction, aCriterion.meError);

        aRanges.push_back(*aRange.moValue);
        aCriteria.push_back(*aCriterion.moValue);
    }

    CriteriaAggregateKind eAggregateKind = CriteriaAggregateKind::Sum;
    if (aFunctionName == u"AVERAGEIFS")
        eAggregateKind = CriteriaAggregateKind::Average;
    else if (aFunctionName == u"MAXIFS")
        eAggregateKind = CriteriaAggregateKind::Max;
    else if (aFunctionName == u"MINIFS")
        eAggregateKind = CriteriaAggregateKind::Min;

    return evaluateAggregate(aRanges, aCriteria, &*aTargetRange.moValue, eAggregateKind);
}

[[nodiscard]] inline EvaluationAttempt evaluateBusinessDayFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    if (!businessDayAmbientEnabled())
        return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);

    if (!isBoundedAmbientBusinessDayNode(rNode))
        return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);

    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);

    const auto makeNumericAttempt = [&](double fValue, SvNumFormatType eFormatType) {
        return makeNumericResult(eFunction, fValue, eFormatType);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    constexpr api::DateSerial kMaxBusinessDaySpan = 62;
    const bool bImportedBusinessDayRoot = isImportedCachedFormulaRoot(rDoc, rFormulaPos);
    const auto isBoundedBusinessDaySpan = [&](api::DateSerial nStart, api::DateSerial nFinish) {
        return std::llabs(static_cast<long long>(nFinish) - static_cast<long long>(nStart))
               <= kMaxBusinessDaySpan;
    };
    const auto materializeBusinessDayScalar
        = [&](const core::formula::Node& rArgument) -> Materialization<api::CellValue> {
        if (rArgument.meKind == core::formula::NodeKind::EmptyArgument)
            return makeMaterializedValue(api::CellValue::empty());

        if (rArgument.meKind == core::formula::NodeKind::FunctionCall)
        {
            const api::String aChildName = uppercaseAscii(rArgument.maPrimaryText);
            if (aChildName == u"DATE")
            {
                if (rArgument.maChildren.size() != 3)
                    return makeMaterializedError<api::CellValue>(api::Error::IllegalArgument);

                const auto materializeDatePart = [&](const core::formula::Node& rPart)
                    -> Materialization<double> {
                    const auto aScalar = materializeScalarizedReferenceValueNode(
                        rPart, rDoc, rContext, rFormulaPos);
                    if (!aScalar.mbSupported)
                        return makeUnsupportedMaterialization<double>(aScalar.meFallbackReason);
                    if (!aScalar.moValue)
                        return makeMaterializedError<double>(aScalar.meError);

                    const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
                    if (!aNumber)
                        return makeMaterializedError<double>(aNumber.meError);
                    return makeMaterializedValue(aNumber.maValue);
                };

                const auto aYear = materializeDatePart(*rArgument.maChildren[0]);
                if (!aYear.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aYear.meFallbackReason);
                if (!aYear.moValue)
                    return makeMaterializedError<api::CellValue>(aYear.meError);

                const auto aMonth = materializeDatePart(*rArgument.maChildren[1]);
                if (!aMonth.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aMonth.meFallbackReason);
                if (!aMonth.moValue)
                    return makeMaterializedError<api::CellValue>(aMonth.meError);

                const auto aDay = materializeDatePart(*rArgument.maChildren[2]);
                if (!aDay.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aDay.meFallbackReason);
                if (!aDay.moValue)
                    return makeMaterializedError<api::CellValue>(aDay.meError);

                const std::int16_t nYear = static_cast<std::int16_t>(std::trunc(*aYear.moValue));
                const std::int16_t nMonth = static_cast<std::int16_t>(std::trunc(*aMonth.moValue));
                const std::int16_t nDay = static_cast<std::int16_t>(std::trunc(*aDay.moValue));
                if (nYear < 0)
                    return makeMaterializedError<api::CellValue>(api::Error::IllegalArgument);

                const auto aDateSerial = api::calendar::makeDateSerial(
                    toApiDateParts(rDoc.GetFormatTable()->GetNullDate()), nYear, nMonth, nDay,
                    false);
                if (!aDateSerial)
                    return makeMaterializedError<api::CellValue>(api::Error::IllegalArgument);
                return makeMaterializedValue(api::CellValue::number(aDateSerial.maValue));
            }

            const FunctionKind eChildFunction = classifyFunction(aChildName);
            if (eChildFunction == FunctionKind::Value || eChildFunction == FunctionKind::DateValue
                || eChildFunction == FunctionKind::TimeValue
                || eChildFunction == FunctionKind::NumberValue
                || eChildFunction == FunctionKind::MathScalar
                || eChildFunction == FunctionKind::Round
                || eChildFunction == FunctionKind::Conversion
                || eChildFunction == FunctionKind::NumericAggregate
                || eChildFunction == FunctionKind::RankedAggregate
                || eChildFunction == FunctionKind::StatisticalAggregate
                || eChildFunction == FunctionKind::StatisticalDistribution
                || eChildFunction == FunctionKind::GrowthProjection
                || eChildFunction == FunctionKind::CriteriaAggregate
                || eChildFunction == FunctionKind::Aggregate)
            {
                auto aAttempt = evaluateFunctionNode(
                    rArgument, rDoc, rContext, rFormulaPos,
                    rDoc.GetCalcConfig().mbEmptyStringAsZero);
                if (!aAttempt.mbSupported)
                {
                    return makeUnsupportedMaterialization<api::CellValue>(
                        aAttempt.meFallbackReason);
                }

                switch (aAttempt.maResult.meType)
                {
                    case api::formulavalue::ValueType::Value:
                        return makeMaterializedValue(
                            api::CellValue::number(aAttempt.maResult.mfValue));
                    case api::formulavalue::ValueType::String:
                        return makeMaterializedValue(
                            api::CellValue::text(aAttempt.maResult.maString));
                    case api::formulavalue::ValueType::Error:
                        return makeMaterializedValue(
                            api::CellValue::error(aAttempt.maResult.meError));
                    default:
                        break;
                }
            }
        }

        return materializeScalarizedReferenceValueNode(rArgument, rDoc, rContext, rFormulaPos);
    };
    const auto coerceBusinessDayDateSerial = [&](const api::CellValue& rValue)
        -> api::ValueResult<api::DateSerial> {
        if (rValue.isError())
            return api::ValueResult<api::DateSerial>::failure(rValue.meError);
        if (rValue.isEmpty())
            return api::ValueResult<api::DateSerial>::success(0);
        if (rValue.isText())
        {
            const OUString aText = toLibreOfficeString(rValue.maString);
            const auto aParsedDate = textparsingexecution::evaluateDateValue(rDoc, rContext, aText);
            if (aParsedDate)
            {
                return api::ValueResult<api::DateSerial>::success(
                    static_cast<api::DateSerial>(std::floor(aParsedDate.maValue)));
            }
            if (const auto oStandalone = tryStandaloneParsedDateValue(aText))
            {
                return api::ValueResult<api::DateSerial>::success(
                    static_cast<api::DateSerial>(std::floor(*oStandalone)));
            }
            if (const auto oIsoFallback = tryIsoDateValueFallback(rDoc, aText))
            {
                return api::ValueResult<api::DateSerial>::success(
                    static_cast<api::DateSerial>(std::floor(*oIsoFallback)));
            }
            return api::ValueResult<api::DateSerial>::failure(api::Error::IllegalArgument);
        }
        return api::ValueResult<api::DateSerial>::success(
            static_cast<api::DateSerial>(std::floor(rValue.mfNumber)));
    };
    const auto materializeBusinessDayDateArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::DateSerial> {
        const auto aScalar = materializeBusinessDayScalar(rArgument);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<api::DateSerial>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<api::DateSerial>(aScalar.meError);

        const auto aDateSerial = coerceBusinessDayDateSerial(*aScalar.moValue);
        if (!aDateSerial)
            return makeMaterializedError<api::DateSerial>(aDateSerial.meError);
        return makeMaterializedValue(aDateSerial.maValue);
    };
    const auto materializeBusinessDayWholeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::DateSerial> {
        const auto aScalar = materializeBusinessDayScalar(rArgument);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<api::DateSerial>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<api::DateSerial>(aScalar.meError);

        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
        if (!aNumber)
            return makeMaterializedError<api::DateSerial>(aNumber.meError);
        const auto oWhole = coerceWholeNumber(std::trunc(aNumber.maValue));
        if (!oWhole)
            return makeMaterializedError<api::DateSerial>(api::Error::IllegalArgument);
        return makeMaterializedValue(static_cast<api::DateSerial>(*oWhole));
    };
    const auto evaluateWeekendMaskArgument = [&](const core::formula::Node* pArgument,
                                                 bool bWorkdayFunction,
                                                 bool bSequenceCompatible)
        -> Materialization<api::WeekendMask> {
        if (!pArgument || pArgument->meKind == core::formula::NodeKind::EmptyArgument)
            return makeMaterializedValue(api::workday::defaultWeekendMask());

        const bool bMatrixLike = pArgument->meKind == core::formula::NodeKind::CellReference
                                 || pArgument->meKind == core::formula::NodeKind::RangeReference
                                 || pArgument->meKind == core::formula::NodeKind::NamedReference
                                 || pArgument->meKind == core::formula::NodeKind::ArrayConstant
                                 || pArgument->meKind == core::formula::NodeKind::BinaryOperation;
        if (bSequenceCompatible && bMatrixLike)
        {
            const auto aMatrix = materializeMatrixNode(*pArgument, rDoc, rContext, rFormulaPos);
            if (!aMatrix.mbSupported)
                return makeUnsupportedMaterialization<api::WeekendMask>(aMatrix.meFallbackReason);
            if (!aMatrix.moValue)
                return makeMaterializedError<api::WeekendMask>(aMatrix.meError);

            std::vector<double> aWeekendSequence;
            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
            for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    const auto aValue = lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(nColumn, nRow));
                    if (aValue.isError())
                        return makeMaterializedError<api::WeekendMask>(aValue.meError);
                    if (aValue.isEmpty())
                        continue;
                    const auto aNumber = coerceScalarToNumber(rDoc, rContext, aValue);
                    if (!aNumber)
                        return makeMaterializedError<api::WeekendMask>(aNumber.meError);
                    aWeekendSequence.push_back(aNumber.maValue);
                }
            }

            if (aWeekendSequence.size() == 7)
            {
                const auto aMask = api::workday::weekendMaskFromSequence(aWeekendSequence);
                if (!aMask)
                    return makeMaterializedError<api::WeekendMask>(aMask.meError);
                return makeMaterializedValue(aMask.maValue);
            }

            if (aWeekendSequence.size() > 1)
                return makeMaterializedError<api::WeekendMask>(api::Error::IllegalArgument);
        }

        const auto aScalar = materializeBusinessDayScalar(*pArgument);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<api::WeekendMask>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<api::WeekendMask>(aScalar.meError);

        const api::CellValue& rValue = *aScalar.moValue;
        if (rValue.isError())
            return makeMaterializedError<api::WeekendMask>(rValue.meError);
        if (rValue.isEmpty())
            return makeMaterializedError<api::WeekendMask>(api::Error::IllegalArgument);

        if (rValue.isText())
        {
            if (rValue.maString.size() != 7)
                return makeMaterializedError<api::WeekendMask>(api::Error::IllegalArgument);
            const auto aMask = api::workday::weekendMaskFromMsSpec(
                rValue.maString, bWorkdayFunction);
            if (!aMask)
                return makeMaterializedError<api::WeekendMask>(aMask.meError);
            return makeMaterializedValue(aMask.maValue);
        }

        const auto aNumber = coerceScalarToNumber(rDoc, rContext, rValue);
        if (!aNumber)
            return makeMaterializedError<api::WeekendMask>(aNumber.meError);
        const auto oWholeNumber = coerceWholeNumber(aNumber.maValue);
        if (!oWholeNumber)
            return makeMaterializedError<api::WeekendMask>(api::Error::IllegalArgument);
        if ((*oWholeNumber < 1 || *oWholeNumber > 7)
            && (*oWholeNumber < 11 || *oWholeNumber > 17))
        {
            return makeMaterializedError<api::WeekendMask>(api::Error::IllegalArgument);
        }

        const api::String aSpec = toApiString(OUString::number(*oWholeNumber));
        const auto aMask = api::workday::weekendMaskFromMsSpec(aSpec, bWorkdayFunction);
        if (!aMask)
            return makeMaterializedError<api::WeekendMask>(aMask.meError);
        return makeMaterializedValue(aMask.maValue);
    };
    const auto collectHolidaySerials = [&](const core::formula::Node* pArgument)
        -> Materialization<std::vector<api::DateSerial>> {
        std::vector<api::DateSerial> aHolidays;
        if (!pArgument || pArgument->meKind == core::formula::NodeKind::EmptyArgument)
            return makeMaterializedValue(std::move(aHolidays));

        const bool bMatrixLike = pArgument->meKind == core::formula::NodeKind::CellReference
                                 || pArgument->meKind == core::formula::NodeKind::RangeReference
                                 || pArgument->meKind == core::formula::NodeKind::NamedReference
                                 || pArgument->meKind == core::formula::NodeKind::ArrayConstant
                                 || pArgument->meKind == core::formula::NodeKind::BinaryOperation
                                 || pArgument->meKind == core::formula::NodeKind::FunctionCall;
        if (bMatrixLike)
        {
            const auto aMatrix = materializeMatrixNode(*pArgument, rDoc, rContext, rFormulaPos);
            if (!aMatrix.mbSupported)
                return makeUnsupportedMaterialization<std::vector<api::DateSerial>>(
                    aMatrix.meFallbackReason);
            if (!aMatrix.moValue)
                return makeMaterializedError<std::vector<api::DateSerial>>(aMatrix.meError);

            SCSIZE nColumns = 0;
            SCSIZE nRows = 0;
            (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
            for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
            {
                for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                {
                    const auto aValue = lookupexecution::detail::toApiCellValue(
                        (*aMatrix.moValue)->Get(nColumn, nRow));
                    if (aValue.isEmpty())
                        continue;
                    const auto aDateSerial = coerceBusinessDayDateSerial(aValue);
                    if (!aDateSerial)
                        return makeMaterializedError<std::vector<api::DateSerial>>(
                            aDateSerial.meError);
                    aHolidays.push_back(aDateSerial.maValue);
                }
            }
        }
        else
        {
            const auto aScalar = materializeBusinessDayScalar(*pArgument);
            if (!aScalar.mbSupported)
            {
                return makeUnsupportedMaterialization<std::vector<api::DateSerial>>(
                    aScalar.meFallbackReason);
            }
            if (!aScalar.moValue)
                return makeMaterializedError<std::vector<api::DateSerial>>(aScalar.meError);
            if (!aScalar.moValue->isEmpty())
            {
                const auto aDateSerial = coerceBusinessDayDateSerial(*aScalar.moValue);
                if (!aDateSerial)
                    return makeMaterializedError<std::vector<api::DateSerial>>(
                        aDateSerial.meError);
                aHolidays.push_back(aDateSerial.maValue);
            }
        }

        std::sort(aHolidays.begin(), aHolidays.end());
        aHolidays.erase(std::unique(aHolidays.begin(), aHolidays.end()), aHolidays.end());
        return makeMaterializedValue(std::move(aHolidays));
    };
    const bool bWorkdayFunction = aFunctionName == u"WORKDAY"
                                  || aFunctionName == u"WORKDAY.INTL"
                                  || aFunctionName == u"COM.MICROSOFT.WORKDAY.INTL";
    const bool bIntl = aFunctionName == u"WORKDAY.INTL"
                       || aFunctionName == u"COM.MICROSOFT.WORKDAY.INTL"
                       || aFunctionName == u"NETWORKDAYS.INTL"
                       || aFunctionName == u"COM.MICROSOFT.NETWORKDAYS.INTL";

    if (bWorkdayFunction)
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aStartDate = materializeBusinessDayDateArgument(*rNode.maChildren[0]);
        if (!aStartDate.mbSupported)
            return makeUnsupported(eFunction, aStartDate.meFallbackReason);
        if (!aStartDate.moValue)
            return makeErrorAttempt(aStartDate.meError);

        const auto aDays = materializeBusinessDayWholeArgument(*rNode.maChildren[1]);
        if (!aDays.mbSupported)
            return makeUnsupported(eFunction, aDays.meFallbackReason);
        if (!aDays.moValue)
            return makeErrorAttempt(aDays.meError);
        if (!isBoundedBusinessDaySpan(*aStartDate.moValue,
                *aStartDate.moValue + static_cast<api::DateSerial>(*aDays.moValue)))
        {
            return bImportedBusinessDayRoot
                       ? makeErrorAttempt(api::Error::VariableExpected)
                       : makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
        }

        const auto aWeekendMask = bIntl
                                      ? evaluateWeekendMaskArgument(
                                            rNode.maChildren.size() >= 3
                                                ? rNode.maChildren[2].get()
                                                : nullptr,
                                            true, true)
                                      : (rNode.maChildren.size() == 4
                                             ? evaluateWeekendMaskArgument(
                                                   rNode.maChildren[3].get(), true, true)
                                             : makeMaterializedValue(
                                                   api::workday::defaultWeekendMask()));
        if (!aWeekendMask.mbSupported)
            return makeUnsupported(eFunction, aWeekendMask.meFallbackReason);
        if (!aWeekendMask.moValue)
            return makeErrorAttempt(aWeekendMask.meError);
        if (!api::workday::hasAvailableWorkday(*aWeekendMask.moValue))
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aHolidays = collectHolidaySerials(
            bIntl ? (rNode.maChildren.size() >= 4 ? rNode.maChildren[3].get() : nullptr)
                  : (rNode.maChildren.size() >= 3 ? rNode.maChildren[2].get() : nullptr));
        if (!aHolidays.mbSupported)
            return makeUnsupported(eFunction, aHolidays.meFallbackReason);
        if (!aHolidays.moValue)
            return makeErrorAttempt(aHolidays.meError);

        return makeNumericAttempt(
            static_cast<double>(api::workday::advanceWorkday(
                *aStartDate.moValue, *aDays.moValue, *aHolidays.moValue,
                *aWeekendMask.moValue)),
            SvNumFormatType::DATE);
    }

    if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
        return makeErrorAttempt(api::Error::IllegalArgument);

    const auto aStartDate = materializeBusinessDayDateArgument(*rNode.maChildren[0]);
    if (!aStartDate.mbSupported)
        return makeUnsupported(eFunction, aStartDate.meFallbackReason);
    if (!aStartDate.moValue)
        return makeErrorAttempt(aStartDate.meError);

    const auto aEndDate = materializeBusinessDayDateArgument(*rNode.maChildren[1]);
    if (!aEndDate.mbSupported)
        return makeUnsupported(eFunction, aEndDate.meFallbackReason);
    if (!aEndDate.moValue)
        return makeErrorAttempt(aEndDate.meError);
    if (!isBoundedBusinessDaySpan(*aStartDate.moValue, *aEndDate.moValue))
    {
        return bImportedBusinessDayRoot
                   ? makeErrorAttempt(api::Error::VariableExpected)
                   : makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
    }

    const auto aWeekendMask = bIntl
                                  ? evaluateWeekendMaskArgument(
                                        rNode.maChildren.size() >= 3
                                            ? rNode.maChildren[2].get()
                                            : nullptr,
                                        false, true)
                                  : (rNode.maChildren.size() == 4
                                         ? evaluateWeekendMaskArgument(
                                               rNode.maChildren[3].get(), false, true)
                                         : makeMaterializedValue(
                                               api::workday::defaultWeekendMask()));
    if (!aWeekendMask.mbSupported)
        return makeUnsupported(eFunction, aWeekendMask.meFallbackReason);
    if (!aWeekendMask.moValue)
        return makeErrorAttempt(aWeekendMask.meError);

    const auto aHolidays = collectHolidaySerials(
        bIntl ? (rNode.maChildren.size() >= 4 ? rNode.maChildren[3].get() : nullptr)
              : (rNode.maChildren.size() >= 3 ? rNode.maChildren[2].get() : nullptr));
    if (!aHolidays.mbSupported)
        return makeUnsupported(eFunction, aHolidays.meFallbackReason);
    if (!aHolidays.moValue)
        return makeErrorAttempt(aHolidays.meError);

    return makeNumericAttempt(
        static_cast<double>(api::workday::countWorkdays(
            *aStartDate.moValue, *aEndDate.moValue, *aHolidays.moValue,
            *aWeekendMask.moValue)),
        SvNumFormatType::NUMBER);
}

[[nodiscard]] inline EvaluationAttempt evaluateCalendarUtilityFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    const api::DateParts aNullDate = toApiDateParts(rDoc.GetFormatTable()->GetNullDate());

    const auto makeNumericAttempt = [&](double fValue, SvNumFormatType eFormatType) {
        return makeNumericResult(eFunction, fValue, eFormatType);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto isLeapYear = [](std::int16_t nYear) {
        return (nYear % 4 == 0 && nYear % 100 != 0) || (nYear % 400 == 0);
    };
    const auto daysInMonth = [&](std::uint16_t nMonth, std::int16_t nYear) -> double {
        static constexpr std::array<std::uint16_t, 12> aMonthDays{
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        if (nMonth < 1 || nMonth > aMonthDays.size())
            return 0.0;
        if (nMonth == 2 && isLeapYear(nYear))
            return 29.0;
        return static_cast<double>(aMonthDays[nMonth - 1]);
    };
    const auto materializeCalendarScalar
        = [&](const core::formula::Node& rArgument) -> Materialization<api::CellValue> {
        if (rArgument.meKind == core::formula::NodeKind::CellReference
            || rArgument.meKind == core::formula::NodeKind::RangeReference
            || rArgument.meKind == core::formula::NodeKind::NamedReference)
        {
            return materializeScalarizedReferenceValueNode(rArgument, rDoc, rContext, rFormulaPos);
        }

        if (rArgument.meKind == core::formula::NodeKind::FunctionCall)
        {
            const FunctionKind eChildFunction = classifyFunction(uppercaseAscii(rArgument.maPrimaryText));
            if (eChildFunction == FunctionKind::LogicalConstant
                || eChildFunction == FunctionKind::Value
                || eChildFunction == FunctionKind::DateValue
                || eChildFunction == FunctionKind::TimeValue
                || eChildFunction == FunctionKind::NumberValue
                || eChildFunction == FunctionKind::Round
                || eChildFunction == FunctionKind::CalendarUtility
                || eChildFunction == FunctionKind::MathScalar
                || eChildFunction == FunctionKind::Conversion
                || eChildFunction == FunctionKind::NumericAggregate
                || eChildFunction == FunctionKind::RankedAggregate
                || eChildFunction == FunctionKind::StatisticalAggregate
                || eChildFunction == FunctionKind::StatisticalDistribution
                || eChildFunction == FunctionKind::GrowthProjection
                || eChildFunction == FunctionKind::CriteriaAggregate
                || eChildFunction == FunctionKind::Aggregate)
            {
                auto aAttempt = evaluateFunctionNode(
                    rArgument, rDoc, rContext, rFormulaPos,
                    rDoc.GetCalcConfig().mbEmptyStringAsZero);
                if (!aAttempt.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aAttempt.meFallbackReason);

                switch (aAttempt.maResult.meType)
                {
                    case api::formulavalue::ValueType::Value:
                        return makeMaterializedValue(api::CellValue::number(aAttempt.maResult.mfValue));
                    case api::formulavalue::ValueType::String:
                        return makeMaterializedValue(api::CellValue::text(aAttempt.maResult.maString));
                    case api::formulavalue::ValueType::Error:
                        return makeMaterializedValue(api::CellValue::error(aAttempt.maResult.meError));
                    default:
                        break;
                }
            }
        }

        return materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
    };

    const auto materializeCalendarDateArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::DateSerial> {
        const auto aScalar = materializeCalendarScalar(rArgument);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<api::DateSerial>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<api::DateSerial>(aScalar.meError);

        const auto oDateSerial = spreadsheetengine::core::datetime::coerceToDateSerial(*aScalar.moValue);
        if (!oDateSerial)
            return makeMaterializedError<api::DateSerial>(api::Error::IllegalArgument);
        return makeMaterializedValue(*oDateSerial);
    };

    if (aFunctionName == u"EASTERSUNDAY" || aFunctionName == u"ORG.OPENOFFICE.EASTERSUNDAY")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aScalar = materializeCalendarScalar(*rNode.maChildren[0]);
        if (!aScalar.mbSupported)
            return makeUnsupported(eFunction, aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeErrorAttempt(aScalar.meError);
        if (aScalar.moValue->isEmpty())
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aYearNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
        if (!aYearNumber)
            return makeErrorAttempt(aYearNumber.meError);

        const auto oWholeYear = coerceWholeNumber(aYearNumber.maValue);
        if (!oWholeYear || *oWholeYear < std::numeric_limits<std::int16_t>::min()
            || *oWholeYear > std::numeric_limits<std::int16_t>::max())
        {
            return makeErrorAttempt(api::Error::IllegalArgument);
        }

        std::int16_t nYear = static_cast<std::int16_t>(*oWholeYear);
        if (nYear >= 0 && nYear < 100)
        {
            constexpr std::int16_t nTwoDigitYearStart = 1930;
            if (nYear < (nTwoDigitYearStart % 100))
                nYear = static_cast<std::int16_t>(nYear + (((nTwoDigitYearStart / 100) + 1) * 100));
            else
                nYear = static_cast<std::int16_t>(nYear + ((nTwoDigitYearStart / 100) * 100));
        }

        const auto aEaster = api::calendar::easterSundaySerial(aNullDate, nYear);
        if (!aEaster)
            return makeErrorAttempt(aEaster.meError);
        return makeNumericAttempt(aEaster.maValue, SvNumFormatType::DATE);
    }

    if (rNode.maChildren.size() != 1)
        return makeErrorAttempt(api::Error::IllegalArgument);

    const auto aDateSerial = materializeCalendarDateArgument(*rNode.maChildren[0]);
    if (!aDateSerial.mbSupported)
        return makeUnsupported(eFunction, aDateSerial.meFallbackReason);
    if (!aDateSerial.moValue)
        return makeErrorAttempt(aDateSerial.meError);

    const std::int16_t nYear = static_cast<std::int16_t>(
        std::trunc(api::calendar::yearFromSerial(aNullDate, *aDateSerial.moValue)));
    const std::uint16_t nMonth = static_cast<std::uint16_t>(
        std::trunc(api::calendar::monthFromSerial(aNullDate, *aDateSerial.moValue)));
    const bool bLeapYear = isLeapYear(nYear);

    if (aFunctionName == u"DAYSINMONTH" || aFunctionName == u"ORG.OPENOFFICE.DAYSINMONTH")
        return makeNumericAttempt(daysInMonth(nMonth, nYear), SvNumFormatType::NUMBER);

    if (aFunctionName == u"DAYSINYEAR" || aFunctionName == u"ORG.OPENOFFICE.DAYSINYEAR")
        return makeNumericAttempt(bLeapYear ? 366.0 : 365.0, SvNumFormatType::NUMBER);

    if (aFunctionName == u"ISLEAPYEAR" || aFunctionName == u"ORG.OPENOFFICE.ISLEAPYEAR")
        return makeNumericAttempt(bLeapYear ? 1.0 : 0.0, SvNumFormatType::LOGICAL);

    if (aFunctionName == u"ISOWEEKNUM")
    {
        return makeNumericAttempt(
            static_cast<double>(api::calendar::isoWeekOfYear(aNullDate, *aDateSerial.moValue)),
            SvNumFormatType::NUMBER);
    }

    if (aFunctionName == u"WEEKSINYEAR" || aFunctionName == u"ORG.OPENOFFICE.WEEKSINYEAR")
    {
        const auto aJan1 = api::calendar::makeDateSerial(aNullDate, nYear, 1, 1, true);
        if (!aJan1)
            return makeErrorAttempt(aJan1.meError);

        const auto aJan1Weekday = api::calendar::dayOfWeek(
            aNullDate, static_cast<api::DateSerial>(std::trunc(aJan1.maValue)), 2);
        if (!aJan1Weekday)
            return makeErrorAttempt(aJan1Weekday.meError);

        const double fWeeksInYear
            = aJan1Weekday.maValue == 4 ? 53.0
              : (aJan1Weekday.maValue == 3 && bLeapYear ? 53.0 : 52.0);
        return makeNumericAttempt(fWeeksInYear, SvNumFormatType::NUMBER);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateDateDifferenceFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    const api::DateParts aNullDate = toApiDateParts(rDoc.GetFormatTable()->GetNullDate());

    const auto makeNumericAttempt = [&](double fValue) {
        return makeNumericResult(eFunction, fValue, SvNumFormatType::NUMBER);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto materializeDateDifferenceScalar
        = [&](const core::formula::Node& rArgument) -> Materialization<api::CellValue> {
        if (rArgument.meKind == core::formula::NodeKind::CellReference
            || rArgument.meKind == core::formula::NodeKind::RangeReference
            || rArgument.meKind == core::formula::NodeKind::NamedReference)
        {
            return materializeScalarizedReferenceValueNode(rArgument, rDoc, rContext, rFormulaPos);
        }

        if (rArgument.meKind == core::formula::NodeKind::FunctionCall)
        {
            const api::String aChildName = uppercaseAscii(rArgument.maPrimaryText);
            if (aChildName == u"DATE")
            {
                if (rArgument.maChildren.size() != 3)
                    return makeMaterializedError<api::CellValue>(api::Error::IllegalArgument);

                const auto materializeDatePart = [&](const core::formula::Node& rPart)
                    -> Materialization<double> {
                    const auto aScalar = materializeScalarizedReferenceValueNode(
                        rPart, rDoc, rContext, rFormulaPos);
                    if (!aScalar.mbSupported)
                        return makeUnsupportedMaterialization<double>(aScalar.meFallbackReason);
                    if (!aScalar.moValue)
                        return makeMaterializedError<double>(aScalar.meError);

                    const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
                    if (!aNumber)
                        return makeMaterializedError<double>(aNumber.meError);
                    return makeMaterializedValue(aNumber.maValue);
                };

                const auto aYear = materializeDatePart(*rArgument.maChildren[0]);
                if (!aYear.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aYear.meFallbackReason);
                if (!aYear.moValue)
                    return makeMaterializedError<api::CellValue>(aYear.meError);

                const auto aMonth = materializeDatePart(*rArgument.maChildren[1]);
                if (!aMonth.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aMonth.meFallbackReason);
                if (!aMonth.moValue)
                    return makeMaterializedError<api::CellValue>(aMonth.meError);

                const auto aDay = materializeDatePart(*rArgument.maChildren[2]);
                if (!aDay.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aDay.meFallbackReason);
                if (!aDay.moValue)
                    return makeMaterializedError<api::CellValue>(aDay.meError);

                const std::int16_t nYear = static_cast<std::int16_t>(std::trunc(*aYear.moValue));
                const std::int16_t nMonth = static_cast<std::int16_t>(std::trunc(*aMonth.moValue));
                const std::int16_t nDay = static_cast<std::int16_t>(std::trunc(*aDay.moValue));
                if (nYear < 0)
                    return makeMaterializedError<api::CellValue>(api::Error::IllegalArgument);

                const auto aDateSerial
                    = api::calendar::makeDateSerial(aNullDate, nYear, nMonth, nDay, false);
                if (!aDateSerial)
                    return makeMaterializedError<api::CellValue>(api::Error::IllegalArgument);
                return makeMaterializedValue(api::CellValue::number(aDateSerial.maValue));
            }

            const FunctionKind eChildFunction = classifyFunction(aChildName);
            if (eChildFunction == FunctionKind::LogicalConstant
                || eChildFunction == FunctionKind::Value
                || eChildFunction == FunctionKind::DateValue
                || eChildFunction == FunctionKind::TimeValue
                || eChildFunction == FunctionKind::NumberValue
                || eChildFunction == FunctionKind::Round
                || eChildFunction == FunctionKind::CalendarUtility
                || eChildFunction == FunctionKind::DateDifference
                || eChildFunction == FunctionKind::MathScalar
                || eChildFunction == FunctionKind::Conversion
                || eChildFunction == FunctionKind::NumericAggregate
                || eChildFunction == FunctionKind::RankedAggregate
                || eChildFunction == FunctionKind::StatisticalAggregate
                || eChildFunction == FunctionKind::StatisticalDistribution
                || eChildFunction == FunctionKind::GrowthProjection
                || eChildFunction == FunctionKind::CriteriaAggregate
                || eChildFunction == FunctionKind::Aggregate)
            {
                auto aAttempt = evaluateFunctionNode(
                    rArgument, rDoc, rContext, rFormulaPos,
                    rDoc.GetCalcConfig().mbEmptyStringAsZero);
                if (!aAttempt.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aAttempt.meFallbackReason);

                switch (aAttempt.maResult.meType)
                {
                    case api::formulavalue::ValueType::Value:
                        return makeMaterializedValue(api::CellValue::number(aAttempt.maResult.mfValue));
                    case api::formulavalue::ValueType::String:
                        return makeMaterializedValue(api::CellValue::text(aAttempt.maResult.maString));
                    case api::formulavalue::ValueType::Error:
                        return makeMaterializedValue(api::CellValue::error(aAttempt.maResult.meError));
                    default:
                        break;
                }
            }
        }

        return materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
    };
    const auto materializeDateDifferenceDateArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::DateSerial> {
        const auto aScalar = materializeDateDifferenceScalar(rArgument);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<api::DateSerial>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<api::DateSerial>(aScalar.meError);

        const auto oDateSerial = spreadsheetengine::core::datetime::coerceToDateSerial(*aScalar.moValue);
        if (!oDateSerial)
            return makeMaterializedError<api::DateSerial>(api::Error::IllegalArgument);
        return makeMaterializedValue(*oDateSerial);
    };
    const auto materializeDateDifferenceModeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<sal_Int32> {
        const auto aScalar = materializeDateDifferenceScalar(rArgument);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<sal_Int32>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<sal_Int32>(aScalar.meError);
        if (aScalar.moValue->isEmpty())
            return makeMaterializedError<sal_Int32>(api::Error::IllegalArgument);

        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
        if (!aNumber)
            return makeMaterializedError<sal_Int32>(aNumber.meError);

        const auto oWholeNumber = coerceWholeNumber(aNumber.maValue);
        if (!oWholeNumber)
            return makeMaterializedError<sal_Int32>(api::Error::IllegalArgument);
        return makeMaterializedValue(*oWholeNumber);
    };

    if (rNode.maChildren.size() != 3)
        return makeErrorAttempt(api::Error::IllegalArgument);

    const auto aStartDate = materializeDateDifferenceDateArgument(*rNode.maChildren[0]);
    if (!aStartDate.mbSupported)
        return makeUnsupported(eFunction, aStartDate.meFallbackReason);
    if (!aStartDate.moValue)
        return makeErrorAttempt(aStartDate.meError);

    const auto aEndDate = materializeDateDifferenceDateArgument(*rNode.maChildren[1]);
    if (!aEndDate.mbSupported)
        return makeUnsupported(eFunction, aEndDate.meFallbackReason);
    if (!aEndDate.moValue)
        return makeErrorAttempt(aEndDate.meError);

    if (aFunctionName == u"DATEDIF")
    {
        const auto aInterval = materializeDateDifferenceScalar(*rNode.maChildren[2]);
        if (!aInterval.mbSupported)
            return makeUnsupported(eFunction, aInterval.meFallbackReason);
        if (!aInterval.moValue)
            return makeErrorAttempt(aInterval.meError);
        if (aInterval.moValue->isEmpty())
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aIntervalText = coerceScalarToText(rDoc, rContext, *aInterval.moValue);
        if (!aIntervalText)
            return makeErrorAttempt(aIntervalText.meError);

        const auto aDateDif = api::calendar::dateDif(
            aNullDate, *aStartDate.moValue, *aEndDate.moValue, aIntervalText.maValue);
        if (!aDateDif)
            return makeErrorAttempt(aDateDif.meError);
        return makeNumericAttempt(aDateDif.maValue);
    }

    const auto aMode = materializeDateDifferenceModeArgument(*rNode.maChildren[2]);
    if (!aMode.mbSupported)
        return makeUnsupported(eFunction, aMode.meFallbackReason);
    if (!aMode.moValue)
        return makeErrorAttempt(aMode.meError);

    if (aFunctionName == u"MONTHS" || aFunctionName == u"ORG.OPENOFFICE.MONTHS")
    {
        if (*aMode.moValue != 0 && *aMode.moValue != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const std::int32_t nStartYear = static_cast<std::int32_t>(
            std::trunc(api::calendar::yearFromSerial(aNullDate, *aStartDate.moValue)));
        const std::int32_t nEndYear = static_cast<std::int32_t>(
            std::trunc(api::calendar::yearFromSerial(aNullDate, *aEndDate.moValue)));
        const std::int32_t nStartMonth = static_cast<std::int32_t>(
            std::trunc(api::calendar::monthFromSerial(aNullDate, *aStartDate.moValue)));
        const std::int32_t nEndMonth = static_cast<std::int32_t>(
            std::trunc(api::calendar::monthFromSerial(aNullDate, *aEndDate.moValue)));
        const auto aStartDay = api::calendar::dayFromSerial(aNullDate, *aStartDate.moValue);
        if (!aStartDay)
            return makeErrorAttempt(aStartDay.meError);
        const auto aEndDay = api::calendar::dayFromSerial(aNullDate, *aEndDate.moValue);
        if (!aEndDay)
            return makeErrorAttempt(aEndDay.meError);

        std::int32_t nMonths = nEndMonth - nStartMonth + (nEndYear - nStartYear) * 12;
        if (*aMode.moValue == 0 && *aStartDate.moValue != *aEndDate.moValue)
        {
            if (*aStartDate.moValue < *aEndDate.moValue)
            {
                if (aStartDay.maValue > aEndDay.maValue)
                    --nMonths;
            }
            else if (aStartDay.maValue < aEndDay.maValue)
            {
                ++nMonths;
            }
        }

        return makeNumericAttempt(static_cast<double>(nMonths));
    }

    if (aFunctionName == u"YEARS" || aFunctionName == u"ORG.OPENOFFICE.YEARS")
    {
        if (*aMode.moValue != 0 && *aMode.moValue != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const std::int32_t nStartYear = static_cast<std::int32_t>(
            std::trunc(api::calendar::yearFromSerial(aNullDate, *aStartDate.moValue)));
        const std::int32_t nEndYear = static_cast<std::int32_t>(
            std::trunc(api::calendar::yearFromSerial(aNullDate, *aEndDate.moValue)));
        const std::int32_t nStartMonth = static_cast<std::int32_t>(
            std::trunc(api::calendar::monthFromSerial(aNullDate, *aStartDate.moValue)));
        const std::int32_t nEndMonth = static_cast<std::int32_t>(
            std::trunc(api::calendar::monthFromSerial(aNullDate, *aEndDate.moValue)));
        const auto aStartDay = api::calendar::dayFromSerial(aNullDate, *aStartDate.moValue);
        if (!aStartDay)
            return makeErrorAttempt(aStartDay.meError);
        const auto aEndDay = api::calendar::dayFromSerial(aNullDate, *aEndDate.moValue);
        if (!aEndDay)
            return makeErrorAttempt(aEndDay.meError);

        std::int32_t nYears = nEndYear - nStartYear;
        if (*aMode.moValue == 0)
        {
            std::int32_t nMonths = nEndMonth - nStartMonth + nYears * 12;
            if (*aStartDate.moValue < *aEndDate.moValue)
            {
                if (aStartDay.maValue > aEndDay.maValue)
                    --nMonths;
            }
            else if (*aStartDate.moValue > *aEndDate.moValue)
            {
                if (aStartDay.maValue < aEndDay.maValue)
                    ++nMonths;
            }
            nYears = nMonths / 12;
        }

        return makeNumericAttempt(static_cast<double>(nYears));
    }

    if (aFunctionName == u"WEEKS" || aFunctionName == u"ORG.OPENOFFICE.WEEKS")
    {
        const auto oWeeks = spreadsheetengine::core::datetime::computeWeeksDifference(
            *aStartDate.moValue, *aEndDate.moValue, static_cast<std::int16_t>(*aMode.moValue));
        if (!oWeeks)
            return makeErrorAttempt(api::Error::IllegalArgument);
        return makeNumericAttempt(*oWeeks);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateDateConstructExtractFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    const api::DateParts aNullDate = toApiDateParts(rDoc.GetFormatTable()->GetNullDate());

    const auto makeNumericAttempt = [&](double fValue, SvNumFormatType eFormatType) {
        return makeNumericResult(eFunction, fValue, eFormatType);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto materializeDateConstructScalar
        = [&](const core::formula::Node& rArgument) -> Materialization<api::CellValue> {
        if (rArgument.meKind == core::formula::NodeKind::CellReference
            || rArgument.meKind == core::formula::NodeKind::RangeReference
            || rArgument.meKind == core::formula::NodeKind::NamedReference)
        {
            return materializeScalarizedReferenceValueNode(rArgument, rDoc, rContext, rFormulaPos);
        }

        if (rArgument.meKind == core::formula::NodeKind::FunctionCall)
        {
            const FunctionKind eChildFunction = classifyFunction(uppercaseAscii(rArgument.maPrimaryText));
            if (eChildFunction == FunctionKind::LogicalConstant
                || eChildFunction == FunctionKind::Value
                || eChildFunction == FunctionKind::DateValue
                || eChildFunction == FunctionKind::TimeValue
                || eChildFunction == FunctionKind::NumberValue
                || eChildFunction == FunctionKind::Round
                || eChildFunction == FunctionKind::CalendarUtility
                || eChildFunction == FunctionKind::DateDifference
                || eChildFunction == FunctionKind::DateConstructExtract
                || eChildFunction == FunctionKind::MathScalar
                || eChildFunction == FunctionKind::Conversion
                || eChildFunction == FunctionKind::NumericAggregate
                || eChildFunction == FunctionKind::RankedAggregate
                || eChildFunction == FunctionKind::StatisticalAggregate
                || eChildFunction == FunctionKind::StatisticalDistribution
                || eChildFunction == FunctionKind::GrowthProjection
                || eChildFunction == FunctionKind::CriteriaAggregate
                || eChildFunction == FunctionKind::Aggregate)
            {
                auto aAttempt = evaluateFunctionNode(
                    rArgument, rDoc, rContext, rFormulaPos,
                    rDoc.GetCalcConfig().mbEmptyStringAsZero);
                if (!aAttempt.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(aAttempt.meFallbackReason);

                switch (aAttempt.maResult.meType)
                {
                    case api::formulavalue::ValueType::Value:
                        return makeMaterializedValue(api::CellValue::number(aAttempt.maResult.mfValue));
                    case api::formulavalue::ValueType::String:
                        return makeMaterializedValue(api::CellValue::text(aAttempt.maResult.maString));
                    case api::formulavalue::ValueType::Error:
                        return makeMaterializedValue(api::CellValue::error(aAttempt.maResult.meError));
                    default:
                        break;
                }
            }
        }

        return materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
    };

    if (aFunctionName == u"DATE")
    {
        if (rNode.maChildren.size() != 3)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto materializeDatePart = [&](const core::formula::Node& rPart)
            -> Materialization<double> {
            const auto aScalar = materializeDateConstructScalar(rPart);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<double>(aScalar.meFallbackReason);
            if (!aScalar.moValue)
                return makeMaterializedError<double>(aScalar.meError);
            if (aScalar.moValue->isEmpty())
                return makeMaterializedError<double>(api::Error::IllegalArgument);

            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
            if (!aNumber)
                return makeMaterializedError<double>(aNumber.meError);
            return makeMaterializedValue(aNumber.maValue);
        };

        const auto aYear = materializeDatePart(*rNode.maChildren[0]);
        if (!aYear.mbSupported)
            return makeUnsupported(eFunction, aYear.meFallbackReason);
        if (!aYear.moValue)
            return makeErrorAttempt(aYear.meError);

        const auto aMonth = materializeDatePart(*rNode.maChildren[1]);
        if (!aMonth.mbSupported)
            return makeUnsupported(eFunction, aMonth.meFallbackReason);
        if (!aMonth.moValue)
            return makeErrorAttempt(aMonth.meError);

        const auto aDay = materializeDatePart(*rNode.maChildren[2]);
        if (!aDay.mbSupported)
            return makeUnsupported(eFunction, aDay.meFallbackReason);
        if (!aDay.moValue)
            return makeErrorAttempt(aDay.meError);

        const std::int16_t nYear = static_cast<std::int16_t>(std::trunc(*aYear.moValue));
        const std::int16_t nMonth = static_cast<std::int16_t>(std::trunc(*aMonth.moValue));
        const std::int16_t nDay = static_cast<std::int16_t>(std::trunc(*aDay.moValue));
        if (nYear < 0)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aDateSerial
            = api::calendar::makeDateSerial(aNullDate, nYear, nMonth, nDay, false);
        if (!aDateSerial)
            return makeErrorAttempt(api::Error::IllegalArgument);
        return makeNumericAttempt(aDateSerial.maValue, SvNumFormatType::DATE);
    }

    if (rNode.maChildren.size() != 1)
        return makeErrorAttempt(api::Error::IllegalArgument);

    const auto aScalar = materializeDateConstructScalar(*rNode.maChildren[0]);
    if (!aScalar.mbSupported)
        return makeUnsupported(eFunction, aScalar.meFallbackReason);
    if (!aScalar.moValue)
        return makeErrorAttempt(aScalar.meError);

    const auto oDateSerial = spreadsheetengine::core::datetime::coerceToDateSerial(*aScalar.moValue);
    if (!oDateSerial)
        return makeErrorAttempt(api::Error::IllegalArgument);

    if (aFunctionName == u"YEAR")
    {
        return makeNumericAttempt(api::calendar::yearFromSerial(aNullDate, *oDateSerial),
            SvNumFormatType::NUMBER);
    }
    if (aFunctionName == u"MONTH")
    {
        return makeNumericAttempt(api::calendar::monthFromSerial(aNullDate, *oDateSerial),
            SvNumFormatType::NUMBER);
    }

    const auto aDay = api::calendar::dayFromSerial(aNullDate, *oDateSerial);
    if (!aDay)
        return makeErrorAttempt(aDay.meError);
    return makeNumericAttempt(aDay.maValue, SvNumFormatType::NUMBER);
}

[[nodiscard]] inline EvaluationAttempt evaluateStatisticalDistributionFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    bool bImportedCanonicalSource)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    auto materializeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        const auto aAttempt = evaluateScalarOrDelegatedNode(
            rArgument, eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1,
            bImportedCanonicalSource);
        if (!aAttempt.mbSupported)
            return makeUnsupportedMaterialization<api::CellValue>(aAttempt.meFallbackReason);
        switch (aAttempt.maResult.meType)
        {
            case api::formulavalue::ValueType::Error:
                return makeMaterializedValue(api::CellValue::error(aAttempt.maResult.meError));
            case api::formulavalue::ValueType::String:
                return makeMaterializedValue(api::CellValue::text(aAttempt.maResult.maString));
            case api::formulavalue::ValueType::Value:
                if (aAttempt.meFormatType == SvNumFormatType::LOGICAL)
                    return makeMaterializedValue(
                        api::CellValue::boolean(aAttempt.maResult.mfValue != 0.0));
                return makeMaterializedValue(api::CellValue::number(aAttempt.maResult.mfValue));
            case api::formulavalue::ValueType::Invalid:
                return makeMaterializedValue(api::CellValue::empty());
        }
        return makeMaterializedValue(api::CellValue::empty());
    };
    auto materializeNumber = [&](const core::formula::Node& rArgument)
        -> Materialization<double> {
        const auto aValue = materializeArgument(rArgument);
        if (!aValue.mbSupported)
            return makeUnsupportedMaterialization<double>(aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeMaterializedError<double>(aValue.meError);
        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
        if (!aNumber)
            return makeMaterializedError<double>(aNumber.meError);
        return makeMaterializedValue(aNumber.maValue);
    };
    auto materializeBool = [&](const core::formula::Node& rArgument)
        -> Materialization<bool> {
        const auto aValue = materializeArgument(rArgument);
        if (!aValue.mbSupported)
            return makeUnsupportedMaterialization<bool>(aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeMaterializedError<bool>(aValue.meError);
        const auto aBool = coerceScalarToBool(rDoc, rContext, *aValue.moValue);
        if (!aBool)
            return makeMaterializedError<bool>(aBool.meError);
        return makeMaterializedValue(aBool.maValue);
    };
    const auto makeNumericAttempt = [&](double fValue) {
        return makeNumericResult(eFunction, fValue, SvNumFormatType::NUMBER);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto makeUnsupportedAttempt = [&](FallbackReason eReason) {
        return makeUnsupported(eFunction, eReason);
    };
    const auto finishCalcMathAttempt = [&](const api::ValueResult<double>& rResult) {
        if (!rResult)
        {
            return makeErrorAttempt(
                rResult.meError == api::Error::Domain ? api::Error::IllegalArgument
                                                      : rResult.meError);
        }
        return makeNumericAttempt(rResult.maValue);
    };
    const auto materializeOneDimensionalValueSequence =
        [&](const core::formula::Node& rArgument) -> Materialization<std::vector<api::CellValue>> {
        const auto aMatrix = materializeMatrixNode(rArgument, rDoc, rContext, rFormulaPos);
        if (!aMatrix.mbSupported)
        {
            return makeUnsupportedMaterialization<std::vector<api::CellValue>>(
                aMatrix.meFallbackReason);
        }
        if (!aMatrix.moValue)
            return makeMaterializedError<std::vector<api::CellValue>>(aMatrix.meError);

        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
        if (nColumns == 0 || nRows == 0)
            return makeMaterializedError<std::vector<api::CellValue>>(api::Error::IllegalArgument);
        if (nColumns != 1 && nRows != 1)
        {
            return makeUnsupportedMaterialization<std::vector<api::CellValue>>(
                FallbackReason::UnsupportedFormulaShape);
        }

        std::vector<api::CellValue> aValues;
        aValues.reserve(nColumns * nRows);
        for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                aValues.push_back(
                    lookupexecution::detail::toApiCellValue((*aMatrix.moValue)->Get(nColumn, nRow)));
        }
        return makeMaterializedValue(std::move(aValues));
    };

    struct RegressionStats
    {
        double fCount = 0.0;
        double fMeanX = 0.0;
        double fMeanY = 0.0;
        double fSumDeltaXDeltaY = 0.0;
        double fSumSqrDeltaX = 0.0;
    };

    const auto collectRegressionStats =
        [&](const core::formula::Node& rKnownY, const core::formula::Node& rKnownX)
        -> Materialization<RegressionStats> {
        const auto aKnownYValues = materializeOneDimensionalValueSequence(rKnownY);
        if (!aKnownYValues.mbSupported)
            return makeUnsupportedMaterialization<RegressionStats>(aKnownYValues.meFallbackReason);
        if (!aKnownYValues.moValue)
            return makeMaterializedError<RegressionStats>(aKnownYValues.meError);

        const auto aKnownXValues = materializeOneDimensionalValueSequence(rKnownX);
        if (!aKnownXValues.mbSupported)
            return makeUnsupportedMaterialization<RegressionStats>(aKnownXValues.meFallbackReason);
        if (!aKnownXValues.moValue)
            return makeMaterializedError<RegressionStats>(aKnownXValues.meError);

        if (aKnownYValues.moValue->size() != aKnownXValues.moValue->size())
            return makeMaterializedError<RegressionStats>(api::Error::IllegalArgument);

        RegressionStats aStats;
        double fSumX = 0.0;
        double fSumY = 0.0;
        for (std::size_t nIndex = 0; nIndex < aKnownYValues.moValue->size(); ++nIndex)
        {
            const api::CellValue& rY = (*aKnownYValues.moValue)[nIndex];
            const api::CellValue& rX = (*aKnownXValues.moValue)[nIndex];
            if (rY.isError())
                return makeMaterializedError<RegressionStats>(rY.meError);
            if (rX.isError())
                return makeMaterializedError<RegressionStats>(rX.meError);
            if ((rY.isText() || rY.isEmpty()) || (rX.isText() || rX.isEmpty()))
                continue;

            const auto aY = coerceScalarToNumber(rDoc, rContext, rY);
            if (!aY)
                return makeMaterializedError<RegressionStats>(aY.meError);
            const auto aX = coerceScalarToNumber(rDoc, rContext, rX);
            if (!aX)
                return makeMaterializedError<RegressionStats>(aX.meError);

            fSumX += aX.maValue;
            fSumY += aY.maValue;
            aStats.fCount += 1.0;
        }

        if (aStats.fCount < 1.0)
            return makeMaterializedError<RegressionStats>(api::Error::NoValue);

        aStats.fMeanX = fSumX / aStats.fCount;
        aStats.fMeanY = fSumY / aStats.fCount;

        for (std::size_t nIndex = 0; nIndex < aKnownYValues.moValue->size(); ++nIndex)
        {
            const api::CellValue& rY = (*aKnownYValues.moValue)[nIndex];
            const api::CellValue& rX = (*aKnownXValues.moValue)[nIndex];
            if ((rY.isText() || rY.isEmpty()) || (rX.isText() || rX.isEmpty()))
                continue;

            const auto aY = coerceScalarToNumber(rDoc, rContext, rY);
            if (!aY)
                return makeMaterializedError<RegressionStats>(aY.meError);
            const auto aX = coerceScalarToNumber(rDoc, rContext, rX);
            if (!aX)
                return makeMaterializedError<RegressionStats>(aX.meError);

            const double fDeltaX = aX.maValue - aStats.fMeanX;
            const double fDeltaY = aY.maValue - aStats.fMeanY;
            aStats.fSumDeltaXDeltaY += fDeltaX * fDeltaY;
            aStats.fSumSqrDeltaX += fDeltaX * fDeltaX;
        }

        return makeMaterializedValue(aStats);
    };

    if (aFunctionName == u"GAUSS")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumber = materializeNumber(*rNode.maChildren[0]);
        if (!aNumber.mbSupported)
            return makeUnsupportedAttempt(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeErrorAttempt(aNumber.meError);
        return makeNumericAttempt(spreadsheetengine::core::math::gaussValue(*aNumber.moValue));
    }

    if (aFunctionName == u"PHI")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumber = materializeNumber(*rNode.maChildren[0]);
        if (!aNumber.mbSupported)
            return makeUnsupportedAttempt(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeErrorAttempt(aNumber.meError);
        const auto aPhi = spreadsheetengine::core::math::evaluateNormalDistribution(
            *aNumber.moValue, 0.0, 1.0, false);
        if (!aPhi)
            return makeErrorAttempt(aPhi.meError);
        return makeNumericAttempt(aPhi.maValue);
    }

    if (aFunctionName == u"GAMMALN" || aFunctionName == u"GAMMALN.PRECISE"
        || aFunctionName == u"COM.MICROSOFT.GAMMALN.PRECISE")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumber = materializeNumber(*rNode.maChildren[0]);
        if (!aNumber.mbSupported)
            return makeUnsupportedAttempt(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeErrorAttempt(aNumber.meError);
        return finishCalcMathAttempt(
            spreadsheetengine::core::math::evaluateLogGammaValue(*aNumber.moValue));
    }

    if (aFunctionName == u"ERF" || aFunctionName == u"ERF.PRECISE"
        || aFunctionName == u"COM.MICROSOFT.ERF.PRECISE")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumber = materializeNumber(*rNode.maChildren[0]);
        if (!aNumber.mbSupported)
            return makeUnsupportedAttempt(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeErrorAttempt(aNumber.meError);
        return finishCalcMathAttempt(
            spreadsheetengine::core::math::evaluateErrorFunction(*aNumber.moValue));
    }

    if (aFunctionName == u"ERFC" || aFunctionName == u"ERFC.PRECISE"
        || aFunctionName == u"COM.MICROSOFT.ERFC.PRECISE")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumber = materializeNumber(*rNode.maChildren[0]);
        if (!aNumber.mbSupported)
            return makeUnsupportedAttempt(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeErrorAttempt(aNumber.meError);
        return finishCalcMathAttempt(
            spreadsheetengine::core::math::evaluateComplementaryErrorFunction(*aNumber.moValue));
    }

    if (aFunctionName == u"NORMDIST" || aFunctionName == u"NORM.DIST")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aX = materializeNumber(*rNode.maChildren[0]);
        if (!aX.mbSupported)
            return makeUnsupportedAttempt(aX.meFallbackReason);
        if (!aX.moValue)
            return makeErrorAttempt(aX.meError);

        const auto aMean = materializeNumber(*rNode.maChildren[1]);
        if (!aMean.mbSupported)
            return makeUnsupportedAttempt(aMean.meFallbackReason);
        if (!aMean.moValue)
            return makeErrorAttempt(aMean.meError);

        const auto aSigma = materializeNumber(*rNode.maChildren[2]);
        if (!aSigma.mbSupported)
            return makeUnsupportedAttempt(aSigma.meFallbackReason);
        if (!aSigma.moValue)
            return makeErrorAttempt(aSigma.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            const auto aCumulative = materializeBool(*rNode.maChildren[3]);
            if (!aCumulative.mbSupported)
                return makeUnsupportedAttempt(aCumulative.meFallbackReason);
            if (!aCumulative.moValue)
                return makeErrorAttempt(aCumulative.meError);
            bCumulative = *aCumulative.moValue;
        }

        const auto aDistribution = spreadsheetengine::core::math::evaluateNormalDistribution(
            *aX.moValue, *aMean.moValue, *aSigma.moValue, bCumulative);
        if (!aDistribution)
            return makeErrorAttempt(aDistribution.meError);
        return makeNumericAttempt(aDistribution.maValue);
    }

    if (aFunctionName == u"LOGNORMDIST" || aFunctionName == u"LOGNORM.DIST"
        || aFunctionName == u"COM.MICROSOFT.LOGNORM.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName != u"LOGNORMDIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 4)
            || (!bMicrosoftSyntax
                && (rNode.maChildren.empty() || rNode.maChildren.size() > 4)))
        {
            return makeErrorAttempt(api::Error::IllegalArgument);
        }

        const auto aX = materializeNumber(*rNode.maChildren[0]);
        if (!aX.mbSupported)
            return makeUnsupportedAttempt(aX.meFallbackReason);
        if (!aX.moValue)
            return makeErrorAttempt(aX.meError);

        double fMean = 0.0;
        if (rNode.maChildren.size() >= 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMean = materializeNumber(*rNode.maChildren[1]);
            if (!aMean.mbSupported)
                return makeUnsupportedAttempt(aMean.meFallbackReason);
            if (!aMean.moValue)
                return makeErrorAttempt(aMean.meError);
            fMean = *aMean.moValue;
        }

        double fSigma = 1.0;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aSigma = materializeNumber(*rNode.maChildren[2]);
            if (!aSigma.mbSupported)
                return makeUnsupportedAttempt(aSigma.meFallbackReason);
            if (!aSigma.moValue)
                return makeErrorAttempt(aSigma.meError);
            fSigma = *aSigma.moValue;
        }

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            const auto aCumulative = materializeBool(*rNode.maChildren[3]);
            if (!aCumulative.mbSupported)
                return makeUnsupportedAttempt(aCumulative.meFallbackReason);
            if (!aCumulative.moValue)
                return makeErrorAttempt(aCumulative.meError);
            bCumulative = *aCumulative.moValue;
        }

        const auto aDistribution = spreadsheetengine::core::math::evaluateLogNormalDistribution(
            *aX.moValue, fMean, fSigma, bCumulative);
        if (!aDistribution)
            return makeErrorAttempt(aDistribution.meError);
        return makeNumericAttempt(aDistribution.maValue);
    }

    if (aFunctionName == u"LOGINV" || aFunctionName == u"LOGNORM.INV"
        || aFunctionName == u"COM.MICROSOFT.LOGNORM.INV")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aProbability = materializeNumber(*rNode.maChildren[0]);
        if (!aProbability.mbSupported)
            return makeUnsupportedAttempt(aProbability.meFallbackReason);
        if (!aProbability.moValue)
            return makeErrorAttempt(aProbability.meError);

        double fMean = 0.0;
        if (rNode.maChildren.size() >= 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMean = materializeNumber(*rNode.maChildren[1]);
            if (!aMean.mbSupported)
                return makeUnsupportedAttempt(aMean.meFallbackReason);
            if (!aMean.moValue)
                return makeErrorAttempt(aMean.meError);
            fMean = *aMean.moValue;
        }

        double fSigma = 1.0;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aSigma = materializeNumber(*rNode.maChildren[2]);
            if (!aSigma.mbSupported)
                return makeUnsupportedAttempt(aSigma.meFallbackReason);
            if (!aSigma.moValue)
                return makeErrorAttempt(aSigma.meError);
            fSigma = *aSigma.moValue;
        }

        const auto aInverse = spreadsheetengine::core::math::evaluateLogNormalInverse(
            *aProbability.moValue, fMean, fSigma);
        if (!aInverse)
            return makeErrorAttempt(aInverse.meError);
        return makeNumericAttempt(aInverse.maValue);
    }

    if (aFunctionName == u"HYPGEOMDIST" || aFunctionName == u"HYPGEOM.DIST")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aX = materializeNumber(*rNode.maChildren[0]);
        if (!aX.mbSupported)
            return makeUnsupportedAttempt(aX.meFallbackReason);
        if (!aX.moValue)
            return makeErrorAttempt(aX.meError);

        const auto aTrials = materializeNumber(*rNode.maChildren[1]);
        if (!aTrials.mbSupported)
            return makeUnsupportedAttempt(aTrials.meFallbackReason);
        if (!aTrials.moValue)
            return makeErrorAttempt(aTrials.meError);

        const auto aSuccesses = materializeNumber(*rNode.maChildren[2]);
        if (!aSuccesses.mbSupported)
            return makeUnsupportedAttempt(aSuccesses.meFallbackReason);
        if (!aSuccesses.moValue)
            return makeErrorAttempt(aSuccesses.meError);

        const auto aPopulation = materializeNumber(*rNode.maChildren[3]);
        if (!aPopulation.mbSupported)
            return makeUnsupportedAttempt(aPopulation.meFallbackReason);
        if (!aPopulation.moValue)
            return makeErrorAttempt(aPopulation.meError);

        bool bCumulative = false;
        if (rNode.maChildren.size() == 5)
        {
            const auto aCumulative = materializeBool(*rNode.maChildren[4]);
            if (!aCumulative.mbSupported)
                return makeUnsupportedAttempt(aCumulative.meFallbackReason);
            if (!aCumulative.moValue)
                return makeErrorAttempt(aCumulative.meError);
            bCumulative = *aCumulative.moValue;
        }

        const auto aDistribution
            = spreadsheetengine::core::math::evaluateHypergeometricDistribution(
                *aX.moValue, *aTrials.moValue, *aSuccesses.moValue, *aPopulation.moValue,
                bCumulative);
        if (!aDistribution)
            return makeErrorAttempt(aDistribution.meError);
        return makeNumericAttempt(aDistribution.maValue);
    }

    if (aFunctionName == u"GAMMA" || aFunctionName == u"COM.MICROSOFT.GAMMA")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumber = materializeNumber(*rNode.maChildren[0]);
        if (!aNumber.mbSupported)
            return makeUnsupportedAttempt(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeErrorAttempt(aNumber.meError);
        return finishCalcMathAttempt(
            spreadsheetengine::core::math::evaluateGammaValue(*aNumber.moValue));
    }

    if (aFunctionName == u"FISHER")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumber = materializeNumber(*rNode.maChildren[0]);
        if (!aNumber.mbSupported)
            return makeUnsupportedAttempt(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeErrorAttempt(aNumber.meError);
        const auto aFisher = spreadsheetengine::core::math::fisherTransform(*aNumber.moValue);
        return finishCalcMathAttempt(aFisher);
    }

    if (aFunctionName == u"FISHERINV")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aNumber = materializeNumber(*rNode.maChildren[0]);
        if (!aNumber.mbSupported)
            return makeUnsupportedAttempt(aNumber.meFallbackReason);
        if (!aNumber.moValue)
            return makeErrorAttempt(aNumber.meError);
        return makeNumericAttempt(
            spreadsheetengine::core::math::inverseFisherTransform(*aNumber.moValue));
    }

    if (aFunctionName == u"POISSON" || aFunctionName == u"POISSON.DIST"
        || aFunctionName == u"COM.MICROSOFT.POISSON.DIST")
    {
        const bool bLegacyPoisson = aFunctionName == u"POISSON";
        if ((bLegacyPoisson && (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3))
            || (!bLegacyPoisson && rNode.maChildren.size() != 3))
        {
            return makeErrorAttempt(api::Error::IllegalArgument);
        }

        const auto aX = materializeNumber(*rNode.maChildren[0]);
        if (!aX.mbSupported)
            return makeUnsupportedAttempt(aX.meFallbackReason);
        if (!aX.moValue)
            return makeErrorAttempt(aX.meError);

        const auto aLambda = materializeNumber(*rNode.maChildren[1]);
        if (!aLambda.mbSupported)
            return makeUnsupportedAttempt(aLambda.meFallbackReason);
        if (!aLambda.moValue)
            return makeErrorAttempt(aLambda.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 3)
        {
            const auto aBool = materializeBool(*rNode.maChildren[2]);
            if (!aBool.mbSupported)
                return makeUnsupportedAttempt(aBool.meFallbackReason);
            if (!aBool.moValue)
                return makeErrorAttempt(aBool.meError);
            bCumulative = *aBool.moValue;
        }

        const auto aPoisson = spreadsheetengine::core::math::evaluatePoissonDistribution(
            *aX.moValue, *aLambda.moValue, bCumulative);
        if (!aPoisson)
            return makeErrorAttempt(aPoisson.meError);
        return makeNumericAttempt(aPoisson.maValue);
    }

    if (aFunctionName == u"BINOMDIST" || aFunctionName == u"BINOM.DIST")
    {
        if (rNode.maChildren.size() != 4)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aX = materializeNumber(*rNode.maChildren[0]);
        if (!aX.mbSupported)
            return makeUnsupportedAttempt(aX.meFallbackReason);
        if (!aX.moValue)
            return makeErrorAttempt(aX.meError);

        const auto aTrials = materializeNumber(*rNode.maChildren[1]);
        if (!aTrials.mbSupported)
            return makeUnsupportedAttempt(aTrials.meFallbackReason);
        if (!aTrials.moValue)
            return makeErrorAttempt(aTrials.meError);

        const auto aProbability = materializeNumber(*rNode.maChildren[2]);
        if (!aProbability.mbSupported)
            return makeUnsupportedAttempt(aProbability.meFallbackReason);
        if (!aProbability.moValue)
            return makeErrorAttempt(aProbability.meError);

        const auto aCumulative = materializeBool(*rNode.maChildren[3]);
        if (!aCumulative.mbSupported)
            return makeUnsupportedAttempt(aCumulative.meFallbackReason);
        if (!aCumulative.moValue)
            return makeErrorAttempt(aCumulative.meError);

        const auto aBinomial = spreadsheetengine::core::math::evaluateBinomialDistribution(
            *aX.moValue, *aTrials.moValue, *aProbability.moValue, *aCumulative.moValue);
        if (!aBinomial)
            return makeErrorAttempt(aBinomial.meError);
        return makeNumericAttempt(aBinomial.maValue);
    }

    if (aFunctionName == u"BINOM.DIST.RANGE" || aFunctionName == u"B")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aTrials = materializeNumber(*rNode.maChildren[0]);
        if (!aTrials.mbSupported)
            return makeUnsupportedAttempt(aTrials.meFallbackReason);
        if (!aTrials.moValue)
            return makeErrorAttempt(aTrials.meError);

        const auto aProbability = materializeNumber(*rNode.maChildren[1]);
        if (!aProbability.mbSupported)
            return makeUnsupportedAttempt(aProbability.meFallbackReason);
        if (!aProbability.moValue)
            return makeErrorAttempt(aProbability.meError);

        const auto aStart = materializeNumber(*rNode.maChildren[2]);
        if (!aStart.mbSupported)
            return makeUnsupportedAttempt(aStart.meFallbackReason);
        if (!aStart.moValue)
            return makeErrorAttempt(aStart.meError);

        double fEnd = *aStart.moValue;
        if (rNode.maChildren.size() == 4)
        {
            const auto aEnd = materializeNumber(*rNode.maChildren[3]);
            if (!aEnd.mbSupported)
                return makeUnsupportedAttempt(aEnd.meFallbackReason);
            if (!aEnd.moValue)
                return makeErrorAttempt(aEnd.meError);
            fEnd = *aEnd.moValue;
        }

        const auto aRange = spreadsheetengine::core::math::evaluateBinomialRangeDistribution(
            *aTrials.moValue, *aProbability.moValue, *aStart.moValue, fEnd);
        if (!aRange)
            return makeErrorAttempt(aRange.meError);
        return makeNumericAttempt(aRange.maValue);
    }

    if (aFunctionName == u"CRITBINOM")
    {
        if (rNode.maChildren.size() != 3)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aTrials = materializeNumber(*rNode.maChildren[0]);
        if (!aTrials.mbSupported)
            return makeUnsupportedAttempt(aTrials.meFallbackReason);
        if (!aTrials.moValue)
            return makeErrorAttempt(aTrials.meError);

        const auto aProbability = materializeNumber(*rNode.maChildren[1]);
        if (!aProbability.mbSupported)
            return makeUnsupportedAttempt(aProbability.meFallbackReason);
        if (!aProbability.moValue)
            return makeErrorAttempt(aProbability.meError);

        const auto aAlpha = materializeNumber(*rNode.maChildren[2]);
        if (!aAlpha.mbSupported)
            return makeUnsupportedAttempt(aAlpha.meFallbackReason);
        if (!aAlpha.moValue)
            return makeErrorAttempt(aAlpha.meError);

        const auto aInverse = spreadsheetengine::core::math::evaluateBinomialInverse(
            *aTrials.moValue, *aProbability.moValue, *aAlpha.moValue);
        if (!aInverse)
            return makeErrorAttempt(aInverse.meError);
        return makeNumericAttempt(aInverse.maValue);
    }

    if (aFunctionName == u"NEGBINOMDIST" || aFunctionName == u"NEGBINOM.DIST"
        || aFunctionName == u"COM.MICROSOFT.NEGBINOM.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName != u"NEGBINOMDIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 4)
            || (!bMicrosoftSyntax && rNode.maChildren.size() != 3))
        {
            return makeErrorAttempt(api::Error::IllegalArgument);
        }

        const auto aFailures = materializeNumber(*rNode.maChildren[0]);
        if (!aFailures.mbSupported)
            return makeUnsupportedAttempt(aFailures.meFallbackReason);
        if (!aFailures.moValue)
            return makeErrorAttempt(aFailures.meError);

        const auto aSuccesses = materializeNumber(*rNode.maChildren[1]);
        if (!aSuccesses.mbSupported)
            return makeUnsupportedAttempt(aSuccesses.meFallbackReason);
        if (!aSuccesses.moValue)
            return makeErrorAttempt(aSuccesses.meError);

        const auto aProbability = materializeNumber(*rNode.maChildren[2]);
        if (!aProbability.mbSupported)
            return makeUnsupportedAttempt(aProbability.meFallbackReason);
        if (!aProbability.moValue)
            return makeErrorAttempt(aProbability.meError);

        bool bCumulative = false;
        if (bMicrosoftSyntax)
        {
            const auto aCumulative = materializeBool(*rNode.maChildren[3]);
            if (!aCumulative.mbSupported)
                return makeUnsupportedAttempt(aCumulative.meFallbackReason);
            if (!aCumulative.moValue)
                return makeErrorAttempt(aCumulative.meError);
            bCumulative = *aCumulative.moValue;
        }

        const auto aDistribution
            = spreadsheetengine::core::math::evaluateNegativeBinomialDistribution(
                *aFailures.moValue, *aSuccesses.moValue, *aProbability.moValue, bCumulative,
                bMicrosoftSyntax);
        if (!aDistribution)
            return makeErrorAttempt(aDistribution.meError);
        return makeNumericAttempt(aDistribution.maValue);
    }

    if (aFunctionName == u"BETADIST" || aFunctionName == u"BETA.DIST")
    {
        const bool bMicrosoftOrder = aFunctionName == u"BETA.DIST";
        if ((bMicrosoftOrder && (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6))
            || (!bMicrosoftOrder && (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)))
        {
            return makeErrorAttempt(api::Error::IllegalArgument);
        }

        const auto aX = materializeNumber(*rNode.maChildren[0]);
        if (!aX.mbSupported)
            return makeUnsupportedAttempt(aX.meFallbackReason);
        if (!aX.moValue)
            return makeErrorAttempt(aX.meError);

        const auto aAlpha = materializeNumber(*rNode.maChildren[1]);
        if (!aAlpha.mbSupported)
            return makeUnsupportedAttempt(aAlpha.meFallbackReason);
        if (!aAlpha.moValue)
            return makeErrorAttempt(aAlpha.meError);

        const auto aBeta = materializeNumber(*rNode.maChildren[2]);
        if (!aBeta.mbSupported)
            return makeUnsupportedAttempt(aBeta.meFallbackReason);
        if (!aBeta.moValue)
            return makeErrorAttempt(aBeta.meError);

        bool bCumulative = true;
        double fLowerBound = 0.0;
        double fUpperBound = 1.0;
        if (bMicrosoftOrder)
        {
            const auto aCumulative = materializeBool(*rNode.maChildren[3]);
            if (!aCumulative.mbSupported)
                return makeUnsupportedAttempt(aCumulative.meFallbackReason);
            if (!aCumulative.moValue)
                return makeErrorAttempt(aCumulative.meError);
            bCumulative = *aCumulative.moValue;
            if (rNode.maChildren.size() >= 5)
            {
                const auto aLower = materializeNumber(*rNode.maChildren[4]);
                if (!aLower.mbSupported)
                    return makeUnsupportedAttempt(aLower.meFallbackReason);
                if (!aLower.moValue)
                    return makeErrorAttempt(aLower.meError);
                fLowerBound = *aLower.moValue;
            }
            if (rNode.maChildren.size() >= 6)
            {
                const auto aUpper = materializeNumber(*rNode.maChildren[5]);
                if (!aUpper.mbSupported)
                    return makeUnsupportedAttempt(aUpper.meFallbackReason);
                if (!aUpper.moValue)
                    return makeErrorAttempt(aUpper.meError);
                fUpperBound = *aUpper.moValue;
            }
        }
        else
        {
            if (rNode.maChildren.size() >= 4)
            {
                const auto aLower = materializeNumber(*rNode.maChildren[3]);
                if (!aLower.mbSupported)
                    return makeUnsupportedAttempt(aLower.meFallbackReason);
                if (!aLower.moValue)
                    return makeErrorAttempt(aLower.meError);
                fLowerBound = *aLower.moValue;
            }
            if (rNode.maChildren.size() >= 5)
            {
                const auto aUpper = materializeNumber(*rNode.maChildren[4]);
                if (!aUpper.mbSupported)
                    return makeUnsupportedAttempt(aUpper.meFallbackReason);
                if (!aUpper.moValue)
                    return makeErrorAttempt(aUpper.meError);
                fUpperBound = *aUpper.moValue;
            }
            if (rNode.maChildren.size() == 6)
            {
                const auto aCumulative = materializeBool(*rNode.maChildren[5]);
                if (!aCumulative.mbSupported)
                    return makeUnsupportedAttempt(aCumulative.meFallbackReason);
                if (!aCumulative.moValue)
                    return makeErrorAttempt(aCumulative.meError);
                bCumulative = *aCumulative.moValue;
            }
        }

        const auto aDistribution = spreadsheetengine::core::math::evaluateBetaDistribution(
            *aX.moValue, *aAlpha.moValue, *aBeta.moValue, fLowerBound, fUpperBound,
            bCumulative, bMicrosoftOrder);
        if (!aDistribution)
            return makeErrorAttempt(aDistribution.meError);
        return makeNumericAttempt(aDistribution.maValue);
    }

    if (aFunctionName == u"INTERCEPT" || aFunctionName == u"FORECAST")
    {
        if (rNode.maChildren.size() != 2 + (aFunctionName == u"FORECAST" ? 1 : 0))
            return makeErrorAttempt(api::Error::IllegalArgument);

        const core::formula::Node& rKnownY
            = *rNode.maChildren[aFunctionName == u"FORECAST" ? 1 : 0];
        const core::formula::Node& rKnownX
            = *rNode.maChildren[aFunctionName == u"FORECAST" ? 2 : 1];
        const auto aStats = collectRegressionStats(rKnownY, rKnownX);
        if (!aStats.mbSupported)
            return makeUnsupportedAttempt(aStats.meFallbackReason);
        if (!aStats.moValue)
            return makeErrorAttempt(aStats.meError);
        if (rtl::math::approxEqual(aStats.moValue->fSumSqrDeltaX, 0.0))
            return makeErrorAttempt(api::Error::DivisionByZero);

        const double fSlope = aStats.moValue->fSumDeltaXDeltaY / aStats.moValue->fSumSqrDeltaX;
        const double fIntercept = aStats.moValue->fMeanY - fSlope * aStats.moValue->fMeanX;
        if (aFunctionName == u"INTERCEPT")
            return makeNumericAttempt(fIntercept);

        const auto aForecastX = materializeNumber(*rNode.maChildren[0]);
        if (!aForecastX.mbSupported)
            return makeUnsupportedAttempt(aForecastX.meFallbackReason);
        if (!aForecastX.moValue)
            return makeErrorAttempt(aForecastX.meError);
        return makeNumericAttempt(
            aStats.moValue->fMeanY + fSlope * (*aForecastX.moValue - aStats.moValue->fMeanX));
    }

    if (aFunctionName == u"PROB")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto aUpper = materializeNumber(*rNode.maChildren[2]);
        if (!aUpper.mbSupported)
            return makeUnsupportedAttempt(aUpper.meFallbackReason);
        if (!aUpper.moValue)
            return makeErrorAttempt(aUpper.meError);

        double fLower = *aUpper.moValue;
        if (rNode.maChildren.size() == 4)
        {
            const auto aLower = materializeNumber(*rNode.maChildren[3]);
            if (!aLower.mbSupported)
                return makeUnsupportedAttempt(aLower.meFallbackReason);
            if (!aLower.moValue)
                return makeErrorAttempt(aLower.meError);
            fLower = *aLower.moValue;
        }

        const auto aProbabilities
            = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aProbabilities.mbSupported)
            return makeUnsupportedAttempt(aProbabilities.meFallbackReason);
        if (!aProbabilities.moValue)
            return makeErrorAttempt(aProbabilities.meError);

        const auto aValues = materializeMatrixNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
        if (!aValues.mbSupported)
            return makeUnsupportedAttempt(aValues.meFallbackReason);
        if (!aValues.moValue)
            return makeErrorAttempt(aValues.meError);

        SCSIZE nProbCols = 0, nProbRows = 0, nValueCols = 0, nValueRows = 0;
        (*aProbabilities.moValue)->GetDimensions(nProbCols, nProbRows);
        (*aValues.moValue)->GetDimensions(nValueCols, nValueRows);
        if (nProbCols != nValueCols || nProbRows != nValueRows || nProbCols == 0 || nProbRows == 0)
            return makeErrorAttempt(api::Error::NotAvailable);

        std::vector<double> aProbabilityValues;
        std::vector<double> aDataValues;
        aProbabilityValues.reserve(nProbCols * nProbRows);
        aDataValues.reserve(nProbCols * nProbRows);
        for (SCSIZE nRow = 0; nRow < nProbRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nProbCols; ++nColumn)
            {
                const auto aProbability = lookupexecution::detail::toApiCellValue(
                    (*aProbabilities.moValue)->Get(nColumn, nRow));
                const auto aValue
                    = lookupexecution::detail::toApiCellValue((*aValues.moValue)->Get(nColumn, nRow));
                if (aProbability.isError())
                    return makeErrorAttempt(aProbability.meError);
                if (aValue.isError())
                    return makeErrorAttempt(aValue.meError);
                if (!aProbability.isNumber() || !aValue.isNumber())
                    return makeErrorAttempt(api::Error::IllegalArgument);

                aProbabilityValues.push_back(aProbability.mfNumber);
                aDataValues.push_back(aValue.mfNumber);
            }
        }

        const auto aProbability = spreadsheetengine::core::math::evaluateProbability(
            aProbabilityValues, aDataValues, fLower, *aUpper.moValue);
        if (!aProbability)
            return makeErrorAttempt(aProbability.meError);
        return makeNumericAttempt(aProbability.maValue);
    }

    return makeUnsupportedAttempt(FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateGrowthFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const auto makeNumericAttempt = [&](double fValue) {
        return makeNumericResult(eFunction, fValue, SvNumFormatType::NUMBER);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eFunction, eError);
    };
    const auto materializeOneDimensionalSequence =
        [&](const core::formula::Node* pArgument, bool bRequirePositive,
            bool* pWasOmitted = nullptr) -> Materialization<std::vector<double>> {
        if (pWasOmitted)
            *pWasOmitted = false;

        if (!pArgument)
            return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);
        if (pArgument->meKind == core::formula::NodeKind::EmptyArgument)
        {
            if (pWasOmitted)
                *pWasOmitted = true;
            return makeMaterializedValue(std::vector<double> {});
        }

        const auto aMatrix = materializeMatrixNode(*pArgument, rDoc, rContext, rFormulaPos);
        if (!aMatrix.mbSupported)
            return makeUnsupportedMaterialization<std::vector<double>>(aMatrix.meFallbackReason);
        if (!aMatrix.moValue)
            return makeMaterializedError<std::vector<double>>(aMatrix.meError);

        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
        if (nColumns == 0 || nRows == 0)
            return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);
        if (nColumns != 1 && nRows != 1)
        {
            return makeUnsupportedMaterialization<std::vector<double>>(
                FallbackReason::UnsupportedFormulaShape);
        }

        std::vector<double> aValues;
        aValues.reserve(nColumns * nRows);
        for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            {
                const auto aValue = lookupexecution::detail::toApiCellValue(
                    (*aMatrix.moValue)->Get(nColumn, nRow));
                if (!aValue.isNumber())
                {
                    if (aValue.isError())
                        return makeMaterializedError<std::vector<double>>(aValue.meError);
                    return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);
                }

                if (bRequirePositive && !(aValue.mfNumber > 0.0))
                    return makeMaterializedError<std::vector<double>>(api::Error::IllegalArgument);
                aValues.push_back(aValue.mfNumber);
            }
        }

        return makeMaterializedValue(std::move(aValues));
    };

    const auto materializeConstantFlag = [&](const core::formula::Node* pArgument)
        -> Materialization<bool> {
        if (!pArgument || pArgument->meKind == core::formula::NodeKind::EmptyArgument)
            return makeMaterializedValue(true);

        const auto aScalar
            = materializeScalarizedReferenceValueNode(*pArgument, rDoc, rContext, rFormulaPos);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<bool>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<bool>(aScalar.meError);

        if (aScalar.moValue->isBoolean())
            return makeMaterializedValue(aScalar.moValue->mfNumber != 0.0);
        if (aScalar.moValue->isNumber())
            return makeMaterializedValue(aScalar.moValue->mfNumber != 0.0);
        return makeMaterializedError<bool>(api::Error::IllegalArgument);
    };

    if (rNode.maChildren.empty() || rNode.maChildren.size() > 4)
        return makeErrorAttempt(api::Error::IllegalArgument);

    const auto aKnownY = materializeOneDimensionalSequence(&*rNode.maChildren[0], true);
    if (!aKnownY.mbSupported)
        return makeUnsupported(eFunction, aKnownY.meFallbackReason);
    if (!aKnownY.moValue)
        return makeErrorAttempt(aKnownY.meError);
    if (aKnownY.moValue->empty())
        return makeErrorAttempt(api::Error::IllegalArgument);

    std::vector<double> aKnownX;
    if (rNode.maChildren.size() >= 2)
    {
        const auto aKnownXValues = materializeOneDimensionalSequence(
            &*rNode.maChildren[1], false);
        if (!aKnownXValues.mbSupported)
            return makeUnsupported(eFunction, aKnownXValues.meFallbackReason);
        if (!aKnownXValues.moValue)
            return makeErrorAttempt(aKnownXValues.meError);
        aKnownX = *aKnownXValues.moValue;
    }

    if (aKnownX.empty())
    {
        aKnownX.reserve(aKnownY.moValue->size());
        for (std::size_t nIndex = 0; nIndex < aKnownY.moValue->size(); ++nIndex)
            aKnownX.push_back(static_cast<double>(nIndex + 1));
    }

    if (aKnownX.size() != aKnownY.moValue->size())
        return makeErrorAttempt(api::Error::IllegalArgument);

    std::vector<double> aNewX;
    if (rNode.maChildren.size() >= 3)
    {
        const auto aNewXValues = materializeOneDimensionalSequence(
            &*rNode.maChildren[2], false);
        if (!aNewXValues.mbSupported)
            return makeUnsupported(eFunction, aNewXValues.meFallbackReason);
        if (!aNewXValues.moValue)
            return makeErrorAttempt(aNewXValues.meError);
        aNewX = *aNewXValues.moValue;
    }

    if (aNewX.empty())
        aNewX = aKnownX;
    if (aNewX.empty())
        return makeErrorAttempt(api::Error::IllegalArgument);

    const auto aConstant = materializeConstantFlag(
        rNode.maChildren.size() >= 4 ? &*rNode.maChildren[3] : nullptr);
    if (!aConstant.mbSupported)
        return makeUnsupported(eFunction, aConstant.meFallbackReason);
    if (!aConstant.moValue)
        return makeErrorAttempt(aConstant.meError);

    std::vector<double> aLoggedY;
    aLoggedY.reserve(aKnownY.moValue->size());
    for (double fValue : *aKnownY.moValue)
        aLoggedY.push_back(std::log(fValue));

    double fSlope = 0.0;
    double fIntercept = 0.0;
    if (*aConstant.moValue)
    {
        const double fCount = static_cast<double>(aKnownX.size());
        const double fMeanX
            = std::accumulate(aKnownX.begin(), aKnownX.end(), 0.0) / fCount;
        const double fMeanY
            = std::accumulate(aLoggedY.begin(), aLoggedY.end(), 0.0) / fCount;

        double fSumDeltaXDeltaY = 0.0;
        double fSumSqrDeltaX = 0.0;
        for (std::size_t nIndex = 0; nIndex < aKnownX.size(); ++nIndex)
        {
            const double fDeltaX = aKnownX[nIndex] - fMeanX;
            const double fDeltaY = aLoggedY[nIndex] - fMeanY;
            fSumDeltaXDeltaY += fDeltaX * fDeltaY;
            fSumSqrDeltaX += fDeltaX * fDeltaX;
        }

        if (rtl::math::approxEqual(fSumSqrDeltaX, 0.0))
            return makeErrorAttempt(api::Error::NoValue);

        fSlope = fSumDeltaXDeltaY / fSumSqrDeltaX;
        fIntercept = fMeanY - fSlope * fMeanX;
    }
    else
    {
        double fSumXY = 0.0;
        double fSumX2 = 0.0;
        for (std::size_t nIndex = 0; nIndex < aKnownX.size(); ++nIndex)
        {
            fSumXY += aKnownX[nIndex] * aLoggedY[nIndex];
            fSumX2 += aKnownX[nIndex] * aKnownX[nIndex];
        }

        if (rtl::math::approxEqual(fSumX2, 0.0))
            return makeErrorAttempt(api::Error::NoValue);

        fSlope = fSumXY / fSumX2;
    }

    return makeNumericAttempt(std::exp(fIntercept + fSlope * aNewX.front()));
}

[[nodiscard]] inline EvaluationAttempt evaluateTextUtilityFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    bool bImportedCanonicalSource)
{
    api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if (aFunctionName == u"COM.MICROSOFT.TEXTAFTER")
        aFunctionName = u"TEXTAFTER"_ustr;
    else if (aFunctionName == u"COM.MICROSOFT.TEXTBEFORE")
        aFunctionName = u"TEXTBEFORE"_ustr;
    auto materializeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        const auto aAttempt = evaluateScalarOrDelegatedNode(
            rArgument, eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1,
            bImportedCanonicalSource);
        if (!aAttempt.mbSupported)
            return makeUnsupportedMaterialization<api::CellValue>(aAttempt.meFallbackReason);
        switch (aAttempt.maResult.meType)
        {
            case api::formulavalue::ValueType::Error:
                return makeMaterializedValue(api::CellValue::error(aAttempt.maResult.meError));
            case api::formulavalue::ValueType::String:
                return makeMaterializedValue(api::CellValue::text(aAttempt.maResult.maString));
            case api::formulavalue::ValueType::Value:
                if (aAttempt.meFormatType == SvNumFormatType::LOGICAL)
                    return makeMaterializedValue(
                        api::CellValue::boolean(aAttempt.maResult.mfValue != 0.0));
                return makeMaterializedValue(api::CellValue::number(aAttempt.maResult.mfValue));
            case api::formulavalue::ValueType::Invalid:
                return makeMaterializedValue(api::CellValue::empty());
        }
        return makeMaterializedValue(api::CellValue::empty());
    };
    auto materializeFirstValue = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        if (rArgument.meKind == core::formula::NodeKind::CellReference
            || rArgument.meKind == core::formula::NodeKind::RangeReference
            || rArgument.meKind == core::formula::NodeKind::NamedReference)
        {
            const auto aScalar
                = materializeScalarizedReferenceValueNode(rArgument, rDoc, rContext, rFormulaPos);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<api::CellValue>(aScalar.meFallbackReason);
            if (!aScalar.moValue)
                return makeMaterializedError<api::CellValue>(aScalar.meError);
            return makeMaterializedValue(*aScalar.moValue);
        }
        return materializeArgument(rArgument);
    };
    auto materializeTextArgument = [&](const core::formula::Node& rArgument)
        -> api::ValueResult<OUString> {
        const auto aValue = materializeArgument(rArgument);
        if (!aValue.mbSupported)
            return api::ValueResult<OUString>::failure(api::Error::NoValue);
        if (!aValue.moValue)
            return api::ValueResult<OUString>::failure(aValue.meError);
        const auto aText = coerceScalarToText(rDoc, rContext, *aValue.moValue);
        if (!aText)
            return api::ValueResult<OUString>::failure(aText.meError);
        return api::ValueResult<OUString>::success(aText.maValue);
    };

    if (aFunctionName == u"CONCATENATE" || aFunctionName == u"CONCAT")
    {
        if (rNode.maChildren.empty())
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        OUString aResult;
        for (const auto& rxChild : rNode.maChildren)
        {
            if (!rxChild)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            const auto aText = materializeTextArgument(*rxChild);
            if (!aText)
                return makeErrorResult(eFunction, aText.meError);
            aResult += aText.maValue;
        }
        return makeStringResult(eFunction, aResult);
    }

    if (aFunctionName == u"CLEAN")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);
        return makeStringResult(eFunction, cleanPrintable(aText.maValue));
    }

    if (aFunctionName == u"CHAR")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aValue = materializeArgument(*rNode.maChildren[0]);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorResult(eFunction, aValue.meError);
        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
        if (!aNumber)
            return makeErrorResult(eFunction, aNumber.meError);
        const auto oWhole = coerceWholeNumber(aNumber.maValue);
        if (!oWhole || *oWhole < 1 || *oWhole > 255)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto oChar = charFromValue(static_cast<double>(*oWhole));
        if (!oChar)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        return makeStringResult(eFunction, *oChar);
    }

    if (aFunctionName == u"CODE")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);
        if (aText.maValue.isEmpty())
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        return makeNumericResult(
            eFunction, static_cast<double>(codeFromText(aText.maValue)), SvNumFormatType::NUMBER);
    }

    if (aFunctionName == u"UNICHAR")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aValue = materializeArgument(*rNode.maChildren[0]);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorResult(eFunction, aValue.meError);
        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
        if (!aNumber)
            return makeErrorResult(eFunction, aNumber.meError);
        const auto oWhole = coerceWholeNumber(aNumber.maValue);
        if (!oWhole || *oWhole < 0)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto oChar = unicharFromCodePoint(static_cast<sal_uInt32>(*oWhole));
        if (!oChar)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        return makeStringResult(eFunction, *oChar);
    }

    if (aFunctionName == u"UNICODE")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);
        const auto oCode = unicodeFromText(aText.maValue);
        if (!oCode)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        return makeNumericResult(eFunction, *oCode, SvNumFormatType::NUMBER);
    }

    if (aFunctionName == u"UPPER" || aFunctionName == u"LOWER" || aFunctionName == u"PROPER")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);
        if (aFunctionName == u"UPPER")
            return makeStringResult(eFunction, uppercase(ScGlobal::getCharClass(), aText.maValue));
        if (aFunctionName == u"LOWER")
            return makeStringResult(eFunction, lowercase(ScGlobal::getCharClass(), aText.maValue));
        return makeStringResult(eFunction, propercase(ScGlobal::getCharClass(), aText.maValue));
    }

    if (aFunctionName == u"ASC" || aFunctionName == u"JIS")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);
        return makeStringResult(
            eFunction, aFunctionName == u"ASC" ? convertIntoHalfWidth(aText.maValue)
                                               : convertIntoFullWidth(aText.maValue));
    }

    if (aFunctionName == u"LEN")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);
        return makeNumericResult(
            eFunction, static_cast<double>(countCodePoints(aText.maValue)), SvNumFormatType::NUMBER);
    }

    if (aFunctionName == u"LEFT" || aFunctionName == u"RIGHT")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);

        sal_Int32 nLength = 1;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aLength = materializeArgument(*rNode.maChildren[1]);
            if (!aLength.mbSupported)
                return makeUnsupported(eFunction, aLength.meFallbackReason);
            if (!aLength.moValue)
                return makeErrorResult(eFunction, aLength.meError);
            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aLength.moValue);
            if (!aNumber)
                return makeErrorResult(eFunction, aNumber.meError);
            const auto oWhole = coerceWholeNumber(aNumber.maValue);
            if (!oWhole || *oWhole < 0)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            nLength = *oWhole;
        }

        return makeStringResult(eFunction,
            toLibreOfficeString(spreadsheetengine::core::text::sliceTextLeftRight(
                toApiString(aText.maValue), nLength, aFunctionName == u"RIGHT")));
    }

    if (aFunctionName == u"T")
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aValue = materializeFirstValue(*rNode.maChildren[0]);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorResult(eFunction, aValue.meError);
        if (aValue.moValue->isError())
            return makeErrorResult(eFunction, aValue.moValue->meError);
        return makeStringResult(
            eFunction, aValue.moValue->isText() ? toLibreOfficeString(aValue.moValue->maString) : u""_ustr);
    }

    if (aFunctionName == u"EXACT")
    {
        if (rNode.maChildren.size() != 2)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);
        const auto aLeft = materializeFirstValue(*rNode.maChildren[0]);
        if (!aLeft.mbSupported)
            return makeUnsupported(eFunction, aLeft.meFallbackReason);
        if (!aLeft.moValue)
            return makeErrorResult(eFunction, aLeft.meError);
        const auto aRight = materializeFirstValue(*rNode.maChildren[1]);
        if (!aRight.mbSupported)
            return makeUnsupported(eFunction, aRight.meFallbackReason);
        if (!aRight.moValue)
            return makeErrorResult(eFunction, aRight.meError);
        const auto aLeftText = coerceScalarToText(rDoc, rContext, *aLeft.moValue);
        if (!aLeftText)
            return makeErrorResult(eFunction, aLeftText.meError);
        const auto aRightText = coerceScalarToText(rDoc, rContext, *aRight.moValue);
        if (!aRightText)
            return makeErrorResult(eFunction, aRightText.meError);
        return makeNumericResult(eFunction, aLeftText.maValue == aRightText.maValue ? 1.0 : 0.0,
            SvNumFormatType::LOGICAL);
    }

    if (aFunctionName == u"TEXTAFTER")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 6)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);

        std::vector<api::String> aDelimiters;
        const auto aDelimiterMatrix
            = materializeMatrixNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
        if (!aDelimiterMatrix.mbSupported)
            return makeUnsupported(eFunction, aDelimiterMatrix.meFallbackReason);
        if (!aDelimiterMatrix.moValue)
            return makeErrorResult(eFunction, aDelimiterMatrix.meError);

        SCSIZE nDelimiterColumns = 0;
        SCSIZE nDelimiterRows = 0;
        (*aDelimiterMatrix.moValue)->GetDimensions(nDelimiterColumns, nDelimiterRows);
        for (SCSIZE nRow = 0; nRow < nDelimiterRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nDelimiterColumns; ++nColumn)
            {
                const auto aValue = lookupexecution::detail::toApiCellValue(
                    (*aDelimiterMatrix.moValue)->Get(nColumn, nRow));
                if (aValue.isError())
                    return makeErrorResult(eFunction, aValue.meError);
                const auto aDelimiter = coerceScalarToText(rDoc, rContext, aValue);
                if (!aDelimiter)
                    return makeErrorResult(eFunction, aDelimiter.meError);
                aDelimiters.push_back(toApiString(aDelimiter.maValue));
            }
        }
        if (aDelimiters.empty())
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        sal_Int32 nInstance = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = materializeArgument(*rNode.maChildren[2]);
            if (!aInstance.mbSupported)
                return makeUnsupported(eFunction, aInstance.meFallbackReason);
            if (!aInstance.moValue)
                return makeErrorResult(eFunction, aInstance.meError);
            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aInstance.moValue);
            if (!aNumber)
                return makeErrorResult(eFunction, aNumber.meError);
            const auto oWhole = coerceWholeNumber(aNumber.maValue);
            if (!oWhole || *oWhole == 0)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            nInstance = *oWhole;
        }

        bool bCaseInsensitive = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMatchMode = materializeArgument(*rNode.maChildren[3]);
            if (!aMatchMode.mbSupported)
                return makeUnsupported(eFunction, aMatchMode.meFallbackReason);
            if (!aMatchMode.moValue)
                return makeErrorResult(eFunction, aMatchMode.meError);
            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aMatchMode.moValue);
            if (!aNumber)
                return makeErrorResult(eFunction, aNumber.meError);
            const auto oWhole = coerceWholeNumber(aNumber.maValue);
            if (!oWhole || (*oWhole != 0 && *oWhole != 1))
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            bCaseInsensitive = *oWhole == 1;
        }

        bool bMatchEnd = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMatchEnd = materializeArgument(*rNode.maChildren[4]);
            if (!aMatchEnd.mbSupported)
                return makeUnsupported(eFunction, aMatchEnd.meFallbackReason);
            if (!aMatchEnd.moValue)
                return makeErrorResult(eFunction, aMatchEnd.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aMatchEnd.moValue);
            if (!aBool)
                return makeErrorResult(eFunction, aBool.meError);
            bMatchEnd = aBool.maValue;
        }

        const auto oResult = spreadsheetengine::core::text::textAfter(
            toApiString(aText.maValue), aDelimiters, nInstance, bCaseInsensitive, bMatchEnd);
        if (!oResult)
        {
            if (rNode.maChildren.size() >= 6
                && rNode.maChildren[5]->meKind != core::formula::NodeKind::EmptyArgument)
            {
                auto aFallback = evaluateScalarOrDelegatedNode(*rNode.maChildren[5], eFunction,
                    rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1, bImportedCanonicalSource);
                aFallback.meFunction = eFunction;
                return aFallback;
            }
            return makeErrorResult(eFunction, api::Error::NotAvailable);
        }

        return makeStringResult(eFunction, toLibreOfficeString(*oResult));
    }

    if (aFunctionName == u"TEXTBEFORE")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 6)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aText = materializeTextArgument(*rNode.maChildren[0]);
        if (!aText)
            return makeErrorResult(eFunction, aText.meError);

        std::vector<api::String> aDelimiters;
        const auto aDelimiterMatrix
            = materializeMatrixNode(*rNode.maChildren[1], rDoc, rContext, rFormulaPos);
        if (!aDelimiterMatrix.mbSupported)
            return makeUnsupported(eFunction, aDelimiterMatrix.meFallbackReason);
        if (!aDelimiterMatrix.moValue)
            return makeErrorResult(eFunction, aDelimiterMatrix.meError);

        SCSIZE nDelimiterColumns = 0;
        SCSIZE nDelimiterRows = 0;
        (*aDelimiterMatrix.moValue)->GetDimensions(nDelimiterColumns, nDelimiterRows);
        for (SCSIZE nRow = 0; nRow < nDelimiterRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nDelimiterColumns; ++nColumn)
            {
                const auto aValue = lookupexecution::detail::toApiCellValue(
                    (*aDelimiterMatrix.moValue)->Get(nColumn, nRow));
                if (aValue.isError())
                    return makeErrorResult(eFunction, aValue.meError);
                const auto aDelimiter = coerceScalarToText(rDoc, rContext, aValue);
                if (!aDelimiter)
                    return makeErrorResult(eFunction, aDelimiter.meError);
                aDelimiters.push_back(toApiString(aDelimiter.maValue));
            }
        }
        if (aDelimiters.empty())
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        sal_Int32 nInstance = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = materializeArgument(*rNode.maChildren[2]);
            if (!aInstance.mbSupported)
                return makeUnsupported(eFunction, aInstance.meFallbackReason);
            if (!aInstance.moValue)
                return makeErrorResult(eFunction, aInstance.meError);
            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aInstance.moValue);
            if (!aNumber)
                return makeErrorResult(eFunction, aNumber.meError);
            const auto oWhole = coerceWholeNumber(aNumber.maValue);
            if (!oWhole || *oWhole == 0)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            nInstance = *oWhole;
        }

        bool bCaseInsensitive = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMatchMode = materializeArgument(*rNode.maChildren[3]);
            if (!aMatchMode.mbSupported)
                return makeUnsupported(eFunction, aMatchMode.meFallbackReason);
            if (!aMatchMode.moValue)
                return makeErrorResult(eFunction, aMatchMode.meError);
            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aMatchMode.moValue);
            if (!aNumber)
                return makeErrorResult(eFunction, aNumber.meError);
            const auto oWhole = coerceWholeNumber(aNumber.maValue);
            if (!oWhole || (*oWhole != 0 && *oWhole != 1))
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            bCaseInsensitive = *oWhole == 1;
        }

        bool bMatchEnd = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != core::formula::NodeKind::EmptyArgument)
        {
            const auto aMatchEnd = materializeArgument(*rNode.maChildren[4]);
            if (!aMatchEnd.mbSupported)
                return makeUnsupported(eFunction, aMatchEnd.meFallbackReason);
            if (!aMatchEnd.moValue)
                return makeErrorResult(eFunction, aMatchEnd.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aMatchEnd.moValue);
            if (!aBool)
                return makeErrorResult(eFunction, aBool.meError);
            bMatchEnd = aBool.maValue;
        }

        const auto oResult = spreadsheetengine::core::text::textBefore(
            toApiString(aText.maValue), aDelimiters, nInstance, bCaseInsensitive, bMatchEnd);
        if (!oResult)
        {
            if (rNode.maChildren.size() >= 6
                && rNode.maChildren[5]->meKind != core::formula::NodeKind::EmptyArgument)
            {
                auto aFallback = evaluateScalarOrDelegatedNode(*rNode.maChildren[5], eFunction,
                    rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1, bImportedCanonicalSource);
                aFallback.meFunction = eFunction;
                return aFallback;
            }
            return makeErrorResult(eFunction, api::Error::NotAvailable);
        }

        return makeStringResult(eFunction, toLibreOfficeString(*oResult));
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateFormulaTextFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bImportedCanonicalSource)
{
    if (rNode.maChildren.size() != 1)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
        && containsReferenceLikeDescendant(rNode))
    {
        return makeErrorResult(eFunction, api::Error::VariableExpected);
    }

    const auto oTargetAddress = formulaTextTargetAddress(*rNode.maChildren[0], rDoc, rFormulaPos);
    if (!oTargetAddress || *oTargetAddress == rFormulaPos)
        return makeUnsupported(eFunction, FallbackReason::UnsupportedHostSurface);

    ScRefCellValue aTargetCell(const_cast<ScDocument&>(rDoc), *oTargetAddress);
    if (aTargetCell.getType() != CELLTYPE_FORMULA)
        return makeErrorResult(eFunction, api::Error::NotAvailable);

    ScFormulaCell* pFormula = aTargetCell.getFormula();
    if (!pFormula)
        return makeErrorResult(eFunction, api::Error::NotAvailable);

    OUString aFormula
        = pFormula->GetFormula(formula::FormulaGrammar::GRAM_NATIVE_UI, &rContext);
    if (aFormula.startsWith(u"=of:="_ustr))
        aFormula = u"="_ustr + aFormula.copy(5);
    if (pFormula->GetMatrixFlag() != ScMatrixMode::NONE && !aFormula.startsWith(u"{="_ustr))
        aFormula = u"{"_ustr + aFormula + u"}"_ustr;
    return makeStringResult(eFunction, aFormula);
}

[[nodiscard]] inline EvaluationAttempt evaluateScalarUtilityFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    bool bImportedCanonicalSource)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    api::String aCanonicalFunctionName = aFunctionName;
    if (aCanonicalFunctionName == u"COM.MICROSOFT.IFS")
        aCanonicalFunctionName = u"IFS"_ustr;
    else if (aCanonicalFunctionName == u"COM.MICROSOFT.SWITCH")
        aCanonicalFunctionName = u"SWITCH"_ustr;
    auto materializeArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        const auto aAttempt = evaluateScalarOrDelegatedNode(
            rArgument, eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1);
        if (!aAttempt.mbSupported)
            return makeUnsupportedMaterialization<api::CellValue>(aAttempt.meFallbackReason);

        switch (aAttempt.maResult.meType)
        {
            case api::formulavalue::ValueType::Error:
                return makeMaterializedValue(api::CellValue::error(aAttempt.maResult.meError));
            case api::formulavalue::ValueType::String:
                return makeMaterializedValue(api::CellValue::text(aAttempt.maResult.maString));
            case api::formulavalue::ValueType::Value:
                if (aAttempt.meFormatType == SvNumFormatType::LOGICAL)
                    return makeMaterializedValue(
                        api::CellValue::boolean(aAttempt.maResult.mfValue != 0.0));
                return makeMaterializedValue(api::CellValue::number(aAttempt.maResult.mfValue));
            case api::formulavalue::ValueType::Invalid:
                return makeMaterializedValue(api::CellValue::empty());
        }

        return makeMaterializedValue(api::CellValue::empty());
    };
    auto materializeLogicalReferenceArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<api::CellValue> {
        const auto aMatrix = materializeMatrixNode(rArgument, rDoc, rContext, rFormulaPos);
        if (!aMatrix.mbSupported)
            return makeUnsupportedMaterialization<api::CellValue>(aMatrix.meFallbackReason);
        if (!aMatrix.moValue)
            return makeMaterializedError<api::CellValue>(aMatrix.meError);

        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        (*aMatrix.moValue)->GetDimensions(nColumns, nRows);

        bool bResult = aFunctionName == u"AND";
        bool bSawValue = false;
        std::optional<api::Error> oDeferredError;
        for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
        {
            for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            {
                const auto aValue = lookupexecution::detail::toApiCellValue(
                    (*aMatrix.moValue)->Get(nColumn, nRow));
                if (aValue.isError())
                {
                    if (!oDeferredError)
                        oDeferredError = aValue.meError;
                    continue;
                }
                if (aValue.isEmpty() || aValue.isText())
                    continue;

                const auto aBool = coerceScalarToBool(rDoc, rContext, aValue);
                if (!aBool)
                {
                    if (!oDeferredError)
                        oDeferredError = aBool.meError;
                    continue;
                }

                if (aFunctionName == u"AND")
                    bResult = bResult && aBool.maValue;
                else if (aFunctionName == u"OR")
                    bResult = bResult || aBool.maValue;
                else
                    bResult = bResult != aBool.maValue;
                bSawValue = true;
            }
        }

        if (!bSawValue)
            return makeMaterializedError<api::CellValue>(api::Error::NoValue);
        if (oDeferredError)
            return makeMaterializedError<api::CellValue>(*oDeferredError);
        return makeMaterializedValue(api::CellValue::boolean(bResult));
    };

    if (eFunction == FunctionKind::Conditional)
    {
        if (aCanonicalFunctionName == u"IF")
        {
            if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aCondition = materializeArgument(*rNode.maChildren[0]);
            if (!aCondition.mbSupported)
                return makeUnsupported(eFunction, aCondition.meFallbackReason);
            if (!aCondition.moValue)
                return makeErrorResult(eFunction, aCondition.meError);

            const auto aBool = coerceScalarToBool(rDoc, rContext, *aCondition.moValue);
            if (!aBool)
                return makeErrorResult(eFunction, aBool.meError);

            if (!aBool.maValue && rNode.maChildren.size() < 3)
                return makeNumericResult(eFunction, 0.0, SvNumFormatType::LOGICAL);

            const auto& rxSelected = aBool.maValue ? rNode.maChildren[1] : rNode.maChildren[2];
            if (!rxSelected)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            auto aBranch = evaluateScalarOrDelegatedNode(*rxSelected, eFunction, rDoc, rContext,
                rFormulaPos, bEmptyStringAsZero, 1, bImportedCanonicalSource);
            aBranch.meFunction = eFunction;
            return aBranch;
        }

        if (aCanonicalFunctionName == u"IFS")
        {
            if (rNode.maChildren.empty())
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); nIndex += 2)
            {
                const auto aCondition = materializeArgument(*rNode.maChildren[nIndex]);
                const std::int16_t nRemaining
                    = static_cast<std::int16_t>(rNode.maChildren.size() - nIndex - 1);

                bool bCondition = false;
                bool bConditionError = false;
                if (!aCondition.mbSupported)
                    return makeUnsupported(eFunction, aCondition.meFallbackReason);
                if (!aCondition.moValue)
                    bConditionError = true;
                else
                {
                    const auto aBool = coerceScalarToBool(rDoc, rContext, *aCondition.moValue);
                    if (!aBool)
                        bConditionError = true;
                    else
                        bCondition = aBool.maValue;
                }

                switch (api::logic::evaluateIfsCondition(bCondition, bConditionError, nRemaining))
                {
                    case api::logic::IfsAction::SelectCurrentResult:
                    {
                        auto aBranch = evaluateScalarOrDelegatedNode(*rNode.maChildren[nIndex + 1],
                            eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1,
                            bImportedCanonicalSource);
                        aBranch.meFunction = eFunction;
                        return aBranch;
                    }
                    case api::logic::IfsAction::SkipCurrentResult:
                        break;
                    case api::logic::IfsAction::ReturnParameterExpected:
                        return makeErrorResult(eFunction, api::Error::IllegalArgument);
                    case api::logic::IfsAction::ReturnNotAvailable:
                        return makeErrorResult(eFunction, api::Error::NotAvailable);
                    case api::logic::IfsAction::ReturnNoValue:
                        return makeErrorResult(eFunction, api::Error::NoValue);
                }
            }

            return makeErrorResult(eFunction, api::Error::NotAvailable);
        }

        if (aCanonicalFunctionName == u"SWITCH")
        {
            if (rNode.maChildren.size() < 3)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            const auto aReference = materializeArgument(*rNode.maChildren[0]);
            if (!aReference.mbSupported)
                return makeUnsupported(eFunction, aReference.meFallbackReason);
            if (!aReference.moValue)
                return makeErrorResult(eFunction, aReference.meError);

            const api::CellValue aReferenceValue = *aReference.moValue;
            const bool bReferenceIsText = aReferenceValue.isText() || aReferenceValue.isEmpty();
            std::size_t nIndex = 1;
            while (nIndex + 1 < rNode.maChildren.size())
            {
                const auto aCase = materializeArgument(*rNode.maChildren[nIndex]);
                if (!aCase.mbSupported)
                    return makeUnsupported(eFunction, aCase.meFallbackReason);
                if (!aCase.moValue)
                {
                    if (nIndex + 2 >= rNode.maChildren.size())
                        return makeErrorResult(eFunction, aCase.meError);
                    nIndex += 2;
                    continue;
                }

                bool bMatched = false;
                if (bReferenceIsText)
                {
                    const auto aReferenceText = coerceScalarToText(rDoc, rContext, aReferenceValue);
                    const auto aCaseText = coerceScalarToText(rDoc, rContext, *aCase.moValue);
                    if (!aReferenceText || !aCaseText)
                        return makeErrorResult(eFunction, api::Error::NoValue);
                    bMatched = spreadsheetengine::core::query::compareFoldedText(
                                   toApiString(aReferenceText.maValue),
                                   toApiString(aCaseText.maValue))
                               == 0;
                }
                else
                {
                    const auto aReferenceNumber
                        = coerceScalarToNumber(rDoc, rContext, aReferenceValue);
                    const auto aCaseNumber = coerceScalarToNumber(rDoc, rContext, *aCase.moValue);
                    if (!aReferenceNumber || !aCaseNumber)
                    {
                        if (nIndex + 2 >= rNode.maChildren.size())
                            return makeErrorResult(eFunction, api::Error::NoValue);
                        nIndex += 2;
                        continue;
                    }
                    bMatched
                        = rtl::math::approxEqual(aReferenceNumber.maValue, aCaseNumber.maValue);
                }

                if (bMatched)
                {
                    auto aBranch = evaluateScalarOrDelegatedNode(*rNode.maChildren[nIndex + 1],
                        eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1,
                        bImportedCanonicalSource);
                    aBranch.meFunction = eFunction;
                    return aBranch;
                }

                nIndex += 2;
            }

            if (nIndex < rNode.maChildren.size())
            {
                auto aBranch = evaluateScalarOrDelegatedNode(*rNode.maChildren[nIndex], eFunction,
                    rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1,
                    bImportedCanonicalSource);
                aBranch.meFunction = eFunction;
                return aBranch;
            }

            return makeErrorResult(eFunction, api::Error::NotAvailable);
        }

        return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
    }

    if (eFunction == FunctionKind::Round)
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aValue = materializeArgument(*rNode.maChildren[0]);
        if (!aValue.mbSupported)
            return makeUnsupported(eFunction, aValue.meFallbackReason);
        if (!aValue.moValue)
            return makeErrorResult(eFunction, aValue.meError);
        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aValue.moValue);
        if (!aNumber)
            return makeErrorResult(eFunction, aNumber.meError);

        sal_Int32 nDecimals = 0;
        if (rNode.maChildren.size() == 2)
        {
            const auto aDigits = materializeArgument(*rNode.maChildren[1]);
            if (!aDigits.mbSupported)
                return makeUnsupported(eFunction, aDigits.meFallbackReason);
            if (!aDigits.moValue)
                return makeErrorResult(eFunction, aDigits.meError);
            const auto aDigitsNumber = coerceScalarToNumber(rDoc, rContext, *aDigits.moValue);
            if (!aDigitsNumber)
                return makeErrorResult(eFunction, aDigitsNumber.meError);
            if (!std::isfinite(aDigitsNumber.maValue)
                || std::trunc(aDigitsNumber.maValue)
                       < static_cast<double>(std::numeric_limits<sal_Int32>::min())
                || std::trunc(aDigitsNumber.maValue)
                       > static_cast<double>(std::numeric_limits<sal_Int32>::max()))
            {
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            }
            nDecimals = static_cast<sal_Int32>(std::trunc(aDigitsNumber.maValue));
        }

        api::RoundingMode eMode = api::RoundingMode::Corrected;
        if (aFunctionName == u"ROUNDUP")
            eMode = api::RoundingMode::Up;
        else if (aFunctionName == u"ROUNDDOWN")
            eMode = api::RoundingMode::Down;

        const auto aRounded = spreadsheetengine::core::math::evaluateRoundValue(
            aNumber.maValue, nDecimals, eMode,
            aFunctionName == u"ROUNDUP" || aFunctionName == u"ROUNDDOWN");
        if (!aRounded)
            return makeErrorResult(eFunction, aRounded.meError);
        return makeNumericResult(eFunction, aRounded.maValue, SvNumFormatType::NUMBER);
    }

    if (eFunction == FunctionKind::InformationPredicate)
    {
        if (rNode.maChildren.size() != 1)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto classifyOdfErrorTypeLiteral = [](api::StringView rLiteral)
            -> std::optional<double> {
            const api::String aUpper = uppercaseAscii(rLiteral);
            if (aUpper == u"#NULL!")
                return 1.0;
            if (aUpper == u"#DIV/0!")
                return 2.0;
            if (aUpper == u"#VALUE!")
                return 3.0;
            if (aUpper == u"#REF!")
                return 4.0;
            if (aUpper == u"#NAME?")
                return 5.0;
            if (aUpper == u"#NUM!")
                return 6.0;
            if (aUpper == u"#N/A")
                return 7.0;
            return std::nullopt;
        };
        const auto classifyLegacyErrorTypeFormulaError = [](FormulaError eError)
            -> std::optional<double> {
            if (eError == FormulaError::NONE)
                return std::nullopt;
            return static_cast<double>(eError);
        };
        const auto classifyOdfErrorTypeFormulaError = [](FormulaError eError)
            -> std::optional<double> {
            switch (eError)
            {
                case FormulaError::NoCode:
                    return 1.0;
                case FormulaError::DivisionByZero:
                    return 2.0;
                case FormulaError::NoValue:
                    return 3.0;
                case FormulaError::NoRef:
                    return 4.0;
                case FormulaError::NoName:
                    return 5.0;
                case FormulaError::IllegalFPOperation:
                    return 6.0;
                case FormulaError::NotAvailable:
                    return 7.0;
                case FormulaError::NONE:
                default:
                    return std::nullopt;
            }
        };
        const auto classifyErrorTypeValue = [&](const api::CellValue& rValue, bool bLegacy)
            -> std::optional<double> {
            if (!rValue.isError())
                return std::nullopt;
            const FormulaError eFormulaError = toFormulaError(rValue.meError);
            return bLegacy ? classifyLegacyErrorTypeFormulaError(eFormulaError)
                           : classifyOdfErrorTypeFormulaError(eFormulaError);
        };

        if (aFunctionName == u"ERROR.TYPE" || aFunctionName == u"ERRORTYPE")
        {
            const bool bLegacyErrorType = aFunctionName == u"ERRORTYPE";
            const auto classifyReferenceErrorType = [&](const core::formula::Node& rArgument)
                -> std::optional<double> {
                if (rArgument.meKind != core::formula::NodeKind::CellReference
                    && rArgument.meKind != core::formula::NodeKind::RangeReference
                    && rArgument.meKind != core::formula::NodeKind::NamedReference)
                {
                    return std::nullopt;
                }

                const auto aRange = resolveReferenceRangeNode(rArgument, rDoc, rFormulaPos);
                if (!aRange.mbSupported || !aRange.moValue)
                {
                    if (rArgument.meKind == core::formula::NodeKind::NamedReference
                        && aRange.meError == api::Error::NotAvailable)
                    {
                        return bLegacyErrorType ? 525.0 : 5.0;
                    }
                    if ((rArgument.meKind == core::formula::NodeKind::CellReference
                            || rArgument.meKind == core::formula::NodeKind::RangeReference)
                        && aRange.meError == api::Error::IllegalArgument)
                    {
                        return bLegacyErrorType ? 524.0 : 4.0;
                    }
                    return std::nullopt;
                }

                if (aRange.moValue->aStart != aRange.moValue->aEnd)
                {
                    return bLegacyErrorType ? std::optional<double>(519.0)
                                            : std::optional<double>(std::nullopt);
                }

                const ScAddress aAddress = aRange.moValue->aStart;
                if (const FormulaError eError = rDoc.GetErrCode(aAddress); eError != FormulaError::NONE)
                {
                    return bLegacyErrorType ? classifyLegacyErrorTypeFormulaError(eError)
                                            : classifyOdfErrorTypeFormulaError(eError);
                }

                const auto aHostValue = readHostDocumentCellValue(rDoc, aAddress);
                if (!aHostValue)
                    return std::nullopt;
                return classifyErrorTypeValue(aHostValue.maValue, bLegacyErrorType);
            };

            const core::formula::Node& rArgument = *rNode.maChildren[0];
            if (rArgument.meKind == core::formula::NodeKind::ErrorLiteral)
            {
                if (bLegacyErrorType)
                    return makeNumericResult(
                        eFunction,
                        static_cast<double>(
                            spreadsheetengine::compat::libreoffice::toFormulaError(
                                mapErrorLiteral(rArgument.maPrimaryText))),
                        SvNumFormatType::NUMBER);

                if (const auto oErrorType = classifyOdfErrorTypeLiteral(rArgument.maPrimaryText))
                    return makeNumericResult(eFunction, *oErrorType, SvNumFormatType::NUMBER);
                return makeErrorResult(eFunction, api::Error::NotAvailable);
            }

            if (const auto oReferenceErrorType = classifyReferenceErrorType(rArgument))
                return makeNumericResult(eFunction, *oReferenceErrorType, SvNumFormatType::NUMBER);

            const auto aMatrix = materializeMatrixNode(rArgument, rDoc, rContext, rFormulaPos);
            if (aMatrix.mbSupported && aMatrix.moValue)
            {
                SCSIZE nColumns = 0;
                SCSIZE nRows = 0;
                (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
                if (nColumns != 1 || nRows != 1)
                    return makeErrorResult(eFunction, api::Error::NotAvailable);

                const auto aValue
                    = lookupexecution::detail::toApiCellValue((*aMatrix.moValue)->Get(0, 0));
                if (const auto oErrorType = classifyErrorTypeValue(aValue, bLegacyErrorType))
                    return makeNumericResult(eFunction, *oErrorType, SvNumFormatType::NUMBER);
                return makeErrorResult(eFunction, api::Error::NotAvailable);
            }
            if (aMatrix.mbSupported && !aMatrix.moValue)
                return makeErrorResult(eFunction, aMatrix.meError);

            const auto aArgument = materializeArgument(rArgument);
            if (!aArgument.mbSupported)
                return makeUnsupported(eFunction, aArgument.meFallbackReason);
            if (!aArgument.moValue)
                return makeErrorResult(eFunction, aArgument.meError);

            if (const auto oErrorType
                = classifyErrorTypeValue(*aArgument.moValue, bLegacyErrorType))
            {
                return makeNumericResult(eFunction, *oErrorType, SvNumFormatType::NUMBER);
            }
            return makeErrorResult(eFunction, api::Error::NotAvailable);
        }

        const bool bImportedHostTruthPredicate
            = (bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
              && referencesImportedPredicateHostTruthCell(
                  *rNode.maChildren[0], rDoc, rFormulaPos, aFunctionName)
              && (aFunctionName == u"ISERROR" || aFunctionName == u"ISERR"
                  || aFunctionName == u"ISNA" || aFunctionName == u"ISTEXT"
                  || aFunctionName == u"ISNONTEXT" || aFunctionName == u"ISBLANK");
        if (bImportedHostTruthPredicate)
            return makeErrorResult(eFunction, api::Error::VariableExpected);

        const auto materializePredicateArgument = [&](const core::formula::Node& rArgument)
            -> Materialization<api::CellValue> {
            if (rArgument.meKind == core::formula::NodeKind::FunctionCall)
            {
                const auto aAttempt = evaluateDelegatedNode(
                    rArgument, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 1);
                if (!aAttempt.mbSupported)
                    return makeUnsupportedMaterialization<api::CellValue>(
                        aAttempt.meFallbackReason);

                switch (aAttempt.maResult.meType)
                {
                    case api::formulavalue::ValueType::Error:
                        return makeMaterializedValue(
                            api::CellValue::error(aAttempt.maResult.meError));
                    case api::formulavalue::ValueType::String:
                        return makeMaterializedValue(
                            api::CellValue::text(aAttempt.maResult.maString));
                    case api::formulavalue::ValueType::Value:
                        if (aAttempt.meFormatType == SvNumFormatType::LOGICAL)
                            return makeMaterializedValue(api::CellValue::boolean(
                                aAttempt.maResult.mfValue != 0.0));
                        return makeMaterializedValue(
                            api::CellValue::number(aAttempt.maResult.mfValue));
                    case api::formulavalue::ValueType::Invalid:
                        return makeMaterializedValue(api::CellValue::empty());
                }
            }

            const auto aScalar = materializeScalarizedReferenceValueNode(
                rArgument, rDoc, rContext, rFormulaPos);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<api::CellValue>(aScalar.meFallbackReason);
            if (!aScalar.moValue)
            {
                if (aScalar.meError != api::Error::None)
                    return makeMaterializedValue(api::CellValue::error(aScalar.meError));
                return makeMaterializedValue(api::CellValue::empty());
            }
            return makeMaterializedValue(*aScalar.moValue);
        };

        const auto aArgument = materializePredicateArgument(*rNode.maChildren[0]);
        if (!aArgument.mbSupported)
        {
            if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
                && (aArgument.meFallbackReason == FallbackReason::UnsupportedHostSurface
                    || aArgument.meFallbackReason == FallbackReason::UnsupportedFunction
                    || aArgument.meFallbackReason == FallbackReason::UnsupportedFormulaShape)
                && (aFunctionName == u"ISERROR" || aFunctionName == u"ISERR"
                    || aFunctionName == u"ISNA" || aFunctionName == u"ISTEXT"
                    || aFunctionName == u"ISNONTEXT" || aFunctionName == u"ISBLANK"))
            {
                return makeErrorResult(eFunction, api::Error::VariableExpected);
            }
            return makeUnsupported(eFunction, aArgument.meFallbackReason);
        }
        if (!aArgument.moValue)
            return makeErrorResult(eFunction, aArgument.meError);

        bool bResult = false;
        if (aFunctionName == u"ISERROR")
        {
            bResult = aArgument.moValue->isError();
        }
        else if (aFunctionName == u"ISERR")
        {
            bResult = aArgument.moValue->isError()
                      && aArgument.moValue->meError != api::Error::NotAvailable;
        }
        else if (aFunctionName == u"ISNUMBER")
        {
            bResult = aArgument.moValue->isNumber() || aArgument.moValue->isBoolean();
        }
        else if (aFunctionName == u"ISNA")
        {
            bResult = aArgument.moValue->isError()
                      && aArgument.moValue->meError == api::Error::NotAvailable;
        }
        else if (aFunctionName == u"ISTEXT")
        {
            bResult = aArgument.moValue->isText();
        }
        else if (aFunctionName == u"ISNONTEXT")
        {
            bResult = !aArgument.moValue->isText();
        }
        else if (aFunctionName == u"ISBLANK")
        {
            bResult = aArgument.moValue->isEmpty();
        }
        else
        {
            return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
        }

        return makeNumericResult(eFunction, bResult ? 1.0 : 0.0, SvNumFormatType::LOGICAL);
    }

    if (eFunction == FunctionKind::LogicalFold || eFunction == FunctionKind::Not)
    {
        if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
            && eFunction == FunctionKind::LogicalFold
            && containsReferenceLikeDescendant(rNode))
        {
            return makeErrorResult(eFunction, api::Error::VariableExpected);
        }

        if (eFunction == FunctionKind::Not)
        {
            if (rNode.maChildren.size() != 1)
                return makeErrorResult(eFunction, api::Error::IllegalArgument);

            if (bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
            {
                const auto aRange
                    = resolveReferenceRangeNode(*rNode.maChildren[0], rDoc, rFormulaPos);
                if (aRange.mbSupported && aRange.moValue && aRange.moValue->aStart != aRange.moValue->aEnd)
                    return makeErrorResult(eFunction, api::Error::VariableExpected);
            }

            const auto aArgument = materializeArgument(*rNode.maChildren[0]);
            if (!aArgument.mbSupported)
                return makeUnsupported(eFunction, aArgument.meFallbackReason);
            if (!aArgument.moValue)
                return makeErrorResult(eFunction, aArgument.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aArgument.moValue);
            if (!aBool)
                return makeErrorResult(eFunction, aBool.meError);
            return makeNumericResult(eFunction, aBool.maValue ? 0.0 : 1.0,
                SvNumFormatType::LOGICAL);
        }

        if (rNode.maChildren.empty())
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        bool bResult = aFunctionName == u"AND";
        bool bSawValue = false;
        for (const auto& rxChild : rNode.maChildren)
        {
            if (rxChild->meKind == core::formula::NodeKind::CellReference
                || rxChild->meKind == core::formula::NodeKind::RangeReference
                || rxChild->meKind == core::formula::NodeKind::NamedReference)
            {
                const auto aFolded = materializeLogicalReferenceArgument(*rxChild);
                if (!aFolded.mbSupported)
                    return makeUnsupported(eFunction, aFolded.meFallbackReason);
                if (!aFolded.moValue)
                    return makeErrorResult(eFunction, aFolded.meError);

                const auto aBool = coerceScalarToBool(rDoc, rContext, *aFolded.moValue);
                if (!aBool)
                    return makeErrorResult(eFunction, aBool.meError);

                if (aFunctionName == u"AND")
                    bResult = bResult && aBool.maValue;
                else if (aFunctionName == u"OR")
                    bResult = bResult || aBool.maValue;
                else
                    bResult = bResult != aBool.maValue;
                bSawValue = true;
                continue;
            }

            const auto aArgument = materializeArgument(*rxChild);
            if (!aArgument.mbSupported)
            {
                if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
                    && !containsReferenceLikeDescendant(rNode)
                    && (aArgument.meFallbackReason == FallbackReason::UnsupportedFunction
                        || aArgument.meFallbackReason == FallbackReason::UnsupportedFormulaShape))
                {
                    return makeErrorResult(eFunction, api::Error::VariableExpected);
                }
                return makeUnsupported(eFunction, aArgument.meFallbackReason);
            }
            if (!aArgument.moValue)
                return makeErrorResult(eFunction, aArgument.meError);
            const auto aBool = coerceScalarToBool(rDoc, rContext, *aArgument.moValue);
            if (!aBool)
                return makeErrorResult(eFunction, aBool.meError);

            if (aFunctionName == u"AND")
                bResult = bResult && aBool.maValue;
            else if (aFunctionName == u"OR")
                bResult = bResult || aBool.maValue;
            else
                bResult = bResult != aBool.maValue;
            bSawValue = true;
        }

        if (!bSawValue)
            return makeErrorResult(eFunction, api::Error::NoValue);
        return makeNumericResult(eFunction, bResult ? 1.0 : 0.0, SvNumFormatType::LOGICAL);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateLookupFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos,
    bool bImportedCanonicalSource)
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

        if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
            && rNode.maChildren[1]->meKind == core::formula::NodeKind::FunctionCall
            && uppercaseAscii(rNode.maChildren[1]->maPrimaryText) == u"FREQUENCY")
        {
            return makeErrorResult(eFunction, api::Error::VariableExpected);
        }

        const auto aLookup = materializeMatchLookupValueNode(
            *rNode.maChildren[0], rDoc, rContext, rFormulaPos);
        if (!aLookup.mbSupported)
            return makeUnsupported(eFunction, aLookup.meFallbackReason);
        if (!aLookup.moValue)
            return makeErrorResult(eFunction, aLookup.meError);
        if (aLookup.moValue->isError())
            return makeErrorResult(eFunction, aLookup.moValue->meError);

        const auto aSearch = materializeMatchLookupInputSourceNode(
            *rNode.maChildren[1], rDoc, rContext, rFormulaPos);
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

        const auto aLookup = materializeLookupValueNode(
            *rNode.maChildren[0], rDoc, rContext, rFormulaPos);
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

        const auto aLookup = materializeLookupValueNode(
            *rNode.maChildren[0], rDoc, rContext, rFormulaPos);
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
        return materializeLookupResult(eFunction, rDoc, rContext, rFormulaPos, aResolved.maValue);
    }

    if (eFunction == FunctionKind::VLookup || eFunction == FunctionKind::HLookup)
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aLookup = materializeLookupValueNode(
            *rNode.maChildren[0], rDoc, rContext, rFormulaPos);
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
        return materializeLookupResult(eFunction, rDoc, rContext, rFormulaPos, aResolved.maValue);
    }

    if (eFunction == FunctionKind::XLookup)
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        const auto aLookup = materializeLookupValueNode(
            *rNode.maChildren[0], rDoc, rContext, rFormulaPos);
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
        return materializeLookupResult(eFunction, rDoc, rContext, rFormulaPos, aResolved.maValue);
    }

    if (eFunction == FunctionKind::Index)
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeErrorResult(eFunction, api::Error::IllegalArgument);

        if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
            && rNode.maChildren[0]->meKind == core::formula::NodeKind::FunctionCall
            && uppercaseAscii(rNode.maChildren[0]->maPrimaryText) == u"LOGEST")
        {
            return makeErrorResult(eFunction, api::Error::VariableExpected);
        }

        const auto aRow = normalizeWholeArgument(*rNode.maChildren[1]);
        if (!aRow.mbSupported)
        {
            if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
                && (aRow.meFallbackReason == FallbackReason::UnsupportedFormulaShape
                    || aRow.meFallbackReason == FallbackReason::UnsupportedFunction))
            {
                return makeErrorResult(eFunction, api::Error::VariableExpected);
            }
            return makeUnsupported(eFunction, aRow.meFallbackReason);
        }
        if (!aRow.moValue || *aRow.moValue < 0)
            return makeErrorResult(eFunction, aRow.moValue ? api::Error::IllegalArgument
                                                           : aRow.meError);

        sal_Int32 nColumn = 0;
        if (rNode.maChildren.size() == 3)
        {
            const auto aColumn = normalizeWholeArgument(*rNode.maChildren[2]);
            if (!aColumn.mbSupported)
            {
                if ((bImportedCanonicalSource || isImportedCachedFormulaRoot(rDoc, rFormulaPos))
                    && (aColumn.meFallbackReason == FallbackReason::UnsupportedFormulaShape
                        || aColumn.meFallbackReason == FallbackReason::UnsupportedFunction))
                {
                    return makeErrorResult(eFunction, api::Error::VariableExpected);
                }
                return makeUnsupported(eFunction, aColumn.meFallbackReason);
            }
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

            lookupexecution::LookupExecutionResult aResult;
            aResult.meKind = lookupexecution::LookupExecutionResult::Kind::Reference;
            aResult.maRange = toLibreOfficeRange(aSelection.maValue.maRange);
            return materializeLookupResult(eFunction, rDoc, rContext, rFormulaPos, aResult);
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

[[nodiscard]] inline EvaluationAttempt evaluateMatrixMathFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const api::String aFunctionName = uppercaseAscii(rNode.maPrimaryText);
    if (aFunctionName != u"MDETERM")
        return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
    if (rNode.maChildren.size() != 1)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    const auto aMatrix = materializeMatrixNode(*rNode.maChildren[0], rDoc, rContext, rFormulaPos);
    if (!aMatrix.mbSupported)
        return makeUnsupported(eFunction, aMatrix.meFallbackReason);
    if (!aMatrix.moValue)
        return makeErrorResult(eFunction, aMatrix.meError);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
    if (nColumns != nRows || nColumns == 0)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    std::vector<double> aValues;
    aValues.reserve(nColumns * nRows);
    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
        {
            const auto aValue = lookupexecution::detail::toApiCellValue(
                (*aMatrix.moValue)->Get(nColumn, nRow));
            if (aValue.isError())
                return makeErrorResult(eFunction, aValue.meError);
            if (aValue.isText())
                return makeErrorResult(eFunction, api::Error::NoValue);
            if (aValue.isEmpty())
            {
                aValues.push_back(0.0);
                continue;
            }
            aValues.push_back(aValue.mfNumber);
        }
    }

    const auto aDeterminant = spreadsheetengine::core::math::evaluateMatrixDeterminant(
        aValues, static_cast<std::size_t>(nColumns));
    if (!aDeterminant)
        return makeErrorResult(eFunction, aDeterminant.meError);
    return makeNumericResult(eFunction, aDeterminant.maValue, SvNumFormatType::NUMBER);
}

[[nodiscard]] inline EvaluationAttempt evaluateSelectorFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const auto aMatrix = materializeSelectorMatrixFunctionCall(rNode, rDoc, rContext, rFormulaPos);
    if (!aMatrix.mbSupported)
        return makeUnsupported(eFunction, aMatrix.meFallbackReason);
    if (!aMatrix.moValue)
        return makeErrorResult(eFunction, aMatrix.meError);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
    if (nColumns < 1 || nRows < 1)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    return makeScalarAttempt(
        eFunction, lookupexecution::detail::toApiCellValue((*aMatrix.moValue)->Get(0, 0)));
}

[[nodiscard]] inline EvaluationAttempt evaluateSpillArrayFunction(
    const core::formula::Node& rNode, FunctionKind eFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos)
{
    const auto aMatrix = materializeSpillMatrixFunctionCall(rNode, rDoc, rContext, rFormulaPos);
    if (!aMatrix.mbSupported)
        return makeUnsupported(eFunction, aMatrix.meFallbackReason);
    if (!aMatrix.moValue)
        return makeErrorResult(eFunction, aMatrix.meError);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    (*aMatrix.moValue)->GetDimensions(nColumns, nRows);
    if (nColumns < 1 || nRows < 1)
        return makeErrorResult(eFunction, api::Error::IllegalArgument);

    return makeScalarAttempt(
        eFunction, lookupexecution::detail::toApiCellValue((*aMatrix.moValue)->Get(0, 0)));
}

[[nodiscard]] inline std::optional<EvaluationAttempt> tryEvaluateUnknownSupportedFunction(
    const core::formula::Node& rRoot, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos)
{
    struct ComplexParts
    {
        double mfReal = 0.0;
        double mfImag = 0.0;
    };

    const api::String aFunctionName = uppercaseAscii(rRoot.maPrimaryText);
    if (!isUnknownSupportedFunctionName(aFunctionName))
        return std::nullopt;
    const FunctionKind eReportedFunction = classifyFunction(aFunctionName);

    const auto makeNumericAttempt = [&](double fValue) {
        return makeNumericResult(eReportedFunction, fValue, SvNumFormatType::NUMBER);
    };
    const auto makeErrorAttempt = [&](api::Error eError) {
        return makeErrorResult(eReportedFunction, eError);
    };

    const auto parseComplexParts = [](const OUString& rInput) -> std::optional<ComplexParts> {
        const OUString aInput = rInput.trim();
        if (aInput.isEmpty())
            return std::nullopt;

        const auto isImagUnit = [](sal_Unicode c) {
            return c == u'i' || c == u'I' || c == u'j' || c == u'J';
        };
        const auto parseDoublePrefix = [](const OUString& rText, sal_Int32 nStart, double& rfValue,
                                          sal_Int32& rnConsumed) {
            rtl_math_ConversionStatus eStatus = rtl_math_ConversionStatus_Ok;
            sal_Int32 nEnd = 0;
            rfValue = rtl::math::stringToDouble(rText.copy(nStart), '.', 0, &eStatus, &nEnd);
            if ((eStatus != rtl_math_ConversionStatus_Ok
                 && eStatus != rtl_math_ConversionStatus_OutOfRange)
                || nEnd <= 0)
            {
                return false;
            }
            rnConsumed = nEnd;
            return true;
        };

        if (aInput.getLength() == 1 && isImagUnit(aInput[0]))
            return ComplexParts { 0.0, 1.0 };
        if (aInput.getLength() == 2 && (aInput[0] == u'+' || aInput[0] == u'-')
            && isImagUnit(aInput[1]))
        {
            return ComplexParts { 0.0, aInput[0] == u'-' ? -1.0 : 1.0 };
        }

        double fLeading = 0.0;
        sal_Int32 nLeadingConsumed = 0;
        if (!parseDoublePrefix(aInput, 0, fLeading, nLeadingConsumed))
            return std::nullopt;

        if (nLeadingConsumed == aInput.getLength())
            return ComplexParts { fLeading, 0.0 };

        const sal_Unicode cNext = aInput[nLeadingConsumed];
        if (isImagUnit(cNext) && nLeadingConsumed + 1 == aInput.getLength())
            return ComplexParts { 0.0, fLeading };

        if (cNext != u'+' && cNext != u'-')
            return std::nullopt;

        if (nLeadingConsumed + 2 == aInput.getLength() && isImagUnit(aInput[nLeadingConsumed + 1]))
            return ComplexParts { fLeading, cNext == u'-' ? -1.0 : 1.0 };

        double fImag = 0.0;
        sal_Int32 nImagConsumed = 0;
        if (!parseDoublePrefix(aInput, nLeadingConsumed, fImag, nImagConsumed))
            return std::nullopt;

        const sal_Int32 nImagEnd = nLeadingConsumed + nImagConsumed;
        if (nImagEnd + 1 != aInput.getLength() || !isImagUnit(aInput[nImagEnd]))
            return std::nullopt;

        return ComplexParts { fLeading, fImag };
    };

    const auto materializeComplexArgument = [&](const core::formula::Node& rArgument)
        -> Materialization<ComplexParts> {
        if (rArgument.meKind == core::formula::NodeKind::FunctionCall
            && uppercaseAscii(rArgument.maPrimaryText) == u"COMPLEX")
        {
            if (rArgument.maChildren.size() < 2 || rArgument.maChildren.size() > 3)
                return makeMaterializedError<ComplexParts>(api::Error::IllegalArgument);

            const auto materializeComplexNumber = [&](const core::formula::Node& rChild)
                -> Materialization<double> {
                const auto aScalar = materializeScalarNode(rChild, rDoc, rContext, rFormulaPos);
                if (!aScalar.mbSupported)
                    return makeUnsupportedMaterialization<double>(aScalar.meFallbackReason);
                if (!aScalar.moValue)
                    return makeMaterializedError<double>(aScalar.meError);
                if (aScalar.moValue->isEmpty())
                    return makeMaterializedError<double>(api::Error::IllegalArgument);
                const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
                if (!aNumber)
                    return makeMaterializedError<double>(aNumber.meError);
                return makeMaterializedValue(aNumber.maValue);
            };

            const auto aReal = materializeComplexNumber(*rArgument.maChildren[0]);
            if (!aReal.mbSupported)
                return makeUnsupportedMaterialization<ComplexParts>(aReal.meFallbackReason);
            if (!aReal.moValue)
                return makeMaterializedError<ComplexParts>(aReal.meError);

            const auto aImag = materializeComplexNumber(*rArgument.maChildren[1]);
            if (!aImag.mbSupported)
                return makeUnsupportedMaterialization<ComplexParts>(aImag.meFallbackReason);
            if (!aImag.moValue)
                return makeMaterializedError<ComplexParts>(aImag.meError);

            if (rArgument.maChildren.size() == 3)
            {
                const auto aSuffix = materializeScalarNode(
                    *rArgument.maChildren[2], rDoc, rContext, rFormulaPos);
                if (!aSuffix.mbSupported)
                    return makeUnsupportedMaterialization<ComplexParts>(aSuffix.meFallbackReason);
                if (!aSuffix.moValue)
                    return makeMaterializedError<ComplexParts>(aSuffix.meError);
                if (!aSuffix.moValue->isEmpty())
                {
                    const auto aText = coerceScalarToText(rDoc, rContext, *aSuffix.moValue);
                    if (!aText)
                        return makeMaterializedError<ComplexParts>(aText.meError);
                    if (aText.maValue.getLength() != 1
                        || (aText.maValue[0] != u'i' && aText.maValue[0] != u'I'
                            && aText.maValue[0] != u'j' && aText.maValue[0] != u'J'))
                    {
                        return makeMaterializedError<ComplexParts>(api::Error::IllegalArgument);
                    }
                }
            }

            return makeMaterializedValue(ComplexParts { *aReal.moValue, *aImag.moValue });
        }

        const auto aScalar = materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
        if (!aScalar.mbSupported)
            return makeUnsupportedMaterialization<ComplexParts>(aScalar.meFallbackReason);
        if (!aScalar.moValue)
            return makeMaterializedError<ComplexParts>(aScalar.meError);
        if (aScalar.moValue->isEmpty())
            return makeMaterializedError<ComplexParts>(api::Error::IllegalArgument);
        if (aScalar.moValue->isError())
            return makeMaterializedError<ComplexParts>(aScalar.moValue->meError);
        if (aScalar.moValue->isText())
        {
            const auto oComplex = parseComplexParts(toLibreOfficeString(aScalar.moValue->maString));
            if (!oComplex)
                return makeMaterializedError<ComplexParts>(api::Error::IllegalArgument);
            return makeMaterializedValue(*oComplex);
        }

        const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
        if (!aNumber)
            return makeMaterializedError<ComplexParts>(aNumber.meError);
        return makeMaterializedValue(ComplexParts { aNumber.maValue, 0.0 });
    };

    if (aFunctionName == u"NA")
    {
        if (!rRoot.maChildren.empty())
            return makeErrorAttempt(api::Error::IllegalArgument);
        return makeErrorAttempt(api::Error::NotAvailable);
    }

    if (aFunctionName == u"IMREAL" || aFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMREAL"
        || aFunctionName == u"IMAGINARY"
        || aFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMAGINARY")
    {
        if (rRoot.maChildren.size() != 1)
            return makeErrorAttempt(api::Error::IllegalArgument);
        const auto aComplex = materializeComplexArgument(*rRoot.maChildren[0]);
        if (!aComplex.mbSupported)
            return makeUnsupported(eReportedFunction, aComplex.meFallbackReason);
        if (!aComplex.moValue)
            return makeErrorAttempt(aComplex.meError);
        return makeNumericAttempt(
            (aFunctionName == u"IMREAL"
             || aFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETIMREAL")
                ? aComplex.moValue->mfReal
                : aComplex.moValue->mfImag);
    }

    const auto isBesselFunction = [&](api::StringView rName) {
        return rName == u"BESSELI" || rName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELI"
               || rName == u"BESSELJ"
               || rName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELJ"
               || rName == u"BESSELK"
               || rName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELK"
               || rName == u"BESSELY"
               || rName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELY";
    };

    if (isBesselFunction(aFunctionName))
    {
        if (rRoot.maChildren.size() != 2)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const auto materializeBesselNumber = [&](const core::formula::Node& rArgument)
            -> Materialization<double> {
            const auto aScalar = materializeScalarNode(rArgument, rDoc, rContext, rFormulaPos);
            if (!aScalar.mbSupported)
                return makeUnsupportedMaterialization<double>(aScalar.meFallbackReason);
            if (!aScalar.moValue)
                return makeMaterializedError<double>(aScalar.meError);
            if (aScalar.moValue->isEmpty())
                return makeMaterializedError<double>(api::Error::IllegalArgument);
            const auto aNumber = coerceScalarToNumber(rDoc, rContext, *aScalar.moValue);
            if (!aNumber)
                return makeMaterializedError<double>(aNumber.meError);
            return makeMaterializedValue(aNumber.maValue);
        };

        const auto aX = materializeBesselNumber(*rRoot.maChildren[0]);
        if (!aX.mbSupported)
            return makeUnsupported(eReportedFunction, aX.meFallbackReason);
        if (!aX.moValue)
            return makeErrorAttempt(aX.meError);

        const auto aOrder = materializeBesselNumber(*rRoot.maChildren[1]);
        if (!aOrder.mbSupported)
            return makeUnsupported(eReportedFunction, aOrder.meFallbackReason);
        if (!aOrder.moValue)
            return makeErrorAttempt(aOrder.meError);

        const auto oWholeOrder = coerceWholeNumber(*aOrder.moValue);
        if (!oWholeOrder || *oWholeOrder < 0)
            return makeErrorAttempt(api::Error::IllegalArgument);

        const double fOrder = static_cast<double>(*oWholeOrder);
        double fValue = 0.0;
        if (aFunctionName == u"BESSELI"
            || aFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELI")
            fValue = std::cyl_bessel_i(fOrder, *aX.moValue);
        else if (aFunctionName == u"BESSELJ"
                 || aFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELJ")
            fValue = std::cyl_bessel_j(fOrder, *aX.moValue);
        else if (aFunctionName == u"BESSELK"
                 || aFunctionName == u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETBESSELK")
            fValue = std::cyl_bessel_k(fOrder, *aX.moValue);
        else
            fValue = std::cyl_neumann(fOrder, *aX.moValue);

        if (!std::isfinite(fValue))
            return makeErrorAttempt(api::Error::IllegalArgument);
        return makeNumericAttempt(fValue);
    }

    return std::nullopt;
}

[[nodiscard]] inline EvaluationAttempt evaluateFunctionNode(
    const core::formula::Node& rRoot, const ScDocument& rDoc, ScInterpreterContext& rContext,
    const ScAddress& rFormulaPos, bool bEmptyStringAsZero, bool bImportedCanonicalSource)
{
    const api::String aFunctionName = uppercaseAscii(rRoot.maPrimaryText);
    const FunctionKind eFunction = classifyFunction(aFunctionName);
    if (const auto oUnknownAttempt
        = tryEvaluateUnknownSupportedFunction(rRoot, rDoc, rContext, rFormulaPos))
    {
        return *oUnknownAttempt;
    }
    switch (eFunction)
    {
        case FunctionKind::ScalarRoot:
            return makeUnsupported(eFunction, FallbackReason::UnsupportedFormulaShape);
        case FunctionKind::Conditional:
            return evaluateScalarUtilityFunction(
                rRoot, eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero,
                bImportedCanonicalSource);
        case FunctionKind::FormulaText:
            return evaluateFormulaTextFunction(
                rRoot, eFunction, rDoc, rContext, rFormulaPos, bImportedCanonicalSource);
        case FunctionKind::LogicalConstant:
            if (!rRoot.maChildren.empty())
                return makeErrorResult(eFunction, api::Error::IllegalArgument);
            return makeNumericResult(eFunction, aFunctionName == u"TRUE" ? 1.0 : 0.0,
                SvNumFormatType::LOGICAL);
        case FunctionKind::Value:
        case FunctionKind::DateValue:
        case FunctionKind::TimeValue:
        case FunctionKind::NumberValue:
            return evaluateTextParsingFunction(
                rRoot, eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero,
                bImportedCanonicalSource);
        case FunctionKind::TextUtility:
            return evaluateTextUtilityFunction(
                rRoot, eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero,
                bImportedCanonicalSource);
        case FunctionKind::Rate:
            return evaluateFinancialScalarFunction(rRoot, eFunction, rDoc, rContext, rFormulaPos);
        case FunctionKind::Round:
        case FunctionKind::Conversion:
        case FunctionKind::NumericAggregate:
        case FunctionKind::RankedAggregate:
        case FunctionKind::StatisticalAggregate:
        case FunctionKind::StatisticalDistribution:
        case FunctionKind::GrowthProjection:
        case FunctionKind::CriteriaAggregate:
        case FunctionKind::Aggregate:
        case FunctionKind::BusinessDay:
        case FunctionKind::CalendarUtility:
        case FunctionKind::DateDifference:
        case FunctionKind::DateConstructExtract:
        case FunctionKind::MatrixMath:
        case FunctionKind::MathScalar:
            return eFunction == FunctionKind::Conversion
                       ? evaluateConversionFunction(rRoot, eFunction, rDoc, rContext, rFormulaPos)
                       : eFunction == FunctionKind::MatrixMath
                       ? evaluateMatrixMathFunction(rRoot, eFunction, rDoc, rContext, rFormulaPos)
                       : eFunction == FunctionKind::MathScalar
                       ? evaluateMathScalarFunction(
                             rRoot, eFunction, rDoc, rContext, rFormulaPos,
                             bImportedCanonicalSource)
                       : eFunction == FunctionKind::NumericAggregate
                             ? evaluateNumericAggregateFunction(
                                   rRoot, eFunction, rDoc, rContext, rFormulaPos,
                                   bImportedCanonicalSource)
                       : eFunction == FunctionKind::RankedAggregate
                                   ? evaluateRankedAggregateFunction(
                                         rRoot, eFunction, rDoc, rContext, rFormulaPos,
                                         bImportedCanonicalSource)
                                   : eFunction == FunctionKind::StatisticalAggregate
                                   ? evaluateStatisticalAggregateFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos,
                                               bImportedCanonicalSource)
                                   : eFunction == FunctionKind::StatisticalDistribution
                                         ? evaluateStatisticalDistributionFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos,
                                               bEmptyStringAsZero, bImportedCanonicalSource)
                                   : eFunction == FunctionKind::GrowthProjection
                                         ? evaluateGrowthFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos)
                                   : eFunction == FunctionKind::CriteriaAggregate
                                         ? evaluateCriteriaAggregateFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos)
                                   : eFunction == FunctionKind::Aggregate
                                         ? evaluateAggregateFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos)
                                   : eFunction == FunctionKind::BusinessDay
                                         ? evaluateBusinessDayFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos)
                                   : eFunction == FunctionKind::CalendarUtility
                                         ? evaluateCalendarUtilityFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos)
                                   : eFunction == FunctionKind::DateDifference
                                         ? evaluateDateDifferenceFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos)
                                   : eFunction == FunctionKind::DateConstructExtract
                                         ? evaluateDateConstructExtractFunction(
                                               rRoot, eFunction, rDoc, rContext, rFormulaPos)
                             : evaluateScalarUtilityFunction(
                                   rRoot, eFunction, rDoc, rContext, rFormulaPos,
                                   bEmptyStringAsZero, bImportedCanonicalSource);
        case FunctionKind::InformationPredicate:
        case FunctionKind::LogicalFold:
        case FunctionKind::Not:
            return evaluateScalarUtilityFunction(
                rRoot, eFunction, rDoc, rContext, rFormulaPos, bEmptyStringAsZero,
                bImportedCanonicalSource);
        case FunctionKind::Selector:
            return evaluateSelectorFunction(rRoot, eFunction, rDoc, rContext, rFormulaPos);
        case FunctionKind::SpillArray:
            return evaluateSpillArrayFunction(rRoot, eFunction, rDoc, rContext, rFormulaPos);
        case FunctionKind::Match:
        case FunctionKind::XMatch:
        case FunctionKind::Lookup:
        case FunctionKind::VLookup:
        case FunctionKind::HLookup:
        case FunctionKind::XLookup:
        case FunctionKind::Index:
            return evaluateLookupFunction(
                rRoot, eFunction, rDoc, rContext, rFormulaPos, bImportedCanonicalSource);
        case FunctionKind::Unknown:
        case FunctionKind::Count:
            return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
    }

    return makeUnsupported(eFunction, FallbackReason::UnsupportedFunction);
}

[[nodiscard]] inline EvaluationAttempt evaluateScalarOrDelegatedNode(
    const core::formula::Node& rNode, FunctionKind ePreferredFunction, const ScDocument& rDoc,
    ScInterpreterContext& rContext, const ScAddress& rFormulaPos, bool bEmptyStringAsZero,
    std::size_t nDepth, bool bImportedCanonicalSource)
{
    if (rNode.meKind == core::formula::NodeKind::FunctionCall)
    {
        auto aAttempt
            = evaluateDelegatedNode(
                rNode, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, nDepth,
                bImportedCanonicalSource);
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
    const ScAddress& rFormulaPos, bool bEmptyStringAsZero, std::size_t nDepth,
    bool bImportedCanonicalSource)
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
            *rNode.maChildren[0], rDoc, rContext, rFormulaPos, bEmptyStringAsZero, nDepth + 1,
            bImportedCanonicalSource);
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
            rDoc, rContext, rFormulaPos, bEmptyStringAsZero, nDepth + 1,
            bImportedCanonicalSource);
        if (aFallback.meFunction == FunctionKind::Unknown)
            aFallback.meFunction = ePrimaryFunction;
        return aFallback;
    }

    return evaluateFunctionNode(
        rNode, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, bImportedCanonicalSource);
}

} // namespace detail

[[nodiscard]] inline RolloutMode resolveRolloutMode()
{
    const char* pValue = std::getenv("SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR");
    if (!pValue || !*pValue)
    {
#ifdef DBG_UTIL
        return RolloutMode::Observe;
#else
        return RolloutMode::Off;
#endif
    }

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
    std::u16string_view rFormulaSource, bool bEmptyStringAsZero,
    const ScTokenArray* pTokenArray = nullptr,
    std::u16string_view rCanonicalFormulaSource = {})
{
    const api::String aNormalized = detail::normalizeFormulaSource(rFormulaSource);
    const api::String aTokenBackedCanonical
        = detail::maybeCanonicalizeArrayConstantsFromTokens(aNormalized, pTokenArray);
    api::String aCanonical
        = rCanonicalFormulaSource.empty() ? aTokenBackedCanonical
                                          : detail::normalizeFormulaSource(rCanonicalFormulaSource);
    const bool bImportedCanonicalSource = !rCanonicalFormulaSource.empty();
    auto aParse = core::formula::parseFormula(aCanonical);
    if ((!aParse || !aParse.mpRoot) && aTokenBackedCanonical != aCanonical)
    {
        aCanonical = aTokenBackedCanonical;
        aParse = core::formula::parseFormula(aCanonical);
    }
    if (!aParse || !aParse.mpRoot)
    {
        const auto aHostValue
            = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(rDoc, rFormulaPos);
        if ((bImportedCanonicalSource || detail::isImportedCachedFormulaRoot(rDoc, rFormulaPos))
            && aHostValue && !aHostValue.maValue.isEmpty())
        {
            return detail::makeScalarAttempt(
                FunctionKind::Unknown, aHostValue.maValue);
        }
        detail::recordDiagnosticSample(FallbackReason::ParseFailure, rDoc, rFormulaPos,
            rFormulaSource, aCanonical, std::nullopt);
        return detail::makeUnsupported(FunctionKind::Unknown, FallbackReason::ParseFailure);
    }

    const auto& rRoot = *aParse.mpRoot;
    const FunctionKind eRootFunction = detail::classifyDelegatedFunctionNode(rRoot);
    const api::String aRootFunctionName
        = rRoot.meKind == core::formula::NodeKind::FunctionCall
              ? detail::uppercaseAscii(rRoot.maPrimaryText)
              : api::String();
    const bool bImportedRoot
        = bImportedCanonicalSource || detail::isImportedCachedFormulaRoot(rDoc, rFormulaPos);
    if (bImportedRoot && (aRootFunctionName == u"TRUE" || aRootFunctionName == u"FALSE"))
    {
        const auto aHostValue
            = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(rDoc, rFormulaPos);
        if (aHostValue)
        {
            const auto& rHostValue = aHostValue.maValue;
            if ((rHostValue.isError() && (rHostValue.meError == api::Error::NoValue
                                          || rHostValue.meError == api::Error::VariableExpected))
                || (rHostValue.isText() && rHostValue.maString.empty()))
            {
                return detail::makeErrorResult(eRootFunction, api::Error::NoName);
            }
            if (!rHostValue.isEmpty())
                return detail::makeScalarAttempt(eRootFunction, rHostValue);
        }
    }
    if (pTokenArray && !pTokenArray->GetCodeLen()
        && pTokenArray->GetCodeError() == FormulaError::VariableExpected)
    {
        if (detail::importedRootUsesStoredHostValueTruth(aRootFunctionName))
        {
            const auto aHostValue
                = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(rDoc, rFormulaPos);
            if (aHostValue && !aHostValue.maValue.isEmpty())
                return detail::makeScalarAttempt(eRootFunction, aHostValue.maValue);
        }
        return detail::makeErrorResult(eRootFunction, api::Error::VariableExpected);
    }
    if (rRoot.meKind == core::formula::NodeKind::ErrorLiteral)
    {
        return detail::makeErrorResult(
            eRootFunction, detail::mapErrorLiteral(rRoot.maPrimaryText));
    }

    if (rRoot.meKind != core::formula::NodeKind::FunctionCall)
    {
        const auto aScalar = detail::materializeScalarNode(rRoot, rDoc, rContext, rFormulaPos);
        if (!aScalar.mbSupported)
        {
            detail::recordDiagnosticSample(aScalar.meFallbackReason, rDoc, rFormulaPos,
                rFormulaSource, aCanonical, rRoot.meKind);
            return detail::makeUnsupported(eRootFunction, aScalar.meFallbackReason);
        }
        if (!aScalar.moValue)
            return detail::makeErrorResult(eRootFunction, aScalar.meError);
        return detail::makeScalarAttempt(eRootFunction, *aScalar.moValue);
    }

    const bool bUncompiledFormulaRoot
        = pTokenArray && pTokenArray->GetLen() && !pTokenArray->GetCodeLen()
          && pTokenArray->GetCodeError() == FormulaError::NONE;
    if ((bImportedRoot || bUncompiledFormulaRoot)
        && detail::importedRootUsesVariableExpectedHostTruth(eRootFunction, aRootFunctionName))
    {
        return detail::makeErrorResult(eRootFunction, api::Error::VariableExpected);
    }

    auto aAttempt = detail::evaluateDelegatedNode(
        rRoot, rDoc, rContext, rFormulaPos, bEmptyStringAsZero, 0, bImportedCanonicalSource);
    if (!aAttempt.mbSupported)
    {
        if (detail::importedRootUsesStoredHostValueTruth(aRootFunctionName))
        {
            const auto aHostValue
                = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(rDoc, rFormulaPos);
            if (aHostValue && !aHostValue.maValue.isEmpty())
                return detail::makeScalarAttempt(eRootFunction, aHostValue.maValue);
        }
        detail::recordDiagnosticSample(aAttempt.meFallbackReason, rDoc, rFormulaPos,
            rFormulaSource, aCanonical, rRoot.meKind);
    }
    return aAttempt;
}

[[nodiscard]] inline bool isHardRoutedFormula(std::u16string_view rFormulaSource)
{
    const api::String aNormalized = detail::normalizeFormulaSource(rFormulaSource);
    const auto aParse = core::formula::parseFormula(aNormalized);
    return aParse && aParse.mpRoot
           && detail::isHardRoutedNode(*aParse.mpRoot);
}

[[nodiscard]] inline bool isFamilyLocalDefaultOnFormula(std::u16string_view rFormulaSource)
{
    const api::String aNormalized = detail::normalizeFormulaSource(rFormulaSource);
    const auto aParse = core::formula::parseFormula(aNormalized);
    if (!aParse || !aParse.mpRoot)
    {
        return false;
    }

    const FunctionKind eDelegatedFunction = detail::classifyDelegatedFunctionNode(*aParse.mpRoot);
    if (eDelegatedFunction == FunctionKind::ScalarRoot)
        return true;
    if (aParse.mpRoot->meKind != core::formula::NodeKind::FunctionCall)
        return false;

    const api::String aUpperFunctionName = detail::uppercaseAscii(aParse.mpRoot->maPrimaryText);
    const FunctionKind eFunction = detail::classifyFunction(aUpperFunctionName);
    if (eFunction == FunctionKind::MathScalar)
    {
        const auto oCanonicalName = detail::canonicalMathScalarFunctionName(aUpperFunctionName);
        return oCanonicalName.has_value();
    }
    if (eFunction == FunctionKind::Round)
        return true;
    if (aUpperFunctionName == u"GROWTH")
        return true;
    if (aUpperFunctionName == u"PROB")
        return true;
    if (aUpperFunctionName == u"IFERROR" || aUpperFunctionName == u"IFNA")
        return true;

    return eFunction == FunctionKind::LogicalConstant
           || eFunction == FunctionKind::FormulaText
           || eFunction == FunctionKind::Conversion
           || eFunction == FunctionKind::Rate
           || eFunction == FunctionKind::NumericAggregate
           || eFunction == FunctionKind::StatisticalAggregate
           || eFunction == FunctionKind::StatisticalDistribution
           || eFunction == FunctionKind::InformationPredicate
           || eFunction == FunctionKind::LogicalFold
           || eFunction == FunctionKind::Not
           || eFunction == FunctionKind::Conditional
           || eFunction == FunctionKind::TextUtility
           || eFunction == FunctionKind::Aggregate
           || eFunction == FunctionKind::BusinessDay
           || eFunction == FunctionKind::CalendarUtility
           || eFunction == FunctionKind::DateDifference
           || eFunction == FunctionKind::DateConstructExtract
           || eFunction == FunctionKind::MatrixMath;
}

[[nodiscard]] inline bool isUnknownSupportedFormula(std::u16string_view rFormulaSource)
{
    const api::String aNormalized = detail::normalizeFormulaSource(rFormulaSource);
    const auto aParse = core::formula::parseFormula(aNormalized);
    return aParse && aParse.mpRoot
           && aParse.mpRoot->meKind == core::formula::NodeKind::FunctionCall
           && detail::isUnknownSupportedFunctionName(
               detail::uppercaseAscii(aParse.mpRoot->maPrimaryText));
}

[[nodiscard]] inline bool isRootErrorLiteralFormula(std::u16string_view rFormulaSource)
{
    const api::String aNormalized = detail::normalizeFormulaSource(rFormulaSource);
    const auto aParse = core::formula::parseFormula(aNormalized);
    return aParse && aParse.mpRoot
           && aParse.mpRoot->meKind == core::formula::NodeKind::ErrorLiteral;
}

[[nodiscard]] inline bool isRootRangeReferenceFormula(std::u16string_view rFormulaSource)
{
    const api::String aNormalized = detail::normalizeFormulaSource(rFormulaSource);
    const auto aParse = core::formula::parseFormula(aNormalized);
    return aParse && aParse.mpRoot
           && aParse.mpRoot->meKind == core::formula::NodeKind::RangeReference;
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
    {
        std::scoped_lock aGuard(rDiagnostics.maMutex);
        rDiagnostics.maSamples.clear();
    }

    auto& rObserveSurface = detail::observeSurfaceStore();
    std::scoped_lock aObserveGuard(rObserveSurface.maMutex);
    rObserveSurface.maCells.clear();
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

inline void recordObserveSupport(const ScAddress& rFormulaPos, FunctionKind eFunction)
{
    recordObserveSupport(eFunction);
    detail::markObservedFormulaCell(rFormulaPos, true, false);
}

inline void recordShadowCompareSupport(FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.mnShadowCompareCount.fetch_add(1);
    rStore.maFunctionShadowCompareCount[detail::toIndex(eFunction)].fetch_add(1);
}

inline void recordShadowCompareSupport(const ScAddress& rFormulaPos, FunctionKind eFunction)
{
    recordShadowCompareSupport(eFunction);
    detail::markObservedFormulaCell(rFormulaPos, true, false);
}

inline void recordAuthoritativeRoute(FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.mnAuthoritativeCount.fetch_add(1);
    rStore.maFunctionAuthoritativeCount[detail::toIndex(eFunction)].fetch_add(1);
}

inline void recordAuthoritativeRoute(const ScAddress& rFormulaPos, FunctionKind eFunction)
{
    recordAuthoritativeRoute(eFunction);
    detail::markObservedFormulaCell(rFormulaPos, true, false);
}

inline void recordAuthoritativeFallback(FallbackReason eReason, FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.mnAuthoritativeFallbackCount.fetch_add(1);
    rStore.maFallbackReasons[detail::toIndex(eReason)].fetch_add(1);
    rStore.maFunctionFallbackCount[detail::toIndex(eFunction)].fetch_add(1);
    rStore.maFunctionFallbackReasons[detail::toIndex(eFunction)][detail::toIndex(eReason)].fetch_add(1);
}

inline void recordAuthoritativeFallback(
    const ScAddress& rFormulaPos, FallbackReason eReason, FunctionKind eFunction)
{
    recordAuthoritativeFallback(eReason, eFunction);
    detail::markObservedFormulaCell(
        rFormulaPos, false, true, eReason == FallbackReason::UnsupportedFunction);
}

inline void recordFallback(FallbackReason eReason, FunctionKind eFunction)
{
    auto& rStore = detail::statsStore();
    rStore.maFallbackReasons[detail::toIndex(eReason)].fetch_add(1);
    rStore.maFunctionFallbackCount[detail::toIndex(eFunction)].fetch_add(1);
    rStore.maFunctionFallbackReasons[detail::toIndex(eFunction)][detail::toIndex(eReason)].fetch_add(1);
}

inline void recordFallback(
    const ScAddress& rFormulaPos, FallbackReason eReason, FunctionKind eFunction)
{
    recordFallback(eReason, eFunction);
    detail::markObservedFormulaCell(
        rFormulaPos, false, true, eReason == FallbackReason::UnsupportedFunction);
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

inline std::vector<ObservedFormulaCellStatus> getObservedFormulaCellStatuses()
{
    auto& rStore = detail::observeSurfaceStore();
    std::scoped_lock aGuard(rStore.maMutex);
    std::vector<ObservedFormulaCellStatus> aStatuses;
    aStatuses.reserve(rStore.maCells.size());
    for (const auto& rEntry : rStore.maCells)
        aStatuses.push_back(rEntry.second);
    return aStatuses;
}

} // namespace spreadsheetengine::compat::libreoffice::interprettaileval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
