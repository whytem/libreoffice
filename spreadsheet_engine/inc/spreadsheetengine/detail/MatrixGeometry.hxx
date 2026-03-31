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
    std::uint64_t mnRowRepeats = 0;
    std::uint64_t mnColumnRepeats = 0;
    bool mbReplicated = false;
    bool mbValid = false;
};

struct ValidRunPlan
{
    std::uint64_t mnStartIndex = 0;
    std::uint64_t mnLength = 0;
    bool mbValid = false;
};

[[nodiscard]] constexpr api::MatrixDimensions makeDimensions(
    std::uint64_t nColumns, std::uint64_t nRows)
{
    return { static_cast<api::MatrixSize>(nColumns), static_cast<api::MatrixSize>(nRows) };
}

[[nodiscard]] constexpr api::MatrixCoordinate makeCoordinate(
    std::uint64_t nColumn, std::uint64_t nRow)
{
    return { static_cast<api::MatrixSize>(nColumn), static_cast<api::MatrixSize>(nRow) };
}

[[nodiscard]] constexpr MatrixRange makeRange(
    const api::MatrixCoordinate& rStart, const api::MatrixCoordinate& rEnd)
{
    return { rStart, rEnd };
}

[[nodiscard]] constexpr api::MatrixCoordinate offsetCoordinate(
    const api::MatrixCoordinate& rCoordinate, std::uint64_t nColumnOffset, std::uint64_t nRowOffset)
{
    return makeCoordinate(
        static_cast<std::uint64_t>(rCoordinate.mnColumn) + nColumnOffset,
        static_cast<std::uint64_t>(rCoordinate.mnRow) + nRowOffset);
}

[[nodiscard]] constexpr bool isValidRange(
    const api::MatrixDimensions& rDimensions, const MatrixRange& rRange)
{
    return api::isValidCoordinate(rDimensions, rRange.maStart)
           && api::isValidCoordinate(rDimensions, rRange.maEnd)
           && rRange.maStart.mnColumn <= rRange.maEnd.mnColumn
           && rRange.maStart.mnRow <= rRange.maEnd.mnRow;
}

[[nodiscard]] constexpr std::uint64_t columnCount(const MatrixRange& rRange)
{
    return static_cast<std::uint64_t>(rRange.maEnd.mnColumn - rRange.maStart.mnColumn) + 1;
}

[[nodiscard]] constexpr std::uint64_t rowCount(const MatrixRange& rRange)
{
    return static_cast<std::uint64_t>(rRange.maEnd.mnRow - rRange.maStart.mnRow) + 1;
}

[[nodiscard]] constexpr std::uint64_t columnMajorLinearIndex(
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rCoordinate)
{
    return static_cast<std::uint64_t>(rDimensions.mnRows)
           * static_cast<std::uint64_t>(rCoordinate.mnColumn)
           + static_cast<std::uint64_t>(rCoordinate.mnRow);
}

[[nodiscard]] constexpr std::uint64_t offsetColumnMajorLinearIndex(
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rCoordinate,
    std::uint64_t nColumnOffset, std::uint64_t nRowOffset)
{
    return columnMajorLinearIndex(
        rDimensions, offsetCoordinate(rCoordinate, nColumnOffset, nRowOffset));
}

[[nodiscard]] constexpr api::MatrixCoordinate coordinateFromLinearIndex(
    const api::MatrixDimensions& rDimensions, std::uint64_t nIndex)
{
    const std::uint64_t nRows = rDimensions.mnRows > 0 ? static_cast<std::uint64_t>(rDimensions.mnRows) : 0;
    const std::uint64_t nColumn = nRows > 1 ? nIndex / nRows : nIndex;
    const std::uint64_t nRow = nIndex - (nColumn * nRows);
    return makeCoordinate(nColumn, nRow);
}

[[nodiscard]] constexpr api::MatrixCoordinate coordinateFromTransposedLinearIndex(
    const api::MatrixDimensions& rDimensions, std::uint64_t nIndex)
{
    const std::uint64_t nColumns
        = rDimensions.mnColumns > 0 ? static_cast<std::uint64_t>(rDimensions.mnColumns) : 0;
    const std::uint64_t nRow = nColumns > 1 ? nIndex / nColumns : nIndex;
    const std::uint64_t nColumn = nIndex - (nRow * nColumns);
    return makeCoordinate(nColumn, nRow);
}

[[nodiscard]] constexpr MatrixRange columnVectorRange(
    const api::MatrixCoordinate& rStart, std::uint64_t nCount)
{
    return makeRange(
        rStart,
        makeCoordinate(
            static_cast<std::uint64_t>(rStart.mnColumn),
            static_cast<std::uint64_t>(rStart.mnRow) + nCount - 1));
}

[[nodiscard]] constexpr bool canPlaceColumnVector(
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rStart, std::uint64_t nCount)
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
    const api::MatrixDimensions& rDimensions, const api::MatrixCoordinate& rStart, std::uint64_t nCount)
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
                           makeCoordinate(static_cast<std::uint64_t>(rTargetDimensions.mnColumns) - 1,
                                          static_cast<std::uint64_t>(rTargetDimensions.mnRows) - 1)),
                 1, 1, false, true };
    }

    const std::uint64_t nOperationColumns = std::min(static_cast<std::uint64_t>(rSourceDimensions.mnColumns),
                                                  static_cast<std::uint64_t>(rTargetDimensions.mnColumns));
    const std::uint64_t nOperationRows = std::min(static_cast<std::uint64_t>(rSourceDimensions.mnRows),
                                               static_cast<std::uint64_t>(rTargetDimensions.mnRows));
    if (!nOperationColumns || !nOperationRows)
        return {};

    return { makeRange(makeCoordinate(0, 0),
                       makeCoordinate(nOperationColumns - 1, nOperationRows - 1)),
             rSourceDimensions.mnRows == 1 ? static_cast<std::uint64_t>(rTargetDimensions.mnRows) : 1,
             rSourceDimensions.mnColumns == 1 ? static_cast<std::uint64_t>(rTargetDimensions.mnColumns)
                                              : 1,
             true, true };
}

template <typename ValidContainer>
[[nodiscard]] auto planContiguousValidRun(const ValidContainer& rValid, std::uint64_t nStartIndex)
    -> ValidRunPlan
{
    if (nStartIndex >= rValid.size() || !rValid[nStartIndex])
        return {};

    auto aBegin = std::next(rValid.begin(), nStartIndex);
    auto aEnd = std::find(aBegin, rValid.end(), false);
    return { nStartIndex, static_cast<std::uint64_t>(std::distance(aBegin, aEnd)), true };
}

[[nodiscard]] constexpr api::MatrixCoordinate advanceColumnMajorLoopSeedCoordinate(
    const api::MatrixCoordinate& rCurrentCoordinate, std::uint64_t nRowCount, std::uint64_t nRunLength)
{
    if (!nRowCount)
        return rCurrentCoordinate;

    std::uint64_t nColumn = static_cast<std::uint64_t>(rCurrentCoordinate.mnColumn)
                         + (nRunLength / nRowCount);
    std::uint64_t nRow
        = static_cast<std::uint64_t>(rCurrentCoordinate.mnRow) + (nRunLength % nRowCount);
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
    const api::MatrixDimensions& rDimensions, std::uint64_t nElementsMax)
{
    if (!rDimensions.isAllocated())
        return false;

    const bool bZeroColumns = rDimensions.mnColumns == 0;
    const bool bZeroRows = rDimensions.mnRows == 0;
    if (bZeroColumns != bZeroRows)
        return false;
    if (bZeroColumns)
        return true;

    const std::uint64_t nColumns = static_cast<std::uint64_t>(rDimensions.mnColumns);
    const std::uint64_t nRows = static_cast<std::uint64_t>(rDimensions.mnRows);
    return nColumns <= (nElementsMax / nRows);
}

} // namespace spreadsheetengine::core::matrix

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
