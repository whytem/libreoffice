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
#include <array>
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

enum class LiveApplyStageKind : sal_uInt8
{
    RawMutation,
    Realization,
    Verification,
    Rollback
};

struct AdmittedLiveApplyPlan
{
    substraterawmutation::AdmittedRawMutationRecord maRawMutation;
    std::optional<substrateobjectrealization::AdmittedObjectRealization> moObjectRealization;
    substraterollback::AdmittedRollbackRecord maRollback;
    std::array<LiveApplyStageKind, 4> maStageOrder{ LiveApplyStageKind::RawMutation,
        LiveApplyStageKind::Verification, LiveApplyStageKind::Rollback,
        LiveApplyStageKind::Rollback };
    sal_uInt8 mnStageCount = 0;
    bool mbRolledBack = false;
    bool mbVerificationRequired = false;

    [[nodiscard]] constexpr bool operator==(const AdmittedLiveApplyPlan& rOther) const = default;
};

enum class LiveApplyPlanBuildResultKind : sal_uInt8
{
    Built,
    RejectedOutOfContract
};

struct LiveApplyPlanBuildResult
{
    LiveApplyPlanBuildResultKind meKind = LiveApplyPlanBuildResultKind::RejectedOutOfContract;
    api::String maReason;
    AdmittedLiveApplyPlan maPlan;
};

[[nodiscard]] inline bool hasStage(
    const AdmittedLiveApplyPlan& rPlan, LiveApplyStageKind eStage)
{
    return std::find(rPlan.maStageOrder.begin(), rPlan.maStageOrder.begin() + rPlan.mnStageCount, eStage)
           != rPlan.maStageOrder.begin() + rPlan.mnStageCount;
}

[[nodiscard]] inline LiveApplyPlanBuildResult buildAdmittedLiveApplyPlan(
    const substraterawmutation::AdmittedRawMutationRecord& rRawMutation,
    const std::optional<substrateobjectrealization::AdmittedObjectRealization>& oObjectRealization,
    const substraterollback::AdmittedRollbackRecord& rRollback, bool bRolledBack)
{
    LiveApplyPlanBuildResult aResult;
    aResult.meKind = LiveApplyPlanBuildResultKind::Built;
    aResult.maPlan.maRawMutation = rRawMutation;
    aResult.maPlan.moObjectRealization = oObjectRealization;
    aResult.maPlan.maRollback = rRollback;
    aResult.maPlan.mbRolledBack = bRolledBack;
    aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++] = LiveApplyStageKind::RawMutation;

    if (oObjectRealization)
        aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++] = LiveApplyStageKind::Realization;

    if (bRolledBack)
    {
        aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++] = LiveApplyStageKind::Rollback;
        return aResult;
    }

    if (!oObjectRealization)
    {
        aResult.meKind = LiveApplyPlanBuildResultKind::RejectedOutOfContract;
        aResult.maReason = u"missing_object_realization_for_apply_plan";
        return aResult;
    }

    aResult.maPlan.mbVerificationRequired = true;
    aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++] = LiveApplyStageKind::Verification;
    return aResult;
}

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

[[nodiscard]] inline const char* toString(LiveApplyStageKind eKind)
{
    switch (eKind)
    {
        case LiveApplyStageKind::RawMutation:
            return "raw_mutation";
        case LiveApplyStageKind::Realization:
            return "realization";
        case LiveApplyStageKind::Verification:
            return "verification";
        case LiveApplyStageKind::Rollback:
            return "rollback";
    }

    return "unknown";
}

} // namespace spreadsheetengine::compat::libreoffice::substrateliveapply

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
