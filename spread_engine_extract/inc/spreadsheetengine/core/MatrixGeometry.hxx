/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Matrix.hxx>

#include <sal/types.h>

namespace spreadsheetengine::core::matrix
{

struct MatrixRange
{
    api::MatrixCoordinate maStart;
    api::MatrixCoordinate maEnd;
};

struct MatrixWritePlan
{
    MatrixRange maRange {};
    bool mbValid = false;
};

[[nodiscard]] constexpr api::MatrixDimensions makeDimensions(
    sal_uInt64 nColumns, sal_uInt64 nRows)
{
    return { static_cast<api::MatrixSize>(nColumns), static_cast<api::MatrixSize>(nRows) };
}

[[nodiscard]] constexpr api::MatrixCoordinate makeCoordinate(
    sal_uInt64 nColumn, sal_uInt64 nRow)
{
    return { static_cast<api::MatrixSize>(nColumn), static_cast<api::MatrixSize>(nRow) };
}

[[nodiscard]] constexpr MatrixRange makeRange(
    const api::MatrixCoordinate& rStart, const api::MatrixCoordinate& rEnd)
{
    return { rStart, rEnd };
}

[[nodiscard]] constexpr api::MatrixCoordinate offsetCoordinate(
    const api::MatrixCoordinate& rCoordinate, sal_uInt64 nColumnOffset, sal_uInt64 nRowOffset)
{
    return makeCoordinate(
        static_cast<sal_uInt64>(rCoordinate.mnColumn) + nColumnOffset,
        static_cast<sal_uInt64>(rCoordinate.mnRow) + nRowOffset);
}

[[nodiscard]] constexpr bool isValidRange(
    const api::MatrixDimensions& rDimensions, const MatrixRange& rRange)
{
    return api::isValidCoordinate(rDimensions, rRange.maStart)
           && api::isValidCoordinate(rDimensions, rRange.maEnd)
           && rRange.maStart.mnColumn <= rRange.maEnd.mnColumn
           && rRange.maStart.mnRow <= rRange.maEnd.mnRow;
}

[[nodiscard]] constexpr sal_uInt64 columnCount(const MatrixRange& rRange)
{
    return static_cast<sal_uInt64>(rRange.maEnd.mnColumn - rRange.maStart.mnColumn) + 1;
}

[[nodiscard]] constexpr sal_uInt64 rowCount(const MatrixRange& rRange)
{
    return static_cast<sal_uInt64>(rRange.maEnd.mnRow - rRange.maStart.mnRow) + 1;
}

[[nodiscard]] constexpr sal_uInt64 columnMajorLinearIndex(
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rCoordinate)
{
    return static_cast<sal_uInt64>(rDimensions.mnRows)
           * static_cast<sal_uInt64>(rCoordinate.mnColumn)
           + static_cast<sal_uInt64>(rCoordinate.mnRow);
}

[[nodiscard]] constexpr sal_uInt64 offsetColumnMajorLinearIndex(
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rCoordinate,
    sal_uInt64 nColumnOffset, sal_uInt64 nRowOffset)
{
    return columnMajorLinearIndex(
        rDimensions, offsetCoordinate(rCoordinate, nColumnOffset, nRowOffset));
}

[[nodiscard]] constexpr api::MatrixCoordinate coordinateFromLinearIndex(
    const api::MatrixDimensions& rDimensions, sal_uInt64 nIndex)
{
    const sal_uInt64 nRows = rDimensions.mnRows > 0 ? static_cast<sal_uInt64>(rDimensions.mnRows) : 0;
    const sal_uInt64 nColumn = nRows > 1 ? nIndex / nRows : nIndex;
    const sal_uInt64 nRow = nIndex - (nColumn * nRows);
    return makeCoordinate(nColumn, nRow);
}

[[nodiscard]] constexpr api::MatrixCoordinate coordinateFromTransposedLinearIndex(
    const api::MatrixDimensions& rDimensions, sal_uInt64 nIndex)
{
    const sal_uInt64 nColumns
        = rDimensions.mnColumns > 0 ? static_cast<sal_uInt64>(rDimensions.mnColumns) : 0;
    const sal_uInt64 nRow = nColumns > 1 ? nIndex / nColumns : nIndex;
    const sal_uInt64 nColumn = nIndex - (nRow * nColumns);
    return makeCoordinate(nColumn, nRow);
}

[[nodiscard]] constexpr MatrixRange columnVectorRange(
    const api::MatrixCoordinate& rStart, sal_uInt64 nCount)
{
    return makeRange(
        rStart,
        makeCoordinate(
            static_cast<sal_uInt64>(rStart.mnColumn),
            static_cast<sal_uInt64>(rStart.mnRow) + nCount - 1));
}

[[nodiscard]] constexpr bool canPlaceColumnVector(
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rStart, sal_uInt64 nCount)
{
    if (!nCount || !api::isValidCoordinate(rDimensions, rStart))
        return false;
    return isValidRange(rDimensions, columnVectorRange(rStart, nCount));
}

[[nodiscard]] constexpr MatrixWritePlan planRangeWrite(
    const api::MatrixDimensions& rDimensions, const MatrixRange& rRange)
{
    const bool bValid = isValidRange(rDimensions, rRange);
    return { bValid ? rRange : MatrixRange {}, bValid };
}

[[nodiscard]] constexpr MatrixWritePlan planColumnVectorWrite(
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rStart, sal_uInt64 nCount)
{
    const bool bValid = canPlaceColumnVector(rDimensions, rStart, nCount);
    return { bValid ? columnVectorRange(rStart, nCount) : MatrixRange {}, bValid };
}

[[nodiscard]] constexpr bool isSizeAllocatable(
    const api::MatrixDimensions& rDimensions, sal_uInt64 nElementsMax)
{
    if (!rDimensions.isAllocated())
        return false;

    const bool bZeroColumns = rDimensions.mnColumns == 0;
    const bool bZeroRows = rDimensions.mnRows == 0;
    if (bZeroColumns != bZeroRows)
        return false;
    if (bZeroColumns)
        return true;

    const sal_uInt64 nColumns = static_cast<sal_uInt64>(rDimensions.mnColumns);
    const sal_uInt64 nRows = static_cast<sal_uInt64>(rDimensions.mnRows);
    return nColumns <= (nElementsMax / nRows);
}

} // namespace spreadsheetengine::core::matrix

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
