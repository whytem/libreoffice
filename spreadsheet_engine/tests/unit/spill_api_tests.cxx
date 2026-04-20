/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnSpill.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::api::CellValue;
using spreadsheetengine::api::Error;
using spreadsheetengine::api::MatrixDimensions;
using spreadsheetengine::api::MatrixSize;
using spreadsheetengine::api::String;
using spreadsheetengine::api::array::Axis;
using spreadsheetengine::api::array::FlattenIgnore;
using spreadsheetengine::api::array::StackDirection;
using spreadsheetengine::core::rpn::MatrixOperand;
using spreadsheetengine::core::rpn::MatrixProvenance;
using spreadsheetengine::core::rpn::spill::planChooseColsOrRows;
using spreadsheetengine::core::rpn::spill::planDrop;
using spreadsheetengine::core::rpn::spill::planExpand;
using spreadsheetengine::core::rpn::spill::planFilter;
using spreadsheetengine::core::rpn::spill::planHStackOrVStack;
using spreadsheetengine::core::rpn::spill::planSort;
using spreadsheetengine::core::rpn::spill::planTake;
using spreadsheetengine::core::rpn::spill::planTextSplit;
using spreadsheetengine::core::rpn::spill::planToColOrRow;
using spreadsheetengine::core::rpn::spill::planUnique;
using spreadsheetengine::core::rpn::spill::planWrapColsOrRows;
using spreadsheetengine::core::rpn::spill::SpillError;
using spreadsheetengine::standalone::test::almostEqual;
using spreadsheetengine::standalone::test::fail;

MatrixOperand makeNumericMatrix(MatrixSize nColumns, MatrixSize nRows,
                                const std::vector<double>& rValues)
{
    MatrixOperand aOperand;
    aOperand.maDimensions = { nColumns, nRows };
    aOperand.maValues.reserve(rValues.size());
    for (double fValue : rValues)
        aOperand.maValues.push_back(CellValue::number(fValue));
    aOperand.meProvenance = MatrixProvenance::InlineLiteral;
    return aOperand;
}

bool expectNumeric(const MatrixOperand& rMatrix, MatrixSize nColumn, MatrixSize nRow,
                   double fExpected)
{
    const std::size_t nLinear = static_cast<std::size_t>(nRow) * rMatrix.maDimensions.mnColumns
                                + nColumn;
    if (nLinear >= rMatrix.maValues.size())
        return false;
    const auto& rCell = rMatrix.maValues[nLinear];
    if (rCell.meKind != spreadsheetengine::api::CellValueKind::Number
        && rCell.meKind != spreadsheetengine::api::CellValueKind::Boolean)
        return false;
    return almostEqual(rCell.mfNumber, fExpected);
}

} // namespace

int main()
{
    // SORT: 3 rows of (key, value) pairs sorted ascending by column 0.
    const auto aSource
        = makeNumericMatrix(2, 3, { 30.0, 'c', 10.0, 'a', 20.0, 'b' });
    const auto aSorted = planSort(aSource, { 0 }, { true }, /*bByRow*/ true);
    if (!aSorted || aSorted.maValue.maDimensions.mnColumns != 2
        || aSorted.maValue.maDimensions.mnRows != 3
        || !expectNumeric(aSorted.maValue, 0, 0, 10.0)
        || !expectNumeric(aSorted.maValue, 0, 1, 20.0)
        || !expectNumeric(aSorted.maValue, 0, 2, 30.0))
    {
        return fail("spreadsheetengine_spill_tests", "SORT ascending-by-column mismatch");
    }

    // SORT: same data sorted descending.
    const auto aSortedDesc = planSort(aSource, { 0 }, { false }, /*bByRow*/ true);
    if (!aSortedDesc || !expectNumeric(aSortedDesc.maValue, 0, 0, 30.0)
        || !expectNumeric(aSortedDesc.maValue, 0, 2, 10.0))
    {
        return fail("spreadsheetengine_spill_tests", "SORT descending mismatch");
    }

    // FILTER: keep rows where mask is non-zero.
    const auto aMask = makeNumericMatrix(1, 3, { 1.0, 0.0, 1.0 });
    const auto aFiltered = planFilter(aSource, aMask);
    if (std::holds_alternative<SpillError>(aFiltered))
        return fail("spreadsheetengine_spill_tests", "FILTER unexpected spill error");
    const auto& rFilterResult = std::get<MatrixOperand>(aFiltered);
    if (rFilterResult.maDimensions.mnColumns != 2 || rFilterResult.maDimensions.mnRows != 2
        || !expectNumeric(rFilterResult, 0, 0, 30.0)
        || !expectNumeric(rFilterResult, 0, 1, 20.0))
    {
        return fail("spreadsheetengine_spill_tests", "FILTER mask mismatch");
    }

    // FILTER: all-zero mask returns OutOfBounds.
    const auto aZeroMask = makeNumericMatrix(1, 3, { 0.0, 0.0, 0.0 });
    const auto aFilteredEmpty = planFilter(aSource, aZeroMask);
    if (!std::holds_alternative<SpillError>(aFilteredEmpty)
        || std::get<SpillError>(aFilteredEmpty) != SpillError::OutOfBounds)
    {
        return fail("spreadsheetengine_spill_tests", "FILTER all-zero mask should report empty");
    }

    // UNIQUE: rows with duplicates collapse to first occurrence.
    const auto aDup
        = makeNumericMatrix(2, 4, { 1.0, 10.0, 2.0, 20.0, 1.0, 10.0, 3.0, 30.0 });
    const auto aUnique = planUnique(aDup, /*bByColumns*/ false, /*bExactlyOnce*/ false);
    if (!aUnique || aUnique.maValue.maDimensions.mnRows != 3
        || !expectNumeric(aUnique.maValue, 0, 0, 1.0)
        || !expectNumeric(aUnique.maValue, 0, 1, 2.0)
        || !expectNumeric(aUnique.maValue, 0, 2, 3.0))
    {
        return fail("spreadsheetengine_spill_tests", "UNIQUE deduplication mismatch");
    }

    // UNIQUE exactly-once: only rows that appear exactly once survive.
    const auto aOnce = planUnique(aDup, /*bByColumns*/ false, /*bExactlyOnce*/ true);
    if (!aOnce || aOnce.maValue.maDimensions.mnRows != 2
        || !expectNumeric(aOnce.maValue, 0, 0, 2.0)
        || !expectNumeric(aOnce.maValue, 0, 1, 3.0))
    {
        return fail("spreadsheetengine_spill_tests", "UNIQUE exactly-once mismatch");
    }

    // TAKE: keep leading 2 rows.
    const auto aTake = planTake(aSource, std::optional<sal_Int32>(2), std::nullopt);
    if (!aTake || aTake.maValue.maDimensions.mnRows != 2
        || !expectNumeric(aTake.maValue, 0, 0, 30.0)
        || !expectNumeric(aTake.maValue, 0, 1, 10.0))
    {
        return fail("spreadsheetengine_spill_tests", "TAKE leading mismatch");
    }

    // TAKE negative: keep trailing 1 row.
    const auto aTakeNeg = planTake(aSource, std::optional<sal_Int32>(-1), std::nullopt);
    if (!aTakeNeg || aTakeNeg.maValue.maDimensions.mnRows != 1
        || !expectNumeric(aTakeNeg.maValue, 0, 0, 20.0))
    {
        return fail("spreadsheetengine_spill_tests", "TAKE trailing mismatch");
    }

    // DROP: drop leading 1 row.
    const auto aDrop = planDrop(aSource, std::optional<sal_Int32>(1), std::nullopt);
    if (!aDrop || aDrop.maValue.maDimensions.mnRows != 2
        || !expectNumeric(aDrop.maValue, 0, 0, 10.0)
        || !expectNumeric(aDrop.maValue, 0, 1, 20.0))
    {
        return fail("spreadsheetengine_spill_tests", "DROP leading mismatch");
    }

    // TAKE zero rows: planTakeDropSlice should surface NotAvailable.
    const auto aTakeZero = planTake(aSource, std::optional<sal_Int32>(0), std::nullopt);
    if (aTakeZero || aTakeZero.meError != Error::NotAvailable)
        return fail("spreadsheetengine_spill_tests", "TAKE zero should report NotAvailable");

    // HSTACK: append columns of two matrices side by side.
    const auto aLeft = makeNumericMatrix(1, 2, { 1.0, 2.0 });
    const auto aRight = makeNumericMatrix(1, 2, { 3.0, 4.0 });
    const auto aHStack = planHStackOrVStack({ aLeft, aRight }, StackDirection::Horizontal);
    if (!aHStack || aHStack.maValue.maDimensions.mnColumns != 2
        || aHStack.maValue.maDimensions.mnRows != 2
        || !expectNumeric(aHStack.maValue, 0, 0, 1.0)
        || !expectNumeric(aHStack.maValue, 1, 0, 3.0)
        || !expectNumeric(aHStack.maValue, 0, 1, 2.0)
        || !expectNumeric(aHStack.maValue, 1, 1, 4.0))
    {
        return fail("spreadsheetengine_spill_tests", "HSTACK column append mismatch");
    }

    // VSTACK: append rows of two matrices top to bottom.
    const auto aTop = makeNumericMatrix(2, 1, { 1.0, 2.0 });
    const auto aBottom = makeNumericMatrix(2, 1, { 3.0, 4.0 });
    const auto aVStack = planHStackOrVStack({ aTop, aBottom }, StackDirection::Vertical);
    if (!aVStack || aVStack.maValue.maDimensions.mnColumns != 2
        || aVStack.maValue.maDimensions.mnRows != 2
        || !expectNumeric(aVStack.maValue, 0, 0, 1.0)
        || !expectNumeric(aVStack.maValue, 1, 0, 2.0)
        || !expectNumeric(aVStack.maValue, 0, 1, 3.0)
        || !expectNumeric(aVStack.maValue, 1, 1, 4.0))
    {
        return fail("spreadsheetengine_spill_tests", "VSTACK row append mismatch");
    }

    // CHOOSECOLS: pick columns 1 and 3 (1-based) from a 3x2 matrix.
    const auto aChooseSrc = makeNumericMatrix(3, 2, { 10.0, 20.0, 30.0, 40.0, 50.0, 60.0 });
    const auto aChooseCols
        = planChooseColsOrRows(aChooseSrc, { 1, 3 }, Axis::Columns);
    if (!aChooseCols || aChooseCols.maValue.maDimensions.mnColumns != 2
        || aChooseCols.maValue.maDimensions.mnRows != 2
        || !expectNumeric(aChooseCols.maValue, 0, 0, 10.0)
        || !expectNumeric(aChooseCols.maValue, 1, 0, 30.0)
        || !expectNumeric(aChooseCols.maValue, 0, 1, 40.0)
        || !expectNumeric(aChooseCols.maValue, 1, 1, 60.0))
    {
        return fail("spreadsheetengine_spill_tests", "CHOOSECOLS 1-based mismatch");
    }

    // CHOOSEROWS: negative index wraps from the end.
    const auto aChooseRows
        = planChooseColsOrRows(aChooseSrc, { -1 }, Axis::Rows);
    if (!aChooseRows || aChooseRows.maValue.maDimensions.mnRows != 1
        || !expectNumeric(aChooseRows.maValue, 0, 0, 40.0)
        || !expectNumeric(aChooseRows.maValue, 1, 0, 50.0)
        || !expectNumeric(aChooseRows.maValue, 2, 0, 60.0))
    {
        return fail("spreadsheetengine_spill_tests", "CHOOSEROWS negative index mismatch");
    }

    // EXPAND: enlarge a 2x2 to 3x3 and pad with 0.
    const auto aExpandSrc = makeNumericMatrix(2, 2, { 1.0, 2.0, 3.0, 4.0 });
    const auto aExpand = planExpand(
        aExpandSrc, std::optional<sal_Int32>(3), std::optional<sal_Int32>(3),
        std::optional<CellValue>(CellValue::number(0.0)));
    if (!aExpand || aExpand.maValue.maDimensions.mnColumns != 3
        || aExpand.maValue.maDimensions.mnRows != 3
        || !expectNumeric(aExpand.maValue, 0, 0, 1.0)
        || !expectNumeric(aExpand.maValue, 2, 2, 0.0)
        || !expectNumeric(aExpand.maValue, 2, 0, 0.0))
    {
        return fail("spreadsheetengine_spill_tests", "EXPAND 2x2 -> 3x3 pad mismatch");
    }

    // TOCOL: flatten a 2x2 row-major into a single column.
    const auto aToCol = planToColOrRow(
        aExpandSrc, /*bToColumn*/ true, /*bByColumn*/ false, FlattenIgnore::Default);
    if (!aToCol || aToCol.maValue.maDimensions.mnColumns != 1
        || aToCol.maValue.maDimensions.mnRows != 4
        || !expectNumeric(aToCol.maValue, 0, 0, 1.0)
        || !expectNumeric(aToCol.maValue, 0, 1, 2.0)
        || !expectNumeric(aToCol.maValue, 0, 2, 3.0)
        || !expectNumeric(aToCol.maValue, 0, 3, 4.0))
    {
        return fail("spreadsheetengine_spill_tests", "TOCOL flatten row-major mismatch");
    }

    // TOROW: flatten a 2x2 row-major into a single row.
    const auto aToRow = planToColOrRow(
        aExpandSrc, /*bToColumn*/ false, /*bByColumn*/ false, FlattenIgnore::Default);
    if (!aToRow || aToRow.maValue.maDimensions.mnRows != 1
        || aToRow.maValue.maDimensions.mnColumns != 4
        || !expectNumeric(aToRow.maValue, 0, 0, 1.0)
        || !expectNumeric(aToRow.maValue, 3, 0, 4.0))
    {
        return fail("spreadsheetengine_spill_tests", "TOROW flatten row-major mismatch");
    }

    // WRAPCOLS: wrap a 4-element row vector into 2x2.  `nWrapCount`
    // names the column height, so the elements fill column-major.
    const auto aWrapSrc = makeNumericMatrix(4, 1, { 1.0, 2.0, 3.0, 4.0 });
    const auto aWrapCols = planWrapColsOrRows(
        aWrapSrc, /*nWrapCount*/ 2, /*bWrapColumns*/ true, std::nullopt);
    if (!aWrapCols || aWrapCols.maValue.maDimensions.mnColumns != 2
        || aWrapCols.maValue.maDimensions.mnRows != 2
        || !expectNumeric(aWrapCols.maValue, 0, 0, 1.0)
        || !expectNumeric(aWrapCols.maValue, 0, 1, 2.0)
        || !expectNumeric(aWrapCols.maValue, 1, 0, 3.0)
        || !expectNumeric(aWrapCols.maValue, 1, 1, 4.0))
    {
        return fail("spreadsheetengine_spill_tests", "WRAPCOLS 4->2x2 mismatch");
    }

    // WRAPROWS: wrap a 4-element column vector into 2x2.  `nWrapCount`
    // names the row width, so the elements fill row-major.
    const auto aWrapColSrc = makeNumericMatrix(1, 4, { 1.0, 2.0, 3.0, 4.0 });
    const auto aWrapRows = planWrapColsOrRows(
        aWrapColSrc, /*nWrapCount*/ 2, /*bWrapColumns*/ false, std::nullopt);
    if (!aWrapRows || aWrapRows.maValue.maDimensions.mnColumns != 2
        || aWrapRows.maValue.maDimensions.mnRows != 2
        || !expectNumeric(aWrapRows.maValue, 0, 0, 1.0)
        || !expectNumeric(aWrapRows.maValue, 1, 0, 2.0)
        || !expectNumeric(aWrapRows.maValue, 0, 1, 3.0)
        || !expectNumeric(aWrapRows.maValue, 1, 1, 4.0))
    {
        return fail("spreadsheetengine_spill_tests", "WRAPROWS 4->2x2 mismatch");
    }

    // TEXTSPLIT: split "a,b;c,d" by column delim ',' and row delim ';'.
    const std::vector<String> aColDelim { String(u",") };
    const std::vector<String> aRowDelim { String(u";") };
    const auto aTextSplit = planTextSplit(
        String(u"a,b;c,d"), aColDelim, aRowDelim,
        /*bIgnoreEmpty*/ false, /*bMatchMode*/ false, std::nullopt);
    if (!aTextSplit || aTextSplit.maValue.maDimensions.mnColumns != 2
        || aTextSplit.maValue.maDimensions.mnRows != 2)
    {
        return fail("spreadsheetengine_spill_tests", "TEXTSPLIT basic grid mismatch");
    }
    {
        const auto& rGrid = aTextSplit.maValue;
        const auto cellAt = [&](MatrixSize nCol, MatrixSize nRow) {
            return rGrid.maValues[
                static_cast<std::size_t>(nRow) * rGrid.maDimensions.mnColumns + nCol];
        };
        if (cellAt(0, 0).maString != String(u"a")
            || cellAt(1, 0).maString != String(u"b")
            || cellAt(0, 1).maString != String(u"c")
            || cellAt(1, 1).maString != String(u"d"))
        {
            return fail("spreadsheetengine_spill_tests", "TEXTSPLIT grid cell text mismatch");
        }
    }

    std::cout << "spreadsheetengine spill api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
