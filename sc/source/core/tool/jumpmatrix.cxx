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

#include <jumpmatrix.hxx>
#include <scmatrix.hxx>
#include <spreadsheetengine/detail/JumpMatrixRuntime.hxx>
#include <osl/diagnose.h>

namespace {
// Don't bother with buffer overhead for less than y rows.
const SCSIZE kBufferThreshold = 128;
}

ScJumpMatrix::ScJumpMatrix( OpCode eOp, SCSIZE nColsP, SCSIZE nRowsP )
    : mvJump(nColsP * nRowsP)
    // Initialize result matrix in case of
    // a premature end of the interpreter
    // due to errors.
    , pMat(new ScMatrix(nColsP, nRowsP, CreateDoubleError(FormulaError::NotAvailable)))
    , nCols(nColsP)
    , nRows(nRowsP)
    , nCurCol(0)
    , nCurRow(0)
    , nResMatCols(nColsP)
    , nResMatRows(nRowsP)
    , meOp(eOp)
    , bStarted(false)
    , mnBufferCol(0)
    , mnBufferRowStart(0)
    , mnBufferEmptyCount(0)
    , mnBufferEmptyPathCount(0)
{
    /*! pJump not initialized */
}

ScJumpMatrix::~ScJumpMatrix()
{
    for (const auto & i : mvParams)
        i->DecRef();
}

void ScJumpMatrix::GetDimensions(SCSIZE& rCols, SCSIZE& rRows) const
{
    rCols = nCols;
    rRows = nRows;
}

void ScJumpMatrix::SetJump(SCSIZE nCol, SCSIZE nRow, double fBool,
                           short nStart, short nNext)
{
    mvJump[spreadsheetengine::core::jumpmatrix::jumpEntryIndex(
        spreadsheetengine::core::matrix::makeDimensions(nCols, nRows),
        spreadsheetengine::core::matrix::makeCoordinate(nCol, nRow))]
        .SetJump(fBool, nStart, nNext, SHRT_MAX);
}

void ScJumpMatrix::GetJump(
    SCSIZE nCol, SCSIZE nRow, double& rBool, short& rStart, short& rNext, short& rStop) const
{
    auto aCoordinate = spreadsheetengine::core::matrix::makeCoordinate(nCol, nRow);
    if (!spreadsheetengine::core::jumpmatrix::normalizeJumpCoordinate(
            spreadsheetengine::core::matrix::makeDimensions(nCols, nRows), aCoordinate))
    {
        OSL_FAIL("ScJumpMatrix::GetJump: dimension error");
        aCoordinate = spreadsheetengine::core::matrix::makeCoordinate(0, 0);
    }
    nCol = aCoordinate.mnColumn;
    nRow = aCoordinate.mnRow;
    mvJump[spreadsheetengine::core::jumpmatrix::jumpEntryIndex(
        spreadsheetengine::core::matrix::makeDimensions(nCols, nRows), aCoordinate)].
        GetJump(rBool, rStart, rNext, rStop);
}

void ScJumpMatrix::SetAllJumps(double fBool, short nStart, short nNext, short nStop)
{
    sal_uInt64 n = spreadsheetengine::core::matrix::makeDimensions(nCols, nRows).elementCount();
    for (sal_uInt64 j = 0; j < n; ++j)
    {
        mvJump[j].SetJump(fBool, nStart,
                         nNext, nStop);
    }
}

void ScJumpMatrix::SetJumpParameters(ScTokenVec&& p)
{
    mvParams = std::move(p);
}

void ScJumpMatrix::GetPos(SCSIZE& rCol, SCSIZE& rRow) const
{
    rCol = nCurCol;
    rRow = nCurRow;
}

bool ScJumpMatrix::Next(SCSIZE& rCol, SCSIZE& rRow)
{
    spreadsheetengine::core::jumpmatrix::ResultCursor aCursor {
        spreadsheetengine::core::matrix::makeCoordinate(nCurCol, nCurRow), bStarted
    };
    const bool bHasNext = spreadsheetengine::core::jumpmatrix::advanceResultCursor(
        spreadsheetengine::core::matrix::makeDimensions(nResMatCols, nResMatRows), aCursor);
    nCurCol = aCursor.maCoordinate.mnColumn;
    nCurRow = aCursor.maCoordinate.mnRow;
    bStarted = aCursor.mbStarted;
    GetPos(rCol, rRow);
    return bHasNext;
}

void ScJumpMatrix::GetResMatDimensions(SCSIZE& rCols, SCSIZE& rRows)
{
    rCols = nResMatCols;
    rRows = nResMatRows;
}

void ScJumpMatrix::SetNewResMat(SCSIZE nNewCols, SCSIZE nNewRows)
{
    const auto aPlan = spreadsheetengine::core::jumpmatrix::planResultExpansion(
        spreadsheetengine::core::matrix::makeDimensions(nCols, nRows),
        spreadsheetengine::core::matrix::makeDimensions(nResMatCols, nResMatRows),
        spreadsheetengine::core::matrix::makeDimensions(nNewCols, nNewRows),
        spreadsheetengine::core::matrix::makeCoordinate(nCurCol, nCurRow));
    const auto nExpandedCols = static_cast<SCSIZE>(aPlan.maExpandedDimensions.mnColumns);
    const auto nExpandedRows = static_cast<SCSIZE>(aPlan.maExpandedDimensions.mnRows);
    if (!aPlan.mbNeedsExpansion)
        return;

    FlushBufferOtherThan( BUFFER_NONE, 0, 0);
    pMat = pMat->CloneAndExtend(nExpandedCols, nExpandedRows);
    if (aPlan.mbFillNewColumns)
    {
        pMat->FillDouble(
            CreateDoubleError(FormulaError::NotAvailable),
            aPlan.maNewColumnRange.maStart.mnColumn, aPlan.maNewColumnRange.maStart.mnRow,
            aPlan.maNewColumnRange.maEnd.mnColumn, aPlan.maNewColumnRange.maEnd.mnRow);
    }
    if (aPlan.mbFillNewRows)
    {
        pMat->FillDouble(
            CreateDoubleError(FormulaError::NotAvailable),
            aPlan.maNewRowRange.maStart.mnColumn, aPlan.maNewRowRange.maStart.mnRow,
            aPlan.maNewRowRange.maEnd.mnColumn, aPlan.maNewRowRange.maEnd.mnRow);
    }
    nCurCol = aPlan.maAdjustedCursor.mnColumn;
    nCurRow = aPlan.maAdjustedCursor.mnRow;
    nResMatCols = nExpandedCols;
    nResMatRows = nExpandedRows;
}

bool ScJumpMatrix::HasResultMatrix() const
{
    // We now always have a matrix but caller logic may still want to check it.
    return bool(pMat);
}

ScRefList& ScJumpMatrix::GetRefList()
{
    return mvRefList;
}

void ScJumpMatrix::FlushBufferOtherThan( ScJumpMatrix::BufferType eType, SCSIZE nC, SCSIZE nR )
{
    const auto aRequestedCoordinate = spreadsheetengine::core::matrix::makeCoordinate(nC, nR);
    spreadsheetengine::core::jumpmatrix::flushBufferedWindowIfNeeded(
        spreadsheetengine::core::jumpmatrix::bufferWindowFromState(
            mnBufferCol, mnBufferRowStart, mvBufferDoubles.size()),
        eType == BUFFER_DOUBLE, aRequestedCoordinate,
        [this]() { pMat->PutDoubleVector( mvBufferDoubles, mnBufferCol, mnBufferRowStart); },
        [this]() { mvBufferDoubles.clear(); });
    spreadsheetengine::core::jumpmatrix::flushBufferedWindowIfNeeded(
        spreadsheetengine::core::jumpmatrix::bufferWindowFromState(
            mnBufferCol, mnBufferRowStart, mvBufferStrings.size()),
        eType == BUFFER_STRING, aRequestedCoordinate,
        [this]() { pMat->PutStringVector( mvBufferStrings, mnBufferCol, mnBufferRowStart); },
        [this]() { mvBufferStrings.clear(); });
    spreadsheetengine::core::jumpmatrix::flushBufferedWindowIfNeeded(
        spreadsheetengine::core::jumpmatrix::bufferWindowFromState(
            mnBufferCol, mnBufferRowStart, mnBufferEmptyCount),
        eType == BUFFER_EMPTY, aRequestedCoordinate,
        [this]() { pMat->PutEmptyVector( mnBufferEmptyCount, mnBufferCol, mnBufferRowStart); },
        [this]() { mnBufferEmptyCount = 0; });
    spreadsheetengine::core::jumpmatrix::flushBufferedWindowIfNeeded(
        spreadsheetengine::core::jumpmatrix::bufferWindowFromState(
            mnBufferCol, mnBufferRowStart, mnBufferEmptyPathCount),
        eType == BUFFER_EMPTYPATH, aRequestedCoordinate,
        [this]() { pMat->PutEmptyPathVector( mnBufferEmptyPathCount, mnBufferCol, mnBufferRowStart); },
        [this]() { mnBufferEmptyPathCount = 0; });
}

ScMatrix* ScJumpMatrix::GetResultMatrix()
{
    if (spreadsheetengine::core::jumpmatrix::shouldBufferResultWrites(
            spreadsheetengine::core::matrix::makeDimensions(nResMatCols, nResMatRows),
            kBufferThreshold))
        FlushBufferOtherThan( BUFFER_NONE, 0, 0);
    return pMat.get();
}

void ScJumpMatrix::PutResultDouble( double fVal, SCSIZE nC, SCSIZE nR )
{
    const auto aPlan = spreadsheetengine::core::jumpmatrix::planBufferedResultWrite(
        spreadsheetengine::core::matrix::makeDimensions(nResMatCols, nResMatRows), kBufferThreshold,
        spreadsheetengine::core::jumpmatrix::bufferWindowFromState(
            mnBufferCol, mnBufferRowStart, mvBufferDoubles.size()),
        spreadsheetengine::core::matrix::makeCoordinate(nC, nR));
    if (!aPlan.mbBufferWrite)
        pMat->PutDouble( fVal, nC, nR);
    else
    {
        FlushBufferOtherThan( BUFFER_DOUBLE, nC, nR);
        spreadsheetengine::core::jumpmatrix::applyBufferWindowStart(aPlan.maWindow, mnBufferCol,
                                                                    mnBufferRowStart);
        mvBufferDoubles.push_back( fVal);
    }
}

void ScJumpMatrix::PutResultString( const svl::SharedString& rStr, SCSIZE nC, SCSIZE nR )
{
    const auto aPlan = spreadsheetengine::core::jumpmatrix::planBufferedResultWrite(
        spreadsheetengine::core::matrix::makeDimensions(nResMatCols, nResMatRows), kBufferThreshold,
        spreadsheetengine::core::jumpmatrix::bufferWindowFromState(
            mnBufferCol, mnBufferRowStart, mvBufferStrings.size()),
        spreadsheetengine::core::matrix::makeCoordinate(nC, nR));
    if (!aPlan.mbBufferWrite)
        pMat->PutString( rStr, nC, nR);
    else
    {
        FlushBufferOtherThan( BUFFER_STRING, nC, nR);
        spreadsheetengine::core::jumpmatrix::applyBufferWindowStart(aPlan.maWindow, mnBufferCol,
                                                                    mnBufferRowStart);
        mvBufferStrings.push_back( rStr);
    }
}

void ScJumpMatrix::PutResultEmpty( SCSIZE nC, SCSIZE nR )
{
    const auto aPlan = spreadsheetengine::core::jumpmatrix::planBufferedResultWrite(
        spreadsheetengine::core::matrix::makeDimensions(nResMatCols, nResMatRows), kBufferThreshold,
        spreadsheetengine::core::jumpmatrix::bufferWindowFromState(
            mnBufferCol, mnBufferRowStart, mnBufferEmptyCount),
        spreadsheetengine::core::matrix::makeCoordinate(nC, nR));
    if (!aPlan.mbBufferWrite)
        pMat->PutEmpty( nC, nR);
    else
    {
        FlushBufferOtherThan( BUFFER_EMPTY, nC, nR);
        spreadsheetengine::core::jumpmatrix::applyBufferWindowStart(aPlan.maWindow, mnBufferCol,
                                                                    mnBufferRowStart);
        mnBufferEmptyCount = aPlan.maWindow.mnCount;
    }
}

void ScJumpMatrix::PutResultEmptyPath( SCSIZE nC, SCSIZE nR )
{
    const auto aPlan = spreadsheetengine::core::jumpmatrix::planBufferedResultWrite(
        spreadsheetengine::core::matrix::makeDimensions(nResMatCols, nResMatRows), kBufferThreshold,
        spreadsheetengine::core::jumpmatrix::bufferWindowFromState(
            mnBufferCol, mnBufferRowStart, mnBufferEmptyPathCount),
        spreadsheetengine::core::matrix::makeCoordinate(nC, nR));
    if (!aPlan.mbBufferWrite)
        pMat->PutEmptyPath( nC, nR);
    else
    {
        FlushBufferOtherThan( BUFFER_EMPTYPATH, nC, nR);
        spreadsheetengine::core::jumpmatrix::applyBufferWindowStart(aPlan.maWindow, mnBufferCol,
                                                                    mnBufferRowStart);
        mnBufferEmptyPathCount = aPlan.maWindow.mnCount;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
