/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/DateTimeParse.hxx>
#include <cstdint>

#include <algorithm>
#include <cmath>
#include <limits>

#include <spreadsheetengine/runtime/FloatingPoint.hxx>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>

#include "CoreRuntimeUtils.hxx"
#include "DateAlgorithms.hxx"

namespace spreadsheetengine::core::datetime
{
namespace
{

using spreadsheetengine::core::util::uppercaseAscii;

[[nodiscard]] std::optional<double> parseAsciiDouble(spreadsheetengine::api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    bool bHasDigit = false;
    bool bHasDecimal = false;
    bool bHasExponent = false;
    std::size_t nPos = 0;

    if (rValue[nPos] == u'+' || rValue[nPos] == u'-')
        ++nPos;

    for (; nPos < rValue.size(); ++nPos)
    {
        const char16_t cChar = rValue[nPos];
        if (cChar >= u'0' && cChar <= u'9')
        {
            bHasDigit = true;
            continue;
        }

        if (cChar == u'.' && !bHasDecimal && !bHasExponent)
        {
            bHasDecimal = true;
            continue;
        }

        if ((cChar == u'e' || cChar == u'E') && bHasDigit && !bHasExponent
            && nPos + 1 < rValue.size())
        {
            bHasExponent = true;
            if (rValue[nPos + 1] == u'+' || rValue[nPos + 1] == u'-')
                ++nPos;
            bHasDigit = false;
            continue;
        }

        return std::nullopt;
    }

    if (!bHasDigit)
        return std::nullopt;

    fp::ConversionStatus eStatus = fp::ConversionStatus::Ok;
    std::int32_t nEnd = 0;
    const double fValue = fp::stringToDouble(rValue, u'.', 0, &eStatus, &nEnd);
    if (eStatus != fp::ConversionStatus::Ok
        || nEnd != static_cast<std::int32_t>(rValue.size()))
    {
        return std::nullopt;
    }

    return fValue;
}

[[nodiscard]] spreadsheetengine::api::StringView trimAsciiWhitespace(
    spreadsheetengine::api::StringView rValue)
{
    while (!rValue.empty()
           && (rValue.front() == u' ' || rValue.front() == u'\t' || rValue.front() == u'\n'
               || rValue.front() == u'\r'))
    {
        rValue.remove_prefix(1);
    }

    while (!rValue.empty()
           && (rValue.back() == u' ' || rValue.back() == u'\t' || rValue.back() == u'\n'
               || rValue.back() == u'\r'))
    {
        rValue.remove_suffix(1);
    }

    return rValue;
}

[[nodiscard]] std::optional<std::int16_t> parseAsciiInt16(spreadsheetengine::api::StringView rValue)
{
    const auto oDouble = parseAsciiDouble(rValue);
    if (!oDouble || !std::isfinite(*oDouble) || fp::approxFloor(*oDouble) != *oDouble
        || *oDouble < static_cast<double>(std::numeric_limits<std::int16_t>::min())
        || *oDouble > static_cast<double>(std::numeric_limits<std::int16_t>::max()))
    {
        return std::nullopt;
    }

    return static_cast<std::int16_t>(*oDouble);
}

[[nodiscard]] std::optional<std::int16_t> parseMonthName(
    spreadsheetengine::api::StringView rValue)
{
    const auto aUpper = uppercaseAscii(rValue);
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

[[nodiscard]] bool splitThreePartNumericDate(spreadsheetengine::api::StringView rValue,
    char16_t cSeparator, std::int16_t& rnFirst, std::int16_t& rnSecond, std::int16_t& rnThird)
{
    const std::size_t nFirstSep = rValue.find(cSeparator);
    if (nFirstSep == spreadsheetengine::api::StringView::npos)
        return false;
    const std::size_t nSecondSep = rValue.find(cSeparator, nFirstSep + 1);
    if (nSecondSep == spreadsheetengine::api::StringView::npos)
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

[[nodiscard]] bool parseDateText(spreadsheetengine::api::StringView rValue, std::int16_t& rnYear,
    std::int16_t& rnMonth, std::int16_t& rnDay)
{
    rValue = trimAsciiWhitespace(rValue);
    if (rValue.empty())
        return false;

    std::int16_t nFirst = 0;
    std::int16_t nSecond = 0;
    std::int16_t nThird = 0;
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

    spreadsheetengine::api::StringView aTail = trimAsciiWhitespace(rValue.substr(nMonthEnd));
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

[[nodiscard]] std::optional<double> parseTimeText(spreadsheetengine::api::StringView rValue)
{
    rValue = trimAsciiWhitespace(rValue);
    if (rValue.empty())
        return std::nullopt;

    bool bHasMeridiem = false;
    bool bPM = false;
    if (rValue.size() >= 2)
    {
        const spreadsheetengine::api::String aSuffix
            = uppercaseAscii(rValue.substr(rValue.size() - 2));
        if (aSuffix == u"AM" || aSuffix == u"PM")
        {
            bHasMeridiem = true;
            bPM = aSuffix == u"PM";
            rValue = trimAsciiWhitespace(rValue.substr(0, rValue.size() - 2));
        }
    }

    const std::size_t nFirstColon = rValue.find(u':');
    if (!bHasMeridiem && nFirstColon == spreadsheetengine::api::StringView::npos)
        return std::nullopt;

    std::int16_t nHour = 0;
    std::int16_t nMinute = 0;
    std::int16_t nSecond = 0;
    if (nFirstColon == spreadsheetengine::api::StringView::npos)
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
        if (nSecondColon == spreadsheetengine::api::StringView::npos)
        {
            const auto oMinute = parseAsciiInt16(rValue.substr(nFirstColon + 1));
            if (!oMinute)
                return std::nullopt;
            nMinute = *oMinute;
        }
        else
        {
            const auto oMinute = parseAsciiInt16(
                rValue.substr(nFirstColon + 1, nSecondColon - nFirstColon - 1));
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
            nHour = nHour == 12 ? 12 : static_cast<std::int16_t>(nHour + 12);
        else
            nHour = nHour == 12 ? 0 : nHour;
    }

    const auto aTimeSerial = spreadsheetengine::api::calendar::makeTimeSerial(
        nHour, nMinute, nSecond);
    if (!aTimeSerial)
        return std::nullopt;
    return aTimeSerial.maValue;
}

}

spreadsheetengine::api::DateParts defaultNullDate() { return { 1899, 12, 30 }; }

std::optional<double> parseOdfTimeDuration(spreadsheetengine::api::StringView rValue)
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

    const auto aTimeSerial
        = spreadsheetengine::api::calendar::makeTimeSerial(fHour, fMinute, fSecond);
    if (!aTimeSerial)
        return std::nullopt;
    return aTimeSerial.maValue;
}

std::optional<spreadsheetengine::api::NumberParseResult> parseStandaloneNumberText(
    spreadsheetengine::api::StringView rValue)
{
    const spreadsheetengine::api::DateParts aDefaultNullDate = defaultNullDate();

    const spreadsheetengine::api::StringView aTrimmed = trimAsciiWhitespace(rValue);
    if (aTrimmed.empty())
        return std::nullopt;

    if (const auto oNumber = parseAsciiDouble(aTrimmed))
        return spreadsheetengine::api::NumberParseResult {
            *oNumber, 0, spreadsheetengine::api::NumberParseResult::Kind::Number
        };

    if (const auto oTime = parseTimeText(aTrimmed))
        return spreadsheetengine::api::NumberParseResult {
            *oTime, 0, spreadsheetengine::api::NumberParseResult::Kind::Time
        };

    std::int16_t nYear = 0;
    std::int16_t nMonth = 0;
    std::int16_t nDay = 0;
    if (parseDateText(aTrimmed, nYear, nMonth, nDay))
    {
        const auto aDateSerial = spreadsheetengine::api::calendar::makeDateSerial(
            aDefaultNullDate, nYear, nMonth, nDay, true);
        if (!aDateSerial)
            return std::nullopt;

        return spreadsheetengine::api::NumberParseResult {
            aDateSerial.maValue, 0, spreadsheetengine::api::NumberParseResult::Kind::Date
        };
    }

    const std::size_t nSplitPos = aTrimmed.find_last_of(u' ');
    if (nSplitPos == spreadsheetengine::api::StringView::npos)
        return std::nullopt;
    const spreadsheetengine::api::StringView aDatePart
        = trimAsciiWhitespace(aTrimmed.substr(0, nSplitPos));
    const spreadsheetengine::api::StringView aTimePart
        = trimAsciiWhitespace(aTrimmed.substr(nSplitPos + 1));
    if (!parseDateText(aDatePart, nYear, nMonth, nDay) || aTimePart.empty())
        return std::nullopt;

    const auto aDateSerial = spreadsheetengine::api::calendar::makeDateSerial(
        aDefaultNullDate, nYear, nMonth, nDay, true);
    if (!aDateSerial)
        return std::nullopt;

    const auto oTimeSerial = parseTimeText(aTimePart);
    if (!oTimeSerial)
        return std::nullopt;

    return spreadsheetengine::api::NumberParseResult {
        aDateSerial.maValue + *oTimeSerial, 0,
        spreadsheetengine::api::NumberParseResult::Kind::DateTime
    };
}

std::optional<double> parseStoredDateValue(spreadsheetengine::api::StringView rValue)
{
    const spreadsheetengine::api::DateParts aDefaultNullDate = defaultNullDate();

    rValue = trimAsciiWhitespace(rValue);
    if (rValue.empty())
        return std::nullopt;

    spreadsheetengine::api::StringView aDatePart = rValue;
    spreadsheetengine::api::StringView aTimePart;
    const std::size_t nTimeSeparator = rValue.find_first_of(u"T ");
    if (nTimeSeparator != spreadsheetengine::api::StringView::npos)
    {
        aDatePart = trimAsciiWhitespace(rValue.substr(0, nTimeSeparator));
        aTimePart = trimAsciiWhitespace(rValue.substr(nTimeSeparator + 1));
    }

    std::int16_t nYear = 0;
    std::int16_t nMonth = 0;
    std::int16_t nDay = 0;
    if (!parseDateText(aDatePart, nYear, nMonth, nDay))
        return std::nullopt;
    if (!detail::date::isValidDate(
            static_cast<std::uint16_t>(nDay), static_cast<std::uint16_t>(nMonth), nYear))
    {
        return std::nullopt;
    }

    const spreadsheetengine::api::DateParts aDate { nYear, nMonth, nDay };
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

std::optional<spreadsheetengine::api::DateSerial> coerceToDateSerial(
    const spreadsheetengine::api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case spreadsheetengine::api::CellValueKind::Empty:
            return static_cast<spreadsheetengine::api::DateSerial>(0);
        case spreadsheetengine::api::CellValueKind::Number:
        case spreadsheetengine::api::CellValueKind::Boolean:
            return static_cast<spreadsheetengine::api::DateSerial>(
                fp::approxFloor(rValue.mfNumber));
        case spreadsheetengine::api::CellValueKind::Text:
        {
            const auto oParsed = parseStandaloneNumberText(rValue.maString);
            if (!oParsed)
                return std::nullopt;
            return static_cast<spreadsheetengine::api::DateSerial>(
                fp::approxFloor(oParsed->mfValue));
        }
        case spreadsheetengine::api::CellValueKind::Error:
            return std::nullopt;
    }

    return std::nullopt;
}

std::optional<double> shiftMonthSerial(
    spreadsheetengine::api::DateSerial nDateSerial, std::int32_t nMonthOffset, bool bEndOfMonth)
{
    const spreadsheetengine::api::DateParts aNullDate = defaultNullDate();

    const std::int16_t nYear
        = static_cast<std::int16_t>(extractYear(aNullDate, nDateSerial));
    const std::int16_t nMonth
        = static_cast<std::int16_t>(extractMonth(aNullDate, nDateSerial));
    const auto aDayResult = spreadsheetengine::api::calendar::dayFromSerial(aNullDate, nDateSerial);
    if (!aDayResult)
        return std::nullopt;

    const std::int32_t nZeroBasedMonth
        = static_cast<std::int32_t>(nYear) * 12 + static_cast<std::int32_t>(nMonth - 1) + nMonthOffset;
    if (nZeroBasedMonth < 12)
        return std::nullopt;

    const std::int16_t nTargetYear = static_cast<std::int16_t>(nZeroBasedMonth / 12);
    const std::int16_t nTargetMonth = static_cast<std::int16_t>((nZeroBasedMonth % 12) + 1);
    const std::uint16_t nDaysInTargetMonth = detail::date::getDaysInMonth(
        static_cast<std::uint16_t>(nTargetMonth), nTargetYear);
    const std::int16_t nTargetDay = bEndOfMonth
                                     ? static_cast<std::int16_t>(nDaysInTargetMonth)
                                     : static_cast<std::int16_t>(std::min<double>(
                                           aDayResult.maValue, nDaysInTargetMonth));

    const auto aShifted = spreadsheetengine::api::calendar::makeDateSerial(
        aNullDate, nTargetYear, nTargetMonth, nTargetDay, true);
    if (!aShifted)
        return std::nullopt;
    return aShifted.maValue;
}

std::optional<double> computeWeeksDifference(
    spreadsheetengine::api::DateSerial nStartDate, spreadsheetengine::api::DateSerial nEndDate,
    std::int16_t nMode)
{
    if (nMode == 0)
        return static_cast<double>((nEndDate - nStartDate) / 7);

    if (nMode != 1)
        return std::nullopt;

    constexpr spreadsheetengine::api::DateParts aEpoch { 1, 1, 1 };
    const spreadsheetengine::api::DateParts aNullDate = defaultNullDate();
    const auto aOffset = spreadsheetengine::api::calendar::makeDateSerial(
        aEpoch, aNullDate.mnYear, aNullDate.mnMonth, aNullDate.mnDay, true);
    if (!aOffset)
        return std::nullopt;

    const double fStartWeek = std::floor((nStartDate + aOffset.maValue) / 7.0);
    const double fEndWeek = std::floor((nEndDate + aOffset.maValue) / 7.0);
    return fEndWeek - fStartWeek;
}

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
