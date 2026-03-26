/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <vector>

#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/detail/JumpMatrixRuntime.hxx>
#include <spreadsheetengine/detail/MatrixGeometry.hxx>
#include <spreadsheetengine/detail/MatrixRuntime.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::MatrixCoordinate;
    using spreadsheetengine::api::MatrixDimensions;
    using spreadsheetengine::api::MatrixValue;
    using spreadsheetengine::api::MatrixValueType;
    using spreadsheetengine::standalone::test::fail;

    if (!spreadsheetengine::api::isValueType(MatrixValueType::Value)
        || !spreadsheetengine::api::isValueType(MatrixValueType::Boolean)
        || spreadsheetengine::api::isValueType(MatrixValueType::Text)
        || !spreadsheetengine::api::isNonValueType(MatrixValueType::EmptyPath)
        || !spreadsheetengine::api::isRealStringType(MatrixValueType::Text)
        || !spreadsheetengine::api::isEmptyType(MatrixValueType::Empty)
        || !spreadsheetengine::api::isEmptyPathType(MatrixValueType::EmptyPath))
    {
        return fail("spreadsheetengine_matrix_tests", "matrix value type helpers mismatch");
    }

    const MatrixDimensions aDimensions { 3, 2 };
    if (!aDimensions.isAllocated() || aDimensions.isEmpty() || aDimensions.elementCount() != 6)
        return fail("spreadsheetengine_matrix_tests", "matrix dimensions mismatch");

    if (!spreadsheetengine::core::matrix::isSizeAllocatable(aDimensions, 6)
        || spreadsheetengine::core::matrix::isSizeAllocatable(aDimensions, 5)
        || spreadsheetengine::core::matrix::isSizeAllocatable({ 3, 0 }, 9))
    {
        return fail("spreadsheetengine_matrix_tests", "matrix allocatability mismatch");
    }

    if (spreadsheetengine::core::matrix::elementsForMemoryBudget(120) != 10
        || spreadsheetengine::core::matrix::cappedElementLimitForMemory(120, 1000) != 10
        || !spreadsheetengine::core::matrix::hasAllocatableShape({ 0, 0 })
        || spreadsheetengine::core::matrix::hasAllocatableShape({ 3, 0 })
        || !spreadsheetengine::core::matrix::fitsWithinElementLimit({ 3, 2 }, 6)
        || spreadsheetengine::core::matrix::fitsWithinElementLimit({ 3, 2 }, 5))
    {
        return fail("spreadsheetengine_matrix_tests", "matrix runtime mismatch");
    }

    const auto aAllocationPlan = spreadsheetengine::core::matrix::planAllocation(
        { 3, 2 }, 6, spreadsheetengine::core::matrix::AllocationFallback::MatrixSize);
    if (aAllocationPlan.usesFallback()
        || aAllocationPlan.maStorageDimensions.mnColumns != 3
        || aAllocationPlan.maStorageDimensions.mnRows != 2)
    {
        return fail("spreadsheetengine_matrix_tests", "matrix allocation plan mismatch");
    }

    const auto aFallbackPlan = spreadsheetengine::core::matrix::planAllocation(
        { 3, 2 }, 5, spreadsheetengine::core::matrix::AllocationFallback::StackOverflow);
    if (!aFallbackPlan.usesFallback()
        || aFallbackPlan.meFallback
               != spreadsheetengine::core::matrix::AllocationFallback::StackOverflow
        || aFallbackPlan.maStorageDimensions.mnColumns != 1
        || aFallbackPlan.maStorageDimensions.mnRows != 1)
    {
        return fail("spreadsheetengine_matrix_tests", "matrix fallback plan mismatch");
    }

    if (spreadsheetengine::core::matrix::defaultMemoryBudgetBytes(4) != 0x40000000
        || spreadsheetengine::core::matrix::defaultMemoryBudgetBytes(8) != 0x180000000
        || spreadsheetengine::core::matrix::defaultElementLimitForPlatform(1000, 8)
               != spreadsheetengine::core::matrix::cappedElementLimitForMemory(0x180000000, 1000))
    {
        return fail("spreadsheetengine_matrix_tests", "matrix default budget mismatch");
    }

    if (!spreadsheetengine::core::matrix::isStoredStringOrEmpty(
            spreadsheetengine::core::matrix::StoredElementType::String)
        || !spreadsheetengine::core::matrix::isStoredStringOrEmpty(
            spreadsheetengine::core::matrix::StoredElementType::Empty)
        || spreadsheetengine::core::matrix::isStoredStringOrEmpty(
            spreadsheetengine::core::matrix::StoredElementType::Numeric)
        || !spreadsheetengine::core::matrix::isStoredValue(
            spreadsheetengine::core::matrix::StoredElementType::Numeric)
        || !spreadsheetengine::core::matrix::isStoredValueOrEmpty(
            spreadsheetengine::core::matrix::StoredElementType::Empty)
        || !spreadsheetengine::core::matrix::isStoredBoolean(
            spreadsheetengine::core::matrix::StoredElementType::Boolean))
    {
        return fail("spreadsheetengine_matrix_tests", "stored element classification mismatch");
    }

    if (!spreadsheetengine::core::matrix::isStoredEmptyCell(
            spreadsheetengine::core::matrix::StoredElementType::Empty,
            spreadsheetengine::core::matrix::StoredFlagType::Empty)
        || spreadsheetengine::core::matrix::classifyStoredEmptyKind(
               spreadsheetengine::core::matrix::StoredFlagType::Empty, 0)
               != spreadsheetengine::core::matrix::StoredEmptyKind::Cell
        || spreadsheetengine::core::matrix::classifyStoredEmptyKind(
               spreadsheetengine::core::matrix::StoredFlagType::Integer,
               spreadsheetengine::core::matrix::kEmptyResultFlagValue)
               != spreadsheetengine::core::matrix::StoredEmptyKind::Result
        || spreadsheetengine::core::matrix::classifyStoredEmptyKind(
               spreadsheetengine::core::matrix::StoredFlagType::Integer,
               spreadsheetengine::core::matrix::kEmptyPathFlagValue)
               != spreadsheetengine::core::matrix::StoredEmptyKind::Path
        || spreadsheetengine::core::matrix::storedFlagValue(
               spreadsheetengine::core::matrix::StoredEmptyKind::Result)
               != spreadsheetengine::core::matrix::kEmptyResultFlagValue
        || spreadsheetengine::core::matrix::storedFlagValue(
               spreadsheetengine::core::matrix::StoredEmptyKind::Path)
               != spreadsheetengine::core::matrix::kEmptyPathFlagValue
        || !spreadsheetengine::core::matrix::isStoredEmptyResult(
            spreadsheetengine::core::matrix::StoredElementType::Empty,
            spreadsheetengine::core::matrix::kEmptyResultFlagValue)
        || !spreadsheetengine::core::matrix::isStoredEmptyPath(
            spreadsheetengine::core::matrix::StoredElementType::Empty,
            spreadsheetengine::core::matrix::kEmptyPathFlagValue)
        || !spreadsheetengine::core::matrix::isStoredLogicalEmpty(
            spreadsheetengine::core::matrix::StoredElementType::Empty, 0)
        || spreadsheetengine::core::matrix::classifyStoredValueType(
               spreadsheetengine::core::matrix::StoredElementType::Empty,
               spreadsheetengine::core::matrix::StoredFlagType::Integer,
               spreadsheetengine::core::matrix::kEmptyPathFlagValue)
               != spreadsheetengine::api::MatrixValueType::EmptyPath
        || spreadsheetengine::core::matrix::classifyStoredValueType(
               spreadsheetengine::core::matrix::StoredElementType::String,
               spreadsheetengine::core::matrix::StoredFlagType::Unknown, 0)
               != spreadsheetengine::api::MatrixValueType::Text)
    {
        return fail("spreadsheetengine_matrix_tests", "stored empty classification mismatch");
    }

    if (spreadsheetengine::core::matrix::budgetWithReleasedCurrentElements(10, 3) != 13
        || spreadsheetengine::core::matrix::budgetAfterAllocation(13, { 4, 2 }) != 5
        || spreadsheetengine::core::matrix::budgetAfterConstruction(13, { 4, 2 }) != 5
        || spreadsheetengine::core::matrix::budgetAfterDestruction(5, { 4, 2 }) != 13
        || spreadsheetengine::core::matrix::budgetAfterResize(10, 3, { 4, 2 }) != 5)
    {
        return fail("spreadsheetengine_matrix_tests", "matrix resize budget mismatch");
    }

    const auto aResizePlan = spreadsheetengine::core::matrix::planResize(
        { 4, 2 }, 3, 10, spreadsheetengine::core::matrix::AllocationFallback::MatrixSize);
    if (aResizePlan.usesFallback() || aResizePlan.maStorageDimensions.mnColumns != 4
        || aResizePlan.maStorageDimensions.mnRows != 2)
    {
        return fail("spreadsheetengine_matrix_tests", "matrix resize plan mismatch");
    }

    MatrixCoordinate aRowVectorCoordinate { 2, 9 };
    if (!spreadsheetengine::api::normalizeReplicatedCoordinate({ 3, 1 }, aRowVectorCoordinate)
        || aRowVectorCoordinate.mnColumn != 2 || aRowVectorCoordinate.mnRow != 0)
    {
        return fail("spreadsheetengine_matrix_tests", "row vector replication mismatch");
    }

    MatrixCoordinate aColumnVectorCoordinate { 9, 2 };
    if (!spreadsheetengine::api::normalizeReplicatedCoordinate(
            { 1, 4 }, aColumnVectorCoordinate)
        || aColumnVectorCoordinate.mnColumn != 0 || aColumnVectorCoordinate.mnRow != 2)
    {
        return fail("spreadsheetengine_matrix_tests", "column vector replication mismatch");
    }

    MatrixCoordinate aNonVectorCoordinate { 3, 3 };
    if (spreadsheetengine::api::normalizeReplicatedCoordinate({ 2, 2 }, aNonVectorCoordinate))
        return fail("spreadsheetengine_matrix_tests", "2D replication should fail");

    MatrixCoordinate aReplicatedCoordinate { 9, 2 };
    if (!spreadsheetengine::core::matrix::isCoordinateValid({ 3, 4 }, 2, 3)
        || spreadsheetengine::core::matrix::isCoordinateValid({ 3, 4 }, 3, 3)
        || !spreadsheetengine::core::matrix::normalizeReplicatedCoordinateInPlace(
            MatrixDimensions { 1, 4 }, aReplicatedCoordinate.mnColumn, aReplicatedCoordinate.mnRow)
        || aReplicatedCoordinate.mnColumn != 0 || aReplicatedCoordinate.mnRow != 2)
    {
        return fail("spreadsheetengine_matrix_tests", "coordinate validation mismatch");
    }

    MatrixCoordinate aValidOrReplicated { 9, 2 };
    MatrixCoordinate aAlreadyValid { 2, 3 };
    if (!spreadsheetengine::core::matrix::isValidOrReplicatedCoordinate(
            MatrixDimensions { 1, 4 }, aValidOrReplicated.mnColumn, aValidOrReplicated.mnRow)
        || aValidOrReplicated.mnColumn != 0 || aValidOrReplicated.mnRow != 2
        || !spreadsheetengine::core::matrix::isValidOrReplicatedCoordinate(
            MatrixDimensions { 3, 4 }, aAlreadyValid.mnColumn, aAlreadyValid.mnRow)
        || aAlreadyValid.mnColumn != 2 || aAlreadyValid.mnRow != 3)
    {
        return fail("spreadsheetengine_matrix_tests", "valid-or-replicated coordinate mismatch");
    }

    const MatrixCoordinate aLinearCoordinate
        = spreadsheetengine::core::matrix::coordinateFromLinearIndex({ 3, 4 }, 6);
    if (aLinearCoordinate.mnColumn != 1 || aLinearCoordinate.mnRow != 2)
        return fail("spreadsheetengine_matrix_tests", "linear index mismatch");

    if (spreadsheetengine::core::matrix::columnMajorLinearIndex({ 3, 4 }, { 1, 2 }) != 6
        || spreadsheetengine::core::matrix::offsetColumnMajorLinearIndex({ 5, 7 }, { 1, 2 }, 3, 4)
               != 34)
    {
        return fail("spreadsheetengine_matrix_tests", "column-major index mismatch");
    }

    const MatrixCoordinate aTransposedCoordinate
        = spreadsheetengine::core::matrix::coordinateFromTransposedLinearIndex({ 3, 4 }, 6);
    if (aTransposedCoordinate.mnColumn != 0 || aTransposedCoordinate.mnRow != 2)
        return fail("spreadsheetengine_matrix_tests", "transposed index mismatch");

    if (!spreadsheetengine::core::matrix::canPlaceColumnVector({ 2, 5 }, { 1, 2 }, 3)
        || spreadsheetengine::core::matrix::canPlaceColumnVector({ 2, 5 }, { 1, 2 }, 4)
        || spreadsheetengine::core::matrix::canPlaceColumnVector({ 2, 5 }, { 3, 0 }, 1))
    {
        return fail("spreadsheetengine_matrix_tests", "column vector placement mismatch");
    }

    const auto aRange = spreadsheetengine::core::matrix::makeRange({ 0, 1 }, { 2, 3 });
    if (!spreadsheetengine::core::matrix::isValidRange({ 4, 5 }, aRange)
        || spreadsheetengine::core::matrix::columnCount(aRange) != 3
        || spreadsheetengine::core::matrix::rowCount(aRange) != 3)
    {
        return fail("spreadsheetengine_matrix_tests", "matrix range mismatch");
    }

    const auto aRangePlan = spreadsheetengine::core::matrix::planRangeWrite({ 4, 5 }, aRange);
    if (!aRangePlan.mbValid || aRangePlan.maRange.maEnd.mnColumn != 2
        || spreadsheetengine::core::matrix::planRangeWrite({ 2, 2 }, aRange).mbValid)
    {
        return fail("spreadsheetengine_matrix_tests", "matrix range planning mismatch");
    }

    const auto aVectorRange = spreadsheetengine::core::matrix::columnVectorRange({ 1, 2 }, 3);
    if (aVectorRange.maEnd.mnColumn != 1 || aVectorRange.maEnd.mnRow != 4)
        return fail("spreadsheetengine_matrix_tests", "column vector range mismatch");

    const auto aColumnWritePlan
        = spreadsheetengine::core::matrix::planColumnVectorWrite({ 2, 5 }, { 1, 2 }, 3);
    if (!aColumnWritePlan.mbValid || aColumnWritePlan.maRange.maEnd.mnRow != 4
        || spreadsheetengine::core::matrix::planColumnVectorWrite({ 2, 5 }, { 1, 2 }, 4).mbValid)
    {
        return fail("spreadsheetengine_matrix_tests", "column vector planning mismatch");
    }

    const auto aBroadcastRowPlan
        = spreadsheetengine::core::matrix::planBroadcastExecution({ 3, 1 }, { 3, 4 });
    if (!aBroadcastRowPlan.mbValid || !aBroadcastRowPlan.mbReplicated
        || aBroadcastRowPlan.mnRowRepeats != 4 || aBroadcastRowPlan.mnColumnRepeats != 1
        || aBroadcastRowPlan.maOperationRange.maEnd.mnColumn != 2
        || aBroadcastRowPlan.maOperationRange.maEnd.mnRow != 0)
    {
        return fail("spreadsheetengine_matrix_tests", "row broadcast planning mismatch");
    }

    const auto aBroadcastColumnPlan
        = spreadsheetengine::core::matrix::planBroadcastExecution({ 1, 4 }, { 3, 4 });
    if (!aBroadcastColumnPlan.mbValid || !aBroadcastColumnPlan.mbReplicated
        || aBroadcastColumnPlan.mnRowRepeats != 1 || aBroadcastColumnPlan.mnColumnRepeats != 3
        || aBroadcastColumnPlan.maOperationRange.maEnd.mnColumn != 0
        || aBroadcastColumnPlan.maOperationRange.maEnd.mnRow != 3)
    {
        return fail("spreadsheetengine_matrix_tests", "column broadcast planning mismatch");
    }

    const auto aPlainBroadcastPlan
        = spreadsheetengine::core::matrix::planBroadcastExecution({ 3, 4 }, { 3, 4 });
    if (!aPlainBroadcastPlan.mbValid || aPlainBroadcastPlan.mbReplicated
        || aPlainBroadcastPlan.mnRowRepeats != 1 || aPlainBroadcastPlan.mnColumnRepeats != 1
        || aPlainBroadcastPlan.maOperationRange.maEnd.mnColumn != 2
        || aPlainBroadcastPlan.maOperationRange.maEnd.mnRow != 3
        || spreadsheetengine::core::matrix::planBroadcastExecution({ 3, 4 }, { 0, 0 }).mbValid)
    {
        return fail("spreadsheetengine_matrix_tests", "plain broadcast planning mismatch");
    }

    const std::vector<bool> aValidityRun { true, true, true, false, true };
    const auto aValidRunPlan
        = spreadsheetengine::core::matrix::planContiguousValidRun(aValidityRun, 1);
    if (!aValidRunPlan.mbValid || aValidRunPlan.mnStartIndex != 1 || aValidRunPlan.mnLength != 2
        || spreadsheetengine::core::matrix::planContiguousValidRun(aValidityRun, 3).mbValid)
    {
        return fail("spreadsheetengine_matrix_tests", "valid run planning mismatch");
    }

    const auto aLoopSeedCoordinate
        = spreadsheetengine::core::matrix::advanceColumnMajorLoopSeedCoordinate({ 0, 0 }, 4, 2);
    const auto aWrappedLoopSeedCoordinate
        = spreadsheetengine::core::matrix::advanceColumnMajorLoopSeedCoordinate({ 0, 3 }, 4, 2);
    if (aLoopSeedCoordinate.mnColumn != 0 || aLoopSeedCoordinate.mnRow != 2
        || aWrappedLoopSeedCoordinate.mnColumn != 1 || aWrappedLoopSeedCoordinate.mnRow != 1
        || spreadsheetengine::core::matrix::advanceColumnMajorLoopSeedCoordinate({ 2, 1 }, 0, 5)
               .mnColumn
               != 2)
    {
        return fail("spreadsheetengine_matrix_tests", "loop seed coordinate mismatch");
    }

    MatrixCoordinate aJumpCoordinate { 9, 2 };
    if (!spreadsheetengine::core::jumpmatrix::normalizeJumpCoordinate({ 1, 4 }, aJumpCoordinate)
        || aJumpCoordinate.mnColumn != 0 || aJumpCoordinate.mnRow != 2)
    {
        return fail("spreadsheetengine_matrix_tests", "jump coordinate normalization mismatch");
    }

    if (spreadsheetengine::core::jumpmatrix::jumpEntryIndex({ 3, 4 }, { 2, 1 }) != 9)
        return fail("spreadsheetengine_matrix_tests", "jump entry index mismatch");

    spreadsheetengine::core::jumpmatrix::ResultCursor aCursor {};
    if (!spreadsheetengine::core::jumpmatrix::advanceResultCursor({ 2, 2 }, aCursor)
        || aCursor.maCoordinate.mnColumn != 0 || aCursor.maCoordinate.mnRow != 0)
    {
        return fail("spreadsheetengine_matrix_tests", "jump cursor first step mismatch");
    }
    if (!spreadsheetengine::core::jumpmatrix::advanceResultCursor({ 2, 2 }, aCursor)
        || aCursor.maCoordinate.mnColumn != 0 || aCursor.maCoordinate.mnRow != 1)
    {
        return fail("spreadsheetengine_matrix_tests", "jump cursor second step mismatch");
    }
    if (!spreadsheetengine::core::jumpmatrix::advanceResultCursor({ 2, 2 }, aCursor)
        || aCursor.maCoordinate.mnColumn != 1 || aCursor.maCoordinate.mnRow != 0)
    {
        return fail("spreadsheetengine_matrix_tests", "jump cursor wrap mismatch");
    }

    const auto aExpanded = spreadsheetengine::core::jumpmatrix::expandResultDimensions(
        { 2, 2 }, { 4, 1 });
    if (aExpanded.mnColumns != 4 || aExpanded.mnRows != 2)
        return fail("spreadsheetengine_matrix_tests", "jump expansion mismatch");

    const auto aAdjusted = spreadsheetengine::core::jumpmatrix::adjustCursorAfterExpansion(
        { 3, 1 }, { 2, 0 }, { 4, 5 });
    if (aAdjusted.mnColumn != 0 || aAdjusted.mnRow != 4)
        return fail("spreadsheetengine_matrix_tests", "jump cursor adjustment mismatch");

    if (!spreadsheetengine::core::jumpmatrix::shouldBufferResultWrites({ 2, 128 }, 128)
        || spreadsheetengine::core::jumpmatrix::shouldBufferResultWrites({ 2, 127 }, 128))
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffering threshold mismatch");
    }

    if (!spreadsheetengine::core::jumpmatrix::isBufferedWriteContinuation(
            { { 1, 3 }, 2 }, { 1, 5 })
        || spreadsheetengine::core::jumpmatrix::isBufferedWriteContinuation(
            { { 1, 3 }, 2 }, { 2, 5 })
        || spreadsheetengine::core::jumpmatrix::isBufferedWriteContinuation(
            { { 1, 3 }, 0 }, { 1, 3 }))
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffer continuation mismatch");
    }

    const auto aExpansionPlan = spreadsheetengine::core::jumpmatrix::planResultExpansion(
        { 3, 1 }, { 2, 2 }, { 4, 5 }, { 2, 0 });
    if (!aExpansionPlan.mbNeedsExpansion || !aExpansionPlan.mbFillNewColumns
        || !aExpansionPlan.mbFillNewRows
        || aExpansionPlan.maExpandedDimensions.mnColumns != 4
        || aExpansionPlan.maExpandedDimensions.mnRows != 5
        || aExpansionPlan.maAdjustedCursor.mnColumn != 0
        || aExpansionPlan.maAdjustedCursor.mnRow != 4
        || aExpansionPlan.maNewColumnRange.maStart.mnColumn != 2
        || aExpansionPlan.maNewRowRange.maStart.mnRow != 2)
    {
        return fail("spreadsheetengine_matrix_tests", "jump expansion plan mismatch");
    }

    const MatrixValue aLeft { 42.0, u"alpha", MatrixValueType::Text };
    const MatrixValue aRight { 42.0, u"alpha", MatrixValueType::Text };
    const MatrixValue aDifferent { 42.0, u"beta", MatrixValueType::Text };
    if (aLeft != aRight || aLeft == aDifferent)
        return fail("spreadsheetengine_matrix_tests", "matrix value equality mismatch");

    if (spreadsheetengine::core::matrix::cloneDimensions({ 3, 2 }).mnColumns != 3
        || spreadsheetengine::core::matrix::extendedCloneDimensions({ 3, 2 }, { 5, 1 }).mnColumns != 5
        || spreadsheetengine::core::matrix::extendedCloneDimensions({ 3, 2 }, { 1, 5 }).mnRows != 5)
    {
        return fail("spreadsheetengine_matrix_tests", "matrix clone planning mismatch");
    }

    if (!spreadsheetengine::core::matrix::canCopyIntoDestination({ 3, 2 }, { 3, 2 })
        || !spreadsheetengine::core::matrix::canCopyIntoDestination({ 3, 2 }, { 4, 5 })
        || spreadsheetengine::core::matrix::canCopyIntoDestination({ 4, 5 }, { 3, 2 }))
    {
        return fail("spreadsheetengine_matrix_tests", "matrix copy planning mismatch");
    }

    const auto aOpenedBuffer = spreadsheetengine::core::jumpmatrix::openBufferWindowIfEmpty(
        spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 4, 7 }, 0), { 1, 3 });
    if (aOpenedBuffer.maStart.mnColumn != 1 || aOpenedBuffer.maStart.mnRow != 3
        || spreadsheetengine::core::jumpmatrix::openBufferWindowIfEmpty(
               spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 4, 7 }, 2), { 1, 3 })
               .maStart.mnColumn
               != 4)
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffer window opening mismatch");
    }

    const auto aStateWindow
        = spreadsheetengine::core::jumpmatrix::bufferWindowFromState(4u, 7u, 3u);
    MatrixCoordinate aAppliedWindowStart {};
    spreadsheetengine::core::jumpmatrix::applyBufferWindowStart(aStateWindow,
                                                                aAppliedWindowStart.mnColumn,
                                                                aAppliedWindowStart.mnRow);
    if (aStateWindow.maStart.mnColumn != 4 || aStateWindow.maStart.mnRow != 7
        || aStateWindow.mnCount != 3 || aAppliedWindowStart.mnColumn != 4
        || aAppliedWindowStart.mnRow != 7)
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffer state mismatch");
    }

    const auto aNextBuffer = spreadsheetengine::core::jumpmatrix::nextBufferedWriteWindow(
        spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2), { 1, 5 });
    if (aNextBuffer.maStart.mnColumn != 1 || aNextBuffer.maStart.mnRow != 3
        || aNextBuffer.mnCount != 3)
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffer window progression mismatch");
    }

    if (!spreadsheetengine::core::jumpmatrix::shouldFlushBufferedWindow(
            spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2), false, { 1, 5 })
        || spreadsheetengine::core::jumpmatrix::shouldFlushBufferedWindow(
            spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2), true, { 1, 5 })
        || !spreadsheetengine::core::jumpmatrix::shouldFlushBufferedWindow(
            spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2), true, { 2, 5 }))
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffer flush decision mismatch");
    }

    int nFlushCount = 0;
    int nResetCount = 0;
    if (!spreadsheetengine::core::jumpmatrix::flushBufferedWindowIfNeeded(
            spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2), false, { 1, 5 },
            [&nFlushCount]() { ++nFlushCount; }, [&nResetCount]() { ++nResetCount; })
        || nFlushCount != 1 || nResetCount != 1
        || spreadsheetengine::core::jumpmatrix::flushBufferedWindowIfNeeded(
            spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2), true, { 1, 5 },
            [&nFlushCount]() { ++nFlushCount; }, [&nResetCount]() { ++nResetCount; })
        || nFlushCount != 1 || nResetCount != 1)
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffer flush helper mismatch");
    }

    const auto aBufferedWritePlan = spreadsheetengine::core::jumpmatrix::planBufferedResultWrite(
        { 2, 128 }, 128, spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2),
        { 1, 5 });
    if (!aBufferedWritePlan.mbBufferWrite || aBufferedWritePlan.mbFlushCurrentType
        || aBufferedWritePlan.maWindow.maStart.mnColumn != 1
        || aBufferedWritePlan.maWindow.maStart.mnRow != 3
        || aBufferedWritePlan.maWindow.mnCount != 3)
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffered write plan mismatch");
    }

    const auto aFlushedWritePlan = spreadsheetengine::core::jumpmatrix::planBufferedResultWrite(
        { 2, 128 }, 128, spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2),
        { 2, 5 });
    if (!aFlushedWritePlan.mbBufferWrite || !aFlushedWritePlan.mbFlushCurrentType
        || aFlushedWritePlan.maWindow.maStart.mnColumn != 2
        || aFlushedWritePlan.maWindow.maStart.mnRow != 5
        || aFlushedWritePlan.maWindow.mnCount != 1)
    {
        return fail("spreadsheetengine_matrix_tests", "jump buffered write flush plan mismatch");
    }

    if (spreadsheetengine::core::jumpmatrix::planBufferedResultWrite(
            { 2, 127 }, 128, spreadsheetengine::core::jumpmatrix::makeBufferWindow({ 1, 3 }, 2),
            { 1, 5 })
            .mbBufferWrite)
    {
        return fail("spreadsheetengine_matrix_tests", "jump direct-write plan mismatch");
    }

    std::cout << "spreadsheetengine matrix api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
