/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/MathScalar.hxx>

#include <cmath>

#include <rtl/math.hxx>

namespace spreadsheetengine::core::math
{

short computePlusMinus(double fValue)
{
    if (fValue < 0.0)
        return -1;
    if (fValue > 0.0)
        return 1;
    return 0;
}

double computeAbs(double fValue)
{
    return std::abs(fValue);
}

double computeInt(double fValue)
{
    return ::rtl::math::approxFloor(fValue);
}

double computeArcTan2(double fY, double fX)
{
    return std::atan2(fY, fX);
}

std::optional<double> computeLog(double fValue, double fBase)
{
    if (fValue > 0.0 && fBase > 0.0 && fBase != 1.0)
        return std::log(fValue) / std::log(fBase);
    return std::nullopt;
}

std::optional<double> computeLn(double fValue)
{
    if (fValue > 0.0)
        return std::log(fValue);
    return std::nullopt;
}

std::optional<double> computeLog10(double fValue)
{
    if (fValue > 0.0)
        return std::log10(fValue);
    return std::nullopt;
}

std::optional<double> computeMod(double fNumerator, double fDenominator)
{
    const double fRes = ::rtl::math::approxSub(
        fNumerator, ::rtl::math::approxFloor(fNumerator / fDenominator) * fDenominator);
    if ((fDenominator > 0 && fRes >= 0 && fRes < fDenominator)
        || (fDenominator < 0 && fRes <= 0 && fRes > fDenominator))
    {
        return fRes;
    }
    return std::nullopt;
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
