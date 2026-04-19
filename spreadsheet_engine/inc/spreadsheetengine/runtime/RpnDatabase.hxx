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
#include <variant>
#include <vector>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 3 substrate: engine-native database-function decision layer.
//
// This header defines the database query descriptor shape plus thin
// planners over it. The corresponding DB opcodes — DSUM / DCOUNT / DAVG
// / DGET / DMAX / DMIN / DPRODUCT / DSTDEV(P) / DVAR(P) — all share the
// same 3-argument contract:
//   (data_range_with_header, field_selector, criteria_range_with_header)
//
// Legacy Calc implements this via GetDBParams + DBIterator, which resolve
// the database range, find the field column, and iterate rows against
// the criteria query. The engine equivalent is a descriptor that the
// caller materializes (host-side) into CriteriaAggregateInput ranges and
// then dispatches through core::query::evaluateCriteriaAggregate.
//
// Per the RPN Evaluator Initiative policy, no Calc opcode currently
// routes through this layer. It exists to lock the shape for subsequent
// engine-first admissions.

namespace spreadsheetengine::core::rpn
{

enum class DatabaseAggregation : std::uint8_t
{
    Sum,
    Count,
    Count2,
    Average,
    Max,
    Min,
    Product,
    StandardDeviation,
    StandardDeviationPopulation,
    Variance,
    VariancePopulation,
    Get
};

// Describes a single database function call. The caller (host) has
// already resolved the data_range and criteria_range to ResolvedReference
// operands and extracted the field selector as either a column index or
// a column header name.
struct DatabaseQueryDescriptor
{
    // The full tabular data range including the header row.
    api::ResolvedReference maDataRange;
    // The full criteria range including its header row.
    api::ResolvedReference maCriteriaRange;
    // Field selector: exactly one of these is populated.
    std::optional<api::ColumnIndex> moFieldByIndex; // 1-based column index
    std::optional<api::String> moFieldByName;       // header name match
    // Whether the field argument was missing entirely (DCOUNT with no
    // field-name selector counts all matching rows regardless of field).
    bool mbFieldMissing = false;
    DatabaseAggregation meAggregation = DatabaseAggregation::Count;
};

// Resolve an RpnValue field-selector argument into the descriptor's
// moFieldByIndex / moFieldByName slots. References and matrices defer.
[[nodiscard]] inline RpnCoercionResult<std::monostate> applyFieldSelector(
    const RpnValue& rSelector, DatabaseQueryDescriptor& rDescriptor)
{
    using Result = RpnCoercionResult<std::monostate>;
    if (rSelector.meKind == RpnValueKind::Reference)
        return Result::deferred(RpnCoercionReadiness::NeedsReferenceResolution);
    if (rSelector.meKind == RpnValueKind::Matrix)
        return Result::deferred(RpnCoercionReadiness::NeedsMatrixMaterialization);

    if (rSelector.meKind == RpnValueKind::Empty)
    {
        rDescriptor.mbFieldMissing = true;
        return Result::success({});
    }
    if (rSelector.meKind == RpnValueKind::Number)
    {
        const double fValue = rSelector.maScalar.mfNumber;
        if (fValue < 1.0)
            return Result::failure(api::Error::IllegalArgument);
        rDescriptor.moFieldByIndex = static_cast<api::ColumnIndex>(fValue);
        return Result::success({});
    }
    if (rSelector.meKind == RpnValueKind::String)
    {
        rDescriptor.moFieldByName = rSelector.maScalar.maString;
        return Result::success({});
    }
    if (rSelector.meKind == RpnValueKind::Error)
        return Result::failure(rSelector.maScalar.meError);
    return Result::failure(api::Error::IllegalArgument);
}

// Map a DatabaseAggregation to the core::query aggregate kind used by
// evaluateCriteriaAggregate. Sum/Average/Max/Min/Count fall through
// directly; Count2 is Count (string counting handled by materializer);
// variance / stddev / product / get require variant-specific iteration
// and are deferred — the caller inspects mbRequiresVarianceAggregation
// to route those to a separate path.
struct AggregationBridge
{
    std::optional<core::query::CriteriaAggregateKind> moKind;
    bool mbRequiresVarianceAggregation = false;
    bool mbRequiresProductAggregation = false;
    bool mbRequiresGetAggregation = false;
    bool mbPopulation = false;
};

[[nodiscard]] inline AggregationBridge bridgeAggregation(DatabaseAggregation eDb)
{
    AggregationBridge aBridge;
    switch (eDb)
    {
        case DatabaseAggregation::Sum:
            aBridge.moKind = core::query::CriteriaAggregateKind::Sum;
            return aBridge;
        case DatabaseAggregation::Count:
        case DatabaseAggregation::Count2:
            aBridge.moKind = core::query::CriteriaAggregateKind::Count;
            return aBridge;
        case DatabaseAggregation::Average:
            aBridge.moKind = core::query::CriteriaAggregateKind::Average;
            return aBridge;
        case DatabaseAggregation::Max:
            aBridge.moKind = core::query::CriteriaAggregateKind::Max;
            return aBridge;
        case DatabaseAggregation::Min:
            aBridge.moKind = core::query::CriteriaAggregateKind::Min;
            return aBridge;
        case DatabaseAggregation::Product:
            aBridge.mbRequiresProductAggregation = true;
            return aBridge;
        case DatabaseAggregation::StandardDeviation:
            aBridge.mbRequiresVarianceAggregation = true;
            aBridge.mbPopulation = false;
            return aBridge;
        case DatabaseAggregation::StandardDeviationPopulation:
            aBridge.mbRequiresVarianceAggregation = true;
            aBridge.mbPopulation = true;
            return aBridge;
        case DatabaseAggregation::Variance:
            aBridge.mbRequiresVarianceAggregation = true;
            aBridge.mbPopulation = false;
            return aBridge;
        case DatabaseAggregation::VariancePopulation:
            aBridge.mbRequiresVarianceAggregation = true;
            aBridge.mbPopulation = true;
            return aBridge;
        case DatabaseAggregation::Get:
            aBridge.mbRequiresGetAggregation = true;
            return aBridge;
    }
    return aBridge;
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
