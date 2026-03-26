/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <vector>

#include <spreadsheetengine/api/Reference.hxx>
#include <spreadsheetengine/api/ReferenceData.hxx>
#include <spreadsheetengine/api/ReferenceUpdate.hxx>

#include "SharedCaseSupport.hxx"
#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::api::CellRange;
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::MatrixDimensions;
    using spreadsheetengine::api::RowIndex;
    using spreadsheetengine::api::ColumnIndex;
    using spreadsheetengine::api::reference::IndexSelectionKind;
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::loadSharedCaseRows;
    using spreadsheetengine::standalone::test::parseDouble;
    using spreadsheetengine::standalone::test::parseExpectedError;
    using spreadsheetengine::standalone::test::fail;

    const std::vector<std::vector<double>> aMatrix
        = { { 1.0, 2.0, 3.0 }, { 4.0, 5.0, 6.0 }, { 7.0, 8.0, 9.0 } };
    const std::vector<double> aRowVector { 10.0, 20.0, 30.0 };

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

    spreadsheetengine::api::refdata::ComplexRefData aWrappedRef;
    aWrappedRef.maRef1.mnColumn = -2;
    aWrappedRef.maRef1.mnRow = -1;
    aWrappedRef.maRef1.mnSheet = -1;
    aWrappedRef.maRef1.maFlags.mbColumnRelative = true;
    aWrappedRef.maRef1.maFlags.mbRowRelative = true;
    aWrappedRef.maRef1.maFlags.mbSheetRelative = true;
    aWrappedRef.maRef2.mnColumn = 3;
    aWrappedRef.maRef2.mnRow = 2;
    aWrappedRef.maRef2.mnSheet = 0;
    aWrappedRef.maRef2.maFlags.mbColumnRelative = true;
    aWrappedRef.maRef2.maFlags.mbRowRelative = true;
    aWrappedRef.maRef2.maFlags.mbSheetRelative = true;
    spreadsheetengine::api::refupdate::moveRelativeWrap(
        aWrappedRef, { 9, 19, 4 }, { 1, 1, 1 }, 9, 19, 4);
    const auto aWrappedRange
        = spreadsheetengine::api::refdata::toAbsoluteRange(aWrappedRef, { 9, 19, 4 }, { 1, 1, 1 });
    if (aWrappedRange.maStart != CellAddress { 0, 4, 0 }
        || aWrappedRange.maEnd != CellAddress { 1, 9, 3 })
    {
        return fail("spreadsheetengine_reference_tests", "relative wrap mismatch");
    }

    spreadsheetengine::api::ColumnIndex nCol1 = 3;
    spreadsheetengine::api::RowIndex nRow1 = 4;
    spreadsheetengine::api::SheetId nTab1 = 1;
    spreadsheetengine::api::refupdate::doTranspose(
        nCol1, nRow1, nTab1, 5, CellRange { { 0, 2, 3 }, { 0, 4, 5 } }, { 2, 10, 20 });
    if (nCol1 != 11 || nRow1 != 21 || nTab1 != 3)
    {
        return fail("spreadsheetengine_reference_tests", "transpose point mismatch");
    }

    CellRange aTransposeRange { { 0, 2, 3 }, { 0, 4, 5 } };
    const CellRange aExpectedTransposeRange { { 2, 10, 20 }, { 2, 12, 22 } };
    if (!spreadsheetengine::api::refupdate::updateTranspose(
            5, CellRange { { 0, 2, 3 }, { 0, 4, 5 } }, { 2, 10, 20 }, aTransposeRange)
        || aTransposeRange != aExpectedTransposeRange)
    {
        return fail("spreadsheetengine_reference_tests", "transpose range mismatch");
    }
    CellRange aUnchangedTranspose { { 0, 0, 0 }, { 0, 1, 1 } };
    if (spreadsheetengine::api::refupdate::updateTranspose(
            5, CellRange { { 0, 2, 3 }, { 0, 4, 5 } }, { 2, 10, 20 }, aUnchangedTranspose))
    {
        return fail("spreadsheetengine_reference_tests", "transpose unchanged mismatch");
    }

    CellRange aGrowRange { { 0, 2, 3 }, { 0, 4, 5 } };
    const CellRange aExpectedGrowRange { { 0, 2, 3 }, { 0, 6, 8 } };
    if (!spreadsheetengine::api::refupdate::shouldUpdateGrowColumns(
            CellRange { { 0, 2, 3 }, { 0, 4, 5 } }, 2, aGrowRange)
        || !spreadsheetengine::api::refupdate::shouldUpdateGrowRows(
            CellRange { { 0, 2, 3 }, { 0, 4, 5 } }, 3,
            CellRange { { 0, 2, 4 }, { 0, 4, 5 } })
        || !spreadsheetengine::api::refupdate::updateGrow(
            CellRange { { 0, 2, 3 }, { 0, 4, 5 } }, 2, 3, aGrowRange)
        || aGrowRange != aExpectedGrowRange)
    {
        return fail("spreadsheetengine_reference_tests", "grow planning mismatch");
    }
    CellRange aUnchangedGrow { { 0, 9, 9 }, { 0, 10, 10 } };
    if (spreadsheetengine::api::refupdate::updateGrow(
            CellRange { { 0, 2, 3 }, { 0, 4, 5 } }, 0, 0, aUnchangedGrow))
    {
        return fail("spreadsheetengine_reference_tests", "grow unchanged mismatch");
    }

    CellRange aUpdatedRef { { 0, 5, 2 }, { 0, 7, 4 } };
    const auto eUpdated = spreadsheetengine::api::refupdate::updateReference(
        spreadsheetengine::api::refupdate::UpdateMode::InsertDelete,
        CellRange { { 0, 5, 0 }, { 0, 9, 9 } }, 2, 0, 0, 9, 19, 4, false, aUpdatedRef);
    if (eUpdated != spreadsheetengine::api::refupdate::UpdateResult::Updated
        || aUpdatedRef != CellRange { { 0, 7, 2 }, { 0, 9, 4 } })
    {
        return fail("spreadsheetengine_reference_tests", "insert/delete update mismatch");
    }

    CellRange aStickyRef { { 0, 0, 2 }, { 0, 9, 4 } };
    const auto eSticky = spreadsheetengine::api::refupdate::updateReference(
        spreadsheetengine::api::refupdate::UpdateMode::InsertDelete,
        CellRange { { 0, 5, 0 }, { 0, 9, 9 } }, 1, 0, 0, 9, 19, 4, false, aStickyRef);
    if (eSticky != spreadsheetengine::api::refupdate::UpdateResult::Sticky
        || aStickyRef != CellRange { { 0, 0, 2 }, { 0, 9, 4 } })
    {
        return fail("spreadsheetengine_reference_tests", "sticky update mismatch");
    }

    CellRange aMovedRef { { 0, 3, 1 }, { 0, 5, 2 } };
    const auto eMoved = spreadsheetengine::api::refupdate::updateReference(
        spreadsheetengine::api::refupdate::UpdateMode::Move,
        CellRange { { 0, 5, 0 }, { 0, 7, 9 } }, 2, 0, 0, 9, 19, 4, false, aMovedRef);
    if (eMoved != spreadsheetengine::api::refupdate::UpdateResult::Updated
        || aMovedRef != CellRange { { 0, 5, 1 }, { 0, 7, 2 } })
    {
        return fail("spreadsheetengine_reference_tests", "move update mismatch");
    }

    CellRange aReorderedRef { { 2, 3, 1 }, { 2, 5, 2 } };
    const auto eReordered = spreadsheetengine::api::refupdate::updateReference(
        spreadsheetengine::api::refupdate::UpdateMode::Reorder,
        CellRange { { 1, 0, 0 }, { 3, 9, 9 } }, 0, 0, 2, 9, 19, 4, false, aReorderedRef);
    if (eReordered != spreadsheetengine::api::refupdate::UpdateResult::Updated
        || aReorderedRef != CellRange { { 4, 3, 1 }, { 4, 5, 2 } })
    {
        return fail("spreadsheetengine_reference_tests", "reorder update mismatch");
    }

    for (const auto& rRow : loadSharedCaseRows("reference_cases.tsv"))
    {
        if (rRow.maColumns.size() < 7)
            return failSharedCase(
                "spreadsheetengine_reference_tests", rRow,
                "reference shared case column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const Error eExpectedError = parseExpectedError(rRow.maColumns[6]);
        const double fExpected = parseDouble(rRow.maColumns[5]);

        if (rFunction == "INDEX")
        {
            const auto aSelection = spreadsheetengine::api::reference::planIndexMatrixSelection(
                MatrixDimensions { 3, 3 }, static_cast<RowIndex>(parseDouble(rRow.maColumns[1])),
                static_cast<ColumnIndex>(parseDouble(rRow.maColumns[2])), false, 3);
            if (eExpectedError != Error::None)
            {
                if (aSelection || aSelection.meError != eExpectedError)
                {
                    return failSharedCase(
                        "spreadsheetengine_reference_tests", rRow, "INDEX error mismatch");
                }
            }
            else if (!aSelection
                     || !almostEqual(aMatrix[aSelection.maValue.maStart.mnRow]
                                                [aSelection.maValue.maStart.mnColumn],
                         fExpected))
            {
                return failSharedCase(
                    "spreadsheetengine_reference_tests", rRow, "INDEX value mismatch");
            }
        }
        else if (rFunction == "INDEX_ROWVECTOR")
        {
            const auto aSelection = spreadsheetengine::api::reference::planIndexReferenceSelection(
                CellRange { CellAddress { 0, 0, 0 }, CellAddress { 0, 2, 0 } },
                0, static_cast<ColumnIndex>(parseDouble(rRow.maColumns[1])), 3);
            if (!aSelection
                || !almostEqual(aRowVector[aSelection.maValue.maRange.maStart.mnColumn], fExpected))
            {
                return failSharedCase(
                    "spreadsheetengine_reference_tests", rRow, "INDEX row-vector mismatch");
            }
        }
        else if (rFunction == "OFFSET_VALUE" || rFunction == "OFFSET_SUM")
        {
            const auto aRange = spreadsheetengine::api::reference::planOffsetRange(
                CellRange { CellAddress { 0, 0, 0 }, CellAddress { 0, 0, 0 } },
                static_cast<RowIndex>(parseDouble(rRow.maColumns[1])),
                static_cast<ColumnIndex>(parseDouble(rRow.maColumns[2])),
                rFunction == "OFFSET_SUM"
                    ? std::optional<RowIndex>(static_cast<RowIndex>(parseDouble(rRow.maColumns[3])))
                    : std::nullopt,
                rFunction == "OFFSET_SUM"
                    ? std::optional<ColumnIndex>(static_cast<ColumnIndex>(parseDouble(rRow.maColumns[4])))
                    : std::nullopt,
                1023, 65535);
            if (!aRange)
                return failSharedCase(
                    "spreadsheetengine_reference_tests", rRow, "OFFSET range mismatch");

            double fActual = 0.0;
            for (RowIndex nRow = aRange.maValue.maStart.mnRow; nRow <= aRange.maValue.maEnd.mnRow;
                 ++nRow)
            {
                for (ColumnIndex nCol = aRange.maValue.maStart.mnColumn;
                     nCol <= aRange.maValue.maEnd.mnColumn; ++nCol)
                {
                    fActual += aMatrix[nRow][nCol];
                }
            }

            if (!almostEqual(fActual, fExpected))
                return failSharedCase(
                    "spreadsheetengine_reference_tests", rRow, "OFFSET value mismatch");
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_reference_tests", rRow,
                "unknown reference shared-case function");
        }
    }

    std::cout << "spreadsheetengine reference api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
