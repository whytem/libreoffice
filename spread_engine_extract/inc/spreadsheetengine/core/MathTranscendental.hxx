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

#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::math
{

SPREADSHEETENGINE_DLLPUBLIC double computePi();
SPREADSHEETENGINE_DLLPUBLIC double computeDegrees(double fRadians);
SPREADSHEETENGINE_DLLPUBLIC double computeRadians(double fDegrees);
SPREADSHEETENGINE_DLLPUBLIC double computeSin(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeCos(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeTan(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeCot(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeArcSin(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeArcCos(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeArcTan(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeArcCot(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeSinHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeCosHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeTanHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeCotHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeArcSinHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeArcCosHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeArcTanHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeArcCotHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeCosecant(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeSecant(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeCosecantHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeSecantHyp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC double computeExp(double fValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeSqrt(double fValue);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
