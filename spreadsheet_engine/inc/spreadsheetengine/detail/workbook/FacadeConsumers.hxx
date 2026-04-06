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
#include <optional>
#include <vector>

#include <spreadsheetengine/detail/workbook/WorkbookFacade.hxx>

namespace spreadsheetengine::detail::facade::consumers
{

/// Result of a formula-cell enumeration shadow check.
struct FormulaCellEnumerationResult
{
    sal_Int32 mnTotalFormulaCells = 0;
    sal_Int32 mnOrdinaryCells = 0;
    sal_Int32 mnSharedGroupMembers = 0;
    sal_Int32 mnMatrixOrigins = 0;
    sal_Int32 mnMatrixMembers = 0;
    sal_Int32 mnDirtyCells = 0;
};

/// Enumerate all formula cells through the facade and produce a summary.
/// This is the first shadow consumer — it proves the facade can enumerate
/// the formula-cell population of a workbook.
[[nodiscard]] inline FormulaCellEnumerationResult
enumerateFormulaCells(const WorkbookFacade& rFacade)
{
    FormulaCellEnumerationResult aResult;

    rFacade.visitAllFormulaCells([&aResult](const FormulaCellDescriptor& rDesc) {
        ++aResult.mnTotalFormulaCells;

        switch (rDesc.meKind)
        {
            case FormulaCellKind::Ordinary:
                ++aResult.mnOrdinaryCells;
                break;
            case FormulaCellKind::SharedGroupMember:
                ++aResult.mnSharedGroupMembers;
                break;
            case FormulaCellKind::MatrixOrigin:
                ++aResult.mnMatrixOrigins;
                break;
            case FormulaCellKind::MatrixMember:
                ++aResult.mnMatrixMembers;
                break;
        }

        if (rDesc.mbDirty)
            ++aResult.mnDirtyCells;

        return true;
    });

    return aResult;
}

/// Result of a shared-formula group comparison.
struct SharedFormulaGroupSummary
{
    sal_Int32 mnGroupCount = 0;
    sal_Int32 mnTotalGroupLength = 0;
    sal_Int32 mnShareableGroups = 0;
};

[[nodiscard]] inline std::vector<FormulaGroupDescriptor>
collectFormulaGroupDescriptors(const WorkbookFacade& rFacade)
{
    std::vector<FormulaGroupDescriptor> aGroups;

    rFacade.visitAllFormulaCells([&](const FormulaCellDescriptor& rDesc) {
        auto oGroup = rFacade.getFormulaGroupDescriptor(rDesc.maId.maAddress);
        if (!oGroup)
            return true;

        if (std::find(aGroups.begin(), aGroups.end(), *oGroup) == aGroups.end())
            aGroups.push_back(*oGroup);
        return true;
    });

    std::sort(aGroups.begin(), aGroups.end(),
        [](const FormulaGroupDescriptor& rLeft, const FormulaGroupDescriptor& rRight) {
            if (rLeft.maAnchor.mnSheet != rRight.maAnchor.mnSheet)
                return rLeft.maAnchor.mnSheet < rRight.maAnchor.mnSheet;
            if (rLeft.maAnchor.mnColumn != rRight.maAnchor.mnColumn)
                return rLeft.maAnchor.mnColumn < rRight.maAnchor.mnColumn;
            if (rLeft.maAnchor.mnRow != rRight.maAnchor.mnRow)
                return rLeft.maAnchor.mnRow < rRight.maAnchor.mnRow;
            if (rLeft.mnLength != rRight.mnLength)
                return rLeft.mnLength < rRight.mnLength;
            return rLeft.mbShareable < rRight.mbShareable;
        });
    return aGroups;
}

enum class SharedFormulaGroupTransitionKind : std::uint8_t
{
    None,
    Preserve,
    Rebuild,
    Split
};

struct SharedFormulaGroupTransition
{
    SharedFormulaGroupTransitionKind meKind = SharedFormulaGroupTransitionKind::None;
    sal_Int32 mnBeforeGroupCount = 0;
    sal_Int32 mnAfterGroupCount = 0;
    bool mbShareableChanged = false;

    [[nodiscard]] constexpr bool operator==(const SharedFormulaGroupTransition& rOther) const
        = default;
};

namespace detail
{

[[nodiscard]] inline std::optional<FormulaGroupDescriptor> shiftFormulaGroupDescriptor(
    const FormulaGroupDescriptor& rDescriptor, const MutationEvent& rMutation)
{
    FormulaGroupDescriptor aShifted = rDescriptor;
    switch (rMutation.meKind)
    {
        case MutationKind::InsertRows:
            if (aShifted.maAnchor.mnSheet == rMutation.mnSheet
                && aShifted.maAnchor.mnRow >= rMutation.maAddress.mnRow)
            {
                aShifted.maAnchor.mnRow = static_cast<api::RowIndex>(
                    aShifted.maAnchor.mnRow + rMutation.mnCount);
            }
            return aShifted;
        case MutationKind::DeleteRows:
            if (aShifted.maAnchor.mnSheet == rMutation.mnSheet)
            {
                if (aShifted.maAnchor.mnRow >= rMutation.maAddress.mnRow
                    && aShifted.maAnchor.mnRow < rMutation.maAddress.mnRow + rMutation.mnCount)
                {
                    return std::nullopt;
                }
                if (aShifted.maAnchor.mnRow >= rMutation.maAddress.mnRow + rMutation.mnCount)
                {
                    aShifted.maAnchor.mnRow = static_cast<api::RowIndex>(
                        aShifted.maAnchor.mnRow - rMutation.mnCount);
                }
            }
            return aShifted;
        case MutationKind::InsertColumns:
            if (aShifted.maAnchor.mnSheet == rMutation.mnSheet
                && aShifted.maAnchor.mnColumn >= rMutation.maAddress.mnColumn)
            {
                aShifted.maAnchor.mnColumn = static_cast<api::ColumnIndex>(
                    aShifted.maAnchor.mnColumn + rMutation.mnCount);
            }
            return aShifted;
        case MutationKind::DeleteColumns:
            if (aShifted.maAnchor.mnSheet == rMutation.mnSheet)
            {
                if (aShifted.maAnchor.mnColumn >= rMutation.maAddress.mnColumn
                    && aShifted.maAnchor.mnColumn
                           < rMutation.maAddress.mnColumn + rMutation.mnCount)
                {
                    return std::nullopt;
                }
                if (aShifted.maAnchor.mnColumn
                    >= rMutation.maAddress.mnColumn + rMutation.mnCount)
                {
                    aShifted.maAnchor.mnColumn = static_cast<api::ColumnIndex>(
                        aShifted.maAnchor.mnColumn - rMutation.mnCount);
                }
            }
            return aShifted;
        default:
            return aShifted;
    }
}

} // namespace detail

[[nodiscard]] inline SharedFormulaGroupTransition classifyFormulaGroupTransition(
    std::vector<FormulaGroupDescriptor> aBeforeGroups,
    std::vector<FormulaGroupDescriptor> aAfterGroups,
    const std::optional<MutationEvent>& oMutation = std::nullopt)
{
    SharedFormulaGroupTransition aTransition;
    aTransition.mnBeforeGroupCount = static_cast<sal_Int32>(aBeforeGroups.size());
    aTransition.mnAfterGroupCount = static_cast<sal_Int32>(aAfterGroups.size());

    auto lSort = [](std::vector<FormulaGroupDescriptor>& rGroups) {
        std::sort(rGroups.begin(), rGroups.end(),
            [](const FormulaGroupDescriptor& rLeft, const FormulaGroupDescriptor& rRight) {
                if (rLeft.maAnchor.mnSheet != rRight.maAnchor.mnSheet)
                    return rLeft.maAnchor.mnSheet < rRight.maAnchor.mnSheet;
                if (rLeft.maAnchor.mnColumn != rRight.maAnchor.mnColumn)
                    return rLeft.maAnchor.mnColumn < rRight.maAnchor.mnColumn;
                if (rLeft.maAnchor.mnRow != rRight.maAnchor.mnRow)
                    return rLeft.maAnchor.mnRow < rRight.maAnchor.mnRow;
                if (rLeft.mnLength != rRight.mnLength)
                    return rLeft.mnLength < rRight.mnLength;
                return rLeft.mbShareable < rRight.mbShareable;
            });
    };

    std::vector<FormulaGroupDescriptor> aExpectedGroups;
    aExpectedGroups.reserve(aBeforeGroups.size());
    for (const auto& rGroup : aBeforeGroups)
    {
        const auto oShifted = oMutation ? detail::shiftFormulaGroupDescriptor(rGroup, *oMutation)
                                        : std::optional<FormulaGroupDescriptor>(rGroup);
        if (!oShifted)
            continue;
        aExpectedGroups.push_back(*oShifted);
    }

    lSort(aExpectedGroups);
    lSort(aAfterGroups);

    if (aExpectedGroups.empty() && aAfterGroups.empty())
        return aTransition;

    if (aExpectedGroups == aAfterGroups)
    {
        aTransition.meKind = SharedFormulaGroupTransitionKind::Preserve;
        return aTransition;
    }

    const auto bSameCount = aExpectedGroups.size() == aAfterGroups.size();
    bool bAnyShareableChanged = false;
    if (bSameCount)
    {
        for (std::size_t nIndex = 0; nIndex < aExpectedGroups.size(); ++nIndex)
        {
            if (aExpectedGroups[nIndex].mbShareable != aAfterGroups[nIndex].mbShareable)
            {
                bAnyShareableChanged = true;
                break;
            }
        }
    }
    aTransition.mbShareableChanged = bAnyShareableChanged;

    if (aAfterGroups.empty() || aAfterGroups.size() < aExpectedGroups.size())
    {
        aTransition.meKind = SharedFormulaGroupTransitionKind::Split;
        return aTransition;
    }

    aTransition.meKind = SharedFormulaGroupTransitionKind::Rebuild;
    return aTransition;
}

/// Scan all formula cells and summarize shared-formula groups.
/// This is a low-risk direct consumer that uses the facade's group
/// descriptor query.
[[nodiscard]] inline SharedFormulaGroupSummary
summarizeFormulaGroups(const WorkbookFacade& rFacade)
{
    SharedFormulaGroupSummary aSummary;
    for (const auto& rGroup : collectFormulaGroupDescriptors(rFacade))
    {
        ++aSummary.mnGroupCount;
        aSummary.mnTotalGroupLength += rGroup.mnLength;
        if (rGroup.mbShareable)
            ++aSummary.mnShareableGroups;
    }

    return aSummary;
}

/// Result of a named-range shadow inventory.
struct NamedRangeInventory
{
    sal_Int32 mnGlobalCount = 0;
    sal_Int32 mnSheetLocalCount = 0;
};

/// Inventory all named ranges through the facade.
[[nodiscard]] inline NamedRangeInventory
inventoryNamedRanges(const WorkbookFacade& rFacade)
{
    NamedRangeInventory aInventory;

    const auto aRanges = rFacade.getNamedRangeDescriptors();
    for (const auto& rRange : aRanges)
    {
        if (rRange.meScope == NamedRangeScope::Global)
            ++aInventory.mnGlobalCount;
        else
            ++aInventory.mnSheetLocalCount;
    }

    return aInventory;
}

/// Snapshot comparison result for differential validation.
struct SnapshotComparison
{
    bool mbSheetCountMatch = false;
    bool mbFormulaCellCountMatch = false;
    bool mbFullMatch = false;
};

/// Compare two facade snapshots for differential validation.
[[nodiscard]] inline SnapshotComparison
compareSnapshots(const WorkbookSnapshotInfo& rLeft, const WorkbookSnapshotInfo& rRight)
{
    SnapshotComparison aResult;
    aResult.mbSheetCountMatch = (rLeft.mnSheetCount == rRight.mnSheetCount);
    aResult.mbFormulaCellCountMatch
        = (rLeft.mnFormulaCellCount == rRight.mnFormulaCellCount);
    aResult.mbFullMatch = (rLeft == rRight);
    return aResult;
}

/// Collect formula source strings for a representative corpus, suitable
/// for feeding into compile-diff or shadow compiler harnesses.
[[nodiscard]] inline std::vector<std::pair<api::CellAddress, api::String>>
collectFormulaCorpus(const WorkbookFacade& rFacade, sal_Int32 nMaxFormulas = -1)
{
    std::vector<std::pair<api::CellAddress, api::String>> aCorpus;
    sal_Int32 nCollected = 0;

    rFacade.visitAllFormulaCells(
        [&aCorpus, &nCollected, nMaxFormulas](const FormulaCellDescriptor& rDesc) {
            aCorpus.emplace_back(rDesc.maId.maAddress, rDesc.maFormulaSource);
            ++nCollected;
            if (nMaxFormulas >= 0 && nCollected >= nMaxFormulas)
                return false;
            return true;
        });

    return aCorpus;
}

} // namespace spreadsheetengine::detail::facade::consumers

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
