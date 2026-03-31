/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cassert>
#include <climits>

#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::core::formulacell
{

enum class NotifyKind : sal_uInt8
{
    Other,
    DataChanged,
    TableOpDirty,
    HiddenRowsChanged
};

enum class VolatileKind : sal_uInt8
{
    Other,
    VolatileMacro,
    NotVolatile
};

enum class DependencyCheckFailure : sal_uInt8
{
    None,
    Cycle,
    RecursionLimit,
    GroupsNotIndependent,
    DependencyCalculationFailed
};

enum class GroupInterpretFailure : sal_uInt8
{
    None,
    DependencyComputationAborted,
    FormulaGroupNotIndependent,
    GroupsNotIndependent,
    CycleDuringDependencyComputation,
    ParentCycleSkipTail
};

enum class GroupBackendFailure : sal_uInt8
{
    None,
    DependencyCheckFailedPreviously,
    ThreadingProhibited,
    ThreadingOpcodeDisabled,
    ThreadingDisabled,
    OpenCLVectorOpcodeDisabled,
    OpenCLVectorStackVariableDisabled,
    OpenCLVectorNotInSubset,
    OpenCLVectorUnknown,
    OpenCLNotVectorizable,
    OpenCLDisabled,
    InterpreterTableOp
};

enum class OpenCLVectorStateClass : sal_uInt8
{
    Enabled,
    DisabledByOpcode,
    DisabledByStackVariable,
    DisabledNotInSubset,
    DisabledOrUnknown
};

enum class FormulaGroupPreflightFailure : sal_uInt8
{
    None,
    PartOfCycle,
    GroupCalcDisabled,
    GroupSizeThreshold,
    MatrixSkipped,
    CellNotInDocument,
    SingleRowWithoutForce
};

struct DirtyPlan
{
    bool mbSkip = false;
    bool mbSetDirtyVar = false;
    bool mbAppendToTrack = false;
    bool mbTrackFormulas = false;
    bool mbInvalidateStream = false;

    [[nodiscard]] constexpr bool operator==(const DirtyPlan& rOther) const = default;
};

struct TableOpDirtyPlan
{
    bool mbSkip = false;
    bool mbSetTableOpDirty = false;
    bool mbAddTableOpCell = false;
    bool mbAppendToTrack = false;
    bool mbTrackFormulas = false;

    [[nodiscard]] constexpr bool operator==(const TableOpDirtyPlan& rOther) const = default;
};

struct LoadTrackingPlan
{
    bool mbSetDirtyVar = false;
    bool mbAppendToTrack = false;
    bool mbPutInFormulaTree = false;

    [[nodiscard]] constexpr bool operator==(const LoadTrackingPlan& rOther) const = default;
};

struct CalcAfterLoadPlan
{
    bool mbStartListening = false;
    bool mbMarkDirty = false;

    [[nodiscard]] constexpr bool operator==(const CalcAfterLoadPlan& rOther) const = default;
};

struct NotifyPlan
{
    bool mbIgnore = false;
    bool mbSetDirtyVar = false;
    bool mbSetTableOpDirty = false;
    bool mbAddTableOpCell = false;
    bool mbAppendToTrack = false;

    [[nodiscard]] constexpr bool operator==(const NotifyPlan& rOther) const = default;
};

struct ParallelCalculationPlan
{
    bool mbSkip = false;
    bool mbRemoveFromFormulaTreeBeforeVolatileCheck = false;
    bool mbSetRecalcModeAlways = false;
    bool mbSetRecalcModeNormal = false;
    bool mbPutInFormulaTree = false;
    bool mbRemoveFromFormulaTree = false;
    bool mbStartListening = false;
    bool mbEndListening = false;
    bool mbEndAlwaysListeningArea = false;

    [[nodiscard]] constexpr bool operator==(const ParallelCalculationPlan& rOther) const = default;
};

struct DependencyCheckPlan
{
    bool mbCanProceed = true;
    bool mbDisableGroupCalc = false;
    DependencyCheckFailure meFailure = DependencyCheckFailure::None;

    [[nodiscard]] constexpr bool operator==(const DependencyCheckPlan& rOther) const = default;
};

struct GroupInterpretPreflightPlan
{
    bool mbCanProceed = true;
    bool mbAbortDependencyComputation = false;
    bool mbNeedCycleCheckGuard = false;
    GroupInterpretFailure meFailure = GroupInterpretFailure::None;

    [[nodiscard]] constexpr bool operator==(const GroupInterpretPreflightPlan& rOther) const
        = default;
};

struct GroupInterpretFallbackPlan
{
    bool mbSkipInterpretTail = false;
    GroupInterpretFailure meFailure = GroupInterpretFailure::None;

    [[nodiscard]] constexpr bool operator==(const GroupInterpretFallbackPlan& rOther) const
        = default;
};

struct GroupBackendDependencyPlan
{
    bool mbCanProceed = true;
    bool mbNeedDependencyCheck = false;
    bool mbMarkDependencyComputed = false;
    bool mbMarkDependencyCheckFailed = false;

    [[nodiscard]] constexpr bool operator==(const GroupBackendDependencyPlan& rOther) const
        = default;
};

struct GroupBackendPreflightPlan
{
    bool mbCanProceed = true;
    bool mbEmitFailureMessage = false;
    GroupBackendFailure meFailure = GroupBackendFailure::None;

    [[nodiscard]] constexpr bool operator==(const GroupBackendPreflightPlan& rOther) const
        = default;
};

struct FormulaGroupPreflightPlan
{
    bool mbCanProceed = true;
    bool mbDisableGroupCalc = false;
    FormulaGroupPreflightFailure meFailure = FormulaGroupPreflightFailure::None;

    [[nodiscard]] constexpr bool operator==(const FormulaGroupPreflightPlan& rOther) const
        = default;
};

struct FormulaGroupOffsetPlan
{
    sal_Int32 mnStartOffset = 0;
    sal_Int32 mnEndOffset = 0;
    bool mbSkipInterpretation = false;
    FormulaGroupPreflightFailure meFailure = FormulaGroupPreflightFailure::None;

    [[nodiscard]] constexpr bool operator==(const FormulaGroupOffsetPlan& rOther) const = default;
};

struct ThreadingProbeWindowPlan
{
    sal_Int32 mnStartColumn = 0;
    sal_Int32 mnEndColumn = 0;
    bool mbProbeNeighbors = false;

    [[nodiscard]] constexpr bool operator==(const ThreadingProbeWindowPlan& rOther) const
        = default;
};

struct ThreadingProbeFallbackPlan
{
    sal_Int32 mnStartColumn = 0;
    sal_Int32 mnEndColumn = 0;
    bool mbRedoOriginalDependencyCheck = false;

    [[nodiscard]] constexpr bool operator==(const ThreadingProbeFallbackPlan& rOther) const
        = default;
};

struct OpenCLChunkingPlan
{
    sal_Int32 mnNumParts = 0;
    sal_Int32 mnNumOnePlus = 0;
    bool mbUseTemporaryGroups = false;

    [[nodiscard]] constexpr bool operator==(const OpenCLChunkingPlan& rOther) const = default;
};

struct OpenCLChunkSpan
{
    sal_Int32 mnOffset = 0;
    sal_Int32 mnLength = 0;

    [[nodiscard]] constexpr bool operator==(const OpenCLChunkSpan& rOther) const = default;
};

struct OpenCLChunkCleanupPlan
{
    bool mbDisableGroupCalc = false;
    bool mbRestoreOriginalPosition = false;
    bool mbDetachTemporaryTopCell = false;
    bool mbRestoreTransferredCode = false;
    bool mbSetGroupCalcEnabled = false;

    [[nodiscard]] constexpr bool operator==(const OpenCLChunkCleanupPlan& rOther) const
        = default;
};

struct OpenCLMaxGroupLengthPlan
{
    sal_Int32 mnMaxGroupLength = INT_MAX;

    [[nodiscard]] constexpr bool operator==(const OpenCLMaxGroupLengthPlan& rOther) const
        = default;
};

struct ThreadingCompletionPlan
{
    sal_Int32 mnStartRow = 0;
    sal_Int32 mnSpanLength = 0;

    [[nodiscard]] constexpr bool operator==(const ThreadingCompletionPlan& rOther) const
        = default;
};

struct InvariantGroupPlan
{
    bool mbResolveStaticReferences = false;

    [[nodiscard]] constexpr bool operator==(const InvariantGroupPlan& rOther) const = default;
};

[[nodiscard]] constexpr DirtyPlan makeSetDirtyPlan(
    bool bInChangeTrack, bool bHardRecalcEnabled, bool bCurrentDirty,
    bool bPostponedDirty, bool bInFormulaTree, bool bRequestDirtyFlag,
    bool bImportingXml, bool bInsertingFromOtherDoc)
{
    if (bInChangeTrack)
        return { true, false, false, false, false };

    if (bHardRecalcEnabled)
        return { false, true, false, false, true };

    const bool bNeedsTrackAppend = !bCurrentDirty || bPostponedDirty || !bInFormulaTree;
    return { false,
             bRequestDirtyFlag && bNeedsTrackAppend,
             bNeedsTrackAppend,
             bNeedsTrackAppend && !bImportingXml && !bInsertingFromOtherDoc,
             true };
}

[[nodiscard]] constexpr bool shouldResetGroupCalcState(
    bool bHasGroup, bool bGroupCalcRunning)
{
    return bHasGroup && bGroupCalcRunning;
}

[[nodiscard]] constexpr bool shouldMarkDirtyForRecalcMode(bool bRecalcModeNormal)
{
    return !bRecalcModeNormal;
}

[[nodiscard]] constexpr LoadTrackingPlan makeLoadTrackingPlan(
    bool bRecalcModeNormal, bool bRecalcModeForced, bool bWasInFormulaTree)
{
    if (!bRecalcModeNormal || bRecalcModeForced)
        return { true, true, false };

    return { false, false, bWasInFormulaTree };
}

[[nodiscard]] constexpr CalcAfterLoadPlan makeCalcAfterLoadPlan(
    bool bNewCompiled, bool bCodeErrorNone, bool bStartListening, bool bRecalcModeNormal,
    bool bRecalcModeAlways)
{
    const bool bCanUsePostLoadState = !bNewCompiled || bCodeErrorNone;
    return { bCanUsePostLoadState && bStartListening,
             (bCanUsePostLoadState && !bRecalcModeNormal) || bRecalcModeAlways };
}

[[nodiscard]] constexpr NotifyPlan makeNotifyPlan(
    bool bHardRecalcEnabled, NotifyKind eKind, bool bSubTotal, bool bCurrentTableOpDirty,
    bool bCurrentDirty, bool bInFormulaTree, bool bRecalcModeAlways, bool bInFormulaTrack)
{
    if (bHardRecalcEnabled)
        return { true, false, false, false, false };

    const bool bRelevant = eKind == NotifyKind::DataChanged
                           || eKind == NotifyKind::TableOpDirty
                           || (bSubTotal && eKind == NotifyKind::HiddenRowsChanged);
    if (!bRelevant)
        return { true, false, false, false, false };

    if (eKind == NotifyKind::TableOpDirty)
    {
        const bool bForceTrack = !bCurrentTableOpDirty;
        return { false, false, bForceTrack, bForceTrack,
                 (bForceTrack || !bInFormulaTree || bRecalcModeAlways) && !bInFormulaTrack };
    }

    const bool bForceTrack = !bCurrentDirty;
    return { false, true, false, false,
             (bForceTrack || !bInFormulaTree || bRecalcModeAlways) && !bInFormulaTrack };
}

[[nodiscard]] constexpr ParallelCalculationPlan makeParallelCalculationPlan(
    bool bHasCode, bool bRecalcModeAlways, VolatileKind eVolatileKind)
{
    if (!bHasCode)
        return { true, false, false, false, false, false, false, false, false };

    ParallelCalculationPlan aPlan;
    aPlan.mbRemoveFromFormulaTreeBeforeVolatileCheck = !bRecalcModeAlways;

    switch (eVolatileKind)
    {
        case VolatileKind::VolatileMacro:
            aPlan.mbSetRecalcModeAlways = true;
            aPlan.mbPutInFormulaTree = true;
            aPlan.mbStartListening = true;
            break;
        case VolatileKind::NotVolatile:
            if (bRecalcModeAlways)
            {
                aPlan.mbEndListening = true;
                aPlan.mbSetRecalcModeNormal = true;
            }
            else
                aPlan.mbEndAlwaysListeningArea = true;

            aPlan.mbRemoveFromFormulaTree = true;
            break;
        case VolatileKind::Other:
            break;
    }

    return aPlan;
}

[[nodiscard]] constexpr TableOpDirtyPlan makeSetTableOpDirtyPlan(
    bool bInChangeTrack, bool bHardRecalcEnabled, bool bCurrentTableOpDirty,
    bool bInFormulaTree)
{
    if (bInChangeTrack)
        return {};

    if (bHardRecalcEnabled)
        return { false, true, false, false, false };

    if (!bCurrentTableOpDirty || !bInFormulaTree)
        return { false, true, !bCurrentTableOpDirty, true, true };

    return {};
}

[[nodiscard]] constexpr DependencyCheckPlan makeDependencyCheckPreflightPlan(
    bool bPartOfCycle)
{
    if (!bPartOfCycle)
        return {};

    return { false, true, DependencyCheckFailure::Cycle };
}

[[nodiscard]] constexpr DependencyCheckPlan makeDependencyCheckResultPlan(
    bool bInRecursionReturn, bool bPartOfCycle, bool bGroupsIndependent, bool bOKToParallelize)
{
    if (bInRecursionReturn)
        return { false, true, DependencyCheckFailure::RecursionLimit };

    if (bPartOfCycle)
        return { false, true, DependencyCheckFailure::Cycle };

    if (!bGroupsIndependent)
        return { false, false, DependencyCheckFailure::GroupsNotIndependent };

    if (!bOKToParallelize)
        return { false, true, DependencyCheckFailure::DependencyCalculationFailed };

    return {};
}

[[nodiscard]] constexpr GroupInterpretPreflightPlan makeGroupInterpretPreflightPlan(
    bool bAbortingDependencyComputation, bool bHasGroup, bool bFormulaGroupIndependent,
    bool bGroupsIndependent)
{
    if (bAbortingDependencyComputation)
        return { false, false, false, GroupInterpretFailure::DependencyComputationAborted };

    if (bHasGroup && !bFormulaGroupIndependent)
        return { false, false, false, GroupInterpretFailure::FormulaGroupNotIndependent };

    if (!bGroupsIndependent)
        return { false, false, false, GroupInterpretFailure::GroupsNotIndependent };

    return {};
}

[[nodiscard]] constexpr GroupInterpretPreflightPlan makeGroupInterpretCycleAbortPlan(
    bool bSeenInPath, bool bInDependencyComputation, bool bAnyCycleMemberInDependencyEvalMode)
{
    if (bSeenInPath && bInDependencyComputation && bAnyCycleMemberInDependencyEvalMode)
        return { false, true, true, GroupInterpretFailure::CycleDuringDependencyComputation };

    return {};
}

[[nodiscard]] constexpr GroupInterpretFallbackPlan makeGroupInterpretFallbackPlan(
    bool bGroupsIndependent, bool bSkipTailForParentCycle)
{
    if (!bGroupsIndependent)
        return { true, GroupInterpretFailure::GroupsNotIndependent };

    if (bSkipTailForParentCycle)
        return { true, GroupInterpretFailure::ParentCycleSkipTail };

    return {};
}

[[nodiscard]] constexpr GroupBackendDependencyPlan makeGroupBackendDependencyEntryPlan(
    bool bDependencyComputed, bool bDependencyCheckFailed)
{
    if (bDependencyCheckFailed)
        return { false, false, false, false };

    if (bDependencyComputed)
        return {};

    return { true, true, false, false };
}

[[nodiscard]] constexpr GroupBackendDependencyPlan makeGroupBackendDependencyResultPlan(
    bool bDependencyCheckSucceeded)
{
    if (bDependencyCheckSucceeded)
        return { true, false, true, false };

    return { false, false, true, true };
}

[[nodiscard]] constexpr GroupBackendPreflightPlan makeThreadingBackendPreflightPlan(
    bool bDependencyCheckFailed, bool bThreadingProhibited, bool bCodeEnabledForThreading,
    bool bThreadingEnabled)
{
    if (bDependencyCheckFailed)
        return { false, false, GroupBackendFailure::DependencyCheckFailedPreviously };

    if (bThreadingProhibited)
        return { false, false, GroupBackendFailure::ThreadingProhibited };

    if (!bCodeEnabledForThreading)
        return { false, false, GroupBackendFailure::ThreadingOpcodeDisabled };

    if (!bThreadingEnabled)
        return { false, false, GroupBackendFailure::ThreadingDisabled };

    return {};
}

[[nodiscard]] constexpr GroupBackendPreflightPlan makeOpenCLBackendPreflightPlan(
    OpenCLVectorStateClass eVectorStateClass, bool bCanVectorize, bool bOpenCLEnabled,
    bool bInInterpreterTableOp, bool bDependencyCheckFailed)
{
    switch (eVectorStateClass)
    {
        case OpenCLVectorStateClass::DisabledByOpcode:
            if (!bCanVectorize)
                return { false, true, GroupBackendFailure::OpenCLVectorOpcodeDisabled };
            return { true, true, GroupBackendFailure::OpenCLVectorOpcodeDisabled };
        case OpenCLVectorStateClass::DisabledByStackVariable:
            if (!bCanVectorize)
                return { false, true, GroupBackendFailure::OpenCLVectorStackVariableDisabled };
            return { true, true, GroupBackendFailure::OpenCLVectorStackVariableDisabled };
        case OpenCLVectorStateClass::DisabledNotInSubset:
            if (!bCanVectorize)
                return { false, true, GroupBackendFailure::OpenCLVectorNotInSubset };
            return { true, true, GroupBackendFailure::OpenCLVectorNotInSubset };
        case OpenCLVectorStateClass::DisabledOrUnknown:
            return { false, true, GroupBackendFailure::OpenCLVectorUnknown };
        case OpenCLVectorStateClass::Enabled:
            break;
    }

    if (!bCanVectorize)
        return { false, false, GroupBackendFailure::OpenCLNotVectorizable };

    if (!bOpenCLEnabled)
        return { false, true, GroupBackendFailure::OpenCLDisabled };

    if (bInInterpreterTableOp)
        return { false, false, GroupBackendFailure::InterpreterTableOp };

    if (bDependencyCheckFailed)
        return { false, false, GroupBackendFailure::DependencyCheckFailedPreviously };

    return {};
}

[[nodiscard]] constexpr FormulaGroupPreflightPlan makeFormulaGroupPreflightPlan(
    bool bPartOfCycle, bool bGroupCalcDisabled, bool bForceCalculationCore,
    bool bBelowMinimumGroupSize, bool bForceCalculationOpenCL, bool bForceCalculationThreads,
    bool bMatrixMode, bool bForceCalculationRequested, bool bCellInDocument)
{
    if (bPartOfCycle)
        return { false, false, FormulaGroupPreflightFailure::PartOfCycle };

    if (bGroupCalcDisabled)
        return { false, false, FormulaGroupPreflightFailure::GroupCalcDisabled };

    if (bForceCalculationCore
        || (bBelowMinimumGroupSize && !bForceCalculationOpenCL && !bForceCalculationThreads))
    {
        return { false, true, FormulaGroupPreflightFailure::GroupSizeThreshold };
    }

    if (bMatrixMode)
        return { false, true, FormulaGroupPreflightFailure::MatrixSkipped };

    if (bForceCalculationRequested && !bCellInDocument)
        return { false, true, FormulaGroupPreflightFailure::CellNotInDocument };

    return {};
}

[[nodiscard]] constexpr FormulaGroupOffsetPlan makeFormulaGroupOffsetPlan(
    sal_Int32 nStartOffset, sal_Int32 nEndOffset, sal_Int32 nMaxOffset, bool bForceCalculationNone)
{
    sal_Int32 nNormalizedStart = nStartOffset < 0 ? 0 : (nStartOffset > nMaxOffset ? nMaxOffset : nStartOffset);
    sal_Int32 nNormalizedEnd = nEndOffset < 0 ? nMaxOffset : (nEndOffset > nMaxOffset ? nMaxOffset : nEndOffset);

    if (nNormalizedEnd < nNormalizedStart)
    {
        nNormalizedStart = 0;
        nNormalizedEnd = nMaxOffset;
    }

    if (nNormalizedEnd == nNormalizedStart && bForceCalculationNone)
        return { nNormalizedStart, nNormalizedEnd, true,
            FormulaGroupPreflightFailure::SingleRowWithoutForce };

    return { nNormalizedStart, nNormalizedEnd, false, FormulaGroupPreflightFailure::None };
}

[[nodiscard]] constexpr ThreadingProbeWindowPlan makeThreadingProbeWindowPlan(
    bool bHasFormulaGroupSet, bool bInDocShellRecalc, sal_Int32 nCurrentColumn)
{
    return { nCurrentColumn, nCurrentColumn, !bHasFormulaGroupSet && bInDocShellRecalc };
}

[[nodiscard]] constexpr ThreadingProbeFallbackPlan makeThreadingProbeFallbackPlan(
    sal_Int32 nCurrentColumn, sal_Int32 nStartColumn, sal_Int32 nEndColumn,
    bool bSpeculativeProbeSucceeded, bool bGroupsIndependent, bool bRedoOriginalDependencyCheck)
{
    if (!bSpeculativeProbeSucceeded || !bGroupsIndependent)
        return { nCurrentColumn, nCurrentColumn, bRedoOriginalDependencyCheck };

    return { nStartColumn, nEndColumn, false };
}

[[nodiscard]] constexpr OpenCLChunkingPlan makeOpenCLChunkingPlan(
    sal_Int32 nSharedLength, sal_Int32 nMaxGroupLength)
{
    assert(nSharedLength > 0);
    assert(nMaxGroupLength > 0);

    sal_Int32 nNumOnePlus = 0;
    sal_Int32 nNumParts = 1;
    if (nSharedLength > nMaxGroupLength)
    {
        const sal_Int32 nIdealNumParts = nSharedLength / nMaxGroupLength;
        if (nIdealNumParts * nMaxGroupLength == nSharedLength)
            nNumParts = nIdealNumParts;
        else
        {
            nNumParts = nIdealNumParts + 1;
            const sal_Int32 nNominalPartSize = nSharedLength / nNumParts;
            nNumOnePlus = nSharedLength - nNumParts * nNominalPartSize;
        }
    }

    return { nNumParts, nNumOnePlus, nNumParts > 1 };
}

[[nodiscard]] constexpr OpenCLChunkSpan makeOpenCLChunkSpan(
    sal_Int32 nSharedLength, sal_Int32 nNumParts, sal_Int32 nNumOnePlus, sal_Int32 nChunkIndex)
{
    const sal_Int32 nBasePartSize = nSharedLength / nNumParts;
    const sal_Int32 nOffsetAdjustment = nChunkIndex < nNumOnePlus ? nChunkIndex : nNumOnePlus;
    return { nChunkIndex * nBasePartSize + nOffsetAdjustment,
             nBasePartSize + (nChunkIndex < nNumOnePlus ? 1 : 0) };
}

[[nodiscard]] constexpr OpenCLChunkCleanupPlan makeOpenCLChunkFailurePlan(
    bool bUseTemporaryGroups)
{
    return { true, bUseTemporaryGroups, bUseTemporaryGroups, bUseTemporaryGroups, false };
}

[[nodiscard]] constexpr OpenCLChunkCleanupPlan makeOpenCLChunkSuccessTransferPlan(
    bool bUseTemporaryGroups)
{
    return { false, false, bUseTemporaryGroups, bUseTemporaryGroups, false };
}

[[nodiscard]] constexpr OpenCLChunkCleanupPlan makeOpenCLChunkFinalizationPlan(
    bool bUseTemporaryGroups)
{
    return { false, bUseTemporaryGroups, false, false, true };
}

[[nodiscard]] constexpr OpenCLMaxGroupLengthPlan makeOpenCLMaxGroupLengthPlan(
    bool bNeedsTDRAvoidance, bool bHasEnvOverride, sal_Int32 nEnvMaxGroupLength)
{
    sal_Int32 nMaxGroupLength = INT_MAX;
    if (bNeedsTDRAvoidance)
        nMaxGroupLength = 1000;
    if (bHasEnvOverride)
        nMaxGroupLength = nEnvMaxGroupLength;
    return { nMaxGroupLength };
}

[[nodiscard]] constexpr ThreadingCompletionPlan makeThreadingCompletionPlan(
    sal_Int32 nTopRow, sal_Int32 nStartOffset, sal_Int32 nEndOffset)
{
    return { nTopRow + nStartOffset, nEndOffset - nStartOffset + 1 };
}

[[nodiscard]] constexpr InvariantGroupPlan makeInvariantGroupPlan(
    bool bVectorCheckReference)
{
    return { bVectorCheckReference };
}

} // namespace spreadsheetengine::core::formulacell

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
