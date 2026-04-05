/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <document.hxx>

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/MutationEntry.hxx>

namespace spreadsheetengine::compat::libreoffice::substraterawmutation
{

enum class RawMutationObservationKind : sal_uInt8
{
    Exact,
    OrderingOnly,
    HiddenHostMutationReconstruction,
    MissingRealizedOrRolledBackObjects,
    QueueOrStateMismatch,
    OutOfContract
};

struct RawMutationObservation
{
    RawMutationObservationKind meKind = RawMutationObservationKind::OutOfContract;
    api::String maReason;
    bool mbMutationApplied = false;
    bool mbRolledBack = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;
    bool mbObjectRealizationExact = false;
    bool mbRollbackExact = false;

    [[nodiscard]] constexpr bool operator==(const RawMutationObservation& rOther) const = default;
};

enum class RawMutationRecordKind : sal_uInt8
{
    SetScalarValue,
    SetFormula,
    ClearCell,
    InsertRows,
    DeleteRows,
    InsertColumns,
    DeleteColumns
};

struct AdmittedRawMutationRecord
{
    RawMutationRecordKind meKind = RawMutationRecordKind::ClearCell;
    api::CellAddress maAddress;
    sal_Int32 mnCount = 0;
    std::optional<api::CellValue> moScalarValue;
    api::String maFormulaSource;
    std::optional<api::CellValue> moFormulaCachedValueAfter;

    [[nodiscard]] constexpr bool operator==(const AdmittedRawMutationRecord& rOther) const = default;
};

enum class RawMutationRecordResultKind : sal_uInt8
{
    Built,
    RejectedOutOfContract
};

struct RawMutationRecordResult
{
    RawMutationRecordResultKind meKind = RawMutationRecordResultKind::RejectedOutOfContract;
    api::String maReason;
    AdmittedRawMutationRecord maRecord;
};

enum class RawMutationApplyResultKind : sal_uInt8
{
    Applied,
    RejectedOutOfContract
};

struct RawMutationApplyResult
{
    RawMutationApplyResultKind meKind = RawMutationApplyResultKind::RejectedOutOfContract;
    api::String maReason;
};

[[nodiscard]] inline RawMutationObservation classifyRawMutationObservation(
    bool bMutationApplied, bool bRolledBack, const recalcshadow::ShadowComparison& rQueue,
    const spreadsheetengine::detail::substrate::ComputationalShadowComparison& rComputational,
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rGraph,
    const std::optional<substrateobjectrealization::ObjectRealizationObservation>& oObjectRealization,
    const std::optional<substraterollback::RollbackObservation>& oRollback,
    api::StringView rReasonIfOutOfContract = {})
{
    RawMutationObservation aObservation;
    aObservation.mbMutationApplied = bMutationApplied;
    aObservation.mbRolledBack = bRolledBack;
    aObservation.mbQueueExact = rQueue.meKind == recalcshadow::ShadowComparisonKind::Exact;
    aObservation.mbComputationalFullMatch = rComputational.mbFullMatch;
    aObservation.mbGraphFullMatch = rGraph.mbFullMatch;
    aObservation.mbObjectRealizationExact = oObjectRealization
                                            && oObjectRealization->meKind
                                                   == substrateobjectrealization::ObjectRealizationObservationKind::Exact;
    aObservation.mbRollbackExact = oRollback
                                   && oRollback->meKind
                                          == substraterollback::RollbackObservationKind::Exact;

    if (!bMutationApplied)
    {
        aObservation.meKind = RawMutationObservationKind::OutOfContract;
        aObservation.maReason = rReasonIfOutOfContract;
        return aObservation;
    }

    if (oRollback)
    {
        switch (oRollback->meKind)
        {
            case substraterollback::RollbackObservationKind::Exact:
                aObservation.meKind = RawMutationObservationKind::Exact;
                aObservation.maReason = u"exact_rollback";
                return aObservation;
            case substraterollback::RollbackObservationKind::OrderingOnly:
                aObservation.meKind = RawMutationObservationKind::OrderingOnly;
                aObservation.maReason = u"rollback_ordering_only";
                return aObservation;
            case substraterollback::RollbackObservationKind::MissingRestoredObjects:
                aObservation.meKind = RawMutationObservationKind::MissingRealizedOrRolledBackObjects;
                aObservation.maReason = u"missing_rolled_back_objects";
                return aObservation;
            case substraterollback::RollbackObservationKind::HostOnlyRollbackReconstruction:
                aObservation.meKind = RawMutationObservationKind::HiddenHostMutationReconstruction;
                aObservation.maReason = u"host_only_rollback_reconstruction";
                return aObservation;
            case substraterollback::RollbackObservationKind::QueueOrStateMismatch:
                aObservation.meKind = RawMutationObservationKind::QueueOrStateMismatch;
                aObservation.maReason = u"rollback_queue_or_state_mismatch";
                return aObservation;
            case substraterollback::RollbackObservationKind::OutOfContract:
                aObservation.meKind = RawMutationObservationKind::OutOfContract;
                aObservation.maReason = oRollback->maReason;
                return aObservation;
        }
    }

    if (!oObjectRealization)
    {
        aObservation.meKind = RawMutationObservationKind::OutOfContract;
        aObservation.maReason = u"missing_object_realization_observation";
        return aObservation;
    }

    switch (oObjectRealization->meKind)
    {
        case substrateobjectrealization::ObjectRealizationObservationKind::Exact:
            aObservation.meKind = RawMutationObservationKind::Exact;
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::OrderingOnly:
            aObservation.meKind = RawMutationObservationKind::OrderingOnly;
            aObservation.maReason = u"object_realization_ordering_only";
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::MissingRealizedObjects:
            aObservation.meKind = RawMutationObservationKind::MissingRealizedOrRolledBackObjects;
            aObservation.maReason = u"missing_realized_objects";
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction:
            aObservation.meKind = RawMutationObservationKind::HiddenHostMutationReconstruction;
            aObservation.maReason = u"host_only_object_reconstruction";
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::QueueOrStateMismatch:
            aObservation.meKind = RawMutationObservationKind::QueueOrStateMismatch;
            aObservation.maReason = u"object_realization_queue_or_state_mismatch";
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::OutOfContract:
            aObservation.meKind = RawMutationObservationKind::OutOfContract;
            aObservation.maReason = oObjectRealization->maReason;
            return aObservation;
    }

    aObservation.meKind = RawMutationObservationKind::OutOfContract;
    aObservation.maReason = u"raw_mutation_observation_unknown";
    return aObservation;
}

[[nodiscard]] inline RawMutationObservation classifyRawMutationObservation(
    bool bMutationApplied, bool bRolledBack,
    const std::optional<substrateobjectrealization::ObjectRealizationObservation>& oObjectRealization,
    const std::optional<substraterollback::RollbackObservation>& oRollback,
    api::StringView rReasonIfOutOfContract = {})
{
    recalcshadow::ShadowComparison aQueue;
    aQueue.meKind = recalcshadow::ShadowComparisonKind::Exact;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputational;
    aComputational.mbCellPopulationMatch = true;
    aComputational.mbFormulaTreeMatch = true;
    aComputational.mbFormulaTrackMatch = true;
    aComputational.mbBroadcasterMatch = true;
    aComputational.mbGroupMatch = true;
    aComputational.mbNamedRangeMatch = true;
    aComputational.mbFullMatch = true;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraph;
    aGraph.meKind = spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::Exact;
    aGraph.mbFormulaNodeMatch = true;
    aGraph.mbFormulaGroupNodeMatch = true;
    aGraph.mbListenerAnchorMatch = true;
    aGraph.mbBroadcasterNodeMatch = true;
    aGraph.mbEdgeMatch = true;
    aGraph.mbFormulaTreeExactMatch = true;
    aGraph.mbFormulaTrackExactMatch = true;
    aGraph.mbFormulaTreeNormalizedMatch = true;
    aGraph.mbFormulaTrackNormalizedMatch = true;
    aGraph.mbFullMatch = true;

    return classifyRawMutationObservation(
        bMutationApplied, bRolledBack, aQueue, aComputational, aGraph,
        oObjectRealization, oRollback, rReasonIfOutOfContract);
}

[[nodiscard]] inline RawMutationRecordResult buildAdmittedRawMutationRecord(
    const spreadsheetengine::detail::substrate::MutationEntryRequest& rRequest)
{
    using spreadsheetengine::detail::facade::MutationKind;

    RawMutationRecordResult aResult;
    aResult.meKind = RawMutationRecordResultKind::Built;
    aResult.maRecord.maAddress = rRequest.maMutation.maAddress;
    aResult.maRecord.moFormulaCachedValueAfter = rRequest.moFormulaCachedValueAfter;

    switch (rRequest.maMutation.meKind)
    {
        case MutationKind::SetScalarValue:
            if (!rRequest.moScalarValueAfter)
            {
                aResult.meKind = RawMutationRecordResultKind::RejectedOutOfContract;
                aResult.maReason = u"missing_scalar_value_after";
                return aResult;
            }
            if (!rRequest.moScalarValueAfter->isNumber() && !rRequest.moScalarValueAfter->isBoolean()
                && !rRequest.moScalarValueAfter->isText())
            {
                aResult.meKind = RawMutationRecordResultKind::RejectedOutOfContract;
                aResult.maReason = u"scalar_value_out_of_contract";
                return aResult;
            }
            aResult.maRecord.meKind = RawMutationRecordKind::SetScalarValue;
            aResult.maRecord.moScalarValue = rRequest.moScalarValueAfter;
            return aResult;
        case MutationKind::SetFormula:
            if (rRequest.maMutation.maText.empty())
            {
                aResult.meKind = RawMutationRecordResultKind::RejectedOutOfContract;
                aResult.maReason = u"formula_source_out_of_contract";
                return aResult;
            }
            aResult.maRecord.meKind = RawMutationRecordKind::SetFormula;
            aResult.maRecord.maFormulaSource = rRequest.maMutation.maText;
            return aResult;
        case MutationKind::ClearCell:
            aResult.maRecord.meKind = RawMutationRecordKind::ClearCell;
            return aResult;
        case MutationKind::InsertRows:
            if (rRequest.maMutation.mnCount <= 0)
            {
                aResult.meKind = RawMutationRecordResultKind::RejectedOutOfContract;
                aResult.maReason = u"structural_count_out_of_contract";
                return aResult;
            }
            aResult.maRecord.meKind = RawMutationRecordKind::InsertRows;
            aResult.maRecord.mnCount = rRequest.maMutation.mnCount;
            return aResult;
        case MutationKind::DeleteRows:
            if (rRequest.maMutation.mnCount <= 0)
            {
                aResult.meKind = RawMutationRecordResultKind::RejectedOutOfContract;
                aResult.maReason = u"structural_count_out_of_contract";
                return aResult;
            }
            aResult.maRecord.meKind = RawMutationRecordKind::DeleteRows;
            aResult.maRecord.mnCount = rRequest.maMutation.mnCount;
            return aResult;
        case MutationKind::InsertColumns:
            if (rRequest.maMutation.mnCount <= 0)
            {
                aResult.meKind = RawMutationRecordResultKind::RejectedOutOfContract;
                aResult.maReason = u"structural_count_out_of_contract";
                return aResult;
            }
            aResult.maRecord.meKind = RawMutationRecordKind::InsertColumns;
            aResult.maRecord.mnCount = rRequest.maMutation.mnCount;
            return aResult;
        case MutationKind::DeleteColumns:
            if (rRequest.maMutation.mnCount <= 0)
            {
                aResult.meKind = RawMutationRecordResultKind::RejectedOutOfContract;
                aResult.maReason = u"structural_count_out_of_contract";
                return aResult;
            }
            aResult.maRecord.meKind = RawMutationRecordKind::DeleteColumns;
            aResult.maRecord.mnCount = rRequest.maMutation.mnCount;
            return aResult;
        default:
            aResult.meKind = RawMutationRecordResultKind::RejectedOutOfContract;
            aResult.maReason = u"mutation_out_of_contract";
            return aResult;
    }
}

namespace detail
{

[[nodiscard]] inline bool applyScalarCellValue(
    ScDocument& rDoc, const ScAddress& rAddress, const api::CellValue& rValue)
{
    if (rValue.isNumber() || rValue.isBoolean())
    {
        rDoc.SetValue(rAddress, rValue.mfNumber);
        return true;
    }
    if (rValue.isText())
    {
        rDoc.SetString(rAddress, toLibreOfficeString(rValue.maString));
        return true;
    }

    return false;
}

} // namespace detail

[[nodiscard]] inline RawMutationApplyResult applyAdmittedRawMutationRecord(
    ScDocument& rDoc, const AdmittedRawMutationRecord& rRecord)
{
    RawMutationApplyResult aResult;
    const ScAddress aAddress = toLibreOfficeAddress(rRecord.maAddress);

    switch (rRecord.meKind)
    {
        case RawMutationRecordKind::SetScalarValue:
            if (!rRecord.moScalarValue
                || !detail::applyScalarCellValue(rDoc, aAddress, *rRecord.moScalarValue))
            {
                aResult.maReason = u"scalar_value_out_of_contract";
                return aResult;
            }
            aResult.meKind = RawMutationApplyResultKind::Applied;
            return aResult;
        case RawMutationRecordKind::SetFormula:
            if (rRecord.maFormulaSource.empty())
            {
                aResult.maReason = u"formula_source_out_of_contract";
                return aResult;
            }
            rDoc.SetString(aAddress, toLibreOfficeString(rRecord.maFormulaSource));
            aResult.meKind = RawMutationApplyResultKind::Applied;
            return aResult;
        case RawMutationRecordKind::ClearCell:
            rDoc.SetEmptyCell(aAddress);
            aResult.meKind = RawMutationApplyResultKind::Applied;
            return aResult;
        case RawMutationRecordKind::InsertRows:
            if (rRecord.mnCount <= 0)
            {
                aResult.maReason = u"structural_count_out_of_contract";
                return aResult;
            }
            rDoc.InsertRow(ScRange(0, rRecord.maAddress.mnRow, rRecord.maAddress.mnSheet, rDoc.MaxCol(),
                rRecord.maAddress.mnRow + rRecord.mnCount - 1, rRecord.maAddress.mnSheet));
            aResult.meKind = RawMutationApplyResultKind::Applied;
            return aResult;
        case RawMutationRecordKind::DeleteRows:
            if (rRecord.mnCount <= 0)
            {
                aResult.maReason = u"structural_count_out_of_contract";
                return aResult;
            }
            rDoc.DeleteRow(ScRange(0, rRecord.maAddress.mnRow, rRecord.maAddress.mnSheet, rDoc.MaxCol(),
                rRecord.maAddress.mnRow + rRecord.mnCount - 1, rRecord.maAddress.mnSheet));
            aResult.meKind = RawMutationApplyResultKind::Applied;
            return aResult;
        case RawMutationRecordKind::InsertColumns:
            if (rRecord.mnCount <= 0)
            {
                aResult.maReason = u"structural_count_out_of_contract";
                return aResult;
            }
            rDoc.InsertCol(ScRange(rRecord.maAddress.mnColumn, 0, rRecord.maAddress.mnSheet,
                rRecord.maAddress.mnColumn + rRecord.mnCount - 1, rDoc.MaxRow(), rRecord.maAddress.mnSheet));
            aResult.meKind = RawMutationApplyResultKind::Applied;
            return aResult;
        case RawMutationRecordKind::DeleteColumns:
            if (rRecord.mnCount <= 0)
            {
                aResult.maReason = u"structural_count_out_of_contract";
                return aResult;
            }
            rDoc.DeleteCol(ScRange(rRecord.maAddress.mnColumn, 0, rRecord.maAddress.mnSheet,
                rRecord.maAddress.mnColumn + rRecord.mnCount - 1, rDoc.MaxRow(), rRecord.maAddress.mnSheet));
            aResult.meKind = RawMutationApplyResultKind::Applied;
            return aResult;
    }

    aResult.maReason = u"mutation_out_of_contract";
    return aResult;
}

[[nodiscard]] inline const char* toString(RawMutationObservationKind eKind)
{
    switch (eKind)
    {
        case RawMutationObservationKind::Exact:
            return "exact";
        case RawMutationObservationKind::OrderingOnly:
            return "ordering_only";
        case RawMutationObservationKind::HiddenHostMutationReconstruction:
            return "hidden_host_mutation_reconstruction";
        case RawMutationObservationKind::MissingRealizedOrRolledBackObjects:
            return "missing_realized_or_rolled_back_objects";
        case RawMutationObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case RawMutationObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "unknown";
}

} // namespace spreadsheetengine::compat::libreoffice::substraterawmutation

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
