/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0/. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <vector>

#include <column.hxx>
#include <document.hxx>
#include <dociter.hxx>
#include <formulacell.hxx>
#include <listenercontext.hxx>
#include <sharedformula.hxx>
#include <table.hxx>

#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/detail/substrate/GraphWiringDelta.hxx>
#include <spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx>

namespace spreadsheetengine::compat::libreoffice::substratewiring
{

enum class WiringApplyResultKind : sal_uInt8
{
    Applied,
    RejectedOutOfContract
};

struct WiringApplyResult
{
    WiringApplyResultKind meKind = WiringApplyResultKind::RejectedOutOfContract;
    api::String maReason;
    sal_Int32 mnBroadcasterNodesRealized = 0;
    sal_Int32 mnListenerEdgesApplied = 0;
    sal_Int32 mnFormulaTreeNodesApplied = 0;
    sal_Int32 mnFormulaTrackNodesApplied = 0;
};

namespace detail
{

[[nodiscard]] inline bool isAdmittedFormulaCell(const ScFormulaCell& rCell)
{
    return rCell.GetMatrixFlag() == ScMatrixMode::NONE;
}

[[nodiscard]] inline bool collectAdmittedFormulaCells(
    ScDocument& rDoc, std::vector<ScFormulaCell*>& rCells, api::String& rReason)
{
    for (SCTAB nTab = 0; nTab < rDoc.GetTableCount(); ++nTab)
    {
        ScCellIterator aIter(rDoc, ScRange(0, 0, nTab, rDoc.MaxCol(), rDoc.MaxRow(), nTab));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            if (aIter.getType() != CELLTYPE_FORMULA)
                continue;

            ScFormulaCell* pCell = aIter.getFormulaCell();
            if (!pCell)
                continue;
            if (!isAdmittedFormulaCell(*pCell))
            {
                rReason = u"formula_shape_out_of_contract";
                return false;
            }

            rCells.push_back(pCell);
        }
    }

    return true;
}

inline void clearAdmittedLiveWiring(ScDocument& rDoc, const std::vector<ScFormulaCell*>& rCells)
{
    sc::EndListeningContext aEndContext(rDoc);
    for (ScFormulaCell* pCell : rCells)
        pCell->EndListeningTo(aEndContext);
    aEndContext.purgeEmptyBroadcasters();

    for (ScFormulaCell* pCell : rCells)
    {
        if (rDoc.IsInFormulaTrack(pCell))
            rDoc.RemoveFromFormulaTrack(pCell);
    }
    rDoc.ClearFormulaTree();
}

[[nodiscard]] inline ScFormulaCell* resolveFormulaCell(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::ListenerAnchorId& rAnchor,
    api::String& rReason)
{
    if (rAnchor.meKind != spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaCell)
    {
        rReason = u"listener_anchor_out_of_contract";
        return nullptr;
    }

    ScFormulaCell* pCell = rDoc.GetFormulaCell(toLibreOfficeAddress(rAnchor.maAnchor));
    if (!pCell)
    {
        rReason = u"missing_formula_cell_anchor";
        return nullptr;
    }
    if (!isAdmittedFormulaCell(*pCell))
    {
        rReason = u"formula_shape_out_of_contract";
        return nullptr;
    }

    return pCell;
}

struct ResolvedFormulaGroupAnchor
{
    spreadsheetengine::detail::substrate::ListenerAnchorId maAnchor;
    ScFormulaCell* mpTopCell = nullptr;
    ScFormulaCell** mppSharedTop = nullptr;
};

[[nodiscard]] inline ResolvedFormulaGroupAnchor resolveFormulaGroupAnchor(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::ListenerAnchorId& rAnchor,
    api::String& rReason)
{
    ResolvedFormulaGroupAnchor aResolved;
    aResolved.maAnchor = rAnchor;

    if (rAnchor.meKind != spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaGroup)
    {
        rReason = u"listener_anchor_out_of_contract";
        return aResolved;
    }

    if (rAnchor.mnLength <= 0)
    {
        rReason = u"missing_formula_group_anchor";
        return aResolved;
    }

    ScFormulaCell* pTopCell = rDoc.GetFormulaCell(toLibreOfficeAddress(rAnchor.maAnchor));
    if (!pTopCell)
    {
        rReason = u"missing_formula_group_anchor";
        return aResolved;
    }
    if (!isAdmittedFormulaCell(*pTopCell))
    {
        rReason = u"formula_shape_out_of_contract";
        return aResolved;
    }
    if (!pTopCell->IsSharedTop())
    {
        rReason = u"formula_group_anchor_not_shared_top";
        return aResolved;
    }
    if (pTopCell->GetSharedLength() != rAnchor.mnLength)
    {
        rReason = u"formula_group_anchor_length_mismatch";
        return aResolved;
    }

    ScTable* pTable = rDoc.FetchTable(rAnchor.maAnchor.mnSheet);
    if (!pTable)
    {
        rReason = u"missing_formula_group_anchor_table";
        return aResolved;
    }

    ScColumn* pColumn = &pTable->CreateColumnIfNotExists(rAnchor.maAnchor.mnColumn);

    size_t nBlockSize = 0;
    ScFormulaCell* const* ppTopCell = pColumn->GetFormulaCellBlockAddress(rAnchor.maAnchor.mnRow, nBlockSize);
    if (!ppTopCell)
    {
        rReason = u"missing_formula_group_anchor_block";
        return aResolved;
    }
    if (*ppTopCell != pTopCell)
    {
        rReason = u"formula_group_anchor_block_mismatch";
        return aResolved;
    }
    if (nBlockSize < static_cast<size_t>(rAnchor.mnLength))
    {
        rReason = u"formula_group_anchor_block_too_short";
        return aResolved;
    }

    aResolved.mpTopCell = pTopCell;
    aResolved.mppSharedTop = const_cast<ScFormulaCell**>(ppTopCell);
    return aResolved;
}

[[nodiscard]] inline ScFormulaCell* resolveFormulaCell(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::ShadowCellId& rNode,
    api::String& rReason)
{
    ScFormulaCell* pCell = rDoc.GetFormulaCell(toLibreOfficeAddress(rNode.maAddress));
    if (!pCell)
    {
        rReason = u"missing_formula_cell_node";
        return nullptr;
    }
    if (!isAdmittedFormulaCell(*pCell))
    {
        rReason = u"formula_shape_out_of_contract";
        return nullptr;
    }

    return pCell;
}

inline void applyListenerEdgeTarget(ScDocument& rDoc,
    const spreadsheetengine::detail::substrate::GraphEdgeRecord& rEdge, api::String& rReason)
{
    ScFormulaCell* pCell = resolveFormulaCell(rDoc, rEdge.maListenerAnchor, rReason);
    if (!pCell)
        return;

    switch (rEdge.maBroadcaster.meKind)
    {
        case spreadsheetengine::detail::substrate::BroadcasterNodeKind::Cell:
            rDoc.StartListeningCell(toLibreOfficeAddress(rEdge.maBroadcaster.maCellAddress), pCell);
            break;
        case spreadsheetengine::detail::substrate::BroadcasterNodeKind::Area:
            rDoc.StartListeningArea(toLibreOfficeRange(rEdge.maBroadcaster.maAreaRange), false, pCell);
            break;
    }
}

[[nodiscard]] inline bool validateListenerAnchor(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::ListenerAnchorId& rAnchor,
    api::String& rReason)
{
    switch (rAnchor.meKind)
    {
        case spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaCell:
            return resolveFormulaCell(rDoc, rAnchor, rReason) != nullptr;
        case spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaGroup:
            return resolveFormulaGroupAnchor(rDoc, rAnchor, rReason).mpTopCell != nullptr;
        case spreadsheetengine::detail::substrate::ListenerAnchorKind::HostUnknown:
            rReason = u"listener_anchor_out_of_contract";
            return false;
    }

    rReason = u"listener_anchor_out_of_contract";
    return false;
}

[[nodiscard]] inline bool containsAnchor(
    const std::vector<ResolvedFormulaGroupAnchor>& rAnchors,
    const spreadsheetengine::detail::substrate::ListenerAnchorId& rAnchor)
{
    return std::any_of(rAnchors.begin(), rAnchors.end(),
        [&rAnchor](const ResolvedFormulaGroupAnchor& rResolved) {
            return rResolved.maAnchor == rAnchor;
        });
}

[[nodiscard]] inline std::vector<ResolvedFormulaGroupAnchor> collectResolvedFormulaGroupAnchors(
    ScDocument& rDoc,
    const spreadsheetengine::detail::substrate::AdmittedWiringContainers& rStore,
    api::String& rReason)
{
    std::vector<ResolvedFormulaGroupAnchor> aAnchors;
    for (const auto& rEdge : rStore.maListenerEdges)
    {
        if (rEdge.maListenerAnchor.meKind
            != spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaGroup)
        {
            continue;
        }

        if (containsAnchor(aAnchors, rEdge.maListenerAnchor))
            continue;

        auto aResolved = resolveFormulaGroupAnchor(rDoc, rEdge.maListenerAnchor, rReason);
        if (!aResolved.mpTopCell)
            return {};

        aAnchors.push_back(aResolved);
    }

    return aAnchors;
}

inline void applyFormulaGroupListenerAnchors(ScDocument& rDoc,
    const std::vector<ResolvedFormulaGroupAnchor>& rAnchors, api::String& rReason)
{
    sc::StartListeningContext aContext(rDoc);
    for (const auto& rAnchor : rAnchors)
    {
        if (!rAnchor.mpTopCell || !rAnchor.mppSharedTop)
        {
            rReason = u"missing_formula_group_anchor";
            return;
        }

        sc::SharedFormulaUtil::startListeningAsGroup(aContext, rAnchor.mppSharedTop);
    }
}

[[nodiscard]] inline bool formulaCellBelongsToFormulaGroupAnchor(
    const ScFormulaCell& rCell,
    const std::vector<ResolvedFormulaGroupAnchor>& rAnchors)
{
    if (!rCell.GetCellGroup())
        return false;

    const api::CellAddress aTopAddress
        = toApiCellAddress(ScAddress(rCell.aPos.Col(), rCell.GetSharedTopRow(), rCell.aPos.Tab()));
    const auto aAnchor = spreadsheetengine::detail::substrate::ListenerAnchorId {
        spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaGroup, aTopAddress,
        rCell.GetSharedLength()
    };
    return containsAnchor(rAnchors, aAnchor);
}

[[nodiscard]] inline const ResolvedFormulaGroupAnchor* findFormulaGroupAnchorForCell(
    const ScFormulaCell& rCell, const std::vector<ResolvedFormulaGroupAnchor>& rAnchors)
{
    if (!rCell.GetCellGroup())
        return nullptr;

    const api::CellAddress aTopAddress
        = toApiCellAddress(ScAddress(rCell.aPos.Col(), rCell.GetSharedTopRow(), rCell.aPos.Tab()));
    const auto aAnchor = spreadsheetengine::detail::substrate::ListenerAnchorId {
        spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaGroup, aTopAddress,
        rCell.GetSharedLength()
    };
    const auto it = std::find_if(rAnchors.begin(), rAnchors.end(),
        [&aAnchor](const ResolvedFormulaGroupAnchor& rResolved) {
            return rResolved.maAnchor == aAnchor;
        });
    return it == rAnchors.end() ? nullptr : &*it;
}

[[nodiscard]] inline bool hasParallelFormulaGroupEdge(
    const spreadsheetengine::detail::substrate::AdmittedWiringContainers& rStore,
    const spreadsheetengine::detail::substrate::BroadcasterNodeId& rBroadcaster,
    const spreadsheetengine::detail::substrate::ListenerAnchorId& rGroupAnchor)
{
    return std::any_of(rStore.maListenerEdges.begin(), rStore.maListenerEdges.end(),
        [&rBroadcaster, &rGroupAnchor](
            const spreadsheetengine::detail::substrate::GraphEdgeRecord& rEdge) {
            return rEdge.maBroadcaster == rBroadcaster && rEdge.maListenerAnchor == rGroupAnchor;
        });
}

[[nodiscard]] inline bool shouldRetainFormulaCellEdgeAlongsideFormulaGroup(
    const spreadsheetengine::detail::substrate::AdmittedWiringContainers& rStore,
    const spreadsheetengine::detail::substrate::GraphEdgeRecord& rEdge,
    const ScFormulaCell&, const ResolvedFormulaGroupAnchor& rResolvedGroup)
{
    if (!rResolvedGroup.mpTopCell)
        return false;

    const auto aTopCellAnchor = spreadsheetengine::detail::substrate::ListenerAnchorId {
        spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaCell,
        toApiCellAddress(rResolvedGroup.mpTopCell->aPos), 1
    };
    if (rEdge.maListenerAnchor != aTopCellAnchor)
        return false;

    return hasParallelFormulaGroupEdge(rStore, rEdge.maBroadcaster, rResolvedGroup.maAnchor);
}

[[nodiscard]] inline bool validateResidentWiringStore(
    ScDocument& rDoc,
    const spreadsheetengine::detail::substrate::AdmittedWiringContainers& rStore,
    api::String& rReason)
{
    for (const auto& rEdge : rStore.maListenerEdges)
    {
        if (!rStore.findBroadcasterNode(rEdge.maBroadcaster))
        {
            rReason = u"wiring_store_missing_broadcaster";
            return false;
        }
        if (!validateListenerAnchor(rDoc, rEdge.maListenerAnchor, rReason))
            return false;
    }

    for (const auto& rNode : rStore.maFormulaTreeNodes)
    {
        if (!resolveFormulaCell(rDoc, rNode, rReason))
            return false;
    }

    for (const auto& rNode : rStore.maFormulaTrackNodes)
    {
        if (!resolveFormulaCell(rDoc, rNode, rReason))
            return false;
    }

    for (const auto& rBroadcaster : rStore.maBroadcasterNodes)
    {
        const sal_Int32 nObservedListeners = static_cast<sal_Int32>(
            std::count_if(rStore.maListenerEdges.begin(), rStore.maListenerEdges.end(),
                [&rBroadcaster](const spreadsheetengine::detail::substrate::GraphEdgeRecord& rEdge) {
                    return rEdge.maBroadcaster == rBroadcaster.maId;
                }));
        if (nObservedListeners != rBroadcaster.mnListenerCount)
        {
            rReason = u"wiring_store_listener_count_mismatch";
            return false;
        }
    }

    return true;
}

} // namespace detail

[[nodiscard]] inline WiringApplyResult realizeAdmittedWiringContainers(
    ScDocument& rDoc,
    const spreadsheetengine::detail::substrate::AdmittedWiringContainers& rStore)
{
    WiringApplyResult aResult;

    std::vector<ScFormulaCell*> aCells;
    if (!detail::collectAdmittedFormulaCells(rDoc, aCells, aResult.maReason))
        return aResult;
    if (!detail::validateResidentWiringStore(rDoc, rStore, aResult.maReason))
        return aResult;

    const auto aResolvedFormulaGroups
        = detail::collectResolvedFormulaGroupAnchors(rDoc, rStore, aResult.maReason);
    if (!aResult.maReason.empty())
        return aResult;

    detail::clearAdmittedLiveWiring(rDoc, aCells);
    detail::applyFormulaGroupListenerAnchors(rDoc, aResolvedFormulaGroups, aResult.maReason);
    if (!aResult.maReason.empty())
        return aResult;

    aResult.mnBroadcasterNodesRealized = rStore.getBroadcasterNodeCount();
    for (const auto& rEdge : rStore.maListenerEdges)
    {
        if (rEdge.maListenerAnchor.meKind
            == spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaGroup)
        {
            ++aResult.mnListenerEdgesApplied;
            continue;
        }

        if (!aResolvedFormulaGroups.empty())
        {
            ScFormulaCell* pCell
                = detail::resolveFormulaCell(rDoc, rEdge.maListenerAnchor, aResult.maReason);
            if (!pCell)
                return aResult;
            if (const auto* pResolvedGroup
                = detail::findFormulaGroupAnchorForCell(*pCell, aResolvedFormulaGroups))
            {
                if (!detail::shouldRetainFormulaCellEdgeAlongsideFormulaGroup(
                        rStore, rEdge, *pCell, *pResolvedGroup))
                {
                    ++aResult.mnListenerEdgesApplied;
                    continue;
                }
            }
        }

        detail::applyListenerEdgeTarget(rDoc, rEdge, aResult.maReason);
        if (!aResult.maReason.empty())
            return aResult;
        ++aResult.mnListenerEdgesApplied;
    }

    for (const auto& rNode : rStore.maFormulaTreeNodes)
    {
        ScFormulaCell* pCell = detail::resolveFormulaCell(rDoc, rNode, aResult.maReason);
        if (!pCell)
            return aResult;
        rDoc.PutInFormulaTree(pCell);
        ++aResult.mnFormulaTreeNodesApplied;
    }

    for (const auto& rNode : rStore.maFormulaTrackNodes)
    {
        ScFormulaCell* pCell = detail::resolveFormulaCell(rDoc, rNode, aResult.maReason);
        if (!pCell)
            return aResult;
        rDoc.AppendToFormulaTrack(pCell);
        ++aResult.mnFormulaTrackNodesApplied;
    }

    aResult.meKind = WiringApplyResultKind::Applied;
    return aResult;
}

[[nodiscard]] inline WiringApplyResult rebuildAdmittedLiveWiring(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::GraphWiringDelta& rDelta)
{
    spreadsheetengine::detail::substrate::AdmittedWiringContainers aStore;
    aStore.maBroadcasterNodes = rDelta.maBroadcasterNodesAfter;
    aStore.maListenerEdges = rDelta.maListenerEdgesAfter;
    aStore.maFormulaTreeNodes = rDelta.maFormulaTreeAfter;
    aStore.maFormulaTrackNodes = rDelta.maFormulaTrackAfter;
    return realizeAdmittedWiringContainers(rDoc, aStore);
}

} // namespace spreadsheetengine::compat::libreoffice::substratewiring

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
