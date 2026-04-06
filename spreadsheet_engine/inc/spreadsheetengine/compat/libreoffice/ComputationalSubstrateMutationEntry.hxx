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

#include <spreadsheetengine/compat/libreoffice/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateLiveApply.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/substrate/MutationEntry.hxx>

namespace spreadsheetengine::compat::libreoffice::substratemutationentry
{

enum class MutationEntryResultKind : sal_uInt8
{
    Disabled,
    RejectedOutOfContract,
    RejectedDirtyBaseline,
    RolledBackVerificationFailure,
    RepairDetected,
    Applied,
    AppliedNormalizedEquivalent
};

struct MutationEntryResult
{
    MutationEntryResultKind meKind = MutationEntryResultKind::Disabled;
    spreadsheetengine::detail::substrate::MutationEntryTransition maTransition;
    std::optional<recalcshadow::ShadowComparison> moQueueComparison;
    std::optional<spreadsheetengine::detail::substrate::ComputationalShadowComparison>
        moComputationalComparison;
    std::optional<spreadsheetengine::detail::substrate::DependencyGraphShadowComparison>
        moGraphComparison;
    std::optional<spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison>
        moIrComparison;
    std::optional<spreadsheetengine::detail::substrate::BroadcasterCanonicalizationComparison>
        moBroadcasterCanonicalization;
    std::optional<substrateobjectrealization::ObjectRealizationObservation>
        moObjectRealizationObservation;
    std::optional<substrateobjectrealization::AdmittedPrimitiveRealizationRecord>
        moPrimitiveRealizationRecord;
    std::optional<substrateobjectrealization::PrimitiveRealizationObservation>
        moPrimitiveRealizationObservation;
    std::optional<substraterollback::RollbackObservation> moRollbackObservation;
    std::optional<substraterollback::AdmittedPrimitiveRollbackRecord> moPrimitiveRollbackRecord;
    std::optional<substraterollback::PrimitiveRollbackObservation>
        moPrimitiveRollbackObservation;
    std::optional<substraterawmutation::AdmittedRawMutationRecord> moRawMutationRecord;
    std::optional<substraterawmutation::AdmittedRawDocumentMutationRecord>
        moRawDocumentMutationRecord;
    std::optional<substraterawmutation::RawMutationObservation> moRawMutationObservation;
    std::optional<substraterawmutation::RawDocumentMutationObservation>
        moRawDocumentMutationObservation;
    std::optional<substrateliveapply::AdmittedLiveApplyPlan> moLiveApplyPlan;
    std::optional<substrateliveapply::LiveApplyObservation> moLiveApplyObservation;
};

namespace detail
{

struct RealizationResult
{
    bool mbApplied = false;
    api::String maReason;
    substrateobjectrealization::PrimitiveRealizationApplyResult maPrimitiveRealization;
};

[[nodiscard]] inline bool isRuntimeEnabled(const ScDocument& rDoc)
{
    return substraterollout::isSurfaceEnabled(rDoc, substraterollout::RolloutSurface::MutationEntry);
}

[[nodiscard]] inline MutationEntryResultKind mapAuthorityKind(
    substrateauthority::PilotResultKind eKind)
{
    switch (eKind)
    {
        case substrateauthority::PilotResultKind::RejectedOutOfContract:
            return MutationEntryResultKind::RejectedOutOfContract;
        case substrateauthority::PilotResultKind::RejectedDirtyBaseline:
            return MutationEntryResultKind::RejectedDirtyBaseline;
        case substrateauthority::PilotResultKind::RolledBackVerificationFailure:
            return MutationEntryResultKind::RolledBackVerificationFailure;
        case substrateauthority::PilotResultKind::Applied:
            return MutationEntryResultKind::Applied;
        case substrateauthority::PilotResultKind::AppliedNormalizedEquivalent:
            return MutationEntryResultKind::AppliedNormalizedEquivalent;
        case substrateauthority::PilotResultKind::Disabled:
            return MutationEntryResultKind::Disabled;
    }

    return MutationEntryResultKind::RolledBackVerificationFailure;
}

[[nodiscard]] inline MutationEntryResultKind mapLifecycleKind(
    substratelifecycle::LifecycleResultKind eKind)
{
    switch (eKind)
    {
        case substratelifecycle::LifecycleResultKind::RejectedOutOfContract:
            return MutationEntryResultKind::RejectedOutOfContract;
        case substratelifecycle::LifecycleResultKind::RejectedDirtyBaseline:
            return MutationEntryResultKind::RejectedDirtyBaseline;
        case substratelifecycle::LifecycleResultKind::RolledBackVerificationFailure:
            return MutationEntryResultKind::RolledBackVerificationFailure;
        case substratelifecycle::LifecycleResultKind::RepairDetected:
            return MutationEntryResultKind::RepairDetected;
        case substratelifecycle::LifecycleResultKind::Applied:
            return MutationEntryResultKind::Applied;
        case substratelifecycle::LifecycleResultKind::AppliedNormalizedEquivalent:
            return MutationEntryResultKind::AppliedNormalizedEquivalent;
        case substratelifecycle::LifecycleResultKind::Disabled:
            return MutationEntryResultKind::Disabled;
    }

    return MutationEntryResultKind::RolledBackVerificationFailure;
}

[[nodiscard]] inline MutationEntryResultKind mapStructuralKind(
    substratestructural::StructuralResultKind eKind)
{
    switch (eKind)
    {
        case substratestructural::StructuralResultKind::RejectedOutOfContract:
            return MutationEntryResultKind::RejectedOutOfContract;
        case substratestructural::StructuralResultKind::RejectedDirtyBaseline:
            return MutationEntryResultKind::RejectedDirtyBaseline;
        case substratestructural::StructuralResultKind::RolledBackVerificationFailure:
            return MutationEntryResultKind::RolledBackVerificationFailure;
        case substratestructural::StructuralResultKind::RepairDetected:
            return MutationEntryResultKind::RepairDetected;
        case substratestructural::StructuralResultKind::Applied:
            return MutationEntryResultKind::Applied;
        case substratestructural::StructuralResultKind::AppliedNormalizedEquivalent:
            return MutationEntryResultKind::AppliedNormalizedEquivalent;
        case substratestructural::StructuralResultKind::Disabled:
            return MutationEntryResultKind::Disabled;
    }

    return MutationEntryResultKind::RolledBackVerificationFailure;
}

[[nodiscard]] inline std::optional<MutationEntryResultKind> classifyImmediateResultKind(
    const spreadsheetengine::detail::substrate::MutationEntryTransition& rTransition)
{
    switch (rTransition.mePath)
    {
        case spreadsheetengine::detail::substrate::MutationEntryPath::Authority:
            if (!rTransition.moAuthorityTransition)
                return MutationEntryResultKind::RejectedOutOfContract;

            switch (rTransition.moAuthorityTransition->meVerdict)
            {
                case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::RejectedOutOfContract:
                    return MutationEntryResultKind::RejectedOutOfContract;
                case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::RejectedDirtyBaseline:
                    return MutationEntryResultKind::RejectedDirtyBaseline;
                case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::Applicable:
                case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::Applied:
                case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::NormalizedEquivalent:
                case spreadsheetengine::detail::substrate::AuthorityPilotVerdict::RolledBack:
                    return std::nullopt;
            }
            break;
        case spreadsheetengine::detail::substrate::MutationEntryPath::Lifecycle:
            if (!rTransition.moLifecycleTransition)
                return MutationEntryResultKind::RejectedOutOfContract;

            switch (rTransition.moLifecycleTransition->meVerdict)
            {
                case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RejectedOutOfContract:
                    return MutationEntryResultKind::RejectedOutOfContract;
                case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RejectedDirtyBaseline:
                    return MutationEntryResultKind::RejectedDirtyBaseline;
                case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RepairDetected:
                    return MutationEntryResultKind::RepairDetected;
                case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RolledBack:
                    return MutationEntryResultKind::RolledBackVerificationFailure;
                case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable:
                case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applied:
                case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::NormalizedEquivalent:
                    return std::nullopt;
            }
            break;
        case spreadsheetengine::detail::substrate::MutationEntryPath::Structural:
            if (!rTransition.moStructuralTransition)
                return MutationEntryResultKind::RejectedOutOfContract;

            switch (rTransition.moStructuralTransition->meVerdict)
            {
                case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RejectedOutOfContract:
                    return MutationEntryResultKind::RejectedOutOfContract;
                case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RejectedDirtyBaseline:
                    return MutationEntryResultKind::RejectedDirtyBaseline;
                case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RepairDetected:
                    return MutationEntryResultKind::RepairDetected;
                case spreadsheetengine::detail::substrate::StructuralPilotVerdict::RolledBack:
                    return MutationEntryResultKind::RolledBackVerificationFailure;
                case spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applicable:
                case spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applied:
                case spreadsheetengine::detail::substrate::StructuralPilotVerdict::NormalizedEquivalent:
                    return std::nullopt;
            }
            break;
    }

    return MutationEntryResultKind::RejectedOutOfContract;
}

[[nodiscard]] inline MutationEntryResultKind classifyVerifiedMutationEntryResult(
    const MutationEntryResult& rResult)
{
    switch (rResult.maTransition.mePath)
    {
        case spreadsheetengine::detail::substrate::MutationEntryPath::Authority:
        {
            if (!rResult.maTransition.moAuthorityTransition)
                return MutationEntryResultKind::RolledBackVerificationFailure;

            substrateauthority::PilotResult aAuthorityResult;
            aAuthorityResult.maTransition = *rResult.maTransition.moAuthorityTransition;
            aAuthorityResult.moQueueComparison = rResult.moQueueComparison;
            aAuthorityResult.moGraphComparison = rResult.moGraphComparison;
            aAuthorityResult.moIrComparison = rResult.moIrComparison;
            return mapAuthorityKind(
                substrateauthority::detail::classifyVerifiedPilotResult(aAuthorityResult));
        }
        case spreadsheetengine::detail::substrate::MutationEntryPath::Lifecycle:
        {
            if (!rResult.maTransition.moLifecycleTransition)
                return MutationEntryResultKind::RolledBackVerificationFailure;

            substratelifecycle::LifecycleResult aLifecycleResult;
            aLifecycleResult.maTransition = *rResult.maTransition.moLifecycleTransition;
            aLifecycleResult.moQueueComparison = rResult.moQueueComparison;
            aLifecycleResult.moComputationalComparison = rResult.moComputationalComparison;
            aLifecycleResult.moGraphComparison = rResult.moGraphComparison;
            aLifecycleResult.moIrComparison = rResult.moIrComparison;
            return mapLifecycleKind(
                substratelifecycle::detail::classifyVerifiedLifecycleResult(aLifecycleResult));
        }
        case spreadsheetengine::detail::substrate::MutationEntryPath::Structural:
        {
            if (!rResult.maTransition.moStructuralTransition)
                return MutationEntryResultKind::RolledBackVerificationFailure;

            substratestructural::StructuralResult aStructuralResult;
            aStructuralResult.maTransition = *rResult.maTransition.moStructuralTransition;
            aStructuralResult.moQueueComparison = rResult.moQueueComparison;
            aStructuralResult.moComputationalComparison = rResult.moComputationalComparison;
            aStructuralResult.moGraphComparison = rResult.moGraphComparison;
            aStructuralResult.moIrComparison = rResult.moIrComparison;
            return mapStructuralKind(
                substratestructural::detail::classifyVerifiedStructuralResult(aStructuralResult));
        }
    }

    return MutationEntryResultKind::RolledBackVerificationFailure;
}

[[nodiscard]] inline substraterawmutation::RawMutationObservation observeRawMutationApply(
    const std::optional<substrateobjectrealization::ObjectRealizationObservation>& oObjectRealization)
{
    return substraterawmutation::classifyRawMutationObservation(
        true, false, oObjectRealization, std::nullopt);
}

[[nodiscard]] inline substraterawmutation::RawDocumentMutationObservation
observeRawDocumentMutationApply(
    const std::optional<substraterawmutation::RawMutationObservation>& oRawMutation)
{
    return substraterawmutation::classifyRawDocumentMutationObservation(
        true, oRawMutation);
}

[[nodiscard]] inline substraterawmutation::RawMutationObservation observeRawMutationRollback(
    const std::optional<substraterollback::RollbackObservation>& oRollback)
{
    return substraterawmutation::classifyRawMutationObservation(
        true, true, std::nullopt, oRollback);
}

[[nodiscard]] inline substraterawmutation::RawDocumentMutationObservation
observeRawDocumentMutationRollback(
    const std::optional<substraterawmutation::RawMutationObservation>& oRawMutation)
{
    return substraterawmutation::classifyRawDocumentMutationObservation(
        true, oRawMutation);
}

[[nodiscard]] inline substrateobjectrealization::PrimitiveRealizationObservation
observePrimitiveRealizationApply(
    const std::optional<substrateobjectrealization::ObjectRealizationObservation>& oObjectRealization)
{
    return substrateobjectrealization::classifyPrimitiveRealizationObservation(
        true, oObjectRealization);
}

[[nodiscard]] inline substraterollback::PrimitiveRollbackObservation
observePrimitiveRollbackApply(
    const std::optional<substraterollback::RollbackObservation>& oRollback)
{
    return substraterollback::classifyPrimitiveRollbackObservation(
        true, oRollback);
}

[[nodiscard]] inline RealizationResult realizeObjectRealization(
    ScDocument& rDoc,
    const substrateobjectrealization::AdmittedPrimitiveRealizationRecord& rObjectRealization)
{
    RealizationResult aResult;
    aResult.maPrimitiveRealization
        = substrateobjectrealization::applyAdmittedPrimitiveRealizationRecord(
            rDoc, rObjectRealization);
    if (aResult.maPrimitiveRealization.meKind
        != substrateobjectrealization::PrimitiveRealizationApplyResultKind::Applied)
    {
        aResult.maReason = aResult.maPrimitiveRealization.maReason;
        return aResult;
    }

    aResult.mbApplied = true;
    return aResult;
}

inline void rollbackToBeforeState(ScDocument& rDoc,
    const substraterollback::AdmittedPrimitiveRollbackRecord& rRollback,
    substraterollback::PrimitiveRollbackApplyResult& rResult)
{
    rResult = substraterollback::applyAdmittedPrimitiveRollbackRecord(rDoc, rRollback);
}

[[nodiscard]] inline substraterollback::RollbackObservation observeRolledBackState(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::MutableComputationalSubstrateState& rBeforeState,
    const recalcqueue::FormulaStateSnapshot& rBeforeFormulaState,
    const substraterollback::PrimitiveRollbackApplyResult& rRollback)
{
    const CalcWorkbookFacade aRollbackFacade(rDoc, rBeforeState.maShadow.maSnapshot.mnGeneration);
    const auto aRollbackObservationState = makeComputationalObservationState(
        substrateobs::collectLiveComputationalState(rDoc));
    const auto aQueueComparison
        = substraterollback::compareRollbackQueueToDocument(rBeforeFormulaState, rDoc);
    const auto aComputationalComparison = spreadsheetengine::detail::substrate::compareComputationalShadow(
        rBeforeState.maShadow, aRollbackFacade, aRollbackObservationState);
    const auto aRollbackShadow = spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow(
        aRollbackFacade, aRollbackObservationState);
    const auto aGraphComparison = spreadsheetengine::detail::substrate::compareDependencyGraphShadow(
        rBeforeState.maGraphShadow, aRollbackShadow, aRollbackObservationState);
    const auto aBroadcasterComparison
        = spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization(
            rBeforeState.maShadow, aRollbackObservationState);
    return substraterollback::classifyRollbackObservation(
        rRollback.maRollback, aQueueComparison, aComputationalComparison, aGraphComparison,
        aBroadcasterComparison);
}

[[nodiscard]] inline std::optional<substrateliveapply::AdmittedLiveApplyPlan>
buildLiveApplyPlanOrReject(
    MutationEntryResult& rResult,
    const substraterawmutation::AdmittedRawMutationRecord& rRawMutationRecord,
    const std::optional<substrateobjectrealization::AdmittedObjectRealization>& oObjectRealization,
    const substraterollback::AdmittedRollbackRecord& rRollback, bool bRolledBack)
{
    const auto aPlan = substrateliveapply::buildAdmittedLiveApplyPlan(
        rRawMutationRecord, oObjectRealization, rRollback, bRolledBack);
    if (aPlan.meKind != substrateliveapply::LiveApplyPlanBuildResultKind::Built)
    {
        rResult.meKind = MutationEntryResultKind::RejectedOutOfContract;
        rResult.maTransition.maReason = aPlan.maReason;
        return std::nullopt;
    }

    rResult.moLiveApplyPlan = aPlan.maPlan;
    return aPlan.maPlan;
}

} // namespace detail

class ScopedComputationalMutationEntry
{
    bool mbCaptured = false;
    bool mbCleanBaseline = false;
    spreadsheetengine::detail::substrate::ComputationalWorkbookShadow maComputationalShadow;
    spreadsheetengine::detail::substrate::DependencyGraphShadow maGraphShadow;
    spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow maIrShadow;
    recalcqueue::FormulaStateSnapshot maFormulaState;

public:
    ScopedComputationalMutationEntry() = default;

    explicit ScopedComputationalMutationEntry(ScDocument& rDoc, bool bCapture)
    {
        if (!bCapture)
            return;

        const CalcWorkbookFacade aFacade(rDoc, 0);
        maComputationalShadow = buildComputationalWorkbookShadow(aFacade, rDoc);
        maGraphShadow = buildDependencyGraphShadow(aFacade, rDoc);
        maIrShadow = buildExecutionIrWorkbookShadow(aFacade, rDoc);
        maFormulaState = recalcqueue::captureFormulaState(rDoc);
        mbCleanBaseline = recalcqueue::isCleanFormulaState(maFormulaState);
        mbCaptured = true;
    }

    [[nodiscard]] static ScopedComputationalMutationEntry captureIfRuntimeEnabled(ScDocument& rDoc)
    {
        return ScopedComputationalMutationEntry(rDoc, detail::isRuntimeEnabled(rDoc));
    }

    [[nodiscard]] bool isCaptured() const { return mbCaptured; }
    [[nodiscard]] bool canApplyMutationEntry() const { return mbCaptured && mbCleanBaseline; }

    [[nodiscard]] std::optional<MutationEntryResult> apply(
        ScDocument& rDoc,
        const spreadsheetengine::detail::substrate::MutationEntryRequest& rRequest) const
    {
        if (!mbCaptured)
            return std::nullopt;

        MutationEntryResult aResult;
        const auto aBeforeMutableState
            = spreadsheetengine::detail::substrate::bootstrapMutableComputationalSubstrateState(
                maComputationalShadow);
        const auto aBeforeRollback
            = substraterollback::buildAdmittedRollbackRecord(aBeforeMutableState, maFormulaState);
        const auto aPrimitiveBeforeRollback
            = substraterollback::buildAdmittedPrimitiveRollbackRecord(aBeforeRollback);
        auto aMutableState = aBeforeMutableState;

        const auto aRawMutationRecord = substraterawmutation::buildAdmittedRawMutationRecord(rRequest);
        if (aRawMutationRecord.meKind
            != substraterawmutation::RawMutationRecordResultKind::Built)
        {
            aResult.meKind = MutationEntryResultKind::RejectedOutOfContract;
            aResult.maTransition.maReason = aRawMutationRecord.maReason;
            aResult.moRawMutationObservation = substraterawmutation::classifyRawMutationObservation(
                false, false, std::nullopt, std::nullopt, aRawMutationRecord.maReason);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, std::nullopt);
            return aResult;
        }
        aResult.moRawMutationRecord = aRawMutationRecord.maRecord;

        const auto aRawDocumentMutationRecord
            = substraterawmutation::buildAdmittedRawDocumentMutationRecord(aRawMutationRecord.maRecord);
        if (aRawDocumentMutationRecord.meKind
            != substraterawmutation::RawDocumentMutationRecordResultKind::Built)
        {
            aResult.meKind = MutationEntryResultKind::RejectedOutOfContract;
            aResult.maTransition.maReason = aRawDocumentMutationRecord.maReason;
            aResult.moRawMutationObservation = substraterawmutation::classifyRawMutationObservation(
                false, false, std::nullopt, std::nullopt, aRawDocumentMutationRecord.maReason);
            aResult.moRawDocumentMutationObservation
                = substraterawmutation::classifyRawDocumentMutationObservation(
                    false, std::nullopt, aRawDocumentMutationRecord.maReason);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, std::nullopt);
            return aResult;
        }
        aResult.moRawDocumentMutationRecord = aRawDocumentMutationRecord.maRecord;

        if (aPrimitiveBeforeRollback.meKind
            != substraterollback::PrimitiveRollbackRecordResultKind::Built)
        {
            aResult.meKind = MutationEntryResultKind::RejectedOutOfContract;
            aResult.maTransition.maReason = aPrimitiveBeforeRollback.maReason;
            aResult.moRawMutationObservation = substraterawmutation::classifyRawMutationObservation(
                false, false, std::nullopt, std::nullopt, aPrimitiveBeforeRollback.maReason);
            aResult.moRawDocumentMutationObservation
                = substraterawmutation::classifyRawDocumentMutationObservation(
                    false, std::nullopt, aPrimitiveBeforeRollback.maReason);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, std::nullopt);
            return aResult;
        }

        const auto aRawApply = substraterawmutation::applyAdmittedRawDocumentMutationRecord(
            rDoc, aRawDocumentMutationRecord.maRecord);
        if (aRawApply.meKind != substraterawmutation::RawDocumentMutationApplyResultKind::Applied)
        {
            aResult.meKind = MutationEntryResultKind::RejectedOutOfContract;
            aResult.maTransition.maReason = aRawApply.maReason;
            aResult.moRawMutationObservation = substraterawmutation::classifyRawMutationObservation(
                false, false, std::nullopt, std::nullopt, aRawApply.maReason);
            aResult.moRawDocumentMutationObservation
                = substraterawmutation::classifyRawDocumentMutationObservation(
                    false, std::nullopt, aRawApply.maReason);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, std::nullopt);
            return aResult;
        }

        const sal_Int64 nAfterGeneration = maComputationalShadow.maSnapshot.mnGeneration + 1;
        const CalcWorkbookFacade aAfterFacade(rDoc, nAfterGeneration);
        const auto aAfterObservation = makeComputationalObservationState(
            substrateobs::collectLiveComputationalState(rDoc));

        spreadsheetengine::detail::substrate::MutationEntryBuildInput aInput;
        aInput.maComputationalShadow = maComputationalShadow;
        aInput.maGraphShadow = maGraphShadow;
        aInput.maIrShadow = maIrShadow;
        aInput.maRequest = rRequest;
        aInput.moObservedAfterComputationalShadow
            = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
        aInput.moObservedAfterIrShadow
            = buildExecutionIrWorkbookShadow(*aInput.moObservedAfterComputationalShadow, rDoc);
        aInput.mbCleanBaseline = mbCleanBaseline;

        aResult.maTransition = spreadsheetengine::detail::substrate::buildMutationEntryTransition(
            aInput, aAfterFacade, aAfterObservation);

        if (const auto oImmediateKind = detail::classifyImmediateResultKind(aResult.maTransition))
        {
            aResult.meKind = *oImmediateKind;
            [[maybe_unused]] const auto oRolledBackLiveApplyPlan
                = detail::buildLiveApplyPlanOrReject(
                aResult, aRawMutationRecord.maRecord, std::nullopt, aBeforeRollback, true);
            aResult.moPrimitiveRollbackRecord = aPrimitiveBeforeRollback.maRecord;
            substraterollback::PrimitiveRollbackApplyResult aRollback;
            detail::rollbackToBeforeState(rDoc, aPrimitiveBeforeRollback.maRecord, aRollback);
            aResult.moRollbackObservation
                = detail::observeRolledBackState(rDoc, aBeforeMutableState, maFormulaState, aRollback);
            aResult.moPrimitiveRollbackObservation = detail::observePrimitiveRollbackApply(
                aResult.moRollbackObservation);
            aResult.moRawMutationObservation = detail::observeRawMutationRollback(
                aResult.moRollbackObservation);
            aResult.moRawDocumentMutationObservation = detail::observeRawDocumentMutationRollback(
                aResult.moRawMutationObservation);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, aResult.moRollbackObservation);
            return aResult;
        }

        if (!spreadsheetengine::detail::substrate::applyMutableMutationEntryTransition(
                aMutableState, aResult.maTransition))
        {
            aResult.meKind = MutationEntryResultKind::RolledBackVerificationFailure;
            [[maybe_unused]] const auto oRolledBackLiveApplyPlan
                = detail::buildLiveApplyPlanOrReject(
                aResult, aRawMutationRecord.maRecord, std::nullopt, aBeforeRollback, true);
            aResult.moPrimitiveRollbackRecord = aPrimitiveBeforeRollback.maRecord;
            substraterollback::PrimitiveRollbackApplyResult aRollback;
            detail::rollbackToBeforeState(rDoc, aPrimitiveBeforeRollback.maRecord, aRollback);
            aResult.moRollbackObservation
                = detail::observeRolledBackState(rDoc, aBeforeMutableState, maFormulaState, aRollback);
            aResult.moPrimitiveRollbackObservation = detail::observePrimitiveRollbackApply(
                aResult.moRollbackObservation);
            aResult.moRawMutationObservation = detail::observeRawMutationRollback(
                aResult.moRollbackObservation);
            aResult.moRawDocumentMutationObservation = detail::observeRawDocumentMutationRollback(
                aResult.moRawMutationObservation);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, aResult.moRollbackObservation);
            return aResult;
        }

        const auto aObjectRealization
            = substrateobjectrealization::buildAdmittedObjectRealization(aMutableState);
        const auto aPrimitiveRealization
            = substrateobjectrealization::buildAdmittedPrimitiveRealizationRecord(aObjectRealization);
        const auto oAppliedLiveApplyPlan = detail::buildLiveApplyPlanOrReject(
            aResult, aRawMutationRecord.maRecord, aObjectRealization, aBeforeRollback, false);
        if (!oAppliedLiveApplyPlan)
        {
            aResult.moRawMutationObservation = substraterawmutation::classifyRawMutationObservation(
                false, false, std::nullopt, std::nullopt, aResult.maTransition.maReason);
            aResult.moRawDocumentMutationObservation
                = substraterawmutation::classifyRawDocumentMutationObservation(
                    false, std::nullopt, aResult.maTransition.maReason);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, std::nullopt);
            return aResult;
        }

        if (aPrimitiveRealization.meKind
            != substrateobjectrealization::PrimitiveRealizationRecordResultKind::Built)
        {
            aResult.meKind = MutationEntryResultKind::RejectedOutOfContract;
            aResult.maTransition.maReason = aPrimitiveRealization.maReason;
            aResult.moRawMutationObservation = substraterawmutation::classifyRawMutationObservation(
                false, false, std::nullopt, std::nullopt, aPrimitiveRealization.maReason);
            aResult.moRawDocumentMutationObservation
                = substraterawmutation::classifyRawDocumentMutationObservation(
                    false, std::nullopt, aPrimitiveRealization.maReason);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, std::nullopt);
            return aResult;
        }
        aResult.moPrimitiveRealizationRecord = aPrimitiveRealization.maRecord;

        const auto aRealization = detail::realizeObjectRealization(rDoc, aPrimitiveRealization.maRecord);
        if (!aRealization.mbApplied)
        {
            aResult.meKind = MutationEntryResultKind::RolledBackVerificationFailure;
            aResult.maTransition.maReason = aRealization.maReason;
            [[maybe_unused]] const auto oRolledBackLiveApplyPlan
                = detail::buildLiveApplyPlanOrReject(
                aResult, aRawMutationRecord.maRecord, aObjectRealization, aBeforeRollback, true);
            aResult.moPrimitiveRollbackRecord = aPrimitiveBeforeRollback.maRecord;
            substraterollback::PrimitiveRollbackApplyResult aRollback;
            detail::rollbackToBeforeState(rDoc, aPrimitiveBeforeRollback.maRecord, aRollback);
            aResult.moRollbackObservation
                = detail::observeRolledBackState(rDoc, aBeforeMutableState, maFormulaState, aRollback);
            aResult.moPrimitiveRollbackObservation = detail::observePrimitiveRollbackApply(
                aResult.moRollbackObservation);
            aResult.moRawMutationObservation = detail::observeRawMutationRollback(
                aResult.moRollbackObservation);
            aResult.moRawDocumentMutationObservation = detail::observeRawDocumentMutationRollback(
                aResult.moRawMutationObservation);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, aResult.moRollbackObservation);
            return aResult;
        }

        const auto* pPlan
            = spreadsheetengine::detail::substrate::findMutationEntryRecalcPlan(aResult.maTransition);
        const auto* pComputationalAfter
            = spreadsheetengine::detail::substrate::findMutationEntryComputationalAfter(
                aResult.maTransition);
        const auto* pGraphAfter
            = spreadsheetengine::detail::substrate::findMutationEntryGraphAfter(aResult.maTransition);
        const auto* pIrAfter
            = spreadsheetengine::detail::substrate::findMutationEntryIrAfter(aResult.maTransition);
        if (!pPlan || !pComputationalAfter || !pGraphAfter || !pIrAfter)
        {
            aResult.meKind = MutationEntryResultKind::RolledBackVerificationFailure;
            [[maybe_unused]] const auto oRolledBackLiveApplyPlan
                = detail::buildLiveApplyPlanOrReject(
                aResult, aRawMutationRecord.maRecord, aObjectRealization, aBeforeRollback, true);
            aResult.moPrimitiveRollbackRecord = aPrimitiveBeforeRollback.maRecord;
            substraterollback::PrimitiveRollbackApplyResult aRollback;
            detail::rollbackToBeforeState(rDoc, aPrimitiveBeforeRollback.maRecord, aRollback);
            aResult.moRollbackObservation
                = detail::observeRolledBackState(rDoc, aBeforeMutableState, maFormulaState, aRollback);
            aResult.moPrimitiveRollbackObservation = detail::observePrimitiveRollbackApply(
                aResult.moRollbackObservation);
            aResult.moRawMutationObservation = detail::observeRawMutationRollback(
                aResult.moRollbackObservation);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, aResult.moRollbackObservation);
            return aResult;
        }

        const CalcWorkbookFacade aVerifiedFacade(rDoc, nAfterGeneration);
        const auto aLiveObservation = makeComputationalObservationState(
            substrateobs::collectLiveComputationalState(rDoc));
        const auto aLiveComputationalShadow
            = spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow(
                aVerifiedFacade, aLiveObservation);

        aResult.moQueueComparison
            = recalcshadow::detail::comparePlanToDocument(*pPlan, aVerifiedFacade, rDoc);
        aResult.moComputationalComparison
            = spreadsheetengine::detail::substrate::compareComputationalShadow(
                *pComputationalAfter, aVerifiedFacade, aLiveObservation);
        aResult.moGraphComparison
            = spreadsheetengine::detail::substrate::compareDependencyGraphShadow(
                *pGraphAfter, aLiveComputationalShadow, aLiveObservation);
        aResult.moIrComparison
            = spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow(
                *pIrAfter, buildExecutionIrWorkbookShadow(aLiveComputationalShadow, rDoc));
        aResult.moBroadcasterCanonicalization
            = spreadsheetengine::detail::substrate::detail::compareBroadcasterCanonicalization(
                *pComputationalAfter, aLiveObservation);
        aResult.moObjectRealizationObservation
            = substrateobjectrealization::classifyObjectRealizationObservation(
                aRealization.maPrimitiveRealization.maObjectRealization, *aResult.moQueueComparison,
                *aResult.moComputationalComparison, *aResult.moGraphComparison,
                *aResult.moBroadcasterCanonicalization);
        aResult.moPrimitiveRealizationObservation = detail::observePrimitiveRealizationApply(
            aResult.moObjectRealizationObservation);
        aResult.moRawMutationObservation
            = detail::observeRawMutationApply(aResult.moObjectRealizationObservation);
        aResult.moRawDocumentMutationObservation = detail::observeRawDocumentMutationApply(
            aResult.moRawMutationObservation);
        aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
            *aResult.moRawMutationObservation, aResult.moObjectRealizationObservation, std::nullopt);

        aResult.meKind = detail::classifyVerifiedMutationEntryResult(aResult);
        if (aResult.meKind == MutationEntryResultKind::RolledBackVerificationFailure
            || aResult.meKind == MutationEntryResultKind::RepairDetected)
        {
            [[maybe_unused]] const auto oRolledBackLiveApplyPlan
                = detail::buildLiveApplyPlanOrReject(
                aResult, aRawMutationRecord.maRecord, aObjectRealization, aBeforeRollback, true);
            aResult.moPrimitiveRollbackRecord = aPrimitiveBeforeRollback.maRecord;
            substraterollback::PrimitiveRollbackApplyResult aRollback;
            detail::rollbackToBeforeState(rDoc, aPrimitiveBeforeRollback.maRecord, aRollback);
            aResult.moRollbackObservation
                = detail::observeRolledBackState(rDoc, aBeforeMutableState, maFormulaState, aRollback);
            aResult.moPrimitiveRollbackObservation = detail::observePrimitiveRollbackApply(
                aResult.moRollbackObservation);
            aResult.moRawMutationObservation = detail::observeRawMutationRollback(
                aResult.moRollbackObservation);
            aResult.moRawDocumentMutationObservation = detail::observeRawDocumentMutationRollback(
                aResult.moRawMutationObservation);
            aResult.moLiveApplyObservation = substrateliveapply::classifyLiveApplyObservation(
                *aResult.moRawMutationObservation, std::nullopt, aResult.moRollbackObservation);
        }

        return aResult;
    }
};

} // namespace spreadsheetengine::compat::libreoffice::substratemutationentry

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
