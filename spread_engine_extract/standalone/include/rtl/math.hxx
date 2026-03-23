#pragma once

#include <algorithm>
#include <cmath>

enum rtl_math_RoundingMode
{
    rtl_math_RoundingMode_Corrected = 0,
    rtl_math_RoundingMode_Down,
    rtl_math_RoundingMode_Up
};

namespace rtl::math
{

inline double approxFloor(double fValue) { return std::floor(fValue); }

inline double approxCeil(double fValue) { return std::ceil(fValue); }

inline double approxSub(double fLeft, double fRight) { return fLeft - fRight; }

inline bool approxEqual(double fLeft, double fRight)
{
    const double fDiff = std::fabs(fLeft - fRight);
    const double fScale = std::max({ 1.0, std::fabs(fLeft), std::fabs(fRight) });
    return fDiff <= (1.0e-12 * fScale);
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
