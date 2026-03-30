/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::math
{

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> fisherTransform(double fValue);

SPREADSHEETENGINE_DLLPUBLIC double inverseFisherTransform(double fValue);

SPREADSHEETENGINE_DLLPUBLIC double betaValue(double fAlpha, double fBeta);

SPREADSHEETENGINE_DLLPUBLIC double betaCdf(double fInput, double fAlpha, double fBeta);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateBetaDistribution(
    double fX, double fAlpha, double fBeta, double fLowerBound, double fUpperBound,
    bool bCumulative, bool bMicrosoftOrder);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePoissonDistribution(
    double fX, double fLambda, bool bCumulative);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateBinomialDistribution(
    double fSuccesses, double fTrials, double fProbability, bool bCumulative);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateBinomialRangeDistribution(
    double fTrials, double fProbability, double fSuccessStart, double fSuccessEnd);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
