/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <limits>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/detail/FodsEvaluator.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::api::CellAddress;
using spreadsheetengine::api::CellValue;
using spreadsheetengine::api::DateParts;
using spreadsheetengine::api::calendar::makeDateSerial;
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
    aSheet1.setCell(0, 3, Cell { CellValue::number(10.0) });
    aSheet1.setCell(0, 4, Cell { CellValue::number(100.0) });
    aSheet1.setCell(0, 5, Cell { CellValue::number(30.0) });
    aSheet1.setRowHidden(4);
    aSheet1.setCell(16, 0, Cell { CellValue::number(40.0),
        u"of:=COM.MICROSOFT.AGGREGATE(9;5;[.A4:.A6])" });
    aSheet1.setCell(17, 0, Cell { CellValue::number(20.0),
        u"of:=COM.MICROSOFT.AGGREGATE(1;5;[.A4:.A6])" });
    aSheet1.setCell(1, 3, Cell { CellValue::number(10.0) });
    aSheet1.setCell(1, 4, Cell { CellValue::error(spreadsheetengine::api::Error::NotAvailable) });
    aSheet1.setCell(1, 5, Cell { CellValue::number(999.0), u"of:=SUBTOTAL(9;[.B4:.B5])" });
    aSheet1.setCell(1, 6, Cell { CellValue::number(30.0) });
    aSheet1.setCell(18, 0, Cell { CellValue::number(20.0),
        u"of:=COM.MICROSOFT.AGGREGATE(1;2;[.B4:.B7])" });
    aSheet1.setCell(19, 0, Cell { CellValue::text(u"AB"), u"of:=CLEAN(\"A\u0001B\")" });
    aSheet1.setCell(20, 0, Cell { CellValue::text(u"AB"), u"of:=CLEAN(UNICHAR(128)&\"AB\")" });
    aSheet1.setCell(21, 0, Cell { CellValue::boolean(true), u"of:=EXACT(1;1)" });
    aSheet1.setCell(22, 0, Cell { CellValue::boolean(true), u"of:=EXACT(1;{1})" });
    aSheet1.setCell(
        23, 0,
        Cell { CellValue::number(1.0), u"of:=YEARFRAC(DATE(2014;1;1);DATE(2015;1;1);0)" });
    aSheet1.setCell(24, 0, Cell { CellValue::text(u"A"), u"of:=DEC2HEX(10)" });
    aSheet1.setCell(25, 0, Cell { CellValue::number(1.23), u"of:=ROUND(1.2345;2)" });
    aSheet1.setCell(26, 0, Cell { CellValue::number(1.24), u"of:=ROUNDUP(1.231;2)" });
    aSheet1.setCell(27, 0, Cell { CellValue::number(1.23), u"of:=ROUNDDOWN(1.239;2)" });
    aSheet1.setCell(
        28, 0, Cell { CellValue::number(1230.0), u"of:=ORG.LIBREOFFICE.ROUNDSIG(1234.567;3)" });
    aSheet1.setCell(29, 0, Cell { CellValue::boolean(true), u"of:=ROUND(1.2345;2)=1.23" });
    aSheet1.setCell(
        30, 0,
        Cell { CellValue::boolean(true),
            u"of:=ORG.LIBREOFFICE.ROUNDSIG(1234.567;3)=1230" });
    aSheet1.setCell(31, 0, Cell { CellValue::number(-45.0), u"of:=ROUNDDOWN(-45.67)" });
    aSheet1.setCell(6, 25, Cell { CellValue::number(0.5) });
    aSheet1.setCell(8, 1, Cell { CellValue::text(u"one") });
    aSheet1.setCell(8, 2, Cell { CellValue::text(u"oneone") });
    aSheet1.setCell(8, 3, Cell { CellValue::text(u"two") });
    aSheet1.setCell(9, 1, Cell { CellValue::text(u"A2") });
    aSheet1.setCell(9, 2, Cell { CellValue::number(2.0) });
    aSheet1.setCell(9, 3, Cell { CellValue::number(3.0) });
    Cell aTypedTimeCell { CellValue::text(u"PT00H01M26.47S") };
    aTypedTimeCell.maRawValueType = u"time";
    aTypedTimeCell.maRawValue = u"PT00H01M26.47S";
    aSheet1.setCell(10, 1, aTypedTimeCell);
    aSheet1.setCell(
        11, 1, Cell { CellValue::number(1.0), u"of:=COUNTIF([.K2:.K2];\"=\"&[.K2])" });
    aSheet1.setCell(12, 2, Cell { CellValue::text(u"A") });
    aSheet1.setCell(12, 3, Cell { CellValue::text(u"B") });
    aSheet1.setCell(12, 4, Cell { CellValue::text(u"C") });
    aSheet1.setCell(40, 0, Cell { CellValue::number(0.0) });
    aSheet1.setCell(40, 1, Cell { CellValue::number(0.0) });
    aSheet1.setCell(40, 2, Cell { CellValue::number(0.0) });
    aSheet1.setCell(44, 1, Cell { CellValue::number(7.0) });
    aSheet1.setCell(44, 2, Cell { CellValue::number(8.0) });
    aSheet1.setCell(46, 0, Cell { CellValue::text(u"abc") });
    aSheet1.setCell(46, 1, Cell { CellValue::text(u"ABC") });
    aSheet1.setCell(46, 2, Cell { CellValue::text(u"Abc") });
    aSheet1.setCell(46, 3, Cell { CellValue::text(u"aBc") });
    aSheet1.setCell(46, 4, Cell { CellValue::text(u"Abc") });
    aSheet1.setCell(46, 5, Cell { CellValue::text(u"a") });
    aSheet1.setCell(46, 6, Cell { CellValue::text(u"A") });
    aSheet1.setCell(46, 7, Cell { CellValue::text(u"A") });
    aSheet1.setCell(46, 8, Cell { CellValue::text(u"A") });
    aSheet1.setCell(47, 0, Cell { CellValue::text(u""), u"of:=\"\"" });

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
    aWorkbook.mbSearchCriteriaMustApplyToWholeCell = false;

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
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 1, 0 });
        if (!aResult || !aResult.maValue.isScalar() || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 12.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled basic arithmetic mismatch");
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
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 1, 1 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 24.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled dependency evaluation mismatch");
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
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 6, 0 });
        if (!aResult || !aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 42.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled cached fallback mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 23, 0 });
        if (!aResult || !aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled external-name cached fallback mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 24, 0 });
        if (!aResult || !aResult.mbUsedCachedValue || !aResult.maValue.maValue.isText()
            || aResult.maValue.maValue.maString != u"A")
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled external-name text cached fallback mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 25, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.23))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "compiled ROUND() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 26, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.24))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "compiled ROUNDUP() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 27, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.23))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "compiled ROUNDDOWN() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 28, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1230.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "compiled ROUNDSIG() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 29, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isBoolean()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled ROUND() equality mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 30, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isBoolean()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled ROUNDSIG() equality mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 31, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, -45.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled one-arg ROUNDDOWN() mismatch");
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
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 7, 0 });
        if (aResult || aResult.maCyclePath.size() != 3
            || !(aResult.maCyclePath[0] == CellAddress { 0, 7, 0 })
            || !(aResult.maCyclePath[1] == CellAddress { 0, 8, 0 })
            || !(aResult.maCyclePath[2] == CellAddress { 0, 7, 0 }))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled cycle detection mismatch");
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
        const auto aSumResult = aEvaluator.evaluateCell({ 0, 16, 0 });
        const auto aAverageResult = aEvaluator.evaluateCell({ 0, 17, 0 });
        if (!aSumResult || aSumResult.mbUsedCachedValue || !aSumResult.maValue.maValue.isNumber()
            || !almostEqual(aSumResult.maValue.maValue.mfNumber, 40.0) || !aAverageResult
            || aAverageResult.mbUsedCachedValue || !aAverageResult.maValue.maValue.isNumber()
            || !almostEqual(aAverageResult.maValue.maValue.mfNumber, 20.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "AGGREGATE() hidden-row mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 18, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 20.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "AGGREGATE() nested-skip mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 19, 0 });
        if (!aResult || !aResult.maValue.maValue.isText()
            || aResult.maValue.maValue.maString != u"AB")
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "CLEAN() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 20, 0 });
        if (!aResult || !aResult.maValue.maValue.isText()
            || aResult.maValue.maValue.maString != u"AB")
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "UNICHAR() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 21, 0 });
        if (!aResult || !aResult.maValue.maValue.isBoolean()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "EXACT() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 22, 0 });
        if (!aResult || !aResult.maValue.maValue.isBoolean()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "scalar array constant evaluation mismatch");
        }
    }

    {
        const auto aCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.G26:.G26];\"=\"&[.G26])", { 0, 0, 0 });
        const auto aCountIfs = aEvaluator.evaluateFormula(
            u"of:=COUNTIFS([.A1:.A2];\">=5\";[.A1:.A2];\"<=7\")", { 0, 0, 0 });
        const auto aAverageIf
            = aEvaluator.evaluateFormula(u"of:=AVERAGEIF([.A1:.A2];\">=5\";[.A1:.A2])", { 0, 0, 0 });
        const auto aMaxIfs
            = aEvaluator.evaluateFormula(u"of:=MAXIFS([.A1:.A2];[.A1:.A2];\">5\")", { 0, 0, 0 });
        const auto aMinIfs
            = aEvaluator.evaluateFormula(u"of:=MINIFS([.A1:.A2];[.A1:.A2];\">=5\")", { 0, 0, 0 });
        const auto aPartialCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.I2:.I4];\"one\")", { 0, 0, 0 });
        const auto aNumericStringCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.J2:.J4];\"=2\")", { 0, 0, 0 });
        const auto aNumericOrderedCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.J2:.J4];\">2\")", { 0, 0, 0 });
        const auto aOrderedTextCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.M2:.M6];\"<B\")", { 0, 0, 0 });
        const auto aBareLessCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.M2:.M6];\"<\")", { 0, 0, 0 });
        const auto aBareGreaterCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.M2:.M6];\">\")", { 0, 0, 0 });
        const auto aTypedTimeCountIf
            = aEvaluator.evaluateCell({ 0, 11, 1 });
        const auto aBlankZeroCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AO1:.AO3];\"\")", { 0, 0, 0 });
        const auto aExplicitZeroCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AO1:.AO3];0)", { 0, 0, 0 });
        const auto aEmptyZeroCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AP1:.AP3];0)", { 0, 0, 0 });
        const auto aEmptyReferenceCriteriaCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AO1:.AO3];[.AR1])", { 0, 0, 0 });
        const auto aBlankOrderedCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AP1:.AP3];\"<1\")", { 0, 0, 0 });
        const auto aNotEqualWithEmptyCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AS1:.AS3];\"<>7\")", { 0, 0, 0 });
        const auto aRegexCaseSensitiveCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AU1:.AU9];\"(?-i)abc\")", { 0, 0, 0 });
        const auto aRegexMixedFlagCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AU1:.AU9];\"a(?-i)B(?i)c\")", { 0, 0, 0 });
        const auto aBareEqualsCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AV1:.AY1];\"=\")", { 0, 0, 0 });
        const auto aBareNotEqualsCountIf
            = aEvaluator.evaluateFormula(u"of:=COUNTIF([.AV1:.AY1];\"<>\")", { 0, 0, 0 });
        const auto aCompiledCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.G26:.G26];\"=\"&[.G26])", { 0, 0, 0 });
        const auto aCompiledCountIfs = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIFS([.A1:.A2];\">=5\";[.A1:.A2];\"<=7\")", { 0, 0, 0 });
        const auto aCompiledPartialCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.I2:.I4];\"one\")", { 0, 0, 0 });
        const auto aCompiledNumericStringCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.J2:.J4];\"=2\")", { 0, 0, 0 });
        const auto aCompiledNumericOrderedCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.J2:.J4];\">2\")", { 0, 0, 0 });
        const auto aCompiledOrderedTextCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.M2:.M6];\"<B\")", { 0, 0, 0 });
        const auto aCompiledBareLessCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.M2:.M6];\"<\")", { 0, 0, 0 });
        const auto aCompiledBareGreaterCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.M2:.M6];\">\")", { 0, 0, 0 });
        const auto aCompiledTypedTimeCountIf = aEvaluator.evaluateCellViaCompiledTokens(
            { 0, 11, 1 });
        const auto aCompiledBlankZeroCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.AO1:.AO3];\"\")", { 0, 0, 0 });
        const auto aCompiledExplicitZeroCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.AO1:.AO3];0)", { 0, 0, 0 });
        const auto aCompiledEmptyZeroCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.AP1:.AP3];0)", { 0, 0, 0 });
        const auto aCompiledEmptyReferenceCriteriaCountIf
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=COUNTIF([.AO1:.AO3];[.AR1])", { 0, 0, 0 });
        const auto aCompiledBlankOrderedCountIf = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUNTIF([.AP1:.AP3];\"<1\")", { 0, 0, 0 });
        const auto aCompiledNotEqualWithEmptyCountIf
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=COUNTIF([.AS1:.AS3];\"<>7\")", { 0, 0, 0 });
        const auto aCompiledRegexCaseSensitiveCountIf
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=COUNTIF([.AU1:.AU9];\"(?-i)abc\")", { 0, 0, 0 });
        const auto aCompiledRegexMixedFlagCountIf
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=COUNTIF([.AU1:.AU9];\"a(?-i)B(?i)c\")", { 0, 0, 0 });
        const auto aCompiledBareEqualsCountIf
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=COUNTIF([.AV1:.AY1];\"=\")", { 0, 0, 0 });
        const auto aCompiledBareNotEqualsCountIf
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=COUNTIF([.AV1:.AY1];\"<>\")", { 0, 0, 0 });
        const auto checkCriteriaNumber = [&](const char* pLabel, const auto& rResult,
                                             double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                const double fActual
                    = (rResult && rResult.maValue.maValue.isNumber()) ? rResult.maValue.maValue.mfNumber
                                                                      : std::numeric_limits<double>::quiet_NaN();
                std::fprintf(stderr,
                    "%s: criteria aggregate mismatch in %s (actual=%g expected=%g cached=%d)\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel, fActual, fExpected,
                    rResult ? static_cast<int>(rResult.mbUsedCachedValue) : -1);
                return false;
            }
            return true;
        };

        if (!checkCriteriaNumber("COUNTIF typed time", aCountIf, 1.0)
            || !checkCriteriaNumber("COUNTIFS range criteria", aCountIfs, 2.0)
            || !checkCriteriaNumber("AVERAGEIF", aAverageIf, 6.0)
            || !checkCriteriaNumber("MAXIFS", aMaxIfs, 7.0)
            || !checkCriteriaNumber("MINIFS", aMinIfs, 5.0)
            || !checkCriteriaNumber("COUNTIF partial text", aPartialCountIf, 2.0)
            || !checkCriteriaNumber("COUNTIF numeric string equality", aNumericStringCountIf, 2.0)
            || !checkCriteriaNumber("COUNTIF ordered numeric", aNumericOrderedCountIf, 1.0)
            || !checkCriteriaNumber("COUNTIF ordered text", aOrderedTextCountIf, 1.0)
            || !checkCriteriaNumber("COUNTIF bare less", aBareLessCountIf, 0.0)
            || !checkCriteriaNumber("COUNTIF bare greater", aBareGreaterCountIf, 3.0)
            || !checkCriteriaNumber("COUNTIF typed time cell", aTypedTimeCountIf, 1.0)
            || !checkCriteriaNumber("COUNTIF blank literal zero-like", aBlankZeroCountIf, 3.0)
            || !checkCriteriaNumber("COUNTIF numeric zero", aExplicitZeroCountIf, 3.0)
            || !checkCriteriaNumber("COUNTIF numeric zero on empty cells", aEmptyZeroCountIf, 0.0)
            || !checkCriteriaNumber("COUNTIF empty reference criteria", aEmptyReferenceCriteriaCountIf, 3.0)
            || !checkCriteriaNumber("COUNTIF ordered numeric on empty cells", aBlankOrderedCountIf, 0.0)
            || !checkCriteriaNumber("COUNTIF numeric not-equal with empty", aNotEqualWithEmptyCountIf, 2.0)
            || !checkCriteriaNumber("COUNTIF regex case-sensitive inline flag", aRegexCaseSensitiveCountIf, 1.0)
            || !checkCriteriaNumber("COUNTIF regex mixed inline flags", aRegexMixedFlagCountIf, 2.0)
            || !checkCriteriaNumber("COUNTIF bare equals with formula empty string", aBareEqualsCountIf, 3.0)
            || !checkCriteriaNumber("COUNTIF bare not-equals with formula empty string", aBareNotEqualsCountIf, 1.0)
            || !checkCriteriaNumber("compiled COUNTIF typed time", aCompiledCountIf, 1.0)
            || !checkCriteriaNumber("compiled COUNTIFS range criteria", aCompiledCountIfs, 2.0)
            || !checkCriteriaNumber("compiled COUNTIF partial text", aCompiledPartialCountIf, 2.0)
            || !checkCriteriaNumber("compiled COUNTIF numeric string equality", aCompiledNumericStringCountIf, 2.0)
            || !checkCriteriaNumber("compiled COUNTIF ordered numeric", aCompiledNumericOrderedCountIf, 1.0)
            || !checkCriteriaNumber("compiled COUNTIF ordered text", aCompiledOrderedTextCountIf, 1.0)
            || !checkCriteriaNumber("compiled COUNTIF bare less", aCompiledBareLessCountIf, 0.0)
            || !checkCriteriaNumber("compiled COUNTIF bare greater", aCompiledBareGreaterCountIf, 3.0)
            || !checkCriteriaNumber("compiled COUNTIF typed time cell", aCompiledTypedTimeCountIf, 1.0)
            || !checkCriteriaNumber("compiled COUNTIF blank literal zero-like", aCompiledBlankZeroCountIf, 3.0)
            || !checkCriteriaNumber("compiled COUNTIF numeric zero", aCompiledExplicitZeroCountIf, 3.0)
            || !checkCriteriaNumber("compiled COUNTIF numeric zero on empty cells", aCompiledEmptyZeroCountIf, 0.0)
            || !checkCriteriaNumber("compiled COUNTIF empty reference criteria", aCompiledEmptyReferenceCriteriaCountIf, 3.0)
            || !checkCriteriaNumber("compiled COUNTIF ordered numeric on empty cells", aCompiledBlankOrderedCountIf, 0.0)
            || !checkCriteriaNumber("compiled COUNTIF numeric not-equal with empty", aCompiledNotEqualWithEmptyCountIf, 2.0)
            || !checkCriteriaNumber("compiled COUNTIF regex case-sensitive inline flag", aCompiledRegexCaseSensitiveCountIf, 1.0)
            || !checkCriteriaNumber("compiled COUNTIF regex mixed inline flags", aCompiledRegexMixedFlagCountIf, 2.0)
            || !checkCriteriaNumber("compiled COUNTIF bare equals with formula empty string", aCompiledBareEqualsCountIf, 3.0)
            || !checkCriteriaNumber("compiled COUNTIF bare not-equals with formula empty string", aCompiledBareNotEqualsCountIf, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "criteria aggregate mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=EXACT(1;{1})", { 0, 0, 0 });
        if (!aResult || !aResult.maValue.maValue.isBoolean()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled formula execution mismatch");
        }
    }

    {
        constexpr DateParts aNullDate { 1899, 12, 30 };
        const auto aExpectedEomonth = makeDateSerial(aNullDate, 2015, 2, 28, true);
        const auto aExpectedEdate = makeDateSerial(aNullDate, 2001, 4, 30, true);
        const auto aExpectedDate = makeDateSerial(aNullDate, 2022, 1, 9, false);
        const auto aDate = aEvaluator.evaluateFormula(u"of:=DATEVALUE(\"Jan1, 2015\")", { 0, 0, 0 });
        const auto aDateFunction = aEvaluator.evaluateFormula(u"of:=DATE(2022;1;9)", { 0, 0, 0 });
        const auto aDateTime
            = aEvaluator.evaluateFormula(u"of:=VALUE(\"1954-07-20 16:30:01\")", { 0, 0, 0 });
        const auto aTime = aEvaluator.evaluateFormula(u"of:=TIMEVALUE(\"4PM\")", { 0, 0, 0 });
        const auto aDateTimeTime = aEvaluator.evaluateFormula(
            u"of:=TIMEVALUE(\"01/09/2019 08:30:00\")", { 0, 0, 0 });
        const auto aInvalidTime = aEvaluator.evaluateFormula(u"of:=TIMEVALUE(\"10\")", { 0, 0, 0 });
        const auto aTimeFunction = aEvaluator.evaluateFormula(u"of:=TIME(24;60;-1)", { 0, 0, 0 });
        const auto aInvalidTimeFunction
            = aEvaluator.evaluateFormula(u"of:=TIME(-1;19;19)", { 0, 0, 0 });
        const auto aRawSubtract
            = aEvaluator.evaluateFormula(u"of:=ORG.LIBREOFFICE.RAWSUBTRACT(1;0.25;0.5)", { 0, 0, 0 });
        const auto aWeeks = aEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.WEEKS(DATEVALUE(\"2021-11-14\");DATEVALUE(\"2021-11-15\");1)",
            { 0, 0, 0 });
        const auto aExactVLookup = aEvaluator.evaluateFormula(
            u"of:=VLOOKUP(7;{5|21;6|22;8|24};2;0)", { 0, 0, 0 });
        const auto aIsNaMax = aEvaluator.evaluateFormula(u"of:=ISNA(MAX(NA()))", { 0, 0, 0 });
        const auto aMaxValue = aEvaluator.evaluateFormula(u"of:=MAX(3;7;2)", { 0, 0, 0 });
        const auto aMinValue = aEvaluator.evaluateFormula(u"of:=MIN(3;7;2)", { 0, 0, 0 });
        const auto aDaysInMonth
            = aEvaluator.evaluateFormula(u"of:=ORG.OPENOFFICE.DAYSINMONTH(\"Jan1, 2015\")", { 0, 0, 0 });
        const auto aDaysInYear
            = aEvaluator.evaluateFormula(u"of:=ORG.OPENOFFICE.DAYSINYEAR(\"Jan1, 2015\")", { 0, 0, 0 });
        const auto aIsLeapYear
            = aEvaluator.evaluateFormula(u"of:=ORG.OPENOFFICE.ISLEAPYEAR(\"2000-02-01\")", { 0, 0, 0 });
        const auto aIsoWeekNum
            = aEvaluator.evaluateFormula(u"of:=ISOWEEKNUM(\"Jan11, 2015\")", { 0, 0, 0 });
        const auto aEomonth
            = aEvaluator.evaluateFormula(u"of:=EOMONTH(\"Jan11, 2015\";1)", { 0, 0, 0 });
        const auto aEdate
            = aEvaluator.evaluateFormula(u"of:=EDATE(\"2001-03-31\";1)", { 0, 0, 0 });
        if (!aDate || !aDate.maValue.maValue.isNumber()
            || !almostEqual(aDate.maValue.maValue.mfNumber, 42005.0)
            || !aExpectedDate || !aDateFunction || !aDateFunction.maValue.maValue.isNumber()
            || !almostEqual(aDateFunction.maValue.maValue.mfNumber, aExpectedDate.maValue)
            || !aDateTime
            || !aDateTime.maValue.maValue.isNumber()
            || !almostEqual(aDateTime.maValue.maValue.mfNumber, 19925.0 + (59401.0 / 86400.0))
            || !aTime || !aTime.maValue.maValue.isNumber()
            || !almostEqual(aTime.maValue.maValue.mfNumber, 16.0 / 24.0) || !aDateTimeTime
            || !aDateTimeTime.maValue.maValue.isNumber()
            || !almostEqual(aDateTimeTime.maValue.maValue.mfNumber, 8.5 / 24.0)
            || !aTimeFunction || !aTimeFunction.maValue.maValue.isNumber()
            || !almostEqual(aTimeFunction.maValue.maValue.mfNumber, 3599.0 / 86400.0)
            || !aRawSubtract || !aRawSubtract.maValue.maValue.isNumber()
            || !almostEqual(aRawSubtract.maValue.maValue.mfNumber, 0.25)
            || !aWeeks || !aWeeks.maValue.maValue.isNumber()
            || !almostEqual(aWeeks.maValue.maValue.mfNumber, 1.0)
            || aExactVLookup || aExactVLookup.meError != spreadsheetengine::api::Error::IllegalArgument
            || !aIsNaMax || !aIsNaMax.maValue.maValue.isBoolean()
            || !almostEqual(aIsNaMax.maValue.maValue.mfNumber, 1.0)
            || !aMaxValue || !aMaxValue.maValue.maValue.isNumber()
            || !almostEqual(aMaxValue.maValue.maValue.mfNumber, 7.0)
            || !aMinValue || !aMinValue.maValue.maValue.isNumber()
            || !almostEqual(aMinValue.maValue.maValue.mfNumber, 2.0)
            || !aDaysInMonth || !aDaysInMonth.maValue.maValue.isNumber()
            || !almostEqual(aDaysInMonth.maValue.maValue.mfNumber, 31.0) || !aDaysInYear
            || !aDaysInYear.maValue.maValue.isNumber()
            || !almostEqual(aDaysInYear.maValue.maValue.mfNumber, 365.0) || !aIsLeapYear
            || !aIsLeapYear.maValue.maValue.isBoolean()
            || !almostEqual(aIsLeapYear.maValue.maValue.mfNumber, 1.0)
            || !aIsoWeekNum || !aIsoWeekNum.maValue.maValue.isNumber()
            || !almostEqual(aIsoWeekNum.maValue.maValue.mfNumber, 2.0)
            || !aExpectedEomonth || !aEomonth || !aEomonth.maValue.maValue.isNumber()
            || !almostEqual(aEomonth.maValue.maValue.mfNumber, aExpectedEomonth.maValue)
            || !aExpectedEdate || !aEdate || !aEdate.maValue.maValue.isNumber()
            || !almostEqual(aEdate.maValue.maValue.mfNumber, aExpectedEdate.maValue)
            || aInvalidTime || aInvalidTime.meError != spreadsheetengine::api::Error::IllegalArgument
            || aInvalidTimeFunction
            || aInvalidTimeFunction.meError != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "date/time parsing mismatch");
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
        const auto aResult = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=GlobalRange", { 0, 0, 0 });
        if (!aResult || !aResult.maValue.isMatrixReference()
            || aResult.maValue.maReference.matrixDimensions().mnColumns != 1
            || aResult.maValue.maReference.matrixDimensions().mnRows != 2)
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled named range view mismatch");
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
        const auto aLocal = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=One+1", { 0, 0, 0 });
        const auto aGlobal = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=One+1", { 1, 0, 0 });
        if (!aLocal || !aGlobal || !aLocal.maValue.maValue.isNumber()
            || !aGlobal.maValue.maValue.isNumber()
            || !almostEqual(aLocal.maValue.maValue.mfNumber, 8.0)
            || !almostEqual(aGlobal.maValue.maValue.mfNumber, 6.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled named-range scope mismatch");
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

    {
        const auto aRepoRoot = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
        const auto aVlookupPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                  / "spreadsheet" / "fods" / "vlookup.fods";
        const auto aLoadResult = spreadsheetengine::core::fods::loadWorkbook(aVlookupPath.string());
        if (!aLoadResult)
            return fail("spreadsheetengine_fods_evaluator_tests", "vlookup.fods load failed");

        Evaluator aVlookupEvaluator(aLoadResult.maValue.maWorkbook);
        const auto aPatternResult = aVlookupEvaluator.evaluateCell({ 1, 0, 7 });
        if (!aPatternResult || aPatternResult.mbUsedCachedValue
            || !aPatternResult.maValue.maValue.isNumber()
            || !almostEqual(aPatternResult.maValue.maValue.mfNumber, 4.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "vlookup.fods regex match mismatch");
        }

        const auto aApproximateTextResult = aVlookupEvaluator.evaluateCell({ 1, 0, 55 });
        if (!aApproximateTextResult || aApproximateTextResult.mbUsedCachedValue
            || !aApproximateTextResult.maValue.maValue.isText()
            || aApproximateTextResult.maValue.maValue.maString != u"abcd")
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "vlookup.fods approximate text best-fit mismatch");
        }

        const auto aTypeMismatchSortedResult = aVlookupEvaluator.evaluateCell({ 1, 0, 18 });
        if (!aTypeMismatchSortedResult || aTypeMismatchSortedResult.mbUsedCachedValue
            || !aTypeMismatchSortedResult.maValue.maValue.isError()
            || aTypeMismatchSortedResult.maValue.maValue.meError
                   != spreadsheetengine::api::Error::NotAvailable)
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "vlookup.fods sorted text over numeric keys mismatch");
        }

        const auto aNotFoundResult = aVlookupEvaluator.evaluateCell({ 1, 0, 72 });
        if (!aNotFoundResult || aNotFoundResult.mbUsedCachedValue
            || !aNotFoundResult.maValue.maValue.isError()
            || aNotFoundResult.maValue.maValue.meError != spreadsheetengine::api::Error::NotAvailable)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "vlookup.fods not-found mismatch");
        }
    }

    {
        const auto aRepoRoot = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
        const auto aFormulaPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                  / "information" / "fods" / "formula.fods";
        const auto aLoadResult = spreadsheetengine::core::fods::loadWorkbook(aFormulaPath.string());
        if (!aLoadResult)
            return fail("spreadsheetengine_fods_evaluator_tests", "formula.fods load failed");

        Evaluator aFormulaEvaluator(aLoadResult.maValue.maWorkbook);
        const auto aMissingFormulaResult = aFormulaEvaluator.evaluateCell({ 1, 0, 2 });
        if (!aMissingFormulaResult || aMissingFormulaResult.mbUsedCachedValue
            || !aMissingFormulaResult.maValue.maValue.isError()
            || aMissingFormulaResult.maValue.maValue.meError
                   != spreadsheetengine::api::Error::NotAvailable)
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "formula.fods missing-formula FORMULA() mismatch");
        }
    }

    {
        const auto aRepoRoot = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
        const auto aIsLogicalPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                    / "information" / "fods" / "islogical.fods";
        const auto aLoadResult = spreadsheetengine::core::fods::loadWorkbook(aIsLogicalPath.string());
        if (!aLoadResult)
            return fail("spreadsheetengine_fods_evaluator_tests", "islogical.fods load failed");

        Evaluator aIsLogicalEvaluator(aLoadResult.maValue.maWorkbook);
        const auto aIsNaMaxResult = aIsLogicalEvaluator.evaluateCell({ 1, 0, 8 });
        if (!aIsNaMaxResult || aIsNaMaxResult.mbUsedCachedValue
            || !aIsNaMaxResult.maValue.maValue.isBoolean()
            || !almostEqual(aIsNaMaxResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "islogical.fods ISNA(MAX(NA())) mismatch");
        }
    }

    {
        const auto aRepoRoot = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
        const auto aTTestPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                / "statistical" / "fods" / "t.test.fods";
        const auto aAggregatePath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                    / "mathematical" / "fods" / "aggregate.fods";
        const auto aTTestLoad = spreadsheetengine::core::fods::loadWorkbook(aTTestPath.string());
        const auto aLoadResult = spreadsheetengine::core::fods::loadWorkbook(aAggregatePath.string());
        if (!aTTestLoad)
            return fail("spreadsheetengine_fods_evaluator_tests", "t.test.fods load failed");
        if (!aLoadResult)
            return fail("spreadsheetengine_fods_evaluator_tests", "aggregate.fods load failed");

        Evaluator aTTestEvaluator(aTTestLoad.maValue.maWorkbook);
        const auto aTTestInvalidResult = aTTestEvaluator.evaluateCell({ 1, 0, 1 });
        if (!aTTestInvalidResult || aTTestInvalidResult.mbUsedCachedValue
            || !aTTestInvalidResult.maValue.maValue.isError()
            || aTTestInvalidResult.maValue.maValue.meError
                   != spreadsheetengine::api::Error::NoValue)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "t.test.fods invalid-mode mismatch");
        }

        const auto* pSheet2 = aLoadResult.maValue.maWorkbook.findSheet(u"Sheet2");
        if (!pSheet2)
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "aggregate sheet lookup mismatch");
        }

        Evaluator aAggregateEvaluator(aLoadResult.maValue.maWorkbook);
        for (int nRow = 8; nRow <= 18; ++nRow)
        {
            const auto aResult = aAggregateEvaluator.evaluateCell({ 1, 0, nRow });
            const auto* pExpected = pSheet2->findCell(1, nRow);
            if (!aResult || aResult.mbUsedCachedValue || !pExpected || !pExpected->maValue.isNumber()
                || !aResult.maValue.maValue.isNumber()
                || !almostEqual(aResult.maValue.maValue.mfNumber, pExpected->maValue.mfNumber))
            {
                return fail(
                    "spreadsheetengine_fods_evaluator_tests", "aggregate.fods live evaluation mismatch");
            }
        }

        const auto oCompleteSheetId
            = aLoadResult.maValue.maWorkbook.findSheetId(u"winfrieds_testCase");
        if (!oCompleteSheetId)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "aggregate complete-variation sheet lookup failed");
        }

        const auto aCompleteResult = aAggregateEvaluator.evaluateCell({ *oCompleteSheetId, 0, 15 });
        const auto* pCompleteSheet
            = aLoadResult.maValue.maWorkbook.findSheet(u"winfrieds_testCase");
        const auto* pCompleteExpected = pCompleteSheet ? pCompleteSheet->findCell(1, 15) : nullptr;
        if (!aCompleteResult || aCompleteResult.mbUsedCachedValue || !pCompleteExpected
            || !pCompleteExpected->maValue.isNumber()
            || !aCompleteResult.maValue.maValue.isNumber()
            || !almostEqual(aCompleteResult.maValue.maValue.mfNumber,
                pCompleteExpected->maValue.mfNumber))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "aggregate.fods nested-option mismatch");
        }
    }

    {
        const auto aRepoRoot = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
        constexpr DateParts aNullDate { 1899, 12, 30 };
        const auto aExpectedEomonth = makeDateSerial(aNullDate, 2015, 2, 28, true);
        const auto aExpectedEdate = makeDateSerial(aNullDate, 2001, 4, 30, true);
        const auto aDateValuePath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                    / "date_time" / "fods" / "datevalue.fods";
        const auto aTimeValuePath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                    / "date_time" / "fods" / "timevalue.fods";

        const auto aDateValueLoad = spreadsheetengine::core::fods::loadWorkbook(aDateValuePath.string());
        const auto aTimeValueLoad = spreadsheetengine::core::fods::loadWorkbook(aTimeValuePath.string());
        if (!aDateValueLoad || !aTimeValueLoad)
            return fail("spreadsheetengine_fods_evaluator_tests", "date/time FODS load failed");

        Evaluator aDateValueEvaluator(aDateValueLoad.maValue.maWorkbook);
        const auto aDateValueResult = aDateValueEvaluator.evaluateCell({ 1, 0, 3 });
        if (!aDateValueResult || aDateValueResult.mbUsedCachedValue
            || !aDateValueResult.maValue.maValue.isNumber()
            || !almostEqual(aDateValueResult.maValue.maValue.mfNumber, 42005.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "datevalue.fods live evaluation mismatch");
        }

        Evaluator aTimeValueEvaluator(aTimeValueLoad.maValue.maWorkbook);
        const auto aTimeValueResult = aTimeValueEvaluator.evaluateCell({ 1, 0, 8 });
        if (!aTimeValueResult || aTimeValueResult.mbUsedCachedValue
            || !aTimeValueResult.maValue.maValue.isNumber()
            || !almostEqual(aTimeValueResult.maValue.maValue.mfNumber, 8.5 / 24.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "timevalue.fods live evaluation mismatch");
        }

        const auto aDaysInMonthPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                      / "date_time" / "fods" / "daysinmonth.fods";
        const auto aDaysInYearPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                     / "date_time" / "fods" / "daysinyear.fods";
        const auto aIsLeapYearPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                     / "date_time" / "fods" / "isleapyear.fods";
        const auto aIsoWeekNumPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                     / "date_time" / "fods" / "isoweeknum.fods";
        const auto aDaysInMonthLoad
            = spreadsheetengine::core::fods::loadWorkbook(aDaysInMonthPath.string());
        const auto aDaysInYearLoad
            = spreadsheetengine::core::fods::loadWorkbook(aDaysInYearPath.string());
        const auto aIsLeapYearLoad
            = spreadsheetengine::core::fods::loadWorkbook(aIsLeapYearPath.string());
        const auto aIsoWeekNumLoad
            = spreadsheetengine::core::fods::loadWorkbook(aIsoWeekNumPath.string());
        if (!aDaysInMonthLoad || !aDaysInYearLoad || !aIsLeapYearLoad || !aIsoWeekNumLoad)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "extended date/time FODS load failed");
        }

        Evaluator aDaysInMonthEvaluator(aDaysInMonthLoad.maValue.maWorkbook);
        const auto aDaysInMonthResult = aDaysInMonthEvaluator.evaluateCell({ 1, 0, 4 });
        if (!aDaysInMonthResult || aDaysInMonthResult.mbUsedCachedValue
            || !aDaysInMonthResult.maValue.maValue.isNumber()
            || !almostEqual(aDaysInMonthResult.maValue.maValue.mfNumber, 31.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "daysinmonth.fods live evaluation mismatch");
        }

        Evaluator aDaysInYearEvaluator(aDaysInYearLoad.maValue.maWorkbook);
        const auto aDaysInYearResult = aDaysInYearEvaluator.evaluateCell({ 1, 0, 4 });
        if (!aDaysInYearResult || aDaysInYearResult.mbUsedCachedValue
            || !aDaysInYearResult.maValue.maValue.isNumber()
            || !almostEqual(aDaysInYearResult.maValue.maValue.mfNumber, 365.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "daysinyear.fods live evaluation mismatch");
        }

        Evaluator aIsLeapYearEvaluator(aIsLeapYearLoad.maValue.maWorkbook);
        const auto aIsLeapYearResult = aIsLeapYearEvaluator.evaluateCell({ 1, 0, 2 });
        if (!aIsLeapYearResult || aIsLeapYearResult.mbUsedCachedValue
            || !aIsLeapYearResult.maValue.maValue.isBoolean()
            || !almostEqual(aIsLeapYearResult.maValue.maValue.mfNumber, 0.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "isleapyear.fods live evaluation mismatch");
        }

        Evaluator aIsoWeekNumEvaluator(aIsoWeekNumLoad.maValue.maWorkbook);
        const auto aIsoWeekNumResult = aIsoWeekNumEvaluator.evaluateCell({ 1, 0, 2 });
        if (!aIsoWeekNumResult || aIsoWeekNumResult.mbUsedCachedValue
            || !aIsoWeekNumResult.maValue.maValue.isNumber()
            || !almostEqual(aIsoWeekNumResult.maValue.maValue.mfNumber, 2.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "isoweeknum.fods live evaluation mismatch");
        }

        const auto aEomonthPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                  / "date_time" / "fods" / "eomonth.fods";
        const auto aEdatePath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                / "date_time" / "fods" / "edate.fods";
        const auto aTimePath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                               / "date_time" / "fods" / "time.fods";
        const auto aWeeksPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                / "date_time" / "fods" / "weeks.fods";
        const auto aVlookupPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                  / "spreadsheet" / "fods" / "vlookup.fods";
        const auto aEomonthLoad = spreadsheetengine::core::fods::loadWorkbook(aEomonthPath.string());
        const auto aEdateLoad = spreadsheetengine::core::fods::loadWorkbook(aEdatePath.string());
        const auto aTimeLoad = spreadsheetengine::core::fods::loadWorkbook(aTimePath.string());
        const auto aWeeksLoad = spreadsheetengine::core::fods::loadWorkbook(aWeeksPath.string());
        const auto aVlookupLoad = spreadsheetengine::core::fods::loadWorkbook(aVlookupPath.string());
        if (!aEomonthLoad || !aEdateLoad || !aTimeLoad || !aWeeksLoad || !aVlookupLoad)
            return fail("spreadsheetengine_fods_evaluator_tests", "month-shift FODS load failed");

        Evaluator aEomonthEvaluator(aEomonthLoad.maValue.maWorkbook);
        const auto aEomonthResult = aEomonthEvaluator.evaluateCell({ 1, 0, 2 });
        if (!aEomonthResult || aEomonthResult.mbUsedCachedValue
            || !aExpectedEomonth || !aEomonthResult.maValue.maValue.isNumber()
            || !almostEqual(aEomonthResult.maValue.maValue.mfNumber, aExpectedEomonth.maValue))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "eomonth.fods live evaluation mismatch");
        }

        Evaluator aEdateEvaluator(aEdateLoad.maValue.maWorkbook);
        const auto aEdateResult = aEdateEvaluator.evaluateCell({ 1, 0, 2 });
        if (!aEdateResult || aEdateResult.mbUsedCachedValue
            || !aExpectedEdate || !aEdateResult.maValue.maValue.isNumber()
            || !almostEqual(aEdateResult.maValue.maValue.mfNumber, aExpectedEdate.maValue))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "edate.fods live evaluation mismatch");
        }

        Evaluator aTimeEvaluator(aTimeLoad.maValue.maWorkbook);
        const auto aTimeResult = aTimeEvaluator.evaluateCell({ 1, 0, 10 });
        if (!aTimeResult || aTimeResult.mbUsedCachedValue
            || !aTimeResult.maValue.maValue.isNumber()
            || !almostEqual(aTimeResult.maValue.maValue.mfNumber, 14.5 / 24.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "time.fods live evaluation mismatch");
        }

        Evaluator aWeeksEvaluator(aWeeksLoad.maValue.maWorkbook);
        const auto aWeeksResult = aWeeksEvaluator.evaluateCell({ 1, 0, 11 });
        if (!aWeeksResult || aWeeksResult.mbUsedCachedValue
            || !aWeeksResult.maValue.maValue.isNumber()
            || !almostEqual(aWeeksResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "weeks.fods live evaluation mismatch");
        }

        Evaluator aVlookupEvaluator(aVlookupLoad.maValue.maWorkbook);
        const auto aVlookupResult = aVlookupEvaluator.evaluateCell({ 1, 0, 72 });
        if (!aVlookupResult || !aVlookupResult.maValue.maValue.isError()
            || aVlookupResult.maValue.maValue.meError != spreadsheetengine::api::Error::NotAvailable)
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "vlookup.fods live evaluation mismatch");
        }
    }

    std::cout << "spreadsheetengine FODS evaluator tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
