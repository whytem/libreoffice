/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <iostream>

#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrLowering.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrMutation.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrReferenceUpdate.hxx>
#include <spreadsheetengine/detail/workbook/InMemoryWorkbookFacade.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::standalone::test::fail;

[[nodiscard]] spreadsheetengine::core::workbook::Workbook makeWorkbook()
{
    spreadsheetengine::core::workbook::Workbook aWorkbook;
    spreadsheetengine::core::workbook::Sheet aSheet;
    aSheet.maName = u"Sheet1";
    aWorkbook.maSheets.push_back(aSheet);
    aWorkbook.maNamedRanges.push_back(
        { u"GlobalRange", {}, u"$Sheet1.$A$1", u"$Sheet1.$A$1:.$A$2" });
    return aWorkbook;
}

[[nodiscard]] bool hasInstructionKind(
    const spreadsheetengine::detail::substrate::ExecutionIrFormulaRecord& rFormula,
    spreadsheetengine::detail::substrate::ExecutionIrInstructionKind eKind)
{
    return std::any_of(rFormula.maInstructions.begin(), rFormula.maInstructions.end(),
        [eKind](const auto& rInstruction) { return rInstruction.meKind == eKind; });
}

[[nodiscard]] int countInstructionKind(
    const spreadsheetengine::detail::substrate::ExecutionIrFormulaRecord& rFormula,
    spreadsheetengine::detail::substrate::ExecutionIrInstructionKind eKind)
{
    return static_cast<int>(std::count_if(rFormula.maInstructions.begin(), rFormula.maInstructions.end(),
        [eKind](const auto& rInstruction) { return rInstruction.meKind == eKind; }));
}

[[nodiscard]] spreadsheetengine::detail::substrate::ExecutionIrCompileArtifacts
compileShadowCellFromWorkbookHost(const spreadsheetengine::detail::compiler::WorkbookCompileHost& rHost,
    const spreadsheetengine::api::Grammar& rGrammar,
    const spreadsheetengine::detail::substrate::ShadowCellRecord& rCell)
{
    namespace secompiler = spreadsheetengine::detail::compiler;
    spreadsheetengine::detail::substrate::ExecutionIrCompileArtifacts aArtifacts;

    if (!rCell.moFormula)
    {
        aArtifacts.maFailureMessage = u"expected formula cell";
        return aArtifacts;
    }

    const auto aContext = secompiler::makeWorkbookCompileContext(rCell.maId.maAddress, rGrammar);
    const auto aLowered
        = secompiler::lowerFormulaSource(rCell.moFormula->maFormulaSource, rHost, aContext);
    if (!aLowered)
    {
        aArtifacts.mnFailureIndex = static_cast<sal_Int32>(aLowered.mnFailureOffset);
        aArtifacts.maFailureMessage = aLowered.maDetail;
        return aArtifacts;
    }

    aArtifacts.moFormula = aLowered.maFormula;
    return aArtifacts;
}

} // namespace

int main()
{
    namespace secompiler = spreadsheetengine::detail::compiler;
    using spreadsheetengine::detail::substrate::ExecutionIrComparisonKind;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildExecutionIrWorkbookShadow;
    using spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow;
    using spreadsheetengine::detail::substrate::ExecutionIrInstructionKind;
    using spreadsheetengine::detail::substrate::ExecutionIrLoweringContext;
    using spreadsheetengine::detail::substrate::rebuildExecutionIrShadowAfterMutation;
    using spreadsheetengine::detail::substrate::makeExecutionIrStructuralUpdatePlan;
    using spreadsheetengine::detail::substrate::lowerCompiledFormulaToExecutionIr;
    using spreadsheetengine::detail::substrate::updateExecutionIrFormulaReferences;

    auto aWorkbook = makeWorkbook();
    secompiler::WorkbookCompileHost aHost(aWorkbook);
    auto oContext = secompiler::makeWorkbookCompileContext(
        aWorkbook, u"Sheet1", 1, 1, secompiler::kDefaultWorkbookCompileGrammar);
    if (!oContext)
        return fail("spreadsheetengine_computational_ir_tests", "compile context mismatch");

    {
        const auto aLowered = secompiler::lowerFormulaSource(u"of:=[.A1]+[.B1]", aHost, *oContext);
        if (!aLowered)
            return fail("spreadsheetengine_computational_ir_tests", "basic lowering mismatch");

        const auto aIr = lowerCompiledFormulaToExecutionIr(aLowered.maFormula,
            { { { 0, 1, 1 } }, std::nullopt, { u"=A1+B1", {} }, true, false });
        if (!aIr || aIr.maFormula.maInstructions.size() < 3
            || countInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::SingleReference) != 2
            || !hasInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::PlainOpcode)
            || aIr.maFormula.getReferenceInstructionCount() != 2 || !aIr.maFormula.mbInFormulaTree
            || aIr.maFormula.mbInFormulaTrack)
        {
            return fail("spreadsheetengine_computational_ir_tests", "basic IR lowering mismatch");
        }
    }

    {
        const auto aLowered
            = secompiler::lowerFormulaSource(u"of:=SUM([.A1:.B2])", aHost, *oContext);
        if (!aLowered)
            return fail("spreadsheetengine_computational_ir_tests", "SUM lowering mismatch");

        const auto aIr = lowerCompiledFormulaToExecutionIr(aLowered.maFormula,
            { { { 0, 2, 1 } }, std::nullopt, { u"=SUM(A1:B2)", {} }, false, true });
        if (!aIr || !hasInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::RangeReference)
            || !hasInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::FunctionCallMarker)
            || !aIr.maFormula.mbInFormulaTrack)
        {
            return fail("spreadsheetengine_computational_ir_tests", "SUM IR lowering mismatch");
        }
    }

    {
        const auto aLowered
            = secompiler::lowerFormulaSource(u"of:=GlobalRange+1", aHost, *oContext);
        if (!aLowered)
            return fail("spreadsheetengine_computational_ir_tests", "named range lowering mismatch");

        const auto aIr = lowerCompiledFormulaToExecutionIr(aLowered.maFormula,
            { { { 0, 3, 1 } }, std::nullopt, { u"=GlobalRange+1", {} }, false, false });
        if (!aIr
            || !hasInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::RangeNameReference)
            || !hasInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::NumberLiteral))
        {
            return fail(
                "spreadsheetengine_computational_ir_tests", "named range IR lowering mismatch");
        }
    }

    {
        const auto aLowered
            = secompiler::lowerFormulaSourceLexical(u"of:=IFERROR([.A1]/[.B1];0)", aHost, *oContext);
        if (!aLowered)
            return fail("spreadsheetengine_computational_ir_tests", "IFERROR lexical lowering mismatch");

        const auto aIr = lowerCompiledFormulaToExecutionIr(aLowered.maFormula,
            { { { 0, 4, 1 } }, std::nullopt, { u"=IFERROR(A1/B1;0)", {} }, false, false });
        if (!aIr || !hasInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::JumpTable)
            || !hasInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::NumberLiteral)
            || !hasInstructionKind(aIr.maFormula, ExecutionIrInstructionKind::PlainOpcode))
        {
            return fail(
                "spreadsheetengine_computational_ir_tests", "IFERROR IR lowering mismatch");
        }
    }

    {
        const auto aLowered
            = secompiler::lowerFormulaSource(u"of:=SUM([.$A$1:.$A$2])", aHost, *oContext);
        if (!aLowered)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row insert lowering mismatch");
        }

        auto aIr = lowerCompiledFormulaToExecutionIr(aLowered.maFormula,
            { { { 0, 2, 0 } }, std::nullopt, { u"=SUM($A$1:$A$2)", {} }, false, false });
        if (!aIr)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row insert IR mismatch");
        }

        const auto oPlan = makeExecutionIrStructuralUpdatePlan(
            spreadsheetengine::detail::facade::MutationEvent::insertRows(0, 1, 1), { 1023, 65535, 15 });
        if (!oPlan)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row insert plan mismatch");
        }

        const auto aSummary = updateExecutionIrFormulaReferences(
            aIr.maFormula, { 0, 2, 0 }, { 1023, 65535, 15 }, *oPlan);
        if (!aSummary.mbChanged || aSummary.meResult != spreadsheetengine::api::refupdate::UpdateResult::Updated)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row insert summary mismatch");
        }

        const auto itRange = std::find_if(aIr.maFormula.maInstructions.begin(), aIr.maFormula.maInstructions.end(),
            [](const auto& rInstruction) {
                return rInstruction.meKind == ExecutionIrInstructionKind::RangeReference;
            });
        if (itRange == aIr.maFormula.maInstructions.end())
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row insert instruction mismatch");
        }
        const auto aAbsolute = spreadsheetengine::api::refdata::toAbsoluteRange(
            std::get<spreadsheetengine::api::refdata::ComplexRefData>(itRange->maPayload),
            { 1023, 65535, 15 }, { 0, 2, 0 });
        if (aAbsolute != spreadsheetengine::api::CellRange { { 0, 0, 0 }, { 0, 0, 2 } })
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row insert absolute-range mismatch");
        }
    }

    {
        const auto aLowered
            = secompiler::lowerFormulaSource(u"of:=[.$B$2]", aHost, *oContext);
        if (!aLowered)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column delete lowering mismatch");
        }

        auto aIr = lowerCompiledFormulaToExecutionIr(aLowered.maFormula,
            { { { 0, 2, 0 } }, std::nullopt, { u"=$B$2", {} }, false, false });
        if (!aIr)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column delete IR mismatch");
        }

        const auto oPlan = makeExecutionIrStructuralUpdatePlan(
            spreadsheetengine::detail::facade::MutationEvent::deleteColumns(0, 0, 1), { 1023, 65535, 15 });
        if (!oPlan)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column delete plan mismatch");
        }

        const auto aSummary = updateExecutionIrFormulaReferences(
            aIr.maFormula, { 0, 2, 0 }, { 1023, 65535, 15 }, *oPlan);
        if (!aSummary.mbChanged || aSummary.meResult != spreadsheetengine::api::refupdate::UpdateResult::Updated)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column delete summary mismatch");
        }

        const auto itRef = std::find_if(aIr.maFormula.maInstructions.begin(), aIr.maFormula.maInstructions.end(),
            [](const auto& rInstruction) {
                return rInstruction.meKind == ExecutionIrInstructionKind::SingleReference;
            });
        if (itRef == aIr.maFormula.maInstructions.end())
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column delete instruction mismatch");
        }
        const auto aAbsolute = spreadsheetengine::api::refdata::toAbsoluteAddress(
            std::get<spreadsheetengine::api::refdata::SingleRefData>(itRef->maPayload),
            { 1023, 65535, 15 }, { 0, 2, 0 });
        if (aAbsolute != spreadsheetengine::api::CellAddress { 0, 0, 1 })
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column delete absolute-address mismatch");
        }
    }

    {
        const auto aLowered
            = secompiler::lowerFormulaSource(u"of:=[.$A$2]", aHost, *oContext);
        if (!aLowered)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row delete lowering mismatch");
        }

        auto aIr = lowerCompiledFormulaToExecutionIr(aLowered.maFormula,
            { { { 0, 0, 2 } }, std::nullopt, { u"=$A$2", {} }, false, false });
        if (!aIr)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row delete IR mismatch");
        }

        const auto oPlan = makeExecutionIrStructuralUpdatePlan(
            spreadsheetengine::detail::facade::MutationEvent::deleteRows(0, 0, 1),
            { 1023, 65535, 15 });
        if (!oPlan)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row delete plan mismatch");
        }

        const auto aSummary = updateExecutionIrFormulaReferences(
            aIr.maFormula, { 0, 0, 2 }, { 1023, 65535, 15 }, *oPlan);
        if (!aSummary.mbChanged
            || aSummary.meResult != spreadsheetengine::api::refupdate::UpdateResult::Updated)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row delete summary mismatch");
        }

        const auto itRef = std::find_if(aIr.maFormula.maInstructions.begin(),
            aIr.maFormula.maInstructions.end(), [](const auto& rInstruction) {
                return rInstruction.meKind == ExecutionIrInstructionKind::SingleReference;
            });
        if (itRef == aIr.maFormula.maInstructions.end())
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row delete instruction mismatch");
        }
        const auto aAbsolute = spreadsheetengine::api::refdata::toAbsoluteAddress(
            std::get<spreadsheetengine::api::refdata::SingleRefData>(itRef->maPayload),
            { 1023, 65535, 15 }, { 0, 0, 1 });
        if (aAbsolute != spreadsheetengine::api::CellAddress { 0, 0, 0 })
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update row delete absolute-address mismatch");
        }
    }

    {
        const auto aLowered
            = secompiler::lowerFormulaSource(u"of:=[.$A$1]", aHost, *oContext);
        if (!aLowered)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column insert lowering mismatch");
        }

        auto aIr = lowerCompiledFormulaToExecutionIr(aLowered.maFormula,
            { { { 0, 1, 0 } }, std::nullopt, { u"=$A$1", {} }, false, false });
        if (!aIr)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column insert IR mismatch");
        }

        const auto oPlan = makeExecutionIrStructuralUpdatePlan(
            spreadsheetengine::detail::facade::MutationEvent::insertColumns(0, 0, 1),
            { 1023, 65535, 15 });
        if (!oPlan)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column insert plan mismatch");
        }

        const auto aSummary = updateExecutionIrFormulaReferences(
            aIr.maFormula, { 0, 1, 0 }, { 1023, 65535, 15 }, *oPlan);
        if (!aSummary.mbChanged
            || aSummary.meResult != spreadsheetengine::api::refupdate::UpdateResult::Updated)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column insert summary mismatch");
        }

        const auto itRef = std::find_if(aIr.maFormula.maInstructions.begin(),
            aIr.maFormula.maInstructions.end(), [](const auto& rInstruction) {
                return rInstruction.meKind == ExecutionIrInstructionKind::SingleReference;
            });
        if (itRef == aIr.maFormula.maInstructions.end())
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column insert instruction mismatch");
        }
        const auto aAbsolute = spreadsheetengine::api::refdata::toAbsoluteAddress(
            std::get<spreadsheetengine::api::refdata::SingleRefData>(itRef->maPayload),
            { 1023, 65535, 15 }, { 0, 2, 0 });
        if (aAbsolute != spreadsheetengine::api::CellAddress { 0, 1, 0 })
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "reference-update column insert absolute-address mismatch");
        }
    }

    {
        spreadsheetengine::detail::facade::InMemoryWorkbookFacade aFacade;
        aFacade.setGrammar(secompiler::kDefaultWorkbookCompileGrammar);
        aFacade.setGeneration(13);
        const auto nSheet = aFacade.addSheet(u"Sheet1");
        aFacade.setCell({ nSheet, 0, 0 }, spreadsheetengine::api::CellValue::number(5.0));
        aFacade.setCell({ nSheet, 1, 0 }, spreadsheetengine::api::CellValue::number(9.0));
        aFacade.setFormulaCell({ nSheet, 2, 0 }, u"=A1+B1",
            spreadsheetengine::api::CellValue::number(14.0));
        aFacade.setFormulaCell({ nSheet, 3, 0 }, u"=SUM(A1:B2)",
            spreadsheetengine::api::CellValue::number(14.0));
        aFacade.setFormulaCell({ nSheet, 4, 0 }, u"=GlobalRange+1",
            spreadsheetengine::api::CellValue::number(6.0));
        aFacade.addNamedRange(u"GlobalRange", std::nullopt, { nSheet, 0, 0 }, u"$Sheet1.$A$1:.$A$2");

        spreadsheetengine::detail::substrate::ComputationalObservationState aObservation;
        aObservation.maFormulaTree = { { nSheet, 2, 0 }, { nSheet, 3, 0 }, { nSheet, 4, 0 } };
        aObservation.maFormulaTrack = { { nSheet, 3, 0 } };
        const auto aShadow = buildComputationalWorkbookShadow(aFacade, aObservation);
        const auto aIrShadow = buildExecutionIrWorkbookShadow(aShadow,
            [&](const spreadsheetengine::detail::substrate::ShadowCellRecord& rCell) {
                return compileShadowCellFromWorkbookHost(aHost, aShadow.maGrammar, rCell);
            });

        if (aIrShadow.getFormulaCount() != 3 || !aIrShadow.maBuildFailures.empty()
            || !aIrShadow.findFormula({ nSheet, 2, 0 }))
        {
            return fail("spreadsheetengine_computational_ir_tests", "IR workbook build mismatch");
        }

        const auto aExactComparison = compareExecutionIrWorkbookShadow(aIrShadow, aIrShadow);
        if (aExactComparison.meKind != ExecutionIrComparisonKind::Exact
            || !aExactComparison.mbFullMatch)
        {
            return fail("spreadsheetengine_computational_ir_tests", "IR exact comparison mismatch");
        }

        auto aReordered = aIrShadow;
        std::reverse(aReordered.maFormulaRecords.begin(), aReordered.maFormulaRecords.end());
        const auto aNormalizedComparison = compareExecutionIrWorkbookShadow(aReordered, aIrShadow);
        if (aNormalizedComparison.meKind != ExecutionIrComparisonKind::NormalizedEquivalent
            || !aNormalizedComparison.mbFullMatch
            || aNormalizedComparison.mbFormulaRecordExactMatch)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "IR normalized comparison mismatch");
        }

        auto aMismatched = aIrShadow;
        aMismatched.maFormulaRecords.front().maSource.maFormula = u"=Changed";
        const auto aMismatchComparison = compareExecutionIrWorkbookShadow(aMismatched, aIrShadow);
        if (aMismatchComparison.meKind != ExecutionIrComparisonKind::Mismatch
            || aMismatchComparison.mbFullMatch)
        {
            return fail("spreadsheetengine_computational_ir_tests",
                "IR mismatch comparison mismatch");
        }

        const auto aMutationState = rebuildExecutionIrShadowAfterMutation(aShadow,
            spreadsheetengine::detail::facade::MutationEvent::setFormula(
                { nSheet, 4, 0 }, u"=GlobalRange+1"),
            [&](const spreadsheetengine::detail::substrate::ShadowCellRecord& rCell) {
                return compileShadowCellFromWorkbookHost(aHost, aShadow.maGrammar, rCell);
            });
        const auto aMutationComparison
            = compareExecutionIrWorkbookShadow(aMutationState.maIrShadow, aIrShadow);
        if (aMutationComparison.meKind != ExecutionIrComparisonKind::Exact
            || !aMutationComparison.mbFullMatch)
        {
            return fail("spreadsheetengine_computational_ir_tests", "IR mutation rebuild mismatch");
        }
    }

    std::cout << "computational_execution_ir_tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
