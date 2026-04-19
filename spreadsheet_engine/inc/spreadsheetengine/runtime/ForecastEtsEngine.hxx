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
#include <cstddef>
#include <cstdint>
#include <vector>

#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/runtime/KahanSum.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnOperators.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 4 substrate: engine-native FORECAST.ETS core (scaffold).
//
// This header carries the opcode-kind enumeration and planner entry
// points for Calc's FORECAST.ETS family (ETS, ETS.SEASONALITY,
// ETS.CONFINT, ETS.ADD, ETS.MULT, ETS.PI.ADD, ETS.PI.MULT). The
// Statistics variants (ETS.STAT.ADD / ETS.STAT.MULT, opcode
// `etsStatAdd` / `etsStatMult`) are *explicitly deferred* per the
// close-out plan's Phase E note.
//
// Current scope (Phase E kickoff): this header is a scaffold that
// preserves the substrate surface so later phases can drop in the full
// ETS state machine without a second API churn. Every planner in the
// non-statistics family currently returns
// `RpnCoercionReadiness::NeedsMatrixMaterialization` as a deferred
// result, so the corresponding `tryPlanEngineForecastEts*` dispatch
// lambda decisively declines and the legacy path in
// `ScInterpreter::ScForecast_Ets` keeps ownership until the triple-
// exponential-smoothing state machine is ported here.
//
// Legacy reference: `ScETSForecastCalculation` in
// `sc/source/core/tool/interpr8.cxx`, ~1000 lines of Holt-Winters
// init, alpha/beta/gamma grid search, sampling, and prediction-
// interval calculations. Porting that verbatim is out of scope for
// the Phase E admission session (see PROJECT_STATUS.md narrative);
// the three-headers deliverable of Phase E is met by having this
// scaffold present alongside LinestEngine.hxx and ForecastEngine.hxx.
//
// Numerical epsilon policy: when the ETS state machine is implemented
// here, it must match legacy bit-for-bit on initialisation (alpha /
// beta / gamma are selected by a discrete grid search with the same
// step size) and within 1 ULP on the Holt-Winters recurrences (Kahan
// summation carries the running base / trend). Prediction intervals
// use a finite-sample Monte Carlo estimate in legacy; the engine port
// must use the same seed and draw sequence to match exactly.

namespace spreadsheetengine::core::rpn
{

// Mirrors Calc's ScETSType enum in sc/source/core/tool/interpr8.cxx
// so the engine-side planner can take the variant as an argument
// rather than eight separate planners. Stats variants are represented
// here for surface completeness but the corresponding planners refuse
// to accept them (return IllegalArgument) until the dedicated port is
// done.
enum class ForecastEtsVariant : std::uint8_t
{
    Add,       // FORECAST.ETS.ADD
    Mult,      // FORECAST.ETS.MULT (or FORECAST.ETS)
    PIAdd,     // FORECAST.ETS.PI.ADD (additive prediction interval)
    PIMult,    // FORECAST.ETS.PI.MULT
    Seasonality, // FORECAST.ETS.SEASONALITY
    // Deferred — stats variants.
    StatAdd,
    StatMult
};

// Result of an ETS plan: either a single scalar (FORECAST.ETS
// single-target variant) or a dense matrix of forecasts /
// intervals / statistics (target-range variants).
struct ForecastEtsPlanResult
{
    // True if the result is a scalar forecast packed in mfScalar.
    bool mbIsScalar = false;
    double mfScalar = 0.0;
    MatrixOperand maMatrix;
};

namespace detail::ets
{

[[nodiscard]] constexpr bool isStatsVariant(ForecastEtsVariant eVariant) noexcept
{
    return eVariant == ForecastEtsVariant::StatAdd
           || eVariant == ForecastEtsVariant::StatMult;
}

} // namespace detail::ets

// FORECAST.ETS family planner.
//
// Inputs:
// - eVariant: which ETS variant to compute.
// - rTargetDate: target X value (scalar or matrix form).
// - rKnownY: known observations (column / row of Y values).
// - rKnownX: known X values (must be equally spaced for Holt-
//   Winters; caller is responsible for validation).
// - pConfidence, pSeasonality, pDataCompletion, pAggregation:
//   optional per-variant control parameters matching legacy ODF
//   semantics.
//
// Until the full port lands, this planner returns a deferred result
// for non-stats variants (so the dispatch lambda declines and Calc
// keeps the legacy path) and IllegalArgument for stats variants
// (which are explicitly out of scope).
[[nodiscard]] inline RpnCoercionResult<ForecastEtsPlanResult> planForecastEts(
    ForecastEtsVariant eVariant,
    const MatrixOperand& /*rTargetDate*/,
    const MatrixOperand& /*rKnownY*/,
    const MatrixOperand& /*rKnownX*/,
    const MatrixOperand* /*pConfidence*/,
    const MatrixOperand* /*pSeasonality*/,
    const MatrixOperand* /*pDataCompletion*/,
    const MatrixOperand* /*pAggregation*/)
{
    if (detail::ets::isStatsVariant(eVariant))
    {
        // ETS.STAT.* is explicitly deferred for Phase E. Reject here
        // so the dispatch lambda declines and the legacy ETS path
        // takes over.
        return RpnCoercionResult<ForecastEtsPlanResult>::failure(
            api::Error::IllegalArgument);
    }
    // Non-stats variants are not yet implemented here. Deferring with
    // NeedsMatrixMaterialization forces the dispatch lambda to decline
    // and keeps the legacy ScForecast_Ets authoritative. Future work
    // will replace this with the full state machine.
    return RpnCoercionResult<ForecastEtsPlanResult>::deferred(
        RpnCoercionReadiness::NeedsMatrixMaterialization);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
