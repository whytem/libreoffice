/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <refupdat.hxx>
#include <document.hxx>
#include <bigrange.hxx>
#include <refdata.hxx>
#include <spreadsheetengine/api/ReferenceUpdate.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>

#include <osl/diagnose.h>

namespace
{

spreadsheetengine::api::refupdate::UpdateMode toApiUpdateMode(UpdateRefMode eMode)
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

ScRefUpdateRes fromApiUpdateResult(spreadsheetengine::api::refupdate::UpdateResult eResult)
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

}

static bool lcl_IsWrapBig( sal_Int64 nRef, sal_Int32 nDelta )
{
    if (nDelta > 0)
        return nRef > std::numeric_limits<sal_Int64>::max() - nDelta;
    else
        return nRef < std::numeric_limits<sal_Int64>::min() - nDelta;
}

static bool lcl_MoveBig( sal_Int64& rRef, sal_Int64 nStart, sal_Int32 nDelta )
{
    bool bCut = false;
    if ( rRef >= nStart )
    {
        if ( nDelta > 0 )
            bCut = lcl_IsWrapBig( rRef, nDelta );
        if ( bCut )
            rRef = ScBigRange::nRangeMax;
        else
            rRef += nDelta;
    }
    return bCut;
}

static bool lcl_MoveItCutBig( sal_Int64& rRef, sal_Int32 nDelta )
{
    bool bCut = lcl_IsWrapBig( rRef, nDelta );
    rRef += nDelta;
    return bCut;
}

ScRefUpdateRes ScRefUpdate::Update( const ScDocument& rDoc, UpdateRefMode eUpdateRefMode,
                                        SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                                        SCCOL nCol2, SCROW nRow2, SCTAB nTab2,
                                        SCCOL nDx, SCROW nDy, SCTAB nDz,
                                        SCCOL& theCol1, SCROW& theRow1, SCTAB& theTab1,
                                        SCCOL& theCol2, SCROW& theRow2, SCTAB& theTab2 )
{
    if (eUpdateRefMode == URM_REORDER)
        OSL_ENSURE(!nDx && !nDy, "URM_REORDER for x and y not yet implemented");

    spreadsheetengine::api::CellRange aWhere {
        { nTab1, nCol1, nRow1 }, { nTab2, nCol2, nRow2 }
    };
    spreadsheetengine::api::CellRange aRef {
        { theTab1, theCol1, theRow1 }, { theTab2, theCol2, theRow2 }
    };
    const auto eResult = spreadsheetengine::api::refupdate::updateReference(
        toApiUpdateMode(eUpdateRefMode), aWhere, nDx, nDy, nDz, rDoc.MaxCol(),
        rDoc.MaxRow(), static_cast<SCTAB>(rDoc.GetTableCount() - 1),
        rDoc.IsExpandRefs(), aRef);
    theTab1 = static_cast<SCTAB>(aRef.maStart.mnSheet);
    theCol1 = static_cast<SCCOL>(aRef.maStart.mnColumn);
    theRow1 = static_cast<SCROW>(aRef.maStart.mnRow);
    theTab2 = static_cast<SCTAB>(aRef.maEnd.mnSheet);
    theCol2 = static_cast<SCCOL>(aRef.maEnd.mnColumn);
    theRow2 = static_cast<SCROW>(aRef.maEnd.mnRow);
    return fromApiUpdateResult(eResult);
}

// simple UpdateReference for ScBigRange (ScChangeAction/ScChangeTrack)
// References can also be located outside of the document!
// Whole columns/rows (ScBigRange::nRangeMin..ScBigRange::nRangeMax) stay as such!
ScRefUpdateRes ScRefUpdate::Update( UpdateRefMode eUpdateRefMode,
        const ScBigRange& rWhere, sal_Int32 nDx, sal_Int32 nDy, sal_Int32 nDz,
        ScBigRange& rWhat )
{
    ScRefUpdateRes eRet = UR_NOTHING;
    const ScBigRange aOldRange( rWhat );

    sal_Int64 nCol1, nRow1, nTab1, nCol2, nRow2, nTab2;
    sal_Int64 theCol1, theRow1, theTab1, theCol2, theRow2, theTab2;
    rWhere.GetVars( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
    rWhat.GetVars( theCol1, theRow1, theTab1, theCol2, theRow2, theTab2 );

    bool bCut1, bCut2;

    if (eUpdateRefMode == URM_INSDEL)
    {
        if ( nDx && (theRow1 >= nRow1) && (theRow2 <= nRow2) &&
                    (theTab1 >= nTab1) && (theTab2 <= nTab2) &&
                    (theCol1 != ScBigRange::nRangeMin || theCol2 != ScBigRange::nRangeMax) )
        {
            bCut1 = lcl_MoveBig( theCol1, nCol1, nDx );
            bCut2 = lcl_MoveBig( theCol2, nCol1, nDx );
            if ( bCut1 || bCut2 )
                eRet = UR_UPDATED;
            rWhat.aStart.SetCol( theCol1 );
            rWhat.aEnd.SetCol( theCol2 );
        }
        if ( nDy && (theCol1 >= nCol1) && (theCol2 <= nCol2) &&
                    (theTab1 >= nTab1) && (theTab2 <= nTab2) &&
                    (theRow1 != ScBigRange::nRangeMin || theRow2 != ScBigRange::nRangeMax) )
        {
            bCut1 = lcl_MoveBig( theRow1, nRow1, nDy );
            bCut2 = lcl_MoveBig( theRow2, nRow1, nDy );
            if ( bCut1 || bCut2 )
                eRet = UR_UPDATED;
            rWhat.aStart.SetRow( theRow1 );
            rWhat.aEnd.SetRow( theRow2 );
        }
        if ( nDz && (theCol1 >= nCol1) && (theCol2 <= nCol2) &&
                    (theRow1 >= nRow1) && (theRow2 <= nRow2) &&
                    (theTab1 != ScBigRange::nRangeMin || theTab2 != ScBigRange::nRangeMax) )
        {
            bCut1 = lcl_MoveBig( theTab1, nTab1, nDz );
            bCut2 = lcl_MoveBig( theTab2, nTab1, nDz );
            if ( bCut1 || bCut2 )
                eRet = UR_UPDATED;
            rWhat.aStart.SetTab( theTab1 );
            rWhat.aEnd.SetTab( theTab2 );
        }
    }
    else if (eUpdateRefMode == URM_MOVE)
    {
        if ( rWhere.Contains( rWhat ) )
        {
            if ( nDx && (theCol1 != ScBigRange::nRangeMin || theCol2 != ScBigRange::nRangeMax) )
            {
                bCut1 = lcl_MoveItCutBig( theCol1, nDx );
                bCut2 = lcl_MoveItCutBig( theCol2, nDx );
                if ( bCut1 || bCut2 )
                    eRet = UR_UPDATED;
                rWhat.aStart.SetCol( theCol1 );
                rWhat.aEnd.SetCol( theCol2 );
            }
            if ( nDy && (theRow1 != ScBigRange::nRangeMin || theRow2 != ScBigRange::nRangeMax) )
            {
                bCut1 = lcl_MoveItCutBig( theRow1, nDy );
                bCut2 = lcl_MoveItCutBig( theRow2, nDy );
                if ( bCut1 || bCut2 )
                    eRet = UR_UPDATED;
                rWhat.aStart.SetRow( theRow1 );
                rWhat.aEnd.SetRow( theRow2 );
            }
            if ( nDz && (theTab1 != ScBigRange::nRangeMin || theTab2 != ScBigRange::nRangeMax) )
            {
                bCut1 = lcl_MoveItCutBig( theTab1, nDz );
                bCut2 = lcl_MoveItCutBig( theTab2, nDz );
                if ( bCut1 || bCut2 )
                    eRet = UR_UPDATED;
                rWhat.aStart.SetTab( theTab1 );
                rWhat.aEnd.SetTab( theTab2 );
            }
        }
    }

    if ( eRet == UR_NOTHING && rWhat != aOldRange )
        eRet = UR_UPDATED;

    return eRet;
}

void ScRefUpdate::MoveRelWrap( const ScDocument& rDoc, const ScAddress& rPos,
                               SCCOL nMaxCol, SCROW nMaxRow, ScComplexRefData& rRef )
{
    auto aApiRef = rRef.toApiComplexRefData();
    spreadsheetengine::api::refupdate::moveRelativeWrap(
        aApiRef,
        { rDoc.MaxCol(), rDoc.MaxRow(), static_cast<SCTAB>(rDoc.GetTableCount() - 1) },
        spreadsheetengine::compat::libreoffice::toApiCellAddress(rPos), nMaxCol, nMaxRow,
        static_cast<SCTAB>(rDoc.GetTableCount() - 1));
    rRef.assignFromApiComplexRefData(aApiRef);
}

void ScRefUpdate::DoTranspose( SCCOL& rCol, SCROW& rRow, SCTAB& rTab,
                        const ScDocument& rDoc, const ScRange& rSource, const ScAddress& rDest )
{
    OSL_ENSURE( rCol>=rSource.aStart.Col() && rRow>=rSource.aStart.Row(),
                "UpdateTranspose: pos. wrong" );

    spreadsheetengine::api::ColumnIndex nApiCol = rCol;
    spreadsheetengine::api::RowIndex nApiRow = rRow;
    spreadsheetengine::api::SheetId nApiTab = rTab;
    spreadsheetengine::api::refupdate::doTranspose(
        nApiCol, nApiRow, nApiTab, rDoc.GetTableCount(),
        spreadsheetengine::compat::libreoffice::toApiCellRange(rSource),
        spreadsheetengine::compat::libreoffice::toApiCellAddress(rDest));
    rCol = static_cast<SCCOL>(nApiCol);
    rRow = static_cast<SCROW>(nApiRow);
    rTab = static_cast<SCTAB>(nApiTab);
}

ScRefUpdateRes ScRefUpdate::UpdateTranspose(
    const ScDocument& rDoc, const ScRange& rSource, const ScAddress& rDest, ScRange& rRef )
{
    auto aApiRef = spreadsheetengine::compat::libreoffice::toApiCellRange(rRef);
    const bool bUpdated = spreadsheetengine::api::refupdate::updateTranspose(
        rDoc.GetTableCount(), spreadsheetengine::compat::libreoffice::toApiCellRange(rSource),
        spreadsheetengine::compat::libreoffice::toApiCellAddress(rDest), aApiRef);
    if (!bUpdated)
        return UR_NOTHING;

    rRef = spreadsheetengine::compat::libreoffice::toLibreOfficeRange(aApiRef);
    return UR_UPDATED;
}

//  UpdateGrow - expands references which point exactly to the area
//  gets by without document

ScRefUpdateRes ScRefUpdate::UpdateGrow(
    const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY, ScRange& rRef )
{
    auto aApiRef = spreadsheetengine::compat::libreoffice::toApiCellRange(rRef);
    const bool bUpdated = spreadsheetengine::api::refupdate::updateGrow(
        spreadsheetengine::compat::libreoffice::toApiCellRange(rArea), nGrowX, nGrowY, aApiRef);
    if (!bUpdated)
        return UR_NOTHING;

    rRef = spreadsheetengine::compat::libreoffice::toLibreOfficeRange(aApiRef);
    return UR_UPDATED;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
