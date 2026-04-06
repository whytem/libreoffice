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
#include <iterator>
#include <optional>
#include <set>
#include <vector>

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>

namespace spreadsheetengine::compat::libreoffice::substraterollback
{

enum class RollbackObservationKind : sal_uInt8
{
    Exact,
    OrderingOnly,
    MissingRestoredObjects,
    HostOnlyRollbackReconstruction,
    QueueOrStateMismatch,
    OutOfContract
};

struct RollbackObservation
{
    RollbackObservationKind meKind = RollbackObservationKind::OutOfContract;
    api::String maReason;
    sal_Int32 mnExpectedBroadcasters = 0;
    sal_Int32 mnLiveBroadcasters = 0;
    bool mbObjectRealizationApplied = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;
    bool mbBroadcasterExact = false;

    [[nodiscard]] constexpr bool operator==(const RollbackObservation& rOther) const = default;
};

enum class PrimitiveRollbackObservationKind : sal_uInt8
{
    Exact,
    OrderingOnly,
    HiddenHostRollbackOrchestration,
    MissingRestoredObjects,
    QueueOrStateMismatch,
    OutOfContract
};

struct PrimitiveRollbackObservation
{
    PrimitiveRollbackObservationKind meKind
        = PrimitiveRollbackObservationKind::OutOfContract;
    api::String maReason;
    bool mbPrimitiveRollbackApplied = false;
    bool mbRollbackExact = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;
    bool mbBroadcasterExact = false;

    [[nodiscard]] constexpr bool operator==(const PrimitiveRollbackObservation& rOther) const
        = default;
};

enum class RollbackResultKind : sal_uInt8
{
    Applied,
    RejectedOutOfContract
};

enum class PrimitiveRollbackRecordResultKind : sal_uInt8
{
    Built,
    RejectedOutOfContract
};

struct AdmittedRollbackRecord
{
    std::int64_t mnGeneration = 0;
    substrateobjectrealization::AdmittedObjectRealization maObjectRealization;
    recalcqueue::FormulaStateSnapshot maFormulaState;

    [[nodiscard]] constexpr bool operator==(const AdmittedRollbackRecord& rOther) const = default;

    [[nodiscard]] sal_Int32 getFormulaCellCount() const
    {
        return maObjectRealization.getFormulaCellCount();
    }

    [[nodiscard]] sal_Int32 getCellCount() const
    {
        return maObjectRealization.getCellCount();
    }

    [[nodiscard]] sal_Int32 getBroadcasterNodeCount() const
    {
        return maObjectRealization.getBroadcasterNodeCount();
    }
};

struct AdmittedPrimitiveRollbackRecord
{
    AdmittedRollbackRecord maRollback;
    bool mbUsesPrimitiveRollback = true;

    [[nodiscard]] constexpr bool operator==(const AdmittedPrimitiveRollbackRecord& rOther) const
        = default;
};

struct PrimitiveRollbackRecordResult
{
    PrimitiveRollbackRecordResultKind meKind
        = PrimitiveRollbackRecordResultKind::RejectedOutOfContract;
    api::String maReason;
    AdmittedPrimitiveRollbackRecord maRecord;
};

struct RollbackResult
{
    RollbackResultKind meKind = RollbackResultKind::RejectedOutOfContract;
    api::String maReason;
    substrateobjectrealization::ObjectRealizationResult maObjectRealization;
};

enum class PrimitiveRollbackApplyResultKind : sal_uInt8
{
    Applied,
    RejectedOutOfContract
};

struct PrimitiveRollbackApplyResult
{
    PrimitiveRollbackApplyResultKind meKind
        = PrimitiveRollbackApplyResultKind::RejectedOutOfContract;
    api::String maReason;
    RollbackResult maRollback;
};

namespace detail
{

[[nodiscard]] inline bool isHostOnlyRollbackKind(
    spreadsheetengine::detail::substrate::BroadcasterCanonicalizationKind eKind)
{
    using spreadsheetengine::detail::substrate::BroadcasterCanonicalizationKind;

    switch (eKind)
    {
        case BroadcasterCanonicalizationKind::DuplicateMaterializationOnly:
        case BroadcasterCanonicalizationKind::EmptyBroadcastersOnly:
        case BroadcasterCanonicalizationKind::DuplicateAndEmptyBroadcasters:
        case BroadcasterCanonicalizationKind::ListenerAnchorCanonicalizationOnly:
        case BroadcasterCanonicalizationKind::UnexpectedHostListeners:
        case BroadcasterCanonicalizationKind::Mixed:
            return true;
        case BroadcasterCanonicalizationKind::Exact:
        case BroadcasterCanonicalizationKind::OrderingOnly:
        case BroadcasterCanonicalizationKind::MissingExpectedBroadcasters:
        case BroadcasterCanonicalizationKind::Unknown:
            return false;
    }

    return false;
}

} // namespace detail

[[nodiscard]] inline recalcshadow::ShadowComparison compareRollbackQueueToDocument(
    const recalcqueue::FormulaStateSnapshot& rSnapshot, const ScDocument& rDoc)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparisonKind;

    const auto aExpected = rSnapshot.maTreeOrder;
    const auto aActual = recalcshadow::detail::collectFormulaTreeAddresses(rDoc);
    const auto aExpectedSorted = recalcshadow::detail::normalizeAddresses(aExpected);
    const auto aActualSorted = recalcshadow::detail::normalizeAddresses(aActual);

    std::vector<api::CellAddress> aMissing;
    std::vector<api::CellAddress> aExtra;
    std::set_difference(aActualSorted.begin(), aActualSorted.end(), aExpectedSorted.begin(),
        aExpectedSorted.end(), std::back_inserter(aMissing), recalcshadow::detail::AddressLess {});
    std::set_difference(aExpectedSorted.begin(), aExpectedSorted.end(), aActualSorted.begin(),
        aActualSorted.end(), std::back_inserter(aExtra), recalcshadow::detail::AddressLess {});

    const auto aExpectedFiltered
        = recalcshadow::detail::filterPredictedToActualOrder(aExpected, aActual);

    ShadowComparison aComparison;
    aComparison.mnPredictedQueueCount = static_cast<sal_Int32>(aExpected.size());
    aComparison.mnActualQueueCount = static_cast<sal_Int32>(aActual.size());
    aComparison.mnMissingQueueCount = static_cast<sal_Int32>(aMissing.size());
    aComparison.mnExtraQueueCount = static_cast<sal_Int32>(aExtra.size());

    for (std::size_t nIndex = 0; nIndex < std::min(aExpectedFiltered.size(), aActual.size()); ++nIndex)
    {
        if (aExpectedFiltered[nIndex] != aActual[nIndex])
        {
            aComparison.mnFirstOrderMismatchIndex = static_cast<sal_Int32>(nIndex);
            aComparison.moPredictedOrderAddress = aExpectedFiltered[nIndex];
            aComparison.moActualOrderAddress = aActual[nIndex];
            break;
        }
    }

    if (!aMissing.empty())
        aComparison.meKind = ShadowComparisonKind::UnderScheduling;
    else if (aComparison.mnFirstOrderMismatchIndex >= 0)
        aComparison.meKind = ShadowComparisonKind::OrderMismatch;
    else if (!aExtra.empty())
        aComparison.meKind = ShadowComparisonKind::ConservativeSuperset;
    else
        aComparison.meKind = ShadowComparisonKind::Exact;

    return aComparison;
}

[[nodiscard]] inline RollbackObservation classifyRollbackObservation(
    const substrateobjectrealization::ObjectRealizationResult& rRealization,
    const recalcshadow::ShadowComparison& rQueue,
    const spreadsheetengine::detail::substrate::ComputationalShadowComparison& rComputational,
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rGraph,
    const spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison& rBroadcasters)
{
    using spreadsheetengine::detail::substrate::BroadcasterCanonicalizationKind;

    RollbackObservation aObservation;
    aObservation.mnExpectedBroadcasters
        = rBroadcasters.mnExpectedCellBroadcasters + rBroadcasters.mnExpectedAreaBroadcasters;
    aObservation.mnLiveBroadcasters
        = rBroadcasters.mnLiveCellBroadcasters + rBroadcasters.mnLiveAreaBroadcasters;
    aObservation.mbObjectRealizationApplied
        = rRealization.meKind == substrateobjectrealization::ObjectRealizationResultKind::Applied;
    aObservation.mbQueueExact = rQueue.meKind == recalcshadow::ShadowComparisonKind::Exact;
    aObservation.mbComputationalFullMatch = rComputational.mbFullMatch;
    aObservation.mbGraphFullMatch = rGraph.mbFullMatch;
    aObservation.mbBroadcasterExact = rBroadcasters.mbExactMatch;

    if (!aObservation.mbObjectRealizationApplied)
    {
        aObservation.meKind = RollbackObservationKind::OutOfContract;
        aObservation.maReason = rRealization.maReason;
        return aObservation;
    }

    if (aObservation.mbQueueExact && aObservation.mbComputationalFullMatch
        && aObservation.mbGraphFullMatch && aObservation.mbBroadcasterExact)
    {
        aObservation.meKind = RollbackObservationKind::Exact;
        return aObservation;
    }

    if (rQueue.meKind == recalcshadow::ShadowComparisonKind::OrderMismatch
        && rComputational.mbFullMatch && rGraph.mbFullMatch
        && (rBroadcasters.meKind == BroadcasterCanonicalizationKind::OrderingOnly
            || rBroadcasters.mbOrderingEquivalent))
    {
        aObservation.meKind = RollbackObservationKind::OrderingOnly;
        aObservation.maReason = u"rollback_ordering_only";
        return aObservation;
    }

    if (!rComputational.mbCellPopulationMatch
        || aObservation.mnLiveBroadcasters < aObservation.mnExpectedBroadcasters
        || rBroadcasters.meKind == BroadcasterCanonicalizationKind::MissingExpectedBroadcasters)
    {
        aObservation.meKind = RollbackObservationKind::MissingRestoredObjects;
        aObservation.maReason = u"missing_restored_objects";
        return aObservation;
    }

    if (detail::isHostOnlyRollbackKind(rBroadcasters.meKind))
    {
        aObservation.meKind = RollbackObservationKind::HostOnlyRollbackReconstruction;
        aObservation.maReason = u"host_only_rollback_reconstruction";
        return aObservation;
    }

    aObservation.meKind = RollbackObservationKind::QueueOrStateMismatch;
    if (rQueue.meKind != recalcshadow::ShadowComparisonKind::Exact)
        aObservation.maReason = u"rollback_queue_mismatch";
    else if (!rComputational.mbFullMatch)
        aObservation.maReason = u"rollback_computational_mismatch";
    else if (!rGraph.mbFullMatch)
        aObservation.maReason = u"rollback_graph_mismatch";
    else
        aObservation.maReason = u"rollback_state_mismatch";
    return aObservation;
}

[[nodiscard]] inline RollbackObservation classifyRollbackObservation(
    const RollbackResult& rResult, const recalcshadow::ShadowComparison& rQueue,
    const spreadsheetengine::detail::substrate::ComputationalShadowComparison& rComputational,
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rGraph,
    const spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison& rBroadcasters)
{
    if (rResult.meKind != RollbackResultKind::Applied)
    {
        RollbackObservation aObservation;
        aObservation.meKind = RollbackObservationKind::OutOfContract;
        aObservation.maReason = rResult.maReason;
        return aObservation;
    }

    return classifyRollbackObservation(
        rResult.maObjectRealization, rQueue, rComputational, rGraph, rBroadcasters);
}

[[nodiscard]] inline PrimitiveRollbackObservation classifyPrimitiveRollbackObservation(
    bool bPrimitiveRollbackApplied, const std::optional<RollbackObservation>& oRollback,
    api::StringView rReasonIfOutOfContract = {})
{
    PrimitiveRollbackObservation aObservation;
    aObservation.mbPrimitiveRollbackApplied = bPrimitiveRollbackApplied;

    if (!bPrimitiveRollbackApplied || !oRollback)
    {
        aObservation.meKind = PrimitiveRollbackObservationKind::OutOfContract;
        aObservation.maReason = rReasonIfOutOfContract;
        return aObservation;
    }

    aObservation.mbRollbackExact = oRollback->meKind == RollbackObservationKind::Exact;
    aObservation.mbQueueExact = oRollback->mbQueueExact;
    aObservation.mbComputationalFullMatch = oRollback->mbComputationalFullMatch;
    aObservation.mbGraphFullMatch = oRollback->mbGraphFullMatch;
    aObservation.mbBroadcasterExact = oRollback->mbBroadcasterExact;

    switch (oRollback->meKind)
    {
        case RollbackObservationKind::Exact:
            aObservation.meKind = PrimitiveRollbackObservationKind::Exact;
            return aObservation;
        case RollbackObservationKind::OrderingOnly:
            aObservation.meKind = PrimitiveRollbackObservationKind::OrderingOnly;
            aObservation.maReason = u"primitive_rollback_ordering_only";
            return aObservation;
        case RollbackObservationKind::MissingRestoredObjects:
            aObservation.meKind = PrimitiveRollbackObservationKind::MissingRestoredObjects;
            aObservation.maReason = u"primitive_rollback_missing_restored_objects";
            return aObservation;
        case RollbackObservationKind::HostOnlyRollbackReconstruction:
            aObservation.meKind
                = PrimitiveRollbackObservationKind::HiddenHostRollbackOrchestration;
            aObservation.maReason = u"host_only_primitive_rollback_orchestration";
            return aObservation;
        case RollbackObservationKind::QueueOrStateMismatch:
            aObservation.meKind = PrimitiveRollbackObservationKind::QueueOrStateMismatch;
            aObservation.maReason = u"primitive_rollback_queue_or_state_mismatch";
            return aObservation;
        case RollbackObservationKind::OutOfContract:
            aObservation.meKind = PrimitiveRollbackObservationKind::OutOfContract;
            aObservation.maReason = oRollback->maReason;
            return aObservation;
    }

    aObservation.meKind = PrimitiveRollbackObservationKind::OutOfContract;
    aObservation.maReason = u"primitive_rollback_observation_unknown";
    return aObservation;
}

[[nodiscard]] inline PrimitiveRollbackRecordResult buildAdmittedPrimitiveRollbackRecord(
    const AdmittedRollbackRecord& rRollback)
{
    PrimitiveRollbackRecordResult aResult;
    if (rRollback.mnGeneration < 0)
    {
        aResult.maReason = u"rollback_generation_out_of_contract";
        return aResult;
    }

    aResult.meKind = PrimitiveRollbackRecordResultKind::Built;
    aResult.maRecord.maRollback = rRollback;
    return aResult;
}

[[nodiscard]] inline AdmittedRollbackRecord buildAdmittedRollbackRecord(
    const spreadsheetengine::detail::substrate::MutableComputationalSubstrateState& rState,
    const recalcqueue::FormulaStateSnapshot& rFormulaState)
{
    AdmittedRollbackRecord aRecord;
    aRecord.mnGeneration = std::max({ rState.maFormulaCellLifetime.mnGeneration,
        rState.maCellStorage.mnGeneration, rState.maWiringContainers.mnGeneration });
    aRecord.maObjectRealization = substrateobjectrealization::buildAdmittedObjectRealization(rState);
    aRecord.maFormulaState = rFormulaState;
    return aRecord;
}

[[nodiscard]] inline RollbackResult applyAdmittedRollback(
    ScDocument& rDoc, const AdmittedRollbackRecord& rRollback)
{
    RollbackResult aResult;
    aResult.maObjectRealization
        = substrateobjectrealization::realizeAdmittedObjectRealization(
            rDoc, rRollback.maObjectRealization);
    if (aResult.maObjectRealization.meKind
        != substrateobjectrealization::ObjectRealizationResultKind::Applied)
    {
        aResult.maReason = aResult.maObjectRealization.maReason;
        return aResult;
    }

    recalcqueue::restoreFormulaState(rDoc, rRollback.maFormulaState);
    aResult.meKind = RollbackResultKind::Applied;
    return aResult;
}

[[nodiscard]] inline PrimitiveRollbackApplyResult applyAdmittedPrimitiveRollbackRecord(
    ScDocument& rDoc, const AdmittedPrimitiveRollbackRecord& rRollback)
{
    PrimitiveRollbackApplyResult aResult;
    aResult.maRollback = applyAdmittedRollback(rDoc, rRollback.maRollback);
    if (aResult.maRollback.meKind != RollbackResultKind::Applied)
    {
        aResult.maReason = aResult.maRollback.maReason;
        return aResult;
    }

    aResult.meKind = PrimitiveRollbackApplyResultKind::Applied;
    return aResult;
}

[[nodiscard]] inline const char* toString(RollbackObservationKind eKind)
{
    switch (eKind)
    {
        case RollbackObservationKind::Exact:
            return "exact";
        case RollbackObservationKind::OrderingOnly:
            return "ordering_only";
        case RollbackObservationKind::MissingRestoredObjects:
            return "missing_restored_objects";
        case RollbackObservationKind::HostOnlyRollbackReconstruction:
            return "host_only_rollback_reconstruction";
        case RollbackObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case RollbackObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "unknown";
}

[[nodiscard]] inline const char* toString(PrimitiveRollbackObservationKind eKind)
{
    switch (eKind)
    {
        case PrimitiveRollbackObservationKind::Exact:
            return "exact";
        case PrimitiveRollbackObservationKind::OrderingOnly:
            return "ordering_only";
        case PrimitiveRollbackObservationKind::HiddenHostRollbackOrchestration:
            return "hidden_host_rollback_orchestration";
        case PrimitiveRollbackObservationKind::MissingRestoredObjects:
            return "missing_restored_objects";
        case PrimitiveRollbackObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case PrimitiveRollbackObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "unknown";
}

} // namespace spreadsheetengine::compat::libreoffice::substraterollback

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
