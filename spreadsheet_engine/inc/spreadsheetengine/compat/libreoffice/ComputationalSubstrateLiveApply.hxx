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
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice::substrateliveapply
{

enum class LiveApplyObservationKind : sal_uInt8
{
    Exact,
    OrderingOnly,
    HiddenHostApplyOrchestration,
    MissingRealizedOrRolledBackObjects,
    QueueOrStateMismatch,
    OutOfContract
};

struct LiveApplyObservation
{
    LiveApplyObservationKind meKind = LiveApplyObservationKind::OutOfContract;
    api::String maReason;
    bool mbRolledBack = false;
    bool mbRawMutationExact = false;
    bool mbObjectRealizationExact = false;
    bool mbRollbackExact = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;

    [[nodiscard]] constexpr bool operator==(const LiveApplyObservation& rOther) const = default;
};

[[nodiscard]] inline LiveApplyObservation classifyLiveApplyObservation(
    const substraterawmutation::RawMutationObservation& rRawMutation,
    const std::optional<substrateobjectrealization::ObjectRealizationObservation>& oObjectRealization,
    const std::optional<substraterollback::RollbackObservation>& oRollback)
{
    LiveApplyObservation aObservation;
    aObservation.mbRolledBack = oRollback.has_value();
    aObservation.mbRawMutationExact
        = rRawMutation.meKind == substraterawmutation::RawMutationObservationKind::Exact;
    aObservation.mbObjectRealizationExact
        = oObjectRealization
          && oObjectRealization->meKind
                 == substrateobjectrealization::ObjectRealizationObservationKind::Exact;
    aObservation.mbRollbackExact = oRollback
                                   && oRollback->meKind
                                          == substraterollback::RollbackObservationKind::Exact;
    aObservation.mbQueueExact = rRawMutation.mbQueueExact;
    aObservation.mbComputationalFullMatch = rRawMutation.mbComputationalFullMatch;
    aObservation.mbGraphFullMatch = rRawMutation.mbGraphFullMatch;

    if (rRawMutation.meKind == substraterawmutation::RawMutationObservationKind::OutOfContract)
    {
        aObservation.meKind = LiveApplyObservationKind::OutOfContract;
        aObservation.maReason = rRawMutation.maReason;
        return aObservation;
    }

    if (oRollback)
    {
        switch (oRollback->meKind)
        {
            case substraterollback::RollbackObservationKind::Exact:
                aObservation.meKind = LiveApplyObservationKind::Exact;
                aObservation.maReason = u"exact_rollback";
                return aObservation;
            case substraterollback::RollbackObservationKind::OrderingOnly:
                aObservation.meKind = LiveApplyObservationKind::OrderingOnly;
                aObservation.maReason = u"rollback_ordering_only";
                return aObservation;
            case substraterollback::RollbackObservationKind::MissingRestoredObjects:
                aObservation.meKind = LiveApplyObservationKind::MissingRealizedOrRolledBackObjects;
                aObservation.maReason = u"missing_rolled_back_objects";
                return aObservation;
            case substraterollback::RollbackObservationKind::HostOnlyRollbackReconstruction:
                aObservation.meKind = LiveApplyObservationKind::HiddenHostApplyOrchestration;
                aObservation.maReason = u"host_only_rollback_orchestration";
                return aObservation;
            case substraterollback::RollbackObservationKind::QueueOrStateMismatch:
                aObservation.meKind = LiveApplyObservationKind::QueueOrStateMismatch;
                aObservation.maReason = u"rollback_queue_or_state_mismatch";
                return aObservation;
            case substraterollback::RollbackObservationKind::OutOfContract:
                aObservation.meKind = LiveApplyObservationKind::OutOfContract;
                aObservation.maReason = oRollback->maReason;
                return aObservation;
        }
    }

    if (!oObjectRealization)
    {
        aObservation.meKind = LiveApplyObservationKind::OutOfContract;
        aObservation.maReason = u"missing_object_realization_observation";
        return aObservation;
    }

    switch (oObjectRealization->meKind)
    {
        case substrateobjectrealization::ObjectRealizationObservationKind::Exact:
            break;
        case substrateobjectrealization::ObjectRealizationObservationKind::OrderingOnly:
            aObservation.meKind = LiveApplyObservationKind::OrderingOnly;
            aObservation.maReason = u"apply_ordering_only";
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::MissingRealizedObjects:
            aObservation.meKind = LiveApplyObservationKind::MissingRealizedOrRolledBackObjects;
            aObservation.maReason = u"missing_realized_objects";
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::HostOnlyRepairOrReconstruction:
            aObservation.meKind = LiveApplyObservationKind::HiddenHostApplyOrchestration;
            aObservation.maReason = u"host_only_apply_orchestration";
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::QueueOrStateMismatch:
            aObservation.meKind = LiveApplyObservationKind::QueueOrStateMismatch;
            aObservation.maReason = u"apply_queue_or_state_mismatch";
            return aObservation;
        case substrateobjectrealization::ObjectRealizationObservationKind::OutOfContract:
            aObservation.meKind = LiveApplyObservationKind::OutOfContract;
            aObservation.maReason = oObjectRealization->maReason;
            return aObservation;
    }

    switch (rRawMutation.meKind)
    {
        case substraterawmutation::RawMutationObservationKind::Exact:
            aObservation.meKind = LiveApplyObservationKind::Exact;
            return aObservation;
        case substraterawmutation::RawMutationObservationKind::OrderingOnly:
            aObservation.meKind = LiveApplyObservationKind::OrderingOnly;
            aObservation.maReason = u"raw_apply_ordering_only";
            return aObservation;
        case substraterawmutation::RawMutationObservationKind::HiddenHostMutationReconstruction:
            aObservation.meKind = LiveApplyObservationKind::HiddenHostApplyOrchestration;
            aObservation.maReason = u"host_only_raw_apply_orchestration";
            return aObservation;
        case substraterawmutation::RawMutationObservationKind::MissingRealizedOrRolledBackObjects:
            aObservation.meKind = LiveApplyObservationKind::MissingRealizedOrRolledBackObjects;
            aObservation.maReason = u"raw_apply_missing_objects";
            return aObservation;
        case substraterawmutation::RawMutationObservationKind::QueueOrStateMismatch:
            aObservation.meKind = LiveApplyObservationKind::QueueOrStateMismatch;
            aObservation.maReason = u"raw_apply_queue_or_state_mismatch";
            return aObservation;
        case substraterawmutation::RawMutationObservationKind::OutOfContract:
            aObservation.meKind = LiveApplyObservationKind::OutOfContract;
            aObservation.maReason = rRawMutation.maReason;
            return aObservation;
    }

    aObservation.meKind = LiveApplyObservationKind::OutOfContract;
    aObservation.maReason = u"live_apply_observation_unknown";
    return aObservation;
}

[[nodiscard]] inline const char* toString(LiveApplyObservationKind eKind)
{
    switch (eKind)
    {
        case LiveApplyObservationKind::Exact:
            return "exact";
        case LiveApplyObservationKind::OrderingOnly:
            return "ordering_only";
        case LiveApplyObservationKind::HiddenHostApplyOrchestration:
            return "hidden_host_apply_orchestration";
        case LiveApplyObservationKind::MissingRealizedOrRolledBackObjects:
            return "missing_realized_or_rolled_back_objects";
        case LiveApplyObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case LiveApplyObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "unknown";
}

} // namespace spreadsheetengine::compat::libreoffice::substrateliveapply

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
