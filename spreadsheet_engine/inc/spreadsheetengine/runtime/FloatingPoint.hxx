/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>

#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::core::fp
{

enum class RoundingMode
{
    Corrected = 0,
    Down,
    Up
};

enum class ConversionStatus
{
    Ok = 0,
    OutOfRange
};

inline double stringToDouble(std::u16string_view aString, char16_t cDecSeparator,
    char16_t cGroupSeparator, ConversionStatus* pStatus = nullptr,
    sal_Int32* pParsedEnd = nullptr)
{
    auto setStatus = [&](ConversionStatus eStatus) {
        if (pStatus)
            *pStatus = eStatus;
    };

    std::string aNormalized;
    aNormalized.reserve(aString.size());

    std::size_t i = 0;
    if (i < aString.size() && (aString[i] == u'+' || aString[i] == u'-'))
        aNormalized.push_back(static_cast<char>(aString[i++]));

    bool bSeenDigit = false;
    bool bSeenDecimal = false;
    while (i < aString.size())
    {
        const char16_t c = aString[i];
        if (u'0' <= c && c <= u'9')
        {
            aNormalized.push_back(static_cast<char>(c));
            bSeenDigit = true;
            ++i;
            continue;
        }
        if (cGroupSeparator != 0 && c == cGroupSeparator)
        {
            ++i;
            continue;
        }
        if (!bSeenDecimal && cDecSeparator != 0 && c == cDecSeparator)
        {
            aNormalized.push_back('.');
            bSeenDecimal = true;
            ++i;
            continue;
        }
        break;
    }

    if (bSeenDigit && i < aString.size() && (aString[i] == u'e' || aString[i] == u'E'))
    {
        std::size_t j = i + 1;
        std::string aExponent;
        aExponent.push_back('e');
        if (j < aString.size() && (aString[j] == u'+' || aString[j] == u'-'))
            aExponent.push_back(static_cast<char>(aString[j++]));

        bool bSeenExponentDigit = false;
        while (j < aString.size() && u'0' <= aString[j] && aString[j] <= u'9')
        {
            aExponent.push_back(static_cast<char>(aString[j++]));
            bSeenExponentDigit = true;
        }

        if (bSeenExponentDigit)
        {
            aNormalized += aExponent;
            i = j;
        }
    }

    if (pParsedEnd)
        *pParsedEnd = bSeenDigit ? static_cast<sal_Int32>(i) : 0;

    if (!bSeenDigit)
    {
        setStatus(ConversionStatus::Ok);
        return 0.0;
    }

    errno = 0;
    char* pEnd = nullptr;
    const double fValue = std::strtod(aNormalized.c_str(), &pEnd);
    setStatus(errno == ERANGE ? ConversionStatus::OutOfRange : ConversionStatus::Ok);
    return fValue;
}

inline bool isRepresentableInteger(double fValue)
{
    return std::isfinite(fValue) && std::trunc(fValue) == fValue
           && std::fabs(fValue) <= 0x1p53;
}

inline double approxValue(double fValue)
{
    const double fBigInt = 0x1p41;
    if (fValue == 0.0 || !std::isfinite(fValue) || std::fabs(fValue) > fBigInt)
        return fValue;

    const bool bNegative = std::signbit(fValue);
    double fAbs = std::fabs(fValue);
    const int nExp = static_cast<int>(std::floor(std::log10(fAbs)));
    const int nScaleExp = 14 - nExp;
    const double fScale = std::pow(10.0, static_cast<double>(std::abs(nScaleExp)));

    double fScaled = nScaleExp < 0 ? fAbs / fScale : fAbs * fScale;
    if (!std::isfinite(fScaled))
        return fValue;

    fScaled = std::round(fScaled);
    fScaled = nScaleExp < 0 ? fScaled * fScale : fScaled / fScale;
    if (!std::isfinite(fScaled))
        return fValue;

    return bNegative ? -fScaled : fScaled;
}

inline double approxFloor(double fValue) { return std::floor(approxValue(fValue)); }

inline double approxCeil(double fValue) { return std::ceil(approxValue(fValue)); }

inline bool approxEqual(double fLeft, double fRight)
{
    static const double e48 = 0x1p-48;
    static const double half15thSignificand = 5E-15;

    if (fLeft == fRight)
        return true;

    if (fLeft == 0.0 || fRight == 0.0 || std::signbit(fLeft) != std::signbit(fRight))
        return false;

    const double fDiff = std::fabs(fLeft - fRight);
    if (!std::isfinite(fDiff))
        return false;

    const double fLeftAbs = std::fabs(fLeft);
    const double fRightAbs = std::fabs(fRight);
    const double fMin = std::min(fLeftAbs, fRightAbs);
    const double fThreshold1 = fMin * e48;
    const double fThreshold2
        = std::pow(10.0, std::floor(std::log10(fMin))) * half15thSignificand;
    if (fDiff >= std::max(fThreshold1, fThreshold2))
        return false;

    if (isRepresentableInteger(fLeftAbs) && isRepresentableInteger(fRightAbs))
        return false;

    return true;
}

inline double approxAdd(double fLeft, double fRight)
{
    if (((fLeft < 0.0 && fRight > 0.0) || (fRight < 0.0 && fLeft > 0.0))
        && approxEqual(fLeft, -fRight))
    {
        return 0.0;
    }
    return fLeft + fRight;
}

inline double approxSub(double fLeft, double fRight)
{
    if (((fLeft < 0.0 && fRight < 0.0) || (fLeft > 0.0 && fRight > 0.0))
        && approxEqual(fLeft, fRight))
    {
        return 0.0;
    }
    return fLeft - fRight;
}

inline double round(double fValue) { return std::round(fValue); }

inline double round(double fValue, int nDecimals, RoundingMode eMode)
{
    const double fScale = std::pow(10.0, static_cast<double>(nDecimals));
    const double fScaled = fValue * fScale;

    switch (eMode)
    {
        case RoundingMode::Down:
            return std::floor(fScaled) / fScale;
        case RoundingMode::Up:
            return std::ceil(fScaled) / fScale;
        default:
            return std::round(fScaled) / fScale;
    }
}

inline bool isValidArcArg(double fValue)
{
    return std::fabs(fValue)
           <= (static_cast<double>(static_cast<unsigned long>(0x80000000))
               * static_cast<double>(static_cast<unsigned long>(0x80000000)) * 4.0);
}

inline double divAllowZero(double a, double b)
{
    if (b == 0.0)
    {
        if (std::isfinite(a) && a != 0.0)
            return std::signbit(a) ? -std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::infinity();
        return std::numeric_limits<double>::quiet_NaN();
    }
    return a / b;
}

} // namespace spreadsheetengine::core::fp

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
