/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/detail/FormulaEvaluator.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::api::CellAddress;
using spreadsheetengine::api::CellValue;
using spreadsheetengine::api::DateParts;
using spreadsheetengine::api::calendar::makeDateSerial;
using spreadsheetengine::core::eval::Evaluator;
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
    aSheet1.setCell(32, 0, Cell { CellValue::number(95.0428743993921),
        u"of:=PRICE(\"1999-02-15\";\"2007-11-15\";0.0575;0.065;100;2;0)" });
    aSheet1.setCell(33, 0, Cell { CellValue::number(95.0780346202577),
        u"of:=PRICE(\"1999-02-15\";\"2007-11-15\";0.0575;0.065;100;1)" });
    aSheet1.setCell(34, 0, Cell { CellValue::number(48.0), u"of:=SUM([.A1:.B2])" });
    aSheet1.setCell(35, 0, Cell { CellValue::text(u"1899-12-26 12:00:00"), u"of:=BASISODATETIME(-3.5)" });
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
    Cell aTypedDateCell { CellValue::text(u"01/01/1000") };
    aTypedDateCell.maRawValueType = u"date";
    aTypedDateCell.maRawValue = u"1000-01-06";
    aSheet1.setCell(10, 2, aTypedDateCell);
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
    aSheet1.setCell(52, 0, Cell { CellValue::number(1.0) });
    aSheet1.setCell(52, 1, Cell { CellValue::number(2.0) });
    aSheet1.setCell(52, 2, Cell { CellValue::number(4.0) });
    aSheet1.setCell(52, 3, Cell { CellValue::number(8.0) });
    aSheet1.setCell(53, 0, Cell { CellValue::number(11.0) });
    aSheet1.setCell(53, 1, Cell { CellValue::number(22.0) });
    aSheet1.setCell(53, 2, Cell { CellValue::number(44.0) });
    aSheet1.setCell(53, 3, Cell { CellValue::number(88.0) });
    aSheet1.setCell(0, 60, Cell { CellValue::number(10.0) });
    aSheet1.setCell(0, 61, Cell { CellValue::number(20.0) });
    aSheet1.setCell(0, 62, Cell { CellValue::number(30.0) });
    aSheet1.setRowFiltered(61);
    aSheet1.setRowHidden(62);
    aSheet1.setCell(54, 0, Cell { CellValue::text(u"A") });
    aSheet1.setCell(55, 0, Cell { CellValue::text(u"C") });
    aSheet1.setCell(56, 0, Cell { CellValue::text(u"E") });
    aSheet1.setCell(57, 0, Cell { CellValue::text(u"G") });
    aSheet1.setCell(54, 1, Cell { CellValue::text(u"CAR") });
    aSheet1.setCell(55, 1, Cell { CellValue::text(u"BIKE") });
    aSheet1.setCell(56, 1, Cell { CellValue::text(u"VAN") });
    aSheet1.setCell(57, 1, Cell { CellValue::text(u"TRAIN") });
    aSheet1.setCell(58, 0, Cell { CellValue::number(2.0) });
    aSheet1.setCell(58, 1, Cell { CellValue::number(4.0) });
    aSheet1.setCell(58, 2, Cell { CellValue::number(6.0) });
    aSheet1.setCell(58, 3, Cell { CellValue::number(8.0) });
    aSheet1.setCell(59, 0, Cell { CellValue::number(2.0) });
    aSheet1.setCell(59, 1, Cell { CellValue::number(4.0) });
    aSheet1.setCell(59, 2, Cell { CellValue::number(4.0) });
    aSheet1.setCell(59, 3, Cell { CellValue::number(8.0) });
    aSheet1.setCell(60, 0, Cell { CellValue::number(9.0) });
    aSheet1.setCell(60, 1, Cell { CellValue::number(7.0) });
    aSheet1.setCell(60, 2, Cell { CellValue::number(5.0) });
    aSheet1.setCell(60, 3, Cell { CellValue::number(3.0) });
    aSheet1.setCell(62, 0, Cell { CellValue::number(1.0) });
    aSheet1.setCell(62, 1, Cell { CellValue::number(3.0) });
    aSheet1.setCell(62, 2, Cell { CellValue::number(5.0) });
    aSheet1.setCell(62, 3, Cell { CellValue::number(7.0) });
    aSheet1.setCell(63, 0, Cell { CellValue::number(10.0) });
    aSheet1.setCell(63, 1, Cell { CellValue::number(30.0) });
    aSheet1.setCell(63, 2, Cell { CellValue::number(50.0) });
    aSheet1.setCell(63, 3, Cell { CellValue::number(70.0) });
    aSheet1.setCell(64, 0, Cell { CellValue::number(0.5), u"of:=NORMDIST(3;3;1;TRUE())" });
    aSheet1.setCell(65, 0, Cell { CellValue::number(0.199471140200716),
        u"of:=NORMDIST(3;3;2;FALSE())" });
    aSheet1.setCell(66, 0, Cell { CellValue::number(0.77686983985157),
        u"of:=CHISQDIST(3;2;1)" });
    aSheet1.setCell(67, 0, Cell { CellValue::number(0.111565080074215),
        u"of:=CHISQDIST(3;2;0)" });
    aSheet1.setCell(68, 0, Cell { CellValue::number(0.550671035882778),
        u"of:=GAMMADIST(0.8;1;1;1)" });
    aSheet1.setCell(69, 0, Cell { CellValue::number(10.0), u"of:=GAMMADIST(0;1;0.1;0)" });
    aSheet1.setCell(70, 0, Cell { CellValue::number(0.809090909090909),
        u"of:=HYPGEOMDIST(2;2;90;100;0)" });
    aSheet1.setCell(71, 0, Cell { CellValue::number(1.0), u"of:=HYPGEOMDIST(2;2;90;100;1)" });
    aSheet1.setCell(72, 0, Cell { CellValue::number(4.0), u"of:=COM.MICROSOFT.BINOM.INV(8;0.35;0.8)" });
    aSheet1.setCell(73, 0, Cell { CellValue::number(0.83), u"of:=PERCENTRANK({1;2;3;4};3.5;2)" });
    aSheet1.setCell(74, 0, Cell { CellValue::number(0.83),
        u"of:=COM.MICROSOFT.PERCENTRANK.INC({1;2;3;4};3.5;2)" });
    aSheet1.setCell(75, 0, Cell { CellValue::number(0.7),
        u"of:=COM.MICROSOFT.PERCENTRANK.EXC({1;2;3;4};3.5;2)" });
    aSheet1.setCell(76, 0, Cell { CellValue::number(0.5), u"of:=LOGNORMDIST(1;0;1)" });
    aSheet1.setCell(77, 0, Cell { CellValue::number(0.398942280401433),
        u"of:=COM.MICROSOFT.LOGNORM.DIST(1;0;1;FALSE())" });
    aSheet1.setCell(78, 0, Cell { CellValue::number(0.931933160851048),
        u"of:=LEGACY.FINV(0.5;5;10)" });
    aSheet1.setCell(79, 0, Cell { CellValue::number(1.9431802805153), u"of:=TINV(0.1;6)" });
    aSheet1.setCell(80, 0, Cell { CellValue::number(2.66666666666667), u"of:=VARP(2;6;4)" });
    aSheet1.setCell(81, 0, Cell { CellValue::number(4.0), u"of:=COM.MICROSOFT.VAR.S(2;6;4)" });
    aSheet1.setCell(82, 0, Cell { CellValue::number(6.66666666666667), u"of:=VARA(\"red\";2;6;4)" });
    aSheet1.setCell(83, 0, Cell { CellValue::number(5.0), u"of:=VARPA(\"red\";2;6;4)" });
    aSheet1.setCell(84, 0, Cell { CellValue::number(1.63299316185545),
        u"of:=COM.MICROSOFT.STDEV.P(2;6;4)" });
    aSheet1.setCell(85, 0, Cell { CellValue::number(1.77245385090552), u"of:=GAMMA(0.5)" });
    aSheet1.setCell(86, 0, Cell { CellValue::number(0.34089313230206),
        u"of:=COM.MICROSOFT.T.DIST.2T(1;10)" });
    aSheet1.setCell(87, 0, Cell { CellValue::number(2.0), u"of:=MODE.SNGL({1;2;2;3})" });
    aSheet1.setCell(88, 0, Cell { CellValue::number(2.5), u"of:=TRIMMEAN({1;2;3;100};0.5)" });
    aSheet1.setCell(89, 0, Cell { CellValue::number(2.0), u"of:=MAXA(FALSE();\"red\";2)" });
    aSheet1.setCell(90, 0, Cell { CellValue::number(0.0), u"of:=MINA(TRUE();\"red\";2)" });
    aSheet1.setCell(51, 139, Cell { CellValue::text(u"Q1") });
    aSheet1.setCell(51, 140, Cell { CellValue::text(u"Q2") });
    aSheet1.setCell(52, 138, Cell { CellValue::text(u"total sales") });
    aSheet1.setCell(53, 138, Cell { CellValue::text(u"cost of sales") });
    aSheet1.setCell(54, 138, Cell { CellValue::text(u"gross profit") });
    aSheet1.setCell(55, 138, Cell { CellValue::empty() });
    aSheet1.setCell(56, 138, Cell { CellValue::text(u"tax") });
    aSheet1.setCell(57, 138, Cell { CellValue::empty() });
    aSheet1.setCell(58, 138, Cell { CellValue::text(u"net profit") });
    aSheet1.setCell(59, 138, Cell { CellValue::text(u"profit [%]") });
    aSheet1.setCell(52, 139, Cell { CellValue::number(50000.0) });
    aSheet1.setCell(53, 139, Cell { CellValue::number(-25000.0) });
    aSheet1.setCell(54, 139, Cell { CellValue::number(25000.0) });
    aSheet1.setCell(55, 139, Cell { CellValue::empty() });
    aSheet1.setCell(56, 139, Cell { CellValue::number(-4246.0) });
    aSheet1.setCell(57, 139, Cell { CellValue::empty() });
    aSheet1.setCell(58, 139, Cell { CellValue::number(19342.0) });
    aSheet1.setCell(59, 139, Cell { CellValue::number(0.293) });
    aSheet1.setCell(52, 140, Cell { CellValue::number(510300.0) });
    aSheet1.setCell(53, 140, Cell { CellValue::number(-320900.0) });
    aSheet1.setCell(54, 140, Cell { CellValue::number(189400.0) });
    aSheet1.setCell(55, 140, Cell { CellValue::empty() });
    aSheet1.setCell(56, 140, Cell { CellValue::number(-25000.0) });
    aSheet1.setCell(57, 140, Cell { CellValue::empty() });
    aSheet1.setCell(58, 140, Cell { CellValue::number(140000.0) });
    aSheet1.setCell(59, 140, Cell { CellValue::number(0.274) });

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
    auto requireNumeric = [&](const CellAddress& rAddress, double fExpected, const char* pMessage,
                              bool bCompiled = false) -> int {
        const auto aResult = bCompiled ? aEvaluator.evaluateCellViaCompiledTokens(rAddress)
                                       : aEvaluator.evaluateCell(rAddress);
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, fExpected))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", pMessage);
        }
        return 0;
    };
    auto requireLiveNumericCell = [&](const CellAddress& rAddress, const char* pMessage,
                                      bool bCompiled = false) -> int {
        const auto aResult = bCompiled ? aEvaluator.evaluateCellViaCompiledTokens(rAddress)
                                       : aEvaluator.evaluateCell(rAddress);
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber())
            return fail("spreadsheetengine_fods_evaluator_tests", pMessage);
        return 0;
    };
    auto requireBooleanFormula = [&](const auto& rResult, bool bExpected,
                                     const char* pMessage) -> int {
        if (!rResult || rResult.mbUsedCachedValue || !rResult.maValue.maValue.isBoolean()
            || !almostEqual(rResult.maValue.maValue.mfNumber, bExpected ? 1.0 : 0.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", pMessage);
        }
        return 0;
    };
    auto requireNumericFormula = [&](const auto& rResult, double fExpected,
                                     const char* pMessage) -> int {
        if (!rResult || rResult.mbUsedCachedValue || !rResult.maValue.maValue.isNumber()
            || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", pMessage);
        }
        return 0;
    };

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

    if (requireLiveNumericCell({ 0, 10, 2 },
            "typed stored date cell materialization mismatch"))
    {
        return 1;
    }

    if (requireLiveNumericCell({ 0, 10, 2 },
            "compiled typed stored date cell materialization mismatch", true))
    {
        return 1;
    }

    {
        const auto aResult = aEvaluator.evaluateFormula(u"of:=ISNUMBER([.K3])", { 0, 0, 0 });
        if (requireBooleanFormula(aResult, true, "typed stored date ISNUMBER mismatch"))
            return 1;
    }

    {
        const auto aResult
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=ISNUMBER([.K3])", { 0, 0, 0 });
        if (requireBooleanFormula(
                aResult, true, "compiled typed stored date ISNUMBER mismatch"))
        {
            return 1;
        }
    }

    {
        const auto aResult = aEvaluator.evaluateFormula(u"of:=ISNUMBER(TRUE())", { 0, 0, 0 });
        if (requireBooleanFormula(aResult, true, "ISNUMBER(TRUE()) mismatch"))
            return 1;
    }

    {
        const auto aResult
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=ISNUMBER(TRUE())", { 0, 0, 0 });
        if (requireBooleanFormula(aResult, true, "compiled ISNUMBER(TRUE()) mismatch"))
            return 1;
    }

    {
        const auto aOr = aEvaluator.evaluateFormula(u"of:=OR(TRUE();FALSE())", { 0, 0, 0 });
        const auto aCompiledOr
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=OR(TRUE();FALSE())", { 0, 0, 0 });
        const auto aXor = aEvaluator.evaluateFormula(u"of:=XOR(TRUE();TRUE();FALSE())", { 0, 0, 0 });
        const auto aCompiledXor = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=XOR(TRUE();TRUE();FALSE())", { 0, 0, 0 });
        const auto aNot = aEvaluator.evaluateFormula(u"of:=NOT(0)", { 0, 0, 0 });
        const auto aCompiledNot
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=NOT(0)", { 0, 0, 0 });
        const auto aIsNonText = aEvaluator.evaluateFormula(u"of:=ISNONTEXT(1)", { 0, 0, 0 });
        const auto aCompiledIsNonText
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=ISNONTEXT(1)", { 0, 0, 0 });
        const auto aIfs
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.IFS(0;1;1;2)", { 0, 0, 0 });
        const auto aCompiledIfs = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.IFS(0;1;1;2)", { 0, 0, 0 });
        const auto aSwitch = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.SWITCH(\"b\";\"a\";1;\"b\";2;9)", { 0, 0, 0 });
        const auto aCompiledSwitch = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.SWITCH(\"b\";\"a\";1;\"b\";2;9)", { 0, 0, 0 });
        if (requireBooleanFormula(aOr, true, "OR mismatch")
            || requireBooleanFormula(aCompiledOr, true, "compiled OR mismatch")
            || requireBooleanFormula(aXor, false, "XOR mismatch")
            || requireBooleanFormula(aCompiledXor, false, "compiled XOR mismatch")
            || requireBooleanFormula(aNot, true, "NOT mismatch")
            || requireBooleanFormula(aCompiledNot, true, "compiled NOT mismatch")
            || requireBooleanFormula(aIsNonText, true, "ISNONTEXT mismatch")
            || requireBooleanFormula(aCompiledIsNonText, true, "compiled ISNONTEXT mismatch")
            || !aIfs || aIfs.mbUsedCachedValue || !aIfs.maValue.maValue.isNumber()
            || !almostEqual(aIfs.maValue.maValue.mfNumber, 2.0) || !aCompiledIfs
            || aCompiledIfs.mbUsedCachedValue || !aCompiledIfs.maValue.maValue.isNumber()
            || !almostEqual(aCompiledIfs.maValue.maValue.mfNumber, 2.0) || !aSwitch
            || aSwitch.mbUsedCachedValue || !aSwitch.maValue.maValue.isNumber()
            || !almostEqual(aSwitch.maValue.maValue.mfNumber, 2.0) || !aCompiledSwitch
            || aCompiledSwitch.mbUsedCachedValue || !aCompiledSwitch.maValue.maValue.isNumber()
            || !almostEqual(aCompiledSwitch.maValue.maValue.mfNumber, 2.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "logical/info function mismatch");
        }
    }

    {
        const auto aEven = aEvaluator.evaluateFormula(u"of:=EVEN(1.2)", { 0, 0, 0 });
        const auto aCompiledEven
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=EVEN(1.2)", { 0, 0, 0 });
        const auto aOdd = aEvaluator.evaluateFormula(u"of:=ODD(-2)", { 0, 0, 0 });
        const auto aCompiledOdd
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=ODD(-2)", { 0, 0, 0 });
        const auto aColor = aEvaluator.evaluateFormula(u"of:=COLOR(1;2;3)", { 0, 0, 0 });
        const auto aCompiledColor
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=COLOR(1;2;3)", { 0, 0, 0 });
        const auto aPermut = aEvaluator.evaluateFormula(u"of:=PERMUT(4;2)", { 0, 0, 0 });
        const auto aCompiledPermut
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=PERMUT(4;2)", { 0, 0, 0 });
        const auto aAverage = aEvaluator.evaluateFormula(u"of:=AVERAGE({1|2|3})", { 0, 0, 0 });
        const auto aCompiledAverage
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=AVERAGE({1|2|3})", { 0, 0, 0 });
        const auto aAverageA
            = aEvaluator.evaluateFormula(u"of:=AVERAGEA({1|TRUE|\"x\"})", { 0, 0, 0 });
        const auto aCompiledAverageA = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=AVERAGEA({1|TRUE|\"x\"})", { 0, 0, 0 });
        const auto aCorrel
            = aEvaluator.evaluateFormula(u"of:=CORREL({1|2|3};{1|2|3})", { 0, 0, 0 });
        const auto aCompiledCorrel = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CORREL({1|2|3};{1|2|3})", { 0, 0, 0 });
        const auto aIntercept
            = aEvaluator.evaluateFormula(u"of:=INTERCEPT({2|4|6};{1|2|3})", { 0, 0, 0 });
        const auto aCompiledIntercept = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=INTERCEPT({2|4|6};{1|2|3})", { 0, 0, 0 });
        const auto aForecast
            = aEvaluator.evaluateFormula(u"of:=FORECAST(4;{2|4|6};{1|2|3})", { 0, 0, 0 });
        const auto aCompiledForecast = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=FORECAST(4;{2|4|6};{1|2|3})", { 0, 0, 0 });
        const auto aDevSq = aEvaluator.evaluateFormula(u"of:=DEVSQ({1|2|3})", { 0, 0, 0 });
        const auto aCompiledDevSq
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=DEVSQ({1|2|3})", { 0, 0, 0 });
        const auto aNumberValue
            = aEvaluator.evaluateFormula(u"of:=NUMBERVALUE(\"1,23\";\",\";\".\")", { 0, 0, 0 });
        const auto aCompiledNumberValue = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=NUMBERVALUE(\"1,23\";\",\";\".\")", { 0, 0, 0 });
        const auto aRegex = aEvaluator.evaluateFormula(u"of:=REGEX(\"abc123\";\"[0-9]+\")",
            { 0, 0, 0 });
        const auto aCompiledRegex = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=REGEX(\"abc123\";\"[0-9]+\")", { 0, 0, 0 });
        const auto aRegexReplace = aEvaluator.evaluateFormula(
            u"of:=REGEX(\"a1b2\";\"[0-9]\";\"x\";\"g\")", { 0, 0, 0 });
        const auto aCompiledRegexReplace = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=REGEX(\"a1b2\";\"[0-9]\";\"x\";\"g\")", { 0, 0, 0 });
        if (const int nResult = requireNumericFormula(aEven, 2.0, "EVEN mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledEven, 2.0, "compiled EVEN mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aOdd, -3.0, "ODD mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledOdd, -3.0, "compiled ODD mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aColor, 66051.0, "COLOR mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledColor, 66051.0, "compiled COLOR mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aPermut, 12.0, "PERMUT mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledPermut, 12.0, "compiled PERMUT mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aAverage, 2.0, "AVERAGE mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledAverage, 2.0, "compiled AVERAGE mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(
                aAverageA, 0.6666666666666666, "AVERAGEA mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aCompiledAverageA, 0.6666666666666666,
                "compiled AVERAGEA mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aCorrel, 1.0, "CORREL mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledCorrel, 1.0, "compiled CORREL mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aIntercept, 0.0, "INTERCEPT mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledIntercept, 0.0, "compiled INTERCEPT mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aForecast, 8.0, "FORECAST mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledForecast, 8.0, "compiled FORECAST mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(aDevSq, 2.0, "DEVSQ mismatch"))
            return nResult;
        if (const int nResult = requireNumericFormula(
                aCompiledDevSq, 2.0, "compiled DEVSQ mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(
                aNumberValue, 1.23, "NUMBERVALUE mismatch"))
        {
            return nResult;
        }
        if (const int nResult = requireNumericFormula(
                aCompiledNumberValue, 1.23, "compiled NUMBERVALUE mismatch"))
        {
            return nResult;
        }
        if (!aRegex || aRegex.mbUsedCachedValue || !aRegex.maValue.maValue.isText()
            || aRegex.maValue.maValue.maString != u"123" || !aCompiledRegex
            || aCompiledRegex.mbUsedCachedValue || !aCompiledRegex.maValue.maValue.isText()
            || aCompiledRegex.maValue.maValue.maString != u"123" || !aRegexReplace
            || aRegexReplace.mbUsedCachedValue || !aRegexReplace.maValue.maValue.isText()
            || aRegexReplace.maValue.maValue.maString != u"axbx" || !aCompiledRegexReplace
            || aCompiledRegexReplace.mbUsedCachedValue
            || !aCompiledRegexReplace.maValue.maValue.isText()
            || aCompiledRegexReplace.maValue.maValue.maString != u"axbx")
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "math/text promotion mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 6, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 42.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "live scalar mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 6, 0 });
        if (!aResult || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 42.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled live scalar mismatch");
        }
    }

    if (const int nResult = requireNumeric(
            { 0, 23, 0 }, 1.0, "YEARFRAC() mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 23, 0 }, 1.0, "compiled YEARFRAC() mismatch", true))
    {
        return nResult;
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 24, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isText()
            || aResult.maValue.maValue.maString != u"A")
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled external-name text mismatch");
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

    if (const int nResult = requireNumeric(
            { 0, 32, 0 }, 95.0428743993921, "PRICE() mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 32, 0 }, 95.0428743993921, "compiled PRICE() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 33, 0 }, 95.0780346202577, "compiled PRICE() default-basis mismatch", true))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 34, 0 }, 48.0, "SUM() mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 34, 0 }, 48.0, "compiled SUM() mismatch", true))
    {
        return nResult;
    }

    {
        const auto aResult = aEvaluator.evaluateCell({ 0, 35, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isText()
            || aResult.maValue.maValue.maString != u"1899-12-26 12:00:00")
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "BASISODATETIME() mismatch");
        }
    }

    {
        const auto aResult = aEvaluator.evaluateCellViaCompiledTokens({ 0, 35, 0 });
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isText()
            || aResult.maValue.maValue.maString != u"1899-12-26 12:00:00")
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled BASISODATETIME() mismatch");
        }
    }

    if (const int nResult
        = requireNumeric({ 0, 64, 0 }, 0.5, "NORMDIST() cumulative mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 65, 0 }, 0.199471140200716, "NORMDIST() density mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 66, 0 }, 0.77686983985157, "CHISQDIST() cumulative mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 67, 0 }, 0.111565080074215, "CHISQDIST() density mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 68, 0 }, 0.550671035882778, "GAMMADIST() cumulative mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 69, 0 }, 10.0, "GAMMADIST() density-at-zero mismatch", true))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 70, 0 }, 0.809090909090909, "HYPGEOMDIST() density mismatch"))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 71, 0 }, 1.0, "HYPGEOMDIST() cumulative mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 72, 0 }, 4.0, "BINOM.INV() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 73, 0 }, 0.83, "PERCENTRANK() mismatch"))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 74, 0 }, 0.83, "PERCENTRANK.INC() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 75, 0 }, 0.7, "PERCENTRANK.EXC() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 76, 0 }, 0.5, "LOGNORMDIST() mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 77, 0 }, 0.398942280401433, "LOGNORM.DIST() density mismatch", true))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 78, 0 }, 0.931933160851048, "LEGACY.FINV() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 79, 0 }, 1.9431802805153, "TINV() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 80, 0 }, 2.66666666666667, "VARP() mismatch"))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 81, 0 }, 4.0, "VAR.S() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 82, 0 }, 6.66666666666667, "VARA() mismatch"))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 83, 0 }, 5.0, "VARPA() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 84, 0 }, 1.63299316185545, "STDEV.P() mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 85, 0 }, 1.77245385090552, "GAMMA() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric(
            { 0, 86, 0 }, 0.34089313230206, "T.DIST.2T() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 87, 0 }, 2.0, "MODE.SNGL() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult
        = requireNumeric({ 0, 88, 0 }, 2.5, "TRIMMEAN() mismatch", true))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric({ 0, 89, 0 }, 2.0, "MAXA() mismatch"))
    {
        return nResult;
    }

    if (const int nResult = requireNumeric({ 0, 90, 0 }, 0.0, "MINA() mismatch"))
    {
        return nResult;
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
        const auto aScalarSum = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.AGGREGATE(9;6;3;4;5)", { 0, 0, 0 });
        const auto aCompiledScalarSum = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.AGGREGATE(9;6;3;4;5)", { 0, 0, 0 });

        const auto checkAggregateNumber = [&](const char* pLabel, const auto& rResult,
                                              double fExpected) -> bool {
            if (!rResult || rResult.mbUsedCachedValue || !rResult.maValue.maValue.isNumber()
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: aggregate mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkAggregateNumber("AGGREGATE scalar sum", aScalarSum, 12.0)
            || !checkAggregateNumber("compiled AGGREGATE scalar sum", aCompiledScalarSum, 12.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "AGGREGATE() extended evaluation mismatch");
        }
    }

    {
        const auto aSubtotal = aEvaluator.evaluateFormula(
            u"of:=SUBTOTAL(9;[.$A$61:.$A$63])", { 0, 0, 0 });
        const auto aSubtotalCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SUBTOTAL(9;[.$A$61:.$A$63])", { 0, 0, 0 });
        const auto aSubtotalHidden = aEvaluator.evaluateFormula(
            u"of:=SUBTOTAL(109;[.$A$61:.$A$63])", { 0, 0, 0 });
        const auto aSubtotalHiddenCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SUBTOTAL(109;[.$A$61:.$A$63])", { 0, 0, 0 });

        const auto checkSubtotal = [&](const char* pLabel, const auto& rResult,
                                       double fExpected) -> bool {
            if (!rResult || rResult.mbUsedCachedValue || !rResult.maValue.maValue.isNumber()
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: subtotal mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkSubtotal("SUBTOTAL filtered rows", aSubtotal, 40.0)
            || !checkSubtotal("compiled SUBTOTAL filtered rows", aSubtotalCompiled, 40.0)
            || !checkSubtotal("SUBTOTAL filtered+hidden rows", aSubtotalHidden, 10.0)
            || !checkSubtotal("compiled SUBTOTAL filtered+hidden rows", aSubtotalHiddenCompiled,
                10.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "SUBTOTAL() filtered-row evaluation mismatch");
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
        const auto aVectorLookup
            = aEvaluator.evaluateFormula(u"of:=LOOKUP(3;[.BA1:.BA4];[.BB1:.BB4])", { 0, 0, 0 });
        const auto aArrayLookup
            = aEvaluator.evaluateFormula(u"of:=LOOKUP(3;[.BA1:.BB4])", { 0, 0, 0 });
        const auto aTextLookup = aEvaluator.evaluateFormula(
            u"of:=LOOKUP(\"F\";[.BC1:.BF1];[.BC2:.BF2])", { 0, 0, 0 });
        const auto aScalarLookup = aEvaluator.evaluateFormula(u"of:=LOOKUP(1;1;3)", { 0, 0, 0 });
        const auto aMmultLookup
            = aEvaluator.evaluateFormula(u"of:=LOOKUP(4;MMULT([.BK1:.BK4];1);[.BL1:.BL4])", { 0, 0, 0 });
        const auto aMmultArrayLookup
            = aEvaluator.evaluateFormula(u"of:=LOOKUP(4;MMULT([.BK1:.BK4];1))", { 0, 0, 0 });
        const auto aInvalidMmultLookup
            = aEvaluator.evaluateFormula(u"of:=LOOKUP(4;MMULT([.BK1:.BL4];1))", { 0, 0, 0 });
        const auto aCompiledVectorLookup = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LOOKUP(3;[.BA1:.BA4];[.BB1:.BB4])", { 0, 0, 0 });
        const auto aCompiledArrayLookup = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LOOKUP(3;[.BA1:.BB4])", { 0, 0, 0 });
        const auto aCompiledTextLookup = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LOOKUP(\"F\";[.BC1:.BF1];[.BC2:.BF2])", { 0, 0, 0 });
        const auto aCompiledScalarLookup = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LOOKUP(1;1;3)", { 0, 0, 0 });
        const auto aCompiledMmultLookup = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LOOKUP(4;MMULT([.BK1:.BK4];1);[.BL1:.BL4])", { 0, 0, 0 });
        const auto aCompiledMmultArrayLookup = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LOOKUP(4;MMULT([.BK1:.BK4];1))", { 0, 0, 0 });
        const auto aCompiledInvalidMmultLookup = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LOOKUP(4;MMULT([.BK1:.BL4];1))", { 0, 0, 0 });

        const auto checkLookupNumber = [&](const char* pLabel, const auto& rResult,
                                           double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: lookup mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto checkLookupText = [&](const char* pLabel, const auto& rResult,
                                         std::u16string_view rExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isText() || rResult.mbUsedCachedValue
                || rResult.maValue.maValue.maString != rExpected)
            {
                std::fprintf(stderr, "%s: lookup mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto checkLookupError = [&](const char* pLabel, const auto& rResult,
                                          spreadsheetengine::api::Error eExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isError() || rResult.mbUsedCachedValue
                || rResult.maValue.maValue.meError != eExpected)
            {
                std::fprintf(stderr, "%s: lookup mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkLookupNumber("vector LOOKUP", aVectorLookup, 22.0)
            || !checkLookupNumber("array LOOKUP", aArrayLookup, 22.0)
            || !checkLookupText("text LOOKUP", aTextLookup, u"VAN")
            || !checkLookupNumber("scalar LOOKUP", aScalarLookup, 3.0)
            || !checkLookupNumber("MMULT LOOKUP", aMmultLookup, 30.0)
            || !checkLookupNumber("MMULT array LOOKUP", aMmultArrayLookup, 3.0)
            || !checkLookupError("invalid MMULT LOOKUP", aInvalidMmultLookup,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkLookupNumber("compiled vector LOOKUP", aCompiledVectorLookup, 22.0)
            || !checkLookupNumber("compiled array LOOKUP", aCompiledArrayLookup, 22.0)
            || !checkLookupText("compiled text LOOKUP", aCompiledTextLookup, u"VAN")
            || !checkLookupNumber("compiled scalar LOOKUP", aCompiledScalarLookup, 3.0)
            || !checkLookupNumber("compiled MMULT LOOKUP", aCompiledMmultLookup, 30.0)
            || !checkLookupNumber("compiled MMULT array LOOKUP", aCompiledMmultArrayLookup, 3.0)
            || !checkLookupError("compiled invalid MMULT LOOKUP", aCompiledInvalidMmultLookup,
                spreadsheetengine::api::Error::IllegalArgument))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "lookup evaluation mismatch");
        }
    }

    {
        const auto aChooseCols = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CHOOSECOLS([.BC1:.BF2];2;-1)", { 0, 0, 0 });
        const auto aChooseColsArray = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CHOOSECOLS([.BA1:.BB4];{2|1})", { 0, 0, 0 });
        const auto aChooseColsInvalid = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CHOOSECOLS([.BA1:.BB4];0)", { 0, 0, 0 });
        const auto aChooseRows = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CHOOSEROWS([.BA1:.BB4];2;-1)", { 0, 0, 0 });
        const auto aChooseRowsArray = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CHOOSEROWS([.BA1:.BB4];{-1|2})", { 0, 0, 0 });
        const auto aChooseRowsInvalid = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CHOOSEROWS([.BA1:.BB4];0)", { 0, 0, 0 });

        const auto aCompiledChooseCols = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CHOOSECOLS([.BC1:.BF2];2;-1)", { 0, 0, 0 });
        const auto aCompiledChooseColsArray = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CHOOSECOLS([.BA1:.BB4];{2|1})", { 0, 0, 0 });
        const auto aCompiledChooseColsInvalid = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CHOOSECOLS([.BA1:.BB4];0)", { 0, 0, 0 });
        const auto aCompiledChooseRows = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CHOOSEROWS([.BA1:.BB4];2;-1)", { 0, 0, 0 });
        const auto aCompiledChooseRowsArray = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CHOOSEROWS([.BA1:.BB4];{-1|2})", { 0, 0, 0 });
        const auto aCompiledChooseRowsInvalid = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CHOOSEROWS([.BA1:.BB4];0)", { 0, 0, 0 });

        const auto checkChooseNumber = [&](const char* pLabel, const auto& rResult,
                                           double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: CHOOSECOLS/CHOOSEROWS mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto checkChooseText = [&](const char* pLabel, const auto& rResult,
                                         std::u16string_view rExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isText() || rResult.mbUsedCachedValue
                || rResult.maValue.maValue.maString != rExpected)
            {
                std::fprintf(stderr, "%s: CHOOSECOLS/CHOOSEROWS mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto checkChooseError = [&](const char* pLabel, const auto& rResult) -> bool {
            if (rResult || rResult.meError != spreadsheetengine::api::Error::IllegalArgument)
            {
                std::fprintf(stderr, "%s: CHOOSECOLS/CHOOSEROWS mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkChooseText("CHOOSECOLS text anchor", aChooseCols, u"C")
            || !checkChooseNumber("CHOOSECOLS array selection", aChooseColsArray, 11.0)
            || !checkChooseError("CHOOSECOLS invalid index", aChooseColsInvalid)
            || !checkChooseNumber("CHOOSEROWS numeric anchor", aChooseRows, 2.0)
            || !checkChooseNumber("CHOOSEROWS array selection", aChooseRowsArray, 8.0)
            || !checkChooseError("CHOOSEROWS invalid index", aChooseRowsInvalid)
            || !checkChooseText("compiled CHOOSECOLS text anchor", aCompiledChooseCols, u"C")
            || !checkChooseNumber(
                "compiled CHOOSECOLS array selection", aCompiledChooseColsArray, 11.0)
            || !checkChooseError(
                "compiled CHOOSECOLS invalid index", aCompiledChooseColsInvalid)
            || !checkChooseNumber("compiled CHOOSEROWS numeric anchor",
                aCompiledChooseRows, 2.0)
            || !checkChooseNumber("compiled CHOOSEROWS array selection",
                aCompiledChooseRowsArray, 8.0)
            || !checkChooseError(
                "compiled CHOOSEROWS invalid index", aCompiledChooseRowsInvalid))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "CHOOSECOLS/CHOOSEROWS evaluation mismatch");
        }
    }

    {
        const auto aRepoRoot = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();
        const auto aErrorTypePath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                    / "spreadsheet" / "fods" / "error.type.fods";
        const auto aLegacyErrorTypePath = aRepoRoot / "sc" / "qa" / "unit" / "data"
                                          / "functions" / "spreadsheet" / "fods"
                                          / "errortype.fods";
        const auto aLoadResult = spreadsheetengine::core::fods::loadWorkbook(aErrorTypePath.string());
        if (!aLoadResult)
            return fail("spreadsheetengine_fods_evaluator_tests", "error.type.fods load failed");
        const auto aLegacyLoadResult
            = spreadsheetengine::core::fods::loadWorkbook(aLegacyErrorTypePath.string());
        if (!aLegacyLoadResult)
            return fail("spreadsheetengine_fods_evaluator_tests", "errortype.fods load failed");

        Evaluator aErrorTypeEvaluator(aLoadResult.maValue.maWorkbook);
        Evaluator aLegacyErrorTypeEvaluator(aLegacyLoadResult.maValue.maWorkbook);
        const auto reportErrorTypeMismatch = [&](const char* pLabel) -> bool {
            std::fprintf(stderr, "%s: ERROR.TYPE mismatch in %s\n",
                "spreadsheetengine_fods_evaluator_tests", pLabel);
            return false;
        };
        const auto checkErrorTypeNumber = [&](const char* pLabel, const auto& rResult,
                                              double fExpected) -> bool {
            if (!rResult || rResult.mbUsedCachedValue || !rResult.maValue.maValue.isNumber()
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                return reportErrorTypeMismatch(pLabel);
            }
            return true;
        };
        const auto checkErrorTypeNa = [&](const char* pLabel, const auto& rResult) -> bool {
            if (!rResult || rResult.mbUsedCachedValue || !rResult.maValue.maValue.isError()
                || rResult.maValue.maValue.meError != spreadsheetengine::api::Error::NotAvailable)
            {
                return reportErrorTypeMismatch(pLabel);
            }
            return true;
        };
        const auto checkErrorTypeValueError = [&](const char* pLabel, const auto& rResult) -> bool {
            if (!rResult || rResult.mbUsedCachedValue || !rResult.maValue.maValue.isError()
                || rResult.maValue.maValue.meError
                       != spreadsheetengine::api::Error::IllegalArgument)
            {
                return reportErrorTypeMismatch(pLabel);
            }
            return true;
        };

        const CellAddress aSheet2Origin { 1, 0, 0 };
        const auto aNa = aErrorTypeEvaluator.evaluateFormula(u"of:=ERROR.TYPE(NA())", aSheet2Origin);
        const auto aRef
            = aErrorTypeEvaluator.evaluateFormula(u"of:=ERROR.TYPE(#REF!)", aSheet2Origin);
        const auto aGettingData = aErrorTypeEvaluator.evaluateFormula(
            u"of:=ERROR.TYPE(#getting_data)", aSheet2Origin);
        const auto aName
            = aErrorTypeEvaluator.evaluateFormula(u"of:=ERROR.TYPE(#NAME?)", aSheet2Origin);
        const auto aDivZero
            = aErrorTypeEvaluator.evaluateFormula(u"of:=ERROR.TYPE([.F11])", aSheet2Origin);
        const auto aGettingDataReference
            = aErrorTypeEvaluator.evaluateFormula(u"of:=ERROR.TYPE([.A9])", aSheet2Origin);
        const auto aUnknownName
            = aErrorTypeEvaluator.evaluateFormula(u"of:=ERROR.TYPE(ahoj)", aSheet2Origin);
        const auto aNonError
            = aErrorTypeEvaluator.evaluateFormula(u"of:=ERROR.TYPE(5)", aSheet2Origin);
        const auto aArrayConstant
            = aErrorTypeEvaluator.evaluateFormula(u"of:=ERROR.TYPE({#N/A})", aSheet2Origin);

        const auto aCompiledNa = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ERROR.TYPE(NA())", aSheet2Origin);
        const auto aCompiledRef = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ERROR.TYPE(#REF!)", aSheet2Origin);
        const auto aCompiledGettingData = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ERROR.TYPE(#getting_data)", aSheet2Origin);
        const auto aCompiledName = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ERROR.TYPE(#NAME?)", aSheet2Origin);
        const auto aCompiledDivZero = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ERROR.TYPE([.F11])", aSheet2Origin);
        const auto aCompiledGettingDataReference
            = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=ERROR.TYPE([.A9])", aSheet2Origin);
        const auto aCompiledUnknownName = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ERROR.TYPE(ahoj)", aSheet2Origin);
        const auto aCompiledNonError = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ERROR.TYPE(5)", aSheet2Origin);
        const auto aCompiledArrayConstant
            = aErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=ERROR.TYPE({#N/A})", aSheet2Origin);
        const auto aLegacyRef = aLegacyErrorTypeEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(#REF!)", { 0, 0, 0 });
        const auto aLegacyGettingData = aLegacyErrorTypeEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(#getting_data)", { 0, 0, 0 });
        const auto aLegacyName = aLegacyErrorTypeEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(#NAME?)", { 0, 0, 0 });
        const auto aLegacyErr7 = aLegacyErrorTypeEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(err:7)", { 0, 0, 0 });
        const auto aLegacyAhoj = aLegacyErrorTypeEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(ahoj)", { 0, 0, 0 });

        const auto aCompiledLegacyRef = aLegacyErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(#REF!)", { 0, 0, 0 });
        const auto aCompiledLegacyName = aLegacyErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(#NAME?)", { 0, 0, 0 });
        const auto aCompiledLegacyErr7 = aLegacyErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(err:7)", { 0, 0, 0 });
        const auto aCompiledLegacyAhoj = aLegacyErrorTypeEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ORG.OPENOFFICE.ERRORTYPE(ahoj)", { 0, 0, 0 });
        const auto aLegacyCellA8 = aLegacyErrorTypeEvaluator.evaluateCell({ 1, 0, 7 });
        const auto aLegacyCellA12 = aLegacyErrorTypeEvaluator.evaluateCell({ 1, 0, 11 });
        const auto aLegacyCellA14 = aLegacyErrorTypeEvaluator.evaluateCell({ 1, 0, 13 });
        const auto aLegacyCellA15 = aLegacyErrorTypeEvaluator.evaluateCell({ 1, 0, 14 });
        const auto aLegacyCellA17 = aLegacyErrorTypeEvaluator.evaluateCell({ 1, 0, 16 });
        const auto aLegacyCellA18 = aLegacyErrorTypeEvaluator.evaluateCell({ 1, 0, 17 });
        const auto aLegacyCellA19 = aLegacyErrorTypeEvaluator.evaluateCell({ 1, 0, 18 });
        const auto aCompiledLegacyCellA8
            = aLegacyErrorTypeEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 7 });
        const auto aCompiledLegacyCellA12
            = aLegacyErrorTypeEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 11 });
        const auto aCompiledLegacyCellA14
            = aLegacyErrorTypeEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 13 });
        const auto aCompiledLegacyCellA15
            = aLegacyErrorTypeEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 14 });
        const auto aCompiledLegacyCellA17
            = aLegacyErrorTypeEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 16 });
        const auto aCompiledLegacyCellA18
            = aLegacyErrorTypeEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 17 });
        const auto aCompiledLegacyCellA19
            = aLegacyErrorTypeEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 18 });

        if (!checkErrorTypeNumber("ERROR.TYPE(NA())", aNa, 7.0)
            || !checkErrorTypeNumber("ERROR.TYPE(#REF!)", aRef, 4.0)
            || !checkErrorTypeValueError("ERROR.TYPE(#getting_data)", aGettingData)
            || !checkErrorTypeNumber("ERROR.TYPE(#NAME?)", aName, 5.0)
            || !checkErrorTypeNumber("ERROR.TYPE(F11)", aDivZero, 2.0)
            || !checkErrorTypeNa("ERROR.TYPE(A9)", aGettingDataReference)
            || !checkErrorTypeNumber("ERROR.TYPE(ahoj)", aUnknownName, 5.0)
            || !checkErrorTypeNa("ERROR.TYPE(5)", aNonError)
            || !checkErrorTypeNumber("ERROR.TYPE({#N/A})", aArrayConstant, 7.0)
            || !checkErrorTypeNumber("compiled ERROR.TYPE(NA())", aCompiledNa, 7.0)
            || !checkErrorTypeNumber("compiled ERROR.TYPE(#REF!)", aCompiledRef, 4.0)
            || !checkErrorTypeValueError(
                "compiled ERROR.TYPE(#getting_data)", aCompiledGettingData)
            || !checkErrorTypeNumber("compiled ERROR.TYPE(#NAME?)", aCompiledName, 5.0)
            || !checkErrorTypeNumber("compiled ERROR.TYPE(F11)", aCompiledDivZero, 2.0)
            || !checkErrorTypeNa("compiled ERROR.TYPE(A9)", aCompiledGettingDataReference)
            || !checkErrorTypeNumber("compiled ERROR.TYPE(ahoj)", aCompiledUnknownName, 5.0)
            || !checkErrorTypeNa("compiled ERROR.TYPE(5)", aCompiledNonError)
            || !checkErrorTypeNumber(
                "compiled ERROR.TYPE({#N/A})", aCompiledArrayConstant, 7.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE(#REF!)", aLegacyRef, 524.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE(#getting_data)", aLegacyGettingData, 508.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE(#NAME?)", aLegacyName, 525.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE(err:7)", aLegacyErr7, 525.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE(ahoj)", aLegacyAhoj, 525.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE cell A8", aLegacyCellA8, 525.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE cell A12", aLegacyCellA12, 532.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE cell A14", aLegacyCellA14, 508.0)
            || !checkErrorTypeNa(
                "ERRORTYPE cell A15", aLegacyCellA15)
            || !checkErrorTypeNumber(
                "ERRORTYPE cell A17", aLegacyCellA17, 511.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE cell A18", aLegacyCellA18, 32767.0)
            || !checkErrorTypeNumber(
                "ERRORTYPE cell A19", aLegacyCellA19, 519.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE(#REF!)", aCompiledLegacyRef, 524.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE(#NAME?)", aCompiledLegacyName, 525.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE(err:7)", aCompiledLegacyErr7, 525.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE(ahoj)", aCompiledLegacyAhoj, 525.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE cell A8", aCompiledLegacyCellA8, 525.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE cell A12", aCompiledLegacyCellA12, 532.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE cell A14", aCompiledLegacyCellA14, 508.0)
            || !checkErrorTypeNa(
                "compiled ERRORTYPE cell A15", aCompiledLegacyCellA15)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE cell A17", aCompiledLegacyCellA17, 511.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE cell A18", aCompiledLegacyCellA18, 32767.0)
            || !checkErrorTypeNumber(
                "compiled ERRORTYPE cell A19", aCompiledLegacyCellA19, 519.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "ERROR.TYPE evaluation mismatch");
        }
    }

    {
        const auto aMatchExact = aEvaluator.evaluateFormula(u"of:=MATCH(4;[.BA1:.BA4];0)", { 0, 0, 0 });
        const auto aMatchApproxAsc
            = aEvaluator.evaluateFormula(u"of:=MATCH(5;[.BA1:.BA4];1)", { 0, 0, 0 });
        const auto aMatchApproxDesc
            = aEvaluator.evaluateFormula(u"of:=MATCH(6;[.BI1:.BI4];-1)", { 0, 0, 0 });
        const auto aMatchText
            = aEvaluator.evaluateFormula(u"of:=MATCH(\"E\";[.BC1:.BF1];0)", { 0, 0, 0 });

        const auto aCompiledMatchExact
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=MATCH(4;[.BA1:.BA4];0)", { 0, 0, 0 });
        const auto aCompiledMatchApproxAsc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MATCH(5;[.BA1:.BA4];1)", { 0, 0, 0 });
        const auto aCompiledMatchApproxDesc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MATCH(6;[.BI1:.BI4];-1)", { 0, 0, 0 });
        const auto aCompiledMatchText = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MATCH(\"E\";[.BC1:.BF1];0)", { 0, 0, 0 });
        const auto aXMatchExact = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XMATCH(4;[.BA1:.BA4])", { 0, 0, 0 });
        const auto aXMatchText = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XMATCH(\"E\";[.BC1:.BF1])", { 0, 0, 0 });
        const auto aXMatchReverse = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XMATCH(4;[.BH1:.BH4];0;-1)", { 0, 0, 0 });
        const auto aXMatchNextSmaller = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XMATCH(5;[.BA1:.BA4];-1;2)", { 0, 0, 0 });
        const auto aXMatchNextLarger = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XMATCH(5;[.BA1:.BA4];1;2)", { 0, 0, 0 });

        const auto checkMatchNumber = [&](const char* pLabel, const auto& rResult,
                                          double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: MATCH mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto aCompiledXMatchExact = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XMATCH(4;[.BA1:.BA4])", { 0, 0, 0 });
        const auto aCompiledXMatchText = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XMATCH(\"E\";[.BC1:.BF1])", { 0, 0, 0 });
        const auto aCompiledXMatchReverse = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XMATCH(4;[.BH1:.BH4];0;-1)", { 0, 0, 0 });
        const auto aCompiledXMatchNextSmaller = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XMATCH(5;[.BA1:.BA4];-1;2)", { 0, 0, 0 });
        const auto aCompiledXMatchNextLarger = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XMATCH(5;[.BA1:.BA4];1;2)", { 0, 0, 0 });

        if (!checkMatchNumber("exact MATCH", aMatchExact, 3.0)
            || !checkMatchNumber("ascending MATCH", aMatchApproxAsc, 3.0)
            || !checkMatchNumber("descending MATCH", aMatchApproxDesc, 2.0)
            || !checkMatchNumber("text MATCH", aMatchText, 3.0)
            || !checkMatchNumber("compiled exact MATCH", aCompiledMatchExact, 3.0)
            || !checkMatchNumber("compiled ascending MATCH", aCompiledMatchApproxAsc, 3.0)
            || !checkMatchNumber("compiled descending MATCH", aCompiledMatchApproxDesc, 2.0)
            || !checkMatchNumber("compiled text MATCH", aCompiledMatchText, 3.0)
            || !checkMatchNumber("exact XMATCH", aXMatchExact, 3.0)
            || !checkMatchNumber("text XMATCH", aXMatchText, 3.0)
            || !checkMatchNumber("reverse XMATCH", aXMatchReverse, 3.0)
            || !checkMatchNumber("next smaller XMATCH", aXMatchNextSmaller, 3.0)
            || !checkMatchNumber("next larger XMATCH", aXMatchNextLarger, 4.0)
            || !checkMatchNumber("compiled exact XMATCH", aCompiledXMatchExact, 3.0)
            || !checkMatchNumber("compiled text XMATCH", aCompiledXMatchText, 3.0)
            || !checkMatchNumber("compiled reverse XMATCH", aCompiledXMatchReverse, 3.0)
            || !checkMatchNumber("compiled next smaller XMATCH", aCompiledXMatchNextSmaller, 3.0)
            || !checkMatchNumber("compiled next larger XMATCH", aCompiledXMatchNextLarger, 4.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "MATCH/XMATCH evaluation mismatch");
        }
    }

    {
        const auto aXLookupExact
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.XLOOKUP(4;[.BA1:.BA4];[.BB1:.BB4])", { 0, 0, 0 });
        const auto aXLookupText = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XLOOKUP(\"E\";[.BC1:.BF1];[.BC2:.BF2])", { 0, 0, 0 });
        const auto aXLookupMissing = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XLOOKUP(5;[.BA1:.BA4];[.BB1:.BB4];\"missing\")", { 0, 0, 0 });
        const auto aXLookupReverse = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XLOOKUP(4;[.BH1:.BH4];[.BB1:.BB4];;0;-1)", { 0, 0, 0 });
        const auto aXLookupNextSmaller = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XLOOKUP(5;[.BA1:.BA4];[.BB1:.BB4];;-1;2)", { 0, 0, 0 });
        const auto aXLookupNextLarger = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XLOOKUP(5;[.BA1:.BA4];[.BB1:.BB4];;1;2)", { 0, 0, 0 });
        const auto aNestedXLookupNetProfit = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.XLOOKUP([.BG139];[.BA139:.BH139];"
            u"COM.MICROSOFT.XLOOKUP([.AZ140];[.AZ140:.AZ141];[.BA140:.BH141]))",
            { 0, 0, 0 });

        const auto aCompiledXLookupExact = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XLOOKUP(4;[.BA1:.BA4];[.BB1:.BB4])", { 0, 0, 0 });
        const auto aCompiledXLookupText = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XLOOKUP(\"E\";[.BC1:.BF1];[.BC2:.BF2])", { 0, 0, 0 });
        const auto aCompiledXLookupMissing = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XLOOKUP(5;[.BA1:.BA4];[.BB1:.BB4];\"missing\")", { 0, 0, 0 });
        const auto aCompiledXLookupReverse = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XLOOKUP(4;[.BH1:.BH4];[.BB1:.BB4];;0;-1)", { 0, 0, 0 });
        const auto aCompiledXLookupNextSmaller = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XLOOKUP(5;[.BA1:.BA4];[.BB1:.BB4];;-1;2)", { 0, 0, 0 });
        const auto aCompiledXLookupNextLarger = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XLOOKUP(5;[.BA1:.BA4];[.BB1:.BB4];;1;2)", { 0, 0, 0 });
        const auto aCompiledNestedXLookupNetProfit = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.XLOOKUP([.BG139];[.BA139:.BH139];"
            u"COM.MICROSOFT.XLOOKUP([.AZ140];[.AZ140:.AZ141];[.BA140:.BH141]))",
            { 0, 0, 0 });

        const auto checkXLookupNumber = [&](const char* pLabel, const auto& rResult,
                                            double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: XLOOKUP mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto checkXLookupText = [&](const char* pLabel, const auto& rResult,
                                          std::u16string_view rExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isText() || rResult.mbUsedCachedValue
                || rResult.maValue.maValue.maString != rExpected)
            {
                std::fprintf(stderr, "%s: XLOOKUP mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkXLookupNumber("exact XLOOKUP", aXLookupExact, 44.0)
            || !checkXLookupText("text XLOOKUP", aXLookupText, u"VAN")
            || !checkXLookupText("missing XLOOKUP", aXLookupMissing, u"missing")
            || !checkXLookupNumber("reverse XLOOKUP", aXLookupReverse, 44.0)
            || !checkXLookupNumber("next smaller XLOOKUP", aXLookupNextSmaller, 44.0)
            || !checkXLookupNumber("next larger XLOOKUP", aXLookupNextLarger, 88.0)
            || !checkXLookupNumber("nested XLOOKUP net profit", aNestedXLookupNetProfit, 19342.0)
            || !checkXLookupNumber("compiled exact XLOOKUP", aCompiledXLookupExact, 44.0)
            || !checkXLookupText("compiled text XLOOKUP", aCompiledXLookupText, u"VAN")
            || !checkXLookupText("compiled missing XLOOKUP", aCompiledXLookupMissing, u"missing")
            || !checkXLookupNumber("compiled reverse XLOOKUP", aCompiledXLookupReverse, 44.0)
            || !checkXLookupNumber("compiled next smaller XLOOKUP", aCompiledXLookupNextSmaller, 44.0)
            || !checkXLookupNumber("compiled next larger XLOOKUP", aCompiledXLookupNextLarger, 88.0)
            || !checkXLookupNumber("compiled nested XLOOKUP net profit",
                aCompiledNestedXLookupNetProfit, 19342.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "XLOOKUP evaluation mismatch");
        }
    }

    {
        const auto aLetNumber = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.LET(_xlpm.first;5;_xlpm.second;_xlpm.first+5;_xlpm.second)",
            { 0, 0, 0 });
        const auto aLetSimple = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.LET(_xlpm.one;9;_xlpm.one)", { 0, 0, 0 });

        const auto aCompiledLetNumber = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.LET(_xlpm.first;5;_xlpm.second;_xlpm.first+5;_xlpm.second)",
            { 0, 0, 0 });
        const auto aCompiledLetSimple = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.LET(_xlpm.one;9;_xlpm.one)", { 0, 0, 0 });

        const auto checkLetNumber = [&](const char* pLabel, const auto& rResult,
                                        double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: LET mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkLetNumber("direct numeric LET", aLetNumber, 10.0)
            || !checkLetNumber("direct simple LET", aLetSimple, 9.0)
            || !checkLetNumber("compiled numeric LET", aCompiledLetNumber, 10.0)
            || !checkLetNumber("compiled simple LET", aCompiledLetSimple, 9.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "LET evaluation mismatch");
        }
    }

    {
        const auto aTextAfterBasic = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"Brown, Lucas, Manager:1234:5678\";\",\";2)",
            { 0, 0, 0 });
        const auto aTextAfterArray = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"Brown, Lucas, Manager:1234:5678\";{\",\";\":\"};4)",
            { 0, 0, 0 });
        const auto aTextAfterCaseInsensitive = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"AlphaBetaGamma\";\"beta\";;1)", { 0, 0, 0 });
        const auto aTextAfterMatchEnd = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"LucasÇÇÇManager\";\"xxx\";1;;1)", { 0, 0, 0 });
        const auto aTextAfterNotFound = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"Brown, Lucas\";\"xxx\";;;;\"Not Found\")",
            { 0, 0, 0 });

        const auto aCompiledTextAfterBasic = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"Brown, Lucas, Manager:1234:5678\";\",\";2)",
            { 0, 0, 0 });
        const auto aCompiledTextAfterArray = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"Brown, Lucas, Manager:1234:5678\";{\",\";\":\"};4)",
            { 0, 0, 0 });
        const auto aCompiledTextAfterCaseInsensitive = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"AlphaBetaGamma\";\"beta\";;1)", { 0, 0, 0 });
        const auto aCompiledTextAfterMatchEnd = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"LucasÇÇÇManager\";\"xxx\";1;;1)", { 0, 0, 0 });
        const auto aCompiledTextAfterNotFound = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTAFTER(\"Brown, Lucas\";\"xxx\";;;;\"Not Found\")",
            { 0, 0, 0 });

        const auto checkTextAfter = [&](const char* pLabel, const auto& rResult,
                                        std::u16string_view rExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isText() || rResult.mbUsedCachedValue
                || rResult.maValue.maValue.maString != rExpected)
            {
                std::fprintf(stderr, "%s: TEXTAFTER mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkTextAfter("direct basic TEXTAFTER", aTextAfterBasic, u" Manager:1234:5678")
            || !checkTextAfter("direct array TEXTAFTER", aTextAfterArray, u"5678")
            || !checkTextAfter("direct case-insensitive TEXTAFTER", aTextAfterCaseInsensitive,
                u"Gamma")
            || !checkTextAfter("direct match-end TEXTAFTER", aTextAfterMatchEnd,
                u"LucasÇÇÇManager")
            || !checkTextAfter("direct not-found TEXTAFTER", aTextAfterNotFound, u"Not Found")
            || !checkTextAfter("compiled basic TEXTAFTER", aCompiledTextAfterBasic,
                u" Manager:1234:5678")
            || !checkTextAfter("compiled array TEXTAFTER", aCompiledTextAfterArray, u"5678")
            || !checkTextAfter("compiled case-insensitive TEXTAFTER",
                aCompiledTextAfterCaseInsensitive, u"Gamma")
            || !checkTextAfter("compiled match-end TEXTAFTER", aCompiledTextAfterMatchEnd,
                u"LucasÇÇÇManager")
            || !checkTextAfter("compiled not-found TEXTAFTER", aCompiledTextAfterNotFound,
                u"Not Found"))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "TEXTAFTER evaluation mismatch");
        }
    }

    {
        const auto aTextBeforeBasic = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"Brown, Lucas, Manager:1234:5678\";\",\";2)",
            { 0, 0, 0 });
        const auto aTextBeforeArray = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"Brown, Lucas, Manager:1234:5678\";{\",\";\":\"};4)",
            { 0, 0, 0 });
        const auto aTextBeforeCaseInsensitive = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"AlphaBetaGamma\";\"beta\";;1)", { 0, 0, 0 });
        const auto aTextBeforeMatchEnd = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"LucasÇÇÇManager\";\"xxx\";1;;1)", { 0, 0, 0 });
        const auto aTextBeforeNotFound = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"Brown, Lucas\";\"xxx\";;;;\"Not Found\")",
            { 0, 0, 0 });

        const auto aCompiledTextBeforeBasic = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"Brown, Lucas, Manager:1234:5678\";\",\";2)",
            { 0, 0, 0 });
        const auto aCompiledTextBeforeArray = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"Brown, Lucas, Manager:1234:5678\";{\",\";\":\"};4)",
            { 0, 0, 0 });
        const auto aCompiledTextBeforeCaseInsensitive = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"AlphaBetaGamma\";\"beta\";;1)", { 0, 0, 0 });
        const auto aCompiledTextBeforeMatchEnd = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"LucasÇÇÇManager\";\"xxx\";1;;1)", { 0, 0, 0 });
        const auto aCompiledTextBeforeNotFound = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.TEXTBEFORE(\"Brown, Lucas\";\"xxx\";;;;\"Not Found\")",
            { 0, 0, 0 });

        const auto checkTextBefore = [&](const char* pLabel, const auto& rResult,
                                         std::u16string_view rExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isText() || rResult.mbUsedCachedValue
                || rResult.maValue.maValue.maString != rExpected)
            {
                std::fprintf(stderr, "%s: TEXTBEFORE mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkTextBefore("direct basic TEXTBEFORE", aTextBeforeBasic, u"Brown, Lucas")
            || !checkTextBefore("direct array TEXTBEFORE", aTextBeforeArray,
                u"Brown, Lucas, Manager:1234")
            || !checkTextBefore("direct case-insensitive TEXTBEFORE",
                aTextBeforeCaseInsensitive, u"Alpha")
            || !checkTextBefore("direct match-end TEXTBEFORE", aTextBeforeMatchEnd,
                u"LucasÇÇÇManager")
            || !checkTextBefore("direct not-found TEXTBEFORE", aTextBeforeNotFound,
                u"Not Found")
            || !checkTextBefore("compiled basic TEXTBEFORE", aCompiledTextBeforeBasic,
                u"Brown, Lucas")
            || !checkTextBefore("compiled array TEXTBEFORE", aCompiledTextBeforeArray,
                u"Brown, Lucas, Manager:1234")
            || !checkTextBefore("compiled case-insensitive TEXTBEFORE",
                aCompiledTextBeforeCaseInsensitive, u"Alpha")
            || !checkTextBefore("compiled match-end TEXTBEFORE", aCompiledTextBeforeMatchEnd,
                u"LucasÇÇÇManager")
            || !checkTextBefore("compiled not-found TEXTBEFORE",
                aCompiledTextBeforeNotFound, u"Not Found"))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "TEXTBEFORE evaluation mismatch");
        }
    }

    {
        const auto aIndirectA1
            = aEvaluator.evaluateFormula(u"of:=INDIRECT(\"A1\")", { 0, 0, 0 });
        const auto aIndirectAbs
            = aEvaluator.evaluateFormula(u"of:=INDIRECT(\"$A$1\")", { 0, 0, 0 });
        const auto aIndirectSheet
            = aEvaluator.evaluateFormula(u"of:=INDIRECT(\"Sheet2!A1\")", { 0, 0, 0 });
        const auto aIndirectR1C1
            = aEvaluator.evaluateFormula(u"of:=INDIRECT(\"R1C1\";0)", { 0, 0, 0 });

        const auto aCompiledIndirectA1
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=INDIRECT(\"A1\")", { 0, 0, 0 });
        const auto aCompiledIndirectAbs = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=INDIRECT(\"$A$1\")", { 0, 0, 0 });
        const auto aCompiledIndirectSheet = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=INDIRECT(\"Sheet2!A1\")", { 0, 0, 0 });
        const auto aCompiledIndirectR1C1 = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=INDIRECT(\"R1C1\";0)", { 0, 0, 0 });

        const auto checkIndirectNumber = [&](const char* pLabel, const auto& rResult,
                                             double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: INDIRECT mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkIndirectNumber("direct A1 INDIRECT", aIndirectA1, 5.0)
            || !checkIndirectNumber("direct absolute INDIRECT", aIndirectAbs, 5.0)
            || !checkIndirectNumber("direct sheet INDIRECT", aIndirectSheet, 3.0)
            || !checkIndirectNumber("direct R1C1 INDIRECT", aIndirectR1C1, 5.0)
            || !checkIndirectNumber("compiled A1 INDIRECT", aCompiledIndirectA1, 5.0)
            || !checkIndirectNumber("compiled absolute INDIRECT", aCompiledIndirectAbs, 5.0)
            || !checkIndirectNumber("compiled sheet INDIRECT", aCompiledIndirectSheet, 3.0)
            || !checkIndirectNumber("compiled R1C1 INDIRECT", aCompiledIndirectR1C1, 5.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "INDIRECT evaluation mismatch");
        }
    }

    {
        const auto aPoissonCdf
            = aEvaluator.evaluateFormula(u"of:=POISSON.DIST(60;50;TRUE())", { 0, 0, 0 });
        const auto aPoissonPmf
            = aEvaluator.evaluateFormula(u"of:=POISSON.DIST(10;10;FALSE())", { 0, 0, 0 });
        const auto aBinomCdf
            = aEvaluator.evaluateFormula(u"of:=BINOMDIST(5;10;0.5;TRUE())", { 0, 0, 0 });
        const auto aBinomRange
            = aEvaluator.evaluateFormula(u"of:=BINOM.DIST.RANGE(10;1/6;2)", { 0, 0, 0 });
        const auto aBetaCdf
            = aEvaluator.evaluateFormula(u"of:=BETADIST(0.5;2;3)", { 0, 0, 0 });
        const auto aBetaPdf
            = aEvaluator.evaluateFormula(u"of:=BETA.DIST(0.5;2;3;FALSE())", { 0, 0, 0 });

        const auto aCompiledPoissonCdf
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=POISSON.DIST(60;50;TRUE())", { 0, 0, 0 });
        const auto aCompiledPoissonPmf
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=POISSON.DIST(10;10;FALSE())", { 0, 0, 0 });
        const auto aCompiledBinomCdf
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=BINOMDIST(5;10;0.5;TRUE())", { 0, 0, 0 });
        const auto aCompiledBinomRange
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=BINOM.DIST.RANGE(10;1/6;2)", { 0, 0, 0 });
        const auto aCompiledBetaCdf
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=BETADIST(0.5;2;3)", { 0, 0, 0 });
        const auto aCompiledBetaPdf
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=BETA.DIST(0.5;2;3;FALSE())", { 0, 0, 0 });

        const auto checkDistribution = [&](const char* pLabel, const auto& rResult,
                                           double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: distribution mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkDistribution("POISSON.DIST cumulative", aPoissonCdf, 0.927839820186743)
            || !checkDistribution("POISSON.DIST mass", aPoissonPmf, 0.125110035721133)
            || !checkDistribution("BINOMDIST cumulative", aBinomCdf, 0.623046875)
            || !checkDistribution("BINOM.DIST.RANGE", aBinomRange, 0.290710049201722)
            || !checkDistribution("BETADIST cumulative", aBetaCdf, 0.6875)
            || !checkDistribution("BETA.DIST density", aBetaPdf, 1.5)
            || !checkDistribution("compiled POISSON.DIST cumulative", aCompiledPoissonCdf,
                0.927839820186743)
            || !checkDistribution("compiled POISSON.DIST mass", aCompiledPoissonPmf,
                0.125110035721133)
            || !checkDistribution("compiled BINOMDIST cumulative", aCompiledBinomCdf, 0.623046875)
            || !checkDistribution("compiled BINOM.DIST.RANGE", aCompiledBinomRange,
                0.290710049201722)
            || !checkDistribution("compiled BETADIST cumulative", aCompiledBetaCdf, 0.6875)
            || !checkDistribution("compiled BETA.DIST density", aCompiledBetaPdf, 1.5))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "distribution evaluation mismatch");
        }
    }

    {
        const auto aNormSDist
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.NORM.S.DIST(1;TRUE())", { 0, 0, 0 });
        const auto aNormSPdf
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.NORM.S.DIST(1;FALSE())", { 0, 0, 0 });
        const auto aNormSInv
            = aEvaluator.evaluateFormula(u"of:=NORMSINV(0.975)", { 0, 0, 0 });
        const auto aNormInv
            = aEvaluator.evaluateFormula(u"of:=NORMINV(0.9;0;1)", { 0, 0, 0 });
        const auto aLogInv = aEvaluator.evaluateFormula(u"of:=LOGINV(0.5)", { 0, 0, 0 });
        const auto aGammaInv
            = aEvaluator.evaluateFormula(u"of:=GAMMAINV(0.5;1;2)", { 0, 0, 0 });
        const auto aBetaInv
            = aEvaluator.evaluateFormula(u"of:=BETAINV(0.5;2;3)", { 0, 0, 0 });
        const auto aChiInv = aEvaluator.evaluateFormula(u"of:=CHIINV(0.05;2)", { 0, 0, 0 });
        const auto aChiSqInvRt
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.CHISQ.INV.RT(0.05;2)", { 0, 0, 0 });
        const auto aChiSqDistRt
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.CHISQ.DIST.RT(3;2)", { 0, 0, 0 });
        const auto aExpLegacy
            = aEvaluator.evaluateFormula(u"of:=EXPONDIST(1;2;TRUE())", { 0, 0, 0 });
        const auto aExpMs
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.EXPON.DIST(1;2;FALSE())", { 0, 0, 0 });
        const auto aNegBinomLegacy
            = aEvaluator.evaluateFormula(u"of:=NEGBINOMDIST(1;1;0.5)", { 0, 0, 0 });
        const auto aNegBinomMs = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.NEGBINOM.DIST(1;1;0.5;TRUE())", { 0, 0, 0 });
        const auto aWeibullLegacy
            = aEvaluator.evaluateFormula(u"of:=WEIBULL(1;2;3;TRUE())", { 0, 0, 0 });
        const auto aWeibullMs = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.WEIBULL.DIST(1;2;3;FALSE())", { 0, 0, 0 });
        const auto aConfidenceNorm = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CONFIDENCE.NORM(0.05;1.5;100)", { 0, 0, 0 });
        const auto aStandardize
            = aEvaluator.evaluateFormula(u"of:=STANDARDIZE(5;2;3)", { 0, 0, 0 });
        const auto aTInv
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.T.INV(0.95;10)", { 0, 0, 0 });
        const auto aTDistRt
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.T.DIST.RT(1;10)", { 0, 0, 0 });
        const auto aFDistRt = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.F.DIST.RT(0.8;8;12)", { 0, 0, 0 });
        const auto aErfPrecise
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.ERF.PRECISE(1)", { 0, 0, 0 });
        const auto aErfcPrecise
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.ERFC.PRECISE(1)", { 0, 0, 0 });
        const auto aGammaLnPreciseCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.GAMMALN.PRECISE(5)", { 0, 0, 0 });
        const auto aNormSDistCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.NORM.S.DIST(1;TRUE())", { 0, 0, 0 });
        const auto aNormSPdfCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.NORM.S.DIST(1;FALSE())", { 0, 0, 0 });
        const auto aNormSInvCompiled
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=NORMSINV(0.975)", { 0, 0, 0 });
        const auto aNormInvCompiled
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=NORMINV(0.9;0;1)", { 0, 0, 0 });
        const auto aLogInvCompiled
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=LOGINV(0.5)", { 0, 0, 0 });
        const auto aGammaInvCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=GAMMAINV(0.5;1;2)", { 0, 0, 0 });
        const auto aBetaInvCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BETAINV(0.5;2;3)", { 0, 0, 0 });
        const auto aChiInvCompiled
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=CHIINV(0.05;2)", { 0, 0, 0 });
        const auto aChiSqInvRtCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CHISQ.INV.RT(0.05;2)", { 0, 0, 0 });
        const auto aChiSqDistRtCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CHISQ.DIST.RT(3;2)", { 0, 0, 0 });
        const auto aExpLegacyCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=EXPONDIST(1;2;TRUE())", { 0, 0, 0 });
        const auto aExpMsCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.EXPON.DIST(1;2;FALSE())", { 0, 0, 0 });
        const auto aNegBinomLegacyCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=NEGBINOMDIST(1;1;0.5)", { 0, 0, 0 });
        const auto aNegBinomMsCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.NEGBINOM.DIST(1;1;0.5;TRUE())", { 0, 0, 0 });
        const auto aWeibullLegacyCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=WEIBULL(1;2;3;TRUE())", { 0, 0, 0 });
        const auto aWeibullMsCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.WEIBULL.DIST(1;2;3;FALSE())", { 0, 0, 0 });
        const auto aConfidenceNormCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CONFIDENCE.NORM(0.05;1.5;100)", { 0, 0, 0 });
        const auto aStandardizeCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=STANDARDIZE(5;2;3)", { 0, 0, 0 });
        const auto aTInvCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.T.INV(0.95;10)", { 0, 0, 0 });
        const auto aTDistRtCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.T.DIST.RT(1;10)", { 0, 0, 0 });
        const auto aFDistRtCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.F.DIST.RT(0.8;8;12)", { 0, 0, 0 });
        const auto aErfPreciseCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.ERF.PRECISE(1)", { 0, 0, 0 });
        const auto aErfcPreciseCompiled = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.ERFC.PRECISE(1)", { 0, 0, 0 });

        const auto checkStatScalar = [&](const char* pLabel, const auto& rResult,
                                         double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: statistical dispatch mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkStatScalar("NORM.S.DIST cumulative", aNormSDist, 0.841344746068543)
            || !checkStatScalar("NORM.S.DIST density", aNormSPdf, 0.241970724519143)
            || !checkStatScalar("NORMSINV", aNormSInv, 1.95996398454005)
            || !checkStatScalar("NORMINV", aNormInv, 1.2815515655446)
            || !checkStatScalar("LOGINV", aLogInv, 1.0)
            || !checkStatScalar("GAMMAINV", aGammaInv, 1.38629436111989)
            || !checkStatScalar("BETAINV", aBetaInv, 0.38572756813239)
            || !checkStatScalar("CHIINV", aChiInv, 5.99146454710798)
            || !checkStatScalar("CHISQ.INV.RT", aChiSqInvRt, 5.99146454710798)
            || !checkStatScalar("CHISQ.DIST.RT", aChiSqDistRt, 0.22313016014843)
            || !checkStatScalar("EXPONDIST cumulative", aExpLegacy, 0.864664716763387)
            || !checkStatScalar("EXPON.DIST density", aExpMs, 0.270670566473225)
            || !checkStatScalar("NEGBINOMDIST", aNegBinomLegacy, 0.25)
            || !checkStatScalar("NEGBINOM.DIST", aNegBinomMs, 0.75)
            || !checkStatScalar("WEIBULL cumulative", aWeibullLegacy, 0.105160683185631)
            || !checkStatScalar("WEIBULL.DIST density", aWeibullMs, 0.198853181514304)
            || !checkStatScalar("CONFIDENCE.NORM", aConfidenceNorm, 0.293994597681008)
            || !checkStatScalar("STANDARDIZE", aStandardize, 1.0)
            || !checkStatScalar("T.INV", aTInv, 1.81246112281073)
            || !checkStatScalar("T.DIST.RT", aTDistRt, 0.17044656615103)
            || !checkStatScalar("F.DIST.RT", aFDistRt, 0.614339643745812)
            || !checkStatScalar("ERF.PRECISE", aErfPrecise, 0.842700792949715)
            || !checkStatScalar("ERFC.PRECISE", aErfcPrecise, 0.157299207050285)
            || !checkStatScalar("compiled GAMMALN.PRECISE", aGammaLnPreciseCompiled,
                3.17805383034795)
            || !checkStatScalar("compiled NORM.S.DIST cumulative", aNormSDistCompiled,
                0.841344746068543)
            || !checkStatScalar("compiled NORM.S.DIST density", aNormSPdfCompiled,
                0.241970724519143)
            || !checkStatScalar("compiled NORMSINV", aNormSInvCompiled, 1.95996398454005)
            || !checkStatScalar("compiled NORMINV", aNormInvCompiled, 1.2815515655446)
            || !checkStatScalar("compiled LOGINV", aLogInvCompiled, 1.0)
            || !checkStatScalar("compiled GAMMAINV", aGammaInvCompiled, 1.38629436111989)
            || !checkStatScalar("compiled BETAINV", aBetaInvCompiled, 0.38572756813239)
            || !checkStatScalar("compiled CHIINV", aChiInvCompiled, 5.99146454710798)
            || !checkStatScalar("compiled CHISQ.INV.RT", aChiSqInvRtCompiled,
                5.99146454710798)
            || !checkStatScalar("compiled CHISQ.DIST.RT", aChiSqDistRtCompiled,
                0.22313016014843)
            || !checkStatScalar("compiled EXPONDIST cumulative", aExpLegacyCompiled,
                0.864664716763387)
            || !checkStatScalar("compiled EXPON.DIST density", aExpMsCompiled,
                0.270670566473225)
            || !checkStatScalar("compiled NEGBINOMDIST", aNegBinomLegacyCompiled, 0.25)
            || !checkStatScalar("compiled NEGBINOM.DIST", aNegBinomMsCompiled, 0.75)
            || !checkStatScalar("compiled WEIBULL cumulative", aWeibullLegacyCompiled,
                0.105160683185631)
            || !checkStatScalar("compiled WEIBULL.DIST density", aWeibullMsCompiled,
                0.198853181514304)
            || !checkStatScalar("compiled CONFIDENCE.NORM", aConfidenceNormCompiled,
                0.293994597681008)
            || !checkStatScalar("compiled STANDARDIZE", aStandardizeCompiled, 1.0)
            || !checkStatScalar("compiled T.INV", aTInvCompiled, 1.81246112281073)
            || !checkStatScalar("compiled T.DIST.RT", aTDistRtCompiled, 0.17044656615103)
            || !checkStatScalar("compiled F.DIST.RT", aFDistRtCompiled,
                0.614339643745812)
            || !checkStatScalar("compiled ERF.PRECISE", aErfPreciseCompiled,
                0.842700792949715)
            || !checkStatScalar("compiled ERFC.PRECISE", aErfcPreciseCompiled,
                0.157299207050285))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "statistical runtime dispatch mismatch");
        }
    }

    {
        const auto aAbs = aEvaluator.evaluateFormula(u"of:=ABS(-7.25)", { 0, 0, 0 });
        const auto aDegrees
            = aEvaluator.evaluateFormula(u"of:=DEGREES(3.141592653589793)", { 0, 0, 0 });
        const auto aPi = aEvaluator.evaluateFormula(u"of:=PI()", { 0, 0, 0 });
        const auto aAtanh = aEvaluator.evaluateFormula(u"of:=ATANH(0.5)", { 0, 0, 0 });
        const auto aFisher = aEvaluator.evaluateFormula(u"of:=FISHER(0.5)", { 0, 0, 0 });
        const auto aFisherInv
            = aEvaluator.evaluateFormula(u"of:=FISHERINV(0.5493061443340549)", { 0, 0, 0 });
        const auto aGauss = aEvaluator.evaluateFormula(u"of:=GAUSS(1)", { 0, 0, 0 });
        const auto aGammaLn = aEvaluator.evaluateFormula(u"of:=GAMMALN(5)", { 0, 0, 0 });
        const auto aGammaLnPrecise
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.GAMMALN.PRECISE(5)", { 0, 0, 0 });
        const auto aGcd = aEvaluator.evaluateFormula(u"of:=GCD({16|32|24};40)", { 0, 0, 0 });
        const auto aLcm = aEvaluator.evaluateFormula(u"of:=LCM({4|6|10})", { 0, 0, 0 });
        const auto aGeoMean
            = aEvaluator.evaluateFormula(u"of:=GEOMEAN({4|1|0.03125})", { 0, 0, 0 });
        const auto aGeoMeanZero
            = aEvaluator.evaluateFormula(u"of:=GEOMEAN({4|0|8})", { 0, 0, 0 });
        const auto aHarMean = aEvaluator.evaluateFormula(u"of:=HARMEAN({1|2|4})", { 0, 0, 0 });
        const auto aHarMeanError
            = aEvaluator.evaluateFormula(u"of:=HARMEAN({1|0|4})", { 0, 0, 0 });
        const auto aDateDif = aEvaluator.evaluateFormula(
            u"of:=DATEDIF(DATE(2020;1;1);DATE(2021;3;15);\"ym\")", { 0, 0, 0 });
        const auto aFloor = aEvaluator.evaluateFormula(u"of:=FLOOR(-11;-2)", { 0, 0, 0 });
        const auto aFloorMode = aEvaluator.evaluateFormula(u"of:=FLOOR(-7.9;;5)", { 0, 0, 0 });
        const auto aFloorMissingValue
            = aEvaluator.evaluateFormula(u"of:=FLOOR(;2.3;5)", { 0, 0, 0 });
        const auto aCeiling = aEvaluator.evaluateFormula(u"of:=CEILING(-11;-2)", { 0, 0, 0 });
        const auto aFloorMath
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.FLOOR.MATH(-11;-2;1)", { 0, 0, 0 });
        const auto aCeilingMath = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CEILING.MATH(-5.5;2;-1)", { 0, 0, 0 });
        const auto aCeilingPrecise = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.CEILING.PRECISE(-2.5;2)", { 0, 0, 0 });
        const auto aFloorPrecise = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.FLOOR.PRECISE(-2.5;2)", { 0, 0, 0 });
        const auto aFloorPreciseError = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.FLOOR.PRECISE(4.3;2;1)", { 0, 0, 0 });
        const auto aIsoCeiling
            = aEvaluator.evaluateFormula(u"of:=COM.MICROSOFT.ISO.CEILING(-4.3;2)", { 0, 0, 0 });
        const auto aChar = aEvaluator.evaluateFormula(u"of:=CHAR(65)", { 0, 0, 0 });
        const auto aCode = aEvaluator.evaluateFormula(u"of:=CODE(\"Az\")", { 0, 0, 0 });
        const auto aDecimal = aEvaluator.evaluateFormula(u"of:=DECIMAL(\"FF\";16)", { 0, 0, 0 });
        const auto aDec2Hex = aEvaluator.evaluateFormula(u"of:=DEC2HEX(255)", { 0, 0, 0 });
        const auto aLog = aEvaluator.evaluateFormula(u"of:=LOG(8;2)", { 0, 0, 0 });
        const auto aMround = aEvaluator.evaluateFormula(u"of:=MROUND(10;4)", { 0, 0, 0 });
        const auto aMroundTie = aEvaluator.evaluateFormula(u"of:=MROUND(1.45;0.1)", { 0, 0, 0 });
        const auto aMroundMissing = aEvaluator.evaluateFormula(u"of:=MROUND(15.5;)", { 0, 0, 0 });
        const auto aCombin = aEvaluator.evaluateFormula(u"of:=COMBIN(6;2)", { 0, 0, 0 });
        const auto aCombina = aEvaluator.evaluateFormula(u"of:=COMBINA(4;3)", { 0, 0, 0 });
        const auto aCombinaZero = aEvaluator.evaluateFormula(u"of:=COMBINA(0;0)", { 0, 0, 0 });
        const auto aCombinaKZero = aEvaluator.evaluateFormula(u"of:=COMBINA(12;0)", { 0, 0, 0 });
        const auto aCombinaError = aEvaluator.evaluateFormula(u"of:=COMBINA(1;2)", { 0, 0, 0 });
        const auto aMultinomial
            = aEvaluator.evaluateFormula(u"of:=MULTINOMIAL(1;2;3)", { 0, 0, 0 });
        const auto aMultinomialLarge = aEvaluator.evaluateFormula(
            u"of:=MULTINOMIAL(1073741824;2;1)", { 0, 0, 0 });
        const auto aBitXor = aEvaluator.evaluateFormula(u"of:=BITXOR(5;3)", { 0, 0, 0 });
        const auto aBitXorLeftMissing = aEvaluator.evaluateFormula(u"of:=BITXOR(;25)", { 0, 0, 0 });
        const auto aBitXorRightMissing = aEvaluator.evaluateFormula(u"of:=BITXOR(25;)", { 0, 0, 0 });
        const auto aBitXorNoValue = aEvaluator.evaluateFormula(u"of:=BITXOR(25)", { 0, 0, 0 });
        const auto aBitXorError = aEvaluator.evaluateFormula(u"of:=BITXOR(-1;2)", { 0, 0, 0 });
        const auto aCsc = aEvaluator.evaluateFormula(u"of:=CSC(PI()/2)", { 0, 0, 0 });
        const auto aCsch = aEvaluator.evaluateFormula(u"of:=CSCH(1)", { 0, 0, 0 });
        const auto aCschZero = aEvaluator.evaluateFormula(u"of:=CSCH(0)", { 0, 0, 0 });
        const auto aTrunc = aEvaluator.evaluateFormula(u"of:=TRUNC(-123.456;2)", { 0, 0, 0 });
        const auto aUpper = aEvaluator.evaluateFormula(u"of:=UPPER(\"MiXeD\")", { 0, 0, 0 });
        const auto aLower = aEvaluator.evaluateFormula(u"of:=LOWER(\"MiXeD\")", { 0, 0, 0 });
        const auto aLen = aEvaluator.evaluateFormula(u"of:=LEN(\"A😀\")", { 0, 0, 0 });
        const auto aLenb = aEvaluator.evaluateFormula(u"of:=LENB(\"ᄩA\")", { 0, 0, 0 });
        const auto aSearch
            = aEvaluator.evaluateFormula(u"of:=SEARCH(\"bc\";\"AbCd\")", { 0, 0, 0 });
        const auto aFind
            = aEvaluator.evaluateFormula(u"of:=FIND(\"bc\";\"AbCd\")", { 0, 0, 0 });
        const auto aMid = aEvaluator.evaluateFormula(u"of:=MID(\"A😀BC\";2;2)", { 0, 0, 0 });
        const auto aReplace
            = aEvaluator.evaluateFormula(u"of:=REPLACE(\"abcdef\";2;3;\"ZZ\")", { 0, 0, 0 });
        const auto aBase = aEvaluator.evaluateFormula(u"of:=BASE(255;16;4)", { 0, 0, 0 });
        const auto aRoman = aEvaluator.evaluateFormula(u"of:=ROMAN(499;4)", { 0, 0, 0 });
        const auto aLeft = aEvaluator.evaluateFormula(u"of:=LEFT(\"A😀BC\";2)", { 0, 0, 0 });
        const auto aRight = aEvaluator.evaluateFormula(u"of:=RIGHT(\"A😀BC\";2)", { 0, 0, 0 });
        const auto aProper
            = aEvaluator.evaluateFormula(u"of:=PROPER(\"HELLO.THERE\")", { 0, 0, 0 });
        const auto aProperDigits
            = aEvaluator.evaluateFormula(u"of:=PROPER(\"76budget\")", { 0, 0, 0 });
        const auto aSubstitute = aEvaluator.evaluateFormula(
            u"of:=SUBSTITUTE(\"123123123\";\"3\";\"abc\";2)", { 0, 0, 0 });
        const auto aT = aEvaluator.evaluateFormula(u"of:=T(7)", { 0, 0, 0 });
        const auto aTError = aEvaluator.evaluateFormula(u"of:=T(NA())", { 0, 0, 0 });
        const auto aConcat
            = aEvaluator.evaluateFormula(u"of:=CONCAT(\"A\";[.BC1:.BF1])", { 0, 0, 0 });
        const auto aFindb
            = aEvaluator.evaluateFormula(u"of:=FINDB(\"ᄔ\";\"ᄩᄔᄕ\")", { 0, 0, 0 });
        const auto aFindbNbsp = aEvaluator.evaluateFormula(
            u"of:=FINDB(\"M\";\"Miriam\u00A0McGovern\";3)", { 0, 0, 0 });
        const auto aSearchb
            = aEvaluator.evaluateFormula(u"of:=SEARCHB(\"ab\";\"zAbz\")", { 0, 0, 0 });
        const auto aReplaceb = aEvaluator.evaluateFormula(
            u"of:=REPLACEB(\"ᄩᄔᄕ\";1;1;\"ab\")", { 0, 0, 0 });
        const auto aAsc = aEvaluator.evaluateFormula(u"of:=ASC(\"ＡＢＣ１２３\")", { 0, 0, 0 });
        const auto aAscKana
            = aEvaluator.evaluateFormula(u"of:=ASC(\"オープンオフィス\")", { 0, 0, 0 });
        const auto aJisPunctuation
            = aEvaluator.evaluateFormula(u"of:=JIS(\"!\"&CHAR(34)&\"#$%&'()*+,-./\")", { 0, 0, 0 });
        const auto aJisQuotes = aEvaluator.evaluateFormula(
            u"of:=JIS(CHAR(34)&\"'\"&CHAR(92)&CHAR(96))", { 0, 0, 0 });
        const auto aJisKanaMarks = aEvaluator.evaluateFormula(u"of:=JIS(\"ﾞﾟ\")", { 0, 0, 0 });
        const auto aJisVoicedKana
            = aEvaluator.evaluateFormula(u"of:=JIS(\"ｶﾞｷﾞｸﾞｹﾞｺﾞ\")", { 0, 0, 0 });
        const auto aJisVoicedVowels
            = aEvaluator.evaluateFormula(u"of:=JIS(\"ｱﾞｲﾞｳﾞｴﾞｵﾞ\")", { 0, 0, 0 });
        const auto aLegacyChiDist
            = aEvaluator.evaluateFormula(u"of:=LEGACY.CHIDIST(2;3)", { 0, 0, 0 });
        const auto aAddress = aEvaluator.evaluateFormula(u"of:=ADDRESS(4;5)", { 0, 0, 0 });
        const auto aAddressRowMixed
            = aEvaluator.evaluateFormula(u"of:=ADDRESS(4;5;2)", { 0, 0, 0 });
        const auto aAddressColumnMixed
            = aEvaluator.evaluateFormula(u"of:=ADDRESS(4;5;3)", { 0, 0, 0 });
        const auto aAddressR1C1
            = aEvaluator.evaluateFormula(u"of:=ADDRESS(4;5;4;0)", { 0, 0, 0 });
        const auto aAddressSheet = aEvaluator.evaluateFormula(
            u"of:=ADDRESS(1;1;2;;\"Sheet2\")", { 0, 0, 0 });
        const auto aAddressQuotedSheet = aEvaluator.evaluateFormula(
            u"of:=ADDRESS(1;1;4;1;\"Sheet 3\")", { 0, 0, 0 });
        const auto aAddressQuotedSheetR1C1 = aEvaluator.evaluateFormula(
            u"of:=ADDRESS(1;1;1;0;\"Sheet 3\")", { 0, 0, 0 });
        const auto aAddressError
            = aEvaluator.evaluateFormula(u"of:=ADDRESS(1;1;0;1)", { 0, 0, 0 });
        const auto aConvert
            = aEvaluator.evaluateFormula(u"of:=CONVERT(1;\"m\";\"mi\")", { 0, 0, 0 });
        const auto aConvertTemp
            = aEvaluator.evaluateFormula(u"of:=CONVERT(2;\"C\";\"F\")", { 0, 0, 0 });
        const auto aConvertCubic
            = aEvaluator.evaluateFormula(u"of:=CONVERT(1;\"picapt3\";\"pica3\")", { 0, 0, 0 });
        const auto aConvertCaretAlias = aEvaluator.evaluateFormula(
            u"of:=CONVERT(1;\"picapt^3\";\"pica^3\")", { 0, 0, 0 });
        const auto aConvertCubicStep
            = aEvaluator.evaluateFormula(u"of:=CONVERT(1;\"pica3\";\"pt\")", { 0, 0, 0 });
        const auto aConvertPica
            = aEvaluator.evaluateFormula(u"of:=CONVERT(1;\"Pica\";\"pica\")", { 0, 0, 0 });
        const auto aConvertPicaStep = aEvaluator.evaluateFormula(
            u"of:=CONVERT(1;\"pica\";\"survey_mi\")", { 0, 0, 0 });
        const auto aConvertIn3ToGal = aEvaluator.evaluateFormula(
            u"of:=CONVERT(9072;\"in3\";\"gal\")", { 0, 0, 0 });
        const auto aConvertM3ToYd3 = aEvaluator.evaluateFormula(
            u"of:=CONVERT(10;\"m3\";\"yd3\")", { 0, 0, 0 });
        const auto aConvertMtonToNmi3 = aEvaluator.evaluateFormula(
            u"of:=CONVERT(100000000000000;\"MTON\";\"Nmi3\")", { 0, 0, 0 });
        const auto aConvertTspmToMl = aEvaluator.evaluateFormula(
            u"of:=CONVERT(1;\"tspm\";\"ml\")", { 0, 0, 0 });
        const auto aBitLShift
            = aEvaluator.evaluateFormula(u"of:=BITLSHIFT(6;1)", { 0, 0, 0 });
        const auto aBitLShiftNegative = aEvaluator.evaluateFormula(
            u"of:=BITLSHIFT(10;-2)", { 0, 0, 0 });
        const auto aBitLShiftDefault
            = aEvaluator.evaluateFormula(u"of:=BITLSHIFT(4;)", { 0, 0, 0 });
        const auto aBitLShiftError = aEvaluator.evaluateFormula(
            u"of:=BITLSHIFT(-4;2)", { 0, 0, 0 });
        const auto aBitRShift
            = aEvaluator.evaluateFormula(u"of:=BITRSHIFT(6;1)", { 0, 0, 0 });
        const auto aBitRShiftNegative = aEvaluator.evaluateFormula(
            u"of:=BITRSHIFT(10;-2)", { 0, 0, 0 });
        const auto aBitRShiftDefault
            = aEvaluator.evaluateFormula(u"of:=BITRSHIFT(4;)", { 0, 0, 0 });
        const auto aBitRShiftError = aEvaluator.evaluateFormula(
            u"of:=BITRSHIFT(-4;2)", { 0, 0, 0 });
        const auto aConvertAlias = aEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.CONVERT(1;\"m\";\"mi\")", { 0, 0, 0 });
        const auto aConvertEuro = aEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.CONVERT(100;\"ATS\";\"EUR\")", { 0, 0, 0 });
        const auto aConvertEuroCaseError = aEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.CONVERT(100;\"skk\";\"skK\")", { 0, 0, 0 });
        const auto aConvertAliasOptionalError = aEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.CONVERT(100;\"EUR\";\"SIT\";FALSE())", { 0, 0, 0 });
        const auto aEuroConvert = aEvaluator.evaluateFormula(
            u"of:=EUROCONVERT(100;\"ATS\";\"EUR\")", { 0, 0, 0 });
        const auto aEuroConvertCase = aEvaluator.evaluateFormula(
            u"of:=EUROCONVERT(100;\"EUR\";\"skK\")", { 0, 0, 0 });
        const auto aEuroConvertPrecision = aEvaluator.evaluateFormula(
            u"of:=EUROCONVERT(100;\"EUR\";\"SIT\";;3)", { 0, 0, 0 });
        const auto aEuroConvertFullPrecision = aEvaluator.evaluateFormula(
            u"of:=EUROCONVERT(100;\"ATS\";\"EUR\";TRUE())", { 0, 0, 0 });
        const auto aEuroConvertPrecisionError = aEvaluator.evaluateFormula(
            u"of:=EUROCONVERT(100;\"EUR\";\"SIT\";0;2)", { 0, 0, 0 });
        const auto aHyperlink = aEvaluator.evaluateFormula(
            u"of:=HYPERLINK(\"https://example.com\";\"Example\")", { 0, 0, 0 });
        const auto aHyperlinkError = aEvaluator.evaluateFormula(
            u"of:=HYPERLINK(NA();\"Example\")", { 0, 0, 0 });
        const auto aTextJoin = aEvaluator.evaluateFormula(
            u"of:=TEXTJOIN(\"-\";1;\"\";\"A\";\"\";\"B\")", { 0, 0, 0 });
        const auto aOffset
            = aEvaluator.evaluateFormula(u"of:=OFFSET([.BA1];1;0;1;2)", { 0, 0, 0 });
        const auto aConcatenate
            = aEvaluator.evaluateFormula(u"of:=CONCATENATE(\"A\";1;\"B\")", { 0, 0, 0 });
        const auto aLarge
            = aEvaluator.evaluateFormula(u"of:=LARGE({1;3;2};2)", { 0, 0, 0 });
        const auto aSmall
            = aEvaluator.evaluateFormula(u"of:=SMALL({1;3;2};2)", { 0, 0, 0 });
        const auto aPercentile
            = aEvaluator.evaluateFormula(u"of:=PERCENTILE({1;2;3;4};0.25)", { 0, 0, 0 });
        const auto aPercentileExc = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.PERCENTILE.EXC({1;2;3;4};0.25)", { 0, 0, 0 });
        const auto aQuartile
            = aEvaluator.evaluateFormula(u"of:=QUARTILE({7;8;9;10};3)", { 0, 0, 0 });
        const auto aQuartileExc = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.QUARTILE.EXC({7;8;9;10};3)", { 0, 0, 0 });
        const auto aSkew
            = aEvaluator.evaluateFormula(u"of:=SKEW({1;2;2;3;9})", { 0, 0, 0 });
        const auto aSkewp
            = aEvaluator.evaluateFormula(u"of:=SKEWP({1;2;2;3;9})", { 0, 0, 0 });

        const auto aCompiledAbs
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=ABS(-7.25)", { 0, 0, 0 });
        const auto aCompiledDegrees = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=DEGREES(3.141592653589793)", { 0, 0, 0 });
        const auto aCompiledPi
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=PI()", { 0, 0, 0 });
        const auto aCompiledAtanh
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=ATANH(0.5)", { 0, 0, 0 });
        const auto aCompiledFisher = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=FISHER(0.5)", { 0, 0, 0 });
        const auto aCompiledFisherInv = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=FISHERINV(0.5493061443340549)", { 0, 0, 0 });
        const auto aCompiledGauss
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=GAUSS(1)", { 0, 0, 0 });
        const auto aCompiledGammaLn = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=GAMMALN(5)", { 0, 0, 0 });
        const auto aCompiledGammaLnPrecise = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.GAMMALN.PRECISE(5)", { 0, 0, 0 });
        const auto aCompiledGcd = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=GCD({16|32|24};40)", { 0, 0, 0 });
        const auto aCompiledLcm = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LCM({4|6|10})", { 0, 0, 0 });
        const auto aCompiledGeoMean = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=GEOMEAN({4|1|0.03125})", { 0, 0, 0 });
        const auto aCompiledGeoMeanZero = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=GEOMEAN({4|0|8})", { 0, 0, 0 });
        const auto aCompiledHarMean = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=HARMEAN({1|2|4})", { 0, 0, 0 });
        const auto aCompiledHarMeanError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=HARMEAN({1|0|4})", { 0, 0, 0 });
        const auto aCompiledDateDif = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=DATEDIF(DATE(2020;1;1);DATE(2021;3;15);\"ym\")", { 0, 0, 0 });
        const auto aCompiledFloor
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=FLOOR(-11;-2)", { 0, 0, 0 });
        const auto aCompiledFloorMode
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=FLOOR(-7.9;;5)", { 0, 0, 0 });
        const auto aCompiledFloorMissingValue
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=FLOOR(;2.3;5)", { 0, 0, 0 });
        const auto aCompiledCeiling
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=CEILING(-11;-2)", { 0, 0, 0 });
        const auto aCompiledFloorMath = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.FLOOR.MATH(-11;-2;1)", { 0, 0, 0 });
        const auto aCompiledCeilingMath = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CEILING.MATH(-5.5;2;-1)", { 0, 0, 0 });
        const auto aCompiledCeilingPrecise = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.CEILING.PRECISE(-2.5;2)", { 0, 0, 0 });
        const auto aCompiledFloorPrecise = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.FLOOR.PRECISE(-2.5;2)", { 0, 0, 0 });
        const auto aCompiledFloorPreciseError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.FLOOR.PRECISE(4.3;2;1)", { 0, 0, 0 });
        const auto aCompiledIsoCeiling = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.ISO.CEILING(-4.3;2)", { 0, 0, 0 });
        const auto aCompiledChar
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=CHAR(65)", { 0, 0, 0 });
        const auto aCompiledCode
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=CODE(\"Az\")", { 0, 0, 0 });
        const auto aCompiledDecimal = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=DECIMAL(\"FF\";16)", { 0, 0, 0 });
        const auto aCompiledDec2Hex = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=DEC2HEX(255)", { 0, 0, 0 });
        const auto aCompiledLog
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=LOG(8;2)", { 0, 0, 0 });
        const auto aCompiledMround
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=MROUND(10;4)", { 0, 0, 0 });
        const auto aCompiledMroundTie = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MROUND(1.45;0.1)", { 0, 0, 0 });
        const auto aCompiledMroundMissing = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MROUND(15.5;)", { 0, 0, 0 });
        const auto aCompiledCombin
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=COMBIN(6;2)", { 0, 0, 0 });
        const auto aCompiledCombina
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=COMBINA(4;3)", { 0, 0, 0 });
        const auto aCompiledCombinaZero = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COMBINA(0;0)", { 0, 0, 0 });
        const auto aCompiledCombinaKZero = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COMBINA(12;0)", { 0, 0, 0 });
        const auto aCompiledCombinaError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COMBINA(1;2)", { 0, 0, 0 });
        const auto aCompiledMultinomial = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MULTINOMIAL(1;2;3)", { 0, 0, 0 });
        const auto aCompiledMultinomialLarge = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MULTINOMIAL(1073741824;2;1)", { 0, 0, 0 });
        const auto aCompiledBitXor
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=BITXOR(5;3)", { 0, 0, 0 });
        const auto aCompiledBitXorLeftMissing = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITXOR(;25)", { 0, 0, 0 });
        const auto aCompiledBitXorRightMissing = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITXOR(25;)", { 0, 0, 0 });
        const auto aCompiledBitXorNoValue = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITXOR(25)", { 0, 0, 0 });
        const auto aCompiledBitXorError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITXOR(-1;2)", { 0, 0, 0 });
        const auto aCompiledCsc
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=CSC(PI()/2)", { 0, 0, 0 });
        const auto aCompiledCsch
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=CSCH(1)", { 0, 0, 0 });
        const auto aCompiledCschZero
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=CSCH(0)", { 0, 0, 0 });
        const auto aCompiledTrunc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=TRUNC(-123.456;2)", { 0, 0, 0 });
        const auto aCompiledUpper
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=UPPER(\"MiXeD\")", { 0, 0, 0 });
        const auto aCompiledLower
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=LOWER(\"MiXeD\")", { 0, 0, 0 });
        const auto aCompiledLen
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=LEN(\"A😀\")", { 0, 0, 0 });
        const auto aCompiledLenb
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=LENB(\"ᄩA\")", { 0, 0, 0 });
        const auto aCompiledSearch = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SEARCH(\"bc\";\"AbCd\")", { 0, 0, 0 });
        const auto aCompiledFind = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=FIND(\"bc\";\"AbCd\")", { 0, 0, 0 });
        const auto aCompiledMid = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MID(\"A😀BC\";2;2)", { 0, 0, 0 });
        const auto aCompiledReplace = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=REPLACE(\"abcdef\";2;3;\"ZZ\")", { 0, 0, 0 });
        const auto aCompiledBase = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BASE(255;16;4)", { 0, 0, 0 });
        const auto aCompiledRoman = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ROMAN(499;4)", { 0, 0, 0 });
        const auto aCompiledLeft = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LEFT(\"A😀BC\";2)", { 0, 0, 0 });
        const auto aCompiledRight = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=RIGHT(\"A😀BC\";2)", { 0, 0, 0 });
        const auto aCompiledProper = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=PROPER(\"HELLO.THERE\")", { 0, 0, 0 });
        const auto aCompiledProperDigits = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=PROPER(\"76budget\")", { 0, 0, 0 });
        const auto aCompiledSubstitute = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SUBSTITUTE(\"123123123\";\"3\";\"abc\";2)", { 0, 0, 0 });
        const auto aCompiledT = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=T(7)", { 0, 0, 0 });
        const auto aCompiledTError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=T(NA())", { 0, 0, 0 });
        const auto aCompiledConcat = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONCAT(\"A\";[.BC1:.BF1])", { 0, 0, 0 });
        const auto aCompiledFindb = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=FINDB(\"ᄔ\";\"ᄩᄔᄕ\")", { 0, 0, 0 });
        const auto aCompiledFindbNbsp = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=FINDB(\"M\";\"Miriam\u00A0McGovern\";3)", { 0, 0, 0 });
        const auto aCompiledSearchb = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SEARCHB(\"ab\";\"zAbz\")", { 0, 0, 0 });
        const auto aCompiledReplaceb = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=REPLACEB(\"ᄩᄔᄕ\";1;1;\"ab\")", { 0, 0, 0 });
        const auto aCompiledAsc
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=ASC(\"ＡＢＣ１２３\")", { 0, 0, 0 });
        const auto aCompiledAscKana = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ASC(\"オープンオフィス\")", { 0, 0, 0 });
        const auto aCompiledJisPunctuation = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=JIS(\"!\"&CHAR(34)&\"#$%&'()*+,-./\")", { 0, 0, 0 });
        const auto aCompiledJisQuotes = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=JIS(CHAR(34)&\"'\"&CHAR(92)&CHAR(96))", { 0, 0, 0 });
        const auto aCompiledJisKanaMarks
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=JIS(\"ﾞﾟ\")", { 0, 0, 0 });
        const auto aCompiledJisVoicedKana = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=JIS(\"ｶﾞｷﾞｸﾞｹﾞｺﾞ\")", { 0, 0, 0 });
        const auto aCompiledJisVoicedVowels = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=JIS(\"ｱﾞｲﾞｳﾞｴﾞｵﾞ\")", { 0, 0, 0 });
        const auto aCompiledLegacyChiDist = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LEGACY.CHIDIST(2;3)", { 0, 0, 0 });
        const auto aCompiledAddress
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=ADDRESS(4;5)", { 0, 0, 0 });
        const auto aCompiledAddressRowMixed = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ADDRESS(4;5;2)", { 0, 0, 0 });
        const auto aCompiledAddressColumnMixed = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ADDRESS(4;5;3)", { 0, 0, 0 });
        const auto aCompiledAddressR1C1 = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ADDRESS(4;5;4;0)", { 0, 0, 0 });
        const auto aCompiledAddressSheet = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ADDRESS(1;1;2;;\"Sheet2\")", { 0, 0, 0 });
        const auto aCompiledAddressQuotedSheet = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ADDRESS(1;1;4;1;\"Sheet 3\")", { 0, 0, 0 });
        const auto aCompiledAddressQuotedSheetR1C1
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=ADDRESS(1;1;1;0;\"Sheet 3\")", { 0, 0, 0 });
        const auto aCompiledAddressError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ADDRESS(1;1;0;1)", { 0, 0, 0 });
        const auto aCompiledConvert = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(1;\"m\";\"mi\")", { 0, 0, 0 });
        const auto aCompiledConvertTemp = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(2;\"C\";\"F\")", { 0, 0, 0 });
        const auto aCompiledConvertCubic = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(1;\"picapt3\";\"pica3\")", { 0, 0, 0 });
        const auto aCompiledConvertCaretAlias = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(1;\"picapt^3\";\"pica^3\")", { 0, 0, 0 });
        const auto aCompiledConvertCubicStep = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(1;\"pica3\";\"pt\")", { 0, 0, 0 });
        const auto aCompiledConvertPica = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(1;\"Pica\";\"pica\")", { 0, 0, 0 });
        const auto aCompiledConvertPicaStep = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(1;\"pica\";\"survey_mi\")", { 0, 0, 0 });
        const auto aCompiledConvertIn3ToGal = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(9072;\"in3\";\"gal\")", { 0, 0, 0 });
        const auto aCompiledConvertM3ToYd3 = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(10;\"m3\";\"yd3\")", { 0, 0, 0 });
        const auto aCompiledConvertMtonToNmi3 = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(100000000000000;\"MTON\";\"Nmi3\")", { 0, 0, 0 });
        const auto aCompiledConvertTspmToMl = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONVERT(1;\"tspm\";\"ml\")", { 0, 0, 0 });
        const auto aCompiledBitLShift = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITLSHIFT(6;1)", { 0, 0, 0 });
        const auto aCompiledBitLShiftNegative = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITLSHIFT(10;-2)", { 0, 0, 0 });
        const auto aCompiledBitLShiftDefault = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITLSHIFT(4;)", { 0, 0, 0 });
        const auto aCompiledBitLShiftError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITLSHIFT(-4;2)", { 0, 0, 0 });
        const auto aCompiledBitRShift = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITRSHIFT(6;1)", { 0, 0, 0 });
        const auto aCompiledBitRShiftNegative = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITRSHIFT(10;-2)", { 0, 0, 0 });
        const auto aCompiledBitRShiftDefault = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITRSHIFT(4;)", { 0, 0, 0 });
        const auto aCompiledBitRShiftError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=BITRSHIFT(-4;2)", { 0, 0, 0 });
        const auto aCompiledConvertAlias = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ORG.OPENOFFICE.CONVERT(1;\"m\";\"mi\")", { 0, 0, 0 });
        const auto aCompiledConvertEuro = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ORG.OPENOFFICE.CONVERT(100;\"ATS\";\"EUR\")", { 0, 0, 0 });
        const auto aCompiledConvertEuroCaseError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ORG.OPENOFFICE.CONVERT(100;\"skk\";\"skK\")", { 0, 0, 0 });
        const auto aCompiledConvertAliasOptionalError
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=ORG.OPENOFFICE.CONVERT(100;\"EUR\";\"SIT\";FALSE())", { 0, 0, 0 });
        const auto aCompiledEuroConvert = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=EUROCONVERT(100;\"ATS\";\"EUR\")", { 0, 0, 0 });
        const auto aCompiledEuroConvertCase = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=EUROCONVERT(100;\"EUR\";\"skK\")", { 0, 0, 0 });
        const auto aCompiledEuroConvertPrecision = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=EUROCONVERT(100;\"EUR\";\"SIT\";;3)", { 0, 0, 0 });
        const auto aCompiledEuroConvertFullPrecision
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=EUROCONVERT(100;\"ATS\";\"EUR\";TRUE())", { 0, 0, 0 });
        const auto aCompiledEuroConvertPrecisionError
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=EUROCONVERT(100;\"EUR\";\"SIT\";0;2)", { 0, 0, 0 });
        const auto aCompiledHyperlink = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=HYPERLINK(\"https://example.com\";\"Example\")", { 0, 0, 0 });
        const auto aCompiledHyperlinkError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=HYPERLINK(NA();\"Example\")", { 0, 0, 0 });
        const auto aCompiledTextJoin = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=TEXTJOIN(\"-\";1;\"\";\"A\";\"\";\"B\")", { 0, 0, 0 });
        const auto aCompiledOffset = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=OFFSET([.BA1];1;0;1;2)", { 0, 0, 0 });
        const auto aCompiledConcatenate = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CONCATENATE(\"A\";1;\"B\")", { 0, 0, 0 });
        const auto aCompiledLarge = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=LARGE({1;3;2};2)", { 0, 0, 0 });
        const auto aCompiledSmall = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SMALL({1;3;2};2)", { 0, 0, 0 });
        const auto aCompiledPercentile = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=PERCENTILE({1;2;3;4};0.25)", { 0, 0, 0 });
        const auto aCompiledPercentileExc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.PERCENTILE.EXC({1;2;3;4};0.25)", { 0, 0, 0 });
        const auto aCompiledQuartile = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=QUARTILE({7;8;9;10};3)", { 0, 0, 0 });
        const auto aCompiledQuartileExc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.QUARTILE.EXC({7;8;9;10};3)", { 0, 0, 0 });
        const auto aCompiledSkew = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SKEW({1;2;2;3;9})", { 0, 0, 0 });
        const auto aCompiledSkewp = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SKEWP({1;2;2;3;9})", { 0, 0, 0 });

        const auto checkNumber = [&](const char* pLabel, const auto& rResult,
                                     double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: scalar function mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto checkText = [&](const char* pLabel, const auto& rResult,
                                   std::u16string_view rExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isText() || rResult.mbUsedCachedValue
                || rResult.maValue.maValue.maString != rExpected)
            {
                std::fprintf(stderr, "%s: scalar function mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto checkError = [&](const char* pLabel, const auto& rResult,
                                    spreadsheetengine::api::Error eExpected) -> bool {
            if (rResult.mbUsedCachedValue)
            {
                std::fprintf(stderr, "%s: scalar function mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }

            if (!rResult)
                return rResult.meError == eExpected;

            if (rResult.maValue.isScalar() && rResult.maValue.maValue.isError()
                && rResult.maValue.maValue.meError == eExpected)
            {
                return true;
            }

            std::fprintf(stderr, "%s: scalar function mismatch in %s\n",
                "spreadsheetengine_fods_evaluator_tests", pLabel);
            return false;
        };

        if (!checkNumber("ABS", aAbs, 7.25)
            || !checkNumber("DEGREES", aDegrees, 180.0)
            || !checkNumber("PI", aPi, 3.141592653589793)
            || !checkNumber("ATANH", aAtanh, 0.5493061443340549)
            || !checkNumber("FISHER", aFisher, 0.5493061443340549)
            || !checkNumber("FISHERINV", aFisherInv, 0.5)
            || !checkNumber("GAUSS", aGauss, 0.341344746068543)
            || !checkNumber("GAMMALN", aGammaLn, 3.17805383034795)
            || !checkNumber("GAMMALN.PRECISE", aGammaLnPrecise, 3.17805383034795)
            || !checkNumber("GCD", aGcd, 8.0)
            || !checkNumber("LCM", aLcm, 60.0)
            || !checkNumber("GEOMEAN", aGeoMean, 0.5)
            || !checkNumber("GEOMEAN zero", aGeoMeanZero, 0.0)
            || !checkNumber("HARMEAN", aHarMean, 1.71428571428571)
            || !checkError(
                "HARMEAN error", aHarMeanError, spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("DATEDIF", aDateDif, 2.0)
            || !checkNumber("FLOOR", aFloor, -12.0)
            || !checkNumber("FLOOR mode", aFloorMode, -7.0)
            || !checkNumber("FLOOR missing value", aFloorMissingValue, 0.0)
            || !checkNumber("CEILING", aCeiling, -10.0)
            || !checkNumber("FLOOR.MATH", aFloorMath, -10.0)
            || !checkNumber("CEILING.MATH", aCeilingMath, -6.0)
            || !checkNumber("CEILING.PRECISE", aCeilingPrecise, -2.0)
            || !checkNumber("FLOOR.PRECISE", aFloorPrecise, -4.0)
            || !checkError("FLOOR.PRECISE error", aFloorPreciseError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("ISO.CEILING", aIsoCeiling, -4.0)
            || !checkText("CHAR", aChar, u"A")
            || !checkNumber("CODE", aCode, 65.0)
            || !checkNumber("DECIMAL", aDecimal, 255.0)
            || !checkText("DEC2HEX", aDec2Hex, u"FF")
            || !checkNumber("LOG", aLog, 3.0)
            || !checkNumber("MROUND", aMround, 12.0)
            || !checkNumber("MROUND tie", aMroundTie, 1.5)
            || !checkError("MROUND missing", aMroundMissing,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("COMBIN", aCombin, 15.0)
            || !checkNumber("COMBINA", aCombina, 20.0)
            || !checkNumber("COMBINA zero", aCombinaZero, 0.0)
            || !checkNumber("COMBINA k zero", aCombinaKZero, 1.0)
            || !checkError("COMBINA error", aCombinaError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("MULTINOMIAL", aMultinomial, 60.0)
            || !checkNumber("MULTINOMIAL large", aMultinomialLarge, 6.1897002310145506E+26)
            || !checkNumber("BITXOR", aBitXor, 6.0)
            || !checkNumber("BITXOR left missing", aBitXorLeftMissing, 25.0)
            || !checkNumber("BITXOR right missing", aBitXorRightMissing, 25.0)
            || !checkError("BITXOR missing arg", aBitXorNoValue,
                spreadsheetengine::api::Error::NoValue)
            || !checkError("BITXOR error", aBitXorError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("CSC", aCsc, 1.0)
            || !checkNumber("CSCH", aCsch, 0.850918128239322)
            || !checkError("CSCH zero", aCschZero,
                spreadsheetengine::api::Error::DivisionByZero)
            || !checkNumber("TRUNC", aTrunc, -123.45)
            || !checkText("UPPER", aUpper, u"MIXED")
            || !checkText("LOWER", aLower, u"mixed")
            || !checkNumber("LEN", aLen, 2.0)
            || !checkNumber("LENB", aLenb, 3.0)
            || !checkNumber("SEARCH", aSearch, 2.0)
            || !checkError("FIND", aFind, spreadsheetengine::api::Error::NotAvailable)
            || !checkText("MID", aMid, u"😀B")
            || !checkText("REPLACE", aReplace, u"aZZef")
            || !checkText("BASE", aBase, u"00FF")
            || !checkText("ROMAN", aRoman, u"ID")
            || !checkText("LEFT", aLeft, u"A😀")
            || !checkText("RIGHT", aRight, u"BC")
            || !checkText("PROPER", aProper, u"Hello.There")
            || !checkText("PROPER digits", aProperDigits, u"76Budget")
            || !checkText("SUBSTITUTE", aSubstitute, u"12312abc123")
            || !checkText("T", aT, u"")
            || !checkError("T error", aTError, spreadsheetengine::api::Error::NotAvailable)
            || !checkText("CONCAT", aConcat, u"AACEG")
            || !checkNumber("FINDB", aFindb, 3.0)
            || !checkNumber("FINDB nbsp", aFindbNbsp, 8.0)
            || !checkNumber("SEARCHB", aSearchb, 2.0)
            || !checkText("REPLACEB", aReplaceb, u"ab ᄔᄕ")
            || !checkText("ASC", aAsc, u"ABC123")
            || !checkText("ASC katakana", aAscKana, u"ｵｰﾌﾟﾝｵﾌｨｽ")
            || !checkText("JIS punctuation", aJisPunctuation, u"！”＃＄％＆’（）＊＋，－．／")
            || !checkText("JIS quotes", aJisQuotes, u"”’￥‘")
            || !checkText("JIS kana marks", aJisKanaMarks, u"゛゜")
            || !checkText("JIS voiced kana", aJisVoicedKana, u"ガギグゲゴ")
            || !checkText("JIS voiced vowels", aJisVoicedVowels, u"ア゛イ゛ウ゛エ゛オ゛")
            || !checkNumber("LEGACY.CHIDIST", aLegacyChiDist, 0.5724067044708797)
            || !checkText("ADDRESS", aAddress, u"$E$4")
            || !checkText("ADDRESS row mixed", aAddressRowMixed, u"E$4")
            || !checkText("ADDRESS column mixed", aAddressColumnMixed, u"$E4")
            || !checkText("ADDRESS R1C1", aAddressR1C1, u"R[4]C[5]")
            || !checkText("ADDRESS sheet", aAddressSheet, u"Sheet2.A$1")
            || !checkText("ADDRESS quoted sheet", aAddressQuotedSheet, u"'Sheet 3'.A1")
            || !checkText("ADDRESS quoted sheet R1C1", aAddressQuotedSheetR1C1,
                u"'Sheet 3'!R1C1")
            || !checkError("ADDRESS error", aAddressError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("CONVERT", aConvert, 0.000621371192237)
            || !checkNumber("CONVERT temperature", aConvertTemp, 35.6)
            || !checkNumber("CONVERT cubic", aConvertCubic, 0.000578703703705)
            || !checkNumber("CONVERT caret alias", aConvertCaretAlias, 0.000578703703705)
            || !checkNumber("CONVERT cubic step", aConvertCubicStep, 0.000160333493666)
            || !checkNumber("CONVERT Pica", aConvertPica, 0.083333333333353)
            || !checkNumber("CONVERT Pica step", aConvertPicaStep, 2.63046611952801E-06)
            || !checkNumber("CONVERT in3->gal", aConvertIn3ToGal, 39.2727272727273)
            || !checkNumber("CONVERT m3->yd3", aConvertM3ToYd3, 13.0795061931439)
            || !checkNumber("CONVERT MTON->Nmi3", aConvertMtonToNmi3, 11.1445349270435)
            || !checkNumber("CONVERT tspm->ml", aConvertTspmToMl, 5.0)
            || !checkNumber("BITLSHIFT", aBitLShift, 12.0)
            || !checkNumber("BITLSHIFT negative shift", aBitLShiftNegative, 2.0)
            || !checkNumber("BITLSHIFT default shift", aBitLShiftDefault, 4.0)
            || !checkError(
                "BITLSHIFT error", aBitLShiftError, spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("BITRSHIFT", aBitRShift, 3.0)
            || !checkNumber("BITRSHIFT negative shift", aBitRShiftNegative, 40.0)
            || !checkNumber("BITRSHIFT default shift", aBitRShiftDefault, 4.0)
            || !checkError(
                "BITRSHIFT error", aBitRShiftError, spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("CONVERT alias", aConvertAlias, 0.000621371192237)
            || !checkNumber("CONVERT euro", aConvertEuro, 7.26728341678597)
            || !checkError("CONVERT euro case error", aConvertEuroCaseError,
                spreadsheetengine::api::Error::NotAvailable)
            || !checkError("CONVERT alias optional error", aConvertAliasOptionalError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("EUROCONVERT", aEuroConvert, 7.27)
            || !checkNumber("EUROCONVERT case-insensitive", aEuroConvertCase, 3012.6)
            || !checkNumber("EUROCONVERT precision", aEuroConvertPrecision, 23964.0)
            || !checkNumber("EUROCONVERT full precision", aEuroConvertFullPrecision,
                7.26728341678597)
            || !checkError("EUROCONVERT precision error", aEuroConvertPrecisionError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkText("HYPERLINK", aHyperlink, u"Example")
            || !checkError(
                "HYPERLINK error", aHyperlinkError, spreadsheetengine::api::Error::NotAvailable)
            || !checkText("TEXTJOIN", aTextJoin, u"A-B")
            || !checkText("CONCATENATE", aConcatenate, u"A1B")
            || !checkNumber("LARGE", aLarge, 2.0)
            || !checkNumber("SMALL", aSmall, 2.0)
            || !checkNumber("PERCENTILE", aPercentile, 1.75)
            || !checkNumber("PERCENTILE.EXC", aPercentileExc, 1.25)
            || !checkNumber("QUARTILE", aQuartile, 9.25)
            || !checkNumber("QUARTILE.EXC", aQuartileExc, 9.75)
            || !checkNumber("SKEW", aSkew, 1.9693601762387922)
            || !checkNumber("SKEWP", aSkewp, 1.3210869678752712)
            || !checkNumber("compiled ABS", aCompiledAbs, 7.25)
            || !checkNumber("compiled DEGREES", aCompiledDegrees, 180.0)
            || !checkNumber("compiled PI", aCompiledPi, 3.141592653589793)
            || !checkNumber("compiled ATANH", aCompiledAtanh, 0.5493061443340549)
            || !checkNumber("compiled FISHER", aCompiledFisher, 0.5493061443340549)
            || !checkNumber("compiled FISHERINV", aCompiledFisherInv, 0.5)
            || !checkNumber("compiled GAUSS", aCompiledGauss, 0.341344746068543)
            || !checkNumber("compiled GAMMALN", aCompiledGammaLn, 3.17805383034795)
            || !checkNumber(
                "compiled GAMMALN.PRECISE", aCompiledGammaLnPrecise, 3.17805383034795)
            || !checkNumber("compiled GCD", aCompiledGcd, 8.0)
            || !checkNumber("compiled LCM", aCompiledLcm, 60.0)
            || !checkNumber("compiled GEOMEAN", aCompiledGeoMean, 0.5)
            || !checkNumber("compiled GEOMEAN zero", aCompiledGeoMeanZero, 0.0)
            || !checkNumber("compiled HARMEAN", aCompiledHarMean, 1.71428571428571)
            || !checkError("compiled HARMEAN error", aCompiledHarMeanError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled DATEDIF", aCompiledDateDif, 2.0)
            || !checkNumber("compiled FLOOR", aCompiledFloor, -12.0)
            || !checkNumber("compiled FLOOR mode", aCompiledFloorMode, -7.0)
            || !checkNumber("compiled FLOOR missing value", aCompiledFloorMissingValue, 0.0)
            || !checkNumber("compiled CEILING", aCompiledCeiling, -10.0)
            || !checkNumber("compiled FLOOR.MATH", aCompiledFloorMath, -10.0)
            || !checkNumber("compiled CEILING.MATH", aCompiledCeilingMath, -6.0)
            || !checkNumber("compiled CEILING.PRECISE", aCompiledCeilingPrecise, -2.0)
            || !checkNumber("compiled FLOOR.PRECISE", aCompiledFloorPrecise, -4.0)
            || !checkError("compiled FLOOR.PRECISE error", aCompiledFloorPreciseError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled ISO.CEILING", aCompiledIsoCeiling, -4.0)
            || !checkText("compiled CHAR", aCompiledChar, u"A")
            || !checkNumber("compiled CODE", aCompiledCode, 65.0)
            || !checkNumber("compiled DECIMAL", aCompiledDecimal, 255.0)
            || !checkText("compiled DEC2HEX", aCompiledDec2Hex, u"FF")
            || !checkNumber("compiled LOG", aCompiledLog, 3.0)
            || !checkNumber("compiled MROUND", aCompiledMround, 12.0)
            || !checkNumber("compiled MROUND tie", aCompiledMroundTie, 1.5)
            || !checkError("compiled MROUND missing", aCompiledMroundMissing,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled COMBIN", aCompiledCombin, 15.0)
            || !checkNumber("compiled COMBINA", aCompiledCombina, 20.0)
            || !checkNumber("compiled COMBINA zero", aCompiledCombinaZero, 0.0)
            || !checkNumber("compiled COMBINA k zero", aCompiledCombinaKZero, 1.0)
            || !checkError("compiled COMBINA error", aCompiledCombinaError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled MULTINOMIAL", aCompiledMultinomial, 60.0)
            || !checkNumber("compiled MULTINOMIAL large", aCompiledMultinomialLarge,
                6.1897002310145506E+26)
            || !checkNumber("compiled BITXOR", aCompiledBitXor, 6.0)
            || !checkNumber("compiled BITXOR left missing", aCompiledBitXorLeftMissing, 25.0)
            || !checkNumber("compiled BITXOR right missing", aCompiledBitXorRightMissing, 25.0)
            || !checkError("compiled BITXOR missing arg", aCompiledBitXorNoValue,
                spreadsheetengine::api::Error::NoValue)
            || !checkError("compiled BITXOR error", aCompiledBitXorError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled CSC", aCompiledCsc, 1.0)
            || !checkNumber("compiled CSCH", aCompiledCsch, 0.850918128239322)
            || !checkError("compiled CSCH zero", aCompiledCschZero,
                spreadsheetengine::api::Error::DivisionByZero)
            || !checkNumber("compiled TRUNC", aCompiledTrunc, -123.45)
            || !checkText("compiled UPPER", aCompiledUpper, u"MIXED")
            || !checkText("compiled LOWER", aCompiledLower, u"mixed")
            || !checkNumber("compiled LEN", aCompiledLen, 2.0)
            || !checkNumber("compiled LENB", aCompiledLenb, 3.0)
            || !checkNumber("compiled SEARCH", aCompiledSearch, 2.0)
            || !checkError(
                "compiled FIND", aCompiledFind, spreadsheetengine::api::Error::NotAvailable)
            || !checkText("compiled MID", aCompiledMid, u"😀B")
            || !checkText("compiled REPLACE", aCompiledReplace, u"aZZef")
            || !checkText("compiled BASE", aCompiledBase, u"00FF")
            || !checkText("compiled ROMAN", aCompiledRoman, u"ID")
            || !checkText("compiled LEFT", aCompiledLeft, u"A😀")
            || !checkText("compiled RIGHT", aCompiledRight, u"BC")
            || !checkText("compiled PROPER", aCompiledProper, u"Hello.There")
            || !checkText("compiled PROPER digits", aCompiledProperDigits, u"76Budget")
            || !checkText("compiled SUBSTITUTE", aCompiledSubstitute, u"12312abc123")
            || !checkText("compiled T", aCompiledT, u"")
            || !checkError(
                "compiled T error", aCompiledTError, spreadsheetengine::api::Error::NotAvailable)
            || !checkText("compiled CONCAT", aCompiledConcat, u"AACEG")
            || !checkNumber("compiled FINDB", aCompiledFindb, 3.0)
            || !checkNumber("compiled FINDB nbsp", aCompiledFindbNbsp, 8.0)
            || !checkNumber("compiled SEARCHB", aCompiledSearchb, 2.0)
            || !checkText("compiled REPLACEB", aCompiledReplaceb, u"ab ᄔᄕ")
            || !checkText("compiled ASC", aCompiledAsc, u"ABC123")
            || !checkText("compiled ASC katakana", aCompiledAscKana, u"ｵｰﾌﾟﾝｵﾌｨｽ")
            || !checkText("compiled JIS punctuation", aCompiledJisPunctuation,
                u"！”＃＄％＆’（）＊＋，－．／")
            || !checkText("compiled JIS quotes", aCompiledJisQuotes, u"”’￥‘")
            || !checkText("compiled JIS kana marks", aCompiledJisKanaMarks, u"゛゜")
            || !checkText("compiled JIS voiced kana", aCompiledJisVoicedKana, u"ガギグゲゴ")
            || !checkText("compiled JIS voiced vowels", aCompiledJisVoicedVowels,
                u"ア゛イ゛ウ゛エ゛オ゛")
            || !checkNumber("compiled LEGACY.CHIDIST", aCompiledLegacyChiDist,
                0.5724067044708797)
            || !checkText("compiled ADDRESS", aCompiledAddress, u"$E$4")
            || !checkText("compiled ADDRESS row mixed", aCompiledAddressRowMixed, u"E$4")
            || !checkText("compiled ADDRESS column mixed", aCompiledAddressColumnMixed, u"$E4")
            || !checkText("compiled ADDRESS R1C1", aCompiledAddressR1C1, u"R[4]C[5]")
            || !checkText("compiled ADDRESS sheet", aCompiledAddressSheet, u"Sheet2.A$1")
            || !checkText("compiled ADDRESS quoted sheet", aCompiledAddressQuotedSheet,
                u"'Sheet 3'.A1")
            || !checkText("compiled ADDRESS quoted sheet R1C1",
                aCompiledAddressQuotedSheetR1C1, u"'Sheet 3'!R1C1")
            || !checkError("compiled ADDRESS error", aCompiledAddressError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled CONVERT", aCompiledConvert, 0.000621371192237)
            || !checkNumber("compiled CONVERT temperature", aCompiledConvertTemp, 35.6)
            || !checkNumber("compiled CONVERT cubic", aCompiledConvertCubic,
                0.000578703703705)
            || !checkNumber("compiled CONVERT caret alias", aCompiledConvertCaretAlias,
                0.000578703703705)
            || !checkNumber("compiled CONVERT cubic step", aCompiledConvertCubicStep,
                0.000160333493666)
            || !checkNumber("compiled CONVERT Pica", aCompiledConvertPica, 0.083333333333353)
            || !checkNumber(
                "compiled CONVERT Pica step", aCompiledConvertPicaStep, 2.63046611952801E-06)
            || !checkNumber(
                "compiled CONVERT in3->gal", aCompiledConvertIn3ToGal, 39.2727272727273)
            || !checkNumber(
                "compiled CONVERT m3->yd3", aCompiledConvertM3ToYd3, 13.0795061931439)
            || !checkNumber("compiled CONVERT MTON->Nmi3", aCompiledConvertMtonToNmi3,
                11.1445349270435)
            || !checkNumber(
                "compiled CONVERT tspm->ml", aCompiledConvertTspmToMl, 5.0)
            || !checkNumber("compiled BITLSHIFT", aCompiledBitLShift, 12.0)
            || !checkNumber("compiled BITLSHIFT negative shift", aCompiledBitLShiftNegative, 2.0)
            || !checkNumber("compiled BITLSHIFT default shift", aCompiledBitLShiftDefault, 4.0)
            || !checkError("compiled BITLSHIFT error", aCompiledBitLShiftError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled BITRSHIFT", aCompiledBitRShift, 3.0)
            || !checkNumber("compiled BITRSHIFT negative shift", aCompiledBitRShiftNegative, 40.0)
            || !checkNumber("compiled BITRSHIFT default shift", aCompiledBitRShiftDefault, 4.0)
            || !checkError("compiled BITRSHIFT error", aCompiledBitRShiftError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled CONVERT alias", aCompiledConvertAlias,
                0.000621371192237)
            || !checkNumber("compiled CONVERT euro", aCompiledConvertEuro, 7.26728341678597)
            || !checkError("compiled CONVERT euro case error", aCompiledConvertEuroCaseError,
                spreadsheetengine::api::Error::NotAvailable)
            || !checkError("compiled CONVERT alias optional error",
                aCompiledConvertAliasOptionalError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled EUROCONVERT", aCompiledEuroConvert, 7.27)
            || !checkNumber("compiled EUROCONVERT case-insensitive",
                aCompiledEuroConvertCase, 3012.6)
            || !checkNumber("compiled EUROCONVERT precision",
                aCompiledEuroConvertPrecision, 23964.0)
            || !checkNumber("compiled EUROCONVERT full precision",
                aCompiledEuroConvertFullPrecision, 7.26728341678597)
            || !checkError("compiled EUROCONVERT precision error",
                aCompiledEuroConvertPrecisionError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkText("compiled HYPERLINK", aCompiledHyperlink, u"Example")
            || !checkError("compiled HYPERLINK error", aCompiledHyperlinkError,
                spreadsheetengine::api::Error::NotAvailable)
            || !checkText("compiled TEXTJOIN", aCompiledTextJoin, u"A-B")
            || !checkText("compiled CONCATENATE", aCompiledConcatenate, u"A1B")
            || !checkNumber("compiled LARGE", aCompiledLarge, 2.0)
            || !checkNumber("compiled SMALL", aCompiledSmall, 2.0)
            || !checkNumber("compiled PERCENTILE", aCompiledPercentile, 1.75)
            || !checkNumber("compiled PERCENTILE.EXC", aCompiledPercentileExc, 1.25)
            || !checkNumber("compiled QUARTILE", aCompiledQuartile, 9.25)
            || !checkNumber("compiled QUARTILE.EXC", aCompiledQuartileExc, 9.75)
            || !checkNumber("compiled SKEW", aCompiledSkew, 1.9693601762387922)
            || !checkNumber("compiled SKEWP", aCompiledSkewp, 1.3210869678752712))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "scalar function evaluation mismatch");
        }

        if (!aOffset || !aOffset.maValue.isMatrixReference()
            || aOffset.maValue.maReference.matrixDimensions().mnColumns != 2
            || aOffset.maValue.maReference.matrixDimensions().mnRows != 1)
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "OFFSET view mismatch");
        }

        const auto aOffsetFirst = aEvaluator.materializeReferenceValue(aOffset.maValue.maReference, 0, 0);
        const auto aOffsetSecond = aEvaluator.materializeReferenceValue(aOffset.maValue.maReference, 1, 0);
        if (!aOffsetFirst || !aOffsetSecond || !aOffsetFirst.maValue.maValue.isNumber()
            || !aOffsetSecond.maValue.maValue.isNumber()
            || !almostEqual(aOffsetFirst.maValue.maValue.mfNumber, 2.0)
            || !almostEqual(aOffsetSecond.maValue.maValue.mfNumber, 22.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "OFFSET materialization mismatch");
        }

        if (!aCompiledOffset || !aCompiledOffset.maValue.isMatrixReference()
            || aCompiledOffset.maValue.maReference.matrixDimensions().mnColumns != 2
            || aCompiledOffset.maValue.maReference.matrixDimensions().mnRows != 1)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "compiled OFFSET view mismatch");
        }

        const auto aCompiledOffsetFirst
            = aEvaluator.materializeReferenceValue(aCompiledOffset.maValue.maReference, 0, 0);
        const auto aCompiledOffsetSecond
            = aEvaluator.materializeReferenceValue(aCompiledOffset.maValue.maReference, 1, 0);
        if (!aCompiledOffsetFirst || !aCompiledOffsetSecond
            || !aCompiledOffsetFirst.maValue.maValue.isNumber()
            || !aCompiledOffsetSecond.maValue.maValue.isNumber()
            || !almostEqual(aCompiledOffsetFirst.maValue.maValue.mfNumber, 2.0)
            || !almostEqual(aCompiledOffsetSecond.maValue.maValue.mfNumber, 22.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "compiled OFFSET materialization mismatch");
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
        const auto aFv = aEvaluator.evaluateFormula(u"of:=FV(0.04;2;750;2500)", { 0, 0, 0 });
        const auto aPv
            = aEvaluator.evaluateFormula(u"of:=PV(0.08/12;48;500;20000)", { 0, 0, 0 });
        const auto aPmt
            = aEvaluator.evaluateFormula(u"of:=PMT(0.0199/12;36;25000)", { 0, 0, 0 });
        const auto aNper
            = aEvaluator.evaluateFormula(u"of:=NPER(0.06;153.75;2600)", { 0, 0, 0 });
        const auto aRate = aEvaluator.evaluateFormula(u"of:=RATE(3;-10;900)", { 0, 0, 0 });
        const auto aIspmt
            = aEvaluator.evaluateFormula(u"of:=ISPMT(0.05;5;7;15000)", { 0, 0, 0 });
        const auto aIpmt
            = aEvaluator.evaluateFormula(u"of:=IPMT(0.05;5;7;15000)", { 0, 0, 0 });
        const auto aPpmt = aEvaluator.evaluateFormula(
            u"of:=PPMT(0.0875/12;1;36;5000;8000;1)", { 0, 0, 0 });
        const auto aDdb
            = aEvaluator.evaluateFormula(u"of:=DDB(25000;1000;36;1;6)", { 0, 0, 0 });
        const auto aVdb
            = aEvaluator.evaluateFormula(u"of:=VDB(35000;7500;36;10;20;2)", { 0, 0, 0 });
        const auto aCumIpmt = aEvaluator.evaluateFormula(
            u"of:=CUMIPMT(0.055/12;24;5000;4;6;1)", { 0, 0, 0 });
        const auto aCumPrinc = aEvaluator.evaluateFormula(
            u"of:=CUMPRINC(0.055/12;24;5000;4;6;1)", { 0, 0, 0 });
        const auto aDb = aEvaluator.evaluateFormula(u"of:=DB(25000;1000;36;1;6)", { 0, 0, 0 });
        const auto aDisc = aEvaluator.evaluateFormula(
            u"of:=DISC(\"2001-01-25\";\"2001-11-15\";97;100;3)", { 0, 0, 0 });
        const auto aMduration = aEvaluator.evaluateFormula(
            u"of:=MDURATION(\"2001-01-01\";\"2006-01-01\";0.08;0.09;2;3)", { 0, 0, 0 });
        const auto aYield = aEvaluator.evaluateFormula(
            u"of:=YIELD(DATE(1999;2;15);DATE(2007;11;15);0.0575;95.04287;100;2;0)",
            { 0, 0, 0 });
        const auto aTbillprice = aEvaluator.evaluateFormula(
            u"of:=TBILLPRICE(DATE(1999;3;31);DATE(1999;6;1);0.0914)", { 0, 0, 0 });
        const auto aTbillyield = aEvaluator.evaluateFormula(
            u"of:=TBILLYIELD(DATE(1999;3;31);DATE(1999;6;1);0.0914)", { 0, 0, 0 });
        const auto aOddlprice = aEvaluator.evaluateFormula(
            u"of:=ODDLPRICE(DATE(1999;2;7);DATE(1999;6;15);DATE(1998;10;15);0.0375;0.0405;100;2;0)",
            { 0, 0, 0 });
        const auto aCompiledFv = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=FV(0.04;2;750;2500)", { 0, 0, 0 });
        const auto aCompiledPv = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=PV(0.08/12;48;500;20000)", { 0, 0, 0 });
        const auto aCompiledPmt = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=PMT(0.0199/12;36;25000)", { 0, 0, 0 });
        const auto aCompiledNper = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=NPER(0.06;153.75;2600)", { 0, 0, 0 });
        const auto aCompiledRate = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=RATE(3;-10;900)", { 0, 0, 0 });
        const auto aCompiledIspmt = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ISPMT(0.05;5;7;15000)", { 0, 0, 0 });
        const auto aCompiledIpmt = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=IPMT(0.05;5;7;15000)", { 0, 0, 0 });
        const auto aCompiledPpmt = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=PPMT(0.0875/12;1;36;5000;8000;1)", { 0, 0, 0 });
        const auto aCompiledDdb = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=DDB(25000;1000;36;1;6)", { 0, 0, 0 });
        const auto aCompiledVdb = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=VDB(35000;7500;36;10;20;2)", { 0, 0, 0 });
        const auto aCompiledCumIpmt = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CUMIPMT(0.055/12;24;5000;4;6;1)", { 0, 0, 0 });
        const auto aCompiledCumPrinc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CUMPRINC(0.055/12;24;5000;4;6;1)", { 0, 0, 0 });
        const auto aCompiledDb = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=DB(25000;1000;36;1;6)", { 0, 0, 0 });
        const auto aCompiledDisc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=DISC(\"2001-01-25\";\"2001-11-15\";97;100;3)", { 0, 0, 0 });
        const auto aCompiledMduration = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MDURATION(\"2001-01-01\";\"2006-01-01\";0.08;0.09;2;3)", { 0, 0, 0 });
        const auto aCompiledYield = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=YIELD(DATE(1999;2;15);DATE(2007;11;15);0.0575;95.04287;100;2;0)",
            { 0, 0, 0 });
        const auto aCompiledTbillprice = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=TBILLPRICE(DATE(1999;3;31);DATE(1999;6;1);0.0914)", { 0, 0, 0 });
        const auto aCompiledTbillyield = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=TBILLYIELD(DATE(1999;3;31);DATE(1999;6;1);0.0914)", { 0, 0, 0 });
        const auto aCompiledOddlprice = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ODDLPRICE(DATE(1999;2;7);DATE(1999;6;15);DATE(1998;10;15);0.0375;0.0405;100;2;0)",
            { 0, 0, 0 });
        const auto aOddlyield = aEvaluator.evaluateFormula(
            u"of:=ODDLYIELD(\"1999-04-20\";\"1999-06-15\";\"1998-10-15\";0.0375;99.875;100;2)",
            { 0, 0, 0 });
        const auto aOddlyieldError = aEvaluator.evaluateFormula(
            u"of:=ODDLYIELD(\"1999-04-20\";\"1999-06-15\";\"1998-10-15\";0.0375;0;100;2;0)",
            { 0, 0, 0 });
        const auto aAmorlinc = aEvaluator.evaluateFormula(
            u"of:=AMORLINC(10000;DATE(2012;3;1);DATE(2012;12;31);1500;1;0.3)", { 0, 0, 0 });
        const auto aAmorlincError = aEvaluator.evaluateFormula(
            u"of:=AMORLINC(-10000;DATE(2012;3;1);DATE(2012;12;31);1500;1;0.3;4)",
            { 0, 0, 0 });
        const auto aReceived = aEvaluator.evaluateFormula(
            u"of:=RECEIVED(DATE(1999;2;15);DATE(1999;5;15);1000;0.0575;0)", { 0, 0, 0 });
        const auto aPricedisc = aEvaluator.evaluateFormula(
            u"of:=PRICEDISC(\"1999-02-15\";\"1999-03-01\";0.0525;100;2)", { 0, 0, 0 });
        const auto aIntrate = aEvaluator.evaluateFormula(
            u"of:=INTRATE(\"1990-01-15\";\"2002-05-05\";1000000;2000000;3)", { 0, 0, 0 });
        const auto aNominal = aEvaluator.evaluateFormula(u"of:=NOMINAL(0.135;12)", { 0, 0, 0 });
        const auto aNpv = aEvaluator.evaluateFormula(u"of:=NPV(0.0875;10;20;30)", { 0, 0, 0 });
        const auto aRri = aEvaluator.evaluateFormula(u"of:=RRI(4;7500;10000)", { 0, 0, 0 });
        const auto aSln = aEvaluator.evaluateFormula(u"of:=SLN(50000;3.5;84)", { 0, 0, 0 });
        const auto aSyd = aEvaluator.evaluateFormula(u"of:=SYD(50000;10000;5;1)", { 0, 0, 0 });
        const auto aAccrintm = aEvaluator.evaluateFormula(
            u"of:=ACCRINTM(DATE(2012;1;1);DATE(2013;2;15);0.065;5000;3)", { 0, 0, 0 });
        const auto aPricemat = aEvaluator.evaluateFormula(
            u"of:=PRICEMAT(\"1999-02-15\";\"1999-04-13\";\"1998-11-11\";0.061;0.061;0)",
            { 0, 0, 0 });
        const auto aYielddisc = aEvaluator.evaluateFormula(
            u"of:=YIELDDISC(DATE(1999;2;15);DATE(1999;3;1);99.795;100;2)", { 0, 0, 0 });
        const auto aCoupdaybs = aEvaluator.evaluateFormula(
            u"of:=COUPDAYBS(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCoupdays = aEvaluator.evaluateFormula(
            u"of:=COUPDAYS(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCoupdaysnc = aEvaluator.evaluateFormula(
            u"of:=COUPDAYSNC(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCouppcd = aEvaluator.evaluateFormula(
            u"of:=COUPPCD(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCoupncd = aEvaluator.evaluateFormula(
            u"of:=COUPNCD(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCoupnum = aEvaluator.evaluateFormula(
            u"of:=COUPNUM(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aAmordegrc = aEvaluator.evaluateFormula(
            u"of:=AMORDEGRC(10000;DATE(2012;3;1);DATE(2012;12;31);1500;1;0.31;1)",
            { 0, 0, 0 });
        const auto aIrr
            = aEvaluator.evaluateFormula(u"of:=IRR({-10000|5000|5000|5000})", { 0, 0, 0 });
        const auto aMirr = aEvaluator.evaluateFormula(
            u"of:=MIRR({-10000|3400|6500|1000};0.065;0.1)", { 0, 0, 0 });
        const auto aCompiledOddlyield = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ODDLYIELD(\"1999-04-20\";\"1999-06-15\";\"1998-10-15\";0.0375;99.875;100;2)",
            { 0, 0, 0 });
        const auto aCompiledOddlyieldError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ODDLYIELD(\"1999-04-20\";\"1999-06-15\";\"1998-10-15\";0.0375;0;100;2;0)",
            { 0, 0, 0 });
        const auto aCompiledAmorlinc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=AMORLINC(10000;DATE(2012;3;1);DATE(2012;12;31);1500;1;0.3)", { 0, 0, 0 });
        const auto aCompiledAmorlincError = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=AMORLINC(-10000;DATE(2012;3;1);DATE(2012;12;31);1500;1;0.3;4)",
            { 0, 0, 0 });
        const auto aCompiledReceived = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=RECEIVED(DATE(1999;2;15);DATE(1999;5;15);1000;0.0575;0)", { 0, 0, 0 });
        const auto aCompiledPricedisc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=PRICEDISC(\"1999-02-15\";\"1999-03-01\";0.0525;100;2)", { 0, 0, 0 });
        const auto aCompiledIntrate = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=INTRATE(\"1990-01-15\";\"2002-05-05\";1000000;2000000;3)", { 0, 0, 0 });
        const auto aCompiledNominal = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=NOMINAL(0.135;12)", { 0, 0, 0 });
        const auto aCompiledNpv = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=NPV(0.0875;10;20;30)", { 0, 0, 0 });
        const auto aCompiledRri = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=RRI(4;7500;10000)", { 0, 0, 0 });
        const auto aCompiledSln = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SLN(50000;3.5;84)", { 0, 0, 0 });
        const auto aCompiledSyd = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SYD(50000;10000;5;1)", { 0, 0, 0 });
        const auto aCompiledAccrintm = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ACCRINTM(DATE(2012;1;1);DATE(2013;2;15);0.065;5000;3)", { 0, 0, 0 });
        const auto aCompiledPricemat = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=PRICEMAT(\"1999-02-15\";\"1999-04-13\";\"1998-11-11\";0.061;0.061;0)",
            { 0, 0, 0 });
        const auto aCompiledYielddisc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=YIELDDISC(DATE(1999;2;15);DATE(1999;3;1);99.795;100;2)", { 0, 0, 0 });
        const auto aCompiledCoupdaybs = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUPDAYBS(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCompiledCoupdays = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUPDAYS(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCompiledCoupdaysnc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUPDAYSNC(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCompiledCouppcd = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUPPCD(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCompiledCoupncd = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUPNCD(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCompiledCoupnum = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COUPNUM(\"2001-01-25\";\"2001-11-15\";2;3)", { 0, 0, 0 });
        const auto aCompiledAmordegrc = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=AMORDEGRC(10000;DATE(2012;3;1);DATE(2012;12;31);1500;1;0.31;1)",
            { 0, 0, 0 });
        const auto aCompiledIrr = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=IRR({-10000|5000|5000|5000})", { 0, 0, 0 });
        const auto aCompiledMirr = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MIRR({-10000|3400|6500|1000};0.065;0.1)", { 0, 0, 0 });
        const auto aVdbMissingFactor = aEvaluator.evaluateFormula(
            u"of:=VDB(35000;7500;36;10;20;;)", { 0, 0, 0 });
        const auto aCompiledCumIpmtEmptyType = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=CUMIPMT(0.055/12;24;5000;4;6;)", { 0, 0, 0 });

        const auto checkNumber = [&](const char* pLabel, const auto& rResult,
                                     double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: financial function mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        const auto checkError = [&](const char* pLabel, const auto& rResult,
                                    spreadsheetengine::api::Error eExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isError() || rResult.mbUsedCachedValue
                || rResult.maValue.maValue.meError != eExpected)
            {
                std::fprintf(stderr, "%s: financial function error mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };

        if (!checkNumber("FV", aFv, -4234.0)
            || !checkNumber("PV", aPv, -35019.3680845542)
            || !checkNumber("PMT", aPmt, -715.955334437392)
            || !checkNumber("NPER", aNper, -12.0207780851555)
            || !checkNumber("RATE", aRate, -0.75626593687807)
            || !checkNumber("ISPMT", aIspmt, -214.285714285714)
            || !checkNumber("IPMT", aIpmt, -352.973422514773)
            || !checkNumber("PPMT", aPpmt, -350.992937038239)
            || !checkNumber("DDB", aDdb, 4166.66666666666)
            || !checkNumber("VDB", aVdb, 8603.80245372397)
            || !checkNumber("CUMIPMT", aCumIpmt, -57.5412415342252)
            || !checkNumber("CUMPRINC", aCumPrinc, -600.875855808337)
            || !checkNumber("compiled FV", aCompiledFv, -4234.0)
            || !checkNumber("compiled PV", aCompiledPv, -35019.3680845542)
            || !checkNumber("compiled PMT", aCompiledPmt, -715.955334437392)
            || !checkNumber("compiled NPER", aCompiledNper, -12.0207780851555)
            || !checkNumber("compiled RATE", aCompiledRate, -0.75626593687807)
            || !checkNumber("compiled ISPMT", aCompiledIspmt, -214.285714285714)
            || !checkNumber("compiled IPMT", aCompiledIpmt, -352.973422514773)
            || !checkNumber("compiled PPMT", aCompiledPpmt, -350.992937038239)
            || !checkNumber("compiled DDB", aCompiledDdb, 4166.66666666666)
            || !checkNumber("compiled VDB", aCompiledVdb, 8603.80245372397)
            || !checkNumber("compiled CUMIPMT", aCompiledCumIpmt, -57.5412415342252)
            || !checkNumber("compiled CUMPRINC", aCompiledCumPrinc, -600.875855808337)
            || !checkNumber("DB", aDb, 1075.0)
            || !checkNumber("DISC", aDisc, 0.0372448979591837)
            || !checkNumber("MDURATION", aMduration, 4.02068710841898)
            || !checkNumber("YIELD", aYield, 0.0650000068807552)
            || !checkNumber("TBILLPRICE", aTbillprice, 98.4258888888889)
            || !checkNumber("TBILLYIELD", aTbillyield, 6346.98524740594)
            || !checkNumber("ODDLPRICE", aOddlprice, 99.8782860147214)
            || !checkNumber("compiled DB", aCompiledDb, 1075.0)
            || !checkNumber("compiled DISC", aCompiledDisc, 0.0372448979591837)
            || !checkNumber("compiled MDURATION", aCompiledMduration, 4.02068710841898)
            || !checkNumber("compiled YIELD", aCompiledYield, 0.0650000068807552)
            || !checkNumber("compiled TBILLPRICE", aCompiledTbillprice, 98.4258888888889)
            || !checkNumber("compiled TBILLYIELD", aCompiledTbillyield, 6346.98524740594)
            || !checkNumber("compiled ODDLPRICE", aCompiledOddlprice, 99.8782860147214)
            || !checkNumber("ODDLYIELD", aOddlyield, 0.0448731663302424)
            || !checkError(
                "ODDLYIELD error", aOddlyieldError, spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("AMORLINC", aAmorlinc, 3000.0)
            || !checkError(
                "AMORLINC error", aAmorlincError, spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("RECEIVED", aReceived, 1014.25593057982)
            || !checkNumber("PRICEDISC", aPricedisc, 99.7958333333333)
            || !checkNumber("INTRATE", aIntrate, 0.0812374805252615)
            || !checkNumber("NOMINAL", aNominal, 0.127303166959042)
            || !checkNumber("NPV", aNpv, 49.432121038173)
            || !checkNumber("RRI", aRri, 0.074569931823542)
            || !checkNumber("SLN", aSln, 595.196428571429)
            || !checkNumber("SYD", aSyd, 13333.3333333333)
            || !checkNumber("ACCRINTM", aAccrintm, 365.958904109589)
            || !checkNumber("PRICEMAT", aPricemat, 99.984498875557)
            || !checkNumber("YIELDDISC", aYielddisc, 0.0528225719868601)
            || !checkNumber("COUPDAYBS", aCoupdaybs, 71.0)
            || !checkNumber("COUPDAYS", aCoupdays, 182.5)
            || !checkNumber("COUPDAYSNC", aCoupdaysnc, 110.0)
            || !checkNumber("COUPPCD", aCouppcd, 36845.0)
            || !checkNumber("COUPNCD", aCoupncd, 37026.0)
            || !checkNumber("COUPNUM", aCoupnum, 2.0)
            || !checkNumber("AMORDEGRC", aAmordegrc, 2848.0)
            || !checkNumber("IRR", aIrr, 0.233751928528259)
            || !checkNumber("MIRR", aMirr, 0.0703949396602768)
            || !checkNumber("compiled ODDLYIELD", aCompiledOddlyield, 0.0448731663302424)
            || !checkError("compiled ODDLYIELD error", aCompiledOddlyieldError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled AMORLINC", aCompiledAmorlinc, 3000.0)
            || !checkError("compiled AMORLINC error", aCompiledAmorlincError,
                spreadsheetengine::api::Error::IllegalArgument)
            || !checkNumber("compiled RECEIVED", aCompiledReceived, 1014.25593057982)
            || !checkNumber("compiled PRICEDISC", aCompiledPricedisc, 99.7958333333333)
            || !checkNumber("compiled INTRATE", aCompiledIntrate, 0.0812374805252615)
            || !checkNumber("compiled NOMINAL", aCompiledNominal, 0.127303166959042)
            || !checkNumber("compiled NPV", aCompiledNpv, 49.432121038173)
            || !checkNumber("compiled RRI", aCompiledRri, 0.074569931823542)
            || !checkNumber("compiled SLN", aCompiledSln, 595.196428571429)
            || !checkNumber("compiled SYD", aCompiledSyd, 13333.3333333333)
            || !checkNumber("compiled ACCRINTM", aCompiledAccrintm, 365.958904109589)
            || !checkNumber("compiled PRICEMAT", aCompiledPricemat, 99.984498875557)
            || !checkNumber("compiled YIELDDISC", aCompiledYielddisc, 0.0528225719868601)
            || !checkNumber("compiled COUPDAYBS", aCompiledCoupdaybs, 71.0)
            || !checkNumber("compiled COUPDAYS", aCompiledCoupdays, 182.5)
            || !checkNumber("compiled COUPDAYSNC", aCompiledCoupdaysnc, 110.0)
            || !checkNumber("compiled COUPPCD", aCompiledCouppcd, 36845.0)
            || !checkNumber("compiled COUPNCD", aCompiledCoupncd, 37026.0)
            || !checkNumber("compiled COUPNUM", aCompiledCoupnum, 2.0)
            || !checkNumber("compiled AMORDEGRC", aCompiledAmordegrc, 2848.0)
            || !checkNumber("compiled IRR", aCompiledIrr, 0.233751928528259)
            || !checkNumber("compiled MIRR", aCompiledMirr, 0.0703949396602768)
            || aVdbMissingFactor || aVdbMissingFactor.meError != spreadsheetengine::api::Error::IllegalArgument
            || aCompiledCumIpmtEmptyType
            || aCompiledCumIpmtEmptyType.meError != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "financial function evaluation mismatch");
        }
    }

    {
        constexpr DateParts aNullDate { 1899, 12, 30 };
        const auto aExpectedEomonth = makeDateSerial(aNullDate, 2015, 2, 28, true);
        const auto aExpectedEdate = makeDateSerial(aNullDate, 2001, 4, 30, true);
        const auto aExpectedDate = makeDateSerial(aNullDate, 2022, 1, 9, false);
        const auto aExpectedSequenceWorkdayIntl
            = makeDateSerial(aNullDate, 2014, 10, 30, true);
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
        const auto aWeeksInYear
            = aEvaluator.evaluateFormula(u"of:=ORG.OPENOFFICE.WEEKSINYEAR(\"2014-12-31\")",
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
        const auto aYear = aEvaluator.evaluateFormula(u"of:=YEAR(1)", { 0, 0, 0 });
        const auto aMonth = aEvaluator.evaluateFormula(u"of:=MONTH(1)", { 0, 0, 0 });
        const auto aDay
            = aEvaluator.evaluateFormula(u"of:=DAY(\"1899-12-29T15:26:14\")", { 0, 0, 0 });
        const auto aHour
            = aEvaluator.evaluateFormula(u"of:=HOUR(\"17:20:00\")", { 0, 0, 0 });
        const auto aMinute = aEvaluator.evaluateFormula(
            u"of:=MINUTE(\"1954-07-20 16:30:01\")", { 0, 0, 0 });
        const auto aSecond = aEvaluator.evaluateFormula(
            u"of:=SECOND(\"1954-07-20 16:30:01\")", { 0, 0, 0 });
        const auto aWeekday
            = aEvaluator.evaluateFormula(u"of:=WEEKDAY(\"2000-06-14\";2)", { 0, 0, 0 });
        const auto aWeeknum
            = aEvaluator.evaluateFormula(u"of:=WEEKNUM(\"2016-07-24\";21)", { 0, 0, 0 });
        const auto aDays360
            = aEvaluator.evaluateFormula(u"of:=DAYS360(DATE(2001;2;28);DATE(2001;3;31);TRUE())",
                { 0, 0, 0 });
        const auto aEasterSunday
            = aEvaluator.evaluateFormula(u"of:=EASTERSUNDAY(2015)", { 0, 0, 0 });
        const auto aYears = aEvaluator.evaluateFormula(
            u"of:=ORG.OPENOFFICE.YEARS(DATE(2014;1;15);DATE(2016;4;1);0)", { 0, 0, 0 });
        const auto aEomonth
            = aEvaluator.evaluateFormula(u"of:=EOMONTH(\"Jan11, 2015\";1)", { 0, 0, 0 });
        const auto aEdate
            = aEvaluator.evaluateFormula(u"of:=EDATE(\"2001-03-31\";1)", { 0, 0, 0 });
        const auto aWorkday = aEvaluator.evaluateFormula(
            u"of:=WORKDAY(DATE(2014;11;1);5;{\"2014-11-2\";\"2014-11-3\";\"2014-11-4\"})",
            { 0, 0, 0 });
        const auto aNetworkdays = aEvaluator.evaluateFormula(
            u"of:=NETWORKDAYS(DATE(2014;11;1);DATE(2014;11;30);{\"2014-11-11\";\"2014-11-28\";\"2014-11-27\"})",
            { 0, 0, 0 });
        const auto aNetworkdaysSequence = aEvaluator.evaluateFormula(
            u"of:=NETWORKDAYS(DATE(2014;11;1);DATE(2014;11;7);;{1;0;1;0;1;0;1})",
            { 0, 0, 0 });
        const auto aWorkdayIntl = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);2;5;{\"2014-11-2\";\"2014-11-3\";\"2014-11-4\"})",
            { 0, 0, 0 });
        const auto aNetworkdaysIntl = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.NETWORKDAYS.INTL(DATE(2014;11;1);DATE(2014;11;30);1;{\"2014-11-11\";\"2014-11-28\";\"2014-11-27\"})",
            { 0, 0, 0 });
        const auto aAllWeekendNetworkdays = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.NETWORKDAYS.INTL(DATE(2006;1;1);DATE(2006;2;1);\"1111111\";{\"2006-1-2\";\"2006-1-16\"})",
            { 0, 0, 0 });
        const auto aInvalidWorkdayIntl = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);2;8;{\"2014-11-2\"})",
            { 0, 0, 0 });
        const auto aTextWeekendWorkdayIntl = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);5;\"3\")", { 0, 0, 0 });
        const auto aNumericMaskWorkdayIntl = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);1;1100000)", { 0, 0, 0 });
        const auto aSequenceWorkdayIntl = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);-2;{1;1;0;0;0;0;0};{\"2014-11-2\";\"2014-11-3\";\"2014-11-4\"})",
            { 0, 0, 0 });
        const auto aSequenceNetworkdaysIntl = aEvaluator.evaluateFormula(
            u"of:=COM.MICROSOFT.NETWORKDAYS.INTL(DATE(2014;11;1);DATE(2014;11;7);{1;0;1;0;1;0;1})",
            { 0, 0, 0 });
        const auto aCompiledWorkdayIntl = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);2;5;{\"2014-11-2\";\"2014-11-3\";\"2014-11-4\"})",
            { 0, 0, 0 });
        const auto aCompiledNetworkdaysIntl = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.NETWORKDAYS.INTL(DATE(2014;11;1);DATE(2014;11;30);1;{\"2014-11-11\";\"2014-11-28\";\"2014-11-27\"})",
            { 0, 0, 0 });
        const auto aCompiledAllWeekendNetworkdays = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.NETWORKDAYS.INTL(DATE(2006;1;1);DATE(2006;2;1);\"1111111\";{\"2006-1-2\";\"2006-1-16\"})",
            { 0, 0, 0 });
        const auto aCompiledInvalidWorkdayIntl = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);2;8;{\"2014-11-2\"})",
            { 0, 0, 0 });
        const auto aCompiledTextWeekendWorkdayIntl = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);5;\"3\")", { 0, 0, 0 });
        const auto aCompiledNumericMaskWorkdayIntl = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);1;1100000)", { 0, 0, 0 });
        const auto aCompiledSequenceWorkdayIntl = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);-2;{1;1;0;0;0;0;0};{\"2014-11-2\";\"2014-11-3\";\"2014-11-4\"})",
            { 0, 0, 0 });
        const auto aCompiledSequenceNetworkdaysIntl = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=COM.MICROSOFT.NETWORKDAYS.INTL(DATE(2014;11;1);DATE(2014;11;7);{1;0;1;0;1;0;1})",
            { 0, 0, 0 });
        const auto aCompiledWorkday = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=WORKDAY(DATE(2014;11;1);5;{\"2014-11-2\";\"2014-11-3\";\"2014-11-4\"})",
            { 0, 0, 0 });
        const auto aCompiledNetworkdays = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=NETWORKDAYS(DATE(2014;11;1);DATE(2014;11;30);{\"2014-11-11\";\"2014-11-28\";\"2014-11-27\"})",
            { 0, 0, 0 });
        const auto aCompiledNetworkdaysSequence = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=NETWORKDAYS(DATE(2014;11;1);DATE(2014;11;7);;{1;0;1;0;1;0;1})",
            { 0, 0, 0 });
        const auto aCompiledYear = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=YEAR(1)",
            { 0, 0, 0 });
        const auto aCompiledMonth = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=MONTH(1)",
            { 0, 0, 0 });
        const auto aCompiledDay = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=DAY(\"1899-12-29T15:26:14\")", { 0, 0, 0 });
        const auto aCompiledHour = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=HOUR(\"17:20:00\")", { 0, 0, 0 });
        const auto aCompiledMinute = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=MINUTE(\"1954-07-20 16:30:01\")", { 0, 0, 0 });
        const auto aCompiledSecond = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=SECOND(\"1954-07-20 16:30:01\")", { 0, 0, 0 });
        const auto aCompiledWeekday
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=WEEKDAY(\"2000-06-14\";2)",
                { 0, 0, 0 });
        const auto aCompiledWeeknum
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=WEEKNUM(\"2016-07-24\";21)",
                { 0, 0, 0 });
        const auto aCompiledDays360
            = aEvaluator.evaluateFormulaViaCompiledTokens(
                u"of:=DAYS360(DATE(2001;2;28);DATE(2001;3;31);TRUE())", { 0, 0, 0 });
        const auto aCompiledEasterSunday
            = aEvaluator.evaluateFormulaViaCompiledTokens(u"of:=EASTERSUNDAY(2015)", { 0, 0, 0 });
        const auto aCompiledYears = aEvaluator.evaluateFormulaViaCompiledTokens(
            u"of:=ORG.OPENOFFICE.YEARS(DATE(2014;1;15);DATE(2016;4;1);0)", { 0, 0, 0 });
        const auto checkNumber = [&](const char* pLabel, const auto& rResult,
                                     double fExpected) -> bool {
            if (!rResult || !rResult.maValue.maValue.isNumber() || rResult.mbUsedCachedValue
                || !almostEqual(rResult.maValue.maValue.mfNumber, fExpected))
            {
                std::fprintf(stderr, "%s: date/time function mismatch in %s\n",
                    "spreadsheetengine_fods_evaluator_tests", pLabel);
                return false;
            }
            return true;
        };
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
            || !aWeeksInYear || !aWeeksInYear.maValue.maValue.isNumber()
            || !almostEqual(aWeeksInYear.maValue.maValue.mfNumber, 52.0)
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
            || !checkNumber("YEAR", aYear, 1899.0)
            || !checkNumber("MONTH", aMonth, 12.0)
            || !checkNumber("DAY", aDay, 29.0)
            || !checkNumber("HOUR", aHour, 17.0)
            || !checkNumber("MINUTE", aMinute, 30.0)
            || !checkNumber("SECOND", aSecond, 1.0)
            || !checkNumber("WEEKDAY", aWeekday, 3.0)
            || !checkNumber("WEEKNUM", aWeeknum, 29.0)
            || !checkNumber("DAYS360", aDays360, 32.0)
            || !checkNumber("EASTERSUNDAY", aEasterSunday, 42099.0)
            || !checkNumber("YEARS", aYears, 2.0)
            || !checkNumber("WORKDAY", aWorkday, 41954.0)
            || !checkNumber("NETWORKDAYS", aNetworkdays, 17.0)
            || !checkNumber("NETWORKDAYS sequence", aNetworkdaysSequence, 3.0)
            || !aExpectedEomonth || !aEomonth || !aEomonth.maValue.maValue.isNumber()
            || !almostEqual(aEomonth.maValue.maValue.mfNumber, aExpectedEomonth.maValue)
            || !aExpectedEdate || !aEdate || !aEdate.maValue.maValue.isNumber()
            || !almostEqual(aEdate.maValue.maValue.mfNumber, aExpectedEdate.maValue)
            || !aWorkdayIntl || !aWorkdayIntl.maValue.maValue.isNumber()
            || !almostEqual(aWorkdayIntl.maValue.maValue.mfNumber, 41951.0)
            || !aNetworkdaysIntl || !aNetworkdaysIntl.maValue.maValue.isNumber()
            || !almostEqual(aNetworkdaysIntl.maValue.maValue.mfNumber, 17.0)
            || !aExpectedSequenceWorkdayIntl || !aSequenceWorkdayIntl
            || !aSequenceWorkdayIntl.maValue.maValue.isNumber()
            || !almostEqual(aSequenceWorkdayIntl.maValue.maValue.mfNumber,
                   aExpectedSequenceWorkdayIntl.maValue)
            || !aSequenceNetworkdaysIntl
            || !aSequenceNetworkdaysIntl.maValue.maValue.isNumber()
            || !almostEqual(aSequenceNetworkdaysIntl.maValue.maValue.mfNumber, 3.0)
            || !aAllWeekendNetworkdays || !aAllWeekendNetworkdays.maValue.maValue.isNumber()
            || !almostEqual(aAllWeekendNetworkdays.maValue.maValue.mfNumber, 0.0)
            || aInvalidWorkdayIntl
            || aInvalidWorkdayIntl.meError != spreadsheetengine::api::Error::IllegalArgument
            || !aCompiledWorkdayIntl || !aCompiledWorkdayIntl.maValue.maValue.isNumber()
            || !almostEqual(aCompiledWorkdayIntl.maValue.maValue.mfNumber, 41951.0)
            || !aCompiledNetworkdaysIntl
            || !aCompiledNetworkdaysIntl.maValue.maValue.isNumber()
            || !almostEqual(aCompiledNetworkdaysIntl.maValue.maValue.mfNumber, 17.0)
            || !aCompiledSequenceWorkdayIntl
            || !aCompiledSequenceWorkdayIntl.maValue.maValue.isNumber()
            || !almostEqual(aCompiledSequenceWorkdayIntl.maValue.maValue.mfNumber,
                   aExpectedSequenceWorkdayIntl.maValue)
            || !aCompiledSequenceNetworkdaysIntl
            || !aCompiledSequenceNetworkdaysIntl.maValue.maValue.isNumber()
            || !almostEqual(aCompiledSequenceNetworkdaysIntl.maValue.maValue.mfNumber, 3.0)
            || !aCompiledAllWeekendNetworkdays
            || !aCompiledAllWeekendNetworkdays.maValue.maValue.isNumber()
            || !almostEqual(aCompiledAllWeekendNetworkdays.maValue.maValue.mfNumber, 0.0)
            || aCompiledInvalidWorkdayIntl
            || aCompiledInvalidWorkdayIntl.meError
                   != spreadsheetengine::api::Error::IllegalArgument
            || aTextWeekendWorkdayIntl
            || aTextWeekendWorkdayIntl.meError
                   != spreadsheetengine::api::Error::IllegalArgument
            || aNumericMaskWorkdayIntl
            || aNumericMaskWorkdayIntl.meError
                   != spreadsheetengine::api::Error::IllegalArgument
            || aCompiledTextWeekendWorkdayIntl
            || aCompiledTextWeekendWorkdayIntl.meError
                   != spreadsheetengine::api::Error::IllegalArgument
            || aCompiledNumericMaskWorkdayIntl
            || aCompiledNumericMaskWorkdayIntl.meError
                   != spreadsheetengine::api::Error::IllegalArgument
            || !checkNumber("compiled YEAR", aCompiledYear, 1899.0)
            || !checkNumber("compiled MONTH", aCompiledMonth, 12.0)
            || !checkNumber("compiled DAY", aCompiledDay, 29.0)
            || !checkNumber("compiled HOUR", aCompiledHour, 17.0)
            || !checkNumber("compiled MINUTE", aCompiledMinute, 30.0)
            || !checkNumber("compiled SECOND", aCompiledSecond, 1.0)
            || !checkNumber("compiled WEEKDAY", aCompiledWeekday, 3.0)
            || !checkNumber("compiled WEEKNUM", aCompiledWeeknum, 29.0)
            || !checkNumber("compiled DAYS360", aCompiledDays360, 32.0)
            || !checkNumber("compiled EASTERSUNDAY", aCompiledEasterSunday, 42099.0)
            || !checkNumber("compiled YEARS", aCompiledYears, 2.0)
            || !checkNumber("compiled WORKDAY", aCompiledWorkday, 41954.0)
            || !checkNumber("compiled NETWORKDAYS", aCompiledNetworkdays, 17.0)
            || !checkNumber("compiled NETWORKDAYS sequence", aCompiledNetworkdaysSequence, 3.0)
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
        if (!aResult || aResult.mbUsedCachedValue || !aResult.maValue.maValue.isNumber()
            || !almostEqual(aResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "fixture SUM() mismatch");
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
        if (!aErr511Result || !aErr511Result.maValue.maValue.isError())
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "Err:511 live error mismatch");
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
        const auto aConvertPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                  / "addin" / "fods" / "convert.fods";
        const auto aConvertAddPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                     / "mathematical" / "fods" / "convert_add.fods";
        const auto aTTestPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                / "statistical" / "fods" / "t.test.fods";
        const auto aAggregatePath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                    / "mathematical" / "fods" / "aggregate.fods";
        const auto aConvertLoad = spreadsheetengine::core::fods::loadWorkbook(aConvertPath.string());
        const auto aConvertAddLoad
            = spreadsheetengine::core::fods::loadWorkbook(aConvertAddPath.string());
        const auto aTTestLoad = spreadsheetengine::core::fods::loadWorkbook(aTTestPath.string());
        const auto aLoadResult = spreadsheetengine::core::fods::loadWorkbook(aAggregatePath.string());
        if (!aConvertLoad)
            return fail("spreadsheetengine_fods_evaluator_tests", "convert.fods load failed");
        if (!aConvertAddLoad)
            return fail("spreadsheetengine_fods_evaluator_tests", "convert_add.fods load failed");
        if (!aTTestLoad)
            return fail("spreadsheetengine_fods_evaluator_tests", "t.test.fods load failed");
        if (!aLoadResult)
            return fail("spreadsheetengine_fods_evaluator_tests", "aggregate.fods load failed");

        Evaluator aConvertEvaluator(aConvertLoad.maValue.maWorkbook);
        const auto aConvertRow150 = aConvertEvaluator.evaluateCell({ 1, 0, 149 });
        const auto aConvertRow150Compiled = aConvertEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 149 });
        const auto aConvertRow151 = aConvertEvaluator.evaluateCell({ 1, 0, 150 });
        const auto aConvertRow155 = aConvertEvaluator.evaluateCell({ 1, 0, 154 });
        const auto aConvertRow156 = aConvertEvaluator.evaluateCell({ 1, 0, 155 });
        if (!aConvertRow150 || aConvertRow150.mbUsedCachedValue
            || !aConvertRow150.maValue.maValue.isNumber()
            || !almostEqual(aConvertRow150.maValue.maValue.mfNumber, 0.00057870370370536402)
            || !aConvertRow150Compiled || aConvertRow150Compiled.mbUsedCachedValue
            || !aConvertRow150Compiled.maValue.maValue.isNumber()
            || !almostEqual(
                aConvertRow150Compiled.maValue.maValue.mfNumber, 0.00057870370370536402)
            || !aConvertRow151 || aConvertRow151.mbUsedCachedValue
            || !aConvertRow151.maValue.maValue.isNumber()
            || !almostEqual(aConvertRow151.maValue.maValue.mfNumber, 0.000160333493666367)
            || !aConvertRow155 || aConvertRow155.mbUsedCachedValue
            || !aConvertRow155.maValue.maValue.isNumber()
            || !almostEqual(aConvertRow155.maValue.maValue.mfNumber, 0.083333333333352799)
            || !aConvertRow156 || aConvertRow156.mbUsedCachedValue
            || !aConvertRow156.maValue.maValue.isNumber()
            || !almostEqual(aConvertRow156.maValue.maValue.mfNumber, 0.00000263046611952801))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "convert.fods cubic-unit mismatch");
        }

        Evaluator aConvertAddEvaluator(aConvertAddLoad.maValue.maWorkbook);
        const auto aConvertAddRow86 = aConvertAddEvaluator.evaluateCell({ 1, 0, 85 });
        const auto aConvertAddRow86Compiled
            = aConvertAddEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 85 });
        const auto checkConvertAddRow86 = [](const auto& rResult) {
            return rResult && !rResult.mbUsedCachedValue && rResult.maValue.maValue.isNumber()
                   && std::abs(rResult.maValue.maValue.mfNumber - 11.1445349270435) <= 1.0e-12;
        };
        if (!checkConvertAddRow86(aConvertAddRow86)
            || !checkConvertAddRow86(aConvertAddRow86Compiled))
        {
            return fail("spreadsheetengine_fods_evaluator_tests",
                "convert_add.fods MTON->Nmi3 mismatch");
        }

        Evaluator aTTestEvaluator(aTTestLoad.maValue.maWorkbook);
        const auto aTTestInvalidResult = aTTestEvaluator.evaluateCell({ 1, 0, 1 });
        if (!aTTestInvalidResult || aTTestInvalidResult.mbUsedCachedValue
            || !aTTestInvalidResult.maValue.maValue.isError()
            || aTTestInvalidResult.maValue.maValue.meError
                   != spreadsheetengine::api::Error::DivisionByZero)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "t.test.fods paired-zero-variance mismatch");
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
        const auto aWeeksInYearPath = aRepoRoot / "sc" / "qa" / "unit" / "data"
                                      / "functions" / "date_time" / "fods"
                                      / "weeksinyear.fods";
        const auto aVlookupPath = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions"
                                  / "spreadsheet" / "fods" / "vlookup.fods";
        const auto aEomonthLoad = spreadsheetengine::core::fods::loadWorkbook(aEomonthPath.string());
        const auto aEdateLoad = spreadsheetengine::core::fods::loadWorkbook(aEdatePath.string());
        const auto aTimeLoad = spreadsheetengine::core::fods::loadWorkbook(aTimePath.string());
        const auto aWeeksLoad = spreadsheetengine::core::fods::loadWorkbook(aWeeksPath.string());
        const auto aWeeksInYearLoad
            = spreadsheetengine::core::fods::loadWorkbook(aWeeksInYearPath.string());
        const auto aVlookupLoad = spreadsheetengine::core::fods::loadWorkbook(aVlookupPath.string());
        if (!aEomonthLoad || !aEdateLoad || !aTimeLoad || !aWeeksLoad || !aWeeksInYearLoad
            || !aVlookupLoad)
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
        const auto aWeeksErrorResult = aWeeksEvaluator.evaluateCell({ 1, 0, 1 });
        const auto aCompiledWeeksResult = aWeeksEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 11 });
        const auto aCompiledWeeksErrorResult
            = aWeeksEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 1 });
        if (!aWeeksResult || aWeeksResult.mbUsedCachedValue
            || !aWeeksResult.maValue.maValue.isNumber()
            || !almostEqual(aWeeksResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_evaluator_tests", "weeks.fods live evaluation mismatch");
        }
        if (!aWeeksErrorResult || aWeeksErrorResult.mbUsedCachedValue
            || !aWeeksErrorResult.maValue.maValue.isError()
            || aWeeksErrorResult.maValue.maValue.meError != spreadsheetengine::api::Error::NoValue)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "weeks.fods live error mismatch");
        }
        if (!aCompiledWeeksResult || aCompiledWeeksResult.mbUsedCachedValue
            || !aCompiledWeeksResult.maValue.maValue.isNumber()
            || !almostEqual(aCompiledWeeksResult.maValue.maValue.mfNumber, 1.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "weeks.fods compiled evaluation mismatch");
        }
        if (!aCompiledWeeksErrorResult || aCompiledWeeksErrorResult.mbUsedCachedValue
            || !aCompiledWeeksErrorResult.maValue.maValue.isError()
            || aCompiledWeeksErrorResult.maValue.maValue.meError
                   != spreadsheetengine::api::Error::NoValue)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "weeks.fods compiled error mismatch");
        }

        Evaluator aWeeksInYearEvaluator(aWeeksInYearLoad.maValue.maWorkbook);
        const auto aWeeksInYearResult = aWeeksInYearEvaluator.evaluateCell({ 1, 0, 1 });
        const auto aWeeksInYearErrorResult = aWeeksInYearEvaluator.evaluateCell({ 1, 0, 4 });
        const auto aCompiledWeeksInYearResult
            = aWeeksInYearEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 1 });
        const auto aCompiledWeeksInYearErrorResult
            = aWeeksInYearEvaluator.evaluateCellViaCompiledTokens({ 1, 0, 4 });
        if (!aWeeksInYearResult || aWeeksInYearResult.mbUsedCachedValue
            || !aWeeksInYearResult.maValue.maValue.isNumber()
            || !almostEqual(aWeeksInYearResult.maValue.maValue.mfNumber, 52.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests",
                "weeksinyear.fods live evaluation mismatch");
        }
        if (!aWeeksInYearErrorResult || aWeeksInYearErrorResult.mbUsedCachedValue
            || !aWeeksInYearErrorResult.maValue.maValue.isError()
            || aWeeksInYearErrorResult.maValue.maValue.meError
                   != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests", "weeksinyear.fods live error mismatch");
        }
        if (!aCompiledWeeksInYearResult || aCompiledWeeksInYearResult.mbUsedCachedValue
            || !aCompiledWeeksInYearResult.maValue.maValue.isNumber()
            || !almostEqual(aCompiledWeeksInYearResult.maValue.maValue.mfNumber, 52.0))
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests",
                "weeksinyear.fods compiled evaluation mismatch");
        }
        if (!aCompiledWeeksInYearErrorResult || aCompiledWeeksInYearErrorResult.mbUsedCachedValue
            || !aCompiledWeeksInYearErrorResult.maValue.maValue.isError()
            || aCompiledWeeksInYearErrorResult.maValue.maValue.meError
                   != spreadsheetengine::api::Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_fods_evaluator_tests",
                "weeksinyear.fods compiled error mismatch");
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
