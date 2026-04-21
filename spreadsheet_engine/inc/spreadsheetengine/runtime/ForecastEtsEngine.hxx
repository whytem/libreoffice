/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <comphelper/random.hxx>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/runtime/KahanSum.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnOperators.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

namespace spreadsheetengine::core::rpn
{

enum class ForecastEtsVariant : std::uint8_t
{
    Add,
    Mult,
    PIAdd,
    PIMult,
    Seasonality,
    StatAdd,
    StatMult
};

struct ForecastEtsPlanResult
{
    bool mbIsScalar = false;
    double mfScalar = 0.0;
    MatrixOperand maMatrix;
};

namespace detail::ets
{

inline constexpr double kMinAbcResolution = 0.001;
inline constexpr std::size_t kScenarioCount = 1000;

struct DataPoint
{
    double mfX = 0.0;
    double mfY = 0.0;
};

[[nodiscard]] constexpr bool isStatsVariant(ForecastEtsVariant eVariant) noexcept
{
    return eVariant == ForecastEtsVariant::StatAdd
           || eVariant == ForecastEtsVariant::StatMult;
}

[[nodiscard]] constexpr bool isPiVariant(ForecastEtsVariant eVariant) noexcept
{
    return eVariant == ForecastEtsVariant::PIAdd
           || eVariant == ForecastEtsVariant::PIMult;
}

[[nodiscard]] constexpr bool isAdditiveVariant(ForecastEtsVariant eVariant) noexcept
{
    return eVariant == ForecastEtsVariant::Add
           || eVariant == ForecastEtsVariant::PIAdd
           || eVariant == ForecastEtsVariant::StatAdd;
}

[[nodiscard]] inline bool isNumericCell(const api::CellValue& rValue)
{
    return rValue.meKind == api::CellValueKind::Number
           || rValue.meKind == api::CellValueKind::Boolean;
}

[[nodiscard]] inline api::ValueResult<double> extractScalarNumber(
    const MatrixOperand* pOperand, double fDefault)
{
    if (!pOperand)
        return api::ValueResult<double>::success(fDefault);
    if (pOperand->cellCount() != 1 || pOperand->maValues.empty())
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);

    const auto& rValue = pOperand->maValues.front();
    if (rValue.isError())
        return api::ValueResult<double>::failure(rValue.meError);
    if (!isNumericCell(rValue))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(rValue.mfNumber);
}

[[nodiscard]] inline api::ValueResult<std::int32_t> extractWholeNumber(
    const MatrixOperand* pOperand, std::int32_t nDefault)
{
    const auto aNumber = extractScalarNumber(pOperand, static_cast<double>(nDefault));
    if (!aNumber)
        return api::ValueResult<std::int32_t>::failure(aNumber.meError);
    if (!std::isfinite(aNumber.maValue) || std::trunc(aNumber.maValue) != aNumber.maValue
        || aNumber.maValue < static_cast<double>(std::numeric_limits<std::int32_t>::min())
        || aNumber.maValue > static_cast<double>(std::numeric_limits<std::int32_t>::max()))
    {
        return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);
    }
    return api::ValueResult<std::int32_t>::success(static_cast<std::int32_t>(aNumber.maValue));
}

[[nodiscard]] inline api::ValueResult<bool> extractBinaryFlag(
    const MatrixOperand* pOperand, bool bDefault)
{
    const auto aNumber = extractScalarNumber(pOperand, bDefault ? 1.0 : 0.0);
    if (!aNumber)
        return api::ValueResult<bool>::failure(aNumber.meError);
    if (aNumber.maValue != 0.0 && aNumber.maValue != 1.0)
        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
    return api::ValueResult<bool>::success(aNumber.maValue != 0.0);
}

[[nodiscard]] inline bool tryReadMatrixNumber(
    const MatrixOperand& rMatrix, std::size_t nIndex, double& rfValue)
{
    if (nIndex >= rMatrix.maValues.size())
        return false;
    const auto& rValue = rMatrix.maValues[nIndex];
    if (!isNumericCell(rValue))
        return false;
    rfValue = rValue.mfNumber;
    return true;
}

[[nodiscard]] inline bool tryReadMatrixNumberAt(
    const MatrixOperand& rMatrix, api::MatrixSize nColumn, api::MatrixSize nRow, double& rfValue)
{
    const std::size_t nIndex
        = static_cast<std::size_t>(nRow) * rMatrix.maDimensions.mnColumns + nColumn;
    return tryReadMatrixNumber(rMatrix, nIndex, rfValue);
}

[[nodiscard]] inline api::ValueResult<api::DateParts> datePartsFromSerial(
    const api::DateParts& rNullDate, double fSerial)
{
    const api::DateSerial nSerial = static_cast<api::DateSerial>(fSerial);
    const auto aDay = api::calendar::dayFromSerial(rNullDate, nSerial);
    if (!aDay)
        return api::ValueResult<api::DateParts>::failure(aDay.meError);

    return api::ValueResult<api::DateParts>::success(
        { static_cast<std::int16_t>(api::calendar::yearFromSerial(rNullDate, nSerial)),
          static_cast<std::int16_t>(api::calendar::monthFromSerial(rNullDate, nSerial)),
          static_cast<std::int16_t>(aDay.maValue) });
}

[[nodiscard]] constexpr bool isLeapYear(std::int32_t nYear) noexcept
{
    return (nYear % 4 == 0 && nYear % 100 != 0) || (nYear % 400 == 0);
}

[[nodiscard]] inline double monthLength(std::int32_t nYear, std::int32_t nMonth)
{
    switch (nMonth)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31.0;
        case 2:
            return isLeapYear(nYear) ? 29.0 : 28.0;
        default:
            return 30.0;
    }
}

[[nodiscard]] inline double convertSerialToMonthCoordinate(
    const api::DateParts& rNullDate, double fSerial, std::int32_t nAnchorDay)
{
    const auto aDate = datePartsFromSerial(rNullDate, fSerial);
    if (!aDate)
        return fSerial;

    const double fMonthLength = monthLength(aDate.maValue.mnYear, aDate.maValue.mnMonth);
    return 12.0 * aDate.maValue.mnYear + aDate.maValue.mnMonth
           + (aDate.maValue.mnDay - nAnchorDay) / fMonthLength;
}

class ForecastEtsCalculation
{
public:
    ForecastEtsCalculation(std::size_t nCount, const api::DateParts& rNullDate)
        : mrNullDate(rNullDate)
        , mnCount(nCount)
    {
        maRange.reserve(mnCount);
    }

    [[nodiscard]] api::Error getError() const { return meError; }

    [[nodiscard]] bool preprocess(
        const MatrixOperand& rKnownX, const MatrixOperand& rKnownY, std::int32_t nSmplInPrd,
        bool bDataCompletion, std::int32_t nAggregation, const MatrixOperand* pTarget,
        ForecastEtsVariant eVariant)
    {
        mbEDS = nSmplInPrd == 0;
        mbAdditive = isAdditiveVariant(eVariant);

        double fX = 0.0;
        double fY = 0.0;
        for (std::size_t i = 0; i < mnCount; ++i)
        {
            if (!tryReadMatrixNumber(rKnownX, i, fX) || !tryReadMatrixNumber(rKnownY, i, fY))
            {
                meError = api::Error::IllegalArgument;
                return false;
            }
            maRange.push_back({ fX, fY });
        }
        std::sort(maRange.begin(), maRange.end(),
            [](const DataPoint& rLeft, const DataPoint& rRight) { return rLeft.mfX < rRight.mfX; });

        if (pTarget)
        {
            double fTarget = 0.0;
            if (!tryReadMatrixNumber(*pTarget, 0, fTarget))
            {
                meError = api::Error::IllegalArgument;
                return false;
            }
            if (!isPiVariant(eVariant))
            {
                if (fTarget < maRange.front().mfX)
                {
                    meError = api::Error::IllegalArgument;
                    return false;
                }
            }
            else if (fTarget < maRange.back().mfX)
            {
                meError = api::Error::IllegalArgument;
                return false;
            }
        }

        const auto aFirstDate = datePartsFromSerial(mrNullDate, maRange.front().mfX);
        mnMonthDay = aFirstDate ? aFirstDate.maValue.mnDay : 0;
        api::DateParts aPreviousDate {};
        bool bHavePreviousDate = false;
        if (aFirstDate)
        {
            aPreviousDate = aFirstDate.maValue;
            bHavePreviousDate = true;
        }
        for (std::size_t i = 1; i < mnCount && mnMonthDay != 0; ++i)
        {
            const auto aCurrentDate = datePartsFromSerial(mrNullDate, maRange[i].mfX);
            if (!aCurrentDate)
            {
                mnMonthDay = 0;
                break;
            }
            if (bHavePreviousDate
                && (aPreviousDate.mnYear != aCurrentDate.maValue.mnYear
                    || aPreviousDate.mnMonth != aCurrentDate.maValue.mnMonth
                    || aPreviousDate.mnDay != aCurrentDate.maValue.mnDay)
                && aCurrentDate.maValue.mnDay != mnMonthDay)
            {
                mnMonthDay = 0;
            }
            aPreviousDate = aCurrentDate.maValue;
            bHavePreviousDate = true;
        }

        mfStepSize = std::numeric_limits<double>::max();
        if (mnMonthDay)
        {
            for (auto& rPoint : maRange)
                rPoint.mfX = convertSerialToMonthCoordinate(mrNullDate, rPoint.mfX, mnMonthDay);
        }

        for (std::size_t i = 1; i < mnCount; ++i)
        {
            double fStep = maRange[i].mfX - maRange[i - 1].mfX;
            if (fStep == 0.0)
            {
                if (nAggregation == 0)
                {
                    meError = api::Error::NoValue;
                    return false;
                }

                double fTmp = maRange[i - 1].mfY;
                std::size_t nCounter = 1;
                switch (nAggregation)
                {
                    case 1:
                        while (i < mnCount && maRange[i].mfX == maRange[i - 1].mfX)
                        {
                            maRange.erase(maRange.begin() + i);
                            --mnCount;
                        }
                        break;
                    case 7:
                        while (i < mnCount && maRange[i].mfX == maRange[i - 1].mfX)
                        {
                            fTmp += maRange[i].mfY;
                            maRange.erase(maRange.begin() + i);
                            --mnCount;
                        }
                        maRange[i - 1].mfY = fTmp;
                        break;
                    case 2:
                    case 3:
                        while (i < mnCount && maRange[i].mfX == maRange[i - 1].mfX)
                        {
                            ++nCounter;
                            maRange.erase(maRange.begin() + i);
                            --mnCount;
                        }
                        maRange[i - 1].mfY = static_cast<double>(nCounter);
                        break;
                    case 4:
                        while (i < mnCount && maRange[i].mfX == maRange[i - 1].mfX)
                        {
                            if (maRange[i].mfY > fTmp)
                                fTmp = maRange[i].mfY;
                            maRange.erase(maRange.begin() + i);
                            --mnCount;
                        }
                        maRange[i - 1].mfY = fTmp;
                        break;
                    case 5:
                    {
                        std::vector<double> aMedianValues { maRange[i - 1].mfY };
                        while (i < mnCount && maRange[i].mfX == maRange[i - 1].mfX)
                        {
                            aMedianValues.push_back(maRange[i].mfY);
                            ++nCounter;
                            maRange.erase(maRange.begin() + i);
                            --mnCount;
                        }
                        std::sort(aMedianValues.begin(), aMedianValues.end());
                        maRange[i - 1].mfY
                            = nCounter % 2 ? aMedianValues[nCounter / 2]
                                           : (aMedianValues[nCounter / 2]
                                                  + aMedianValues[nCounter / 2 - 1])
                                                 / 2.0;
                        break;
                    }
                    case 6:
                        while (i < mnCount && maRange[i].mfX == maRange[i - 1].mfX)
                        {
                            if (maRange[i].mfY < fTmp)
                                fTmp = maRange[i].mfY;
                            maRange.erase(maRange.begin() + i);
                            --mnCount;
                        }
                        maRange[i - 1].mfY = fTmp;
                        break;
                    default:
                        meError = api::Error::IllegalArgument;
                        return false;
                }
                fStep = i < mnCount - 1 ? maRange[i].mfX - maRange[i - 1].mfX : mfStepSize;
            }
            if (fStep > 0.0 && fStep < mfStepSize)
                mfStepSize = fStep;
        }

        bool bHasGap = false;
        for (std::size_t i = 1; i < mnCount && !bHasGap; ++i)
        {
            const double fStep = maRange[i].mfX - maRange[i - 1].mfX;
            if (fStep != mfStepSize)
            {
                if (std::fmod(fStep, mfStepSize) != 0.0)
                {
                    meError = api::Error::NoValue;
                    return false;
                }
                bHasGap = true;
            }
        }

        if (bHasGap)
        {
            std::size_t nMissingXCount = 0;
            const double fOriginalCount = static_cast<double>(mnCount);
            for (std::size_t i = 1; i < mnCount; ++i)
            {
                const double fDist = maRange[i].mfX - maRange[i - 1].mfX;
                if (fDist > mfStepSize)
                {
                    const double fYGap = (maRange[i].mfY + maRange[i - 1].mfY) / 2.0;
                    for (fp::KahanSum aGap = maRange[i - 1].mfX + mfStepSize;
                         aGap.get() < maRange[i].mfX; aGap += mfStepSize)
                    {
                        maRange.insert(
                            maRange.begin() + i,
                            DataPoint { aGap.get(), bDataCompletion ? fYGap : 0.0 });
                        ++i;
                        ++mnCount;
                        ++nMissingXCount;
                        if (static_cast<double>(nMissingXCount) / fOriginalCount > 0.3)
                        {
                            meError = api::Error::NoValue;
                            return false;
                        }
                    }
                }
            }
        }

        if (nSmplInPrd != 1)
            mnSmplInPrd = static_cast<std::size_t>(nSmplInPrd);
        else
        {
            mnSmplInPrd = calcPeriodLen();
            if (mnSmplInPrd == 1)
                mbEDS = true;
        }

        return initData();
    }

    [[nodiscard]] api::ValueResult<double> samplesInPeriod()
    {
        initCalc();
        if (meError != api::Error::None)
            return api::ValueResult<double>::failure(meError);
        return api::ValueResult<double>::success(static_cast<double>(mnSmplInPrd));
    }

    bool fillForecastRange(const MatrixOperand& rTarget, MatrixOperand& rOut)
    {
        if (!prepareLike(rTarget, rOut))
            return false;

        for (api::MatrixSize nRow = 0; nRow < rTarget.maDimensions.mnRows; ++nRow)
        {
            for (api::MatrixSize nColumn = 0; nColumn < rTarget.maDimensions.mnColumns; ++nColumn)
            {
                double fTarget = 0.0;
                if (!tryReadMatrixNumberAt(rTarget, nColumn, nRow, fTarget))
                {
                    meError = api::Error::IllegalArgument;
                    return false;
                }
                if (mnMonthDay)
                    fTarget = convertXtoMonths(fTarget);

                const auto aForecast = getForecast(fTarget);
                if (!aForecast)
                    return false;
                putNumber(rOut, nColumn, nRow, aForecast.maValue);
            }
        }
        return true;
    }

    bool fillStatistics(const MatrixOperand& rType, MatrixOperand& rOut)
    {
        initCalc();
        if (meError != api::Error::None || !prepareLike(rType, rOut))
            return false;

        for (api::MatrixSize nRow = 0; nRow < rType.maDimensions.mnRows; ++nRow)
        {
            for (api::MatrixSize nColumn = 0; nColumn < rType.maDimensions.mnColumns; ++nColumn)
            {
                double fType = 0.0;
                if (!tryReadMatrixNumberAt(rType, nColumn, nRow, fType))
                {
                    meError = api::Error::IllegalArgument;
                    return false;
                }

                switch (static_cast<int>(fType))
                {
                    case 1:
                        putNumber(rOut, nColumn, nRow, mfAlpha);
                        break;
                    case 2:
                        putNumber(rOut, nColumn, nRow, mfGamma);
                        break;
                    case 3:
                        putNumber(rOut, nColumn, nRow, mfBeta);
                        break;
                    case 4:
                        putNumber(rOut, nColumn, nRow, mfMASE);
                        break;
                    case 5:
                        putNumber(rOut, nColumn, nRow, mfSMAPE);
                        break;
                    case 6:
                        putNumber(rOut, nColumn, nRow, mfMAE);
                        break;
                    case 7:
                        putNumber(rOut, nColumn, nRow, mfRMSE);
                        break;
                    case 8:
                        putNumber(rOut, nColumn, nRow, mfStepSize);
                        break;
                    case 9:
                        putNumber(rOut, nColumn, nRow, static_cast<double>(mnSmplInPrd));
                        break;
                    default:
                        meError = api::Error::IllegalArgument;
                        return false;
                }
            }
        }
        return true;
    }

    bool fillEtsPredictionIntervals(const MatrixOperand& rTarget, MatrixOperand& rOut, double fPiLevel)
    {
        initCalc();
        if (meError != api::Error::None || !prepareLike(rTarget, rOut))
            return false;

        double fMaxTarget = 0.0;
        if (!tryReadMatrixNumber(rTarget, 0, fMaxTarget))
        {
            meError = api::Error::IllegalArgument;
            return false;
        }
        for (api::MatrixSize nRow = 0; nRow < rTarget.maDimensions.mnRows; ++nRow)
        {
            for (api::MatrixSize nColumn = 0; nColumn < rTarget.maDimensions.mnColumns; ++nColumn)
            {
                double fCurrent = 0.0;
                if (!tryReadMatrixNumberAt(rTarget, nColumn, nRow, fCurrent))
                {
                    meError = api::Error::IllegalArgument;
                    return false;
                }
                if (fCurrent > fMaxTarget)
                    fMaxTarget = fCurrent;
            }
        }

        fMaxTarget = (mnMonthDay ? convertXtoMonths(fMaxTarget) : fMaxTarget) - maRange[mnCount - 1].mfX;
        std::size_t nSize = static_cast<std::size_t>(fMaxTarget / mfStepSize);
        if (std::fmod(fMaxTarget, mfStepSize) != 0.0)
            ++nSize;
        if (nSize == 0)
        {
            meError = api::Error::IllegalArgument;
            return false;
        }

        std::vector<double> aScenRange(nSize);
        std::vector<double> aScenBase(nSize);
        std::vector<double> aScenTrend(nSize);
        std::vector<double> aScenPerIdx(nSize);
        std::vector<std::vector<double>> aPredictions(
            nSize, std::vector<double>(kScenarioCount));

        for (std::size_t k = 0; k < kScenarioCount; ++k)
        {
            if (mbAdditive)
            {
                const double fPIdx = !mbEDS ? maPerIdx[mnCount - mnSmplInPrd] : 0.0;
                aScenRange[0] = maBase[mnCount - 1] + maTrend[mnCount - 1] + fPIdx + randDeviation();
                aPredictions[0][k] = aScenRange[0];
                aScenBase[0] = mfAlpha * (aScenRange[0] - fPIdx)
                               + (1.0 - mfAlpha) * (maBase[mnCount - 1] + maTrend[mnCount - 1]);
                aScenTrend[0] = mfGamma * (aScenBase[0] - maBase[mnCount - 1])
                                + (1.0 - mfGamma) * maTrend[mnCount - 1];
                aScenPerIdx[0] = mfBeta * (aScenRange[0] - aScenBase[0])
                                 + (1.0 - mfBeta) * fPIdx;
                for (std::size_t i = 1; i < nSize; ++i)
                {
                    const double fPerIdx = i < mnSmplInPrd
                                               ? maPerIdx[mnCount + i - mnSmplInPrd]
                                               : aScenPerIdx[i - mnSmplInPrd];
                    aScenRange[i]
                        = aScenBase[i - 1] + aScenTrend[i - 1] + fPerIdx + randDeviation();
                    aPredictions[i][k] = aScenRange[i];
                    aScenBase[i] = mfAlpha * (aScenRange[i] - fPerIdx)
                                   + (1.0 - mfAlpha) * (aScenBase[i - 1] + aScenTrend[i - 1]);
                    aScenTrend[i] = mfGamma * (aScenBase[i] - aScenBase[i - 1])
                                    + (1.0 - mfGamma) * aScenTrend[i - 1];
                    aScenPerIdx[i] = mfBeta * (aScenRange[i] - aScenBase[i])
                                     + (1.0 - mfBeta) * fPerIdx;
                }
            }
            else
            {
                aScenRange[0] = (maBase[mnCount - 1] + maTrend[mnCount - 1])
                                * maPerIdx[mnCount - mnSmplInPrd] + randDeviation();
                aPredictions[0][k] = aScenRange[0];
                aScenBase[0] = mfAlpha * (aScenRange[0] / maPerIdx[mnCount - mnSmplInPrd])
                               + (1.0 - mfAlpha) * (maBase[mnCount - 1] + maTrend[mnCount - 1]);
                aScenTrend[0] = mfGamma * (aScenBase[0] - maBase[mnCount - 1])
                                + (1.0 - mfGamma) * maTrend[mnCount - 1];
                aScenPerIdx[0] = mfBeta * (aScenRange[0] / aScenBase[0])
                                 + (1.0 - mfBeta) * maPerIdx[mnCount - mnSmplInPrd];
                for (std::size_t i = 1; i < nSize; ++i)
                {
                    const double fPerIdx = i < mnSmplInPrd
                                               ? maPerIdx[mnCount + i - mnSmplInPrd]
                                               : aScenPerIdx[i - mnSmplInPrd];
                    aScenRange[i]
                        = (aScenBase[i - 1] + aScenTrend[i - 1]) * fPerIdx + randDeviation();
                    aPredictions[i][k] = aScenRange[i];
                    aScenBase[i] = mfAlpha * (aScenRange[i] / fPerIdx)
                                   + (1.0 - mfAlpha) * (aScenBase[i - 1] + aScenTrend[i - 1]);
                    aScenTrend[i] = mfGamma * (aScenBase[i] - aScenBase[i - 1])
                                    + (1.0 - mfGamma) * aScenTrend[i - 1];
                    aScenPerIdx[i] = mfBeta * (aScenRange[i] / aScenBase[i])
                                     + (1.0 - mfBeta) * fPerIdx;
                }
            }
            if (meError != api::Error::None)
                return false;
        }

        std::vector<double> aPercentiles(nSize);
        for (std::size_t i = 0; i < nSize; ++i)
        {
            math::AggregateScan aUpperScan;
            aUpperScan.maNumbers = aPredictions[i];
            const auto aUpper
                = math::evaluateAggregateRankedNumbers(16, aUpperScan, (1.0 + fPiLevel) / 2.0);
            if (!aUpper)
            {
                meError = aUpper.meError;
                return false;
            }

            math::AggregateScan aMedianScan;
            aMedianScan.maNumbers = aPredictions[i];
            const auto aMedian = math::evaluateAggregateRankedNumbers(16, aMedianScan, 0.5);
            if (!aMedian)
            {
                meError = aMedian.meError;
                return false;
            }
            aPercentiles[i] = aUpper.maValue - aMedian.maValue;
        }

        return fillPredictionIntervalOutput(rTarget, rOut, [&](std::size_t nSteps, double fFactor) {
            double fPi = aPercentiles[nSteps];
            if (fFactor != 0.0)
            {
                const double fPi1 = aPercentiles[nSteps + 1];
                fPi += fFactor * (fPi1 - fPi);
            }
            return fPi;
        });
    }

    bool fillEdsPredictionIntervals(const MatrixOperand& rTarget, MatrixOperand& rOut, double fPiLevel)
    {
        initCalc();
        if (meError != api::Error::None || !prepareLike(rTarget, rOut))
            return false;

        double fMaxTarget = 0.0;
        if (!tryReadMatrixNumber(rTarget, 0, fMaxTarget))
        {
            meError = api::Error::IllegalArgument;
            return false;
        }
        for (api::MatrixSize nRow = 0; nRow < rTarget.maDimensions.mnRows; ++nRow)
        {
            for (api::MatrixSize nColumn = 0; nColumn < rTarget.maDimensions.mnColumns; ++nColumn)
            {
                double fCurrent = 0.0;
                if (!tryReadMatrixNumberAt(rTarget, nColumn, nRow, fCurrent))
                {
                    meError = api::Error::IllegalArgument;
                    return false;
                }
                if (fCurrent > fMaxTarget)
                    fMaxTarget = fCurrent;
            }
        }

        fMaxTarget = (mnMonthDay ? convertXtoMonths(fMaxTarget) : fMaxTarget) - maRange[mnCount - 1].mfX;
        std::size_t nSize = static_cast<std::size_t>(fMaxTarget / mfStepSize);
        if (std::fmod(fMaxTarget, mfStepSize) != 0.0)
            ++nSize;
        if (nSize == 0)
        {
            meError = api::Error::IllegalArgument;
            return false;
        }

        const auto aNormal = math::evaluateStandardNormalInverse((1.0 + fPiLevel) / 2.0);
        if (!aNormal)
        {
            meError = aNormal.meError;
            return false;
        }
        const double fZ = aNormal.maValue;
        const double fO = 1.0 - fPiLevel;
        std::vector<double> aCoefficients(nSize);
        for (std::size_t i = 0; i < nSize; ++i)
        {
            aCoefficients[i]
                = std::sqrt(1.0 + (fPiLevel / std::pow(1.0 + fO, 3.0))
                                       * ((1.0 + 4.0 * fO + 5.0 * fO * fO)
                                          + 2.0 * static_cast<double>(i) * fPiLevel * (1.0 + 3.0 * fO)
                                          + 2.0 * static_cast<double>(i * i) * fPiLevel * fPiLevel));
        }

        return fillPredictionIntervalOutput(rTarget, rOut, [&](std::size_t nSteps, double fFactor) {
            double fPi = fZ * mfRMSE * aCoefficients[nSteps] / aCoefficients[0];
            if (fFactor != 0.0)
            {
                const double fPi1 = fZ * mfRMSE * aCoefficients[nSteps + 1] / aCoefficients[0];
                fPi += fFactor * (fPi1 - fPi);
            }
            return fPi;
        });
    }

private:
    const api::DateParts& mrNullDate;
    std::vector<DataPoint> maRange;
    std::vector<double> maBase;
    std::vector<double> maTrend;
    std::vector<double> maPerIdx;
    std::vector<double> maForecast;
    std::size_t mnSmplInPrd = 0;
    double mfStepSize = 0.0;
    double mfAlpha = 0.0;
    double mfBeta = 0.0;
    double mfGamma = 0.0;
    std::size_t mnCount = 0;
    bool mbInitialised = false;
    std::int32_t mnMonthDay = 0;
    double mfMAE = 0.0;
    double mfMASE = 0.0;
    double mfMSE = 0.0;
    double mfRMSE = 0.0;
    double mfSMAPE = 0.0;
    api::Error meError = api::Error::None;
    bool mbAdditive = false;
    bool mbEDS = false;

    [[nodiscard]] bool initData()
    {
        maBase.assign(mnCount, 0.0);
        maTrend.assign(mnCount, 0.0);
        maForecast.assign(mnCount, 0.0);
        if (!mbEDS)
            maPerIdx.assign(mnCount, 0.0);
        maForecast[0] = maRange[0].mfY;

        if (!prefillTrendData())
            return false;
        if (!prefillPeriodIndex())
            return false;

        prefillBaseData();
        return true;
    }

    [[nodiscard]] bool prefillTrendData()
    {
        if (mbEDS)
        {
            maTrend[0] = (maRange[mnCount - 1].mfY - maRange[0].mfY)
                         / static_cast<double>(mnCount - 1);
            return true;
        }

        if (mnCount < 2 * mnSmplInPrd)
        {
            meError = api::Error::NoValue;
            return false;
        }

        fp::KahanSum aSum;
        for (std::size_t i = 0; i < mnSmplInPrd; ++i)
        {
            aSum += maRange[i + mnSmplInPrd].mfY;
            aSum -= maRange[i].mfY;
        }
        maTrend[0] = aSum.get() / static_cast<double>(mnSmplInPrd * mnSmplInPrd);
        return true;
    }

    [[nodiscard]] bool prefillPeriodIndex()
    {
        if (mbEDS)
            return true;

        if (mnSmplInPrd == 0)
        {
            meError = api::Error::IllegalArgument;
            return false;
        }

        const std::size_t nPeriods = mnCount / mnSmplInPrd;
        std::vector<fp::KahanSum> aPeriodAverage(nPeriods);
        for (std::size_t i = 0; i < nPeriods; ++i)
        {
            for (std::size_t j = 0; j < mnSmplInPrd; ++j)
                aPeriodAverage[i] += maRange[i * mnSmplInPrd + j].mfY;
            aPeriodAverage[i] = aPeriodAverage[i].get() / static_cast<double>(mnSmplInPrd);
            if (aPeriodAverage[i].get() == 0.0)
            {
                meError = api::Error::DivisionByZero;
                return false;
            }
        }

        for (std::size_t j = 0; j < mnSmplInPrd; ++j)
        {
            fp::KahanSum aIndex;
            for (std::size_t i = 0; i < nPeriods; ++i)
            {
                const double fOffset
                    = (static_cast<double>(j) - 0.5 * (mnSmplInPrd - 1)) * maTrend[0];
                if (mbAdditive)
                    aIndex += maRange[i * mnSmplInPrd + j].mfY - (aPeriodAverage[i].get() + fOffset);
                else
                    aIndex += maRange[i * mnSmplInPrd + j].mfY / (aPeriodAverage[i].get() + fOffset);
            }
            maPerIdx[j] = aIndex.get() / static_cast<double>(nPeriods);
        }
        if (mnSmplInPrd < mnCount)
            maPerIdx[mnSmplInPrd] = 0.0;
        return true;
    }

    void prefillBaseData()
    {
        maBase[0] = mbEDS ? maRange[0].mfY : maRange[0].mfY / maPerIdx[0];
    }

    void initCalc()
    {
        if (mbInitialised)
            return;
        calcAlphaBetaGamma();
        if (meError == api::Error::None)
            calcAccuracyIndicators();
        mbInitialised = true;
    }

    void calcAccuracyIndicators()
    {
        fp::KahanSum aSumAbsErr;
        fp::KahanSum aSumDivisor;
        fp::KahanSum aSumErrSq;
        fp::KahanSum aSumAbsPercErr;

        for (std::size_t i = 1; i < mnCount; ++i)
        {
            const double fError = maForecast[i] - maRange[i].mfY;
            aSumAbsErr += std::fabs(fError);
            aSumErrSq += fError * fError;
            aSumAbsPercErr += std::fabs(fError) / (std::fabs(maForecast[i]) + std::fabs(maRange[i].mfY));
        }
        for (std::size_t i = 2; i < mnCount; ++i)
            aSumDivisor += std::fabs(maRange[i].mfY - maRange[i - 1].mfY);

        const double fCalcCount = static_cast<double>(mnCount - 1);
        mfMAE = aSumAbsErr.get() / fCalcCount;
        const double fMaseDivisor
            = mnCount <= 2 ? 0.0
                           : fCalcCount * aSumDivisor.get() / static_cast<double>(mnCount - 2);
        mfMASE = fMaseDivisor == 0.0 ? 0.0 : aSumAbsErr.get() / fMaseDivisor;
        mfMSE = aSumErrSq.get() / fCalcCount;
        mfRMSE = std::sqrt(mfMSE);
        mfSMAPE = aSumAbsPercErr.get() * 2.0 / fCalcCount;
    }

    [[nodiscard]] std::size_t calcPeriodLen() const
    {
        std::size_t nBestValue = mnCount;
        double fBestMeanError = std::numeric_limits<double>::max();

        for (std::size_t nPeriodLen = mnCount / 2; nPeriodLen > 0; --nPeriodLen)
        {
            fp::KahanSum aMeanError;
            const std::size_t nPeriods = mnCount / nPeriodLen;
            const std::size_t nStart = mnCount - (nPeriods * nPeriodLen) + 1;
            for (std::size_t i = nStart; i < (mnCount - nPeriodLen); ++i)
            {
                aMeanError += std::fabs((maRange[i].mfY - maRange[i - 1].mfY)
                                        - (maRange[nPeriodLen + i].mfY - maRange[nPeriodLen + i - 1].mfY));
            }
            if ((nPeriods - 1) * nPeriodLen <= 1)
                continue;
            double fMeanError = aMeanError.get();
            fMeanError /= static_cast<double>((nPeriods - 1) * nPeriodLen - 1);

            if (fMeanError <= fBestMeanError || fMeanError == 0.0)
            {
                nBestValue = nPeriodLen;
                fBestMeanError = fMeanError;
            }
        }

        return nBestValue;
    }

    void calcAlphaBetaGamma()
    {
        double f0 = 0.0;
        mfAlpha = f0;
        if (mbEDS)
        {
            mfBeta = 0.0;
            calcGamma();
        }
        else
        {
            calcBetaGamma();
        }
        refill();
        double fE0 = mfMSE;

        double f2 = 1.0;
        mfAlpha = f2;
        if (mbEDS)
            calcGamma();
        else
            calcBetaGamma();
        refill();
        double fE2 = mfMSE;

        double f1 = 0.5;
        mfAlpha = f1;
        if (mbEDS)
            calcGamma();
        else
            calcBetaGamma();
        refill();

        if (fE0 == mfMSE && mfMSE == fE2)
        {
            mfAlpha = 0.0;
            if (mbEDS)
                calcGamma();
            else
                calcBetaGamma();
            refill();
            return;
        }

        while ((f2 - f1) > kMinAbcResolution)
        {
            if (fE2 > fE0)
            {
                f2 = f1;
                fE2 = mfMSE;
                f1 = (f0 + f1) / 2.0;
            }
            else
            {
                f0 = f1;
                fE0 = mfMSE;
                f1 = (f1 + f2) / 2.0;
            }
            mfAlpha = f1;
            if (mbEDS)
                calcGamma();
            else
                calcBetaGamma();
            refill();
        }

        if (fE2 > fE0)
        {
            if (fE0 < mfMSE)
            {
                mfAlpha = f0;
                if (mbEDS)
                    calcGamma();
                else
                    calcBetaGamma();
                refill();
            }
        }
        else if (fE2 < mfMSE)
        {
            mfAlpha = f2;
            if (mbEDS)
                calcGamma();
            else
                calcBetaGamma();
            refill();
        }
        calcAccuracyIndicators();
    }

    void calcBetaGamma()
    {
        double f0 = 0.0;
        mfBeta = f0;
        calcGamma();
        refill();
        double fE0 = mfMSE;

        double f2 = 1.0;
        mfBeta = f2;
        calcGamma();
        refill();
        double fE2 = mfMSE;

        double f1 = 0.5;
        mfBeta = f1;
        calcGamma();
        refill();

        if (fE0 == mfMSE && mfMSE == fE2)
        {
            mfBeta = 0.0;
            calcGamma();
            refill();
            return;
        }

        while ((f2 - f1) > kMinAbcResolution)
        {
            if (fE2 > fE0)
            {
                f2 = f1;
                fE2 = mfMSE;
                f1 = (f0 + f1) / 2.0;
            }
            else
            {
                f0 = f1;
                fE0 = mfMSE;
                f1 = (f1 + f2) / 2.0;
            }
            mfBeta = f1;
            calcGamma();
            refill();
        }

        if (fE2 > fE0)
        {
            if (fE0 < mfMSE)
            {
                mfBeta = f0;
                calcGamma();
                refill();
            }
        }
        else if (fE2 < mfMSE)
        {
            mfBeta = f2;
            calcGamma();
            refill();
        }
    }

    void calcGamma()
    {
        double f0 = 0.0;
        mfGamma = f0;
        refill();
        double fE0 = mfMSE;

        double f2 = 1.0;
        mfGamma = f2;
        refill();
        double fE2 = mfMSE;

        double f1 = 0.5;
        mfGamma = f1;
        refill();

        if (fE0 == mfMSE && mfMSE == fE2)
        {
            mfGamma = 0.0;
            refill();
            return;
        }

        while ((f2 - f1) > kMinAbcResolution)
        {
            if (fE2 > fE0)
            {
                f2 = f1;
                fE2 = mfMSE;
                f1 = (f0 + f1) / 2.0;
            }
            else
            {
                f0 = f1;
                fE0 = mfMSE;
                f1 = (f1 + f2) / 2.0;
            }
            mfGamma = f1;
            refill();
        }

        if (fE2 > fE0)
        {
            if (fE0 < mfMSE)
            {
                mfGamma = f0;
                refill();
            }
        }
        else if (fE2 < mfMSE)
        {
            mfGamma = f2;
            refill();
        }
    }

    void refill()
    {
        for (std::size_t i = 1; i < mnCount; ++i)
        {
            if (mbEDS)
            {
                maBase[i] = mfAlpha * maRange[i].mfY
                            + (1.0 - mfAlpha) * (maBase[i - 1] + maTrend[i - 1]);
                maTrend[i] = mfGamma * (maBase[i] - maBase[i - 1])
                             + (1.0 - mfGamma) * maTrend[i - 1];
                maForecast[i] = maBase[i - 1] + maTrend[i - 1];
            }
            else
            {
                std::size_t nIndex = 0;
                if (mbAdditive)
                {
                    nIndex = i > mnSmplInPrd ? i - mnSmplInPrd : i;
                    maBase[i] = mfAlpha * (maRange[i].mfY - maPerIdx[nIndex])
                                + (1.0 - mfAlpha) * (maBase[i - 1] + maTrend[i - 1]);
                    maPerIdx[i] = mfBeta * (maRange[i].mfY - maBase[i])
                                  + (1.0 - mfBeta) * maPerIdx[nIndex];
                }
                else
                {
                    nIndex = i >= mnSmplInPrd ? i - mnSmplInPrd : i;
                    maBase[i] = mfAlpha * (maRange[i].mfY / maPerIdx[nIndex])
                                + (1.0 - mfAlpha) * (maBase[i - 1] + maTrend[i - 1]);
                    maPerIdx[i] = mfBeta * (maRange[i].mfY / maBase[i])
                                  + (1.0 - mfBeta) * maPerIdx[nIndex];
                }
                maTrend[i] = mfGamma * (maBase[i] - maBase[i - 1])
                             + (1.0 - mfGamma) * maTrend[i - 1];
                maForecast[i] = mbAdditive
                                    ? maBase[i - 1] + maTrend[i - 1] + maPerIdx[nIndex]
                                    : (maBase[i - 1] + maTrend[i - 1]) * maPerIdx[nIndex];
            }
        }
        calcAccuracyIndicators();
    }

    [[nodiscard]] double convertXtoMonths(double fTarget) const
    {
        return convertSerialToMonthCoordinate(mrNullDate, fTarget, mnMonthDay);
    }

    [[nodiscard]] api::ValueResult<double> getForecast(double fTarget)
    {
        initCalc();
        if (meError != api::Error::None)
            return api::ValueResult<double>::failure(meError);

        double fForecast = 0.0;
        if (fTarget <= maRange[mnCount - 1].mfX)
        {
            const std::size_t nIndex = static_cast<std::size_t>(
                (fTarget - maRange[0].mfX) / mfStepSize);
            const double fInterpolate = std::fmod(fTarget - maRange[0].mfX, mfStepSize);
            fForecast = maRange[nIndex].mfY;
            if (fInterpolate >= kMinAbcResolution)
            {
                const double fInterpolateFactor = fInterpolate / mfStepSize;
                const double fForecastNext = maForecast[nIndex + 1];
                fForecast += fInterpolateFactor * (fForecastNext - fForecast);
            }
        }
        else
        {
            const std::size_t nIndex = static_cast<std::size_t>(
                (fTarget - maRange[mnCount - 1].mfX) / mfStepSize);
            const double fInterpolate = std::fmod(fTarget - maRange[mnCount - 1].mfX, mfStepSize);
            if (mbEDS)
            {
                fForecast = maBase[mnCount - 1] + nIndex * maTrend[mnCount - 1];
            }
            else if (mbAdditive)
            {
                fForecast = maBase[mnCount - 1] + nIndex * maTrend[mnCount - 1]
                            + maPerIdx[mnCount - 1 - mnSmplInPrd + (nIndex % mnSmplInPrd)];
            }
            else
            {
                fForecast = (maBase[mnCount - 1] + nIndex * maTrend[mnCount - 1])
                            * maPerIdx[mnCount - 1 - mnSmplInPrd + (nIndex % mnSmplInPrd)];
            }

            if (fInterpolate >= kMinAbcResolution)
            {
                const double fInterpolateFactor = fInterpolate / mfStepSize;
                double fForecastNext = 0.0;
                if (mbEDS)
                {
                    fForecastNext = maBase[mnCount - 1] + (nIndex + 1) * maTrend[mnCount - 1];
                }
                else if (mbAdditive)
                {
                    fForecastNext = maBase[mnCount - 1] + (nIndex + 1) * maTrend[mnCount - 1]
                                    + maPerIdx[mnCount - 1 - mnSmplInPrd
                                               + ((nIndex + 1) % mnSmplInPrd)];
                }
                else
                {
                    fForecastNext = (maBase[mnCount - 1] + (nIndex + 1) * maTrend[mnCount - 1])
                                    * maPerIdx[mnCount - 1 - mnSmplInPrd
                                               + ((nIndex + 1) % mnSmplInPrd)];
                }
                fForecast += fInterpolateFactor * (fForecastNext - fForecast);
            }
        }
        return api::ValueResult<double>::success(fForecast);
    }

    [[nodiscard]] double randDeviation()
    {
        const double fProbability = ::comphelper::rng::uniform_real_distribution(0.5, 1.0);
        const auto aNormal = math::evaluateStandardNormalInverse(fProbability);
        if (!aNormal)
        {
            meError = aNormal.meError;
            return 0.0;
        }
        return mfRMSE * aNormal.maValue;
    }

    [[nodiscard]] bool prepareLike(const MatrixOperand& rSource, MatrixOperand& rOut) const
    {
        rOut.maDimensions = rSource.maDimensions;
        rOut.meProvenance = MatrixProvenance::ComputedResult;
        rOut.maValues.assign(rSource.cellCount(), api::CellValue::number(0.0));
        return true;
    }

    static void putNumber(MatrixOperand& rMatrix, api::MatrixSize nColumn, api::MatrixSize nRow, double fValue)
    {
        const std::size_t nIndex
            = static_cast<std::size_t>(nRow) * rMatrix.maDimensions.mnColumns + nColumn;
        rMatrix.maValues[nIndex] = api::CellValue::number(fValue);
    }

    template <typename TCompute>
    bool fillPredictionIntervalOutput(const MatrixOperand& rTarget, MatrixOperand& rOut, TCompute aCompute)
    {
        for (api::MatrixSize nRow = 0; nRow < rTarget.maDimensions.mnRows; ++nRow)
        {
            for (api::MatrixSize nColumn = 0; nColumn < rTarget.maDimensions.mnColumns; ++nColumn)
            {
                double fTarget = 0.0;
                if (!tryReadMatrixNumberAt(rTarget, nColumn, nRow, fTarget))
                {
                    meError = api::Error::IllegalArgument;
                    return false;
                }

                fTarget = (mnMonthDay ? convertXtoMonths(fTarget) : fTarget) - maRange[mnCount - 1].mfX;
                const std::size_t nSteps = static_cast<std::size_t>(fTarget / mfStepSize) - 1;
                const double fFactor = std::fmod(fTarget, mfStepSize);
                putNumber(rOut, nColumn, nRow, aCompute(nSteps, fFactor));
            }
        }
        return true;
    }
};

} // namespace detail::ets

[[nodiscard]] inline RpnCoercionResult<ForecastEtsPlanResult> planForecastEts(
    ForecastEtsVariant eVariant, const api::DateParts& rNullDate, const MatrixOperand& rTargetOrType,
    const MatrixOperand& rKnownY, const MatrixOperand& rKnownX, const MatrixOperand* pConfidence,
    const MatrixOperand* pSeasonality, const MatrixOperand* pDataCompletion,
    const MatrixOperand* pAggregation)
{
    if (rKnownY.isEmpty() || rKnownX.isEmpty()
        || rKnownY.maDimensions.mnColumns != rKnownX.maDimensions.mnColumns
        || rKnownY.maDimensions.mnRows != rKnownX.maDimensions.mnRows)
    {
        return RpnCoercionResult<ForecastEtsPlanResult>::failure(api::Error::IllegalArgument);
    }

    const auto aAggregation = detail::ets::extractWholeNumber(pAggregation, 1);
    if (!aAggregation)
        return RpnCoercionResult<ForecastEtsPlanResult>::failure(aAggregation.meError);
    if (aAggregation.maValue < 1 || aAggregation.maValue > 7)
        return RpnCoercionResult<ForecastEtsPlanResult>::failure(api::Error::IllegalArgument);

    const auto aDataCompletion = detail::ets::extractBinaryFlag(pDataCompletion, true);
    if (!aDataCompletion)
        return RpnCoercionResult<ForecastEtsPlanResult>::failure(aDataCompletion.meError);

    const auto aSamplesInPeriod = detail::ets::extractWholeNumber(
        pSeasonality, eVariant == ForecastEtsVariant::Seasonality ? 1 : 1);
    if (!aSamplesInPeriod)
        return RpnCoercionResult<ForecastEtsPlanResult>::failure(aSamplesInPeriod.meError);
    if (aSamplesInPeriod.maValue < 0)
        return RpnCoercionResult<ForecastEtsPlanResult>::failure(api::Error::IllegalArgument);

    double fPiLevel = 0.0;
    if (detail::ets::isPiVariant(eVariant))
    {
        const auto aPiLevel = detail::ets::extractScalarNumber(pConfidence, 0.95);
        if (!aPiLevel)
            return RpnCoercionResult<ForecastEtsPlanResult>::failure(aPiLevel.meError);
        if (aPiLevel.maValue < 0.0 || aPiLevel.maValue > 1.0)
            return RpnCoercionResult<ForecastEtsPlanResult>::failure(api::Error::IllegalArgument);
        fPiLevel = aPiLevel.maValue;
    }

    if (detail::ets::isStatsVariant(eVariant))
    {
        for (const auto& rValue : rTargetOrType.maValues)
        {
            if (!detail::ets::isNumericCell(rValue))
                return RpnCoercionResult<ForecastEtsPlanResult>::failure(api::Error::IllegalArgument);
            const int nType = static_cast<int>(rValue.mfNumber);
            if (nType < 1 || nType > 9)
                return RpnCoercionResult<ForecastEtsPlanResult>::failure(api::Error::IllegalArgument);
        }
    }
    else if (eVariant != ForecastEtsVariant::Seasonality)
    {
        for (const auto& rValue : rTargetOrType.maValues)
        {
            if (!detail::ets::isNumericCell(rValue))
                return RpnCoercionResult<ForecastEtsPlanResult>::failure(api::Error::IllegalArgument);
        }
    }

    detail::ets::ForecastEtsCalculation aCalculation(rKnownX.cellCount(), rNullDate);
    if (!aCalculation.preprocess(
            rKnownX, rKnownY, aSamplesInPeriod.maValue, aDataCompletion.maValue,
            aAggregation.maValue,
            detail::ets::isStatsVariant(eVariant) || eVariant == ForecastEtsVariant::Seasonality
                ? nullptr
                : &rTargetOrType,
            eVariant))
    {
        return RpnCoercionResult<ForecastEtsPlanResult>::failure(aCalculation.getError());
    }

    ForecastEtsPlanResult aResult;
    switch (eVariant)
    {
        case ForecastEtsVariant::Add:
        case ForecastEtsVariant::Mult:
            if (!aCalculation.fillForecastRange(rTargetOrType, aResult.maMatrix))
                return RpnCoercionResult<ForecastEtsPlanResult>::failure(aCalculation.getError());
            break;
        case ForecastEtsVariant::PIAdd:
        case ForecastEtsVariant::PIMult:
            if (aSamplesInPeriod.maValue == 0)
            {
                if (!aCalculation.fillEdsPredictionIntervals(rTargetOrType, aResult.maMatrix, fPiLevel))
                {
                    return RpnCoercionResult<ForecastEtsPlanResult>::failure(aCalculation.getError());
                }
            }
            else if (!aCalculation.fillEtsPredictionIntervals(rTargetOrType, aResult.maMatrix, fPiLevel))
            {
                return RpnCoercionResult<ForecastEtsPlanResult>::failure(aCalculation.getError());
            }
            break;
        case ForecastEtsVariant::StatAdd:
        case ForecastEtsVariant::StatMult:
            if (!aCalculation.fillStatistics(rTargetOrType, aResult.maMatrix))
                return RpnCoercionResult<ForecastEtsPlanResult>::failure(aCalculation.getError());
            break;
        case ForecastEtsVariant::Seasonality:
        {
            const auto aSamples = aCalculation.samplesInPeriod();
            if (!aSamples)
                return RpnCoercionResult<ForecastEtsPlanResult>::failure(aSamples.meError);
            aResult.mbIsScalar = true;
            aResult.mfScalar = aSamples.maValue;
            break;
        }
    }

    return RpnCoercionResult<ForecastEtsPlanResult>::success(aResult);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
