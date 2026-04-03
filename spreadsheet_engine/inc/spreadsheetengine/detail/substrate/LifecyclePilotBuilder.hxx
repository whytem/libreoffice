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

struct AddressLess
{
    [[nodiscard]] bool operator()(const api::CellAddress& rLeft, const api::CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

struct RangeLess
{
    [[nodiscard]] bool operator()(const api::CellRange& rLeft, const api::CellRange& rRight) const
    {
        if (!(rLeft.maStart == rRight.maStart))
            return AddressLess {}(rLeft.maStart, rRight.maStart);
        return AddressLess {}(rLeft.maEnd, rRight.maEnd);
    }
};

[[nodiscard]] inline bool isSingleScalarFormulaCell(const ShadowCellRecord* pCell)
{
    return pCell && pCell->hasFormula()
           && pCell->moFormula->meKind == facade::FormulaCellKind::Ordinary
           && !pCell->moFormulaGroup.has_value();
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
                rAction.meKind = LifecycleSyncActionKind::InsertFormulaCell;
                rAction.mbFormulaPresentBefore = false;
                rAction.mbFormulaPresentAfter = true;
                return true;
            }

            if (!isSingleScalarFormulaCell(pBeforeCell))
            {
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

[[nodiscard]] inline ComputationalObservationState buildLifecycleObservationState(
    const dependency::DependencySnapshot& rSnapshot, const dependency::RecalcPlan& rPlan)
{
    ComputationalObservationState aObservation;
    aObservation.maFormulaTree = authoritybuilddetail::collectQueueAddresses(rPlan);

    std::map<api::CellAddress, std::vector<ListenerAnchorId>, AddressLess> aCellBroadcasters;
    std::map<api::CellRange, std::vector<ListenerAnchorId>, RangeLess> aAreaBroadcasters;

    for (const auto& rNode : rSnapshot.maNodes)
    {
        if (rNode.meKind != dependency::DependencyNodeKind::FormulaCell || !rNode.moOutputAddress)
            continue;

        const auto aListenerAnchor
            = graphmapping::makeGraphFormulaCellListenerAnchorId(*rNode.moOutputAddress);
        std::vector<facade::NamedRangeId> aVisitedNamedRanges;
        std::vector<dependency::DependencySource> aResolvedSources;
        for (const auto& rDependency : rSnapshot.getDependencies(rNode.maId))
        {
            authoritybuilddetail::collectResolvedDependencySources(
                rSnapshot, rDependency.maSource, aVisitedNamedRanges, aResolvedSources);
        }

        for (const auto& rSource : aResolvedSources)
        {
            if (rSource.meKind == dependency::DependencySourceKind::Cell)
            {
                aCellBroadcasters[rSource.maCellAddress].push_back(aListenerAnchor);
                continue;
            }

            if (rSource.meKind == dependency::DependencySourceKind::Range)
            {
                aAreaBroadcasters[dependency::detail::normalizeRange(rSource.maCellRange)].push_back(
                    aListenerAnchor);
            }
        }
    }

    for (auto& [rAddress, rListeners] : aCellBroadcasters)
    {
        graphmapping::sortAndUnique(rListeners, graphmapping::ListenerAnchorIdLess {});
        aObservation.maCellBroadcasters.push_back({ rAddress, std::move(rListeners) });
    }

    for (auto& [rRange, rListeners] : aAreaBroadcasters)
    {
        graphmapping::sortAndUnique(rListeners, graphmapping::ListenerAnchorIdLess {});
        aObservation.maAreaBroadcasters.push_back({ rRange, std::move(rListeners) });
    }

    return aObservation;
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

    auto aFacade
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
    const auto aObservation = lifecyclebuilddetail::buildLifecycleObservationState(
        aTransition.maDependencySnapshot, aTransition.maRecalcPlan);
    aTransition.maComputationalAfter
        = buildComputationalWorkbookShadow(aFacade, aObservation);
    aTransition.maGraphAfter = buildDependencyGraphShadow(aTransition.maComputationalAfter, aObservation);
    aTransition.maIrAfter
        = authoritybuilddetail::buildAuthorityExecutionIrShadow(aTransition.maComputationalAfter, aFacade);
    aTransition.maSyncActions.push_back(std::move(aSyncAction));
    aTransition.meVerdict = LifecyclePilotVerdict::Applicable;
    aTransition.maReason = u"ready";
    return aTransition;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
