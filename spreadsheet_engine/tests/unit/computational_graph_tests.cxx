/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowMapping.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowMutation.hxx>
#include <spreadsheetengine/detail/workbook/InMemoryWorkbookFacade.hxx>

#include "TestSupport.hxx"

int main()
{
    using namespace spreadsheetengine::detail::facade;
    using namespace spreadsheetengine::detail::substrate;
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::standalone::test::fail;

    InMemoryWorkbookFacade aFacade;
    aFacade.setGrammar({ spreadsheetengine::api::FormulaLanguage::Odff,
        spreadsheetengine::api::AddressConvention::OdfA1, false });
    aFacade.setGeneration(12);

    const auto nData = aFacade.addSheet(u"Data");
    aFacade.setCell({ nData, 0, 0 }, CellValue::number(10.0));
    aFacade.setCell({ nData, 0, 1 }, CellValue::text(u"seed"));
    aFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*2", CellValue::number(20.0));
    aFacade.setFormulaCell({ nData, 1, 1 }, u"=SUM(A1:B1)", CellValue::number(30.0),
        FormulaCellKind::SharedGroupMember, true, true);
    aFacade.addFormulaGroup({ nData, 1, 0 }, 2, true);
    aFacade.addNamedRange(u"Metric", std::nullopt, { nData, 0, 0 }, u"$Data.$A$1:$B$2");

    ComputationalObservationState aObservation;
    aObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 } };
    aObservation.maFormulaTrack = { { nData, 1, 1 } };
    aObservation.maCellBroadcasters.push_back({
        { nData, 0, 0 },
        { { ListenerAnchorKind::FormulaCell, { nData, 1, 0 }, 1 },
            { ListenerAnchorKind::FormulaGroup, { nData, 1, 0 }, 2 } } });

    const auto aShadow = buildComputationalWorkbookShadow(aFacade, aObservation);
    const auto aGraph = buildDependencyGraphShadow(aShadow, aObservation);
    const auto aComparison = compareDependencyGraphShadow(aGraph, aShadow, aObservation);

    if (aGraph.maSnapshot.mnGeneration != 12)
        return fail("computational_graph", "snapshot generation mismatch");
    if (aComparison.meKind != graphmapping::GraphComparisonKind::Exact || !aComparison.mbFullMatch)
        return fail("computational_graph", "initial graph comparison mismatch");
    if (aGraph.getFormulaNodeCount() != 2)
        return fail("computational_graph", "formula node count mismatch");
    if (aGraph.maFormulaGroupNodes.size() != 1)
        return fail("computational_graph", "formula group node count mismatch");
    if (aGraph.getListenerAnchorCount() != 3)
        return fail("computational_graph", "listener anchor count mismatch");
    if (aGraph.getBroadcasterNodeCount() != 1)
        return fail("computational_graph", "broadcaster node count mismatch");
    if (aGraph.getEdgeCount() != 2)
        return fail("computational_graph", "edge count mismatch");
    if (aGraph.maFormulaTreeNodes.size() != 2 || aGraph.maFormulaTrackNodes.size() != 1)
        return fail("computational_graph", "formula subset count mismatch");

    const auto aFormulaAnchor
        = mapping::makeListenerAnchorId(ListenerAnchorKind::FormulaCell, { nData, 1, 0 }, 1);
    const auto aGroupAnchor
        = mapping::makeListenerAnchorId(ListenerAnchorKind::FormulaGroup, { nData, 1, 0 }, 2);
    if (!aGraph.findListenerAnchor(aFormulaAnchor))
        return fail("computational_graph", "formula anchor missing");
    if (!aGraph.findListenerAnchor(aGroupAnchor))
        return fail("computational_graph", "group anchor missing");
    if (!aGraph.findBroadcasterNode(BroadcasterNodeId::forCell({ nData, 0, 0 })))
        return fail("computational_graph", "broadcaster node missing");

    if (!graphmapping::hasNormalizedFormulaSubsetEquivalence(
            { { { nData, 1, 1 } }, { { nData, 1, 0 } } }, aGraph.maFormulaTreeNodes))
    {
        return fail("computational_graph", "formula subset equivalence mismatch");
    }

    if (graphmapping::hasNormalizedGraphEdgeEquivalence(
            { { BroadcasterNodeId::forCell({ nData, 0, 0 }), aGroupAnchor } }, aGraph.maEdges))
    {
        return fail("computational_graph", "graph edge equivalence should not collapse mismatch");
    }

    // Normalized-equivalent graph ordering should not count as mismatch.
    ComputationalObservationState aReorderedObservation = aObservation;
    aReorderedObservation.maFormulaTree = { { nData, 1, 1 }, { nData, 1, 0 } };
    const auto aReorderedGraph = buildDependencyGraphShadow(aShadow, aReorderedObservation);
    const auto aReorderedComparison
        = compareDependencyGraphShadow(aReorderedGraph, aShadow, aReorderedObservation);
    if (aReorderedComparison.meKind != graphmapping::GraphComparisonKind::NormalizedEquivalent
        || !aReorderedComparison.mbFullMatch)
    {
        return fail("computational_graph", "normalized graph comparison mismatch");
    }

    // Safe mutation rebuilds should keep the graph exact on the admitted subset.
    aFacade.setCell({ nData, 0, 0 }, CellValue::number(11.0));
    auto aMutationState = rebuildDependencyGraphShadowAfterMutation(
        aFacade, aObservation, MutationEvent::setScalarValue({ nData, 0, 0 }));
    if (compareDependencyGraphShadow(
            aMutationState.maGraphShadow, aMutationState.maComputationalShadow, aMutationState.maObservation)
            .meKind
        != graphmapping::GraphComparisonKind::Exact)
    {
        return fail("computational_graph", "scalar graph mutation mismatch");
    }

    aFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*4", CellValue::number(44.0));
    aMutationState = rebuildDependencyGraphShadowAfterMutation(
        aFacade, aObservation, MutationEvent::setFormula({ nData, 1, 0 }, u"=A1*4"));
    if (compareDependencyGraphShadow(
            aMutationState.maGraphShadow, aMutationState.maComputationalShadow, aMutationState.maObservation)
            .meKind
        != graphmapping::GraphComparisonKind::Exact)
    {
        return fail("computational_graph", "formula graph mutation mismatch");
    }

    aFacade.clearCell({ nData, 0, 1 });
    aMutationState = rebuildDependencyGraphShadowAfterMutation(
        aFacade, aObservation, MutationEvent::clearCell({ nData, 0, 1 }));
    if (compareDependencyGraphShadow(
            aMutationState.maGraphShadow, aMutationState.maComputationalShadow, aMutationState.maObservation)
            .meKind
        != graphmapping::GraphComparisonKind::Exact)
    {
        return fail("computational_graph", "clear graph mutation mismatch");
    }

    const auto aBeforeRange = *aFacade.findNamedRange(u"Metric", std::nullopt);
    if (!aFacade.renameNamedRange(u"Metric", u"MetricRenamed"))
        return fail("computational_graph", "named range rename setup failed");
    const auto aAfterRange = *aFacade.findNamedRange(u"MetricRenamed", std::nullopt);
    aMutationState = rebuildDependencyGraphShadowAfterMutation(
        aFacade, aObservation, MutationEvent::renameNamedRange(aBeforeRange, aAfterRange));
    if (compareDependencyGraphShadow(
            aMutationState.maGraphShadow, aMutationState.maComputationalShadow, aMutationState.maObservation)
            .meKind
        != graphmapping::GraphComparisonKind::Exact)
    {
        return fail("computational_graph", "named range graph mutation mismatch");
    }

    std::cout << "computational_graph_tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
