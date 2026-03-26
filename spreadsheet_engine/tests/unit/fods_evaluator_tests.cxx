/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <filesystem>
#include <iostream>

#include <spreadsheetengine/detail/FodsEvaluator.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::api::CellAddress;
using spreadsheetengine::api::CellValue;
using spreadsheetengine::core::fods::Evaluator;
using spreadsheetengine::core::workbook::Cell;
using spreadsheetengine::core::workbook::NamedRange;
using spreadsheetengine::core::workbook::Sheet;
using spreadsheetengine::core::workbook::Workbook;

Workbook makeWorkbook()
{
    Workbook aWorkbook;

    Sheet aSheet1;
    aSheet1.maName = u"Sheet1";
    aSheet1.setCell(0, 0, Cell { CellValue::number(5.0) });
    aSheet1.setCell(0, 1, Cell { CellValue::number(7.0) });
    aSheet1.setCell(1, 0, Cell { CellValue::number(12.0), u"of:=[.A1]+[.A2]" });
    aSheet1.setCell(1, 1, Cell { CellValue::number(24.0), u"of:=[.B1]*2" });
    aSheet1.setCell(2, 0, Cell { CellValue::text(u"of:=[.A1]+[.A2]"), u"of:=FORMULA([.B1])" });
    aSheet1.setCell(3, 0, Cell { CellValue::number(4.0), u"of:=[Sheet2.A1]+1" });
    aSheet1.setCell(4, 0, Cell { CellValue::boolean(true), u"of:=AND([.A1:.A2])" });
    aSheet1.setCell(5, 0, Cell { CellValue::boolean(true), u"of:=ISERROR(#N/A)" });
    aSheet1.setCell(6, 0, Cell { CellValue::number(42.0), u"of:=UNKNOWN(1)" });
    aSheet1.setCell(7, 0, Cell { CellValue::number(1.0), u"of:=[.I1]" });
    aSheet1.setCell(8, 0, Cell { CellValue::number(1.0), u"of:=[.H1]" });
    aSheet1.setCell(9, 0, Cell { CellValue::number(0.0), u"of:=-0.3+0.2+0.1" });
    aSheet1.setCell(10, 0, Cell { CellValue::number(1.0), u"of:=MOD(11;2)" });
    aSheet1.setCell(11, 0, Cell { CellValue::number(9.0), u"of:=IFERROR([Sheet3.A1];9)" });
    aSheet1.setCell(12, 0, Cell { CellValue::number(4.0), u"of:=IFERROR(3;4)" });
    aSheet1.setCell(13, 0, Cell { CellValue::number(8.0), u"of:=IFNA([Sheet3.B1];8)" });
    aSheet1.setCell(14, 0, Cell { CellValue::number(2.0), u"of:=IF(0;1;2)" });
    aSheet1.setCell(15, 0, Cell { CellValue::number(7.0), u"of:=IF(1;7;[Sheet3.A1])" });

    Sheet aSheet2;
    aSheet2.maName = u"Sheet2";
    aSheet2.setCell(0, 0, Cell { CellValue::number(3.0) });

    Sheet aSheet3;
    aSheet3.maName = u"Sheet3";
    aSheet3.setCell(0, 0, Cell { CellValue::error(spreadsheetengine::api::Error::DivisionByZero) });
    aSheet3.setCell(1, 0, Cell { CellValue::error(spreadsheetengine::api::Error::NotAvailable) });

    aWorkbook.maSheets.push_back(std::move(aSheet1));
    aWorkbook.maSheets.push_back(std::move(aSheet2));
    aWorkbook.maSheets.push_back(std::move(aSheet3));

    aWorkbook.maNamedRanges.push_back(
        NamedRange { u"GlobalRange", {}, u"$Sheet1.$A$1", u"$Sheet1.$A$1:.$A$2" });
    aWorkbook.maNamedRanges.push_back(
        NamedRange { u"One", {}, u"$Sheet1.$A$1", u"$Sheet1.$A$1:.$A$1" });
    aWorkbook.maNamedRanges.push_back(
        NamedRange { u"One", u"Sheet1", u"$Sheet1.$A$2", u"$Sheet1.$A$2:.$A$2" });

    return aWorkbook;
}

}

int main()
{
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::fail;

    const Workbook aWorkbook = makeWorkbook();
    Evaluator aEvaluator(aWorkbook);

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 1, 0 });
        if (!aResult || !aResult.maValue.isScalar() || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 12.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "basic arithmetic mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 1, 1 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 24.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "dependency evaluation mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 2, 0 });
        if (!aResult || !aResult.maValue.maValue.isText()
            || aResult.maValue.maValue.maString != u"=A1+A2")
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "FORMULA() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 3, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 4.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "cross-sheet reference mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 4, 0 });
        if (!aResult || !aResult.maValue.maValue.isBoolean()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "AND() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 5, 0 });
        if (!aResult || !aResult.maValue.maValue.isBoolean()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "ISERROR() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 6, 0 });
        if (!aResult || !aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 42.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "cached fallback mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 7, 0 });
        if (aResult || aResult.maCyclePath.size() != 3
            || !(aResult.maCyclePath[0] == CellAddress { 0, 7, 0 })
            || !(aResult.maCyclePath[1] == CellAddress { 0, 8, 0 })
            || !(aResult.maCyclePath[2] == CellAddress { 0, 7, 0 }))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "cycle detection mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 9, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 0.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "approximate add mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 10, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "MOD() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 11, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 9.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "IFERROR() alternate mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 12, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 3.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "IFERROR() keep-primary mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 13, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 8.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "IFNA() alternate mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 14, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 2.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "IF() false-branch mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 15, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 7.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "IF() lazy branch mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateFormula(u"of:=GlobalRange", { 0, 0, 0 });
        if (!aResult || !aResult.maValue.isMatrixReference()
            || aResult.maValue.maReference.matrixDimensions().mnColumns != 1
            || aResult.maValue.maReference.matrixDimensions().mnRows != 2)
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "named range view mismatch");
        }

        const auto aFirst = aEvaluator.materializeReferenceValue(aResult.maValue.maReference, 0, 0);
        const auto aSecond = aEvaluator.materializeReferenceValue(aResult.maValue.maReference, 0, 1);
        if (!aFirst || !aSecond || !almostEqual(aFirst.maValue.maValue.mfNumber, 5.0)
            || !almostEqual(aSecond.maValue.maValue.mfNumber, 7.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "range materialization mismatch");
        }
    }

    {
        const auto aLocal = aEvaluator.evaluateFormula(u"of:=One+1", { 0, 0, 0 });
        const auto aGlobal = aEvaluator.evaluateFormula(u"of:=One+1", { 1, 0, 0 });
        if (!aLocal || !aGlobal || !aLocal.maValue.maValue.isNumber()
            || !aGlobal.maValue.maValue.isNumber()
            || !almostEqual(aLocal.maValue.maValue.mfNumber, 8.0)
            || !almostEqual(aGlobal.maValue.maValue.mfNumber, 6.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "named-range scope mismatch");
        }
    }

    {
        const auto aWorkbookPath = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT)
                                   / "tests" / "data" / "fods" / "minimal_workbook.fods";
        const auto aLoadResult = spreadsheetengine::core::fods::loadWorkbook(aWorkbookPath.string());
        if (!aLoadResult)
            return fail("spreadsheetengine_fods_evaluator_tests", "fixture workbook load failed");

        Evaluator aFixtureEvaluator(aLoadResult.maValue.maWorkbook);
        const auto aResult = aFixtureEvaluator.evaluateCell({ 0, 4, 0 });
        if (!aResult || !aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "fixture cached fallback mismatch");
        }

        const auto aImportedResult
            = aFixtureEvaluator.evaluateFormula(u"of:=[ImportedResults.A1]", { 0, 0, 0 });
        if (!aImportedResult || !aImportedResult.maValue.maValue.isText()
            || aImportedResult.maValue.maValue.maString != u"source")
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "imported sheet evaluation mismatch");
        }

        const auto aErr511Result = aFixtureEvaluator.evaluateCell({ 0, 7, 0 });
        if (!aErr511Result || !aErr511Result.mbUsedCachedValue
            || !aErr511Result.maValue.maValue.isError())
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "Err:511 cached fallback mismatch");
        }
    }

    std::cout << "spreadsheetengine FODS evaluator tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
