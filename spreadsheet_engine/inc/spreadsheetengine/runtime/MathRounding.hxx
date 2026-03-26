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

#include <rtl/math.hxx>
#include <sal/types.h>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::math
{

SPREADSHEETENGINE_DLLPUBLIC double roundToDecimals(
    double fValue, sal_Int16 nDecimals, rtl_math_RoundingMode eMode);

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

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
