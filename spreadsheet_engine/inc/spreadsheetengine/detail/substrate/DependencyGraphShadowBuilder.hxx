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
#include <utility>

#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowMapping.hxx>

namespace spreadsheetengine::detail::substrate
{

namespace graphdetail
{

[[nodiscard]] inline ListenerAnchorId makeFormulaCellListenerAnchor(const ShadowCellRecord& rCell)
{
    return graphmapping::makeGraphFormulaCellListenerAnchorId(rCell.maId.maAddress);
}

[[nodiscard]] inline ListenerAnchorId
makeFormulaGroupListenerAnchor(const ShadowFormulaGroupRecord& rGroup)
{
    return graphmapping::makeGraphFormulaGroupListenerAnchorId(rGroup.maId);
}

[[nodiscard]] inline bool containsCellId(
    const std::vector<ShadowCellId>& rIds, const ShadowCellId& rId)
{
    return std::find(rIds.begin(), rIds.end(), rId) != rIds.end();
}

[[nodiscard]] inline GraphListenerAnchorRecord
makeFormulaListenerAnchorRecord(const GraphFormulaNodeRecord& rNode)
{
    GraphListenerAnchorRecord aRecord;
    aRecord.maId = rNode.maListenerAnchor;
    aRecord.moFormulaCell = rNode.maId;
    aRecord.mbInFormulaTree = rNode.mbInFormulaTree;
    aRecord.mbInFormulaTrack = rNode.mbInFormulaTrack;
    return aRecord;
}

[[nodiscard]] inline GraphListenerAnchorRecord
makeObservedListenerAnchorRecord(const ListenerAnchorId& rId)
{
    GraphListenerAnchorRecord aRecord;
    aRecord.maId = rId;
    return aRecord;
}

[[nodiscard]] inline GraphListenerAnchorRecord makeGroupListenerAnchorRecord(
    const GraphFormulaGroupNodeRecord& rNode, const std::vector<ShadowCellId>& rFormulaTreeNodes,
    const std::vector<ShadowCellId>& rFormulaTrackNodes)
{
    GraphListenerAnchorRecord aRecord;
    aRecord.maId = rNode.maListenerAnchor;
    aRecord.moFormulaGroup = rNode.maId;
    aRecord.mbInFormulaTree = std::any_of(rNode.maMembers.begin(), rNode.maMembers.end(),
        [&rFormulaTreeNodes](const ShadowCellId& rId) {
            return containsCellId(rFormulaTreeNodes, rId);
        });
    aRecord.mbInFormulaTrack = std::any_of(rNode.maMembers.begin(), rNode.maMembers.end(),
        [&rFormulaTrackNodes](const ShadowCellId& rId) {
            return containsCellId(rFormulaTrackNodes, rId);
        });
    return aRecord;
}

inline void mergeListenerAnchorRecord(
    std::vector<GraphListenerAnchorRecord>& rAnchors, GraphListenerAnchorRecord aRecord)
{
    auto it = std::find_if(rAnchors.begin(), rAnchors.end(),
        [&aRecord](const GraphListenerAnchorRecord& rExisting) {
            return rExisting.maId == aRecord.maId;
        });
    if (it == rAnchors.end())
    {
        rAnchors.push_back(std::move(aRecord));
        return;
    }

    if (!it->moFormulaCell && aRecord.moFormulaCell)
        it->moFormulaCell = aRecord.moFormulaCell;
    if (!it->moFormulaGroup && aRecord.moFormulaGroup)
        it->moFormulaGroup = aRecord.moFormulaGroup;
    it->mbInFormulaTree = it->mbInFormulaTree || aRecord.mbInFormulaTree;
    it->mbInFormulaTrack = it->mbInFormulaTrack || aRecord.mbInFormulaTrack;
}

[[nodiscard]] inline std::vector<ShadowCellId> collectFormulaSubsetNodes(
    const std::vector<api::CellAddress>& rAddresses)
{
    std::vector<ShadowCellId> aNodes;
    aNodes.reserve(rAddresses.size());
    for (const auto& rAddress : rAddresses)
        aNodes.push_back(ShadowCellId { rAddress });
    return aNodes;
}

} // namespace graphdetail

[[nodiscard]] inline DependencyGraphShadow buildDependencyGraphShadow(
    const ComputationalWorkbookShadow& rShadow,
    const ComputationalObservationState& rObservation = {})
{
    DependencyGraphShadow aGraph;
    aGraph.maSnapshot = rShadow.maSnapshot;
    aGraph.maGrammar = rShadow.maGrammar;
    aGraph.maFormulaTreeNodes = graphdetail::collectFormulaSubsetNodes(
        rObservation.maFormulaTree.empty() ? rShadow.maFormulaTree : rObservation.maFormulaTree);
    aGraph.maFormulaTrackNodes = graphdetail::collectFormulaSubsetNodes(
        rObservation.maFormulaTrack.empty() ? rShadow.maFormulaTrack : rObservation.maFormulaTrack);

    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;

            GraphFormulaNodeRecord aNode;
            aNode.maId = rCell.maId;
            aNode.moFormulaGroup = rCell.moFormulaGroup;
            aNode.maListenerAnchor = graphdetail::makeFormulaCellListenerAnchor(rCell);
            aNode.mbInFormulaTree = rCell.mbInFormulaTree;
            aNode.mbInFormulaTrack = rCell.mbInFormulaTrack;
            aGraph.maFormulaNodes.push_back(aNode);
            graphdetail::mergeListenerAnchorRecord(
                aGraph.maListenerAnchors, graphdetail::makeFormulaListenerAnchorRecord(aNode));
        }
    }
    std::sort(aGraph.maFormulaNodes.begin(), aGraph.maFormulaNodes.end(),
        [](const GraphFormulaNodeRecord& rLeft, const GraphFormulaNodeRecord& rRight) {
            return graphmapping::ShadowCellIdLess {}(rLeft.maId, rRight.maId);
        });

    for (const auto& rGroup : rShadow.maFormulaGroups)
    {
        GraphFormulaGroupNodeRecord aNode;
        aNode.maId = rGroup.maId;
        aNode.maListenerAnchor = graphdetail::makeFormulaGroupListenerAnchor(rGroup);
        aNode.maMembers = rGroup.maMembers;
        graphmapping::sortAndUnique(aNode.maMembers, graphmapping::ShadowCellIdLess {});
        aGraph.maFormulaGroupNodes.push_back(aNode);
    }
    std::sort(aGraph.maFormulaGroupNodes.begin(), aGraph.maFormulaGroupNodes.end(),
        [](const GraphFormulaGroupNodeRecord& rLeft, const GraphFormulaGroupNodeRecord& rRight) {
            return graphmapping::ShadowFormulaGroupIdLess {}(rLeft.maId, rRight.maId);
        });
    for (const auto& rNode : aGraph.maFormulaGroupNodes)
    {
        graphdetail::mergeListenerAnchorRecord(aGraph.maListenerAnchors,
            graphdetail::makeGroupListenerAnchorRecord(
                rNode, aGraph.maFormulaTreeNodes, aGraph.maFormulaTrackNodes));
    }

    for (const auto& rBroadcaster : rShadow.maCellBroadcasters)
    {
        GraphBroadcasterNodeRecord aNode;
        aNode.maId = BroadcasterNodeId::forCell(rBroadcaster.maBroadcaster);
        aNode.mnListenerCount = static_cast<sal_Int32>(rBroadcaster.maListeners.size());
        aGraph.maBroadcasterNodes.push_back(aNode);

        for (const auto& rListener : rBroadcaster.maListeners)
        {
            graphdetail::mergeListenerAnchorRecord(
                aGraph.maListenerAnchors, graphdetail::makeObservedListenerAnchorRecord(rListener));
            aGraph.maEdges.push_back({ aNode.maId, rListener });
        }
    }

    for (const auto& rBroadcaster : rShadow.maAreaBroadcasters)
    {
        GraphBroadcasterNodeRecord aNode;
        aNode.maId = BroadcasterNodeId::forArea(rBroadcaster.maBroadcaster);
        aNode.mnListenerCount = static_cast<sal_Int32>(rBroadcaster.maListeners.size());
        aGraph.maBroadcasterNodes.push_back(aNode);

        for (const auto& rListener : rBroadcaster.maListeners)
        {
            graphdetail::mergeListenerAnchorRecord(
                aGraph.maListenerAnchors, graphdetail::makeObservedListenerAnchorRecord(rListener));
            aGraph.maEdges.push_back({ aNode.maId, rListener });
        }
    }

    aGraph.maFormulaTreeNodes
        = graphmapping::normalizeFormulaSubsetNodes(std::move(aGraph.maFormulaTreeNodes));
    aGraph.maFormulaTrackNodes
        = graphmapping::normalizeFormulaSubsetNodes(std::move(aGraph.maFormulaTrackNodes));
    aGraph.maListenerAnchors
        = graphmapping::normalizeListenerAnchors(std::move(aGraph.maListenerAnchors));
    aGraph.maBroadcasterNodes
        = graphmapping::normalizeBroadcasterNodes(std::move(aGraph.maBroadcasterNodes));
    aGraph.maEdges = graphmapping::normalizeGraphEdges(std::move(aGraph.maEdges));

    return aGraph;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
