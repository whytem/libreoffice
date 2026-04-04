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
#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilot.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilot.hxx>

namespace spreadsheetengine::detail::substrate
{

struct MutableComputationalSubstrateState
{
    facade::InMemoryWorkbookFacade maFacade;
    ComputationalObservationState maObservation;
    ComputationalWorkbookShadow maShadow;
    facade::MutationEvent maLastMutation;
    sal_Int32 mnAppliedMutationCount = 0;
    bool mbBootstrapped = false;
};

namespace mutablesubstratedetail
{

[[nodiscard]] inline ComputationalObservationState
makeObservationStateFromShadow(const ComputationalWorkbookShadow& rShadow)
{
    ComputationalObservationState aObservation;
    aObservation.maFormulaTree = rShadow.maFormulaTree;
    aObservation.maFormulaTrack = rShadow.maFormulaTrack;
    aObservation.maCellBroadcasters = rShadow.maCellBroadcasters;
    aObservation.maAreaBroadcasters = rShadow.maAreaBroadcasters;
    return aObservation;
}

inline void setStateFromShadow(MutableComputationalSubstrateState& rState,
    const ComputationalWorkbookShadow& rShadow)
{
    rState.maObservation = makeObservationStateFromShadow(rShadow);
    rState.maShadow = rShadow;
    rState.maFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(rShadow);
    rState.mbBootstrapped = true;
}

inline void applyLifecycleSyncAction(facade::InMemoryWorkbookFacade& rFacade,
    const LifecycleSyncAction& rAction)
{
    switch (rAction.meKind)
    {
        case LifecycleSyncActionKind::InsertFormulaCell:
        case LifecycleSyncActionKind::ReplaceFormulaCell:
            if (rAction.moFormulaSource)
            {
                rFacade.setFormulaCell(rAction.maAddress, *rAction.moFormulaSource,
                    rAction.moCachedValueAfter.value_or(api::CellValue::number(0.0)));
            }
            break;
        case LifecycleSyncActionKind::RemoveFormulaCell:
            rFacade.clearCell(rAction.maAddress);
            break;
    }
}

} // namespace mutablesubstratedetail

[[nodiscard]] inline MutableComputationalSubstrateState
bootstrapMutableComputationalSubstrateState(const ComputationalWorkbookShadow& rShadow)
{
    MutableComputationalSubstrateState aState;
    mutablesubstratedetail::setStateFromShadow(aState, rShadow);
    return aState;
}

[[nodiscard]] inline MutableComputationalSubstrateState
bootstrapMutableComputationalSubstrateState(const facade::WorkbookFacade& rFacade,
    const ComputationalObservationState& rObservation = {})
{
    return bootstrapMutableComputationalSubstrateState(
        buildComputationalWorkbookShadow(rFacade, rObservation));
}

[[nodiscard]] inline bool applyMutableAuthorityTransition(
    MutableComputationalSubstrateState& rState, const AuthorityPilotTransition& rTransition)
{
    if (rTransition.meVerdict != AuthorityPilotVerdict::Applicable
        && rTransition.meVerdict != AuthorityPilotVerdict::Applied
        && rTransition.meVerdict != AuthorityPilotVerdict::NormalizedEquivalent)
    {
        return false;
    }

    if (!rState.mbBootstrapped)
        mutablesubstratedetail::setStateFromShadow(rState, rTransition.maInput.maComputationalShadow);

    AuthorityPilotInput aFacadeInput;
    aFacadeInput.maMutation = rTransition.maInput.maMutation;
    aFacadeInput.moScalarValueAfter = rTransition.maInput.moScalarValueAfter;
    aFacadeInput.moFormulaCachedValueAfter = rTransition.maInput.moFormulaCachedValueAfter;

    api::String aIgnoredReason;
    if (!authoritybuilddetail::applyAuthorityMutationToFacade(rState.maFacade, aFacadeInput, aIgnoredReason))
        return false;

    rState.maFacade.setGeneration(rTransition.maComputationalAfter.maSnapshot.mnGeneration);
    rState.maObservation
        = mutablesubstratedetail::makeObservationStateFromShadow(rTransition.maComputationalAfter);
    rState.maShadow = rTransition.maComputationalAfter;
    rState.maLastMutation = rTransition.maInput.maMutation;
    ++rState.mnAppliedMutationCount;
    return true;
}

[[nodiscard]] inline bool applyMutableLifecycleTransition(
    MutableComputationalSubstrateState& rState, const LifecyclePilotTransition& rTransition)
{
    if (rTransition.meVerdict != LifecyclePilotVerdict::Applicable
        && rTransition.meVerdict != LifecyclePilotVerdict::Applied
        && rTransition.meVerdict != LifecyclePilotVerdict::NormalizedEquivalent)
    {
        return false;
    }

    if (!rState.mbBootstrapped)
        mutablesubstratedetail::setStateFromShadow(rState, rTransition.maInput.maComputationalShadow);

    for (const auto& rAction : rTransition.maSyncActions)
        mutablesubstratedetail::applyLifecycleSyncAction(rState.maFacade, rAction);

    rState.maFacade.setGeneration(rTransition.maComputationalAfter.maSnapshot.mnGeneration);
    rState.maObservation
        = mutablesubstratedetail::makeObservationStateFromShadow(rTransition.maComputationalAfter);
    rState.maShadow = rTransition.maComputationalAfter;
    rState.maLastMutation = rTransition.maInput.maMutation;
    ++rState.mnAppliedMutationCount;
    return true;
}

[[nodiscard]] inline bool applyMutableStructuralTransition(
    MutableComputationalSubstrateState& rState, const StructuralPilotTransition& rTransition)
{
    if (rTransition.meVerdict != StructuralPilotVerdict::Applicable
        && rTransition.meVerdict != StructuralPilotVerdict::Applied
        && rTransition.meVerdict != StructuralPilotVerdict::NormalizedEquivalent)
    {
        return false;
    }

    mutablesubstratedetail::setStateFromShadow(rState, rTransition.maComputationalAfter);
    rState.maLastMutation = rTransition.maInput.maMutation;
    ++rState.mnAppliedMutationCount;
    return true;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
