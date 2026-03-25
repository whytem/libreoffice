/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/core/FormulaCellReferenceUpdate.hxx>
#include <spreadsheetengine/core/FormulaCellState.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::core::formulacell::CalcAfterLoadPlan;
    using spreadsheetengine::core::formulacell::DirtyPlan;
    using spreadsheetengine::core::formulacell::LoadTrackingPlan;
    using spreadsheetengine::core::formulacell::NotifyKind;
    using spreadsheetengine::core::formulacell::NotifyPlan;
    using spreadsheetengine::core::formulacell::ParallelCalculationPlan;
    using spreadsheetengine::core::formulacell::TableOpDirtyPlan;
    using spreadsheetengine::core::formulacell::VolatileKind;
    using spreadsheetengine::core::formulacellrefupdate::CopyUpdatePlan;
    using spreadsheetengine::core::formulacellrefupdate::InsertDeleteTabUpdatePlan;
    using spreadsheetengine::core::formulacellrefupdate::MoveUpdatePlan;
    using spreadsheetengine::core::formulacellrefupdate::MoveTabUpdatePlan;
    using spreadsheetengine::core::formulacellrefupdate::GrowFinishPlan;
    using spreadsheetengine::core::formulacellrefupdate::ShiftUpdatePlan;
    using spreadsheetengine::core::formulacellrefupdate::Transpose3DFlagPlan;
    using spreadsheetengine::core::formulacellrefupdate::TransposeFinishPlan;
    using spreadsheetengine::core::formulacellrefupdate::TransposePositionPlan;
    using spreadsheetengine::standalone::test::fail;

    if (spreadsheetengine::core::formulacell::makeSetDirtyPlan(
            true, false, false, false, false, true, false, false)
            != DirtyPlan { true, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeSetDirtyPlan(
               false, true, false, false, false, true, false, false)
               != DirtyPlan { false, true, false, false, true }
        || spreadsheetengine::core::formulacell::makeSetDirtyPlan(
               false, false, false, false, true, true, false, false)
               != DirtyPlan { false, true, true, true, true }
        || spreadsheetengine::core::formulacell::makeSetDirtyPlan(
               false, false, true, false, true, true, false, false)
               != DirtyPlan { false, false, false, false, true }
        || spreadsheetengine::core::formulacell::makeSetDirtyPlan(
               false, false, true, true, true, false, false, false)
               != DirtyPlan { false, false, true, true, true })
    {
        return fail("spreadsheetengine_formulacell_tests", "set dirty plan mismatch");
    }

    if (!spreadsheetengine::core::formulacell::shouldResetGroupCalcState(true, true)
        || spreadsheetengine::core::formulacell::shouldResetGroupCalcState(true, false)
        || spreadsheetengine::core::formulacell::shouldResetGroupCalcState(false, true)
        || !spreadsheetengine::core::formulacell::shouldMarkDirtyForRecalcMode(false)
        || spreadsheetengine::core::formulacell::shouldMarkDirtyForRecalcMode(true))
    {
        return fail("spreadsheetengine_formulacell_tests", "group or recalc policy mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeLoadTrackingPlan(false, false, false)
            != LoadTrackingPlan { true, true, false }
        || spreadsheetengine::core::formulacell::makeLoadTrackingPlan(true, true, false)
               != LoadTrackingPlan { true, true, false }
        || spreadsheetengine::core::formulacell::makeLoadTrackingPlan(true, false, true)
               != LoadTrackingPlan { false, false, true }
        || spreadsheetengine::core::formulacell::makeLoadTrackingPlan(true, false, false)
               != LoadTrackingPlan { false, false, false })
    {
        return fail("spreadsheetengine_formulacell_tests", "load tracking plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeCalcAfterLoadPlan(
            false, false, true, true, false)
            != CalcAfterLoadPlan { true, false }
        || spreadsheetengine::core::formulacell::makeCalcAfterLoadPlan(
               true, false, true, false, false)
               != CalcAfterLoadPlan { false, false }
        || spreadsheetengine::core::formulacell::makeCalcAfterLoadPlan(
               true, true, true, false, false)
               != CalcAfterLoadPlan { true, true }
        || spreadsheetengine::core::formulacell::makeCalcAfterLoadPlan(
               false, true, false, true, true)
               != CalcAfterLoadPlan { false, true })
    {
        return fail("spreadsheetengine_formulacell_tests", "calc-after-load plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeNotifyPlan(
            true, NotifyKind::DataChanged, false, false, false, false, false, false)
            != NotifyPlan { true, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeNotifyPlan(
               false, NotifyKind::Other, false, false, false, false, false, false)
               != NotifyPlan { true, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeNotifyPlan(
               false, NotifyKind::HiddenRowsChanged, false, false, false, false, false, false)
               != NotifyPlan { true, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeNotifyPlan(
               false, NotifyKind::HiddenRowsChanged, true, false, true, true, false, false)
               != NotifyPlan { false, true, false, false, false }
        || spreadsheetengine::core::formulacell::makeNotifyPlan(
               false, NotifyKind::DataChanged, false, false, false, true, false, false)
               != NotifyPlan { false, true, false, false, true }
        || spreadsheetengine::core::formulacell::makeNotifyPlan(
               false, NotifyKind::TableOpDirty, false, false, false, true, false, false)
               != NotifyPlan { false, false, true, true, true }
        || spreadsheetengine::core::formulacell::makeNotifyPlan(
               false, NotifyKind::TableOpDirty, false, true, false, true, false, false)
               != NotifyPlan { false, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeNotifyPlan(
               false, NotifyKind::DataChanged, false, true, true, false, true, true)
               != NotifyPlan { false, true, false, false, false })
    {
        return fail("spreadsheetengine_formulacell_tests", "notify plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeParallelCalculationPlan(
            false, false, VolatileKind::Other)
            != ParallelCalculationPlan { true, false, false, false, false, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeParallelCalculationPlan(
               true, false, VolatileKind::VolatileMacro)
               != ParallelCalculationPlan { false, true, true, false, true, false, true, false, false }
        || spreadsheetengine::core::formulacell::makeParallelCalculationPlan(
               true, true, VolatileKind::NotVolatile)
               != ParallelCalculationPlan { false, false, false, true, false, true, false, true, false }
        || spreadsheetengine::core::formulacell::makeParallelCalculationPlan(
               true, false, VolatileKind::NotVolatile)
               != ParallelCalculationPlan { false, true, false, false, false, true, false, false, true }
        || spreadsheetengine::core::formulacell::makeParallelCalculationPlan(
               true, true, VolatileKind::Other)
               != ParallelCalculationPlan { false, false, false, false, false, false, false, false, false })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "parallel calculation plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeSetTableOpDirtyPlan(
            true, false, false, false)
            != TableOpDirtyPlan { false, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeSetTableOpDirtyPlan(
               false, true, false, false)
               != TableOpDirtyPlan { false, true, false, false, false }
        || spreadsheetengine::core::formulacell::makeSetTableOpDirtyPlan(
               false, false, false, true)
               != TableOpDirtyPlan { false, true, true, true, true }
        || spreadsheetengine::core::formulacell::makeSetTableOpDirtyPlan(
               false, false, true, false)
               != TableOpDirtyPlan { false, true, false, true, true }
        || spreadsheetengine::core::formulacell::makeSetTableOpDirtyPlan(
               false, false, true, true)
               != TableOpDirtyPlan { false, false, false, false, false })
    {
        return fail("spreadsheetengine_formulacell_tests", "table op dirty plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::computePreviousPosition(
            { 5, 11, 19 }, false, 2, 3, 1)
            != CellAddress { 5, 11, 19 }
        || spreadsheetengine::core::formulacellrefupdate::computePreviousPosition(
               { 5, 11, 19 }, true, 2, 3, 1)
               != CellAddress { 4, 9, 16 })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "copy previous-position plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::makeCopyUpdatePlan(
            { 5, 11, 19 }, false, 2, 3, 1, false, false, false, false)
            != CopyUpdatePlan { { 5, 11, 19 }, false, false, false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeCopyUpdatePlan(
               { 5, 11, 19 }, true, 2, 3, 1, false, false, true, false)
               != CopyUpdatePlan { { 4, 9, 16 }, true, true, true, false }
        || spreadsheetengine::core::formulacellrefupdate::makeCopyUpdatePlan(
               { 5, 11, 19 }, false, 2, 3, 1, true, false, false, false)
               != CopyUpdatePlan { { 5, 11, 19 }, true, false, false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeCopyUpdatePlan(
               { 5, 11, 19 }, false, 2, 3, 1, false, true, false, true)
               != CopyUpdatePlan { { 5, 11, 19 }, true, false, true, true })
    {
        return fail("spreadsheetengine_formulacell_tests", "copy update plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::makeMoveUpdatePlan(
            { 5, 11, 19 }, false, 2, 3, 1, false, false, false, false, false, false, false,
            false, false, false)
            != MoveUpdatePlan { { 5, 11, 19 }, false, false, false, false, false, false,
                false }
        || spreadsheetengine::core::formulacellrefupdate::makeMoveUpdatePlan(
               { 5, 11, 19 }, true, 2, 3, 1, true, false, true, false, true, false, false,
               false, false, false)
               != MoveUpdatePlan { { 4, 9, 16 }, true, true, false, true, false, true,
                   true }
        || spreadsheetengine::core::formulacellrefupdate::makeMoveUpdatePlan(
               { 5, 11, 19 }, false, 2, 3, 1, true, false, false, false, false, true, false,
               false, false, false)
               != MoveUpdatePlan { { 5, 11, 19 }, true, false, false, true, true, true,
                   true }
        || spreadsheetengine::core::formulacellrefupdate::makeMoveUpdatePlan(
               { 5, 11, 19 }, true, 2, 3, 1, true, true, false, true, false, false, true,
               true, true, true)
               != MoveUpdatePlan { { 4, 9, 16 }, true, true, false, true, true, false,
                   false })
    {
        return fail("spreadsheetengine_formulacell_tests", "move update plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::makeShiftUpdatePlan(
            { 5, 11, 19 }, { 5, 11, 19 }, true, false, false, false, false, false, false,
            false, false, false)
            != ShiftUpdatePlan { false, true, false, false, false, false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeShiftUpdatePlan(
               { 5, 11, 19 }, { 5, 11, 20 }, false, true, false, true, false, true, false,
               false, false, false)
               != ShiftUpdatePlan { true, true, true, true, false, true, true }
        || spreadsheetengine::core::formulacellrefupdate::makeShiftUpdatePlan(
               { 5, 11, 19 }, { 5, 11, 19 }, false, true, true, false, true, false, false,
               false, false, true)
               != ShiftUpdatePlan { true, true, true, true, false, true, false }
        || spreadsheetengine::core::formulacellrefupdate::makeShiftUpdatePlan(
               { 5, 11, 19 }, { 5, 14, 19 }, true, true, false, true, false, false, true,
               true, true, false)
               != ShiftUpdatePlan { true, true, true, true, true, true, true })
    {
        return fail("spreadsheetengine_formulacell_tests", "shift update plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::makeInsertTabUpdatePlan(
            5, 3, 2, true, true, true)
            != InsertDeleteTabUpdatePlan { true, false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeInsertTabUpdatePlan(
               5, 7, 2, false, false, true)
               != InsertDeleteTabUpdatePlan { false, false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeInsertTabUpdatePlan(
               5, 5, 2, false, true, false)
               != InsertDeleteTabUpdatePlan { true, true, false }
        || spreadsheetengine::core::formulacellrefupdate::makeInsertTabUpdatePlan(
               5, 3, 2, false, true, true)
               != InsertDeleteTabUpdatePlan { true, true, true })
    {
        return fail("spreadsheetengine_formulacell_tests", "insert-tab update plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::makeDeleteTabUpdatePlan(
            5, 3, 2, true, true, true)
            != InsertDeleteTabUpdatePlan { true, false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeDeleteTabUpdatePlan(
               4, 5, 2, false, false, true)
               != InsertDeleteTabUpdatePlan { false, false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeDeleteTabUpdatePlan(
               8, 5, 2, false, true, false)
               != InsertDeleteTabUpdatePlan { true, true, false }
        || spreadsheetengine::core::formulacellrefupdate::makeDeleteTabUpdatePlan(
               8, 5, 2, false, true, true)
               != InsertDeleteTabUpdatePlan { true, true, true })
    {
        return fail("spreadsheetengine_formulacell_tests", "delete-tab update plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::makeMoveTabUpdatePlan(
            true, true, true)
            != MoveTabUpdatePlan { false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeMoveTabUpdatePlan(
               false, false, true)
               != MoveTabUpdatePlan { false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeMoveTabUpdatePlan(
               false, true, false)
               != MoveTabUpdatePlan { true, false }
        || spreadsheetengine::core::formulacellrefupdate::makeMoveTabUpdatePlan(
               false, true, true)
               != MoveTabUpdatePlan { true, true }
        || !spreadsheetengine::core::formulacellrefupdate::shouldCompileAfterTabAdjust(true)
        || spreadsheetengine::core::formulacellrefupdate::shouldCompileAfterTabAdjust(false))
    {
        return fail("spreadsheetengine_formulacell_tests", "move-tab update plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::makeTransposePositionPlan(
            { 2, 11, 21 }, { { 0, 2, 3 }, { 0, 4, 5 } }, { 2, 10, 20 }, 5)
            != TransposePositionPlan { { { 2, 10, 20 }, { 2, 12, 22 } }, { 0, 3, 4 }, true }
        || spreadsheetengine::core::formulacellrefupdate::makeTransposePositionPlan(
               { 0, 0, 0 }, { { 0, 2, 3 }, { 0, 4, 5 } }, { 2, 10, 20 }, 5)
               != TransposePositionPlan { { { 2, 10, 20 }, { 2, 12, 22 } }, { 0, 0, 0 }, false }
        || spreadsheetengine::core::formulacellrefupdate::makeTranspose3DFlagPlan(
               { { 2, 10, 20 }, { 4, 12, 22 } }, 0, 2, false, true, true)
               != Transpose3DFlagPlan { true, true }
        || spreadsheetengine::core::formulacellrefupdate::makeTranspose3DFlagPlan(
               { { 2, 10, 20 }, { 2, 12, 22 } }, 2, 2, true, true, true)
               != Transpose3DFlagPlan { false, false }
        || spreadsheetengine::core::formulacellrefupdate::makeTransposeFinishPlan(false, false)
               != TransposeFinishPlan { false, false, false, true }
        || spreadsheetengine::core::formulacellrefupdate::makeTransposeFinishPlan(true, true)
               != TransposeFinishPlan { true, true, true, false })
    {
        return fail("spreadsheetengine_formulacell_tests", "transpose update plan mismatch");
    }

    if (spreadsheetengine::core::formulacellrefupdate::makeGrowFinishPlan(false)
            != GrowFinishPlan { false, false, true }
        || spreadsheetengine::core::formulacellrefupdate::makeGrowFinishPlan(true)
               != GrowFinishPlan { true, true, false })
    {
        return fail("spreadsheetengine_formulacell_tests", "grow update plan mismatch");
    }

    std::cout << "spreadsheetengine formulacell api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
