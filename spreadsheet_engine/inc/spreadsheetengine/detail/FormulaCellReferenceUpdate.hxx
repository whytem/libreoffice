/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/ReferenceUpdate.hxx>

namespace spreadsheetengine::core::formulacellrefupdate
{

struct CopyUpdatePlan
{
    api::CellAddress maOldPosition;
    bool mbHasWork = false;
    bool mbNeedUndoCapture = false;
    bool mbNeedDirty = false;
    bool mbNeedCompile = false;

    [[nodiscard]] constexpr bool operator==(const CopyUpdatePlan& rOther) const = default;
};

struct MoveUpdatePlan
{
    api::CellAddress maOldPosition;
    bool mbHasWork = false;
    bool mbCellStateChanged = false;
    bool mbNeedUndoCapture = false;
    bool mbNeedDirty = false;
    bool mbNeedCompile = false;
    bool mbNeedEndListening = false;
    bool mbNeedStartListening = false;

    [[nodiscard]] constexpr bool operator==(const MoveUpdatePlan& rOther) const = default;
};

struct ShiftUpdatePlan
{
    bool mbHasWork = false;
    bool mbCellStateChanged = false;
    bool mbNeedUndoCapture = false;
    bool mbNeedPostponedDirty = false;
    bool mbNeedCompile = false;
    bool mbNeedEndListening = false;
    bool mbNeedSetNeedsListening = false;

    [[nodiscard]] constexpr bool operator==(const ShiftUpdatePlan& rOther) const = default;
};

struct InsertDeleteTabUpdatePlan
{
    bool mbPositionChanged = false;
    bool mbNeedEndListening = false;
    bool mbNeedAdjustCode = false;

    [[nodiscard]] constexpr bool operator==(const InsertDeleteTabUpdatePlan& rOther) const
        = default;
};

struct MoveTabUpdatePlan
{
    bool mbNeedEndListening = false;
    bool mbNeedAdjustCode = false;

    [[nodiscard]] constexpr bool operator==(const MoveTabUpdatePlan& rOther) const = default;
};

struct TransposePositionPlan
{
    api::CellRange maDestinationRange;
    api::CellAddress maOldPosition;
    bool mbPositionChanged = false;

    [[nodiscard]] constexpr bool operator==(const TransposePositionPlan& rOther) const = default;
};

struct Transpose3DFlagPlan
{
    bool mbRef1Flag3D = false;
    bool mbRef2Flag3D = false;

    [[nodiscard]] constexpr bool operator==(const Transpose3DFlagPlan& rOther) const = default;
};

struct TransposeFinishPlan
{
    bool mbNeedUndoCapture = false;
    bool mbNeedCompile = false;
    bool mbNeedDirty = false;
    bool mbNeedRestoreListening = false;

    [[nodiscard]] constexpr bool operator==(const TransposeFinishPlan& rOther) const = default;
};

struct GrowFinishPlan
{
    bool mbNeedCompile = false;
    bool mbNeedDirty = false;
    bool mbNeedRestoreListening = false;

    [[nodiscard]] constexpr bool operator==(const GrowFinishPlan& rOther) const = default;
};

[[nodiscard]] constexpr api::CellAddress computePreviousPosition(
    const api::CellAddress& rCurrentPosition, bool bPositionInMovedRange,
    api::ColumnIndex nColDelta, api::RowIndex nRowDelta, api::SheetId nSheetDelta)
{
    if (!bPositionInMovedRange)
        return rCurrentPosition;

    return { static_cast<api::SheetId>(rCurrentPosition.mnSheet - nSheetDelta),
        static_cast<api::ColumnIndex>(rCurrentPosition.mnColumn - nColDelta),
        static_cast<api::RowIndex>(rCurrentPosition.mnRow - nRowDelta) };
}

[[nodiscard]] constexpr CopyUpdatePlan makeCopyUpdatePlan(
    const api::CellAddress& rCurrentPosition, bool bPositionInMovedRange,
    api::ColumnIndex nColDelta, api::RowIndex nRowDelta, api::SheetId nSheetDelta,
    bool bHasReferences, bool bHasColRowNames, bool bRecalcOnRefMove, bool bCompile)
{
    const api::CellAddress aOldPosition = computePreviousPosition(
        rCurrentPosition, bPositionInMovedRange, nColDelta, nRowDelta, nSheetDelta);
    const bool bHasRefWork = bHasReferences || bHasColRowNames;
    if (!bHasRefWork && !bRecalcOnRefMove)
        return { aOldPosition, false, false, false, false };

    const bool bOnRefMove = bRecalcOnRefMove && aOldPosition != rCurrentPosition;
    const bool bNeedDirty = bOnRefMove || bCompile;
    return { aOldPosition, true, bOnRefMove, bNeedDirty, bCompile };
}

[[nodiscard]] constexpr MoveUpdatePlan makeMoveUpdatePlan(
    const api::CellAddress& rCurrentPosition, bool bPositionInMovedRange,
    api::ColumnIndex nColDelta, api::RowIndex nRowDelta, api::SheetId nSheetDelta,
    bool bHasReferences, bool bHasColRowNames, bool bRecalcOnRefMove, bool bValueChanged,
    bool bReferenceModified, bool bColRowNameCompile, bool bHasRelName, bool bCompile,
    bool bInsertingFromOtherDoc, bool bInDeleteUndo)
{
    const api::CellAddress aOldPosition = computePreviousPosition(
        rCurrentPosition, bPositionInMovedRange, nColDelta, nRowDelta, nSheetDelta);
    const bool bHasRefWork = bHasReferences || bHasColRowNames;
    if (!bHasRefWork && !bRecalcOnRefMove)
        return { aOldPosition, false, false, false, false, false, false, false };

    const bool bCellStateChanged = bValueChanged || bReferenceModified;
    const bool bOnRefMove = bRecalcOnRefMove && (bValueChanged || aOldPosition != rCurrentPosition);
    const bool bNeedEndListening
        = bHasRefWork && (bReferenceModified || bColRowNameCompile || bValueChanged || bHasRelName)
          && !(bInsertingFromOtherDoc && bPositionInMovedRange);
    const bool bNeedUndoCapture
        = !bPositionInMovedRange && (bValueChanged || bReferenceModified || bOnRefMove);
    const bool bNeedCompile = bCompile || bColRowNameCompile;
    const bool bNeedDirty = bReferenceModified || bColRowNameCompile
                            || (bValueChanged && bHasRelName) || bOnRefMove || bNeedCompile;
    return { aOldPosition, true, bCellStateChanged, bNeedUndoCapture, bNeedDirty, bNeedCompile,
        bNeedEndListening, bNeedEndListening && !bInDeleteUndo };
}

[[nodiscard]] constexpr ShiftUpdatePlan makeShiftUpdatePlan(
    const api::CellAddress& rOldPosition, const api::CellAddress& rCurrentPosition,
    bool bCellPositionChanged, bool bHasReferences, bool bHasColRowNames, bool bRecalcOnRefMove,
    bool bValueChanged, bool bReferenceModified, bool bRecompile, bool bHasRelName,
    bool bCompile, bool bInDeleteUndo)
{
    const bool bHasRefWork = bHasReferences || bHasColRowNames;
    if (!bHasRefWork && !bRecalcOnRefMove)
        return { false, bCellPositionChanged, false, false, false, false, false };

    const bool bCellStateChanged = bCellPositionChanged || bValueChanged || bReferenceModified;
    const bool bOnRefMove
        = bRecalcOnRefMove && (bValueChanged || rCurrentPosition != rOldPosition || bReferenceModified);
    const bool bNeedEndListening = bHasRefWork
                                   && (bReferenceModified || bRecompile
                                       || (bValueChanged && bInDeleteUndo) || bHasRelName);
    const bool bNeedCompile = bCompile || bRecompile;
    const bool bNeedPostponedDirty = bValueChanged || bRecompile || bOnRefMove || bNeedCompile;
    return { true, bCellStateChanged, bValueChanged || bOnRefMove, bNeedPostponedDirty,
        bNeedCompile, bNeedEndListening, bNeedEndListening && !bInDeleteUndo };
}

[[nodiscard]] constexpr InsertDeleteTabUpdatePlan makeInsertTabUpdatePlan(
    api::SheetId nCurrentTab, api::SheetId nInsertPos, api::SheetId nSheets, bool bClipOrUndo,
    bool bHasReferences, bool bAdjustCode)
{
    (void)nSheets;
    const bool bPositionChanged = nInsertPos <= nCurrentTab;
    if (bClipOrUndo || !bHasReferences)
        return { bPositionChanged, false, false };

    return { bPositionChanged, true, bAdjustCode };
}

[[nodiscard]] constexpr InsertDeleteTabUpdatePlan makeDeleteTabUpdatePlan(
    api::SheetId nCurrentTab, api::SheetId nDeletePos, api::SheetId nSheets, bool bClipOrUndo,
    bool bHasReferences, bool bAdjustCode)
{
    const bool bPositionChanged = nCurrentTab >= nDeletePos + nSheets;
    if (bClipOrUndo || !bHasReferences)
        return { bPositionChanged, false, false };

    return { bPositionChanged, true, bAdjustCode };
}

[[nodiscard]] constexpr MoveTabUpdatePlan makeMoveTabUpdatePlan(
    bool bClipOrUndo, bool bHasReferences, bool bAdjustCode)
{
    if (bClipOrUndo || !bHasReferences)
        return { false, false };

    return { true, bAdjustCode };
}

[[nodiscard]] constexpr bool shouldCompileAfterTabAdjust(bool bNameModified)
{
    return bNameModified;
}

[[nodiscard]] constexpr api::CellRange makeTransposeDestinationRange(
    const api::CellRange& rSource, const api::CellAddress& rDest)
{
    return { rDest,
        { static_cast<api::SheetId>(rDest.mnSheet + rSource.maEnd.mnSheet - rSource.maStart.mnSheet),
            static_cast<api::ColumnIndex>(rDest.mnColumn + rSource.maEnd.mnRow - rSource.maStart.mnRow),
            static_cast<api::RowIndex>(rDest.mnRow + rSource.maEnd.mnColumn - rSource.maStart.mnColumn) } };
}

[[nodiscard]] constexpr bool containsAddress(
    const api::CellRange& rRange, const api::CellAddress& rAddress)
{
    return rRange.maStart.mnSheet <= rAddress.mnSheet && rAddress.mnSheet <= rRange.maEnd.mnSheet
           && rRange.maStart.mnColumn <= rAddress.mnColumn
           && rAddress.mnColumn <= rRange.maEnd.mnColumn && rRange.maStart.mnRow <= rAddress.mnRow
           && rAddress.mnRow <= rRange.maEnd.mnRow;
}

[[nodiscard]] inline TransposePositionPlan makeTransposePositionPlan(
    const api::CellAddress& rCurrentPosition, const api::CellRange& rSource,
    const api::CellAddress& rDest, api::SheetId nSheetCount)
{
    const api::CellRange aDestinationRange = makeTransposeDestinationRange(rSource, rDest);
    api::CellAddress aOldPosition = rCurrentPosition;
    bool bPositionChanged = false;
    if (containsAddress(aDestinationRange, rCurrentPosition))
    {
        spreadsheetengine::api::ColumnIndex nColumn = aOldPosition.mnColumn;
        spreadsheetengine::api::RowIndex nRow = aOldPosition.mnRow;
        spreadsheetengine::api::SheetId nSheet = aOldPosition.mnSheet;
        spreadsheetengine::api::refupdate::doTranspose(
            nColumn, nRow, nSheet, nSheetCount, aDestinationRange, rSource.maStart);
        aOldPosition = { nSheet, nColumn, nRow };
        bPositionChanged = true;
    }

    return { aDestinationRange, aOldPosition, bPositionChanged };
}

[[nodiscard]] constexpr Transpose3DFlagPlan makeTranspose3DFlagPlan(
    const api::CellRange& rAbsoluteRange, api::SheetId nSourceStartTab, api::SheetId nDestTab,
    bool bPositionChanged, bool bRef1TabRelative, bool bRef2TabRelative)
{
    const bool bRef2Flag3D
        = rAbsoluteRange.maStart.mnSheet != rAbsoluteRange.maEnd.mnSheet || !bRef2TabRelative;
    const bool bRef1Flag3D
        = (nSourceStartTab != nDestTab && !bPositionChanged) || !bRef1TabRelative || bRef2Flag3D;
    return { bRef1Flag3D, bRef2Flag3D };
}

[[nodiscard]] constexpr TransposeFinishPlan makeTransposeFinishPlan(
    bool bReferenceChanged, bool bHasUndoDoc)
{
    return { bReferenceChanged && bHasUndoDoc, bReferenceChanged, bReferenceChanged,
        !bReferenceChanged };
}

[[nodiscard]] constexpr GrowFinishPlan makeGrowFinishPlan(bool bReferenceChanged)
{
    return { bReferenceChanged, bReferenceChanged, !bReferenceChanged };
}

} // namespace spreadsheetengine::core::formulacellrefupdate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
