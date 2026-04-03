/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <iostream>

#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrLowering.hxx>

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

} // namespace

int main()
{
    namespace secompiler = spreadsheetengine::detail::compiler;
    using spreadsheetengine::detail::substrate::ExecutionIrInstructionKind;
    using spreadsheetengine::detail::substrate::ExecutionIrLoweringContext;
    using spreadsheetengine::detail::substrate::lowerCompiledFormulaToExecutionIr;

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

    std::cout << "computational_execution_ir_tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
