/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/FodsEvaluator.hxx>

#include <rtl/math.hxx>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Text.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>

#include "DateAlgorithms.hxx"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>

namespace spreadsheetengine::core::fods
{
namespace
{

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

[[nodiscard]] api::String formatNumber(double fValue)
{
    std::string aAscii = std::to_string(fValue);
    const std::size_t nDot = aAscii.find('.');
    if (nDot != std::string::npos)
    {
        while (!aAscii.empty() && aAscii.back() == '0')
            aAscii.pop_back();
        if (!aAscii.empty() && aAscii.back() == '.')
            aAscii.pop_back();
    }

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

[[nodiscard]] api::String normalizeFunctionName(api::StringView rName)
{
    return uppercaseAscii(normalizeDisplayFunctionName(rName));
}

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

EvaluationResult Evaluator::materializeReferenceValue(
    const api::ResolvedReference& rReference, api::ColumnIndex nColumnOffset,
    api::RowIndex nRowOffset)
{
    if (!rReference.isNormalized() || !rReference.containsOffset(nColumnOffset, nRowOffset))
        return makeFailure(api::Error::IllegalArgument);

    return evaluateCell(rReference.addressAt(nColumnOffset, nRowOffset));
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
            return makeScalarResult(api::CellValue::text({}));

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

    if (aFunctionName == u"ISERROR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(true));
        return makeScalarResult(api::CellValue::boolean(aArgument.maValue.maValue.isError()));
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

    if (aFunctionName == u"AGGREGATE")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aFunctionCode
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aFunctionCode)
            return aFunctionCode;

        EvaluationResult aOptionCode
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aOptionCode)
            return aOptionCode;

        const auto aFunctionNumber = coerceToNumber(aFunctionCode.maValue.maValue);
        if (!aFunctionNumber)
            return makeFailure(aFunctionNumber.meError);
        const auto aOptionNumber = coerceToNumber(aOptionCode.maValue.maValue);
        if (!aOptionNumber)
            return makeFailure(aOptionNumber.meError);

        const auto oFunction = toWholeNumber(aFunctionNumber.maValue);
        const auto oOption = toWholeNumber(aOptionNumber.maValue);
        if (!oFunction || !oOption || *oFunction < 1 || *oFunction > 11)
            return makeFailure(api::Error::IllegalArgument);

        const auto oOptions = decodeAggregateOptions(*oOption);
        if (!oOptions)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aReference = evaluateReferenceNode(*rNode.maChildren[2], rCurrentAddress);
        if (!aReference)
            return aReference;
        if (!aReference.maValue.isMatrixReference())
            return makeFailure(api::Error::IllegalArgument);

        const auto& rReference = aReference.maValue.maReference;
        const workbook::Sheet* pSheet = getSheet(rReference.maRange.maStart.mnSheet);
        if (!pSheet)
            return makeFailure(api::Error::IllegalArgument);

        AggregateScan aScan;
        for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
        {
            for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
            {
                const api::CellAddress aAddress = rReference.addressAt(nCol, nRow);
                if (oOptions->mbIgnoreHiddenRows && pSheet->isRowHidden(aAddress.mnRow))
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
                    return aCell;
                }

                if (!aCell.maValue.isScalar())
                    return makeFailure(api::Error::IllegalArgument);

                const api::CellValue& rValue = aCell.maValue.maValue;
                if (rValue.isError())
                {
                    if (oOptions->mbIgnoreErrors)
                        continue;
                    if (*oFunction == 2)
                        continue;
                    if (*oFunction == 3)
                    {
                        ++aScan.mnNonEmptyCount;
                        continue;
                    }
                    return makeFailure(rValue.meError);
                }

                if (!rValue.isEmpty())
                    ++aScan.mnNonEmptyCount;

                if (rValue.isNumber())
                    aScan.maNumbers.push_back(rValue.mfNumber);
            }
        }

        const auto aAggregate = evaluateAggregateNumbers(*oFunction, aScan);
        if (!aAggregate)
            return makeFailure(aAggregate.meError);
        return makeScalarResult(api::CellValue::number(aAggregate.maValue));
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
            const auto aRange = resolveNamedRange(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            if (aRange.maValue.isSingleCell())
                return materializeReferenceValue(aRange.maValue, 0, 0);
            return makeReferenceResult(aRange.maValue);
        }
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

EvaluationResult Evaluator::evaluateCell(const api::CellAddress& rAddress)
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

    CacheEntry& rEntry = maCellCache[makeAddressKey(rAddress)];
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

    EvaluationResult aResult = evaluateFormula(pCell->maFormula, rAddress);
    if (aResult && aResult.maValue.isMatrixReference())
    {
        if (aResult.maValue.maReference.isSingleCell())
            aResult = materializeReferenceValue(aResult.maValue.maReference, 0, 0);
        else
            aResult = makeFailure(api::Error::IllegalArgument);
    }

    if (!aResult && aResult.maCyclePath.empty() && hasCachedFallbackValue(*pCell))
    {
        if (const auto oTypedValue = parseTypedStoredCellValue(*pCell))
            return finalize(makeScalarResult(*oTypedValue, true));
        return finalize(makeScalarResult(pCell->maValue, true));
    }

    return finalize(aResult);
}

} // namespace spreadsheetengine::core::fods

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
