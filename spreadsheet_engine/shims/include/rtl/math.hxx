#pragma once

// Shim: provides rtl::math namespace by delegating to the engine's fp:: module.
// This file is only used in standalone builds; the LO build uses the real rtl/math.hxx.

#include <spreadsheetengine/runtime/FloatingPoint.hxx>

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

namespace fp = spreadsheetengine::core::fp;

inline double stringToDouble(std::u16string_view aString, char16_t cDecSeparator,
    char16_t cGroupSeparator, rtl_math_ConversionStatus* pStatus = nullptr,
    sal_Int32* pParsedEnd = nullptr)
{
    fp::ConversionStatus eStatus = fp::ConversionStatus::Ok;
    const double fResult = fp::stringToDouble(aString, cDecSeparator, cGroupSeparator,
        &eStatus, pParsedEnd);
    if (pStatus)
        *pStatus = eStatus == fp::ConversionStatus::OutOfRange
                       ? rtl_math_ConversionStatus_OutOfRange
                       : rtl_math_ConversionStatus_Ok;
    return fResult;
}

inline bool isRepresentableInteger(double fValue)
{
    return fp::isRepresentableInteger(fValue);
}

inline double approxValue(double fValue) { return fp::approxValue(fValue); }

inline double approxFloor(double fValue) { return fp::approxFloor(fValue); }

inline double approxCeil(double fValue) { return fp::approxCeil(fValue); }

inline bool approxEqual(double fLeft, double fRight)
{
    return fp::approxEqual(fLeft, fRight);
}

inline double approxAdd(double fLeft, double fRight)
{
    return fp::approxAdd(fLeft, fRight);
}

inline double approxSub(double fLeft, double fRight)
{
    return fp::approxSub(fLeft, fRight);
}

inline double round(double fValue) { return fp::round(fValue); }

inline double round(double fValue, int nDecimals, rtl_math_RoundingMode eMode)
{
    fp::RoundingMode eFpMode = fp::RoundingMode::Corrected;
    if (eMode == rtl_math_RoundingMode_Down)
        eFpMode = fp::RoundingMode::Down;
    else if (eMode == rtl_math_RoundingMode_Up)
        eFpMode = fp::RoundingMode::Up;
    return fp::round(fValue, nDecimals, eFpMode);
}

inline bool isValidArcArg(double fValue) { return fp::isValidArcArg(fValue); }

inline double sin(double fValue)
{
    return isValidArcArg(fValue) ? std::sin(fValue)
                                 : std::numeric_limits<double>::quiet_NaN();
}

inline double cos(double fValue)
{
    return isValidArcArg(fValue) ? std::cos(fValue)
                                 : std::numeric_limits<double>::quiet_NaN();
}

inline double tan(double fValue)
{
    return isValidArcArg(fValue) ? std::tan(fValue)
                                 : std::numeric_limits<double>::quiet_NaN();
}

inline double asinh(double fValue) { return std::asinh(fValue); }

inline double acosh(double fValue) { return std::acosh(fValue); }

} // namespace rtl::math
