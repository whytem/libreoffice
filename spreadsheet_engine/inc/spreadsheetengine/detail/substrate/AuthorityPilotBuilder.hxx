/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>

#include <spreadsheetengine/api/SharedFormula.hxx>
#include <spreadsheetengine/detail/SharedFormulaToken.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/substrate/AuthorityPilot.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowMapping.hxx>
#include <spreadsheetengine/detail/workbook/InMemoryWorkbookFacade.hxx>

namespace spreadsheetengine::detail::substrate
{

namespace authoritybuilddetail
{

struct AddressLess
{
    [[nodiscard]] bool operator()(const api::CellAddress& rLeft, const api::CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

struct RangeLess
{
    [[nodiscard]] bool operator()(const api::CellRange& rLeft, const api::CellRange& rRight) const
    {
        if (!(rLeft.maStart == rRight.maStart))
            return AddressLess {}(rLeft.maStart, rRight.maStart);
        return AddressLess {}(rLeft.maEnd, rRight.maEnd);
    }
};

class FacadeCompileHost
{
    const facade::InMemoryWorkbookFacade& mrFacade;

public:
    explicit FacadeCompileHost(const facade::InMemoryWorkbookFacade& rFacade)
        : mrFacade(rFacade)
    {
    }

    [[nodiscard]] std::optional<api::SheetId> lookupSheetId(api::StringView rSheetName) const
    {
        return mrFacade.findSheetId(rSheetName);
    }

    [[nodiscard]] std::optional<token::NameData> lookupRangeName(
        api::StringView rName, std::optional<api::SheetId> onSheet,
        const compiler::CompileContext&) const
    {
        const auto oDescriptor = mrFacade.findNamedRange(rName, onSheet);
        if (!oDescriptor)
            return std::nullopt;

        return token::NameData { oDescriptor->maId.isGlobal()
                ? static_cast<std::int16_t>(-1)
                : static_cast<std::int16_t>(*oDescriptor->maId.moSheet),
            static_cast<std::uint16_t>(oDescriptor->maId.mnIndex) };
    }

    [[nodiscard]] std::optional<token::DatabaseRangeData> lookupDatabaseRange(
        api::StringView, const compiler::CompileContext&) const
    {
        return std::nullopt;
    }

    [[nodiscard]] std::optional<token::TableRefData> lookupTableReference(
        api::StringView, api::StringView, const compiler::CompileContext&) const
    {
        return std::nullopt;
    }

    [[nodiscard]] std::optional<api::refdata::SingleRefData> lookupColRowName(
        api::StringView, const compiler::CompileContext&) const
    {
        return std::nullopt;
    }

    [[nodiscard]] std::optional<token::ExternalNameData> lookupExternalName(
        api::StringView, const compiler::CompileContext&) const
    {
        return std::nullopt;
    }
};

[[nodiscard]] inline facade::InMemoryWorkbookFacade
materializeFacadeFromComputationalShadow(const ComputationalWorkbookShadow& rShadow)
{
    facade::InMemoryWorkbookFacade aFacade;
    aFacade.setGrammar(rShadow.maGrammar);
    aFacade.setGeneration(rShadow.maSnapshot.mnGeneration);

    auto aSheets = rShadow.maSheets;
    std::sort(aSheets.begin(), aSheets.end(),
        [](const ComputationalSheetShadow& rLeft, const ComputationalSheetShadow& rRight) {
            return rLeft.maSheet.mnId < rRight.maSheet.mnId;
        });
    for (const auto& rSheet : aSheets)
        aFacade.addSheet(rSheet.maSheet.maName, rSheet.maSheet.mbHidden);

    for (const auto& rSheet : aSheets)
    {
        for (const auto& rCell : rSheet.maCells)
        {
            if (rCell.moFormula)
            {
                aFacade.setFormulaCell(rCell.maId.maAddress, rCell.moFormula->maFormulaSource,
                    rCell.moFormula->maCachedValue, rCell.moFormula->meKind,
                    rCell.moFormula->mbDirty, rCell.moFormula->mbNeedsRecalc);
            }
            else if (rCell.maCell.meKind == facade::CellKind::Scalar)
            {
                aFacade.setCell(rCell.maId.maAddress, rCell.maCell.maValue);
            }
        }
    }

    for (const auto& rNamedRange : rShadow.maNamedRanges)
    {
        aFacade.addNamedRange(rNamedRange.maName, rNamedRange.moScopeSheet,
            rNamedRange.maBaseAddress, rNamedRange.maTargetExpression);
    }

    for (const auto& rGroup : rShadow.maFormulaGroups)
        aFacade.addFormulaGroup(rGroup.maId.maAnchor, rGroup.maId.mnLength, rGroup.maDescriptor.mbShareable);

    return aFacade;
}

[[nodiscard]] inline bool applyAuthorityMutationToFacade(
    facade::InMemoryWorkbookFacade& rFacade, const AuthorityPilotInput& rInput, api::String& rReason)
{
    switch (rInput.maMutation.meKind)
    {
        case facade::MutationKind::SetScalarValue:
            if (!rInput.moScalarValueAfter)
            {
                rReason = u"missing_scalar_value_after";
                return false;
            }
            rFacade.setCell(rInput.maMutation.maAddress, *rInput.moScalarValueAfter);
            return true;
        case facade::MutationKind::SetFormula:
            rFacade.setFormulaCell(rInput.maMutation.maAddress, rInput.maMutation.maText,
                rInput.moFormulaCachedValueAfter.value_or(api::CellValue::number(0.0)));
            return true;
        case facade::MutationKind::ClearCell:
            rFacade.clearCell(rInput.maMutation.maAddress);
            return true;
        default:
            rReason = u"mutation_not_supported";
            return false;
    }
}

inline void normalizeShadowFormulaGroups(std::vector<ShadowFormulaGroupRecord>& rGroups)
{
    for (auto& rGroup : rGroups)
        graphmapping::sortAndUnique(rGroup.maMembers, graphmapping::ShadowCellIdLess {});

    std::sort(rGroups.begin(), rGroups.end(),
        [](const ShadowFormulaGroupRecord& rLeft, const ShadowFormulaGroupRecord& rRight) {
            return graphmapping::ShadowFormulaGroupIdLess {}(rLeft.maId, rRight.maId);
        });
}

[[nodiscard]] inline std::vector<api::CellAddress> collectShadowCellAddresses(
    const ComputationalWorkbookShadow& rShadow)
{
    std::vector<api::CellAddress> aAddresses;
    aAddresses.reserve(rShadow.getCellCount());
    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
            aAddresses.push_back(rCell.maId.maAddress);
    }

    std::sort(aAddresses.begin(), aAddresses.end(), AddressLess {});
    return aAddresses;
}

[[nodiscard]] inline ShadowCellRecord* findMutableShadowCell(
    ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    for (auto& rSheet : rShadow.maSheets)
    {
        for (auto& rCell : rSheet.maCells)
        {
            if (rCell.maId.maAddress == rAddress)
                return &rCell;
        }
    }

    return nullptr;
}

[[nodiscard]] inline ShadowCellRecord* findOrCreateMutableShadowCell(
    ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    if (auto* pCell = findMutableShadowCell(rShadow, rAddress))
        return pCell;

    for (auto& rSheet : rShadow.maSheets)
    {
        if (rSheet.maSheet.mnId != rAddress.mnSheet)
            continue;

        ShadowCellRecord aCell;
        aCell.maId = { rAddress };
        aCell.maCell.maAddress = rAddress;
        aCell.maCell.meKind = facade::CellKind::Empty;
        aCell.maCell.maValue = api::CellValue::number(0.0);
        aCell.maCell.mbHasFormula = false;
        rSheet.maCells.push_back(aCell);
        return &rSheet.maCells.back();
    }

    return nullptr;
}

[[nodiscard]] inline const ShadowCellRecord* findShadowCell(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    return rShadow.findCell(rAddress);
}

[[nodiscard]] inline const ShadowFormulaGroupRecord* findFormulaGroupContainingAddress(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    auto it = std::find_if(rShadow.maFormulaGroups.begin(), rShadow.maFormulaGroups.end(),
        [&rAddress](const ShadowFormulaGroupRecord& rGroup) {
            return std::find_if(rGroup.maMembers.begin(), rGroup.maMembers.end(),
                       [&rAddress](const ShadowCellId& rMember) {
                           return rMember.maAddress == rAddress;
                       })
                   != rGroup.maMembers.end();
        });
    return it == rShadow.maFormulaGroups.end() ? nullptr : &*it;
}

[[nodiscard]] inline bool isNonStructuralSharedGroupMutation(
    const facade::MutationEvent& rMutation)
{
    switch (rMutation.meKind)
    {
        case facade::MutationKind::SetScalarValue:
        case facade::MutationKind::SetFormula:
        case facade::MutationKind::ClearCell:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] inline bool isAdmittedNonMatrixFormulaCell(const ShadowCellRecord& rCell)
{
    return rCell.hasFormula()
           && rCell.moFormula->meKind != facade::FormulaCellKind::MatrixOrigin
           && rCell.moFormula->meKind != facade::FormulaCellKind::MatrixMember;
}

[[nodiscard]] inline bool sortShadowForComparison(ComputationalWorkbookShadow& rShadow)
{
    for (auto& rSheet : rShadow.maSheets)
    {
        std::sort(rSheet.maCells.begin(), rSheet.maCells.end(),
            [](const ShadowCellRecord& rLeft, const ShadowCellRecord& rRight) {
                return AddressLess {}(rLeft.maId.maAddress, rRight.maId.maAddress);
            });
    }

    normalizeShadowFormulaGroups(rShadow.maFormulaGroups);
    std::sort(rShadow.maNamedRanges.begin(), rShadow.maNamedRanges.end(),
        [](const facade::NamedRangeDescriptor& rLeft, const facade::NamedRangeDescriptor& rRight) {
            if (rLeft.maId.moSheet != rRight.maId.moSheet)
                return rLeft.maId.moSheet < rRight.maId.moSheet;
            return rLeft.maId.mnIndex < rRight.maId.mnIndex;
        });
    return true;
}

[[nodiscard]] inline std::vector<facade::NamedRangeDescriptor> sortNamedRangesForComparison(
    std::vector<facade::NamedRangeDescriptor> aNamedRanges)
{
    std::sort(aNamedRanges.begin(), aNamedRanges.end(),
        [](const facade::NamedRangeDescriptor& rLeft, const facade::NamedRangeDescriptor& rRight) {
            if (rLeft.maId.moSheet != rRight.maId.moSheet)
                return rLeft.maId.moSheet < rRight.maId.moSheet;
            return rLeft.maId.mnIndex < rRight.maId.mnIndex;
        });
    return aNamedRanges;
}

inline void overlayObservedCellPayloadsPreservingGroupBindings(
    ComputationalWorkbookShadow& rPredicted, const ComputationalWorkbookShadow& rObservedAfter)
{
    std::map<api::CellAddress, const ShadowCellRecord*, AddressLess> aObservedCellsByAddress;
    for (const auto& rSheet : rObservedAfter.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
            aObservedCellsByAddress.emplace(rCell.maId.maAddress, &rCell);
    }

    for (auto& rSheet : rPredicted.maSheets)
    {
        for (auto& rCell : rSheet.maCells)
        {
            const auto itObserved = aObservedCellsByAddress.find(rCell.maId.maAddress);
            if (itObserved == aObservedCellsByAddress.end())
                continue;

            const auto oPredictedFormulaGroup = rCell.moFormulaGroup;
            const auto& rObservedCell = *itObserved->second;
            rCell.maCell = rObservedCell.maCell;
            rCell.moFormula = rObservedCell.moFormula;
            rCell.mbInFormulaTree = rObservedCell.mbInFormulaTree;
            rCell.mbInFormulaTrack = rObservedCell.mbInFormulaTrack;
            rCell.moFormulaGroup = oPredictedFormulaGroup;
        }
    }
}

inline void reapplyPredictedSharedGroupFormulaKinds(
    ComputationalWorkbookShadow& rPredicted)
{
    for (const auto& rGroup : rPredicted.maFormulaGroups)
    {
        for (const auto& rMember : rGroup.maMembers)
        {
            ShadowCellRecord* pCell = findMutableShadowCell(rPredicted, rMember.maAddress);
            if (!pCell || !pCell->moFormula)
                continue;

            pCell->moFormulaGroup = rGroup.maId;
            pCell->moFormula->meKind = facade::FormulaCellKind::SharedGroupMember;
        }
    }

    for (auto& rSheet : rPredicted.maSheets)
    {
        for (auto& rCell : rSheet.maCells)
        {
            if (!rCell.moFormula || rCell.moFormulaGroup.has_value())
                continue;
            if (rCell.moFormula->meKind == facade::FormulaCellKind::MatrixOrigin
                || rCell.moFormula->meKind == facade::FormulaCellKind::MatrixMember)
            {
                continue;
            }

            rCell.moFormula->meKind = facade::FormulaCellKind::Ordinary;
        }
    }
}

struct SharedGroupRebuildWindow
{
    api::RowIndex mnStartRow = 0;
    api::RowIndex mnEndRow = 0;
};

struct SharedGroupGapMergeParticipants
{
    const ShadowFormulaGroupRecord* mpAboveGroup = nullptr;
    const ShadowFormulaGroupRecord* mpBelowGroup = nullptr;

    [[nodiscard]] bool isValid() const
    {
        return mpAboveGroup && mpBelowGroup && mpAboveGroup->maId != mpBelowGroup->maId;
    }
};

enum class SharedGroupOneSidedInsertDirection : std::uint8_t
{
    Upward,
    Downward
};

struct SharedGroupOneSidedInsertParticipants
{
    const ShadowFormulaGroupRecord* mpAdjacentGroup = nullptr;
    SharedGroupOneSidedInsertDirection meDirection
        = SharedGroupOneSidedInsertDirection::Upward;

    [[nodiscard]] bool isValid() const { return mpAdjacentGroup != nullptr; }
};

enum class SharedGroupReplacementMergeDirection : std::uint8_t
{
    Upward,
    Downward
};

struct SharedGroupReplacementMergeParticipants
{
    const ShadowFormulaGroupRecord* mpTouchedGroup = nullptr;
    const ShadowFormulaGroupRecord* mpAdjacentGroup = nullptr;
    SharedGroupReplacementMergeDirection meDirection
        = SharedGroupReplacementMergeDirection::Upward;

    [[nodiscard]] bool isValid() const
    {
        return mpTouchedGroup && mpAdjacentGroup && mpTouchedGroup->maId != mpAdjacentGroup->maId;
    }
};

struct LoweredSharedFormulaCell
{
    api::CellAddress maAddress;
    token::CompiledFormula maFormula;
};

[[nodiscard]] inline std::optional<LoweredSharedFormulaCell> lowerSharedFormulaCellForGrouping(
    const facade::InMemoryWorkbookFacade& rFacade, const ShadowCellRecord& rCell,
    api::String& rReason);

[[nodiscard]] inline api::sharedformula::TokenCompareState compareLoweredSharedFormulaCells(
    const LoweredSharedFormulaCell& rLeft, const LoweredSharedFormulaCell& rRight);

[[nodiscard]] inline bool isShareableSameColumnGroup(
    const ShadowFormulaGroupRecord& rGroup, api::SheetId nSheet, api::ColumnIndex nColumn)
{
    return rGroup.maDescriptor.mbShareable && rGroup.maId.maAnchor.mnSheet == nSheet
           && rGroup.maId.maAnchor.mnColumn == nColumn;
}

[[nodiscard]] inline const ShadowFormulaGroupRecord* findShareableSameColumnGroup(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    const auto* pGroup = findFormulaGroupContainingAddress(rShadow, rAddress);
    if (!pGroup || !isShareableSameColumnGroup(*pGroup, rAddress.mnSheet, rAddress.mnColumn))
        return nullptr;
    return pGroup;
}

[[nodiscard]] inline bool hasAdjacentShareableSameColumnGroup(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    if (rAddress.mnRow > 0)
    {
        const api::CellAddress aAbove { rAddress.mnSheet, rAddress.mnColumn,
            static_cast<api::RowIndex>(rAddress.mnRow - 1) };
        if (findShareableSameColumnGroup(rShadow, aAbove))
            return true;
    }

    const api::CellAddress aBelow { rAddress.mnSheet, rAddress.mnColumn,
        static_cast<api::RowIndex>(rAddress.mnRow + 1) };
    return findShareableSameColumnGroup(rShadow, aBelow);
}

[[nodiscard]] inline std::optional<SharedGroupGapMergeParticipants>
findGapMergeParticipantGroups(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    if (findShadowCell(rShadow, rAddress) || rAddress.mnRow <= 0)
        return std::nullopt;

    const api::CellAddress aAbove { rAddress.mnSheet, rAddress.mnColumn,
        static_cast<api::RowIndex>(rAddress.mnRow - 1) };
    const api::CellAddress aBelow { rAddress.mnSheet, rAddress.mnColumn,
        static_cast<api::RowIndex>(rAddress.mnRow + 1) };
    const auto* pAboveGroup = findShareableSameColumnGroup(rShadow, aAbove);
    const auto* pBelowGroup = findShareableSameColumnGroup(rShadow, aBelow);
    if (!pAboveGroup || !pBelowGroup || pAboveGroup->maId == pBelowGroup->maId)
        return std::nullopt;

    const api::RowIndex nAboveEnd = static_cast<api::RowIndex>(
        pAboveGroup->maId.maAnchor.mnRow + pAboveGroup->maId.mnLength - 1);
    const api::RowIndex nBelowStart = pBelowGroup->maId.maAnchor.mnRow;
    if (nAboveEnd != static_cast<api::RowIndex>(rAddress.mnRow - 1)
        || nBelowStart != static_cast<api::RowIndex>(rAddress.mnRow + 1))
    {
        return std::nullopt;
    }

    return SharedGroupGapMergeParticipants { pAboveGroup, pBelowGroup };
}

[[nodiscard]] inline bool matchesGapMergeObservedAfterGroup(
    const ShadowFormulaGroupRecord& rObservedAfterGroup,
    const SharedGroupGapMergeParticipants& rParticipants, const api::CellAddress& rTouchedAddress)
{
    if (!rParticipants.isValid())
        return false;

    const api::RowIndex nExpectedEnd = static_cast<api::RowIndex>(
        rParticipants.mpBelowGroup->maId.maAnchor.mnRow
        + rParticipants.mpBelowGroup->maId.mnLength - 1);
    return rObservedAfterGroup.maId.maAnchor == rParticipants.mpAboveGroup->maId.maAnchor
           && rObservedAfterGroup.maId.mnLength
                  == static_cast<sal_Int32>(nExpectedEnd
                                            - rParticipants.mpAboveGroup->maId.maAnchor.mnRow
                                            + 1)
           && rTouchedAddress.mnRow >= rObservedAfterGroup.maId.maAnchor.mnRow
           && rTouchedAddress.mnRow <= nExpectedEnd;
}

[[nodiscard]] inline std::optional<SharedGroupOneSidedInsertParticipants>
findOneSidedInsertParticipantGroup(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    if (findShadowCell(rShadow, rAddress))
        return std::nullopt;

    const ShadowFormulaGroupRecord* pAboveGroup = nullptr;
    if (rAddress.mnRow > 0)
    {
        const api::CellAddress aAbove { rAddress.mnSheet, rAddress.mnColumn,
            static_cast<api::RowIndex>(rAddress.mnRow - 1) };
        pAboveGroup = findShareableSameColumnGroup(rShadow, aAbove);
    }
    const api::CellAddress aBelow { rAddress.mnSheet, rAddress.mnColumn,
        static_cast<api::RowIndex>(rAddress.mnRow + 1) };
    const ShadowFormulaGroupRecord* pBelowGroup = findShareableSameColumnGroup(rShadow, aBelow);

    if (pAboveGroup)
    {
        const api::RowIndex nAboveEnd = static_cast<api::RowIndex>(
            pAboveGroup->maId.maAnchor.mnRow + pAboveGroup->maId.mnLength - 1);
        if (nAboveEnd != static_cast<api::RowIndex>(rAddress.mnRow - 1))
            pAboveGroup = nullptr;
    }

    if (pBelowGroup && pBelowGroup->maId.maAnchor.mnRow != static_cast<api::RowIndex>(rAddress.mnRow + 1))
        pBelowGroup = nullptr;

    if ((pAboveGroup && pBelowGroup) || (!pAboveGroup && !pBelowGroup))
        return std::nullopt;

    if (pBelowGroup)
    {
        return SharedGroupOneSidedInsertParticipants {
            pBelowGroup, SharedGroupOneSidedInsertDirection::Upward };
    }

    return SharedGroupOneSidedInsertParticipants { pAboveGroup,
        SharedGroupOneSidedInsertDirection::Downward };
}

[[nodiscard]] inline bool matchesOneSidedInsertObservedAfterGroup(
    const ShadowFormulaGroupRecord& rObservedAfterGroup,
    const SharedGroupOneSidedInsertParticipants& rParticipants,
    const api::CellAddress& rTouchedAddress)
{
    if (!rParticipants.isValid())
        return false;

    const api::RowIndex nObservedEnd = static_cast<api::RowIndex>(
        rObservedAfterGroup.maId.maAnchor.mnRow + rObservedAfterGroup.maId.mnLength - 1);
    const api::RowIndex nAdjacentEnd = static_cast<api::RowIndex>(
        rParticipants.mpAdjacentGroup->maId.maAnchor.mnRow
        + rParticipants.mpAdjacentGroup->maId.mnLength - 1);

    switch (rParticipants.meDirection)
    {
        case SharedGroupOneSidedInsertDirection::Upward:
            return rObservedAfterGroup.maId.maAnchor.mnRow == rTouchedAddress.mnRow
                   && nObservedEnd == nAdjacentEnd
                   && rObservedAfterGroup.maId.mnLength
                          == rParticipants.mpAdjacentGroup->maId.mnLength + 1;
        case SharedGroupOneSidedInsertDirection::Downward:
            return rObservedAfterGroup.maId.maAnchor
                       == rParticipants.mpAdjacentGroup->maId.maAnchor
                   && nObservedEnd == rTouchedAddress.mnRow
                   && rObservedAfterGroup.maId.mnLength
                          == rParticipants.mpAdjacentGroup->maId.mnLength + 1;
    }

    return false;
}

[[nodiscard]] inline std::optional<SharedGroupReplacementMergeParticipants>
findReplacementMergeParticipantGroups(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress)
{
    const auto* pTouchedGroup = findShareableSameColumnGroup(rShadow, rAddress);
    if (!pTouchedGroup)
        return std::nullopt;

    const api::RowIndex nTouchedStart = pTouchedGroup->maId.maAnchor.mnRow;
    const api::RowIndex nTouchedEnd = static_cast<api::RowIndex>(
        pTouchedGroup->maId.maAnchor.mnRow + pTouchedGroup->maId.mnLength - 1);
    const bool bAtAnchor = rAddress.mnRow == nTouchedStart;
    const bool bAtTail = rAddress.mnRow == nTouchedEnd;
    if (!bAtAnchor && !bAtTail)
        return std::nullopt;

    const ShadowFormulaGroupRecord* pAboveGroup = nullptr;
    const ShadowFormulaGroupRecord* pBelowGroup = nullptr;
    if (bAtAnchor && nTouchedStart > 0)
    {
        const api::CellAddress aAbove { rAddress.mnSheet, rAddress.mnColumn,
            static_cast<api::RowIndex>(nTouchedStart - 1) };
        pAboveGroup = findShareableSameColumnGroup(rShadow, aAbove);
        if (pAboveGroup)
        {
            const api::RowIndex nAboveEnd = static_cast<api::RowIndex>(
                pAboveGroup->maId.maAnchor.mnRow + pAboveGroup->maId.mnLength - 1);
            if (nAboveEnd != static_cast<api::RowIndex>(nTouchedStart - 1))
                pAboveGroup = nullptr;
        }
    }

    if (bAtTail)
    {
        const api::CellAddress aBelow { rAddress.mnSheet, rAddress.mnColumn,
            static_cast<api::RowIndex>(nTouchedEnd + 1) };
        pBelowGroup = findShareableSameColumnGroup(rShadow, aBelow);
        if (pBelowGroup && pBelowGroup->maId.maAnchor.mnRow != static_cast<api::RowIndex>(nTouchedEnd + 1))
            pBelowGroup = nullptr;
    }

    if (pAboveGroup && pBelowGroup)
        return std::nullopt;

    if (pAboveGroup)
    {
        return SharedGroupReplacementMergeParticipants {
            pTouchedGroup, pAboveGroup, SharedGroupReplacementMergeDirection::Upward };
    }
    if (pBelowGroup)
    {
        return SharedGroupReplacementMergeParticipants {
            pTouchedGroup, pBelowGroup, SharedGroupReplacementMergeDirection::Downward };
    }

    return std::nullopt;
}

[[nodiscard]] inline bool matchesReplacementMergeObservedAfterGroup(
    const ShadowFormulaGroupRecord& rObservedAfterGroup,
    const SharedGroupReplacementMergeParticipants& rParticipants,
    const api::CellAddress& rTouchedAddress)
{
    if (!rParticipants.isValid())
        return false;

    const api::RowIndex nObservedEnd = static_cast<api::RowIndex>(
        rObservedAfterGroup.maId.maAnchor.mnRow + rObservedAfterGroup.maId.mnLength - 1);
    const api::RowIndex nTouchedStart = rParticipants.mpTouchedGroup->maId.maAnchor.mnRow;
    const api::RowIndex nTouchedEnd = static_cast<api::RowIndex>(
        rParticipants.mpTouchedGroup->maId.maAnchor.mnRow
        + rParticipants.mpTouchedGroup->maId.mnLength - 1);
    const api::RowIndex nAdjacentEnd = static_cast<api::RowIndex>(
        rParticipants.mpAdjacentGroup->maId.maAnchor.mnRow
        + rParticipants.mpAdjacentGroup->maId.mnLength - 1);

    switch (rParticipants.meDirection)
    {
        case SharedGroupReplacementMergeDirection::Upward:
            return rObservedAfterGroup.maId.maAnchor
                       == rParticipants.mpAdjacentGroup->maId.maAnchor
                   && nObservedEnd >= rTouchedAddress.mnRow && nObservedEnd <= nTouchedEnd;
        case SharedGroupReplacementMergeDirection::Downward:
            return nObservedEnd == nAdjacentEnd
                   && rObservedAfterGroup.maId.maAnchor.mnRow >= nTouchedStart
                   && rObservedAfterGroup.maId.maAnchor.mnRow <= rTouchedAddress.mnRow;
    }

    return false;
}

[[nodiscard]] inline bool isDeferredSharedGroupNonStructuralFormulaInsert(
    const AuthorityPilotInput& rInput)
{
    return rInput.mbAllowSharedGroupNonStructuralAdmission
           && rInput.maMutation.meKind == facade::MutationKind::SetFormula
           && !findShadowCell(rInput.maComputationalShadow, rInput.maMutation.maAddress)
           && hasAdjacentShareableSameColumnGroup(
               rInput.maComputationalShadow, rInput.maMutation.maAddress)
           && !findGapMergeParticipantGroups(
                   rInput.maComputationalShadow, rInput.maMutation.maAddress)
                   .has_value()
           && !findOneSidedInsertParticipantGroup(
                   rInput.maComputationalShadow, rInput.maMutation.maAddress)
                   .has_value();
}

[[nodiscard]] inline SharedGroupRebuildWindow determineSharedGroupRebuildWindow(
    const ComputationalWorkbookShadow& rBeforeShadow, const ComputationalWorkbookShadow& rPredicted,
    const facade::InMemoryWorkbookFacade& rFacade, const api::CellAddress& rTouchedAddress,
    bool bAllowRegroupExtension, api::String& rReason)
{
    SharedGroupRebuildWindow aWindow { rTouchedAddress.mnRow, rTouchedAddress.mnRow };

    auto lExtendWithGroup = [&](const ShadowFormulaGroupRecord* pGroup) {
        if (!pGroup)
            return;

        aWindow.mnStartRow = std::min(aWindow.mnStartRow, pGroup->maId.maAnchor.mnRow);
        aWindow.mnEndRow = std::max(aWindow.mnEndRow, static_cast<api::RowIndex>(
                                                        pGroup->maId.maAnchor.mnRow
                                                        + pGroup->maId.mnLength - 1));
    };

    const auto* pTouchedGroup = findShareableSameColumnGroup(rBeforeShadow, rTouchedAddress);
    lExtendWithGroup(pTouchedGroup);
    if (!bAllowRegroupExtension || !pTouchedGroup)
        return aWindow;

    const auto* pTouchedCell = findShadowCell(rPredicted, rTouchedAddress);
    if (!pTouchedCell || !pTouchedCell->hasFormula())
        return aWindow;

    const auto oTouchedLowered = lowerSharedFormulaCellForGrouping(rFacade, *pTouchedCell, rReason);
    if (!oTouchedLowered)
        return aWindow;

    auto lExtendOrdinaryRun = [&](api::RowIndex nRowStart, api::RowIndex nStep) {
        for (api::RowIndex nRow = nRowStart;; nRow = static_cast<api::RowIndex>(nRow + nStep))
        {
            if (nRow < 0)
                break;

            const api::CellAddress aAddress { rTouchedAddress.mnSheet, rTouchedAddress.mnColumn, nRow };
            const auto* pCandidateCell = findShadowCell(rPredicted, aAddress);
            if (!pCandidateCell || !pCandidateCell->hasFormula())
                break;
            if (pCandidateCell->moFormulaGroup.has_value())
                break;

            const auto oCandidateLowered
                = lowerSharedFormulaCellForGrouping(rFacade, *pCandidateCell, rReason);
            if (!oCandidateLowered)
                return false;
            if (compareLoweredSharedFormulaCells(*oTouchedLowered, *oCandidateLowered)
                == api::sharedformula::TokenCompareState::NotEqual)
            {
                break;
            }

            aWindow.mnStartRow = std::min(aWindow.mnStartRow, nRow);
            aWindow.mnEndRow = std::max(aWindow.mnEndRow, nRow);
        }

        return true;
    };

    const api::RowIndex nGroupEnd = static_cast<api::RowIndex>(
        pTouchedGroup->maId.maAnchor.mnRow + pTouchedGroup->maId.mnLength - 1);
    if (rTouchedAddress.mnRow == pTouchedGroup->maId.maAnchor.mnRow
        && !lExtendOrdinaryRun(
            static_cast<api::RowIndex>(pTouchedGroup->maId.maAnchor.mnRow - 1), -1))
    {
        return aWindow;
    }
    if (rTouchedAddress.mnRow == nGroupEnd
        && !lExtendOrdinaryRun(static_cast<api::RowIndex>(nGroupEnd + 1), 1))
    {
        return aWindow;
    }

    return aWindow;
}

[[nodiscard]] inline std::optional<SharedGroupRebuildWindow> determineSharedGroupGapMergeWindow(
    const ComputationalWorkbookShadow& rBeforeShadow, const api::CellAddress& rTouchedAddress)
{
    const auto oParticipants = findGapMergeParticipantGroups(rBeforeShadow, rTouchedAddress);
    if (!oParticipants)
        return std::nullopt;

    SharedGroupRebuildWindow aWindow;
    aWindow.mnStartRow = oParticipants->mpAboveGroup->maId.maAnchor.mnRow;
    aWindow.mnEndRow = static_cast<api::RowIndex>(
        oParticipants->mpBelowGroup->maId.maAnchor.mnRow
        + oParticipants->mpBelowGroup->maId.mnLength - 1);
    return aWindow;
}

[[nodiscard]] inline std::optional<SharedGroupRebuildWindow>
determineSharedGroupOneSidedInsertWindow(
    const ComputationalWorkbookShadow& rBeforeShadow, const api::CellAddress& rTouchedAddress)
{
    const auto oParticipants = findOneSidedInsertParticipantGroup(rBeforeShadow, rTouchedAddress);
    if (!oParticipants)
        return std::nullopt;

    SharedGroupRebuildWindow aWindow;
    const api::RowIndex nAdjacentEnd = static_cast<api::RowIndex>(
        oParticipants->mpAdjacentGroup->maId.maAnchor.mnRow
        + oParticipants->mpAdjacentGroup->maId.mnLength - 1);
    switch (oParticipants->meDirection)
    {
        case SharedGroupOneSidedInsertDirection::Upward:
            aWindow.mnStartRow = rTouchedAddress.mnRow;
            aWindow.mnEndRow = nAdjacentEnd;
            break;
        case SharedGroupOneSidedInsertDirection::Downward:
            aWindow.mnStartRow = oParticipants->mpAdjacentGroup->maId.maAnchor.mnRow;
            aWindow.mnEndRow = rTouchedAddress.mnRow;
            break;
    }

    return aWindow;
}

[[nodiscard]] inline std::optional<SharedGroupRebuildWindow>
determineSharedGroupReplacementMergeWindow(
    const ComputationalWorkbookShadow& rBeforeShadow, const api::CellAddress& rTouchedAddress)
{
    const auto oParticipants = findReplacementMergeParticipantGroups(rBeforeShadow, rTouchedAddress);
    if (!oParticipants)
        return std::nullopt;

    SharedGroupRebuildWindow aWindow;
    const api::RowIndex nTouchedEnd = static_cast<api::RowIndex>(
        oParticipants->mpTouchedGroup->maId.maAnchor.mnRow
        + oParticipants->mpTouchedGroup->maId.mnLength - 1);
    const api::RowIndex nAdjacentEnd = static_cast<api::RowIndex>(
        oParticipants->mpAdjacentGroup->maId.maAnchor.mnRow
        + oParticipants->mpAdjacentGroup->maId.mnLength - 1);
    switch (oParticipants->meDirection)
    {
        case SharedGroupReplacementMergeDirection::Upward:
            aWindow.mnStartRow = oParticipants->mpAdjacentGroup->maId.maAnchor.mnRow;
            aWindow.mnEndRow = nTouchedEnd;
            break;
        case SharedGroupReplacementMergeDirection::Downward:
            aWindow.mnStartRow = oParticipants->mpTouchedGroup->maId.maAnchor.mnRow;
            aWindow.mnEndRow = nAdjacentEnd;
            break;
    }

    return aWindow;
}

[[nodiscard]] inline std::optional<LoweredSharedFormulaCell> lowerSharedFormulaCellForGrouping(
    const facade::InMemoryWorkbookFacade& rFacade, const ShadowCellRecord& rCell,
    api::String& rReason)
{
    if (!isAdmittedNonMatrixFormulaCell(rCell))
    {
        rReason = u"shared_group_non_structural_out_of_contract";
        return std::nullopt;
    }

    const FacadeCompileHost aHost(rFacade);
    compiler::CompileContext aContext;
    aContext.maGrammar = rFacade.getGrammar();
    aContext.maBaseAddress = rCell.maId.maAddress;

    const auto aLowered = compiler::lowerFormulaSource(rCell.moFormula->maFormulaSource, aHost, aContext);
    if (!aLowered)
    {
        rReason = u"shared_group_formula_lowering_failed";
        return std::nullopt;
    }
    if (!aLowered.maFormula.mbShareable)
    {
        rReason = u"shared_group_formula_not_shareable";
        return std::nullopt;
    }

    return LoweredSharedFormulaCell { rCell.maId.maAddress, aLowered.maFormula };
}

[[nodiscard]] inline api::sharedformula::TokenCompareState compareLoweredSharedFormulaCells(
    const LoweredSharedFormulaCell& rLeft, const LoweredSharedFormulaCell& rRight)
{
    if (rLeft.maFormula.mnCodeError != rRight.maFormula.mnCodeError)
        return api::sharedformula::TokenCompareState::NotEqual;

    if (sharedformulatoken::hashSharedFormulaLexicalTokens(rLeft.maFormula.maTokens)
        != sharedformulatoken::hashSharedFormulaLexicalTokens(rRight.maFormula.maTokens))
    {
        return api::sharedformula::TokenCompareState::NotEqual;
    }

    return sharedformulatoken::compareSharedFormulaTokenStreams(
        sharedformulatoken::StreamKind::Lexical, rLeft.maFormula.maTokens, rRight.maFormula.maTokens);
}

inline void clearPredictedSharedGroupBindingsInWindow(ComputationalWorkbookShadow& rPredicted,
    api::SheetId nSheet, api::ColumnIndex nColumn, const SharedGroupRebuildWindow& rWindow)
{
    for (auto& rSheet : rPredicted.maSheets)
    {
        for (auto& rCell : rSheet.maCells)
        {
            const auto& rAddress = rCell.maId.maAddress;
            if (rAddress.mnSheet != nSheet || rAddress.mnColumn != nColumn
                || rAddress.mnRow < rWindow.mnStartRow || rAddress.mnRow > rWindow.mnEndRow)
            {
                continue;
            }

            rCell.moFormulaGroup.reset();
            if (rCell.moFormula
                && rCell.moFormula->meKind != facade::FormulaCellKind::MatrixOrigin
                && rCell.moFormula->meKind != facade::FormulaCellKind::MatrixMember)
            {
                rCell.moFormula->meKind = facade::FormulaCellKind::Ordinary;
            }
        }
    }

    auto aGroupIt = std::remove_if(rPredicted.maFormulaGroups.begin(), rPredicted.maFormulaGroups.end(),
        [nSheet, nColumn, &rWindow](const ShadowFormulaGroupRecord& rGroup) {
            if (rGroup.maId.maAnchor.mnSheet != nSheet || rGroup.maId.maAnchor.mnColumn != nColumn)
                return false;

            const api::RowIndex nGroupEnd
                = static_cast<api::RowIndex>(rGroup.maId.maAnchor.mnRow + rGroup.maId.mnLength - 1);
            return !(nGroupEnd < rWindow.mnStartRow || rGroup.maId.maAnchor.mnRow > rWindow.mnEndRow);
        });
    rPredicted.maFormulaGroups.erase(aGroupIt, rPredicted.maFormulaGroups.end());
}

[[nodiscard]] inline bool rebuildPredictedSharedGroupBindingsInWindow(
    ComputationalWorkbookShadow& rPredicted, const facade::InMemoryWorkbookFacade& rFacade,
    api::SheetId nSheet, api::ColumnIndex nColumn, const SharedGroupRebuildWindow& rWindow,
    api::String& rReason)
{
    std::vector<LoweredSharedFormulaCell> aFormulaCells;
    for (const auto& rSheet : rPredicted.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
        {
            const auto& rAddress = rCell.maId.maAddress;
            if (rAddress.mnSheet != nSheet || rAddress.mnColumn != nColumn
                || rAddress.mnRow < rWindow.mnStartRow || rAddress.mnRow > rWindow.mnEndRow)
            {
                continue;
            }
            if (!rCell.hasFormula())
                continue;

            const auto oLowered = lowerSharedFormulaCellForGrouping(rFacade, rCell, rReason);
            if (!oLowered)
                return false;
            aFormulaCells.push_back(*oLowered);
        }
    }

    std::sort(aFormulaCells.begin(), aFormulaCells.end(),
        [](const LoweredSharedFormulaCell& rLeft, const LoweredSharedFormulaCell& rRight) {
            return AddressLess {}(rLeft.maAddress, rRight.maAddress);
        });

    std::vector<api::CellAddress> aRun;
    auto lFlushRun = [&](std::vector<api::CellAddress>& rRun) {
        if (rRun.size() <= 1)
        {
            rRun.clear();
            return;
        }

        ShadowFormulaGroupRecord aGroup;
        aGroup.maId = { rRun.front(), static_cast<sal_Int32>(rRun.size()) };
        aGroup.maDescriptor.maAnchor = rRun.front();
        aGroup.maDescriptor.mnLength = static_cast<sal_Int32>(rRun.size());
        aGroup.maDescriptor.mbShareable = true;
        for (const auto& rAddress : rRun)
        {
            ShadowCellRecord* pCell = findMutableShadowCell(rPredicted, rAddress);
            if (!pCell || !isAdmittedNonMatrixFormulaCell(*pCell))
            {
                rReason = u"shared_group_topology_out_of_contract";
                rRun.clear();
                return;
            }

            pCell->moFormulaGroup = aGroup.maId;
            pCell->moFormula->meKind = facade::FormulaCellKind::SharedGroupMember;
            aGroup.maMembers.push_back({ rAddress });
        }

        rPredicted.maFormulaGroups.push_back(std::move(aGroup));
        rRun.clear();
    };

    for (std::size_t nIndex = 0; nIndex < aFormulaCells.size(); ++nIndex)
    {
        if (aRun.empty())
        {
            aRun.push_back(aFormulaCells[nIndex].maAddress);
            continue;
        }

        const auto& rPrevious = aFormulaCells[nIndex - 1];
        const auto& rCurrent = aFormulaCells[nIndex];
        const bool bConsecutiveRows
            = rCurrent.maAddress.mnSheet == rPrevious.maAddress.mnSheet
              && rCurrent.maAddress.mnColumn == rPrevious.maAddress.mnColumn
              && rCurrent.maAddress.mnRow
                     == static_cast<api::RowIndex>(rPrevious.maAddress.mnRow + 1);
        const bool bJoinable
            = bConsecutiveRows
              && compareLoweredSharedFormulaCells(rPrevious, rCurrent)
                     != api::sharedformula::TokenCompareState::NotEqual;
        if (!bJoinable)
            lFlushRun(aRun);

        aRun.push_back(rCurrent.maAddress);
    }

    lFlushRun(aRun);
    if (!rReason.empty())
        return false;

    normalizeShadowFormulaGroups(rPredicted.maFormulaGroups);
    return true;
}

[[nodiscard]] inline bool mutateSharedGroupNonStructuralCell(
    ComputationalWorkbookShadow& rPredicted, const AuthorityPilotInput& rInput, api::String& rReason)
{
    switch (rInput.maMutation.meKind)
    {
        case facade::MutationKind::SetScalarValue:
        {
            ShadowCellRecord* pCell = findMutableShadowCell(rPredicted, rInput.maMutation.maAddress);
            if (!pCell || !rInput.moScalarValueAfter)
            {
                rReason = u"missing_scalar_value_after";
                return false;
            }

            pCell->maCell.meKind = facade::CellKind::Scalar;
            pCell->maCell.maValue = *rInput.moScalarValueAfter;
            pCell->maCell.mbHasFormula = false;
            pCell->moFormula.reset();
            pCell->moFormulaGroup.reset();
            pCell->mbInFormulaTree = false;
            pCell->mbInFormulaTrack = false;
            return true;
        }
        case facade::MutationKind::SetFormula:
        {
            ShadowCellRecord* pCell
                = findOrCreateMutableShadowCell(rPredicted, rInput.maMutation.maAddress);
            if (!pCell)
            {
                rReason = u"missing_shared_group_formula_cell";
                return false;
            }

            pCell->maId = { rInput.maMutation.maAddress };
            pCell->maCell.maAddress = rInput.maMutation.maAddress;
            pCell->maCell.meKind = facade::CellKind::Formula;
            pCell->maCell.mbHasFormula = true;
            pCell->maCell.maValue
                = rInput.moFormulaCachedValueAfter.value_or(api::CellValue::number(0.0));
            facade::FormulaCellDescriptor aFormula;
            aFormula.maId = { rInput.maMutation.maAddress };
            aFormula.maCachedValue = pCell->maCell.maValue;
            aFormula.maFormulaSource = rInput.maMutation.maText;
            aFormula.meKind = facade::FormulaCellKind::Ordinary;
            pCell->moFormula = std::move(aFormula);
            pCell->moFormulaGroup.reset();
            pCell->mbInFormulaTree = false;
            pCell->mbInFormulaTrack = false;
            return true;
        }
        case facade::MutationKind::ClearCell:
        {
            for (auto& rSheet : rPredicted.maSheets)
            {
                auto it = std::remove_if(rSheet.maCells.begin(), rSheet.maCells.end(),
                    [&rInput](const ShadowCellRecord& rCell) {
                        return rCell.maId.maAddress == rInput.maMutation.maAddress;
                    });
                if (it == rSheet.maCells.end())
                    continue;

                rSheet.maCells.erase(it, rSheet.maCells.end());
                return true;
            }

            rReason = u"missing_shared_group_formula_cell";
            return false;
        }
        default:
            rReason = u"mutation_not_supported";
            return false;
    }
}

[[nodiscard]] inline bool applyPredictedSharedGroupNonStructuralTopology(
    ComputationalWorkbookShadow& rPredicted, const AuthorityPilotInput& rInput, api::String& rReason)
{
    const auto aFacade = materializeFacadeFromComputationalShadow(rPredicted);
    const auto oGapMergeWindow = determineSharedGroupGapMergeWindow(
        rInput.maComputationalShadow, rInput.maMutation.maAddress);
    const auto oOneSidedInsertParticipants = findOneSidedInsertParticipantGroup(
        rInput.maComputationalShadow, rInput.maMutation.maAddress);
    const auto* pBeforeTouchedGroup
        = findShareableSameColumnGroup(rInput.maComputationalShadow, rInput.maMutation.maAddress);
    const auto* pObservedAfterTouchedGroup
        = rInput.moObservedAfterComputationalShadow
              ? findShareableSameColumnGroup(
                    *rInput.moObservedAfterComputationalShadow, rInput.maMutation.maAddress)
              : nullptr;
    const bool bAllowOneSidedInsertWindow
        = !pBeforeTouchedGroup && pObservedAfterTouchedGroup && oOneSidedInsertParticipants
          && matchesOneSidedInsertObservedAfterGroup(
              *pObservedAfterTouchedGroup, *oOneSidedInsertParticipants,
              rInput.maMutation.maAddress);
    const auto oOneSidedInsertWindow = bAllowOneSidedInsertWindow
                                           ? determineSharedGroupOneSidedInsertWindow(
                                                 rInput.maComputationalShadow,
                                                 rInput.maMutation.maAddress)
                                           : std::nullopt;
    const auto oReplacementMergeParticipants = findReplacementMergeParticipantGroups(
        rInput.maComputationalShadow, rInput.maMutation.maAddress);
    const bool bAllowReplacementMergeWindow
        = pBeforeTouchedGroup && pObservedAfterTouchedGroup
          && pObservedAfterTouchedGroup->maId != pBeforeTouchedGroup->maId
          && oReplacementMergeParticipants
          && matchesReplacementMergeObservedAfterGroup(
              *pObservedAfterTouchedGroup, *oReplacementMergeParticipants,
              rInput.maMutation.maAddress);
    const auto oReplacementMergeWindow = bAllowReplacementMergeWindow
                                             ? determineSharedGroupReplacementMergeWindow(
                                                   rInput.maComputationalShadow,
                                                   rInput.maMutation.maAddress)
                                             : std::nullopt;
    const bool bAllowRegroupExtension
        = rInput.maMutation.meKind == facade::MutationKind::SetFormula && pBeforeTouchedGroup
          && pObservedAfterTouchedGroup && pObservedAfterTouchedGroup->maId != pBeforeTouchedGroup->maId;
    const auto aWindow = oGapMergeWindow
                             ? *oGapMergeWindow
                             : oOneSidedInsertWindow
                                   ? *oOneSidedInsertWindow
                             : oReplacementMergeWindow
                                   ? *oReplacementMergeWindow
                             : determineSharedGroupRebuildWindow(
                                   rInput.maComputationalShadow, rPredicted, aFacade,
                                   rInput.maMutation.maAddress, bAllowRegroupExtension, rReason);
    if (!rReason.empty())
        return false;

    clearPredictedSharedGroupBindingsInWindow(
        rPredicted, rInput.maMutation.maAddress.mnSheet, rInput.maMutation.maAddress.mnColumn, aWindow);

    return rebuildPredictedSharedGroupBindingsInWindow(
        rPredicted, aFacade, rInput.maMutation.maAddress.mnSheet, rInput.maMutation.maAddress.mnColumn,
        aWindow, rReason);
}

[[nodiscard]] inline bool matchesPredictedSharedGroupTopology(
    const ComputationalWorkbookShadow& rPredicted, const ComputationalWorkbookShadow& rObservedAfter)
{
    auto aPredictedGroups = rPredicted.maFormulaGroups;
    auto aObservedGroups = rObservedAfter.maFormulaGroups;
    normalizeShadowFormulaGroups(aPredictedGroups);
    normalizeShadowFormulaGroups(aObservedGroups);
    return collectShadowCellAddresses(rPredicted) == collectShadowCellAddresses(rObservedAfter)
           && aPredictedGroups == aObservedGroups;
}

[[nodiscard]] inline bool isSharedGroupNonStructuralCandidate(
    const AuthorityPilotInput& rInput, const ShadowCellRecord*& rpBeforeCell,
    const ShadowFormulaGroupRecord*& rpTouchedGroup, api::String& rReason)
{
    rpBeforeCell = nullptr;
    rpTouchedGroup = nullptr;

    if (!isNonStructuralSharedGroupMutation(rInput.maMutation))
        return false;

    if (rInput.maMutation.meKind == facade::MutationKind::SetFormula)
    {
        const auto oMergeParticipants = findGapMergeParticipantGroups(
            rInput.maComputationalShadow, rInput.maMutation.maAddress);
        if (oMergeParticipants)
        {
            if (!rInput.mbAllowSharedGroupNonStructuralAdmission)
            {
                rReason = u"shared_group_non_structural_disabled";
                return false;
            }

            if (!rInput.moObservedAfterComputationalShadow)
            {
                rReason = u"missing_shared_group_after_shadow";
                return false;
            }

            const bool bNamedRangesStable
                = sortNamedRangesForComparison(rInput.maComputationalShadow.maNamedRanges)
                  == sortNamedRangesForComparison(
                      rInput.moObservedAfterComputationalShadow->maNamedRanges);
            if (!bNamedRangesStable)
            {
                rReason = u"shared_group_named_range_out_of_contract";
                return false;
            }

            const auto* pObservedAfterCell = findShadowCell(
                *rInput.moObservedAfterComputationalShadow, rInput.maMutation.maAddress);
            const auto* pObservedAfterGroup = findShareableSameColumnGroup(
                *rInput.moObservedAfterComputationalShadow, rInput.maMutation.maAddress);
            if (!pObservedAfterCell || !pObservedAfterCell->hasFormula())
            {
                rReason = u"missing_shared_group_formula_cell";
                return false;
            }

            if (!pObservedAfterCell->moFormulaGroup.has_value() || !pObservedAfterGroup
                || !matchesGapMergeObservedAfterGroup(
                    *pObservedAfterGroup, *oMergeParticipants, rInput.maMutation.maAddress))
            {
                rReason = u"shared_group_non_structural_out_of_contract";
                return false;
            }

            return true;
        }

        const auto oOneSidedInsertParticipants = findOneSidedInsertParticipantGroup(
            rInput.maComputationalShadow, rInput.maMutation.maAddress);
        if (oOneSidedInsertParticipants)
        {
            if (!rInput.mbAllowSharedGroupNonStructuralAdmission)
            {
                rReason = u"shared_group_non_structural_disabled";
                return false;
            }

            if (!rInput.moObservedAfterComputationalShadow)
            {
                rReason = u"missing_shared_group_after_shadow";
                return false;
            }

            const bool bNamedRangesStable
                = sortNamedRangesForComparison(rInput.maComputationalShadow.maNamedRanges)
                  == sortNamedRangesForComparison(
                      rInput.moObservedAfterComputationalShadow->maNamedRanges);
            if (!bNamedRangesStable)
            {
                rReason = u"shared_group_named_range_out_of_contract";
                return false;
            }

            const auto* pObservedAfterCell = findShadowCell(
                *rInput.moObservedAfterComputationalShadow, rInput.maMutation.maAddress);
            const auto* pObservedAfterGroup = findShareableSameColumnGroup(
                *rInput.moObservedAfterComputationalShadow, rInput.maMutation.maAddress);
            if (!pObservedAfterCell || !pObservedAfterCell->hasFormula())
            {
                rReason = u"missing_shared_group_formula_cell";
                return false;
            }

            if (!pObservedAfterCell->moFormulaGroup.has_value() || !pObservedAfterGroup
                || !matchesOneSidedInsertObservedAfterGroup(
                    *pObservedAfterGroup, *oOneSidedInsertParticipants,
                    rInput.maMutation.maAddress))
            {
                rReason = u"shared_group_non_structural_out_of_contract";
                return false;
            }

            return true;
        }
    }

    rpBeforeCell = findShadowCell(rInput.maComputationalShadow, rInput.maMutation.maAddress);
    if (!rpBeforeCell || !isAdmittedNonMatrixFormulaCell(*rpBeforeCell)
        || !rpBeforeCell->moFormulaGroup.has_value())
    {
        return false;
    }

    rpTouchedGroup = findShareableSameColumnGroup(
        rInput.maComputationalShadow, rInput.maMutation.maAddress);
    if (!rpTouchedGroup)
        return false;

    if (!rInput.mbAllowSharedGroupNonStructuralAdmission)
    {
        rReason = u"shared_group_non_structural_disabled";
        return false;
    }

    if (!rInput.moObservedAfterComputationalShadow)
    {
        rReason = u"missing_shared_group_after_shadow";
        return false;
    }

    const bool bNamedRangesStable
        = sortNamedRangesForComparison(rInput.maComputationalShadow.maNamedRanges)
          == sortNamedRangesForComparison(
              rInput.moObservedAfterComputationalShadow->maNamedRanges);
    if (!bNamedRangesStable)
    {
        rReason = u"shared_group_named_range_out_of_contract";
        return false;
    }

    if (!rpTouchedGroup->maDescriptor.mbShareable)
    {
        rReason = u"shared_group_non_structural_out_of_contract";
        return false;
    }

    const auto* pObservedAfterCell = findShadowCell(
        *rInput.moObservedAfterComputationalShadow, rInput.maMutation.maAddress);
    const auto* pObservedAfterGroup = findShareableSameColumnGroup(
        *rInput.moObservedAfterComputationalShadow, rInput.maMutation.maAddress);

    switch (rInput.maMutation.meKind)
    {
        case facade::MutationKind::SetScalarValue:
        case facade::MutationKind::ClearCell:
            if (pObservedAfterCell && pObservedAfterCell->moFormulaGroup.has_value())
            {
                rReason = u"shared_group_non_structural_out_of_contract";
                return false;
            }
            return true;
        case facade::MutationKind::SetFormula:
            if (!pObservedAfterCell || !pObservedAfterCell->hasFormula())
            {
                rReason = u"missing_shared_group_formula_cell";
                return false;
            }

            if (!pObservedAfterCell->moFormulaGroup.has_value())
                return true;

            if (!pObservedAfterGroup)
            {
                rReason = u"shared_group_non_structural_out_of_contract";
                return false;
            }

            if (const auto oReplacementParticipants = findReplacementMergeParticipantGroups(
                    rInput.maComputationalShadow, rInput.maMutation.maAddress);
                oReplacementParticipants && pObservedAfterGroup->maId != rpTouchedGroup->maId
                && matchesReplacementMergeObservedAfterGroup(
                    *pObservedAfterGroup, *oReplacementParticipants, rInput.maMutation.maAddress))
            {
                return true;
            }

            if (pObservedAfterGroup->maId == rpTouchedGroup->maId)
            {
                if (!rpBeforeCell->moFormula
                    || rpBeforeCell->moFormula->maFormulaSource != rInput.maMutation.maText)
                {
                    rReason = u"shared_group_non_structural_out_of_contract";
                    return false;
                }
            }

            return true;
        default:
            return false;
    }
}

[[nodiscard]] inline std::optional<ComputationalWorkbookShadow>
buildPredictedSharedGroupNonStructuralComputationalShadow(
    const AuthorityPilotInput& rInput, api::String& rReason)
{
    const ShadowCellRecord* pBeforeCell = nullptr;
    const ShadowFormulaGroupRecord* pTouchedGroup = nullptr;
    if (!isSharedGroupNonStructuralCandidate(rInput, pBeforeCell, pTouchedGroup, rReason))
        return std::nullopt;

    (void)pBeforeCell;
    (void)pTouchedGroup;
    ComputationalWorkbookShadow aPredicted = rInput.maComputationalShadow;
    aPredicted.maSnapshot = rInput.moObservedAfterComputationalShadow->maSnapshot;
    aPredicted.maCellBroadcasters.clear();
    aPredicted.maAreaBroadcasters.clear();
    aPredicted.maFormulaTree.clear();
    aPredicted.maFormulaTrack.clear();

    if (!mutateSharedGroupNonStructuralCell(aPredicted, rInput, rReason))
        return std::nullopt;
    if (!applyPredictedSharedGroupNonStructuralTopology(aPredicted, rInput, rReason))
    {
        return std::nullopt;
    }

    overlayObservedCellPayloadsPreservingGroupBindings(
        aPredicted, *rInput.moObservedAfterComputationalShadow);
    reapplyPredictedSharedGroupFormulaKinds(aPredicted);
    [[maybe_unused]] const bool bSortedPredicted = sortShadowForComparison(aPredicted);

    if (!matchesPredictedSharedGroupTopology(
            aPredicted, *rInput.moObservedAfterComputationalShadow))
    {
        rReason = u"shared_group_topology_out_of_contract";
        return std::nullopt;
    }

    return aPredicted;
}

[[nodiscard]] inline ComputationalWorkbookShadow applyObservationStateToSharedGroupNonStructuralShadow(
    ComputationalWorkbookShadow aShadow, const ComputationalObservationState& rObservation)
{
    aShadow.maCellBroadcasters = rObservation.maCellBroadcasters;
    aShadow.maAreaBroadcasters = rObservation.maAreaBroadcasters;
    aShadow.maFormulaTree = rObservation.maFormulaTree;
    aShadow.maFormulaTrack = rObservation.maFormulaTrack;
    for (auto& rSheet : aShadow.maSheets)
    {
        for (auto& rCell : rSheet.maCells)
        {
            rCell.mbInFormulaTree
                = detail::containsAddress(rObservation.maFormulaTree, rCell.maId.maAddress);
            rCell.mbInFormulaTrack
                = detail::containsAddress(rObservation.maFormulaTrack, rCell.maId.maAddress);
        }
    }

    [[maybe_unused]] const bool bSortedShadow = sortShadowForComparison(aShadow);
    return aShadow;
}

[[nodiscard]] inline std::vector<api::CellAddress> collectQueueAddresses(
    const dependency::RecalcPlan& rPlan)
{
    std::vector<api::CellAddress> aAddresses;
    aAddresses.reserve(rPlan.maQueue.size());
    for (const auto& rEntry : rPlan.maQueue)
        aAddresses.push_back(rEntry.maAddress);
    return aAddresses;
}

[[nodiscard]] inline ComputationalWorkbookShadow buildAuthorityComputationalShadow(
    const facade::InMemoryWorkbookFacade& rFacade, const ComputationalObservationState& rObservation)
{
    return buildComputationalWorkbookShadow(rFacade, rObservation);
}

[[nodiscard]] inline ExecutionIrCompileArtifacts compileAuthorityFormula(
    const facade::InMemoryWorkbookFacade& rFacade, const ShadowCellRecord& rCell)
{
    ExecutionIrCompileArtifacts aArtifacts;
    if (!rCell.moFormula)
        return aArtifacts;

    const FacadeCompileHost aHost(rFacade);
    compiler::CompileContext aContext;
    aContext.maGrammar = rFacade.getGrammar();
    aContext.maBaseAddress = rCell.maId.maAddress;

    const auto aLowered
        = compiler::lowerFormulaSource(rCell.moFormula->maFormulaSource, aHost, aContext);
    if (!aLowered)
    {
        aArtifacts.maFailureMessage = aLowered.maDetail;
        aArtifacts.mnFailureIndex = static_cast<sal_Int32>(aLowered.mnFailureOffset);
        return aArtifacts;
    }

    aArtifacts.moFormula = aLowered.maFormula;
    return aArtifacts;
}

[[nodiscard]] inline ExecutionIrWorkbookShadow buildAuthorityExecutionIrShadow(
    const ComputationalWorkbookShadow& rShadow, const facade::InMemoryWorkbookFacade& rFacade)
{
    return buildExecutionIrWorkbookShadow(
        rShadow, [&rFacade](const ShadowCellRecord& rCell) {
            return compileAuthorityFormula(rFacade, rCell);
        });
}

inline void collectResolvedDependencySources(const dependency::DependencySnapshot& rSnapshot,
    const dependency::DependencySource& rSource,
    std::vector<facade::NamedRangeId>& rVisitedNamedRanges,
    std::vector<dependency::DependencySource>& rResolved)
{
    switch (rSource.meKind)
    {
        case dependency::DependencySourceKind::Cell:
        case dependency::DependencySourceKind::Range:
            rResolved.push_back(rSource);
            return;
        case dependency::DependencySourceKind::NamedRange:
        {
            if (std::find(rVisitedNamedRanges.begin(), rVisitedNamedRanges.end(),
                    rSource.maNamedRangeId)
                != rVisitedNamedRanges.end())
                return;
            rVisitedNamedRanges.push_back(rSource.maNamedRangeId);

            const auto oNamedRangeNode = rSnapshot.findNamedRangeNode(rSource.maNamedRangeId);
            if (!oNamedRangeNode)
                return;

            for (const auto& rEdge : rSnapshot.getDependencies(*oNamedRangeNode))
                collectResolvedDependencySources(rSnapshot, rEdge.maSource, rVisitedNamedRanges, rResolved);
            return;
        }
        case dependency::DependencySourceKind::OpaqueWorkbook:
            return;
    }
}

[[nodiscard]] inline ComputationalObservationState buildAuthorityObservationState(
    const dependency::DependencySnapshot& rSnapshot, const dependency::RecalcPlan& rPlan)
{
    ComputationalObservationState aObservation;
    aObservation.maFormulaTree = collectQueueAddresses(rPlan);

    std::map<api::CellAddress, std::vector<ListenerAnchorId>, AddressLess> aCellBroadcasters;
    std::map<api::CellRange, std::vector<ListenerAnchorId>, RangeLess> aAreaBroadcasters;

    for (const auto& rNode : rSnapshot.maNodes)
    {
        if (rNode.meKind != dependency::DependencyNodeKind::FormulaCell || !rNode.moOutputAddress)
            continue;

        const auto aListenerAnchor
            = graphmapping::makeGraphFormulaCellListenerAnchorId(*rNode.moOutputAddress);
        std::vector<facade::NamedRangeId> aVisitedNamedRanges;
        std::vector<dependency::DependencySource> aResolvedSources;
        for (const auto& rDependency : rSnapshot.getDependencies(rNode.maId))
            collectResolvedDependencySources(
                rSnapshot, rDependency.maSource, aVisitedNamedRanges, aResolvedSources);

        for (const auto& rSource : aResolvedSources)
        {
            if (rSource.meKind == dependency::DependencySourceKind::Cell)
            {
                aCellBroadcasters[rSource.maCellAddress].push_back(aListenerAnchor);
                continue;
            }

            if (rSource.meKind == dependency::DependencySourceKind::Range)
            {
                aAreaBroadcasters[dependency::detail::normalizeRange(rSource.maCellRange)].push_back(
                    aListenerAnchor);
            }
        }
    }

    for (auto& [rAddress, rListeners] : aCellBroadcasters)
    {
        graphmapping::sortAndUnique(rListeners, graphmapping::ListenerAnchorIdLess {});
        aObservation.maCellBroadcasters.push_back({ rAddress, std::move(rListeners) });
    }

    for (auto& [rRange, rListeners] : aAreaBroadcasters)
    {
        graphmapping::sortAndUnique(rListeners, graphmapping::ListenerAnchorIdLess {});
        aObservation.maAreaBroadcasters.push_back({ rRange, std::move(rListeners) });
    }

    return aObservation;
}

[[nodiscard]] inline DependencyGraphShadow buildAuthorityGraphShadow(
    const ComputationalWorkbookShadow& rShadow, const dependency::DependencySnapshot& rSnapshot,
    const dependency::RecalcPlan&)
{
    DependencyGraphShadow aGraph;
    aGraph.maSnapshot = rShadow.maSnapshot;
    aGraph.maGrammar = rShadow.maGrammar;

    for (const auto& rAddress : rShadow.maFormulaTree)
        aGraph.maFormulaTreeNodes.push_back({ rAddress });
    for (const auto& rAddress : rShadow.maFormulaTrack)
        aGraph.maFormulaTrackNodes.push_back({ rAddress });

    auto aIsInFormulaTree = [&aGraph](const api::CellAddress& rAddress) {
        return std::find(aGraph.maFormulaTreeNodes.begin(), aGraph.maFormulaTreeNodes.end(),
                   ShadowCellId { rAddress })
               != aGraph.maFormulaTreeNodes.end();
    };
    auto aIsInFormulaTrack = [&aGraph](const api::CellAddress& rAddress) {
        return std::find(aGraph.maFormulaTrackNodes.begin(), aGraph.maFormulaTrackNodes.end(),
                   ShadowCellId { rAddress })
               != aGraph.maFormulaTrackNodes.end();
    };

    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;

            GraphFormulaNodeRecord aNode;
            aNode.maId = rCell.maId;
            aNode.moFormulaGroup = rCell.moFormulaGroup;
            aNode.maListenerAnchor
                = graphmapping::makeGraphFormulaCellListenerAnchorId(rCell.maId.maAddress);
            aNode.mbInFormulaTree = aIsInFormulaTree(rCell.maId.maAddress);
            aNode.mbInFormulaTrack = aIsInFormulaTrack(rCell.maId.maAddress);
            aGraph.maFormulaNodes.push_back(aNode);

            GraphListenerAnchorRecord aAnchor;
            aAnchor.maId = aNode.maListenerAnchor;
            aAnchor.moFormulaCell = aNode.maId;
            aAnchor.mbInFormulaTree = aNode.mbInFormulaTree;
            aAnchor.mbInFormulaTrack = aNode.mbInFormulaTrack;
            aGraph.maListenerAnchors.push_back(aAnchor);
        }
    }

    for (const auto& rGroup : rShadow.maFormulaGroups)
    {
        GraphFormulaGroupNodeRecord aGroup;
        aGroup.maId = rGroup.maId;
        aGroup.maListenerAnchor = graphmapping::makeGraphFormulaGroupListenerAnchorId(rGroup.maId);
        aGroup.maMembers = rGroup.maMembers;
        aGraph.maFormulaGroupNodes.push_back(aGroup);

        GraphListenerAnchorRecord aAnchor;
        aAnchor.maId = aGroup.maListenerAnchor;
        aAnchor.moFormulaGroup = aGroup.maId;
        aAnchor.mbInFormulaTree = std::any_of(aGroup.maMembers.begin(), aGroup.maMembers.end(),
            [&aIsInFormulaTree](const ShadowCellId& rId) { return aIsInFormulaTree(rId.maAddress); });
        aAnchor.mbInFormulaTrack = std::any_of(aGroup.maMembers.begin(), aGroup.maMembers.end(),
            [&aIsInFormulaTrack](const ShadowCellId& rId) { return aIsInFormulaTrack(rId.maAddress); });
        aGraph.maListenerAnchors.push_back(aAnchor);
    }

    std::map<BroadcasterNodeId, sal_Int32, graphmapping::BroadcasterNodeIdLess> aBroadcasterCounts;
    for (const auto& rNode : rSnapshot.maNodes)
    {
        if (rNode.meKind != dependency::DependencyNodeKind::FormulaCell || !rNode.moOutputAddress)
            continue;

        const auto aListenerAnchor
            = graphmapping::makeGraphFormulaCellListenerAnchorId(*rNode.moOutputAddress);
        std::vector<facade::NamedRangeId> aVisitedNamedRanges;
        std::vector<dependency::DependencySource> aResolvedSources;
        for (const auto& rDependency : rSnapshot.getDependencies(rNode.maId))
            collectResolvedDependencySources(rSnapshot, rDependency.maSource, aVisitedNamedRanges, aResolvedSources);

        for (const auto& rSource : aResolvedSources)
        {
            BroadcasterNodeId aBroadcaster;
            if (rSource.meKind == dependency::DependencySourceKind::Cell)
                aBroadcaster = BroadcasterNodeId::forCell(rSource.maCellAddress);
            else
                aBroadcaster = BroadcasterNodeId::forArea(
                    dependency::detail::normalizeRange(rSource.maCellRange));

            aGraph.maEdges.push_back({ aBroadcaster, aListenerAnchor });
            ++aBroadcasterCounts[aBroadcaster];
        }
    }

    for (const auto& [rBroadcaster, nCount] : aBroadcasterCounts)
        aGraph.maBroadcasterNodes.push_back({ rBroadcaster, nCount });

    aGraph.maFormulaTreeNodes
        = graphmapping::normalizeFormulaSubsetNodes(std::move(aGraph.maFormulaTreeNodes));
    aGraph.maFormulaTrackNodes
        = graphmapping::normalizeFormulaSubsetNodes(std::move(aGraph.maFormulaTrackNodes));
    std::sort(aGraph.maFormulaNodes.begin(), aGraph.maFormulaNodes.end(),
        [](const GraphFormulaNodeRecord& rLeft, const GraphFormulaNodeRecord& rRight) {
            return graphmapping::ShadowCellIdLess {}(rLeft.maId, rRight.maId);
        });
    std::sort(aGraph.maFormulaGroupNodes.begin(), aGraph.maFormulaGroupNodes.end(),
        [](const GraphFormulaGroupNodeRecord& rLeft, const GraphFormulaGroupNodeRecord& rRight) {
            return graphmapping::ShadowFormulaGroupIdLess {}(rLeft.maId, rRight.maId);
        });
    aGraph.maListenerAnchors
        = graphmapping::normalizeListenerAnchors(std::move(aGraph.maListenerAnchors));
    aGraph.maBroadcasterNodes
        = graphmapping::normalizeBroadcasterNodes(std::move(aGraph.maBroadcasterNodes));
    aGraph.maEdges = graphmapping::normalizeGraphEdges(std::move(aGraph.maEdges));

    return aGraph;
}

} // namespace authoritybuilddetail

[[nodiscard]] inline AuthorityPilotTransition
buildAuthorityPilotTransition(const AuthorityPilotInput& rInput)
{
    AuthorityPilotTransition aTransition;
    aTransition.maInput = rInput;
    aTransition.maContract = authoritydetail::classifyAuthorityMutation(rInput.maMutation);
    aTransition.maVerification = authoritydetail::makeAuthorityVerification(aTransition.maContract);

    if (!aTransition.maContract.isAdmitted())
    {
        aTransition.meVerdict = AuthorityPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"mutation_out_of_contract";
        return aTransition;
    }

    if (aTransition.maContract.mbRequiresCleanBaseline && !rInput.mbCleanBaseline)
    {
        aTransition.meVerdict = AuthorityPilotVerdict::RejectedDirtyBaseline;
        aTransition.maReason = u"dirty_baseline";
        return aTransition;
    }

    if (authoritybuilddetail::isDeferredSharedGroupNonStructuralFormulaInsert(rInput))
    {
        aTransition.meVerdict = AuthorityPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"shared_group_non_structural_out_of_contract";
        return aTransition;
    }

    api::String aSharedGroupReason;
    const auto oPredictedSharedGroupShadow
        = authoritybuilddetail::buildPredictedSharedGroupNonStructuralComputationalShadow(
            rInput, aSharedGroupReason);
    if (!oPredictedSharedGroupShadow && !aSharedGroupReason.empty())
    {
        aTransition.meVerdict = AuthorityPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = aSharedGroupReason;
        return aTransition;
    }

    facade::InMemoryWorkbookFacade aFacade;
    if (oPredictedSharedGroupShadow)
    {
        aFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
            *oPredictedSharedGroupShadow);
        aFacade.setGeneration(oPredictedSharedGroupShadow->maSnapshot.mnGeneration);
    }
    else
    {
        aFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
            rInput.maComputationalShadow);
        aFacade.setGeneration(rInput.maComputationalShadow.maSnapshot.mnGeneration + 1);

        if (!authoritybuilddetail::applyAuthorityMutationToFacade(aFacade, rInput, aTransition.maReason))
        {
            aTransition.meVerdict = AuthorityPilotVerdict::RejectedOutOfContract;
            return aTransition;
        }
    }

    aTransition.maDependencySnapshot = dependency::buildDependencySnapshot(aFacade);
    if (aTransition.maDependencySnapshot.maReport.mnOpaqueNodeCount > 0
        || aTransition.maDependencySnapshot.maReport.mnOpaqueEdgeCount > 0)
    {
        aTransition.meVerdict = AuthorityPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"opaque_dependency_surface";
        return aTransition;
    }

    aTransition.maInvalidationPlan
        = dependency::planInvalidation(aTransition.maDependencySnapshot, rInput.maMutation);
    aTransition.maRecalcPlan
        = dependency::buildRecalcPlan(aTransition.maDependencySnapshot, aTransition.maInvalidationPlan);
    const auto aObservation = authoritybuilddetail::buildAuthorityObservationState(
        aTransition.maDependencySnapshot, aTransition.maRecalcPlan);
    if (oPredictedSharedGroupShadow)
    {
        aTransition.maComputationalAfter
            = authoritybuilddetail::applyObservationStateToSharedGroupNonStructuralShadow(
                *oPredictedSharedGroupShadow, aObservation);
    }
    else
    {
        aTransition.maComputationalAfter
            = authoritybuilddetail::buildAuthorityComputationalShadow(aFacade, aObservation);
    }
    aTransition.maGraphAfter = authoritybuilddetail::buildAuthorityGraphShadow(
        aTransition.maComputationalAfter, aTransition.maDependencySnapshot, aTransition.maRecalcPlan);
    auto aIrFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
        aTransition.maComputationalAfter);
    aTransition.maIrAfter
        = authoritybuilddetail::buildAuthorityExecutionIrShadow(
            aTransition.maComputationalAfter, aIrFacade);
    aTransition.meVerdict = AuthorityPilotVerdict::Applicable;
    aTransition.maReason = u"ready";
    return aTransition;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
