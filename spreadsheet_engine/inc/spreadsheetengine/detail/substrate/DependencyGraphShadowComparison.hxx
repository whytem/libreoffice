/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/substrate/DependencyGraphShadowBuilder.hxx>

namespace spreadsheetengine::detail::substrate
{

struct DependencyGraphShadowComparison
{
    graphmapping::GraphComparisonKind meKind = graphmapping::GraphComparisonKind::Mismatch;
    bool mbFormulaNodeMatch = false;
    bool mbFormulaGroupNodeMatch = false;
    bool mbListenerAnchorMatch = false;
    bool mbBroadcasterNodeMatch = false;
    bool mbEdgeMatch = false;
    bool mbFormulaTreeExactMatch = false;
    bool mbFormulaTrackExactMatch = false;
    bool mbFormulaTreeNormalizedMatch = false;
    bool mbFormulaTrackNormalizedMatch = false;
    bool mbFullMatch = false;

    [[nodiscard]] constexpr bool operator==(const DependencyGraphShadowComparison& rOther) const
        = default;
};

namespace graphcmpdetail
{

[[nodiscard]] inline std::vector<ShadowCellId> collectOrderedFormulaSubset(
    const std::vector<api::CellAddress>& rAddresses)
{
    std::vector<ShadowCellId> aNodes;
    aNodes.reserve(rAddresses.size());
    for (const auto& rAddress : rAddresses)
        aNodes.push_back({ rAddress });
    return aNodes;
}

} // namespace graphcmpdetail

[[nodiscard]] inline DependencyGraphShadowComparison compareDependencyGraphShadow(
    const DependencyGraphShadow& rGraph,
    const ComputationalWorkbookShadow& rShadow,
    const ComputationalObservationState& rObservation = {})
{
    const auto aExpected = buildDependencyGraphShadow(rShadow, rObservation);
    const auto aExpectedTree = graphcmpdetail::collectOrderedFormulaSubset(
        rObservation.maFormulaTree.empty() ? rShadow.maFormulaTree : rObservation.maFormulaTree);
    const auto aExpectedTrack = graphcmpdetail::collectOrderedFormulaSubset(
        rObservation.maFormulaTrack.empty() ? rShadow.maFormulaTrack : rObservation.maFormulaTrack);

    DependencyGraphShadowComparison aComparison;
    aComparison.mbFormulaNodeMatch = rGraph.maFormulaNodes == aExpected.maFormulaNodes;
    aComparison.mbFormulaGroupNodeMatch = rGraph.maFormulaGroupNodes == aExpected.maFormulaGroupNodes;
    aComparison.mbListenerAnchorMatch = rGraph.maListenerAnchors == aExpected.maListenerAnchors;
    aComparison.mbBroadcasterNodeMatch = rGraph.maBroadcasterNodes == aExpected.maBroadcasterNodes;
    aComparison.mbEdgeMatch = rGraph.maEdges == aExpected.maEdges;
    aComparison.mbFormulaTreeExactMatch = rGraph.maFormulaTreeNodes == aExpectedTree;
    aComparison.mbFormulaTrackExactMatch = rGraph.maFormulaTrackNodes == aExpectedTrack;
    aComparison.mbFormulaTreeNormalizedMatch = graphmapping::hasNormalizedFormulaSubsetEquivalence(
        rGraph.maFormulaTreeNodes, aExpectedTree);
    aComparison.mbFormulaTrackNormalizedMatch = graphmapping::hasNormalizedFormulaSubsetEquivalence(
        rGraph.maFormulaTrackNodes, aExpectedTrack);

    const bool bCoreMatch = aComparison.mbFormulaNodeMatch && aComparison.mbFormulaGroupNodeMatch
        && aComparison.mbListenerAnchorMatch && aComparison.mbBroadcasterNodeMatch
        && aComparison.mbEdgeMatch;
    aComparison.mbFullMatch = bCoreMatch && aComparison.mbFormulaTreeNormalizedMatch
        && aComparison.mbFormulaTrackNormalizedMatch;

    if (!aComparison.mbFullMatch)
        aComparison.meKind = graphmapping::GraphComparisonKind::Mismatch;
    else if (aComparison.mbFormulaTreeExactMatch && aComparison.mbFormulaTrackExactMatch)
        aComparison.meKind = graphmapping::GraphComparisonKind::Exact;
    else
        aComparison.meKind = graphmapping::GraphComparisonKind::NormalizedEquivalent;

    return aComparison;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
