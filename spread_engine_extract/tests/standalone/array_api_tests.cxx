/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/Array.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::MatrixCoordinate;
    using spreadsheetengine::api::MatrixDimensions;
    using spreadsheetengine::api::array::Axis;
    using spreadsheetengine::api::array::FlattenIgnore;
    using spreadsheetengine::api::array::StackDirection;
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

    std::cout << "spreadsheetengine array api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
