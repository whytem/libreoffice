/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowBuilder.hxx>
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

    ComputationalObservationState aObservation;
    aObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 } };
    aObservation.maFormulaTrack = { { nData, 1, 1 } };
    aObservation.maCellBroadcasters.push_back({
        { nData, 0, 0 },
        { { ListenerAnchorKind::FormulaCell, { nData, 1, 0 }, 1 },
            { ListenerAnchorKind::FormulaGroup, { nData, 1, 0 }, 2 } } });

    const auto aShadow = buildComputationalWorkbookShadow(aFacade, aObservation);
    const auto aGraph = buildDependencyGraphShadow(aShadow, aObservation);

    if (aGraph.maSnapshot.mnGeneration != 12)
        return fail("computational_graph", "snapshot generation mismatch");
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

    std::cout << "computational_graph_tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
