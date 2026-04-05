/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>

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

} // namespace spreadsheetengine::compat::libreoffice::substrateobjectrealization

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
