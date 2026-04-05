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
    aTransition.maComputationalAfter
        = authoritybuilddetail::buildAuthorityComputationalShadow(aFacade, aObservation);
    aTransition.maGraphAfter = authoritybuilddetail::buildAuthorityGraphShadow(
        aTransition.maComputationalAfter, aTransition.maDependencySnapshot, aTransition.maRecalcPlan);
    aTransition.maIrAfter
        = authoritybuilddetail::buildAuthorityExecutionIrShadow(aTransition.maComputationalAfter, aFacade);
    aTransition.meVerdict = AuthorityPilotVerdict::Applicable;
    aTransition.maReason = u"ready";
    return aTransition;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
