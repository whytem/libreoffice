/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/types.h>

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

} // namespace spreadsheetengine::core::formulacell

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
