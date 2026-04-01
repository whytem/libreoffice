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
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>

namespace spreadsheetengine::detail::dependency
{

enum class RecalcSeedKind : std::uint8_t
{
    FormulaCell,
    NamedRange,
    StructuralRebuild
};

enum class RecalcGroupPolicy : std::uint8_t
{
    None,
    PreserveSharedGroup,
    SplitSharedGroup
};

struct RecalcSeed
{
    RecalcSeedKind meKind = RecalcSeedKind::FormulaCell;
    DependencyNodeId maNodeId;
    DirtyReason meReason = DirtyReason::DirectDependency;
    std::optional<api::CellAddress> moAddress;
    std::optional<facade::NamedRangeId> moNamedRangeId;
    std::optional<RebuildScopeKind> moRebuildScopeKind;

    [[nodiscard]] constexpr bool operator==(const RecalcSeed& rOther) const = default;
};

struct RecalcQueueEntry
{
    DependencyNodeId maNodeId;
    api::CellAddress maAddress;
    DirtyReason meReason = DirtyReason::DirectDependency;
    sal_Int32 mnOrder = -1;
    sal_Int32 mnDependencyDepth = 0;
    std::optional<api::CellAddress> moSharedGroupAnchor;
    sal_Int32 mnSharedGroupLength = 0;
    RecalcGroupPolicy meGroupPolicy = RecalcGroupPolicy::None;
    bool mbDependsOnDirtyNamedRange = false;
    bool mbOpaqueOrdering = false;

    [[nodiscard]] constexpr bool operator==(const RecalcQueueEntry& rOther) const = default;
};

struct RecalcPlan
{
    std::vector<RecalcSeed> maSeeds;
    std::vector<RecalcQueueEntry> maQueue;
    std::vector<RebuildScope> maRebuildScopes;
    bool mbRequiresSnapshotRebuild = false;
    bool mbUsedConservativeWidening = false;
    bool mbHasOpaqueOrdering = false;

    [[nodiscard]] constexpr bool operator==(const RecalcPlan& rOther) const = default;
};

namespace detail
{

struct DependencyNodeIdHash
{
    [[nodiscard]] std::size_t operator()(DependencyNodeId aNodeId) const
    {
        return static_cast<std::size_t>(aNodeId.mnIndex);
    }
};

struct AddressLess
{
    [[nodiscard]] bool operator()(const api::CellAddress& rLeft,
        const api::CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

[[nodiscard]] inline bool isEarlierNodeAddress(const DependencySnapshot& rSnapshot,
    DependencyNodeId aLeft, DependencyNodeId aRight)
{
    const DependencyNode* pLeft = rSnapshot.getNode(aLeft);
    const DependencyNode* pRight = rSnapshot.getNode(aRight);
    if (!pLeft || !pRight || !pLeft->moOutputAddress || !pRight->moOutputAddress)
        return aLeft.mnIndex < aRight.mnIndex;

    return AddressLess {}(*pLeft->moOutputAddress, *pRight->moOutputAddress);
}

[[nodiscard]] inline std::optional<DirtyReason> findDirtyReasonForFormulaNode(
    const InvalidationPlan& rPlan, const DependencySnapshot& rSnapshot, DependencyNodeId aNodeId)
{
    const DependencyNode* pNode = rSnapshot.getNode(aNodeId);
    if (!pNode || !pNode->moOutputAddress)
        return std::nullopt;

    const auto aIt = std::find_if(rPlan.maDirtyFormulaCells.begin(), rPlan.maDirtyFormulaCells.end(),
        [&rAddress = *pNode->moOutputAddress](const DirtyFormulaCell& rEntry) {
            return rEntry.maAddress == rAddress;
        });
    if (aIt == rPlan.maDirtyFormulaCells.end())
        return std::nullopt;

    return aIt->meReason;
}

[[nodiscard]] inline std::optional<DirtyReason> findDirtyReasonForNamedRangeNode(
    const InvalidationPlan& rPlan, const DependencySnapshot& rSnapshot, DependencyNodeId aNodeId)
{
    const DependencyNode* pNode = rSnapshot.getNode(aNodeId);
    if (!pNode || !pNode->moNamedRangeId)
        return std::nullopt;

    const auto aIt = std::find_if(rPlan.maDirtyNamedRanges.begin(), rPlan.maDirtyNamedRanges.end(),
        [&rId = *pNode->moNamedRangeId](const DirtyNamedRange& rEntry) {
            return rEntry.maId == rId;
        });
    if (aIt == rPlan.maDirtyNamedRanges.end())
        return std::nullopt;

    return aIt->meReason;
}

[[nodiscard]] inline std::vector<DependencyNodeId> collectDirtyFormulaNodes(
    const InvalidationPlan& rPlan, const DependencySnapshot& rSnapshot)
{
    std::vector<DependencyNodeId> aNodes;
    for (const auto& rEntry : rPlan.maDirtyFormulaCells)
    {
        const auto oNodeId = rSnapshot.findFormulaCellNode(rEntry.maAddress);
        if (!oNodeId)
            continue;

        if (std::find(aNodes.begin(), aNodes.end(), *oNodeId) == aNodes.end())
            aNodes.push_back(*oNodeId);
    }
    return aNodes;
}

inline void collectFormulaPredecessorsForSource(const DependencySnapshot& rSnapshot,
    const std::unordered_set<DependencyNodeId, DependencyNodeIdHash>& rDirtyFormulaNodeSet,
    const DependencySource& rSource,
    std::unordered_set<DependencyNodeId, DependencyNodeIdHash>& rPredecessors,
    bool& rDependsOnDirtyNamedRange)
{
    auto addIfDirtyFormulaAddress = [&](const api::CellAddress& rAddress) {
        const auto oDependency = rSnapshot.findFormulaCellNode(rAddress);
        if (!oDependency || !rDirtyFormulaNodeSet.contains(*oDependency))
            return;
        rPredecessors.insert(*oDependency);
    };

    switch (rSource.meKind)
    {
        case DependencySourceKind::Cell:
            addIfDirtyFormulaAddress(rSource.maCellAddress);
            return;
        case DependencySourceKind::Range:
            for (const auto aNodeId : rDirtyFormulaNodeSet)
            {
                const DependencyNode* pNode = rSnapshot.getNode(aNodeId);
                if (!pNode || !pNode->moOutputAddress)
                    continue;
                if (rangeContainsCell(rSource.maCellRange, *pNode->moOutputAddress))
                    rPredecessors.insert(aNodeId);
            }
            return;
        case DependencySourceKind::NamedRange:
        {
            const auto oNamedRangeNode = rSnapshot.findNamedRangeNode(rSource.maNamedRangeId);
            if (!oNamedRangeNode)
                return;

            rDependsOnDirtyNamedRange = true;
            for (const auto& rNamedRangeDependency : rSnapshot.getDependencies(*oNamedRangeNode))
            {
                collectFormulaPredecessorsForSource(rSnapshot, rDirtyFormulaNodeSet,
                    rNamedRangeDependency.maSource, rPredecessors, rDependsOnDirtyNamedRange);
            }
            return;
        }
        case DependencySourceKind::OpaqueWorkbook:
            return;
    }
}

[[nodiscard]] inline RecalcGroupPolicy determineGroupPolicy(const DependencyNode& rNode)
{
    if (!rNode.moSharedGroupAnchor || rNode.mnSharedGroupLength <= 1)
        return RecalcGroupPolicy::None;
    return rNode.mbShareableGroup ? RecalcGroupPolicy::PreserveSharedGroup
                                  : RecalcGroupPolicy::SplitSharedGroup;
}

} // namespace detail

[[nodiscard]] inline RecalcPlan buildRecalcPlan(
    const DependencySnapshot& rSnapshot, const InvalidationPlan& rInvalidationPlan)
{
    RecalcPlan aPlan;
    aPlan.maRebuildScopes = rInvalidationPlan.maRebuildScopes;
    aPlan.mbRequiresSnapshotRebuild = rInvalidationPlan.mbRequiresSnapshotRebuild;
    aPlan.mbUsedConservativeWidening = rInvalidationPlan.mbUsedConservativeWidening;

    for (const auto& rEntry : rInvalidationPlan.maDirtyFormulaCells)
    {
        const auto oNodeId = rSnapshot.findFormulaCellNode(rEntry.maAddress);
        if (!oNodeId)
            continue;

        const DependencyNode* pNode = rSnapshot.getNode(*oNodeId);
        if (!pNode)
            continue;

        if (rEntry.meReason == DirtyReason::TransitiveDependency)
            continue;

        RecalcSeed aSeed;
        aSeed.meKind = RecalcSeedKind::FormulaCell;
        aSeed.maNodeId = *oNodeId;
        aSeed.meReason = rEntry.meReason;
        aSeed.moAddress = rEntry.maAddress;
        aPlan.maSeeds.push_back(std::move(aSeed));
    }

    for (const auto aNodeId : rInvalidationPlan.maDirtyNodeIds)
    {
        const DependencyNode* pNode = rSnapshot.getNode(aNodeId);
        if (!pNode)
            continue;

        if (pNode->meKind == DependencyNodeKind::NamedRange)
        {
            const auto oReason
                = detail::findDirtyReasonForNamedRangeNode(rInvalidationPlan, rSnapshot, aNodeId);
            if (!oReason)
                continue;

            RecalcSeed aSeed;
            aSeed.meKind = RecalcSeedKind::NamedRange;
            aSeed.maNodeId = aNodeId;
            aSeed.meReason = *oReason;
            aSeed.moNamedRangeId = pNode->moNamedRangeId;
            aPlan.maSeeds.push_back(std::move(aSeed));
        }
    }

    for (const auto& rScope : aPlan.maRebuildScopes)
    {
        RecalcSeed aSeed;
        aSeed.meKind = RecalcSeedKind::StructuralRebuild;
        aSeed.meReason = DirtyReason::StructuralMutation;
        aSeed.moRebuildScopeKind = rScope.meKind;
        aPlan.maSeeds.push_back(std::move(aSeed));
    }

    const std::vector<DependencyNodeId> aDirtyFormulaNodes
        = detail::collectDirtyFormulaNodes(rInvalidationPlan, rSnapshot);
    std::unordered_set<DependencyNodeId, detail::DependencyNodeIdHash> aDirtyFormulaNodeSet(
        aDirtyFormulaNodes.begin(), aDirtyFormulaNodes.end());
    std::unordered_map<DependencyNodeId, std::vector<DependencyNodeId>,
        detail::DependencyNodeIdHash>
        aDependentsByNode;
    std::unordered_map<DependencyNodeId, sal_Int32, detail::DependencyNodeIdHash> aIndegreeByNode;
    std::unordered_map<DependencyNodeId, sal_Int32, detail::DependencyNodeIdHash> aDepthByNode;
    std::unordered_map<DependencyNodeId, DirtyReason, detail::DependencyNodeIdHash> aReasonByNode;
    std::unordered_map<DependencyNodeId, bool, detail::DependencyNodeIdHash> aDirtyNamedRangeDepByNode;
    std::unordered_map<DependencyNodeId, bool, detail::DependencyNodeIdHash> aOpaqueOrderingByNode;

    for (const auto aNodeId : aDirtyFormulaNodes)
    {
        aIndegreeByNode[aNodeId] = 0;
        aDepthByNode[aNodeId] = 0;
        aDirtyNamedRangeDepByNode[aNodeId] = false;
        aOpaqueOrderingByNode[aNodeId] = false;
        aReasonByNode[aNodeId]
            = detail::findDirtyReasonForFormulaNode(rInvalidationPlan, rSnapshot, aNodeId)
                  .value_or(DirtyReason::TransitiveDependency);
    }

    for (const auto aNodeId : aDirtyFormulaNodes)
    {
        std::unordered_set<DependencyNodeId, detail::DependencyNodeIdHash> aPredecessors;
        const auto& rDependencies = rSnapshot.getDependencies(aNodeId);
        for (const auto& rDependency : rDependencies)
        {
            if (rDependency.maSource.meKind == DependencySourceKind::OpaqueWorkbook)
            {
                aOpaqueOrderingByNode[aNodeId] = true;
                aPlan.mbHasOpaqueOrdering = true;
                continue;
            }

            bool bDependsOnDirtyNamedRange = false;
            detail::collectFormulaPredecessorsForSource(rSnapshot, aDirtyFormulaNodeSet,
                rDependency.maSource, aPredecessors, bDependsOnDirtyNamedRange);
            if (bDependsOnDirtyNamedRange)
                aDirtyNamedRangeDepByNode[aNodeId] = true;
        }

        aPredecessors.erase(aNodeId);
        for (const auto aPredecessorId : aPredecessors)
        {
            aDependentsByNode[aPredecessorId].push_back(aNodeId);
            ++aIndegreeByNode[aNodeId];
        }
    }

    std::vector<DependencyNodeId> aReady;
    aReady.reserve(aDirtyFormulaNodes.size());
    for (const auto aNodeId : aDirtyFormulaNodes)
    {
        if (aIndegreeByNode[aNodeId] == 0)
            aReady.push_back(aNodeId);
    }

    std::vector<DependencyNodeId> aOrderedNodes;
    aOrderedNodes.reserve(aDirtyFormulaNodes.size());

    while (!aReady.empty())
    {
        const DependencyNodeId aCurrent = aReady.front();
        aReady.erase(aReady.begin());
        aOrderedNodes.push_back(aCurrent);

        auto& rDependents = aDependentsByNode[aCurrent];
        std::sort(rDependents.begin(), rDependents.end(),
            [&rSnapshot](DependencyNodeId aLeft, DependencyNodeId aRight) {
                return detail::isEarlierNodeAddress(rSnapshot, aLeft, aRight);
            });

        for (const auto aDependentId : rDependents)
        {
            aDepthByNode[aDependentId]
                = std::max(aDepthByNode[aDependentId], aDepthByNode[aCurrent] + 1);
            sal_Int32& rIndegree = aIndegreeByNode[aDependentId];
            --rIndegree;
            if (rIndegree == 0)
                aReady.push_back(aDependentId);
        }
    }

    if (aOrderedNodes.size() != aDirtyFormulaNodes.size())
    {
        for (const auto aNodeId : aDirtyFormulaNodes)
        {
            if (std::find(aOrderedNodes.begin(), aOrderedNodes.end(), aNodeId) == aOrderedNodes.end())
                aOrderedNodes.push_back(aNodeId);
        }
    }

    for (std::size_t nIndex = 0; nIndex < aOrderedNodes.size(); ++nIndex)
    {
        const DependencyNodeId aNodeId = aOrderedNodes[nIndex];
        const DependencyNode* pNode = rSnapshot.getNode(aNodeId);
        if (!pNode || !pNode->moOutputAddress)
            continue;

        RecalcQueueEntry aEntry;
        aEntry.maNodeId = aNodeId;
        aEntry.maAddress = *pNode->moOutputAddress;
        aEntry.meReason = aReasonByNode[aNodeId];
        aEntry.mnOrder = static_cast<sal_Int32>(nIndex);
        aEntry.mnDependencyDepth = aDepthByNode[aNodeId];
        aEntry.moSharedGroupAnchor = pNode->moSharedGroupAnchor;
        aEntry.mnSharedGroupLength = pNode->mnSharedGroupLength;
        aEntry.meGroupPolicy = detail::determineGroupPolicy(*pNode);
        aEntry.mbDependsOnDirtyNamedRange = aDirtyNamedRangeDepByNode[aNodeId];
        aEntry.mbOpaqueOrdering = aOpaqueOrderingByNode[aNodeId];
        aPlan.maQueue.push_back(std::move(aEntry));
    }

    return aPlan;
}

} // namespace spreadsheetengine::detail::dependency

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
