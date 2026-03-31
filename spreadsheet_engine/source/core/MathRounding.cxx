/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <cstdint>

#include <cmath>

namespace spreadsheetengine::core::math
{

double roundToDecimals(double fValue, std::int16_t nDecimals, fp::RoundingMode eMode)
{
    constexpr std::int16_t kSigDig = 12;

    if ((eMode == fp::RoundingMode::Down || eMode == fp::RoundingMode::Up)
        && nDecimals < kSigDig && std::fmod(fValue, 1.0) != 0.0)
    {
        double fRes = fValue;
        const double fTemp = std::floor(std::log10(std::abs(fRes))) + 1.0 - kSigDig;
        if (fTemp < 0.0)
            fRes *= std::pow(10.0, -fTemp);
        else
            fRes /= std::pow(10.0, fTemp);

        if (std::isfinite(fRes))
        {
            if (eMode == fp::RoundingMode::Up)
                fRes = fp::approxFloor(fRes);

            double fRounded = fp::round(fRes, nDecimals + fTemp, eMode);
            if (fTemp < 0.0)
                fRounded /= std::pow(10.0, -fTemp);
            else
                fRounded *= std::pow(10.0, fTemp);
            return fRounded;
        }
    }

    return fp::round(fValue, nDecimals, eMode);
}

double roundToSignificantDigits(double fValue, double fDigits)
{
    const double fTemp = std::floor(std::log10(std::abs(fValue))) + 1.0 - fDigits;
    double fIn = fValue;
    if (fTemp < 0.0)
        fIn *= std::pow(10.0, -fTemp);
    else
        fIn /= std::pow(10.0, fTemp);

    double fRes = fp::round(fIn);
    if (fTemp < 0.0)
        fRes /= std::pow(10.0, -fTemp);
    else
        fRes *= std::pow(10.0, fTemp);
    return fRes;
}

std::optional<double> computeCeiling(double fValue, double fSignificance, bool bAbs, bool bODFF)
{
    if (fValue == 0.0 || fSignificance == 0.0)
        return 0.0;

    if (bODFF && fValue * fSignificance < 0.0)
        return std::nullopt;

    if (fValue * fSignificance < 0.0)
        fSignificance = -fSignificance;

    if (!bAbs && fValue < 0.0)
        return fp::approxFloor(fValue / fSignificance) * fSignificance;

    return fp::approxCeil(fValue / fSignificance) * fSignificance;
}

std::optional<double> computeCeilingMs(double fValue, double fSignificance)
{
    if (fValue == 0.0 || fSignificance == 0.0)
        return 0.0;

    if (fValue * fSignificance > 0.0)
        return fp::approxCeil(fValue / fSignificance) * fSignificance;

    if (fValue < 0.0)
        return fp::approxFloor(fValue / -fSignificance) * -fSignificance;

    return std::nullopt;
}

double computeCeilingPrecise(double fValue, double fSignificance)
{
    if (fValue == 0.0 || fSignificance == 0.0)
        return 0.0;

    return fp::approxCeil(fValue / fSignificance) * fSignificance;
}

std::optional<double> computeFloor(double fValue, double fSignificance, bool bAbs, bool bODFF)
{
    if (fValue == 0.0 || fSignificance == 0.0)
        return 0.0;

    if (bODFF && fValue * fSignificance < 0.0)
        return std::nullopt;

    if (fValue * fSignificance < 0.0)
        fSignificance = -fSignificance;

    if (!bAbs && fValue < 0.0)
        return fp::approxCeil(fValue / fSignificance) * fSignificance;

    return fp::approxFloor(fValue / fSignificance) * fSignificance;
}

std::optional<double> computeFloorMs(double fValue, double fSignificance)
{
    if (fValue == 0.0)
        return 0.0;

    if (fValue * fSignificance > 0.0)
        return fp::approxFloor(fValue / fSignificance) * fSignificance;

    if (fSignificance == 0.0)
        return std::nullopt;

    if (fValue < 0.0)
        return fp::approxCeil(fValue / -fSignificance) * -fSignificance;

    return std::nullopt;
}

double computeFloorPrecise(double fValue, double fSignificance)
{
    if (fValue == 0.0 || fSignificance == 0.0)
        return 0.0;

    return fp::approxFloor(fValue / fSignificance) * fSignificance;
}

double computeEven(double fValue)
{
    if (fValue < 0.0)
        return fp::approxFloor(fValue / 2.0) * 2.0;

    return fp::approxCeil(fValue / 2.0) * 2.0;
}

double computeOdd(double fValue)
{
    if (fValue >= 0.0)
    {
        fValue = fp::approxCeil(fValue);
        if (std::fmod(fValue, 2.0) == 0.0)
            ++fValue;
    }
    else
    {
        fValue = fp::approxFloor(fValue);
        if (std::fmod(fValue, 2.0) == 0.0)
            --fValue;
    }

    return fValue;
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
