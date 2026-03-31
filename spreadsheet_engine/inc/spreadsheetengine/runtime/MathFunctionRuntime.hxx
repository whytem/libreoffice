/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Rounding.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::math
{

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateRoundValue(
    double fValue, sal_Int32 nDecimals, api::RoundingMode eMode, bool bDirectional);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCeilingFloorValue(
    double fValue, double fSignificance, bool bAbs, bool bCeiling, bool bMicrosoftCompat);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCeilingFloorMathValue(
    double fValue, double fSignificance, double fMode, bool bCeiling);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCeilingFloorPreciseValue(
    double fValue, double fSignificance, bool bFloor);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateRoundSigValue(
    double fValue, double fDigits);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateLogValue(
    double fValue, double fBase);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateMroundValue(
    double fValue, double fMultiple);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateModValue(
    double fNumerator, double fDenominator);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateFactorialValue(double fValue);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCombinValue(
    double fN, double fK, bool bAllowRepetition);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePermutationValue(
    double fN, double fK);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePermutationAValue(
    double fN, double fK);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateMultinomialValue(
    const std::vector<double>& rValues);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCscValue(double fValue);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateCschValue(double fValue);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateTruncValue(
    double fValue, sal_Int32 nDigits);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
