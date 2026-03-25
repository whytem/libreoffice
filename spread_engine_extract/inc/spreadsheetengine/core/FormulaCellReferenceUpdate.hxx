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

} // namespace spreadsheetengine::core::formulacellrefupdate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
