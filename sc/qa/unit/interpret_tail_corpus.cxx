/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helper/qahelper.hxx"

#include <docoptio.hxx>
#include <formula/errorcodes.hxx>
#include <formula/grammar.hxx>
#include <rangenam.hxx>
#include <scopetools.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace
{

using spreadsheetengine::compat::libreoffice::toFormulaError;
using spreadsheetengine::compat::libreoffice::toApiString;
using spreadsheetengine::compat::libreoffice::toLibreOfficeString;
using spreadsheetengine::compat::libreoffice::interprettaileval::FallbackReason;
using spreadsheetengine::compat::libreoffice::interprettaileval::FunctionKind;
using spreadsheetengine::compat::libreoffice::interprettaileval::StatsSnapshot;
using spreadsheetengine::core::fods::loadWorkbook;
using spreadsheetengine::core::workbook::Cell;
using spreadsheetengine::core::workbook::FormulaSearchType;
using spreadsheetengine::core::workbook::NamedRange;
using spreadsheetengine::core::workbook::Sheet;
using spreadsheetengine::core::workbook::Workbook;

class ScopedEnvironmentOverride
{
    std::string maName;
    std::optional<std::string> moOriginalValue;

public:
    ScopedEnvironmentOverride(const char* pName, const char* pValue)
        : maName(pName)
    {
        if (const char* pOriginal = std::getenv(pName))
            moOriginalValue = pOriginal;

        if (pValue)
            setenv(maName.c_str(), pValue, 1);
        else
            unsetenv(maName.c_str());
    }

    ~ScopedEnvironmentOverride()
    {
        if (moOriginalValue)
            setenv(maName.c_str(), moOriginalValue->c_str(), 1);
        else
            unsetenv(maName.c_str());
    }
};

class TestInterpretTailCorpus : public ScUcalcTestBase
{
};

bool envEnabled(const char* pName)
{
    const char* pValue = std::getenv(pName);
    if (!pValue)
        return false;

    const std::string aValue(pValue);
    return !aValue.empty() && aValue != "0" && aValue != "off" && aValue != "false";
}

std::vector<std::filesystem::path> collectFodsFiles(const std::filesystem::path& rPath)
{
    std::vector<std::filesystem::path> aFiles;

    if (std::filesystem::is_regular_file(rPath))
    {
        if (rPath.extension() == ".fods")
            aFiles.push_back(rPath);
        return aFiles;
    }

    if (!std::filesystem::is_directory(rPath))
        return aFiles;

    for (const auto& rEntry : std::filesystem::directory_iterator(rPath))
    {
        if (rEntry.is_regular_file() && rEntry.path().extension() == ".fods")
            aFiles.push_back(rEntry.path());
    }

    std::sort(aFiles.begin(), aFiles.end());
    return aFiles;
}

std::vector<std::filesystem::path> collectDefaultReplayCorpus()
{
    const std::filesystem::path aRepoRoot
        = std::filesystem::path(SPREADSHEETENGINE_TEST_ROOT).parent_path();

    std::vector<std::filesystem::path> aFiles
        = collectFodsFiles(aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "logical"
                           / "fods");

    const std::filesystem::path aMathRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "mathematical" / "fods";
    const auto aMathFiles = collectFodsFiles(aMathRoot);
    aFiles.insert(aFiles.end(), aMathFiles.begin(), aMathFiles.end());

    const std::filesystem::path aTextRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "text" / "fods";
    const auto aTextFiles = collectFodsFiles(aTextRoot);
    aFiles.insert(aFiles.end(), aTextFiles.begin(), aTextFiles.end());

    const std::filesystem::path aDateTimeRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "date_time" / "fods";
    const auto aDateTimeFiles = collectFodsFiles(aDateTimeRoot);
    aFiles.insert(aFiles.end(), aDateTimeFiles.begin(), aDateTimeFiles.end());

    const std::filesystem::path aSpreadsheetRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "spreadsheet" / "fods";
    const auto aSpreadsheetFiles = collectFodsFiles(aSpreadsheetRoot);
    aFiles.insert(aFiles.end(), aSpreadsheetFiles.begin(), aSpreadsheetFiles.end());

    const std::filesystem::path aInformationRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "information" / "fods";
    const auto aInformationFiles = collectFodsFiles(aInformationRoot);
    aFiles.insert(aFiles.end(), aInformationFiles.begin(), aInformationFiles.end());

    const std::filesystem::path aFinancialRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "financial" / "fods";
    const auto aFinancialFiles = collectFodsFiles(aFinancialRoot);
    aFiles.insert(aFiles.end(), aFinancialFiles.begin(), aFinancialFiles.end());

    const std::filesystem::path aStatisticalRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "statistical" / "fods";
    const auto aStatisticalFiles = collectFodsFiles(aStatisticalRoot);
    aFiles.insert(aFiles.end(), aStatisticalFiles.begin(), aStatisticalFiles.end());

    const std::filesystem::path aAddinRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "addin" / "fods";
    const auto aAddinFiles = collectFodsFiles(aAddinRoot);
    aFiles.insert(aFiles.end(), aAddinFiles.begin(), aAddinFiles.end());

    const std::filesystem::path aArrayRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "array" / "fods";
    const auto aArrayFiles = collectFodsFiles(aArrayRoot);
    aFiles.insert(aFiles.end(), aArrayFiles.begin(), aArrayFiles.end());

    const std::filesystem::path aDatabaseRoot
        = aRepoRoot / "sc" / "qa" / "unit" / "data" / "functions" / "database" / "fods";
    const auto aDatabaseFiles = collectFodsFiles(aDatabaseRoot);
    aFiles.insert(aFiles.end(), aDatabaseFiles.begin(), aDatabaseFiles.end());

    return aFiles;
}

void applyWorkbookSearchSettings(ScDocument& rDoc, const Workbook& rWorkbook)
{
    ScDocOptions aOptions(rDoc.GetDocOptions());
    aOptions.SetFormulaRegexEnabled(false);
    aOptions.SetFormulaWildcardsEnabled(false);

    switch (rWorkbook.meFormulaSearchType)
    {
        case FormulaSearchType::Regex:
            aOptions.SetFormulaRegexEnabled(true);
            break;
        case FormulaSearchType::Wildcard:
            aOptions.SetFormulaWildcardsEnabled(true);
            break;
        case FormulaSearchType::Normal:
            break;
    }

    aOptions.SetMatchWholeCell(rWorkbook.mbSearchCriteriaMustApplyToWholeCell);
    rDoc.SetDocOptions(aOptions);
    rDoc.SetStorageGrammar(formula::FormulaGrammar::GRAM_ODFF);
    rDoc.SetGrammar(formula::FormulaGrammar::GRAM_ODFF);
}

void prepareSheetLayout(ScDocument& rDoc, const Workbook& rWorkbook, std::string_view rWorkbookLabel)
{
    CPPUNIT_ASSERT_MESSAGE("replay workbook must have at least one sheet",
        !rWorkbook.maSheets.empty());

    while (rDoc.GetTableCount() > 1)
        CPPUNIT_ASSERT(rDoc.DeleteTab(rDoc.GetTableCount() - 1));

    const OUString aFirstSheetName = toLibreOfficeString(rWorkbook.maSheets.front().maName);
    if (rDoc.GetTableCount() == 0)
    {
        CPPUNIT_ASSERT_MESSAGE(
            ("failed to insert first sheet for " + std::string(rWorkbookLabel) + ": "
             + aFirstSheetName.toUtf8().getStr())
                .c_str(),
            rDoc.InsertTab(0, aFirstSheetName));
    }
    else
    {
        OUString aCurrentName;
        CPPUNIT_ASSERT(rDoc.GetName(0, aCurrentName));
        if (aCurrentName != aFirstSheetName)
        {
            CPPUNIT_ASSERT_MESSAGE(
                ("failed to rename first sheet for " + std::string(rWorkbookLabel) + ": "
                 + aFirstSheetName.toUtf8().getStr())
                    .c_str(),
                rDoc.RenameTab(0, aFirstSheetName));
        }
    }

    for (std::size_t nIndex = 1; nIndex < rWorkbook.maSheets.size(); ++nIndex)
    {
        const OUString aSheetName = toLibreOfficeString(rWorkbook.maSheets[nIndex].maName);
        CPPUNIT_ASSERT_MESSAGE(
            ("failed to insert sheet for " + std::string(rWorkbookLabel) + ": "
             + aSheetName.toUtf8().getStr())
                .c_str(),
            rDoc.InsertTab(static_cast<SCTAB>(nIndex), aSheetName));
    }
}

ScAddress parseBaseAddress(const Workbook& rWorkbook, ScDocument& rDoc, const NamedRange& rNamedRange)
{
    SCTAB nScopeTab = 0;
    if (!rNamedRange.maScopeSheetName.empty())
    {
        const auto oScopeSheet = rWorkbook.findSheetId(rNamedRange.maScopeSheetName);
        CPPUNIT_ASSERT_MESSAGE("named-range scope sheet missing", oScopeSheet.has_value());
        nScopeTab = static_cast<SCTAB>(*oScopeSheet);
    }

    ScAddress aBase(0, 0, nScopeTab);
    if (rNamedRange.maBaseCellAddress.empty())
        return aBase;

    ScAddress aParsed;
    const auto nFlags
        = aParsed.Parse(toLibreOfficeString(rNamedRange.maBaseCellAddress), rDoc);
    CPPUNIT_ASSERT_MESSAGE("named-range base address parse failed",
        (nFlags & ScRefFlags::VALID) == ScRefFlags::VALID);
    return aParsed;
}

void materializeNamedRanges(const Workbook& rWorkbook, ScDocument& rDoc)
{
    for (const auto& rNamedRange : rWorkbook.maNamedRanges)
    {
        const ScAddress aBase = parseBaseAddress(rWorkbook, rDoc, rNamedRange);
        ScRangeName* pNames = nullptr;
        if (rNamedRange.isGlobal())
            pNames = rDoc.GetRangeName();
        else
        {
            const auto oScopeSheet = rWorkbook.findSheetId(rNamedRange.maScopeSheetName);
            CPPUNIT_ASSERT_MESSAGE("named-range scope sheet missing", oScopeSheet.has_value());
            const SCTAB nScopeTab = static_cast<SCTAB>(*oScopeSheet);
            pNames = rDoc.GetRangeName(nScopeTab);
            if (!pNames)
            {
                rDoc.SetRangeName(nScopeTab, std::make_unique<ScRangeName>());
                pNames = rDoc.GetRangeName(nScopeTab);
            }
        }

        CPPUNIT_ASSERT(pNames);
        auto pData = new ScRangeData(rDoc, toLibreOfficeString(rNamedRange.maName),
            toLibreOfficeString(rNamedRange.maCellRangeAddress), aBase, ScRangeData::Type::Name,
            formula::FormulaGrammar::GRAM_ODFF);
        CPPUNIT_ASSERT_MESSAGE("named-range insertion failed", pNames->insert(pData));
    }
}

void materializeScalarCell(ScDocument& rDoc, const ScAddress& rPos, const Cell& rCell)
{
    const auto& rValue = rCell.maValue;
    if (rValue.isEmpty())
        return;

    if (rValue.isNumber() || rValue.isBoolean())
    {
        rDoc.SetValue(rPos, rValue.mfNumber);
        return;
    }

    if (rValue.isText())
    {
        rDoc.SetTextCell(rPos, toLibreOfficeString(rValue.maString));
        return;
    }

    if (rValue.isError())
        rDoc.SetError(rPos.Col(), rPos.Row(), rPos.Tab(), toFormulaError(rValue.meError));
}

void materializeSheetCells(const Sheet& rSheet, SCTAB nTab, ScDocument& rDoc)
{
    for (const auto& rEntry : rSheet.maCells)
    {
        const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
            static_cast<SCROW>(rEntry.first.second), nTab);
        const Cell& rCell = rEntry.second;
        if (!rCell.hasFormula())
            materializeScalarCell(rDoc, aPos, rCell);
    }

    for (const auto& rEntry : rSheet.maCells)
    {
        const ScAddress aPos(static_cast<SCCOL>(rEntry.first.first),
            static_cast<SCROW>(rEntry.first.second), nTab);
        const Cell& rCell = rEntry.second;
        if (rCell.hasFormula())
        {
            rDoc.SetFormula(aPos, toLibreOfficeString(rCell.maFormula),
                formula::FormulaGrammar::GRAM_ODFF);
        }
    }
}

std::size_t materializeWorkbookToCalc(
    const Workbook& rWorkbook, ScDocument& rDoc, std::string_view rWorkbookLabel)
{
    applyWorkbookSearchSettings(rDoc, rWorkbook);
    prepareSheetLayout(rDoc, rWorkbook, rWorkbookLabel);

    sc::AutoCalcSwitch aPopulateSwitch(rDoc, false);
    materializeNamedRanges(rWorkbook, rDoc);

    std::size_t nFormulaCellCount = 0;
    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        materializeSheetCells(rWorkbook.maSheets[nSheet], static_cast<SCTAB>(nSheet), rDoc);
        for (const auto& rEntry : rWorkbook.maSheets[nSheet].maCells)
        {
            if (rEntry.second.hasFormula())
                ++nFormulaCellCount;
        }
    }

    return nFormulaCellCount;
}

const char* functionKindName(FunctionKind eFunction)
{
    switch (eFunction)
    {
        case FunctionKind::Unknown:
            return "unknown";
        case FunctionKind::Value:
            return "value";
        case FunctionKind::DateValue:
            return "datevalue";
        case FunctionKind::TimeValue:
            return "timevalue";
        case FunctionKind::NumberValue:
            return "numbervalue";
        case FunctionKind::Match:
            return "match";
        case FunctionKind::XMatch:
            return "xmatch";
        case FunctionKind::Lookup:
            return "lookup";
        case FunctionKind::VLookup:
            return "vlookup";
        case FunctionKind::HLookup:
            return "hlookup";
        case FunctionKind::Index:
            return "index";
        case FunctionKind::Count:
            break;
    }

    return "count";
}

const char* fallbackReasonName(FallbackReason eReason)
{
    switch (eReason)
    {
        case FallbackReason::UnsupportedTailContext:
            return "unsupported_tail_context";
        case FallbackReason::UnsupportedFormulaShape:
            return "unsupported_formula_shape";
        case FallbackReason::UnsupportedHostSurface:
            return "unsupported_host_surface";
        case FallbackReason::UnsupportedFunction:
            return "unsupported_function";
        case FallbackReason::ParseFailure:
            return "parse_failure";
        case FallbackReason::ProjectionFailure:
            return "projection_failure";
        case FallbackReason::ShadowMismatch:
            return "shadow_mismatch";
        case FallbackReason::Count:
            break;
    }

    return "count";
}

void printStats(std::size_t nWorkbookCount, std::size_t nFormulaCellCount, const StatsSnapshot& rStats)
{
    std::cout << "interpret_tail_corpus_workbooks=" << nWorkbookCount << '\n';
    std::cout << "interpret_tail_corpus_formula_cells=" << nFormulaCellCount << '\n';
    std::cout << "interpret_tail_observe_total=" << rStats.mnObserveCount << '\n';
    std::cout << "interpret_tail_shadow_compare_total=" << rStats.mnShadowCompareCount << '\n';
    std::cout << "interpret_tail_authoritative_total=" << rStats.mnAuthoritativeCount << '\n';
    std::cout << "interpret_tail_authoritative_fallback_total="
              << rStats.mnAuthoritativeFallbackCount << '\n';
    std::cout << "interpret_tail_shadow_match_total=" << rStats.mnShadowMatchCount << '\n';

    const auto aOldFlags = std::cout.flags();
    const auto nOldPrecision = std::cout.precision();
    std::cout << std::fixed << std::setprecision(2);
    for (std::size_t nIndex = 1; nIndex < static_cast<std::size_t>(FunctionKind::Count); ++nIndex)
    {
        const auto eFunction = static_cast<FunctionKind>(nIndex);
        const sal_uInt64 nAuthoritative = rStats.maFunctionAuthoritativeCount[nIndex];
        const sal_uInt64 nFallback = rStats.maFunctionFallbackCount[nIndex];
        const sal_uInt64 nAttempts = nAuthoritative + nFallback;
        const double fSupportRate
            = nAttempts ? (static_cast<double>(nAuthoritative) * 100.0 / nAttempts) : 0.0;
        const char* pName = functionKindName(eFunction);
        std::cout << "interpret_tail_function_" << pName << "_authoritative=" << nAuthoritative
                  << '\n';
        std::cout << "interpret_tail_function_" << pName << "_fallback=" << nFallback << '\n';
        std::cout << "interpret_tail_function_" << pName << "_attempts=" << nAttempts << '\n';
        std::cout << "interpret_tail_function_" << pName << "_support_rate=" << fSupportRate
                  << '\n';
    }
    std::cout.flags(aOldFlags);
    std::cout.precision(nOldPrecision);

    for (std::size_t nIndex = 0; nIndex < static_cast<std::size_t>(FallbackReason::Count); ++nIndex)
    {
        const auto eReason = static_cast<FallbackReason>(nIndex);
        std::cout << "interpret_tail_fallback_" << fallbackReasonName(eReason)
                  << "=" << rStats.maFallbackReasons[nIndex] << '\n';
    }
}

OUString makeSafeSheetName(std::size_t nSheetIndex)
{
    return u"ReplaySheet"_ustr + OUString::number(static_cast<sal_Int32>(nSheetIndex + 1));
}

void replaceSheetNameReferences(
    spreadsheetengine::api::String& rText,
    const std::vector<std::pair<OUString, OUString>>& rRenameMap)
{
    if (rText.empty() || rRenameMap.empty())
        return;

    OUString aText = toLibreOfficeString(rText);
    for (const auto& rRename : rRenameMap)
        aText = aText.replaceAll(rRename.first, rRename.second);
    rText = toApiString(aText);
}

void normalizeWorkbookSheetNamesForCalc(Workbook& rWorkbook)
{
    std::vector<std::pair<OUString, OUString>> aRenameMap;
    std::vector<OUString> aTakenNames;
    aTakenNames.reserve(rWorkbook.maSheets.size());

    for (std::size_t nSheet = 0; nSheet < rWorkbook.maSheets.size(); ++nSheet)
    {
        const OUString aOriginalName = toLibreOfficeString(rWorkbook.maSheets[nSheet].maName);
        OUString aCandidate = aOriginalName;

        auto hasTakenName = [&aTakenNames](const OUString& rName) {
            return std::find(aTakenNames.begin(), aTakenNames.end(), rName) != aTakenNames.end();
        };

        if (!ScDocument::ValidTabName(aCandidate) || hasTakenName(aCandidate))
        {
            aCandidate = makeSafeSheetName(nSheet);
            while (!ScDocument::ValidTabName(aCandidate) || hasTakenName(aCandidate))
                aCandidate += u"X"_ustr;
        }

        aTakenNames.push_back(aCandidate);
        if (aCandidate != aOriginalName)
        {
            aRenameMap.emplace_back(aOriginalName, aCandidate);
            rWorkbook.maSheets[nSheet].maName = toApiString(aCandidate);
        }
    }

    if (aRenameMap.empty())
        return;

    for (auto& rNamedRange : rWorkbook.maNamedRanges)
    {
        replaceSheetNameReferences(rNamedRange.maBaseCellAddress, aRenameMap);
        replaceSheetNameReferences(rNamedRange.maCellRangeAddress, aRenameMap);
        for (const auto& rRename : aRenameMap)
        {
            if (rNamedRange.maScopeSheetName == toApiString(rRename.first))
            {
                rNamedRange.maScopeSheetName = toApiString(rRename.second);
                break;
            }
        }
    }

    for (auto& rSheet : rWorkbook.maSheets)
    {
        if (rSheet.moSource)
            replaceSheetNameReferences(rSheet.moSource->maTableName, aRenameMap);

        for (auto& rEntry : rSheet.maCells)
        {
            replaceSheetNameReferences(rEntry.second.maFormula, aRenameMap);
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestInterpretTailCorpus, testAuthorityStats)
{
    if (!envEnabled("SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS"))
        return;

    const auto aCorpus = collectDefaultReplayCorpus();
    CPPUNIT_ASSERT_MESSAGE("replay corpus should not be empty", !aCorpus.empty());

    ScopedEnvironmentOverride aModeOverride(
        "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR", "authority");
    spreadsheetengine::compat::libreoffice::interprettaileval::resetStats();

    std::size_t nWorkbookCount = 0;
    std::size_t nFormulaCellCount = 0;
    for (const auto& rWorkbookPath : aCorpus)
    {
        const auto aLoadResult = loadWorkbook(rWorkbookPath.string());
        CPPUNIT_ASSERT_MESSAGE(("loadWorkbook failed for " + rWorkbookPath.string()).c_str(),
            static_cast<bool>(aLoadResult));
        Workbook aWorkbook = aLoadResult.maValue.maWorkbook;
        normalizeWorkbookSheetNamesForCalc(aWorkbook);

        ScDocShellRef xDocShell
            = new ScDocShell(SfxModelFlags::EMBEDDED_OBJECT | SfxModelFlags::DISABLE_EMBEDDED_SCRIPTS
                             | SfxModelFlags::DISABLE_DOCUMENT_RECOVERY);
        xDocShell->DoInitUnitTest();
        ScDocument& rDoc = xDocShell->GetDocument();

        nFormulaCellCount += materializeWorkbookToCalc(
            aWorkbook, rDoc, rWorkbookPath.string());

        {
            sc::AutoCalcSwitch aCalcSwitch(rDoc, true);
            rDoc.CalcAll();
        }

        xDocShell->DoClose();
        ++nWorkbookCount;
    }

    const auto aStats
        = spreadsheetengine::compat::libreoffice::interprettaileval::getStatsSnapshot();
    printStats(nWorkbookCount, nFormulaCellCount, aStats);

    CPPUNIT_ASSERT_EQUAL(aCorpus.size(), nWorkbookCount);
}

} // namespace

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
