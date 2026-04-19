/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 3 substrate: engine-native criteria-aggregate decision layer.
//
// This header defines *pure* planners that consume RpnValue inputs and
// produce CriteriaPredicate objects plus the orchestration descriptor the
// engine needs to drive COUNTIF / SUMIF / AVERAGEIF / MINIFS_MS / MAXIFS_MS
// / COUNTIFS / SUMIFS / AVERAGEIFS.
//
// The computational primitives themselves live in core::query
// (makeCriteriaPredicate, matchesCriteriaPredicate, evaluateCriteriaAggregate).
// This layer wraps them with the RpnValue-aware contract that defers
// reference and matrix operand resolution explicitly to the caller via
// NeedsReferenceResolution / NeedsMatrixMaterialization.
//
// Per the RPN Evaluator Initiative policy, no Calc opcode currently routes
// through this layer. It exists so the subsequent engine-first admissions
// for ocCountIf / ocSumIf / ocAverageIf / ocCountIfs / ocSumIfs /
// ocAverageIfs / ocMinIfs_MS / ocMaxIfs_MS / ocCountEmptyCells can consume
// a single tested decision substrate rather than re-deriving predicate
// parsing per opcode.

namespace spreadsheetengine::core::rpn
{

// Build a CriteriaPredicate from an RpnValue criterion argument. Scalar
// criteria (numbers, text, booleans, errors) are parsed through the
// canonical core::query::makeCriteriaPredicate path. References and
// matrices defer explicitly — the caller owns materialization.
[[nodiscard]] inline RpnCoercionResult<core::query::CriteriaPredicate> buildCriteriaPredicate(
    const RpnValue& rCriterion, core::query::CriteriaNumberTextParser pParseNumberText,
    core::query::CriteriaAsciiDoubleParser pParseAsciiDouble)
{
    using Result = RpnCoercionResult<core::query::CriteriaPredicate>;
    if (rCriterion.meKind == RpnValueKind::Reference)
        return Result::deferred(RpnCoercionReadiness::NeedsReferenceResolution);
    if (rCriterion.meKind == RpnValueKind::Matrix)
        return Result::deferred(RpnCoercionReadiness::NeedsMatrixMaterialization);

    auto oPredicate = core::query::makeCriteriaPredicate(
        rCriterion.maScalar, pParseNumberText, pParseAsciiDouble);
    if (!oPredicate)
        return Result::failure(api::Error::IllegalArgument);
    return Result::success(*oPredicate);
}

// Single-criterion aggregation request: one criteria range + one predicate
// plus an optional target range to aggregate over. If no target is
// supplied, the criteria range itself is aggregated (COUNTIF semantics).
struct SingleCriterionAggregateRequest
{
    core::query::CriteriaAggregateInput maCriteriaRange;
    core::query::CriteriaPredicate maPredicate;
    std::optional<core::query::CriteriaAggregateInput> moTargetRange;
    core::query::CriteriaAggregateKind meKind
        = core::query::CriteriaAggregateKind::Count;
    api::query::SearchType meSearchType = api::query::SearchType::Normal;
    bool mbMatchWholeCell = true;
};

[[nodiscard]] inline RpnCoercionResult<api::CellValue> planSingleCriterionAggregate(
    const core::query::CriteriaAggregateMaterializer& rMaterializer,
    const SingleCriterionAggregateRequest& rRequest)
{
    const std::vector<core::query::CriteriaAggregateInput> aRanges
        = { rRequest.maCriteriaRange };
    const std::vector<core::query::CriteriaPredicate> aCriteria
        = { rRequest.maPredicate };
    const core::query::CriteriaAggregateInput* pTargetRange
        = rRequest.moTargetRange ? &*rRequest.moTargetRange : nullptr;

    const auto aResult = core::query::evaluateCriteriaAggregate(
        rMaterializer, aRanges, aCriteria, pTargetRange, rRequest.meKind,
        rRequest.meSearchType, rRequest.mbMatchWholeCell);
    if (!aResult)
        return RpnCoercionResult<api::CellValue>::failure(aResult.meError);
    return RpnCoercionResult<api::CellValue>::success(aResult.maValue);
}

// Multi-criterion aggregation request: N parallel (range, predicate)
// pairs plus a target range to aggregate over when all criteria match.
// Mirrors COUNTIFS / SUMIFS / AVERAGEIFS / MINIFS_MS / MAXIFS_MS.
struct MultiCriterionAggregateRequest
{
    std::vector<core::query::CriteriaAggregateInput> maCriteriaRanges;
    std::vector<core::query::CriteriaPredicate> maCriteria;
    std::optional<core::query::CriteriaAggregateInput> moTargetRange;
    core::query::CriteriaAggregateKind meKind
        = core::query::CriteriaAggregateKind::Count;
    api::query::SearchType meSearchType = api::query::SearchType::Normal;
    bool mbMatchWholeCell = true;
};

[[nodiscard]] inline RpnCoercionResult<api::CellValue> planMultiCriterionAggregate(
    const core::query::CriteriaAggregateMaterializer& rMaterializer,
    const MultiCriterionAggregateRequest& rRequest)
{
    if (rRequest.maCriteriaRanges.size() != rRequest.maCriteria.size())
        return RpnCoercionResult<api::CellValue>::failure(api::Error::IllegalArgument);

    const core::query::CriteriaAggregateInput* pTargetRange
        = rRequest.moTargetRange ? &*rRequest.moTargetRange : nullptr;

    const auto aResult = core::query::evaluateCriteriaAggregate(
        rMaterializer, rRequest.maCriteriaRanges, rRequest.maCriteria, pTargetRange,
        rRequest.meKind, rRequest.meSearchType, rRequest.mbMatchWholeCell);
    if (!aResult)
        return RpnCoercionResult<api::CellValue>::failure(aResult.meError);
    return RpnCoercionResult<api::CellValue>::success(aResult.maValue);
}

// COUNTBLANK-style empty-cell count. Takes a materialized range with
// values already resolved; empty predicate matching is intentionally
// host-delegated so locale-specific empty detection stays with Calc until
// it can migrate alongside the range-iteration contract.
[[nodiscard]] inline RpnCoercionResult<double> countEmptyCells(
    const std::vector<api::CellValue>& rValues)
{
    double fCount = 0.0;
    for (const auto& rValue : rValues)
    {
        if (rValue.meKind == api::CellValueKind::Empty)
            fCount += 1.0;
        else if (rValue.meKind == api::CellValueKind::Text && rValue.maString.empty())
            fCount += 1.0;
    }
    return RpnCoercionResult<double>::success(fCount);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
