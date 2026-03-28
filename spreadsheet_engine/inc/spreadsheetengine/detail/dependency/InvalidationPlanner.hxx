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

#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>

namespace spreadsheetengine::detail::dependency
{

namespace detail
{

inline void addDirtyNode(InvalidationPlan& rPlan, const DependencySnapshot& rSnapshot,
    DependencyNodeId aNodeId, DirtyReason eReason)
{
    if (std::find(rPlan.maDirtyNodeIds.begin(), rPlan.maDirtyNodeIds.end(), aNodeId)
        != rPlan.maDirtyNodeIds.end())
    {
        return;
    }

    rPlan.maDirtyNodeIds.push_back(aNodeId);

    const DependencyNode* pNode = rSnapshot.getNode(aNodeId);
    if (!pNode)
        return;

    if (pNode->meKind == DependencyNodeKind::FormulaCell && pNode->moOutputAddress)
    {
        const auto aIt = std::find_if(rPlan.maDirtyFormulaCells.begin(),
            rPlan.maDirtyFormulaCells.end(),
            [&rAddress = *pNode->moOutputAddress](const DirtyFormulaCell& rEntry) {
                return rEntry.maAddress == rAddress;
            });
        if (aIt == rPlan.maDirtyFormulaCells.end())
            rPlan.maDirtyFormulaCells.push_back({ *pNode->moOutputAddress, eReason });
        return;
    }

    if (pNode->meKind == DependencyNodeKind::NamedRange && pNode->moNamedRangeId)
    {
        const auto aIt = std::find_if(rPlan.maDirtyNamedRanges.begin(),
            rPlan.maDirtyNamedRanges.end(),
            [&rNamedRangeId = *pNode->moNamedRangeId](const DirtyNamedRange& rEntry) {
                return rEntry.maId == rNamedRangeId;
            });
        if (aIt == rPlan.maDirtyNamedRanges.end())
            rPlan.maDirtyNamedRanges.push_back({ *pNode->moNamedRangeId, eReason });
    }
}

inline void addDirtyFormulaAddress(
    InvalidationPlan& rPlan, const api::CellAddress& rAddress, DirtyReason eReason)
{
    const auto aIt = std::find_if(rPlan.maDirtyFormulaCells.begin(), rPlan.maDirtyFormulaCells.end(),
        [&rAddress](const DirtyFormulaCell& rEntry) { return rEntry.maAddress == rAddress; });
    if (aIt == rPlan.maDirtyFormulaCells.end())
        rPlan.maDirtyFormulaCells.push_back({ rAddress, eReason });
}

inline void addRebuildScope(InvalidationPlan& rPlan, RebuildScopeKind eKind,
    api::StringView rReason, facade::SheetId nSheet = 0,
    api::CellAddress aAddress = {})
{
    RebuildScope aScope;
    aScope.meKind = eKind;
    aScope.mnSheet = nSheet;
    aScope.maAddress = aAddress;
    aScope.maReason = api::String(rReason);

    if (std::find(rPlan.maRebuildScopes.begin(), rPlan.maRebuildScopes.end(), aScope)
        == rPlan.maRebuildScopes.end())
    {
        rPlan.maRebuildScopes.push_back(std::move(aScope));
    }
}

[[nodiscard]] inline bool sourceMatchesCellMutation(
    const DependencySource& rSource, const api::CellAddress& rAddress)
{
    switch (rSource.meKind)
    {
        case DependencySourceKind::Cell:
            return rSource.maCellAddress == rAddress;
        case DependencySourceKind::Range:
            return detail::rangeContainsCell(rSource.maCellRange, rAddress);
        case DependencySourceKind::NamedRange:
        case DependencySourceKind::OpaqueWorkbook:
            return false;
    }

    return false;
}

[[nodiscard]] inline bool sourceMatchesRangeMutation(
    const DependencySource& rSource, const api::CellRange& rRange)
{
    switch (rSource.meKind)
    {
        case DependencySourceKind::Cell:
            return detail::rangeContainsCell(rRange, rSource.maCellAddress);
        case DependencySourceKind::Range:
            return detail::rangesIntersect(rSource.maCellRange, rRange);
        case DependencySourceKind::NamedRange:
        case DependencySourceKind::OpaqueWorkbook:
            return false;
    }

    return false;
}

[[nodiscard]] inline bool sourceMatchesNamedRangeMutation(
    const DependencySource& rSource, const facade::MutationEvent& rMutation)
{
    if (rSource.meKind != DependencySourceKind::NamedRange)
        return false;

    if (rMutation.moNamedRangeBefore && rSource.maNamedRangeId == rMutation.moNamedRangeBefore->maId)
        return true;
    if (rMutation.moNamedRangeAfter && rSource.maNamedRangeId == rMutation.moNamedRangeAfter->maId)
        return true;
    return false;
}

[[nodiscard]] inline bool sourceMatchesEvent(
    const DependencySource& rSource, const facade::MutationEvent& rMutation)
{
    switch (rMutation.meKind)
    {
        case facade::MutationKind::SetScalarValue:
        case facade::MutationKind::SetFormula:
        case facade::MutationKind::ClearCell:
            return sourceMatchesCellMutation(rSource, rMutation.maAddress);
        case facade::MutationKind::ClearRange:
            return sourceMatchesRangeMutation(rSource, rMutation.maRange);
        case facade::MutationKind::AddNamedRange:
        case facade::MutationKind::RemoveNamedRange:
        case facade::MutationKind::RenameNamedRange:
            return sourceMatchesNamedRangeMutation(rSource, rMutation);
        case facade::MutationKind::InsertRows:
        case facade::MutationKind::DeleteRows:
        case facade::MutationKind::InsertColumns:
        case facade::MutationKind::DeleteColumns:
        case facade::MutationKind::MoveRange:
        case facade::MutationKind::CopyRange:
        case facade::MutationKind::RenameSheet:
            return rSource.meKind == DependencySourceKind::OpaqueWorkbook;
    }

    return false;
}

[[nodiscard]] inline bool isStructuralMutation(facade::MutationKind eKind)
{
    switch (eKind)
    {
        case facade::MutationKind::InsertRows:
        case facade::MutationKind::DeleteRows:
        case facade::MutationKind::InsertColumns:
        case facade::MutationKind::DeleteColumns:
        case facade::MutationKind::MoveRange:
        case facade::MutationKind::CopyRange:
        case facade::MutationKind::RenameSheet:
            return true;
        case facade::MutationKind::SetScalarValue:
        case facade::MutationKind::SetFormula:
        case facade::MutationKind::ClearCell:
        case facade::MutationKind::ClearRange:
        case facade::MutationKind::AddNamedRange:
        case facade::MutationKind::RemoveNamedRange:
        case facade::MutationKind::RenameNamedRange:
            return false;
    }

    return false;
}

[[nodiscard]] inline DirtyReason directReasonForMutation(facade::MutationKind eKind)
{
    switch (eKind)
    {
        case facade::MutationKind::SetScalarValue:
            return DirtyReason::ScalarValueChanged;
        case facade::MutationKind::SetFormula:
            return DirtyReason::FormulaChanged;
        case facade::MutationKind::ClearCell:
            return DirtyReason::CellCleared;
        case facade::MutationKind::ClearRange:
            return DirtyReason::RangeCleared;
        case facade::MutationKind::AddNamedRange:
        case facade::MutationKind::RemoveNamedRange:
        case facade::MutationKind::RenameNamedRange:
            return DirtyReason::NamedRangeChanged;
        case facade::MutationKind::InsertRows:
        case facade::MutationKind::DeleteRows:
        case facade::MutationKind::InsertColumns:
        case facade::MutationKind::DeleteColumns:
        case facade::MutationKind::MoveRange:
        case facade::MutationKind::CopyRange:
        case facade::MutationKind::RenameSheet:
            return DirtyReason::StructuralMutation;
    }

    return DirtyReason::DirectDependency;
}

} // namespace detail

[[nodiscard]] inline InvalidationPlan planInvalidation(
    const DependencySnapshot& rSnapshot, const facade::MutationEvent& rMutation)
{
    InvalidationPlan aPlan;

    if (detail::isStructuralMutation(rMutation.meKind))
    {
        aPlan.mbRequiresSnapshotRebuild = true;
        aPlan.mbUsedConservativeWidening = true;

        switch (rMutation.meKind)
        {
            case facade::MutationKind::InsertRows:
            case facade::MutationKind::DeleteRows:
            case facade::MutationKind::InsertColumns:
            case facade::MutationKind::DeleteColumns:
            case facade::MutationKind::MoveRange:
            case facade::MutationKind::CopyRange:
            case facade::MutationKind::RenameSheet:
                detail::addRebuildScope(aPlan, RebuildScopeKind::Workbook, u"structural_mutation");
                break;
            default:
                break;
        }

        for (const auto& rNode : rSnapshot.maNodes)
            detail::addDirtyNode(aPlan, rSnapshot, rNode.maId, DirtyReason::StructuralMutation);

        return aPlan;
    }

    const DirtyReason eDirectReason = detail::directReasonForMutation(rMutation.meKind);

    for (const auto& rNode : rSnapshot.maNodes)
    {
        if (rNode.mbOpaqueDependencies)
        {
            aPlan.mbUsedConservativeWidening = true;
            detail::addDirtyNode(aPlan, rSnapshot, rNode.maId, DirtyReason::OpaqueDependency);
            continue;
        }

        const auto& rDependencies = rSnapshot.getDependencies(rNode.maId);
        for (const auto& rDependency : rDependencies)
        {
            if (!detail::sourceMatchesEvent(rDependency.maSource, rMutation))
                continue;

            detail::addDirtyNode(aPlan, rSnapshot, rNode.maId, eDirectReason);
            break;
        }
    }

    if (rMutation.meKind == facade::MutationKind::SetFormula)
    {
        detail::addDirtyFormulaAddress(aPlan, rMutation.maAddress, DirtyReason::FormulaChanged);
        detail::addRebuildScope(
            aPlan, RebuildScopeKind::FormulaCell, u"formula_changed", 0, rMutation.maAddress);
    }
    else if (rMutation.meKind == facade::MutationKind::AddNamedRange
             || rMutation.meKind == facade::MutationKind::RemoveNamedRange
             || rMutation.meKind == facade::MutationKind::RenameNamedRange)
    {
        aPlan.mbRequiresSnapshotRebuild = true;
        detail::addRebuildScope(aPlan, RebuildScopeKind::NamedRanges, u"named_range_mutation");
    }

    for (std::size_t nIndex = 0; nIndex < aPlan.maDirtyNodeIds.size(); ++nIndex)
    {
        const DependencyNode* pDirtyNode = rSnapshot.getNode(aPlan.maDirtyNodeIds[nIndex]);
        if (!pDirtyNode)
            continue;

        for (const auto aDependentId : rSnapshot.getReverseDependents(pDirtyNode->maId))
        {
            detail::addDirtyNode(
                aPlan, rSnapshot, aDependentId, DirtyReason::TransitiveDependency);
        }
    }

    return aPlan;
}

} // namespace spreadsheetengine::detail::dependency

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
