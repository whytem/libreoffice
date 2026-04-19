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
using spreadsheetengine::core::rpn::MatrixOperand;
using spreadsheetengine::core::rpn::MatrixProvenance;
using spreadsheetengine::core::rpn::spill::planDrop;
using spreadsheetengine::core::rpn::spill::planFilter;
using spreadsheetengine::core::rpn::spill::planSort;
using spreadsheetengine::core::rpn::spill::planTake;
using spreadsheetengine::core::rpn::spill::planUnique;
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

    std::cout << "spreadsheetengine spill api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
