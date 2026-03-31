/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <cstdint>

#include <algorithm>
#include <cmath>
#include <vector>

#include <spreadsheetengine/runtime/KahanSum.hxx>
#include <spreadsheetengine/runtime/FloatingPoint.hxx>

#include "CoreRuntimeUtils.hxx"

namespace spreadsheetengine::core::math
{
namespace
{

using spreadsheetengine::core::util::toWholeNumber;

[[nodiscard]] double sumNumbers(const std::vector<double>& rNumbers)
{
    double fSum = 0.0;
    for (const double fValue : rNumbers)
        fSum = fp::approxAdd(fSum, fValue);
    return fSum;
}

} // namespace

std::optional<AggregateOptions> decodeAggregateOptions(std::int32_t nOption)
{
    switch (nOption)
    {
        case 0:
            return AggregateOptions{ false, false, true };
        case 1:
            return AggregateOptions{ true, false, true };
        case 2:
            return AggregateOptions{ false, true, true };
        case 3:
            return AggregateOptions{ true, true, true };
        case 4:
            return AggregateOptions{ false, false, false };
        case 5:
            return AggregateOptions{ true, false, false };
        case 6:
            return AggregateOptions{ false, true, false };
        case 7:
            return AggregateOptions{ true, true, false };
        default:
            return std::nullopt;
    }
}

api::ValueResult<double> evaluateExtremaNumbers(
    const std::vector<double>& rNumbers, bool bFindMaximum, bool bDefaultZeroIfEmpty)
{
    if (rNumbers.empty())
    {
        if (bDefaultZeroIfEmpty)
            return api::ValueResult<double>::success(0.0);
        return api::ValueResult<double>::failure(api::Error::NoValue);
    }

    return api::ValueResult<double>::success(
        bFindMaximum ? *std::max_element(rNumbers.begin(), rNumbers.end())
                     : *std::min_element(rNumbers.begin(), rNumbers.end()));
}

api::ValueResult<double> evaluateVarianceNumbers(
    const std::vector<double>& rNumbers, bool bSample, bool bReturnStdDev)
{
    const std::size_t nCount = rNumbers.size();
    if (nCount == 0 || (bSample && nCount < 2))
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    const double fMean = sumNumbers(rNumbers) / static_cast<double>(nCount);
    fp::KahanSum fSquaredDeviation = 0.0;
    for (const double fValue : rNumbers)
    {
        const double fDelta = fp::approxSub(fValue, fMean);
        fSquaredDeviation += fDelta * fDelta;
    }

    double fResult = fSquaredDeviation.get() / static_cast<double>(bSample ? (nCount - 1) : nCount);
    if (bReturnStdDev)
        fResult = std::sqrt(fResult);
    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluateTrimmean(std::vector<double> aValues, double fPercent)
{
    if (aValues.empty() || fPercent < 0.0 || fPercent >= 1.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    std::sort(aValues.begin(), aValues.end());
    std::int32_t nTrimCount
        = static_cast<std::int32_t>(fp::approxFloor(fPercent * aValues.size()));
    nTrimCount -= nTrimCount % 2;
    const std::size_t nTrimEachSide = static_cast<std::size_t>(nTrimCount / 2);
    if (nTrimEachSide * 2 >= aValues.size())
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    fp::KahanSum fSum = 0.0;
    for (std::size_t nIndex = nTrimEachSide; nIndex < aValues.size() - nTrimEachSide; ++nIndex)
        fSum += aValues[nIndex];

    const std::size_t nRemaining = aValues.size() - (nTrimEachSide * 2);
    return api::ValueResult<double>::success(fSum.get() / static_cast<double>(nRemaining));
}

api::ValueResult<double> evaluateGeometricMeanNumbers(const std::vector<double>& rValues)
{
    if (rValues.empty())
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    fp::KahanSum fLogSum = 0.0;
    for (const double fValue : rValues)
    {
        if (fValue < 0.0 || !std::isfinite(fValue))
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        if (fp::approxEqual(fValue, 0.0))
            return api::ValueResult<double>::success(0.0);
        fLogSum += std::log(fValue);
    }

    return api::ValueResult<double>::success(
        std::exp(fLogSum.get() / static_cast<double>(rValues.size())));
}

api::ValueResult<double> evaluateHarmonicMeanNumbers(const std::vector<double>& rValues)
{
    if (rValues.empty())
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    fp::KahanSum fInverseSum = 0.0;
    for (const double fValue : rValues)
    {
        if (!(fValue > 0.0) || !std::isfinite(fValue))
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        fInverseSum += 1.0 / fValue;
    }

    if (fp::approxEqual(fInverseSum.get(), 0.0))
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    return api::ValueResult<double>::success(
        static_cast<double>(rValues.size()) / fInverseSum.get());
}

api::ValueResult<double> evaluateModeSingle(const std::vector<double>& rValues)
{
    const auto aModes = evaluateModeValues(rValues);
    if (!aModes)
    {
        if (aModes.meError == api::Error::NoValue)
            return api::ValueResult<double>::failure(api::Error::NotAvailable);
        return api::ValueResult<double>::failure(aModes.meError);
    }
    return api::ValueResult<double>::success(aModes.maValue.front());
}

api::ValueResult<std::vector<double>> evaluateModeValues(const std::vector<double>& rValues)
{
    if (rValues.empty())
        return api::ValueResult<std::vector<double>>::failure(api::Error::NoValue);

    std::vector<double> aSorted = rValues;
    std::sort(aSorted.begin(), aSorted.end());

    std::vector<double> aModes;
    std::int32_t nMaxCount = 1;
    std::int32_t nCurrentCount = 1;
    double fCurrentValue = aSorted.front();
    for (std::size_t nIndex = 1; nIndex <= aSorted.size(); ++nIndex)
    {
        if (nIndex < aSorted.size() && aSorted[nIndex] == fCurrentValue)
        {
            ++nCurrentCount;
            continue;
        }

        if (nCurrentCount > 1)
        {
            if (nCurrentCount > nMaxCount)
            {
                nMaxCount = nCurrentCount;
                aModes.assign(1, fCurrentValue);
            }
            else if (nCurrentCount == nMaxCount)
            {
                aModes.push_back(fCurrentValue);
            }
        }

        if (nIndex < aSorted.size())
        {
            fCurrentValue = aSorted[nIndex];
            nCurrentCount = 1;
        }
    }

    if (aModes.empty())
        return api::ValueResult<std::vector<double>>::failure(api::Error::NoValue);

    std::vector<double> aOrderedModes;
    aOrderedModes.reserve(aModes.size());
    for (const double fValue : rValues)
    {
        const auto itMode = std::find(aModes.begin(), aModes.end(), fValue);
        if (itMode == aModes.end())
            continue;
        if (std::find(aOrderedModes.begin(), aOrderedModes.end(), fValue) == aOrderedModes.end())
            aOrderedModes.push_back(fValue);
    }

    return api::ValueResult<std::vector<double>>::success(aOrderedModes);
}

api::ValueResult<double> evaluateHypergeometricDistribution(
    double fX, double fTrials, double fSuccesses, double fPopulation, bool bCumulative)
{
    const double fWholePopulation = fp::approxFloor(fPopulation);
    const double fWholeSuccesses = fp::approxFloor(fSuccesses);
    const double fWholeTrials = fp::approxFloor(fTrials);
    const double fWholeX = fp::approxFloor(fX);
    if (fWholeX < 0.0 || fWholeTrials < fWholeX || fWholePopulation < fWholeTrials
        || fWholePopulation < fWholeSuccesses || fWholeSuccesses < 0.0)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const auto probability = [fWholePopulation, fWholeSuccesses,
                              fWholeTrials](std::int32_t nValue) {
        const long double fK = static_cast<long double>(nValue);
        if (fK < 0.0 || fK > fWholeTrials || fK > fWholeSuccesses
            || (fWholeTrials - fK) > (fWholePopulation - fWholeSuccesses))
        {
            return 0.0L;
        }

        const long double fLogProbability
            = std::lgammal(static_cast<long double>(fWholeSuccesses) + 1.0L)
              - std::lgammal(fK + 1.0L)
              - std::lgammal(static_cast<long double>(fWholeSuccesses) - fK + 1.0L)
              + std::lgammal(static_cast<long double>(fWholePopulation - fWholeSuccesses) + 1.0L)
              - std::lgammal(static_cast<long double>(fWholeTrials) - fK + 1.0L)
              - std::lgammal(static_cast<long double>(fWholePopulation - fWholeSuccesses)
                             - (static_cast<long double>(fWholeTrials) - fK) + 1.0L)
              - std::lgammal(static_cast<long double>(fWholePopulation) + 1.0L)
              + std::lgammal(static_cast<long double>(fWholeTrials) + 1.0L)
              + std::lgammal(static_cast<long double>(fWholePopulation - fWholeTrials) + 1.0L);
        return std::exp(fLogProbability);
    };

    const std::int32_t nX = static_cast<std::int32_t>(fWholeX);
    if (!bCumulative)
        return api::ValueResult<double>::success(static_cast<double>(probability(nX)));

    long double fSum = 0.0L;
    for (std::int32_t nValue = 0; nValue <= nX; ++nValue)
        fSum += probability(nValue);
    return api::ValueResult<double>::success(std::min(1.0, static_cast<double>(fSum)));
}

api::ValueResult<double> evaluateProbability(
    const std::vector<double>& rProbabilities, const std::vector<double>& rValues,
    double fLower, double fUpper)
{
    if (rProbabilities.empty() || rProbabilities.size() != rValues.size())
        return api::ValueResult<double>::failure(api::Error::NotAvailable);

    double fLo = fLower;
    double fUp = fUpper;
    if (fLo > fUp)
        std::swap(fLo, fUp);

    fp::KahanSum fSum = 0.0;
    fp::KahanSum fResult = 0.0;
    for (std::size_t nIndex = 0; nIndex < rProbabilities.size(); ++nIndex)
    {
        const double fProbability = rProbabilities[nIndex];
        const double fValue = rValues[nIndex];
        if (!std::isfinite(fProbability) || !std::isfinite(fValue) || fProbability < 0.0
            || fProbability > 1.0)
        {
            return api::ValueResult<double>::failure(api::Error::NoValue);
        }

        fSum += fProbability;
        if (fValue >= fLo && fValue <= fUp)
            fResult += fProbability;
    }

    if (std::abs((fSum - 1.0).get()) > 1.0E-7)
        return api::ValueResult<double>::failure(api::Error::NoValue);

    return api::ValueResult<double>::success(fResult.get());
}

api::ValueResult<double> evaluatePercentrank(
    std::vector<double> aValues, double fValue, bool bInclusive, std::int32_t nSignificance)
{
    if (aValues.empty())
        return api::ValueResult<double>::failure(api::Error::NotAvailable);
    if (nSignificance < 1)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    std::sort(aValues.begin(), aValues.end());
    const std::size_t nSize = aValues.size();
    if (fValue < aValues.front() || fValue > aValues.back())
        return api::ValueResult<double>::failure(api::Error::NotAvailable);
    if (nSize == 1)
        return api::ValueResult<double>::success(1.0);

    double fResult = 0.0;
    if (fp::approxEqual(fValue, aValues.front()))
    {
        fResult = bInclusive ? 0.0 : 1.0 / static_cast<double>(nSize + 1);
    }
    else
    {
        std::size_t nOldCount = 0;
        double fOldValue = aValues.front();
        std::size_t nIndex = 1;
        for (; nIndex < nSize && aValues[nIndex] < fValue; ++nIndex)
        {
            if (!fp::approxEqual(aValues[nIndex], fOldValue))
            {
                nOldCount = nIndex;
                fOldValue = aValues[nIndex];
            }
        }

        if (nIndex < nSize && !fp::approxEqual(aValues[nIndex], fOldValue))
            nOldCount = nIndex;

        if (nIndex < nSize && fp::approxEqual(fValue, aValues[nIndex]))
        {
            if (bInclusive)
                fResult = static_cast<double>(nOldCount) / static_cast<double>(nSize - 1);
            else
                fResult = static_cast<double>(nIndex + 1) / static_cast<double>(nSize + 1);
        }
        else
        {
            if (nOldCount == 0 || nOldCount >= nSize)
                return api::ValueResult<double>::failure(api::Error::NotAvailable);

            const double fLower = aValues[nOldCount - 1];
            const double fUpper = aValues[nOldCount];
            if (fp::approxEqual(fLower, fUpper))
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);

            const double fFraction = (fValue - fLower) / (fUpper - fLower);
            if (bInclusive)
            {
                fResult = (static_cast<double>(nOldCount - 1) + fFraction)
                          / static_cast<double>(nSize - 1);
            }
            else
            {
                fResult = (static_cast<double>(nOldCount) + fFraction)
                          / static_cast<double>(nSize + 1);
            }
        }
    }

    if (!fp::approxEqual(fResult, 0.0))
    {
        const double fExponent = fp::approxFloor(std::log10(fResult)) + 1.0
                                 - static_cast<double>(nSignificance);
        const double fScale = std::pow(10.0, -fExponent);
        fResult = fp::round(fResult * fScale) / fScale;
    }

    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluateSkewNumbers(const std::vector<double>& rValues, bool bPopulation)
{
    if (rValues.size() < 3)
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    fp::KahanSum fSum = 0.0;
    for (const double fValue : rValues)
        fSum += fValue;

    const double fCount = static_cast<double>(rValues.size());
    const double fMean = fSum.get() / fCount;

    fp::KahanSum fVarianceSum = 0.0;
    for (const double fValue : rValues)
        fVarianceSum += (fValue - fMean) * (fValue - fMean);

    const double fStdDev
        = std::sqrt(fVarianceSum.get() / (bPopulation ? fCount : (fCount - 1.0)));
    if (fStdDev == 0.0)
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    fp::KahanSum fCubeSum = 0.0;
    for (const double fValue : rValues)
    {
        const double fDelta = (fValue - fMean) / fStdDev;
        fCubeSum += fDelta * fDelta * fDelta;
    }

    if (bPopulation)
        return api::ValueResult<double>::success(fCubeSum.get() / fCount);

    return api::ValueResult<double>::success(
        ((fCubeSum.get() * fCount) / (fCount - 1.0)) / (fCount - 2.0));
}

api::ValueResult<double> evaluateKurtosisNumbers(const std::vector<double>& rValues)
{
    const double fCount = static_cast<double>(rValues.size());
    if (fCount < 4.0)
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    fp::KahanSum fSum = 0.0;
    for (const double fValue : rValues)
        fSum += fValue;

    const double fMean = fSum.get() / fCount;

    fp::KahanSum fVarianceSum = 0.0;
    for (const double fValue : rValues)
        fVarianceSum += (fValue - fMean) * (fValue - fMean);

    const double fStdDev = std::sqrt(fVarianceSum.get() / (fCount - 1.0));
    if (fStdDev == 0.0)
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    fp::KahanSum fFourthMoment = 0.0;
    for (const double fValue : rValues)
    {
        const double fDelta = (fValue - fMean) / fStdDev;
        fFourthMoment += (fDelta * fDelta) * (fDelta * fDelta);
    }

    const double fDenominator = (fCount - 2.0) * (fCount - 3.0);
    const double fLeading
        = fCount * (fCount + 1.0) / ((fCount - 1.0) * fDenominator);
    const double fTrailing = 3.0 * (fCount - 1.0) * (fCount - 1.0) / fDenominator;
    return api::ValueResult<double>::success(fFourthMoment.get() * fLeading - fTrailing);
}

api::ValueResult<double> evaluateAggregateNumbers(std::int32_t nFunction, const AggregateScan& rScan)
{
    switch (nFunction)
    {
        case 1:
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::failure(api::Error::DivisionByZero);
            return api::ValueResult<double>::success(
                sumNumbers(rScan.maNumbers) / static_cast<double>(rScan.maNumbers.size()));
        case 2:
            return api::ValueResult<double>::success(static_cast<double>(rScan.maNumbers.size()));
        case 3:
            return api::ValueResult<double>::success(static_cast<double>(rScan.mnNonEmptyCount));
        case 4:
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::success(0.0);
            return api::ValueResult<double>::success(
                *std::max_element(rScan.maNumbers.begin(), rScan.maNumbers.end()));
        case 5:
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::success(0.0);
            return api::ValueResult<double>::success(
                *std::min_element(rScan.maNumbers.begin(), rScan.maNumbers.end()));
        case 6:
        {
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::success(0.0);

            double fProduct = 1.0;
            for (const double fValue : rScan.maNumbers)
                fProduct *= fValue;
            return api::ValueResult<double>::success(fProduct);
        }
        case 7:
        case 8:
        case 10:
        case 11:
        {
            const bool bSample = nFunction == 7 || nFunction == 10;
            const bool bReturnStdDev = nFunction == 7 || nFunction == 8;
            return evaluateVarianceNumbers(rScan.maNumbers, bSample, bReturnStdDev);
        }
        case 9:
            return api::ValueResult<double>::success(sumNumbers(rScan.maNumbers));
        case 12:
        {
            if (rScan.maNumbers.empty())
                return api::ValueResult<double>::failure(api::Error::DivisionByZero);
            std::vector<double> aSorted = rScan.maNumbers;
            std::sort(aSorted.begin(), aSorted.end());
            const std::size_t nMid = aSorted.size() / 2;
            if ((aSorted.size() % 2) != 0)
                return api::ValueResult<double>::success(aSorted[nMid]);
            return api::ValueResult<double>::success(
                (aSorted[nMid - 1] + aSorted[nMid]) / 2.0);
        }
        case 13:
            return evaluateModeSingle(rScan.maNumbers);
        default:
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
}

api::ValueResult<double> evaluateAggregateRankedNumbers(
    std::int32_t nFunction, const AggregateScan& rScan, double fRankValue)
{
    if (!std::isfinite(fRankValue) || rScan.maNumbers.empty())
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    std::vector<double> aSorted = rScan.maNumbers;
    std::sort(aSorted.begin(), aSorted.end());

    const auto percentileInc = [&](double fFraction) -> api::ValueResult<double> {
        if (!(fFraction >= 0.0 && fFraction <= 1.0))
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        if (aSorted.size() == 1)
            return api::ValueResult<double>::success(aSorted.front());

        const double fIndex = fFraction * static_cast<double>(aSorted.size() - 1);
        const std::size_t nLower = static_cast<std::size_t>(std::floor(fIndex));
        const std::size_t nUpper = static_cast<std::size_t>(std::ceil(fIndex));
        if (nLower == nUpper)
            return api::ValueResult<double>::success(aSorted[nLower]);

        const double fWeight = fIndex - static_cast<double>(nLower);
        return api::ValueResult<double>::success(
            aSorted[nLower] + (aSorted[nUpper] - aSorted[nLower]) * fWeight);
    };

    const auto percentileExc = [&](double fFraction) -> api::ValueResult<double> {
        if (!(fFraction > 0.0 && fFraction < 1.0))
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const double fIndex = fFraction * static_cast<double>(aSorted.size() + 1);
        if (!(fIndex >= 1.0 && fIndex <= static_cast<double>(aSorted.size())))
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const std::size_t nLower = static_cast<std::size_t>(std::floor(fIndex));
        const std::size_t nUpper = static_cast<std::size_t>(std::ceil(fIndex));
        if (nLower == nUpper)
            return api::ValueResult<double>::success(aSorted[nLower - 1]);

        const double fWeight = fIndex - static_cast<double>(nLower);
        return api::ValueResult<double>::success(
            aSorted[nLower - 1] + (aSorted[nUpper - 1] - aSorted[nLower - 1]) * fWeight);
    };

    switch (nFunction)
    {
        case 14:
        case 15:
        {
            const auto oRank = toWholeNumber(fRankValue);
            if (!oRank || *oRank < 1 || *oRank > static_cast<std::int32_t>(aSorted.size()))
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
            const std::size_t nIndex = nFunction == 14
                                           ? aSorted.size() - static_cast<std::size_t>(*oRank)
                                           : static_cast<std::size_t>(*oRank - 1);
            return api::ValueResult<double>::success(aSorted[nIndex]);
        }
        case 16:
            return percentileInc(fRankValue);
        case 17:
        {
            const auto oQuartile = toWholeNumber(fRankValue);
            if (!oQuartile || *oQuartile < 0 || *oQuartile > 4)
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
            return percentileInc(static_cast<double>(*oQuartile) / 4.0);
        }
        case 18:
            return percentileExc(fRankValue);
        case 19:
        {
            const auto oQuartile = toWholeNumber(fRankValue);
            if (!oQuartile || *oQuartile < 1 || *oQuartile > 3)
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
            return percentileExc(static_cast<double>(*oQuartile) / 4.0);
        }
        default:
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
