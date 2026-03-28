/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

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

/// Scan all formula cells and summarize shared-formula groups.
/// This is a low-risk direct consumer that uses the facade's group
/// descriptor query.
[[nodiscard]] inline SharedFormulaGroupSummary
summarizeFormulaGroups(const WorkbookFacade& rFacade)
{
    SharedFormulaGroupSummary aSummary;

    // Track seen group anchors to avoid double-counting.
    struct AnchorKey
    {
        api::SheetId mnSheet;
        api::ColumnIndex mnColumn;
        api::RowIndex mnRow;
        bool operator==(const AnchorKey& rOther) const = default;
    };
    std::vector<AnchorKey> aSeenAnchors;

    rFacade.visitAllFormulaCells([&](const FormulaCellDescriptor& rDesc) {
        auto oGroup = rFacade.getFormulaGroupDescriptor(rDesc.maId.maAddress);
        if (!oGroup)
            return true;

        AnchorKey aKey { oGroup->maAnchor.mnSheet,
            oGroup->maAnchor.mnColumn, oGroup->maAnchor.mnRow };

        // Check if we've already counted this group.
        bool bSeen = false;
        for (const auto& rSeen : aSeenAnchors)
        {
            if (rSeen == aKey)
            {
                bSeen = true;
                break;
            }
        }

        if (!bSeen)
        {
            aSeenAnchors.push_back(aKey);
            ++aSummary.mnGroupCount;
            aSummary.mnTotalGroupLength += oGroup->mnLength;
            if (oGroup->mbShareable)
                ++aSummary.mnShareableGroups;
        }

        return true;
    });

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
