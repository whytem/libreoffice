/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilot.hxx>

namespace spreadsheetengine::detail::substrate
{

namespace lifecyclebuilddetail
{

[[nodiscard]] inline bool isSingleScalarFormulaCell(const ShadowCellRecord* pCell)
{
    return pCell && pCell->hasFormula()
           && pCell->moFormula->meKind == facade::FormulaCellKind::Ordinary
           && !pCell->moFormulaGroup.has_value();
}

[[nodiscard]] inline AuthorityPilotInput
makeAuthorityInput(const LifecyclePilotInput& rInput)
{
    AuthorityPilotInput aAuthorityInput;
    aAuthorityInput.maComputationalShadow = rInput.maComputationalShadow;
    aAuthorityInput.maGraphShadow = rInput.maGraphShadow;
    aAuthorityInput.maIrShadow = rInput.maIrShadow;
    aAuthorityInput.maMutation = rInput.maMutation;
    aAuthorityInput.moFormulaCachedValueAfter = rInput.moFormulaCachedValueAfter;
    aAuthorityInput.moObservedAfterComputationalShadow = rInput.moObservedAfterComputationalShadow;
    aAuthorityInput.mbAllowSharedGroupNonStructuralAdmission
        = rInput.mbAllowSharedGroupNonStructuralAdmission;
    aAuthorityInput.mbCleanBaseline = rInput.mbCleanBaseline;
    return aAuthorityInput;
}

[[nodiscard]] inline bool classifyLifecycleShape(
    const LifecyclePilotInput& rInput, LifecycleSyncAction& rAction, api::String& rReason)
{
    const ShadowCellRecord* pBeforeCell = rInput.maComputationalShadow.findCell(rInput.maMutation.maAddress);

    switch (rInput.maMutation.meKind)
    {
        case facade::MutationKind::SetFormula:
        {
            rAction.maAddress = rInput.maMutation.maAddress;
            rAction.moFormulaSource = rInput.maMutation.maText;
            rAction.moCachedValueAfter
                = rInput.moFormulaCachedValueAfter.value_or(api::CellValue::number(0.0));

            if (!pBeforeCell || !pBeforeCell->hasFormula())
            {
                if (authoritybuilddetail::isDeferredSharedGroupNonStructuralFormulaInsert(
                        makeAuthorityInput(rInput)))
                {
                    rReason = u"shared_group_non_structural_out_of_contract";
                    return false;
                }

                rAction.meKind = LifecycleSyncActionKind::InsertFormulaCell;
                rAction.mbFormulaPresentBefore = false;
                rAction.mbFormulaPresentAfter = true;
                return true;
            }

            if (!isSingleScalarFormulaCell(pBeforeCell))
            {
                if (pBeforeCell && pBeforeCell->hasFormula() && pBeforeCell->moFormulaGroup)
                {
                    const auto aAuthorityInput = makeAuthorityInput(rInput);
                    const ShadowCellRecord* pSharedCell = nullptr;
                    const ShadowFormulaGroupRecord* pTouchedGroup = nullptr;
                    api::String aSharedGroupReason;
                    if (authoritybuilddetail::isSharedGroupNonStructuralCandidate(
                            aAuthorityInput, pSharedCell, pTouchedGroup, aSharedGroupReason))
                    {
                        rAction.meKind = LifecycleSyncActionKind::ReplaceFormulaCell;
                        rAction.mbFormulaPresentBefore = true;
                        rAction.mbFormulaPresentAfter = true;
                        return true;
                    }

                    rReason = aSharedGroupReason.empty() ? api::String(u"formula_shape_out_of_contract")
                                                         : aSharedGroupReason;
                    return false;
                }

                rReason = u"formula_shape_out_of_contract";
                return false;
            }

            rAction.meKind = LifecycleSyncActionKind::ReplaceFormulaCell;
            rAction.mbFormulaPresentBefore = true;
            rAction.mbFormulaPresentAfter = true;
            return true;
        }
        case facade::MutationKind::ClearCell:
        {
            if (!isSingleScalarFormulaCell(pBeforeCell))
            {
                if (pBeforeCell && pBeforeCell->hasFormula() && pBeforeCell->moFormulaGroup)
                {
                    const auto aAuthorityInput = makeAuthorityInput(rInput);
                    const ShadowCellRecord* pSharedCell = nullptr;
                    const ShadowFormulaGroupRecord* pTouchedGroup = nullptr;
                    api::String aSharedGroupReason;
                    if (authoritybuilddetail::isSharedGroupNonStructuralCandidate(
                            aAuthorityInput, pSharedCell, pTouchedGroup, aSharedGroupReason))
                    {
                        rAction.meKind = LifecycleSyncActionKind::RemoveFormulaCell;
                        rAction.maAddress = rInput.maMutation.maAddress;
                        rAction.mbFormulaPresentBefore = true;
                        rAction.mbFormulaPresentAfter = false;
                        return true;
                    }

                    rReason = aSharedGroupReason.empty() ? api::String(u"formula_shape_out_of_contract")
                                                         : aSharedGroupReason;
                    return false;
                }

                rReason = u"formula_shape_out_of_contract";
                return false;
            }

            rAction.meKind = LifecycleSyncActionKind::RemoveFormulaCell;
            rAction.maAddress = rInput.maMutation.maAddress;
            rAction.mbFormulaPresentBefore = true;
            rAction.mbFormulaPresentAfter = false;
            return true;
        }
        default:
            rReason = u"mutation_not_supported";
            return false;
    }
}

} // namespace lifecyclebuilddetail

[[nodiscard]] inline LifecyclePilotTransition
buildLifecyclePilotTransition(const LifecyclePilotInput& rInput)
{
    LifecyclePilotTransition aTransition;
    aTransition.maInput = rInput;
    aTransition.maContract = lifecycledetail::classifyLifecycleMutation(rInput.maMutation);
    aTransition.maVerification = lifecycledetail::makeLifecycleVerification(aTransition.maContract);

    if (!aTransition.maContract.isAdmitted())
    {
        aTransition.meVerdict = LifecyclePilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"mutation_out_of_contract";
        return aTransition;
    }

    if (aTransition.maContract.mbRequiresCleanBaseline && !rInput.mbCleanBaseline)
    {
        aTransition.meVerdict = LifecyclePilotVerdict::RejectedDirtyBaseline;
        aTransition.maReason = u"dirty_baseline";
        return aTransition;
    }

    LifecycleSyncAction aSyncAction;
    if (!lifecyclebuilddetail::classifyLifecycleShape(rInput, aSyncAction, aTransition.maReason))
    {
        aTransition.meVerdict = LifecyclePilotVerdict::RejectedOutOfContract;
        return aTransition;
    }

    api::String aSharedGroupReason;
    const auto oPredictedSharedGroupShadow
        = authoritybuilddetail::buildPredictedSharedGroupNonStructuralComputationalShadow(
            lifecyclebuilddetail::makeAuthorityInput(rInput), aSharedGroupReason);
    if (!oPredictedSharedGroupShadow && !aSharedGroupReason.empty())
    {
        aTransition.meVerdict = LifecyclePilotVerdict::RejectedOutOfContract;
        aTransition.maReason = aSharedGroupReason;
        return aTransition;
    }

    facade::InMemoryWorkbookFacade aFacade;
    if (oPredictedSharedGroupShadow)
    {
        aFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
            *oPredictedSharedGroupShadow);
        aFacade.setGeneration(oPredictedSharedGroupShadow->maSnapshot.mnGeneration);
    }
    else
    {
        aFacade
            = authoritybuilddetail::materializeFacadeFromComputationalShadow(rInput.maComputationalShadow);
        aFacade.setGeneration(rInput.maComputationalShadow.maSnapshot.mnGeneration + 1);

        AuthorityPilotInput aAuthorityInput;
        aAuthorityInput.maComputationalShadow = rInput.maComputationalShadow;
        aAuthorityInput.maGraphShadow = rInput.maGraphShadow;
        aAuthorityInput.maIrShadow = rInput.maIrShadow;
        aAuthorityInput.maMutation = rInput.maMutation;
        aAuthorityInput.moFormulaCachedValueAfter = rInput.moFormulaCachedValueAfter;
        aAuthorityInput.mbCleanBaseline = rInput.mbCleanBaseline;

        if (!authoritybuilddetail::applyAuthorityMutationToFacade(
                aFacade, aAuthorityInput, aTransition.maReason))
        {
            aTransition.meVerdict = LifecyclePilotVerdict::RejectedOutOfContract;
            return aTransition;
        }
    }

    aTransition.maDependencySnapshot = dependency::buildDependencySnapshot(aFacade);
    if (aTransition.maDependencySnapshot.maReport.mnOpaqueNodeCount > 0
        || aTransition.maDependencySnapshot.maReport.mnOpaqueEdgeCount > 0)
    {
        aTransition.meVerdict = LifecyclePilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"opaque_dependency_surface";
        return aTransition;
    }

    aTransition.maInvalidationPlan
        = dependency::planInvalidation(aTransition.maDependencySnapshot, rInput.maMutation);
    aTransition.maRecalcPlan
        = dependency::buildRecalcPlan(aTransition.maDependencySnapshot, aTransition.maInvalidationPlan);
    const auto aObservation = authoritybuilddetail::buildAuthorityObservationState(
        aTransition.maDependencySnapshot, aTransition.maRecalcPlan);
    if (oPredictedSharedGroupShadow)
    {
        aTransition.maComputationalAfter
            = authoritybuilddetail::applyObservationStateToSharedGroupNonStructuralShadow(
                *oPredictedSharedGroupShadow, aObservation);
    }
    else
    {
        aTransition.maComputationalAfter = buildComputationalWorkbookShadow(aFacade, aObservation);
    }
    aTransition.maGraphAfter = buildDependencyGraphShadow(aTransition.maComputationalAfter, aObservation);
    auto aIrFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
        aTransition.maComputationalAfter);
    aTransition.maIrAfter
        = authoritybuilddetail::buildAuthorityExecutionIrShadow(
            aTransition.maComputationalAfter, aIrFacade);
    aTransition.maSyncActions.push_back(std::move(aSyncAction));
    aTransition.meVerdict = LifecyclePilotVerdict::Applicable;
    aTransition.maReason = u"ready";
    return aTransition;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
