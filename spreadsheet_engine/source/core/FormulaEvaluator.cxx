/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/FormulaEvaluator.hxx>

#include <rtl/math.hxx>

#include <spreadsheetengine/detail/BuiltinExternalNames.hxx>
#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Numeral.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/api/Text.hxx>
#include <spreadsheetengine/api/Workday.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/TextCase.hxx>
#include <spreadsheetengine/runtime/TextScalar.hxx>

#include "DateAlgorithms.hxx"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <string>

#include <unicode/coll.h>
#include <unicode/uchar.h>
#include <unicode/translit.h>
#include <unicode/unistr.h>

#include <kahan.hxx>

namespace spreadsheetengine::core::eval
{
namespace
{

namespace secompiler = spreadsheetengine::detail::compiler;
namespace semath = spreadsheetengine::core::math;
namespace sequery = spreadsheetengine::core::query;
namespace setoken = spreadsheetengine::detail::token;

[[nodiscard]] std::tuple<api::SheetId, api::ColumnIndex, api::RowIndex> makeAddressKey(
    const api::CellAddress& rAddress)
{
    return { rAddress.mnSheet, rAddress.mnColumn, rAddress.mnRow };
}

[[nodiscard]] EvaluationResult makeScalarResult(
    const api::CellValue& rValue, bool bUsedCachedValue = false)
{
    EvaluationResult aResult;
    aResult.maValue = api::CellValueView::scalar(rValue);
    aResult.mbUsedCachedValue = bUsedCachedValue;
    return aResult;
}

[[nodiscard]] EvaluationResult makeReferenceResult(const api::ResolvedReference& rReference)
{
    EvaluationResult aResult;
    aResult.maValue = api::CellValueView::matrixReference(rReference);
    return aResult;
}

[[nodiscard]] EvaluationResult makeFailure(api::Error eError)
{
    EvaluationResult aResult;
    aResult.meError = eError;
    return aResult;
}

[[nodiscard]] bool hasCachedFallbackValue(const workbook::Cell& rCell)
{
    return rCell.maValue.isNumber() || rCell.maValue.isBoolean() || rCell.maValue.isText()
           || rCell.maValue.isError();
}

[[nodiscard]] api::String uppercaseAscii(api::StringView rValue)
{
    api::String aResult;
    aResult.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar >= u'a' && cChar <= u'z')
            aResult.push_back(static_cast<char16_t>(cChar - u'a' + u'A'));
        else
            aResult.push_back(cChar);
    }
    return aResult;
}

[[nodiscard]] api::query::SearchType toQuerySearchType(workbook::FormulaSearchType eSearchType)
{
    switch (eSearchType)
    {
        case workbook::FormulaSearchType::Normal:
            return api::query::SearchType::Normal;
        case workbook::FormulaSearchType::Wildcard:
            return api::query::SearchType::Wildcard;
        case workbook::FormulaSearchType::Regex:
            return api::query::SearchType::Regex;
    }

    return api::query::SearchType::Normal;
}

[[nodiscard]] api::String fromUnicodeString(const icu::UnicodeString& rText)
{
    api::String aResult;
    aResult.resize(static_cast<std::size_t>(rText.length()));
    rText.extract(0, rText.length(), reinterpret_cast<UChar*>(aResult.data()));
    return aResult;
}

class EvaluatorCaseMappingService final : public core::text::CaseMappingService
{
public:
    api::String uppercase(api::StringView rInput) const override
    {
        icu::UnicodeString aText(
            reinterpret_cast<const UChar*>(rInput.data()), static_cast<int32_t>(rInput.size()));
        aText.toUpper();
        return fromUnicodeString(aText);
    }

    api::String lowercase(api::StringView rInput) const override
    {
        icu::UnicodeString aText(
            reinterpret_cast<const UChar*>(rInput.data()), static_cast<int32_t>(rInput.size()));
        aText.toLower();
        return fromUnicodeString(aText);
    }

    bool isLetter(char32_t nCodePoint) const override { return u_isalpha(nCodePoint); }
};

class EvaluatorEncodingService final : public core::text::SingleByteEncodingService
{
public:
    sal_Int32 encodeFirstCharacter(api::StringView rInput) const override
    {
        if (rInput.empty())
            return 0;
        return static_cast<unsigned char>(rInput.front() & 0x00FF);
    }

    std::optional<api::String> decodeSingleByte(unsigned char nValue) const override
    {
        return api::String(1, static_cast<char16_t>(nValue));
    }
};

[[nodiscard]] const EvaluatorCaseMappingService& evaluatorCaseMappingService()
{
    static const EvaluatorCaseMappingService aService;
    return aService;
}

[[nodiscard]] const EvaluatorEncodingService& evaluatorEncodingService()
{
    static const EvaluatorEncodingService aService;
    return aService;
}

[[nodiscard]] api::String propercaseText(api::StringView rValue)
{
    icu::UnicodeString aText(
        reinterpret_cast<const UChar*>(rValue.data()), static_cast<int32_t>(rValue.size()));
    aText.toLower();

    bool bNewWord = true;
    for (int32_t nOffset = 0; nOffset < aText.length();)
    {
        const UChar32 nCodePoint = aText.char32At(nOffset);
        int32_t nNextOffset = aText.moveIndex32(nOffset, 1);
        if (u_isalpha(nCodePoint))
        {
            if (bNewWord)
            {
                icu::UnicodeString aReplacement(u_totitle(nCodePoint));
                aText.replace(nOffset, nNextOffset - nOffset, aReplacement);
                nNextOffset = nOffset + aReplacement.length();
            }
            bNewWord = false;
        }
        else if (u_isdigit(nCodePoint))
        {
            bNewWord = true;
        }
        else
        {
            bNewWord = true;
        }
        nOffset = nNextOffset;
    }

    return fromUnicodeString(aText);
}

[[nodiscard]] icu::UnicodeString toUnicodeString(api::StringView rValue)
{
    return icu::UnicodeString(
        reinterpret_cast<const UChar*>(rValue.data()), static_cast<int32_t>(rValue.size()));
}

[[nodiscard]] sal_Int32 clampCodePointIndex(const icu::UnicodeString& rText, sal_Int32 nCodePointIndex)
{
    if (nCodePointIndex <= 0)
        return 0;

    const sal_Int32 nCodePointCount = rText.countChar32();
    if (nCodePointIndex >= nCodePointCount)
        return rText.length();

    return rText.moveIndex32(0, nCodePointIndex);
}

[[nodiscard]] api::String substringByCodePoints(
    api::StringView rText, sal_Int32 nCodePointStart, sal_Int32 nCodePointLength)
{
    icu::UnicodeString aText = toUnicodeString(rText);
    const sal_Int32 nStartOffset = clampCodePointIndex(aText, std::max<sal_Int32>(0, nCodePointStart));
    const sal_Int32 nEndOffset
        = clampCodePointIndex(aText, std::max<sal_Int32>(0, nCodePointStart + nCodePointLength));
    return fromUnicodeString(aText.tempSubStringBetween(nStartOffset, nEndOffset));
}

[[nodiscard]] api::String replaceByCodePoints(api::StringView rText, sal_Int32 nCodePointStart,
    sal_Int32 nCodePointLength, api::StringView rReplacement)
{
    icu::UnicodeString aText = toUnicodeString(rText);
    const sal_Int32 nStartOffset = clampCodePointIndex(aText, std::max<sal_Int32>(0, nCodePointStart));
    const sal_Int32 nEndOffset
        = clampCodePointIndex(aText, std::max<sal_Int32>(0, nCodePointStart + nCodePointLength));
    aText.replace(nStartOffset, nEndOffset - nStartOffset,
        icu::UnicodeString(reinterpret_cast<const UChar*>(rReplacement.data()),
            static_cast<int32_t>(rReplacement.size())));
    return fromUnicodeString(aText);
}

[[nodiscard]] std::optional<sal_Int32> findTextCodePointIndex(
    api::StringView rNeedle, api::StringView rHaystack, sal_Int32 nCodePointStart, bool bCaseInsensitive)
{
    icu::UnicodeString aNeedle = toUnicodeString(rNeedle);
    icu::UnicodeString aHaystack = toUnicodeString(rHaystack);
    if (bCaseInsensitive)
    {
        aNeedle.foldCase();
        aHaystack.foldCase();
    }

    const sal_Int32 nStartOffset = clampCodePointIndex(aHaystack, std::max<sal_Int32>(0, nCodePointStart));
    const sal_Int32 nFoundOffset = aHaystack.indexOf(aNeedle, nStartOffset);
    if (nFoundOffset < 0)
        return std::nullopt;

    return aHaystack.countChar32(0, nFoundOffset);
}

struct TextDelimiterMatch
{
    sal_Int32 mnCodePointIndex = 0;
    sal_Int32 mnCodePointLength = 0;
    std::size_t mnDelimiterOrder = 0;
};

[[nodiscard]] std::optional<TextDelimiterMatch> findNextTextDelimiterMatch(
    api::StringView rText, const std::vector<api::String>& rDelimiters, sal_Int32 nCodePointStart,
    bool bCaseInsensitive)
{
    std::optional<TextDelimiterMatch> oBestMatch;
    for (std::size_t nIndex = 0; nIndex < rDelimiters.size(); ++nIndex)
    {
        if (rDelimiters[nIndex].empty())
            continue;

        const auto oFound = findTextCodePointIndex(
            rDelimiters[nIndex], rText, nCodePointStart, bCaseInsensitive);
        if (!oFound)
            continue;

        const TextDelimiterMatch aCandidate {
            *oFound,
            api::text::countCodePoints(rDelimiters[nIndex]),
            nIndex,
        };
        if (!oBestMatch || aCandidate.mnCodePointIndex < oBestMatch->mnCodePointIndex
            || (aCandidate.mnCodePointIndex == oBestMatch->mnCodePointIndex
                && aCandidate.mnDelimiterOrder < oBestMatch->mnDelimiterOrder))
        {
            oBestMatch = aCandidate;
        }
    }

    return oBestMatch;
}

[[nodiscard]] std::vector<TextDelimiterMatch> collectTextDelimiterMatches(
    api::StringView rText, const std::vector<api::String>& rDelimiters, bool bCaseInsensitive)
{
    std::vector<TextDelimiterMatch> aMatches;
    sal_Int32 nSearchStart = 0;
    while (true)
    {
        const auto oMatch = findNextTextDelimiterMatch(
            rText, rDelimiters, nSearchStart, bCaseInsensitive);
        if (!oMatch)
            break;

        aMatches.push_back(*oMatch);
        nSearchStart = oMatch->mnCodePointIndex
                       + std::max<sal_Int32>(oMatch->mnCodePointLength, 1);
    }
    return aMatches;
}

[[nodiscard]] api::ValueResult<api::String> transliterateTextWidth(
    api::StringView rValue, api::StringView rTransliteratorId)
{
    static std::unique_ptr<icu::Transliterator> xFullToHalf = [] {
        UErrorCode eCreateStatus = U_ZERO_ERROR;
        return std::unique_ptr<icu::Transliterator>(icu::Transliterator::createInstance(
            icu::UnicodeString::fromUTF8("Fullwidth-Halfwidth"), UTRANS_FORWARD, eCreateStatus));
    }();
    static std::unique_ptr<icu::Transliterator> xHalfToFull = [] {
        UErrorCode eCreateStatus = U_ZERO_ERROR;
        return std::unique_ptr<icu::Transliterator>(icu::Transliterator::createInstance(
            icu::UnicodeString::fromUTF8("Halfwidth-Fullwidth"), UTRANS_FORWARD, eCreateStatus));
    }();

    icu::Transliterator* pTransliterator = nullptr;
    if (rTransliteratorId == u"Fullwidth-Halfwidth")
        pTransliterator = xFullToHalf.get();
    else if (rTransliteratorId == u"Halfwidth-Fullwidth")
        pTransliterator = xHalfToFull.get();

    if (!pTransliterator)
        return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);

    api::String aNormalizedInput;
    aNormalizedInput.reserve(rValue.size());
    if (rTransliteratorId == u"Fullwidth-Halfwidth")
    {
        for (const char16_t cChar : rValue)
        {
            if (cChar == u'\u3000')
                aNormalizedInput.push_back(u' ');
            else if (cChar == u'\u2018')
                aNormalizedInput.push_back(u'`');
            else if (cChar == u'\u2019')
                aNormalizedInput.push_back(u'\'');
            else if (cChar == u'\u201D')
                aNormalizedInput.push_back(u'"');
            else if (cChar == u'\u3002')
                aNormalizedInput.push_back(u'\uFF61');
            else if (cChar == u'\u300C')
                aNormalizedInput.push_back(u'\uFF62');
            else if (cChar == u'\u300D')
                aNormalizedInput.push_back(u'\uFF63');
            else if (cChar == u'\u3001')
                aNormalizedInput.push_back(u'\uFF64');
            else if (cChar == u'\u30FB')
                aNormalizedInput.push_back(u'\uFF65');
            else if (cChar == u'\u309B')
                aNormalizedInput.push_back(u'\uFF9E');
            else if (cChar == u'\u309C')
                aNormalizedInput.push_back(u'\uFF9F');
            else if (cChar == u'\u30FC' || cChar == u'\u2015')
                aNormalizedInput.push_back(u'\uFF70');
            else if (cChar == u'\uFFE5')
                aNormalizedInput.push_back(u'\\');
            else if (cChar >= u'\uFF01' && cChar <= u'\uFF5E')
                aNormalizedInput.push_back(static_cast<char16_t>(cChar - 0xFEE0));
            else
                aNormalizedInput.push_back(cChar);
        }
    }
    else
        aNormalizedInput.assign(rValue);

    if (rTransliteratorId == u"Fullwidth-Halfwidth")
    {
        bool bNeedsKanaTransliteration = false;
        for (const char16_t cChar : aNormalizedInput)
        {
            if ((cChar >= u'\u30A0' && cChar <= u'\u30FF')
                || (cChar >= u'\uFF61' && cChar <= u'\uFF9F'))
            {
                bNeedsKanaTransliteration = true;
                break;
            }
        }
        if (!bNeedsKanaTransliteration)
            return api::ValueResult<api::String>::success(aNormalizedInput);
    }

    icu::UnicodeString aText(reinterpret_cast<const UChar*>(aNormalizedInput.data()),
        static_cast<int32_t>(aNormalizedInput.size()));
    pTransliterator->transliterate(aText);

    api::String aResult = fromUnicodeString(aText);
    if (rTransliteratorId == u"Halfwidth-Fullwidth")
    {
        api::String aNormalizedResult;
        aNormalizedResult.reserve(aResult.size() * 2);
        for (const char16_t cOriginalChar : aResult)
        {
            char16_t cChar = cOriginalChar;
            if (cChar == u'\uFF02')
                cChar = u'\u201D';
            else if (cChar == u'\uFF07')
                cChar = u'\u2019';
            else if (cChar == u'\uFF40')
                cChar = u'\u2018';
            else if (cChar == u'\uFF3C')
                cChar = u'\uFFE5';
            if (cChar == u'\u30F4')
            {
                aNormalizedResult.push_back(u'\u30A6');
                aNormalizedResult.push_back(u'\u309B');
                continue;
            }
            if (cChar == u'\u3099')
                cChar = u'\u309B';
            else if (cChar == u'\u309A')
                cChar = u'\u309C';
            aNormalizedResult.push_back(cChar);
        }
        aResult = std::move(aNormalizedResult);
    }

    return api::ValueResult<api::String>::success(aResult);
}

[[nodiscard]] bool hasFunctionPrefix(api::StringView rName, api::StringView rPrefix)
{
    return rName.substr(0, rPrefix.size()) == rPrefix;
}

[[nodiscard]] bool usesMicrosoftCompatibilityName(api::StringView rName)
{
    return hasFunctionPrefix(rName, u"COM.MICROSOFT.");
}

[[nodiscard]] api::ValueResult<int> compareLookupText(
    api::StringView rLeft, api::StringView rRight)
{
    static std::unique_ptr<icu::Collator> xCollator = [] {
        UErrorCode eStatus = U_ZERO_ERROR;
        std::unique_ptr<icu::Collator> xInstance(
            icu::Collator::createInstance(icu::Locale(), eStatus));
        if (!xInstance || U_FAILURE(eStatus))
            return std::unique_ptr<icu::Collator>();
        xInstance->setStrength(icu::Collator::SECONDARY);
        return xInstance;
    }();

    if (!xCollator)
        return api::ValueResult<int>::failure(api::Error::IllegalArgument);

    UErrorCode eStatus = U_ZERO_ERROR;
    const icu::UnicodeString aLeft(
        reinterpret_cast<const UChar*>(rLeft.data()), static_cast<int32_t>(rLeft.size()));
    const icu::UnicodeString aRight(
        reinterpret_cast<const UChar*>(rRight.data()), static_cast<int32_t>(rRight.size()));
    const UCollationResult eCompare = xCollator->compare(aLeft, aRight, eStatus);
    if (U_FAILURE(eStatus))
        return api::ValueResult<int>::failure(api::Error::IllegalArgument);

    if (eCompare == UCOL_EQUAL)
        return api::ValueResult<int>::success(0);
    return api::ValueResult<int>::success(eCompare == UCOL_LESS ? -1 : 1);
}

[[nodiscard]] api::String normalizeDisplayFunctionName(api::StringView rName)
{
    const api::StringView aMicrosoftPrefix = u"COM.MICROSOFT.";
    const api::StringView aLibreOfficePrefix = u"ORG.LIBREOFFICE.";
    const api::StringView aOpenOfficePrefix = u"ORG.OPENOFFICE.";
    if (rName.substr(0, aMicrosoftPrefix.size()) == aMicrosoftPrefix)
        return api::String(rName.substr(aMicrosoftPrefix.size()));
    if (rName.substr(0, aLibreOfficePrefix.size()) == aLibreOfficePrefix)
        return api::String(rName.substr(aLibreOfficePrefix.size()));
    if (rName.substr(0, aOpenOfficePrefix.size()) == aOpenOfficePrefix)
        return api::String(rName.substr(aOpenOfficePrefix.size()));
    return api::String(rName);
}

[[nodiscard]] api::Error mapErrorLiteral(api::StringView rText)
{
    if (rText == u"#N/A")
        return api::Error::NotAvailable;
    if (rText == u"#DIV/0!")
        return api::Error::DivisionByZero;
    if (rText == u"#VALUE!")
        return api::Error::NoValue;
    if (rText == u"#NUM!")
        return api::Error::NoConvergence;
    if (rText == u"#NAME?" || rText == u"#REF!" || rText == u"#NULL!")
        return api::Error::IllegalArgument;
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
        {
            if (aCode == u"503" || aCode == u"523")
                return api::Error::NoConvergence;
            if (aCode == u"513")
                return api::Error::StringOverflow;
            if (aCode == u"519")
                return api::Error::NoValue;
            if (aCode == u"532")
                return api::Error::DivisionByZero;
            return api::Error::IllegalArgument;
        }
    }

    return api::Error::NoValue;
}

[[nodiscard]] std::optional<double> parseAsciiDouble(api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    std::string aAscii;
    aAscii.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar > 0x7f)
            return std::nullopt;
        aAscii.push_back(static_cast<char>(cChar));
    }

    char* pEnd = nullptr;
    const double fValue = std::strtod(aAscii.c_str(), &pEnd);
    if (!pEnd || *pEnd != '\0')
        return std::nullopt;

    return fValue;
}

[[nodiscard]] bool isAsciiWhitespace(char16_t cChar)
{
    return cChar == u' ' || cChar == u'\t' || cChar == u'\r' || cChar == u'\n';
}

[[nodiscard]] api::StringView trimAsciiWhitespace(api::StringView rValue)
{
    while (!rValue.empty() && isAsciiWhitespace(rValue.front()))
        rValue.remove_prefix(1);
    while (!rValue.empty() && isAsciiWhitespace(rValue.back()))
        rValue.remove_suffix(1);
    return rValue;
}

[[nodiscard]] std::optional<sal_Int16> parseAsciiInt16(api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    sal_Int32 nValue = 0;
    for (const char16_t cChar : rValue)
    {
        if (cChar < u'0' || cChar > u'9')
            return std::nullopt;
        nValue = (nValue * 10) + (cChar - u'0');
    }
    return static_cast<sal_Int16>(nValue);
}

[[nodiscard]] std::optional<sal_Int16> parseMonthName(api::StringView rValue)
{
    const api::String aUpper = uppercaseAscii(rValue);
    if (aUpper == u"JAN" || aUpper == u"JANUARY")
        return 1;
    if (aUpper == u"FEB" || aUpper == u"FEBRUARY")
        return 2;
    if (aUpper == u"MAR" || aUpper == u"MARCH")
        return 3;
    if (aUpper == u"APR" || aUpper == u"APRIL")
        return 4;
    if (aUpper == u"MAY")
        return 5;
    if (aUpper == u"JUN" || aUpper == u"JUNE")
        return 6;
    if (aUpper == u"JUL" || aUpper == u"JULY")
        return 7;
    if (aUpper == u"AUG" || aUpper == u"AUGUST")
        return 8;
    if (aUpper == u"SEP" || aUpper == u"SEPT" || aUpper == u"SEPTEMBER")
        return 9;
    if (aUpper == u"OCT" || aUpper == u"OCTOBER")
        return 10;
    if (aUpper == u"NOV" || aUpper == u"NOVEMBER")
        return 11;
    if (aUpper == u"DEC" || aUpper == u"DECEMBER")
        return 12;
    return std::nullopt;
}

[[nodiscard]] bool splitThreePartNumericDate(api::StringView rValue, char16_t cSeparator,
    sal_Int16& rnFirst, sal_Int16& rnSecond, sal_Int16& rnThird)
{
    const std::size_t nFirstSep = rValue.find(cSeparator);
    if (nFirstSep == api::StringView::npos)
        return false;
    const std::size_t nSecondSep = rValue.find(cSeparator, nFirstSep + 1);
    if (nSecondSep == api::StringView::npos)
        return false;

    const auto oFirst = parseAsciiInt16(rValue.substr(0, nFirstSep));
    const auto oSecond
        = parseAsciiInt16(rValue.substr(nFirstSep + 1, nSecondSep - nFirstSep - 1));
    const auto oThird = parseAsciiInt16(rValue.substr(nSecondSep + 1));
    if (!oFirst || !oSecond || !oThird)
        return false;

    rnFirst = *oFirst;
    rnSecond = *oSecond;
    rnThird = *oThird;
    return true;
}

[[nodiscard]] bool parseDateText(
    api::StringView rValue, sal_Int16& rnYear, sal_Int16& rnMonth, sal_Int16& rnDay)
{
    rValue = trimAsciiWhitespace(rValue);
    if (rValue.empty())
        return false;

    sal_Int16 nFirst = 0;
    sal_Int16 nSecond = 0;
    sal_Int16 nThird = 0;
    if (splitThreePartNumericDate(rValue, u'-', nFirst, nSecond, nThird))
    {
        rnYear = nFirst;
        rnMonth = nSecond;
        rnDay = nThird;
        return true;
    }

    if (splitThreePartNumericDate(rValue, u'/', nFirst, nSecond, nThird))
    {
        rnMonth = nFirst;
        rnDay = nSecond;
        rnYear = nThird;
        return true;
    }

    std::size_t nMonthEnd = 0;
    while (nMonthEnd < rValue.size()
           && ((rValue[nMonthEnd] >= u'A' && rValue[nMonthEnd] <= u'Z')
               || (rValue[nMonthEnd] >= u'a' && rValue[nMonthEnd] <= u'z')))
    {
        ++nMonthEnd;
    }

    if (nMonthEnd == 0)
        return false;

    const auto oMonth = parseMonthName(rValue.substr(0, nMonthEnd));
    if (!oMonth)
        return false;
    rnMonth = *oMonth;

    api::StringView aTail = trimAsciiWhitespace(rValue.substr(nMonthEnd));
    std::size_t nDayEnd = 0;
    while (nDayEnd < aTail.size() && aTail[nDayEnd] >= u'0' && aTail[nDayEnd] <= u'9')
        ++nDayEnd;
    if (nDayEnd == 0)
        return false;

    const auto oDay = parseAsciiInt16(aTail.substr(0, nDayEnd));
    if (!oDay)
        return false;
    rnDay = *oDay;

    aTail = trimAsciiWhitespace(aTail.substr(nDayEnd));
    if (!aTail.empty() && aTail.front() == u',')
        aTail.remove_prefix(1);
    aTail = trimAsciiWhitespace(aTail);

    const auto oYear = parseAsciiInt16(aTail);
    if (!oYear)
        return false;
    rnYear = *oYear;
    return true;
}

[[nodiscard]] std::optional<double> parseTimeText(api::StringView rValue)
{
    rValue = trimAsciiWhitespace(rValue);
    if (rValue.empty())
        return std::nullopt;

    bool bHasMeridiem = false;
    bool bPM = false;
    if (rValue.size() >= 2)
    {
        const api::String aSuffix = uppercaseAscii(rValue.substr(rValue.size() - 2));
        if (aSuffix == u"AM" || aSuffix == u"PM")
        {
            bHasMeridiem = true;
            bPM = aSuffix == u"PM";
            rValue = trimAsciiWhitespace(rValue.substr(0, rValue.size() - 2));
        }
    }

    const std::size_t nFirstColon = rValue.find(u':');
    if (!bHasMeridiem && nFirstColon == api::StringView::npos)
        return std::nullopt;

    sal_Int16 nHour = 0;
    sal_Int16 nMinute = 0;
    sal_Int16 nSecond = 0;
    if (nFirstColon == api::StringView::npos)
    {
        const auto oHour = parseAsciiInt16(rValue);
        if (!oHour)
            return std::nullopt;
        nHour = *oHour;
    }
    else
    {
        const auto oHour = parseAsciiInt16(rValue.substr(0, nFirstColon));
        if (!oHour)
            return std::nullopt;
        nHour = *oHour;

        const std::size_t nSecondColon = rValue.find(u':', nFirstColon + 1);
        if (nSecondColon == api::StringView::npos)
        {
            const auto oMinute = parseAsciiInt16(rValue.substr(nFirstColon + 1));
            if (!oMinute)
                return std::nullopt;
            nMinute = *oMinute;
        }
        else
        {
            const auto oMinute
                = parseAsciiInt16(rValue.substr(nFirstColon + 1, nSecondColon - nFirstColon - 1));
            const auto oSecond = parseAsciiInt16(rValue.substr(nSecondColon + 1));
            if (!oMinute || !oSecond)
                return std::nullopt;
            nMinute = *oMinute;
            nSecond = *oSecond;
        }
    }

    if (bHasMeridiem)
    {
        if (nHour < 1 || nHour > 12)
            return std::nullopt;
        if (bPM)
            nHour = nHour == 12 ? 12 : static_cast<sal_Int16>(nHour + 12);
        else
            nHour = nHour == 12 ? 0 : nHour;
    }

    const auto aTimeSerial = api::calendar::makeTimeSerial(nHour, nMinute, nSecond);
    if (!aTimeSerial)
        return std::nullopt;
    return aTimeSerial.maValue;
}

[[nodiscard]] std::optional<double> parseOdfTimeDuration(api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    bool bNegative = false;
    if (rValue.front() == u'-')
    {
        bNegative = true;
        rValue.remove_prefix(1);
    }

    if (rValue.size() < 2 || rValue[0] != u'P' || rValue[1] != u'T')
        return std::nullopt;

    rValue.remove_prefix(2);
    if (rValue.empty())
        return std::nullopt;

    double fHour = 0.0;
    double fMinute = 0.0;
    double fSecond = 0.0;
    bool bSawField = false;
    while (!rValue.empty())
    {
        std::size_t nFieldEnd = 0;
        while (nFieldEnd < rValue.size()
               && ((rValue[nFieldEnd] >= u'0' && rValue[nFieldEnd] <= u'9')
                   || rValue[nFieldEnd] == u'.'))
        {
            ++nFieldEnd;
        }
        if (nFieldEnd == 0 || nFieldEnd >= rValue.size())
            return std::nullopt;

        const auto oNumber = parseAsciiDouble(rValue.substr(0, nFieldEnd));
        if (!oNumber)
            return std::nullopt;

        switch (rValue[nFieldEnd])
        {
            case u'H':
                fHour = *oNumber;
                break;
            case u'M':
                fMinute = *oNumber;
                break;
            case u'S':
                fSecond = *oNumber;
                break;
            default:
                return std::nullopt;
        }

        bSawField = true;
        rValue.remove_prefix(nFieldEnd + 1);
    }

    if (!bSawField)
        return std::nullopt;

    if (bNegative)
    {
        fHour = -fHour;
        fMinute = -fMinute;
        fSecond = -fSecond;
    }

    const auto aTimeSerial = api::calendar::makeTimeSerial(fHour, fMinute, fSecond);
    if (!aTimeSerial)
        return std::nullopt;
    return aTimeSerial.maValue;
}

[[nodiscard]] std::optional<api::NumberParseResult> parseStandaloneNumberText(
    api::StringView rValue)
{
    constexpr api::DateParts aDefaultNullDate { 1899, 12, 30 };

    const api::StringView aTrimmed = trimAsciiWhitespace(rValue);
    if (aTrimmed.empty())
        return std::nullopt;

    if (const auto oNumber = parseAsciiDouble(aTrimmed))
        return api::NumberParseResult { *oNumber, 0, api::NumberParseResult::Kind::Number };

    if (const auto oTime = parseTimeText(aTrimmed))
        return api::NumberParseResult { *oTime, 0, api::NumberParseResult::Kind::Time };

    sal_Int16 nYear = 0;
    sal_Int16 nMonth = 0;
    sal_Int16 nDay = 0;
    if (parseDateText(aTrimmed, nYear, nMonth, nDay))
    {
        const auto aDateSerial
            = api::calendar::makeDateSerial(aDefaultNullDate, nYear, nMonth, nDay, true);
        if (!aDateSerial)
            return std::nullopt;

        return api::NumberParseResult {
            aDateSerial.maValue, 0, api::NumberParseResult::Kind::Date
        };
    }

    const std::size_t nSplitPos = aTrimmed.find_last_of(u' ');
    if (nSplitPos == api::StringView::npos)
        return std::nullopt;
    const api::StringView aDatePart = trimAsciiWhitespace(aTrimmed.substr(0, nSplitPos));
    const api::StringView aTimePart = trimAsciiWhitespace(aTrimmed.substr(nSplitPos + 1));
    if (!parseDateText(aDatePart, nYear, nMonth, nDay) || aTimePart.empty())
        return std::nullopt;

    const auto aDateSerial = api::calendar::makeDateSerial(aDefaultNullDate, nYear, nMonth, nDay, true);
    if (!aDateSerial)
        return std::nullopt;

    const auto oTimeSerial = parseTimeText(aTimePart);
    if (!oTimeSerial)
        return std::nullopt;

    return api::NumberParseResult {
        aDateSerial.maValue + *oTimeSerial, 0, api::NumberParseResult::Kind::DateTime
    };
}

[[nodiscard]] std::optional<double> parseStoredDateValue(api::StringView rValue)
{
    constexpr api::DateParts aDefaultNullDate { 1899, 12, 30 };

    rValue = trimAsciiWhitespace(rValue);
    if (rValue.empty())
        return std::nullopt;

    api::StringView aDatePart = rValue;
    api::StringView aTimePart;
    const std::size_t nTimeSeparator = rValue.find_first_of(u"T ");
    if (nTimeSeparator != api::StringView::npos)
    {
        aDatePart = trimAsciiWhitespace(rValue.substr(0, nTimeSeparator));
        aTimePart = trimAsciiWhitespace(rValue.substr(nTimeSeparator + 1));
    }

    sal_Int16 nYear = 0;
    sal_Int16 nMonth = 0;
    sal_Int16 nDay = 0;
    if (!parseDateText(aDatePart, nYear, nMonth, nDay))
        return std::nullopt;
    if (!detail::date::isValidDate(
            static_cast<sal_uInt16>(nDay), static_cast<sal_uInt16>(nMonth), nYear))
    {
        return std::nullopt;
    }

    const api::DateParts aDate { nYear, nMonth, nDay };
    double fSerial = static_cast<double>(
        detail::date::toAbsoluteDays(aDate) - detail::date::toAbsoluteDays(aDefaultNullDate));

    if (!aTimePart.empty())
    {
        const auto oTimeSerial = parseTimeText(aTimePart);
        if (!oTimeSerial)
            return std::nullopt;
        fSerial += *oTimeSerial;
    }

    return fSerial;
}

[[nodiscard]] constexpr api::DateParts defaultFodsNullDate()
{
    return { 1899, 12, 30 };
}

[[nodiscard]] std::optional<api::CellValue> parseTypedStoredCellValue(
    const workbook::Cell& rCell)
{
    if (!rCell.maValue.isText())
        return std::nullopt;

    const api::StringView aLexical = !rCell.maRawValue.empty() ? api::StringView(rCell.maRawValue)
                                                               : api::StringView(rCell.maValue.maString);
    if (rCell.maRawValueType == u"date")
    {
        if (const auto oStoredDate = parseStoredDateValue(aLexical))
            return api::CellValue::number(*oStoredDate);

        if (const auto oParsed = parseStandaloneNumberText(aLexical))
        {
            if (oParsed->meKind == api::NumberParseResult::Kind::Date
                || oParsed->meKind == api::NumberParseResult::Kind::DateTime)
            {
                return api::CellValue::number(oParsed->mfValue);
            }
        }
    }

    if (rCell.maRawValueType == u"time")
    {
        if (const auto oDuration = parseOdfTimeDuration(aLexical))
        {
            return api::CellValue::number(
                spreadsheetengine::core::datetime::normalizeTimeFraction(*oDuration));
        }

        if (const auto oParsed = parseStandaloneNumberText(aLexical))
        {
            if (oParsed->meKind == api::NumberParseResult::Kind::Time
                || oParsed->meKind == api::NumberParseResult::Kind::DateTime)
            {
                return api::CellValue::number(
                    spreadsheetengine::core::datetime::normalizeTimeFraction(oParsed->mfValue));
            }
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<api::DateSerial> coerceToDateSerial(
    const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return static_cast<api::DateSerial>(0);
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return static_cast<api::DateSerial>(rtl::math::approxFloor(rValue.mfNumber));
        case api::CellValueKind::Text:
        {
            const auto oParsed = parseStandaloneNumberText(rValue.maString);
            if (!oParsed)
                return std::nullopt;
            return static_cast<api::DateSerial>(rtl::math::approxFloor(oParsed->mfValue));
        }
        case api::CellValueKind::Error:
            return std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<double> shiftMonthSerial(
    api::DateSerial nDateSerial, sal_Int32 nMonthOffset, bool bEndOfMonth)
{
    constexpr api::DateParts aNullDate = defaultFodsNullDate();

    const sal_Int16 nYear = static_cast<sal_Int16>(
        spreadsheetengine::core::datetime::extractYear(aNullDate, nDateSerial));
    const sal_Int16 nMonth = static_cast<sal_Int16>(
        spreadsheetengine::core::datetime::extractMonth(aNullDate, nDateSerial));
    const auto aDayResult = spreadsheetengine::api::calendar::dayFromSerial(aNullDate, nDateSerial);
    if (!aDayResult)
        return std::nullopt;

    const sal_Int32 nZeroBasedMonth
        = static_cast<sal_Int32>(nYear) * 12 + static_cast<sal_Int32>(nMonth - 1) + nMonthOffset;
    if (nZeroBasedMonth < 12)
        return std::nullopt;

    const sal_Int16 nTargetYear = static_cast<sal_Int16>(nZeroBasedMonth / 12);
    const sal_Int16 nTargetMonth = static_cast<sal_Int16>((nZeroBasedMonth % 12) + 1);
    const sal_uInt16 nDaysInTargetMonth = spreadsheetengine::core::detail::date::getDaysInMonth(
        static_cast<sal_uInt16>(nTargetMonth), nTargetYear);
    const sal_Int16 nTargetDay = bEndOfMonth
                                     ? static_cast<sal_Int16>(nDaysInTargetMonth)
                                     : static_cast<sal_Int16>(std::min<double>(
                                           aDayResult.maValue, nDaysInTargetMonth));

    const auto aShifted = spreadsheetengine::api::calendar::makeDateSerial(
        aNullDate, nTargetYear, nTargetMonth, nTargetDay, true);
    if (!aShifted)
        return std::nullopt;
    return aShifted.maValue;
}

[[nodiscard]] std::optional<double> computeWeeksDifference(
    api::DateSerial nStartDate, api::DateSerial nEndDate, sal_Int16 nMode)
{
    if (nMode == 0)
        return static_cast<double>((nEndDate - nStartDate) / 7);

    if (nMode != 1)
        return std::nullopt;

    constexpr api::DateParts aEpoch { 1, 1, 1 };
    constexpr api::DateParts aNullDate = defaultFodsNullDate();
    const auto aOffset = api::calendar::makeDateSerial(
        aEpoch, aNullDate.mnYear, aNullDate.mnMonth, aNullDate.mnDay, true);
    if (!aOffset)
        return std::nullopt;

    const double fStartWeek = std::floor((nStartDate + aOffset.maValue) / 7.0);
    const double fEndWeek = std::floor((nEndDate + aOffset.maValue) / 7.0);
    return fEndWeek - fStartWeek;
}

[[nodiscard]] std::optional<int> weekdayIndexForFodsDate(api::DateSerial nDate)
{
    const auto aWeekday = api::calendar::dayOfWeek(defaultFodsNullDate(), nDate, 2);
    if (!aWeekday || aWeekday.maValue < 1 || aWeekday.maValue > 7)
        return std::nullopt;
    return aWeekday.maValue - 1;
}

[[nodiscard]] bool isWeekendFodsDate(api::DateSerial nDate, const api::WeekendMask& rWeekendMask)
{
    const auto oWeekdayIndex = weekdayIndexForFodsDate(nDate);
    return oWeekdayIndex && rWeekendMask[static_cast<std::size_t>(*oWeekdayIndex)];
}

[[nodiscard]] bool isLiteralArrayWeekendNode(const formula::Node& rNode)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::NumberLiteral:
        case formula::NodeKind::StringLiteral:
        case formula::NodeKind::BooleanLiteral:
        case formula::NodeKind::EmptyArgument:
            return true;
        case formula::NodeKind::UnaryOperation:
            return rNode.maChildren.size() == 1 && isLiteralArrayWeekendNode(*rNode.maChildren[0]);
        default:
            return false;
    }
}

[[nodiscard]] bool isHolidayFodsDate(
    api::DateSerial nDate, const std::vector<api::DateSerial>& rSortedHolidays)
{
    return std::binary_search(rSortedHolidays.begin(), rSortedHolidays.end(), nDate);
}

[[nodiscard]] api::DateSerial countWorkdaysFods(api::DateSerial nDate1, api::DateSerial nDate2,
    const std::vector<api::DateSerial>& rSortedHolidays, const api::WeekendMask& rWeekendMask)
{
    sal_Int32 nCount = 0;
    const bool bReverse = nDate1 > nDate2;
    if (bReverse)
        std::swap(nDate1, nDate2);

    while (nDate1 <= nDate2)
    {
        if (!isWeekendFodsDate(nDate1, rWeekendMask)
            && !isHolidayFodsDate(nDate1, rSortedHolidays))
        {
            ++nCount;
        }
        ++nDate1;
    }

    return bReverse ? -nCount : nCount;
}

[[nodiscard]] api::DateSerial advanceWorkdayFods(api::DateSerial nDate, api::DateSerial nDays,
    const std::vector<api::DateSerial>& rSortedHolidays, const api::WeekendMask& rWeekendMask)
{
    if (!nDays)
        return nDate;

    if (nDays > 0)
    {
        while (nDays)
        {
            do
            {
                ++nDate;
            } while (isWeekendFodsDate(nDate, rWeekendMask));

            if (!isHolidayFodsDate(nDate, rSortedHolidays))
                --nDays;
        }
    }
    else
    {
        while (nDays)
        {
            do
            {
                --nDate;
            } while (isWeekendFodsDate(nDate, rWeekendMask));

            if (!isHolidayFodsDate(nDate, rSortedHolidays))
                ++nDays;
        }
    }

    return nDate;
}

[[nodiscard]] api::String formatNumber(double fValue)
{
    char aBuffer[32];
    const int nLength = std::snprintf(aBuffer, sizeof(aBuffer), "%.17G", fValue);
    const std::string aAscii(aBuffer, static_cast<std::size_t>(std::max(nLength, 0)));

    api::String aResult;
    aResult.reserve(aAscii.size());
    for (const char cChar : aAscii)
        aResult.push_back(static_cast<char16_t>(cChar));
    return aResult;
}

[[nodiscard]] api::String formatQuotedString(api::StringView rValue)
{
    api::String aResult;
    aResult.reserve(rValue.size() + 2);
    aResult.push_back(u'"');
    for (const char16_t cChar : rValue)
    {
        if (cChar == u'"')
            aResult.push_back(u'"');
        aResult.push_back(cChar);
    }
    aResult.push_back(u'"');
    return aResult;
}

[[nodiscard]] api::ValueResult<double> coerceToNumber(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<double>::success(0.0);
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return api::ValueResult<double>::success(rValue.mfNumber);
        case api::CellValueKind::Text:
        {
            if (auto oValue = parseAsciiDouble(rValue.maString))
                return api::ValueResult<double>::success(*oValue);
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        }
        case api::CellValueKind::Error:
            return api::ValueResult<double>::failure(rValue.meError);
    }

    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

[[nodiscard]] api::ValueResult<bool> coerceToBoolean(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<bool>::success(false);
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return api::ValueResult<bool>::success(rValue.mfNumber != 0.0);
        case api::CellValueKind::Text:
        {
            const api::String aUpper = uppercaseAscii(rValue.maString);
            if (aUpper == u"TRUE")
                return api::ValueResult<bool>::success(true);
            if (aUpper == u"FALSE")
                return api::ValueResult<bool>::success(false);
            if (auto oNumber = parseAsciiDouble(rValue.maString))
                return api::ValueResult<bool>::success(*oNumber != 0.0);
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
        }
        case api::CellValueKind::Error:
            return api::ValueResult<bool>::failure(rValue.meError);
    }

    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
}

[[nodiscard]] api::ValueResult<api::String> coerceToString(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<api::String>::success({});
        case api::CellValueKind::Number:
            return api::ValueResult<api::String>::success(formatNumber(rValue.mfNumber));
        case api::CellValueKind::Boolean:
            return api::ValueResult<api::String>::success(
                rValue.mfNumber != 0.0 ? api::String(u"TRUE") : api::String(u"FALSE"));
        case api::CellValueKind::Text:
            return api::ValueResult<api::String>::success(rValue.maString);
        case api::CellValueKind::Error:
            return api::ValueResult<api::String>::failure(rValue.meError);
    }

    return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
}

struct AggregateOptions
{
    bool mbIgnoreHiddenRows = false;
    bool mbIgnoreErrors = false;
    bool mbIgnoreNestedAggregates = false;
};

struct AggregateScan
{
    std::vector<double> maNumbers;
    sal_Int32 mnNonEmptyCount = 0;
};

struct UnitConversionFactor
{
    api::StringView maFromUnit;
    api::StringView maToUnit;
    double mfFactor = 1.0;
};

struct EuroCurrencyInfo
{
    api::StringView maCode;
    double mfRate = 1.0;
    sal_Int32 mnDecimals = 2;
};

constexpr UnitConversionFactor kKnownConversions[] = {
    { u"uk_acre", u"us_acre", 0.999996000004 },
    { u"us_acre", u"ang2", 4.04687260987425E+023 },
    { u"ang2", u"ar", 1E-22 },
    { u"ar", u"ft2", 1076.39104167097 },
    { u"ft2", u"ha", 9.290304E-06 },
    { u"ha", u"in2", 15500031.000062 },
    { u"in2", u"ly2", 7.20836355779189E-36 },
    { u"ly2", u"m2", 8.9501590038784E+031 },
    { u"m2", u"Morgen", 0.0004 },
    { u"Morgen", u"mi2", 0.000965255396356 },
    { u"mi2", u"Nmi2", 0.755119708987773 },
    { u"Nmi2", u"Pica2", 27560019740839.5 },
    { u"ang", u"ell", 8.748906E-11 },
    { u"ell", u"ft", 3.75000016575001 },
    { u"ft", u"in", 12 },
    { u"in", u"ly", 2.68483957766416E-18 },
    { u"ly", u"m", 9.460528E+015 },
    { u"m", u"mi", 0.000621371192237 },
    { u"mi", u"Nmi", 0.868976241900648 },
    { u"Nmi", u"parsec", 6.001922708E-14 },
    { u"parsec", u"Pica", 8.74680337440886E+019 },
    { u"survey_mi", u"yd", 1760.00352000704 },
    { u"BTU", u"c", 252.165488508169 },
    { u"c", u"cal", 0.99933031528756 },
    { u"cal", u"e", 41867948.4613929 },
    { u"e", u"eV", 624145700000 },
    { u"eV", u"flb", 3.80206452103493E-18 },
    { u"flb", u"HPh", 1.5697407642781E-08 },
    { u"HPh", u"J", 2684519.71705162 },
    { u"J", u"Wh", 0.000277777777778 },
    { u"dyn", u"N", 1E-05 },
    { u"N", u"lbf", 0.224808923655339 },
    { u"lbf", u"pond", 453.5923144952 },
    { u"ga", u"T", 0.0001 },
    { u"g", u"grain", 15.43236 },
    { u"cwt", u"uk_cwt", 0.892857142857143 },
    { u"uk_cwt", u"lbm", 112.000014877089 },
    { u"lbm", u"stone", 0.071428541793075 },
    { u"stone", u"ton", 0.007 },
    { u"ton", u"ozm", 32000.017962592 },
    { u"ozm", u"sg", 0.001942566898708 },
    { u"sg", u"u", 8.78861184032002E+027 },
    { u"HP", u"PS", 1.0138700185381 },
    { u"PS", u"W", 735.498542977386 },
    { u"atm", u"mmHg", 760 },
    { u"mmHg", u"Pa", 133.322363925 },
    { u"Pa", u"psi", 0.0001450377 },
    { u"psi", u"Torr", 51.7150920071126 },
    { u"admkn", u"kn", 0.999999913606911 },
    { u"kn", u"m/h", 1852 },
    { u"m/h", u"m/s", 0.000277777777778 },
    { u"m/s", u"mph", 2.2369362920544 },
    { u"C", u"F", 33.8 },
    { u"F", u"K", 255.927777777778 },
    { u"K", u"Rank", 1.8 },
    { u"Rank", u"Reau", -218.075555555556 },
    { u"d", u"hr", 24 },
    { u"hr", u"mn", 60 },
    { u"mn", u"sec", 60 },
    { u"sec", u"yr", 3.16880878140289E-08 },
    { u"ang3", u"barrel", 6.28981077043211E-30 },
    { u"barrel", u"bushel", 4.51167627067586 },
    { u"bushel", u"cup", 148.946856929372 },
    { u"cup", u"ft3", 0.008355034722222 },
    { u"ft3", u"gal", 7.48051948051948 },
    { u"in3", u"l", 0.016387064 },
    { u"in3", u"gal", 0.00432900432900433 },
    { u"l", u"ly3", 1.18101081256238E-51 },
    { u"l", u"ml", 1000.0 },
    { u"ly3", u"m3", 8.46732298606437E+047 },
    { u"m3", u"yd3", 1.30795061931439 },
    { u"m3", u"mi3", 2.39912758578928E-10 },
    { u"mi3", u"Nmi3", 0.65618108690130639 },
    { u"mi3", u"MTON", 5887918080000 },
    { u"MTON", u"Nmi3", 1.1144534927043455E-13 },
    { u"Nmi3", u"oz", 214792833387555 },
    { u"oz", u"Pica3", 673596 },
    { u"pt", u"qt", 0.5 },
    { u"qt", u"tbs", 64 },
    { u"tbs", u"tsp", 3 },
    { u"tsp", u"tspm", 0.98578431875 },
    { u"tspm", u"ml", 5.0 },
    { u"tspm", u"uk_gal", 0.001099846241495 },
    { u"uk_gal", u"uk_pt", 8 },
    { u"uk_pt", u"uk_qt", 0.5 },
    { u"uk_qt", u"yd3", 0.00148651530774 },
    { u"Pica2", u"picapt2", 1 },
    { u"picapt2", u"yd2", 1.48843545191282E-07 },
    { u"Pica3", u"picapt3", 1 },
    { u"picapt3", u"pica3", 0.000578703703705 },
    { u"pica3", u"pt", 0.000160333493666 },
    { u"gal", u"GRT", 0.001336805679661 },
    { u"GRT", u"in3", 172799.98395775 },
    { u"u", u"uk_ton", 1.63431440967062E-30 },
    { u"Pica", u"pica", 0.083333333333353 },
    { u"pica", u"survey_mi", 2.63046611952801E-06 },
};

constexpr EuroCurrencyInfo kEuroCurrencies[] = {
    { u"EUR", 1.0, 2 },       { u"ATS", 13.7603, 2 },  { u"DEM", 1.95583, 2 },
    { u"BEF", 40.3399, 0 },   { u"ESP", 166.386, 0 },  { u"FIM", 5.94573, 2 },
    { u"FRF", 6.55957, 2 },   { u"IEP", 0.787564, 2 }, { u"ITL", 1936.27, 0 },
    { u"LUF", 40.3399, 0 },   { u"NLG", 2.20371, 2 },  { u"PTE", 200.482, 1 },
    { u"GRD", 340.75, 0 },    { u"SIT", 239.64, 0 },   { u"MTL", 0.4293, 2 },
    { u"CYP", 0.585274, 2 },  { u"SKK", 30.126, 1 },
};

[[nodiscard]] std::optional<sal_Int32> toWholeNumber(double fValue)
{
    if (!std::isfinite(fValue))
        return std::nullopt;

    const double fRounded = std::round(fValue);
    if (std::abs(fValue - fRounded) > 1e-9)
        return std::nullopt;

    return static_cast<sal_Int32>(fRounded);
}

[[nodiscard]] std::optional<AggregateOptions> decodeAggregateOptions(sal_Int32 nOption)
{
    switch (nOption)
    {
        case 0:
            return AggregateOptions { false, false, true };
        case 1:
            return AggregateOptions { true, false, true };
        case 2:
            return AggregateOptions { false, true, true };
        case 3:
            return AggregateOptions { true, true, true };
        case 4:
            return AggregateOptions { false, false, false };
        case 5:
            return AggregateOptions { true, false, false };
        case 6:
            return AggregateOptions { false, true, false };
        case 7:
            return AggregateOptions { true, true, false };
        default:
            return std::nullopt;
    }
}

[[nodiscard]] api::String normalizeUnitSymbol(api::StringView rUnit)
{
    api::String aResult;
    aResult.reserve(rUnit.size());
    for (std::size_t nIndex = 0; nIndex < rUnit.size(); ++nIndex)
    {
        const char16_t cChar = rUnit[nIndex];
        if (cChar == u'^' && nIndex + 1 < rUnit.size()
            && rUnit[nIndex + 1] >= u'0' && rUnit[nIndex + 1] <= u'9')
        {
            continue;
        }

        aResult.push_back(cChar);
    }
    return aResult;
}

[[nodiscard]] api::String normalizeAsciiUpper(api::StringView rValue)
{
    api::String aResult;
    aResult.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar >= u'a' && cChar <= u'z')
            aResult.push_back(static_cast<char16_t>(cChar - (u'a' - u'A')));
        else
            aResult.push_back(cChar);
    }
    return aResult;
}

[[nodiscard]] api::String expandDbcsByteText(api::StringView rText, bool bFoldAscii)
{
    api::String aExpanded;
    for (const char16_t cChar : rText)
    {
        char16_t cSlot = cChar;
        if (bFoldAscii && cSlot >= u'a' && cSlot <= u'z')
            cSlot = static_cast<char16_t>(cSlot - (u'a' - u'A'));

        aExpanded.push_back(cSlot);
        const int nEastAsianWidth = u_getIntPropertyValue(cChar, UCHAR_EAST_ASIAN_WIDTH);
        if (nEastAsianWidth == U_EA_FULLWIDTH || nEastAsianWidth == U_EA_WIDE)
            aExpanded.push_back(cSlot);
    }
    return aExpanded;
}

[[nodiscard]] api::String collapseDbcsByteText(api::StringView rExpandedText)
{
    api::String aCollapsed;
    for (std::size_t nIndex = 0; nIndex < rExpandedText.size();)
    {
        const char16_t cChar = rExpandedText[nIndex];
        if (cChar <= 0x7F)
        {
            aCollapsed.push_back(cChar);
            ++nIndex;
            continue;
        }

        const int nEastAsianWidth = u_getIntPropertyValue(cChar, UCHAR_EAST_ASIAN_WIDTH);
        if (nEastAsianWidth != U_EA_FULLWIDTH && nEastAsianWidth != U_EA_WIDE)
        {
            aCollapsed.push_back(cChar);
            ++nIndex;
            continue;
        }

        if (nIndex + 1 < rExpandedText.size() && rExpandedText[nIndex + 1] == cChar)
        {
            aCollapsed.push_back(cChar);
            nIndex += 2;
            continue;
        }

        aCollapsed.push_back(u' ');
        ++nIndex;
    }

    return aCollapsed;
}

[[nodiscard]] std::optional<std::size_t> findDbcsExpandedText(
    api::StringView rNeedle, api::StringView rHaystack, std::size_t nStartIndex, bool bFoldAscii)
{
    const api::String aNeedle = expandDbcsByteText(rNeedle, bFoldAscii);
    const api::String aHaystack = expandDbcsByteText(rHaystack, bFoldAscii);
    if (nStartIndex > aHaystack.size())
        return std::nullopt;
    if (aNeedle.empty())
        return nStartIndex;

    const std::size_t nPos = aHaystack.find(aNeedle, nStartIndex);
    if (nPos == api::String::npos)
        return std::nullopt;
    return nPos;
}

[[nodiscard]] std::optional<EuroCurrencyInfo> lookupEuroCurrency(
    api::StringView rCode, bool bCaseInsensitive)
{
    const api::String aNormalized = bCaseInsensitive ? normalizeAsciiUpper(rCode) : api::String(rCode);
    for (const auto& rCurrency : kEuroCurrencies)
    {
        if (aNormalized == rCurrency.maCode)
            return rCurrency;
    }
    return std::nullopt;
}

[[nodiscard]] double roundToDecimalPlaces(double fValue, sal_Int32 nDecimals)
{
    if (nDecimals < 0)
        return fValue;
    const double fScale = std::pow(10.0, static_cast<double>(nDecimals));
    return std::round(fValue * fScale) / fScale;
}

[[nodiscard]] api::ValueResult<double> evaluateEuroConvertValue(double fValue,
    api::StringView rFromCurrency, api::StringView rToCurrency, bool bCaseInsensitive,
    bool bRoundToTargetDecimals)
{
    const auto oFrom = lookupEuroCurrency(rFromCurrency, bCaseInsensitive);
    const auto oTo = lookupEuroCurrency(rToCurrency, bCaseInsensitive);
    if (!oFrom || !oTo)
        return api::ValueResult<double>::failure(api::Error::NotAvailable);

    double fResult = fValue;
    if (oFrom->maCode != oTo->maCode)
    {
        if (oFrom->maCode == u"EUR")
            fResult *= oTo->mfRate;
        else if (oTo->maCode == u"EUR")
            fResult /= oFrom->mfRate;
        else
            fResult = (fValue / oFrom->mfRate) * oTo->mfRate;
    }

    if (bRoundToTargetDecimals)
    {
        fResult = roundToDecimalPlaces(fResult, oTo->mnDecimals);
    }

    return api::ValueResult<double>::success(fResult);
}

[[nodiscard]] bool equalUnitSymbol(api::StringView rLeft, api::StringView rRight)
{
    return normalizeUnitSymbol(rLeft) == normalizeUnitSymbol(rRight);
}

[[nodiscard]] std::optional<double> convertTemperatureUnit(
    double fValue, api::StringView rFromUnit, api::StringView rToUnit)
{
    const api::String aNormalizedFrom = normalizeUnitSymbol(rFromUnit);
    const api::String aNormalizedTo = normalizeUnitSymbol(rToUnit);

    const auto toKelvin = [&](api::StringView rUnit) -> std::optional<double> {
        if (rUnit == u"C")
            return fValue + 273.15;
        if (rUnit == u"F")
            return (fValue + 459.67) * (5.0 / 9.0);
        if (rUnit == u"K")
            return fValue;
        if (rUnit == u"Rank" || rUnit == u"RANK")
            return fValue * (5.0 / 9.0);
        if (rUnit == u"Reau" || rUnit == u"REAU")
            return fValue * 1.25 + 273.15;
        return std::nullopt;
    };

    const auto oKelvin = toKelvin(aNormalizedFrom);
    if (!oKelvin)
        return std::nullopt;

    if (aNormalizedTo == u"C")
        return *oKelvin - 273.15;
    if (aNormalizedTo == u"F")
        return *oKelvin * (9.0 / 5.0) - 459.67;
    if (aNormalizedTo == u"K")
        return *oKelvin;
    if (aNormalizedTo == u"Rank" || aNormalizedTo == u"RANK")
        return *oKelvin * (9.0 / 5.0);
    if (aNormalizedTo == u"Reau" || aNormalizedTo == u"REAU")
        return (*oKelvin - 273.15) * 0.8;
    return std::nullopt;
}

[[nodiscard]] api::ValueResult<double> evaluateConvertValue(
    double fValue, api::StringView rFromUnit, api::StringView rToUnit)
{
    const api::String aNormalizedFrom = normalizeUnitSymbol(rFromUnit);
    const api::String aNormalizedTo = normalizeUnitSymbol(rToUnit);
    if (aNormalizedFrom == aNormalizedTo)
        return api::ValueResult<double>::success(fValue);

    if (const auto oTemperature = convertTemperatureUnit(fValue, rFromUnit, rToUnit))
        return api::ValueResult<double>::success(*oTemperature);

    for (const auto& rConversion : kKnownConversions)
    {
        if (equalUnitSymbol(rConversion.maFromUnit, rFromUnit)
            && equalUnitSymbol(rConversion.maToUnit, rToUnit))
        {
            return api::ValueResult<double>::success(fValue * rConversion.mfFactor);
        }
    }

    for (const auto& rConversion : kKnownConversions)
    {
        if (equalUnitSymbol(rConversion.maFromUnit, rToUnit)
            && equalUnitSymbol(rConversion.maToUnit, rFromUnit))
        {
            return api::ValueResult<double>::success(fValue / rConversion.mfFactor);
        }
    }

    struct PendingUnitConversion
    {
        api::String maUnit;
        double mfFactor = 1.0;
    };

    std::vector<PendingUnitConversion> aPending { { aNormalizedFrom, 1.0 } };
    std::vector<api::String> aVisited { aNormalizedFrom };

    for (std::size_t nIndex = 0; nIndex < aPending.size(); ++nIndex)
    {
        const PendingUnitConversion& rCurrent = aPending[nIndex];
        if (rCurrent.maUnit == aNormalizedTo)
            return api::ValueResult<double>::success(fValue * rCurrent.mfFactor);

        for (const auto& rConversion : kKnownConversions)
        {
            api::String aNextUnit;
            double fNextFactor = 1.0;
            if (normalizeUnitSymbol(rConversion.maFromUnit) == rCurrent.maUnit)
            {
                aNextUnit = normalizeUnitSymbol(rConversion.maToUnit);
                fNextFactor = rCurrent.mfFactor * rConversion.mfFactor;
            }
            else if (normalizeUnitSymbol(rConversion.maToUnit) == rCurrent.maUnit)
            {
                aNextUnit = normalizeUnitSymbol(rConversion.maFromUnit);
                fNextFactor = rCurrent.mfFactor / rConversion.mfFactor;
            }
            else
            {
                continue;
            }

            if (std::find(aVisited.begin(), aVisited.end(), aNextUnit) != aVisited.end())
                continue;

            aVisited.push_back(aNextUnit);
            aPending.push_back({ std::move(aNextUnit), fNextFactor });
        }
    }

    return api::ValueResult<double>::failure(api::Error::NotAvailable);
}

[[nodiscard]] api::String normalizeFunctionName(api::StringView rName)
{
    return uppercaseAscii(normalizeDisplayFunctionName(rName));
}

using CriteriaAggregateInput = sequery::CriteriaAggregateInput;
using CriteriaPredicate = sequery::CriteriaPredicate;
using CriteriaAggregateKind = sequery::CriteriaAggregateKind;

struct LookupInput
{
    bool mbScalar = true;
    api::CellValue maScalar = api::CellValue::empty();
    api::ResolvedReference maReference;
    std::vector<api::CellValue> maValues;
    api::MatrixSize mnColumns = 1;
    api::MatrixSize mnRows = 1;
};

[[nodiscard]] std::optional<LookupInput> makeLookupInput(const EvaluationResult& rResult)
{
    if (!rResult)
        return std::nullopt;

    LookupInput aInput;
    if (rResult.maValue.isScalar())
    {
        aInput.maScalar = rResult.maValue.maValue;
        return aInput;
    }

    aInput.mbScalar = false;
    aInput.maReference = rResult.maValue.maReference;
    const auto aDimensions = rResult.maValue.maReference.matrixDimensions();
    aInput.mnColumns = aDimensions.mnColumns;
    aInput.mnRows = aDimensions.mnRows;
    return aInput;
}

[[nodiscard]] LookupInput makeLookupScalarError(api::Error eError)
{
    LookupInput aInput;
    aInput.maScalar = api::CellValue::error(eError);
    return aInput;
}

[[nodiscard]] api::ValueResult<api::lookup::VectorLayout> detectLookupLayout(
    const LookupInput& rInput, bool bAllowMajorVector)
{
    if (rInput.mbScalar)
    {
        return api::ValueResult<api::lookup::VectorLayout>::success(
            { api::lookup::VectorOrientation::Column, 1 });
    }

    const api::MatrixDimensions aDimensions { rInput.mnColumns, rInput.mnRows };
    if (const auto aVectorLayout = api::lookup::detectVectorLayout(aDimensions))
        return aVectorLayout;

    if (!bAllowMajorVector)
        return api::ValueResult<api::lookup::VectorLayout>::failure(api::Error::IllegalArgument);

    return api::ValueResult<api::lookup::VectorLayout>::success(
        api::lookup::majorVectorLayout(aDimensions));
}

[[nodiscard]] EvaluationResult materializeLookupInputValue(Evaluator& rEvaluator,
    const LookupInput& rInput, api::lookup::VectorOrientation eOrientation,
    api::MatrixSize nIndex)
{
    if (rInput.mbScalar)
    {
        if (nIndex != 0)
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(rInput.maScalar);
    }

    const auto aCoordinate = api::lookup::planVectorElement(
        eOrientation, nIndex, { rInput.mnColumns, rInput.mnRows });
    if (!aCoordinate)
        return makeFailure(aCoordinate.meError);

    if (!rInput.maValues.empty())
    {
        const sal_Int64 nLinearIndex
            = static_cast<sal_Int64>(aCoordinate.maValue.mnRow) * rInput.mnColumns
              + aCoordinate.maValue.mnColumn;
        if (nLinearIndex < 0 || static_cast<std::size_t>(nLinearIndex) >= rInput.maValues.size())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(rInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
    }

    return rEvaluator.materializeReferenceValue(
        rInput.maReference, aCoordinate.maValue.mnColumn, aCoordinate.maValue.mnRow);
}

[[nodiscard]] std::optional<CriteriaAggregateInput> makeCriteriaAggregateInput(
    const EvaluationResult& rResult)
{
    if (!rResult)
        return std::nullopt;

    CriteriaAggregateInput aInput;
    if (rResult.maValue.isScalar())
    {
        aInput.maScalar = rResult.maValue.maValue;
        return aInput;
    }

    aInput.mbScalar = false;
    aInput.maReference = rResult.maValue.maReference;
    const auto aDimensions = rResult.maValue.maReference.matrixDimensions();
    aInput.mnColumns = aDimensions.mnColumns;
    aInput.mnRows = aDimensions.mnRows;
    return aInput;
}

class EvaluatorCriteriaAggregateMaterializer final
    : public sequery::CriteriaAggregateMaterializer
{
    Evaluator& mrEvaluator;

public:
    explicit EvaluatorCriteriaAggregateMaterializer(Evaluator& rEvaluator)
        : mrEvaluator(rEvaluator)
    {
    }

    [[nodiscard]] api::ValueResult<api::CellValue> materialize(
        const CriteriaAggregateInput& rInput, api::MatrixCoordinate aCoordinate) const override
    {
        if (rInput.mbScalar)
        {
            if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(rInput.maScalar);
        }

        EvaluationResult aResult = mrEvaluator.materializeReferenceValue(
            rInput.maReference, aCoordinate.mnColumn, aCoordinate.mnRow);
        if (!aResult)
            return api::ValueResult<api::CellValue>::failure(aResult.meError);
        if (!aResult.maValue.isScalar())
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(aResult.maValue.maValue);
    }
};

[[nodiscard]] bool formulaContainsAggregateLike(const formula::Node& rNode)
{
    if (rNode.meKind == formula::NodeKind::FunctionCall)
    {
        const api::String aName = normalizeFunctionName(rNode.maPrimaryText);
        if (aName == u"AGGREGATE" || aName == u"SUBTOTAL")
            return true;
    }

    for (const auto& pChild : rNode.maChildren)
    {
        if (formulaContainsAggregateLike(*pChild))
            return true;
    }

    return false;
}

[[nodiscard]] bool cellContainsAggregateLike(const workbook::Cell& rCell)
{
    if (!rCell.hasFormula())
        return false;

    const formula::ParseResult aParsed = formula::parseFormula(rCell.maFormula);
    return aParsed && formulaContainsAggregateLike(*aParsed.mpRoot);
}

[[nodiscard]] double sumNumbers(const std::vector<double>& rNumbers)
{
    double fSum = 0.0;
    for (const double fValue : rNumbers)
        fSum = ::rtl::math::approxAdd(fSum, fValue);
    return fSum;
}

[[nodiscard]] double phiValue(double fValue)
{
    return 0.39894228040143268 * std::exp(-(fValue * fValue) / 2.0);
}

[[nodiscard]] double taylorPolynomial(const double* pPolynomial, sal_uInt16 nMax, double fValue)
{
    double fResult = pPolynomial[nMax];
    for (short nIndex = nMax - 1; nIndex >= 0; --nIndex)
        fResult = (fResult * fValue) + pPolynomial[nIndex];
    return fResult;
}

[[nodiscard]] double gaussValue(double fValue)
{
    const double fAbs = std::abs(fValue);
    const sal_uInt16 nBucket = static_cast<sal_uInt16>(::rtl::math::approxFloor(fAbs));

    double fResult = 0.0;
    if (nBucket == 0)
    {
        static const double aT0[] = { 0.39894228040143268, -0.06649038006690545,
            0.00997355701003582, -0.00118732821548045, 0.00011543468761616,
            -0.00000944465625950, 0.00000066596935163, -0.00000004122667415,
            0.00000000227352982, 0.00000000011301172, 0.00000000000511243,
            -0.00000000000021218 };
        fResult = taylorPolynomial(aT0, 11, fAbs * fAbs) * fAbs;
    }
    else if (nBucket <= 2)
    {
        static const double aT2[] = { 0.47724986805182079, 0.05399096651318805,
            -0.05399096651318805, 0.02699548325659403, -0.00449924720943234,
            -0.00224962360471617, 0.00134977416282970, -0.00011783742691370,
            -0.00011515930357476, 0.00003704737285544, 0.00000282690796889,
            -0.00000354513195524, 0.00000037669563126, 0.00000019202407921,
            -0.00000005226908590, -0.00000000491799345, 0.00000000366377919,
            -0.00000000015981997, -0.00000000017381238, 0.00000000002624031,
            0.00000000000560919, -0.00000000000172127, -0.00000000000008634,
            0.00000000000007894 };
        fResult = taylorPolynomial(aT2, 23, fAbs - 2.0);
    }
    else if (nBucket <= 4)
    {
        static const double aT4[] = { 0.49996832875816688, 0.00013383022576489,
            -0.00026766045152977, 0.00033457556441221, -0.00028996548915725,
            0.00018178605666397, -0.00008252863922168, 0.00002551802519049,
            -0.00000391665839292, -0.00000074018205222, 0.00000064422023359,
            -0.00000017370155340, 0.00000000909595465, 0.00000000944943118,
            -0.00000000329957075, 0.00000000029492075, 0.00000000011874477,
            -0.00000000004420396, 0.00000000000361422, 0.00000000000143638,
            -0.00000000000045848 };
        fResult = taylorPolynomial(aT4, 20, fAbs - 4.0);
    }
    else
    {
        static const double aAsympt[] = { -1.0, 1.0, -3.0, 15.0, -105.0 };
        fResult = 0.5
                  + phiValue(fAbs)
                        * (taylorPolynomial(aAsympt, 4, 1.0 / (fAbs * fAbs)) / fAbs);
    }

    return fValue < 0.0 ? -fResult : fResult;
}

[[nodiscard]] api::ValueResult<double> gammaContinuedFraction(double fAlpha, double fX)
{
    constexpr double fHalfMachEps = 0.5 * std::numeric_limits<double>::epsilon();
    const double fBigInv = std::numeric_limits<double>::epsilon();
    const double fBig = 1.0 / fBigInv;
    double fCount = 0.0;
    double fY = 1.0 - fAlpha;
    double fDenom = fX + 2.0 - fAlpha;
    double fPkm1 = fX + 1.0;
    double fPkm2 = 1.0;
    double fQkm1 = fDenom * fX;
    double fQkm2 = fX;
    double fApprox = fPkm1 / fQkm1;
    bool bFinished = false;
    do
    {
        fCount += 1.0;
        fY += 1.0;
        const double fNum = fY * fCount;
        fDenom += 2.0;
        double fPk = fPkm1 * fDenom - fPkm2 * fNum;
        const double fQk = fQkm1 * fDenom - fQkm2 * fNum;
        if (!::rtl::math::approxEqual(fQk, 0.0))
        {
            const double fR = fPk / fQk;
            bFinished = std::abs((fApprox - fR) / fR) <= fHalfMachEps;
            fApprox = fR;
        }

        fPkm2 = fPkm1;
        fPkm1 = fPk;
        fQkm2 = fQkm1;
        fQkm1 = fQk;
        if (std::abs(fPk) > fBig)
        {
            fPkm2 *= fBigInv;
            fPkm1 *= fBigInv;
            fQkm2 *= fBigInv;
            fQkm1 *= fBigInv;
        }
    } while (!bFinished && fCount < 10000.0);

    if (!bFinished)
        return api::ValueResult<double>::failure(api::Error::NoConvergence);
    return api::ValueResult<double>::success(fApprox);
}

[[nodiscard]] api::ValueResult<double> gammaSeries(double fAlpha, double fX)
{
    constexpr double fHalfMachEps = 0.5 * std::numeric_limits<double>::epsilon();
    double fDenomFactor = fAlpha;
    double fSummand = 1.0 / fAlpha;
    double fSum = fSummand;
    int nCount = 1;
    do
    {
        fDenomFactor += 1.0;
        fSummand *= fX / fDenomFactor;
        fSum += fSummand;
        ++nCount;
    } while (fSummand / fSum > fHalfMachEps && nCount <= 10000);

    if (nCount > 10000)
        return api::ValueResult<double>::failure(api::Error::NoConvergence);
    return api::ValueResult<double>::success(fSum);
}

[[nodiscard]] api::ValueResult<double> lowRegularizedIncompleteGamma(double fAlpha, double fX)
{
    const double fLnFactor = fAlpha * std::log(fX) - fX - std::lgamma(fAlpha);
    const double fFactor = std::exp(fLnFactor);
    if (fX > fAlpha + 1.0)
    {
        const auto aContinuedFraction = gammaContinuedFraction(fAlpha, fX);
        if (!aContinuedFraction)
            return aContinuedFraction;
        return api::ValueResult<double>::success(1.0 - fFactor * aContinuedFraction.maValue);
    }

    const auto aSeries = gammaSeries(fAlpha, fX);
    if (!aSeries)
        return aSeries;
    return api::ValueResult<double>::success(fFactor * aSeries.maValue);
}

[[nodiscard]] api::ValueResult<double> upRegularizedIncompleteGamma(double fAlpha, double fX)
{
    const double fLnFactor = fAlpha * std::log(fX) - fX - std::lgamma(fAlpha);
    const double fFactor = std::exp(fLnFactor);
    if (fX > fAlpha + 1.0)
    {
        const auto aContinuedFraction = gammaContinuedFraction(fAlpha, fX);
        if (!aContinuedFraction)
            return aContinuedFraction;
        return api::ValueResult<double>::success(fFactor * aContinuedFraction.maValue);
    }

    const auto aSeries = gammaSeries(fAlpha, fX);
    if (!aSeries)
        return aSeries;
    return api::ValueResult<double>::success(1.0 - fFactor * aSeries.maValue);
}

[[nodiscard]] api::ValueResult<double> evaluateLegacyChiDist(double fChi, double fDegreesFreedom)
{
    if (fDegreesFreedom < 1.0 || fChi < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (fChi <= 0.0)
        return api::ValueResult<double>::success(1.0);
    return upRegularizedIncompleteGamma(fDegreesFreedom / 2.0, fChi / 2.0);
}

[[nodiscard]] api::ValueResult<double> evaluateBinomialInverse(
    double fTrials, double fProbability, double fAlpha)
{
    const double fN = ::rtl::math::approxFloor(fTrials);
    if (fN < 0.0 || fProbability < 0.0 || fProbability > 1.0 || fAlpha < 0.0 || fAlpha > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (::rtl::math::approxEqual(fAlpha, 0.0))
        return api::ValueResult<double>::success(0.0);
    if (::rtl::math::approxEqual(fProbability, 0.0))
        return api::ValueResult<double>::success(0.0);
    if (::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(fN);
    if (::rtl::math::approxEqual(fAlpha, 1.0))
        return api::ValueResult<double>::success(fN);

    sal_Int32 nLow = 0;
    sal_Int32 nHigh = static_cast<sal_Int32>(fN);
    while (nLow < nHigh)
    {
        const sal_Int32 nMid = nLow + ((nHigh - nLow) / 2);
        const auto aDistribution = semath::evaluateBinomialDistribution(
            static_cast<double>(nMid), fN, fProbability, true);
        if (!aDistribution)
            return aDistribution;

        if (aDistribution.maValue >= fAlpha || ::rtl::math::approxEqual(aDistribution.maValue, fAlpha))
            nHigh = nMid;
        else
            nLow = nMid + 1;
    }

    return api::ValueResult<double>::success(static_cast<double>(nLow));
}

[[nodiscard]] api::ValueResult<double> evaluateNormalDistribution(
    double fX, double fMean, double fSigma, bool bCumulative)
{
    if (!(fSigma > 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    constexpr double fSqrtTwo = 1.4142135623730950488;
    constexpr double fInvSqrtTwoPi = 0.39894228040143267794;
    const double fZ = (fX - fMean) / fSigma;
    if (bCumulative)
    {
        return api::ValueResult<double>::success(
            std::clamp(0.5 * std::erfc(-fZ / fSqrtTwo), 0.0, 1.0));
    }

    return api::ValueResult<double>::success(
        std::exp(-0.5 * fZ * fZ) * fInvSqrtTwoPi / fSigma);
}

[[nodiscard]] api::ValueResult<double> evaluateLogNormalDistribution(
    double fX, double fMean, double fSigma, bool bCumulative)
{
    if (!(fSigma > 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (bCumulative)
    {
        if (fX <= 0.0)
            return api::ValueResult<double>::success(0.0);
        return evaluateNormalDistribution(std::log(fX), fMean, fSigma, true);
    }

    if (!(fX > 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    constexpr double fInvSqrtTwoPi = 0.39894228040143267794;
    const double fZ = (std::log(fX) - fMean) / fSigma;
    return api::ValueResult<double>::success(
        std::exp(-0.5 * fZ * fZ) * fInvSqrtTwoPi / (fSigma * fX));
}

[[nodiscard]] api::ValueResult<double> evaluateChiSquareDistribution(
    double fX, double fDegreesFreedom, bool bCumulative, bool bMicrosoftSyntax)
{
    if (fDegreesFreedom < 1.0 || (bMicrosoftSyntax && fX < 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (bCumulative)
    {
        if (fX <= 0.0)
            return api::ValueResult<double>::success(0.0);
        return lowRegularizedIncompleteGamma(fDegreesFreedom / 2.0, fX / 2.0);
    }

    if (fX <= 0.0)
        return api::ValueResult<double>::success(0.0);

    const double fHalfDf = fDegreesFreedom / 2.0;
    const double fLogValue = (fHalfDf - 1.0) * std::log(fX * 0.5) - (fX / 2.0)
                             - std::log(2.0) - std::lgamma(fHalfDf);
    return api::ValueResult<double>::success(std::exp(fLogValue));
}

[[nodiscard]] api::ValueResult<double> evaluateGammaDistribution(
    double fX, double fAlpha, double fBeta, bool bCumulative, bool bMicrosoftSyntax)
{
    if (fAlpha <= 0.0 || fBeta <= 0.0 || (bMicrosoftSyntax && fX < 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (bCumulative)
    {
        if (fX <= 0.0)
            return api::ValueResult<double>::success(0.0);
        return lowRegularizedIncompleteGamma(fAlpha, fX / fBeta);
    }

    if (fX < 0.0)
        return api::ValueResult<double>::success(0.0);

    if (::rtl::math::approxEqual(fX, 0.0))
    {
        if (fAlpha < 1.0)
            return api::ValueResult<double>::failure(api::Error::DivisionByZero);
        if (::rtl::math::approxEqual(fAlpha, 1.0))
            return api::ValueResult<double>::success(1.0 / fBeta);
        return api::ValueResult<double>::success(0.0);
    }

    const double fScaledX = fX / fBeta;
    const double fLogValue = (fAlpha - 1.0) * std::log(fScaledX) - fScaledX - std::log(fBeta)
                             - std::lgamma(fAlpha);
    return api::ValueResult<double>::success(std::exp(fLogValue));
}

[[nodiscard]] api::ValueResult<double> evaluateGammaValue(double fX)
{
    const double fWhole = ::rtl::math::approxFloor(fX);
    if (fX <= 0.0 && ::rtl::math::approxEqual(fX, fWhole))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const double fResult = std::tgamma(fX);
    if (!std::isfinite(fResult))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(fResult);
}

[[nodiscard]] api::ValueResult<double> evaluateStudentDistribution(
    double fT, double fDegreesFreedom, int nType)
{
    if (fDegreesFreedom < 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    switch (nType)
    {
        case 1:
            return api::ValueResult<double>::success(
                0.5 * semath::betaCdf(
                          fDegreesFreedom / (fDegreesFreedom + fT * fT),
                          fDegreesFreedom / 2.0, 0.5));
        case 2:
            return api::ValueResult<double>::success(semath::betaCdf(
                fDegreesFreedom / (fDegreesFreedom + fT * fT), fDegreesFreedom / 2.0, 0.5));
        case 3:
            return api::ValueResult<double>::success(
                std::pow(1.0 + (fT * fT / fDegreesFreedom), -(fDegreesFreedom + 1.0) / 2.0)
                / (std::sqrt(fDegreesFreedom)
                   * semath::betaValue(0.5, fDegreesFreedom / 2.0)));
        case 4:
        {
            const double fX = fDegreesFreedom / (fT * fT + fDegreesFreedom);
            const double fRightHalf = 0.5 * semath::betaCdf(
                                                fX, 0.5 * fDegreesFreedom, 0.5);
            return api::ValueResult<double>::success(fT < 0.0 ? fRightHalf : 1.0 - fRightHalf);
        }
        default:
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
}

template <typename DistributionFn>
[[nodiscard]] api::ValueResult<double> invertMonotonicPositiveDistribution(
    double fTarget, double fInitialHigh, bool bIncreasing, const DistributionFn& rDistribution)
{
    double fLow = 0.0;
    double fHigh = std::max(1.0, fInitialHigh);
    auto aHigh = rDistribution(fHigh);
    if (!aHigh)
        return aHigh;

    bool bBracketed = bIncreasing ? (aHigh.maValue >= fTarget) : (aHigh.maValue <= fTarget);
    for (int nIter = 0; !bBracketed && nIter < 128; ++nIter)
    {
        fHigh *= 2.0;
        if (!std::isfinite(fHigh) || fHigh > 1.0e10)
            return api::ValueResult<double>::failure(api::Error::NoConvergence);

        aHigh = rDistribution(fHigh);
        if (!aHigh)
            return aHigh;
        bBracketed = bIncreasing ? (aHigh.maValue >= fTarget) : (aHigh.maValue <= fTarget);
    }

    if (!bBracketed)
        return api::ValueResult<double>::failure(api::Error::NoConvergence);

    for (int nIter = 0; nIter < 160; ++nIter)
    {
        const double fMid = 0.5 * (fLow + fHigh);
        const auto aMid = rDistribution(fMid);
        if (!aMid)
            return aMid;

        if (bIncreasing ? (aMid.maValue < fTarget) : (aMid.maValue > fTarget))
            fLow = fMid;
        else
            fHigh = fMid;
    }

    return api::ValueResult<double>::success(0.5 * (fLow + fHigh));
}

[[nodiscard]] api::ValueResult<double> evaluateTInverse(
    double fProbability, double fDegreesFreedom, int nType)
{
    if (fDegreesFreedom < 1.0 || fProbability <= 0.0 || fProbability > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if ((nType == 2 || nType == 4) && ::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(0.0);

    if (nType == 4)
    {
        if (fProbability >= 1.0)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        if (::rtl::math::approxEqual(fProbability, 0.5))
            return api::ValueResult<double>::success(0.0);
        if (fProbability < 0.5)
        {
            const auto aMirror = evaluateTInverse(1.0 - fProbability, fDegreesFreedom, nType);
            if (!aMirror)
                return aMirror;
            return api::ValueResult<double>::success(-aMirror.maValue);
        }

        return invertMonotonicPositiveDistribution(fProbability, fDegreesFreedom, true,
            [fDegreesFreedom](double fX) {
                return evaluateStudentDistribution(fX, fDegreesFreedom, 4);
            });
    }

    if (nType != 2)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return invertMonotonicPositiveDistribution(fProbability, fDegreesFreedom, false,
        [fDegreesFreedom](double fX) {
            return evaluateStudentDistribution(fX, fDegreesFreedom, 2);
        });
}

[[nodiscard]] api::ValueResult<double> evaluateFRightTailDistribution(
    double fX, double fDegreesFreedom1, double fDegreesFreedom2)
{
    if (fX < 0.0 || fDegreesFreedom1 < 1.0 || fDegreesFreedom2 < 1.0
        || fDegreesFreedom1 >= 1.0e10 || fDegreesFreedom2 >= 1.0e10)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const double fArgument
        = fDegreesFreedom2 / (fDegreesFreedom2 + fDegreesFreedom1 * fX);
    return api::ValueResult<double>::success(semath::betaCdf(
        fArgument, fDegreesFreedom2 / 2.0, fDegreesFreedom1 / 2.0));
}

[[nodiscard]] api::ValueResult<double> evaluateFInverseRightTail(
    double fProbability, double fDegreesFreedom1, double fDegreesFreedom2)
{
    if (fProbability <= 0.0 || fProbability > 1.0 || fDegreesFreedom1 < 1.0
        || fDegreesFreedom2 < 1.0 || fDegreesFreedom1 >= 1.0e10
        || fDegreesFreedom2 >= 1.0e10)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    if (::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(0.0);

    return invertMonotonicPositiveDistribution(fProbability, fDegreesFreedom1, false,
        [fDegreesFreedom1, fDegreesFreedom2](double fX) {
            return evaluateFRightTailDistribution(fX, fDegreesFreedom1, fDegreesFreedom2);
        });
}

[[nodiscard]] api::ValueResult<double> evaluateVarianceNumbers(
    const std::vector<double>& rNumbers, bool bSample, bool bReturnStdDev)
{
    const std::size_t nCount = rNumbers.size();
    if (nCount == 0 || (bSample && nCount < 2))
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    const double fMean = sumNumbers(rNumbers) / static_cast<double>(nCount);
    KahanSum fSquaredDeviation = 0.0;
    for (const double fValue : rNumbers)
    {
        const double fDelta = ::rtl::math::approxSub(fValue, fMean);
        fSquaredDeviation += fDelta * fDelta;
    }

    double fResult = fSquaredDeviation.get() / static_cast<double>(bSample ? (nCount - 1) : nCount);
    if (bReturnStdDev)
        fResult = std::sqrt(fResult);
    return api::ValueResult<double>::success(fResult);
}

[[nodiscard]] api::ValueResult<double> evaluateTrimmean(
    std::vector<double> aValues, double fPercent)
{
    if (aValues.empty() || fPercent < 0.0 || fPercent >= 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    std::sort(aValues.begin(), aValues.end());
    sal_Int32 nTrimCount
        = static_cast<sal_Int32>(::rtl::math::approxFloor(fPercent * aValues.size()));
    nTrimCount -= nTrimCount % 2;
    const std::size_t nTrimEachSide = static_cast<std::size_t>(nTrimCount / 2);
    if (nTrimEachSide * 2 >= aValues.size())
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    KahanSum fSum = 0.0;
    for (std::size_t nIndex = nTrimEachSide; nIndex < aValues.size() - nTrimEachSide; ++nIndex)
        fSum += aValues[nIndex];

    const std::size_t nRemaining = aValues.size() - (nTrimEachSide * 2);
    return api::ValueResult<double>::success(fSum.get() / static_cast<double>(nRemaining));
}

[[nodiscard]] api::ValueResult<double> evaluateModeSingle(const std::vector<double>& rValues)
{
    std::vector<std::pair<double, sal_Int32>> aCounts;
    aCounts.reserve(rValues.size());

    for (const double fValue : rValues)
    {
        auto it = std::find_if(aCounts.begin(), aCounts.end(), [fValue](const auto& rEntry) {
            return ::rtl::math::approxEqual(rEntry.first, fValue);
        });
        if (it == aCounts.end())
        {
            aCounts.emplace_back(fValue, 1);
            it = std::prev(aCounts.end());
        }
        else
        {
            ++it->second;
        }
    }

    auto itBest = aCounts.end();
    for (auto it = aCounts.begin(); it != aCounts.end(); ++it)
    {
        if (itBest == aCounts.end() || it->second > itBest->second)
            itBest = it;
    }

    if (itBest == aCounts.end() || itBest->second < 2)
        return api::ValueResult<double>::failure(api::Error::NotAvailable);
    return api::ValueResult<double>::success(itBest->first);
}

[[nodiscard]] api::ValueResult<double> evaluateHypergeometricDistribution(
    double fX, double fTrials, double fSuccesses, double fPopulation, bool bCumulative)
{
    const double fWholePopulation = ::rtl::math::approxFloor(fPopulation);
    const double fWholeSuccesses = ::rtl::math::approxFloor(fSuccesses);
    const double fWholeTrials = ::rtl::math::approxFloor(fTrials);
    const double fWholeX = ::rtl::math::approxFloor(fX);
    if (fWholeX < 0.0 || fWholeTrials < fWholeX || fWholePopulation < fWholeTrials
        || fWholePopulation < fWholeSuccesses || fWholeSuccesses < 0.0)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const auto probability = [fWholePopulation, fWholeSuccesses,
                              fWholeTrials](sal_Int32 nValue) {
        const long double fK = static_cast<long double>(nValue);
        if (fK < 0.0 || fK > fWholeTrials || fK > fWholeSuccesses
            || (fWholeTrials - fK) > (fWholePopulation - fWholeSuccesses))
        {
            return 0.0L;
        }

        const long double fLogProbability
            = std::lgammal(static_cast<long double>(fWholeSuccesses) + 1.0L)
              - std::lgammal(fK + 1.0L)
              - std::lgammal(static_cast<long double>(fWholeSuccesses) - fK + 1.0L)
              + std::lgammal(static_cast<long double>(fWholePopulation - fWholeSuccesses) + 1.0L)
              - std::lgammal(static_cast<long double>(fWholeTrials) - fK + 1.0L)
              - std::lgammal(static_cast<long double>(fWholePopulation - fWholeSuccesses)
                             - (static_cast<long double>(fWholeTrials) - fK) + 1.0L)
              - std::lgammal(static_cast<long double>(fWholePopulation) + 1.0L)
              + std::lgammal(static_cast<long double>(fWholeTrials) + 1.0L)
              + std::lgammal(static_cast<long double>(fWholePopulation - fWholeTrials) + 1.0L);
        return std::exp(fLogProbability);
    };

    const sal_Int32 nX = static_cast<sal_Int32>(fWholeX);
    if (!bCumulative)
    {
        return api::ValueResult<double>::success(static_cast<double>(probability(nX)));
    }

    long double fSum = 0.0L;
    for (sal_Int32 nValue = 0; nValue <= nX; ++nValue)
        fSum += probability(nValue);
    return api::ValueResult<double>::success(
        std::min(1.0, static_cast<double>(fSum)));
}

[[nodiscard]] api::ValueResult<double> evaluatePercentrank(
    std::vector<double> aValues, double fValue, bool bInclusive, sal_Int32 nSignificance)
{
    if (aValues.empty())
        return api::ValueResult<double>::failure(api::Error::NotAvailable);
    if (nSignificance < 1)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    std::sort(aValues.begin(), aValues.end());
    const std::size_t nSize = aValues.size();
    if (fValue < aValues.front() || fValue > aValues.back())
        return api::ValueResult<double>::failure(api::Error::NotAvailable);
    if (nSize == 1)
        return api::ValueResult<double>::success(1.0);

    double fResult = 0.0;
    if (::rtl::math::approxEqual(fValue, aValues.front()))
    {
        fResult = bInclusive ? 0.0 : 1.0 / static_cast<double>(nSize + 1);
    }
    else
    {
        std::size_t nOldCount = 0;
        double fOldValue = aValues.front();
        std::size_t nIndex = 1;
        for (; nIndex < nSize && aValues[nIndex] < fValue; ++nIndex)
        {
            if (!::rtl::math::approxEqual(aValues[nIndex], fOldValue))
            {
                nOldCount = nIndex;
                fOldValue = aValues[nIndex];
            }
        }

        if (nIndex < nSize && !::rtl::math::approxEqual(aValues[nIndex], fOldValue))
            nOldCount = nIndex;

        if (nIndex < nSize && ::rtl::math::approxEqual(fValue, aValues[nIndex]))
        {
            if (bInclusive)
                fResult = static_cast<double>(nOldCount) / static_cast<double>(nSize - 1);
            else
                fResult = static_cast<double>(nIndex + 1) / static_cast<double>(nSize + 1);
        }
        else
        {
            if (nOldCount == 0 || nOldCount >= nSize)
                return api::ValueResult<double>::failure(api::Error::NotAvailable);

            const double fLower = aValues[nOldCount - 1];
            const double fUpper = aValues[nOldCount];
            if (::rtl::math::approxEqual(fLower, fUpper))
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);

            const double fFraction = (fValue - fLower) / (fUpper - fLower);
            if (bInclusive)
            {
                fResult = (static_cast<double>(nOldCount - 1) + fFraction)
                          / static_cast<double>(nSize - 1);
            }
            else
            {
                fResult = (static_cast<double>(nOldCount) + fFraction)
                          / static_cast<double>(nSize + 1);
            }
        }
    }

    if (!::rtl::math::approxEqual(fResult, 0.0))
    {
        const double fExponent = ::rtl::math::approxFloor(std::log10(fResult)) + 1.0
                                 - static_cast<double>(nSignificance);
        const double fScale = std::pow(10.0, -fExponent);
        fResult = ::rtl::math::round(fResult * fScale) / fScale;
    }

    return api::ValueResult<double>::success(fResult);
}

[[nodiscard]] api::ValueResult<double> evaluateAggregateNumbers(
    sal_Int32 nFunction, const AggregateScan& rScan)
{
    switch (nFunction)
    {
        case 1:
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::failure(api::Error::DivisionByZero);
            return api::ValueResult<double>::success(
                sumNumbers(rScan.maNumbers) / static_cast<double>(rScan.maNumbers.size()));
        case 2:
            return api::ValueResult<double>::success(static_cast<double>(rScan.maNumbers.size()));
        case 3:
            return api::ValueResult<double>::success(static_cast<double>(rScan.mnNonEmptyCount));
        case 4:
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::success(0.0);
            return api::ValueResult<double>::success(
                *std::max_element(rScan.maNumbers.begin(), rScan.maNumbers.end()));
        case 5:
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::success(0.0);
            return api::ValueResult<double>::success(
                *std::min_element(rScan.maNumbers.begin(), rScan.maNumbers.end()));
        case 6:
        {
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::success(0.0);

            double fProduct = 1.0;
            for (const double fValue : rScan.maNumbers)
                fProduct *= fValue;
            return api::ValueResult<double>::success(fProduct);
        }
        case 7:
        case 8:
        case 10:
        case 11:
        {
            const bool bSample = nFunction == 7 || nFunction == 10;
            const sal_Int32 nCount = static_cast<sal_Int32>(rScan.maNumbers.size());
            if (nCount == 0 || (bSample && nCount < 2))
                return api::ValueResult<double>::failure(api::Error::DivisionByZero);

            const double fMean = sumNumbers(rScan.maNumbers) / static_cast<double>(nCount);
            double fSquaredDeviation = 0.0;
            for (const double fValue : rScan.maNumbers)
            {
                const double fDelta = fValue - fMean;
                fSquaredDeviation += fDelta * fDelta;
            }

            const double fVariance = fSquaredDeviation
                                     / static_cast<double>(bSample ? (nCount - 1) : nCount);
            if (nFunction == 7 || nFunction == 8)
                return api::ValueResult<double>::success(std::sqrt(fVariance));
            return api::ValueResult<double>::success(fVariance);
        }
        case 9:
            return api::ValueResult<double>::success(sumNumbers(rScan.maNumbers));
        case 12:
        {
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::failure(api::Error::DivisionByZero);
            std::vector<double> aSorted = rScan.maNumbers;
            std::sort(aSorted.begin(), aSorted.end());
            const std::size_t nMid = aSorted.size() / 2;
            if ((aSorted.size() % 2) != 0)
                return api::ValueResult<double>::success(aSorted[nMid]);
            return api::ValueResult<double>::success(
                (aSorted[nMid - 1] + aSorted[nMid]) / 2.0);
        }
        case 13:
        {
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::failure(api::Error::NotAvailable);

            std::vector<double> aSorted = rScan.maNumbers;
            std::sort(aSorted.begin(), aSorted.end());

            double fBestValue = 0.0;
            sal_Int32 nBestCount = 1;
            bool bHasMode = false;
            for (std::size_t nIndex = 0; nIndex < aSorted.size();)
            {
                std::size_t nNext = nIndex + 1;
                while (nNext < aSorted.size()
                       && ::rtl::math::approxEqual(aSorted[nNext], aSorted[nIndex]))
                {
                    ++nNext;
                }

                const sal_Int32 nCount = static_cast<sal_Int32>(nNext - nIndex);
                if (nCount > nBestCount)
                {
                    nBestCount = nCount;
                    fBestValue = aSorted[nIndex];
                    bHasMode = true;
                }
                nIndex = nNext;
            }

            if (!bHasMode)
                return api::ValueResult<double>::failure(api::Error::NotAvailable);
            return api::ValueResult<double>::success(fBestValue);
        }
        default:
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
}

[[nodiscard]] api::ValueResult<double> evaluateAggregateRankedNumbers(
    sal_Int32 nFunction, const AggregateScan& rScan, double fRankValue)
{
    if (!std::isfinite(fRankValue) || rScan.maNumbers.empty())
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    std::vector<double> aSorted = rScan.maNumbers;
    std::sort(aSorted.begin(), aSorted.end());

    const auto percentileInc = [&](double fFraction) -> api::ValueResult<double> {
        if (!(fFraction >= 0.0 && fFraction <= 1.0))
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        if (aSorted.size() == 1)
            return api::ValueResult<double>::success(aSorted.front());

        const double fIndex = fFraction * static_cast<double>(aSorted.size() - 1);
        const std::size_t nLower = static_cast<std::size_t>(std::floor(fIndex));
        const std::size_t nUpper = static_cast<std::size_t>(std::ceil(fIndex));
        if (nLower == nUpper)
            return api::ValueResult<double>::success(aSorted[nLower]);

        const double fWeight = fIndex - static_cast<double>(nLower);
        return api::ValueResult<double>::success(
            aSorted[nLower] + (aSorted[nUpper] - aSorted[nLower]) * fWeight);
    };

    const auto percentileExc = [&](double fFraction) -> api::ValueResult<double> {
        if (!(fFraction > 0.0 && fFraction < 1.0))
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const double fIndex = fFraction * static_cast<double>(aSorted.size() + 1);
        if (!(fIndex >= 1.0 && fIndex <= static_cast<double>(aSorted.size())))
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const std::size_t nLower = static_cast<std::size_t>(std::floor(fIndex));
        const std::size_t nUpper = static_cast<std::size_t>(std::ceil(fIndex));
        if (nLower == nUpper)
            return api::ValueResult<double>::success(aSorted[nLower - 1]);

        const double fWeight = fIndex - static_cast<double>(nLower);
        return api::ValueResult<double>::success(
            aSorted[nLower - 1] + (aSorted[nUpper - 1] - aSorted[nLower - 1]) * fWeight);
    };

    switch (nFunction)
    {
        case 14:
        case 15:
        {
            const auto oRank = toWholeNumber(fRankValue);
            if (!oRank || *oRank < 1 || *oRank > static_cast<sal_Int32>(aSorted.size()))
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
            const std::size_t nIndex = nFunction == 14
                                           ? aSorted.size() - static_cast<std::size_t>(*oRank)
                                           : static_cast<std::size_t>(*oRank - 1);
            return api::ValueResult<double>::success(aSorted[nIndex]);
        }
        case 16:
            return percentileInc(fRankValue);
        case 17:
        {
            const auto oQuartile = toWholeNumber(fRankValue);
            if (!oQuartile || *oQuartile < 0 || *oQuartile > 4)
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
            return percentileInc(static_cast<double>(*oQuartile) / 4.0);
        }
        case 18:
            return percentileExc(fRankValue);
        case 19:
        {
            const auto oQuartile = toWholeNumber(fRankValue);
            if (!oQuartile || *oQuartile < 1 || *oQuartile > 3)
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
            return percentileExc(static_cast<double>(*oQuartile) / 4.0);
        }
        default:
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
}

[[nodiscard]] std::optional<api::ColumnIndex> parseColumnName(api::StringView rColumnName)
{
    if (rColumnName.empty())
        return std::nullopt;

    sal_Int64 nColumn = 0;
    for (const char16_t cChar : rColumnName)
    {
        char16_t cUpper = cChar;
        if (cUpper >= u'a' && cUpper <= u'z')
            cUpper = static_cast<char16_t>(cUpper - u'a' + u'A');
        if (cUpper < u'A' || cUpper > u'Z')
            return std::nullopt;
        nColumn = nColumn * 26 + (cUpper - u'A' + 1);
    }

    return static_cast<api::ColumnIndex>(nColumn - 1);
}

[[nodiscard]] api::String unquoteSheetName(api::StringView rSheetName)
{
    if (rSheetName.size() < 2 || rSheetName.front() != u'\'' || rSheetName.back() != u'\'')
        return api::String(rSheetName);

    api::String aResult;
    aResult.reserve(rSheetName.size() - 2);
    for (std::size_t nIndex = 1; nIndex + 1 < rSheetName.size(); ++nIndex)
    {
        if (rSheetName[nIndex] == u'\'' && nIndex + 1 < rSheetName.size() - 1
            && rSheetName[nIndex + 1] == u'\'')
        {
            aResult.push_back(u'\'');
            ++nIndex;
            continue;
        }

        aResult.push_back(rSheetName[nIndex]);
    }
    return aResult;
}

[[nodiscard]] api::String formatReferenceTokenForDisplay(api::StringView rToken)
{
    if (rToken.empty())
        return {};

    std::size_t nDotPos = rToken.rfind(u'.');
    if (nDotPos == api::StringView::npos)
        return api::String(rToken);

    api::String aResult;
    api::StringView aSheet = rToken.substr(0, nDotPos);
    api::StringView aAddress = rToken.substr(nDotPos + 1);
    while (!aSheet.empty() && aSheet.front() == u'$')
        aSheet.remove_prefix(1);
    while (!aAddress.empty() && aAddress.front() == u'.')
        aAddress.remove_prefix(1);

    if (!aSheet.empty())
    {
        aResult += aSheet;
        aResult.push_back(u'.');
    }
    aResult += aAddress;
    return aResult;
}

[[nodiscard]] constexpr api::refdata::SheetLimits runtimeSheetLimits(
    const workbook::Workbook& rWorkbook)
{
    return { secompiler::detail::kSmokeMaxColumn, secompiler::detail::kSmokeMaxRow,
        static_cast<api::SheetId>(rWorkbook.maSheets.empty() ? 0 : rWorkbook.maSheets.size() - 1) };
}

[[nodiscard]] bool needsQuotedSheetName(api::StringView rSheetName)
{
    if (rSheetName.empty())
        return false;

    for (const char16_t cChar : rSheetName)
    {
        const bool bAlphaNum = (cChar >= u'0' && cChar <= u'9')
                               || (cChar >= u'A' && cChar <= u'Z')
                               || (cChar >= u'a' && cChar <= u'z') || cChar == u'_';
        if (!bAlphaNum)
            return true;
    }

    return false;
}

[[nodiscard]] api::String quoteSheetNameForFormula(api::StringView rSheetName)
{
    if (!needsQuotedSheetName(rSheetName))
        return api::String(rSheetName);

    api::String aQuoted;
    aQuoted.reserve(rSheetName.size() + 2);
    aQuoted.push_back(u'\'');
    for (const char16_t cChar : rSheetName)
    {
        if (cChar == u'\'')
            aQuoted.push_back(u'\'');
        aQuoted.push_back(cChar);
    }
    aQuoted.push_back(u'\'');
    return aQuoted;
}

[[nodiscard]] api::String columnNameFromIndex(api::ColumnIndex nColumn)
{
    api::String aName;
    api::ColumnIndex nCurrent = nColumn;
    do
    {
        const api::ColumnIndex nRemainder = nCurrent % 26;
        aName.insert(aName.begin(), static_cast<char16_t>(u'A' + nRemainder));
        nCurrent = (nCurrent / 26) - 1;
    } while (nCurrent >= 0);
    return aName;
}

[[nodiscard]] api::String formatAddressFunctionResult(api::RowIndex nRow, api::ColumnIndex nColumn,
    sal_Int32 nAbsMode, bool bA1Style, api::StringView rSheetName)
{
    api::String aResult;
    if (!rSheetName.empty())
    {
        aResult = quoteSheetNameForFormula(rSheetName);
        aResult.push_back(bA1Style ? u'.' : u'!');
    }

    const bool bRowAbsolute = nAbsMode == 1 || nAbsMode == 2;
    const bool bColumnAbsolute = nAbsMode == 1 || nAbsMode == 3;
    if (bA1Style)
    {
        if (bColumnAbsolute)
            aResult.push_back(u'$');
        aResult += columnNameFromIndex(nColumn);
        if (bRowAbsolute)
            aResult.push_back(u'$');
        aResult += formatNumber(static_cast<double>(nRow + 1));
        return aResult;
    }

    aResult.push_back(u'R');
    if (bRowAbsolute)
    {
        aResult += formatNumber(static_cast<double>(nRow + 1));
    }
    else
    {
        aResult.push_back(u'[');
        aResult += formatNumber(static_cast<double>(nRow + 1));
        aResult.push_back(u']');
    }

    aResult.push_back(u'C');
    if (bColumnAbsolute)
    {
        aResult += formatNumber(static_cast<double>(nColumn + 1));
    }
    else
    {
        aResult.push_back(u'[');
        aResult += formatNumber(static_cast<double>(nColumn + 1));
        aResult.push_back(u']');
    }

    return aResult;
}

[[nodiscard]] api::String formatAbsoluteCellReferenceToken(
    const api::CellAddress& rAddress, const workbook::Workbook& rWorkbook, api::SheetId nCurrentSheet)
{
    api::String aToken;
    if (rAddress.mnSheet == nCurrentSheet)
    {
        aToken = u".";
    }
    else
    {
        if (rAddress.mnSheet < 0 || static_cast<std::size_t>(rAddress.mnSheet) >= rWorkbook.maSheets.size())
            return {};
        aToken = quoteSheetNameForFormula(rWorkbook.maSheets[static_cast<std::size_t>(rAddress.mnSheet)].maName);
        aToken.push_back(u'.');
    }

    aToken += columnNameFromIndex(rAddress.mnColumn);
    aToken += formatNumber(static_cast<double>(rAddress.mnRow + 1));
    return aToken;
}

[[nodiscard]] api::String formatSingleReferenceToken(
    const api::refdata::SingleRefData& rReference, const workbook::Workbook& rWorkbook,
    const api::CellAddress& rCurrentAddress)
{
    const auto aAbsolute
        = api::refdata::toAbsoluteAddress(rReference, runtimeSheetLimits(rWorkbook), rCurrentAddress);
    return formatAbsoluteCellReferenceToken(aAbsolute, rWorkbook, rCurrentAddress.mnSheet);
}

[[nodiscard]] api::String errorCodeToLiteral(setoken::ErrorCode nErrorCode)
{
    switch (nErrorCode)
    {
        case 2042:
            return u"#N/A";
        case 2007:
            return u"#DIV/0!";
        case 2015:
            return u"#VALUE!";
        case 2023:
            return u"#REF!";
        case 2029:
            return u"#NAME?";
        case 2036:
            return u"#NUM!";
        case 2000:
            return u"#NULL!";
        default:
        {
            api::String aLiteral = u"#ERR";
            aLiteral += formatNumber(static_cast<double>(nErrorCode));
            aLiteral.push_back(u'!');
            return aLiteral;
        }
    }
}

struct InflatedStackItem
{
    enum class Kind : sal_uInt8
    {
        Node = 0,
        FunctionName,
        Byte
    };

    Kind meKind = Kind::Node;
    std::unique_ptr<formula::Node> mpNode;
    api::String maText;
    setoken::ByteData maByte;
};

[[nodiscard]] std::unique_ptr<formula::Node> makeSimpleNode(formula::NodeKind eKind)
{
    auto pNode = std::make_unique<formula::Node>();
    pNode->meKind = eKind;
    return pNode;
}

[[nodiscard]] std::unique_ptr<formula::Node> inflateMatrixScalarNode(const setoken::MatrixScalar& rScalar)
{
    auto pNode = std::make_unique<formula::Node>();
    if (const auto* pNumber = std::get_if<double>(&rScalar))
    {
        pNode->meKind = formula::NodeKind::NumberLiteral;
        pNode->mfNumber = *pNumber;
        return pNode;
    }
    if (const auto* pString = std::get_if<api::String>(&rScalar))
    {
        pNode->meKind = formula::NodeKind::StringLiteral;
        pNode->maPrimaryText = *pString;
        return pNode;
    }

    pNode->meKind = formula::NodeKind::ErrorLiteral;
    pNode->maPrimaryText = errorCodeToLiteral(std::get<setoken::ErrorCode>(rScalar));
    return pNode;
}

[[nodiscard]] bool popNode(
    std::vector<InflatedStackItem>& rStack, std::unique_ptr<formula::Node>& rpNode)
{
    if (rStack.empty() || rStack.back().meKind != InflatedStackItem::Kind::Node)
        return false;
    rpNode = std::move(rStack.back().mpNode);
    rStack.pop_back();
    return true;
}

[[nodiscard]] bool popByte(std::vector<InflatedStackItem>& rStack, setoken::ByteData& rByte)
{
    if (rStack.empty() || rStack.back().meKind != InflatedStackItem::Kind::Byte)
        return false;
    rByte = rStack.back().maByte;
    rStack.pop_back();
    return true;
}

[[nodiscard]] bool popFunctionName(std::vector<InflatedStackItem>& rStack, api::String& rName)
{
    if (rStack.empty() || rStack.back().meKind != InflatedStackItem::Kind::FunctionName)
        return false;
    rName = std::move(rStack.back().maText);
    rStack.pop_back();
    return true;
}

[[nodiscard]] std::optional<std::unique_ptr<formula::Node>> inflateCompiledFormulaNode(
    const setoken::CompiledFormula& rFormula, const workbook::Workbook& rWorkbook,
    const api::CellAddress& rCurrentAddress)
{
    std::vector<InflatedStackItem> aStack;
    aStack.reserve(rFormula.maTokens.size());

    for (const auto& rToken : rFormula.maTokens)
    {
        switch (rToken.meKind)
        {
            case setoken::Kind::Missing:
                aStack.push_back({ InflatedStackItem::Kind::Node,
                    makeSimpleNode(formula::NodeKind::EmptyArgument), {}, {} });
                break;
            case setoken::Kind::Value:
            {
                auto pNode = makeSimpleNode(formula::NodeKind::NumberLiteral);
                pNode->mfNumber = std::get<double>(rToken.maPayload);
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::String:
            {
                auto pNode = makeSimpleNode(formula::NodeKind::StringLiteral);
                pNode->maPrimaryText = std::get<setoken::StringData>(rToken.maPayload).maText;
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::StringName:
            {
                if (rToken.mnOpCode == setoken::kOpCodeName)
                {
                    auto pNode = makeSimpleNode(formula::NodeKind::NamedReference);
                    pNode->maPrimaryText = std::get<setoken::StringData>(rToken.maPayload).maText;
                    aStack.push_back(
                        { InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                }
                else
                {
                    aStack.push_back({ InflatedStackItem::Kind::FunctionName, nullptr,
                        std::get<setoken::StringData>(rToken.maPayload).maText, {} });
                }
                break;
            }
            case setoken::Kind::ExternalName:
            {
                const auto& rExternalName = std::get<setoken::ExternalNameData>(rToken.maPayload);
                const auto oBuiltinSymbol
                    = spreadsheetengine::detail::compiler::lookupBuiltinExternalSymbol(
                        rExternalName.maName);
                aStack.push_back({ InflatedStackItem::Kind::FunctionName, nullptr,
                    oBuiltinSymbol ? *oBuiltinSymbol : rExternalName.maName, {} });
                break;
            }
            case setoken::Kind::Byte:
                aStack.push_back(
                    { InflatedStackItem::Kind::Byte, nullptr, {}, std::get<setoken::ByteData>(rToken.maPayload) });
                break;
            case setoken::Kind::Error:
            {
                auto pNode = makeSimpleNode(formula::NodeKind::ErrorLiteral);
                pNode->maPrimaryText = errorCodeToLiteral(std::get<setoken::ErrorCode>(rToken.maPayload));
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::SingleRef:
            {
                auto pNode = makeSimpleNode(formula::NodeKind::CellReference);
                pNode->maPrimaryText = formatSingleReferenceToken(
                    std::get<api::refdata::SingleRefData>(rToken.maPayload), rWorkbook, rCurrentAddress);
                if (pNode->maPrimaryText.empty())
                    return std::nullopt;
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::DoubleRef:
            {
                const auto& rReference = std::get<api::refdata::ComplexRefData>(rToken.maPayload);
                auto pNode = makeSimpleNode(formula::NodeKind::RangeReference);
                pNode->maPrimaryText = formatSingleReferenceToken(rReference.maRef1, rWorkbook, rCurrentAddress);
                pNode->maSecondaryText = formatSingleReferenceToken(rReference.maRef2, rWorkbook, rCurrentAddress);
                if (pNode->maPrimaryText.empty() || pNode->maSecondaryText.empty())
                    return std::nullopt;
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::RangeName:
            {
                const auto& rName = std::get<setoken::NameData>(rToken.maPayload);
                if (rName.mnIndex == 0
                    || static_cast<std::size_t>(rName.mnIndex - 1) >= rWorkbook.maNamedRanges.size())
                {
                    return std::nullopt;
                }
                auto pNode = makeSimpleNode(formula::NodeKind::NamedReference);
                pNode->maPrimaryText = rWorkbook.maNamedRanges[static_cast<std::size_t>(rName.mnIndex - 1)].maName;
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::Matrix:
            {
                const auto& rMatrix = std::get<setoken::MatrixData>(rToken.maPayload);
                auto pNode = makeSimpleNode(formula::NodeKind::ArrayConstant);
                pNode->mnArrayRows = rMatrix.mnRows;
                pNode->mnArrayColumns = rMatrix.mnColumns;
                for (const auto& rScalar : rMatrix.maValues)
                    pNode->maChildren.push_back(inflateMatrixScalarNode(rScalar));
                aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                break;
            }
            case setoken::Kind::PlainOpcode:
            {
                auto makeUnary = [&](formula::UnaryOperator eOperator) -> bool {
                    std::unique_ptr<formula::Node> pChild;
                    if (!popNode(aStack, pChild))
                        return false;
                    auto pNode = makeSimpleNode(formula::NodeKind::UnaryOperation);
                    pNode->meUnaryOperator = eOperator;
                    pNode->maChildren.push_back(std::move(pChild));
                    aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                    return true;
                };

                auto makeBinary = [&](formula::BinaryOperator eOperator) -> bool {
                    std::unique_ptr<formula::Node> pRight;
                    std::unique_ptr<formula::Node> pLeft;
                    if (!popNode(aStack, pRight) || !popNode(aStack, pLeft))
                        return false;
                    auto pNode = makeSimpleNode(formula::NodeKind::BinaryOperation);
                    pNode->meBinaryOperator = eOperator;
                    pNode->maChildren.push_back(std::move(pLeft));
                    pNode->maChildren.push_back(std::move(pRight));
                    aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                    return true;
                };

                switch (rToken.mnOpCode)
                {
                    case secompiler::detail::kLoweredOpUnaryPlus:
                        if (!makeUnary(formula::UnaryOperator::Plus))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeNegSub:
                        if (!makeUnary(formula::UnaryOperator::Minus))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeAdd:
                        if (!makeBinary(formula::BinaryOperator::Add))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeSub:
                        if (!makeBinary(formula::BinaryOperator::Subtract))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeMul:
                        if (!makeBinary(formula::BinaryOperator::Multiply))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeDiv:
                        if (!makeBinary(formula::BinaryOperator::Divide))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodePow:
                        if (!makeBinary(formula::BinaryOperator::Power))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeAmpersand:
                        if (!makeBinary(formula::BinaryOperator::Concat))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeEqual:
                        if (!makeBinary(formula::BinaryOperator::Equal))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeNotEqual:
                        if (!makeBinary(formula::BinaryOperator::NotEqual))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeLess:
                        if (!makeBinary(formula::BinaryOperator::Less))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeLessEqual:
                        if (!makeBinary(formula::BinaryOperator::LessEqual))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeGreater:
                        if (!makeBinary(formula::BinaryOperator::Greater))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeGreaterEqual:
                        if (!makeBinary(formula::BinaryOperator::GreaterEqual))
                            return std::nullopt;
                        break;
                    case setoken::kOpCodeRange:
                    {
                        std::unique_ptr<formula::Node> pRight;
                        std::unique_ptr<formula::Node> pLeft;
                        if (!popNode(aStack, pRight) || !popNode(aStack, pLeft))
                            return std::nullopt;
                        auto pNode = makeSimpleNode(formula::NodeKind::RangeConstructor);
                        pNode->maChildren.push_back(std::move(pLeft));
                        pNode->maChildren.push_back(std::move(pRight));
                        aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                        break;
                    }
                    case setoken::kOpCodeUnion:
                    {
                        std::unique_ptr<formula::Node> pRight;
                        std::unique_ptr<formula::Node> pLeft;
                        if (!popNode(aStack, pRight) || !popNode(aStack, pLeft))
                            return std::nullopt;
                        auto pNode = makeSimpleNode(formula::NodeKind::ReferenceList);
                        auto appendChild = [&](std::unique_ptr<formula::Node> pChild) {
                            if (pChild->meKind == formula::NodeKind::ReferenceList)
                            {
                                for (auto& pGrandChild : pChild->maChildren)
                                    pNode->maChildren.push_back(std::move(pGrandChild));
                                return;
                            }
                            pNode->maChildren.push_back(std::move(pChild));
                        };
                        appendChild(std::move(pLeft));
                        appendChild(std::move(pRight));
                        aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                        break;
                    }
                    case secompiler::detail::kLoweredOpReferenceList:
                    {
                        setoken::ByteData aCount;
                        if (!popByte(aStack, aCount))
                            return std::nullopt;
                        auto pNode = makeSimpleNode(formula::NodeKind::ReferenceList);
                        std::vector<std::unique_ptr<formula::Node>> aChildren(
                            static_cast<std::size_t>(aCount.mnByte));
                        for (std::size_t nIndex = aChildren.size(); nIndex-- > 0;)
                        {
                            if (!popNode(aStack, aChildren[nIndex]))
                                return std::nullopt;
                        }
                        pNode->maChildren = std::move(aChildren);
                        aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                        break;
                    }
                    case secompiler::detail::kLoweredOpFunctionCall:
                    {
                        setoken::ByteData aCount;
                        api::String aName;
                        if (!popByte(aStack, aCount) || !popFunctionName(aStack, aName))
                            return std::nullopt;
                        auto pNode = makeSimpleNode(formula::NodeKind::FunctionCall);
                        pNode->maPrimaryText = aName;
                        std::vector<std::unique_ptr<formula::Node>> aChildren(
                            static_cast<std::size_t>(aCount.mnByte));
                        for (std::size_t nIndex = aChildren.size(); nIndex-- > 0;)
                        {
                            if (!popNode(aStack, aChildren[nIndex]))
                                return std::nullopt;
                        }
                        pNode->maChildren = std::move(aChildren);
                        aStack.push_back({ InflatedStackItem::Kind::Node, std::move(pNode), {}, {} });
                        break;
                    }
                    default:
                        return std::nullopt;
                }
                break;
            }
            default:
                return std::nullopt;
        }
    }

    if (aStack.size() != 1 || aStack.back().meKind != InflatedStackItem::Kind::Node)
        return std::nullopt;

    return std::move(aStack.back().mpNode);
}

[[nodiscard]] int binaryPrecedence(formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
        case formula::BinaryOperator::NotEqual:
        case formula::BinaryOperator::Less:
        case formula::BinaryOperator::LessEqual:
        case formula::BinaryOperator::Greater:
        case formula::BinaryOperator::GreaterEqual:
            return 1;
        case formula::BinaryOperator::Concat:
            return 2;
        case formula::BinaryOperator::Add:
        case formula::BinaryOperator::Subtract:
            return 3;
        case formula::BinaryOperator::Multiply:
        case formula::BinaryOperator::Divide:
            return 4;
        case formula::BinaryOperator::Power:
            return 5;
    }
    return 0;
}

[[nodiscard]] api::String binaryOperatorToken(formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Add:
            return u"+";
        case formula::BinaryOperator::Subtract:
            return u"-";
        case formula::BinaryOperator::Multiply:
            return u"*";
        case formula::BinaryOperator::Divide:
            return u"/";
        case formula::BinaryOperator::Power:
            return u"^";
        case formula::BinaryOperator::Concat:
            return u"&";
        case formula::BinaryOperator::Equal:
            return u"=";
        case formula::BinaryOperator::NotEqual:
            return u"<>";
        case formula::BinaryOperator::Less:
            return u"<";
        case formula::BinaryOperator::LessEqual:
            return u"<=";
        case formula::BinaryOperator::Greater:
            return u">";
        case formula::BinaryOperator::GreaterEqual:
            return u">=";
    }
    return {};
}

[[nodiscard]] std::optional<api::String> formatFormulaNodeForDisplay(
    const formula::Node& rNode, int nParentPrecedence = 0);

[[nodiscard]] std::optional<api::String> formatChildForDisplay(
    const formula::Node& rNode, int nParentPrecedence)
{
    return formatFormulaNodeForDisplay(rNode, nParentPrecedence);
}

[[nodiscard]] std::optional<api::String> formatFormulaNodeForDisplay(
    const formula::Node& rNode, int nParentPrecedence)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::NumberLiteral:
            return formatNumber(rNode.mfNumber);
        case formula::NodeKind::StringLiteral:
            return formatQuotedString(rNode.maPrimaryText);
        case formula::NodeKind::BooleanLiteral:
            return api::String(rNode.mbBoolean ? u"TRUE()" : u"FALSE()");
        case formula::NodeKind::ErrorLiteral:
            return api::String(rNode.maPrimaryText);
        case formula::NodeKind::EmptyArgument:
            return api::String {};
        case formula::NodeKind::CellReference:
            return formatReferenceTokenForDisplay(rNode.maPrimaryText);
        case formula::NodeKind::RangeReference:
        {
            api::String aResult = formatReferenceTokenForDisplay(rNode.maPrimaryText);
            aResult.push_back(u':');
            aResult += formatReferenceTokenForDisplay(rNode.maSecondaryText);
            return aResult;
        }
        case formula::NodeKind::NamedReference:
            return api::String(rNode.maPrimaryText);
        case formula::NodeKind::RangeConstructor:
        {
            const auto oLeft = formatChildForDisplay(*rNode.maChildren[0], 0);
            const auto oRight = formatChildForDisplay(*rNode.maChildren[1], 0);
            if (!oLeft || !oRight)
                return std::nullopt;
            api::String aResult = *oLeft;
            aResult.push_back(u':');
            aResult += *oRight;
            return aResult;
        }
        case formula::NodeKind::ReferenceList:
        {
            api::String aResult;
            for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); ++nIndex)
            {
                const auto oChild = formatChildForDisplay(*rNode.maChildren[nIndex], 0);
                if (!oChild)
                    return std::nullopt;
                aResult += *oChild;
                if (nIndex + 1 < rNode.maChildren.size())
                    aResult.push_back(u'~');
            }
            return aResult;
        }
        case formula::NodeKind::ArrayConstant:
        {
            api::String aResult = u"{";
            for (sal_Int32 nRow = 0; nRow < rNode.mnArrayRows; ++nRow)
            {
                for (sal_Int32 nColumn = 0; nColumn < rNode.mnArrayColumns; ++nColumn)
                {
                    const std::size_t nIndex
                        = static_cast<std::size_t>(nRow * rNode.mnArrayColumns + nColumn);
                    const auto oElement = formatChildForDisplay(*rNode.maChildren[nIndex], 0);
                    if (!oElement)
                        return std::nullopt;
                    aResult += *oElement;
                    if (nColumn + 1 < rNode.mnArrayColumns)
                        aResult.push_back(u',');
                }
                if (nRow + 1 < rNode.mnArrayRows)
                    aResult.push_back(u';');
            }
            aResult.push_back(u'}');
            return aResult;
        }
        case formula::NodeKind::UnaryOperation:
        {
            const auto oChild = formatChildForDisplay(*rNode.maChildren[0], 6);
            if (!oChild)
                return std::nullopt;
            api::String aResult = rNode.meUnaryOperator == formula::UnaryOperator::Minus
                                      ? api::String(u"-")
                                      : api::String(u"+");
            aResult += *oChild;
            return aResult;
        }
        case formula::NodeKind::BinaryOperation:
        {
            const int nPrecedence = binaryPrecedence(rNode.meBinaryOperator);
            const auto oLeft = formatChildForDisplay(*rNode.maChildren[0], nPrecedence);
            const auto oRight = formatChildForDisplay(*rNode.maChildren[1], nPrecedence + 1);
            if (!oLeft || !oRight)
                return std::nullopt;

            api::String aResult = *oLeft;
            aResult += binaryOperatorToken(rNode.meBinaryOperator);
            aResult += *oRight;
            if (nPrecedence < nParentPrecedence)
            {
                api::String aWrapped;
                aWrapped.push_back(u'(');
                aWrapped += aResult;
                aWrapped.push_back(u')');
                return aWrapped;
            }
            return aResult;
        }
        case formula::NodeKind::FunctionCall:
        {
            api::String aResult = normalizeDisplayFunctionName(rNode.maPrimaryText);
            aResult.push_back(u'(');
            for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); ++nIndex)
            {
                const auto oArgument = formatChildForDisplay(*rNode.maChildren[nIndex], 0);
                if (!oArgument)
                    return std::nullopt;
                aResult += *oArgument;
                if (nIndex + 1 < rNode.maChildren.size())
                    aResult.push_back(u',');
            }
            aResult.push_back(u')');
            return aResult;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<api::CellAddress> parseCellAddressToken(
    api::StringView rToken, const workbook::Workbook& rWorkbook, api::SheetId nImplicitSheet)
{
    const std::size_t nDotPos = rToken.rfind(u'.');
    if (nDotPos == api::StringView::npos || nDotPos + 1 >= rToken.size())
        return std::nullopt;

    api::SheetId nSheet = nImplicitSheet;
    api::StringView aSheetToken = rToken.substr(0, nDotPos);
    if (!aSheetToken.empty())
    {
        while (!aSheetToken.empty() && aSheetToken.front() == u'$')
            aSheetToken.remove_prefix(1);

        if (!aSheetToken.empty())
        {
            const api::String aSheetName = unquoteSheetName(aSheetToken);
            const auto oSheetId = rWorkbook.findSheetId(aSheetName);
            if (!oSheetId)
                return std::nullopt;
            nSheet = *oSheetId;
        }
    }

    api::StringView aAddressToken = rToken.substr(nDotPos + 1);
    if (!aAddressToken.empty() && aAddressToken.front() == u'$')
        aAddressToken.remove_prefix(1);

    std::size_t nColumnEnd = 0;
    while (nColumnEnd < aAddressToken.size())
    {
        const char16_t cChar = aAddressToken[nColumnEnd];
        const bool bAlpha = (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
        if (!bAlpha)
            break;
        ++nColumnEnd;
    }

    if (nColumnEnd == 0)
        return std::nullopt;

    const auto oColumn = parseColumnName(aAddressToken.substr(0, nColumnEnd));
    if (!oColumn)
        return std::nullopt;

    aAddressToken.remove_prefix(nColumnEnd);
    if (!aAddressToken.empty() && aAddressToken.front() == u'$')
        aAddressToken.remove_prefix(1);
    if (aAddressToken.empty())
        return std::nullopt;

    sal_Int64 nRow = 0;
    for (const char16_t cChar : aAddressToken)
    {
        if (cChar < u'0' || cChar > u'9')
            return std::nullopt;
        nRow = nRow * 10 + (cChar - u'0');
    }

    if (nRow <= 0)
        return std::nullopt;

    return api::CellAddress { nSheet, *oColumn, static_cast<api::RowIndex>(nRow - 1) };
}

[[nodiscard]] bool looksLikeA1AddressToken(api::StringView rToken)
{
    if (rToken.empty())
        return false;

    while (!rToken.empty() && rToken.front() == u'$')
        rToken.remove_prefix(1);

    std::size_t nColumnEnd = 0;
    while (nColumnEnd < rToken.size())
    {
        const char16_t cChar = rToken[nColumnEnd];
        if (!((cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z')))
            break;
        ++nColumnEnd;
    }
    if (nColumnEnd == 0 || nColumnEnd >= rToken.size())
        return false;

    api::StringView aRowToken = rToken.substr(nColumnEnd);
    if (!aRowToken.empty() && aRowToken.front() == u'$')
        aRowToken.remove_prefix(1);
    if (aRowToken.empty())
        return false;

    for (const char16_t cChar : aRowToken)
    {
        if (cChar < u'0' || cChar > u'9')
            return false;
    }

    return true;
}

[[nodiscard]] api::String prefixImplicitSheet(api::StringView rToken)
{
    api::String aResult = u".";
    aResult += rToken;
    return aResult;
}

[[nodiscard]] std::optional<api::String> normalizeIndirectA1ReferenceText(api::StringView rText)
{
    const std::size_t nBangPos = rText.rfind(u'!');
    api::StringView aSheetToken;
    api::StringView aAddressToken = rText;
    if (nBangPos != api::StringView::npos)
    {
        aSheetToken = rText.substr(0, nBangPos);
        aAddressToken = rText.substr(nBangPos + 1);
    }

    const auto normalizeRangePart = [&](api::StringView rPart) -> std::optional<api::String> {
        if (rPart.find(u'.') != api::StringView::npos)
            return api::String(rPart);
        if (!looksLikeA1AddressToken(rPart))
            return std::nullopt;
        return prefixImplicitSheet(rPart);
    };

    const std::size_t nColonPos = aAddressToken.find(u':');
    api::String aNormalized;
    if (!aSheetToken.empty())
    {
        aNormalized += aSheetToken;
        aNormalized.push_back(u'.');
    }

    if (nColonPos == api::StringView::npos)
    {
        if (!aSheetToken.empty())
        {
            if (!looksLikeA1AddressToken(aAddressToken))
                return std::nullopt;
            aNormalized += aAddressToken;
            return aNormalized;
        }

        return normalizeRangePart(aAddressToken);
    }

    const auto oStart = normalizeRangePart(aAddressToken.substr(0, nColonPos));
    const auto oEnd = normalizeRangePart(aAddressToken.substr(nColonPos + 1));
    if (!oStart || !oEnd)
        return std::nullopt;

    if (!aSheetToken.empty())
    {
        aNormalized += aAddressToken.substr(0, nColonPos);
        aNormalized.push_back(u':');
        aNormalized += *oEnd;
        return aNormalized;
    }

    aNormalized = *oStart;
    aNormalized.push_back(u':');
    aNormalized += *oEnd;
    return aNormalized;
}

[[nodiscard]] std::optional<api::ResolvedReference> parseIndirectR1C1ReferenceText(
    api::StringView rText, const workbook::Workbook& rWorkbook, api::SheetId nImplicitSheet)
{
    const auto parsePositiveIndex = [](api::StringView rDigits) -> std::optional<sal_Int64> {
        if (rDigits.empty())
            return std::nullopt;
        sal_Int64 nValue = 0;
        for (const char16_t cChar : rDigits)
        {
            if (cChar < u'0' || cChar > u'9')
                return std::nullopt;
            nValue = nValue * 10 + (cChar - u'0');
        }
        return nValue > 0 ? std::optional<sal_Int64>(nValue) : std::nullopt;
    };

    const std::size_t nBangPos = rText.rfind(u'!');
    api::StringView aSheetToken;
    api::StringView aAddressToken = rText;
    if (nBangPos != api::StringView::npos)
    {
        aSheetToken = rText.substr(0, nBangPos);
        aAddressToken = rText.substr(nBangPos + 1);
    }

    if (aAddressToken.size() < 4 || (aAddressToken[0] != u'R' && aAddressToken[0] != u'r'))
        return std::nullopt;

    std::size_t nIndex = 1;
    const std::size_t nRowStart = nIndex;
    while (nIndex < aAddressToken.size() && aAddressToken[nIndex] >= u'0'
           && aAddressToken[nIndex] <= u'9')
    {
        ++nIndex;
    }
    if (nIndex == nRowStart || nIndex >= aAddressToken.size()
        || (aAddressToken[nIndex] != u'C' && aAddressToken[nIndex] != u'c'))
    {
        return std::nullopt;
    }

    const auto oRow = parsePositiveIndex(aAddressToken.substr(nRowStart, nIndex - nRowStart));
    if (!oRow || *oRow < 1)
        return std::nullopt;

    ++nIndex;
    const std::size_t nColumnStart = nIndex;
    while (nIndex < aAddressToken.size() && aAddressToken[nIndex] >= u'0'
           && aAddressToken[nIndex] <= u'9')
    {
        ++nIndex;
    }
    if (nIndex != aAddressToken.size() || nIndex == nColumnStart)
        return std::nullopt;

    const auto oColumn = parsePositiveIndex(aAddressToken.substr(nColumnStart, nIndex - nColumnStart));
    if (!oColumn || *oColumn < 1)
        return std::nullopt;

    api::SheetId nSheet = nImplicitSheet;
    if (!aSheetToken.empty())
    {
        const auto oSheetId = rWorkbook.findSheetId(unquoteSheetName(aSheetToken));
        if (!oSheetId)
            return std::nullopt;
        nSheet = *oSheetId;
    }

    api::CellAddress aAddress {
        nSheet,
        static_cast<api::ColumnIndex>(*oColumn - 1),
        static_cast<api::RowIndex>(*oRow - 1),
    };
    return api::ResolvedReference { { aAddress, aAddress } };
}

[[nodiscard]] EvaluationResult ensureScalarValue(Evaluator& rEvaluator, EvaluationResult aResult)
{
    if (!aResult)
        return aResult;
    if (aResult.maValue.isScalar())
        return aResult;
    if (!aResult.maValue.maReference.isSingleCell())
        return makeFailure(api::Error::IllegalArgument);
    return rEvaluator.materializeReferenceValue(aResult.maValue.maReference, 0, 0);
}

[[nodiscard]] bool evaluateNumericComparison(
    double fLeft, double fRight, formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
            return ::rtl::math::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::NotEqual:
            return !::rtl::math::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::Less:
            return fLeft < fRight;
        case formula::BinaryOperator::LessEqual:
            return fLeft < fRight || ::rtl::math::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::Greater:
            return fLeft > fRight;
        case formula::BinaryOperator::GreaterEqual:
            return fLeft > fRight || ::rtl::math::approxEqual(fLeft, fRight);
        default:
            return false;
    }
}

[[nodiscard]] bool evaluateStringComparison(
    api::StringView rLeft, api::StringView rRight, formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
            return rLeft == rRight;
        case formula::BinaryOperator::NotEqual:
            return rLeft != rRight;
        case formula::BinaryOperator::Less:
            return rLeft < rRight;
        case formula::BinaryOperator::LessEqual:
            return rLeft <= rRight;
        case formula::BinaryOperator::Greater:
            return rLeft > rRight;
        case formula::BinaryOperator::GreaterEqual:
            return rLeft >= rRight;
        default:
            return false;
    }
}

[[nodiscard]] double roundMagnitudeDirectional(
    double fValue, int nDecimals, api::RoundingMode eMode)
{
    double fRoundedMagnitude = api::math::roundToDecimals(std::abs(fValue), nDecimals, eMode);

    const double fScale = std::pow(10.0, static_cast<double>(std::abs(nDecimals)));
    if (!std::isfinite(fScale) || fScale == 0.0)
        return fValue;

    double fScaled = nDecimals >= 0 ? std::abs(fValue) * fScale : std::abs(fValue) / fScale;
    if (nDecimals < 12)
    {
        const double fRoundedInteger = ::rtl::math::round(fScaled);
        if (std::abs(fScaled - fRoundedInteger) <= 1e-12)
        {
            fRoundedMagnitude
                = nDecimals >= 0 ? fRoundedInteger / fScale : fRoundedInteger * fScale;
        }
    }

    return std::signbit(fValue) ? -fRoundedMagnitude : fRoundedMagnitude;
}

} // namespace

const workbook::Sheet* Evaluator::getSheet(api::SheetId nSheet) const
{
    if (nSheet < 0 || static_cast<std::size_t>(nSheet) >= mrWorkbook.maSheets.size())
        return nullptr;
    return &mrWorkbook.maSheets[static_cast<std::size_t>(nSheet)];
}

const workbook::Cell* Evaluator::getCell(const api::CellAddress& rAddress) const
{
    const workbook::Sheet* pSheet = getSheet(rAddress.mnSheet);
    return pSheet ? pSheet->findCell(rAddress.mnColumn, rAddress.mnRow) : nullptr;
}

const EvaluationResult* Evaluator::lookupLocalBinding(api::StringView rName) const
{
    const api::String aNormalizedName = normalizeAsciiUpper(rName);
    for (auto aScopeIt = maLocalBindings.rbegin(); aScopeIt != maLocalBindings.rend(); ++aScopeIt)
    {
        const auto aBindingIt = aScopeIt->find(aNormalizedName);
        if (aBindingIt != aScopeIt->end())
            return &aBindingIt->second;
    }
    return nullptr;
}

std::map<Evaluator::AddressKey, Evaluator::CacheEntry>& Evaluator::cacheForMode(ExecutionMode eMode)
{
    return eMode == ExecutionMode::CompiledToken ? maCompiledCellCache : maAstCellCache;
}

EvaluationResult Evaluator::materializeReferenceValue(
    const api::ResolvedReference& rReference, api::ColumnIndex nColumnOffset,
    api::RowIndex nRowOffset)
{
    if (!rReference.isNormalized() || !rReference.containsOffset(nColumnOffset, nRowOffset))
        return makeFailure(api::Error::IllegalArgument);

    return evaluateCellInternal(rReference.addressAt(nColumnOffset, nRowOffset), meActiveExecutionMode);
}

api::ValueResult<api::ResolvedReference> Evaluator::resolveReferenceText(
    api::StringView rReference, api::SheetId nCurrentSheet) const
{
    const std::size_t nColonPos = rReference.find(u':');
    if (nColonPos == api::StringView::npos)
    {
        const auto oAddress = parseCellAddressToken(rReference, mrWorkbook, nCurrentSheet);
        if (!oAddress)
            return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

        return api::ValueResult<api::ResolvedReference>::success({ { *oAddress, *oAddress } });
    }

    const auto oStart
        = parseCellAddressToken(rReference.substr(0, nColonPos), mrWorkbook, nCurrentSheet);
    if (!oStart)
        return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

    const auto oEnd = parseCellAddressToken(
        rReference.substr(nColonPos + 1), mrWorkbook, oStart->mnSheet);
    if (!oEnd || oStart->mnSheet != oEnd->mnSheet)
        return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

    api::CellRange aRange { *oStart, *oEnd };
    if (aRange.maStart.mnColumn > aRange.maEnd.mnColumn)
        std::swap(aRange.maStart.mnColumn, aRange.maEnd.mnColumn);
    if (aRange.maStart.mnRow > aRange.maEnd.mnRow)
        std::swap(aRange.maStart.mnRow, aRange.maEnd.mnRow);

    return api::ValueResult<api::ResolvedReference>::success({ aRange });
}

api::ValueResult<api::ResolvedReference> Evaluator::resolveNamedRange(
    api::StringView rName, api::SheetId nScopeSheet) const
{
    if (nScopeSheet >= 0 && static_cast<std::size_t>(nScopeSheet) < mrWorkbook.maSheets.size())
    {
        const auto& rSheet = mrWorkbook.maSheets[static_cast<std::size_t>(nScopeSheet)];
        if (const auto* pLocal = mrWorkbook.findNamedRange(rName, rSheet.maName))
            return resolveReferenceText(pLocal->maCellRangeAddress, nScopeSheet);
    }

    if (const auto* pGlobal = mrWorkbook.findNamedRange(rName))
        return resolveReferenceText(pGlobal->maCellRangeAddress, nScopeSheet);

    return api::ValueResult<api::ResolvedReference>::failure(api::Error::NotAvailable);
}

EvaluationResult Evaluator::evaluateReferenceNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::CellReference:
        {
            const auto aReference = resolveReferenceText(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aReference)
                return makeFailure(aReference.meError);
            return makeReferenceResult(aReference.maValue);
        }
        case formula::NodeKind::RangeReference:
        {
            api::String aReference = rNode.maPrimaryText;
            aReference.push_back(u':');
            aReference += rNode.maSecondaryText;
            const auto aRange = resolveReferenceText(aReference, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::NamedReference:
        {
            const auto aRange = resolveNamedRange(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::RangeConstructor:
            return makeFailure(api::Error::IllegalArgument);
        case formula::NodeKind::ReferenceList:
            return makeFailure(api::Error::IllegalArgument);
        default:
        {
            EvaluationResult aValue = evaluateNode(rNode, rCurrentAddress);
            if (!aValue)
                return aValue;
            if (!aValue.maValue.isMatrixReference())
                return makeFailure(api::Error::IllegalArgument);
            return aValue;
        }
    }
}

EvaluationResult Evaluator::evaluateFunction(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    const api::String aFunctionName = normalizeFunctionName(rNode.maPrimaryText);

    auto visitFlattenedValues
        = [&](const auto& self, const formula::Node& rArgument,
              const auto& rVisitor) -> api::ValueResult<bool> {
        if (rArgument.meKind == formula::NodeKind::ArrayConstant
            || rArgument.meKind == formula::NodeKind::ReferenceList)
        {
            for (const auto& pChild : rArgument.maChildren)
            {
                const auto aChild = self(self, *pChild, rVisitor);
                if (!aChild)
                    return aChild;
            }
            return api::ValueResult<bool>::success(true);
        }

        const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                    || rArgument.meKind == formula::NodeKind::RangeReference
                                    || rArgument.meKind == formula::NodeKind::NamedReference;

        EvaluationResult aValue = bReferenceLike ? evaluateReferenceNode(rArgument, rCurrentAddress)
                                                 : evaluateNode(rArgument, rCurrentAddress);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);

        auto visitScalar = [&](const api::CellValue& rValue, bool bFromReference)
            -> api::ValueResult<bool> { return rVisitor(rValue, bFromReference); };

        if (aValue.maValue.isMatrixReference())
        {
            const auto& rReference = aValue.maValue.maReference;
            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                        return api::ValueResult<bool>::failure(aCell.meError);

                    const auto aVisited = visitScalar(aCell.maValue.maValue, true);
                    if (!aVisited)
                        return aVisited;
                }
            }

            return api::ValueResult<bool>::success(true);
        }

        return visitScalar(aValue.maValue.maValue, bReferenceLike);
    };

    auto collectNumericArguments = [&](bool bIgnoreTextAndEmptyFromReferences,
                                      bool bTreatScalarEmptyAsZero = false)
        -> api::ValueResult<std::vector<double>> {
        std::vector<double> aNumbers;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
                [&](const api::CellValue& rValue,
                    bool bFromReference) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);

                    if (bFromReference && bIgnoreTextAndEmptyFromReferences
                        && (rValue.isEmpty() || rValue.isText()))
                    {
                        return api::ValueResult<bool>::success(true);
                    }

                    if (rValue.isEmpty())
                    {
                        if (bTreatScalarEmptyAsZero)
                            aNumbers.push_back(0.0);
                        return api::ValueResult<bool>::success(true);
                    }

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                    {
                        if (bFromReference && bIgnoreTextAndEmptyFromReferences
                            && rValue.isText())
                        {
                            return api::ValueResult<bool>::success(true);
                        }

                        return api::ValueResult<bool>::failure(aNumber.meError);
                    }

                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(aNumbers);
    };

    auto collectVarianceArguments = [&](bool bTextAsZero)
        -> api::ValueResult<std::vector<double>> {
        std::vector<double> aNumbers;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
                [&](const api::CellValue& rValue,
                    bool bFromReference) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);

                    if (rValue.isEmpty())
                        return api::ValueResult<bool>::success(true);

                    if (rValue.isText())
                    {
                        if (bTextAsZero)
                        {
                            aNumbers.push_back(0.0);
                            return api::ValueResult<bool>::success(true);
                        }

                        if (bFromReference)
                            return api::ValueResult<bool>::success(true);
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
                    }

                    if (rValue.isBoolean() && bFromReference && !bTextAsZero)
                        return api::ValueResult<bool>::success(true);

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                        return api::ValueResult<bool>::failure(aNumber.meError);
                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(aNumbers);
    };

    auto evaluateScalarArgumentValue = [&](const formula::Node& rArgument)
        -> api::ValueResult<api::CellValue> {
        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(rArgument, rCurrentAddress));
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        return api::ValueResult<api::CellValue>::success(aValue.maValue.maValue);
    };

    auto evaluateNumericArgument = [&](const formula::Node& rArgument,
                                      std::optional<double> oDefaultForEmpty)
        -> api::ValueResult<double> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<double>::failure(aNumber.meError);
        return aNumber;
    };

    auto collectAggregateScanFromArgument = [&](const formula::Node& rArgument)
        -> api::ValueResult<AggregateScan> {
        AggregateScan aScan;
        const auto aVisited = visitFlattenedValues(
            visitFlattenedValues, rArgument,
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isNumber())
                    aScan.maNumbers.push_back(rValue.mfNumber);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<AggregateScan>::failure(aVisited.meError);
        return api::ValueResult<AggregateScan>::success(std::move(aScan));
    };

    auto evaluateLookupInputNode = [&](const formula::Node& rLookupNode)
        -> api::ValueResult<LookupInput> {
        auto evaluateMatrixOperand = [&](const formula::Node& rOperand)
            -> api::ValueResult<LookupInput> {
            EvaluationResult aValue = evaluateNode(rOperand, rCurrentAddress);
            if (!aValue)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(aValue.meError));

            if (aValue.maValue.isScalar())
            {
                const auto aNumber = coerceToNumber(aValue.maValue.maValue);
                if (!aNumber)
                {
                    return api::ValueResult<LookupInput>::success(
                        makeLookupScalarError(aNumber.meError));
                }

                LookupInput aInput;
                aInput.maScalar = api::CellValue::number(aNumber.maValue);
                return api::ValueResult<LookupInput>::success(aInput);
            }

            LookupInput aInput;
            aInput.mbScalar = false;
            const auto aDimensions = aValue.maValue.maReference.matrixDimensions();
            aInput.mnColumns = aDimensions.mnColumns;
            aInput.mnRows = aDimensions.mnRows;
            aInput.maValues.reserve(
                static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnColumns, 0))
                * static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnRows, 0)));
            for (api::MatrixSize nRow = 0; nRow < aInput.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aInput.mnColumns; ++nCol)
                {
                    EvaluationResult aElement
                        = materializeReferenceValue(aValue.maValue.maReference, nCol, nRow);
                    if (!aElement)
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(aElement.meError));
                    }
                    if (!aElement.maValue.isScalar())
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(api::Error::IllegalArgument));
                    }

                    const auto aNumber = coerceToNumber(aElement.maValue.maValue);
                    if (!aNumber)
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(aNumber.meError));
                    }
                    aInput.maValues.push_back(api::CellValue::number(aNumber.maValue));
                }
            }
            return api::ValueResult<LookupInput>::success(aInput);
        };

        if (rLookupNode.meKind == formula::NodeKind::FunctionCall
            && normalizeFunctionName(rLookupNode.maPrimaryText) == u"ISNUMBER")
        {
            if (rLookupNode.maChildren.size() != 1)
            {
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));
            }

            EvaluationResult aValue = evaluateNode(*rLookupNode.maChildren[0], rCurrentAddress);
            if (!aValue)
            {
                LookupInput aScalar;
                aScalar.maScalar = api::CellValue::boolean(false);
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            if (aValue.maValue.isScalar())
            {
                LookupInput aScalar;
                aScalar.maScalar = api::CellValue::boolean(aValue.maValue.maValue.isNumber());
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            LookupInput aInput;
            aInput.mbScalar = false;
            const auto aDimensions = aValue.maValue.maReference.matrixDimensions();
            aInput.mnColumns = aDimensions.mnColumns;
            aInput.mnRows = aDimensions.mnRows;
            aInput.maValues.reserve(
                static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnColumns, 0))
                * static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnRows, 0)));
            for (api::MatrixSize nRow = 0; nRow < aInput.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aInput.mnColumns; ++nCol)
                {
                    EvaluationResult aElement
                        = materializeReferenceValue(aValue.maValue.maReference, nCol, nRow);
                    const bool bIsNumber = aElement && aElement.maValue.isScalar()
                                           && aElement.maValue.maValue.isNumber();
                    aInput.maValues.push_back(api::CellValue::boolean(bIsNumber));
                }
            }

            if (aInput.mnColumns == 1 && aInput.mnRows == 1 && !aInput.maValues.empty())
            {
                LookupInput aScalar;
                aScalar.maScalar = aInput.maValues.front();
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            return api::ValueResult<LookupInput>::success(aInput);
        }

        if (rLookupNode.meKind == formula::NodeKind::FunctionCall
            && rLookupNode.maPrimaryText == u"MMULT")
        {
            if (rLookupNode.maChildren.size() != 2)
            {
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));
            }

            const auto aLeft = evaluateMatrixOperand(*rLookupNode.maChildren[0]);
            const auto aRight = evaluateMatrixOperand(*rLookupNode.maChildren[1]);
            if (!aLeft)
                return aLeft;
            if (!aRight)
                return aRight;
            if (aLeft.maValue.mbScalar && aLeft.maValue.maScalar.isError())
                return api::ValueResult<LookupInput>::success(aLeft.maValue);
            if (aRight.maValue.mbScalar && aRight.maValue.maScalar.isError())
                return api::ValueResult<LookupInput>::success(aRight.maValue);

            const api::MatrixSize nLeftColumns
                = aLeft.maValue.mbScalar ? 1 : aLeft.maValue.mnColumns;
            const api::MatrixSize nLeftRows = aLeft.maValue.mbScalar ? 1 : aLeft.maValue.mnRows;
            const api::MatrixSize nRightColumns
                = aRight.maValue.mbScalar ? 1 : aRight.maValue.mnColumns;
            const api::MatrixSize nRightRows
                = aRight.maValue.mbScalar ? 1 : aRight.maValue.mnRows;
            if (nLeftColumns != nRightRows)
            {
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));
            }

            auto getValueAt = [](const LookupInput& rInput, api::MatrixSize nColumn,
                                 api::MatrixSize nRow) -> double {
                if (rInput.mbScalar)
                    return rInput.maScalar.mfNumber;
                const std::size_t nIndex
                    = static_cast<std::size_t>(nRow * rInput.mnColumns + nColumn);
                return rInput.maValues[nIndex].mfNumber;
            };

            LookupInput aResult;
            aResult.mbScalar = false;
            aResult.mnColumns = nRightColumns;
            aResult.mnRows = nLeftRows;
            aResult.maValues.reserve(static_cast<std::size_t>(aResult.mnColumns)
                                     * static_cast<std::size_t>(aResult.mnRows));
            for (api::MatrixSize nRow = 0; nRow < aResult.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aResult.mnColumns; ++nCol)
                {
                    double fSum = 0.0;
                    for (api::MatrixSize nIndex = 0; nIndex < nLeftColumns; ++nIndex)
                    {
                        fSum += getValueAt(aLeft.maValue, nIndex, nRow)
                                * getValueAt(aRight.maValue, nCol, nIndex);
                    }
                    aResult.maValues.push_back(api::CellValue::number(fSum));
                }
            }

            if (aResult.mnColumns == 1 && aResult.mnRows == 1)
            {
                LookupInput aScalar;
                aScalar.maScalar = aResult.maValues.front();
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            return api::ValueResult<LookupInput>::success(aResult);
        }

        EvaluationResult aValue = evaluateNode(rLookupNode, rCurrentAddress);
        if (!aValue)
            return api::ValueResult<LookupInput>::failure(aValue.meError);

        const auto oInput = makeLookupInput(aValue);
        if (!oInput)
            return api::ValueResult<LookupInput>::failure(api::Error::IllegalArgument);
        return api::ValueResult<LookupInput>::success(*oInput);
    };

    auto evaluatePayTypeArgument = [&](const formula::Node& rArgument, bool bDefaultValue)
        -> api::ValueResult<bool> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<bool>::success(bDefaultValue);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<bool>::success(bDefaultValue);

        return coerceToBoolean(aValue.maValue);
    };

    auto evaluateStrictPaymentTypeArgument = [&](const formula::Node& rArgument)
        -> api::ValueResult<bool> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<bool>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole || (*oWhole != 0 && *oWhole != 1))
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
        return api::ValueResult<bool>::success(*oWhole != 0);
    };

    auto makeFiniteNumberResult = [&](double fValue) -> EvaluationResult {
        if (!std::isfinite(fValue))
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(fValue));
    };

    auto evaluateWeekendMaskArgument = [&](const formula::Node* pArgument,
                                          bool bWorkdayFunction)
        -> api::ValueResult<api::WeekendMask> {
        if (!pArgument || pArgument->meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<api::WeekendMask>::success(api::workday::defaultWeekendMask());

        EvaluationResult aWeekendValue;
        if (pArgument->meKind == formula::NodeKind::ArrayConstant)
        {
            if (pArgument->maChildren.empty())
                return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            for (const auto& pChild : pArgument->maChildren)
            {
                if (!isLiteralArrayWeekendNode(*pChild))
                    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            }

            aWeekendValue = ensureScalarValue(*this, evaluateNode(*pArgument->maChildren.front(), rCurrentAddress));
        }
        else
        {
            aWeekendValue = evaluateNode(*pArgument, rCurrentAddress);
            if (aWeekendValue && aWeekendValue.maValue.isMatrixReference())
            {
                if (!aWeekendValue.maValue.maReference.isSingleCell())
                    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
                aWeekendValue
                    = materializeReferenceValue(aWeekendValue.maValue.maReference, 0, 0);
            }
        }

        if (!aWeekendValue)
            return api::ValueResult<api::WeekendMask>::failure(aWeekendValue.meError);
        if (!aWeekendValue.maValue.isScalar())
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);

        const api::CellValue& rValue = aWeekendValue.maValue.maValue;
        if (rValue.isError())
            return api::ValueResult<api::WeekendMask>::failure(rValue.meError);
        if (rValue.isEmpty())
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);

        if (rValue.isText())
        {
            if (rValue.maString.size() != 7)
                return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            const auto aMask = api::workday::weekendMaskFromMsSpec(
                rValue.maString, bWorkdayFunction);
            if (!aMask)
                return api::ValueResult<api::WeekendMask>::failure(aMask.meError);
            return aMask;
        }

        const auto aNumber = coerceToNumber(rValue);
        if (!aNumber)
            return api::ValueResult<api::WeekendMask>::failure(aNumber.meError);
        const auto oWholeNumber = toWholeNumber(aNumber.maValue);
        if (!oWholeNumber)
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
        if ((*oWholeNumber < 1 || *oWholeNumber > 7)
            && (*oWholeNumber < 11 || *oWholeNumber > 17))
        {
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
        }

        const auto aMask = api::workday::weekendMaskFromMsSpec(
            formatNumber(static_cast<double>(*oWholeNumber)), bWorkdayFunction);
        if (!aMask)
            return api::ValueResult<api::WeekendMask>::failure(aMask.meError);
        return aMask;
    };

    auto collectHolidaySerials = [&](const formula::Node* pArgument)
        -> api::ValueResult<std::vector<api::DateSerial>> {
        std::vector<api::DateSerial> aHolidays;
        if (!pArgument || pArgument->meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::vector<api::DateSerial>>::success(aHolidays);

        const auto aVisited = visitFlattenedValues(
            visitFlattenedValues, *pArgument,
            [&](const api::CellValue& rValue, bool /*bFromReference*/) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isEmpty())
                    return api::ValueResult<bool>::success(true);

                const auto oDateSerial = coerceToDateSerial(rValue);
                if (!oDateSerial)
                    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                aHolidays.push_back(*oDateSerial);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<std::vector<api::DateSerial>>::failure(aVisited.meError);

        std::sort(aHolidays.begin(), aHolidays.end());
        aHolidays.erase(std::unique(aHolidays.begin(), aHolidays.end()), aHolidays.end());
        return api::ValueResult<std::vector<api::DateSerial>>::success(aHolidays);
    };

    if (aFunctionName == u"TRUE")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::boolean(true));
    }

    if (aFunctionName == u"FALSE")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::boolean(false));
    }

    if (aFunctionName == u"NA")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

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

    if (aFunctionName == u"FORMULA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aReference = evaluateReferenceNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aReference)
            return aReference;
        if (!aReference.maValue.isMatrixReference() || !aReference.maValue.maReference.isSingleCell())
            return makeFailure(api::Error::IllegalArgument);

        const workbook::Cell* pCell = getCell(aReference.maValue.maReference.maRange.maStart);
        if (!pCell || !pCell->hasFormula())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const formula::ParseResult aParsed = formula::parseFormula(pCell->maFormula);
        if (!aParsed)
            return makeFailure(api::Error::IllegalArgument);

        const auto oDisplay = formatFormulaNodeForDisplay(*aParsed.mpRoot);
        if (!oDisplay)
            return makeFailure(api::Error::IllegalArgument);

        api::String aFormula = u"=";
        aFormula += *oDisplay;
        return makeScalarResult(api::CellValue::text(aFormula));
    }

    if (aFunctionName == u"IF")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aCondition
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        bool bCondition = false;
        bool bConditionError = false;
        api::Error eConditionError = api::Error::None;
        if (!aCondition)
        {
            bConditionError = true;
            eConditionError = aCondition.meError;
        }
        else
        {
            const auto aBool = coerceToBoolean(aCondition.maValue.maValue);
            if (!aBool)
            {
                bConditionError = true;
                eConditionError = aBool.meError;
            }
            else
                bCondition = aBool.maValue;
        }

        const auto eAction = api::logic::selectIfBranch(
            bCondition, bConditionError, rNode.maChildren.size() >= 2, rNode.maChildren.size() >= 3);
        switch (eAction)
        {
            case api::logic::IfBranchAction::PropagateError:
                return makeFailure(eConditionError);
            case api::logic::IfBranchAction::ThenPath:
                return evaluateNode(*rNode.maChildren[1], rCurrentAddress);
            case api::logic::IfBranchAction::ElsePath:
                return evaluateNode(*rNode.maChildren[2], rCurrentAddress);
            case api::logic::IfBranchAction::ReturnTrue:
                return makeScalarResult(api::CellValue::boolean(true));
            case api::logic::IfBranchAction::ReturnFalse:
                return makeScalarResult(api::CellValue::boolean(false));
        }
    }

    if (aFunctionName == u"LET")
    {
        if (rNode.maChildren.size() < 3 || (rNode.maChildren.size() % 2) == 0)
            return makeFailure(api::Error::IllegalArgument);

        maLocalBindings.emplace_back();
        auto popBindings = [this]() { maLocalBindings.pop_back(); };

        for (std::size_t nIndex = 0; nIndex + 1 < rNode.maChildren.size() - 1; nIndex += 2)
        {
            const formula::Node& rNameNode = *rNode.maChildren[nIndex];
            if (rNameNode.meKind != formula::NodeKind::NamedReference)
            {
                popBindings();
                return makeFailure(api::Error::IllegalArgument);
            }

            EvaluationResult aValue = evaluateNode(*rNode.maChildren[nIndex + 1], rCurrentAddress);
            if (!aValue)
            {
                popBindings();
                return aValue;
            }

            maLocalBindings.back()[normalizeAsciiUpper(rNameNode.maPrimaryText)] = aValue;
        }

        EvaluationResult aResult = evaluateNode(*rNode.maChildren.back(), rCurrentAddress);
        popBindings();
        return aResult;
    }

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

    if (aFunctionName == u"ISERROR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(true));
        return makeScalarResult(api::CellValue::boolean(aArgument.maValue.maValue.isError()));
    }

    if (aFunctionName == u"ISNUMBER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (aArgument && !aArgument.maValue.isScalar())
            aArgument = materializeReferenceValue(aArgument.maValue.maReference, 0, 0);
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(false));
        return makeScalarResult(api::CellValue::boolean(
            aArgument.maValue.maValue.isNumber() || aArgument.maValue.maValue.isBoolean()));
    }

    if (aFunctionName == u"ISNA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(
                aArgument.meError == api::Error::NotAvailable));
        return makeScalarResult(api::CellValue::boolean(
            aArgument.maValue.maValue.isError()
            && aArgument.maValue.maValue.meError == api::Error::NotAvailable));
    }

    if (aFunctionName == u"IFERROR" || aFunctionName == u"IFNA")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);
        if (rNode.maChildren[0]->meKind == formula::NodeKind::EmptyArgument)
            return makeFailure(api::Error::IllegalArgument);

        const bool bNAOnly = aFunctionName == u"IFNA";
        EvaluationResult aPrimary = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aPrimary)
        {
            const auto eAction
                = api::logic::selectIfErrorAction(aPrimary.meError, bNAOnly);
            if (eAction == api::logic::IfErrorAction::KeepPrimary)
                return aPrimary;
            return evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        }

        if (aPrimary.maValue.isScalar() && aPrimary.maValue.maValue.isError())
        {
            const auto eAction = api::logic::selectIfErrorAction(
                aPrimary.maValue.maValue.meError, bNAOnly);
            if (eAction == api::logic::IfErrorAction::EvaluateAlternate)
                return evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        }

        return aPrimary;
    }

    if (aFunctionName == u"COUNTIF" || aFunctionName == u"COUNTIFS"
        || aFunctionName == u"SUMIF" || aFunctionName == u"SUMIFS"
        || aFunctionName == u"AVERAGEIF" || aFunctionName == u"AVERAGEIFS"
        || aFunctionName == u"MAXIFS" || aFunctionName == u"MINIFS")
    {
        const auto eQuerySearchType = toQuerySearchType(mrWorkbook.meFormulaSearchType);
        const EvaluatorCriteriaAggregateMaterializer aMaterializer(*this);

        auto evaluateAggregateInput = [&](const formula::Node& rArgument)
            -> api::ValueResult<CriteriaAggregateInput> {
            EvaluationResult aValue = evaluateNode(rArgument, rCurrentAddress);
            if (!aValue)
                return api::ValueResult<CriteriaAggregateInput>::failure(aValue.meError);
            const auto oInput = makeCriteriaAggregateInput(aValue);
            if (!oInput)
                return api::ValueResult<CriteriaAggregateInput>::failure(api::Error::IllegalArgument);
            return api::ValueResult<CriteriaAggregateInput>::success(*oInput);
        };

        auto evaluateCriteria = [&](const formula::Node& rArgument)
            -> api::ValueResult<CriteriaPredicate> {
            EvaluationResult aValue
                = ensureScalarValue(*this, evaluateNode(rArgument, rCurrentAddress));
            if (!aValue)
                return api::ValueResult<CriteriaPredicate>::failure(aValue.meError);

            api::CellValue aCriteriaValue = aValue.maValue.maValue;
            const bool bReferenceLikeArgument = rArgument.meKind == formula::NodeKind::CellReference
                                                || rArgument.meKind == formula::NodeKind::RangeReference
                                                || rArgument.meKind == formula::NodeKind::NamedReference;
            if (bReferenceLikeArgument && aCriteriaValue.isEmpty())
                aCriteriaValue = api::CellValue::number(0.0);

            const auto oCriteria = sequery::makeCriteriaPredicate(
                aCriteriaValue, parseStandaloneNumberText, parseAsciiDouble);
            if (!oCriteria)
                return api::ValueResult<CriteriaPredicate>::failure(api::Error::IllegalArgument);
            return api::ValueResult<CriteriaPredicate>::success(*oCriteria);
        };

        if (aFunctionName == u"COUNTIF")
        {
            if (rNode.maChildren.size() != 2)
                return makeFailure(api::Error::IllegalArgument);

            const auto aRange = evaluateAggregateInput(*rNode.maChildren[0]);
            if (!aRange)
                return makeFailure(aRange.meError);
            const auto aCriteria = evaluateCriteria(*rNode.maChildren[1]);
            if (!aCriteria)
                return makeFailure(aCriteria.meError);

            const auto aResult = sequery::evaluateCriteriaAggregate(aMaterializer,
                { aRange.maValue }, { aCriteria.maValue }, nullptr,
                CriteriaAggregateKind::Count, eQuerySearchType,
                mrWorkbook.mbSearchCriteriaMustApplyToWholeCell);
            return aResult ? makeScalarResult(aResult.maValue) : makeFailure(aResult.meError);
        }

        if (aFunctionName == u"COUNTIFS")
        {
            if (rNode.maChildren.size() < 2 || (rNode.maChildren.size() % 2) != 0)
                return makeFailure(api::Error::IllegalArgument);

            std::vector<CriteriaAggregateInput> aRanges;
            std::vector<CriteriaPredicate> aCriteria;
            aRanges.reserve(rNode.maChildren.size() / 2);
            aCriteria.reserve(rNode.maChildren.size() / 2);
            for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); nIndex += 2)
            {
                const auto aRange = evaluateAggregateInput(*rNode.maChildren[nIndex]);
                if (!aRange)
                    return makeFailure(aRange.meError);
                const auto aCriterion = evaluateCriteria(*rNode.maChildren[nIndex + 1]);
                if (!aCriterion)
                    return makeFailure(aCriterion.meError);
                aRanges.push_back(aRange.maValue);
                aCriteria.push_back(aCriterion.maValue);
            }

            const auto aResult = sequery::evaluateCriteriaAggregate(aMaterializer, aRanges,
                aCriteria, nullptr, CriteriaAggregateKind::Count, eQuerySearchType,
                mrWorkbook.mbSearchCriteriaMustApplyToWholeCell);
            return aResult ? makeScalarResult(aResult.maValue) : makeFailure(aResult.meError);
        }

        if (aFunctionName == u"SUMIF" || aFunctionName == u"AVERAGEIF")
        {
            if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
                return makeFailure(api::Error::IllegalArgument);

            const auto aCriteriaRange = evaluateAggregateInput(*rNode.maChildren[0]);
            if (!aCriteriaRange)
                return makeFailure(aCriteriaRange.meError);
            const auto aCriteria = evaluateCriteria(*rNode.maChildren[1]);
            if (!aCriteria)
                return makeFailure(aCriteria.meError);

            std::optional<CriteriaAggregateInput> oTargetRange;
            if (rNode.maChildren.size() == 3)
            {
                const auto aTarget = evaluateAggregateInput(*rNode.maChildren[2]);
                if (!aTarget)
                    return makeFailure(aTarget.meError);
                oTargetRange = aTarget.maValue;
            }

            const auto aResult = sequery::evaluateCriteriaAggregate(aMaterializer,
                { aCriteriaRange.maValue }, { aCriteria.maValue },
                oTargetRange ? &*oTargetRange : nullptr,
                aFunctionName == u"SUMIF" ? CriteriaAggregateKind::Sum
                                          : CriteriaAggregateKind::Average,
                eQuerySearchType,
                mrWorkbook.mbSearchCriteriaMustApplyToWholeCell);
            return aResult ? makeScalarResult(aResult.maValue) : makeFailure(aResult.meError);
        }

        if (rNode.maChildren.size() < 3 || (rNode.maChildren.size() % 2) == 0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTargetRange = evaluateAggregateInput(*rNode.maChildren[0]);
        if (!aTargetRange)
            return makeFailure(aTargetRange.meError);

        std::vector<CriteriaAggregateInput> aRanges;
        std::vector<CriteriaPredicate> aCriteria;
        aRanges.reserve((rNode.maChildren.size() - 1) / 2);
        aCriteria.reserve((rNode.maChildren.size() - 1) / 2);
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); nIndex += 2)
        {
            const auto aRange = evaluateAggregateInput(*rNode.maChildren[nIndex]);
            if (!aRange)
                return makeFailure(aRange.meError);
            const auto aCriterion = evaluateCriteria(*rNode.maChildren[nIndex + 1]);
            if (!aCriterion)
                return makeFailure(aCriterion.meError);
            aRanges.push_back(aRange.maValue);
            aCriteria.push_back(aCriterion.maValue);
        }

        CriteriaAggregateKind eAggregateKind = CriteriaAggregateKind::Sum;
        if (aFunctionName == u"AVERAGEIFS")
            eAggregateKind = CriteriaAggregateKind::Average;
        else if (aFunctionName == u"MAXIFS")
            eAggregateKind = CriteriaAggregateKind::Max;
        else if (aFunctionName == u"MINIFS")
            eAggregateKind = CriteriaAggregateKind::Min;

        const auto aResult = sequery::evaluateCriteriaAggregate(aMaterializer, aRanges, aCriteria,
            &aTargetRange.maValue, eAggregateKind, eQuerySearchType,
            mrWorkbook.mbSearchCriteriaMustApplyToWholeCell);
        return aResult ? makeScalarResult(aResult.maValue) : makeFailure(aResult.meError);
    }

    if (aFunctionName == u"T.TEST" || aFunctionName == u"TTEST")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aTailsResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aTailsResult)
            return aTailsResult;
        const auto aTailsNumber = coerceToNumber(aTailsResult.maValue.maValue);
        if (!aTailsNumber)
            return makeFailure(aTailsNumber.meError);

        EvaluationResult aTypeResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aTypeResult)
            return aTypeResult;
        const auto aTypeNumber = coerceToNumber(aTypeResult.maValue.maValue);
        if (!aTypeNumber)
            return makeFailure(aTypeNumber.meError);

        const auto oTails = toWholeNumber(aTailsNumber.maValue);
        const auto oType = toWholeNumber(aTypeNumber.maValue);
        if (!oTails || !oType || (*oTails != 1 && *oTails != 2) || (*oType < 1 || *oType > 3)
            || *oType == 1)
        {
            return makeScalarResult(api::CellValue::error(api::Error::NoValue));
        }

        return makeFailure(api::Error::IllegalArgument);
    }

    if (aFunctionName == u"FISHER")
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
        const auto aFisher = semath::fisherTransform(aNumber.maValue);
        if (!aFisher)
            return makeFailure(aFisher.meError);
        return makeScalarResult(api::CellValue::number(aFisher.maValue));
    }

    if (aFunctionName == u"FISHERINV")
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
        return makeScalarResult(
            api::CellValue::number(semath::inverseFisherTransform(aNumber.maValue)));
    }

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

    if (aFunctionName == u"GAUSS")
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

        return makeScalarResult(api::CellValue::number(gaussValue(aNumber.maValue)));
    }

    if (aFunctionName == u"GAMMALN" || aFunctionName == u"GAMMALN.PRECISE")
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
        if (!(aNumber.maValue > 0.0))
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(std::lgamma(aNumber.maValue)));
    }

    if (aFunctionName == u"GCD" || aFunctionName == u"LCM")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = collectNumericArguments(true, true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        sal_Int64 nResult = 0;
        bool bSawValue = false;
        for (const double fValue : aNumbers.maValue)
        {
            if (!std::isfinite(fValue) || fValue < 0.0)
                return makeFailure(api::Error::IllegalArgument);

            const auto fTruncated = std::trunc(fValue);
            if (fTruncated < static_cast<double>(std::numeric_limits<sal_Int64>::min())
                || fTruncated > static_cast<double>(std::numeric_limits<sal_Int64>::max()))
            {
                return makeFailure(api::Error::IllegalArgument);
            }

            const sal_Int64 nValue = static_cast<sal_Int64>(fTruncated);
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

    if (aFunctionName == u"GEOMEAN" || aFunctionName == u"HARMEAN")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = collectNumericArguments(true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        if (aFunctionName == u"GEOMEAN")
        {
            KahanSum fLogSum = 0.0;
            for (const double fValue : aNumbers.maValue)
            {
                if (!(fValue > 0.0))
                    return makeFailure(api::Error::IllegalArgument);
                fLogSum += std::log(fValue);
            }

            return makeScalarResult(api::CellValue::number(
                std::exp(fLogSum.get() / static_cast<double>(aNumbers.maValue.size()))));
        }

        KahanSum fInverseSum = 0.0;
        for (const double fValue : aNumbers.maValue)
        {
            if (!(fValue > 0.0))
                return makeFailure(api::Error::IllegalArgument);
            fInverseSum += 1.0 / fValue;
        }

        if (::rtl::math::approxEqual(fInverseSum.get(), 0.0))
            return makeFailure(api::Error::DivisionByZero);

        return makeScalarResult(api::CellValue::number(
            static_cast<double>(aNumbers.maValue.size()) / fInverseSum.get()));
    }

    if (aFunctionName == u"FV" || aFunctionName == u"PV" || aFunctionName == u"PMT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aNper = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aNper)
            return makeFailure(aNper.meError);
        const auto aPayment = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFuture = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aFuture)
                return makeFailure(aFuture.meError);
            fFutureValue = aFuture.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 5)
        {
            const auto aPayType = evaluatePayTypeArgument(*rNode.maChildren[4], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        if (aFunctionName == u"FV")
        {
            return makeFiniteNumberResult(api::math::futureValue(
                aRate.maValue, aNper.maValue, aPayment.maValue, fFutureValue,
                bPayInAdvance));
        }

        if (aFunctionName == u"PV")
        {
            return makeFiniteNumberResult(api::math::presentValue(
                aRate.maValue, aNper.maValue, aPayment.maValue, fFutureValue,
                bPayInAdvance));
        }

        if (aFunctionName == u"PMT")
        {
            if (::rtl::math::approxEqual(aNper.maValue, 0.0))
                return makeFailure(api::Error::IllegalArgument);

            return makeFiniteNumberResult(api::math::payment(
                aRate.maValue, aNper.maValue, aPayment.maValue, fFutureValue,
                bPayInAdvance));
        }

    }

    if (aFunctionName == u"NPER")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPayment = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);
        const auto aPresentValue = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFutureValue = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 5)
        {
            const auto aPayType = evaluatePayTypeArgument(*rNode.maChildren[4], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        const double fResult = api::math::periodsForFutureValue(
            aRate.maValue, aPayment.maValue, aPresentValue.maValue, fFutureValue,
            bPayInAdvance);
        return makeFiniteNumberResult(fResult);
    }

    if (aFunctionName == u"RATE")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aNper = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aNper)
            return makeFailure(aNper.meError);
        const auto aPayment = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);
        const auto aPresentValue = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFutureValue = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
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
                const auto aPayTypeValue = evaluateScalarArgumentValue(*rNode.maChildren[4]);
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
                const auto aGuessValue = evaluateScalarArgumentValue(*rNode.maChildren[5]);
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

        if (!(aNper.maValue > 0.0))
            return makeFailure(api::Error::IllegalArgument);

        const auto aRateResult = api::math::solveRate(
            aNper.maValue, aPayment.maValue, aPresentValue.maValue, fFutureValue,
            bPayInAdvance, fGuess, true);
        if (!aRateResult.mbConverged || !std::isfinite(aRateResult.mfRate))
            return makeFailure(aRateResult.meError);
        return makeScalarResult(api::CellValue::number(aRateResult.mfRate));
    }

    if (aFunctionName == u"ISPMT")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPeriod = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);
        const auto aTotalPeriods = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aInvestment = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aInvestment)
            return makeFailure(aInvestment.meError);

        if (::rtl::math::approxEqual(aTotalPeriods.maValue, 0.0))
            return makeFailure(api::Error::IllegalArgument);

        return makeFiniteNumberResult(api::math::interestSchedulePayment(
            aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue, aInvestment.maValue));
    }

    if (aFunctionName == u"IPMT" || aFunctionName == u"PPMT")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPeriod = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);
        const auto aTotalPeriods = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aPresentValue = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 5)
        {
            const auto aFutureValue = evaluateNumericArgument(*rNode.maChildren[4], 0.0);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 6)
        {
            const auto aPayType = evaluatePayTypeArgument(*rNode.maChildren[5], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        if (!(aTotalPeriods.maValue > 0.0) || !(aPeriod.maValue >= 1.0)
            || aPeriod.maValue > aTotalPeriods.maValue)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        if (aFunctionName == u"IPMT")
        {
            const auto aInterest = api::math::interestPayment(
                aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue,
                aPresentValue.maValue, fFutureValue, bPayInAdvance);
            return makeFiniteNumberResult(aInterest.mfInterest);
        }

        return makeFiniteNumberResult(api::math::principalPayment(
            aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
            fFutureValue, bPayInAdvance));
    }

    if (aFunctionName == u"CUMIPMT" || aFunctionName == u"CUMPRINC")
    {
        if (rNode.maChildren.size() != 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aTotalPeriods = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aPresentValue = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);
        const auto aStart = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aEnd = evaluateNumericArgument(*rNode.maChildren[4], 0.0);
        if (!aEnd)
            return makeFailure(aEnd.meError);
        const auto aPayType = evaluateStrictPaymentTypeArgument(*rNode.maChildren[5]);
        if (!aPayType)
            return makeFailure(aPayType.meError);

        if (!(aRate.maValue > 0.0) || !(aTotalPeriods.maValue > 0.0)
            || !(aPresentValue.maValue > 0.0) || !(aStart.maValue >= 1.0)
            || !(aEnd.maValue >= aStart.maValue))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        const auto oWholeStart = toWholeNumber(aStart.maValue);
        const auto oWholeEnd = toWholeNumber(aEnd.maValue);
        if (!oWholeStart || !oWholeEnd)
            return makeFailure(api::Error::IllegalArgument);

        if (aFunctionName == u"CUMIPMT")
        {
            return makeFiniteNumberResult(api::math::cumulativeInterest(
                aRate.maValue, static_cast<double>(*oWholeStart),
                static_cast<double>(*oWholeEnd), aTotalPeriods.maValue,
                aPresentValue.maValue, 0.0, aPayType.maValue));
        }

        return makeFiniteNumberResult(api::math::cumulativePrincipal(
            aRate.maValue, static_cast<double>(*oWholeStart),
            static_cast<double>(*oWholeEnd), aTotalPeriods.maValue,
            aPresentValue.maValue, 0.0, aPayType.maValue));
    }

    if (aFunctionName == u"DDB")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCost = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aCost)
            return makeFailure(aCost.meError);
        const auto aSalvage = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aSalvage)
            return makeFailure(aSalvage.meError);
        const auto aLife = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aLife)
            return makeFailure(aLife.meError);
        const auto aPeriod = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);

        double fFactor = 2.0;
        if (rNode.maChildren.size() == 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);
            const auto aFactor = evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aFactor)
                return makeFailure(aFactor.meError);
            fFactor = aFactor.maValue;
        }

        if (!(aCost.maValue > 0.0) || aSalvage.maValue < 0.0 || aCost.maValue < aSalvage.maValue
            || !(aLife.maValue > 0.0) || !(aPeriod.maValue > 0.0)
            || aPeriod.maValue > aLife.maValue || !(fFactor > 0.0))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        return makeFiniteNumberResult(api::math::doubleDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aPeriod.maValue, fFactor));
    }

    if (aFunctionName == u"VDB")
    {
        if (rNode.maChildren.size() < 5 || rNode.maChildren.size() > 7)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCost = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aCost)
            return makeFailure(aCost.meError);
        const auto aSalvage = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aSalvage)
            return makeFailure(aSalvage.meError);
        const auto aLife = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aLife)
            return makeFailure(aLife.meError);
        const auto aStart = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aStart)
            return makeFailure(aStart.meError);
        if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
            return makeFailure(api::Error::IllegalArgument);
        const auto aEnd = evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
        if (!aEnd)
            return makeFailure(aEnd.meError);

        double fFactor = 2.0;
        if (rNode.maChildren.size() >= 6)
        {
            if (rNode.maChildren[5]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);
            const auto aFactor = evaluateNumericArgument(*rNode.maChildren[5], std::nullopt);
            if (!aFactor)
                return makeFailure(aFactor.meError);
            fFactor = aFactor.maValue;
        }

        bool bNoSwitch = false;
        if (rNode.maChildren.size() == 7)
        {
            const auto aNoSwitch = evaluatePayTypeArgument(*rNode.maChildren[6], false);
            if (!aNoSwitch)
                return makeFailure(aNoSwitch.meError);
            bNoSwitch = aNoSwitch.maValue;
        }

        if (!(aCost.maValue > 0.0) || aSalvage.maValue < 0.0 || aCost.maValue < aSalvage.maValue
            || !(aLife.maValue > 0.0) || aStart.maValue < 0.0 || aEnd.maValue < aStart.maValue
            || aEnd.maValue > aLife.maValue || !(fFactor > 0.0))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        return makeFiniteNumberResult(api::math::variableDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aStart.maValue,
            aEnd.maValue, fFactor, bNoSwitch));
    }

    if (aFunctionName == u"POISSON" || aFunctionName == u"POISSON.DIST")
    {
        const bool bLegacyPoisson = aFunctionName == u"POISSON";
        if ((bLegacyPoisson && (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3))
            || (!bLegacyPoisson && rNode.maChildren.size() != 3))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);

        EvaluationResult aLambdaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aLambdaResult)
            return aLambdaResult;
        const auto aLambdaNumber = coerceToNumber(aLambdaResult.maValue.maValue);
        if (!aLambdaNumber)
            return makeFailure(aLambdaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aPoisson = semath::evaluatePoissonDistribution(
            aXNumber.maValue, aLambdaNumber.maValue, bCumulative);
        if (!aPoisson)
            return makeFailure(aPoisson.meError);
        return makeScalarResult(api::CellValue::number(aPoisson.maValue));
    }

    if (aFunctionName == u"LEGACY.CHIDIST")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aChiResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aChiResult)
            return aChiResult;
        EvaluationResult aDfResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDfResult)
            return aDfResult;

        const auto aChiNumber = coerceToNumber(aChiResult.maValue.maValue);
        if (!aChiNumber)
            return makeFailure(aChiNumber.meError);
        const auto aDfNumber = coerceToNumber(aDfResult.maValue.maValue);
        if (!aDfNumber)
            return makeFailure(aDfNumber.meError);

        const auto aChiDist = evaluateLegacyChiDist(
            aChiNumber.maValue, ::rtl::math::approxFloor(aDfNumber.maValue));
        if (!aChiDist)
            return makeFailure(aChiDist.meError);
        return makeScalarResult(api::CellValue::number(aChiDist.maValue));
    }

    if (aFunctionName == u"CHISQDIST" || aFunctionName == u"CHISQ.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName == u"CHISQ.DIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 3)
            || (!bMicrosoftSyntax
                && (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aDfResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aDfResult)
            return aDfResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aDfNumber = coerceToNumber(aDfResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aDfNumber)
            return makeFailure(aDfNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = evaluateChiSquareDistribution(
            aXNumber.maValue, ::rtl::math::approxFloor(aDfNumber.maValue), bCumulative,
            bMicrosoftSyntax);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"NORMDIST" || aFunctionName == u"NORM.DIST")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aMeanResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aSigmaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aMeanResult)
            return aMeanResult;
        if (!aSigmaResult)
            return aSigmaResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aMeanNumber = coerceToNumber(aMeanResult.maValue.maValue);
        const auto aSigmaNumber = coerceToNumber(aSigmaResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aMeanNumber)
            return makeFailure(aMeanNumber.meError);
        if (!aSigmaNumber)
            return makeFailure(aSigmaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = evaluateNormalDistribution(
            aXNumber.maValue, aMeanNumber.maValue, aSigmaNumber.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"LOGNORMDIST" || aFunctionName == u"LOGNORM.DIST"
        || aFunctionName == u"COM.MICROSOFT.LOGNORM.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName != u"LOGNORMDIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 4)
            || (!bMicrosoftSyntax
                && (rNode.maChildren.empty() || rNode.maChildren.size() > 4)))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        const auto aXNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);

        const auto aMeanNumber = rNode.maChildren.size() >= 2
                                     ? evaluateNumericArgument(*rNode.maChildren[1], 0.0)
                                     : api::ValueResult<double>::success(0.0);
        if (!aMeanNumber)
            return makeFailure(aMeanNumber.meError);

        const auto aSigmaNumber = rNode.maChildren.size() >= 3
                                      ? evaluateNumericArgument(*rNode.maChildren[2], 1.0)
                                      : api::ValueResult<double>::success(1.0);
        if (!aSigmaNumber)
            return makeFailure(aSigmaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = evaluateLogNormalDistribution(
            aXNumber.maValue, aMeanNumber.maValue, aSigmaNumber.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"GAMMADIST" || aFunctionName == u"GAMMA.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName == u"GAMMA.DIST";
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aAlphaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aBetaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aAlphaResult)
            return aAlphaResult;
        if (!aBetaResult)
            return aBetaResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aAlphaNumber = coerceToNumber(aAlphaResult.maValue.maValue);
        const auto aBetaNumber = coerceToNumber(aBetaResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aAlphaNumber)
            return makeFailure(aAlphaNumber.meError);
        if (!aBetaNumber)
            return makeFailure(aBetaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = evaluateGammaDistribution(
            aXNumber.maValue, aAlphaNumber.maValue, aBetaNumber.maValue, bCumulative,
            bMicrosoftSyntax);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"GAMMA" || aFunctionName == u"COM.MICROSOFT.GAMMA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aNumber)
            return makeFailure(aNumber.meError);

        const auto aGammaValue = evaluateGammaValue(aNumber.maValue);
        if (!aGammaValue)
            return makeFailure(aGammaValue.meError);
        return makeScalarResult(api::CellValue::number(aGammaValue.maValue));
    }

    if (aFunctionName == u"TINV" || aFunctionName == u"T.INV.2T"
        || aFunctionName == u"COM.MICROSOFT.T.INV.2T")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aProbability = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aProbability)
            return makeFailure(aProbability.meError);
        if (!aDegreesFreedom)
            return makeFailure(aDegreesFreedom.meError);

        const auto aInverse = evaluateTInverse(
            aProbability.maValue, ::rtl::math::approxFloor(aDegreesFreedom.maValue), 2);
        if (!aInverse)
            return makeFailure(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"T.DIST.2T" || aFunctionName == u"COM.MICROSOFT.T.DIST.2T")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aX = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aX)
            return makeFailure(aX.meError);
        if (!aDegreesFreedom)
            return makeFailure(aDegreesFreedom.meError);
        if (aX.maValue < 0.0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDistribution = evaluateStudentDistribution(
            aX.maValue, ::rtl::math::approxFloor(aDegreesFreedom.maValue), 2);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"FINV" || aFunctionName == u"LEGACY.FINV"
        || aFunctionName == u"F.INV.RT" || aFunctionName == u"COM.MICROSOFT.F.INV.RT"
        || aFunctionName == u"F.INV" || aFunctionName == u"COM.MICROSOFT.F.INV")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aProbability = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom1 = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aDegreesFreedom2 = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aProbability)
            return makeFailure(aProbability.meError);
        if (!aDegreesFreedom1)
            return makeFailure(aDegreesFreedom1.meError);
        if (!aDegreesFreedom2)
            return makeFailure(aDegreesFreedom2.meError);

        const double fDegreesFreedom1 = ::rtl::math::approxFloor(aDegreesFreedom1.maValue);
        const double fDegreesFreedom2 = ::rtl::math::approxFloor(aDegreesFreedom2.maValue);
        const bool bLeftTail = aFunctionName == u"FINV" || aFunctionName == u"F.INV"
                               || aFunctionName == u"COM.MICROSOFT.F.INV";
        if (bLeftTail && (aProbability.maValue <= 0.0 || aProbability.maValue >= 1.0))
            return makeFailure(api::Error::IllegalArgument);
        const double fRightTailProbability
            = bLeftTail ? 1.0 - aProbability.maValue : aProbability.maValue;
        const auto aInverse = evaluateFInverseRightTail(
            fRightTailProbability, fDegreesFreedom1, fDegreesFreedom2);
        if (!aInverse)
            return makeFailure(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"VAR" || aFunctionName == u"VAR.S" || aFunctionName == u"VARP"
        || aFunctionName == u"VAR.P" || aFunctionName == u"VARA" || aFunctionName == u"VARPA"
        || aFunctionName == u"STDEV" || aFunctionName == u"STDEV.S"
        || aFunctionName == u"STDEVP" || aFunctionName == u"STDEV.P"
        || aFunctionName == u"STDEVA" || aFunctionName == u"STDEVPA")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const bool bTextAsZero = aFunctionName == u"VARA" || aFunctionName == u"VARPA"
                                 || aFunctionName == u"STDEVA" || aFunctionName == u"STDEVPA";
        const auto aNumbers = collectVarianceArguments(bTextAsZero);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);

        const bool bSample = aFunctionName == u"VAR" || aFunctionName == u"VAR.S"
                             || aFunctionName == u"VARA" || aFunctionName == u"STDEV"
                             || aFunctionName == u"STDEV.S" || aFunctionName == u"STDEVA";
        const bool bReturnStdDev = aFunctionName == u"STDEV" || aFunctionName == u"STDEV.S"
                                   || aFunctionName == u"STDEVP"
                                   || aFunctionName == u"STDEV.P"
                                   || aFunctionName == u"STDEVA"
                                   || aFunctionName == u"STDEVPA";
        const auto aVariance = evaluateVarianceNumbers(aNumbers.maValue, bSample, bReturnStdDev);
        if (!aVariance)
            return makeFailure(aVariance.meError);
        return makeScalarResult(api::CellValue::number(aVariance.maValue));
    }

    if (aFunctionName == u"BINOMDIST" || aFunctionName == u"BINOM.DIST")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aNResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aPResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        EvaluationResult aCumulativeResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aNResult)
            return aNResult;
        if (!aPResult)
            return aPResult;
        if (!aCumulativeResult)
            return aCumulativeResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aNNumber = coerceToNumber(aNResult.maValue.maValue);
        const auto aPNumber = coerceToNumber(aPResult.maValue.maValue);
        const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aNNumber)
            return makeFailure(aNNumber.meError);
        if (!aPNumber)
            return makeFailure(aPNumber.meError);
        if (!aCumulativeBool)
            return makeFailure(aCumulativeBool.meError);

        const auto aBinomial = semath::evaluateBinomialDistribution(
            aXNumber.maValue, aNNumber.maValue, aPNumber.maValue, aCumulativeBool.maValue);
        if (!aBinomial)
            return makeFailure(aBinomial.meError);
        return makeScalarResult(api::CellValue::number(aBinomial.maValue));
    }

    if (aFunctionName == u"BINOM.INV")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTrials = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aProbability = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aAlpha = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aTrials)
            return makeFailure(aTrials.meError);
        if (!aProbability)
            return makeFailure(aProbability.meError);
        if (!aAlpha)
            return makeFailure(aAlpha.meError);

        const auto aInverse = evaluateBinomialInverse(
            aTrials.maValue, aProbability.maValue, aAlpha.maValue);
        if (!aInverse)
            return makeFailure(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"BINOM.DIST.RANGE" || aFunctionName == u"B")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aPResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aStartResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aNResult)
            return aNResult;
        if (!aPResult)
            return aPResult;
        if (!aStartResult)
            return aStartResult;

        const auto aNNumber = coerceToNumber(aNResult.maValue.maValue);
        const auto aPNumber = coerceToNumber(aPResult.maValue.maValue);
        const auto aStartNumber = coerceToNumber(aStartResult.maValue.maValue);
        if (!aNNumber)
            return makeFailure(aNNumber.meError);
        if (!aPNumber)
            return makeFailure(aPNumber.meError);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);

        double fEnd = aStartNumber.maValue;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aEndResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aEndResult)
                return aEndResult;
            const auto aEndNumber = coerceToNumber(aEndResult.maValue.maValue);
            if (!aEndNumber)
                return makeFailure(aEndNumber.meError);
            fEnd = aEndNumber.maValue;
        }

        const auto aRange = semath::evaluateBinomialRangeDistribution(
            aNNumber.maValue, aPNumber.maValue, aStartNumber.maValue, fEnd);
        if (!aRange)
            return makeFailure(aRange.meError);
        return makeScalarResult(api::CellValue::number(aRange.maValue));
    }

    if (aFunctionName == u"HYPGEOMDIST" || aFunctionName == u"HYPGEOM.DIST")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aX = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aTrials = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aSuccesses = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        const auto aPopulation = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
        if (!aX)
            return makeFailure(aX.meError);
        if (!aTrials)
            return makeFailure(aTrials.meError);
        if (!aSuccesses)
            return makeFailure(aSuccesses.meError);
        if (!aPopulation)
            return makeFailure(aPopulation.meError);

        bool bCumulative = false;
        if (rNode.maChildren.size() == 5)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[4], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = evaluateHypergeometricDistribution(
            aX.maValue, aTrials.maValue, aSuccesses.maValue, aPopulation.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"PERCENTRANK" || aFunctionName == u"PERCENTRANK.INC"
        || aFunctionName == u"PERCENTRANK.EXC")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aScan = collectAggregateScanFromArgument(*rNode.maChildren[0]);
        if (!aScan)
            return makeFailure(aScan.meError);

        const auto aValue = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        sal_Int32 nSignificance = 3;
        if (rNode.maChildren.size() == 3)
        {
            const auto aSignificance = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aSignificance)
                return makeFailure(aSignificance.meError);
            nSignificance = static_cast<sal_Int32>(::rtl::math::approxFloor(aSignificance.maValue));
        }

        const bool bInclusive = aFunctionName != u"PERCENTRANK.EXC";
        const auto aRank = evaluatePercentrank(
            aScan.maValue.maNumbers, aValue.maValue, bInclusive, nSignificance);
        if (!aRank)
            return makeFailure(aRank.meError);
        return makeScalarResult(api::CellValue::number(aRank.maValue));
    }

    if (aFunctionName == u"MODE.SNGL")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        AggregateScan aScan;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aCollected = collectAggregateScanFromArgument(*pChild);
            if (!aCollected)
                return makeFailure(aCollected.meError);
            aScan.maNumbers.insert(aScan.maNumbers.end(), aCollected.maValue.maNumbers.begin(),
                aCollected.maValue.maNumbers.end());
        }

        const auto aMode = evaluateModeSingle(aScan.maNumbers);
        if (!aMode)
            return makeFailure(aMode.meError);
        return makeScalarResult(api::CellValue::number(aMode.maValue));
    }

    if (aFunctionName == u"TRIMMEAN")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aScan = collectAggregateScanFromArgument(*rNode.maChildren[0]);
        if (!aScan)
            return makeFailure(aScan.meError);

        const auto aPercent = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aPercent)
            return makeFailure(aPercent.meError);

        const auto aTrimmean = evaluateTrimmean(aScan.maValue.maNumbers, aPercent.maValue);
        if (!aTrimmean)
            return makeFailure(aTrimmean.meError);
        return makeScalarResult(api::CellValue::number(aTrimmean.maValue));
    }

    if (aFunctionName == u"CHISQ.TEST" || aFunctionName == u"LEGACY.CHITEST")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aObservedInput = evaluateLookupInputNode(*rNode.maChildren[0]);
        const auto aExpectedInput = evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aObservedInput)
            return makeFailure(aObservedInput.meError);
        if (!aExpectedInput)
            return makeFailure(aExpectedInput.meError);

        const api::MatrixSize nObservedColumns = aObservedInput.maValue.mbScalar
                                                     ? 1
                                                     : aObservedInput.maValue.mnColumns;
        const api::MatrixSize nObservedRows = aObservedInput.maValue.mbScalar ? 1
                                                                              : aObservedInput.maValue.mnRows;
        const api::MatrixSize nExpectedColumns = aExpectedInput.maValue.mbScalar
                                                     ? 1
                                                     : aExpectedInput.maValue.mnColumns;
        const api::MatrixSize nExpectedRows = aExpectedInput.maValue.mbScalar ? 1
                                                                              : aExpectedInput.maValue.mnRows;
        if (nObservedColumns != nExpectedColumns || nObservedRows != nExpectedRows)
            return makeFailure(api::Error::IllegalArgument);

        auto materializeInputCell = [&](const LookupInput& rInput, api::MatrixSize nColumn,
                                        api::MatrixSize nRow)
            -> api::ValueResult<api::CellValue> {
            if (rInput.mbScalar)
            {
                if (nColumn != 0 || nRow != 0)
                    return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
                return api::ValueResult<api::CellValue>::success(rInput.maScalar);
            }

            const std::size_t nIndex = static_cast<std::size_t>(nRow * rInput.mnColumns + nColumn);
            if (!rInput.maValues.empty())
            {
                if (nIndex >= rInput.maValues.size())
                    return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
                return api::ValueResult<api::CellValue>::success(rInput.maValues[nIndex]);
            }

            EvaluationResult aCell = materializeReferenceValue(rInput.maReference, nColumn, nRow);
            if (!aCell)
                return api::ValueResult<api::CellValue>::failure(aCell.meError);
            if (!aCell.maValue.isScalar())
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(aCell.maValue.maValue);
        };

        KahanSum fChi = 0.0;
        bool bSawNonEmptyPair = false;
        for (api::MatrixSize nColumn = 0; nColumn < nObservedColumns; ++nColumn)
        {
            for (api::MatrixSize nRow = 0; nRow < nObservedRows; ++nRow)
            {
                const auto aObservedCell = materializeInputCell(aObservedInput.maValue, nColumn, nRow);
                const auto aExpectedCell = materializeInputCell(aExpectedInput.maValue, nColumn, nRow);
                if (!aObservedCell)
                    return makeFailure(aObservedCell.meError);
                if (!aExpectedCell)
                    return makeFailure(aExpectedCell.meError);

                if (aObservedCell.maValue.isEmpty() || aExpectedCell.maValue.isEmpty())
                    continue;

                bSawNonEmptyPair = true;
                if (aObservedCell.maValue.isText() || aExpectedCell.maValue.isText())
                    return makeFailure(api::Error::IllegalArgument);
                if (aObservedCell.maValue.isError())
                    return makeFailure(aObservedCell.maValue.meError);
                if (aExpectedCell.maValue.isError())
                    return makeFailure(aExpectedCell.maValue.meError);

                const auto aObservedNumber = coerceToNumber(aObservedCell.maValue);
                const auto aExpectedNumber = coerceToNumber(aExpectedCell.maValue);
                if (!aObservedNumber)
                    return makeFailure(aObservedNumber.meError);
                if (!aExpectedNumber)
                    return makeFailure(aExpectedNumber.meError);
                if (::rtl::math::approxEqual(aExpectedNumber.maValue, 0.0))
                    return makeFailure(api::Error::DivisionByZero);

                const double fDifference = aObservedNumber.maValue - aExpectedNumber.maValue;
                const double fTerm = (fDifference * fDifference) / aExpectedNumber.maValue;
                if (std::isinf(fTerm))
                    return makeFailure(api::Error::NoConvergence);
                fChi += fTerm;
            }
        }

        if (!bSawNonEmptyPair)
            return makeFailure(api::Error::IllegalArgument);

        double fDegreesFreedom = 0.0;
        if (nObservedColumns == 1 || nObservedRows == 1)
        {
            fDegreesFreedom = static_cast<double>(nObservedColumns * nObservedRows - 1);
            if (::rtl::math::approxEqual(fDegreesFreedom, 0.0))
                return makeFailure(api::Error::NotAvailable);
        }
        else
        {
            fDegreesFreedom
                = static_cast<double>(nObservedColumns - 1) * static_cast<double>(nObservedRows - 1);
        }

        const auto aChiDist = evaluateLegacyChiDist(fChi.get(), fDegreesFreedom);
        if (!aChiDist)
            return makeFailure(aChiDist.meError);
        return makeScalarResult(api::CellValue::number(aChiDist.maValue));
    }

    if (aFunctionName == u"BETADIST" || aFunctionName == u"BETA.DIST")
    {
        const bool bMicrosoftOrder = aFunctionName == u"BETA.DIST";
        if ((bMicrosoftOrder && (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6))
            || (!bMicrosoftOrder && (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        auto evaluateScalarNumber = [&](std::size_t nIndex) -> api::ValueResult<double> {
            EvaluationResult aValue
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress));
            if (!aValue)
                return api::ValueResult<double>::failure(aValue.meError);
            return coerceToNumber(aValue.maValue.maValue);
        };

        const auto aXNumber = evaluateScalarNumber(0);
        const auto aAlphaNumber = evaluateScalarNumber(1);
        const auto aBetaNumber = evaluateScalarNumber(2);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aAlphaNumber)
            return makeFailure(aAlphaNumber.meError);
        if (!aBetaNumber)
            return makeFailure(aBetaNumber.meError);

        bool bCumulative = true;
        double fLowerBound = 0.0;
        double fUpperBound = 1.0;
        if (bMicrosoftOrder)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
            if (rNode.maChildren.size() >= 5)
            {
                const auto aLower = evaluateScalarNumber(4);
                if (!aLower)
                    return makeFailure(aLower.meError);
                fLowerBound = aLower.maValue;
            }
            if (rNode.maChildren.size() >= 6)
            {
                const auto aUpper = evaluateScalarNumber(5);
                if (!aUpper)
                    return makeFailure(aUpper.meError);
                fUpperBound = aUpper.maValue;
            }
        }
        else
        {
            if (rNode.maChildren.size() >= 4)
            {
                const auto aLower = evaluateScalarNumber(3);
                if (!aLower)
                    return makeFailure(aLower.meError);
                fLowerBound = aLower.maValue;
            }
            if (rNode.maChildren.size() >= 5)
            {
                const auto aUpper = evaluateScalarNumber(4);
                if (!aUpper)
                    return makeFailure(aUpper.meError);
                fUpperBound = aUpper.maValue;
            }
            if (rNode.maChildren.size() == 6)
            {
                EvaluationResult aCumulativeResult = ensureScalarValue(
                    *this, evaluateNode(*rNode.maChildren[5], rCurrentAddress));
                if (!aCumulativeResult)
                    return aCumulativeResult;
                const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
                if (!aCumulativeBool)
                    return makeFailure(aCumulativeBool.meError);
                bCumulative = aCumulativeBool.maValue;
            }
        }

        const auto aBetaDistribution = semath::evaluateBetaDistribution(aXNumber.maValue,
            aAlphaNumber.maValue, aBetaNumber.maValue, fLowerBound, fUpperBound, bCumulative,
            bMicrosoftOrder);
        if (!aBetaDistribution)
            return makeFailure(aBetaDistribution.meError);
        return makeScalarResult(api::CellValue::number(aBetaDistribution.maValue));
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

    if (aFunctionName == u"VALUE" || aFunctionName == u"DATEVALUE" || aFunctionName == u"TIMEVALUE")
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

        const auto oParsed = parseStandaloneNumberText(aText.maValue);
        if (!oParsed)
            return makeFailure(api::Error::IllegalArgument);

        if (aFunctionName == u"VALUE")
            return makeScalarResult(api::CellValue::number(oParsed->mfValue));

        if (aFunctionName == u"DATEVALUE")
        {
            if (oParsed->meKind != api::NumberParseResult::Kind::Date
                && oParsed->meKind != api::NumberParseResult::Kind::DateTime)
            {
                return makeFailure(api::Error::IllegalArgument);
            }

            return makeScalarResult(api::CellValue::number(
                rtl::math::approxFloor(oParsed->mfValue)));
        }

        if (oParsed->meKind != api::NumberParseResult::Kind::Time
            && oParsed->meKind != api::NumberParseResult::Kind::DateTime)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        return makeScalarResult(api::CellValue::number(
            spreadsheetengine::core::datetime::normalizeTimeFraction(oParsed->mfValue)));
    }

    if (aFunctionName == u"TIME")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aHour
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aHour)
            return aHour;
        EvaluationResult aMinute
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMinute)
            return aMinute;
        EvaluationResult aSecond
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aSecond)
            return aSecond;

        const auto aHourNumber = coerceToNumber(aHour.maValue.maValue);
        if (!aHourNumber)
            return makeFailure(aHourNumber.meError);
        const auto aMinuteNumber = coerceToNumber(aMinute.maValue.maValue);
        if (!aMinuteNumber)
            return makeFailure(aMinuteNumber.meError);
        const auto aSecondNumber = coerceToNumber(aSecond.maValue.maValue);
        if (!aSecondNumber)
            return makeFailure(aSecondNumber.meError);

        const auto aTimeSerial = api::calendar::makeTimeSerial(
            aHourNumber.maValue, aMinuteNumber.maValue, aSecondNumber.maValue);
        if (!aTimeSerial)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(aTimeSerial.maValue));
    }

    if (aFunctionName == u"DATE")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aYear
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aYear)
            return aYear;
        EvaluationResult aMonth
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMonth)
            return aMonth;
        EvaluationResult aDay
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aDay)
            return aDay;

        if (aYear.maValue.maValue.isEmpty() || aMonth.maValue.maValue.isEmpty()
            || aDay.maValue.maValue.isEmpty())
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        const auto aYearNumber = coerceToNumber(aYear.maValue.maValue);
        if (!aYearNumber)
            return makeFailure(aYearNumber.meError);
        const auto aMonthNumber = coerceToNumber(aMonth.maValue.maValue);
        if (!aMonthNumber)
            return makeFailure(aMonthNumber.meError);
        const auto aDayNumber = coerceToNumber(aDay.maValue.maValue);
        if (!aDayNumber)
            return makeFailure(aDayNumber.meError);

        const sal_Int16 nYear = static_cast<sal_Int16>(std::trunc(aYearNumber.maValue));
        const sal_Int16 nMonth = static_cast<sal_Int16>(std::trunc(aMonthNumber.maValue));
        const sal_Int16 nDay = static_cast<sal_Int16>(std::trunc(aDayNumber.maValue));
        if (nYear < 0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDateSerial
            = api::calendar::makeDateSerial(defaultFodsNullDate(), nYear, nMonth, nDay, false);
        if (!aDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(aDateSerial.maValue));
    }

    if (aFunctionName == u"DATEDIF")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aStart
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aStart)
            return aStart;
        EvaluationResult aEnd
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aEnd)
            return aEnd;
        EvaluationResult aInterval
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aInterval)
            return aInterval;

        const auto oStartDate = coerceToDateSerial(aStart.maValue.maValue);
        const auto oEndDate = coerceToDateSerial(aEnd.maValue.maValue);
        if (!oStartDate || !oEndDate)
            return makeFailure(api::Error::IllegalArgument);

        const auto aIntervalText = coerceToString(aInterval.maValue.maValue);
        if (!aIntervalText)
            return makeFailure(aIntervalText.meError);

        const auto aDateDif
            = api::calendar::dateDif(defaultFodsNullDate(), *oStartDate, *oEndDate,
                aIntervalText.maValue);
        if (!aDateDif)
            return makeFailure(aDateDif.meError);

        return makeScalarResult(api::CellValue::number(aDateDif.maValue));
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

        if (aFunctionName == u"ROUNDUP" || aFunctionName == u"ROUNDDOWN")
        {
            return makeScalarResult(api::CellValue::number(roundMagnitudeDirectional(
                aValueNumber.maValue, nDecimals, eMode)));
        }

        if (rNode.maChildren.size() == 1)
        {
            return makeScalarResult(api::CellValue::number(::rtl::math::round(
                aValueNumber.maValue, 0, api::math::toCoreRoundingMode(eMode))));
        }

        return makeScalarResult(api::CellValue::number(
            api::math::roundToDecimals(aValueNumber.maValue, nDecimals, eMode)));
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
            return makeFailure(api::Error::IllegalArgument);
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
                return makeFailure(aValueNumber.meError);
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
                return makeFailure(aSignificanceNumber.meError);
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
                return makeFailure(aModeNumber.meError);
            bAbs = !::rtl::math::approxEqual(aModeNumber.maValue, 0.0);
        }

        if (!bMicrosoftCompat && bAbs && bMissingSignificance && fValue < 0.0)
            fSignificance = -1.0;

        const bool bCeiling = aFunctionName == u"CEILING" || aFunctionName == u"CEILING.XCL";
        if (bMicrosoftCompat)
        {
            if (bCeiling)
            {
                const auto aCeilingMs = api::math::ceilingMs(fValue, fSignificance);
                if (!aCeilingMs)
                    return makeFailure(aCeilingMs.meError);
                return makeScalarResult(api::CellValue::number(aCeilingMs.maValue));
            }

            const auto aFloorMs = api::math::floorMs(fValue, fSignificance);
            if (!aFloorMs)
                return makeFailure(aFloorMs.meError);
            return makeScalarResult(api::CellValue::number(aFloorMs.maValue));
        }

        if (bCeiling)
        {
            const auto aCeiling = api::math::ceiling(fValue, fSignificance, bAbs, true);
            if (!aCeiling)
                return makeFailure(aCeiling.meError);
            return makeScalarResult(api::CellValue::number(aCeiling.maValue));
        }

        const auto aFloor = api::math::floor(fValue, fSignificance, bAbs, true);
        if (!aFloor)
            return makeFailure(aFloor.meError);
        return makeScalarResult(api::CellValue::number(aFloor.maValue));
    }

    if (aFunctionName == u"CEILING.MATH" || aFunctionName == u"FLOOR.MATH")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        double fSignificance = 1.0;
        if (rNode.maChildren.size() >= 2)
        {
            EvaluationResult aSignificance
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aSignificance)
                return aSignificance;

            const auto aSignificanceNumber = coerceToNumber(aSignificance.maValue.maValue);
            if (!aSignificanceNumber)
                return makeFailure(aSignificanceNumber.meError);
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
                return makeFailure(aModeNumber.meError);
            fMode = aModeNumber.maValue;
        }

        const double fMagnitude = std::abs(fSignificance);
        double fResult = 0.0;
        if (aFunctionName == u"CEILING.MATH")
        {
            if (aValueNumber.maValue < 0.0 && fMode != 0.0)
                fResult = ::rtl::math::approxFloor(aValueNumber.maValue / fMagnitude) * fMagnitude;
            else
                fResult = core::math::computeCeilingPrecise(aValueNumber.maValue, fMagnitude);
        }
        else
        {
            if (aValueNumber.maValue < 0.0 && fMode != 0.0)
                fResult = ::rtl::math::approxCeil(aValueNumber.maValue / fMagnitude) * fMagnitude;
            else
                fResult = core::math::computeFloorPrecise(aValueNumber.maValue, fMagnitude);
        }

        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"CEILING.PRECISE" || aFunctionName == u"FLOOR.PRECISE"
        || aFunctionName == u"ISO.CEILING")
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

        double fSignificance = 1.0;
        if (rNode.maChildren.size() == 2)
        {
            EvaluationResult aSignificance
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aSignificance)
                return aSignificance;

            const auto aSignificanceNumber = coerceToNumber(aSignificance.maValue.maValue);
            if (!aSignificanceNumber)
                return makeFailure(aSignificanceNumber.meError);
            fSignificance = aSignificanceNumber.maValue;
        }

        const double fMagnitude = std::abs(fSignificance);
        if (aFunctionName == u"FLOOR.PRECISE")
        {
            return makeScalarResult(api::CellValue::number(
                api::math::floorPrecise(aValueNumber.maValue, fMagnitude)));
        }

        return makeScalarResult(api::CellValue::number(
            api::math::ceilingPrecise(aValueNumber.maValue, fMagnitude)));
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

        const double fDigits = ::rtl::math::approxFloor(aDigitsNumber.maValue);
        if (fDigits < 1.0)
            return makeFailure(api::Error::IllegalArgument);
        if (aValueNumber.maValue == 0.0)
            return makeScalarResult(api::CellValue::number(0.0));

        return makeScalarResult(api::CellValue::number(api::math::roundToSignificantDigits(
            aValueNumber.maValue, fDigits)));
    }

    if (aFunctionName == u"DAYSINMONTH" || aFunctionName == u"DAYSINYEAR"
        || aFunctionName == u"ISLEAPYEAR" || aFunctionName == u"ISOWEEKNUM")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto oDateSerial = coerceToDateSerial(aArgument.maValue.maValue);
        if (!oDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        constexpr api::DateParts aNullDate = defaultFodsNullDate();
        const sal_Int16 nYear = static_cast<sal_Int16>(
            spreadsheetengine::core::datetime::extractYear(aNullDate, *oDateSerial));
        const sal_Int16 nMonth = static_cast<sal_Int16>(
            spreadsheetengine::core::datetime::extractMonth(aNullDate, *oDateSerial));

        if (aFunctionName == u"DAYSINMONTH")
        {
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(spreadsheetengine::core::detail::date::getDaysInMonth(
                    static_cast<sal_uInt16>(nMonth), nYear))));
        }

        const bool bLeapYear = spreadsheetengine::core::detail::date::isLeapYear(nYear);
        if (aFunctionName == u"DAYSINYEAR")
            return makeScalarResult(api::CellValue::number(bLeapYear ? 366.0 : 365.0));

        if (aFunctionName == u"ISOWEEKNUM")
        {
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(spreadsheetengine::api::calendar::isoWeekOfYear(
                    aNullDate, *oDateSerial))));
        }

        return makeScalarResult(api::CellValue::boolean(bLeapYear));
    }

    if (aFunctionName == u"EDATE" || aFunctionName == u"EOMONTH")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aStart
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aStart)
            return aStart;
        EvaluationResult aMonths
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMonths)
            return aMonths;

        const auto oDateSerial = coerceToDateSerial(aStart.maValue.maValue);
        if (!oDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        const auto aMonthNumber = coerceToNumber(aMonths.maValue.maValue);
        if (!aMonthNumber || !std::isfinite(aMonthNumber.maValue))
            return makeFailure(api::Error::IllegalArgument);

        const sal_Int32 nMonthOffset = static_cast<sal_Int32>(std::trunc(aMonthNumber.maValue));
        const auto oShifted = shiftMonthSerial(
            *oDateSerial, nMonthOffset, aFunctionName == u"EOMONTH");
        if (!oShifted)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(*oShifted));
    }

    if (aFunctionName == u"WEEKS")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aStart
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aStart)
            return aStart;
        EvaluationResult aEnd
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aEnd)
            return aEnd;
        EvaluationResult aMode
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aMode)
            return aMode;

        const auto oStartDate = coerceToDateSerial(aStart.maValue.maValue);
        const auto oEndDate = coerceToDateSerial(aEnd.maValue.maValue);
        if (!oStartDate || !oEndDate || aMode.maValue.maValue.isEmpty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
        if (!aModeNumber)
            return makeFailure(aModeNumber.meError);

        const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
        if (!oWholeMode)
            return makeFailure(api::Error::IllegalArgument);

        const auto oWeeks = computeWeeksDifference(
            *oStartDate, *oEndDate, static_cast<sal_Int16>(*oWholeMode));
        if (!oWeeks)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(*oWeeks));
    }

    if (aFunctionName == u"WORKDAY.INTL")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aStartNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDaysNumber = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);
        if (!aDaysNumber)
            return makeFailure(aDaysNumber.meError);

        const auto oStartDate = toWholeNumber(std::trunc(aStartNumber.maValue));
        const auto oDays = toWholeNumber(std::trunc(aDaysNumber.maValue));
        if (!oStartDate || !oDays)
            return makeFailure(api::Error::IllegalArgument);

        const auto aWeekendMask = evaluateWeekendMaskArgument(
            rNode.maChildren.size() >= 3 ? rNode.maChildren[2].get() : nullptr, true);
        if (!aWeekendMask)
            return makeFailure(aWeekendMask.meError);

        const auto aHolidays = collectHolidaySerials(
            rNode.maChildren.size() >= 4 ? rNode.maChildren[3].get() : nullptr);
        if (!aHolidays)
            return makeFailure(aHolidays.meError);

        return makeScalarResult(api::CellValue::number(advanceWorkdayFods(
            static_cast<api::DateSerial>(*oStartDate), static_cast<api::DateSerial>(*oDays),
            aHolidays.maValue, aWeekendMask.maValue)));
    }

    if (aFunctionName == u"NETWORKDAYS.INTL")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aStartNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aEndNumber = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);
        if (!aEndNumber)
            return makeFailure(aEndNumber.meError);

        const auto oStartDate = toWholeNumber(std::trunc(aStartNumber.maValue));
        const auto oEndDate = toWholeNumber(std::trunc(aEndNumber.maValue));
        if (!oStartDate || !oEndDate)
            return makeFailure(api::Error::IllegalArgument);

        const auto aWeekendMask = evaluateWeekendMaskArgument(
            rNode.maChildren.size() >= 3 ? rNode.maChildren[2].get() : nullptr, false);
        if (!aWeekendMask)
            return makeFailure(aWeekendMask.meError);

        const auto aHolidays = collectHolidaySerials(
            rNode.maChildren.size() >= 4 ? rNode.maChildren[3].get() : nullptr);
        if (!aHolidays)
            return makeFailure(aHolidays.meError);

        return makeScalarResult(api::CellValue::number(countWorkdaysFods(
            static_cast<api::DateSerial>(*oStartDate), static_cast<api::DateSerial>(*oEndDate),
            aHolidays.maValue, aWeekendMask.maValue)));
    }

    if (aFunctionName == u"VLOOKUP" || aFunctionName == u"HLOOKUP")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aLookupValue)
            return aLookupValue;

        EvaluationResult aTable = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        if (!aTable)
            return aTable;
        if (!aTable.maValue.isMatrixReference())
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aIndex
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aIndex)
            return aIndex;
        if (aIndex.maValue.maValue.isEmpty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aIndexNumber = coerceToNumber(aIndex.maValue.maValue);
        if (!aIndexNumber)
            return makeFailure(aIndexNumber.meError);
        const auto oWholeIndex = toWholeNumber(aIndexNumber.maValue);
        if (!oWholeIndex || *oWholeIndex <= 0)
            return makeFailure(api::Error::IllegalArgument);

        bool bApproximate = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aMode)
                return aMode;

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            bApproximate = !rtl::math::approxEqual(aModeNumber.maValue, 0.0);
        }

        const auto& rReference = aTable.maValue.maReference;
        const auto aDimensions = rReference.matrixDimensions();
        const auto eOrientation = aFunctionName == u"VLOOKUP"
                                      ? api::lookup::VectorOrientation::Column
                                      : api::lookup::VectorOrientation::Row;

        const api::MatrixSize nSearchLength
            = eOrientation == api::lookup::VectorOrientation::Column ? aDimensions.mnRows
                                                                     : aDimensions.mnColumns;
        const api::MatrixSize nResultIndex = *oWholeIndex - 1;
        if (nSearchLength <= 0)
            return makeFailure(api::Error::IllegalArgument);
        if (eOrientation == api::lookup::VectorOrientation::Column
            && nResultIndex >= aDimensions.mnColumns)
        {
            return makeFailure(api::Error::IllegalArgument);
        }
        if (eOrientation == api::lookup::VectorOrientation::Row && nResultIndex >= aDimensions.mnRows)
            return makeFailure(api::Error::IllegalArgument);

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);

        auto loadCandidateAt = [&](api::MatrixSize nSearchIndex) -> EvaluationResult {
            const api::MatrixCoordinate aSearchCoordinate
                = eOrientation == api::lookup::VectorOrientation::Column
                      ? api::MatrixCoordinate { 0, nSearchIndex }
                      : api::MatrixCoordinate { nSearchIndex, 0 };
            return materializeReferenceValue(
                rReference, aSearchCoordinate.mnColumn, aSearchCoordinate.mnRow);
        };

        auto compareForExactLookup = [&](const api::CellValue& rCandidate)
            -> api::ValueResult<int> {
            if (rCandidate.isError())
                return api::ValueResult<int>::failure(rCandidate.meError);

            if (rLookup.isText())
            {
                if (!rCandidate.isText())
                    return api::ValueResult<int>::failure(api::Error::IllegalArgument);

                return api::ValueResult<int>::success(sequery::matchesWholeCellLookupText(
                    rLookup.maString, rCandidate.maString,
                    toQuerySearchType(mrWorkbook.meFormulaSearchType))
                        ? 0
                        : 1);
            }

            if (rCandidate.isText())
                return api::ValueResult<int>::failure(api::Error::IllegalArgument);

            const auto aCandidateNumber = coerceToNumber(rCandidate);
            if (!aCandidateNumber)
                return api::ValueResult<int>::failure(aCandidateNumber.meError);
            const auto aLookupNumber = coerceToNumber(rLookup);
            if (!aLookupNumber)
                return api::ValueResult<int>::failure(aLookupNumber.meError);

            if (rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue))
                return api::ValueResult<int>::success(0);
            return api::ValueResult<int>::success(1);
        };

        std::optional<api::MatrixSize> oResolvedIndex;
        if (bApproximate)
        {
            if (rLookup.isText())
            {
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                    {
                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        const auto aTextCompare = compareLookupText(aCandidateText, rLookup.maString);
                        if (!aTextCompare)
                            continue;
                        if (aTextCompare.maValue <= 0)
                        {
                            oResolvedIndex = nSearchIndex;
                        }
                        else if (nSearchIndex > 0)
                        {
                            break;
                        }
                    }
                    else
                    {
                        oResolvedIndex = nSearchIndex;
                    }
                }
            }
            else
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return makeFailure(aLookupNumber.meError);

                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber)
                        continue;

                    if (aCandidateNumber.maValue < aLookupNumber.maValue
                        || rtl::math::approxEqual(
                            aCandidateNumber.maValue, aLookupNumber.maValue))
                    {
                        oResolvedIndex = nSearchIndex;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                if (!aCandidate)
                    continue;

                const auto aComparison = compareForExactLookup(aCandidate.maValue.maValue);
                if (!aComparison)
                    continue;
                if (aComparison.maValue == 0)
                {
                    oResolvedIndex = nSearchIndex;
                    break;
                }
            }
        }

        if (!oResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        EvaluationResult aMatchedSearchValue = loadCandidateAt(*oResolvedIndex);
        if (!aMatchedSearchValue)
            return aMatchedSearchValue;
        if (rLookup.isText() && (aMatchedSearchValue.maValue.maValue.isNumber()
                                 || aMatchedSearchValue.maValue.maValue.isBoolean()))
        {
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        const auto aResultCoordinate = api::lookup::planTabularLookupResult(
            eOrientation, *oResolvedIndex, nResultIndex, aDimensions);
        if (!aResultCoordinate)
            return makeFailure(aResultCoordinate.meError);

        return materializeReferenceValue(
            rReference, aResultCoordinate.maValue.mnColumn, aResultCoordinate.maValue.mnRow);
    }

    if (aFunctionName == u"LOOKUP")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);
        if (rLookup.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const auto aDataInput = evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aDataInput)
            return makeFailure(aDataInput.meError);
        const LookupInput& rDataInput = aDataInput.maValue;
        if (rDataInput.mbScalar && rDataInput.maScalar.isError())
            return makeScalarResult(api::CellValue::error(rDataInput.maScalar.meError));

        const auto aDataLayout = detectLookupLayout(rDataInput, true);
        if (!aDataLayout)
            return makeFailure(aDataLayout.meError);

        std::optional<LookupInput> oResultInput;
        std::optional<api::lookup::VectorLayout> oResultLayout;
        if (rNode.maChildren.size() == 3)
        {
            const auto aResultInput = evaluateLookupInputNode(*rNode.maChildren[2]);
            if (!aResultInput)
                return makeFailure(aResultInput.meError);
            oResultInput = aResultInput.maValue;
            if (oResultInput->mbScalar && oResultInput->maScalar.isError())
                return makeScalarResult(api::CellValue::error(oResultInput->maScalar.meError));

            const auto aResultLayout = detectLookupLayout(*oResultInput, false);
            if (!aResultLayout)
                return makeFailure(aResultLayout.meError);
            oResultLayout = aResultLayout.maValue;
        }

        auto materializeResultAt = [&](api::MatrixSize nIndex) -> EvaluationResult {
            if (oResultInput)
            {
                if (oResultInput->mbScalar)
                {
                    if (nIndex != 0)
                        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
                    return makeScalarResult(oResultInput->maScalar);
                }

                return materializeLookupInputValue(
                    *this, *oResultInput, oResultLayout->meOrientation, nIndex);
            }

            if (rDataInput.mbScalar)
                return makeScalarResult(rDataInput.maScalar);

            const api::MatrixDimensions aDimensions { rDataInput.mnColumns, rDataInput.mnRows };
            const api::MatrixSize nResultIndex
                = aDataLayout.maValue.meOrientation == api::lookup::VectorOrientation::Column
                      ? aDimensions.mnColumns - 1
                      : aDimensions.mnRows - 1;
            const auto aResultCoordinate = api::lookup::planTabularLookupResult(
                aDataLayout.maValue.meOrientation, nIndex, nResultIndex, aDimensions);
            if (!aResultCoordinate)
                return makeFailure(aResultCoordinate.meError);

            if (!rDataInput.maValues.empty())
            {
                const sal_Int64 nLinearIndex
                    = static_cast<sal_Int64>(aResultCoordinate.maValue.mnRow) * rDataInput.mnColumns
                      + aResultCoordinate.maValue.mnColumn;
                if (nLinearIndex < 0
                    || static_cast<std::size_t>(nLinearIndex) >= rDataInput.maValues.size())
                {
                    return makeFailure(api::Error::IllegalArgument);
                }

                return makeScalarResult(
                    rDataInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
            }

            return materializeReferenceValue(rDataInput.maReference,
                aResultCoordinate.maValue.mnColumn, aResultCoordinate.maValue.mnRow);
        };

        auto compareExactTypeMatch = [&](const api::CellValue& rCandidate) -> bool {
            if (rLookup.isText())
            {
                if (!rCandidate.isText())
                    return false;
                return sequery::matchesWholeCellLookupText(
                    rLookup.maString, rCandidate.maString,
                    toQuerySearchType(mrWorkbook.meFormulaSearchType));
            }

            if (rCandidate.isText() || rCandidate.isEmpty())
                return false;

            const auto aCandidateNumber = coerceToNumber(rCandidate);
            if (!aCandidateNumber)
                return false;
            const auto aLookupNumber = coerceToNumber(rLookup);
            if (!aLookupNumber)
                return false;

            return rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue);
        };

        std::optional<api::MatrixSize> oResolvedIndex;
        if (rDataInput.mbScalar)
        {
            if (!compareExactTypeMatch(rDataInput.maScalar))
            {
                if (rLookup.isText())
                    return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

                const auto aLookupNumber = coerceToNumber(rLookup);
                const auto aDataNumber = coerceToNumber(rDataInput.maScalar);
                if (!aLookupNumber || !aDataNumber || aDataNumber.maValue > aLookupNumber.maValue)
                    return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
            }

            return materializeResultAt(0);
        }

        auto loadCandidateAt = [&](api::MatrixSize nSearchIndex) -> EvaluationResult {
            return materializeLookupInputValue(
                *this, rDataInput, aDataLayout.maValue.meOrientation, nSearchIndex);
        };

        if (rLookup.isText())
        {
            bool bSeenExactTextMatch = false;
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < aDataLayout.maValue.mnLength;
                 ++nSearchIndex)
            {
                EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                {
                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    const sal_Int32 nCompare
                        = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                    if (nCompare == 0)
                    {
                        oResolvedIndex = nSearchIndex;
                        bSeenExactTextMatch = true;
                        continue;
                    }
                    if (bSeenExactTextMatch)
                    {
                        break;
                    }
                    if (nCompare < 0)
                    {
                        oResolvedIndex = nSearchIndex;
                    }
                    else if (nSearchIndex > 0)
                    {
                        break;
                    }
                }
                else
                {
                    oResolvedIndex = nSearchIndex;
                }
            }
        }
        else
        {
            const auto aLookupNumber = coerceToNumber(rLookup);
            if (!aLookupNumber)
                return makeFailure(aLookupNumber.meError);

            bool bSeenExactNumericMatch = false;
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < aDataLayout.maValue.mnLength;
                 ++nSearchIndex)
            {
                EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                    continue;

                const auto aCandidateNumber = coerceToNumber(rCandidate);
                if (!aCandidateNumber)
                    continue;

                if (rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue))
                {
                    oResolvedIndex = nSearchIndex;
                    bSeenExactNumericMatch = true;
                }
                else if (bSeenExactNumericMatch)
                {
                    break;
                }
                else if (aCandidateNumber.maValue < aLookupNumber.maValue)
                {
                    oResolvedIndex = nSearchIndex;
                }
                else
                {
                    break;
                }
            }
        }

        if (!oResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        EvaluationResult aMatchedSearchValue = loadCandidateAt(*oResolvedIndex);
        if (!aMatchedSearchValue)
            return aMatchedSearchValue;
        if (rLookup.isText() && (aMatchedSearchValue.maValue.maValue.isNumber()
                                 || aMatchedSearchValue.maValue.maValue.isBoolean()))
        {
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        return materializeResultAt(*oResolvedIndex);
    }

    if (aFunctionName == u"MATCH")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);

        EvaluationResult aSearchValue = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        const auto oSearchInput = makeLookupInput(aSearchValue);
        if (!oSearchInput)
            return makeFailure(api::Error::IllegalArgument);

        const auto aSearchLayout = detectLookupLayout(*oSearchInput, true);
        if (!aSearchLayout)
            return makeFailure(aSearchLayout.meError);

        api::lookup::MatchSearchMode aModes;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);

            const auto aNormalized = api::lookup::normalizeMatchType(aModeNumber.maValue);
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            aModes = aNormalized.maValue;
        }
        else
        {
            aModes.meMatchMode = api::lookup::MatchMode::ExactOrNextSmaller;
            aModes.meSearchMode = api::lookup::SearchMode::BinaryAscending;
        }

        auto loadCandidateAt = [&](api::MatrixSize nSearchIndex) -> EvaluationResult {
            return materializeLookupInputValue(
                *this, *oSearchInput, aSearchLayout.maValue.meOrientation, nSearchIndex);
        };

        api::MatrixSize nSearchLength = aSearchLayout.maValue.mnLength;
        while (nSearchLength > 0)
        {
            EvaluationResult aTailCandidate = loadCandidateAt(nSearchLength - 1);
            if (!aTailCandidate || !aTailCandidate.maValue.maValue.isEmpty())
                break;
            --nSearchLength;
        }

        auto isExactMatch = [&](const api::CellValue& rCandidate) -> bool {
            if (rLookup.isText())
            {
                if (!rCandidate.isText())
                    return false;

                return sequery::matchesWholeCellLookupText(
                    rLookup.maString, rCandidate.maString,
                    toQuerySearchType(mrWorkbook.meFormulaSearchType));
            }

            if (rCandidate.isText() || rCandidate.isEmpty())
                return false;

            const auto aLookupNumber = coerceToNumber(rLookup);
            const auto aCandidateNumber = coerceToNumber(rCandidate);
            if (!aLookupNumber || !aCandidateNumber)
                return false;

            return rtl::math::approxEqual(aLookupNumber.maValue, aCandidateNumber.maValue);
        };

        std::optional<api::MatrixSize> oResolvedIndex;
        if (aModes.meMatchMode == api::lookup::MatchMode::ExactOrNotAvailable)
        {
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength;
                 ++nSearchIndex)
            {
                EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                if (!aCandidate)
                    continue;
                if (isExactMatch(aCandidate.maValue.maValue))
                {
                    oResolvedIndex = nSearchIndex;
                    break;
                }
            }
        }
        else if (aModes.meMatchMode == api::lookup::MatchMode::ExactOrNextSmaller)
        {
            if (rLookup.isText())
            {
                bool bSeenExactTextMatch = false;
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength;
                     ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                    {
                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        const sal_Int32 nCompare
                            = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                        if (nCompare == 0)
                        {
                            oResolvedIndex = nSearchIndex;
                            bSeenExactTextMatch = true;
                            continue;
                        }
                        if (bSeenExactTextMatch)
                            break;
                        if (nCompare < 0)
                            oResolvedIndex = nSearchIndex;
                        else if (nSearchIndex > 0)
                            break;
                    }
                }
            }
            else
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return makeFailure(aLookupNumber.meError);

                bool bSeenExactNumericMatch = false;
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength;
                     ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber)
                        continue;

                    if (rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue))
                    {
                        oResolvedIndex = nSearchIndex;
                        bSeenExactNumericMatch = true;
                    }
                    else if (bSeenExactNumericMatch)
                    {
                        break;
                    }
                    else if (aCandidateNumber.maValue < aLookupNumber.maValue)
                    {
                        oResolvedIndex = nSearchIndex;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        else if (aModes.meMatchMode == api::lookup::MatchMode::ExactOrNextLarger)
        {
            if (rLookup.isText())
            {
                bool bSeenExactTextMatch = false;
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength;
                     ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                    {
                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        const sal_Int32 nCompare
                            = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                        if (nCompare == 0)
                        {
                            oResolvedIndex = nSearchIndex;
                            bSeenExactTextMatch = true;
                            continue;
                        }
                        if (bSeenExactTextMatch)
                            break;
                        if (nCompare > 0)
                            oResolvedIndex = nSearchIndex;
                        else
                            break;
                    }
                }
            }
            else
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return makeFailure(aLookupNumber.meError);

                bool bSeenExactNumericMatch = false;
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength;
                     ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadCandidateAt(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber)
                        continue;

                    if (rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue))
                    {
                        oResolvedIndex = nSearchIndex;
                        bSeenExactNumericMatch = true;
                    }
                    else if (bSeenExactNumericMatch)
                    {
                        break;
                    }
                    else if (aCandidateNumber.maValue > aLookupNumber.maValue)
                    {
                        oResolvedIndex = nSearchIndex;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        if (!oResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        return makeScalarResult(api::CellValue::number(static_cast<double>(*oResolvedIndex + 1)));
    }

    if (aFunctionName == u"XMATCH")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);

        const auto aSearchInput = evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aSearchInput)
            return makeFailure(aSearchInput.meError);

        const auto aSearchLayout = detectLookupLayout(aSearchInput.maValue, false);
        if (!aSearchLayout)
            return makeFailure(aSearchLayout.meError);

        api::lookup::MatchMode eMatchMode = api::lookup::MatchMode::ExactOrNotAvailable;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeExtendedMatchMode(static_cast<sal_Int16>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eMatchMode = aNormalized.maValue;
        }

        api::lookup::SearchMode eSearchMode = api::lookup::SearchMode::Forward;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeSearchMode(static_cast<sal_Int16>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eSearchMode = aNormalized.maValue;
        }

        if ((eMatchMode == api::lookup::MatchMode::Wildcard
             || eMatchMode == api::lookup::MatchMode::Regex)
            && api::lookup::isBinarySearchMode(eSearchMode))
        {
            return makeFailure(api::Error::NoValue);
        }

        auto loadSearchCandidate = [&](api::MatrixSize nSearchIndex) -> EvaluationResult {
            return materializeLookupInputValue(
                *this, aSearchInput.maValue, aSearchLayout.maValue.meOrientation, nSearchIndex);
        };

        api::MatrixSize nSearchLength = aSearchLayout.maValue.mnLength;
        while (nSearchLength > 0)
        {
            EvaluationResult aTailCandidate = loadSearchCandidate(nSearchLength - 1);
            if (!aTailCandidate || !aTailCandidate.maValue.maValue.isEmpty())
                break;
            --nSearchLength;
        }

        const api::query::SearchType ePatternSearchType
            = eMatchMode == api::lookup::MatchMode::Wildcard
                  ? api::query::SearchType::Wildcard
                  : (eMatchMode == api::lookup::MatchMode::Regex
                         ? api::query::SearchType::Regex
                         : api::query::SearchType::Normal);

        auto isExactMatch = [&](const api::CellValue& rCandidate) -> bool {
            if (rLookup.isEmpty())
                return rCandidate.isEmpty();

            if (rLookup.isText())
            {
                if (!(rCandidate.isText() || rCandidate.isEmpty()))
                    return false;

                const api::StringView aCandidateText
                    = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                          : api::StringView();
                if (ePatternSearchType == api::query::SearchType::Normal)
                    return sequery::compareFoldedText(aCandidateText, rLookup.maString) == 0;
                return sequery::matchesWholeCellLookupText(rLookup.maString, aCandidateText,
                    ePatternSearchType);
            }

            if (rCandidate.isText() || rCandidate.isEmpty())
                return false;

            const auto aLookupNumber = coerceToNumber(rLookup);
            const auto aCandidateNumber = coerceToNumber(rCandidate);
            if (!aLookupNumber || !aCandidateNumber)
                return false;

            return rtl::math::approxEqual(aLookupNumber.maValue, aCandidateNumber.maValue);
        };

        auto findExactIndex = [&]() -> std::optional<api::MatrixSize> {
            const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse
                                  || eSearchMode == api::lookup::SearchMode::BinaryDescending;
            if (bReverse)
            {
                for (api::MatrixSize nSearchIndex = nSearchLength; nSearchIndex > 0; --nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex - 1);
                    if (!aCandidate)
                        continue;
                    if (isExactMatch(aCandidate.maValue.maValue))
                        return nSearchIndex - 1;
                }
                return std::nullopt;
            }

            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                if (!aCandidate)
                    continue;
                if (isExactMatch(aCandidate.maValue.maValue))
                    return nSearchIndex;
            }

            return std::nullopt;
        };

        std::optional<api::MatrixSize> oResolvedIndex;
        if (eMatchMode == api::lookup::MatchMode::ExactOrNotAvailable
            || eMatchMode == api::lookup::MatchMode::Wildcard
            || eMatchMode == api::lookup::MatchMode::Regex)
        {
            oResolvedIndex = findExactIndex();
        }
        else if (eMatchMode == api::lookup::MatchMode::ExactOrNextSmaller)
        {
            oResolvedIndex = findExactIndex();
            if (!oResolvedIndex && (eSearchMode == api::lookup::SearchMode::Forward
                                    || eSearchMode == api::lookup::SearchMode::Reverse))
            {
                const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse;
                if (rLookup.isText())
                {
                    api::String aBestText;
                    bool bHaveBestText = false;
                    for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                    {
                        const api::MatrixSize nSearchIndex
                            = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                        EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                        if (!aCandidate)
                            continue;

                        const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                        if (!(rCandidate.isText() || rCandidate.isEmpty()))
                            continue;

                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        if (sequery::compareFoldedText(aCandidateText, rLookup.maString) >= 0)
                            continue;

                        if (!bHaveBestText
                            || sequery::compareFoldedText(aCandidateText, aBestText) > 0)
                        {
                            aBestText = api::String(aCandidateText);
                            bHaveBestText = true;
                            oResolvedIndex = nSearchIndex;
                        }
                    }
                }
                else
                {
                    const auto aLookupNumber = coerceToNumber(rLookup);
                    if (!aLookupNumber)
                        return makeFailure(aLookupNumber.meError);

                    std::optional<double> ofBestNumber;
                    for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                    {
                        const api::MatrixSize nSearchIndex
                            = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                        EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                        if (!aCandidate)
                            continue;

                        const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                        if (rCandidate.isText() || rCandidate.isEmpty())
                            continue;

                        const auto aCandidateNumber = coerceToNumber(rCandidate);
                        if (!aCandidateNumber
                            || !(aCandidateNumber.maValue < aLookupNumber.maValue))
                        {
                            continue;
                        }

                        if (!ofBestNumber || aCandidateNumber.maValue > *ofBestNumber)
                        {
                            ofBestNumber = aCandidateNumber.maValue;
                            oResolvedIndex = nSearchIndex;
                        }
                    }
                }
            }
            else if (!oResolvedIndex && rLookup.isText())
            {
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                    {
                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        const sal_Int32 nCompare
                            = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                        if (nCompare < 0)
                            oResolvedIndex = nSearchIndex;
                        else if (nSearchIndex > 0)
                            break;
                    }
                }
            }
            else if (!oResolvedIndex)
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return makeFailure(aLookupNumber.meError);

                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber)
                        continue;

                    if (aCandidateNumber.maValue < aLookupNumber.maValue)
                    {
                        oResolvedIndex = nSearchIndex;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        else if (eMatchMode == api::lookup::MatchMode::ExactOrNextLarger)
        {
            oResolvedIndex = findExactIndex();
            if (!oResolvedIndex && (eSearchMode == api::lookup::SearchMode::Forward
                                    || eSearchMode == api::lookup::SearchMode::Reverse))
            {
                const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse;
                if (rLookup.isText())
                {
                    api::String aBestText;
                    bool bHaveBestText = false;
                    for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                    {
                        const api::MatrixSize nSearchIndex
                            = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                        EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                        if (!aCandidate)
                            continue;

                        const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                        if (!(rCandidate.isText() || rCandidate.isEmpty()))
                            continue;

                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        if (sequery::compareFoldedText(aCandidateText, rLookup.maString) <= 0)
                            continue;

                        if (!bHaveBestText
                            || sequery::compareFoldedText(aCandidateText, aBestText) < 0)
                        {
                            aBestText = api::String(aCandidateText);
                            bHaveBestText = true;
                            oResolvedIndex = nSearchIndex;
                        }
                    }
                }
                else
                {
                    const auto aLookupNumber = coerceToNumber(rLookup);
                    if (!aLookupNumber)
                        return makeFailure(aLookupNumber.meError);

                    std::optional<double> ofBestNumber;
                    for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                    {
                        const api::MatrixSize nSearchIndex
                            = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                        EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                        if (!aCandidate)
                            continue;

                        const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                        if (rCandidate.isText() || rCandidate.isEmpty())
                            continue;

                        const auto aCandidateNumber = coerceToNumber(rCandidate);
                        if (!aCandidateNumber
                            || !(aCandidateNumber.maValue > aLookupNumber.maValue))
                        {
                            continue;
                        }

                        if (!ofBestNumber || aCandidateNumber.maValue < *ofBestNumber)
                        {
                            ofBestNumber = aCandidateNumber.maValue;
                            oResolvedIndex = nSearchIndex;
                        }
                    }
                }
            }
            else if (!oResolvedIndex && rLookup.isText())
            {
                const bool bDescending = eSearchMode == api::lookup::SearchMode::BinaryDescending;
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (!(rCandidate.isText() || rCandidate.isEmpty()))
                        continue;

                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    const sal_Int32 nCompare
                        = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                    if ((!bDescending && nCompare > 0) || (bDescending && nCompare < 0))
                    {
                        oResolvedIndex = nSearchIndex;
                        if (!bDescending)
                            break;
                    }
                    else if (bDescending)
                    {
                        break;
                    }
                }
            }
            else if (!oResolvedIndex)
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return makeFailure(aLookupNumber.meError);

                const bool bDescending = eSearchMode == api::lookup::SearchMode::BinaryDescending;
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber)
                        continue;

                    if ((!bDescending && aCandidateNumber.maValue > aLookupNumber.maValue)
                        || (bDescending && aCandidateNumber.maValue < aLookupNumber.maValue))
                    {
                        oResolvedIndex = nSearchIndex;
                        if (!bDescending)
                            break;
                    }
                    else if (bDescending)
                    {
                        break;
                    }
                }
            }
        }

        if (!oResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        return makeScalarResult(api::CellValue::number(static_cast<double>(*oResolvedIndex + 1)));
    }

    if (aFunctionName == u"XLOOKUP")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);
        if (rLookup.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        EvaluationResult aSearchValue = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        EvaluationResult aReturnValue = evaluateNode(*rNode.maChildren[2], rCurrentAddress);
        const auto oSearchInput = makeLookupInput(aSearchValue);
        const auto oReturnInput = makeLookupInput(aReturnValue);
        if (!oSearchInput || !oReturnInput)
            return makeFailure(api::Error::IllegalArgument);

        const api::MatrixDimensions aSearchDimensions { oSearchInput->mnColumns, oSearchInput->mnRows };
        const api::MatrixDimensions aReturnDimensions { oReturnInput->mnColumns, oReturnInput->mnRows };
        const auto aSearchLayout = detectLookupLayout(*oSearchInput, false);
        if (!aSearchLayout)
            return makeFailure(aSearchLayout.meError);
        if (aReturnDimensions.isEmpty())
            return makeFailure(api::Error::IllegalArgument);
        if (aSearchLayout.maValue.meOrientation == api::lookup::VectorOrientation::Column)
        {
            if (aReturnDimensions.mnRows != aSearchDimensions.mnRows)
                return makeFailure(api::Error::IllegalArgument);
        }
        else if (aReturnDimensions.mnColumns != aSearchDimensions.mnColumns)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        auto loadSearchCandidate = [&](api::MatrixSize nSearchIndex) -> EvaluationResult {
            return materializeLookupInputValue(
                *this, *oSearchInput, aSearchLayout.maValue.meOrientation, nSearchIndex);
        };

        api::MatrixSize nSearchLength = aSearchLayout.maValue.mnLength;
        while (nSearchLength > 0)
        {
            EvaluationResult aTailCandidate = loadSearchCandidate(nSearchLength - 1);
            if (!aTailCandidate || !aTailCandidate.maValue.maValue.isEmpty())
                break;
            --nSearchLength;
        }

        api::lookup::MatchMode eMatchMode = api::lookup::MatchMode::ExactOrNotAvailable;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[4], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized = api::lookup::normalizeExtendedMatchMode(
                static_cast<sal_Int16>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eMatchMode = aNormalized.maValue;
        }

        api::lookup::SearchMode eSearchMode = api::lookup::SearchMode::Forward;
        if (rNode.maChildren.size() >= 6
            && rNode.maChildren[5]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[5], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeSearchMode(static_cast<sal_Int16>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eSearchMode = aNormalized.maValue;
        }

        if (eMatchMode == api::lookup::MatchMode::Wildcard
            || eMatchMode == api::lookup::MatchMode::Regex)
        {
            return makeFailure(api::Error::NoValue);
        }

        auto isExactMatch = [&](const api::CellValue& rCandidate) -> bool {
            if (rLookup.isText())
            {
                if (!rCandidate.isText())
                    return false;

                return sequery::matchesWholeCellLookupText(
                    rLookup.maString, rCandidate.maString,
                    toQuerySearchType(mrWorkbook.meFormulaSearchType));
            }

            if (rCandidate.isText() || rCandidate.isEmpty())
                return false;

            const auto aLookupNumber = coerceToNumber(rLookup);
            const auto aCandidateNumber = coerceToNumber(rCandidate);
            if (!aLookupNumber || !aCandidateNumber)
                return false;

            return rtl::math::approxEqual(aLookupNumber.maValue, aCandidateNumber.maValue);
        };

        auto findExactIndex = [&]() -> std::optional<api::MatrixSize> {
            const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse
                                  || eSearchMode == api::lookup::SearchMode::BinaryDescending;
            if (bReverse)
            {
                for (api::MatrixSize nSearchIndex = nSearchLength; nSearchIndex > 0; --nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex - 1);
                    if (!aCandidate)
                        continue;
                    if (isExactMatch(aCandidate.maValue.maValue))
                        return nSearchIndex - 1;
                }
                return std::nullopt;
            }

            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                if (!aCandidate)
                    continue;
                if (isExactMatch(aCandidate.maValue.maValue))
                    return nSearchIndex;
            }

            return std::nullopt;
        };

        std::optional<api::MatrixSize> oResolvedIndex;
        if (eMatchMode == api::lookup::MatchMode::ExactOrNotAvailable)
        {
            oResolvedIndex = findExactIndex();
        }
        else if (eMatchMode == api::lookup::MatchMode::ExactOrNextSmaller)
        {
            oResolvedIndex = findExactIndex();
            if (oResolvedIndex)
            {
                // exact hit wins even if the lookup vector is not sorted
            }
            else if (eSearchMode == api::lookup::SearchMode::Forward
                     || eSearchMode == api::lookup::SearchMode::Reverse)
            {
                const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse;
                if (rLookup.isText())
                {
                    api::String aBestText;
                    bool bHaveBestText = false;
                    for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                    {
                        const api::MatrixSize nSearchIndex
                            = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                        EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                        if (!aCandidate)
                            continue;

                        const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                        if (!(rCandidate.isText() || rCandidate.isEmpty()))
                            continue;

                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        if (sequery::compareFoldedText(aCandidateText, rLookup.maString) >= 0)
                            continue;

                        if (!bHaveBestText
                            || sequery::compareFoldedText(aCandidateText, aBestText) > 0)
                        {
                            aBestText = api::String(aCandidateText);
                            bHaveBestText = true;
                            oResolvedIndex = nSearchIndex;
                        }
                    }
                }
                else
                {
                    const auto aLookupNumber = coerceToNumber(rLookup);
                    if (!aLookupNumber)
                        return makeFailure(aLookupNumber.meError);

                    std::optional<double> ofBestNumber;
                    for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                    {
                        const api::MatrixSize nSearchIndex
                            = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                        EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                        if (!aCandidate)
                            continue;

                        const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                        if (rCandidate.isText() || rCandidate.isEmpty())
                            continue;

                        const auto aCandidateNumber = coerceToNumber(rCandidate);
                        if (!aCandidateNumber
                            || !(aCandidateNumber.maValue < aLookupNumber.maValue))
                        {
                            continue;
                        }

                        if (!ofBestNumber || aCandidateNumber.maValue > *ofBestNumber)
                        {
                            ofBestNumber = aCandidateNumber.maValue;
                            oResolvedIndex = nSearchIndex;
                        }
                    }
                }
            }
            else if (rLookup.isText())
            {
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                    {
                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        const sal_Int32 nCompare
                            = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                        if (nCompare < 0)
                            oResolvedIndex = nSearchIndex;
                        else if (nSearchIndex > 0)
                            break;
                    }
                }
            }
            else
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return makeFailure(aLookupNumber.meError);

                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber)
                        continue;

                    if (aCandidateNumber.maValue < aLookupNumber.maValue)
                    {
                        oResolvedIndex = nSearchIndex;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        else if (eMatchMode == api::lookup::MatchMode::ExactOrNextLarger)
        {
            oResolvedIndex = findExactIndex();
            if (oResolvedIndex)
            {
                // exact hit wins even if the lookup vector is not sorted
            }
            else if (eSearchMode == api::lookup::SearchMode::Forward
                     || eSearchMode == api::lookup::SearchMode::Reverse)
            {
                const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse;
                if (rLookup.isText())
                {
                    api::String aBestText;
                    bool bHaveBestText = false;
                    for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                    {
                        const api::MatrixSize nSearchIndex
                            = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                        EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                        if (!aCandidate)
                            continue;

                        const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                        if (!(rCandidate.isText() || rCandidate.isEmpty()))
                            continue;

                        const api::StringView aCandidateText
                            = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                                  : api::StringView();
                        if (sequery::compareFoldedText(aCandidateText, rLookup.maString) <= 0)
                            continue;

                        if (!bHaveBestText
                            || sequery::compareFoldedText(aCandidateText, aBestText) < 0)
                        {
                            aBestText = api::String(aCandidateText);
                            bHaveBestText = true;
                            oResolvedIndex = nSearchIndex;
                        }
                    }
                }
                else
                {
                    const auto aLookupNumber = coerceToNumber(rLookup);
                    if (!aLookupNumber)
                        return makeFailure(aLookupNumber.meError);

                    std::optional<double> ofBestNumber;
                    for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                    {
                        const api::MatrixSize nSearchIndex
                            = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                        EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                        if (!aCandidate)
                            continue;

                        const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                        if (rCandidate.isText() || rCandidate.isEmpty())
                            continue;

                        const auto aCandidateNumber = coerceToNumber(rCandidate);
                        if (!aCandidateNumber
                            || !(aCandidateNumber.maValue > aLookupNumber.maValue))
                        {
                            continue;
                        }

                        if (!ofBestNumber || aCandidateNumber.maValue < *ofBestNumber)
                        {
                            ofBestNumber = aCandidateNumber.maValue;
                            oResolvedIndex = nSearchIndex;
                        }
                    }
                }
            }
            else if (rLookup.isText())
            {
                const bool bDescending = eSearchMode == api::lookup::SearchMode::BinaryDescending;
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (!(rCandidate.isText() || rCandidate.isEmpty()))
                        continue;

                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    const sal_Int32 nCompare
                        = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                    if ((!bDescending && nCompare > 0) || (bDescending && nCompare < 0))
                    {
                        oResolvedIndex = nSearchIndex;
                        if (!bDescending)
                            break;
                    }
                    else if (bDescending)
                    {
                        break;
                    }
                }
            }
            else
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return makeFailure(aLookupNumber.meError);

                const bool bDescending = eSearchMode == api::lookup::SearchMode::BinaryDescending;
                for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
                {
                    EvaluationResult aCandidate = loadSearchCandidate(nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber)
                        continue;

                    if ((!bDescending && aCandidateNumber.maValue > aLookupNumber.maValue)
                        || (bDescending && aCandidateNumber.maValue < aLookupNumber.maValue))
                    {
                        oResolvedIndex = nSearchIndex;
                        if (!bDescending)
                            break;
                    }
                    else if (bDescending)
                    {
                        break;
                    }
                }
            }
        }

        if (!oResolvedIndex)
        {
            if (rNode.maChildren.size() >= 4
                && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
            {
                return evaluateNode(*rNode.maChildren[3], rCurrentAddress);
            }
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        const auto aResultSlice = api::lookup::planXLookupResultSlice(
            aSearchLayout.maValue.meOrientation, *oResolvedIndex, aReturnDimensions);
        if (!aResultSlice)
            return makeFailure(aResultSlice.meError);

        if (oReturnInput->mbScalar)
            return makeScalarResult(oReturnInput->maScalar);

        api::ResolvedReference aSliceReference;
        aSliceReference.maRange.maStart = oReturnInput->maReference.addressAt(
            aResultSlice.maValue.maStart.mnColumn, aResultSlice.maValue.maStart.mnRow);
        aSliceReference.maRange.maEnd = oReturnInput->maReference.addressAt(
            aResultSlice.maValue.maStart.mnColumn + aResultSlice.maValue.maDimensions.mnColumns - 1,
            aResultSlice.maValue.maStart.mnRow + aResultSlice.maValue.maDimensions.mnRows - 1);

        if (aSliceReference.isSingleCell())
            return materializeReferenceValue(aSliceReference, 0, 0);

        return makeReferenceResult(aSliceReference);
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
            evaluatorEncodingService(), static_cast<double>(*oWholeNumber));
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
            api::text::codeFromText(evaluatorEncodingService(), aText.maValue))));
    }

    if (aFunctionName == u"ADDRESS")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRowNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aRowNumber)
            return makeFailure(aRowNumber.meError);
        const auto aColumnNumber = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aColumnNumber)
            return makeFailure(aColumnNumber.meError);

        const auto oWholeRow = toWholeNumber(aRowNumber.maValue);
        const auto oWholeColumn = toWholeNumber(aColumnNumber.maValue);
        if (!oWholeRow || !oWholeColumn || *oWholeRow < 1 || *oWholeColumn < 1)
            return makeFailure(api::Error::IllegalArgument);

        sal_Int32 nAbsMode = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aAbsMode = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aAbsMode)
                return makeFailure(aAbsMode.meError);
            const auto oWholeAbsMode = toWholeNumber(aAbsMode.maValue);
            if (!oWholeAbsMode || *oWholeAbsMode < 1 || *oWholeAbsMode > 4)
                return makeFailure(api::Error::IllegalArgument);
            nAbsMode = *oWholeAbsMode;
        }

        bool bA1Style = true;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aA1Argument = evaluateScalarArgumentValue(*rNode.maChildren[3]);
            if (!aA1Argument)
                return makeFailure(aA1Argument.meError);
            if (!aA1Argument.maValue.isEmpty())
            {
                const auto aA1Bool = coerceToBoolean(aA1Argument.maValue);
                if (!aA1Bool)
                    return makeFailure(aA1Bool.meError);
                bA1Style = aA1Bool.maValue;
            }
        }

        api::String aSheetName;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aSheetArgument = evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aSheetArgument)
                return makeFailure(aSheetArgument.meError);
            const auto aSheetText = coerceToString(aSheetArgument.maValue);
            if (!aSheetText)
                return makeFailure(aSheetText.meError);
            aSheetName = aSheetText.maValue;
        }

        return makeScalarResult(api::CellValue::text(formatAddressFunctionResult(
            static_cast<api::RowIndex>(*oWholeRow - 1),
            static_cast<api::ColumnIndex>(*oWholeColumn - 1), nAbsMode, bA1Style, aSheetName)));
    }

    if (aFunctionName == u"EUROCONVERT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

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
            return makeFailure(aValueNumber.meError);
        const auto aFromUnit = coerceToString(aFromUnitArgument.maValue.maValue);
        if (!aFromUnit)
            return makeFailure(aFromUnit.meError);
        const auto aToUnit = coerceToString(aToUnitArgument.maValue.maValue);
        if (!aToUnit)
            return makeFailure(aToUnit.meError);

        bool bFullPrecision = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aFullPrecision = evaluateScalarArgumentValue(*rNode.maChildren[3]);
            if (!aFullPrecision)
                return makeFailure(aFullPrecision.meError);
            if (!aFullPrecision.maValue.isEmpty())
            {
                const auto aBool = coerceToBoolean(aFullPrecision.maValue);
                if (!aBool)
                    return makeFailure(aBool.meError);
                bFullPrecision = aBool.maValue;
            }
        }

        if (rNode.maChildren.size() == 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);

            const auto aTriangulationPrecision
                = evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aTriangulationPrecision)
                return makeFailure(aTriangulationPrecision.meError);

            const auto oWholePrecision = toWholeNumber(aTriangulationPrecision.maValue);
            if (!oWholePrecision || *oWholePrecision < 3)
                return makeFailure(api::Error::IllegalArgument);
        }

        const auto aConverted = evaluateEuroConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue, true, !bFullPrecision);
        if (!aConverted)
            return makeFailure(aConverted.meError);
        return makeScalarResult(api::CellValue::number(aConverted.maValue));
    }

    if (aFunctionName == u"CONVERT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);
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
            return makeFailure(aValueNumber.meError);
        const auto aFromUnit = coerceToString(aFromUnitArgument.maValue.maValue);
        if (!aFromUnit)
            return makeFailure(aFromUnit.meError);
        const auto aToUnit = coerceToString(aToUnitArgument.maValue.maValue);
        if (!aToUnit)
            return makeFailure(aToUnit.meError);

        const auto aConverted = evaluateConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue);
        if (aConverted)
            return makeScalarResult(api::CellValue::number(aConverted.maValue));

        const auto aEuroConverted = evaluateEuroConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue, false, false);
        if (aEuroConverted)
        {
            return makeScalarResult(api::CellValue::number(aEuroConverted.maValue));
        }

        return makeFailure(aConverted.meError);
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

        const auto aResult = api::numeral::fromBase(aText.maValue, aBaseNumber.maValue);
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

        const auto aResult = api::numeral::toBase(aValueNumber.maValue, 16.0, oPlaces);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::text(aResult.maValue));
    }

    if (aFunctionName == u"BITXOR")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        auto evaluateBitXorArgument = [&](const formula::Node& rArgument,
                                          std::optional<double> oDefaultValue)
            -> api::ValueResult<sal_uInt64> {
            const auto aValue = evaluateNumericArgument(rArgument, oDefaultValue);
            if (!aValue)
                return api::ValueResult<sal_uInt64>::failure(aValue.meError);

            if (!std::isfinite(aValue.maValue) || aValue.maValue < 0.0
                || aValue.maValue > 281474976710655.0)
            {
                return api::ValueResult<sal_uInt64>::failure(api::Error::IllegalArgument);
            }

            const double fRounded = std::round(aValue.maValue);
            if (std::abs(aValue.maValue - fRounded) > 1e-9)
                return api::ValueResult<sal_uInt64>::failure(api::Error::IllegalArgument);
            return api::ValueResult<sal_uInt64>::success(
                static_cast<sal_uInt64>(fRounded));
        };

        const auto aLeft = evaluateBitXorArgument(*rNode.maChildren[0], 0.0);
        if (!aLeft)
            return makeFailure(aLeft.meError);

        if (rNode.maChildren.size() == 1)
            return makeFailure(api::Error::NoValue);

        const auto aRight = evaluateBitXorArgument(*rNode.maChildren[1], 0.0);
        if (!aRight)
            return makeFailure(aRight.meError);

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(aLeft.maValue ^ aRight.maValue)));
    }

    if (aFunctionName == u"BITLSHIFT" || aFunctionName == u"BITRSHIFT")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValueNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aShiftNumber = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aShiftNumber)
            return makeFailure(aShiftNumber.meError);

        const auto oWholeValue = toWholeNumber(aValueNumber.maValue);
        const auto oWholeShift = toWholeNumber(aShiftNumber.maValue);
        if (!oWholeValue || !oWholeShift || *oWholeValue < 0)
            return makeFailure(api::Error::IllegalArgument);

        const sal_Int32 nShift = *oWholeShift;
        const sal_uInt64 nValue = static_cast<sal_uInt64>(*oWholeValue);

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
            const sal_Int32 nLeftShift = -nShift;
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

        if (!(fBase > 0.0) || rtl::math::approxEqual(fBase, 1.0))
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(
            std::log(aValueNumber.maValue) / std::log(fBase)));
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

        if (rtl::math::approxEqual(aMultipleNumber.maValue, 0.0))
            return makeScalarResult(api::CellValue::number(0.0));

        const double fResult = aMultipleNumber.maValue
                               * rtl::math::round(
                                   rtl::math::approxValue(aValueNumber.maValue
                                                          / aMultipleNumber.maValue));
        if (!std::isfinite(fResult))
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"COMBIN")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aN = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aN)
            return makeFailure(aN.meError);
        const auto aK = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aK)
            return makeFailure(aK.meError);

        const auto oWholeN = toWholeNumber(aN.maValue);
        const auto oWholeK = toWholeNumber(aK.maValue);
        if (!oWholeN || !oWholeK || *oWholeN < 0 || *oWholeK < 0 || *oWholeK > *oWholeN)
            return makeFailure(api::Error::IllegalArgument);

        sal_Int32 n = *oWholeN;
        sal_Int32 k = std::min(*oWholeK, static_cast<sal_Int32>(*oWholeN - *oWholeK));
        double fResult = 1.0;
        for (sal_Int32 i = 1; i <= k; ++i)
            fResult = fResult * static_cast<double>(n - k + i) / static_cast<double>(i);
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"COMBINA")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aN = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aN)
            return makeFailure(aN.meError);
        const auto aK = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aK)
            return makeFailure(aK.meError);

        const auto oWholeN = toWholeNumber(aN.maValue);
        const auto oWholeK = toWholeNumber(aK.maValue);
        if (!oWholeN || !oWholeK || *oWholeN < 0 || *oWholeK < 0)
            return makeFailure(api::Error::IllegalArgument);
        if (*oWholeN == 0 && *oWholeK == 0)
            return makeScalarResult(api::CellValue::number(0.0));
        if (*oWholeK == 0)
            return makeScalarResult(api::CellValue::number(1.0));
        if (*oWholeN < *oWholeK)
            return makeFailure(api::Error::IllegalArgument);

        const sal_Int32 n = *oWholeN + *oWholeK - 1;
        const sal_Int32 k = *oWholeK;
        sal_Int32 nChoose = std::min(k, static_cast<sal_Int32>(n - k));
        double fResult = 1.0;
        for (sal_Int32 i = 1; i <= nChoose; ++i)
            fResult = fResult * static_cast<double>(n - nChoose + i) / static_cast<double>(i);
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"MULTINOMIAL")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        auto binomialCoefficient = [](double fN, double fK) {
            double fValue = 0.0;
            fK = ::rtl::math::approxFloor(fK);
            if (fN < fK)
                return fValue;
            if (fK == 0.0)
                return 1.0;

            fValue = fN / fK;
            fN -= 1.0;
            fK -= 1.0;
            while (fK > 0.0)
            {
                fValue *= fN / fK;
                fK -= 1.0;
                fN -= 1.0;
            }
            return fValue;
        };

        double fTotal = 0.0;
        double fResult = 1.0;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aValue = evaluateNumericArgument(*pChild, std::nullopt);
            if (!aValue)
                return makeFailure(aValue.meError);

            const double fRounded
                = aValue.maValue >= 0.0 ? ::rtl::math::approxFloor(aValue.maValue)
                                        : ::rtl::math::approxCeil(aValue.maValue);
            if (fRounded < 0.0)
                return makeFailure(api::Error::IllegalArgument);

            if (fRounded > 0.0)
            {
                fTotal += fRounded;
                fResult *= binomialCoefficient(fTotal, fRounded);
                if (!std::isfinite(fResult))
                    return makeFailure(api::Error::IllegalArgument);
            }
        }
        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"CSC")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aValue)
            return makeFailure(aValue.meError);
        if (!aValue.maValue.isNumber())
            return makeFailure(api::Error::IllegalArgument);

        const double fSine = ::rtl::math::sin(aValue.maValue.mfNumber);
        if (!std::isfinite(fSine))
            return makeFailure(api::Error::IllegalArgument);
        if (rtl::math::approxEqual(fSine, 0.0))
            return makeFailure(api::Error::DivisionByZero);
        return makeScalarResult(api::CellValue::number(1.0 / fSine));
    }

    if (aFunctionName == u"CSCH")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aValue)
            return makeFailure(aValue.meError);
        if (!aValue.maValue.isNumber())
            return makeFailure(api::Error::IllegalArgument);

        const double fSinh = std::sinh(aValue.maValue.mfNumber);
        if (!std::isfinite(fSinh))
            return makeFailure(api::Error::IllegalArgument);
        if (rtl::math::approxEqual(fSinh, 0.0))
            return makeFailure(api::Error::DivisionByZero);
        return makeScalarResult(api::CellValue::number(1.0 / fSinh));
    }

    if (aFunctionName == u"TRUNC")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        sal_Int32 nDigits = 0;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aDigits = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aDigits)
                return makeFailure(aDigits.meError);
            const auto oWholeDigits = toWholeNumber(aDigits.maValue);
            if (!oWholeDigits)
                return makeFailure(api::Error::IllegalArgument);
            nDigits = *oWholeDigits;
        }

        const double fScale = std::pow(10.0, std::abs(nDigits));
        double fResult = 0.0;
        if (nDigits >= 0)
            fResult = std::trunc(aValue.maValue * fScale) / fScale;
        else
            fResult = std::trunc(aValue.maValue / fScale) * fScale;
        return makeScalarResult(api::CellValue::number(fResult));
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
            = api::text::unicharFromCodePoint(static_cast<sal_uInt32>(*oWholeNumber));
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
                                              evaluatorCaseMappingService(), aText.maValue)
                                        : api::text::lowercase(
                                              evaluatorCaseMappingService(), aText.maValue);
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
        return makeScalarResult(api::CellValue::text(propercaseText(aText.maValue)));
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

        const auto aConverted = transliterateTextWidth(
            aText.maValue, aFunctionName == u"ASC" ? u"Fullwidth-Halfwidth" : u"Halfwidth-Fullwidth");
        if (!aConverted)
            return makeFailure(aConverted.meError);
        return makeScalarResult(api::CellValue::text(aConverted.maValue));
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
            static_cast<double>(expandDbcsByteText(aText.maValue, false).size())));
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
            const auto aStart = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aStart)
                return makeFailure(aStart.meError);
            const auto oWholeStart = toWholeNumber(aStart.maValue);
            if (!oWholeStart || *oWholeStart < 1)
                return makeFailure(api::Error::IllegalArgument);
            nStartIndex = static_cast<std::size_t>(*oWholeStart - 1);
        }

        const auto oFoundIndex = findDbcsExpandedText(aNeedle.maValue, aHaystack.maValue,
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

        api::String aExpandedSource = expandDbcsByteText(aSource.maValue, false);
        const api::String aExpandedReplacement = expandDbcsByteText(aReplacement.maValue, false);
        const std::size_t nStartIndex = static_cast<std::size_t>(*oWholeStart - 1);
        const std::size_t nReplaceLength = static_cast<std::size_t>(*oWholeLength);
        if (nStartIndex >= aExpandedSource.size()
            || nReplaceLength > aExpandedSource.size() - nStartIndex)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        aExpandedSource.replace(nStartIndex, nReplaceLength, aExpandedReplacement);
        return makeScalarResult(
            api::CellValue::text(collapseDbcsByteText(aExpandedSource)));
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

        std::optional<sal_Int32> oInstance;
        if (rNode.maChildren.size() == 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aInstance)
                return makeFailure(aInstance.meError);
            const auto oWholeInstance = toWholeNumber(aInstance.maValue);
            if (!oWholeInstance || *oWholeInstance < 1)
                return makeFailure(api::Error::IllegalArgument);
            oInstance = *oWholeInstance;
        }

        api::String aResult;
        std::size_t nSearchOffset = 0;
        sal_Int32 nMatchCount = 0;
        while (nSearchOffset <= aSource.maValue.size())
        {
            const std::size_t nFound
                = aSource.maValue.find(aOldText.maValue, nSearchOffset);
            if (nFound == api::String::npos)
            {
                aResult.append(aSource.maValue.substr(nSearchOffset));
                break;
            }

            aResult.append(aSource.maValue.substr(nSearchOffset, nFound - nSearchOffset));
            ++nMatchCount;
            if (!oInstance || *oInstance == nMatchCount)
                aResult.append(aNewText.maValue);
            else
                aResult.append(aOldText.maValue);

            nSearchOffset = nFound + aOldText.maValue.size();
        }

        return makeScalarResult(api::CellValue::text(aResult));
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

        sal_Int32 nStart = 1;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aStart = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aStart)
                return makeFailure(aStart.meError);
            const auto oWholeStart = toWholeNumber(aStart.maValue);
            if (!oWholeStart || *oWholeStart < 1)
                return makeFailure(api::Error::IllegalArgument);
            nStart = *oWholeStart;
        }

        const auto oFoundIndex = findTextCodePointIndex(
            aNeedle.maValue, aHaystack.maValue, nStart - 1, aFunctionName == u"SEARCH");
        if (!oFoundIndex)
            return makeFailure(api::Error::NotAvailable);

        return makeScalarResult(api::CellValue::number(static_cast<double>(*oFoundIndex + 1)));
    }

    if (aFunctionName == u"TEXTAFTER")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTextValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTextValue)
            return makeFailure(aTextValue.meError);
        const auto aText = coerceToString(aTextValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        std::vector<api::String> aDelimiters;
        const auto aDelimiterVisit = visitFlattenedValues(
            visitFlattenedValues, *rNode.maChildren[1],
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

        sal_Int32 nInstance = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
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
            const auto aMatchMode = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
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
            const auto aMatchEnd = evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aMatchEnd)
                return makeFailure(aMatchEnd.meError);
            const auto aMatchEndBool = coerceToBoolean(aMatchEnd.maValue);
            if (!aMatchEndBool)
                return makeFailure(aMatchEndBool.meError);
            bMatchEnd = aMatchEndBool.maValue;
        }

        const auto aMatches
            = collectTextDelimiterMatches(aText.maValue, aDelimiters, bCaseInsensitive);
        const auto handleNotFound = [&]() -> EvaluationResult {
            if (rNode.maChildren.size() >= 6)
                return evaluateNode(*rNode.maChildren[5], rCurrentAddress);
            return makeFailure(api::Error::NotAvailable);
        };

        sal_Int32 nSliceStart = 0;
        if (nInstance > 0)
        {
            const std::size_t nRequested = static_cast<std::size_t>(nInstance);
            if (aMatches.size() >= nRequested)
            {
                const auto& rMatch = aMatches[nRequested - 1];
                nSliceStart = rMatch.mnCodePointIndex + rMatch.mnCodePointLength;
            }
            else if (bMatchEnd)
            {
                if (aMatches.empty())
                    nSliceStart = 0;
                else
                {
                    const auto& rMatch = aMatches.back();
                    nSliceStart = rMatch.mnCodePointIndex + rMatch.mnCodePointLength;
                }
            }
            else
                return handleNotFound();
        }
        else
        {
            const std::size_t nRequested = static_cast<std::size_t>(-nInstance);
            if (aMatches.size() >= nRequested)
            {
                const auto& rMatch = aMatches[aMatches.size() - nRequested];
                nSliceStart = rMatch.mnCodePointIndex + rMatch.mnCodePointLength;
            }
            else if (bMatchEnd)
            {
                if (aMatches.empty())
                    nSliceStart = 0;
                else
                {
                    const auto& rMatch = aMatches.front();
                    nSliceStart = rMatch.mnCodePointIndex + rMatch.mnCodePointLength;
                }
            }
            else
                return handleNotFound();
        }

        const sal_Int32 nTextLength = api::text::countCodePoints(aText.maValue);
        return makeScalarResult(api::CellValue::text(
            substringByCodePoints(aText.maValue, nSliceStart, nTextLength - nSliceStart)));
    }

    if (aFunctionName == u"TEXTBEFORE")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTextValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTextValue)
            return makeFailure(aTextValue.meError);
        const auto aText = coerceToString(aTextValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        std::vector<api::String> aDelimiters;
        const auto aDelimiterVisit = visitFlattenedValues(
            visitFlattenedValues, *rNode.maChildren[1],
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

        sal_Int32 nInstance = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
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
            const auto aMatchMode = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
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
            const auto aMatchEnd = evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aMatchEnd)
                return makeFailure(aMatchEnd.meError);
            const auto aMatchEndBool = coerceToBoolean(aMatchEnd.maValue);
            if (!aMatchEndBool)
                return makeFailure(aMatchEndBool.meError);
            bMatchEnd = aMatchEndBool.maValue;
        }

        const auto aMatches
            = collectTextDelimiterMatches(aText.maValue, aDelimiters, bCaseInsensitive);
        const auto handleNotFound = [&]() -> EvaluationResult {
            if (rNode.maChildren.size() >= 6)
                return evaluateNode(*rNode.maChildren[5], rCurrentAddress);
            return makeFailure(api::Error::NotAvailable);
        };

        sal_Int32 nSliceLength = 0;
        if (nInstance > 0)
        {
            const std::size_t nRequested = static_cast<std::size_t>(nInstance);
            if (aMatches.size() >= nRequested)
                nSliceLength = aMatches[nRequested - 1].mnCodePointIndex;
            else if (bMatchEnd)
                nSliceLength = api::text::countCodePoints(aText.maValue);
            else
                return handleNotFound();
        }
        else
        {
            const std::size_t nRequested = static_cast<std::size_t>(-nInstance);
            if (aMatches.size() >= nRequested)
                nSliceLength = aMatches[aMatches.size() - nRequested].mnCodePointIndex;
            else if (bMatchEnd)
                nSliceLength = api::text::countCodePoints(aText.maValue);
            else
                return handleNotFound();
        }

        return makeScalarResult(api::CellValue::text(
            substringByCodePoints(aText.maValue, 0, nSliceLength)));
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
            substringByCodePoints(aText.maValue, *oWholeStart - 1, *oWholeLength)));
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

        return makeScalarResult(api::CellValue::text(replaceByCodePoints(
            aSource.maValue, *oWholeStart - 1, *oWholeLength, aReplacement.maValue)));
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

        const auto aResult = api::numeral::toBase(
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

        const auto aResult = api::numeral::toRoman(aValueNumber.maValue, ofMode);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::text(aResult.maValue));
    }

    if (aFunctionName == u"INDIRECT")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aReferenceTextValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aReferenceTextValue)
            return makeFailure(aReferenceTextValue.meError);
        const auto aReferenceText = coerceToString(aReferenceTextValue.maValue);
        if (!aReferenceText)
            return makeFailure(aReferenceText.meError);

        bool bUseA1 = true;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aA1Argument = evaluateScalarArgumentValue(*rNode.maChildren[1]);
            if (!aA1Argument)
                return makeFailure(aA1Argument.meError);
            const auto aA1Bool = coerceToBoolean(aA1Argument.maValue);
            if (!aA1Bool)
                return makeFailure(aA1Bool.meError);
            bUseA1 = aA1Bool.maValue;
        }

        std::optional<api::ResolvedReference> oReference;
        if (bUseA1)
        {
            if (const auto oNormalized = normalizeIndirectA1ReferenceText(aReferenceText.maValue))
            {
                const auto aResolved = resolveReferenceText(*oNormalized, rCurrentAddress.mnSheet);
                if (aResolved)
                    oReference = aResolved.maValue;
            }

            if (!oReference)
            {
                const auto aNamed
                    = resolveNamedRange(aReferenceText.maValue, rCurrentAddress.mnSheet);
                if (aNamed)
                    oReference = aNamed.maValue;
            }
        }
        else
            oReference = parseIndirectR1C1ReferenceText(
                aReferenceText.maValue, mrWorkbook, rCurrentAddress.mnSheet);

        if (!oReference)
            return makeFailure(api::Error::IllegalArgument);
        if (oReference->isSingleCell())
            return materializeReferenceValue(*oReference, 0, 0);
        return makeReferenceResult(*oReference);
    }

    if (aFunctionName == u"HYPERLINK")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLinkTarget
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aLinkTarget)
            return aLinkTarget;
        if (aLinkTarget.maValue.maValue.isError())
            return makeFailure(aLinkTarget.maValue.maValue.meError);

        if (rNode.maChildren.size() == 1)
            return makeScalarResult(aLinkTarget.maValue.maValue);

        EvaluationResult aDisplayValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDisplayValue)
            return aDisplayValue;
        return makeScalarResult(aDisplayValue.maValue.maValue);
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

        sal_Int32 nLength = 1;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aLength = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aLength)
                return makeFailure(aLength.meError);
            const auto oWholeLength = toWholeNumber(aLength.maValue);
            if (!oWholeLength || *oWholeLength < 0)
                return makeFailure(api::Error::IllegalArgument);
            nLength = *oWholeLength;
        }

        const sal_Int32 nCodePointCount = api::text::countCodePoints(aText.maValue);
        const sal_Int32 nSliceLength = std::min(nLength, nCodePointCount);
        const sal_Int32 nSliceStart
            = aFunctionName == u"LEFT" ? 0 : std::max<sal_Int32>(0, nCodePointCount - nSliceLength);
        return makeScalarResult(api::CellValue::text(
            substringByCodePoints(aText.maValue, nSliceStart, nSliceLength)));
    }

    if (aFunctionName == u"TEXTJOIN")
    {
        if (rNode.maChildren.size() < 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDelimiterValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDelimiterValue)
            return makeFailure(aDelimiterValue.meError);
        const auto aDelimiter = coerceToString(aDelimiterValue.maValue);
        if (!aDelimiter)
            return makeFailure(aDelimiter.meError);

        const auto aIgnoreEmptyValue = evaluateScalarArgumentValue(*rNode.maChildren[1]);
        if (!aIgnoreEmptyValue)
            return makeFailure(aIgnoreEmptyValue.meError);
        const auto aIgnoreEmpty = coerceToBoolean(aIgnoreEmptyValue.maValue);
        if (!aIgnoreEmpty)
            return makeFailure(aIgnoreEmpty.meError);

        api::String aResult;
        bool bHaveAny = false;
        for (std::size_t nIndex = 2; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *rNode.maChildren[nIndex],
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
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
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

    if (aFunctionName == u"OFFSET")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const bool bReferenceLike = rNode.maChildren[0]->meKind == formula::NodeKind::CellReference
                                    || rNode.maChildren[0]->meKind == formula::NodeKind::RangeReference
                                    || rNode.maChildren[0]->meKind == formula::NodeKind::NamedReference;
        EvaluationResult aReference = bReferenceLike
                                          ? evaluateReferenceNode(*rNode.maChildren[0], rCurrentAddress)
                                          : evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aReference)
            return aReference;
        if (!aReference.maValue.isMatrixReference())
            return makeFailure(api::Error::IllegalArgument);

        const auto aRows = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aRows)
            return makeFailure(aRows.meError);
        const auto aColumns = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aColumns)
            return makeFailure(aColumns.meError);

        const sal_Int32 nRowOffset = static_cast<sal_Int32>(std::trunc(aRows.maValue));
        const sal_Int32 nColumnOffset = static_cast<sal_Int32>(std::trunc(aColumns.maValue));

        sal_Int32 nHeight
            = static_cast<sal_Int32>(aReference.maValue.maReference.maRange.rowCount());
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aHeight = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aHeight)
                return makeFailure(aHeight.meError);
            const auto oWholeHeight = toWholeNumber(aHeight.maValue);
            if (!oWholeHeight || *oWholeHeight < 1)
                return makeFailure(api::Error::IllegalArgument);
            nHeight = *oWholeHeight;
        }

        sal_Int32 nWidth
            = static_cast<sal_Int32>(aReference.maValue.maReference.maRange.columnCount());
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aWidth = evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aWidth)
                return makeFailure(aWidth.meError);
            const auto oWholeWidth = toWholeNumber(aWidth.maValue);
            if (!oWholeWidth || *oWholeWidth < 1)
                return makeFailure(api::Error::IllegalArgument);
            nWidth = *oWholeWidth;
        }

        const auto& rSourceRange = aReference.maValue.maReference.maRange;
        const sal_Int64 nStartColumn
            = static_cast<sal_Int64>(rSourceRange.maStart.mnColumn) + nColumnOffset;
        const sal_Int64 nStartRow
            = static_cast<sal_Int64>(rSourceRange.maStart.mnRow) + nRowOffset;
        const sal_Int64 nEndColumn = nStartColumn + nWidth - 1;
        const sal_Int64 nEndRow = nStartRow + nHeight - 1;
        if (nStartColumn < 0 || nStartRow < 0 || nEndColumn < 0 || nEndRow < 0)
            return makeFailure(api::Error::NoValue);

        api::ResolvedReference aOffsetReference = aReference.maValue.maReference;
        aOffsetReference.maRange.maStart.mnColumn = static_cast<api::ColumnIndex>(nStartColumn);
        aOffsetReference.maRange.maStart.mnRow = static_cast<api::RowIndex>(nStartRow);
        aOffsetReference.maRange.maEnd.mnColumn = static_cast<api::ColumnIndex>(nEndColumn);
        aOffsetReference.maRange.maEnd.mnRow = static_cast<api::RowIndex>(nEndRow);
        return makeReferenceResult(aOffsetReference);
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

        if (aRightNumber.maValue == 0.0)
            return makeFailure(api::Error::DivisionByZero);

        const auto aModResult = api::math::modulo(aLeftNumber.maValue, aRightNumber.maValue);
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

    if (aFunctionName == u"AND")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        bool bResult = true;
        bool bSawValue = false;
        for (const auto& pChild : rNode.maChildren)
        {
            const bool bReferenceLike = pChild->meKind == formula::NodeKind::CellReference
                                        || pChild->meKind == formula::NodeKind::RangeReference
                                        || pChild->meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike
                                             ? evaluateReferenceNode(*pChild, rCurrentAddress)
                                             : evaluateNode(*pChild, rCurrentAddress);
            if (!aArgument)
                return aArgument;

            if (aArgument.maValue.isMatrixReference())
            {
                const auto& rReference = aArgument.maValue.maReference;
                for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
                {
                    for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                    {
                        EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                        if (!aCell)
                            return aCell;
                        if (aCell.maValue.maValue.isEmpty() || aCell.maValue.maValue.isText())
                            continue;
                        const auto aBool = coerceToBoolean(aCell.maValue.maValue);
                        if (!aBool)
                            return makeFailure(aBool.meError);
                        bResult = bResult && aBool.maValue;
                        bSawValue = true;
                    }
                }
                continue;
            }

            const auto aBool = coerceToBoolean(aArgument.maValue.maValue);
            if (!aBool)
                return makeFailure(aBool.meError);
            bResult = bResult && aBool.maValue;
            bSawValue = true;
        }

        if (!bSawValue)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::boolean(bResult));
    }

    if (aFunctionName == u"MAX" || aFunctionName == u"MIN")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        std::optional<double> oBestValue;
        const bool bFindMaximum = aFunctionName == u"MAX";
        for (const auto& pChild : rNode.maChildren)
        {
            const bool bReferenceLike = pChild->meKind == formula::NodeKind::CellReference
                                        || pChild->meKind == formula::NodeKind::RangeReference
                                        || pChild->meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike
                                             ? evaluateReferenceNode(*pChild, rCurrentAddress)
                                             : evaluateNode(*pChild, rCurrentAddress);
            if (!aArgument)
                return aArgument;

            auto considerValue = [&](const api::CellValue& rValue) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);

                if (rValue.isNumber())
                {
                    if (!oBestValue
                        || (bFindMaximum ? rValue.mfNumber > *oBestValue
                                         : rValue.mfNumber < *oBestValue))
                    {
                        oBestValue = rValue.mfNumber;
                    }
                    return api::ValueResult<bool>::success(true);
                }

                return api::ValueResult<bool>::success(false);
            };

            if (aArgument.maValue.isMatrixReference())
            {
                const auto& rReference = aArgument.maValue.maReference;
                for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
                {
                    for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                    {
                        EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                        if (!aCell)
                            return aCell;

                        const auto aConsidered = considerValue(aCell.maValue.maValue);
                        if (!aConsidered)
                            return makeFailure(aConsidered.meError);
                    }
                }
                continue;
            }

            const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
            if (!aNumber)
                return makeFailure(aNumber.meError);

            if (!oBestValue
                || (bFindMaximum ? aNumber.maValue > *oBestValue
                                 : aNumber.maValue < *oBestValue))
            {
                oBestValue = aNumber.maValue;
            }
        }

        return makeScalarResult(api::CellValue::number(oBestValue.value_or(0.0)));
    }

    if (aFunctionName == u"MAXA" || aFunctionName == u"MINA")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = collectVarianceArguments(true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeFailure(api::Error::NoValue);

        const auto pBest = aFunctionName == u"MAXA"
                               ? std::max_element(aNumbers.maValue.begin(), aNumbers.maValue.end())
                               : std::min_element(aNumbers.maValue.begin(), aNumbers.maValue.end());
        return makeScalarResult(api::CellValue::number(*pBest));
    }

    if (aFunctionName == u"SUBTOTAL")
    {
        const auto makeSubtotalError = [](api::Error eError) {
            return makeScalarResult(api::CellValue::error(eError));
        };

        if (rNode.maChildren.size() < 2)
            return makeSubtotalError(api::Error::IllegalArgument);

        EvaluationResult aFunctionCode
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aFunctionCode)
            return aFunctionCode.maCyclePath.empty() ? makeSubtotalError(aFunctionCode.meError)
                                                     : aFunctionCode;

        const auto aFunctionNumber = coerceToNumber(aFunctionCode.maValue.maValue);
        if (!aFunctionNumber)
            return makeSubtotalError(aFunctionNumber.meError);

        const auto oFunctionCode = toWholeNumber(aFunctionNumber.maValue);
        if (!oFunctionCode)
            return makeSubtotalError(api::Error::IllegalArgument);

        bool bIgnoreHiddenRows = false;
        sal_Int32 nAggregateFunction = 0;
        if (*oFunctionCode >= 1 && *oFunctionCode <= 11)
            nAggregateFunction = *oFunctionCode;
        else if (*oFunctionCode >= 101 && *oFunctionCode <= 111)
        {
            nAggregateFunction = *oFunctionCode - 100;
            bIgnoreHiddenRows = true;
        }
        else
        {
            return makeSubtotalError(api::Error::IllegalArgument);
        }

        AggregateScan aScan;
        auto consumeSubtotalValue = [&](const api::CellValue& rValue) -> api::ValueResult<bool> {
            if (rValue.isError())
            {
                if (nAggregateFunction == 2)
                    return api::ValueResult<bool>::success(true);
                if (nAggregateFunction == 3)
                {
                    ++aScan.mnNonEmptyCount;
                    return api::ValueResult<bool>::success(true);
                }
                return api::ValueResult<bool>::failure(rValue.meError);
            }

            if (!rValue.isEmpty())
                ++aScan.mnNonEmptyCount;

            if (rValue.isNumber())
                aScan.maNumbers.push_back(rValue.mfNumber);
            return api::ValueResult<bool>::success(true);
        };

        auto scanSubtotalArgument = [&](const formula::Node& rArgument) -> api::ValueResult<bool> {
            const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                        || rArgument.meKind == formula::NodeKind::RangeReference
                                        || rArgument.meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike
                                             ? evaluateReferenceNode(rArgument, rCurrentAddress)
                                             : evaluateNode(rArgument, rCurrentAddress);
            if (!aArgument)
            {
                if (nAggregateFunction == 2)
                    return api::ValueResult<bool>::success(true);
                if (nAggregateFunction == 3)
                {
                    ++aScan.mnNonEmptyCount;
                    return api::ValueResult<bool>::success(true);
                }
                return api::ValueResult<bool>::failure(aArgument.meError);
            }

            if (!aArgument.maValue.isMatrixReference())
                return consumeSubtotalValue(aArgument.maValue.maValue);

            const auto& rReference = aArgument.maValue.maReference;
            const workbook::Sheet* pSheet = getSheet(rReference.maRange.maStart.mnSheet);
            if (!pSheet)
                return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    const api::CellAddress aAddress = rReference.addressAt(nCol, nRow);
                    const bool bFilteredRow = pSheet->isRowFiltered(aAddress.mnRow);
                    const bool bManuallyHiddenRow = pSheet->isRowHidden(aAddress.mnRow);
                    if (bFilteredRow || (bIgnoreHiddenRows && bManuallyHiddenRow))
                        continue;

                    const workbook::Cell* pReferencedCell = getCell(aAddress);
                    if (pReferencedCell && cellContainsAggregateLike(*pReferencedCell))
                        continue;

                    EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                    {
                        if (nAggregateFunction == 2)
                            continue;
                        if (nAggregateFunction == 3)
                        {
                            ++aScan.mnNonEmptyCount;
                            continue;
                        }
                        return api::ValueResult<bool>::failure(aCell.meError);
                    }

                    if (!aCell.maValue.isScalar())
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                    const auto aConsumed = consumeSubtotalValue(aCell.maValue.maValue);
                    if (!aConsumed)
                        return aConsumed;
                }
            }
            return api::ValueResult<bool>::success(true);
        };

        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aScanned = scanSubtotalArgument(*rNode.maChildren[nIndex]);
            if (!aScanned)
                return makeSubtotalError(aScanned.meError);
        }

        const auto aAggregate = evaluateAggregateNumbers(nAggregateFunction, aScan);
        if (!aAggregate)
            return makeSubtotalError(aAggregate.meError);
        return makeScalarResult(api::CellValue::number(aAggregate.maValue));
    }

    if (aFunctionName == u"AGGREGATE")
    {
        const auto makeAggregateError = [](api::Error eError) {
            return makeScalarResult(api::CellValue::error(eError));
        };

        if (rNode.maChildren.size() < 3)
            return makeAggregateError(api::Error::IllegalArgument);

        EvaluationResult aFunctionCode
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aFunctionCode)
            return aFunctionCode.maCyclePath.empty() ? makeAggregateError(aFunctionCode.meError)
                                                     : aFunctionCode;

        EvaluationResult aOptionCode
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aOptionCode)
            return aOptionCode.maCyclePath.empty() ? makeAggregateError(aOptionCode.meError)
                                                   : aOptionCode;

        const auto aFunctionNumber = coerceToNumber(aFunctionCode.maValue.maValue);
        if (!aFunctionNumber)
            return makeAggregateError(aFunctionNumber.meError);
        const auto aOptionNumber = coerceToNumber(aOptionCode.maValue.maValue);
        if (!aOptionNumber)
            return makeAggregateError(aOptionNumber.meError);

        const auto oFunction = toWholeNumber(aFunctionNumber.maValue);
        const auto oOption = toWholeNumber(aOptionNumber.maValue);
        if (!oFunction || !oOption || *oFunction < 1 || *oFunction > 19)
            return makeAggregateError(api::Error::IllegalArgument);

        const auto oOptions = decodeAggregateOptions(*oOption);
        if (!oOptions)
            return makeAggregateError(api::Error::IllegalArgument);

        AggregateScan aScan;
        auto consumeAggregateValue = [&](const api::CellValue& rValue) -> api::ValueResult<bool> {
            if (rValue.isError())
            {
                if (oOptions->mbIgnoreErrors)
                    return api::ValueResult<bool>::success(true);
                if (*oFunction == 2)
                    return api::ValueResult<bool>::success(true);
                if (*oFunction == 3)
                {
                    ++aScan.mnNonEmptyCount;
                    return api::ValueResult<bool>::success(true);
                }
                return api::ValueResult<bool>::failure(rValue.meError);
            }

            if (!rValue.isEmpty())
                ++aScan.mnNonEmptyCount;

            if (rValue.isNumber())
                aScan.maNumbers.push_back(rValue.mfNumber);
            return api::ValueResult<bool>::success(true);
        };

        auto scanAggregateArgument = [&](const formula::Node& rArgument) -> api::ValueResult<bool> {
            const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                        || rArgument.meKind == formula::NodeKind::RangeReference
                                        || rArgument.meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike
                                             ? evaluateReferenceNode(rArgument, rCurrentAddress)
                                             : evaluateNode(rArgument, rCurrentAddress);
            if (!aArgument)
            {
                if (oOptions->mbIgnoreErrors && aArgument.maCyclePath.empty())
                    return api::ValueResult<bool>::success(true);
                if (*oFunction == 2)
                    return api::ValueResult<bool>::success(true);
                if (*oFunction == 3)
                {
                    ++aScan.mnNonEmptyCount;
                    return api::ValueResult<bool>::success(true);
                }
                return api::ValueResult<bool>::failure(aArgument.meError);
            }

            if (!aArgument.maValue.isMatrixReference())
                return consumeAggregateValue(aArgument.maValue.maValue);

            const auto& rReference = aArgument.maValue.maReference;
            const workbook::Sheet* pSheet = getSheet(rReference.maRange.maStart.mnSheet);
            if (!pSheet)
                return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    const api::CellAddress aAddress = rReference.addressAt(nCol, nRow);
                    const bool bFilteredRow = pSheet->isRowFiltered(aAddress.mnRow);
                    const bool bManuallyHiddenRow = pSheet->isRowHidden(aAddress.mnRow);
                    if (bFilteredRow || (oOptions->mbIgnoreHiddenRows && bManuallyHiddenRow))
                        continue;

                    const workbook::Cell* pReferencedCell = getCell(aAddress);
                    if (pReferencedCell && oOptions->mbIgnoreNestedAggregates
                        && cellContainsAggregateLike(*pReferencedCell))
                    {
                        continue;
                    }

                    EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                    {
                        if (oOptions->mbIgnoreErrors && aCell.maCyclePath.empty())
                            continue;
                        if (*oFunction == 2)
                            continue;
                        if (*oFunction == 3)
                        {
                            ++aScan.mnNonEmptyCount;
                            continue;
                        }
                        return api::ValueResult<bool>::failure(aCell.meError);
                    }

                    if (!aCell.maValue.isScalar())
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                    const auto aConsumed = consumeAggregateValue(aCell.maValue.maValue);
                    if (!aConsumed)
                        return aConsumed;
                }
            }
            return api::ValueResult<bool>::success(true);
        };

        const bool bRankedFunction = *oFunction >= 14;
        if (bRankedFunction)
        {
            if (rNode.maChildren.size() != 4)
                return makeAggregateError(api::Error::IllegalArgument);
            const auto aScanned = scanAggregateArgument(*rNode.maChildren[2]);
            if (!aScanned)
                return makeAggregateError(aScanned.meError);

            EvaluationResult aRank
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aRank)
                return aRank.maCyclePath.empty() ? makeAggregateError(aRank.meError) : aRank;
            const auto aRankNumber = coerceToNumber(aRank.maValue.maValue);
            if (!aRankNumber)
                return makeAggregateError(aRankNumber.meError);

            const auto aAggregate = evaluateAggregateRankedNumbers(
                *oFunction, aScan, aRankNumber.maValue);
            if (!aAggregate)
                return makeAggregateError(aAggregate.meError);
            return makeScalarResult(api::CellValue::number(aAggregate.maValue));
        }

        for (std::size_t nIndex = 2; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aScanned = scanAggregateArgument(*rNode.maChildren[nIndex]);
            if (!aScanned)
                return makeAggregateError(aScanned.meError);
        }

        const auto aAggregate = evaluateAggregateNumbers(*oFunction, aScan);
        if (!aAggregate)
            return makeAggregateError(aAggregate.meError);
        return makeScalarResult(api::CellValue::number(aAggregate.maValue));
    }

    if (aFunctionName == u"LARGE" || aFunctionName == u"SMALL" || aFunctionName == u"PERCENTILE"
        || aFunctionName == u"PERCENTILE.INC"
        || aFunctionName == u"COM.MICROSOFT.PERCENTILE.INC"
        || aFunctionName == u"PERCENTILE.EXC"
        || aFunctionName == u"COM.MICROSOFT.PERCENTILE.EXC"
        || aFunctionName == u"QUARTILE" || aFunctionName == u"QUARTILE.INC"
        || aFunctionName == u"COM.MICROSOFT.QUARTILE.INC"
        || aFunctionName == u"QUARTILE.EXC"
        || aFunctionName == u"COM.MICROSOFT.QUARTILE.EXC")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aScan = collectAggregateScanFromArgument(*rNode.maChildren[0]);
        if (!aScan)
            return makeFailure(aScan.meError);

        const auto aRank = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aRank)
            return makeFailure(aRank.meError);

        sal_Int32 nAggregateFunction = 0;
        if (aFunctionName == u"LARGE")
            nAggregateFunction = 14;
        else if (aFunctionName == u"SMALL")
            nAggregateFunction = 15;
        else if (aFunctionName == u"PERCENTILE" || aFunctionName == u"PERCENTILE.INC"
                 || aFunctionName == u"COM.MICROSOFT.PERCENTILE.INC")
            nAggregateFunction = 16;
        else if (aFunctionName == u"QUARTILE" || aFunctionName == u"QUARTILE.INC"
                 || aFunctionName == u"COM.MICROSOFT.QUARTILE.INC")
            nAggregateFunction = 17;
        else if (aFunctionName == u"PERCENTILE.EXC"
                 || aFunctionName == u"COM.MICROSOFT.PERCENTILE.EXC")
            nAggregateFunction = 18;
        else
            nAggregateFunction = 19;

        const auto aAggregate = evaluateAggregateRankedNumbers(
            nAggregateFunction, aScan.maValue, aRank.maValue);
        if (!aAggregate)
            return makeFailure(aAggregate.meError);
        return makeScalarResult(api::CellValue::number(aAggregate.maValue));
    }

    if (aFunctionName == u"SKEW" || aFunctionName == u"SKEWP")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        AggregateScan aScan;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aCollected = collectAggregateScanFromArgument(*pChild);
            if (!aCollected)
                return makeFailure(aCollected.meError);
            aScan.maNumbers.insert(aScan.maNumbers.end(), aCollected.maValue.maNumbers.begin(),
                aCollected.maValue.maNumbers.end());
        }

        const std::size_t nCount = aScan.maNumbers.size();
        if (aFunctionName == u"SKEW")
        {
            if (nCount < 3)
                return makeFailure(api::Error::DivisionByZero);
        }
        else if (nCount == 0)
            return makeFailure(api::Error::DivisionByZero);

        const double fMean = sumNumbers(aScan.maNumbers) / static_cast<double>(nCount);
        double fSumSquares = 0.0;
        double fSumCubes = 0.0;
        for (const double fValue : aScan.maNumbers)
        {
            const double fDelta = fValue - fMean;
            fSumSquares += fDelta * fDelta;
            fSumCubes += fDelta * fDelta * fDelta;
        }

        if (fSumSquares == 0.0)
            return makeFailure(api::Error::DivisionByZero);

        if (aFunctionName == u"SKEW")
        {
            const double fSampleVariance
                = fSumSquares / static_cast<double>(nCount - 1);
            const double fSampleDeviation = std::sqrt(fSampleVariance);
            if (fSampleDeviation == 0.0)
                return makeFailure(api::Error::DivisionByZero);

            const double fSkew = static_cast<double>(nCount) * fSumCubes
                                 / ((static_cast<double>(nCount - 1)
                                     * static_cast<double>(nCount - 2))
                                     * std::pow(fSampleDeviation, 3.0));
            return makeScalarResult(api::CellValue::number(fSkew));
        }

        const double fPopulationVariance = fSumSquares / static_cast<double>(nCount);
        const double fPopulationDeviation = std::sqrt(fPopulationVariance);
        if (fPopulationDeviation == 0.0)
            return makeFailure(api::Error::DivisionByZero);

        const double fSkewP = fSumCubes
                              / (static_cast<double>(nCount)
                                 * std::pow(fPopulationDeviation, 3.0));
        return makeScalarResult(api::CellValue::number(fSkewP));
    }

    return makeFailure(api::Error::IllegalArgument);
}

EvaluationResult Evaluator::evaluateNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::NumberLiteral:
            return makeScalarResult(api::CellValue::number(rNode.mfNumber));
        case formula::NodeKind::StringLiteral:
            return makeScalarResult(api::CellValue::text(rNode.maPrimaryText));
        case formula::NodeKind::BooleanLiteral:
            return makeScalarResult(api::CellValue::boolean(rNode.mbBoolean));
        case formula::NodeKind::ErrorLiteral:
            return makeScalarResult(api::CellValue::error(mapErrorLiteral(rNode.maPrimaryText)));
        case formula::NodeKind::EmptyArgument:
            return makeScalarResult(api::CellValue::empty());
        case formula::NodeKind::CellReference:
        {
            const auto aReference = resolveReferenceText(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aReference)
                return makeFailure(aReference.meError);
            return materializeReferenceValue(aReference.maValue, 0, 0);
        }
        case formula::NodeKind::RangeReference:
        {
            api::String aReference = rNode.maPrimaryText;
            aReference.push_back(u':');
            aReference += rNode.maSecondaryText;
            const auto aRange = resolveReferenceText(aReference, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            if (aRange.maValue.isSingleCell())
                return materializeReferenceValue(aRange.maValue, 0, 0);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::NamedReference:
        {
            if (const auto* pLocalBinding = lookupLocalBinding(rNode.maPrimaryText))
                return *pLocalBinding;
            const auto aRange = resolveNamedRange(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            if (aRange.maValue.isSingleCell())
                return materializeReferenceValue(aRange.maValue, 0, 0);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::RangeConstructor:
            return makeFailure(api::Error::IllegalArgument);
        case formula::NodeKind::ReferenceList:
            return makeFailure(api::Error::IllegalArgument);
        case formula::NodeKind::ArrayConstant:
        {
            if (rNode.mnArrayRows != 1 || rNode.mnArrayColumns != 1 || rNode.maChildren.empty())
                return makeFailure(api::Error::IllegalArgument);
            return evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        }
        case formula::NodeKind::UnaryOperation:
        {
            EvaluationResult aChild = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
            if (!aChild)
                return aChild;
            const auto aNumber = coerceToNumber(aChild.maValue.maValue);
            if (!aNumber)
                return makeFailure(aNumber.meError);
            const double fValue = rNode.meUnaryOperator == formula::UnaryOperator::Minus
                                      ? -aNumber.maValue
                                      : aNumber.maValue;
            return makeScalarResult(api::CellValue::number(fValue));
        }
        case formula::NodeKind::BinaryOperation:
        {
            EvaluationResult aLeft = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
            if (!aLeft)
                return aLeft;
            EvaluationResult aRight = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aRight)
                return aRight;

            if (rNode.meBinaryOperator == formula::BinaryOperator::Concat)
            {
                const auto aLeftText = coerceToString(aLeft.maValue.maValue);
                if (!aLeftText)
                    return makeFailure(aLeftText.meError);
                const auto aRightText = coerceToString(aRight.maValue.maValue);
                if (!aRightText)
                    return makeFailure(aRightText.meError);
                api::String aValue = aLeftText.maValue;
                aValue += aRightText.maValue;
                return makeScalarResult(api::CellValue::text(aValue));
            }

            if (rNode.meBinaryOperator == formula::BinaryOperator::Equal
                || rNode.meBinaryOperator == formula::BinaryOperator::NotEqual
                || rNode.meBinaryOperator == formula::BinaryOperator::Less
                || rNode.meBinaryOperator == formula::BinaryOperator::LessEqual
                || rNode.meBinaryOperator == formula::BinaryOperator::Greater
                || rNode.meBinaryOperator == formula::BinaryOperator::GreaterEqual)
            {
                if (aLeft.maValue.maValue.isText() && aRight.maValue.maValue.isText())
                {
                    return makeScalarResult(api::CellValue::boolean(evaluateStringComparison(
                        aLeft.maValue.maValue.maString, aRight.maValue.maValue.maString,
                        rNode.meBinaryOperator)));
                }

                const auto aLeftNumber = coerceToNumber(aLeft.maValue.maValue);
                if (!aLeftNumber)
                    return makeFailure(aLeftNumber.meError);
                const auto aRightNumber = coerceToNumber(aRight.maValue.maValue);
                if (!aRightNumber)
                    return makeFailure(aRightNumber.meError);
                return makeScalarResult(api::CellValue::boolean(evaluateNumericComparison(
                    aLeftNumber.maValue, aRightNumber.maValue, rNode.meBinaryOperator)));
            }

            const auto aLeftNumber = coerceToNumber(aLeft.maValue.maValue);
            if (!aLeftNumber)
                return makeFailure(aLeftNumber.meError);
            const auto aRightNumber = coerceToNumber(aRight.maValue.maValue);
            if (!aRightNumber)
                return makeFailure(aRightNumber.meError);

            switch (rNode.meBinaryOperator)
            {
                case formula::BinaryOperator::Add:
                    return makeScalarResult(api::CellValue::number(
                        ::rtl::math::approxAdd(aLeftNumber.maValue, aRightNumber.maValue)));
                case formula::BinaryOperator::Subtract:
                    return makeScalarResult(api::CellValue::number(
                        ::rtl::math::approxSub(aLeftNumber.maValue, aRightNumber.maValue)));
                case formula::BinaryOperator::Multiply:
                    return makeScalarResult(
                        api::CellValue::number(aLeftNumber.maValue * aRightNumber.maValue));
                case formula::BinaryOperator::Divide:
                    if (aRightNumber.maValue == 0.0)
                        return makeFailure(api::Error::DivisionByZero);
                    return makeScalarResult(
                        api::CellValue::number(aLeftNumber.maValue / aRightNumber.maValue));
                case formula::BinaryOperator::Power:
                    return makeScalarResult(api::CellValue::number(
                        std::pow(aLeftNumber.maValue, aRightNumber.maValue)));
                default:
                    return makeFailure(api::Error::IllegalArgument);
            }
        }
        case formula::NodeKind::FunctionCall:
            return evaluateFunction(rNode, rCurrentAddress);
    }

    return makeFailure(api::Error::IllegalArgument);
}

EvaluationResult Evaluator::evaluateFormula(
    api::StringView rFormula, const api::CellAddress& rCurrentAddress)
{
    const formula::ParseResult aParse = formula::parseFormula(rFormula);
    if (!aParse)
        return makeFailure(api::Error::IllegalArgument);
    return evaluateNode(*aParse.mpRoot, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateCompiledFormula(
    const spreadsheetengine::detail::token::CompiledFormula& rFormula,
    const api::CellAddress& rCurrentAddress)
{
    const auto oInflated = inflateCompiledFormulaNode(rFormula, mrWorkbook, rCurrentAddress);
    if (!oInflated)
        return makeFailure(api::Error::IllegalArgument);
    return evaluateNode(**oInflated, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateFormulaViaCompiledTokens(
    api::StringView rFormula, const api::CellAddress& rCurrentAddress)
{
    if (rCurrentAddress.mnSheet < 0
        || static_cast<std::size_t>(rCurrentAddress.mnSheet) >= mrWorkbook.maSheets.size())
    {
        return makeFailure(api::Error::IllegalArgument);
    }

    secompiler::WorkbookCompileHost aHost(mrWorkbook);
    const auto oContext = secompiler::makeWorkbookCompileContext(mrWorkbook,
        mrWorkbook.maSheets[static_cast<std::size_t>(rCurrentAddress.mnSheet)].maName,
        rCurrentAddress.mnColumn, rCurrentAddress.mnRow);
    if (!oContext)
        return makeFailure(api::Error::IllegalArgument);

    const auto aLowered = secompiler::lowerFormulaSource(rFormula, aHost, *oContext);
    if (!aLowered)
        return makeFailure(api::Error::IllegalArgument);
    return evaluateCompiledFormula(aLowered.maFormula, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateCellInternal(
    const api::CellAddress& rAddress, ExecutionMode eMode)
{
    if (!getSheet(rAddress.mnSheet))
        return makeFailure(api::Error::IllegalArgument);

    const workbook::Cell* pCell = getCell(rAddress);
    if (!pCell)
        return makeScalarResult(api::CellValue::empty());
    if (!pCell->hasFormula())
    {
        if (const auto oTypedValue = parseTypedStoredCellValue(*pCell))
            return makeScalarResult(*oTypedValue);
        return makeScalarResult(pCell->maValue);
    }

    CacheEntry& rEntry = cacheForMode(eMode)[makeAddressKey(rAddress)];
    if (rEntry.meState == CacheState::Complete)
        return rEntry.maResult;

    if (rEntry.meState == CacheState::Active)
    {
        EvaluationResult aCycle = makeFailure(api::Error::IllegalArgument);
        auto aIt = std::find(maEvaluationStack.begin(), maEvaluationStack.end(), rAddress);
        if (aIt != maEvaluationStack.end())
            aCycle.maCyclePath.assign(aIt, maEvaluationStack.end());
        aCycle.maCyclePath.push_back(rAddress);
        return aCycle;
    }

    rEntry.meState = CacheState::Active;
    maEvaluationStack.push_back(rAddress);

    const auto finalize = [&](EvaluationResult aResult) -> EvaluationResult {
        maEvaluationStack.pop_back();
        rEntry.meState = CacheState::Complete;
        rEntry.maResult = aResult;
        return aResult;
    };

    const ExecutionMode ePreviousMode = meActiveExecutionMode;
    meActiveExecutionMode = eMode;
    EvaluationResult aResult = eMode == ExecutionMode::CompiledToken
                                   ? evaluateFormulaViaCompiledTokens(pCell->maFormula, rAddress)
                                   : evaluateFormula(pCell->maFormula, rAddress);
    if (aResult && aResult.maValue.isMatrixReference())
    {
        // Standalone replay compares the anchor cell stored in FODS for matrix formulas.
        // Materialize the top-left value here instead of treating multi-cell array results as
        // an evaluation failure and falling back to the cached workbook value.
        aResult = materializeReferenceValue(aResult.maValue.maReference, 0, 0);
    }

    if (!aResult && aResult.maCyclePath.empty() && hasCachedFallbackValue(*pCell))
    {
        meActiveExecutionMode = ePreviousMode;
        if (const auto oTypedValue = parseTypedStoredCellValue(*pCell))
            return finalize(makeScalarResult(*oTypedValue, true));
        return finalize(makeScalarResult(pCell->maValue, true));
    }

    meActiveExecutionMode = ePreviousMode;
    return finalize(aResult);
}

EvaluationResult Evaluator::evaluateCell(const api::CellAddress& rAddress)
{
    return evaluateCellInternal(rAddress, ExecutionMode::Ast);
}

EvaluationResult Evaluator::evaluateCellViaCompiledTokens(const api::CellAddress& rAddress)
{
    return evaluateCellInternal(rAddress, ExecutionMode::CompiledToken);
}

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
