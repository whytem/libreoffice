/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/TextScalar.hxx>

#include <cmath>
#include <optional>

#include <osl/thread.h>
#include <rtl/character.hxx>
#include <rtl/math.hxx>
#include <rtl/textenc.h>
#include <rtl/ustrbuf.hxx>
#include <unicode/uchar.h>

namespace spreadsheetengine::core::text
{

namespace
{

bool isPrintableCodePoint(sal_uInt32 nCodePoint)
{
    return !u_isISOControl(nCodePoint) && u_isdefined(nCodePoint);
}

}

OUString trimRepeatedSpaces(const OUString& rInput)
{
    const sal_Int32 nLength = rInput.getLength();
    sal_Int32 nStart = 0;
    while (nStart < nLength && rInput[nStart] == ' ')
        ++nStart;

    sal_Int32 nEnd = nLength;
    while (nEnd > nStart && rInput[nEnd - 1] == ' ')
        --nEnd;

    OUStringBuffer aBuffer;
    bool bPreviousWasSpace = false;
    for (sal_Int32 i = nStart; i < nEnd; ++i)
    {
        const sal_Unicode c = rInput[i];
        if (c == ' ')
        {
            if (!bPreviousWasSpace)
                aBuffer.append(c);
            bPreviousWasSpace = true;
        }
        else
        {
            aBuffer.append(c);
            bPreviousWasSpace = false;
        }
    }

    return aBuffer.makeStringAndClear();
}

sal_Int32 countCodePoints(const OUString& rInput)
{
    sal_Int32 nIndex = 0;
    sal_Int32 nCount = 0;
    while (nIndex < rInput.getLength())
    {
        rInput.iterateCodePoints(&nIndex);
        ++nCount;
    }
    return nCount;
}

NumberValueResult parseNumberValue(
    const OUString& rInput, const std::optional<OUString>& roDecimalSeparator,
    const std::optional<OUString>& roGroupSeparator, bool bEmptyStringAsZero)
{
    NumberValueResult aResult;

    sal_Unicode cDecimalSeparator = 0;
    if (roDecimalSeparator)
    {
        if (roDecimalSeparator->getLength() != 1)
        {
            aResult.meStatus = NumberValueStatus::IllegalArgument;
            return aResult;
        }
        cDecimalSeparator = (*roDecimalSeparator)[0];
    }

    const OUString aGroupSeparator = roGroupSeparator.value_or(OUString());
    if (cDecimalSeparator && aGroupSeparator.indexOf(cDecimalSeparator) != -1)
    {
        aResult.meStatus = NumberValueStatus::IllegalArgument;
        return aResult;
    }

    if (rInput.isEmpty())
    {
        aResult.meStatus = bEmptyStringAsZero ? NumberValueStatus::Ok : NumberValueStatus::NoValue;
        aResult.mfValue = 0.0;
        return aResult;
    }

    OUString aInputString(rInput);
    const sal_Int32 nDecSep = aInputString.indexOf(cDecimalSeparator);
    if (nDecSep != 0)
    {
        OUString aTemporary(nDecSep >= 0 ? aInputString.copy(0, nDecSep) : aInputString);
        sal_Int32 nIndex = 0;
        while (nIndex < aGroupSeparator.getLength())
        {
            sal_uInt32 nChar = aGroupSeparator.iterateCodePoints(&nIndex);
            aTemporary = aTemporary.replaceAll(OUString(&nChar, 1), u"");
        }
        if (nDecSep >= 0)
            aInputString = aTemporary + aInputString.subView(nDecSep);
        else
            aInputString = aTemporary;
    }

    for (sal_Int32 i = aInputString.getLength(); --i >= 0;)
    {
        const sal_Unicode c = aInputString[i];
        if (c == 0x0020 || c == 0x0009 || c == 0x000A || c == 0x000D)
            aInputString = aInputString.replaceAt(i, 1, u"");
    }

    sal_Int32 nPercentCount = 0;
    for (sal_Int32 i = aInputString.getLength() - 1; i >= 0 && aInputString[i] == 0x0025; --i)
    {
        aInputString = aInputString.replaceAt(i, 1, u"");
        ++nPercentCount;
    }

    rtl_math_ConversionStatus eStatus;
    sal_Int32 nParseEnd;
    double fValue
        = ::rtl::math::stringToDouble(aInputString, cDecimalSeparator, 0, &eStatus, &nParseEnd);
    if (eStatus == rtl_math_ConversionStatus_Ok && nParseEnd == aInputString.getLength())
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

OUString cleanPrintable(const OUString& rInput)
{
    OUStringBuffer aBuffer(rInput.getLength());
    sal_Int32 nIndex = 0;
    while (nIndex < rInput.getLength())
    {
        sal_uInt32 nCodePoint = rInput.iterateCodePoints(&nIndex);
        if (isPrintableCodePoint(nCodePoint))
            aBuffer.appendUtf32(nCodePoint);
    }
    return aBuffer.makeStringAndClear();
}

sal_Int32 codeFromText(const OUString& rInput)
{
    if (rInput.isEmpty())
        return 0;

    const sal_uInt32 nConvertFlags = RTL_UNICODETOTEXT_FLAGS_NONSPACING_IGNORE
                                     | RTL_UNICODETOTEXT_FLAGS_CONTROL_IGNORE
                                     | RTL_UNICODETOTEXT_FLAGS_FLUSH
                                     | RTL_UNICODETOTEXT_FLAGS_UNDEFINED_DEFAULT
                                     | RTL_UNICODETOTEXT_FLAGS_INVALID_DEFAULT
                                     | RTL_UNICODETOTEXT_FLAGS_UNDEFINED_REPLACE;
    return static_cast<unsigned char>(
        OUStringToOString(OUStringChar(rInput[0]), osl_getThreadTextEncoding(), nConvertFlags)
            .toChar());
}

std::optional<OUString> charFromValue(double fValue)
{
    if (fValue < 0.0 || fValue >= 256.0)
        return std::nullopt;

    const sal_uInt32 nConvertFlags = RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_DEFAULT
                                     | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_DEFAULT
                                     | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT;
    const char cEncodedChar = static_cast<char>(fValue);
    return OUString(&cEncodedChar, 1, osl_getThreadTextEncoding(), nConvertFlags);
}

std::optional<double> unicodeFromText(const OUString& rInput)
{
    if (rInput.isEmpty())
        return std::nullopt;

    sal_Int32 nIndex = 0;
    return static_cast<double>(rInput.iterateCodePoints(&nIndex));
}

std::optional<OUString> unicharFromCodePoint(sal_uInt32 nCodePoint)
{
    if (!rtl::isUnicodeCodePoint(nCodePoint))
        return std::nullopt;

    return OUString(&nCodePoint, 1);
}

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
