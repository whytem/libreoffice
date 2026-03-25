/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/Reference.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::api::CellRange;
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::MatrixDimensions;
    using spreadsheetengine::api::reference::IndexSelectionKind;
    using spreadsheetengine::standalone::test::fail;

    const auto aArea = spreadsheetengine::api::reference::normalizeAreaSelection(2, 3);
    const auto aBadArea = spreadsheetengine::api::reference::normalizeAreaSelection(4, 3);
    if (!aArea || aArea.maValue != 1 || aBadArea || aBadArea.meError != Error::NotAvailable)
        return fail("spreadsheetengine_reference_tests", "area selection mismatch");

    const CellRange aBaseRange { CellAddress { 0, 2, 3 }, CellAddress { 0, 4, 5 } };
    const auto aOffset = spreadsheetengine::api::reference::planOffsetRange(
        aBaseRange, 1, -2, std::nullopt, std::nullopt, 1023, 65535);
    const auto aOffsetResize = spreadsheetengine::api::reference::planOffsetRange(
        aBaseRange, 0, 1, sal_Int32(2), sal_Int32(4), 1023, 65535);
    const auto aBadOffset = spreadsheetengine::api::reference::planOffsetRange(
        aBaseRange, 0, 0, sal_Int32(0), std::nullopt, 1023, 65535);
    if (!aOffset || aOffset.maValue.maStart.mnColumn != 0 || aOffset.maValue.maStart.mnRow != 4
        || aOffset.maValue.maEnd.mnColumn != 2 || aOffset.maValue.maEnd.mnRow != 6
        || !aOffsetResize || aOffsetResize.maValue.columnCount() != 4
        || aOffsetResize.maValue.rowCount() != 2 || aBadOffset
        || aBadOffset.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_reference_tests", "offset planning mismatch");
    }

    const auto aMatrixKeep = spreadsheetengine::api::reference::planIndexMatrixSelection(
        MatrixDimensions { 3, 2 }, 0, 0, false, 2);
    const auto aMatrixRow = spreadsheetengine::api::reference::planIndexMatrixSelection(
        MatrixDimensions { 3, 2 }, 2, 0, false, 3);
    const auto aMatrixColumn = spreadsheetengine::api::reference::planIndexMatrixSelection(
        MatrixDimensions { 3, 2 }, 0, 2, false, 3);
    const auto aRowVectorElement = spreadsheetengine::api::reference::planIndexMatrixSelection(
        MatrixDimensions { 3, 1 }, 2, 0, false, 2);
    const auto aBadMatrix = spreadsheetengine::api::reference::planIndexMatrixSelection(
        MatrixDimensions { 3, 2 }, 3, 4, false, 3);
    if (!aMatrixKeep || aMatrixKeep.maValue.meKind != IndexSelectionKind::KeepSource
        || !aMatrixRow || aMatrixRow.maValue.meKind != IndexSelectionKind::RowSlice
        || aMatrixRow.maValue.maStart.mnRow != 1
        || aMatrixRow.maValue.maDimensions.mnColumns != 3
        || !aMatrixColumn || aMatrixColumn.maValue.meKind != IndexSelectionKind::ColumnSlice
        || aMatrixColumn.maValue.maStart.mnColumn != 1
        || aMatrixColumn.maValue.maDimensions.mnRows != 2 || !aRowVectorElement
        || aRowVectorElement.maValue.meKind != IndexSelectionKind::Scalar
        || aRowVectorElement.maValue.maStart.mnColumn != 1
        || aRowVectorElement.maValue.maStart.mnRow != 0 || aBadMatrix
        || aBadMatrix.meError != Error::NotAvailable)
    {
        return fail("spreadsheetengine_reference_tests", "matrix INDEX planning mismatch");
    }

    const auto aReferenceKeep = spreadsheetengine::api::reference::planIndexReferenceSelection(
        aBaseRange, 0, 0, 2);
    const auto aReferenceRow = spreadsheetengine::api::reference::planIndexReferenceSelection(
        aBaseRange, 2, 0, 3);
    const auto aReferenceColumn = spreadsheetengine::api::reference::planIndexReferenceSelection(
        aBaseRange, 0, 2, 3);
    const auto aRowArrayElement = spreadsheetengine::api::reference::planIndexReferenceSelection(
        CellRange { CellAddress { 0, 4, 2 }, CellAddress { 0, 6, 2 } }, 3, 0, 2);
    const auto aBadReference = spreadsheetengine::api::reference::planIndexReferenceSelection(
        aBaseRange, 5, 0, 3);
    if (!aReferenceKeep || aReferenceKeep.maValue.meKind != IndexSelectionKind::KeepSource
        || !aReferenceRow || aReferenceRow.maValue.meKind != IndexSelectionKind::RowSlice
        || aReferenceRow.maValue.maRange.maStart.mnRow != 4
        || aReferenceRow.maValue.maRange.maEnd.mnColumn != 4 || !aReferenceColumn
        || aReferenceColumn.maValue.meKind != IndexSelectionKind::ColumnSlice
        || aReferenceColumn.maValue.maRange.maStart.mnColumn != 3
        || aReferenceColumn.maValue.maRange.maEnd.mnRow != 5 || !aRowArrayElement
        || aRowArrayElement.maValue.meKind != IndexSelectionKind::Scalar
        || aRowArrayElement.maValue.maRange.maStart.mnColumn != 6
        || aRowArrayElement.maValue.maRange.maStart.mnRow != 2 || aBadReference
        || aBadReference.meError != Error::NotAvailable)
    {
        return fail("spreadsheetengine_reference_tests", "reference INDEX planning mismatch");
    }

    std::cout << "spreadsheetengine reference api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
