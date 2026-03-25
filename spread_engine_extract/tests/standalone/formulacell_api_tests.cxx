/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/core/FormulaCellState.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::core::formulacell::CalcAfterLoadPlan;
    using spreadsheetengine::core::formulacell::DirtyPlan;
    using spreadsheetengine::core::formulacell::LoadTrackingPlan;
    using spreadsheetengine::core::formulacell::TableOpDirtyPlan;
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

    std::cout << "spreadsheetengine formulacell api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
