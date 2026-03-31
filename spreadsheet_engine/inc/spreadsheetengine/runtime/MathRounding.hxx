/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <spreadsheetengine/runtime/FloatingPoint.hxx>
#include <spreadsheetengine/api/Types.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

#if __has_include(<sal/config.h>)
#include <rtl/math.hxx>
#endif

namespace spreadsheetengine::core::math
{

SPREADSHEETENGINE_DLLPUBLIC double roundToDecimals(
    double fValue, sal_Int16 nDecimals, fp::RoundingMode eMode);

SPREADSHEETENGINE_DLLPUBLIC double roundToSignificantDigits(
    double fValue, double fDigits);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeCeiling(
    double fValue, double fSignificance, bool bAbs, bool bODFF);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeCeilingMs(
    double fValue, double fSignificance);

SPREADSHEETENGINE_DLLPUBLIC double computeCeilingPrecise(
    double fValue, double fSignificance);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeFloor(
    double fValue, double fSignificance, bool bAbs, bool bODFF);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeFloorMs(
    double fValue, double fSignificance);

SPREADSHEETENGINE_DLLPUBLIC double computeFloorPrecise(
    double fValue, double fSignificance);

SPREADSHEETENGINE_DLLPUBLIC double computeEven(double fValue);

SPREADSHEETENGINE_DLLPUBLIC double computeOdd(double fValue);

// Backward-compatible overload for callers passing rtl_math_RoundingMode directly
#if __has_include(<sal/config.h>)
inline double roundToDecimals(double fValue, sal_Int16 nDecimals, rtl_math_RoundingMode eMode)
{
    fp::RoundingMode eFpMode = fp::RoundingMode::Corrected;
    if (eMode == rtl_math_RoundingMode_Down)
        eFpMode = fp::RoundingMode::Down;
    else if (eMode == rtl_math_RoundingMode_Up)
        eFpMode = fp::RoundingMode::Up;
    return roundToDecimals(fValue, nDecimals, eFpMode);
}
#endif

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
