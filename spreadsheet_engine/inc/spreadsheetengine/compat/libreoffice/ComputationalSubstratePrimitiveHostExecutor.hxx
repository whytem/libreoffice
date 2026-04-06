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

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateFinalVerification.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstratePrimitiveExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor
{

enum class PrimitiveHostExecutorObservationKind : sal_uInt8
{
    Exact,
    NormalizedEquivalent,
    OrderingOnly,
    HiddenHostCallOrchestration,
    MissingHostCallInputs,
    QueueOrStateMismatch,
    OutOfContract
};

struct PrimitiveHostExecutorObservation
{
    PrimitiveHostExecutorObservationKind meKind
        = PrimitiveHostExecutorObservationKind::OutOfContract;
    api::String maReason;
    bool mbPrimitiveHostExecutorApplied = false;
    bool mbRolledBack = false;
    bool mbRawDocumentMutationExact = false;
    bool mbPrimitiveExecutionExact = false;
    bool mbPrimitiveRealizationExact = false;
    bool mbPrimitiveRollbackExact = false;
    bool mbFinalVerificationExact = false;
    bool mbFinalVerificationNormalizedEquivalent = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;

    [[nodiscard]] constexpr bool operator==(const PrimitiveHostExecutorObservation& rOther) const
        = default;
};

enum class PrimitiveHostExecutorStageKind : sal_uInt8
{
    RawDocumentMutationCall,
    PrimitiveRealizationCall,
    PrimitiveRollbackCall,
    FinalVerificationCall
};

struct AdmittedPrimitiveHostExecutorPlan
{
    substraterawmutation::AdmittedRawDocumentMutationRecord maRawDocumentMutation;
    substrateprimitiveexecution::AdmittedPrimitiveExecutionPlan maPrimitiveExecution;
    std::optional<substrateobjectrealization::AdmittedPrimitiveRealizationRecord>
        moPrimitiveRealization;
    std::optional<substraterollback::AdmittedPrimitiveRollbackRecord> moPrimitiveRollback;
    substratefinalverification::AdmittedFinalVerificationRecord maFinalVerification;
    bool mbRolledBack = false;
    std::array<PrimitiveHostExecutorStageKind, 4> maStageOrder{
        PrimitiveHostExecutorStageKind::RawDocumentMutationCall,
        PrimitiveHostExecutorStageKind::RawDocumentMutationCall,
        PrimitiveHostExecutorStageKind::RawDocumentMutationCall,
        PrimitiveHostExecutorStageKind::RawDocumentMutationCall,
    };
    sal_uInt8 mnStageCount = 0;
    bool mbUsesPrimitiveHostExecutor = true;

    [[nodiscard]] constexpr bool operator==(const AdmittedPrimitiveHostExecutorPlan& rOther) const
        = default;
};

enum class PrimitiveHostExecutorPlanBuildResultKind : sal_uInt8
{
    Built,
    RejectedOutOfContract
};

struct PrimitiveHostExecutorPlanBuildResult
{
    PrimitiveHostExecutorPlanBuildResultKind meKind
        = PrimitiveHostExecutorPlanBuildResultKind::RejectedOutOfContract;
    api::String maReason;
    AdmittedPrimitiveHostExecutorPlan maPlan;
};

namespace detail
{

[[nodiscard]] inline bool isHiddenHostRawDocumentMutation(
    substraterawmutation::RawDocumentMutationObservationKind eKind)
{
    return eKind
           == substraterawmutation::RawDocumentMutationObservationKind::
               HiddenHostMutationOrchestration;
}

[[nodiscard]] inline bool isMissingRawDocumentMutationInput(
    substraterawmutation::RawDocumentMutationObservationKind eKind)
{
    return eKind
           == substraterawmutation::RawDocumentMutationObservationKind::
               MissingRealizedOrRolledBackObjects;
}

[[nodiscard]] inline bool isHiddenHostPrimitiveExecution(
    substrateprimitiveexecution::PrimitiveExecutionObservationKind eKind)
{
    return eKind
           == substrateprimitiveexecution::PrimitiveExecutionObservationKind::
               HiddenHostPrimitiveExecutionOrchestration;
}

[[nodiscard]] inline bool isMissingPrimitiveExecutionInput(
    substrateprimitiveexecution::PrimitiveExecutionObservationKind eKind)
{
    return eKind
           == substrateprimitiveexecution::PrimitiveExecutionObservationKind::
               MissingPrimitiveExecutionInputs;
}

[[nodiscard]] inline bool isHiddenHostPrimitiveRealization(
    substrateobjectrealization::PrimitiveRealizationObservationKind eKind)
{
    return eKind
           == substrateobjectrealization::PrimitiveRealizationObservationKind::
               HiddenHostRealizationOrchestration;
}

[[nodiscard]] inline bool isMissingPrimitiveRealizationInput(
    substrateobjectrealization::PrimitiveRealizationObservationKind eKind)
{
    return eKind
           == substrateobjectrealization::PrimitiveRealizationObservationKind::
               MissingRealizedObjects;
}

[[nodiscard]] inline bool isHiddenHostPrimitiveRollback(
    substraterollback::PrimitiveRollbackObservationKind eKind)
{
    return eKind
           == substraterollback::PrimitiveRollbackObservationKind::
               HiddenHostRollbackOrchestration;
}

[[nodiscard]] inline bool isMissingPrimitiveRollbackInput(
    substraterollback::PrimitiveRollbackObservationKind eKind)
{
    return eKind
           == substraterollback::PrimitiveRollbackObservationKind::MissingRestoredObjects;
}

[[nodiscard]] inline bool isHiddenHostFinalVerification(
    substratefinalverification::FinalVerificationObservationKind eKind)
{
    return eKind
           == substratefinalverification::FinalVerificationObservationKind::
               HiddenHostVerificationOrchestration;
}

[[nodiscard]] inline bool isMissingFinalVerificationInput(
    substratefinalverification::FinalVerificationObservationKind eKind)
{
    return eKind
           == substratefinalverification::FinalVerificationObservationKind::
               MissingVerificationInputs;
}

} // namespace detail

[[nodiscard]] inline bool hasStage(
    const AdmittedPrimitiveHostExecutorPlan& rPlan, PrimitiveHostExecutorStageKind eStage)
{
    return std::find(rPlan.maStageOrder.begin(), rPlan.maStageOrder.begin() + rPlan.mnStageCount, eStage)
           != rPlan.maStageOrder.begin() + rPlan.mnStageCount;
}

[[nodiscard]] inline PrimitiveHostExecutorPlanBuildResult buildAdmittedPrimitiveHostExecutorPlan(
    const substraterawmutation::AdmittedRawDocumentMutationRecord& rRawDocumentMutation,
    const substrateprimitiveexecution::AdmittedPrimitiveExecutionPlan& rPrimitiveExecution,
    const std::optional<substrateobjectrealization::AdmittedPrimitiveRealizationRecord>&
        oPrimitiveRealization,
    const std::optional<substraterollback::AdmittedPrimitiveRollbackRecord>& oPrimitiveRollback,
    const substratefinalverification::AdmittedFinalVerificationRecord& rFinalVerification)
{
    PrimitiveHostExecutorPlanBuildResult aResult;
    aResult.maPlan.maRawDocumentMutation = rRawDocumentMutation;
    aResult.maPlan.maPrimitiveExecution = rPrimitiveExecution;
    aResult.maPlan.moPrimitiveRealization = oPrimitiveRealization;
    aResult.maPlan.moPrimitiveRollback = oPrimitiveRollback;
    aResult.maPlan.maFinalVerification = rFinalVerification;
    aResult.maPlan.mbRolledBack = oPrimitiveRollback.has_value();
    aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++]
        = PrimitiveHostExecutorStageKind::RawDocumentMutationCall;

    if (oPrimitiveRealization && oPrimitiveRollback)
    {
        aResult.maReason = u"conflicting_primitive_host_executor_branch";
        return aResult;
    }

    if (!oPrimitiveRealization && !oPrimitiveRollback)
    {
        aResult.maReason = u"missing_primitive_host_executor_branch";
        return aResult;
    }

    if (rPrimitiveExecution.mbRolledBack != oPrimitiveRollback.has_value())
    {
        aResult.maReason = u"primitive_execution_and_host_executor_branch_mismatch";
        return aResult;
    }

    if (rFinalVerification.moPrimitiveRealization.has_value() != oPrimitiveRealization.has_value()
        || rFinalVerification.moPrimitiveRollback.has_value() != oPrimitiveRollback.has_value())
    {
        aResult.maReason = u"final_verification_and_host_executor_branch_mismatch";
        return aResult;
    }

    if (oPrimitiveRealization)
    {
        if (!substrateprimitiveexecution::hasStage(
                rPrimitiveExecution,
                substrateprimitiveexecution::PrimitiveExecutionStageKind::PrimitiveRealization))
        {
            aResult.maReason = u"missing_primitive_realization_stage_for_host_executor";
            return aResult;
        }

        aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++]
            = PrimitiveHostExecutorStageKind::PrimitiveRealizationCall;
    }
    else
    {
        if (!substrateprimitiveexecution::hasStage(
                rPrimitiveExecution,
                substrateprimitiveexecution::PrimitiveExecutionStageKind::PrimitiveRollback))
        {
            aResult.maReason = u"missing_primitive_rollback_stage_for_host_executor";
            return aResult;
        }

        aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++]
            = PrimitiveHostExecutorStageKind::PrimitiveRollbackCall;
    }

    if (!substrateprimitiveexecution::hasStage(
            rPrimitiveExecution,
            substrateprimitiveexecution::PrimitiveExecutionStageKind::FinalVerification)
        || !rFinalVerification.mbUsesFinalVerification)
    {
        aResult.maReason = u"missing_final_verification_stage_for_host_executor";
        return aResult;
    }

    aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++]
        = PrimitiveHostExecutorStageKind::FinalVerificationCall;
    aResult.meKind = PrimitiveHostExecutorPlanBuildResultKind::Built;
    return aResult;
}

[[nodiscard]] inline PrimitiveHostExecutorObservation classifyPrimitiveHostExecutorObservation(
    bool bPrimitiveHostExecutorApplied,
    const std::optional<substraterawmutation::RawDocumentMutationObservation>&
        oRawDocumentMutation,
    const std::optional<substrateprimitiveexecution::PrimitiveExecutionObservation>&
        oPrimitiveExecution,
    const std::optional<substrateobjectrealization::PrimitiveRealizationObservation>&
        oPrimitiveRealization,
    const std::optional<substraterollback::PrimitiveRollbackObservation>& oPrimitiveRollback,
    const std::optional<substratefinalverification::FinalVerificationObservation>&
        oFinalVerification,
    api::StringView rReasonIfOutOfContract = {})
{
    PrimitiveHostExecutorObservation aObservation;
    aObservation.mbPrimitiveHostExecutorApplied = bPrimitiveHostExecutorApplied;
    aObservation.mbRolledBack = oPrimitiveRollback.has_value();

    if (!bPrimitiveHostExecutorApplied || !oRawDocumentMutation || !oPrimitiveExecution
        || !oFinalVerification)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::OutOfContract;
        aObservation.maReason = rReasonIfOutOfContract;
        return aObservation;
    }

    if (oPrimitiveRealization.has_value() == oPrimitiveRollback.has_value())
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::OutOfContract;
        aObservation.maReason = u"missing_or_conflicting_primitive_host_executor_branch";
        return aObservation;
    }

    aObservation.mbRawDocumentMutationExact
        = oRawDocumentMutation->meKind
          == substraterawmutation::RawDocumentMutationObservationKind::Exact;
    aObservation.mbPrimitiveExecutionExact
        = oPrimitiveExecution->meKind
          == substrateprimitiveexecution::PrimitiveExecutionObservationKind::Exact;
    aObservation.mbPrimitiveRealizationExact
        = oPrimitiveRealization
          && oPrimitiveRealization->meKind
                 == substrateobjectrealization::PrimitiveRealizationObservationKind::Exact;
    aObservation.mbPrimitiveRollbackExact
        = oPrimitiveRollback
          && oPrimitiveRollback->meKind
                 == substraterollback::PrimitiveRollbackObservationKind::Exact;
    aObservation.mbFinalVerificationExact
        = oFinalVerification->meKind
          == substratefinalverification::FinalVerificationObservationKind::Exact;
    aObservation.mbFinalVerificationNormalizedEquivalent
        = oFinalVerification->meKind
          == substratefinalverification::FinalVerificationObservationKind::NormalizedEquivalent;
    aObservation.mbQueueExact = oFinalVerification->mbQueueExact;
    aObservation.mbComputationalFullMatch = oFinalVerification->mbComputationalFullMatch;
    aObservation.mbGraphFullMatch = oFinalVerification->mbGraphFullMatch;

    if (oRawDocumentMutation->meKind
        == substraterawmutation::RawDocumentMutationObservationKind::OutOfContract)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::OutOfContract;
        aObservation.maReason = oRawDocumentMutation->maReason;
        return aObservation;
    }

    if (oPrimitiveExecution->meKind
        == substrateprimitiveexecution::PrimitiveExecutionObservationKind::OutOfContract)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::OutOfContract;
        aObservation.maReason = oPrimitiveExecution->maReason;
        return aObservation;
    }

    if (oPrimitiveRealization
        && oPrimitiveRealization->meKind
               == substrateobjectrealization::PrimitiveRealizationObservationKind::OutOfContract)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::OutOfContract;
        aObservation.maReason = oPrimitiveRealization->maReason;
        return aObservation;
    }

    if (oPrimitiveRollback
        && oPrimitiveRollback->meKind
               == substraterollback::PrimitiveRollbackObservationKind::OutOfContract)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::OutOfContract;
        aObservation.maReason = oPrimitiveRollback->maReason;
        return aObservation;
    }

    if (oFinalVerification->meKind
        == substratefinalverification::FinalVerificationObservationKind::OutOfContract)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::OutOfContract;
        aObservation.maReason = oFinalVerification->maReason;
        return aObservation;
    }

    if (detail::isHiddenHostRawDocumentMutation(oRawDocumentMutation->meKind)
        || detail::isHiddenHostPrimitiveExecution(oPrimitiveExecution->meKind)
        || (oPrimitiveRealization
            && detail::isHiddenHostPrimitiveRealization(oPrimitiveRealization->meKind))
        || (oPrimitiveRollback
            && detail::isHiddenHostPrimitiveRollback(oPrimitiveRollback->meKind))
        || detail::isHiddenHostFinalVerification(oFinalVerification->meKind))
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::HiddenHostCallOrchestration;
        aObservation.maReason = u"hidden_host_call_orchestration";
        return aObservation;
    }

    if (detail::isMissingRawDocumentMutationInput(oRawDocumentMutation->meKind)
        || detail::isMissingPrimitiveExecutionInput(oPrimitiveExecution->meKind)
        || (oPrimitiveRealization
            && detail::isMissingPrimitiveRealizationInput(oPrimitiveRealization->meKind))
        || (oPrimitiveRollback
            && detail::isMissingPrimitiveRollbackInput(oPrimitiveRollback->meKind))
        || detail::isMissingFinalVerificationInput(oFinalVerification->meKind))
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::MissingHostCallInputs;
        aObservation.maReason = u"missing_host_call_inputs";
        return aObservation;
    }

    if (oRawDocumentMutation->meKind
            == substraterawmutation::RawDocumentMutationObservationKind::QueueOrStateMismatch
        || oPrimitiveExecution->meKind
               == substrateprimitiveexecution::PrimitiveExecutionObservationKind::
                   QueueOrStateMismatch
        || (oPrimitiveRealization
            && oPrimitiveRealization->meKind
                   == substrateobjectrealization::PrimitiveRealizationObservationKind::
                       QueueOrStateMismatch)
        || (oPrimitiveRollback
            && oPrimitiveRollback->meKind
                   == substraterollback::PrimitiveRollbackObservationKind::QueueOrStateMismatch)
        || oFinalVerification->meKind
               == substratefinalverification::FinalVerificationObservationKind::
                   QueueOrStateMismatch)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::QueueOrStateMismatch;
        aObservation.maReason = u"primitive_host_executor_queue_or_state_mismatch";
        return aObservation;
    }

    if (oRawDocumentMutation->meKind
            == substraterawmutation::RawDocumentMutationObservationKind::OrderingOnly
        || oPrimitiveExecution->meKind
               == substrateprimitiveexecution::PrimitiveExecutionObservationKind::OrderingOnly
        || (oPrimitiveRealization
            && oPrimitiveRealization->meKind
                   == substrateobjectrealization::PrimitiveRealizationObservationKind::
                       OrderingOnly)
        || (oPrimitiveRollback
            && oPrimitiveRollback->meKind
                   == substraterollback::PrimitiveRollbackObservationKind::OrderingOnly)
        || oFinalVerification->meKind
               == substratefinalverification::FinalVerificationObservationKind::OrderingOnly)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::OrderingOnly;
        aObservation.maReason = u"primitive_host_executor_ordering_only";
        return aObservation;
    }

    if (oPrimitiveExecution->meKind
            == substrateprimitiveexecution::PrimitiveExecutionObservationKind::
                NormalizedEquivalent
        || oFinalVerification->meKind
               == substratefinalverification::FinalVerificationObservationKind::
                   NormalizedEquivalent)
    {
        aObservation.meKind = PrimitiveHostExecutorObservationKind::NormalizedEquivalent;
        aObservation.maReason = u"primitive_host_executor_normalized_equivalent";
        return aObservation;
    }

    aObservation.meKind = PrimitiveHostExecutorObservationKind::Exact;
    return aObservation;
}

[[nodiscard]] inline const char* toString(PrimitiveHostExecutorObservationKind eKind)
{
    switch (eKind)
    {
        case PrimitiveHostExecutorObservationKind::Exact:
            return "exact";
        case PrimitiveHostExecutorObservationKind::NormalizedEquivalent:
            return "normalized_equivalent";
        case PrimitiveHostExecutorObservationKind::OrderingOnly:
            return "ordering_only";
        case PrimitiveHostExecutorObservationKind::HiddenHostCallOrchestration:
            return "hidden_host_call_orchestration";
        case PrimitiveHostExecutorObservationKind::MissingHostCallInputs:
            return "missing_host_call_inputs";
        case PrimitiveHostExecutorObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case PrimitiveHostExecutorObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "unknown";
}

[[nodiscard]] inline const char* toString(PrimitiveHostExecutorStageKind eKind)
{
    switch (eKind)
    {
        case PrimitiveHostExecutorStageKind::RawDocumentMutationCall:
            return "raw_document_mutation_call";
        case PrimitiveHostExecutorStageKind::PrimitiveRealizationCall:
            return "primitive_realization_call";
        case PrimitiveHostExecutorStageKind::PrimitiveRollbackCall:
            return "primitive_rollback_call";
        case PrimitiveHostExecutorStageKind::FinalVerificationCall:
            return "final_verification_call";
    }

    return "unknown";
}

} // namespace spreadsheetengine::compat::libreoffice::substrateprimitivehostexecutor

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
