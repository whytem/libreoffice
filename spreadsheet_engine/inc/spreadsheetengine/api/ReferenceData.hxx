/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>

#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::api::refdata
{

struct SheetLimits
{
    ColumnIndex mnMaxColumn = 0;
    RowIndex mnMaxRow = 0;
    SheetId mnMaxSheet = 0;

    [[nodiscard]] constexpr bool validColumn(ColumnIndex nColumn) const
    {
        return nColumn >= 0 && nColumn <= mnMaxColumn;
    }

    [[nodiscard]] constexpr bool validRow(RowIndex nRow) const
    {
        return nRow >= 0 && nRow <= mnMaxRow;
    }

    [[nodiscard]] constexpr bool validSheet(SheetId nSheet) const
    {
        return nSheet >= 0 && nSheet <= mnMaxSheet;
    }
};

struct SingleRefFlags
{
    bool mbColumnRelative = false;
    bool mbColumnDeleted = false;
    bool mbRowRelative = false;
    bool mbRowDeleted = false;
    bool mbSheetRelative = false;
    bool mbSheetDeleted = false;
    bool mbFlag3D = false;
    bool mbRelativeName = false;

    [[nodiscard]] constexpr bool operator==(const SingleRefFlags& rOther) const = default;
};

struct SingleRefData
{
    ColumnIndex mnColumn = 0;
    RowIndex mnRow = 0;
    SheetId mnSheet = 0;
    SingleRefFlags maFlags;

    [[nodiscard]] constexpr bool operator==(const SingleRefData& rOther) const = default;
};

struct ComplexRefData
{
    SingleRefData maRef1;
    SingleRefData maRef2;
    bool mbTrimToData = false;

    [[nodiscard]] constexpr bool operator==(const ComplexRefData& rOther) const = default;
};

[[nodiscard]] constexpr ColumnIndex displayedColumn(const SingleRefData& rData)
{
    return rData.maFlags.mbColumnDeleted ? -1 : rData.mnColumn;
}

[[nodiscard]] constexpr RowIndex displayedRow(const SingleRefData& rData)
{
    return rData.maFlags.mbRowDeleted ? -1 : rData.mnRow;
}

[[nodiscard]] constexpr SheetId displayedSheet(const SingleRefData& rData)
{
    return rData.maFlags.mbSheetDeleted ? -1 : rData.mnSheet;
}

[[nodiscard]] constexpr bool isDeleted(const SingleRefData& rData)
{
    return rData.maFlags.mbColumnDeleted || rData.maFlags.mbRowDeleted || rData.maFlags.mbSheetDeleted;
}

[[nodiscard]] inline CellAddress toAbsoluteAddress(
    const SingleRefData& rData, const SheetLimits& rLimits, const CellAddress& rPos)
{
    const ColumnIndex nColumn
        = rData.maFlags.mbColumnRelative ? rData.mnColumn + rPos.mnColumn : rData.mnColumn;
    const RowIndex nRow = rData.maFlags.mbRowRelative ? rData.mnRow + rPos.mnRow : rData.mnRow;
    const SheetId nSheet
        = rData.maFlags.mbSheetRelative ? rData.mnSheet + rPos.mnSheet : rData.mnSheet;

    return { rLimits.validSheet(nSheet) ? nSheet : -1, rLimits.validColumn(nColumn) ? nColumn : -1,
        rLimits.validRow(nRow) ? nRow : -1 };
}

inline void setAddress(
    SingleRefData& rData, const SheetLimits& rLimits, const CellAddress& rAddr, const CellAddress& rPos)
{
    rData.mnColumn = rData.maFlags.mbColumnRelative ? rAddr.mnColumn - rPos.mnColumn : rAddr.mnColumn;
    rData.maFlags.mbColumnDeleted = !rLimits.validColumn(rAddr.mnColumn);

    rData.mnRow = rData.maFlags.mbRowRelative ? rAddr.mnRow - rPos.mnRow : rAddr.mnRow;
    rData.maFlags.mbRowDeleted = !rLimits.validRow(rAddr.mnRow);

    rData.mnSheet = rData.maFlags.mbSheetRelative ? rAddr.mnSheet - rPos.mnSheet : rAddr.mnSheet;
    rData.maFlags.mbSheetDeleted = !rLimits.validSheet(rAddr.mnSheet);
}

inline void putInOrder(SingleRefData& rRef1, SingleRefData& rRef2, const CellAddress& rPos)
{
    const std::uint8_t kColumn = 1;
    const std::uint8_t kRow = 2;
    const std::uint8_t kSheet = 4;

    std::uint8_t nRelState1 = rRef1.maFlags.mbRelativeName
                               ? ((rRef1.maFlags.mbSheetRelative ? kSheet : 0)
                                  | (rRef1.maFlags.mbRowRelative ? kRow : 0)
                                  | (rRef1.maFlags.mbColumnRelative ? kColumn : 0))
                               : 0;
    std::uint8_t nRelState2 = rRef2.maFlags.mbRelativeName
                               ? ((rRef2.maFlags.mbSheetRelative ? kSheet : 0)
                                  | (rRef2.maFlags.mbRowRelative ? kRow : 0)
                                  | (rRef2.maFlags.mbColumnRelative ? kColumn : 0))
                               : 0;

    const auto swapDimension = [&](auto SingleRefFlags::* pRelative, auto SingleRefFlags::* pDeleted,
                                   auto& nValue1, auto& nValue2, auto nResolved1, auto nResolved2,
                                   auto nBase, std::uint8_t nMask) {
        if (nResolved2 >= nResolved1)
            return;

        nValue1 = (rRef2.maFlags.*pRelative) ? nResolved2 - nBase : nResolved2;
        nValue2 = (rRef1.maFlags.*pRelative) ? nResolved1 - nBase : nResolved1;

        if (rRef1.maFlags.mbRelativeName && (rRef1.maFlags.*pRelative))
            nRelState2 |= nMask;
        else
            nRelState2 &= ~nMask;
        if (rRef2.maFlags.mbRelativeName && (rRef2.maFlags.*pRelative))
            nRelState1 |= nMask;
        else
            nRelState1 &= ~nMask;

        std::swap(rRef1.maFlags.*pRelative, rRef2.maFlags.*pRelative);
        std::swap(rRef1.maFlags.*pDeleted, rRef2.maFlags.*pDeleted);
    };

    const ColumnIndex nColumn1
        = rRef1.maFlags.mbColumnRelative ? rPos.mnColumn + rRef1.mnColumn : rRef1.mnColumn;
    const ColumnIndex nColumn2
        = rRef2.maFlags.mbColumnRelative ? rPos.mnColumn + rRef2.mnColumn : rRef2.mnColumn;
    swapDimension(&SingleRefFlags::mbColumnRelative, &SingleRefFlags::mbColumnDeleted, rRef1.mnColumn,
        rRef2.mnColumn, nColumn1, nColumn2, rPos.mnColumn, kColumn);

    const RowIndex nRow1 = rRef1.maFlags.mbRowRelative ? rPos.mnRow + rRef1.mnRow : rRef1.mnRow;
    const RowIndex nRow2 = rRef2.maFlags.mbRowRelative ? rPos.mnRow + rRef2.mnRow : rRef2.mnRow;
    swapDimension(&SingleRefFlags::mbRowRelative, &SingleRefFlags::mbRowDeleted, rRef1.mnRow,
        rRef2.mnRow, nRow1, nRow2, rPos.mnRow, kRow);

    const SheetId nSheet1
        = rRef1.maFlags.mbSheetRelative ? rPos.mnSheet + rRef1.mnSheet : rRef1.mnSheet;
    const SheetId nSheet2
        = rRef2.maFlags.mbSheetRelative ? rPos.mnSheet + rRef2.mnSheet : rRef2.mnSheet;
    swapDimension(&SingleRefFlags::mbSheetRelative, &SingleRefFlags::mbSheetDeleted, rRef1.mnSheet,
        rRef2.mnSheet, nSheet1, nSheet2, rPos.mnSheet, kSheet);

    rRef1.maFlags.mbRelativeName = (nRelState1 != 0);
    rRef2.maFlags.mbRelativeName = (nRelState2 != 0);
}

[[nodiscard]] inline CellRange toAbsoluteRange(
    const ComplexRefData& rData, const SheetLimits& rLimits, const CellAddress& rPos)
{
    return { toAbsoluteAddress(rData.maRef1, rLimits, rPos), toAbsoluteAddress(rData.maRef2, rLimits, rPos) };
}

inline void setRange(
    ComplexRefData& rData, const SheetLimits& rLimits, const CellRange& rRange, const CellAddress& rPos)
{
    setAddress(rData.maRef1, rLimits, rRange.maStart, rPos);
    setAddress(rData.maRef2, rLimits, rRange.maEnd, rPos);
}

inline void putInOrder(ComplexRefData& rData, const CellAddress& rPos)
{
    putInOrder(rData.maRef1, rData.maRef2, rPos);
}

[[nodiscard]] constexpr bool isEntireColumn(const ComplexRefData& rData, const SheetLimits& rLimits)
{
    return displayedRow(rData.maRef1) == 0 && displayedRow(rData.maRef2) == rLimits.mnMaxRow
           && !rData.maRef1.maFlags.mbRowRelative && !rData.maRef2.maFlags.mbRowRelative;
}

[[nodiscard]] constexpr bool isEntireRow(const ComplexRefData& rData, const SheetLimits& rLimits)
{
    return displayedColumn(rData.maRef1) == 0
           && displayedColumn(rData.maRef2) == rLimits.mnMaxColumn
           && !rData.maRef1.maFlags.mbColumnRelative && !rData.maRef2.maFlags.mbColumnRelative;
}

inline ComplexRefData extendReferenceRange(ComplexRefData aRangeRef, const SingleRefData& rExtendRef,
    const SheetLimits& rLimits, const CellAddress& rPos)
{
    const bool bInherit3D
        = (aRangeRef.maRef1.maFlags.mbFlag3D && !aRangeRef.maRef2.maFlags.mbFlag3D
           && !rExtendRef.maFlags.mbFlag3D);
    CellRange aAbsRange = toAbsoluteRange(aRangeRef, rLimits, rPos);

    SingleRefData aExtend = rExtendRef;
    if (!rExtendRef.maFlags.mbFlag3D)
    {
        aExtend.maFlags.mbSheetRelative = aRangeRef.maRef2.maFlags.mbSheetRelative;
        aExtend.mnSheet = aRangeRef.maRef2.mnSheet;
    }

    const CellAddress aAbs = toAbsoluteAddress(aExtend, rLimits, rPos);
    aAbsRange.maStart.mnColumn = std::min(aAbsRange.maStart.mnColumn, aAbs.mnColumn);
    aAbsRange.maStart.mnRow = std::min(aAbsRange.maStart.mnRow, aAbs.mnRow);
    aAbsRange.maStart.mnSheet = std::min(aAbsRange.maStart.mnSheet, aAbs.mnSheet);
    aAbsRange.maEnd.mnColumn = std::max(aAbsRange.maEnd.mnColumn, aAbs.mnColumn);
    aAbsRange.maEnd.mnRow = std::max(aAbsRange.maEnd.mnRow, aAbs.mnRow);
    aAbsRange.maEnd.mnSheet = std::max(aAbsRange.maEnd.mnSheet, aAbs.mnSheet);

    if (aAbsRange.maEnd.mnColumn == aAbs.mnColumn)
        aRangeRef.maRef2.maFlags.mbColumnRelative = rExtendRef.maFlags.mbColumnRelative;
    if (aAbsRange.maEnd.mnRow == aAbs.mnRow)
        aRangeRef.maRef2.maFlags.mbRowRelative = rExtendRef.maFlags.mbRowRelative;
    if (aAbsRange.maStart.mnSheet == aAbs.mnSheet && rExtendRef.maFlags.mbFlag3D)
        aRangeRef.maRef1.maFlags.mbSheetRelative = rExtendRef.maFlags.mbSheetRelative;
    if (aAbsRange.maEnd.mnSheet == aAbs.mnSheet)
    {
        aRangeRef.maRef2.maFlags.mbSheetRelative
            = bInherit3D ? aRangeRef.maRef1.maFlags.mbSheetRelative
                         : rExtendRef.maFlags.mbSheetRelative;
    }
    if (aAbsRange.maStart.mnSheet != rPos.mnSheet
        || aAbsRange.maStart.mnSheet != aAbsRange.maEnd.mnSheet)
    {
        aRangeRef.maRef1.maFlags.mbFlag3D = true;
    }
    if (aAbsRange.maStart.mnSheet != aAbsRange.maEnd.mnSheet)
        aRangeRef.maRef2.maFlags.mbFlag3D = true;
    if (rExtendRef.maFlags.mbFlag3D)
        aRangeRef.maRef1.maFlags.mbFlag3D = true;
    if (rExtendRef.maFlags.mbRelativeName)
        aRangeRef.maRef2.maFlags.mbRelativeName = true;

    setRange(aRangeRef, rLimits, aAbsRange, rPos);
    return aRangeRef;
}

inline ComplexRefData extendReferenceRange(ComplexRefData aRangeRef, const ComplexRefData& rExtendRef,
    const SheetLimits& rLimits, const CellAddress& rPos)
{
    return extendReferenceRange(
        extendReferenceRange(aRangeRef, rExtendRef.maRef1, rLimits, rPos), rExtendRef.maRef2,
        rLimits, rPos);
}

} // namespace spreadsheetengine::api::refdata

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
