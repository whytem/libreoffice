/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/MathStatistical.hxx>

#include <cmath>
#include <functional>
#include <limits>

#include <rtl/math.hxx>

#include <kahan.hxx>

namespace spreadsheetengine::core::math
{
namespace
{

[[nodiscard]] double lanczosSum(double fZ)
{
    static constexpr double fNum[13] = {
        23531376880.41075968857200767445163675473,
        42919803642.64909876895789904700198885093,
        35711959237.35566804944018545154716670596,
        17921034426.03720969991975575445893111267,
        6039542586.35202800506429164430729792107,
        1439720407.311721673663223072794912393972,
        248874557.8620541565114603864132294232163,
        31426415.58540019438061423162831820536287,
        2876370.628935372441225409051620849613599,
        186056.2653952234950402949897160456992822,
        8071.672002365816210638002902272250613822,
        210.8242777515793458725097339207133627117,
        2.506628274631000270164908177133837338626
    };
    static constexpr double fDenom[13] = {
        0.0,
        39916800.0,
        120543840.0,
        150917976.0,
        105258076.0,
        45995730.0,
        13339535.0,
        2637558.0,
        357423.0,
        32670.0,
        1925.0,
        66.0,
        1.0
    };

    double fSumNum;
    double fSumDenom;
    if (fZ <= 1.0)
    {
        fSumNum = fNum[12];
        fSumDenom = fDenom[12];
        for (int nIndex = 11; nIndex >= 0; --nIndex)
        {
            fSumNum *= fZ;
            fSumNum += fNum[nIndex];
            fSumDenom *= fZ;
            fSumDenom += fDenom[nIndex];
        }
    }
    else
    {
        const double fInverse = 1.0 / fZ;
        fSumNum = fNum[0];
        fSumDenom = fDenom[0];
        for (int nIndex = 1; nIndex <= 12; ++nIndex)
        {
            fSumNum *= fInverse;
            fSumNum += fNum[nIndex];
            fSumDenom *= fInverse;
            fSumDenom += fDenom[nIndex];
        }
    }

    return fSumNum / fSumDenom;
}

[[nodiscard]] double logBeta(double fAlpha, double fBeta)
{
    double fA = fAlpha;
    double fB = fBeta;
    if (fB > fA)
        std::swap(fA, fB);

    constexpr double fMaxGammaArgument = 171.624376956302;
    if (fA + fB < fMaxGammaArgument)
        return std::log(betaValue(fA, fB));

    constexpr long double fG = 6.024680040776729583740234375L;
    const long double fGMinusHalf = fG - 0.5L;
    long double fLanczos = static_cast<long double>(lanczosSum(fA));
    fLanczos /= static_cast<long double>(lanczosSum(fA + fB));
    fLanczos *= static_cast<long double>(lanczosSum(fB));
    long double fLogLanczos = std::log(fLanczos);
    const long double fABG = static_cast<long double>(fA + fB) + fGMinusHalf;
    fLogLanczos += 0.5L
                   * (std::log(fABG) - std::log(static_cast<long double>(fA) + fGMinusHalf)
                      - std::log(static_cast<long double>(fB) + fGMinusHalf));
    const long double fTempA
        = static_cast<long double>(fB) / (static_cast<long double>(fA) + fGMinusHalf);
    const long double fTempB
        = static_cast<long double>(fA) / (static_cast<long double>(fB) + fGMinusHalf);
    return static_cast<double>(-static_cast<long double>(fA) * std::log1p(fTempA)
                               - static_cast<long double>(fB) * std::log1p(fTempB)
                               - fGMinusHalf + fLogLanczos);
}

[[nodiscard]] double betaPdf(double fX, double fAlpha, double fBeta)
{
    if (fAlpha == 1.0)
    {
        if (fBeta == 1.0)
            return 1.0;
        if (fBeta == 2.0)
            return -2.0 * fX + 2.0;
        if (fX == 1.0 && fBeta < 1.0)
            return HUGE_VAL;
        if (fX <= 0.01)
            return fBeta + fBeta * std::expm1((fBeta - 1.0) * std::log1p(-fX));
        return fBeta * std::pow((0.5 - fX) + 0.5, fBeta - 1.0);
    }
    if (fBeta == 1.0)
    {
        if (fAlpha == 2.0)
            return fAlpha * fX;
        if (fX == 0.0 && fAlpha < 1.0)
            return HUGE_VAL;
        return fAlpha * std::pow(fX, fAlpha - 1.0);
    }

    if (fX <= 0.0)
    {
        if (fX == 0.0 && fAlpha < 1.0)
            return HUGE_VAL;
        return 0.0;
    }
    if (fX >= 1.0)
    {
        if (fX == 1.0 && fBeta < 1.0)
            return HUGE_VAL;
        return 0.0;
    }

    const double fLogDoubleMax = std::log(std::numeric_limits<double>::max());
    const double fLogDoubleMin = std::log(std::numeric_limits<double>::min());
    const double fLogY = fX < 0.1 ? std::log1p(-fX) : std::log((0.5 - fX) + 0.5);
    const double fLogX = std::log(fX);
    const double fAlphaMinusOneLogX = (fAlpha - 1.0) * fLogX;
    const double fBetaMinusOneLogY = (fBeta - 1.0) * fLogY;
    const double fLogBeta = logBeta(fAlpha, fBeta);
    if (fAlphaMinusOneLogX < fLogDoubleMax && fAlphaMinusOneLogX > fLogDoubleMin
        && fBetaMinusOneLogY < fLogDoubleMax && fBetaMinusOneLogY > fLogDoubleMin
        && fLogBeta < fLogDoubleMax && fLogBeta > fLogDoubleMin
        && fAlphaMinusOneLogX + fBetaMinusOneLogY < fLogDoubleMax
        && fAlphaMinusOneLogX + fBetaMinusOneLogY > fLogDoubleMin)
    {
        return std::pow(fX, fAlpha - 1.0) * std::pow((0.5 - fX) + 0.5, fBeta - 1.0)
               / betaValue(fAlpha, fBeta);
    }

    return static_cast<double>(std::exp((static_cast<long double>(fAlpha) - 1.0L)
                                            * static_cast<long double>(fLogX)
                                        + (static_cast<long double>(fBeta) - 1.0L)
                                              * static_cast<long double>(fLogY)
                                        - static_cast<long double>(fLogBeta)));
}

[[nodiscard]] double betaContinuedFraction(double fX, double fAlpha, double fBeta)
{
    double fA1 = 1.0;
    double fB1 = 1.0;
    double fB2 = 1.0 - (fAlpha + fBeta) / (fAlpha + 1.0) * fX;
    double fA2 = 1.0;
    double fNorm = 1.0;
    double fCurrent = 1.0;
    if (!::rtl::math::approxEqual(fB2, 0.0))
    {
        fNorm = 1.0 / fB2;
        fCurrent = fA2 * fNorm;
    }
    else
        fA2 = 0.0;

    double fNext = fCurrent;
    for (double fM = 1.0; fM < 50000.0; fM += 1.0)
    {
        const double fAlphaPlus2M = fAlpha + 2.0 * fM;
        const double fEven = fM * (fBeta - fM) * fX / ((fAlphaPlus2M - 1.0) * fAlphaPlus2M);
        const double fOdd = -(fAlpha + fM) * (fAlpha + fBeta + fM) * fX
                            / (fAlphaPlus2M * (fAlphaPlus2M + 1.0));
        fA1 = (fA2 + fEven * fA1) * fNorm;
        fB1 = (fB2 + fEven * fB1) * fNorm;
        fA2 = fA1 + fOdd * fA2 * fNorm;
        fB2 = fB1 + fOdd * fB2 * fNorm;
        if (::rtl::math::approxEqual(fB2, 0.0))
            continue;

        fNorm = 1.0 / fB2;
        fNext = fA2 * fNorm;
        if (std::abs(fCurrent - fNext)
            <= std::abs(fCurrent) * std::numeric_limits<double>::epsilon())
        {
            return fNext;
        }
        fCurrent = fNext;
    }

    return fCurrent;
}

[[nodiscard]] double binomialLogPmf(double fSuccesses, double fTrials, double fProbability)
{
    return std::lgamma(fTrials + 1.0) - std::lgamma(fSuccesses + 1.0)
           - std::lgamma(fTrials - fSuccesses + 1.0) + fSuccesses * std::log(fProbability)
           + (fTrials - fSuccesses) * std::log1p(-fProbability);
}

[[nodiscard]] double sumExpProbabilityRange(
    sal_Int32 nStart, sal_Int32 nEnd, const std::function<double(sal_Int32)>& rLogProbability)
{
    if (nEnd < nStart)
        return 0.0;

    double fMaxLog = -std::numeric_limits<double>::infinity();
    for (sal_Int32 nIndex = nStart; nIndex <= nEnd; ++nIndex)
        fMaxLog = std::max(fMaxLog, rLogProbability(nIndex));
    if (!std::isfinite(fMaxLog))
        return 0.0;

    KahanSum fSum = 0.0;
    for (sal_Int32 nIndex = nStart; nIndex <= nEnd; ++nIndex)
        fSum += std::exp(rLogProbability(nIndex) - fMaxLog);
    return std::exp(fMaxLog) * fSum.get();
}

} // namespace

api::ValueResult<double> fisherTransform(double fValue)
{
    if (fValue <= -1.0 || fValue >= 1.0)
        return api::ValueResult<double>::failure(api::Error::Domain);

    return api::ValueResult<double>::success(
        0.5 * std::log((1.0 + fValue) / (1.0 - fValue)));
}

double inverseFisherTransform(double fValue)
{
    return std::tanh(fValue);
}

double betaValue(double fAlpha, double fBeta)
{
    double fA = fAlpha;
    double fB = fBeta;
    if (fB > fA)
        std::swap(fA, fB);

    constexpr double fMaxGammaArgument = 171.624376956302;
    if (fA + fB < fMaxGammaArgument)
        return (std::tgamma(fA) / std::tgamma(fA + fB)) * std::tgamma(fB);

    constexpr long double fG = 6.024680040776729583740234375L;
    const long double fGMinusHalf = fG - 0.5L;
    long double fLanczos = static_cast<long double>(lanczosSum(fA));
    fLanczos /= static_cast<long double>(lanczosSum(fA + fB));
    fLanczos *= static_cast<long double>(lanczosSum(fB));
    const long double fABG = static_cast<long double>(fA + fB) + fGMinusHalf;
    fLanczos *= std::sqrt(
        (fABG / (static_cast<long double>(fA) + fGMinusHalf))
        / (static_cast<long double>(fB) + fGMinusHalf));
    const long double fTempA
        = static_cast<long double>(fB) / (static_cast<long double>(fA) + fGMinusHalf);
    const long double fTempB
        = static_cast<long double>(fA) / (static_cast<long double>(fB) + fGMinusHalf);
    const long double fResult = std::exp(-static_cast<long double>(fA) * std::log1p(fTempA)
                                         - static_cast<long double>(fB) * std::log1p(fTempB)
                                         - fGMinusHalf)
                                * fLanczos;
    return static_cast<double>(fResult);
}

double betaCdf(double fInput, double fAlpha, double fBeta)
{
    if (fInput <= 0.0)
        return 0.0;
    if (fInput >= 1.0)
        return 1.0;
    if (fBeta == 1.0)
        return std::pow(fInput, fAlpha);
    if (fAlpha == 1.0)
        return -std::expm1(fBeta * std::log1p(-fInput));

    double fX = fInput;
    double fY = 1.0 - fInput;
    double fLnX = std::log(fInput);
    double fLnY = std::log1p(-fInput);
    double fA = fAlpha;
    double fB = fBeta;
    const bool bReflect = fInput > fAlpha / (fAlpha + fBeta);
    if (bReflect)
    {
        fA = fBeta;
        fB = fAlpha;
        fX = fY;
        fY = fInput;
        fLnX = fLnY;
        fLnY = std::log(fInput);
    }

    double fResult = betaContinuedFraction(fX, fA, fB) / fA;
    const double fP = fA / (fA + fB);
    const double fQ = fB / (fA + fB);
    double fScale = 0.0;
    if (fA > 1.0 && fB > 1.0 && fP < 0.97 && fQ < 0.97)
        fScale = betaPdf(fX, fA, fB) * fX * fY;
    else
        fScale = std::exp(fA * fLnX + fB * fLnY - logBeta(fA, fB));
    fResult *= fScale;
    if (bReflect)
        fResult = 1.0 - fResult;
    return std::clamp(fResult, 0.0, 1.0);
}

api::ValueResult<double> evaluateBetaDistribution(
    double fX, double fAlpha, double fBeta, double fLowerBound, double fUpperBound,
    bool bCumulative, bool bMicrosoftOrder)
{
    const double fScale = fUpperBound - fLowerBound;
    if (fScale <= 0.0 || fAlpha <= 0.0 || fBeta <= 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (bCumulative)
    {
        if (!bMicrosoftOrder)
        {
            if (fX < fLowerBound)
                return api::ValueResult<double>::success(0.0);
            if (fX > fUpperBound)
                return api::ValueResult<double>::success(1.0);
        }
        else if (fX < fLowerBound || fX > fUpperBound)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const double fStandardX = (fX - fLowerBound) / fScale;
        return api::ValueResult<double>::success(betaCdf(fStandardX, fAlpha, fBeta));
    }

    if (!bMicrosoftOrder)
    {
        if (fX < fLowerBound || fX > fUpperBound)
            return api::ValueResult<double>::success(0.0);
    }
    else if (fX < fLowerBound || fX > fUpperBound)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const double fStandardX = (fX - fLowerBound) / fScale;
    if ((::rtl::math::approxEqual(fStandardX, 0.0) && fAlpha < 1.0)
        || (::rtl::math::approxEqual(fStandardX, 1.0) && fBeta < 1.0))
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    return api::ValueResult<double>::success(betaPdf(fStandardX, fAlpha, fBeta) / fScale);
}

api::ValueResult<double> evaluatePoissonDistribution(
    double fX, double fLambda, bool bCumulative)
{
    if (fLambda <= 0.0 || fX < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const sal_Int32 nX = static_cast<sal_Int32>(::rtl::math::approxFloor(fX));
    const auto logProbability = [fLambda](sal_Int32 nValue) {
        return static_cast<double>(nValue) * std::log(fLambda) - fLambda
               - std::lgamma(static_cast<double>(nValue) + 1.0);
    };

    if (!bCumulative)
        return api::ValueResult<double>::success(std::exp(logProbability(nX)));

    return api::ValueResult<double>::success(
        std::min(1.0, sumExpProbabilityRange(0, nX, logProbability)));
}

api::ValueResult<double> evaluateBinomialDistribution(
    double fSuccesses, double fTrials, double fProbability, bool bCumulative)
{
    const double fN = ::rtl::math::approxFloor(fTrials);
    const double fX = ::rtl::math::approxFloor(fSuccesses);
    if (fN < 0.0 || fX < 0.0 || fX > fN || fProbability < 0.0 || fProbability > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (::rtl::math::approxEqual(fProbability, 0.0))
    {
        return api::ValueResult<double>::success(
            (::rtl::math::approxEqual(fX, 0.0) || bCumulative) ? 1.0 : 0.0);
    }
    if (::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(::rtl::math::approxEqual(fX, fN) ? 1.0 : 0.0);

    const sal_Int32 nN = static_cast<sal_Int32>(fN);
    const sal_Int32 nX = static_cast<sal_Int32>(fX);
    const double fQ = (0.5 - fProbability) + 0.5;
    auto accumulateRange = [&](sal_Int32 nStart, sal_Int32 nEnd, double fTerm,
                               double fNumeratorProbability,
                               double fDenominatorProbability) {
        for (sal_Int32 nIndex = 1; nIndex <= nStart && fTerm > 0.0; ++nIndex)
            fTerm *= (fN - static_cast<double>(nIndex) + 1.0) / static_cast<double>(nIndex)
                     * fNumeratorProbability / fDenominatorProbability;

        KahanSum fSum = fTerm;
        for (sal_Int32 nIndex = nStart + 1; nIndex <= nEnd && fTerm > 0.0; ++nIndex)
        {
            fTerm *= (fN - static_cast<double>(nIndex) + 1.0) / static_cast<double>(nIndex)
                     * fNumeratorProbability / fDenominatorProbability;
            fSum += fTerm;
        }
        return std::min(1.0, fSum.get());
    };

    const auto logProbability = [fN, fProbability](sal_Int32 nValue) {
        return binomialLogPmf(static_cast<double>(nValue), fN, fProbability);
    };

    if (!bCumulative)
        return api::ValueResult<double>::success(std::exp(logProbability(nX)));

    if (nX == nN)
        return api::ValueResult<double>::success(1.0);

    const double fLowTerm = std::pow(fQ, fN);
    if (fLowTerm > std::numeric_limits<double>::min())
    {
        return api::ValueResult<double>::success(
            accumulateRange(0, nX, fLowTerm, fProbability, fQ));
    }

    const double fHighTerm = std::pow(fProbability, fN);
    if (fHighTerm > std::numeric_limits<double>::min())
    {
        const double fTail = accumulateRange(0, nN - nX - 1, fHighTerm, fQ, fProbability);
        return api::ValueResult<double>::success(std::max(0.0, 1.0 - fTail));
    }

    return api::ValueResult<double>::success(
        std::min(1.0, sumExpProbabilityRange(0, nX, logProbability)));
}

api::ValueResult<double> evaluateBinomialRangeDistribution(
    double fTrials, double fProbability, double fSuccessStart, double fSuccessEnd)
{
    const double fN = ::rtl::math::approxFloor(fTrials);
    const double fStart = ::rtl::math::approxFloor(fSuccessStart);
    const double fEnd = ::rtl::math::approxFloor(fSuccessEnd);
    if (fN < 0.0 || fStart < 0.0 || fStart > fEnd || fEnd > fN || fProbability < 0.0
        || fProbability > 1.0)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    if (::rtl::math::approxEqual(fProbability, 0.0))
        return api::ValueResult<double>::success(::rtl::math::approxEqual(fStart, 0.0) ? 1.0 : 0.0);
    if (::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(::rtl::math::approxEqual(fEnd, fN) ? 1.0 : 0.0);

    const sal_Int32 nN = static_cast<sal_Int32>(fN);
    const sal_Int32 nStart = static_cast<sal_Int32>(fStart);
    const sal_Int32 nEnd = static_cast<sal_Int32>(fEnd);
    const double fQ = (0.5 - fProbability) + 0.5;
    auto accumulateRange = [&](sal_Int32 nRangeStart, sal_Int32 nRangeEnd, double fTerm,
                               double fNumeratorProbability,
                               double fDenominatorProbability) {
        for (sal_Int32 nIndex = 1; nIndex <= nRangeStart && fTerm > 0.0; ++nIndex)
            fTerm *= (fN - static_cast<double>(nIndex) + 1.0) / static_cast<double>(nIndex)
                     * fNumeratorProbability / fDenominatorProbability;

        KahanSum fSum = fTerm;
        for (sal_Int32 nIndex = nRangeStart + 1; nIndex <= nRangeEnd && fTerm > 0.0; ++nIndex)
        {
            fTerm *= (fN - static_cast<double>(nIndex) + 1.0) / static_cast<double>(nIndex)
                     * fNumeratorProbability / fDenominatorProbability;
            fSum += fTerm;
        }
        return std::min(1.0, fSum.get());
    };

    const double fLowTerm = std::pow(fQ, fN);
    if (fLowTerm > std::numeric_limits<double>::min())
    {
        return api::ValueResult<double>::success(
            accumulateRange(nStart, nEnd, fLowTerm, fProbability, fQ));
    }

    const double fHighTerm = std::pow(fProbability, fN);
    if (fHighTerm > std::numeric_limits<double>::min())
    {
        return api::ValueResult<double>::success(
            accumulateRange(nN - nEnd, nN - nStart, fHighTerm, fQ, fProbability));
    }

    const auto logProbability = [fN, fProbability](sal_Int32 nValue) {
        return binomialLogPmf(static_cast<double>(nValue), fN, fProbability);
    };
    return api::ValueResult<double>::success(sumExpProbabilityRange(nStart, nEnd, logProbability));
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
