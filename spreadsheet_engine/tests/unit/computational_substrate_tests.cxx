/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
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
    aFacade.setGeneration(9);

    const auto nData = aFacade.addSheet(u"Data");
    aFacade.addSheet(u"Summary");

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

    const ComputationalWorkbookShadow aShadow
        = buildComputationalWorkbookShadow(aFacade, aObservation);

    if (aShadow.maSnapshot.mnGeneration != 9)
        return fail("computational_substrate", "snapshot generation mismatch");
    if (aShadow.getCellCount() != 4)
        return fail("computational_substrate", "cell count mismatch");
    if (aShadow.getFormulaCellCount() != 2)
        return fail("computational_substrate", "formula cell count mismatch");
    if (aShadow.maNamedRanges.size() != 1)
        return fail("computational_substrate", "named range count mismatch");
    if (aShadow.maFormulaGroups.size() != 1)
        return fail("computational_substrate", "formula group count mismatch");
    if (aShadow.maCellBroadcasters.size() != 1)
        return fail("computational_substrate", "cell broadcaster count mismatch");

    const ShadowCellRecord* pScalar = aShadow.findCell({ nData, 0, 0 });
    if (!pScalar || pScalar->hasFormula() || pScalar->mbInFormulaTree)
        return fail("computational_substrate", "scalar cell projection mismatch");

    const ShadowCellRecord* pFormula = aShadow.findCell({ nData, 1, 1 });
    if (!pFormula || !pFormula->hasFormula() || !pFormula->mbInFormulaTrack
        || !pFormula->moFormulaGroup.has_value() || !pFormula->moFormula->mbDirty
        || !pFormula->moFormula->mbNeedsRecalc)
    {
        return fail("computational_substrate", "formula cell projection mismatch");
    }

    if (aShadow.maFormulaGroups.front().maMembers.size() != 2)
        return fail("computational_substrate", "formula group membership mismatch");

    std::cout << "computational_substrate_tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
