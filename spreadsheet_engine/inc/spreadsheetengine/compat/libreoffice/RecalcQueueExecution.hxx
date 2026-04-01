/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>

#include <dociter.hxx>
#include <document.hxx>
#include <formulacell.hxx>
#include <table.hxx>

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>

namespace spreadsheetengine::compat::libreoffice::recalcqueue
{

struct FormulaStateSnapshot
{
    std::vector<api::CellAddress> maTreeOrder;
    std::vector<api::CellAddress> maTrackOrder;
    std::vector<api::CellAddress> maDirtyOnly;
};

namespace detail
{

[[nodiscard]] inline std::vector<ScFormulaCell*> collectFormulaCells(const ScDocument& rDoc)
{
    std::vector<ScFormulaCell*> aCells;
    for (SCTAB nTab = 0; nTab < rDoc.GetTableCount(); ++nTab)
    {
        ScCellIterator aIter(const_cast<ScDocument&>(rDoc),
            ScRange(0, 0, nTab, rDoc.MaxCol(), rDoc.MaxRow(), nTab));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            if (aIter.getType() != CELLTYPE_FORMULA)
                continue;
            if (ScFormulaCell* pCell = aIter.getFormulaCell())
                aCells.push_back(pCell);
        }
    }
    return aCells;
}

[[nodiscard]] inline ScFormulaCell* getFormulaCell(
    ScDocument& rDoc, const api::CellAddress& rAddress)
{
    ScTable* pTable = rDoc.FetchTable(rAddress.mnSheet);
    if (!pTable)
        return nullptr;
    return pTable->GetFormulaCell(rAddress.mnColumn, rAddress.mnRow);
}

inline void appendQueueAddressToTree(ScDocument& rDoc, const api::CellAddress& rAddress)
{
    ScFormulaCell* pCell = getFormulaCell(rDoc, rAddress);
    if (!pCell)
        return;

    if (rDoc.IsInFormulaTrack(pCell))
        rDoc.RemoveFromFormulaTrack(pCell);
    if (rDoc.IsInFormulaTree(pCell))
        rDoc.RemoveFromFormulaTree(pCell);

    pCell->SetDirtyVar();
    rDoc.PutInFormulaTree(pCell);
}

} // namespace detail

[[nodiscard]] inline FormulaStateSnapshot captureFormulaState(const ScDocument& rDoc)
{
    FormulaStateSnapshot aSnapshot;
    aSnapshot.maTreeOrder = recalcshadow::detail::collectFormulaTreeAddresses(rDoc);

    for (ScFormulaCell* pCell : detail::collectFormulaCells(rDoc))
    {
        const auto aAddress = toApiCellAddress(pCell->aPos);
        if (rDoc.IsInFormulaTrack(pCell))
            aSnapshot.maTrackOrder.push_back(aAddress);
        else if (pCell->GetDirty() || pCell->NeedsInterpret())
            aSnapshot.maDirtyOnly.push_back(aAddress);
    }

    return aSnapshot;
}

[[nodiscard]] inline bool isCleanFormulaState(const FormulaStateSnapshot& rSnapshot)
{
    return rSnapshot.maTreeOrder.empty() && rSnapshot.maTrackOrder.empty()
           && rSnapshot.maDirtyOnly.empty();
}

inline void clearFormulaState(ScDocument& rDoc)
{
    for (ScFormulaCell* pCell : detail::collectFormulaCells(rDoc))
    {
        if (rDoc.IsInFormulaTrack(pCell))
            rDoc.RemoveFromFormulaTrack(pCell);
        if (rDoc.IsInFormulaTree(pCell))
            rDoc.RemoveFromFormulaTree(pCell);
        if (pCell->GetDirty() || pCell->NeedsInterpret())
            pCell->ResetDirty();
    }
}

inline void restoreFormulaState(ScDocument& rDoc, const FormulaStateSnapshot& rSnapshot)
{
    clearFormulaState(rDoc);

    for (const auto& rAddress : rSnapshot.maTreeOrder)
        detail::appendQueueAddressToTree(rDoc, rAddress);

    for (const auto& rAddress : rSnapshot.maTrackOrder)
    {
        ScFormulaCell* pCell = detail::getFormulaCell(rDoc, rAddress);
        if (!pCell)
            continue;
        pCell->SetDirtyVar();
        rDoc.AppendToFormulaTrack(pCell);
    }

    for (const auto& rAddress : rSnapshot.maDirtyOnly)
    {
        ScFormulaCell* pCell = detail::getFormulaCell(rDoc, rAddress);
        if (!pCell)
            continue;
        if (!rDoc.IsInFormulaTree(pCell) && !rDoc.IsInFormulaTrack(pCell))
            pCell->SetDirtyVar();
    }
}

inline void applyRecalcPlan(ScDocument& rDoc,
    const spreadsheetengine::detail::dependency::RecalcPlan& rPlan)
{
    clearFormulaState(rDoc);

    for (const auto& rEntry : rPlan.maQueue)
        detail::appendQueueAddressToTree(rDoc, rEntry.maAddress);
}

} // namespace spreadsheetengine::compat::libreoffice::recalcqueue

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
