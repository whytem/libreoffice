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
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice::substrateprimitiveexecution
{

enum class PrimitiveExecutionObservationKind : sal_uInt8
{
    Exact,
    NormalizedEquivalent,
    OrderingOnly,
    HiddenHostPrimitiveExecutionOrchestration,
    MissingPrimitiveExecutionInputs,
    QueueOrStateMismatch,
    OutOfContract
};

struct PrimitiveExecutionObservation
{
    PrimitiveExecutionObservationKind meKind = PrimitiveExecutionObservationKind::OutOfContract;
    api::String maReason;
    bool mbPrimitiveExecutionApplied = false;
    bool mbRolledBack = false;
    bool mbRawDocumentMutationExact = false;
    bool mbPrimitiveRealizationExact = false;
    bool mbPrimitiveRollbackExact = false;
    bool mbFinalVerificationExact = false;
    bool mbFinalVerificationNormalizedEquivalent = false;
    bool mbQueueExact = false;
    bool mbComputationalFullMatch = false;
    bool mbGraphFullMatch = false;

    [[nodiscard]] constexpr bool operator==(const PrimitiveExecutionObservation& rOther) const
        = default;
};

enum class PrimitiveExecutionStageKind : sal_uInt8
{
    RawDocumentMutation,
    PrimitiveRealization,
    PrimitiveRollback,
    FinalVerification
};

struct AdmittedPrimitiveExecutionPlan
{
    substraterawmutation::AdmittedRawDocumentMutationRecord maRawDocumentMutation;
    std::optional<substrateobjectrealization::AdmittedPrimitiveRealizationRecord>
        moPrimitiveRealization;
    std::optional<substraterollback::AdmittedPrimitiveRollbackRecord> moPrimitiveRollback;
    bool mbRequiresFinalVerification = false;
    bool mbRolledBack = false;
    std::array<PrimitiveExecutionStageKind, 4> maStageOrder{
        PrimitiveExecutionStageKind::RawDocumentMutation,
        PrimitiveExecutionStageKind::RawDocumentMutation,
        PrimitiveExecutionStageKind::RawDocumentMutation,
        PrimitiveExecutionStageKind::RawDocumentMutation,
    };
    sal_uInt8 mnStageCount = 0;
    bool mbUsesPrimitiveExecution = true;

    [[nodiscard]] constexpr bool operator==(const AdmittedPrimitiveExecutionPlan& rOther) const
        = default;
};

enum class PrimitiveExecutionPlanBuildResultKind : sal_uInt8
{
    Built,
    RejectedOutOfContract
};

struct PrimitiveExecutionPlanBuildResult
{
    PrimitiveExecutionPlanBuildResultKind meKind
        = PrimitiveExecutionPlanBuildResultKind::RejectedOutOfContract;
    api::String maReason;
    AdmittedPrimitiveExecutionPlan maPlan;
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
    const AdmittedPrimitiveExecutionPlan& rPlan, PrimitiveExecutionStageKind eStage)
{
    return std::find(rPlan.maStageOrder.begin(), rPlan.maStageOrder.begin() + rPlan.mnStageCount, eStage)
           != rPlan.maStageOrder.begin() + rPlan.mnStageCount;
}

[[nodiscard]] inline PrimitiveExecutionPlanBuildResult buildAdmittedPrimitiveExecutionPlan(
    const substraterawmutation::AdmittedRawDocumentMutationRecord& rRawDocumentMutation,
    const std::optional<substrateobjectrealization::AdmittedPrimitiveRealizationRecord>&
        oPrimitiveRealization,
    const std::optional<substraterollback::AdmittedPrimitiveRollbackRecord>& oPrimitiveRollback,
    bool bRequireFinalVerification)
{
    PrimitiveExecutionPlanBuildResult aResult;
    aResult.maPlan.maRawDocumentMutation = rRawDocumentMutation;
    aResult.maPlan.moPrimitiveRealization = oPrimitiveRealization;
    aResult.maPlan.moPrimitiveRollback = oPrimitiveRollback;
    aResult.maPlan.mbRequiresFinalVerification = bRequireFinalVerification;
    aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++]
        = PrimitiveExecutionStageKind::RawDocumentMutation;

    if (oPrimitiveRealization && oPrimitiveRollback)
    {
        aResult.maReason = u"conflicting_primitive_execution_records";
        return aResult;
    }

    if (!oPrimitiveRealization && !oPrimitiveRollback)
    {
        aResult.maReason = u"missing_primitive_execution_record";
        return aResult;
    }

    if (oPrimitiveRealization)
    {
        aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++]
            = PrimitiveExecutionStageKind::PrimitiveRealization;
    }
    else
    {
        aResult.maPlan.mbRolledBack = true;
        aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++]
            = PrimitiveExecutionStageKind::PrimitiveRollback;
    }

    if (bRequireFinalVerification)
    {
        aResult.maPlan.maStageOrder[aResult.maPlan.mnStageCount++]
            = PrimitiveExecutionStageKind::FinalVerification;
    }

    aResult.meKind = PrimitiveExecutionPlanBuildResultKind::Built;
    return aResult;
}

[[nodiscard]] inline PrimitiveExecutionObservation classifyPrimitiveExecutionObservation(
    bool bPrimitiveExecutionApplied,
    const std::optional<substraterawmutation::RawDocumentMutationObservation>&
        oRawDocumentMutation,
    const std::optional<substrateobjectrealization::PrimitiveRealizationObservation>&
        oPrimitiveRealization,
    const std::optional<substraterollback::PrimitiveRollbackObservation>& oPrimitiveRollback,
    const std::optional<substratefinalverification::FinalVerificationObservation>&
        oFinalVerification,
    api::StringView rReasonIfOutOfContract = {})
{
    PrimitiveExecutionObservation aObservation;
    aObservation.mbPrimitiveExecutionApplied = bPrimitiveExecutionApplied;
    aObservation.mbRolledBack = oPrimitiveRollback.has_value();

    if (!bPrimitiveExecutionApplied || !oRawDocumentMutation)
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::OutOfContract;
        aObservation.maReason = rReasonIfOutOfContract;
        return aObservation;
    }

    if (oPrimitiveRealization.has_value() == oPrimitiveRollback.has_value())
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::OutOfContract;
        aObservation.maReason = u"missing_or_conflicting_primitive_execution_branch";
        return aObservation;
    }

    if (!oFinalVerification)
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::OutOfContract;
        aObservation.maReason = u"missing_final_verification_observation";
        return aObservation;
    }

    aObservation.mbRawDocumentMutationExact
        = oRawDocumentMutation->meKind
          == substraterawmutation::RawDocumentMutationObservationKind::Exact;
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
        aObservation.meKind = PrimitiveExecutionObservationKind::OutOfContract;
        aObservation.maReason = oRawDocumentMutation->maReason;
        return aObservation;
    }

    if (oPrimitiveRealization
        && oPrimitiveRealization->meKind
               == substrateobjectrealization::PrimitiveRealizationObservationKind::OutOfContract)
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::OutOfContract;
        aObservation.maReason = oPrimitiveRealization->maReason;
        return aObservation;
    }

    if (oPrimitiveRollback
        && oPrimitiveRollback->meKind
               == substraterollback::PrimitiveRollbackObservationKind::OutOfContract)
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::OutOfContract;
        aObservation.maReason = oPrimitiveRollback->maReason;
        return aObservation;
    }

    if (oFinalVerification->meKind
        == substratefinalverification::FinalVerificationObservationKind::OutOfContract)
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::OutOfContract;
        aObservation.maReason = oFinalVerification->maReason;
        return aObservation;
    }

    if (detail::isHiddenHostRawDocumentMutation(oRawDocumentMutation->meKind)
        || (oPrimitiveRealization
            && detail::isHiddenHostPrimitiveRealization(oPrimitiveRealization->meKind))
        || (oPrimitiveRollback
            && detail::isHiddenHostPrimitiveRollback(oPrimitiveRollback->meKind))
        || detail::isHiddenHostFinalVerification(oFinalVerification->meKind))
    {
        aObservation.meKind
            = PrimitiveExecutionObservationKind::HiddenHostPrimitiveExecutionOrchestration;
        aObservation.maReason = u"hidden_host_primitive_execution_orchestration";
        return aObservation;
    }

    if (detail::isMissingRawDocumentMutationInput(oRawDocumentMutation->meKind)
        || (oPrimitiveRealization
            && detail::isMissingPrimitiveRealizationInput(oPrimitiveRealization->meKind))
        || (oPrimitiveRollback
            && detail::isMissingPrimitiveRollbackInput(oPrimitiveRollback->meKind))
        || detail::isMissingFinalVerificationInput(oFinalVerification->meKind))
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::MissingPrimitiveExecutionInputs;
        aObservation.maReason = u"missing_primitive_execution_inputs";
        return aObservation;
    }

    if (oRawDocumentMutation->meKind
            == substraterawmutation::RawDocumentMutationObservationKind::QueueOrStateMismatch
        || (oPrimitiveRealization
            && oPrimitiveRealization->meKind
                   == substrateobjectrealization::PrimitiveRealizationObservationKind::
                       QueueOrStateMismatch)
        || (oPrimitiveRollback
            && oPrimitiveRollback->meKind
                   == substraterollback::PrimitiveRollbackObservationKind::QueueOrStateMismatch)
        || oFinalVerification->meKind
               == substratefinalverification::FinalVerificationObservationKind::QueueOrStateMismatch)
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::QueueOrStateMismatch;
        aObservation.maReason = u"primitive_execution_queue_or_state_mismatch";
        return aObservation;
    }

    if (oRawDocumentMutation->meKind
            == substraterawmutation::RawDocumentMutationObservationKind::OrderingOnly
        || (oPrimitiveRealization
            && oPrimitiveRealization->meKind
                   == substrateobjectrealization::PrimitiveRealizationObservationKind::OrderingOnly)
        || (oPrimitiveRollback
            && oPrimitiveRollback->meKind
                   == substraterollback::PrimitiveRollbackObservationKind::OrderingOnly)
        || oFinalVerification->meKind
               == substratefinalverification::FinalVerificationObservationKind::OrderingOnly)
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::OrderingOnly;
        aObservation.maReason = u"primitive_execution_ordering_only";
        return aObservation;
    }

    if (oFinalVerification->meKind
        == substratefinalverification::FinalVerificationObservationKind::NormalizedEquivalent)
    {
        aObservation.meKind = PrimitiveExecutionObservationKind::NormalizedEquivalent;
        aObservation.maReason = u"primitive_execution_normalized_equivalent";
        return aObservation;
    }

    aObservation.meKind = PrimitiveExecutionObservationKind::Exact;
    return aObservation;
}

[[nodiscard]] inline const char* toString(PrimitiveExecutionObservationKind eKind)
{
    switch (eKind)
    {
        case PrimitiveExecutionObservationKind::Exact:
            return "exact";
        case PrimitiveExecutionObservationKind::NormalizedEquivalent:
            return "normalized_equivalent";
        case PrimitiveExecutionObservationKind::OrderingOnly:
            return "ordering_only";
        case PrimitiveExecutionObservationKind::HiddenHostPrimitiveExecutionOrchestration:
            return "hidden_host_primitive_execution_orchestration";
        case PrimitiveExecutionObservationKind::MissingPrimitiveExecutionInputs:
            return "missing_primitive_execution_inputs";
        case PrimitiveExecutionObservationKind::QueueOrStateMismatch:
            return "queue_or_state_mismatch";
        case PrimitiveExecutionObservationKind::OutOfContract:
            return "out_of_contract";
    }

    return "unknown";
}

[[nodiscard]] inline const char* toString(PrimitiveExecutionStageKind eKind)
{
    switch (eKind)
    {
        case PrimitiveExecutionStageKind::RawDocumentMutation:
            return "raw_document_mutation";
        case PrimitiveExecutionStageKind::PrimitiveRealization:
            return "primitive_realization";
        case PrimitiveExecutionStageKind::PrimitiveRollback:
            return "primitive_rollback";
        case PrimitiveExecutionStageKind::FinalVerification:
            return "final_verification";
    }

    return "unknown";
}

} // namespace spreadsheetengine::compat::libreoffice::substrateprimitiveexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
