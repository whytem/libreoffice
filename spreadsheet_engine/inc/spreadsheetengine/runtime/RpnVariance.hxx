/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <spreadsheetengine/api/Types.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 3 tail substrate: numerically-stable variance accumulator plus a
// criteria-aware planner that drives it from the standard
// CriteriaAggregateMaterializer + criteria-ranges / predicates / target
// range triple used elsewhere in the DB-family admissions.
//
// VarianceAccumulator implements Welford's online algorithm: it carries a
// running mean and M2 (sum of squared deviations from the running mean)
// and updates both in place as values stream in. Welford is the usual
// recipe for single-pass variance that preserves numeric accuracy when
// the input values are close to each other or far from zero; the
// two-pass formula in MathAggregate::evaluateVarianceNumbers (mean
// subtraction + Kahan-summed squared deviation) is equivalent in result
// but requires the full value list up front. For the DB variance
// family we want to stream the match set through the materializer
// without buffering, which Welford supports directly.
//
// The planner itself is the thin wrapper that turns a set of criteria
// ranges + predicates + target range into a VarianceResult. Admission
// helpers on the Calc side feed it the same CriteriaAggregateInput
// triple they build for DSUM / DCOUNT / …; the planner iterates the
// base range coordinate-by-coordinate, runs the same per-row predicate
// check that evaluateCriteriaAggregate runs, and feeds surviving
// numeric target values to a VarianceAccumulator.

namespace spreadsheetengine::core::rpn
{

// Welford-style running variance accumulator. All updates are O(1);
// finalizing for sample or population variance / stddev is a single
// division + optional sqrt.
class VarianceAccumulator
{
    std::size_t mnCount = 0;
    double mfMean = 0.0;
    double mfM2 = 0.0;

public:
    void add(double fValue)
    {
        ++mnCount;
        const double fDelta = fValue - mfMean;
        mfMean += fDelta / static_cast<double>(mnCount);
        const double fDelta2 = fValue - mfMean;
        mfM2 += fDelta * fDelta2;
    }

    [[nodiscard]] std::size_t count() const { return mnCount; }
    [[nodiscard]] double mean() const { return mfMean; }
    [[nodiscard]] double sumSquaredDeviations() const { return mfM2; }

    // Returns the sample variance (divisor n-1). Caller must check
    // count() >= 2 before using the result; this accessor does not
    // enforce that.
    [[nodiscard]] double sampleVariance() const
    {
        return mnCount >= 2 ? mfM2 / static_cast<double>(mnCount - 1) : 0.0;
    }

    // Returns the population variance (divisor n). Caller must check
    // count() >= 1.
    [[nodiscard]] double populationVariance() const
    {
        return mnCount >= 1 ? mfM2 / static_cast<double>(mnCount) : 0.0;
    }
};

enum class VarianceKind : std::uint8_t
{
    SampleVariance = 0,
    PopulationVariance,
    SampleStandardDeviation,
    PopulationStandardDeviation
};

[[nodiscard]] inline bool isSampleVarianceKind(VarianceKind eKind)
{
    return eKind == VarianceKind::SampleVariance
           || eKind == VarianceKind::SampleStandardDeviation;
}

[[nodiscard]] inline bool isStandardDeviationKind(VarianceKind eKind)
{
    return eKind == VarianceKind::SampleStandardDeviation
           || eKind == VarianceKind::PopulationStandardDeviation;
}

[[nodiscard]] inline api::ValueResult<double> finalizeVariance(
    const VarianceAccumulator& rAccumulator, VarianceKind eKind)
{
    const std::size_t nCount = rAccumulator.count();
    if (nCount == 0)
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);
    if (isSampleVarianceKind(eKind) && nCount < 2)
        return api::ValueResult<double>::failure(api::Error::DivisionByZero);

    const double fVariance = isSampleVarianceKind(eKind) ? rAccumulator.sampleVariance()
                                                         : rAccumulator.populationVariance();
    const double fResult = isStandardDeviationKind(eKind) ? std::sqrt(fVariance) : fVariance;
    return api::ValueResult<double>::success(fResult);
}

// Criteria-aware database variance plan. Iterates the criteria-range
// coordinate grid identical to evaluateCriteriaAggregate, runs the
// same predicate check per row, and when the row matches, materializes
// the target-range cell and folds its numeric value into the
// VarianceAccumulator. Non-numeric target values are skipped — this
// mirrors the ScDBQueryDataIterator default (mbSkipString = true) that
// GetDBStVarParams relies on for the legacy variance family.
struct DatabaseVarianceRequest
{
    std::vector<core::query::CriteriaAggregateInput> maCriteriaRanges;
    std::vector<core::query::CriteriaPredicate> maCriteria;
    core::query::CriteriaAggregateInput maTargetRange;
    VarianceKind meKind = VarianceKind::SampleVariance;
    api::query::SearchType meSearchType = api::query::SearchType::Normal;
    bool mbMatchWholeCell = true;
};

[[nodiscard]] inline api::ValueResult<double> planDatabaseVarianceAggregate(
    const core::query::CriteriaAggregateMaterializer& rMaterializer,
    const DatabaseVarianceRequest& rRequest)
{
    if (rRequest.maCriteriaRanges.empty()
        || rRequest.maCriteriaRanges.size() != rRequest.maCriteria.size())
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    const auto& rBase = rRequest.maCriteriaRanges.front();
    for (std::size_t nIndex = 1; nIndex < rRequest.maCriteriaRanges.size(); ++nIndex)
    {
        const auto& rOther = rRequest.maCriteriaRanges[nIndex];
        if (rOther.mnColumns != rBase.mnColumns || rOther.mnRows != rBase.mnRows)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
    if (rRequest.maTargetRange.mnColumns != rBase.mnColumns
        || rRequest.maTargetRange.mnRows != rBase.mnRows)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    VarianceAccumulator aAccumulator;
    for (api::MatrixSize nRow = 0; nRow < rBase.mnRows; ++nRow)
    {
        for (api::MatrixSize nCol = 0; nCol < rBase.mnColumns; ++nCol)
        {
            const api::MatrixCoordinate aCoord { nCol, nRow };
            bool bAllMatch = true;
            for (std::size_t nIndex = 0; nIndex < rRequest.maCriteriaRanges.size(); ++nIndex)
            {
                const auto aCandidate
                    = rMaterializer.materialize(rRequest.maCriteriaRanges[nIndex], aCoord);
                if (!aCandidate
                    || !core::query::matchesCriteriaPredicate(rRequest.maCriteria[nIndex],
                        aCandidate.maValue, rRequest.meSearchType, rRequest.mbMatchWholeCell))
                {
                    bAllMatch = false;
                    break;
                }
            }
            if (!bAllMatch)
                continue;

            const auto aTarget = rMaterializer.materialize(rRequest.maTargetRange, aCoord);
            if (!aTarget)
                return api::ValueResult<double>::failure(aTarget.meError);

            switch (aTarget.maValue.meKind)
            {
                case api::CellValueKind::Number:
                case api::CellValueKind::Boolean:
                    aAccumulator.add(aTarget.maValue.mfNumber);
                    break;
                default:
                    // Skip strings, errors, empties — legacy
                    // ScDBQueryDataIterator with mbSkipString = true
                    // does the same.
                    break;
            }
        }
    }

    return finalizeVariance(aAccumulator, rRequest.meKind);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
