/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>
#include <optional>

#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::api::reference
{

enum class ReferenceAxis : std::uint8_t
{
    Column,
    Row
};

struct AxisReferencePlan
{
    double mfStart = 1.0;
    MatrixSize mnLength = 1;
    ReferenceAxis meAxis = ReferenceAxis::Column;

    [[nodiscard]] constexpr bool operator==(const AxisReferencePlan& rOther) const = default;

    [[nodiscard]] constexpr bool requiresMatrixResult() const
    {
        return mnLength > 1;
    }

    [[nodiscard]] constexpr MatrixDimensions resultDimensions() const
    {
        return meAxis == ReferenceAxis::Column ? MatrixDimensions { mnLength, 1 }
                                               : MatrixDimensions { 1, mnLength };
    }
};

enum class IndexSelectionKind : std::uint8_t
{
    KeepSource,
    Scalar,
    RowSlice,
    ColumnSlice
};

struct IndexMatrixSelection
{
    IndexSelectionKind meKind = IndexSelectionKind::KeepSource;
    MatrixCoordinate maStart;
    MatrixDimensions maDimensions { 1, 1 };

    [[nodiscard]] constexpr bool operator==(const IndexMatrixSelection& rOther) const = default;
};

struct IndexReferenceSelection
{
    IndexSelectionKind meKind = IndexSelectionKind::KeepSource;
    CellRange maRange;

    [[nodiscard]] constexpr bool operator==(const IndexReferenceSelection& rOther) const = default;
};

enum class ReferenceListMaterializationKind : std::uint8_t
{
    KeepList,
    SingleReference,
    ColumnVector
};

struct ReferenceListMaterializationPlan
{
    ReferenceListMaterializationKind meKind
        = ReferenceListMaterializationKind::KeepList;
    MatrixDimensions maMatrixDimensions { 0, 0 };

    [[nodiscard]] constexpr bool operator==(
        const ReferenceListMaterializationPlan& rOther) const = default;
};

[[nodiscard]] inline bool hasOrderedBounds(const CellRange& rRange)
{
    return rRange.maStart.mnSheet <= rRange.maEnd.mnSheet
           && rRange.maStart.mnColumn <= rRange.maEnd.mnColumn
           && rRange.maStart.mnRow <= rRange.maEnd.mnRow;
}

[[nodiscard]] inline ValueResult<AxisReferencePlan> planAxisReference(
    const CellRange& rRange, ReferenceAxis eAxis)
{
    if (!hasOrderedBounds(rRange))
        return ValueResult<AxisReferencePlan>::failure(Error::IllegalArgument);

    AxisReferencePlan aPlan;
    aPlan.meAxis = eAxis;
    if (eAxis == ReferenceAxis::Column)
    {
        aPlan.mfStart = static_cast<double>(rRange.maStart.mnColumn + 1);
        aPlan.mnLength = rRange.maEnd.mnColumn - rRange.maStart.mnColumn + 1;
    }
    else
    {
        aPlan.mfStart = static_cast<double>(rRange.maStart.mnRow + 1);
        aPlan.mnLength = rRange.maEnd.mnRow - rRange.maStart.mnRow + 1;
    }

    return ValueResult<AxisReferencePlan>::success(aPlan);
}

[[nodiscard]] inline ValueResult<double> countReferenceAxisSpan(
    const CellRange& rRange, ReferenceAxis eAxis, bool bMultiplyAcrossSheets)
{
    if (!hasOrderedBounds(rRange))
        return ValueResult<double>::failure(Error::IllegalArgument);

    const std::int64_t nAxisSpan = eAxis == ReferenceAxis::Column
                                       ? static_cast<std::int64_t>(rRange.maEnd.mnColumn)
                                             - rRange.maStart.mnColumn + 1
                                       : static_cast<std::int64_t>(rRange.maEnd.mnRow)
                                             - rRange.maStart.mnRow + 1;
    const std::int64_t nSheetSpan = bMultiplyAcrossSheets
                                        ? static_cast<std::int64_t>(rRange.maEnd.mnSheet)
                                              - rRange.maStart.mnSheet + 1
                                        : 1;
    if (nAxisSpan <= 0 || nSheetSpan <= 0)
        return ValueResult<double>::failure(Error::IllegalArgument);

    return ValueResult<double>::success(static_cast<double>(nAxisSpan * nSheetSpan));
}

[[nodiscard]] inline ValueResult<double> countMatrixAxisSpan(
    const MatrixDimensions& rDimensions, ReferenceAxis eAxis)
{
    const MatrixSize nAxisSpan
        = eAxis == ReferenceAxis::Column ? rDimensions.mnColumns : rDimensions.mnRows;
    if (rDimensions.mnColumns <= 0 || rDimensions.mnRows <= 0 || nAxisSpan <= 0)
        return ValueResult<double>::failure(Error::IllegalArgument);

    return ValueResult<double>::success(static_cast<double>(nAxisSpan));
}

[[nodiscard]] inline ValueResult<double> countAreas(std::size_t nAreaCount)
{
    if (nAreaCount == 0)
        return ValueResult<double>::failure(Error::IllegalArgument);

    return ValueResult<double>::success(static_cast<double>(nAreaCount));
}

[[nodiscard]] inline ValueResult<double> sheetOrdinalFromSheetId(SheetId nSheet)
{
    if (nSheet < 0)
        return ValueResult<double>::failure(Error::IllegalArgument);

    return ValueResult<double>::success(static_cast<double>(nSheet + 1));
}

[[nodiscard]] inline ValueResult<double> sheetCountFromWorkbookSize(std::size_t nSheetCount)
{
    return ValueResult<double>::success(static_cast<double>(nSheetCount));
}

[[nodiscard]] inline ValueResult<double> sheetOrdinalFromReference(const CellRange& rRange)
{
    if (!hasOrderedBounds(rRange))
        return ValueResult<double>::failure(Error::IllegalArgument);

    return sheetOrdinalFromSheetId(rRange.maStart.mnSheet);
}

[[nodiscard]] inline ValueResult<double> sheetCountFromReference(const CellRange& rRange)
{
    if (!hasOrderedBounds(rRange))
        return ValueResult<double>::failure(Error::IllegalArgument);

    return ValueResult<double>::success(static_cast<double>(
        static_cast<std::int64_t>(rRange.maEnd.mnSheet) - rRange.maStart.mnSheet + 1));
}

[[nodiscard]] inline ValueResult<std::size_t> normalizeAreaSelection(
    sal_Int32 nArea, std::size_t nAreaCount)
{
    if (nArea < 1 || nAreaCount == 0 || static_cast<std::size_t>(nArea) > nAreaCount)
        return ValueResult<std::size_t>::failure(Error::NotAvailable);

    return ValueResult<std::size_t>::success(static_cast<std::size_t>(nArea - 1));
}

[[nodiscard]] inline ValueResult<CellAddress> selectScalarReferenceCell(
    const CellRange& rRange, const CellAddress& rFormulaPos,
    const std::optional<MatrixCoordinate>& oJumpCoordinate = std::nullopt)
{
    if (!hasOrderedBounds(rRange))
        return ValueResult<CellAddress>::failure(Error::IllegalArgument);

    if (rRange.maStart == rRange.maEnd)
        return ValueResult<CellAddress>::success(rRange.maStart);

    if (oJumpCoordinate)
    {
        if (rRange.maStart.mnSheet != rRange.maEnd.mnSheet || oJumpCoordinate->mnColumn < 0
            || oJumpCoordinate->mnRow < 0)
        {
            return ValueResult<CellAddress>::failure(Error::IllegalArgument);
        }

        const ColumnIndex nColumn = rRange.maStart.mnColumn + oJumpCoordinate->mnColumn;
        const RowIndex nRow = rRange.maStart.mnRow + oJumpCoordinate->mnRow;
        if (nColumn < rRange.maStart.mnColumn || nColumn > rRange.maEnd.mnColumn
            || nRow < rRange.maStart.mnRow || nRow > rRange.maEnd.mnRow)
        {
            return ValueResult<CellAddress>::failure(Error::NoValue);
        }

        return ValueResult<CellAddress>::success(
            { rRange.maStart.mnSheet, nColumn, nRow });
    }

    bool bOk = false;
    ColumnIndex nColumn = 0;
    RowIndex nRow = 0;
    SheetId nSheet = rRange.maStart.mnSheet;

    if (rRange.maStart.mnColumn <= rFormulaPos.mnColumn
        && rFormulaPos.mnColumn <= rRange.maEnd.mnColumn)
    {
        nRow = rRange.maStart.mnRow;
        if (nRow == rRange.maEnd.mnRow)
        {
            bOk = true;
            nColumn = rFormulaPos.mnColumn;
        }
        else if (nSheet != rFormulaPos.mnSheet && nSheet == rRange.maEnd.mnSheet
                 && rRange.maStart.mnRow <= rFormulaPos.mnRow
                 && rFormulaPos.mnRow <= rRange.maEnd.mnRow)
        {
            bOk = true;
            nColumn = rFormulaPos.mnColumn;
            nRow = rFormulaPos.mnRow;
        }
    }
    else if (rRange.maStart.mnRow <= rFormulaPos.mnRow
             && rFormulaPos.mnRow <= rRange.maEnd.mnRow)
    {
        nColumn = rRange.maStart.mnColumn;
        if (nColumn == rRange.maEnd.mnColumn)
        {
            bOk = true;
            nRow = rFormulaPos.mnRow;
        }
        else if (nSheet != rFormulaPos.mnSheet && nSheet == rRange.maEnd.mnSheet
                 && rRange.maStart.mnColumn <= rFormulaPos.mnColumn
                 && rFormulaPos.mnColumn <= rRange.maEnd.mnColumn)
        {
            bOk = true;
            nColumn = rFormulaPos.mnColumn;
            nRow = rFormulaPos.mnRow;
        }
    }

    if (!bOk)
        return ValueResult<CellAddress>::failure(Error::NoValue);

    if (nSheet != rRange.maEnd.mnSheet)
    {
        if (nSheet <= rFormulaPos.mnSheet && rFormulaPos.mnSheet <= rRange.maEnd.mnSheet)
            nSheet = rFormulaPos.mnSheet;
        else
            return ValueResult<CellAddress>::failure(Error::NoValue);
    }

    return ValueResult<CellAddress>::success({ nSheet, nColumn, nRow });
}

[[nodiscard]] inline ValueResult<ReferenceListMaterializationPlan>
planReferenceListMaterialization(
    std::size_t nEntryCount, bool bMatrixFormula, bool bAllSingleCellReferences)
{
    if (nEntryCount == 0)
    {
        return ValueResult<ReferenceListMaterializationPlan>::failure(Error::IllegalArgument);
    }

    if (nEntryCount == 1)
    {
        return ValueResult<ReferenceListMaterializationPlan>::success(
            { ReferenceListMaterializationKind::SingleReference, { 1, 1 } });
    }

    if (!bMatrixFormula || !bAllSingleCellReferences)
    {
        return ValueResult<ReferenceListMaterializationPlan>::success(
            { ReferenceListMaterializationKind::KeepList, { 0, 0 } });
    }

    return ValueResult<ReferenceListMaterializationPlan>::success(
        { ReferenceListMaterializationKind::ColumnVector,
            { 1, static_cast<MatrixSize>(nEntryCount) } });
}

[[nodiscard]] inline ValueResult<CellRange> planOffsetRange(const CellRange& rBaseRange,
    RowIndex nRowOffset, ColumnIndex nColumnOffset, const std::optional<RowIndex>& oNewHeight,
    const std::optional<ColumnIndex>& oNewWidth, ColumnIndex nMaxColumn, RowIndex nMaxRow)
{
    if (!rBaseRange.isNormalized() || rBaseRange.maStart.mnSheet != rBaseRange.maEnd.mnSheet
        || nMaxColumn < 0 || nMaxRow < 0)
    {
        return ValueResult<CellRange>::failure(Error::IllegalArgument);
    }

    const ColumnIndex nWidth = oNewWidth.value_or(rBaseRange.columnCount());
    const RowIndex nHeight = oNewHeight.value_or(rBaseRange.rowCount());
    if (nWidth <= 0 || nHeight <= 0)
        return ValueResult<CellRange>::failure(Error::IllegalArgument);

    const std::int64_t nStartColumn
        = static_cast<std::int64_t>(rBaseRange.maStart.mnColumn) + nColumnOffset;
    const std::int64_t nStartRow = static_cast<std::int64_t>(rBaseRange.maStart.mnRow) + nRowOffset;
    const std::int64_t nEndColumn = nStartColumn + nWidth - 1;
    const std::int64_t nEndRow = nStartRow + nHeight - 1;
    if (nStartColumn < 0 || nStartRow < 0 || nEndColumn > nMaxColumn || nEndRow > nMaxRow)
        return ValueResult<CellRange>::failure(Error::IllegalArgument);

    return ValueResult<CellRange>::success({ { rBaseRange.maStart.mnSheet,
                                                  static_cast<ColumnIndex>(nStartColumn),
                                                  static_cast<RowIndex>(nStartRow) },
        { rBaseRange.maStart.mnSheet, static_cast<ColumnIndex>(nEndColumn),
            static_cast<RowIndex>(nEndRow) } });
}

[[nodiscard]] inline ValueResult<IndexMatrixSelection> planIndexMatrixSelection(
    const MatrixDimensions& rSourceDimensions, RowIndex nRow, ColumnIndex nColumn,
    bool bColumnMissing, std::uint8_t nParamCount)
{
    if (nRow < 0 || nColumn < 0 || rSourceDimensions.mnColumns <= 0 || rSourceDimensions.mnRows <= 0)
        return ValueResult<IndexMatrixSelection>::failure(Error::IllegalArgument);

    const bool bRowVectorSpecial = (nParamCount == 2 || bColumnMissing);
    const bool bRowVectorElement = (rSourceDimensions.mnRows == 1
                                    && (nColumn != 0 || (bRowVectorSpecial && nRow != 0)));
    const bool bVectorElement
        = (bRowVectorElement || (rSourceDimensions.mnColumns == 1 && nRow != 0));

    if (!bVectorElement && (nColumn > rSourceDimensions.mnColumns || nRow > rSourceDimensions.mnRows))
        return ValueResult<IndexMatrixSelection>::failure(Error::NotAvailable);

    if (nColumn == 0 && nRow == 0)
    {
        return ValueResult<IndexMatrixSelection>::success(
            { IndexSelectionKind::KeepSource, {}, rSourceDimensions });
    }

    if (bVectorElement)
    {
        MatrixSize nElement = 0;
        MatrixSize nOtherDimension = 0;
        if (bRowVectorElement && !bRowVectorSpecial)
        {
            nElement = nColumn;
            nOtherDimension = nRow;
        }
        else
        {
            nElement = nRow;
            nOtherDimension = nColumn;
        }

        if (nElement <= 0 || nElement > rSourceDimensions.mnColumns * rSourceDimensions.mnRows
            || nOtherDimension > 1)
        {
            return ValueResult<IndexMatrixSelection>::failure(Error::NotAvailable);
        }

        const MatrixCoordinate aCoordinate
            = bRowVectorElement ? MatrixCoordinate { nElement - 1, 0 }
                                : MatrixCoordinate { 0, nElement - 1 };
        return ValueResult<IndexMatrixSelection>::success(
            { IndexSelectionKind::Scalar, aCoordinate, { 1, 1 } });
    }

    if (nColumn == 0)
    {
        return ValueResult<IndexMatrixSelection>::success({ IndexSelectionKind::RowSlice,
            { 0, nRow - 1 }, { rSourceDimensions.mnColumns, 1 } });
    }

    if (nRow == 0)
    {
        return ValueResult<IndexMatrixSelection>::success({ IndexSelectionKind::ColumnSlice,
            { nColumn - 1, 0 }, { 1, rSourceDimensions.mnRows } });
    }

    return ValueResult<IndexMatrixSelection>::success(
        { IndexSelectionKind::Scalar, { nColumn - 1, nRow - 1 }, { 1, 1 } });
}

[[nodiscard]] inline ValueResult<IndexReferenceSelection> planIndexReferenceSelection(
    const CellRange& rSourceRange, RowIndex nRow, ColumnIndex nColumn, std::uint8_t nParamCount)
{
    if (nRow < 0 || nColumn < 0 || !rSourceRange.isNormalized()
        || rSourceRange.maStart.mnSheet != rSourceRange.maEnd.mnSheet)
    {
        return ValueResult<IndexReferenceSelection>::failure(Error::IllegalArgument);
    }

    const bool bRowArray
        = (nParamCount == 2 && rSourceRange.maStart.mnRow == rSourceRange.maEnd.mnRow);
    if ((nColumn > 0 && rSourceRange.maStart.mnColumn + nColumn - 1 > rSourceRange.maEnd.mnColumn)
        || (nRow > 0 && !bRowArray
            && rSourceRange.maStart.mnRow + nRow - 1 > rSourceRange.maEnd.mnRow)
        || (bRowArray && nRow > rSourceRange.columnCount()))
    {
        return ValueResult<IndexReferenceSelection>::failure(Error::NotAvailable);
    }

    if (nColumn == 0 && nRow == 0)
    {
        return ValueResult<IndexReferenceSelection>::success(
            { IndexSelectionKind::KeepSource, rSourceRange });
    }

    if (nRow == 0)
    {
        const ColumnIndex nResolvedColumn = rSourceRange.maStart.mnColumn + nColumn - 1;
        const CellRange aRange { { rSourceRange.maStart.mnSheet, nResolvedColumn,
                                      rSourceRange.maStart.mnRow },
            { rSourceRange.maStart.mnSheet, nResolvedColumn, rSourceRange.maEnd.mnRow } };
        return ValueResult<IndexReferenceSelection>::success(
            { aRange.isSingleCell() ? IndexSelectionKind::Scalar : IndexSelectionKind::ColumnSlice,
                aRange });
    }

    if (nColumn == 0)
    {
        if (rSourceRange.maStart.mnColumn == rSourceRange.maEnd.mnColumn)
        {
            const CellRange aRange { { rSourceRange.maStart.mnSheet, rSourceRange.maStart.mnColumn,
                                          rSourceRange.maStart.mnRow + nRow - 1 },
                { rSourceRange.maStart.mnSheet, rSourceRange.maStart.mnColumn,
                    rSourceRange.maStart.mnRow + nRow - 1 } };
            return ValueResult<IndexReferenceSelection>::success(
                { IndexSelectionKind::Scalar, aRange });
        }

        if (bRowArray)
        {
            const CellRange aRange { { rSourceRange.maStart.mnSheet,
                                          rSourceRange.maStart.mnColumn + nRow - 1,
                                          rSourceRange.maStart.mnRow },
                { rSourceRange.maStart.mnSheet, rSourceRange.maStart.mnColumn + nRow - 1,
                    rSourceRange.maStart.mnRow } };
            return ValueResult<IndexReferenceSelection>::success(
                { IndexSelectionKind::Scalar, aRange });
        }

        const RowIndex nResolvedRow = rSourceRange.maStart.mnRow + nRow - 1;
        return ValueResult<IndexReferenceSelection>::success({ IndexSelectionKind::RowSlice,
            { { rSourceRange.maStart.mnSheet, rSourceRange.maStart.mnColumn, nResolvedRow },
                { rSourceRange.maStart.mnSheet, rSourceRange.maEnd.mnColumn, nResolvedRow } } });
    }

    const CellRange aRange { { rSourceRange.maStart.mnSheet,
                                  rSourceRange.maStart.mnColumn + nColumn - 1,
                                  rSourceRange.maStart.mnRow + nRow - 1 },
        { rSourceRange.maStart.mnSheet, rSourceRange.maStart.mnColumn + nColumn - 1,
            rSourceRange.maStart.mnRow + nRow - 1 } };
    return ValueResult<IndexReferenceSelection>::success(
        { IndexSelectionKind::Scalar, aRange });
}

} // namespace spreadsheetengine::api::reference

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
