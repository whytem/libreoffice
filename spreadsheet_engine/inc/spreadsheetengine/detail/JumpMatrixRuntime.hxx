/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/MatrixGeometry.hxx>

namespace spreadsheetengine::core::jumpmatrix
{

struct ResultCursor
{
    api::MatrixCoordinate maCoordinate {};
    bool mbStarted = false;
};

struct BufferWindow
{
    api::MatrixCoordinate maStart {};
    api::MatrixSize mnCount = 0;
};

struct ResultExpansionPlan
{
    api::MatrixDimensions maExpandedDimensions {};
    api::MatrixCoordinate maAdjustedCursor {};
    matrix::MatrixRange maNewColumnRange {};
    matrix::MatrixRange maNewRowRange {};
    bool mbNeedsExpansion = false;
    bool mbFillNewColumns = false;
    bool mbFillNewRows = false;
};

struct BufferedWritePlan
{
    BufferWindow maWindow {};
    bool mbBufferWrite = false;
    bool mbFlushCurrentType = false;
};

template <typename Column, typename Row, typename Count>
[[nodiscard]] constexpr BufferWindow bufferWindowFromState(Column nColumn, Row nRow, Count nCount)
{
    return { matrix::makeCoordinate(nColumn, nRow), static_cast<api::MatrixSize>(nCount) };
}

template <typename Column, typename Row>
constexpr void applyBufferWindowStart(const BufferWindow& rWindow, Column& rnColumn, Row& rnRow)
{
    rnColumn = rWindow.maStart.mnColumn;
    rnRow = rWindow.maStart.mnRow;
}

[[nodiscard]] constexpr bool normalizeJumpCoordinate(
    const api::MatrixDimensions& rDimensions, api::MatrixCoordinate& rCoordinate)
{
    return api::normalizeReplicatedCoordinate(rDimensions, rCoordinate);
}

[[nodiscard]] constexpr sal_uInt64 jumpEntryIndex(
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rCoordinate)
{
    return matrix::columnMajorLinearIndex(rDimensions, rCoordinate);
}

[[nodiscard]] constexpr bool advanceResultCursor(
    const api::MatrixDimensions& rResultDimensions, ResultCursor& rCursor)
{
    if (!rCursor.mbStarted)
    {
        rCursor.mbStarted = true;
        rCursor.maCoordinate = matrix::makeCoordinate(0, 0);
    }
    else
    {
        ++rCursor.maCoordinate.mnRow;
        if (rCursor.maCoordinate.mnRow >= rResultDimensions.mnRows)
        {
            rCursor.maCoordinate.mnRow = 0;
            ++rCursor.maCoordinate.mnColumn;
        }
    }

    return rCursor.maCoordinate.mnColumn < rResultDimensions.mnColumns;
}

[[nodiscard]] constexpr api::MatrixDimensions expandResultDimensions(
    const api::MatrixDimensions& rCurrent, const api::MatrixDimensions& rRequested)
{
    return {
        rCurrent.mnColumns >= rRequested.mnColumns ? rCurrent.mnColumns : rRequested.mnColumns,
        rCurrent.mnRows >= rRequested.mnRows ? rCurrent.mnRows : rRequested.mnRows
    };
}

[[nodiscard]] constexpr api::MatrixCoordinate adjustCursorAfterExpansion(
    const api::MatrixDimensions& rInputDimensions, const api::MatrixCoordinate& rCurrent,
    const api::MatrixDimensions& rExpandedResult)
{
    if (rInputDimensions.mnRows == 1 && rCurrent.mnColumn != 0)
        return { 0, static_cast<api::MatrixSize>(rExpandedResult.mnRows - 1) };

    return rCurrent;
}

[[nodiscard]] constexpr ResultExpansionPlan planResultExpansion(
    const api::MatrixDimensions& rInputDimensions, const api::MatrixDimensions& rCurrent,
    const api::MatrixDimensions& rRequested, const api::MatrixCoordinate& rCurrentCursor)
{
    const auto aExpandedDimensions = expandResultDimensions(rCurrent, rRequested);
    const bool bNeedsExpansion = aExpandedDimensions.mnColumns != rCurrent.mnColumns
                                 || aExpandedDimensions.mnRows != rCurrent.mnRows;

    ResultExpansionPlan aPlan;
    aPlan.maExpandedDimensions = aExpandedDimensions;
    aPlan.maAdjustedCursor
        = adjustCursorAfterExpansion(rInputDimensions, rCurrentCursor, aExpandedDimensions);
    aPlan.mbNeedsExpansion = bNeedsExpansion;
    aPlan.mbFillNewColumns = rCurrent.mnColumns < aExpandedDimensions.mnColumns && rCurrent.mnRows > 0;
    aPlan.mbFillNewRows = rCurrent.mnRows < aExpandedDimensions.mnRows && aExpandedDimensions.mnColumns > 0;

    if (aPlan.mbFillNewColumns)
    {
        aPlan.maNewColumnRange = matrix::makeRange(
            matrix::makeCoordinate(rCurrent.mnColumns, 0),
            matrix::makeCoordinate(aExpandedDimensions.mnColumns - 1, rCurrent.mnRows - 1));
    }

    if (aPlan.mbFillNewRows)
    {
        aPlan.maNewRowRange = matrix::makeRange(
            matrix::makeCoordinate(0, rCurrent.mnRows),
            matrix::makeCoordinate(aExpandedDimensions.mnColumns - 1, aExpandedDimensions.mnRows - 1));
    }

    return aPlan;
}

[[nodiscard]] constexpr bool shouldBufferResultWrites(
    const api::MatrixDimensions& rResultDimensions, api::MatrixSize nThreshold)
{
    return rResultDimensions.mnRows >= nThreshold;
}

[[nodiscard]] constexpr BufferWindow makeBufferWindow(
    const api::MatrixCoordinate& rStart, api::MatrixSize nCount)
{
    return { rStart, nCount };
}

[[nodiscard]] constexpr BufferWindow openBufferWindowIfEmpty(
    const BufferWindow& rWindow, const api::MatrixCoordinate& rRequestedCoordinate)
{
    return rWindow.mnCount > 0 ? rWindow : makeBufferWindow(rRequestedCoordinate, 0);
}

[[nodiscard]] constexpr BufferWindow appendedBufferWindow(const BufferWindow& rWindow)
{
    return { rWindow.maStart, static_cast<api::MatrixSize>(rWindow.mnCount + 1) };
}

[[nodiscard]] constexpr BufferWindow nextBufferedWriteWindow(
    const BufferWindow& rWindow, const api::MatrixCoordinate& rRequestedCoordinate)
{
    return appendedBufferWindow(openBufferWindowIfEmpty(rWindow, rRequestedCoordinate));
}

[[nodiscard]] constexpr bool isBufferedWriteContinuation(
    const BufferWindow& rWindow, const api::MatrixCoordinate& rCoordinate)
{
    return rWindow.mnCount > 0 && rCoordinate.mnColumn == rWindow.maStart.mnColumn
           && rCoordinate.mnRow
                  == static_cast<api::MatrixSize>(rWindow.maStart.mnRow + rWindow.mnCount);
}

[[nodiscard]] constexpr bool shouldFlushBufferedWindow(
    const BufferWindow& rWindow, bool bSameBufferType, const api::MatrixCoordinate& rCoordinate)
{
    return rWindow.mnCount > 0
           && (!bSameBufferType || !isBufferedWriteContinuation(rWindow, rCoordinate));
}

template <typename FlushAction, typename ResetAction>
bool flushBufferedWindowIfNeeded(const BufferWindow& rWindow, bool bSameBufferType,
                                 const api::MatrixCoordinate& rCoordinate, FlushAction aFlushAction,
                                 ResetAction aResetAction)
{
    if (!shouldFlushBufferedWindow(rWindow, bSameBufferType, rCoordinate))
        return false;

    aFlushAction();
    aResetAction();
    return true;
}

[[nodiscard]] constexpr BufferedWritePlan planBufferedResultWrite(
    const api::MatrixDimensions& rResultDimensions, api::MatrixSize nThreshold,
    const BufferWindow& rCurrentWindow, const api::MatrixCoordinate& rRequestedCoordinate)
{
    if (!shouldBufferResultWrites(rResultDimensions, nThreshold))
        return {};

    const bool bFlushCurrentType = shouldFlushBufferedWindow(rCurrentWindow, true, rRequestedCoordinate);

    BufferedWritePlan aPlan;
    aPlan.mbBufferWrite = true;
    aPlan.mbFlushCurrentType = bFlushCurrentType;
    aPlan.maWindow = bFlushCurrentType ? makeBufferWindow(rRequestedCoordinate, 1)
                                       : nextBufferedWriteWindow(rCurrentWindow, rRequestedCoordinate);
    return aPlan;
}

} // namespace spreadsheetengine::core::jumpmatrix

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
