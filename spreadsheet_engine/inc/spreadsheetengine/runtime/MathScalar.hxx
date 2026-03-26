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

SPREADSHEETENGINE_DLLPUBLIC short computePlusMinus(double fValue);

SPREADSHEETENGINE_DLLPUBLIC double computeAbs(double fValue);

SPREADSHEETENGINE_DLLPUBLIC double computeInt(double fValue);

SPREADSHEETENGINE_DLLPUBLIC double computeArcTan2(double fY, double fX);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeLog(
    double fValue, double fBase);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeLn(double fValue);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeLog10(double fValue);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeMod(
    double fNumerator, double fDenominator);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
