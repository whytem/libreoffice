#pragma once

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>

#include <sal/types.h>

enum rtl_math_ConversionStatus
{
    rtl_math_ConversionStatus_Ok = 0,
    rtl_math_ConversionStatus_OutOfRange
};

enum rtl_math_RoundingMode
{
    rtl_math_RoundingMode_Corrected = 0,
    rtl_math_RoundingMode_Down,
    rtl_math_RoundingMode_Up
};

namespace rtl::math
{

inline double stringToDouble(std::u16string_view aString, sal_Unicode cDecSeparator,
    sal_Unicode cGroupSeparator, rtl_math_ConversionStatus* pStatus = nullptr,
    sal_Int32* pParsedEnd = nullptr)
{
    auto setStatus = [&](rtl_math_ConversionStatus eStatus)
    {
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
        const sal_Unicode c = aString[i];
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
        setStatus(rtl_math_ConversionStatus_Ok);
        return 0.0;
    }

    errno = 0;
    char* pEnd = nullptr;
    const double fValue = std::strtod(aNormalized.c_str(), &pEnd);
    setStatus(errno == ERANGE ? rtl_math_ConversionStatus_OutOfRange
                              : rtl_math_ConversionStatus_Ok);
    return fValue;
}

inline double approxFloor(double fValue) { return std::floor(fValue); }

inline double approxCeil(double fValue) { return std::ceil(fValue); }

inline bool isRepresentableInteger(double fValue)
{
    return std::isfinite(fValue) && std::trunc(fValue) == fValue
           && std::fabs(fValue) <= 0x1p53;
}

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
    const double fThreshold2 = std::pow(10.0, std::floor(std::log10(fMin))) * half15thSignificand;
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

inline double round(double fValue, int nDecimals, rtl_math_RoundingMode eMode)
{
    const double fScale = std::pow(10.0, static_cast<double>(nDecimals));
    const double fScaled = fValue * fScale;

    switch (eMode)
    {
        case rtl_math_RoundingMode_Down:
            return std::floor(fScaled) / fScale;
        case rtl_math_RoundingMode_Up:
            return std::ceil(fScaled) / fScale;
        default:
            return std::round(fScaled) / fScale;
    }
}

inline double sin(double fValue) { return std::sin(fValue); }

inline double cos(double fValue) { return std::cos(fValue); }

inline double tan(double fValue) { return std::tan(fValue); }

inline double asinh(double fValue) { return std::asinh(fValue); }

inline double acosh(double fValue) { return std::acosh(fValue); }

} // namespace rtl::math
