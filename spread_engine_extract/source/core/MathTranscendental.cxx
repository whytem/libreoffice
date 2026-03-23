/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/MathTranscendental.hxx>

#include <cmath>

#include <basegfx/numeric/ftools.hxx>
#include <rtl/math.hxx>

namespace spreadsheetengine::core::math
{

double computePi() { return M_PI; }

double computeDegrees(double fRadians) { return basegfx::rad2deg(fRadians); }

double computeRadians(double fDegrees) { return basegfx::deg2rad(fDegrees); }

double computeSin(double fValue) { return ::rtl::math::sin(fValue); }

double computeCos(double fValue) { return ::rtl::math::cos(fValue); }

double computeTan(double fValue) { return ::rtl::math::tan(fValue); }

double computeCot(double fValue) { return 1.0 / ::rtl::math::tan(fValue); }

double computeArcSin(double fValue) { return std::asin(fValue); }

double computeArcCos(double fValue) { return std::acos(fValue); }

double computeArcTan(double fValue) { return std::atan(fValue); }

double computeArcCot(double fValue) { return M_PI_2 - std::atan(fValue); }

double computeSinHyp(double fValue) { return std::sinh(fValue); }

double computeCosHyp(double fValue) { return std::cosh(fValue); }

double computeTanHyp(double fValue) { return std::tanh(fValue); }

double computeCotHyp(double fValue) { return 1.0 / std::tanh(fValue); }

double computeArcSinHyp(double fValue) { return ::rtl::math::asinh(fValue); }

std::optional<double> computeArcCosHyp(double fValue)
{
    if (fValue < 1.0)
        return std::nullopt;
    return ::rtl::math::acosh(fValue);
}

std::optional<double> computeArcTanHyp(double fValue)
{
    if (std::fabs(fValue) >= 1.0)
        return std::nullopt;
    return std::atanh(fValue);
}

std::optional<double> computeArcCotHyp(double fValue)
{
    if (std::fabs(fValue) <= 1.0)
        return std::nullopt;
    return 0.5 * std::log((fValue + 1.0) / (fValue - 1.0));
}

double computeCosecant(double fValue) { return 1.0 / ::rtl::math::sin(fValue); }

double computeSecant(double fValue) { return 1.0 / ::rtl::math::cos(fValue); }

double computeCosecantHyp(double fValue) { return 1.0 / std::sinh(fValue); }

double computeSecantHyp(double fValue) { return 1.0 / std::cosh(fValue); }

double computeExp(double fValue) { return std::exp(fValue); }

std::optional<double> computeSqrt(double fValue)
{
    if (fValue < 0.0)
        return std::nullopt;
    return std::sqrt(fValue);
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
