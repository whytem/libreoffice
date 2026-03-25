/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace spreadsheetengine::core::formulacell
{

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
