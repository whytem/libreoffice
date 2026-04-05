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

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>

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
