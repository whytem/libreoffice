/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/math.hxx>
#include <sal/types.h>
#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <spreadsheetengine/runtime/MathFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>

#include <cmath>

namespace spreadsheetengine::compat::libreoffice::interpreterdispatch
{

enum class ComparisonMode : sal_uInt8
{
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual
};

enum class LogicalFoldMode : sal_uInt8
{
    And,
    Or,
    Xor
};

enum class UnaryMatrixScalarMode : sal_uInt8
{
    Negate,
    LogicalNot
};

[[nodiscard]] constexpr bool matchesComparisonResult(short nCompareResult, ComparisonMode eMode)
{
    switch (eMode)
    {
        case ComparisonMode::Equal:
            return nCompareResult == 0;
        case ComparisonMode::NotEqual:
            return nCompareResult != 0;
        case ComparisonMode::Less:
            return nCompareResult < 0;
        case ComparisonMode::Greater:
            return nCompareResult > 0;
        case ComparisonMode::LessEqual:
            return nCompareResult <= 0;
        case ComparisonMode::GreaterEqual:
            return nCompareResult >= 0;
    }

    return false;
}

[[nodiscard]] constexpr bool initialLogicalFoldValue(LogicalFoldMode eMode)
{
    return eMode == LogicalFoldMode::And;
}

[[nodiscard]] constexpr bool foldLogicalValue(
    LogicalFoldMode eMode, bool bAccumulated, bool bValue)
{
    switch (eMode)
    {
        case LogicalFoldMode::And:
            return bAccumulated && bValue;
        case LogicalFoldMode::Or:
            return bAccumulated || bValue;
        case LogicalFoldMode::Xor:
            return bAccumulated != bValue;
    }

    return false;
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyStdNormDist(
    double fX, bool bCumulative)
{
    return core::math::evaluateNormalDistribution(fX, 0.0, 1.0, bCumulative);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyExponentialDist(
    double fX, double fLambda, bool bCumulative)
{
    return core::math::evaluateExponentialDistribution(fX, fLambda, bCumulative);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyPermutation(
    double fN, double fK, bool bAllowRepetition)
{
    return bAllowRepetition ? core::math::evaluatePermutationAValue(fN, fK)
                            : core::math::evaluatePermutationValue(fN, fK);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyWeibull(
    double fX, double fAlpha, double fBeta, bool bCumulative)
{
    return core::math::evaluateWeibullDistribution(fX, fAlpha, fBeta, bCumulative);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacySNormInv(double fProbability)
{
    return core::math::evaluateStandardNormalInverse(fProbability);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyGammaInverse(
    double fProbability, double fAlpha, double fBeta)
{
    return core::math::evaluateGammaInverse(fProbability, fAlpha, fBeta);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyBinomDist(
    double fSuccesses, double fTrials, double fProbability)
{
    return core::math::evaluateBinomialDistribution(fSuccesses, fTrials, fProbability, false);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyBinomDistMs(
    double fSuccesses, double fTrials, double fProbability, bool bCumulative)
{
    return core::math::evaluateBinomialDistribution(
        fSuccesses, fTrials, fProbability, bCumulative);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyBinomRange(
    double fTrials, double fProbability, double fSuccessStart, double fSuccessEnd)
{
    return core::math::evaluateBinomialRangeDistribution(
        fTrials, fProbability, fSuccessStart, fSuccessEnd);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyNormDist(
    double fX, double fMean, double fSigma, bool bCumulative)
{
    return core::math::evaluateNormalDistribution(fX, fMean, fSigma, bCumulative);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyHypGeomDist(
    double fX, double fSampleSuccesses, double fPopulationSuccesses,
    double fPopulationSize, bool bCumulative)
{
    const double fWholeX = ::rtl::math::approxFloor(fX);
    const double fWholeSampleSuccesses = ::rtl::math::approxFloor(fSampleSuccesses);
    const double fWholePopulationSuccesses = ::rtl::math::approxFloor(fPopulationSuccesses);
    const double fWholePopulationSize = ::rtl::math::approxFloor(fPopulationSize);
    if ((fWholeX < 0.0) || (fWholeSampleSuccesses < fWholeX)
        || (fWholePopulationSize < fWholeSampleSuccesses)
        || (fWholePopulationSize < fWholePopulationSuccesses)
        || (fWholePopulationSuccesses < 0.0))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    return core::math::evaluateHypergeometricDistribution(
        fWholeX, fWholeSampleSuccesses, fWholePopulationSuccesses,
        fWholePopulationSize, bCumulative);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyLogNormDist(
    double fX, double fMean, double fSigma, bool bCumulative)
{
    return core::math::evaluateLogNormalDistribution(fX, fMean, fSigma, bCumulative);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyLogNormInv(
    double fProbability, double fMean, double fSigma)
{
    return core::math::evaluateLogNormalInverse(fProbability, fMean, fSigma);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyBetaDist(
    double fX, double fAlpha, double fBeta, double fLowerBound, double fUpperBound,
    bool bCumulative, bool bMicrosoftOrder)
{
    return core::math::evaluateBetaDistribution(
        fX, fAlpha, fBeta, fLowerBound, fUpperBound, bCumulative, bMicrosoftOrder);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyBetaInv(
    double fProbability, double fAlpha, double fBeta, double fLowerBound,
    double fUpperBound)
{
    return core::math::evaluateBetaInverse(
        fProbability, fAlpha, fBeta, fLowerBound, fUpperBound);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyCritBinom(
    double fTrials, double fProbability, double fAlpha)
{
    return core::math::evaluateBinomialInverse(fTrials, fProbability, fAlpha);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyNegBinomDist(
    double fFailures, double fSuccesses, double fProbability, bool bCumulative,
    bool bMicrosoftSyntax)
{
    return core::math::evaluateNegativeBinomialDistribution(
        fFailures, fSuccesses, fProbability, bCumulative, bMicrosoftSyntax);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyStandardize(
    double fX, double fMean, double fSigma)
{
    if (fSigma < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (fSigma == 0.0)
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);
    return api::ValueResult<double>::success((fX - fMean) / fSigma);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyPoissonDist(
    double fX, double fLambda, bool bCumulative)
{
    return core::math::evaluatePoissonDistribution(fX, fLambda, bCumulative);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyNormInv(
    double fProbability, double fMean, double fSigma)
{
    return core::math::evaluateNormalInverse(fProbability, fMean, fSigma);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyConfidence(
    double fAlpha, double fSigma, double fSampleSize, bool bStudent)
{
    const double fWholeSampleSize = ::rtl::math::approxFloor(fSampleSize);
    return bStudent ? core::math::evaluateConfidenceT(fAlpha, fSigma, fWholeSampleSize)
                    : core::math::evaluateConfidence(fAlpha, fSigma, fWholeSampleSize);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyChiSqDist(
    double fX, double fDegreesFreedom, bool bCumulative, bool bMicrosoftSyntax)
{
    const double fWholeDegreesFreedom = ::rtl::math::approxFloor(fDegreesFreedom);
    if (fWholeDegreesFreedom < 1.0 || (bMicrosoftSyntax && fWholeDegreesFreedom > 1.0e10)
        || (bMicrosoftSyntax && fX < 0.0))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
    return core::math::evaluateChiSquareDistribution(
        fX, fWholeDegreesFreedom, bCumulative, false);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyTDist(
    double fT, double fDegreesFreedom, int nType)
{
    const double fWholeDegreesFreedom = ::rtl::math::approxFloor(fDegreesFreedom);
    if (fWholeDegreesFreedom < 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return core::math::evaluateStudentDistribution(fT, fWholeDegreesFreedom, nType);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyTDistLegacy(
    double fT, double fDegreesFreedom, double fFlag)
{
    const double fWholeDegreesFreedom = ::rtl::math::approxFloor(fDegreesFreedom);
    const double fWholeFlag = ::rtl::math::approxFloor(fFlag);
    if (fWholeDegreesFreedom < 1.0 || fT < 0.0 || (fWholeFlag != 1.0 && fWholeFlag != 2.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return core::math::evaluateStudentDistribution(
        fT, fWholeDegreesFreedom, static_cast<int>(fWholeFlag));
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyTDistTails(
    double fT, double fDegreesFreedom, int nTails)
{
    const double fWholeDegreesFreedom = ::rtl::math::approxFloor(fDegreesFreedom);
    if (fWholeDegreesFreedom < 1.0 || (nTails == 2 && fT < 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const auto aResult = core::math::evaluateStudentDistribution(fT, fWholeDegreesFreedom, nTails);
    if (!aResult)
        return aResult;
    if (nTails == 1 && fT < 0.0)
        return api::ValueResult<double>::success(1.0 - aResult.maValue);
    return aResult;
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyTDistMs(
    double fT, double fDegreesFreedom, bool bCumulative)
{
    const double fWholeDegreesFreedom = ::rtl::math::approxFloor(fDegreesFreedom);
    if (fWholeDegreesFreedom < 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return core::math::evaluateStudentDistribution(
        fT, fWholeDegreesFreedom, bCumulative ? 4 : 3);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyFDistRightTail(
    double fRatio, double fDegreesFreedom1, double fDegreesFreedom2)
{
    const double fWholeDegreesFreedom1 = ::rtl::math::approxFloor(fDegreesFreedom1);
    const double fWholeDegreesFreedom2 = ::rtl::math::approxFloor(fDegreesFreedom2);
    if (fRatio < 0.0 || fWholeDegreesFreedom1 < 1.0 || fWholeDegreesFreedom2 < 1.0
        || fWholeDegreesFreedom1 >= 1.0e10 || fWholeDegreesFreedom2 >= 1.0e10)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
    return core::math::evaluateFRightTailDistribution(
        fRatio, fWholeDegreesFreedom1, fWholeDegreesFreedom2);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyFDistLeftTail(
    double fRatio, double fDegreesFreedom1, double fDegreesFreedom2, bool bCumulative)
{
    const double fWholeDegreesFreedom1 = ::rtl::math::approxFloor(fDegreesFreedom1);
    const double fWholeDegreesFreedom2 = ::rtl::math::approxFloor(fDegreesFreedom2);
    if (fRatio < 0.0 || fWholeDegreesFreedom1 < 1.0 || fWholeDegreesFreedom2 < 1.0
        || fWholeDegreesFreedom1 >= 1.0e10 || fWholeDegreesFreedom2 >= 1.0e10)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
    if (bCumulative)
    {
        const auto aRightTail = core::math::evaluateFRightTailDistribution(
            fRatio, fWholeDegreesFreedom1, fWholeDegreesFreedom2);
        if (!aRightTail)
            return aRightTail;
        return api::ValueResult<double>::success(1.0 - aRightTail.maValue);
    }

    return api::ValueResult<double>::success(
        std::pow(fWholeDegreesFreedom1 / fWholeDegreesFreedom2, fWholeDegreesFreedom1 / 2.0)
        * std::pow(fRatio, (fWholeDegreesFreedom1 / 2.0) - 1.0)
        / (std::pow(1.0 + (fRatio * fWholeDegreesFreedom1 / fWholeDegreesFreedom2),
                    (fWholeDegreesFreedom1 + fWholeDegreesFreedom2) / 2.0)
           * core::math::betaValue(
               fWholeDegreesFreedom1 / 2.0, fWholeDegreesFreedom2 / 2.0)));
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyChiDist(
    double fChi, double fDegreesFreedom, bool bOdfSyntax)
{
    const double fWholeDegreesFreedom = ::rtl::math::approxFloor(fDegreesFreedom);
    if (fWholeDegreesFreedom < 1.0 || (!bOdfSyntax && fChi < 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return core::math::evaluateLegacyChiDist(fChi, fWholeDegreesFreedom);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyGammaDist(
    double fX, double fAlpha, double fBeta, bool bCumulative, bool bOdfSyntax)
{
    if ((!bOdfSyntax && fX < 0.0) || fAlpha <= 0.0 || fBeta <= 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return core::math::evaluateGammaDistribution(
        fX, fAlpha, fBeta, bCumulative, !bOdfSyntax);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyTInv(
    double fProbability, double fDegreesFreedom, int nType)
{
    const double fWholeDegreesFreedom = ::rtl::math::approxFloor(fDegreesFreedom);
    if (fWholeDegreesFreedom < 1.0 || fProbability <= 0.0 || fProbability > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (nType == 4)
    {
        if (fProbability == 1.0)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        const double fMirroredProbability
            = fProbability < 0.5 ? 1.0 - fProbability : fProbability;
        const auto aResult
            = core::math::evaluateTInverse(fMirroredProbability, fWholeDegreesFreedom, nType);
        if (!aResult)
            return aResult;
        return api::ValueResult<double>::success(
            fProbability < 0.5 ? -aResult.maValue : aResult.maValue);
    }

    return core::math::evaluateTInverse(fProbability, fWholeDegreesFreedom, nType);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyFInv(
    double fProbability, double fDegreesFreedom1, double fDegreesFreedom2, bool bLeftTail)
{
    const double fWholeDegreesFreedom1 = ::rtl::math::approxFloor(fDegreesFreedom1);
    const double fWholeDegreesFreedom2 = ::rtl::math::approxFloor(fDegreesFreedom2);
    if (fProbability <= 0.0 || fProbability > 1.0 || fWholeDegreesFreedom1 < 1.0
        || fWholeDegreesFreedom2 < 1.0 || fWholeDegreesFreedom1 >= 1.0e10
        || fWholeDegreesFreedom2 >= 1.0e10)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    return core::math::evaluateFInverseRightTail(
        bLeftTail ? 1.0 - fProbability : fProbability,
        fWholeDegreesFreedom1, fWholeDegreesFreedom2);
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyChiInv(
    double fProbability, double fDegreesFreedom)
{
    return core::math::evaluateLegacyChiInverse(
        fProbability, ::rtl::math::approxFloor(fDegreesFreedom));
}

[[nodiscard]] inline api::ValueResult<double> evaluateLegacyChiSqInv(
    double fProbability, double fDegreesFreedom)
{
    return core::math::evaluateChiSquareInverse(
        fProbability, ::rtl::math::approxFloor(fDegreesFreedom));
}

} // namespace spreadsheetengine::compat::libreoffice::interpreterdispatch

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
