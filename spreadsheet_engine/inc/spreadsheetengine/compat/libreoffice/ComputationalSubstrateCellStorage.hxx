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

#include <document.hxx>
#include <dociter.hxx>
#include <formulacell.hxx>

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx>

namespace spreadsheetengine::compat::libreoffice::substratecellstorage
{

enum class CellStorageMirrorResultKind : sal_uInt8
{
    Applied,
    RejectedOutOfContract
};

struct CellStorageMirrorResult
{
    CellStorageMirrorResultKind meKind = CellStorageMirrorResultKind::RejectedOutOfContract;
    api::String maReason;
    sal_Int32 mnCellsCleared = 0;
    sal_Int32 mnCellsApplied = 0;
};

namespace detail
{

[[nodiscard]] inline bool isAdmittedFormulaCell(const ScFormulaCell& rCell)
{
    return rCell.GetMatrixFlag() == ScMatrixMode::NONE;
}

[[nodiscard]] inline bool collectAdmittedLiveCellAddresses(
    ScDocument& rDoc, std::vector<ScAddress>& rAddresses, api::String& rReason)
{
    for (SCTAB nTab = 0; nTab < rDoc.GetTableCount(); ++nTab)
    {
        ScCellIterator aIter(rDoc, ScRange(0, 0, nTab, rDoc.MaxCol(), rDoc.MaxRow(), nTab));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            if (aIter.getType() == CELLTYPE_FORMULA)
            {
                ScFormulaCell* pCell = aIter.getFormulaCell();
                if (!pCell || !isAdmittedFormulaCell(*pCell))
                {
                    rReason = u"formula_shape_out_of_contract";
                    return false;
                }
            }

            rAddresses.push_back(aIter.GetPos());
        }
    }

    return true;
}

inline void clearRemovedLiveCells(ScDocument& rDoc,
    const std::vector<ScAddress>& rLiveAddresses,
    const spreadsheetengine::detail::substrate::AdmittedCellStorage& rStore,
    CellStorageMirrorResult& rResult)
{
    for (const auto& rAddress : rLiveAddresses)
    {
        if (rStore.findCell(toApiCellAddress(rAddress)))
            continue;

        rDoc.SetEmptyCell(rAddress);
        ++rResult.mnCellsCleared;
    }
}

inline void applyMirroredRecord(ScDocument& rDoc,
    const spreadsheetengine::detail::substrate::AdmittedCellStorageRecord& rRecord,
    CellStorageMirrorResult& rResult, bool bApplyFormulaRecords)
{
    const ScAddress aAddress = toLibreOfficeAddress(rRecord.maId.maAddress);

    if (rRecord.hasFormula())
    {
        if (!bApplyFormulaRecords)
            return;

        if (rRecord.moFormula->meKind
            != spreadsheetengine::detail::facade::FormulaCellKind::Ordinary)
        {
            rResult.maReason = u"formula_kind_out_of_contract";
            return;
        }

        rDoc.SetString(aAddress, toLibreOfficeString(rRecord.moFormula->maFormulaSource));
        ++rResult.mnCellsApplied;
        return;
    }

    if (rRecord.maCell.meKind == spreadsheetengine::detail::facade::CellKind::Scalar)
    {
        if (rRecord.maCell.maValue.isNumber())
        {
            rDoc.SetValue(aAddress, rRecord.maCell.maValue.mfNumber);
            ++rResult.mnCellsApplied;
            return;
        }
        if (rRecord.maCell.maValue.isText())
        {
            rDoc.SetString(aAddress, toLibreOfficeString(rRecord.maCell.maValue.maString));
            ++rResult.mnCellsApplied;
            return;
        }

        rResult.maReason = u"scalar_value_out_of_contract";
        return;
    }

    rResult.maReason = u"cell_kind_out_of_contract";
}

} // namespace detail

[[nodiscard]] inline CellStorageMirrorResult mirrorAdmittedCellStorage(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::AdmittedCellStorage& rStore)
{
    CellStorageMirrorResult aResult;

    std::vector<ScAddress> aLiveAddresses;
    if (!detail::collectAdmittedLiveCellAddresses(rDoc, aLiveAddresses, aResult.maReason))
        return aResult;

    detail::clearRemovedLiveCells(rDoc, aLiveAddresses, rStore, aResult);
    for (const auto& rRecord : rStore.maCells)
    {
        detail::applyMirroredRecord(rDoc, rRecord, aResult, true);
        if (!aResult.maReason.empty())
            return aResult;
    }

    aResult.meKind = CellStorageMirrorResultKind::Applied;
    return aResult;
}

[[nodiscard]] inline CellStorageMirrorResult mirrorAdmittedScalarCellStorage(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::AdmittedCellStorage& rStore)
{
    CellStorageMirrorResult aResult;

    std::vector<ScAddress> aLiveAddresses;
    if (!detail::collectAdmittedLiveCellAddresses(rDoc, aLiveAddresses, aResult.maReason))
        return aResult;

    detail::clearRemovedLiveCells(rDoc, aLiveAddresses, rStore, aResult);
    for (const auto& rRecord : rStore.maCells)
    {
        detail::applyMirroredRecord(rDoc, rRecord, aResult, false);
        if (!aResult.maReason.empty())
            return aResult;
    }

    aResult.meKind = CellStorageMirrorResultKind::Applied;
    return aResult;
}

} // namespace spreadsheetengine::compat::libreoffice::substratecellstorage

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
