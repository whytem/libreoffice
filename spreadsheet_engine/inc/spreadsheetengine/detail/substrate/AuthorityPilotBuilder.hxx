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

[[nodiscard]] inline bool mutationChangesFormulaSource(
    const ShadowCellRecord& rCell, const facade::MutationEvent& rMutation)
{
    return rCell.moFormula && rCell.moFormula->maFormulaSource != rMutation.maText;
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

[[nodiscard]] inline bool buildSharedGroupRunsFromSurvivingMembers(
    const ShadowFormulaGroupRecord& rGroup, const api::CellAddress& rTouchedAddress,
    std::vector<ShadowFormulaGroupRecord>& rRuns)
{
    std::vector<ShadowCellId> aMembers = rGroup.maMembers;
    graphmapping::sortAndUnique(aMembers, graphmapping::ShadowCellIdLess {});
    if (aMembers.size() != rGroup.maMembers.size()
        || static_cast<sal_Int32>(aMembers.size()) != rGroup.maId.mnLength)
    {
        return false;
    }

    rRuns.clear();
    for (std::size_t nIndex = 0; nIndex < aMembers.size();)
    {
        if (aMembers[nIndex].maAddress == rTouchedAddress)
        {
            ++nIndex;
            continue;
        }

        std::vector<ShadowCellId> aRun;
        aRun.push_back(aMembers[nIndex]);
        ++nIndex;

        while (nIndex < aMembers.size())
        {
            if (aMembers[nIndex].maAddress == rTouchedAddress)
            {
                ++nIndex;
                break;
            }

            const auto& rPrevious = aRun.back().maAddress;
            const auto& rCurrent = aMembers[nIndex].maAddress;
            if (rCurrent.mnSheet != rPrevious.mnSheet || rCurrent.mnColumn != rPrevious.mnColumn
                || rCurrent.mnRow != static_cast<api::RowIndex>(rPrevious.mnRow + 1))
            {
                break;
            }

            aRun.push_back(aMembers[nIndex]);
            ++nIndex;
        }

        if (aRun.size() <= 1)
            continue;

        ShadowFormulaGroupRecord aRunGroup;
        aRunGroup.maId = { aRun.front().maAddress, static_cast<sal_Int32>(aRun.size()) };
        aRunGroup.maDescriptor = rGroup.maDescriptor;
        aRunGroup.maDescriptor.maAnchor = aRun.front().maAddress;
        aRunGroup.maDescriptor.mnLength = static_cast<sal_Int32>(aRun.size());
        aRunGroup.maMembers = std::move(aRun);
        rRuns.push_back(std::move(aRunGroup));
    }

    normalizeShadowFormulaGroups(rRuns);
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
            ShadowCellRecord* pCell = findMutableShadowCell(rPredicted, rInput.maMutation.maAddress);
            if (!pCell)
            {
                rReason = u"missing_shared_group_formula_cell";
                return false;
            }

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
    ComputationalWorkbookShadow& rPredicted, const ShadowFormulaGroupRecord& rTouchedGroup,
    const api::CellAddress& rTouchedAddress)
{
    for (const auto& rMember : rTouchedGroup.maMembers)
    {
        if (ShadowCellRecord* pCell = findMutableShadowCell(rPredicted, rMember.maAddress))
        {
            pCell->moFormulaGroup.reset();
            if (pCell->moFormula
                && pCell->moFormula->meKind != facade::FormulaCellKind::MatrixOrigin
                && pCell->moFormula->meKind != facade::FormulaCellKind::MatrixMember)
            {
                pCell->moFormula->meKind = facade::FormulaCellKind::Ordinary;
            }
        }
    }

    auto aGroupIt = std::remove_if(rPredicted.maFormulaGroups.begin(), rPredicted.maFormulaGroups.end(),
        [&rTouchedGroup](const ShadowFormulaGroupRecord& rGroup) {
            return rGroup.maId == rTouchedGroup.maId;
        });
    rPredicted.maFormulaGroups.erase(aGroupIt, rPredicted.maFormulaGroups.end());

    std::vector<ShadowFormulaGroupRecord> aRuns;
    if (!buildSharedGroupRunsFromSurvivingMembers(rTouchedGroup, rTouchedAddress, aRuns))
        return false;

    for (const auto& rRun : aRuns)
    {
        for (const auto& rMember : rRun.maMembers)
        {
            ShadowCellRecord* pCell = findMutableShadowCell(rPredicted, rMember.maAddress);
            if (!pCell || !isAdmittedNonMatrixFormulaCell(*pCell))
                return false;

            pCell->moFormulaGroup = rRun.maId;
            pCell->moFormula->meKind = facade::FormulaCellKind::SharedGroupMember;
        }

        rPredicted.maFormulaGroups.push_back(rRun);
    }

    normalizeShadowFormulaGroups(rPredicted.maFormulaGroups);
    return true;
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

    rpBeforeCell = findShadowCell(rInput.maComputationalShadow, rInput.maMutation.maAddress);
    if (!rpBeforeCell || !isAdmittedNonMatrixFormulaCell(*rpBeforeCell)
        || !rpBeforeCell->moFormulaGroup.has_value())
    {
        return false;
    }

    rpTouchedGroup
        = findFormulaGroupContainingAddress(rInput.maComputationalShadow, rInput.maMutation.maAddress);
    if (!rpTouchedGroup)
    {
        rReason = u"shared_group_topology_out_of_contract";
        return false;
    }

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

    if (!rpTouchedGroup->maDescriptor.mbShareable || !rInput.maComputationalShadow.maNamedRanges.empty()
        || !rInput.moObservedAfterComputationalShadow->maNamedRanges.empty())
    {
        rReason = u"shared_group_non_structural_out_of_contract";
        return false;
    }

    if (rInput.maMutation.meKind == facade::MutationKind::SetFormula
        && !mutationChangesFormulaSource(*rpBeforeCell, rInput.maMutation))
    {
        rReason = u"shared_group_non_structural_preserve_out_of_contract";
        return false;
    }

    return true;
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
    ComputationalWorkbookShadow aPredicted = rInput.maComputationalShadow;
    aPredicted.maSnapshot = rInput.moObservedAfterComputationalShadow->maSnapshot;
    aPredicted.maCellBroadcasters.clear();
    aPredicted.maAreaBroadcasters.clear();
    aPredicted.maFormulaTree.clear();
    aPredicted.maFormulaTrack.clear();

    if (!mutateSharedGroupNonStructuralCell(aPredicted, rInput, rReason))
        return std::nullopt;
    if (!applyPredictedSharedGroupNonStructuralTopology(
            aPredicted, *pTouchedGroup, rInput.maMutation.maAddress))
    {
        rReason = u"shared_group_topology_out_of_contract";
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

    auto aFacade = authoritybuilddetail::materializeFacadeFromComputationalShadow(
        rInput.maComputationalShadow);
    aFacade.setGeneration(rInput.maComputationalShadow.maSnapshot.mnGeneration + 1);

    if (!authoritybuilddetail::applyAuthorityMutationToFacade(aFacade, rInput, aTransition.maReason))
    {
        aTransition.meVerdict = AuthorityPilotVerdict::RejectedOutOfContract;
        return aTransition;
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
