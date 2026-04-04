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

struct AdmittedCellStorageRecord
{
    ShadowCellId maId;
    facade::CellDescriptor maCell;
    std::optional<facade::FormulaCellDescriptor> moFormula;

    [[nodiscard]] constexpr bool operator==(const AdmittedCellStorageRecord& rOther) const = default;
    [[nodiscard]] constexpr bool hasFormula() const { return moFormula.has_value(); }
};

struct AdmittedCellStorage
{
    std::int64_t mnGeneration = 0;
    std::vector<AdmittedCellStorageRecord> maCells;

    [[nodiscard]] constexpr bool operator==(const AdmittedCellStorage& rOther) const = default;

    [[nodiscard]] sal_Int32 getCellCount() const
    {
        return static_cast<sal_Int32>(maCells.size());
    }

    [[nodiscard]] sal_Int32 getFormulaCellCount() const
    {
        return static_cast<sal_Int32>(std::count_if(maCells.begin(), maCells.end(),
            [](const AdmittedCellStorageRecord& rCell) { return rCell.hasFormula(); }));
    }

    [[nodiscard]] const AdmittedCellStorageRecord* findCell(const api::CellAddress& rAddress) const
    {
        auto aIt = std::find_if(maCells.begin(), maCells.end(),
            [&rAddress](const AdmittedCellStorageRecord& rCell) {
                return rCell.maId.maAddress == rAddress;
            });
        return aIt == maCells.end() ? nullptr : &*aIt;
    }
};

struct AdmittedCellStorageComparison
{
    bool mbPopulationMatch = false;
    bool mbPayloadMatch = false;
    bool mbGenerationMatch = false;
    bool mbFullMatch = false;

    [[nodiscard]] constexpr bool operator==(const AdmittedCellStorageComparison& rOther) const
        = default;
};

struct MutableComputationalSubstrateState
{
    facade::InMemoryWorkbookFacade maFacade;
    ComputationalObservationState maObservation;
    ComputationalWorkbookShadow maShadow;
    AdmittedCellStorage maCellStorage;
    facade::MutationEvent maLastMutation;
    sal_Int32 mnAppliedMutationCount = 0;
    bool mbBootstrapped = false;
};

namespace mutablesubstratedetail
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

[[nodiscard]] inline AdmittedCellStorageRecord
makeCellStorageRecord(const ShadowCellRecord& rCell)
{
    AdmittedCellStorageRecord aRecord;
    aRecord.maId = rCell.maId;
    aRecord.maCell = rCell.maCell;
    aRecord.moFormula = rCell.moFormula;
    return aRecord;
}

[[nodiscard]] inline AdmittedCellStorage
buildAdmittedCellStorage(const ComputationalWorkbookShadow& rShadow)
{
    AdmittedCellStorage aStore;
    aStore.mnGeneration = rShadow.maSnapshot.mnGeneration;

    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
            aStore.maCells.push_back(makeCellStorageRecord(rCell));
    }

    std::sort(aStore.maCells.begin(), aStore.maCells.end(),
        [](const AdmittedCellStorageRecord& rLeft, const AdmittedCellStorageRecord& rRight) {
            return AddressLess {}(rLeft.maId.maAddress, rRight.maId.maAddress);
        });
    return aStore;
}

inline void reconcileAdmittedCellStorage(
    AdmittedCellStorage& rStore, const ComputationalWorkbookShadow& rShadow)
{
    const auto aAfter = buildAdmittedCellStorage(rShadow);

    std::vector<AdmittedCellStorageRecord> aMerged;
    aMerged.reserve(aAfter.maCells.size());

    std::size_t nBeforeIndex = 0;
    std::size_t nAfterIndex = 0;
    while (nBeforeIndex < rStore.maCells.size() || nAfterIndex < aAfter.maCells.size())
    {
        if (nBeforeIndex >= rStore.maCells.size())
        {
            aMerged.push_back(aAfter.maCells[nAfterIndex++]);
            continue;
        }
        if (nAfterIndex >= aAfter.maCells.size())
        {
            ++nBeforeIndex;
            continue;
        }

        const auto& rBefore = rStore.maCells[nBeforeIndex];
        const auto& rAfter = aAfter.maCells[nAfterIndex];

        if (AddressLess {}(rBefore.maId.maAddress, rAfter.maId.maAddress))
        {
            ++nBeforeIndex;
            continue;
        }
        if (AddressLess {}(rAfter.maId.maAddress, rBefore.maId.maAddress))
        {
            aMerged.push_back(rAfter);
            ++nAfterIndex;
            continue;
        }

        aMerged.push_back(rAfter);
        ++nBeforeIndex;
        ++nAfterIndex;
    }

    rStore.mnGeneration = aAfter.mnGeneration;
    rStore.maCells = std::move(aMerged);
}

[[nodiscard]] inline AdmittedCellStorageComparison compareAdmittedCellStorage(
    const AdmittedCellStorage& rStore, const ComputationalWorkbookShadow& rShadow)
{
    const auto aExpected = buildAdmittedCellStorage(rShadow);

    AdmittedCellStorageComparison aComparison;
    aComparison.mbPopulationMatch = rStore.getCellCount() == aExpected.getCellCount()
        && rStore.getFormulaCellCount() == aExpected.getFormulaCellCount();
    aComparison.mbPayloadMatch = rStore.maCells == aExpected.maCells;
    aComparison.mbGenerationMatch = rStore.mnGeneration == aExpected.mnGeneration;
    aComparison.mbFullMatch = aComparison.mbPopulationMatch && aComparison.mbPayloadMatch
        && aComparison.mbGenerationMatch;
    return aComparison;
}

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
    rState.maCellStorage = buildAdmittedCellStorage(rShadow);
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

[[nodiscard]] inline AdmittedCellStorage
buildAdmittedCellStorage(const ComputationalWorkbookShadow& rShadow)
{
    return mutablesubstratedetail::buildAdmittedCellStorage(rShadow);
}

[[nodiscard]] inline AdmittedCellStorageComparison compareAdmittedCellStorage(
    const AdmittedCellStorage& rStore, const ComputationalWorkbookShadow& rShadow)
{
    return mutablesubstratedetail::compareAdmittedCellStorage(rStore, rShadow);
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
    mutablesubstratedetail::reconcileAdmittedCellStorage(
        rState.maCellStorage, rTransition.maComputationalAfter);
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
    mutablesubstratedetail::reconcileAdmittedCellStorage(
        rState.maCellStorage, rTransition.maComputationalAfter);
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
    mutablesubstratedetail::reconcileAdmittedCellStorage(
        rState.maCellStorage, rTransition.maComputationalAfter);
    rState.maLastMutation = rTransition.maInput.maMutation;
    ++rState.mnAppliedMutationCount;
    return true;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
