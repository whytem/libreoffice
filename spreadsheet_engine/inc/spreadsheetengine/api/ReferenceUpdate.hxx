/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <utility>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/ReferenceData.hxx>

namespace spreadsheetengine::api::refupdate
{

enum class UpdateMode : sal_uInt8
{
    InsertDelete,
    Move,
    Reorder
};

enum class UpdateResult : sal_uInt8
{
    Nothing,
    Updated,
    Invalid,
    Sticky
};

[[nodiscard]] constexpr bool containsRange(
    const CellRange& rOuter, const CellRange& rInner)
{
    return rOuter.maStart.mnSheet <= rInner.maStart.mnSheet
           && rInner.maEnd.mnSheet <= rOuter.maEnd.mnSheet
           && rOuter.maStart.mnColumn <= rInner.maStart.mnColumn
           && rInner.maEnd.mnColumn <= rOuter.maEnd.mnColumn
           && rOuter.maStart.mnRow <= rInner.maStart.mnRow
           && rInner.maEnd.mnRow <= rOuter.maEnd.mnRow;
}

inline void putRangeInOrder(CellRange& rRange)
{
    if (rRange.maStart.mnColumn > rRange.maEnd.mnColumn)
        std::swap(rRange.maStart.mnColumn, rRange.maEnd.mnColumn);
    if (rRange.maStart.mnRow > rRange.maEnd.mnRow)
        std::swap(rRange.maStart.mnRow, rRange.maEnd.mnRow);
    if (rRange.maStart.mnSheet > rRange.maEnd.mnSheet)
        std::swap(rRange.maStart.mnSheet, rRange.maEnd.mnSheet);
}

template <typename T>
constexpr void moveItWrap(T& rRef, T nMask)
{
    if (rRef < 0)
        rRef = static_cast<T>(rRef + nMask + 1);
    else if (rRef > nMask)
        rRef = static_cast<T>(rRef - nMask - 1);
}

template <typename R, typename S, typename U>
constexpr bool moveStart(R& rRef, U nStart, S nDelta, U nMask, bool bShrink = true)
{
    bool bCut = false;
    if (rRef >= nStart)
        rRef = static_cast<R>(rRef + nDelta);
    else if (nDelta < 0 && bShrink && rRef >= nStart + nDelta)
        rRef = static_cast<R>(nStart + nDelta);
    if (rRef < 0)
    {
        rRef = 0;
        bCut = true;
    }
    else if (rRef > nMask)
    {
        rRef = nMask;
        bCut = true;
    }
    return bCut;
}

template <typename R, typename S, typename U>
constexpr bool moveEnd(R& rRef, U nStart, S nDelta, U nMask, bool bShrink = true)
{
    bool bCut = false;
    if (rRef >= nStart)
        rRef = static_cast<R>(rRef + nDelta);
    else if (nDelta < 0 && bShrink && rRef >= nStart + nDelta)
        rRef = static_cast<R>(nStart + nDelta - 1);
    if (rRef < 0)
    {
        rRef = 0;
        bCut = true;
    }
    else if (rRef > nMask)
    {
        rRef = nMask;
        bCut = true;
    }
    return bCut;
}

template <typename R, typename S, typename U>
constexpr bool moveReorder(R& rRef, U nStart, U nEnd, S nDelta)
{
    if (rRef >= nStart && rRef <= nEnd)
    {
        rRef = static_cast<R>(rRef + nDelta);
        return true;
    }

    if (nDelta > 0)
    {
        if (rRef >= nStart && rRef <= nEnd + nDelta)
        {
            if (rRef <= nEnd)
                rRef = static_cast<R>(rRef + nDelta);
            else
                rRef = static_cast<R>(rRef - (nEnd - nStart + 1));
            return true;
        }
    }
    else
    {
        if (rRef >= nStart + nDelta && rRef <= nEnd)
        {
            if (rRef >= nStart)
                rRef = static_cast<R>(rRef + nDelta);
            else
                rRef = static_cast<R>(rRef + (nEnd - nStart + 1));
            return true;
        }
    }

    return false;
}

template <typename R, typename S, typename U>
constexpr bool moveItCut(R& rRef, S nDelta, U nMask)
{
    bool bCut = false;
    rRef = static_cast<R>(rRef + nDelta);
    if (rRef < 0)
    {
        rRef = 0;
        bCut = true;
    }
    else if (rRef > nMask)
    {
        rRef = nMask;
        bCut = true;
    }
    return bCut;
}

template <typename R, typename S, typename U>
[[nodiscard]] constexpr bool isExpand(R n1, R n2, U nStart, S nDelta)
{
    return nDelta > 0 && n1 < n2
           && ((nStart <= n1 && n1 < nStart + nDelta) || (n2 + 1 == nStart));
}

template <typename R, typename S, typename U>
constexpr void expand(R& r1, R& r2, U nStart, S nDelta)
{
    if (r2 + 1 == nStart)
    {
        r2 = static_cast<R>(r2 + nDelta);
        return;
    }

    r1 = static_cast<R>(r1 - nDelta);
}

[[nodiscard]] constexpr UpdateResult normalizeUpdateResult(
    UpdateResult eResult, const CellRange& rOriginal, const CellRange& rUpdated)
{
    if (eResult == UpdateResult::Nothing && rOriginal != rUpdated)
        return UpdateResult::Updated;
    return eResult;
}

[[nodiscard]] inline UpdateResult updateReference(
    UpdateMode eMode, const CellRange& rWhere, ColumnIndex nDx, RowIndex nDy, SheetId nDz,
    ColumnIndex nMaxCol, RowIndex nMaxRow, SheetId nMaxTab, bool bExpandRefs,
    CellRange& rRef)
{
    const CellRange aOriginal = rRef;
    UpdateResult eResult = UpdateResult::Nothing;
    bool bCut1 = false;
    bool bCut2 = false;

    if (eMode == UpdateMode::InsertDelete)
    {
        if (nDx && (rRef.maStart.mnRow >= rWhere.maStart.mnRow)
            && (rRef.maEnd.mnRow <= rWhere.maEnd.mnRow)
            && (rRef.maStart.mnSheet >= rWhere.maStart.mnSheet)
            && (rRef.maEnd.mnSheet <= rWhere.maEnd.mnSheet))
        {
            const bool bExpand = bExpandRefs
                                 && isExpand(
                                     rRef.maStart.mnColumn, rRef.maEnd.mnColumn,
                                     rWhere.maStart.mnColumn, nDx);
            bCut1 = moveStart(rRef.maStart.mnColumn, rWhere.maStart.mnColumn, nDx, nMaxCol);
            bCut2 = moveEnd(rRef.maEnd.mnColumn, rWhere.maStart.mnColumn, nDx, nMaxCol);
            if (rRef.maEnd.mnColumn < rRef.maStart.mnColumn)
            {
                eResult = UpdateResult::Invalid;
                rRef.maEnd.mnColumn = rRef.maStart.mnColumn;
            }
            else if (bCut2 && rRef.maEnd.mnColumn == 0)
                eResult = UpdateResult::Invalid;
            else if (bCut1 || bCut2)
                eResult = UpdateResult::Updated;
            if (bExpand)
            {
                expand(rRef.maStart.mnColumn, rRef.maEnd.mnColumn, rWhere.maStart.mnColumn, nDx);
                eResult = UpdateResult::Updated;
            }
            if (eResult != UpdateResult::Nothing && aOriginal.maStart.mnColumn == 0
                && aOriginal.maEnd.mnColumn == nMaxCol)
            {
                eResult = UpdateResult::Sticky;
                rRef.maStart.mnColumn = aOriginal.maStart.mnColumn;
                rRef.maEnd.mnColumn = aOriginal.maEnd.mnColumn;
            }
            else if (aOriginal.maEnd.mnColumn == nMaxCol
                     && aOriginal.maStart.mnColumn < nMaxCol)
            {
                rRef.maEnd.mnColumn = aOriginal.maEnd.mnColumn;
                if (eResult == UpdateResult::Nothing)
                    eResult = UpdateResult::Sticky;
            }
        }

        if (nDy && (rRef.maStart.mnColumn >= rWhere.maStart.mnColumn)
            && (rRef.maEnd.mnColumn <= rWhere.maEnd.mnColumn)
            && (rRef.maStart.mnSheet >= rWhere.maStart.mnSheet)
            && (rRef.maEnd.mnSheet <= rWhere.maEnd.mnSheet))
        {
            const bool bExpand = bExpandRefs
                                 && isExpand(
                                     rRef.maStart.mnRow, rRef.maEnd.mnRow,
                                     rWhere.maStart.mnRow, nDy);
            bCut1 = moveStart(rRef.maStart.mnRow, rWhere.maStart.mnRow, nDy, nMaxRow);
            bCut2 = moveEnd(rRef.maEnd.mnRow, rWhere.maStart.mnRow, nDy, nMaxRow);
            if (rRef.maEnd.mnRow < rRef.maStart.mnRow)
            {
                eResult = UpdateResult::Invalid;
                rRef.maEnd.mnRow = rRef.maStart.mnRow;
            }
            else if (bCut2 && rRef.maEnd.mnRow == 0)
                eResult = UpdateResult::Invalid;
            else if (bCut1 || bCut2)
                eResult = UpdateResult::Updated;
            if (bExpand)
            {
                expand(rRef.maStart.mnRow, rRef.maEnd.mnRow, rWhere.maStart.mnRow, nDy);
                eResult = UpdateResult::Updated;
            }
            if (eResult != UpdateResult::Nothing && aOriginal.maStart.mnRow == 0
                && aOriginal.maEnd.mnRow == nMaxRow)
            {
                eResult = UpdateResult::Sticky;
                rRef.maStart.mnRow = aOriginal.maStart.mnRow;
                rRef.maEnd.mnRow = aOriginal.maEnd.mnRow;
            }
            else if (aOriginal.maEnd.mnRow == nMaxRow && aOriginal.maStart.mnRow < nMaxRow)
            {
                rRef.maEnd.mnRow = aOriginal.maEnd.mnRow;
                if (eResult == UpdateResult::Nothing)
                    eResult = UpdateResult::Sticky;
            }
        }

        if (nDz && (rRef.maStart.mnColumn >= rWhere.maStart.mnColumn)
            && (rRef.maEnd.mnColumn <= rWhere.maEnd.mnColumn)
            && (rRef.maStart.mnRow >= rWhere.maStart.mnRow)
            && (rRef.maEnd.mnRow <= rWhere.maEnd.mnRow))
        {
            const SheetId nAdjustedMaxTab = static_cast<SheetId>(nMaxTab + nDz);
            const bool bExpand = bExpandRefs
                                 && isExpand(
                                     rRef.maStart.mnSheet, rRef.maEnd.mnSheet,
                                     rWhere.maStart.mnSheet, nDz);
            bCut1 = moveStart(
                rRef.maStart.mnSheet, rWhere.maStart.mnSheet, nDz, nAdjustedMaxTab, false);
            bCut2 = moveEnd(
                rRef.maEnd.mnSheet, rWhere.maStart.mnSheet, nDz, nAdjustedMaxTab, false);
            if (rRef.maEnd.mnSheet < rRef.maStart.mnSheet)
            {
                eResult = UpdateResult::Invalid;
                rRef.maEnd.mnSheet = rRef.maStart.mnSheet;
            }
            else if (bCut1 || bCut2)
                eResult = UpdateResult::Updated;
            if (bExpand)
            {
                expand(
                    rRef.maStart.mnSheet, rRef.maEnd.mnSheet, rWhere.maStart.mnSheet, nDz);
                eResult = UpdateResult::Updated;
            }
        }
    }
    else if (eMode == UpdateMode::Move)
    {
        if ((rRef.maStart.mnColumn >= rWhere.maStart.mnColumn - nDx)
            && (rRef.maStart.mnRow >= rWhere.maStart.mnRow - nDy)
            && (rRef.maStart.mnSheet >= rWhere.maStart.mnSheet - nDz)
            && (rRef.maEnd.mnColumn <= rWhere.maEnd.mnColumn - nDx)
            && (rRef.maEnd.mnRow <= rWhere.maEnd.mnRow - nDy)
            && (rRef.maEnd.mnSheet <= rWhere.maEnd.mnSheet - nDz))
        {
            if (nDx)
            {
                bCut1 = moveItCut(rRef.maStart.mnColumn, nDx, nMaxCol);
                bCut2 = moveItCut(rRef.maEnd.mnColumn, nDx, nMaxCol);
                if (bCut1 || bCut2)
                    eResult = UpdateResult::Updated;
                if (eResult != UpdateResult::Nothing && aOriginal.maStart.mnColumn == 0
                    && aOriginal.maEnd.mnColumn == nMaxCol)
                {
                    eResult = UpdateResult::Sticky;
                    rRef.maStart.mnColumn = aOriginal.maStart.mnColumn;
                    rRef.maEnd.mnColumn = aOriginal.maEnd.mnColumn;
                }
            }
            if (nDy)
            {
                bCut1 = moveItCut(rRef.maStart.mnRow, nDy, nMaxRow);
                bCut2 = moveItCut(rRef.maEnd.mnRow, nDy, nMaxRow);
                if (bCut1 || bCut2)
                    eResult = UpdateResult::Updated;
                if (eResult != UpdateResult::Nothing && aOriginal.maStart.mnRow == 0
                    && aOriginal.maEnd.mnRow == nMaxRow)
                {
                    eResult = UpdateResult::Sticky;
                    rRef.maStart.mnRow = aOriginal.maStart.mnRow;
                    rRef.maEnd.mnRow = aOriginal.maEnd.mnRow;
                }
            }
            if (nDz)
            {
                bCut1 = moveItCut(rRef.maStart.mnSheet, nDz, nMaxTab);
                bCut2 = moveItCut(rRef.maEnd.mnSheet, nDz, nMaxTab);
                if (bCut1 || bCut2)
                    eResult = UpdateResult::Updated;
            }
        }
    }
    else if (eMode == UpdateMode::Reorder)
    {
        if (nDz && (rRef.maStart.mnColumn >= rWhere.maStart.mnColumn)
            && (rRef.maEnd.mnColumn <= rWhere.maEnd.mnColumn)
            && (rRef.maStart.mnRow >= rWhere.maStart.mnRow)
            && (rRef.maEnd.mnRow <= rWhere.maEnd.mnRow))
        {
            bCut1 = moveReorder(
                rRef.maStart.mnSheet, rWhere.maStart.mnSheet, rWhere.maEnd.mnSheet, nDz);
            bCut2 = moveReorder(
                rRef.maEnd.mnSheet, rWhere.maStart.mnSheet, rWhere.maEnd.mnSheet, nDz);
            if (bCut1 || bCut2)
                eResult = UpdateResult::Updated;
        }
    }

    return normalizeUpdateResult(eResult, aOriginal, rRef);
}

inline void moveRelativeWrap(
    refdata::ComplexRefData& rRef, const refdata::SheetLimits& rLimits,
    const CellAddress& rPos, ColumnIndex nMaxCol, RowIndex nMaxRow, SheetId nMaxSheet)
{
    CellRange aAbsRange = refdata::toAbsoluteRange(rRef, rLimits, rPos);
    if (rRef.maRef1.maFlags.mbColumnRelative)
        moveItWrap(aAbsRange.maStart.mnColumn, nMaxCol);
    if (rRef.maRef2.maFlags.mbColumnRelative)
        moveItWrap(aAbsRange.maEnd.mnColumn, nMaxCol);
    if (rRef.maRef1.maFlags.mbRowRelative)
        moveItWrap(aAbsRange.maStart.mnRow, nMaxRow);
    if (rRef.maRef2.maFlags.mbRowRelative)
        moveItWrap(aAbsRange.maEnd.mnRow, nMaxRow);
    if (rRef.maRef1.maFlags.mbSheetRelative)
        moveItWrap(aAbsRange.maStart.mnSheet, nMaxSheet);
    if (rRef.maRef2.maFlags.mbSheetRelative)
        moveItWrap(aAbsRange.maEnd.mnSheet, nMaxSheet);

    putRangeInOrder(aAbsRange);
    refdata::setRange(rRef, rLimits, aAbsRange, rPos);
}

inline void doTranspose(
    ColumnIndex& rCol, RowIndex& rRow, SheetId& rSheet, SheetId nSheetCount,
    const CellRange& rSource, const CellAddress& rDest)
{
    const SheetId nSheetDelta = rDest.mnSheet - rSource.maStart.mnSheet;
    if (nSheetDelta && nSheetCount > 0)
    {
        SheetId nNewSheet = rSheet + nSheetDelta;
        while (nNewSheet < 0)
            nNewSheet = static_cast<SheetId>(nNewSheet + nSheetCount);
        while (nNewSheet >= nSheetCount)
            nNewSheet = static_cast<SheetId>(nNewSheet - nSheetCount);
        rSheet = nNewSheet;
    }

    const ColumnIndex nRelX = rCol - rSource.maStart.mnColumn;
    const RowIndex nRelY = rRow - rSource.maStart.mnRow;

    rCol = static_cast<ColumnIndex>(rDest.mnColumn + nRelY);
    rRow = static_cast<RowIndex>(rDest.mnRow + nRelX);
}

[[nodiscard]] inline bool updateTranspose(
    SheetId nSheetCount, const CellRange& rSource, const CellAddress& rDest,
    CellRange& rRef)
{
    if (!containsRange(rSource, rRef))
        return false;

    doTranspose(
        rRef.maStart.mnColumn, rRef.maStart.mnRow, rRef.maStart.mnSheet,
        nSheetCount, rSource, rDest);
    doTranspose(
        rRef.maEnd.mnColumn, rRef.maEnd.mnRow, rRef.maEnd.mnSheet,
        nSheetCount, rSource, rDest);
    return true;
}

[[nodiscard]] constexpr bool shouldUpdateGrowColumns(
    const CellRange& rArea, ColumnIndex nGrowX, const CellRange& rRef)
{
    return nGrowX && rRef.maStart.mnColumn == rArea.maStart.mnColumn
           && rRef.maEnd.mnColumn == rArea.maEnd.mnColumn
           && rRef.maStart.mnRow >= rArea.maStart.mnRow
           && rRef.maEnd.mnRow <= rArea.maEnd.mnRow
           && rRef.maStart.mnSheet >= rArea.maStart.mnSheet
           && rRef.maEnd.mnSheet <= rArea.maEnd.mnSheet;
}

[[nodiscard]] constexpr bool shouldUpdateGrowRows(
    const CellRange& rArea, RowIndex nGrowY, const CellRange& rRef)
{
    return nGrowY && rRef.maStart.mnColumn >= rArea.maStart.mnColumn
           && rRef.maEnd.mnColumn <= rArea.maEnd.mnColumn
           && (rRef.maStart.mnRow == rArea.maStart.mnRow
               || rRef.maStart.mnRow == rArea.maStart.mnRow + 1)
           && rRef.maEnd.mnRow == rArea.maEnd.mnRow
           && rRef.maStart.mnSheet >= rArea.maStart.mnSheet
           && rRef.maEnd.mnSheet <= rArea.maEnd.mnSheet;
}

[[nodiscard]] inline bool updateGrow(
    const CellRange& rArea, ColumnIndex nGrowX, RowIndex nGrowY, CellRange& rRef)
{
    const bool bUpdateColumns = shouldUpdateGrowColumns(rArea, nGrowX, rRef);
    const bool bUpdateRows = shouldUpdateGrowRows(rArea, nGrowY, rRef);
    if (bUpdateColumns)
    {
        rRef.maEnd.mnColumn = static_cast<ColumnIndex>(rRef.maEnd.mnColumn + nGrowX);
    }
    if (bUpdateRows)
    {
        rRef.maEnd.mnRow = static_cast<RowIndex>(rRef.maEnd.mnRow + nGrowY);
    }
    return bUpdateColumns || bUpdateRows;
}

} // namespace spreadsheetengine::api::refupdate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
