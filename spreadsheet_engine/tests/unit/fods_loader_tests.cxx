/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <filesystem>
#include <iostream>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::core::fods::loadWorkbook;
    using spreadsheetengine::standalone::test::fail;

    const auto aWorkbookPath = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT)
                               / "tests" / "data" / "fods" / "minimal_workbook.fods";

    const auto aLoadResult = loadWorkbook(aWorkbookPath.string());
    if (!aLoadResult)
        return fail("spreadsheetengine_fods_tests", "loadWorkbook() failed");

    const auto& rWorkbook = aLoadResult.maValue.maWorkbook;
    const auto& rIgnored = aLoadResult.maValue.maIgnoredFeatures;

    if (rIgnored.mnChartElementCount != 1 || rIgnored.mnDrawElementCount != 1
        || rIgnored.mnContentValidationCount != 1)
    {
        return fail("spreadsheetengine_fods_tests", "ignored feature counting mismatch");
    }

    if (rWorkbook.maSheets.size() != 2 || rWorkbook.maNamedRanges.size() != 2)
        return fail("spreadsheetengine_fods_tests", "workbook structure mismatch");

    const auto* pSheet1 = rWorkbook.findSheet(u"Sheet1");
    const auto* pImported = rWorkbook.findSheet(u"ImportedResults");
    if (!pSheet1 || !pImported || !pImported->moSource
        || !pImported->moSource->isCopyResultsOnly()
        || pImported->moSource->maHref != u"linked_source.fods"
        || pImported->moSource->maTableName != u"SourceSheet")
    {
        return fail("spreadsheetengine_fods_tests", "sheet source parsing mismatch");
    }

    const auto* pImportedA1 = pImported->findCell(0, 0);
    if (!pImportedA1 || !pImportedA1->maValue.isText() || pImportedA1->maValue.maString != u"source")
        return fail("spreadsheetengine_fods_tests", "imported sheet resolution mismatch");

    const auto* pGlobalRange = rWorkbook.findNamedRange(u"GlobalRange");
    const auto* pLocalRange = rWorkbook.findNamedRange(u"LocalRange", u"Sheet1");
    if (!pGlobalRange || !pGlobalRange->isGlobal()
        || pGlobalRange->maCellRangeAddress != u"$Sheet1.$A$1:.$B$2" || !pLocalRange
        || pLocalRange->maCellRangeAddress != u"$Sheet1.$C$1:.$C$2")
    {
        return fail("spreadsheetengine_fods_tests", "named range parsing mismatch");
    }

    const auto* pA1 = pSheet1->findCell(0, 0);
    const auto* pB1 = pSheet1->findCell(1, 0);
    const auto* pC1 = pSheet1->findCell(2, 0);
    const auto* pD1 = pSheet1->findCell(3, 0);
    const auto* pE1 = pSheet1->findCell(4, 0);
    const auto* pF1 = pSheet1->findCell(5, 0);
    const auto* pA2 = pSheet1->findCell(0, 1);
    const auto* pA3 = pSheet1->findCell(0, 2);
    const auto* pCovered = pSheet1->findCell(1, 1);
    const auto* pG1 = pSheet1->findCell(6, 0);
    const auto* pH1 = pSheet1->findCell(7, 0);
    const auto* pI1 = pSheet1->findCell(8, 0);
    const auto* pJ1 = pSheet1->findCell(9, 0);
    const auto* pK1 = pSheet1->findCell(10, 0);
    const auto* pL1 = pSheet1->findCell(11, 0);

    if (!pA1 || !pA1->maValue.isNumber() || pA1->maValue.mfNumber != 1.0 || !pB1
        || !pB1->maValue.isText() || pB1->maValue.maString != u"hello" || !pC1
        || !pC1->maValue.isBoolean() || pC1->maValue.mfNumber != 1.0 || !pD1
        || !pD1->maValue.isBoolean() || !pE1 || !pE1->hasFormula()
        || pE1->maFormula != u"of:=SUM([.A1:.A1])" || !pE1->maValue.isNumber()
        || pE1->maValue.mfNumber != 1.0 || !pF1 || !pF1->maValue.isError()
        || pF1->maValue.meError != Error::NotAvailable || !pG1 || !pG1->maValue.isError()
        || pG1->maValue.meError != Error::NotAvailable || !pH1 || !pH1->maValue.isError()
        || !pI1 || !pI1->maValue.isText() || pI1->maValue.maString != u"\u00A0"
        || !pJ1 || !pJ1->maValue.isNumber() || pJ1->maValue.mfNumber != 2.5
        || !pK1 || !pK1->maValue.isText() || pK1->maValue.maString != u"  lead"
        || !pL1 || !pL1->maValue.isText() || pL1->maValue.maString != u"a\tb"
        || pSheet1->isRowHidden(1) || !pSheet1->isRowHidden(2)
        || !pA2 || !pA3
        || pA2->maValue.maString != u"rep" || pA3->maValue.maString != u"rep"
        || !pCovered || !pCovered->mbCovered)
    {
        return fail("spreadsheetengine_fods_tests", "cell parsing mismatch");
    }

    {
        const auto aRelativeWorkbookPath = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT)
                                           / "tests" / "data" / "fods" / "subdir"
                                           / "relative_import_workbook.fods";
        const auto aRelativeLoadResult = loadWorkbook(aRelativeWorkbookPath.string());
        if (!aRelativeLoadResult)
            return fail("spreadsheetengine_fods_tests", "relative import workbook load failed");

        const auto* pRelativeImported
            = aRelativeLoadResult.maValue.maWorkbook.findSheet(u"ImportedResults");
        const auto* pRelativeA1 = pRelativeImported ? pRelativeImported->findCell(0, 0) : nullptr;
        if (!pRelativeA1 || !pRelativeA1->maValue.isText()
            || pRelativeA1->maValue.maString != u"source")
        {
            return fail("spreadsheetengine_fods_tests", "parent relative import mismatch");
        }
    }

    {
        const auto aPrefixedWorkbookPath = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT)
                                           / "tests" / "data" / "fods"
                                           / "prefixed_import_workbook.fods";
        const auto aPrefixedLoadResult = loadWorkbook(aPrefixedWorkbookPath.string());
        if (!aPrefixedLoadResult)
            return fail("spreadsheetengine_fods_tests", "prefixed import workbook load failed");

        const auto* pPrefixedImported
            = aPrefixedLoadResult.maValue.maWorkbook.findSheet(u"ImportedResults");
        const auto* pPrefixedA1 = pPrefixedImported ? pPrefixedImported->findCell(0, 0) : nullptr;
        if (!pPrefixedA1 || !pPrefixedA1->maValue.isText()
            || pPrefixedA1->maValue.maString != u"source")
        {
            return fail("spreadsheetengine_fods_tests", "trimmed import fallback mismatch");
        }
    }

    std::cout << "spreadsheetengine FODS loader tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
