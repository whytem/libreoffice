/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <vector>

#include <spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx>

namespace spreadsheetengine::detail::substrate::graphmapping
{

enum class GraphComparisonKind : std::uint8_t
{
    Exact,
    NormalizedEquivalent,
    Mismatch
};

struct ShadowCellIdLess
{
    [[nodiscard]] bool operator()(const ShadowCellId& rLeft, const ShadowCellId& rRight) const
    {
        if (rLeft.maAddress.mnSheet != rRight.maAddress.mnSheet)
            return rLeft.maAddress.mnSheet < rRight.maAddress.mnSheet;
        if (rLeft.maAddress.mnColumn != rRight.maAddress.mnColumn)
            return rLeft.maAddress.mnColumn < rRight.maAddress.mnColumn;
        return rLeft.maAddress.mnRow < rRight.maAddress.mnRow;
    }
};

struct ShadowFormulaGroupIdLess
{
    [[nodiscard]] bool operator()(const ShadowFormulaGroupId& rLeft,
        const ShadowFormulaGroupId& rRight) const
    {
        if (rLeft.maAnchor != rRight.maAnchor)
            return ShadowCellIdLess {}({ rLeft.maAnchor }, { rRight.maAnchor });
        return rLeft.mnLength < rRight.mnLength;
    }
};

struct ListenerAnchorIdLess
{
    [[nodiscard]] bool operator()(const ListenerAnchorId& rLeft,
        const ListenerAnchorId& rRight) const
    {
        if (rLeft.meKind != rRight.meKind)
            return rLeft.meKind < rRight.meKind;
        if (!(rLeft.maAnchor == rRight.maAnchor))
            return ShadowCellIdLess {}({ rLeft.maAnchor }, { rRight.maAnchor });
        return rLeft.mnLength < rRight.mnLength;
    }
};

struct BroadcasterNodeIdLess
{
    [[nodiscard]] bool operator()(const BroadcasterNodeId& rLeft,
        const BroadcasterNodeId& rRight) const
    {
        if (rLeft.meKind != rRight.meKind)
            return rLeft.meKind < rRight.meKind;

        if (rLeft.meKind == BroadcasterNodeKind::Cell)
            return ShadowCellIdLess {}({ rLeft.maCellAddress }, { rRight.maCellAddress });

        if (rLeft.maAreaRange.maStart != rRight.maAreaRange.maStart)
            return ShadowCellIdLess {}({ rLeft.maAreaRange.maStart }, { rRight.maAreaRange.maStart });
        return ShadowCellIdLess {}({ rLeft.maAreaRange.maEnd }, { rRight.maAreaRange.maEnd });
    }
};

struct GraphEdgeRecordLess
{
    [[nodiscard]] bool operator()(const GraphEdgeRecord& rLeft, const GraphEdgeRecord& rRight) const
    {
        if (!(rLeft.maBroadcaster == rRight.maBroadcaster))
            return BroadcasterNodeIdLess {}(rLeft.maBroadcaster, rRight.maBroadcaster);
        return ListenerAnchorIdLess {}(rLeft.maListenerAnchor, rRight.maListenerAnchor);
    }
};

[[nodiscard]] inline ListenerAnchorId
makeGraphFormulaCellListenerAnchorId(const api::CellAddress& rAddress)
{
    return ListenerAnchorId { ListenerAnchorKind::FormulaCell, rAddress, 1 };
}

[[nodiscard]] inline ListenerAnchorId
makeGraphFormulaGroupListenerAnchorId(const ShadowFormulaGroupId& rId)
{
    return ListenerAnchorId { ListenerAnchorKind::FormulaGroup, rId.maAnchor, rId.mnLength };
}

template <typename Container, typename Less>
void sortAndUnique(Container& rContainer, Less aLess)
{
    std::sort(rContainer.begin(), rContainer.end(), aLess);
    rContainer.erase(std::unique(rContainer.begin(), rContainer.end()), rContainer.end());
}

[[nodiscard]] inline std::vector<ShadowCellId> normalizeFormulaSubsetNodes(
    std::vector<ShadowCellId> aNodes)
{
    sortAndUnique(aNodes, ShadowCellIdLess {});
    return aNodes;
}

[[nodiscard]] inline std::vector<GraphListenerAnchorRecord> normalizeListenerAnchors(
    std::vector<GraphListenerAnchorRecord> aAnchors)
{
    sortAndUnique(aAnchors,
        [](const GraphListenerAnchorRecord& rLeft, const GraphListenerAnchorRecord& rRight) {
            return ListenerAnchorIdLess {}(rLeft.maId, rRight.maId);
        });
    return aAnchors;
}

[[nodiscard]] inline std::vector<GraphBroadcasterNodeRecord> normalizeBroadcasterNodes(
    std::vector<GraphBroadcasterNodeRecord> aNodes)
{
    sortAndUnique(aNodes,
        [](const GraphBroadcasterNodeRecord& rLeft, const GraphBroadcasterNodeRecord& rRight) {
            return BroadcasterNodeIdLess {}(rLeft.maId, rRight.maId);
        });
    return aNodes;
}

[[nodiscard]] inline std::vector<GraphEdgeRecord> normalizeGraphEdges(
    std::vector<GraphEdgeRecord> aEdges)
{
    sortAndUnique(aEdges, GraphEdgeRecordLess {});
    return aEdges;
}

[[nodiscard]] inline bool hasNormalizedFormulaSubsetEquivalence(
    std::vector<ShadowCellId> aLeft, std::vector<ShadowCellId> aRight)
{
    return normalizeFormulaSubsetNodes(std::move(aLeft))
           == normalizeFormulaSubsetNodes(std::move(aRight));
}

[[nodiscard]] inline bool hasNormalizedGraphEdgeEquivalence(
    std::vector<GraphEdgeRecord> aLeft, std::vector<GraphEdgeRecord> aRight)
{
    return normalizeGraphEdges(std::move(aLeft)) == normalizeGraphEdges(std::move(aRight));
}

} // namespace spreadsheetengine::detail::substrate::graphmapping

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
