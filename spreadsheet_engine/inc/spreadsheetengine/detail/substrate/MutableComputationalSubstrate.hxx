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
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/GraphWiringDelta.hxx>
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

struct AdmittedWiringContainers
{
    std::int64_t mnGeneration = 0;
    std::vector<GraphBroadcasterNodeRecord> maBroadcasterNodes;
    std::vector<GraphEdgeRecord> maListenerEdges;
    std::vector<ShadowCellId> maFormulaTreeNodes;
    std::vector<ShadowCellId> maFormulaTrackNodes;

    [[nodiscard]] constexpr bool operator==(const AdmittedWiringContainers& rOther) const = default;

    [[nodiscard]] sal_Int32 getBroadcasterNodeCount() const
    {
        return static_cast<sal_Int32>(maBroadcasterNodes.size());
    }

    [[nodiscard]] sal_Int32 getListenerEdgeCount() const
    {
        return static_cast<sal_Int32>(maListenerEdges.size());
    }

    [[nodiscard]] const GraphBroadcasterNodeRecord* findBroadcasterNode(
        const BroadcasterNodeId& rId) const
    {
        auto aIt = std::find_if(maBroadcasterNodes.begin(), maBroadcasterNodes.end(),
            [&rId](const GraphBroadcasterNodeRecord& rNode) { return rNode.maId == rId; });
        return aIt == maBroadcasterNodes.end() ? nullptr : &*aIt;
    }

    [[nodiscard]] const GraphEdgeRecord* findListenerEdge(const GraphEdgeRecord& rEdge) const
    {
        auto aIt = std::find_if(maListenerEdges.begin(), maListenerEdges.end(),
            [&rEdge](const GraphEdgeRecord& rCandidate) { return rCandidate == rEdge; });
        return aIt == maListenerEdges.end() ? nullptr : &*aIt;
    }
};

struct AdmittedWiringContainerComparison
{
    bool mbPopulationMatch = false;
    bool mbPayloadMatch = false;
    bool mbGenerationMatch = false;
    bool mbFullMatch = false;

    [[nodiscard]] constexpr bool operator==(const AdmittedWiringContainerComparison& rOther) const
        = default;
};

struct MutableComputationalSubstrateState
{
    facade::InMemoryWorkbookFacade maFacade;
    ComputationalObservationState maObservation;
    ComputationalWorkbookShadow maShadow;
    DependencyGraphShadow maGraphShadow;
    AdmittedCellStorage maCellStorage;
    AdmittedWiringContainers maWiringContainers;
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

[[nodiscard]] inline AdmittedWiringContainers
buildAdmittedWiringContainers(const DependencyGraphShadow& rGraph)
{
    AdmittedWiringContainers aStore;
    aStore.mnGeneration = rGraph.maSnapshot.mnGeneration;
    aStore.maBroadcasterNodes = rGraph.maBroadcasterNodes;
    aStore.maListenerEdges = rGraph.maEdges;
    aStore.maFormulaTreeNodes = rGraph.maFormulaTreeNodes;
    aStore.maFormulaTrackNodes = rGraph.maFormulaTrackNodes;

    std::sort(aStore.maBroadcasterNodes.begin(), aStore.maBroadcasterNodes.end(),
        [](const GraphBroadcasterNodeRecord& rLeft, const GraphBroadcasterNodeRecord& rRight) {
            return graphmapping::BroadcasterNodeIdLess {}(rLeft.maId, rRight.maId);
        });
    std::sort(aStore.maListenerEdges.begin(), aStore.maListenerEdges.end(),
        graphmapping::GraphEdgeRecordLess {});
    return aStore;
}

inline void upsertBroadcasterNode(AdmittedWiringContainers& rStore,
    const BroadcasterNodeDeltaRecord& rDelta)
{
    auto aIt = std::remove_if(rStore.maBroadcasterNodes.begin(), rStore.maBroadcasterNodes.end(),
        [&rDelta](const GraphBroadcasterNodeRecord& rNode) { return rNode.maId == rDelta.maNode; });
    rStore.maBroadcasterNodes.erase(aIt, rStore.maBroadcasterNodes.end());

    if (rDelta.meAction == GraphDeltaAction::Add)
    {
        GraphBroadcasterNodeRecord aNode;
        aNode.maId = rDelta.maNode;
        aNode.mnListenerCount = rDelta.mnListenerCount;
        rStore.maBroadcasterNodes.insert(
            std::lower_bound(rStore.maBroadcasterNodes.begin(), rStore.maBroadcasterNodes.end(),
                aNode,
                [](const GraphBroadcasterNodeRecord& rLeft,
                    const GraphBroadcasterNodeRecord& rRight) {
                    return graphmapping::BroadcasterNodeIdLess {}(rLeft.maId, rRight.maId);
                }),
            aNode);
    }
}

inline void upsertListenerEdge(
    AdmittedWiringContainers& rStore, const ListenerEdgeDeltaRecord& rDelta)
{
    auto aIt = std::remove(rStore.maListenerEdges.begin(), rStore.maListenerEdges.end(), rDelta.maEdge);
    rStore.maListenerEdges.erase(aIt, rStore.maListenerEdges.end());

    if (rDelta.meAction == GraphDeltaAction::Add)
    {
        rStore.maListenerEdges.insert(
            std::lower_bound(rStore.maListenerEdges.begin(), rStore.maListenerEdges.end(),
                rDelta.maEdge, graphmapping::GraphEdgeRecordLess {}),
            rDelta.maEdge);
    }
}

inline void reconcileAdmittedWiringContainers(AdmittedWiringContainers& rStore,
    const GraphWiringDelta& rDelta, const DependencyGraphShadow& rGraphAfter)
{
    for (const auto& rNodeDelta : rDelta.maBroadcasterNodeDeltas)
        upsertBroadcasterNode(rStore, rNodeDelta);
    for (const auto& rEdgeDelta : rDelta.maListenerEdgeDeltas)
        upsertListenerEdge(rStore, rEdgeDelta);

    rStore.maBroadcasterNodes = rGraphAfter.maBroadcasterNodes;
    std::sort(rStore.maBroadcasterNodes.begin(), rStore.maBroadcasterNodes.end(),
        [](const GraphBroadcasterNodeRecord& rLeft, const GraphBroadcasterNodeRecord& rRight) {
            return graphmapping::BroadcasterNodeIdLess {}(rLeft.maId, rRight.maId);
        });
    rStore.maFormulaTreeNodes = rDelta.maFormulaTreeAfter;
    rStore.maFormulaTrackNodes = rDelta.maFormulaTrackAfter;
    rStore.mnGeneration = rGraphAfter.maSnapshot.mnGeneration;
}

[[nodiscard]] inline AdmittedWiringContainerComparison compareAdmittedWiringContainers(
    const AdmittedWiringContainers& rStore, const DependencyGraphShadow& rGraph)
{
    const auto aExpected = buildAdmittedWiringContainers(rGraph);

    AdmittedWiringContainerComparison aComparison;
    aComparison.mbPopulationMatch = rStore.getBroadcasterNodeCount() == aExpected.getBroadcasterNodeCount()
        && rStore.getListenerEdgeCount() == aExpected.getListenerEdgeCount()
        && rStore.maFormulaTreeNodes.size() == aExpected.maFormulaTreeNodes.size()
        && rStore.maFormulaTrackNodes.size() == aExpected.maFormulaTrackNodes.size();
    aComparison.mbPayloadMatch = rStore.maBroadcasterNodes == aExpected.maBroadcasterNodes
        && rStore.maListenerEdges == aExpected.maListenerEdges
        && rStore.maFormulaTreeNodes == aExpected.maFormulaTreeNodes
        && rStore.maFormulaTrackNodes == aExpected.maFormulaTrackNodes;
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
    rState.maGraphShadow = buildDependencyGraphShadow(rShadow, rState.maObservation);
    rState.maCellStorage = buildAdmittedCellStorage(rShadow);
    rState.maWiringContainers = buildAdmittedWiringContainers(rState.maGraphShadow);
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

[[nodiscard]] inline AdmittedWiringContainers
buildAdmittedWiringContainers(const DependencyGraphShadow& rGraph)
{
    return mutablesubstratedetail::buildAdmittedWiringContainers(rGraph);
}

[[nodiscard]] inline AdmittedWiringContainerComparison compareAdmittedWiringContainers(
    const AdmittedWiringContainers& rStore, const DependencyGraphShadow& rGraph)
{
    return mutablesubstratedetail::compareAdmittedWiringContainers(rStore, rGraph);
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

    const auto aWiringDelta = buildGraphWiringDelta(
        rState.maGraphShadow, rTransition.maGraphAfter, rTransition.maRecalcPlan,
        rTransition.maInput.maMutation);
    rState.maFacade.setGeneration(rTransition.maComputationalAfter.maSnapshot.mnGeneration);
    rState.maObservation
        = mutablesubstratedetail::makeObservationStateFromShadow(rTransition.maComputationalAfter);
    rState.maShadow = rTransition.maComputationalAfter;
    rState.maGraphShadow = rTransition.maGraphAfter;
    mutablesubstratedetail::reconcileAdmittedCellStorage(
        rState.maCellStorage, rTransition.maComputationalAfter);
    mutablesubstratedetail::reconcileAdmittedWiringContainers(
        rState.maWiringContainers, aWiringDelta, rTransition.maGraphAfter);
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

    const auto aWiringDelta = buildGraphWiringDelta(
        rState.maGraphShadow, rTransition.maGraphAfter, rTransition.maRecalcPlan,
        rTransition.maInput.maMutation);
    rState.maFacade.setGeneration(rTransition.maComputationalAfter.maSnapshot.mnGeneration);
    rState.maObservation
        = mutablesubstratedetail::makeObservationStateFromShadow(rTransition.maComputationalAfter);
    rState.maShadow = rTransition.maComputationalAfter;
    rState.maGraphShadow = rTransition.maGraphAfter;
    mutablesubstratedetail::reconcileAdmittedCellStorage(
        rState.maCellStorage, rTransition.maComputationalAfter);
    mutablesubstratedetail::reconcileAdmittedWiringContainers(
        rState.maWiringContainers, aWiringDelta, rTransition.maGraphAfter);
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

    if (!rState.mbBootstrapped)
        mutablesubstratedetail::setStateFromShadow(rState, rTransition.maInput.maComputationalShadow);

    const auto aWiringDelta = buildGraphWiringDelta(
        rState.maGraphShadow, rTransition.maGraphAfter, rTransition.maRecalcPlan,
        rTransition.maInput.maMutation);
    rState.maFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
        rTransition.maComputationalAfter);
    rState.maObservation
        = mutablesubstratedetail::makeObservationStateFromShadow(rTransition.maComputationalAfter);
    rState.maShadow = rTransition.maComputationalAfter;
    rState.maGraphShadow = rTransition.maGraphAfter;
    mutablesubstratedetail::reconcileAdmittedCellStorage(
        rState.maCellStorage, rTransition.maComputationalAfter);
    mutablesubstratedetail::reconcileAdmittedWiringContainers(
        rState.maWiringContainers, aWiringDelta, rTransition.maGraphAfter);
    rState.maLastMutation = rTransition.maInput.maMutation;
    ++rState.mnAppliedMutationCount;
    return true;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
