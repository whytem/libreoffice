/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <spreadsheetengine/api/Array.hxx>

#include "SharedCaseSupport.hxx"
#include "TestSupport.hxx"

namespace
{

std::vector<std::vector<double>> parseMatrixSpec(std::string_view rSpec)
{
    std::vector<std::vector<double>> aMatrix;
    std::size_t nRowStart = 0;
    while (nRowStart <= rSpec.size())
    {
        const std::size_t nRowEnd = rSpec.find('|', nRowStart);
        const std::string_view aRowToken = nRowEnd == std::string_view::npos
                                               ? rSpec.substr(nRowStart)
                                               : rSpec.substr(nRowStart, nRowEnd - nRowStart);

        std::vector<double> aRow;
        std::size_t nValueStart = 0;
        while (nValueStart <= aRowToken.size())
        {
            const std::size_t nValueEnd = aRowToken.find(',', nValueStart);
            const std::string_view aValueToken
                = nValueEnd == std::string_view::npos
                      ? aRowToken.substr(nValueStart)
                      : aRowToken.substr(nValueStart, nValueEnd - nValueStart);
            if (!aValueToken.empty())
                aRow.push_back(std::stod(std::string(aValueToken)));

            if (nValueEnd == std::string_view::npos)
                break;

            nValueStart = nValueEnd + 1;
        }

        if (!aRow.empty())
            aMatrix.push_back(std::move(aRow));

        if (nRowEnd == std::string_view::npos)
            break;

        nRowStart = nRowEnd + 1;
    }

    return aMatrix;
}

bool matricesEqual(const std::vector<std::vector<double>>& rLeft,
                   const std::vector<std::vector<double>>& rRight)
{
    if (rLeft.size() != rRight.size())
        return false;

    for (std::size_t i = 0; i < rLeft.size(); ++i)
    {
        if (rLeft[i].size() != rRight[i].size())
            return false;

        for (std::size_t j = 0; j < rLeft[i].size(); ++j)
        {
            if (!spreadsheetengine::standalone::test::almostEqual(rLeft[i][j], rRight[i][j]))
                return false;
        }
    }

    return true;
}

} // namespace

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::MatrixCoordinate;
    using spreadsheetengine::api::MatrixDimensions;
    using spreadsheetengine::api::MatrixSize;
    using spreadsheetengine::api::array::Axis;
    using spreadsheetengine::api::array::FlattenIgnore;
    using spreadsheetengine::api::array::StackDirection;
    using spreadsheetengine::standalone::test::loadSharedCaseRows;
    using spreadsheetengine::standalone::test::parseDouble;
    using spreadsheetengine::standalone::test::fail;

    const auto aChooseIndex = spreadsheetengine::api::array::normalizeSelectionIndex(-1, 5);
    const auto aChooseBad = spreadsheetengine::api::array::normalizeSelectionIndex(0, 5);
    if (!aChooseIndex || aChooseIndex.maValue != 4 || aChooseBad
        || aChooseBad.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_array_tests", "selection index planning mismatch");
    }

    const auto aChooseCols = spreadsheetengine::api::array::planChooseResultDimensions(
        { 3, 2 }, 2, Axis::Columns);
    const auto aChooseRows = spreadsheetengine::api::array::planChooseResultDimensions(
        { 3, 2 }, 2, Axis::Rows);
    if (!aChooseCols || aChooseCols.maValue.mnColumns != 2 || aChooseCols.maValue.mnRows != 2
        || !aChooseRows || aChooseRows.maValue.mnColumns != 3 || aChooseRows.maValue.mnRows != 2)
    {
        return fail("spreadsheetengine_array_tests", "choose result dimensions mismatch");
    }

    const auto aTake = spreadsheetengine::api::array::planTakeDropSlice(
        { 4, 5 }, true, sal_Int32(2), sal_Int32(-2));
    const auto aDrop = spreadsheetengine::api::array::planTakeDropSlice(
        { 4, 5 }, false, sal_Int32(2), sal_Int32(1));
    const auto aTakeEmpty = spreadsheetengine::api::array::planTakeDropSlice(
        { 4, 5 }, true, sal_Int32(0), std::nullopt);
    if (!aTake || aTake.maValue.maStart.mnColumn != 2 || aTake.maValue.maStart.mnRow != 0
        || aTake.maValue.maDimensions.mnColumns != 2 || aTake.maValue.maDimensions.mnRows != 2
        || !aDrop || aDrop.maValue.maStart.mnColumn != 1 || aDrop.maValue.maStart.mnRow != 2
        || aDrop.maValue.maDimensions.mnColumns != 3 || aDrop.maValue.maDimensions.mnRows != 3
        || aTakeEmpty || aTakeEmpty.meError != Error::NotAvailable)
    {
        return fail("spreadsheetengine_array_tests", "take/drop planning mismatch");
    }

    const auto aExpand = spreadsheetengine::api::array::planExpandDimensions(
        { 2, 2 }, sal_Int32(3), sal_Int32(4));
    const auto aExpandBad = spreadsheetengine::api::array::planExpandDimensions(
        { 2, 2 }, sal_Int32(1), std::nullopt);
    if (!aExpand || aExpand.maValue.mnColumns != 4 || aExpand.maValue.mnRows != 3
        || aExpandBad || aExpandBad.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_array_tests", "expand planning mismatch");
    }

    const MatrixDimensions aHStack = spreadsheetengine::api::array::appendStackDimensions(
        { 2, 3 }, { 4, 1 }, StackDirection::Horizontal);
    const MatrixDimensions aVStack = spreadsheetengine::api::array::appendStackDimensions(
        { 2, 3 }, { 4, 1 }, StackDirection::Vertical);
    const MatrixCoordinate aHStackDest = spreadsheetengine::api::array::stackDestination(
        StackDirection::Horizontal, { 1, 0 }, 3);
    const MatrixCoordinate aVStackDest = spreadsheetengine::api::array::stackDestination(
        StackDirection::Vertical, { 1, 0 }, 3);
    if (aHStack.mnColumns != 6 || aHStack.mnRows != 3 || aVStack.mnColumns != 4
        || aVStack.mnRows != 4 || aHStackDest.mnColumn != 4 || aHStackDest.mnRow != 0
        || aVStackDest.mnColumn != 1 || aVStackDest.mnRow != 3)
    {
        return fail("spreadsheetengine_array_tests", "stack planning mismatch");
    }

    if (!spreadsheetengine::api::array::shouldIncludeFlattenedValue(
            FlattenIgnore::Default, false, false)
        || spreadsheetengine::api::array::shouldIncludeFlattenedValue(
            FlattenIgnore::Blanks, true, false)
        || spreadsheetengine::api::array::shouldIncludeFlattenedValue(
            FlattenIgnore::Errors, false, true)
        || spreadsheetengine::api::array::shouldIncludeFlattenedValue(
            FlattenIgnore::All, true, false))
    {
        return fail("spreadsheetengine_array_tests", "flatten ignore mismatch");
    }

    const auto aToCol
        = spreadsheetengine::api::array::planFlattenOutputDimensions(5, true);
    const auto aToRow
        = spreadsheetengine::api::array::planFlattenOutputDimensions(5, false);
    if (!aToCol || aToCol.maValue.mnColumns != 1 || aToCol.maValue.mnRows != 5
        || !aToRow || aToRow.maValue.mnColumns != 5 || aToRow.maValue.mnRows != 1
        || spreadsheetengine::api::array::flattenDestination(3, true).mnRow != 3
        || spreadsheetengine::api::array::flattenDestination(3, false).mnColumn != 3)
    {
        return fail("spreadsheetengine_array_tests", "flatten planning mismatch");
    }

    const auto aWrapCols = spreadsheetengine::api::array::planWrapOutputDimensions(
        { 1, 5 }, 2, true);
    const auto aWrapRows = spreadsheetengine::api::array::planWrapOutputDimensions(
        { 5, 1 }, 2, false);
    const auto aWrapBad = spreadsheetengine::api::array::planWrapOutputDimensions(
        { 2, 2 }, 2, true);
    if (!aWrapCols || aWrapCols.maValue.mnColumns != 3 || aWrapCols.maValue.mnRows != 2
        || !aWrapRows || aWrapRows.maValue.mnColumns != 2 || aWrapRows.maValue.mnRows != 3
        || aWrapBad || aWrapBad.meError != Error::IllegalArgument
        || spreadsheetengine::api::array::wrapDestination(4, 2, true).mnColumn != 2
        || spreadsheetengine::api::array::wrapDestination(4, 2, true).mnRow != 0
        || spreadsheetengine::api::array::wrapDestination(4, 2, false).mnColumn != 0
        || spreadsheetengine::api::array::wrapDestination(4, 2, false).mnRow != 2)
    {
        return fail("spreadsheetengine_array_tests", "wrap planning mismatch");
    }

    const std::vector<std::vector<double>> aSource3x3
        = { { 1.0, 2.0, 3.0 }, { 4.0, 5.0, 6.0 }, { 7.0, 8.0, 9.0 } };
    const std::vector<std::vector<double>> aSource2x2 = { { 1.0, 2.0 }, { 4.0, 5.0 } };
    const std::vector<double> aVector5 = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    const std::vector<std::vector<double>> aStackLeft = { { 10.0, 11.0 }, { 12.0, 13.0 } };
    const std::vector<std::vector<double>> aStackRight = { { 20.0, 21.0 }, { 22.0, 23.0 } };

    for (const auto& rRow : loadSharedCaseRows("array_cases.tsv"))
    {
        if (rRow.maColumns.size() < 6)
            return failSharedCase(
                "spreadsheetengine_array_tests", rRow, "array shared case column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const auto aExpected = parseMatrixSpec(rRow.maColumns[4]);
        std::vector<std::vector<double>> aActual;

        if (rFunction == "TAKE" || rFunction == "DROP")
        {
            const auto aSlice = spreadsheetengine::api::array::planTakeDropSlice(
                { 3, 3 }, rFunction == "TAKE",
                static_cast<sal_Int32>(parseDouble(rRow.maColumns[1])),
                static_cast<sal_Int32>(parseDouble(rRow.maColumns[2])));
            if (!aSlice)
                return failSharedCase(
                    "spreadsheetengine_array_tests", rRow, "TAKE/DROP slice mismatch");

            for (MatrixSize nRow = 0; nRow < aSlice.maValue.maDimensions.mnRows; ++nRow)
            {
                std::vector<double> aOutRow;
                for (MatrixSize nCol = 0; nCol < aSlice.maValue.maDimensions.mnColumns; ++nCol)
                {
                    aOutRow.push_back(aSource3x3[aSlice.maValue.maStart.mnRow + nRow]
                                                [aSlice.maValue.maStart.mnColumn + nCol]);
                }
                aActual.push_back(std::move(aOutRow));
            }
        }
        else if (rFunction == "EXPAND")
        {
            const auto aDimensions = spreadsheetengine::api::array::planExpandDimensions(
                { 2, 2 }, static_cast<sal_Int32>(parseDouble(rRow.maColumns[1])),
                static_cast<sal_Int32>(parseDouble(rRow.maColumns[2])));
            if (!aDimensions)
                return failSharedCase(
                    "spreadsheetengine_array_tests", rRow, "EXPAND dimensions mismatch");

            aActual.assign(aDimensions.maValue.mnRows,
                           std::vector<double>(aDimensions.maValue.mnColumns,
                                               parseDouble(rRow.maColumns[3])));
            for (std::size_t nRow = 0; nRow < aSource2x2.size(); ++nRow)
            {
                for (std::size_t nCol = 0; nCol < aSource2x2[nRow].size(); ++nCol)
                    aActual[nRow][nCol] = aSource2x2[nRow][nCol];
            }
        }
        else if (rFunction == "CHOOSECOLS" || rFunction == "CHOOSEROWS")
        {
            const auto aIndex1 = spreadsheetengine::api::array::normalizeSelectionIndex(
                static_cast<sal_Int32>(parseDouble(rRow.maColumns[1])),
                rFunction == "CHOOSECOLS" ? 3 : 3);
            const auto aIndex2 = spreadsheetengine::api::array::normalizeSelectionIndex(
                static_cast<sal_Int32>(parseDouble(rRow.maColumns[2])),
                rFunction == "CHOOSECOLS" ? 3 : 3);
            const auto aDimensions = spreadsheetengine::api::array::planChooseResultDimensions(
                { 3, 3 }, 2,
                rFunction == "CHOOSECOLS" ? Axis::Columns : Axis::Rows);
            if (!aIndex1 || !aIndex2 || !aDimensions)
                return failSharedCase(
                    "spreadsheetengine_array_tests", rRow, "CHOOSE selection mismatch");

            aActual.assign(aDimensions.maValue.mnRows,
                           std::vector<double>(aDimensions.maValue.mnColumns, 0.0));
            if (rFunction == "CHOOSECOLS")
            {
                const std::vector<MatrixSize> aColumns { aIndex1.maValue, aIndex2.maValue };
                for (MatrixSize nRow = 0; nRow < 3; ++nRow)
                {
                    for (std::size_t i = 0; i < aColumns.size(); ++i)
                        aActual[nRow][i] = aSource3x3[nRow][aColumns[i]];
                }
            }
            else
            {
                const std::vector<MatrixSize> aRows { aIndex1.maValue, aIndex2.maValue };
                for (std::size_t i = 0; i < aRows.size(); ++i)
                    aActual[i] = aSource3x3[aRows[i]];
            }
        }
        else if (rFunction == "TOCOL" || rFunction == "TOROW")
        {
            const bool bToColumn = rFunction == "TOCOL";
            const auto aDimensions
                = spreadsheetengine::api::array::planFlattenOutputDimensions(4, bToColumn);
            if (!aDimensions)
                return failSharedCase(
                    "spreadsheetengine_array_tests", rRow, "flatten dimensions mismatch");

            aActual.assign(aDimensions.maValue.mnRows,
                           std::vector<double>(aDimensions.maValue.mnColumns, 0.0));
            MatrixSize nLinearIndex = 0;
            for (const auto& rSourceRow : aSource2x2)
            {
                for (double fValue : rSourceRow)
                {
                    const auto aDestination = spreadsheetengine::api::array::flattenDestination(
                        nLinearIndex++, bToColumn);
                    aActual[aDestination.mnRow][aDestination.mnColumn] = fValue;
                }
            }
        }
        else if (rFunction == "WRAPROWS" || rFunction == "WRAPCOLS")
        {
            const bool bWrapColumns = rFunction == "WRAPCOLS";
            const auto aDimensions = spreadsheetengine::api::array::planWrapOutputDimensions(
                { 1, 5 }, static_cast<MatrixSize>(parseDouble(rRow.maColumns[1])), bWrapColumns);
            if (!aDimensions)
                return failSharedCase(
                    "spreadsheetengine_array_tests", rRow, "wrap dimensions mismatch");

            aActual.assign(aDimensions.maValue.mnRows,
                           std::vector<double>(aDimensions.maValue.mnColumns,
                                               parseDouble(rRow.maColumns[2])));
            for (MatrixSize nIndex = 0; nIndex < static_cast<MatrixSize>(aVector5.size()); ++nIndex)
            {
                const auto aDestination = spreadsheetengine::api::array::wrapDestination(
                    nIndex, static_cast<MatrixSize>(parseDouble(rRow.maColumns[1])), bWrapColumns);
                aActual[aDestination.mnRow][aDestination.mnColumn] = aVector5[nIndex];
            }
        }
        else if (rFunction == "HSTACK" || rFunction == "VSTACK")
        {
            const StackDirection eDirection = rFunction == "HSTACK"
                                                 ? StackDirection::Horizontal
                                                 : StackDirection::Vertical;
            const MatrixDimensions aDimensions
                = spreadsheetengine::api::array::appendStackDimensions({ 2, 2 }, { 2, 2 }, eDirection);
            aActual.assign(aDimensions.mnRows, std::vector<double>(aDimensions.mnColumns, 0.0));

            for (MatrixSize nRow = 0; nRow < 2; ++nRow)
            {
                for (MatrixSize nCol = 0; nCol < 2; ++nCol)
                {
                    const auto aLeftDest = spreadsheetengine::api::array::stackDestination(
                        eDirection, MatrixCoordinate { nCol, nRow }, 0);
                    const auto aRightDest = spreadsheetengine::api::array::stackDestination(
                        eDirection, MatrixCoordinate { nCol, nRow }, 2);
                    aActual[aLeftDest.mnRow][aLeftDest.mnColumn] = aStackLeft[nRow][nCol];
                    aActual[aRightDest.mnRow][aRightDest.mnColumn] = aStackRight[nRow][nCol];
                }
            }
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_array_tests", rRow, "unknown array shared-case function");
        }

        if (!matricesEqual(aActual, aExpected))
            return failSharedCase(
                "spreadsheetengine_array_tests", rRow, "array shared-case mismatch");
    }

    std::cout << "spreadsheetengine array api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
