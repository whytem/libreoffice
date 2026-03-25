/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/core/FormulaCellReferenceUpdate.hxx>
#include <spreadsheetengine/core/FormulaCellState.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::core::formulacell::CalcAfterLoadPlan;
    using spreadsheetengine::core::formulacell::DependencyCheckFailure;
    using spreadsheetengine::core::formulacell::DependencyCheckPlan;
    using spreadsheetengine::core::formulacell::DirtyPlan;
    using spreadsheetengine::core::formulacell::GroupBackendFailure;
    using spreadsheetengine::core::formulacell::GroupInterpretFailure;
    using spreadsheetengine::core::formulacell::GroupBackendDependencyPlan;
    using spreadsheetengine::core::formulacell::GroupBackendPreflightPlan;
    using spreadsheetengine::core::formulacell::FormulaGroupOffsetPlan;
    using spreadsheetengine::core::formulacell::FormulaGroupPreflightFailure;
    using spreadsheetengine::core::formulacell::FormulaGroupPreflightPlan;
    using spreadsheetengine::core::formulacell::GroupInterpretFallbackPlan;
    using spreadsheetengine::core::formulacell::GroupInterpretPreflightPlan;
    using spreadsheetengine::core::formulacell::LoadTrackingPlan;
    using spreadsheetengine::core::formulacell::NotifyKind;
    using spreadsheetengine::core::formulacell::NotifyPlan;
    using spreadsheetengine::core::formulacell::OpenCLChunkCleanupPlan;
    using spreadsheetengine::core::formulacell::OpenCLChunkingPlan;
    using spreadsheetengine::core::formulacell::OpenCLChunkSpan;
    using spreadsheetengine::core::formulacell::OpenCLMaxGroupLengthPlan;
    using spreadsheetengine::core::formulacell::ParallelCalculationPlan;
    using spreadsheetengine::core::formulacell::TableOpDirtyPlan;
    using spreadsheetengine::core::formulacell::ThreadingCompletionPlan;
    using spreadsheetengine::core::formulacell::ThreadingProbeFallbackPlan;
    using spreadsheetengine::core::formulacell::ThreadingProbeWindowPlan;
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

    if (spreadsheetengine::core::formulacell::makeDependencyCheckPreflightPlan(false)
            != DependencyCheckPlan { true, false, DependencyCheckFailure::None }
        || spreadsheetengine::core::formulacell::makeDependencyCheckPreflightPlan(true)
               != DependencyCheckPlan { false, true, DependencyCheckFailure::Cycle }
        || spreadsheetengine::core::formulacell::makeDependencyCheckResultPlan(
               true, false, true, true)
               != DependencyCheckPlan { false, true, DependencyCheckFailure::RecursionLimit }
        || spreadsheetengine::core::formulacell::makeDependencyCheckResultPlan(
               false, true, true, true)
               != DependencyCheckPlan { false, true, DependencyCheckFailure::Cycle }
        || spreadsheetengine::core::formulacell::makeDependencyCheckResultPlan(
               false, false, false, true)
               != DependencyCheckPlan { false, false,
                   DependencyCheckFailure::GroupsNotIndependent }
        || spreadsheetengine::core::formulacell::makeDependencyCheckResultPlan(
               false, false, true, false)
               != DependencyCheckPlan { false, true,
                   DependencyCheckFailure::DependencyCalculationFailed }
        || spreadsheetengine::core::formulacell::makeDependencyCheckResultPlan(
               false, false, true, true)
               != DependencyCheckPlan { true, false, DependencyCheckFailure::None })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "dependency check plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeGroupInterpretPreflightPlan(
            false, false, true, true)
            != GroupInterpretPreflightPlan { true, false, false, GroupInterpretFailure::None }
        || spreadsheetengine::core::formulacell::makeGroupInterpretPreflightPlan(
               true, true, true, true)
               != GroupInterpretPreflightPlan { false, false, false,
                   GroupInterpretFailure::DependencyComputationAborted }
        || spreadsheetengine::core::formulacell::makeGroupInterpretPreflightPlan(
               false, true, false, true)
               != GroupInterpretPreflightPlan { false, false, false,
                   GroupInterpretFailure::FormulaGroupNotIndependent }
        || spreadsheetengine::core::formulacell::makeGroupInterpretPreflightPlan(
               false, false, true, false)
               != GroupInterpretPreflightPlan { false, false, false,
                   GroupInterpretFailure::GroupsNotIndependent }
        || spreadsheetengine::core::formulacell::makeGroupInterpretCycleAbortPlan(
               false, true, true)
               != GroupInterpretPreflightPlan { true, false, false, GroupInterpretFailure::None }
        || spreadsheetengine::core::formulacell::makeGroupInterpretCycleAbortPlan(
               true, false, true)
               != GroupInterpretPreflightPlan { true, false, false, GroupInterpretFailure::None }
        || spreadsheetengine::core::formulacell::makeGroupInterpretCycleAbortPlan(
               true, true, false)
               != GroupInterpretPreflightPlan { true, false, false, GroupInterpretFailure::None }
        || spreadsheetengine::core::formulacell::makeGroupInterpretCycleAbortPlan(
               true, true, true)
               != GroupInterpretPreflightPlan { false, true, true,
                   GroupInterpretFailure::CycleDuringDependencyComputation }
        || spreadsheetengine::core::formulacell::makeGroupInterpretFallbackPlan(true, false)
               != GroupInterpretFallbackPlan { false, GroupInterpretFailure::None }
        || spreadsheetengine::core::formulacell::makeGroupInterpretFallbackPlan(false, false)
               != GroupInterpretFallbackPlan { true, GroupInterpretFailure::GroupsNotIndependent }
        || spreadsheetengine::core::formulacell::makeGroupInterpretFallbackPlan(true, true)
               != GroupInterpretFallbackPlan { true, GroupInterpretFailure::ParentCycleSkipTail })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "group interpret plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeGroupBackendDependencyEntryPlan(false, false)
            != GroupBackendDependencyPlan { true, true, false, false }
        || spreadsheetengine::core::formulacell::makeGroupBackendDependencyEntryPlan(true, false)
               != GroupBackendDependencyPlan { true, false, false, false }
        || spreadsheetengine::core::formulacell::makeGroupBackendDependencyEntryPlan(false, true)
               != GroupBackendDependencyPlan { false, false, false, false }
        || spreadsheetengine::core::formulacell::makeGroupBackendDependencyResultPlan(true)
               != GroupBackendDependencyPlan { true, false, true, false }
        || spreadsheetengine::core::formulacell::makeGroupBackendDependencyResultPlan(false)
               != GroupBackendDependencyPlan { false, false, true, true })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "group backend dependency plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeThreadingBackendPreflightPlan(
            false, false, true, true)
            != GroupBackendPreflightPlan { true, false, GroupBackendFailure::None }
        || spreadsheetengine::core::formulacell::makeThreadingBackendPreflightPlan(
               true, false, true, true)
               != GroupBackendPreflightPlan { false, false,
                   GroupBackendFailure::DependencyCheckFailedPreviously }
        || spreadsheetengine::core::formulacell::makeThreadingBackendPreflightPlan(
               false, true, true, true)
               != GroupBackendPreflightPlan { false, false,
                   GroupBackendFailure::ThreadingProhibited }
        || spreadsheetengine::core::formulacell::makeThreadingBackendPreflightPlan(
               false, false, false, true)
               != GroupBackendPreflightPlan { false, false,
                   GroupBackendFailure::ThreadingOpcodeDisabled }
        || spreadsheetengine::core::formulacell::makeThreadingBackendPreflightPlan(
               false, false, true, false)
               != GroupBackendPreflightPlan { false, false,
                   GroupBackendFailure::ThreadingDisabled }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::Enabled, true,
               true, false, false)
               != GroupBackendPreflightPlan { true, false, GroupBackendFailure::None }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::DisabledByOpcode,
               false, true, false, false)
               != GroupBackendPreflightPlan { false, true,
                   GroupBackendFailure::OpenCLVectorOpcodeDisabled }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::DisabledByStackVariable,
               false, true, false, false)
               != GroupBackendPreflightPlan { false, true,
                   GroupBackendFailure::OpenCLVectorStackVariableDisabled }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::DisabledNotInSubset,
               false, true, false, false)
               != GroupBackendPreflightPlan { false, true,
                   GroupBackendFailure::OpenCLVectorNotInSubset }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::DisabledOrUnknown,
               false, true, false, false)
               != GroupBackendPreflightPlan { false, true,
                   GroupBackendFailure::OpenCLVectorUnknown }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::Enabled, false,
               true, false, false)
               != GroupBackendPreflightPlan { false, false,
                   GroupBackendFailure::OpenCLNotVectorizable }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::Enabled, true,
               false, false, false)
               != GroupBackendPreflightPlan { false, true,
                   GroupBackendFailure::OpenCLDisabled }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::Enabled, true,
               true, true, false)
               != GroupBackendPreflightPlan { false, false,
                   GroupBackendFailure::InterpreterTableOp }
        || spreadsheetengine::core::formulacell::makeOpenCLBackendPreflightPlan(
               spreadsheetengine::core::formulacell::OpenCLVectorStateClass::Enabled, true,
               true, false, true)
               != GroupBackendPreflightPlan { false, false,
                   GroupBackendFailure::DependencyCheckFailedPreviously })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "group backend preflight plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeFormulaGroupPreflightPlan(
            false, false, false, false, false, false, false, false, true)
            != FormulaGroupPreflightPlan { true, false, FormulaGroupPreflightFailure::None }
        || spreadsheetengine::core::formulacell::makeFormulaGroupPreflightPlan(
               true, false, false, false, false, false, false, false, true)
               != FormulaGroupPreflightPlan { false, false,
                   FormulaGroupPreflightFailure::PartOfCycle }
        || spreadsheetengine::core::formulacell::makeFormulaGroupPreflightPlan(
               false, true, false, false, false, false, false, false, true)
               != FormulaGroupPreflightPlan { false, false,
                   FormulaGroupPreflightFailure::GroupCalcDisabled }
        || spreadsheetengine::core::formulacell::makeFormulaGroupPreflightPlan(
               false, false, true, false, false, false, false, false, true)
               != FormulaGroupPreflightPlan { false, true,
                   FormulaGroupPreflightFailure::GroupSizeThreshold }
        || spreadsheetengine::core::formulacell::makeFormulaGroupPreflightPlan(
               false, false, false, true, false, false, false, false, true)
               != FormulaGroupPreflightPlan { false, true,
                   FormulaGroupPreflightFailure::GroupSizeThreshold }
        || spreadsheetengine::core::formulacell::makeFormulaGroupPreflightPlan(
               false, false, false, true, true, false, false, false, true)
               != FormulaGroupPreflightPlan { true, false,
                   FormulaGroupPreflightFailure::None }
        || spreadsheetengine::core::formulacell::makeFormulaGroupPreflightPlan(
               false, false, false, false, false, false, true, false, true)
               != FormulaGroupPreflightPlan { false, true,
                   FormulaGroupPreflightFailure::MatrixSkipped }
        || spreadsheetengine::core::formulacell::makeFormulaGroupPreflightPlan(
               false, false, false, false, false, false, false, true, false)
               != FormulaGroupPreflightPlan { false, true,
                   FormulaGroupPreflightFailure::CellNotInDocument })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "formula-group preflight plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeFormulaGroupOffsetPlan(-1, -1, 9, true)
            != FormulaGroupOffsetPlan { 0, 9, false, FormulaGroupPreflightFailure::None }
        || spreadsheetengine::core::formulacell::makeFormulaGroupOffsetPlan(12, 14, 9, true)
               != FormulaGroupOffsetPlan { 9, 9, true,
                   FormulaGroupPreflightFailure::SingleRowWithoutForce }
        || spreadsheetengine::core::formulacell::makeFormulaGroupOffsetPlan(7, 3, 9, true)
               != FormulaGroupOffsetPlan { 0, 9, false, FormulaGroupPreflightFailure::None }
        || spreadsheetengine::core::formulacell::makeFormulaGroupOffsetPlan(5, 5, 9, true)
               != FormulaGroupOffsetPlan { 5, 5, true,
                   FormulaGroupPreflightFailure::SingleRowWithoutForce }
        || spreadsheetengine::core::formulacell::makeFormulaGroupOffsetPlan(5, 5, 9, false)
               != FormulaGroupOffsetPlan { 5, 5, false, FormulaGroupPreflightFailure::None })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "formula-group offset plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeThreadingProbeWindowPlan(false, true, 7)
            != ThreadingProbeWindowPlan { 7, 7, true }
        || spreadsheetengine::core::formulacell::makeThreadingProbeWindowPlan(true, true, 7)
               != ThreadingProbeWindowPlan { 7, 7, false }
        || spreadsheetengine::core::formulacell::makeThreadingProbeWindowPlan(false, false, 7)
               != ThreadingProbeWindowPlan { 7, 7, false }
        || spreadsheetengine::core::formulacell::makeThreadingProbeFallbackPlan(
               7, 5, 9, true, true, false)
               != ThreadingProbeFallbackPlan { 5, 9, false }
        || spreadsheetengine::core::formulacell::makeThreadingProbeFallbackPlan(
               7, 5, 9, false, true, false)
               != ThreadingProbeFallbackPlan { 7, 7, false }
        || spreadsheetengine::core::formulacell::makeThreadingProbeFallbackPlan(
               7, 5, 9, true, false, false)
               != ThreadingProbeFallbackPlan { 7, 7, false }
        || spreadsheetengine::core::formulacell::makeThreadingProbeFallbackPlan(
               7, 5, 9, false, true, true)
               != ThreadingProbeFallbackPlan { 7, 7, true })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "threading probe plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeOpenCLChunkingPlan(10, 1000)
            != OpenCLChunkingPlan { 1, 0, false }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkingPlan(1000, 1000)
               != OpenCLChunkingPlan { 1, 0, false }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkingPlan(1001, 1000)
               != OpenCLChunkingPlan { 2, 1, true }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkingPlan(10, 4)
               != OpenCLChunkingPlan { 3, 1, true }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkSpan(10, 3, 1, 0)
               != OpenCLChunkSpan { 0, 4 }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkSpan(10, 3, 1, 1)
               != OpenCLChunkSpan { 4, 3 }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkSpan(10, 3, 1, 2)
               != OpenCLChunkSpan { 7, 3 }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkFailurePlan(false)
               != OpenCLChunkCleanupPlan { true, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkFailurePlan(true)
               != OpenCLChunkCleanupPlan { true, true, true, true, false }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkSuccessTransferPlan(false)
               != OpenCLChunkCleanupPlan { false, false, false, false, false }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkSuccessTransferPlan(true)
               != OpenCLChunkCleanupPlan { false, false, true, true, false }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkFinalizationPlan(false)
               != OpenCLChunkCleanupPlan { false, false, false, false, true }
        || spreadsheetengine::core::formulacell::makeOpenCLChunkFinalizationPlan(true)
               != OpenCLChunkCleanupPlan { false, true, false, false, true })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "opencl chunk plan mismatch");
    }

    if (spreadsheetengine::core::formulacell::makeOpenCLMaxGroupLengthPlan(false, false, 1234)
            != OpenCLMaxGroupLengthPlan { INT_MAX }
        || spreadsheetengine::core::formulacell::makeOpenCLMaxGroupLengthPlan(true, false, 1234)
               != OpenCLMaxGroupLengthPlan { 1000 }
        || spreadsheetengine::core::formulacell::makeOpenCLMaxGroupLengthPlan(false, true, 2048)
               != OpenCLMaxGroupLengthPlan { 2048 }
        || spreadsheetengine::core::formulacell::makeOpenCLMaxGroupLengthPlan(true, true, 2048)
               != OpenCLMaxGroupLengthPlan { 2048 }
        || spreadsheetengine::core::formulacell::makeThreadingCompletionPlan(20, 3, 8)
               != ThreadingCompletionPlan { 23, 6 })
    {
        return fail("spreadsheetengine_formulacell_tests",
                    "final backend plan mismatch");
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
