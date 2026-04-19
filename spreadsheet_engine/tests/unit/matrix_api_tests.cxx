/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cmath>
#include <iostream>
#include <vector>

#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/detail/JumpMatrixRuntime.hxx>
#include <spreadsheetengine/detail/MatrixGeometry.hxx>
#include <spreadsheetengine/detail/MatrixRuntime.hxx>
#include <spreadsheetengine/runtime/MathMatrix.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>

#include "TestSupport.hxx"

namespace
{

constexpr double kMatrixParityEpsilon = 1.0e-10;

[[nodiscard]] bool approxEqual(double fLeft, double fRight)
{
    return std::fabs(fLeft - fRight) <= kMatrixParityEpsilon;
}

} // namespace

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

    // MMULT / MINVERSE numerical-core parity fixtures.
    {
        using spreadsheetengine::core::math::evaluateMatrixInverse;
        using spreadsheetengine::core::math::evaluateMatrixMultiply;

        // Identity times identity stays identity.
        const std::vector<double> aId3 {
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0
        };
        const auto aIdProduct = evaluateMatrixMultiply(aId3, 3, 3, aId3, 3);
        if (!aIdProduct || aIdProduct.maValue.size() != 9
            || !approxEqual(aIdProduct.maValue[0], 1.0)
            || !approxEqual(aIdProduct.maValue[4], 1.0)
            || !approxEqual(aIdProduct.maValue[8], 1.0)
            || !approxEqual(aIdProduct.maValue[1], 0.0))
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixMultiply identity parity mismatch");
        }

        // 2x2 product: {{1,2},{3,4}} * {{5,6},{7,8}} = {{19,22},{43,50}}.
        const std::vector<double> aA22 { 1.0, 2.0, 3.0, 4.0 };
        const std::vector<double> aB22 { 5.0, 6.0, 7.0, 8.0 };
        const auto a22Product = evaluateMatrixMultiply(aA22, 2, 2, aB22, 2);
        if (!a22Product || a22Product.maValue.size() != 4
            || !approxEqual(a22Product.maValue[0], 19.0)
            || !approxEqual(a22Product.maValue[1], 22.0)
            || !approxEqual(a22Product.maValue[2], 43.0)
            || !approxEqual(a22Product.maValue[3], 50.0))
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixMultiply 2x2 parity mismatch");
        }

        // Non-square: 2x3 * 3x2 product
        //   {{1,2,3},{4,5,6}} * {{7,8},{9,10},{11,12}}
        //   = {{58,64},{139,154}}.
        const std::vector<double> aA23 { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
        const std::vector<double> aB32 { 7.0, 8.0, 9.0, 10.0, 11.0, 12.0 };
        const auto aNonSquareProduct = evaluateMatrixMultiply(aA23, 2, 3, aB32, 2);
        if (!aNonSquareProduct || aNonSquareProduct.maValue.size() != 4
            || !approxEqual(aNonSquareProduct.maValue[0], 58.0)
            || !approxEqual(aNonSquareProduct.maValue[1], 64.0)
            || !approxEqual(aNonSquareProduct.maValue[2], 139.0)
            || !approxEqual(aNonSquareProduct.maValue[3], 154.0))
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixMultiply non-square parity mismatch");
        }

        // Dimension mismatch: (2x3) * (2x3) has incompatible inner
        // dimensions.
        const auto aBadProduct = evaluateMatrixMultiply(aA23, 2, 3, aA23, 3);
        if (aBadProduct
            || aBadProduct.meError != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixMultiply dimension mismatch should fail");
        }

        // MINVERSE of identity is identity.
        const auto aIdInverse = evaluateMatrixInverse(aId3, 3);
        if (!aIdInverse || aIdInverse.maValue.size() != 9
            || !approxEqual(aIdInverse.maValue[0], 1.0)
            || !approxEqual(aIdInverse.maValue[4], 1.0)
            || !approxEqual(aIdInverse.maValue[8], 1.0)
            || !approxEqual(aIdInverse.maValue[1], 0.0))
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixInverse identity parity mismatch");
        }

        // MINVERSE 2x2: {{4,7},{2,6}} -> {{0.6,-0.7},{-0.2,0.4}}.
        const std::vector<double> aInv22 { 4.0, 7.0, 2.0, 6.0 };
        const auto aInv22Result = evaluateMatrixInverse(aInv22, 2);
        if (!aInv22Result || aInv22Result.maValue.size() != 4
            || !approxEqual(aInv22Result.maValue[0], 0.6)
            || !approxEqual(aInv22Result.maValue[1], -0.7)
            || !approxEqual(aInv22Result.maValue[2], -0.2)
            || !approxEqual(aInv22Result.maValue[3], 0.4))
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixInverse 2x2 parity mismatch");
        }

        // MINVERSE 3x3 self-consistency: inverse(A) * A = I.
        const std::vector<double> aInv33 {
            2.0, 1.0, 3.0,
            1.0, 3.0, 2.0,
            3.0, 2.0, 1.0
        };
        const auto aInv33Result = evaluateMatrixInverse(aInv33, 3);
        if (!aInv33Result)
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixInverse 3x3 parity mismatch");
        }
        const auto aRoundTrip = evaluateMatrixMultiply(
            aInv33Result.maValue, 3, 3, aInv33, 3);
        if (!aRoundTrip
            || !approxEqual(aRoundTrip.maValue[0], 1.0)
            || !approxEqual(aRoundTrip.maValue[4], 1.0)
            || !approxEqual(aRoundTrip.maValue[8], 1.0)
            || !approxEqual(aRoundTrip.maValue[1], 0.0)
            || !approxEqual(aRoundTrip.maValue[2], 0.0)
            || !approxEqual(aRoundTrip.maValue[3], 0.0))
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixInverse 3x3 round-trip mismatch");
        }

        // Singular matrix: zero row surfaces as IllegalArgument.
        const std::vector<double> aSingular {
            1.0, 2.0, 3.0,
            2.0, 4.0, 6.0,
            0.0, 0.0, 0.0
        };
        const auto aSingularResult = evaluateMatrixInverse(aSingular, 3);
        if (aSingularResult
            || aSingularResult.meError != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "evaluateMatrixInverse singular detection mismatch");
        }
    }

    // planMatrixMultiply / planMatrixInverse RPN planner contract.
    {
        using spreadsheetengine::core::rpn::MatrixOperand;
        using spreadsheetengine::core::rpn::MatrixProvenance;
        using spreadsheetengine::core::rpn::planMatrixInverse;
        using spreadsheetengine::core::rpn::planMatrixMultiply;

        MatrixOperand aLeft;
        aLeft.maDimensions = { 2, 2 };
        aLeft.maValues
            = { spreadsheetengine::api::CellValue::number(1.0),
                spreadsheetengine::api::CellValue::number(2.0),
                spreadsheetengine::api::CellValue::number(3.0),
                spreadsheetengine::api::CellValue::number(4.0) };
        aLeft.meProvenance = MatrixProvenance::InlineLiteral;

        MatrixOperand aRight;
        aRight.maDimensions = { 2, 2 };
        aRight.maValues
            = { spreadsheetengine::api::CellValue::number(5.0),
                spreadsheetengine::api::CellValue::number(6.0),
                spreadsheetengine::api::CellValue::number(7.0),
                spreadsheetengine::api::CellValue::number(8.0) };
        aRight.meProvenance = MatrixProvenance::InlineLiteral;

        const auto aProduct = planMatrixMultiply(aLeft, aRight);
        if (!aProduct || aProduct.maValue.maDimensions.mnColumns != 2
            || aProduct.maValue.maDimensions.mnRows != 2
            || !approxEqual(aProduct.maValue.maValues[0].mfNumber, 19.0)
            || !approxEqual(aProduct.maValue.maValues[3].mfNumber, 50.0))
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "planMatrixMultiply contract mismatch");
        }

        // Mismatched inner dimensions decline.
        MatrixOperand aIncompat;
        aIncompat.maDimensions = { 2, 3 };
        aIncompat.maValues.assign(6, spreadsheetengine::api::CellValue::number(1.0));
        const auto aBad = planMatrixMultiply(aLeft, aIncompat);
        if (aBad
            || aBad.meError != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "planMatrixMultiply dimension fence mismatch");
        }

        MatrixOperand aSquare;
        aSquare.maDimensions = { 2, 2 };
        aSquare.maValues
            = { spreadsheetengine::api::CellValue::number(4.0),
                spreadsheetengine::api::CellValue::number(7.0),
                spreadsheetengine::api::CellValue::number(2.0),
                spreadsheetengine::api::CellValue::number(6.0) };
        aSquare.meProvenance = MatrixProvenance::InlineLiteral;
        const auto aInverse = planMatrixInverse(aSquare);
        if (!aInverse
            || !approxEqual(aInverse.maValue.maValues[0].mfNumber, 0.6)
            || !approxEqual(aInverse.maValue.maValues[1].mfNumber, -0.7)
            || !approxEqual(aInverse.maValue.maValues[2].mfNumber, -0.2)
            || !approxEqual(aInverse.maValue.maValues[3].mfNumber, 0.4))
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "planMatrixInverse contract mismatch");
        }

        // Non-square input declines via IllegalArgument.
        MatrixOperand aNonSquare;
        aNonSquare.maDimensions = { 3, 2 };
        aNonSquare.maValues.assign(6, spreadsheetengine::api::CellValue::number(1.0));
        const auto aBadInverse = planMatrixInverse(aNonSquare);
        if (aBadInverse
            || aBadInverse.meError != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "planMatrixInverse shape fence mismatch");
        }

        // Singular input surfaces as IllegalArgument (same error as
        // Calc's PushIllegalArgument).
        MatrixOperand aSingular;
        aSingular.maDimensions = { 3, 3 };
        aSingular.maValues = {
            spreadsheetengine::api::CellValue::number(1.0),
            spreadsheetengine::api::CellValue::number(2.0),
            spreadsheetengine::api::CellValue::number(3.0),
            spreadsheetengine::api::CellValue::number(2.0),
            spreadsheetengine::api::CellValue::number(4.0),
            spreadsheetengine::api::CellValue::number(6.0),
            spreadsheetengine::api::CellValue::number(3.0),
            spreadsheetengine::api::CellValue::number(6.0),
            spreadsheetengine::api::CellValue::number(9.0)
        };
        const auto aSingularInverse = planMatrixInverse(aSingular);
        if (aSingularInverse
            || aSingularInverse.meError != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_matrix_tests",
                "planMatrixInverse singular should fail");
        }
    }

    std::cout << "spreadsheetengine matrix api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
