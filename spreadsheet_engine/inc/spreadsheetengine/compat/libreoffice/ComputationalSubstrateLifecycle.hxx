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

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcQueueExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx>

namespace spreadsheetengine::compat::libreoffice::substratelifecycle
{

enum class LifecycleResultKind : sal_uInt8
{
    Disabled,
    RejectedOutOfContract,
    RejectedDirtyBaseline,
    RolledBackVerificationFailure,
    RepairDetected,
    Applied,
    AppliedNormalizedEquivalent
};

struct LifecycleResult
{
    LifecycleResultKind meKind = LifecycleResultKind::Disabled;
    spreadsheetengine::detail::substrate::LifecyclePilotTransition maTransition;
    std::optional<recalcshadow::ShadowComparison> moQueueComparison;
    std::optional<spreadsheetengine::detail::substrate::ComputationalShadowComparison>
        moComputationalComparison;
    std::optional<spreadsheetengine::detail::substrate::DependencyGraphShadowComparison>
        moGraphComparison;
    std::optional<spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison>
        moIrComparison;
};

namespace detail
{

[[nodiscard]] inline bool isRuntimeEnabled(const ScDocument& rDoc)
{
    return substraterollout::isSurfaceEnabled(rDoc, substraterollout::RolloutSurface::Lifecycle);
}

[[nodiscard]] inline bool isSharedGroupNonStructuralCandidateEnabled(const ScDocument& rDoc)
{
    return isRuntimeEnabled(rDoc)
           && substraterollout::isSurfaceEnabled(
               rDoc, substraterollout::RolloutSurface::SharedGroupNonStructural);
}

[[nodiscard]] inline bool acceptsQueueComparison(const recalcshadow::ShadowComparison& rComparison)
{
    return rComparison.meKind == recalcshadow::ShadowComparisonKind::Exact;
}

[[nodiscard]] inline bool acceptsComputationalComparison(
    const spreadsheetengine::detail::substrate::ComputationalShadowComparison& rComparison,
    spreadsheetengine::detail::substrate::LifecycleVerificationMode)
{
    return rComparison.mbFullMatch;
}

[[nodiscard]] inline bool acceptsGraphComparison(
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rComparison,
    spreadsheetengine::detail::substrate::LifecycleVerificationMode eMode)
{
    if (eMode == spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact)
        return rComparison.meKind
               == spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::Exact;
    return rComparison.meKind
           != spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::Mismatch;
}

[[nodiscard]] inline bool anyNormalizedEquivalent(const LifecycleResult& rResult)
{
    return (rResult.moGraphComparison
            && rResult.moGraphComparison->meKind
                   == spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind::
                       NormalizedEquivalent)
           || (rResult.moIrComparison
               && rResult.moIrComparison->meKind
                      == spreadsheetengine::detail::substrate::ExecutionIrComparisonKind::
                          NormalizedEquivalent);
}

[[nodiscard]] inline LifecycleResultKind classifyVerifiedLifecycleResult(
    const LifecycleResult& rResult)
{
    switch (rResult.maTransition.meVerdict)
    {
        case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RejectedOutOfContract:
            return LifecycleResultKind::RejectedOutOfContract;
        case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RejectedDirtyBaseline:
            return LifecycleResultKind::RejectedDirtyBaseline;
        case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RepairDetected:
            return LifecycleResultKind::RepairDetected;
        case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RolledBack:
            return LifecycleResultKind::RolledBackVerificationFailure;
        case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable:
        case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applied:
        case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::NormalizedEquivalent:
            break;
    }

    if (!rResult.moQueueComparison || !rResult.moComputationalComparison || !rResult.moGraphComparison)
        return LifecycleResultKind::RolledBackVerificationFailure;

    if (!acceptsQueueComparison(*rResult.moQueueComparison)
        || !acceptsGraphComparison(
            *rResult.moGraphComparison, rResult.maTransition.maVerification.meGraphMode))
    {
        return LifecycleResultKind::RolledBackVerificationFailure;
    }

    if (!acceptsComputationalComparison(
            *rResult.moComputationalComparison,
            rResult.maTransition.maVerification.meComputationalMode))
    {
        return LifecycleResultKind::RepairDetected;
    }

    return anyNormalizedEquivalent(rResult) ? LifecycleResultKind::AppliedNormalizedEquivalent
                                            : LifecycleResultKind::Applied;
}

[[nodiscard]] inline spreadsheetengine::detail::substrate::LifecyclePilotInput
prepareLifecycleInput(
    const spreadsheetengine::detail::substrate::ComputationalWorkbookShadow& rComputationalShadow,
    const spreadsheetengine::detail::substrate::DependencyGraphShadow& rGraphShadow,
    const spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow& rIrShadow,
    const spreadsheetengine::detail::facade::MutationEvent& rMutation,
    const spreadsheetengine::detail::facade::WorkbookFacade& rAfterFacade,
    const spreadsheetengine::detail::substrate::ComputationalWorkbookShadow&
        rObservedAfterComputationalShadow,
    bool bAllowSharedGroupNonStructuralAdmission, bool bCleanBaseline)
{
    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = rComputationalShadow;
    aInput.maGraphShadow = rGraphShadow;
    aInput.maIrShadow = rIrShadow;
    aInput.maMutation = rMutation;
    aInput.moObservedAfterComputationalShadow = rObservedAfterComputationalShadow;
    aInput.mbAllowSharedGroupNonStructuralAdmission
        = bAllowSharedGroupNonStructuralAdmission;
    aInput.mbCleanBaseline = bCleanBaseline;

    if (rMutation.meKind == spreadsheetengine::detail::facade::MutationKind::SetFormula)
    {
        if (const auto oFormula = rAfterFacade.getFormulaCellDescriptor(rMutation.maAddress))
            aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;
    }

    return aInput;
}

[[nodiscard]] inline bool documentSatisfiesSyncAction(
    const ScDocument& rDoc, const spreadsheetengine::detail::substrate::LifecycleSyncAction& rAction)
{
    const ScAddress aAddress(rAction.maAddress.mnColumn, rAction.maAddress.mnRow, rAction.maAddress.mnSheet);
    const ScFormulaCell* pCell = rDoc.GetFormulaCell(aAddress);

    switch (rAction.meKind)
    {
        case spreadsheetengine::detail::substrate::LifecycleSyncActionKind::InsertFormulaCell:
        case spreadsheetengine::detail::substrate::LifecycleSyncActionKind::ReplaceFormulaCell:
            return pCell && rAction.moFormulaSource
                   && pCell->GetFormula() == toLibreOfficeString(*rAction.moFormulaSource);
        case spreadsheetengine::detail::substrate::LifecycleSyncActionKind::RemoveFormulaCell:
            return pCell == nullptr;
    }

    return false;
}

inline void applyLifecycleSyncAction(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::LifecycleSyncAction& rAction)
{
    if (documentSatisfiesSyncAction(rDoc, rAction))
        return;

    const ScAddress aAddress(rAction.maAddress.mnColumn, rAction.maAddress.mnRow, rAction.maAddress.mnSheet);
    switch (rAction.meKind)
    {
        case spreadsheetengine::detail::substrate::LifecycleSyncActionKind::InsertFormulaCell:
        case spreadsheetengine::detail::substrate::LifecycleSyncActionKind::ReplaceFormulaCell:
            if (rAction.moFormulaSource)
                rDoc.SetString(aAddress, toLibreOfficeString(*rAction.moFormulaSource));
            break;
        case spreadsheetengine::detail::substrate::LifecycleSyncActionKind::RemoveFormulaCell:
            rDoc.SetEmptyCell(aAddress);
            break;
    }
}

} // namespace detail

class ScopedComputationalLifecycle
{
    bool mbCaptured = false;
    bool mbCleanBaseline = false;
    spreadsheetengine::detail::substrate::ComputationalWorkbookShadow maComputationalShadow;
    spreadsheetengine::detail::substrate::DependencyGraphShadow maGraphShadow;
    spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow maIrShadow;

public:
    ScopedComputationalLifecycle() = default;

    explicit ScopedComputationalLifecycle(ScDocument& rDoc, bool bCapture)
    {
        if (!bCapture)
            return;

        const CalcWorkbookFacade aFacade(rDoc, 0);
        maComputationalShadow = buildComputationalWorkbookShadow(aFacade, rDoc);
        maGraphShadow = buildDependencyGraphShadow(aFacade, rDoc);
        maIrShadow = buildExecutionIrWorkbookShadow(aFacade, rDoc);
        mbCleanBaseline = recalcqueue::isCleanFormulaState(recalcqueue::captureFormulaState(rDoc));
        mbCaptured = true;
    }

    [[nodiscard]] static ScopedComputationalLifecycle captureIfRuntimeEnabled(ScDocument& rDoc)
    {
        return ScopedComputationalLifecycle(rDoc, detail::isRuntimeEnabled(rDoc));
    }

    [[nodiscard]] bool isCaptured() const { return mbCaptured; }
    [[nodiscard]] bool canApplyLifecycle() const { return mbCaptured && mbCleanBaseline; }

    [[nodiscard]] std::optional<LifecycleResult> apply(
        ScDocument& rDoc, const spreadsheetengine::detail::facade::MutationEvent& rMutation) const
    {
        if (!mbCaptured)
            return std::nullopt;

        LifecycleResult aResult;
        const sal_Int64 nAfterGeneration = maComputationalShadow.maSnapshot.mnGeneration + 1;
        const CalcWorkbookFacade aAfterFacade(rDoc, nAfterGeneration);
        const auto aAfterObservation = makeComputationalObservationState(
            substrateobs::collectLiveComputationalState(rDoc));
        const auto aObservedAfterComputationalShadow
            = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
        aResult.maTransition = spreadsheetengine::detail::substrate::buildLifecyclePilotTransition(
            detail::prepareLifecycleInput(maComputationalShadow, maGraphShadow, maIrShadow,
                rMutation, aAfterFacade, aObservedAfterComputationalShadow,
                detail::isSharedGroupNonStructuralCandidateEnabled(rDoc), mbCleanBaseline));

        switch (aResult.maTransition.meVerdict)
        {
            case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RejectedDirtyBaseline:
                aResult.meKind = LifecycleResultKind::RejectedDirtyBaseline;
                return aResult;
            case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RejectedOutOfContract:
                aResult.meKind = LifecycleResultKind::RejectedOutOfContract;
                return aResult;
            case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable:
            case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applied:
            case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::NormalizedEquivalent:
            case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RolledBack:
            case spreadsheetengine::detail::substrate::LifecyclePilotVerdict::RepairDetected:
                break;
        }

        const auto aStateBeforeApply = recalcqueue::captureFormulaState(rDoc);
        for (const auto& rAction : aResult.maTransition.maSyncActions)
            detail::applyLifecycleSyncAction(rDoc, rAction);
        recalcqueue::applyRecalcPlan(rDoc, aResult.maTransition.maRecalcPlan);

        const CalcWorkbookFacade aVerifiedFacade(rDoc, nAfterGeneration);
        aResult.moQueueComparison = recalcshadow::detail::comparePlanToDocument(
            aResult.maTransition.maRecalcPlan, aVerifiedFacade, rDoc);

        const auto aObservation = makeComputationalObservationState(
            substrateobs::collectLiveComputationalState(rDoc));
        const auto aLiveComputationalShadow
            = spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow(
                aVerifiedFacade, aObservation);
        aResult.moComputationalComparison
            = spreadsheetengine::detail::substrate::compareComputationalShadow(
                aResult.maTransition.maComputationalAfter, aVerifiedFacade, aObservation);
        aResult.moGraphComparison = spreadsheetengine::detail::substrate::compareDependencyGraphShadow(
            aResult.maTransition.maGraphAfter, aLiveComputationalShadow, aObservation);
        aResult.moIrComparison = spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow(
            aResult.maTransition.maIrAfter, buildExecutionIrWorkbookShadow(aLiveComputationalShadow, rDoc));

        aResult.meKind = detail::classifyVerifiedLifecycleResult(aResult);
        if (aResult.meKind == LifecycleResultKind::RolledBackVerificationFailure
            || aResult.meKind == LifecycleResultKind::RepairDetected)
        {
            recalcqueue::restoreFormulaState(rDoc, aStateBeforeApply);
        }

        return aResult;
    }
};

} // namespace spreadsheetengine::compat::libreoffice::substratelifecycle

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
