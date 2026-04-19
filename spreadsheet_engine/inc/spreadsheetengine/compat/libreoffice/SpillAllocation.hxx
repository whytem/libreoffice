/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <variant>

#include <address.hxx>
#include <document.hxx>

#include <spreadsheetengine/api/Host.hxx>

// Batch 5 host-facade adapter: bridges the engine's SpillRangeAllocator
// contract onto Calc's ScDocument + ScInterpreter::aPos.  Phase 5A
// admissions (FILTER / SORT / SORTBY / UNIQUE / TAKE / DROP) do not yet
// reach the allocator — they PushMatrix directly through the existing
// convertMatrixOperandToMatrixRef bridge, matching the legacy
// ScInterpreter::ScFilter etc. behavior.  Phase 5B (HSTACK / VSTACK /
// CHOOSECOLS / CHOOSEROWS / EXPAND / TOCOL / TOROW / WRAPCOLS / WRAPROWS
// / TEXTSPLIT) will consume this adapter once the engine's spill
// lifecycle is wired into the Calc dispatch tail.

namespace spreadsheetengine::compat::libreoffice::spillallocation
{

// Report the sheet position of the formula currently being evaluated.
// The libreoffice compat layer reads `ScInterpreter::aPos` at the call
// site and hands it to this helper as an (sheet, col, row) triple so the
// engine never needs to import ScAddress.
[[nodiscard]] inline api::CellAddress getCurrentFormulaPosition(
    const ScAddress& rPos)
{
    return { static_cast<api::SheetId>(rPos.Tab()),
             static_cast<api::ColumnIndex>(rPos.Col()),
             static_cast<api::RowIndex>(rPos.Row()) };
}

// Non-destructive collision probe.  Returns true when any cell strictly
// inside the closed rectangle defined by `rRange` (other than the anchor
// at `maStart`) holds a non-empty value — matching Calc's `#SPILL!`
// semantics from the live-array family.  Single-cell ranges always
// return false because the anchor itself is allowed to hold the formula.
[[nodiscard]] inline bool checkSpillCollision(
    const ScDocument& rDoc, const api::CellRange& rRange)
{
    if (!rRange.isNormalized() || rRange.isSingleCell())
        return false;

    const SCTAB nTab = static_cast<SCTAB>(rRange.maStart.mnSheet);
    const SCCOL nStartCol = static_cast<SCCOL>(rRange.maStart.mnColumn);
    const SCROW nStartRow = static_cast<SCROW>(rRange.maStart.mnRow);
    const SCCOL nEndCol = static_cast<SCCOL>(rRange.maEnd.mnColumn);
    const SCROW nEndRow = static_cast<SCROW>(rRange.maEnd.mnRow);

    for (SCROW nRow = nStartRow; nRow <= nEndRow; ++nRow)
    {
        for (SCCOL nCol = nStartCol; nCol <= nEndCol; ++nCol)
        {
            if (nCol == nStartCol && nRow == nStartRow)
                continue;
            if (!rDoc.HasData(nCol, nRow, nTab))
                continue;
            return true;
        }
    }
    return false;
}

// Allocate a spill rectangle anchored at `rAnchor` with the requested
// dimensions.  Returns the committed CellRange on success, or a
// SpillAllocationError describing the failure.  Phase 5A never reaches
// this path; the symbol is exported so Phase 5B can consume it.
[[nodiscard]] inline std::variant<api::CellRange, api::SpillAllocationError>
allocateSpillRange(const ScDocument& rDoc, const api::CellAddress& rAnchor,
                   const api::MatrixDimensions& rDimensions)
{
    if (rDimensions.mnColumns <= 0 || rDimensions.mnRows <= 0)
        return api::SpillAllocationError::InvalidShape;

    const SCTAB nTab = static_cast<SCTAB>(rAnchor.mnSheet);
    if (!rDoc.HasTable(nTab))
        return api::SpillAllocationError::InvalidRequest;

    const SCCOL nMaxCol = rDoc.GetSheetLimits().MaxCol();
    const SCROW nMaxRow = rDoc.GetSheetLimits().MaxRow();
    const SCCOL nEndCol
        = static_cast<SCCOL>(rAnchor.mnColumn) + static_cast<SCCOL>(rDimensions.mnColumns) - 1;
    const SCROW nEndRow
        = static_cast<SCROW>(rAnchor.mnRow) + static_cast<SCROW>(rDimensions.mnRows) - 1;
    if (nEndCol > nMaxCol || nEndRow > nMaxRow)
        return api::SpillAllocationError::OutOfBounds;

    api::CellRange aRange;
    aRange.maStart = rAnchor;
    aRange.maEnd = { rAnchor.mnSheet, static_cast<api::ColumnIndex>(nEndCol),
                     static_cast<api::RowIndex>(nEndRow) };
    if (checkSpillCollision(rDoc, aRange))
        return api::SpillAllocationError::Collision;
    return aRange;
}

// Record the bounds of the dynamic-array formula after a successful
// allocation.  No-op stub for Phase 5A — Phase 5B wires this into the
// dependency tracking / draw-layer machinery.
inline void markArrayFormulaBounds(ScDocument& /*rDoc*/, const api::CellRange& /*rRange*/)
{
    // Intentionally empty: Phase 5A admissions emit a matrix token
    // directly through PushMatrix and inherit the existing array-formula
    // bounds handling; no explicit call-out is required yet.
}

} // namespace spreadsheetengine::compat::libreoffice::spillallocation

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
