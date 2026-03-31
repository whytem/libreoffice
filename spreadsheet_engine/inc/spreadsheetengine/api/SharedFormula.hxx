/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <vector>

#include <spreadsheetengine/api/Host.hxx>

#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::api::sharedformula
{

enum class TokenCompareState : std::uint8_t
{
    NotEqual,
    EqualInvariant,
    EqualRelativeRef
};

enum class JoinAction : std::uint8_t
{
    None,
    MergeGroups,
    ExtendUpperGroup,
    AdoptLowerGroup,
    CreateGroup
};

enum class UnsharePosition : std::uint8_t
{
    None,
    Top,
    Bottom,
    Middle
};

enum class GroupRunAction : std::uint8_t
{
    None,
    ExtendExistingGroup,
    CreateGroup
};

struct JoinPlan
{
    JoinAction meAction = JoinAction::None;
    bool mbInvariant = false;

    [[nodiscard]] constexpr bool operator==(const JoinPlan& rOther) const = default;
};

struct SplitPlan
{
    bool mbShouldSplit = false;
    bool mbCreateLowerGroup = false;
    bool mbUnshareUpperGroup = false;
    sal_Int32 mnUpperLength = 0;
    sal_Int32 mnLowerLength = 0;

    [[nodiscard]] constexpr bool operator==(const SplitPlan& rOther) const = default;
};

struct UnsharePlan
{
    UnsharePosition mePosition = UnsharePosition::None;
    bool mbUnshareAdjacentUpper = false;
    bool mbUnshareAdjacentLower = false;
    bool mbCreateLowerGroup = false;
    sal_Int32 mnUpperLength = 0;
    sal_Int32 mnLowerLength = 0;

    [[nodiscard]] constexpr bool operator==(const UnsharePlan& rOther) const = default;
};

struct GroupRunPlan
{
    GroupRunAction meAction = GroupRunAction::None;
    bool mbInvariant = false;

    [[nodiscard]] constexpr bool operator==(const GroupRunPlan& rOther) const = default;
};

struct GroupDoubleRefListenPlan
{
    CellRange maOriginalRange;
    CellRange maListenedRange;
    bool mbRef1RowFixed = false;
    bool mbRef2RowFixed = false;

    [[nodiscard]] constexpr bool operator==(const GroupDoubleRefListenPlan& rOther) const = default;
};

struct GroupSingleRefListenPlan
{
    CellAddress maAddress;
    bool mbListen = false;

    [[nodiscard]] constexpr bool operator==(const GroupSingleRefListenPlan& rOther) const = default;
};

[[nodiscard]] constexpr bool shouldJoinFormulaCells(TokenCompareState eState)
{
    return eState != TokenCompareState::NotEqual;
}

[[nodiscard]] constexpr bool shouldReturnSharedTopFormulaCell(
    bool bFormulaBlock, bool bShared)
{
    return bFormulaBlock && bShared;
}

[[nodiscard]] constexpr JoinPlan makeJoinPlan(
    TokenCompareState eState, bool bUpperShared, bool bLowerShared, bool bSameGroup)
{
    if (!shouldJoinFormulaCells(eState) || (bUpperShared && bLowerShared && bSameGroup))
        return {};

    if (bUpperShared)
    {
        if (bLowerShared)
            return { JoinAction::MergeGroups, false };

        return { JoinAction::ExtendUpperGroup, false };
    }

    if (bLowerShared)
        return { JoinAction::AdoptLowerGroup, false };

    return { JoinAction::CreateGroup, eState == TokenCompareState::EqualInvariant };
}

[[nodiscard]] constexpr bool canJoinFormulaCellAbove(
    bool bFormulaBlock, sal_Int32 nBlockOffset)
{
    return bFormulaBlock && nBlockOffset > 0;
}

[[nodiscard]] constexpr GroupRunPlan makeGroupRunPlan(
    TokenCompareState eState, bool bPreviousShared)
{
    if (!shouldJoinFormulaCells(eState))
        return {};

    if (bPreviousShared)
        return { GroupRunAction::ExtendExistingGroup, false };

    return { GroupRunAction::CreateGroup, eState == TokenCompareState::EqualInvariant };
}

[[nodiscard]] constexpr bool isValidListenAddress(const CellAddress& rAddress)
{
    return rAddress.mnSheet >= 0 && rAddress.mnColumn >= 0 && rAddress.mnRow >= 0;
}

[[nodiscard]] constexpr GroupSingleRefListenPlan makeGroupSingleRefListenPlan(
    const CellAddress& rAddress)
{
    return { rAddress, isValidListenAddress(rAddress) };
}

[[nodiscard]] constexpr GroupDoubleRefListenPlan makeGroupDoubleRefListenPlan(
    const CellRange& rAbsoluteRange, bool bRef1RowRelative, bool bRef2RowRelative,
    sal_Int32 nGroupLength)
{
    CellRange aListenedRange = rAbsoluteRange;
    if (bRef2RowRelative && nGroupLength > 1)
        aListenedRange.maEnd.mnRow += nGroupLength - 1;

    return { rAbsoluteRange, aListenedRange, !bRef1RowRelative, !bRef2RowRelative };
}

[[nodiscard]] constexpr bool canSplitSharedFormulaGroup(
    bool bFormulaBlock, sal_Int32 nBlockOffset, bool bShared, sal_Int32 nSplitRow,
    sal_Int32 nSharedTopRow)
{
    return bFormulaBlock && nBlockOffset > 0 && bShared && nSplitRow != nSharedTopRow;
}

[[nodiscard]] constexpr SplitPlan makeSplitPlan(
    sal_Int32 nTopRow, sal_Int32 nGroupLength, sal_Int32 nSplitRow)
{
    const sal_Int32 nUpperLength = nSplitRow - nTopRow;
    const sal_Int32 nLowerLength = nTopRow + nGroupLength - nSplitRow;
    return { true, nLowerLength > 1, nUpperLength == 1, nUpperLength, nLowerLength };
}

[[nodiscard]] constexpr UnsharePosition classifyUnsharePosition(
    sal_Int32 nRow, sal_Int32 nTopRow, sal_Int32 nLength)
{
    if (nLength <= 0)
        return UnsharePosition::None;

    if (nRow == nTopRow)
        return UnsharePosition::Top;

    if (nRow == nTopRow + nLength - 1)
        return UnsharePosition::Bottom;

    return UnsharePosition::Middle;
}

[[nodiscard]] constexpr UnsharePlan makeUnsharePlan(
    sal_Int32 nRow, sal_Int32 nTopRow, sal_Int32 nLength)
{
    const UnsharePosition ePos = classifyUnsharePosition(nRow, nTopRow, nLength);
    switch (ePos)
    {
        case UnsharePosition::Top:
            return { ePos, false, nLength == 2, false, nLength - 1, 0 };
        case UnsharePosition::Bottom:
            return { ePos, nLength == 2, false, false, nLength - 1, 0 };
        case UnsharePosition::Middle:
        {
            const sal_Int32 nUpperLength = nRow - nTopRow;
            const sal_Int32 nLowerLength = nTopRow + nLength - 1 - nRow;
            return { ePos, nUpperLength == 1, nLowerLength == 1, nLowerLength >= 2,
                     nUpperLength, nLowerLength };
        }
        case UnsharePosition::None:
            break;
    }

    return {};
}

inline void sortAndUniqueRows(std::vector<sal_Int32>& rRows)
{
    std::sort(rRows.begin(), rRows.end());
    rRows.erase(std::unique(rRows.begin(), rRows.end()), rRows.end());
}

[[nodiscard]] inline std::vector<sal_Int32> makeUnshareBoundaryRows(
    std::vector<sal_Int32> aRows, sal_Int32 nMaxRow)
{
    sortAndUniqueRows(aRows);

    std::vector<sal_Int32> aBounds;
    for (sal_Int32 nRow : aRows)
    {
        if (nRow > nMaxRow)
            break;

        aBounds.push_back(nRow);
        if (nRow < nMaxRow)
            aBounds.push_back(nRow + 1);
    }

    sortAndUniqueRows(aBounds);
    return aBounds;
}

} // namespace spreadsheetengine::api::sharedformula

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
