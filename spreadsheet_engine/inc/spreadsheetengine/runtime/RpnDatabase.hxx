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
#include <spreadsheetengine/runtime/FloatingPoint.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/RpnVariance.hxx>
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
// This layer now backs the admitted database-family paths for DSUM /
// DCOUNT / DCOUNT2 / DAVERAGE / DGET / DMAX / DMIN / DPRODUCT /
// DSTDEV(P) / DVAR(P) when their arguments stay within the admitted
// single-sheet scalarized contract.

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
        if (fValue == 0.0)
        {
            rDescriptor.mbFieldMissing = true;
            return Result::success({});
        }
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

struct DatabaseCriteriaGroup
{
    std::vector<core::query::CriteriaAggregateInput> maCriteriaRanges;
    std::vector<core::query::CriteriaPredicate> maCriteria;
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
            aBridge.moKind = core::query::CriteriaAggregateKind::CountNumeric;
            return aBridge;
        case DatabaseAggregation::Count2:
            aBridge.moKind = core::query::CriteriaAggregateKind::Count2;
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

struct MaterializedDatabaseQuery
{
    core::query::CriteriaAggregateInput maBaseRange;
    std::vector<DatabaseCriteriaGroup> maCriteriaGroups;
    std::optional<core::query::CriteriaAggregateInput> moTargetRange;
    bool mbCountMatchesOnly = false;
};

[[nodiscard]] inline core::query::CriteriaAggregateInput makeCriteriaAggregateInput(
    const api::ResolvedReference& rReference)
{
    core::query::CriteriaAggregateInput aInput;
    aInput.mbScalar = false;
    aInput.maReference = rReference;
    aInput.mnColumns = static_cast<api::MatrixSize>(
        rReference.maRange.maEnd.mnColumn - rReference.maRange.maStart.mnColumn + 1);
    aInput.mnRows = static_cast<api::MatrixSize>(
        rReference.maRange.maEnd.mnRow - rReference.maRange.maStart.mnRow + 1);
    return aInput;
}

[[nodiscard]] inline bool isSingleSheetReference(const api::ResolvedReference& rReference)
{
    return rReference.maRange.maStart.mnSheet == rReference.maRange.maEnd.mnSheet;
}

[[nodiscard]] inline api::ValueResult<MaterializedDatabaseQuery> materializeDatabaseQuery(
    const core::query::CriteriaAggregateMaterializer& rMaterializer,
    const DatabaseQueryDescriptor& rDescriptor,
    core::query::CriteriaNumberTextParser pParseNumberText,
    core::query::CriteriaAsciiDoubleParser pParseAsciiDouble)
{
    using CriteriaAggregateInput = core::query::CriteriaAggregateInput;
    using MaterializedResult = api::ValueResult<MaterializedDatabaseQuery>;

    if (!isSingleSheetReference(rDescriptor.maDataRange)
        || !isSingleSheetReference(rDescriptor.maCriteriaRange))
    {
        return MaterializedResult::failure(api::Error::IllegalArgument);
    }

    const CriteriaAggregateInput aDatabaseInput
        = makeCriteriaAggregateInput(rDescriptor.maDataRange);
    const CriteriaAggregateInput aCriteriaGridInput
        = makeCriteriaAggregateInput(rDescriptor.maCriteriaRange);
    if (aDatabaseInput.mnRows < 2 || aCriteriaGridInput.mnRows < 2)
        return MaterializedResult::failure(api::Error::IllegalArgument);

    CriteriaAggregateInput aBaseInput;
    aBaseInput.mbScalar = false;
    aBaseInput.maReference.maRange.maStart = {
        rDescriptor.maDataRange.maRange.maStart.mnSheet,
        rDescriptor.maDataRange.maRange.maStart.mnColumn,
        static_cast<api::RowIndex>(rDescriptor.maDataRange.maRange.maStart.mnRow + 1)
    };
    aBaseInput.maReference.maRange.maEnd = {
        rDescriptor.maDataRange.maRange.maEnd.mnSheet,
        rDescriptor.maDataRange.maRange.maStart.mnColumn,
        rDescriptor.maDataRange.maRange.maEnd.mnRow
    };
    aBaseInput.mnColumns = 1;
    aBaseInput.mnRows = static_cast<api::MatrixSize>(aDatabaseInput.mnRows - 1);

    MaterializedDatabaseQuery aMaterialized;
    aMaterialized.maBaseRange = aBaseInput;
    std::optional<api::ColumnIndex> oFieldColumn;

    if (rDescriptor.mbFieldMissing)
    {
        if (rDescriptor.meAggregation == DatabaseAggregation::Count
            || rDescriptor.meAggregation == DatabaseAggregation::Count2)
        {
            aMaterialized.mbCountMatchesOnly = true;
        }
        else
        {
            return MaterializedResult::failure(api::Error::IllegalArgument);
        }
    }
    else if (rDescriptor.moFieldByIndex)
    {
        if (*rDescriptor.moFieldByIndex < 1
            || *rDescriptor.moFieldByIndex > aDatabaseInput.mnColumns)
        {
            return MaterializedResult::failure(api::Error::IllegalArgument);
        }
        oFieldColumn = static_cast<api::ColumnIndex>(
            rDescriptor.maDataRange.maRange.maStart.mnColumn + *rDescriptor.moFieldByIndex - 1);
    }
    else if (rDescriptor.moFieldByName)
    {
        for (api::MatrixSize nCol = 0; nCol < aDatabaseInput.mnColumns; ++nCol)
        {
            const auto aHeader = rMaterializer.materialize(aDatabaseInput, { nCol, 0 });
            if (!aHeader)
                return MaterializedResult::failure(aHeader.meError);
            if (aHeader.maValue.meKind == api::CellValueKind::Text
                && core::query::compareFoldedText(
                       aHeader.maValue.maString, *rDescriptor.moFieldByName)
                       == 0)
            {
                oFieldColumn = static_cast<api::ColumnIndex>(
                    rDescriptor.maDataRange.maRange.maStart.mnColumn + nCol);
                break;
            }
        }
        if (!oFieldColumn)
            return MaterializedResult::failure(api::Error::IllegalArgument);
    }
    else
    {
        return MaterializedResult::failure(api::Error::IllegalArgument);
    }

    aMaterialized.maCriteriaGroups.reserve(aCriteriaGridInput.mnRows - 1);
    for (api::MatrixSize nCriteriaRow = 1; nCriteriaRow < aCriteriaGridInput.mnRows;
         ++nCriteriaRow)
    {
        DatabaseCriteriaGroup aGroup;
        aGroup.maCriteriaRanges.reserve(aCriteriaGridInput.mnColumns);
        aGroup.maCriteria.reserve(aCriteriaGridInput.mnColumns);

        for (api::MatrixSize nCriteriaColumn = 0; nCriteriaColumn < aCriteriaGridInput.mnColumns;
             ++nCriteriaColumn)
        {
            const auto aCriterionValue
                = rMaterializer.materialize(aCriteriaGridInput, { nCriteriaColumn, nCriteriaRow });
            if (!aCriterionValue)
                return MaterializedResult::failure(aCriterionValue.meError);
            if (aCriterionValue.maValue.meKind == api::CellValueKind::Empty)
                continue;

            const auto aCriterionHeader
                = rMaterializer.materialize(aCriteriaGridInput, { nCriteriaColumn, 0 });
            if (!aCriterionHeader)
                return MaterializedResult::failure(aCriterionHeader.meError);
            if (aCriterionHeader.maValue.meKind != api::CellValueKind::Text)
                return MaterializedResult::failure(api::Error::IllegalArgument);

            std::optional<api::ColumnIndex> oMatchedColumn;
            for (api::MatrixSize nDatabaseColumn = 0; nDatabaseColumn < aDatabaseInput.mnColumns;
                 ++nDatabaseColumn)
            {
                const auto aDatabaseHeader
                    = rMaterializer.materialize(aDatabaseInput, { nDatabaseColumn, 0 });
                if (!aDatabaseHeader)
                    return MaterializedResult::failure(aDatabaseHeader.meError);
                if (aDatabaseHeader.maValue.meKind == api::CellValueKind::Text
                    && core::query::compareFoldedText(
                           aDatabaseHeader.maValue.maString, aCriterionHeader.maValue.maString)
                           == 0)
                {
                    oMatchedColumn = static_cast<api::ColumnIndex>(
                        rDescriptor.maDataRange.maRange.maStart.mnColumn + nDatabaseColumn);
                    break;
                }
            }
            if (!oMatchedColumn)
                return MaterializedResult::failure(api::Error::IllegalArgument);

            const auto oPredicate = core::query::makeCriteriaPredicate(
                aCriterionValue.maValue, pParseNumberText, pParseAsciiDouble);
            if (!oPredicate)
                return MaterializedResult::failure(api::Error::IllegalArgument);

            CriteriaAggregateInput aRangeInput;
            aRangeInput.mbScalar = false;
            aRangeInput.maReference.maRange.maStart = {
                rDescriptor.maDataRange.maRange.maStart.mnSheet,
                *oMatchedColumn,
                static_cast<api::RowIndex>(rDescriptor.maDataRange.maRange.maStart.mnRow + 1)
            };
            aRangeInput.maReference.maRange.maEnd = {
                rDescriptor.maDataRange.maRange.maEnd.mnSheet,
                *oMatchedColumn,
                rDescriptor.maDataRange.maRange.maEnd.mnRow
            };
            aRangeInput.mnColumns = 1;
            aRangeInput.mnRows = static_cast<api::MatrixSize>(aDatabaseInput.mnRows - 1);
            aGroup.maCriteriaRanges.push_back(aRangeInput);
            aGroup.maCriteria.push_back(*oPredicate);
        }

        aMaterialized.maCriteriaGroups.push_back(std::move(aGroup));
    }

    if (!aMaterialized.mbCountMatchesOnly)
    {
        if (!oFieldColumn)
            return MaterializedResult::failure(api::Error::IllegalArgument);

        CriteriaAggregateInput aTargetInput;
        aTargetInput.mbScalar = false;
        aTargetInput.maReference.maRange.maStart = {
            rDescriptor.maDataRange.maRange.maStart.mnSheet,
            *oFieldColumn,
            static_cast<api::RowIndex>(rDescriptor.maDataRange.maRange.maStart.mnRow + 1)
        };
        aTargetInput.maReference.maRange.maEnd = {
            rDescriptor.maDataRange.maRange.maEnd.mnSheet,
            *oFieldColumn,
            rDescriptor.maDataRange.maRange.maEnd.mnRow
        };
        aTargetInput.mnColumns = 1;
        aTargetInput.mnRows = static_cast<api::MatrixSize>(aDatabaseInput.mnRows - 1);
        aMaterialized.moTargetRange = aTargetInput;
    }

    return MaterializedResult::success(std::move(aMaterialized));
}

namespace detail::database
{

[[nodiscard]] inline std::optional<double> coerceAggregateNumber(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return rValue.mfNumber;
        case api::CellValueKind::Text:
        case api::CellValueKind::Empty:
        case api::CellValueKind::Error:
            return std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] inline bool isNonEmptyCount2Value(const api::CellValue& rValue)
{
    return rValue.meKind != api::CellValueKind::Empty
           && (rValue.meKind != api::CellValueKind::Text || !rValue.maString.empty());
}

[[nodiscard]] inline api::ValueResult<bool> matchesCriteriaGroups(
    const core::query::CriteriaAggregateMaterializer& rMaterializer,
    const MaterializedDatabaseQuery& rMaterialized, api::query::SearchType eSearchType,
    bool bMatchWholeCell, api::MatrixCoordinate aCoordinate)
{
    if (rMaterialized.maCriteriaGroups.empty())
        return api::ValueResult<bool>::success(true);

    for (const auto& rGroup : rMaterialized.maCriteriaGroups)
    {
        bool bMatch = true;
        for (std::size_t nIndex = 0; nIndex < rGroup.maCriteriaRanges.size(); ++nIndex)
        {
            const auto aCandidate
                = rMaterializer.materialize(rGroup.maCriteriaRanges[nIndex], aCoordinate);
            if (!aCandidate
                || !core::query::matchesCriteriaPredicate(
                    rGroup.maCriteria[nIndex], aCandidate.maValue, eSearchType,
                    bMatchWholeCell))
            {
                bMatch = false;
                break;
            }
        }

        if (bMatch)
            return api::ValueResult<bool>::success(true);
    }

    return api::ValueResult<bool>::success(false);
}

} // namespace detail::database

[[nodiscard]] inline api::ValueResult<api::CellValue> evaluateDatabaseQuery(
    const core::query::CriteriaAggregateMaterializer& rMaterializer,
    const DatabaseQueryDescriptor& rDescriptor, api::query::SearchType eSearchType,
    bool bMatchWholeCell, core::query::CriteriaNumberTextParser pParseNumberText,
    core::query::CriteriaAsciiDoubleParser pParseAsciiDouble)
{
    const auto aMaterialized = materializeDatabaseQuery(
        rMaterializer, rDescriptor, pParseNumberText, pParseAsciiDouble);
    if (!aMaterialized)
        return api::ValueResult<api::CellValue>::failure(aMaterialized.meError);

    const AggregationBridge aBridge = bridgeAggregation(rDescriptor.meAggregation);
    std::size_t nCount = 0;
    double fSum = 0.0;
    double fProduct = 1.0;
    double fBest = 0.0;
    bool bHasBest = false;
    VarianceAccumulator aVariance;
    std::optional<api::CellValue> oUniqueMatch;

    for (api::MatrixSize nRow = 0; nRow < aMaterialized.maValue.maBaseRange.mnRows; ++nRow)
    {
        const api::MatrixCoordinate aCoordinate { 0, nRow };
        const auto aMatches = detail::database::matchesCriteriaGroups(
            rMaterializer, aMaterialized.maValue, eSearchType, bMatchWholeCell, aCoordinate);
        if (!aMatches)
            return api::ValueResult<api::CellValue>::failure(aMatches.meError);
        if (!aMatches.maValue)
            continue;

        if (aMaterialized.maValue.mbCountMatchesOnly)
        {
            ++nCount;
            continue;
        }

        const auto aTarget
            = rMaterializer.materialize(*aMaterialized.maValue.moTargetRange, aCoordinate);
        if (!aTarget)
            return api::ValueResult<api::CellValue>::failure(aTarget.meError);

        if (aBridge.mbRequiresGetAggregation)
        {
            if (oUniqueMatch.has_value())
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            oUniqueMatch = aTarget.maValue;
            continue;
        }

        if (aBridge.mbRequiresVarianceAggregation)
        {
            const auto oNumber = detail::database::coerceAggregateNumber(aTarget.maValue);
            if (oNumber)
                aVariance.add(*oNumber);
            continue;
        }

        if (aBridge.mbRequiresProductAggregation)
        {
            const auto oNumber = detail::database::coerceAggregateNumber(aTarget.maValue);
            if (!oNumber)
                continue;
            fProduct = fProduct * *oNumber;
            ++nCount;
            continue;
        }

        if (!aBridge.moKind)
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);

        switch (*aBridge.moKind)
        {
            case core::query::CriteriaAggregateKind::Count:
                ++nCount;
                break;
            case core::query::CriteriaAggregateKind::Count2:
                if (detail::database::isNonEmptyCount2Value(aTarget.maValue))
                    ++nCount;
                break;
            case core::query::CriteriaAggregateKind::CountNumeric:
                if (detail::database::coerceAggregateNumber(aTarget.maValue))
                    ++nCount;
                break;
            case core::query::CriteriaAggregateKind::Sum:
            case core::query::CriteriaAggregateKind::Average:
            {
                const auto oNumber = detail::database::coerceAggregateNumber(aTarget.maValue);
                if (!oNumber)
                    break;
                fSum = fp::approxAdd(fSum, *oNumber);
                ++nCount;
                break;
            }
            case core::query::CriteriaAggregateKind::Max:
            case core::query::CriteriaAggregateKind::Min:
            {
                const auto oNumber = detail::database::coerceAggregateNumber(aTarget.maValue);
                if (!oNumber)
                    break;
                if (!bHasBest
                    || (*aBridge.moKind == core::query::CriteriaAggregateKind::Max
                            ? *oNumber > fBest
                            : *oNumber < fBest))
                {
                    fBest = *oNumber;
                    bHasBest = true;
                }
                break;
            }
            case core::query::CriteriaAggregateKind::Product:
                break;
        }
    }

    if (aMaterialized.maValue.mbCountMatchesOnly)
    {
        return api::ValueResult<api::CellValue>::success(
            api::CellValue::number(static_cast<double>(nCount)));
    }

    if (aBridge.mbRequiresGetAggregation)
    {
        if (!oUniqueMatch)
            return api::ValueResult<api::CellValue>::failure(api::Error::NoValue);
        return api::ValueResult<api::CellValue>::success(*oUniqueMatch);
    }

    if (aBridge.mbRequiresVarianceAggregation)
    {
        const auto aResult = finalizeVariance(
            aVariance, aBridge.mbPopulation
                           ? (rDescriptor.meAggregation
                                      == DatabaseAggregation::StandardDeviationPopulation
                                  ? VarianceKind::PopulationStandardDeviation
                                  : VarianceKind::PopulationVariance)
                           : (rDescriptor.meAggregation == DatabaseAggregation::StandardDeviation
                                  ? VarianceKind::SampleStandardDeviation
                                  : VarianceKind::SampleVariance));
        return aResult ? api::ValueResult<api::CellValue>::success(
                             api::CellValue::number(aResult.maValue))
                       : api::ValueResult<api::CellValue>::failure(aResult.meError);
    }

    if (aBridge.mbRequiresProductAggregation)
    {
        return api::ValueResult<api::CellValue>::success(
            api::CellValue::number(nCount == 0 ? 0.0 : fProduct));
    }

    if (!aBridge.moKind)
        return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);

    switch (*aBridge.moKind)
    {
        case core::query::CriteriaAggregateKind::Count:
        case core::query::CriteriaAggregateKind::Count2:
        case core::query::CriteriaAggregateKind::CountNumeric:
            return api::ValueResult<api::CellValue>::success(
                api::CellValue::number(static_cast<double>(nCount)));
        case core::query::CriteriaAggregateKind::Sum:
            return api::ValueResult<api::CellValue>::success(api::CellValue::number(fSum));
        case core::query::CriteriaAggregateKind::Average:
            if (nCount == 0)
                return api::ValueResult<api::CellValue>::failure(api::Error::DivisionByZero);
            return api::ValueResult<api::CellValue>::success(
                api::CellValue::number(fSum / static_cast<double>(nCount)));
        case core::query::CriteriaAggregateKind::Max:
        case core::query::CriteriaAggregateKind::Min:
            return api::ValueResult<api::CellValue>::success(
                api::CellValue::number(bHasBest ? fBest : 0.0));
        case core::query::CriteriaAggregateKind::Product:
            break;
    }

    return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
