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
#include <optional>
#include <vector>

#include <spreadsheetengine/detail/substrate/ComputationalShadow.hxx>

namespace spreadsheetengine::detail::substrate
{

enum class BroadcasterNodeKind : std::uint8_t
{
    Cell,
    Area
};

struct BroadcasterNodeId
{
    BroadcasterNodeKind meKind = BroadcasterNodeKind::Cell;
    api::CellAddress maCellAddress;
    api::CellRange maAreaRange;

    [[nodiscard]] static constexpr BroadcasterNodeId forCell(const api::CellAddress& rAddress)
    {
        BroadcasterNodeId aId;
        aId.meKind = BroadcasterNodeKind::Cell;
        aId.maCellAddress = rAddress;
        return aId;
    }

    [[nodiscard]] static constexpr BroadcasterNodeId forArea(const api::CellRange& rRange)
    {
        BroadcasterNodeId aId;
        aId.meKind = BroadcasterNodeKind::Area;
        aId.maAreaRange = rRange;
        return aId;
    }

    [[nodiscard]] constexpr bool operator==(const BroadcasterNodeId& rOther) const = default;
};

struct GraphFormulaNodeRecord
{
    ShadowCellId maId;
    std::optional<ShadowFormulaGroupId> moFormulaGroup;
    ListenerAnchorId maListenerAnchor;
    bool mbInFormulaTree = false;
    bool mbInFormulaTrack = false;

    [[nodiscard]] constexpr bool operator==(const GraphFormulaNodeRecord& rOther) const = default;
};

struct GraphFormulaGroupNodeRecord
{
    ShadowFormulaGroupId maId;
    ListenerAnchorId maListenerAnchor;
    std::vector<ShadowCellId> maMembers;

    [[nodiscard]] constexpr bool operator==(const GraphFormulaGroupNodeRecord& rOther) const
        = default;
};

struct GraphListenerAnchorRecord
{
    ListenerAnchorId maId;
    std::optional<ShadowCellId> moFormulaCell;
    std::optional<ShadowFormulaGroupId> moFormulaGroup;
    bool mbInFormulaTree = false;
    bool mbInFormulaTrack = false;

    [[nodiscard]] constexpr bool operator==(const GraphListenerAnchorRecord& rOther) const
        = default;
};

struct GraphBroadcasterNodeRecord
{
    BroadcasterNodeId maId;
    sal_Int32 mnListenerCount = 0;

    [[nodiscard]] constexpr bool operator==(const GraphBroadcasterNodeRecord& rOther) const
        = default;
};

struct GraphEdgeRecord
{
    BroadcasterNodeId maBroadcaster;
    ListenerAnchorId maListenerAnchor;

    [[nodiscard]] constexpr bool operator==(const GraphEdgeRecord& rOther) const = default;
};

struct DependencyGraphShadow
{
    facade::WorkbookSnapshotInfo maSnapshot;
    api::Grammar maGrammar;
    std::vector<GraphFormulaNodeRecord> maFormulaNodes;
    std::vector<GraphFormulaGroupNodeRecord> maFormulaGroupNodes;
    std::vector<GraphListenerAnchorRecord> maListenerAnchors;
    std::vector<GraphBroadcasterNodeRecord> maBroadcasterNodes;
    std::vector<GraphEdgeRecord> maEdges;
    std::vector<ShadowCellId> maFormulaTreeNodes;
    std::vector<ShadowCellId> maFormulaTrackNodes;

    [[nodiscard]] constexpr bool operator==(const DependencyGraphShadow& rOther) const = default;

    [[nodiscard]] sal_Int32 getFormulaNodeCount() const
    {
        return static_cast<sal_Int32>(maFormulaNodes.size());
    }

    [[nodiscard]] sal_Int32 getListenerAnchorCount() const
    {
        return static_cast<sal_Int32>(maListenerAnchors.size());
    }

    [[nodiscard]] sal_Int32 getBroadcasterNodeCount() const
    {
        return static_cast<sal_Int32>(maBroadcasterNodes.size());
    }

    [[nodiscard]] sal_Int32 getEdgeCount() const
    {
        return static_cast<sal_Int32>(maEdges.size());
    }

    [[nodiscard]] const GraphListenerAnchorRecord* findListenerAnchor(
        const ListenerAnchorId& rId) const
    {
        auto it = std::find_if(maListenerAnchors.begin(), maListenerAnchors.end(),
            [&rId](const GraphListenerAnchorRecord& rAnchor) { return rAnchor.maId == rId; });
        return it == maListenerAnchors.end() ? nullptr : &*it;
    }

    [[nodiscard]] const GraphBroadcasterNodeRecord* findBroadcasterNode(
        const BroadcasterNodeId& rId) const
    {
        auto it = std::find_if(maBroadcasterNodes.begin(), maBroadcasterNodes.end(),
            [&rId](const GraphBroadcasterNodeRecord& rNode) { return rNode.maId == rId; });
        return it == maBroadcasterNodes.end() ? nullptr : &*it;
    }
};

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
