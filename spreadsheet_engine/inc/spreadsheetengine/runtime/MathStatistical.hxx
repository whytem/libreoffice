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

SPREADSHEETENGINE_DLLPUBLIC double gaussValue(double fValue);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateStandardNormalInverse(
    double fProbability);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> lowRegularizedIncompleteGamma(
    double fAlpha, double fX);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> upRegularizedIncompleteGamma(
    double fAlpha, double fX);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateLegacyChiDist(
    double fChi, double fDegreesFreedom);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateBinomialInverse(
    double fTrials, double fProbability, double fAlpha);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateNormalDistribution(
    double fX, double fMean, double fSigma, bool bCumulative);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateNormalInverse(
    double fProbability, double fMean, double fSigma);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateLogNormalDistribution(
    double fX, double fMean, double fSigma, bool bCumulative);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateLogNormalInverse(
    double fProbability, double fMean, double fSigma);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateChiSquareDistribution(
    double fX, double fDegreesFreedom, bool bCumulative, bool bMicrosoftSyntax);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateChiSquareInverse(
    double fProbability, double fDegreesFreedom);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateGammaDistribution(
    double fX, double fAlpha, double fBeta, bool bCumulative, bool bMicrosoftSyntax);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateGammaInverse(
    double fProbability, double fAlpha, double fBeta);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateGammaValue(double fX);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateStudentDistribution(
    double fT, double fDegreesFreedom, int nType);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateTInverse(
    double fProbability, double fDegreesFreedom, int nType);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateFRightTailDistribution(
    double fX, double fDegreesFreedom1, double fDegreesFreedom2);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateFInverseRightTail(
    double fProbability, double fDegreesFreedom1, double fDegreesFreedom2);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateLegacyChiInverse(
    double fProbability, double fDegreesFreedom);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateBetaDistribution(
    double fX, double fAlpha, double fBeta, double fLowerBound, double fUpperBound,
    bool bCumulative, bool bMicrosoftOrder);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateBetaInverse(
    double fProbability, double fAlpha, double fBeta, double fLowerBound,
    double fUpperBound);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePoissonDistribution(
    double fX, double fLambda, bool bCumulative);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateBinomialDistribution(
    double fSuccesses, double fTrials, double fProbability, bool bCumulative);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateBinomialRangeDistribution(
    double fTrials, double fProbability, double fSuccessStart, double fSuccessEnd);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateConfidence(
    double fAlpha, double fSigma, double fSampleSize);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateConfidenceT(
    double fAlpha, double fSigma, double fSampleSize);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
