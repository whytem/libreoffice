/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/NumeralConversion.hxx>
#include <cstdint>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

#include <spreadsheetengine/runtime/FloatingPoint.hxx>

namespace spreadsheetengine::core::convert
{

namespace
{

constexpr char16_t DIGITS[] = u"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr int DIGIT_COUNT = static_cast<int>((sizeof(DIGITS) / sizeof(DIGITS[0])) - 1);

char16_t asciiToUpper(char16_t cChar)
{
    if (cChar >= u'a' && cChar <= u'z')
        return cChar - (u'a' - u'A');
    return cChar;
}

bool getArabicValue(char16_t cChar, std::uint16_t& rnValue, bool& rbIsDec)
{
    switch (asciiToUpper(cChar))
    {
        case u'M':
            rnValue = 1000;
            rbIsDec = true;
            break;
        case u'D':
            rnValue = 500;
            rbIsDec = false;
            break;
        case u'C':
            rnValue = 100;
            rbIsDec = true;
            break;
        case u'L':
            rnValue = 50;
            rbIsDec = false;
            break;
        case u'X':
            rnValue = 10;
            rbIsDec = true;
            break;
        case u'V':
            rnValue = 5;
            rbIsDec = false;
            break;
        case u'I':
            rnValue = 1;
            rbIsDec = true;
            break;
        default:
            return false;
    }
    return true;
}

bool isIntegralAndAtMostUInt64(double fValue)
{
    return fValue >= 0.0
           && fValue <= static_cast<double>(std::numeric_limits<std::uint64_t>::max())
           && fp::approxEqual(fValue, fp::approxFloor(fValue));
}

}

NumeralStringResult convertToBase(double fValue, double fBase, std::optional<double> ofMinLength)
{
    NumeralStringResult aResult;

    std::int32_t nMinLen = 1;
    if (ofMinLength)
    {
        const double fLen = fp::approxFloor(*ofMinLength);
        if (1.0 <= fLen && fLen < static_cast<double>(std::numeric_limits<std::uint16_t>::max()))
            nMinLen = static_cast<std::int32_t>(fLen);
        else
            nMinLen = (fLen == 0.0) ? 1 : 0;
    }

    fBase = fp::approxFloor(fBase);
    fValue = fp::approxFloor(fValue);

    const double fChars = (fValue > 0.0 && fBase > 0.0 && fBase != 1.0)
                              ? (std::ceil(std::log(fValue) / std::log(fBase)) + 2.0)
                              : 2.0;
    if (fChars >= static_cast<double>(std::numeric_limits<std::uint16_t>::max()))
        nMinLen = 0;

    if (!(nMinLen && 2.0 <= fBase && fBase <= DIGIT_COUNT && 0.0 <= fValue))
    {
        aResult.meError = NumeralStringError::IllegalArgument;
        return aResult;
    }

    spreadsheetengine::api::String aDigits;
    if (isIntegralAndAtMostUInt64(fValue))
    {
        std::uint64_t nValue = static_cast<std::uint64_t>(fValue);
        const std::uint64_t nBase = static_cast<std::uint64_t>(fBase);
        do
        {
            aDigits.push_back(DIGITS[nValue % nBase]);
            nValue /= nBase;
        } while (nValue != 0);
    }
    else
    {
        bool bDirt = false;
        do
        {
            const double fInt = fp::approxFloor(fValue / fBase);
            const double fMult = fInt * fBase;
            std::size_t nDigit = 0;
            if (fValue < fMult)
            {
                bDirt = true;
            }
            else
            {
                double fDigit
                    = fp::approxFloor(fp::approxSub(fValue, fMult));
                if (bDirt)
                {
                    bDirt = false;
                    --fDigit;
                }
                if (fDigit > 0.0)
                {
                    if (fDigit >= fBase)
                        nDigit = static_cast<std::size_t>(fBase) - 1;
                    else
                        nDigit = static_cast<std::size_t>(fDigit);
                }
            }
            aDigits.push_back(DIGITS[nDigit]);
            fValue = fInt;
        } while (fValue != 0.0);
    }

    std::reverse(aDigits.begin(), aDigits.end());
    if (aDigits.size() < static_cast<std::size_t>(nMinLen))
    {
        aResult.maValue.assign(static_cast<std::size_t>(nMinLen) - aDigits.size(), u'0');
        aResult.maValue += aDigits;
    }
    else
        aResult.maValue = std::move(aDigits);

    return aResult;
}

std::optional<double> convertFromBase(spreadsheetengine::api::StringView rText, double fBase)
{
    fBase = fp::approxFloor(fBase);
    if (!(2.0 <= fBase && fBase <= 36.0))
        return std::nullopt;

    double fValue = 0.0;
    const int nBase = static_cast<int>(fBase);
    std::size_t nIndex = 0;
    while (nIndex < rText.size() && (rText[nIndex] == u' ' || rText[nIndex] == u'\t'))
        ++nIndex;

    if (nBase == 16)
    {
        if (nIndex < rText.size() && (rText[nIndex] == u'x' || rText[nIndex] == u'X'))
            ++nIndex;
        else if (nIndex + 1 < rText.size() && rText[nIndex] == u'0'
                 && (rText[nIndex + 1] == u'x' || rText[nIndex + 1] == u'X'))
        {
            nIndex += 2;
        }
    }

    for (; nIndex < rText.size(); ++nIndex)
    {
        const char16_t cChar = rText[nIndex];
        int nDigit;
        if (u'0' <= cChar && cChar <= u'9')
            nDigit = cChar - u'0';
        else if (u'A' <= cChar && cChar <= u'Z')
            nDigit = 10 + (cChar - u'A');
        else if (u'a' <= cChar && cChar <= u'z')
            nDigit = 10 + (cChar - u'a');
        else
            nDigit = nBase;

        if (nBase <= nDigit)
        {
            const bool bHasTrailingBaseSuffix
                = (nIndex + 1 == rText.size())
                  && ((nBase == 2 && (cChar == u'b' || cChar == u'B'))
                      || (nBase == 16 && (cChar == u'h' || cChar == u'H')));
            if (bHasTrailingBaseSuffix)
                break;
            return std::nullopt;
        }

        fValue = fValue * fBase + nDigit;
    }

    return fValue;
}

std::optional<spreadsheetengine::api::String> convertToRoman(
    double fValue, std::optional<double> ofMode)
{
    const double fNormalizedMode = ofMode ? fp::approxFloor(*ofMode) : 0.0;
    const double fNormalizedValue = fp::approxFloor(fValue);

    if (!(fNormalizedMode >= 0.0 && fNormalizedMode < 5.0 && fNormalizedValue >= 0.0
          && fNormalizedValue < 4000.0))
    {
        return std::nullopt;
    }

    static const char16_t pChars[] = { u'M', u'D', u'C', u'L', u'X', u'V', u'I' };
    static const std::uint16_t pValues[] = { 1000, 500, 100, 50, 10, 5, 1 };
    static const std::uint16_t nMaxIndex = static_cast<std::uint16_t>((sizeof(pValues) / sizeof(pValues[0])) - 1);

    spreadsheetengine::api::String aRoman;
    std::uint16_t nValue = static_cast<std::uint16_t>(fNormalizedValue);
    const std::uint16_t nMode = static_cast<std::uint16_t>(fNormalizedMode);

    for (std::uint16_t i = 0; i <= nMaxIndex / 2; ++i)
    {
        std::uint16_t nIndex = 2 * i;
        const std::uint16_t nDigit = nValue / pValues[nIndex];

        if ((nDigit % 5) == 4)
        {
            std::uint16_t nIndex2 = (nDigit == 4) ? nIndex - 1 : nIndex - 2;
            std::uint16_t nSteps = 0;
            while ((nSteps < nMode) && (nIndex < nMaxIndex))
            {
                ++nSteps;
                if (pValues[nIndex2] - pValues[nIndex + 1] <= nValue)
                    ++nIndex;
                else
                    nSteps = nMode;
            }
            aRoman.push_back(pChars[nIndex]);
            aRoman.push_back(pChars[nIndex2]);
            nValue = static_cast<std::uint16_t>(nValue + pValues[nIndex]);
            nValue = static_cast<std::uint16_t>(nValue - pValues[nIndex2]);
        }
        else
        {
            if (nDigit > 4)
                aRoman.push_back(pChars[nIndex - 1]);

            std::int32_t nPad = nDigit % 5;
            while (nPad-- > 0)
                aRoman.push_back(pChars[nIndex]);

            nValue %= pValues[nIndex];
        }
    }

    return aRoman;
}

std::optional<std::int32_t> convertFromRoman(spreadsheetengine::api::StringView rRoman)
{
    std::uint16_t nValue = 0;
    std::uint16_t nValidRest = 3999;
    std::size_t nCharIndex = 0;
    const std::size_t nCharCount = rRoman.size();
    bool bValid = true;

    while (bValid && (nCharIndex < nCharCount))
    {
        std::uint16_t nDigit1 = 0;
        std::uint16_t nDigit2 = 0;
        bool bIsDec1 = false;
        bValid = getArabicValue(rRoman[nCharIndex], nDigit1, bIsDec1);
        if (bValid && (nCharIndex + 1 < nCharCount))
        {
            bool bIsDec2 = false;
            bValid = getArabicValue(rRoman[nCharIndex + 1], nDigit2, bIsDec2);
        }
        if (bValid)
        {
            if (nDigit1 >= nDigit2)
            {
                nValue = static_cast<std::uint16_t>(nValue + nDigit1);
                nValidRest %= (nDigit1 * (bIsDec1 ? 5 : 2));
                bValid = (nValidRest >= nDigit1);
                if (bValid)
                    nValidRest = static_cast<std::uint16_t>(nValidRest - nDigit1);
                ++nCharIndex;
            }
            else if (nDigit1 * 2 != nDigit2)
            {
                const std::uint16_t nDiff = nDigit2 - nDigit1;
                nValue = static_cast<std::uint16_t>(nValue + nDiff);
                bValid = (nValidRest >= nDiff);
                if (bValid)
                    nValidRest = nDigit1 - 1;
                nCharIndex += 2;
            }
            else
                bValid = false;
        }
    }

    if (!bValid)
        return std::nullopt;

    return static_cast<std::int32_t>(nValue);
}

} // namespace spreadsheetengine::core::convert

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
