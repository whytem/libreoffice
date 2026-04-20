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

// Populate a per-cell JumpMatrix for `IF(matrix-condition, ...)`. Mirrors
// `ScMatrixImpl::IfJump`: boolean and numeric cells become TRUE when
// finite-nonzero, strings become FALSE with a `NoValue` marker that the
// matrix-frame iteration keeps as a double error, empty cells are FALSE-zero.
// The caller is responsible for constructing the ScJumpMatrix with the matrix
// dimensions beforehand.
inline void initializeIfJumpMatrix(
    const ScMatrix& rMatrix, ScJumpMatrix& rJumpMatrix, const short* pJump, short nJumpCount)
{
    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rMatrix.GetDimensions(nColumns, nRows);

    for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
    {
        for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
        {
            bool bIsValue;
            bool bTrue;
            double fVal;
            if (rMatrix.IsBoolean(nColumn, nRow))
            {
                fVal = rMatrix.GetDouble(nColumn, nRow) != 0.0 ? 1.0 : 0.0;
                bIsValue = std::isfinite(fVal);
                bTrue = bIsValue && (fVal != 0.0);
                if (bTrue)
                    fVal = 1.0;
            }
            else if (rMatrix.IsValue(nColumn, nRow))
            {
                fVal = rMatrix.GetDouble(nColumn, nRow);
                bIsValue = std::isfinite(fVal);
                bTrue = bIsValue && (fVal != 0.0);
                if (bTrue)
                    fVal = 1.0;
            }
            else if (rMatrix.IsEmpty(nColumn, nRow)
                     || rMatrix.IsEmptyPath(nColumn, nRow))
            {
                // Legacy ScMatrixImpl::IfJump dispatches on the mdds-level
                // `element_empty` type, which covers both `empty` /
                // `empty cell` / `empty result` (ScMatrix::IsEmpty) and
                // `empty path` (ScMatrix::IsEmptyPath). In either case the
                // cell is treated as a 0-valued FALSE, so the matrix-frame
                // iteration carries a plain 0 up to the outer consumer.
                bIsValue = true;
                bTrue = false;
                fVal = 0.0;
            }
            else
            {
                // String: treated as error by legacy ScMatrixImpl::IfJump.
                bIsValue = false;
                bTrue = false;
                fVal = CreateDoubleError(FormulaError::NoValue);
            }

            if (bTrue)
            {
                // THEN path if the condition is truthy; fall through to
                // the endpoint if the opcode has no THEN slot.
                if (nJumpCount >= 2)
                    rJumpMatrix.SetJump(
                        nColumn, nRow, fVal, pJump[1], pJump[nJumpCount]);
                else
                    rJumpMatrix.SetJump(
                        nColumn, nRow, fVal, pJump[nJumpCount], pJump[nJumpCount]);
            }
            else
            {
                // ELSE path only taken when the cell is a well-defined
                // value; string/error cells route directly to the endpoint
                // so the per-cell result carries the DoubleError marker.
                if (nJumpCount == 3 && bIsValue)
                    rJumpMatrix.SetJump(
                        nColumn, nRow, fVal, pJump[2], pJump[nJumpCount]);
                else
                    rJumpMatrix.SetJump(
                        nColumn, nRow, fVal, pJump[nJumpCount], pJump[nJumpCount]);
            }
        }
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
