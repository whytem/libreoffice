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

#include <document.hxx>
#include <dociter.hxx>
#include <formulacell.hxx>
#include <listenercontext.hxx>

#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/detail/substrate/GraphWiringDelta.hxx>

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
    sal_Int32 mnListenerEdgesApplied = 0;
    sal_Int32 mnFormulaTreeNodesApplied = 0;
    sal_Int32 mnFormulaTrackNodesApplied = 0;
};

namespace detail
{

[[nodiscard]] inline bool isAdmittedFormulaCell(const ScFormulaCell& rCell)
{
    return !rCell.IsShared() && rCell.GetMatrixFlag() == ScMatrixMode::NONE;
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

} // namespace detail

[[nodiscard]] inline WiringApplyResult rebuildAdmittedLiveWiring(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::GraphWiringDelta& rDelta)
{
    WiringApplyResult aResult;

    std::vector<ScFormulaCell*> aCells;
    if (!detail::collectAdmittedFormulaCells(rDoc, aCells, aResult.maReason))
        return aResult;

    detail::clearAdmittedLiveWiring(rDoc, aCells);

    for (const auto& rEdge : rDelta.maListenerEdgesAfter)
    {
        detail::applyListenerEdgeTarget(rDoc, rEdge, aResult.maReason);
        if (!aResult.maReason.empty())
            return aResult;
        ++aResult.mnListenerEdgesApplied;
    }

    for (const auto& rNode : rDelta.maFormulaTreeAfter)
    {
        ScFormulaCell* pCell = detail::resolveFormulaCell(rDoc, rNode, aResult.maReason);
        if (!pCell)
            return aResult;
        rDoc.PutInFormulaTree(pCell);
        ++aResult.mnFormulaTreeNodesApplied;
    }

    for (const auto& rNode : rDelta.maFormulaTrackAfter)
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

} // namespace spreadsheetengine::compat::libreoffice::substratewiring

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
