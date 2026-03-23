/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/NumeralConversion.hxx>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include <o3tl/float_int_conversion.hxx>
#include <rtl/math.hxx>

namespace spreadsheetengine::core::convert
{

namespace
{

constexpr sal_Unicode DIGITS[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0
};

constexpr int DIGIT_COUNT = SAL_N_ELEMENTS(DIGITS) - 1;

bool getArabicValue(sal_Unicode cChar, sal_uInt16& rnValue, bool& rbIsDec)
{
    switch (cChar)
    {
        case 'M':
            rnValue = 1000;
            rbIsDec = true;
            break;
        case 'D':
            rnValue = 500;
            rbIsDec = false;
            break;
        case 'C':
            rnValue = 100;
            rbIsDec = true;
            break;
        case 'L':
            rnValue = 50;
            rbIsDec = false;
            break;
        case 'X':
            rnValue = 10;
            rbIsDec = true;
            break;
        case 'V':
            rnValue = 5;
            rbIsDec = false;
            break;
        case 'I':
            rnValue = 1;
            rbIsDec = true;
            break;
        default:
            return false;
    }
    return true;
}

}

NumeralStringResult convertToBase(double fValue, double fBase, std::optional<double> ofMinLength)
{
    NumeralStringResult aResult;

    sal_Int32 nMinLen = 1;
    if (ofMinLength)
    {
        const double fLen = ::rtl::math::approxFloor(*ofMinLength);
        if (1.0 <= fLen && fLen < SAL_MAX_UINT16)
            nMinLen = static_cast<sal_Int32>(fLen);
        else
            nMinLen = (fLen == 0.0) ? 1 : 0;
    }

    fBase = ::rtl::math::approxFloor(fBase);
    fValue = ::rtl::math::approxFloor(fValue);

    const double fChars = (fValue > 0.0 && fBase > 0.0 && fBase != 1.0)
                              ? (std::ceil(std::log(fValue) / std::log(fBase)) + 2.0)
                              : 2.0;
    if (fChars >= SAL_MAX_UINT16)
        nMinLen = 0;

    if (!(nMinLen && 2.0 <= fBase && fBase <= DIGIT_COUNT && 0.0 <= fValue))
    {
        aResult.meError = NumeralStringError::IllegalArgument;
        return aResult;
    }

    const sal_Int32 nBuf = std::max<sal_Int32>(static_cast<sal_Int32>(fChars), nMinLen + 1);
    std::vector<sal_Unicode> aBuf(static_cast<std::size_t>(nBuf + 1), '0');
    aBuf[static_cast<std::size_t>(nBuf)] = 0;
    sal_Unicode* pBuf = aBuf.data();
    sal_Unicode* p = pBuf + nBuf;

    if (o3tl::convertsToAtMost(fValue, std::numeric_limits<sal_uInt64>::max()))
    {
        sal_uInt64 nVal = static_cast<sal_uInt64>(fValue);
        const sal_uInt64 nBase = static_cast<sal_uInt64>(fBase);
        while (nVal && p > pBuf)
        {
            *--p = DIGITS[nVal % nBase];
            nVal /= nBase;
        }
        fValue = static_cast<double>(nVal);
    }
    else
    {
        bool bDirt = false;
        while (fValue && p > pBuf)
        {
            const double fInt = ::rtl::math::approxFloor(fValue / fBase);
            const double fMult = fInt * fBase;
            std::size_t nDig;
            if (fValue < fMult)
            {
                bDirt = true;
                nDig = 0;
            }
            else
            {
                double fDig
                    = ::rtl::math::approxFloor(::rtl::math::approxSub(fValue, fMult));
                if (bDirt)
                {
                    bDirt = false;
                    --fDig;
                }
                if (fDig <= 0.0)
                    nDig = 0;
                else if (fDig >= fBase)
                    nDig = static_cast<std::size_t>(fBase) - 1;
                else
                    nDig = static_cast<std::size_t>(fDig);
            }
            *--p = DIGITS[nDig];
            fValue = fInt;
        }
    }

    if (fValue)
    {
        aResult.meError = NumeralStringError::StringOverflow;
        return aResult;
    }

    if (nBuf - (p - pBuf) <= nMinLen)
        p = pBuf + nBuf - nMinLen;

    aResult.maValue = OUString(p);
    return aResult;
}

std::optional<double> convertFromBase(const OUString& rText, double fBase)
{
    fBase = ::rtl::math::approxFloor(fBase);
    if (!(2.0 <= fBase && fBase <= 36.0))
        return std::nullopt;

    double fValue = 0.0;
    const int nBase = static_cast<int>(fBase);
    const sal_Unicode* p = rText.getStr();
    while (*p == ' ' || *p == '\t')
        ++p;

    if (nBase == 16)
    {
        if (*p == 'x' || *p == 'X')
            ++p;
        else if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X'))
            p += 2;
    }

    while (*p)
    {
        int n;
        if ('0' <= *p && *p <= '9')
            n = *p - '0';
        else if ('A' <= *p && *p <= 'Z')
            n = 10 + (*p - 'A');
        else if ('a' <= *p && *p <= 'z')
            n = 10 + (*p - 'a');
        else
            n = nBase;

        if (nBase <= n)
        {
            if (*(p + 1) == 0
                && ((nBase == 2 && (*p == 'b' || *p == 'B'))
                    || (nBase == 16 && (*p == 'h' || *p == 'H'))))
            {
                break;
            }
            return std::nullopt;
        }

        fValue = fValue * fBase + n;
        ++p;
    }

    return fValue;
}

std::optional<OUString> convertToRoman(double fValue, std::optional<double> ofMode)
{
    const double fNormalizedMode = ofMode ? ::rtl::math::approxFloor(*ofMode) : 0.0;
    const double fNormalizedValue = ::rtl::math::approxFloor(fValue);

    if (!(fNormalizedMode >= 0.0 && fNormalizedMode < 5.0 && fNormalizedValue >= 0.0
          && fNormalizedValue < 4000.0))
    {
        return std::nullopt;
    }

    static const sal_Unicode pChars[] = { 'M', 'D', 'C', 'L', 'X', 'V', 'I' };
    static const sal_uInt16 pValues[] = { 1000, 500, 100, 50, 10, 5, 1 };
    static const sal_uInt16 nMaxIndex = sal_uInt16(SAL_N_ELEMENTS(pValues) - 1);

    OUStringBuffer aRoman;
    sal_uInt16 nValue = static_cast<sal_uInt16>(fNormalizedValue);
    const sal_uInt16 nMode = static_cast<sal_uInt16>(fNormalizedMode);

    for (sal_uInt16 i = 0; i <= nMaxIndex / 2; ++i)
    {
        sal_uInt16 nIndex = 2 * i;
        const sal_uInt16 nDigit = nValue / pValues[nIndex];

        if ((nDigit % 5) == 4)
        {
            sal_uInt16 nIndex2 = (nDigit == 4) ? nIndex - 1 : nIndex - 2;
            sal_uInt16 nSteps = 0;
            while ((nSteps < nMode) && (nIndex < nMaxIndex))
            {
                ++nSteps;
                if (pValues[nIndex2] - pValues[nIndex + 1] <= nValue)
                    ++nIndex;
                else
                    nSteps = nMode;
            }
            aRoman.append(OUStringChar(pChars[nIndex]) + OUStringChar(pChars[nIndex2]));
            nValue = sal::static_int_cast<sal_uInt16>(nValue + pValues[nIndex]);
            nValue = sal::static_int_cast<sal_uInt16>(nValue - pValues[nIndex2]);
        }
        else
        {
            if (nDigit > 4)
                aRoman.append(pChars[nIndex - 1]);

            sal_Int32 nPad = nDigit % 5;
            while (nPad-- > 0)
                aRoman.append(pChars[nIndex]);

            nValue %= pValues[nIndex];
        }
    }

    return aRoman.makeStringAndClear();
}

std::optional<sal_Int32> convertFromRoman(const OUString& rRoman)
{
    const OUString aRoman = rRoman.toAsciiUpperCase();

    sal_uInt16 nValue = 0;
    sal_uInt16 nValidRest = 3999;
    sal_Int32 nCharIndex = 0;
    const sal_Int32 nCharCount = aRoman.getLength();
    bool bValid = true;

    while (bValid && (nCharIndex < nCharCount))
    {
        sal_uInt16 nDigit1 = 0;
        sal_uInt16 nDigit2 = 0;
        bool bIsDec1 = false;
        bValid = getArabicValue(aRoman[nCharIndex], nDigit1, bIsDec1);
        if (bValid && (nCharIndex + 1 < nCharCount))
        {
            bool bIsDec2 = false;
            bValid = getArabicValue(aRoman[nCharIndex + 1], nDigit2, bIsDec2);
        }
        if (bValid)
        {
            if (nDigit1 >= nDigit2)
            {
                nValue = sal::static_int_cast<sal_uInt16>(nValue + nDigit1);
                nValidRest %= (nDigit1 * (bIsDec1 ? 5 : 2));
                bValid = (nValidRest >= nDigit1);
                if (bValid)
                    nValidRest = sal::static_int_cast<sal_uInt16>(nValidRest - nDigit1);
                ++nCharIndex;
            }
            else if (nDigit1 * 2 != nDigit2)
            {
                const sal_uInt16 nDiff = nDigit2 - nDigit1;
                nValue = sal::static_int_cast<sal_uInt16>(nValue + nDiff);
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

    return static_cast<sal_Int32>(nValue);
}

} // namespace spreadsheetengine::core::convert

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
