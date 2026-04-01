/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#include <optional>

#include <jumpmatrix.hxx>
#include <math.hxx>
#include <scmatrix.hxx>

#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>

namespace spreadsheetengine::compat::libreoffice::jumpexecution
{

struct MatrixCoordinate
{
    SCSIZE mnColumn = 0;
    SCSIZE mnRow = 0;
};

[[nodiscard]] inline bool matchesIfErrorPolicy(FormulaError eError, bool bNAonly)
{
    return spreadsheetengine::api::logic::matchesIfErrorPolicy(toApiError(eError), bNAonly);
}

inline void storeJumpMatrixResult(
    const ScMatrix& rMatrix, ScJumpMatrix& rJumpMatrix, SCSIZE nColumn, SCSIZE nRow)
{
    if (rMatrix.IsValue(nColumn, nRow))
    {
        rJumpMatrix.PutResultDouble(rMatrix.GetDouble(nColumn, nRow), nColumn, nRow);
    }
    else if (rMatrix.IsEmpty(nColumn, nRow))
    {
        rJumpMatrix.PutResultEmpty(nColumn, nRow);
    }
    else
    {
        rJumpMatrix.PutResultString(rMatrix.GetString(nColumn, nRow), nColumn, nRow);
    }
}

[[nodiscard]] inline std::optional<MatrixCoordinate> findFirstIfErrorCoordinate(
    const ScMatrix& rMatrix, bool bNAonly)
{
    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rMatrix.GetDimensions(nColumns, nRows);
    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
    {
        for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
        {
            if (matchesIfErrorPolicy(rMatrix.GetError(nColumn, nRow), bNAonly))
                return MatrixCoordinate { nColumn, nRow };
        }
    }

    return std::nullopt;
}

inline void initializeIfErrorJumpMatrix(const ScMatrix& rMatrix, ScJumpMatrix& rJumpMatrix,
    const short* pJump, short nJumpCount, bool bNAonly, const MatrixCoordinate& rFirstError)
{
    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rMatrix.GetDimensions(nColumns, nRows);

    SCSIZE nColumn = 0;
    SCSIZE nRow = 0;
    for (; nColumn < nColumns
           && (nColumn != rFirstError.mnColumn || nRow != rFirstError.mnRow);)
    {
        for (nRow = 0;
             nRow < nRows && (nColumn != rFirstError.mnColumn || nRow != rFirstError.mnRow);
             ++nRow)
        {
            storeJumpMatrixResult(rMatrix, rJumpMatrix, nColumn, nRow);
        }
        if (nColumn != rFirstError.mnColumn && nRow != rFirstError.mnRow)
            ++nColumn;
    }

    for (; nColumn < nColumns; ++nColumn)
    {
        for (; nRow < nRows; ++nRow)
        {
            const FormulaError eError = rMatrix.GetError(nColumn, nRow);
            if (matchesIfErrorPolicy(eError, bNAonly))
            {
                rJumpMatrix.SetJump(nColumn, nRow, 1.0, pJump[1], pJump[nJumpCount]);
            }
            else
            {
                storeJumpMatrixResult(rMatrix, rJumpMatrix, nColumn, nRow);
            }
        }
        nRow = 0;
    }
}

inline void initializeChooseJumpMatrix(
    const ScMatrix& rMatrix, ScJumpMatrix& rJumpMatrix, const short* pJump, short nJumpCount)
{
    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rMatrix.GetDimensions(nColumns, nRows);

    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
    {
        for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
        {
            double fDecisionValue = CreateDoubleError(FormulaError::NoValue);
            bool bHasJumpPath = false;
            if (rMatrix.IsValue(nColumn, nRow))
            {
                fDecisionValue = rMatrix.GetDouble(nColumn, nRow);
                if (const auto oJumpIndex
                    = spreadsheetengine::api::logic::normalizeChooseIndex(
                        fDecisionValue, nJumpCount))
                {
                    fDecisionValue = *oJumpIndex;
                    bHasJumpPath = true;
                }
                else if (std::isfinite(fDecisionValue))
                {
                    fDecisionValue = CreateDoubleError(
                        toFormulaError(spreadsheetengine::api::Error::IllegalArgument));
                }
            }

            if (bHasJumpPath)
            {
                rJumpMatrix.SetJump(nColumn, nRow, fDecisionValue,
                    pJump[static_cast<short>(fDecisionValue)], pJump[nJumpCount]);
            }
            else
            {
                rJumpMatrix.SetJump(
                    nColumn, nRow, fDecisionValue, pJump[nJumpCount], pJump[nJumpCount]);
            }
        }
    }
}

} // namespace spreadsheetengine::compat::libreoffice::jumpexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
