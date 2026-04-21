/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cfloat>
#include <cmath>
#include <cstddef>
#include <rtl/math.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>

namespace spreadsheetengine::core::rpn
{

struct RandomOutputFrame
{
    bool mbArrayContext = false;
    bool mbSingleCellScalarCompat = false;
    api::MatrixDimensions maDimensions { 1, 1 };

    [[nodiscard]] constexpr api::MatrixDimensions resultDimensions() const
    {
        if (!mbArrayContext)
            return { 1, 1 };

        api::MatrixDimensions aResult = maDimensions;
        if (aResult.mnColumns <= 0)
            aResult.mnColumns = 1;
        if (aResult.mnRows <= 0)
            aResult.mnRows = 1;
        return aResult;
    }

    [[nodiscard]] constexpr bool returnsScalar() const
    {
        return !mbArrayContext || (mbSingleCellScalarCompat && maDimensions.mnColumns == 1
                                   && maDimensions.mnRows == 1);
    }
};

struct RandomPlanResult
{
    bool mbIsScalar = true;
    double mfScalar = 0.0;
    MatrixOperand maMatrix;
};

namespace detail::random
{

template <typename Sampler>
[[nodiscard]] inline api::ValueResult<RandomPlanResult> sampleIntoResult(
    const api::MatrixDimensions& rDimensions, bool bReturnScalar, Sampler&& rSampler)
{
    if (rDimensions.mnColumns <= 0 || rDimensions.mnRows <= 0)
        return api::ValueResult<RandomPlanResult>::failure(api::Error::IllegalArgument);

    RandomPlanResult aResult;
    if (bReturnScalar)
    {
        const auto aSample = rSampler();
        if (!aSample)
            return api::ValueResult<RandomPlanResult>::failure(aSample.meError);
        aResult.mfScalar = aSample.maValue;
        return api::ValueResult<RandomPlanResult>::success(aResult);
    }

    aResult.mbIsScalar = false;
    aResult.maMatrix.maDimensions = rDimensions;
    aResult.maMatrix.meProvenance = MatrixProvenance::ComputedResult;
    aResult.maMatrix.maValues.assign(static_cast<std::size_t>(rDimensions.elementCount()),
        api::CellValue::empty());

    // Legacy Calc fills volatile matrices column-major; preserve draw order
    // while storing the result in row-major MatrixOperand form.
    for (api::MatrixSize nColumn = 0; nColumn < rDimensions.mnColumns; ++nColumn)
    {
        for (api::MatrixSize nRow = 0; nRow < rDimensions.mnRows; ++nRow)
        {
            const auto aSample = rSampler();
            if (!aSample)
                return api::ValueResult<RandomPlanResult>::failure(aSample.meError);

            const std::size_t nIndex
                = static_cast<std::size_t>(nRow) * rDimensions.mnColumns + nColumn;
            aResult.maMatrix.maValues[nIndex] = api::CellValue::number(aSample.maValue);
        }
    }

    return api::ValueResult<RandomPlanResult>::success(aResult);
}

[[nodiscard]] inline api::ValueResult<double> drawUniformReal(
    const api::RuntimeEnvironment& rEnvironment, double fLowerInclusive,
    double fUpperExclusive)
{
    return rEnvironment.sampleUniformReal(fLowerInclusive, fUpperExclusive);
}

[[nodiscard]] inline api::ValueResult<double> drawUniformWhole(
    const api::RuntimeEnvironment& rEnvironment, double fLowerInclusive,
    double fUpperExclusive)
{
    const auto aSample = rEnvironment.sampleUniformReal(fLowerInclusive, fUpperExclusive);
    if (!aSample)
        return api::ValueResult<double>::failure(aSample.meError);
    return api::ValueResult<double>::success(std::floor(aSample.maValue));
}

} // namespace detail::random

[[nodiscard]] inline api::ValueResult<RandomPlanResult> planRandom(
    const api::RuntimeEnvironment& rEnvironment, const RandomOutputFrame& rFrame)
{
    const api::MatrixDimensions aDimensions = rFrame.resultDimensions();
    return detail::random::sampleIntoResult(aDimensions, rFrame.returnsScalar(),
        [&]() { return detail::random::drawUniformReal(rEnvironment, 0.0, 1.0); });
}

[[nodiscard]] inline api::ValueResult<RandomPlanResult> planRandbetween(
    const api::RuntimeEnvironment& rEnvironment, const RandomOutputFrame& rFrame, double fMin,
    double fMax)
{
    const double fRoundedMax = rtl::math::round(fMax, 0, rtl_math_RoundingMode_Up);
    const double fRoundedMin = rtl::math::round(fMin, 0, rtl_math_RoundingMode_Up);
    if (fRoundedMin > fRoundedMax)
        return api::ValueResult<RandomPlanResult>::failure(api::Error::IllegalArgument);

    const api::MatrixDimensions aDimensions = rFrame.resultDimensions();
    const double fUpperBound = std::nextafter(fRoundedMax + 1.0, -DBL_MAX);
    return detail::random::sampleIntoResult(aDimensions, rFrame.returnsScalar(), [&]() {
        return detail::random::drawUniformWhole(rEnvironment, fRoundedMin, fUpperBound);
    });
}

[[nodiscard]] inline api::ValueResult<RandomPlanResult> planRandArray(
    const api::RuntimeEnvironment& rEnvironment, api::MatrixDimensions aDimensions, double fMin,
    double fMax, bool bWholeNumber)
{
    if (bWholeNumber)
    {
        fMax = rtl::math::round(fMax, 0, rtl_math_RoundingMode_Up);
        fMin = rtl::math::round(fMin, 0, rtl_math_RoundingMode_Up);
    }

    if (fMin > fMax || aDimensions.mnColumns <= 0 || aDimensions.mnRows <= 0)
        return api::ValueResult<RandomPlanResult>::failure(api::Error::IllegalArgument);

    const double fUpperBound = bWholeNumber ? std::nextafter(fMax + 1.0, -DBL_MAX)
                                            : std::nextafter(fMax, DBL_MAX);
    return detail::random::sampleIntoResult(
        aDimensions, aDimensions.mnColumns == 1 && aDimensions.mnRows == 1, [&]() {
            return bWholeNumber
                       ? detail::random::drawUniformWhole(rEnvironment, fMin, fUpperBound)
                       : detail::random::drawUniformReal(rEnvironment, fMin, fUpperBound);
        });
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
