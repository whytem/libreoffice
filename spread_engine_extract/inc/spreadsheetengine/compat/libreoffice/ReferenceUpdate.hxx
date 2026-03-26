/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <document.hxx>
#include <refdata.hxx>
#include <refupdat.hxx>

#include <spreadsheetengine/api/ReferenceUpdate.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::refupdate::UpdateMode toApiUpdateMode(UpdateRefMode eMode)
{
    switch (eMode)
    {
        case URM_INSDEL:
            return spreadsheetengine::api::refupdate::UpdateMode::InsertDelete;
        case URM_MOVE:
            return spreadsheetengine::api::refupdate::UpdateMode::Move;
        case URM_REORDER:
            return spreadsheetengine::api::refupdate::UpdateMode::Reorder;
        default:
            return spreadsheetengine::api::refupdate::UpdateMode::InsertDelete;
    }
}

inline ScRefUpdateRes toLibreOfficeUpdateResult(
    spreadsheetengine::api::refupdate::UpdateResult eResult)
{
    switch (eResult)
    {
        case spreadsheetengine::api::refupdate::UpdateResult::Nothing:
            return UR_NOTHING;
        case spreadsheetengine::api::refupdate::UpdateResult::Updated:
            return UR_UPDATED;
        case spreadsheetengine::api::refupdate::UpdateResult::Invalid:
            return UR_INVALID;
        case spreadsheetengine::api::refupdate::UpdateResult::Sticky:
            return UR_STICKY;
    }

    return UR_NOTHING;
}

inline ScRefUpdateRes updateReference(const ScDocument& rDoc, UpdateRefMode eUpdateRefMode,
    SCCOL nCol1, SCROW nRow1, SCTAB nTab1, SCCOL nCol2, SCROW nRow2, SCTAB nTab2, SCCOL nDx,
    SCROW nDy, SCTAB nDz, SCCOL& rCol1, SCROW& rRow1, SCTAB& rTab1, SCCOL& rCol2,
    SCROW& rRow2, SCTAB& rTab2)
{
    spreadsheetengine::api::CellRange aWhere { { nTab1, nCol1, nRow1 }, { nTab2, nCol2, nRow2 } };
    spreadsheetengine::api::CellRange aRef { { rTab1, rCol1, rRow1 }, { rTab2, rCol2, rRow2 } };
    const auto eResult = spreadsheetengine::api::refupdate::updateReference(
        toApiUpdateMode(eUpdateRefMode), aWhere, nDx, nDy, nDz, rDoc.MaxCol(), rDoc.MaxRow(),
        static_cast<SCTAB>(rDoc.GetTableCount() - 1), rDoc.IsExpandRefs(), aRef);
    rTab1 = static_cast<SCTAB>(aRef.maStart.mnSheet);
    rCol1 = static_cast<SCCOL>(aRef.maStart.mnColumn);
    rRow1 = static_cast<SCROW>(aRef.maStart.mnRow);
    rTab2 = static_cast<SCTAB>(aRef.maEnd.mnSheet);
    rCol2 = static_cast<SCCOL>(aRef.maEnd.mnColumn);
    rRow2 = static_cast<SCROW>(aRef.maEnd.mnRow);
    return toLibreOfficeUpdateResult(eResult);
}

inline void moveRelativeWrap(const ScDocument& rDoc, const ScAddress& rPos, SCCOL nMaxCol,
    SCROW nMaxRow, ScComplexRefData& rRef)
{
    auto aApiRef = rRef.toApiComplexRefData();
    spreadsheetengine::api::refupdate::moveRelativeWrap(
        aApiRef,
        { rDoc.MaxCol(), rDoc.MaxRow(), static_cast<SCTAB>(rDoc.GetTableCount() - 1) },
        toApiCellAddress(rPos), nMaxCol, nMaxRow, static_cast<SCTAB>(rDoc.GetTableCount() - 1));
    rRef.assignFromApiComplexRefData(aApiRef);
}

inline void doTranspose(SCCOL& rCol, SCROW& rRow, SCTAB& rTab, const ScDocument& rDoc,
    const ScRange& rSource, const ScAddress& rDest)
{
    spreadsheetengine::api::ColumnIndex nApiCol = rCol;
    spreadsheetengine::api::RowIndex nApiRow = rRow;
    spreadsheetengine::api::SheetId nApiTab = rTab;
    spreadsheetengine::api::refupdate::doTranspose(
        nApiCol, nApiRow, nApiTab, rDoc.GetTableCount(), toApiCellRange(rSource),
        toApiCellAddress(rDest));
    rCol = static_cast<SCCOL>(nApiCol);
    rRow = static_cast<SCROW>(nApiRow);
    rTab = static_cast<SCTAB>(nApiTab);
}

inline ScRefUpdateRes updateTranspose(
    const ScDocument& rDoc, const ScRange& rSource, const ScAddress& rDest, ScRange& rRef)
{
    auto aApiRef = toApiCellRange(rRef);
    const bool bUpdated = spreadsheetengine::api::refupdate::updateTranspose(
        rDoc.GetTableCount(), toApiCellRange(rSource), toApiCellAddress(rDest), aApiRef);
    if (!bUpdated)
        return UR_NOTHING;

    rRef = toLibreOfficeRange(aApiRef);
    return UR_UPDATED;
}

inline ScRefUpdateRes updateGrow(
    const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY, ScRange& rRef)
{
    auto aApiRef = toApiCellRange(rRef);
    const bool bUpdated
        = spreadsheetengine::api::refupdate::updateGrow(toApiCellRange(rArea), nGrowX, nGrowY, aApiRef);
    if (!bUpdated)
        return UR_NOTHING;

    rRef = toLibreOfficeRange(aApiRef);
    return UR_UPDATED;
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
