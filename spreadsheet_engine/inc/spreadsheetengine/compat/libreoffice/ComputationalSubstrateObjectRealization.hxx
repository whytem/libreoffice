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
#include <optional>

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx>

namespace spreadsheetengine::compat::libreoffice::substrateobjectrealization
{

enum class ObjectRealizationObservationKind : sal_uInt8
{
    Exact,
    OrderingOnly,
    MissingRealizedObjects,
    HostOnlyRepairOrReconstruction,
    QueueOrStateMismatch,
    OutOfContract
};

struct ObjectRealizationObservation
{
    ObjectRealizationObservationKind meKind = ObjectRealizationObservationKind::OutOfContract;
    api::String maReason;
    sal_Int32 mnExpectedBroadcasters = 0;
    sal_Int32 mnLiveBroadcasters = 0;
    bool mbFormulaCellLifetimeApplied = false;
    bool mbCellStorageApplied = false;
    bool mbWiringApplied = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;
    bool mbBroadcasterExact = false;

    [[nodiscard]] constexpr bool operator==(const ObjectRealizationObservation& rOther) const
        = default;
};

enum class PrimitiveRealizationObservationKind : sal_uInt8
{
    Exact,
    OrderingOnly,
    HiddenHostRealizationOrchestration,
    MissingRealizedObjects,
    QueueOrStateMismatch,
    OutOfContract
};

struct PrimitiveRealizationObservation
{
    PrimitiveRealizationObservationKind meKind
        = PrimitiveRealizationObservationKind::OutOfContract;
    api::String maReason;
    bool mbPrimitiveRealizationApplied = false;
    bool mbObjectRealizationExact = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;
    bool mbBroadcasterExact = false;

    [[nodiscard]] constexpr bool operator==(const PrimitiveRealizationObservation& rOther) const
        = default;
};

enum class ObjectRealizationResultKind : sal_uInt8
{
    Applied,
    RejectedOutOfContract
};

enum class PrimitiveRealizationRecordResultKind : sal_uInt8
{
    Built,
    RejectedOutOfContract
};

struct AdmittedObjectRealization
{
    std::int64_t mnGeneration = 0;
    spreadsheetengine::detail::substrate::AdmittedFormulaCellLifetime maFormulaCellLifetime;
    spreadsheetengine::detail::substrate::AdmittedCellStorage maCellStorage;
    spreadsheetengine::detail::substrate::AdmittedWiringContainers maWiringContainers;

    [[nodiscard]] constexpr bool operator==(const AdmittedObjectRealization& rOther) const = default;

    [[nodiscard]] sal_Int32 getFormulaCellCount() const
    {
        return maFormulaCellLifetime.getFormulaCellCount();
    }

    [[nodiscard]] sal_Int32 getCellCount() const
    {
        return maCellStorage.getCellCount();
    }

    [[nodiscard]] sal_Int32 getBroadcasterNodeCount() const
    {
        return maWiringContainers.getBroadcasterNodeCount();
    }
};

struct AdmittedPrimitiveRealizationRecord
{
    AdmittedObjectRealization maObjectRealization;
    bool mbUsesPrimitiveRealization = true;

    [[nodiscard]] constexpr bool operator==(const AdmittedPrimitiveRealizationRecord& rOther) const
        = default;
};

struct PrimitiveRealizationRecordResult
{
    PrimitiveRealizationRecordResultKind meKind
        = PrimitiveRealizationRecordResultKind::RejectedOutOfContract;
    api::String maReason;
    AdmittedPrimitiveRealizationRecord maRecord;
};

struct ObjectRealizationResult
{
    ObjectRealizationResultKind meKind = ObjectRealizationResultKind::RejectedOutOfContract;
    api::String maReason;
    substrateformulalifetime::FormulaCellLifetimeResult maFormulaCellLifetime;
    substratecellstorage::CellStorageMirrorResult maCellStorage;
    substratewiring::WiringApplyResult maWiring;
};

enum class PrimitiveRealizationApplyResultKind : sal_uInt8
{
    Applied,
    RejectedOutOfContract
};

struct PrimitiveRealizationApplyResult
{
    PrimitiveRealizationApplyResultKind meKind
        = PrimitiveRealizationApplyResultKind::RejectedOutOfContract;
    api::String maReason;
    ObjectRealizationResult maObjectRealization;
};

namespace detail
{

[[nodiscard]] inline const api::String& firstNonEmptyReason(
    const api::String& rFirst, const api::String& rSecond, const api::String& rThird)
{
    if (!rFirst.empty())
        return rFirst;
    if (!rSecond.empty())
        return rSecond;
    return rThird;
}

[[nodiscard]] inline bool isHostOnlyRepairKind(
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

[[nodiscard]] inline ObjectRealizationObservation classifyObjectRealizationObservation(
    const substrateformulalifetime::FormulaCellLifetimeResult& rFormulaCellLifetime,
    const substratecellstorage::CellStorageMirrorResult& rCellStorage,
    const substratewiring::WiringApplyResult& rWiring,
    const recalcshadow::ShadowComparison& rQueue,
    const spreadsheetengine::detail::substrate::ComputationalShadowComparison& rComputational,
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rGraph,
    const spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison& rBroadcasters)
{
    using spreadsheetengine::detail::substrate::BroadcasterCanonicalizationKind;

    ObjectRealizationObservation aObservation;
    aObservation.mnExpectedBroadcasters
        = rBroadcasters.mnExpectedCellBroadcasters + rBroadcasters.mnExpectedAreaBroadcasters;
    aObservation.mnLiveBroadcasters
        = rBroadcasters.mnLiveCellBroadcasters + rBroadcasters.mnLiveAreaBroadcasters;
    aObservation.mbFormulaCellLifetimeApplied
        = rFormulaCellLifetime.meKind
          == substrateformulalifetime::FormulaCellLifetimeResultKind::Applied;
    aObservation.mbCellStorageApplied
        = rCellStorage.meKind == substratecellstorage::CellStorageMirrorResultKind::Applied;
    aObservation.mbWiringApplied = rWiring.meKind == substratewiring::WiringApplyResultKind::Applied;
    aObservation.mbQueueExact = rQueue.meKind == recalcshadow::ShadowComparisonKind::Exact;
    aObservation.mbComputationalFullMatch = rComputational.mbFullMatch;
    aObservation.mbGraphFullMatch = rGraph.mbFullMatch;
    aObservation.mbBroadcasterExact = rBroadcasters.mbExactMatch;

    if (!aObservation.mbFormulaCellLifetimeApplied || !aObservation.mbCellStorageApplied
        || !aObservation.mbWiringApplied)
    {
        aObservation.meKind = ObjectRealizationObservationKind::OutOfContract;
        aObservation.maReason = detail::firstNonEmptyReason(
            rFormulaCellLifetime.maReason, rCellStorage.maReason, rWiring.maReason);
        return aObservation;
    }

    if (aObservation.mbQueueExact && aObservation.mbComputationalFullMatch
        && aObservation.mbGraphFullMatch && aObservation.mbBroadcasterExact)
    {
        aObservation.meKind = ObjectRealizationObservationKind::Exact;
        return aObservation;
    }

    if (rQueue.meKind == recalcshadow::ShadowComparisonKind::OrderMismatch
        && rComputational.mbFullMatch && rGraph.mbFullMatch
        && (rBroadcasters.meKind == BroadcasterCanonicalizationKind::OrderingOnly
            || rBroadcasters.mbOrderingEquivalent))
    {
        aObservation.meKind = ObjectRealizationObservationKind::OrderingOnly;
        aObservation.maReason = u"realization_ordering_only";
        return aObservation;
    }

    if (!rComputational.mbCellPopulationMatch
        || aObservation.mnLiveBroadcasters < aObservation.mnExpectedBroadcasters
        || rBroadcasters.meKind == BroadcasterCanonicalizationKind::MissingExpectedBroadcasters)
    {
        aObservation.meKind = ObjectRealizationObservationKind::MissingRealizedObjects;
        aObservation.maReason = u"missing_realized_objects";
        return aObservation;
    }

    if (detail::isHostOnlyRepairKind(rBroadcasters.meKind))
    {
        aObservation.meKind = ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction;
        aObservation.maReason = u"host_only_repair_or_reconstruction";
        return aObservation;
    }

    aObservation.meKind = ObjectRealizationObservationKind::QueueOrStateMismatch;
    if (rQueue.meKind != recalcshadow::ShadowComparisonKind::Exact)
        aObservation.maReason = u"queue_mismatch";
    else if (!rComputational.mbFullMatch)
        aObservation.maReason = u"computational_mismatch";
    else if (!rGraph.mbFullMatch)
        aObservation.maReason = u"graph_mismatch";
    else
        aObservation.maReason = u"object_realization_mismatch";
    return aObservation;
}

[[nodiscard]] inline ObjectRealizationObservation classifyObjectRealizationObservation(
    const ObjectRealizationResult& rResult, const recalcshadow::ShadowComparison& rQueue,
    const spreadsheetengine::detail::substrate::ComputationalShadowComparison& rComputational,
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rGraph,
    const spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison& rBroadcasters)
{
    return classifyObjectRealizationObservation(rResult.maFormulaCellLifetime, rResult.maCellStorage,
        rResult.maWiring, rQueue, rComputational, rGraph, rBroadcasters);
}

[[nodiscard]] inline PrimitiveRealizationObservation classifyPrimitiveRealizationObservation(
    bool bPrimitiveRealizationApplied,
    const std::optional<ObjectRealizationObservation>& oObjectRealization,
    api::StringView rReasonIfOutOfContract = {})
{
    PrimitiveRealizationObservation aObservation;
    aObservation.mbPrimitiveRealizationApplied = bPrimitiveRealizationApplied;

    if (!bPrimitiveRealizationApplied || !oObjectRealization)
    {
        aObservation.meKind = PrimitiveRealizationObservationKind::OutOfContract;
        aObservation.maReason = rReasonIfOutOfContract;
        return aObservation;
    }

    aObservation.mbObjectRealizationExact
        = oObjectRealization->meKind == ObjectRealizationObservationKind::Exact;
    aObservation.mbQueueExact = oObjectRealization->mbQueueExact;
    aObservation.mbComputationalFullMatch = oObjectRealization->mbComputationalFullMatch;
    aObservation.mbGraphFullMatch = oObjectRealization->mbGraphFullMatch;
    aObservation.mbBroadcasterExact = oObjectRealization->mbBroadcasterExact;

    switch (oObjectRealization->meKind)
    {
        case ObjectRealizationObservationKind::Exact:
            aObservation.meKind = PrimitiveRealizationObservationKind::Exact;
            return aObservation;
        case ObjectRealizationObservationKind::OrderingOnly:
            aObservation.meKind = PrimitiveRealizationObservationKind::OrderingOnly;
            aObservation.maReason = u"primitive_realization_ordering_only";
            return aObservation;
        case ObjectRealizationObservationKind::MissingRealizedObjects:
            aObservation.meKind = PrimitiveRealizationObservationKind::MissingRealizedObjects;
            aObservation.maReason = u"primitive_realization_missing_objects";
            return aObservation;
        case ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction:
            aObservation.meKind
                = PrimitiveRealizationObservationKind::HiddenHostRealizationOrchestration;
            aObservation.maReason = u"host_only_primitive_realization_orchestration";
            return aObservation;
        case ObjectRealizationObservationKind::QueueOrStateMismatch:
            aObservation.meKind = PrimitiveRealizationObservationKind::QueueOrStateMismatch;
            aObservation.maReason = u"primitive_realization_queue_or_state_mismatch";
            return aObservation;
        case ObjectRealizationObservationKind::OutOfContract:
            aObservation.meKind = PrimitiveRealizationObservationKind::OutOfContract;
            aObservation.maReason = oObjectRealization->maReason;
            return aObservation;
    }

    aObservation.meKind = PrimitiveRealizationObservationKind::OutOfContract;
    aObservation.maReason = u"primitive_realization_observation_unknown";
    return aObservation;
}

[[nodiscard]] inline PrimitiveRealizationRecordResult buildAdmittedPrimitiveRealizationRecord(
    const AdmittedObjectRealization& rObjectRealization)
{
    PrimitiveRealizationRecordResult aResult;
    if (rObjectRealization.mnGeneration < 0)
    {
        aResult.maReason = u"realization_generation_out_of_contract";
        return aResult;
    }

    aResult.meKind = PrimitiveRealizationRecordResultKind::Built;
    aResult.maRecord.maObjectRealization = rObjectRealization;
    return aResult;
}

[[nodiscard]] inline AdmittedObjectRealization buildAdmittedObjectRealization(
    const spreadsheetengine::detail::substrate::MutableComputationalSubstrateState& rState)
{
    AdmittedObjectRealization aRealization;
    aRealization.mnGeneration
        = std::max({ rState.maFormulaCellLifetime.mnGeneration, rState.maCellStorage.mnGeneration,
            rState.maWiringContainers.mnGeneration });
    aRealization.maFormulaCellLifetime = rState.maFormulaCellLifetime;
    aRealization.maCellStorage = rState.maCellStorage;
    aRealization.maWiringContainers = rState.maWiringContainers;
    return aRealization;
}

[[nodiscard]] inline ObjectRealizationResult realizeAdmittedObjectRealization(
    ScDocument& rDoc, const AdmittedObjectRealization& rRealization)
{
    ObjectRealizationResult aResult;

    aResult.maFormulaCellLifetime
        = substrateformulalifetime::realizeAdmittedFormulaCellLifetime(
            rDoc, rRealization.maFormulaCellLifetime);
    if (aResult.maFormulaCellLifetime.meKind
        != substrateformulalifetime::FormulaCellLifetimeResultKind::Applied)
    {
        aResult.maReason = aResult.maFormulaCellLifetime.maReason;
        return aResult;
    }

    aResult.maCellStorage
        = substratecellstorage::mirrorAdmittedScalarCellStorage(rDoc, rRealization.maCellStorage);
    if (aResult.maCellStorage.meKind
        != substratecellstorage::CellStorageMirrorResultKind::Applied)
    {
        aResult.maReason = aResult.maCellStorage.maReason;
        return aResult;
    }

    rDoc.CalcAll();

    aResult.maWiring
        = substratewiring::realizeAdmittedWiringContainers(rDoc, rRealization.maWiringContainers);
    if (aResult.maWiring.meKind != substratewiring::WiringApplyResultKind::Applied)
    {
        aResult.maReason = aResult.maWiring.maReason;
        return aResult;
    }

    aResult.meKind = ObjectRealizationResultKind::Applied;
    return aResult;
}

[[nodiscard]] inline PrimitiveRealizationApplyResult applyAdmittedPrimitiveRealizationRecord(
    ScDocument& rDoc, const AdmittedPrimitiveRealizationRecord& rRealization)
{
    PrimitiveRealizationApplyResult aResult;
    aResult.maObjectRealization
        = realizeAdmittedObjectRealization(rDoc, rRealization.maObjectRealization);
    if (aResult.maObjectRealization.meKind != ObjectRealizationResultKind::Applied)
    {
        aResult.maReason = aResult.maObjectRealization.maReason;
        return aResult;
    }

    aResult.meKind = PrimitiveRealizationApplyResultKind::Applied;
    return aResult;
}

[[nodiscard]] inline const char* toString(ObjectRealizationObservationKind eKind)
{
    switch (eKind)
    {
        case ObjectRealizationObservationKind::Exact:
            return "exact";
        case ObjectRealizationObservationKind::OrderingOnly:
            return "ordering_only";
        case ObjectRealizationObservationKind::MissingRealizedObjects:
            return "missing_realized_objects";
        case ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction:
            return "host_only_repair_or_reconstruction";
        case ObjectRealizationObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case ObjectRealizationObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "out_of_contract";
}

[[nodiscard]] inline const char* toString(PrimitiveRealizationObservationKind eKind)
{
    switch (eKind)
    {
        case PrimitiveRealizationObservationKind::Exact:
            return "exact";
        case PrimitiveRealizationObservationKind::OrderingOnly:
            return "ordering_only";
        case PrimitiveRealizationObservationKind::HiddenHostRealizationOrchestration:
            return "hidden_host_realization_orchestration";
        case PrimitiveRealizationObservationKind::MissingRealizedObjects:
            return "missing_realized_objects";
        case PrimitiveRealizationObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case PrimitiveRealizationObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "out_of_contract";
}

} // namespace spreadsheetengine::compat::libreoffice::substrateobjectrealization

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
