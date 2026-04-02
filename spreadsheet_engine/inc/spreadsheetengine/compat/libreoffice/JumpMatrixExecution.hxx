/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>

#include <jumpmatrix.hxx>
#include <math.hxx>

namespace spreadsheetengine::compat::libreoffice::jumpmatrixexecution
{

struct PendingJump
{
    bool mbHasPendingJump = false;
    short mnStart = 0;
    short mnNext = 0;
    short mnStop = 0;
};

inline void adjustResultMatrixDimensions(
    ScJumpMatrix& rJumpMatrix, SCSIZE nParameterColumns, SCSIZE nParameterRows)
{
    SCSIZE nJumpColumns = 0;
    SCSIZE nJumpRows = 0;
    SCSIZE nResultColumns = 0;
    SCSIZE nResultRows = 0;
    rJumpMatrix.GetDimensions(nJumpColumns, nJumpRows);
    rJumpMatrix.GetResMatDimensions(nResultColumns, nResultRows);
    if (!((nJumpColumns == 1 && nParameterColumns > nResultColumns)
            || (nJumpRows == 1 && nParameterRows > nResultRows)))
    {
        return;
    }

    SCSIZE nAdjustedColumns = 0;
    SCSIZE nAdjustedRows = 0;
    if (nJumpColumns == 1 && nJumpRows == 1)
    {
        nAdjustedColumns = std::max(nParameterColumns, nResultColumns);
        nAdjustedRows = std::max(nParameterRows, nResultRows);
    }
    else if (nJumpColumns == 1)
    {
        nAdjustedColumns = nParameterColumns;
        nAdjustedRows = nResultRows;
    }
    else
    {
        nAdjustedColumns = nResultColumns;
        nAdjustedRows = nParameterRows;
    }

    rJumpMatrix.SetNewResMat(nAdjustedColumns, nAdjustedRows);
}

[[nodiscard]] inline PendingJump advanceToPendingJump(
    ScJumpMatrix& rJumpMatrix, bool bHasResultMatrix)
{
    PendingJump aResult;
    SCSIZE nColumn = 0;
    SCSIZE nRow = 0;
    bool bContinue = rJumpMatrix.Next(nColumn, nRow);
    if (!bContinue)
        return aResult;

    double fDecision = 0.0;
    short nStart = 0;
    short nNext = 0;
    short nStop = 0;
    rJumpMatrix.GetJump(nColumn, nRow, fDecision, nStart, nNext, nStop);
    while (bContinue && nStart == nNext)
    {
        if (bHasResultMatrix
            && (GetDoubleErrorValue(fDecision) != FormulaError::JumpMatHasResult))
        {
            if (fDecision == 0.0)
                rJumpMatrix.PutResultEmptyPath(nColumn, nRow);
            else
                rJumpMatrix.PutResultDouble(fDecision, nColumn, nRow);
        }

        bContinue = rJumpMatrix.Next(nColumn, nRow);
        if (bContinue)
            rJumpMatrix.GetJump(nColumn, nRow, fDecision, nStart, nNext, nStop);
    }

    if (!bContinue)
        return aResult;

    aResult.mbHasPendingJump = (nStart != nNext);
    aResult.mnStart = nStart;
    aResult.mnNext = nNext;
    aResult.mnStop = nStop;
    return aResult;
}

[[nodiscard]] inline bool shouldReturnReferenceList(bool bReferenceOrRefArray,
    std::size_t nReferenceCount, bool bReferenceReturnType, bool bIsJumpCommand,
    bool bHasNextOperator)
{
    return bReferenceOrRefArray && nReferenceCount > 1 && bReferenceReturnType
           && !bIsJumpCommand && bHasNextOperator;
}

} // namespace spreadsheetengine::compat::libreoffice::jumpmatrixexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
