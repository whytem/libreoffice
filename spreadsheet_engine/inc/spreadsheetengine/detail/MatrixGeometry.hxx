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

#include <algorithm>
#include <spreadsheetengine/api/Types.hxx>
#include <iterator>

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

struct BroadcastExecutionPlan
{
    MatrixRange maOperationRange {};
    sal_uInt64 mnRowRepeats = 0;
    sal_uInt64 mnColumnRepeats = 0;
    bool mbReplicated = false;
    bool mbValid = false;
};

struct ValidRunPlan
{
    sal_uInt64 mnStartIndex = 0;
    sal_uInt64 mnLength = 0;
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

[[nodiscard]] constexpr BroadcastExecutionPlan planBroadcastExecution(
    const api::MatrixDimensions& rSourceDimensions, const api::MatrixDimensions& rTargetDimensions)
{
    if (!rSourceDimensions.isAllocated() || !rTargetDimensions.isAllocated() || rTargetDimensions.isEmpty())
        return {};

    const bool bReplicated = rSourceDimensions.mnColumns == 1 || rSourceDimensions.mnRows == 1;
    if (!bReplicated)
    {
        return { makeRange(makeCoordinate(0, 0),
                           makeCoordinate(static_cast<sal_uInt64>(rTargetDimensions.mnColumns) - 1,
                                          static_cast<sal_uInt64>(rTargetDimensions.mnRows) - 1)),
                 1, 1, false, true };
    }

    const sal_uInt64 nOperationColumns = std::min(static_cast<sal_uInt64>(rSourceDimensions.mnColumns),
                                                  static_cast<sal_uInt64>(rTargetDimensions.mnColumns));
    const sal_uInt64 nOperationRows = std::min(static_cast<sal_uInt64>(rSourceDimensions.mnRows),
                                               static_cast<sal_uInt64>(rTargetDimensions.mnRows));
    if (!nOperationColumns || !nOperationRows)
        return {};

    return { makeRange(makeCoordinate(0, 0),
                       makeCoordinate(nOperationColumns - 1, nOperationRows - 1)),
             rSourceDimensions.mnRows == 1 ? static_cast<sal_uInt64>(rTargetDimensions.mnRows) : 1,
             rSourceDimensions.mnColumns == 1 ? static_cast<sal_uInt64>(rTargetDimensions.mnColumns)
                                              : 1,
             true, true };
}

template <typename ValidContainer>
[[nodiscard]] auto planContiguousValidRun(const ValidContainer& rValid, sal_uInt64 nStartIndex)
    -> ValidRunPlan
{
    if (nStartIndex >= rValid.size() || !rValid[nStartIndex])
        return {};

    auto aBegin = std::next(rValid.begin(), nStartIndex);
    auto aEnd = std::find(aBegin, rValid.end(), false);
    return { nStartIndex, static_cast<sal_uInt64>(std::distance(aBegin, aEnd)), true };
}

[[nodiscard]] constexpr api::MatrixCoordinate advanceColumnMajorLoopSeedCoordinate(
    const api::MatrixCoordinate& rCurrentCoordinate, sal_uInt64 nRowCount, sal_uInt64 nRunLength)
{
    if (!nRowCount)
        return rCurrentCoordinate;

    sal_uInt64 nColumn = static_cast<sal_uInt64>(rCurrentCoordinate.mnColumn)
                         + (nRunLength / nRowCount);
    sal_uInt64 nRow
        = static_cast<sal_uInt64>(rCurrentCoordinate.mnRow) + (nRunLength % nRowCount);
    if (nRow >= nRowCount)
    {
        nRow -= nRowCount;
        ++nColumn;
    }

    return makeCoordinate(nColumn, nRow);
}

template <typename Column, typename Row>
[[nodiscard]] constexpr bool isCoordinateValid(
    const api::MatrixDimensions& rDimensions, Column nColumn, Row nRow)
{
    return api::isValidCoordinate(rDimensions, makeCoordinate(nColumn, nRow));
}

template <typename Column, typename Row>
[[nodiscard]] constexpr bool normalizeReplicatedCoordinateInPlace(
    const api::MatrixDimensions& rDimensions, Column& rnColumn, Row& rnRow)
{
    auto aCoordinate = makeCoordinate(rnColumn, rnRow);
    if (!api::normalizeReplicatedCoordinate(rDimensions, aCoordinate))
        return false;

    rnColumn = aCoordinate.mnColumn;
    rnRow = aCoordinate.mnRow;
    return true;
}

template <typename Column, typename Row>
[[nodiscard]] constexpr bool isValidOrReplicatedCoordinate(
    const api::MatrixDimensions& rDimensions, Column& rnColumn, Row& rnRow)
{
    return isCoordinateValid(rDimensions, rnColumn, rnRow)
           || normalizeReplicatedCoordinateInPlace(rDimensions, rnColumn, rnRow);
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
