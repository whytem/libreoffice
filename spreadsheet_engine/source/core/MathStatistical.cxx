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

constexpr double fMaxGammaArgument = 171.624376956302;
constexpr double fLanczosG = 6.024680040776729583740234375;

[[nodiscard]] double gammaHelperPositive(double fZ)
{
    double fGamma = lanczosSum(fZ);
    const double fZgHelp = fZ + fLanczosG - 0.5;
    const double fHalfpower = std::pow(fZgHelp, fZ / 2.0 - 0.25);
    fGamma *= fHalfpower;
    fGamma /= std::exp(fZgHelp);
    fGamma *= fHalfpower;
    if (fZ <= 20.0 && fZ == ::rtl::math::approxFloor(fZ))
        fGamma = ::rtl::math::round(fGamma);
    return fGamma;
}

[[nodiscard]] double logGammaHelperPositive(double fZ)
{
    const double fZgHelp = fZ + fLanczosG - 0.5;
    return std::log(lanczosSum(fZ)) + (fZ - 0.5) * std::log(fZgHelp) - fZgHelp;
}

[[nodiscard]] double logGammaValuePositive(double fZ)
{
    if (fZ >= fMaxGammaArgument)
        return logGammaHelperPositive(fZ);
    if (fZ >= 1.0)
        return std::log(gammaHelperPositive(fZ));
    if (fZ >= 0.5)
        return std::log(gammaHelperPositive(fZ + 1.0) / fZ);
    return logGammaHelperPositive(fZ + 2.0) - std::log1p(fZ) - std::log(fZ);
}

[[nodiscard]] double logBetaInternal(double fAlpha, double fBeta)
{
    double fA;
    double fB;
    if (fAlpha > fBeta)
    {
        fA = fAlpha;
        fB = fBeta;
    }
    else
    {
        fA = fBeta;
        fB = fAlpha;
    }

    const double fGMinusHalf = fLanczosG - 0.5;
    double fLanczos = lanczosSum(fA);
    fLanczos /= lanczosSum(fA + fB);
    fLanczos *= lanczosSum(fB);
    double fLogLanczos = std::log(fLanczos);
    const double fABG = fA + fB + fGMinusHalf;
    fLogLanczos += 0.5
                   * (std::log(fABG) - std::log(fA + fGMinusHalf)
                      - std::log(fB + fGMinusHalf));
    const double fTempA = fB / (fA + fGMinusHalf);
    const double fTempB = fA / (fB + fGMinusHalf);
    return -fA * std::log1p(fTempA) - fB * std::log1p(fTempB) - fGMinusHalf + fLogLanczos;
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
    const double fLogBeta = logBetaInternal(fAlpha, fBeta);
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
    return logGammaValuePositive(fTrials + 1.0) - logGammaValuePositive(fSuccesses + 1.0)
           - logGammaValuePositive(fTrials - fSuccesses + 1.0)
           + fSuccesses * std::log(fProbability)
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

[[nodiscard]] double phiValue(double fValue)
{
    return 0.39894228040143268 * std::exp(-(fValue * fValue) / 2.0);
}

[[nodiscard]] double taylorPolynomial(const double* pPolynomial, sal_uInt16 nMax, double fValue)
{
    double fResult = pPolynomial[nMax];
    for (short nIndex = nMax - 1; nIndex >= 0; --nIndex)
        fResult = (fResult * fValue) + pPolynomial[nIndex];
    return fResult;
}

[[nodiscard]] api::ValueResult<double> gammaContinuedFraction(double fAlpha, double fX)
{
    constexpr double fHalfMachEps = 0.5 * std::numeric_limits<double>::epsilon();
    const double fBigInv = std::numeric_limits<double>::epsilon();
    const double fBig = 1.0 / fBigInv;
    double fCount = 0.0;
    double fY = 1.0 - fAlpha;
    double fDenom = fX + 2.0 - fAlpha;
    double fPkm1 = fX + 1.0;
    double fPkm2 = 1.0;
    double fQkm1 = fDenom * fX;
    double fQkm2 = fX;
    double fApprox = fPkm1 / fQkm1;
    bool bFinished = false;
    do
    {
        fCount += 1.0;
        fY += 1.0;
        const double fNum = fY * fCount;
        fDenom += 2.0;
        double fPk = fPkm1 * fDenom - fPkm2 * fNum;
        const double fQk = fQkm1 * fDenom - fQkm2 * fNum;
        if (!::rtl::math::approxEqual(fQk, 0.0))
        {
            const double fR = fPk / fQk;
            bFinished = std::abs((fApprox - fR) / fR) <= fHalfMachEps;
            fApprox = fR;
        }

        fPkm2 = fPkm1;
        fPkm1 = fPk;
        fQkm2 = fQkm1;
        fQkm1 = fQk;
        if (std::abs(fPk) > fBig)
        {
            fPkm2 *= fBigInv;
            fPkm1 *= fBigInv;
            fQkm2 *= fBigInv;
            fQkm1 *= fBigInv;
        }
    } while (!bFinished && fCount < 10000.0);

    if (!bFinished)
        return api::ValueResult<double>::failure(api::Error::NoConvergence);
    return api::ValueResult<double>::success(fApprox);
}

[[nodiscard]] api::ValueResult<double> gammaSeries(double fAlpha, double fX)
{
    constexpr double fHalfMachEps = 0.5 * std::numeric_limits<double>::epsilon();
    double fDenomFactor = fAlpha;
    double fSummand = 1.0 / fAlpha;
    double fSum = fSummand;
    int nCount = 1;
    do
    {
        fDenomFactor += 1.0;
        fSummand *= fX / fDenomFactor;
        fSum += fSummand;
        ++nCount;
    } while (fSummand / fSum > fHalfMachEps && nCount <= 10000);

    if (nCount > 10000)
        return api::ValueResult<double>::failure(api::Error::NoConvergence);
    return api::ValueResult<double>::success(fSum);
}

[[nodiscard]] bool hasChangeOfSign(double fLeft, double fRight)
{
    return (fLeft < 0.0 && fRight > 0.0) || (fLeft > 0.0 && fRight < 0.0);
}

template <typename DistributionFn>
[[nodiscard]] api::ValueResult<double> iterateInverseCalcStyle(
    double fAx, double fBx, const DistributionFn& rFunction)
{
    constexpr double fYEps = 1.0E-307;
    constexpr double fXEps = std::numeric_limits<double>::epsilon();

    if (!(fAx < fBx))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    KahanSum fkAx = fAx;
    KahanSum fkBx = fBx;

    auto aAy = rFunction(fAx);
    if (!aAy)
        return aAy;
    auto aBy = rFunction(fBx);
    if (!aBy)
        return aBy;

    double fAy = aAy.maValue;
    double fBy = aBy.maValue;
    KahanSum fTemp = 0.0;
    unsigned short nCount = 0;
    for (; nCount < 1000 && !hasChangeOfSign(fAy, fBy); ++nCount)
    {
        if (std::abs(fAy) <= std::abs(fBy))
        {
            fTemp = fkAx;
            fkAx += (fkAx.get() - fkBx.get()) * 2.0;
            if (fkAx.get() < 0.0)
                fkAx = 0.0;
            fkBx = fTemp;
            fBy = fAy;
            aAy = rFunction(fkAx.get());
            if (!aAy)
                return aAy;
            fAy = aAy.maValue;
        }
        else
        {
            fTemp = fkBx;
            fkBx += (fkBx.get() - fkAx.get()) * 2.0;
            fkAx = fTemp;
            fAy = fBy;
            aBy = rFunction(fkBx.get());
            if (!aBy)
                return aBy;
            fBy = aBy.maValue;
        }
    }

    fAx = fkAx.get();
    fBx = fkBx.get();
    if (fAy == 0.0)
        return api::ValueResult<double>::success(fAx);
    if (fBy == 0.0)
        return api::ValueResult<double>::success(fBx);
    if (!hasChangeOfSign(fAy, fBy))
        return api::ValueResult<double>::failure(api::Error::NoConvergence);

    double fPx = fAx;
    double fPy = fAy;
    double fQx = fBx;
    double fQy = fBy;
    double fRx = fAx;
    double fRy = fAy;
    double fSx = 0.5 * (fAx + fBx);
    bool bHasToInterpolate = true;
    nCount = 0;
    while (nCount < 500 && std::abs(fRy) > fYEps
           && (fBx - fAx) > std::max(std::abs(fAx), std::abs(fBx)) * fXEps)
    {
        if (bHasToInterpolate)
        {
            if (fPy != fQy && fQy != fRy && fRy != fPy)
            {
                fSx = fPx * fRy * fQy / (fRy - fPy) / (fQy - fPy)
                      + fRx * fQy * fPy / (fQy - fRy) / (fPy - fRy)
                      + fQx * fPy * fRy / (fPy - fQy) / (fRy - fQy);
                bHasToInterpolate = (fAx < fSx) && (fSx < fBx);
            }
            else
            {
                bHasToInterpolate = false;
            }
        }
        if (!bHasToInterpolate)
        {
            fSx = 0.5 * (fAx + fBx);
            fQx = fBx;
            fQy = fBy;
            bHasToInterpolate = true;
        }

        fPx = fQx;
        fQx = fRx;
        fRx = fSx;
        fPy = fQy;
        fQy = fRy;

        const auto aRy = rFunction(fSx);
        if (!aRy)
            return aRy;
        fRy = aRy.maValue;

        if (hasChangeOfSign(fAy, fRy))
        {
            fBx = fRx;
            fBy = fRy;
        }
        else
        {
            fAx = fRx;
            fAy = fRy;
        }

        bHasToInterpolate = bHasToInterpolate && (std::abs(fRy) * 2.0 <= std::abs(fQy));
        ++nCount;
    }

    return api::ValueResult<double>::success(fRx);
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

    if (fA + fB < fMaxGammaArgument)
        return (std::tgamma(fA) / std::tgamma(fA + fB)) * std::tgamma(fB);

    constexpr long double fG = static_cast<long double>(fLanczosG);
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

double logBetaValue(double fAlpha, double fBeta)
{
    return logBetaInternal(fAlpha, fBeta);
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
        fScale = std::exp(fA * fLnX + fB * fLnY - logBetaInternal(fA, fB));
    fResult *= fScale;
    if (bReflect)
        fResult = 1.0 - fResult;
    return std::clamp(fResult, 0.0, 1.0);
}

double gaussValue(double fValue)
{
    const double fAbs = std::abs(fValue);
    const sal_uInt16 nBucket = static_cast<sal_uInt16>(::rtl::math::approxFloor(fAbs));

    double fResult = 0.0;
    if (nBucket == 0)
    {
        static const double aT0[] = { 0.39894228040143268, -0.06649038006690545,
            0.00997355701003582, -0.00118732821548045, 0.00011543468761616,
            -0.00000944465625950, 0.00000066596935163, -0.00000004122667415,
            0.00000000227352982, 0.00000000011301172, 0.00000000000511243,
            -0.00000000000021218 };
        fResult = taylorPolynomial(aT0, 11, fAbs * fAbs) * fAbs;
    }
    else if (nBucket <= 2)
    {
        static const double aT2[] = { 0.47724986805182079, 0.05399096651318805,
            -0.05399096651318805, 0.02699548325659403, -0.00449924720943234,
            -0.00224962360471617, 0.00134977416282970, -0.00011783742691370,
            -0.00011515930357476, 0.00003704737285544, 0.00000282690796889,
            -0.00000354513195524, 0.00000037669563126, 0.00000019202407921,
            -0.00000005226908590, -0.00000000491799345, 0.00000000366377919,
            -0.00000000015981997, -0.00000000017381238, 0.00000000002624031,
            0.00000000000560919, -0.00000000000172127, -0.00000000000008634,
            0.00000000000007894 };
        fResult = taylorPolynomial(aT2, 23, fAbs - 2.0);
    }
    else if (nBucket <= 4)
    {
        static const double aT4[] = { 0.49996832875816688, 0.00013383022576489,
            -0.00026766045152977, 0.00033457556441221, -0.00028996548915725,
            0.00018178605666397, -0.00008252863922168, 0.00002551802519049,
            -0.00000391665839292, -0.00000074018205222, 0.00000064422023359,
            -0.00000017370155340, 0.00000000909595465, 0.00000000944943118,
            -0.00000000329957075, 0.00000000029492075, 0.00000000011874477,
            -0.00000000004420396, 0.00000000000361422, 0.00000000000143638,
            -0.00000000000045848 };
        fResult = taylorPolynomial(aT4, 20, fAbs - 4.0);
    }
    else
    {
        static const double aAsympt[] = { -1.0, 1.0, -3.0, 15.0, -105.0 };
        fResult = 0.5
                  + phiValue(fAbs)
                        * (taylorPolynomial(aAsympt, 4, 1.0 / (fAbs * fAbs)) / fAbs);
    }

    return fValue < 0.0 ? -fResult : fResult;
}

api::ValueResult<double> evaluateStandardNormalInverse(double fProbability)
{
    if (fProbability < 0.0 || fProbability > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (::rtl::math::approxEqual(fProbability, 0.0)
        || ::rtl::math::approxEqual(fProbability, 1.0))
    {
        return api::ValueResult<double>::failure(api::Error::NoValue);
    }

    const double fQ = fProbability - 0.5;
    double fT = 0.0;
    double fZ = 0.0;

    if (std::abs(fQ) <= 0.425)
    {
        fT = 0.180625 - fQ * fQ;
        fZ = fQ
             * (((((((fT * 2509.0809287301226727 + 33430.575583588128105) * fT
                         + 67265.770927008700853)
                        * fT
                    + 45921.953931549871457)
                       * fT
                   + 13731.693765509461125)
                      * fT
                  + 1971.5909503065514427)
                     * fT
                 + 133.14166789178437745)
                    * fT
                + 3.387132872796366608)
               / (((((((fT * 5226.495278852854561 + 28729.085735721942674) * fT
                            + 39307.89580009271061)
                           * fT
                       + 21213.794301586595867)
                          * fT
                      + 5394.1960214247511077)
                         * fT
                     + 687.1870074920579083)
                        * fT
                    + 42.313330701600911252)
                       * fT
                   + 1.0);
    }
    else
    {
        fT = fQ > 0.0 ? 1.0 - fProbability : fProbability;
        fT = std::sqrt(-std::log(fT));
        if (fT <= 5.0)
        {
            fT -= 1.6;
            fZ = (((((((fT * 7.7454501427834140764e-4 + 0.0227238449892691845833) * fT
                            + 0.24178072517745061177)
                           * fT
                       + 1.27045825245236838258)
                          * fT
                      + 3.64784832476320460504)
                         * fT
                     + 5.7694972214606914055)
                        * fT
                    + 4.6303378461565452959)
                       * fT
                   + 1.42343711074968357734)
                  / (((((((fT * 1.05075007164441684324e-9 + 5.475938084995344946e-4) * fT
                               + 0.0151986665636164571966)
                              * fT
                          + 0.14810397642748007459)
                             * fT
                         + 0.68976733498510000455)
                            * fT
                        + 1.6763848301838038494)
                           * fT
                       + 2.05319162663775882187)
                          * fT
                      + 1.0);
        }
        else
        {
            fT -= 5.0;
            fZ = (((((((fT * 2.01033439929228813265e-7 + 2.71155556874348757815e-5) * fT
                            + 0.0012426609473880784386)
                           * fT
                       + 0.026532189526576123093)
                          * fT
                      + 0.29656057182850489123)
                         * fT
                     + 1.7848265399172913358)
                        * fT
                    + 5.4637849111641143699)
                       * fT
                   + 6.6579046435011037772)
                  / (((((((fT * 2.04426310338993978564e-15 + 1.4215117583164458887e-7) * fT
                               + 1.8463183175100546818e-5)
                              * fT
                          + 7.868691311456132591e-4)
                             * fT
                         + 0.0148753612908506148525)
                            * fT
                        + 0.13692988092273580531)
                           * fT
                       + 0.59983220655588793769)
                          * fT
                      + 1.0);
        }

        if (fQ < 0.0)
            fZ = -fZ;
    }

    return api::ValueResult<double>::success(fZ);
}

api::ValueResult<double> lowRegularizedIncompleteGamma(double fAlpha, double fX)
{
    const double fLnFactor = fAlpha * std::log(fX) - fX - logGammaValuePositive(fAlpha);
    const double fFactor = std::exp(fLnFactor);
    if (fX > fAlpha + 1.0)
    {
        const auto aContinuedFraction = gammaContinuedFraction(fAlpha, fX);
        if (!aContinuedFraction)
            return aContinuedFraction;
        return api::ValueResult<double>::success(1.0 - fFactor * aContinuedFraction.maValue);
    }

    const auto aSeries = gammaSeries(fAlpha, fX);
    if (!aSeries)
        return aSeries;
    return api::ValueResult<double>::success(fFactor * aSeries.maValue);
}

api::ValueResult<double> upRegularizedIncompleteGamma(double fAlpha, double fX)
{
    const double fLnFactor = fAlpha * std::log(fX) - fX - logGammaValuePositive(fAlpha);
    const double fFactor = std::exp(fLnFactor);
    if (fX > fAlpha + 1.0)
    {
        const auto aContinuedFraction = gammaContinuedFraction(fAlpha, fX);
        if (!aContinuedFraction)
            return aContinuedFraction;
        return api::ValueResult<double>::success(fFactor * aContinuedFraction.maValue);
    }

    const auto aSeries = gammaSeries(fAlpha, fX);
    if (!aSeries)
        return aSeries;
    return api::ValueResult<double>::success(1.0 - fFactor * aSeries.maValue);
}

api::ValueResult<double> evaluateLegacyChiDist(double fChi, double fDegreesFreedom)
{
    if (fDegreesFreedom < 1.0 || fChi < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (fChi <= 0.0)
        return api::ValueResult<double>::success(1.0);
    return upRegularizedIncompleteGamma(fDegreesFreedom / 2.0, fChi / 2.0);
}

api::ValueResult<double> evaluateBinomialInverse(
    double fTrials, double fProbability, double fAlpha)
{
    const double fN = ::rtl::math::approxFloor(fTrials);
    if (fN < 0.0 || fProbability < 0.0 || fProbability > 1.0 || fAlpha < 0.0 || fAlpha > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (::rtl::math::approxEqual(fAlpha, 0.0))
        return api::ValueResult<double>::success(0.0);
    if (::rtl::math::approxEqual(fProbability, 0.0))
        return api::ValueResult<double>::success(0.0);
    if (::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(fN);
    if (::rtl::math::approxEqual(fAlpha, 1.0))
        return api::ValueResult<double>::success(fN);

    sal_Int32 nLow = 0;
    sal_Int32 nHigh = static_cast<sal_Int32>(fN);
    while (nLow < nHigh)
    {
        const sal_Int32 nMid = nLow + ((nHigh - nLow) / 2);
        const auto aDistribution = evaluateBinomialDistribution(
            static_cast<double>(nMid), fN, fProbability, true);
        if (!aDistribution)
            return aDistribution;

        if (aDistribution.maValue >= fAlpha
            || ::rtl::math::approxEqual(aDistribution.maValue, fAlpha))
        {
            nHigh = nMid;
        }
        else
        {
            nLow = nMid + 1;
        }
    }

    return api::ValueResult<double>::success(static_cast<double>(nLow));
}

api::ValueResult<double> evaluateNormalDistribution(
    double fX, double fMean, double fSigma, bool bCumulative)
{
    if (!(fSigma > 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    constexpr double fSqrtTwo = 1.4142135623730950488;
    constexpr double fInvSqrtTwoPi = 0.39894228040143267794;
    const double fZ = (fX - fMean) / fSigma;
    if (bCumulative)
    {
        return api::ValueResult<double>::success(
            std::clamp(0.5 * std::erfc(-fZ / fSqrtTwo), 0.0, 1.0));
    }

    return api::ValueResult<double>::success(
        std::exp(-0.5 * fZ * fZ) * fInvSqrtTwoPi / fSigma);
}

api::ValueResult<double> evaluateNormalInverse(
    double fProbability, double fMean, double fSigma)
{
    if (!(fSigma > 0.0) || fProbability < 0.0 || fProbability > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const auto aStandard = evaluateStandardNormalInverse(fProbability);
    if (!aStandard)
        return aStandard;
    return api::ValueResult<double>::success(aStandard.maValue * fSigma + fMean);
}

api::ValueResult<double> evaluateLogNormalDistribution(
    double fX, double fMean, double fSigma, bool bCumulative)
{
    if (!(fSigma > 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (bCumulative)
    {
        if (fX <= 0.0)
            return api::ValueResult<double>::success(0.0);
        return evaluateNormalDistribution(std::log(fX), fMean, fSigma, true);
    }

    if (!(fX > 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    constexpr double fInvSqrtTwoPi = 0.39894228040143267794;
    const double fZ = (std::log(fX) - fMean) / fSigma;
    return api::ValueResult<double>::success(
        std::exp(-0.5 * fZ * fZ) * fInvSqrtTwoPi / (fSigma * fX));
}

api::ValueResult<double> evaluateLogNormalInverse(
    double fProbability, double fMean, double fSigma)
{
    if (!(fSigma > 0.0) || !(fProbability > 0.0) || !(fProbability < 1.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const auto aStandard = evaluateStandardNormalInverse(fProbability);
    if (!aStandard)
        return aStandard;
    return api::ValueResult<double>::success(std::exp(fMean + fSigma * aStandard.maValue));
}

api::ValueResult<double> evaluateChiSquareDistribution(
    double fX, double fDegreesFreedom, bool bCumulative, bool bMicrosoftSyntax)
{
    if (fDegreesFreedom < 1.0 || (bMicrosoftSyntax && fX < 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (bCumulative)
    {
        if (fX <= 0.0)
            return api::ValueResult<double>::success(0.0);
        return lowRegularizedIncompleteGamma(fDegreesFreedom / 2.0, fX / 2.0);
    }

    if (fX <= 0.0)
        return api::ValueResult<double>::success(0.0);

    const double fHalfDf = fDegreesFreedom / 2.0;
    const double fLogValue = (fHalfDf - 1.0) * std::log(fX * 0.5) - (fX / 2.0)
                             - std::log(2.0) - logGammaValuePositive(fHalfDf);
    return api::ValueResult<double>::success(std::exp(fLogValue));
}

api::ValueResult<double> evaluateChiSquareInverse(double fProbability, double fDegreesFreedom)
{
    if (fDegreesFreedom < 1.0 || fProbability < 0.0 || fProbability >= 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (::rtl::math::approxEqual(fProbability, 0.0))
        return api::ValueResult<double>::success(0.0);

    return iterateInverseCalcStyle(fDegreesFreedom * 0.5, fDegreesFreedom,
        [fProbability, fDegreesFreedom](double fX) {
            const auto aDistribution = evaluateChiSquareDistribution(fX, fDegreesFreedom, true, false);
            if (!aDistribution)
                return aDistribution;
            return api::ValueResult<double>::success(fProbability - aDistribution.maValue);
        });
}

api::ValueResult<double> evaluateGammaDistribution(
    double fX, double fAlpha, double fBeta, bool bCumulative, bool bMicrosoftSyntax)
{
    if (fAlpha <= 0.0 || fBeta <= 0.0 || (bMicrosoftSyntax && fX < 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (bCumulative)
    {
        if (fX <= 0.0)
            return api::ValueResult<double>::success(0.0);
        return lowRegularizedIncompleteGamma(fAlpha, fX / fBeta);
    }

    if (fX < 0.0)
        return api::ValueResult<double>::success(0.0);

    if (::rtl::math::approxEqual(fX, 0.0))
    {
        if (fAlpha < 1.0)
            return api::ValueResult<double>::failure(api::Error::DivisionByZero);
        if (::rtl::math::approxEqual(fAlpha, 1.0))
            return api::ValueResult<double>::success(1.0 / fBeta);
        return api::ValueResult<double>::success(0.0);
    }

    const double fScaledX = fX / fBeta;
    const double fLogValue = (fAlpha - 1.0) * std::log(fScaledX) - fScaledX - std::log(fBeta)
                             - logGammaValuePositive(fAlpha);
    return api::ValueResult<double>::success(std::exp(fLogValue));
}

api::ValueResult<double> evaluateGammaInverse(
    double fProbability, double fAlpha, double fBeta)
{
    if (fAlpha <= 0.0 || fBeta <= 0.0 || fProbability < 0.0 || fProbability >= 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (::rtl::math::approxEqual(fProbability, 0.0))
        return api::ValueResult<double>::success(0.0);

    const double fStart = fAlpha * fBeta;
    return iterateInverseCalcStyle(fStart * 0.5, fStart,
        [fProbability, fAlpha, fBeta](double fX) {
            const auto aDistribution = evaluateGammaDistribution(fX, fAlpha, fBeta, true, false);
            if (!aDistribution)
                return aDistribution;
            return api::ValueResult<double>::success(fProbability - aDistribution.maValue);
        });
}

api::ValueResult<double> evaluateGammaValue(double fX)
{
    const double fWhole = ::rtl::math::approxFloor(fX);
    if (fX <= 0.0 && ::rtl::math::approxEqual(fX, fWhole))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (fX > fMaxGammaArgument)
        return api::ValueResult<double>::failure(api::Error::Domain);

    const double fLogPi = std::log(M_PI);
    const double fLogDblMax = std::log(std::numeric_limits<double>::max());
    if (fX >= 1.0)
        return api::ValueResult<double>::success(gammaHelperPositive(fX));
    if (fX >= 0.5)
        return api::ValueResult<double>::success(gammaHelperPositive(fX + 1.0) / fX);
    if (fX >= -0.5)
    {
        const double fLogTest = logGammaHelperPositive(fX + 2.0) - std::log1p(fX)
                                - std::log(std::abs(fX));
        if (fLogTest >= fLogDblMax)
            return api::ValueResult<double>::failure(api::Error::Domain);
        return api::ValueResult<double>::success(gammaHelperPositive(fX + 2.0) / (fX + 1.0) / fX);
    }

    const double fSin = ::rtl::math::sin(M_PI * fX);
    const double fLogDivisor = logGammaHelperPositive(1.0 - fX) + std::log(std::abs(fSin));
    if (fLogDivisor - fLogPi >= fLogDblMax)
        return api::ValueResult<double>::success(0.0);
    if (fLogDivisor < 0.0 && fLogPi - fLogDivisor > fLogDblMax)
        return api::ValueResult<double>::failure(api::Error::Domain);

    return api::ValueResult<double>::success(
        std::exp(fLogPi - fLogDivisor) * (fSin < 0.0 ? -1.0 : 1.0));
}

api::ValueResult<double> evaluateLogGammaValue(double fX)
{
    if (!(fX > 0.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(logGammaValuePositive(fX));
}

api::ValueResult<double> evaluateStudentDistribution(
    double fT, double fDegreesFreedom, int nType)
{
    if (fDegreesFreedom < 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    switch (nType)
    {
        case 1:
            return api::ValueResult<double>::success(
                0.5 * betaCdf(fDegreesFreedom / (fDegreesFreedom + fT * fT),
                    fDegreesFreedom / 2.0, 0.5));
        case 2:
            return api::ValueResult<double>::success(betaCdf(
                fDegreesFreedom / (fDegreesFreedom + fT * fT), fDegreesFreedom / 2.0, 0.5));
        case 3:
            return api::ValueResult<double>::success(
                std::pow(1.0 + (fT * fT / fDegreesFreedom), -(fDegreesFreedom + 1.0) / 2.0)
                / (std::sqrt(fDegreesFreedom) * betaValue(0.5, fDegreesFreedom / 2.0)));
        case 4:
        {
            const double fX = fDegreesFreedom / (fT * fT + fDegreesFreedom);
            const double fRightHalf = 0.5 * betaCdf(fX, 0.5 * fDegreesFreedom, 0.5);
            return api::ValueResult<double>::success(fT < 0.0 ? fRightHalf : 1.0 - fRightHalf);
        }
        default:
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
}

api::ValueResult<double> evaluateTInverse(
    double fProbability, double fDegreesFreedom, int nType)
{
    if (fDegreesFreedom < 1.0 || fProbability <= 0.0 || fProbability > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if ((nType == 2 || nType == 4) && ::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(0.0);

    if (nType == 4)
    {
        if (fProbability >= 1.0)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        if (::rtl::math::approxEqual(fProbability, 0.5))
            return api::ValueResult<double>::success(0.0);
        if (fProbability < 0.5)
        {
            const auto aMirror = evaluateTInverse(1.0 - fProbability, fDegreesFreedom, nType);
            if (!aMirror)
                return aMirror;
            return api::ValueResult<double>::success(-aMirror.maValue);
        }

        return iterateInverseCalcStyle(fDegreesFreedom * 0.5, fDegreesFreedom,
            [fProbability, fDegreesFreedom](double fX) {
                const auto aDistribution = evaluateStudentDistribution(fX, fDegreesFreedom, 4);
                if (!aDistribution)
                    return aDistribution;
                return api::ValueResult<double>::success(fProbability - aDistribution.maValue);
            });
    }

    if (nType != 2)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    return iterateInverseCalcStyle(fDegreesFreedom * 0.5, fDegreesFreedom,
        [fProbability, fDegreesFreedom](double fX) {
            const auto aDistribution = evaluateStudentDistribution(fX, fDegreesFreedom, 2);
            if (!aDistribution)
                return aDistribution;
            return api::ValueResult<double>::success(fProbability - aDistribution.maValue);
        });
}

api::ValueResult<double> evaluateFRightTailDistribution(
    double fX, double fDegreesFreedom1, double fDegreesFreedom2)
{
    if (fX < 0.0 || fDegreesFreedom1 < 1.0 || fDegreesFreedom2 < 1.0
        || fDegreesFreedom1 >= 1.0e10 || fDegreesFreedom2 >= 1.0e10)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const double fArgument
        = fDegreesFreedom2 / (fDegreesFreedom2 + fDegreesFreedom1 * fX);
    return api::ValueResult<double>::success(
        betaCdf(fArgument, fDegreesFreedom2 / 2.0, fDegreesFreedom1 / 2.0));
}

api::ValueResult<double> evaluateFInverseRightTail(
    double fProbability, double fDegreesFreedom1, double fDegreesFreedom2)
{
    if (fProbability <= 0.0 || fProbability > 1.0 || fDegreesFreedom1 < 1.0
        || fDegreesFreedom2 < 1.0 || fDegreesFreedom1 >= 1.0e10
        || fDegreesFreedom2 >= 1.0e10)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    if (::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(0.0);

    return iterateInverseCalcStyle(fDegreesFreedom1 * 0.5, fDegreesFreedom1,
        [fProbability, fDegreesFreedom1, fDegreesFreedom2](double fX) {
            const auto aDistribution
                = evaluateFRightTailDistribution(fX, fDegreesFreedom1, fDegreesFreedom2);
            if (!aDistribution)
                return aDistribution;
            return api::ValueResult<double>::success(fProbability - aDistribution.maValue);
        });
}

api::ValueResult<double> evaluateLegacyChiInverse(
    double fProbability, double fDegreesFreedom)
{
    if (fDegreesFreedom < 1.0 || fProbability <= 0.0 || fProbability > 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(0.0);

    return iterateInverseCalcStyle(fDegreesFreedom * 0.5, fDegreesFreedom,
        [fProbability, fDegreesFreedom](double fX) {
            const auto aDistribution = evaluateLegacyChiDist(fX, fDegreesFreedom);
            if (!aDistribution)
                return aDistribution;
            return api::ValueResult<double>::success(fProbability - aDistribution.maValue);
        });
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

api::ValueResult<double> evaluateBetaInverse(
    double fProbability, double fAlpha, double fBeta, double fLowerBound, double fUpperBound)
{
    if (fProbability < 0.0 || fProbability > 1.0 || !(fLowerBound < fUpperBound)
        || fAlpha <= 0.0 || fBeta <= 0.0)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
    if (::rtl::math::approxEqual(fProbability, 0.0))
        return api::ValueResult<double>::success(fLowerBound);
    if (::rtl::math::approxEqual(fProbability, 1.0))
        return api::ValueResult<double>::success(fUpperBound);

    const auto aStandard = iterateInverseCalcStyle(0.0, 1.0,
        [fProbability, fAlpha, fBeta](double fX) {
            const auto aDistribution
                = evaluateBetaDistribution(fX, fAlpha, fBeta, 0.0, 1.0, true, false);
            if (!aDistribution)
                return aDistribution;
            return api::ValueResult<double>::success(fProbability - aDistribution.maValue);
        });
    if (!aStandard)
        return aStandard;

    return api::ValueResult<double>::success(
        fLowerBound + aStandard.maValue * (fUpperBound - fLowerBound));
}

api::ValueResult<double> evaluatePoissonDistribution(
    double fX, double fLambda, bool bCumulative)
{
    if (fLambda <= 0.0 || fX < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const sal_Int32 nX = static_cast<sal_Int32>(::rtl::math::approxFloor(fX));
    const auto logProbability = [fLambda](sal_Int32 nValue) {
        return static_cast<double>(nValue) * std::log(fLambda) - fLambda
               - logGammaValuePositive(static_cast<double>(nValue) + 1.0);
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

api::ValueResult<double> evaluateNegativeBinomialDistribution(
    double fFailures, double fSuccesses, double fProbability, bool bCumulative,
    bool bMicrosoftSyntax)
{
    const double fWholeFailures = ::rtl::math::approxFloor(fFailures);
    const double fWholeSuccesses = ::rtl::math::approxFloor(fSuccesses);
    if (bMicrosoftSyntax)
    {
        if (fWholeSuccesses < 1.0 || fWholeFailures < 0.0 || fProbability < 0.0
            || fProbability > 1.0)
        {
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        }
    }
    else if ((fWholeFailures + fWholeSuccesses) <= 1.0 || fProbability < 0.0
             || fProbability > 1.0)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const double fQ = 1.0 - fProbability;
    if (bMicrosoftSyntax && bCumulative)
    {
        return api::ValueResult<double>::success(
            1.0 - betaCdf(fQ, fWholeFailures + 1.0, fWholeSuccesses));
    }

    double fFactor = std::pow(fProbability, fWholeSuccesses);
    for (double fIndex = 0.0; fIndex < fWholeFailures; ++fIndex)
        fFactor *= (fIndex + fWholeSuccesses) / (fIndex + 1.0) * fQ;
    return api::ValueResult<double>::success(fFactor);
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

api::ValueResult<double> evaluateExponentialDistribution(
    double fX, double fLambda, bool bCumulative)
{
    if (fLambda <= 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (!bCumulative)
        return api::ValueResult<double>::success(
            fX >= 0.0 ? fLambda * std::exp(-fLambda * fX) : 0.0);

    return api::ValueResult<double>::success(fX > 0.0 ? 1.0 - std::exp(-fLambda * fX) : 0.0);
}

api::ValueResult<double> evaluateWeibullDistribution(
    double fX, double fAlpha, double fBeta, bool bCumulative)
{
    if (fAlpha <= 0.0 || fBeta <= 0.0 || fX < 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    if (!bCumulative)
    {
        return api::ValueResult<double>::success(
            fAlpha / std::pow(fBeta, fAlpha) * std::pow(fX, fAlpha - 1.0)
            * std::exp(-std::pow(fX / fBeta, fAlpha)));
    }

    return api::ValueResult<double>::success(1.0 - std::exp(-std::pow(fX / fBeta, fAlpha)));
}

api::ValueResult<double> evaluateErrorFunction(double fValue)
{
    return api::ValueResult<double>::success(std::erf(fValue));
}

api::ValueResult<double> evaluateComplementaryErrorFunction(double fValue)
{
    return api::ValueResult<double>::success(std::erfc(fValue));
}

api::ValueResult<double> evaluateConfidence(
    double fAlpha, double fSigma, double fSampleSize)
{
    const double fN = ::rtl::math::approxFloor(fSampleSize);
    if (!(fSigma > 0.0) || !(fAlpha > 0.0) || !(fAlpha < 1.0) || !(fN >= 1.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const auto aStandard = evaluateStandardNormalInverse(1.0 - fAlpha / 2.0);
    if (!aStandard)
        return aStandard;
    return api::ValueResult<double>::success(aStandard.maValue * fSigma / std::sqrt(fN));
}

api::ValueResult<double> evaluateConfidenceT(
    double fAlpha, double fSigma, double fSampleSize)
{
    const double fN = ::rtl::math::approxFloor(fSampleSize);
    if (!(fSigma > 0.0) || !(fAlpha > 0.0) || !(fAlpha < 1.0) || !(fN >= 1.0))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    if (::rtl::math::approxEqual(fN, 1.0))
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    const auto aInverse = evaluateTInverse(fAlpha, fN - 1.0, 2);
    if (!aInverse)
        return aInverse;
    return api::ValueResult<double>::success(aInverse.maValue * fSigma / std::sqrt(fN));
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
