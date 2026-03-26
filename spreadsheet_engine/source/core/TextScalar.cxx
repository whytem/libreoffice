/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/TextScalar.hxx>

#include <cmath>
#include <optional>

#include <rtl/math.hxx>
#include <unicode/uchar.h>

namespace spreadsheetengine::core::text
{

namespace
{

bool isHighSurrogate(char16_t cChar) { return 0xD800 <= cChar && cChar <= 0xDBFF; }
bool isLowSurrogate(char16_t cChar) { return 0xDC00 <= cChar && cChar <= 0xDFFF; }

char32_t iterateCodePoint(spreadsheetengine::api::StringView rInput, std::size_t& rIndex)
{
    const char16_t cLead = rInput[rIndex++];
    if (isHighSurrogate(cLead) && rIndex < rInput.size() && isLowSurrogate(rInput[rIndex]))
    {
        const char32_t nTrail = rInput[rIndex++] - 0xDC00;
        return ((static_cast<char32_t>(cLead) - 0xD800) << 10) + nTrail + 0x10000;
    }
    return cLead;
}

void appendCodePoint(spreadsheetengine::api::String& rOutput, char32_t nCodePoint)
{
    if (nCodePoint <= 0xFFFF)
        rOutput.push_back(static_cast<char16_t>(nCodePoint));
    else
    {
        nCodePoint -= 0x10000;
        rOutput.push_back(static_cast<char16_t>(0xD800 + (nCodePoint >> 10)));
        rOutput.push_back(static_cast<char16_t>(0xDC00 + (nCodePoint & 0x3FF)));
    }
}

bool isUnicodeScalarValue(char32_t nCodePoint)
{
    return nCodePoint <= 0x10FFFF && !(0xD800 <= nCodePoint && nCodePoint <= 0xDFFF);
}

bool isPrintableCodePoint(char32_t nCodePoint)
{
    if (!isUnicodeScalarValue(nCodePoint))
        return false;
    return !u_isISOControl(nCodePoint) && u_isdefined(nCodePoint);
}

bool containsChar(spreadsheetengine::api::StringView rInput, char16_t cNeedle)
{
    return rInput.find(cNeedle) != spreadsheetengine::api::StringView::npos;
}

void removeChars(spreadsheetengine::api::String& rInput, spreadsheetengine::api::StringView rChars)
{
    spreadsheetengine::api::String aResult;
    aResult.reserve(rInput.size());
    for (char16_t c : rInput)
    {
        if (!containsChar(rChars, c))
            aResult.push_back(c);
    }
    rInput.swap(aResult);
}

}

spreadsheetengine::api::String trimRepeatedSpaces(spreadsheetengine::api::StringView rInput)
{
    const std::size_t nLength = rInput.size();
    std::size_t nStart = 0;
    while (nStart < nLength && rInput[nStart] == u' ')
        ++nStart;

    std::size_t nEnd = nLength;
    while (nEnd > nStart && rInput[nEnd - 1] == u' ')
        --nEnd;

    spreadsheetengine::api::String aBuffer;
    bool bPreviousWasSpace = false;
    for (std::size_t i = nStart; i < nEnd; ++i)
    {
        const char16_t cChar = rInput[i];
        if (cChar == u' ')
        {
            if (!bPreviousWasSpace)
                aBuffer.push_back(cChar);
            bPreviousWasSpace = true;
        }
        else
        {
            aBuffer.push_back(cChar);
            bPreviousWasSpace = false;
        }
    }

    return aBuffer;
}

sal_Int32 countCodePoints(spreadsheetengine::api::StringView rInput)
{
    std::size_t nIndex = 0;
    sal_Int32 nCount = 0;
    while (nIndex < rInput.size())
    {
        iterateCodePoint(rInput, nIndex);
        ++nCount;
    }
    return nCount;
}

NumberValueResult parseNumberValue(spreadsheetengine::api::StringView rInput,
    const std::optional<spreadsheetengine::api::String>& roDecimalSeparator,
    const std::optional<spreadsheetengine::api::String>& roGroupSeparator, bool bEmptyStringAsZero)
{
    NumberValueResult aResult;

    char16_t cDecimalSeparator = 0;
    if (roDecimalSeparator)
    {
        if (roDecimalSeparator->size() != 1)
        {
            aResult.meStatus = NumberValueStatus::IllegalArgument;
            return aResult;
        }
        cDecimalSeparator = (*roDecimalSeparator)[0];
    }

    const auto aGroupSeparator = roGroupSeparator.value_or(spreadsheetengine::api::String());
    if (cDecimalSeparator && containsChar(aGroupSeparator, cDecimalSeparator))
    {
        aResult.meStatus = NumberValueStatus::IllegalArgument;
        return aResult;
    }

    if (rInput.empty())
    {
        aResult.meStatus = bEmptyStringAsZero ? NumberValueStatus::Ok : NumberValueStatus::NoValue;
        aResult.mfValue = 0.0;
        return aResult;
    }

    spreadsheetengine::api::String aInputString(rInput);
    const auto nDecSepPos = cDecimalSeparator ? aInputString.find(cDecimalSeparator)
                                              : spreadsheetengine::api::String::npos;
    if (nDecSepPos != 0)
    {
        spreadsheetengine::api::String aTemporary(
            nDecSepPos != spreadsheetengine::api::String::npos ? aInputString.substr(0, nDecSepPos)
                                                               : aInputString);
        removeChars(aTemporary, aGroupSeparator);
        if (nDecSepPos != spreadsheetengine::api::String::npos)
            aInputString = aTemporary + aInputString.substr(nDecSepPos);
        else
            aInputString = aTemporary;
    }

    removeChars(aInputString, u" \t\n\r");

    sal_Int32 nPercentCount = 0;
    while (!aInputString.empty() && aInputString.back() == u'%')
    {
        aInputString.pop_back();
        ++nPercentCount;
    }

    rtl_math_ConversionStatus eStatus = rtl_math_ConversionStatus_Ok;
    sal_Int32 nParseEnd = 0;
    double fValue = rtl::math::stringToDouble(
        aInputString, cDecimalSeparator, 0, &eStatus, &nParseEnd);
    if (eStatus == rtl_math_ConversionStatus_Ok
        && nParseEnd == static_cast<sal_Int32>(aInputString.size()))
    {
        if (nPercentCount)
            fValue *= std::pow(10.0, -(nPercentCount * 2));
        aResult.meStatus = NumberValueStatus::Ok;
        aResult.mfValue = fValue;
        return aResult;
    }

    aResult.meStatus = NumberValueStatus::NoValue;
    return aResult;
}

spreadsheetengine::api::String cleanPrintable(spreadsheetengine::api::StringView rInput)
{
    spreadsheetengine::api::String aBuffer;
    std::size_t nIndex = 0;
    while (nIndex < rInput.size())
    {
        const char32_t nCodePoint = iterateCodePoint(rInput, nIndex);
        if (isPrintableCodePoint(nCodePoint))
            appendCodePoint(aBuffer, nCodePoint);
    }
    return aBuffer;
}

sal_Int32 codeFromText(
    const SingleByteEncodingService& rEncodingService, spreadsheetengine::api::StringView rInput)
{
    if (rInput.empty())
        return 0;
    return rEncodingService.encodeFirstCharacter(rInput);
}

std::optional<spreadsheetengine::api::String> charFromValue(
    const SingleByteEncodingService& rEncodingService, double fValue)
{
    if (fValue < 0.0 || fValue >= 256.0)
        return std::nullopt;
    return rEncodingService.decodeSingleByte(static_cast<unsigned char>(fValue));
}

std::optional<double> unicodeFromText(spreadsheetengine::api::StringView rInput)
{
    if (rInput.empty())
        return std::nullopt;

    std::size_t nIndex = 0;
    return static_cast<double>(iterateCodePoint(rInput, nIndex));
}

std::optional<spreadsheetengine::api::String> unicharFromCodePoint(sal_uInt32 nCodePoint)
{
    if (!isUnicodeScalarValue(nCodePoint))
        return std::nullopt;

    spreadsheetengine::api::String aResult;
    appendCodePoint(aResult, nCodePoint);
    return aResult;
}

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
