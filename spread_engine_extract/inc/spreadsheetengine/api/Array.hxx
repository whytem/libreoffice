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
#include <cmath>
#include <optional>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Matrix.hxx>

namespace spreadsheetengine::api::array
{

enum class Axis : sal_uInt8
{
    Columns,
    Rows
};

enum class StackDirection : sal_uInt8
{
    Horizontal,
    Vertical
};

enum class FlattenIgnore : sal_uInt8
{
    Default = 0,
    Blanks = 1,
    Errors = 2,
    All = 3
};

struct MatrixSlice
{
    MatrixCoordinate maStart;
    MatrixDimensions maDimensions;

    [[nodiscard]] constexpr bool isEmpty() const
    {
        return maDimensions.mnColumns <= 0 || maDimensions.mnRows <= 0;
    }

    [[nodiscard]] constexpr bool operator==(const MatrixSlice& rOther) const = default;
};

[[nodiscard]] inline ValueResult<MatrixSize> normalizeSelectionIndex(
    sal_Int32 nIndex, MatrixSize nMax)
{
    if (nMax <= 0 || nIndex == 0)
        return ValueResult<MatrixSize>::failure(Error::IllegalArgument);

    sal_Int32 nResolved = nIndex;
    if (nResolved < 0)
        nResolved = nMax + nResolved + 1;

    if (nResolved <= 0 || nResolved > nMax)
        return ValueResult<MatrixSize>::failure(Error::IllegalArgument);

    return ValueResult<MatrixSize>::success(nResolved - 1);
}

[[nodiscard]] inline ValueResult<MatrixDimensions> planChooseResultDimensions(
    const MatrixDimensions& rSourceDimensions, MatrixSize nSelectionCount, Axis eAxis)
{
    if (rSourceDimensions.mnColumns < 1 || rSourceDimensions.mnRows < 1 || nSelectionCount < 1)
        return ValueResult<MatrixDimensions>::failure(Error::IllegalArgument);

    if (eAxis == Axis::Columns)
        return ValueResult<MatrixDimensions>::success(
            { nSelectionCount, rSourceDimensions.mnRows });

    return ValueResult<MatrixDimensions>::success(
        { rSourceDimensions.mnColumns, nSelectionCount });
}

[[nodiscard]] inline std::pair<MatrixSize, MatrixSize> planAxisWindow(
    MatrixSize nSize, bool bTake, const std::optional<sal_Int32>& oCount)
{
    MatrixSize nStart = 0;
    MatrixSize nEnd = nSize;
    if (oCount && nSize > 0 && static_cast<sal_uInt64>(std::abs(*oCount)) < static_cast<sal_uInt64>(nSize))
    {
        if (bTake)
        {
            if (*oCount < 0)
                nStart = nSize + *oCount;
            else
                nEnd = *oCount;
        }
        else
        {
            if (*oCount < 0)
                nEnd = nSize + *oCount;
            else
                nStart = *oCount;
        }
    }

    return { nStart, nEnd };
}

[[nodiscard]] inline ValueResult<MatrixSlice> planTakeDropSlice(
    const MatrixDimensions& rSourceDimensions, bool bTake, const std::optional<sal_Int32>& oRows,
    const std::optional<sal_Int32>& oColumns)
{
    if (rSourceDimensions.mnColumns < 1 || rSourceDimensions.mnRows < 1)
        return ValueResult<MatrixSlice>::failure(Error::IllegalArgument);

    const auto [nStartColumn, nEndColumn]
        = planAxisWindow(rSourceDimensions.mnColumns, bTake, oColumns);
    const auto [nStartRow, nEndRow] = planAxisWindow(rSourceDimensions.mnRows, bTake, oRows);

    if (nStartColumn >= nEndColumn || nStartRow >= nEndRow)
        return ValueResult<MatrixSlice>::failure(Error::NotAvailable);

    return ValueResult<MatrixSlice>::success(
        { { nStartColumn, nStartRow }, { nEndColumn - nStartColumn, nEndRow - nStartRow } });
}

[[nodiscard]] inline ValueResult<MatrixDimensions> planExpandDimensions(
    const MatrixDimensions& rSourceDimensions, const std::optional<sal_Int32>& oRows,
    const std::optional<sal_Int32>& oColumns)
{
    if (rSourceDimensions.mnColumns < 1 || rSourceDimensions.mnRows < 1)
        return ValueResult<MatrixDimensions>::failure(Error::IllegalArgument);

    MatrixDimensions aResult = rSourceDimensions;
    if (oColumns)
    {
        if (*oColumns < rSourceDimensions.mnColumns)
            return ValueResult<MatrixDimensions>::failure(Error::IllegalArgument);
        aResult.mnColumns = *oColumns;
    }
    if (oRows)
    {
        if (*oRows < rSourceDimensions.mnRows)
            return ValueResult<MatrixDimensions>::failure(Error::IllegalArgument);
        aResult.mnRows = *oRows;
    }

    return ValueResult<MatrixDimensions>::success(aResult);
}

[[nodiscard]] constexpr MatrixDimensions appendStackDimensions(
    const MatrixDimensions& rCurrentDimensions, const MatrixDimensions& rNextDimensions,
    StackDirection eDirection)
{
    if (eDirection == StackDirection::Horizontal)
    {
        return { rCurrentDimensions.mnColumns + rNextDimensions.mnColumns,
            std::max(rCurrentDimensions.mnRows, rNextDimensions.mnRows) };
    }

    return { std::max(rCurrentDimensions.mnColumns, rNextDimensions.mnColumns),
        rCurrentDimensions.mnRows + rNextDimensions.mnRows };
}

[[nodiscard]] constexpr MatrixCoordinate stackDestination(
    StackDirection eDirection, const MatrixCoordinate& rSourceCoordinate, MatrixSize nOffset)
{
    if (eDirection == StackDirection::Horizontal)
        return { rSourceCoordinate.mnColumn + nOffset, rSourceCoordinate.mnRow };

    return { rSourceCoordinate.mnColumn, rSourceCoordinate.mnRow + nOffset };
}

[[nodiscard]] constexpr bool shouldIncludeFlattenedValue(
    FlattenIgnore eIgnore, bool bEmptyCell, bool bErrorValue)
{
    if ((eIgnore == FlattenIgnore::All || eIgnore == FlattenIgnore::Blanks) && bEmptyCell)
        return false;
    if ((eIgnore == FlattenIgnore::All || eIgnore == FlattenIgnore::Errors) && bErrorValue)
        return false;
    return true;
}

[[nodiscard]] inline ValueResult<MatrixDimensions> planFlattenOutputDimensions(
    MatrixSize nValueCount, bool bToColumn)
{
    if (nValueCount <= 0)
        return ValueResult<MatrixDimensions>::failure(Error::NotAvailable);

    return ValueResult<MatrixDimensions>::success(
        bToColumn ? MatrixDimensions { 1, nValueCount } : MatrixDimensions { nValueCount, 1 });
}

[[nodiscard]] constexpr MatrixCoordinate flattenDestination(MatrixSize nLinearIndex, bool bToColumn)
{
    return bToColumn ? MatrixCoordinate { 0, nLinearIndex } : MatrixCoordinate { nLinearIndex, 0 };
}

[[nodiscard]] inline ValueResult<MatrixSize> vectorElementCount(
    const MatrixDimensions& rSourceDimensions)
{
    if (!isColumnVector(rSourceDimensions) && !isRowVector(rSourceDimensions))
        return ValueResult<MatrixSize>::failure(Error::IllegalArgument);

    return ValueResult<MatrixSize>::success(rSourceDimensions.mnColumns * rSourceDimensions.mnRows);
}

[[nodiscard]] inline ValueResult<MatrixDimensions> planWrapOutputDimensions(
    const MatrixDimensions& rSourceDimensions, MatrixSize nWrapCount, bool bWrapColumns)
{
    if (nWrapCount <= 0)
        return ValueResult<MatrixDimensions>::failure(Error::IllegalArgument);

    const auto aElementCount = vectorElementCount(rSourceDimensions);
    if (!aElementCount)
        return ValueResult<MatrixDimensions>::failure(aElementCount.meError);

    const MatrixSize nBands = static_cast<MatrixSize>(
        std::ceil(aElementCount.maValue / static_cast<double>(nWrapCount)));
    return ValueResult<MatrixDimensions>::success(
        bWrapColumns ? MatrixDimensions { nBands, nWrapCount }
                     : MatrixDimensions { nWrapCount, nBands });
}

[[nodiscard]] constexpr MatrixCoordinate wrapDestination(
    MatrixSize nLinearIndex, MatrixSize nWrapCount, bool bWrapColumns)
{
    if (bWrapColumns)
        return { nLinearIndex / nWrapCount, nLinearIndex % nWrapCount };

    return { nLinearIndex % nWrapCount, nLinearIndex / nWrapCount };
}

} // namespace spreadsheetengine::api::array

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
