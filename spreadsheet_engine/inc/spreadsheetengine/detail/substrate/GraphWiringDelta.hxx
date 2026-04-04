/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0/. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>

#include <spreadsheetengine/detail/dependency/RecalcPlanner.hxx>
#include <spreadsheetengine/detail/substrate/AuthorityPilot.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowMapping.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilot.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilot.hxx>

namespace spreadsheetengine::detail::substrate
{

enum class GraphDeltaAction : std::uint8_t
{
    Add,
    Remove
};

enum class FormulaSubsetKind : std::uint8_t
{
    FormulaTree,
    FormulaTrack
};

struct FormulaSubsetDeltaRecord
{
    GraphDeltaAction meAction = GraphDeltaAction::Add;
    FormulaSubsetKind meSubset = FormulaSubsetKind::FormulaTree;
    ShadowCellId maNode;

    [[nodiscard]] constexpr bool operator==(const FormulaSubsetDeltaRecord& rOther) const = default;
};

struct BroadcasterNodeDeltaRecord
{
    GraphDeltaAction meAction = GraphDeltaAction::Add;
    BroadcasterNodeId maNode;
    sal_Int32 mnListenerCount = 0;

    [[nodiscard]] constexpr bool operator==(const BroadcasterNodeDeltaRecord& rOther) const = default;
};

struct ListenerEdgeDeltaRecord
{
    GraphDeltaAction meAction = GraphDeltaAction::Add;
    GraphEdgeRecord maEdge;

    [[nodiscard]] constexpr bool operator==(const ListenerEdgeDeltaRecord& rOther) const = default;
};

struct GraphWiringDelta
{
    facade::MutationEvent maMutation;
    std::vector<FormulaSubsetDeltaRecord> maFormulaTreeDeltas;
    std::vector<FormulaSubsetDeltaRecord> maFormulaTrackDeltas;
    std::vector<BroadcasterNodeDeltaRecord> maBroadcasterNodeDeltas;
    std::vector<ListenerEdgeDeltaRecord> maListenerEdgeDeltas;
    std::vector<ShadowCellId> maFormulaTreeAfter;
    std::vector<ShadowCellId> maFormulaTrackAfter;
    std::vector<GraphBroadcasterNodeRecord> maBroadcasterNodesAfter;
    std::vector<GraphEdgeRecord> maListenerEdgesAfter;
    dependency::RecalcPlan maRecalcPlan;

    [[nodiscard]] sal_Int32 getAddCount() const
    {
        return static_cast<sal_Int32>(
            std::count_if(maFormulaTreeDeltas.begin(), maFormulaTreeDeltas.end(),
                [](const FormulaSubsetDeltaRecord& rDelta) {
                    return rDelta.meAction == GraphDeltaAction::Add;
                })
            + std::count_if(maFormulaTrackDeltas.begin(), maFormulaTrackDeltas.end(),
                [](const FormulaSubsetDeltaRecord& rDelta) {
                    return rDelta.meAction == GraphDeltaAction::Add;
                })
            + std::count_if(maBroadcasterNodeDeltas.begin(), maBroadcasterNodeDeltas.end(),
                [](const BroadcasterNodeDeltaRecord& rDelta) {
                    return rDelta.meAction == GraphDeltaAction::Add;
                })
            + std::count_if(maListenerEdgeDeltas.begin(), maListenerEdgeDeltas.end(),
                [](const ListenerEdgeDeltaRecord& rDelta) {
                    return rDelta.meAction == GraphDeltaAction::Add;
                }));
    }

    [[nodiscard]] sal_Int32 getRemoveCount() const
    {
        return static_cast<sal_Int32>(
            std::count_if(maFormulaTreeDeltas.begin(), maFormulaTreeDeltas.end(),
                [](const FormulaSubsetDeltaRecord& rDelta) {
                    return rDelta.meAction == GraphDeltaAction::Remove;
                })
            + std::count_if(maFormulaTrackDeltas.begin(), maFormulaTrackDeltas.end(),
                [](const FormulaSubsetDeltaRecord& rDelta) {
                    return rDelta.meAction == GraphDeltaAction::Remove;
                })
            + std::count_if(maBroadcasterNodeDeltas.begin(), maBroadcasterNodeDeltas.end(),
                [](const BroadcasterNodeDeltaRecord& rDelta) {
                    return rDelta.meAction == GraphDeltaAction::Remove;
                })
            + std::count_if(maListenerEdgeDeltas.begin(), maListenerEdgeDeltas.end(),
                [](const ListenerEdgeDeltaRecord& rDelta) {
                    return rDelta.meAction == GraphDeltaAction::Remove;
                }));
    }
};

namespace graphdeltadetail
{

template <typename Item, typename Less, typename Emit>
inline void appendSetDifference(const std::vector<Item>& rLeft, const std::vector<Item>& rRight,
    Less aLess, Emit&& rEmit)
{
    auto itLeft = rLeft.begin();
    auto itRight = rRight.begin();
    while (itLeft != rLeft.end())
    {
        if (itRight == rRight.end())
        {
            rEmit(*itLeft++);
            continue;
        }

        if (aLess(*itLeft, *itRight))
        {
            rEmit(*itLeft++);
            continue;
        }

        if (aLess(*itRight, *itLeft))
        {
            ++itRight;
            continue;
        }

        ++itLeft;
        ++itRight;
    }
}

[[nodiscard]] inline std::vector<FormulaSubsetDeltaRecord> buildFormulaSubsetDeltas(
    const std::vector<ShadowCellId>& rBefore, const std::vector<ShadowCellId>& rAfter,
    FormulaSubsetKind eSubset)
{
    std::vector<FormulaSubsetDeltaRecord> aDeltas;
    appendSetDifference(rBefore, rAfter, graphmapping::ShadowCellIdLess {},
        [&aDeltas, eSubset](const ShadowCellId& rNode) {
            aDeltas.push_back({ GraphDeltaAction::Remove, eSubset, rNode });
        });
    appendSetDifference(rAfter, rBefore, graphmapping::ShadowCellIdLess {},
        [&aDeltas, eSubset](const ShadowCellId& rNode) {
            aDeltas.push_back({ GraphDeltaAction::Add, eSubset, rNode });
        });
    return aDeltas;
}

[[nodiscard]] inline std::vector<BroadcasterNodeDeltaRecord> buildBroadcasterNodeDeltas(
    const std::vector<GraphBroadcasterNodeRecord>& rBefore,
    const std::vector<GraphBroadcasterNodeRecord>& rAfter)
{
    std::vector<BroadcasterNodeDeltaRecord> aDeltas;
    appendSetDifference(rBefore, rAfter,
        [](const GraphBroadcasterNodeRecord& rLeft, const GraphBroadcasterNodeRecord& rRight) {
            return graphmapping::BroadcasterNodeIdLess {}(rLeft.maId, rRight.maId);
        },
        [&aDeltas](const GraphBroadcasterNodeRecord& rNode) {
            aDeltas.push_back({ GraphDeltaAction::Remove, rNode.maId, rNode.mnListenerCount });
        });
    appendSetDifference(rAfter, rBefore,
        [](const GraphBroadcasterNodeRecord& rLeft, const GraphBroadcasterNodeRecord& rRight) {
            return graphmapping::BroadcasterNodeIdLess {}(rLeft.maId, rRight.maId);
        },
        [&aDeltas](const GraphBroadcasterNodeRecord& rNode) {
            aDeltas.push_back({ GraphDeltaAction::Add, rNode.maId, rNode.mnListenerCount });
        });
    return aDeltas;
}

[[nodiscard]] inline std::vector<ListenerEdgeDeltaRecord> buildListenerEdgeDeltas(
    const std::vector<GraphEdgeRecord>& rBefore, const std::vector<GraphEdgeRecord>& rAfter)
{
    std::vector<ListenerEdgeDeltaRecord> aDeltas;
    appendSetDifference(rBefore, rAfter, graphmapping::GraphEdgeRecordLess {},
        [&aDeltas](const GraphEdgeRecord& rEdge) {
            aDeltas.push_back({ GraphDeltaAction::Remove, rEdge });
        });
    appendSetDifference(rAfter, rBefore, graphmapping::GraphEdgeRecordLess {},
        [&aDeltas](const GraphEdgeRecord& rEdge) {
            aDeltas.push_back({ GraphDeltaAction::Add, rEdge });
        });
    return aDeltas;
}

} // namespace graphdeltadetail

[[nodiscard]] inline GraphWiringDelta buildGraphWiringDelta(
    const DependencyGraphShadow& rBefore, const DependencyGraphShadow& rAfter,
    const dependency::RecalcPlan& rRecalcPlan, const facade::MutationEvent& rMutation)
{
    GraphWiringDelta aDelta;
    aDelta.maMutation = rMutation;
    aDelta.maFormulaTreeDeltas = graphdeltadetail::buildFormulaSubsetDeltas(
        rBefore.maFormulaTreeNodes, rAfter.maFormulaTreeNodes, FormulaSubsetKind::FormulaTree);
    aDelta.maFormulaTrackDeltas = graphdeltadetail::buildFormulaSubsetDeltas(
        rBefore.maFormulaTrackNodes, rAfter.maFormulaTrackNodes, FormulaSubsetKind::FormulaTrack);
    aDelta.maBroadcasterNodeDeltas = graphdeltadetail::buildBroadcasterNodeDeltas(
        rBefore.maBroadcasterNodes, rAfter.maBroadcasterNodes);
    aDelta.maListenerEdgeDeltas = graphdeltadetail::buildListenerEdgeDeltas(
        rBefore.maEdges, rAfter.maEdges);
    aDelta.maFormulaTreeAfter = rAfter.maFormulaTreeNodes;
    aDelta.maFormulaTrackAfter = rAfter.maFormulaTrackNodes;
    aDelta.maBroadcasterNodesAfter = rAfter.maBroadcasterNodes;
    aDelta.maListenerEdgesAfter = rAfter.maEdges;
    aDelta.maRecalcPlan = rRecalcPlan;
    return aDelta;
}

[[nodiscard]] inline GraphWiringDelta buildGraphWiringDelta(
    const AuthorityPilotTransition& rTransition)
{
    return buildGraphWiringDelta(rTransition.maInput.maGraphShadow, rTransition.maGraphAfter,
        rTransition.maRecalcPlan, rTransition.maInput.maMutation);
}

[[nodiscard]] inline GraphWiringDelta buildGraphWiringDelta(
    const LifecyclePilotTransition& rTransition)
{
    return buildGraphWiringDelta(rTransition.maInput.maGraphShadow, rTransition.maGraphAfter,
        rTransition.maRecalcPlan, rTransition.maInput.maMutation);
}

[[nodiscard]] inline GraphWiringDelta buildGraphWiringDelta(
    const StructuralPilotTransition& rTransition)
{
    return buildGraphWiringDelta(rTransition.maInput.maGraphShadow, rTransition.maGraphAfter,
        rTransition.maRecalcPlan, rTransition.maInput.maMutation);
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
