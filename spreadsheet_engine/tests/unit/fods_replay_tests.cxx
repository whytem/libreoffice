/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <map>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <spreadsheetengine/detail/FodsEvaluator.hxx>
#include <spreadsheetengine/detail/FodsCompilerPreflight.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>
#include <spreadsheetengine/detail/OdfFormulaParser.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::api::CellAddress;
using spreadsheetengine::api::CellValue;
using spreadsheetengine::api::ColumnIndex;
using spreadsheetengine::api::Error;
using spreadsheetengine::api::RowIndex;
using spreadsheetengine::api::SheetId;
using spreadsheetengine::api::String;
using spreadsheetengine::api::StringView;
using spreadsheetengine::detail::compiler::FormulaPreflightReason;
using spreadsheetengine::detail::compiler::preflightReasonName;
using spreadsheetengine::core::fods::EvaluationResult;
using spreadsheetengine::core::fods::Evaluator;
using spreadsheetengine::core::formula::Node;
using spreadsheetengine::core::formula::NodeKind;
using spreadsheetengine::core::fods::loadWorkbook;
using spreadsheetengine::core::workbook::Sheet;
using spreadsheetengine::core::workbook::Workbook;

constexpr ColumnIndex SUCCESS_COLUMN = 1; // B
constexpr RowIndex SUCCESS_ROW = 2; // 3rd row

struct ReplaySummary
{
    std::size_t mnWorkbooks = 0;
    std::size_t mnFormulaCells = 0;
    std::size_t mnParsedFormulas = 0;
    std::size_t mnCachedFallbackCells = 0;
    std::size_t mnCellReferenceNodes = 0;
    std::size_t mnRangeReferenceNodes = 0;
    std::size_t mnNamedReferenceNodes = 0;
    std::size_t mnArrayConstantNodes = 0;
    std::size_t mnFunctionCallNodes = 0;
    std::size_t mnUnaryOperationNodes = 0;
    std::size_t mnBinaryOperationNodes = 0;
    std::size_t mnLiteralNodes = 0;
    std::map<std::string, std::size_t> maFamilyWorkbookCounts;
    std::map<std::string, std::size_t> maFunctionCallCounts;
};

struct PreflightSummary
{
    std::size_t mnFormulaCells = 0;
    std::size_t mnReady = 0;
    std::size_t mnExpectedError = 0;
    std::map<std::string, std::size_t> maReasonCounts;
    std::map<std::string, std::string> maReasonExamples;
    std::map<std::string, std::size_t> maExpectedErrorReasonCounts;
    std::map<std::string, std::string> maExpectedErrorReasonExamples;
};

std::string toUtf8(StringView rText)
{
    std::string aUtf8;
    aUtf8.reserve(rText.size());
    for (std::size_t nIndex = 0; nIndex < rText.size(); ++nIndex)
    {
        char32_t nCodePoint = rText[nIndex];
        if (0xD800 <= nCodePoint && nCodePoint <= 0xDBFF && nIndex + 1 < rText.size())
        {
            const char32_t nTrail = rText[nIndex + 1];
            if (0xDC00 <= nTrail && nTrail <= 0xDFFF)
            {
                nCodePoint = 0x10000 + ((nCodePoint - 0xD800) << 10) + (nTrail - 0xDC00);
                ++nIndex;
            }
        }

        if (nCodePoint <= 0x7F)
            aUtf8.push_back(static_cast<char>(nCodePoint));
        else if (nCodePoint <= 0x7FF)
        {
            aUtf8.push_back(static_cast<char>(0xC0 | (nCodePoint >> 6)));
            aUtf8.push_back(static_cast<char>(0x80 | (nCodePoint & 0x3F)));
        }
        else if (nCodePoint <= 0xFFFF)
        {
            aUtf8.push_back(static_cast<char>(0xE0 | (nCodePoint >> 12)));
            aUtf8.push_back(static_cast<char>(0x80 | ((nCodePoint >> 6) & 0x3F)));
            aUtf8.push_back(static_cast<char>(0x80 | (nCodePoint & 0x3F)));
        }
        else
        {
            aUtf8.push_back(static_cast<char>(0xF0 | (nCodePoint >> 18)));
            aUtf8.push_back(static_cast<char>(0x80 | ((nCodePoint >> 12) & 0x3F)));
            aUtf8.push_back(static_cast<char>(0x80 | ((nCodePoint >> 6) & 0x3F)));
            aUtf8.push_back(static_cast<char>(0x80 | (nCodePoint & 0x3F)));
        }
    }

    return aUtf8;
}

std::string formatNumber(double fValue)
{
    char aBuffer[32];
    const int nLength = std::snprintf(aBuffer, sizeof(aBuffer), "%.17G", fValue);
    return std::string(aBuffer, static_cast<std::size_t>(std::max(nLength, 0)));
}

std::string formatError(Error eError)
{
    switch (eError)
    {
        case Error::None:
            return "";
        case Error::IllegalArgument:
        case Error::NoValue:
            return "#VALUE!";
        case Error::DivisionByZero:
            return "#DIV/0!";
        case Error::Domain:
        case Error::NoConvergence:
            return "#NUM!";
        case Error::StringOverflow:
            return "#STRING!";
        case Error::NotAvailable:
            return "#N/A";
    }

    return "#ERROR!";
}

std::string formatScalarValue(const CellValue& rValue)
{
    if (rValue.isEmpty())
        return "";
    if (rValue.isNumber() || rValue.isBoolean())
        return formatNumber(rValue.mfNumber);
    if (rValue.isText())
        return toUtf8(rValue.maString);
    if (rValue.isError())
        return formatError(rValue.meError);
    return "";
}

bool cellCachesError(const spreadsheetengine::core::workbook::Cell& rCell)
{
    return rCell.maValue.isError() || rCell.maRawValueType == u"error";
}

std::string preflightBucketName(
    FormulaPreflightReason eReason, bool bExpectedErrorPath)
{
    std::string aReason = toUtf8(preflightReasonName(eReason));
    if (bExpectedErrorPath)
        aReason = "expected_error_" + aReason;
    return aReason;
}

bool isSuccessLike(const EvaluationResult& rResult)
{
    if (!rResult || !rResult.maValue.isScalar())
        return false;

    const CellValue& rValue = rResult.maValue.maValue;
    if (rValue.isBoolean())
        return rValue.mfNumber != 0.0;
    if (rValue.isNumber())
        return spreadsheetengine::standalone::test::almostEqual(rValue.mfNumber, 1.0);
    if (rValue.isText())
        return rValue.maString == u"1" || rValue.maString == u"TRUE";
    return false;
}

std::string columnLabel(ColumnIndex nColumn)
{
    std::string aLabel;
    ColumnIndex nValue = nColumn;
    do
    {
        const int nRemainder = nValue % 26;
        aLabel.insert(aLabel.begin(), static_cast<char>('A' + nRemainder));
        nValue = (nValue / 26) - 1;
    } while (nValue >= 0);
    return aLabel;
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

    return aFiles;
}

std::string familyNameForWorkbook(const std::filesystem::path& rWorkbookPath)
{
    const auto aFodsDir = rWorkbookPath.parent_path();
    if (aFodsDir.filename() != "fods")
        return "custom";

    const auto aFamilyDir = aFodsDir.parent_path();
    if (aFamilyDir.empty())
        return "custom";

    return aFamilyDir.filename().string();
}

void accumulateFormulaNodeSummary(const Node& rNode, ReplaySummary& rSummary)
{
    switch (rNode.meKind)
    {
        case NodeKind::NumberLiteral:
        case NodeKind::StringLiteral:
        case NodeKind::BooleanLiteral:
        case NodeKind::ErrorLiteral:
        case NodeKind::EmptyArgument:
            ++rSummary.mnLiteralNodes;
            break;
        case NodeKind::CellReference:
            ++rSummary.mnCellReferenceNodes;
            break;
        case NodeKind::RangeReference:
            ++rSummary.mnRangeReferenceNodes;
            break;
        case NodeKind::NamedReference:
            ++rSummary.mnNamedReferenceNodes;
            break;
        case NodeKind::ArrayConstant:
            ++rSummary.mnArrayConstantNodes;
            break;
        case NodeKind::UnaryOperation:
            ++rSummary.mnUnaryOperationNodes;
            break;
        case NodeKind::BinaryOperation:
            ++rSummary.mnBinaryOperationNodes;
            break;
        case NodeKind::FunctionCall:
            ++rSummary.mnFunctionCallNodes;
            ++rSummary.maFunctionCallCounts[toUtf8(rNode.maPrimaryText)];
            break;
    }

    for (const auto& pChild : rNode.maChildren)
    {
        if (pChild)
            accumulateFormulaNodeSummary(*pChild, rSummary);
    }
}

ReplaySummary summarizeWorkbook(
    const std::filesystem::path& rWorkbookPath, const Workbook& rWorkbook)
{
    ReplaySummary aSummary;
    ++aSummary.mnWorkbooks;
    ++aSummary.maFamilyWorkbookCounts[familyNameForWorkbook(rWorkbookPath)];

    Evaluator aEvaluator(rWorkbook);
    for (SheetId nSheet = 0; nSheet < static_cast<SheetId>(rWorkbook.maSheets.size()); ++nSheet)
    {
        const Sheet& rSheet = rWorkbook.maSheets[nSheet];
        for (const auto& [rKey, rCell] : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;

            ++aSummary.mnFormulaCells;

            const auto aParse = spreadsheetengine::core::formula::parseFormula(rCell.maFormula);
            if (aParse)
            {
                ++aSummary.mnParsedFormulas;
                accumulateFormulaNodeSummary(*aParse.mpRoot, aSummary);
            }

            const auto aResult = aEvaluator.evaluateCell({ nSheet, rKey.first, rKey.second });
            if (aResult.mbUsedCachedValue)
                ++aSummary.mnCachedFallbackCells;
        }
    }

    return aSummary;
}

void mergeSummary(ReplaySummary& rInto, const ReplaySummary& rFrom)
{
    rInto.mnWorkbooks += rFrom.mnWorkbooks;
    rInto.mnFormulaCells += rFrom.mnFormulaCells;
    rInto.mnParsedFormulas += rFrom.mnParsedFormulas;
    rInto.mnCachedFallbackCells += rFrom.mnCachedFallbackCells;
    rInto.mnCellReferenceNodes += rFrom.mnCellReferenceNodes;
    rInto.mnRangeReferenceNodes += rFrom.mnRangeReferenceNodes;
    rInto.mnNamedReferenceNodes += rFrom.mnNamedReferenceNodes;
    rInto.mnArrayConstantNodes += rFrom.mnArrayConstantNodes;
    rInto.mnFunctionCallNodes += rFrom.mnFunctionCallNodes;
    rInto.mnUnaryOperationNodes += rFrom.mnUnaryOperationNodes;
    rInto.mnBinaryOperationNodes += rFrom.mnBinaryOperationNodes;
    rInto.mnLiteralNodes += rFrom.mnLiteralNodes;

    for (const auto& [rKey, nValue] : rFrom.maFamilyWorkbookCounts)
        rInto.maFamilyWorkbookCounts[rKey] += nValue;
    for (const auto& [rKey, nValue] : rFrom.maFunctionCallCounts)
        rInto.maFunctionCallCounts[rKey] += nValue;
}

void printSummary(const ReplaySummary& rSummary)
{
    std::cout << "workbooks=" << rSummary.mnWorkbooks << '\n';
    std::cout << "formula_cells=" << rSummary.mnFormulaCells << '\n';
    std::cout << "parsed_formulas=" << rSummary.mnParsedFormulas << '\n';
    std::cout << "cached_fallback_cells=" << rSummary.mnCachedFallbackCells << '\n';
    std::cout << "cell_reference_nodes=" << rSummary.mnCellReferenceNodes << '\n';
    std::cout << "range_reference_nodes=" << rSummary.mnRangeReferenceNodes << '\n';
    std::cout << "named_reference_nodes=" << rSummary.mnNamedReferenceNodes << '\n';
    std::cout << "array_constant_nodes=" << rSummary.mnArrayConstantNodes << '\n';
    std::cout << "function_call_nodes=" << rSummary.mnFunctionCallNodes << '\n';
    std::cout << "unary_operation_nodes=" << rSummary.mnUnaryOperationNodes << '\n';
    std::cout << "binary_operation_nodes=" << rSummary.mnBinaryOperationNodes << '\n';
    std::cout << "literal_nodes=" << rSummary.mnLiteralNodes << '\n';

    std::cout << "family_workbooks=";
    bool bFirst = true;
    for (const auto& [rFamily, nCount] : rSummary.maFamilyWorkbookCounts)
    {
        if (!bFirst)
            std::cout << ",";
        bFirst = false;
        std::cout << rFamily << ":" << nCount;
    }
    std::cout << '\n';

    std::vector<std::pair<std::string, std::size_t>> aTopFunctions(
        rSummary.maFunctionCallCounts.begin(), rSummary.maFunctionCallCounts.end());
    std::sort(aTopFunctions.begin(), aTopFunctions.end(),
        [](const auto& rLeft, const auto& rRight) {
            if (rLeft.second != rRight.second)
                return rLeft.second > rRight.second;
            return rLeft.first < rRight.first;
        });

    std::cout << "top_functions=";
    for (std::size_t nIndex = 0; nIndex < std::min<std::size_t>(10, aTopFunctions.size()); ++nIndex)
    {
        if (nIndex > 0)
            std::cout << ",";
        std::cout << aTopFunctions[nIndex].first << ":" << aTopFunctions[nIndex].second;
    }
    std::cout << '\n';
}

PreflightSummary preflightWorkbook(
    const std::filesystem::path& rWorkbookPath, const Workbook& rWorkbook)
{
    PreflightSummary aSummary;
    spreadsheetengine::detail::compiler::WorkbookCompileHost aHost(rWorkbook);

    for (SheetId nSheet = 0; nSheet < static_cast<SheetId>(rWorkbook.maSheets.size()); ++nSheet)
    {
        const Sheet& rSheet = rWorkbook.maSheets[nSheet];
        for (const auto& [rKey, rCell] : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;

            ++aSummary.mnFormulaCells;
            const auto aContext = spreadsheetengine::detail::compiler::makeWorkbookCompileContext(
                { nSheet, rKey.first, rKey.second });
            const auto aResult = spreadsheetengine::detail::compiler::preflightFormulaSource(
                rCell.maFormula, aHost, aContext);
            if (aResult)
            {
                ++aSummary.mnReady;
                continue;
            }

            const bool bExpectedErrorPath = cellCachesError(rCell);
            const std::string aReason
                = preflightBucketName(aResult.meReason, bExpectedErrorPath);
            auto& rReasonCounts = bExpectedErrorPath ? aSummary.maExpectedErrorReasonCounts
                                                     : aSummary.maReasonCounts;
            auto& rReasonExamples = bExpectedErrorPath ? aSummary.maExpectedErrorReasonExamples
                                                       : aSummary.maReasonExamples;

            if (bExpectedErrorPath)
                ++aSummary.mnExpectedError;
            ++rReasonCounts[aReason];

            if (!rReasonExamples.contains(aReason))
            {
                std::string aExample
                    = rWorkbookPath.filename().string() + " " + toUtf8(rSheet.maName) + "."
                      + columnLabel(rKey.first) + std::to_string(rKey.second + 1) + " "
                      + toUtf8(rCell.maFormula);
                if (bExpectedErrorPath)
                    aExample += " => " + formatScalarValue(rCell.maValue);
                rReasonExamples[aReason] = std::move(aExample);
            }
        }
    }

    return aSummary;
}

void mergePreflightSummary(PreflightSummary& rInto, const PreflightSummary& rFrom)
{
    rInto.mnFormulaCells += rFrom.mnFormulaCells;
    rInto.mnReady += rFrom.mnReady;
    rInto.mnExpectedError += rFrom.mnExpectedError;
    for (const auto& [rReason, nCount] : rFrom.maReasonCounts)
        rInto.maReasonCounts[rReason] += nCount;
    for (const auto& [rReason, rExample] : rFrom.maReasonExamples)
    {
        if (!rInto.maReasonExamples.contains(rReason))
            rInto.maReasonExamples[rReason] = rExample;
    }
    for (const auto& [rReason, nCount] : rFrom.maExpectedErrorReasonCounts)
        rInto.maExpectedErrorReasonCounts[rReason] += nCount;
    for (const auto& [rReason, rExample] : rFrom.maExpectedErrorReasonExamples)
    {
        if (!rInto.maExpectedErrorReasonExamples.contains(rReason))
            rInto.maExpectedErrorReasonExamples[rReason] = rExample;
    }
}

void printPreflightSummary(const PreflightSummary& rSummary)
{
    const std::size_t nHardBlockers
        = rSummary.mnFormulaCells - rSummary.mnReady - rSummary.mnExpectedError;
    std::cout << "preflight_formula_cells=" << rSummary.mnFormulaCells << '\n';
    std::cout << "preflight_ready=" << rSummary.mnReady << '\n';
    std::cout << "preflight_expected_error=" << rSummary.mnExpectedError << '\n';
    std::cout << "preflight_not_ready=" << (rSummary.mnFormulaCells - rSummary.mnReady) << '\n';
    std::cout << "preflight_hard_blockers=" << nHardBlockers << '\n';
    std::cout << "preflight_ready_rate="
              << (rSummary.mnFormulaCells
                      ? (100.0 * static_cast<double>(rSummary.mnReady)
                            / static_cast<double>(rSummary.mnFormulaCells))
                      : 0.0)
              << '\n';
    std::cout << "preflight_effective_ready_rate="
              << (rSummary.mnFormulaCells
                      ? (100.0 * static_cast<double>(rSummary.mnReady + rSummary.mnExpectedError)
                            / static_cast<double>(rSummary.mnFormulaCells))
                      : 0.0)
              << '\n';

    std::cout << "preflight_reasons=";
    bool bFirst = true;
    for (const auto& [rReason, nCount] : rSummary.maReasonCounts)
    {
        if (!bFirst)
            std::cout << ",";
        bFirst = false;
        std::cout << rReason << ":" << nCount;
    }
    std::cout << '\n';

    std::cout << "preflight_expected_error_reasons=";
    bFirst = true;
    for (const auto& [rReason, nCount] : rSummary.maExpectedErrorReasonCounts)
    {
        if (!bFirst)
            std::cout << ",";
        bFirst = false;
        std::cout << rReason << ":" << nCount;
    }
    std::cout << '\n';

    std::cout << "preflight_examples=";
    bFirst = true;
    for (const auto& [rReason, rExample] : rSummary.maReasonExamples)
    {
        if (!bFirst)
            std::cout << " | ";
        bFirst = false;
        std::cout << rReason << ":" << rExample;
    }
    std::cout << '\n';

    std::cout << "preflight_expected_error_examples=";
    bFirst = true;
    for (const auto& [rReason, rExample] : rSummary.maExpectedErrorReasonExamples)
    {
        if (!bFirst)
            std::cout << " | ";
        bFirst = false;
        std::cout << rReason << ":" << rExample;
    }
    std::cout << '\n';
}

std::optional<ColumnIndex> findHeaderColumn(const Sheet& rSheet, StringView rHeader)
{
    for (const auto& [rKey, rCell] : rSheet.maCells)
    {
        if (rKey.second != 0 || !rCell.maValue.isText())
            continue;
        if (rCell.maValue.maString == rHeader)
            return rKey.first;
    }

    return std::nullopt;
}

RowIndex findLastDataRow(const Sheet& rSheet, ColumnIndex nColumn)
{
    RowIndex nLast = 0;
    for (const auto& [rKey, rCell] : rSheet.maCells)
    {
        if (rKey.first == nColumn)
            nLast = std::max(nLast, rKey.second);
    }
    return nLast;
}

std::string readEvaluatedCell(
    Evaluator& rEvaluator, const Sheet& rSheet, SheetId nSheet, ColumnIndex nColumn, RowIndex nRow)
{
    if (!rSheet.findCell(nColumn, nRow))
        return "";

    const EvaluationResult aResult = rEvaluator.evaluateCell({ nSheet, nColumn, nRow });
    if (!aResult)
        return formatError(aResult.meError);
    if (!aResult.maValue.isScalar())
        return "<range>";
    return formatScalarValue(aResult.maValue.maValue);
}

std::string readRawCell(const Sheet& rSheet, ColumnIndex nColumn, RowIndex nRow)
{
    const auto* pCell = rSheet.findCell(nColumn, nRow);
    if (!pCell)
        return "";
    return formatScalarValue(pCell->maValue);
}

std::string collectColumnGroup(
    Evaluator& rEvaluator, const Sheet& rSheet, SheetId nSheet, ColumnIndex nStartColumn,
    ColumnIndex nCount, RowIndex nRow)
{
    std::string aJoined;
    for (ColumnIndex nOffset = 0; nOffset < nCount; ++nOffset)
    {
        if (nOffset > 0)
            aJoined += ", ";
        aJoined += readEvaluatedCell(rEvaluator, rSheet, nSheet, nStartColumn + nOffset, nRow);
    }
    return aJoined;
}

std::optional<std::string> findFirstFailure(
    const std::filesystem::path& rWorkbookPath, const Workbook& rWorkbook, Evaluator& rEvaluator)
{
    for (SheetId nSheet = 1; nSheet < static_cast<SheetId>(rWorkbook.maSheets.size()); ++nSheet)
    {
        const Sheet& rSheet = rWorkbook.maSheets[nSheet];
        const auto oExpected = findHeaderColumn(rSheet, u"Expected");
        const auto oCorrect = findHeaderColumn(rSheet, u"Correct");
        const auto oFunction = findHeaderColumn(rSheet, u"FunctionString");
        if (!oExpected || !oCorrect || !oFunction || *oCorrect <= *oExpected)
            continue;

        const RowIndex nMaxRow = findLastDataRow(rSheet, *oCorrect);
        for (RowIndex nRow = 1; nRow <= nMaxRow; ++nRow)
        {
            if (!rSheet.findCell(*oCorrect, nRow))
                continue;

            const EvaluationResult aCorrect
                = rEvaluator.evaluateCell({ nSheet, *oCorrect, nRow });
            if (isSuccessLike(aCorrect))
                continue;

            const ColumnIndex nResultColumnCount = *oExpected;
            const ColumnIndex nExpectedColumnCount = *oCorrect - *oExpected;
            if (nResultColumnCount != nExpectedColumnCount)
            {
                return "Function columns != Expected columns in "
                       + rWorkbookPath.filename().string() + " [" + toUtf8(rSheet.maName) + "]";
            }

            const std::string aResult = collectColumnGroup(
                rEvaluator, rSheet, nSheet, 0, nResultColumnCount, nRow);
            const std::string aExpected = collectColumnGroup(
                rEvaluator, rSheet, nSheet, *oExpected, nExpectedColumnCount, nRow);
            const std::string aFunctionString = readRawCell(rSheet, *oFunction, nRow);

            return "Testing " + rWorkbookPath.filename().string() + " failed, "
                   + toUtf8(rSheet.maName) + "." + columnLabel(0) + std::to_string(nRow + 1)
                   + " '" + aFunctionString + "' result: '" + aResult + "', expected: '"
                   + aExpected + "'";
        }
    }

    return std::nullopt;
}

std::optional<std::string> replayWorkbook(const std::filesystem::path& rWorkbookPath)
{
    const auto aLoadResult = loadWorkbook(rWorkbookPath.string());
    if (!aLoadResult)
        return "Failed to load " + rWorkbookPath.string();

    Evaluator aEvaluator(aLoadResult.maValue.maWorkbook);
    const EvaluationResult aSuccess = aEvaluator.evaluateCell({ 0, SUCCESS_COLUMN, SUCCESS_ROW });
    if (isSuccessLike(aSuccess))
        return std::nullopt;

    if (const auto oFailure
        = findFirstFailure(rWorkbookPath, aLoadResult.maValue.maWorkbook, aEvaluator))
    {
        return oFailure;
    }

    std::string aMessage = "Workbook " + rWorkbookPath.filename().string()
                           + " failed cumulative success check at Sheet1.B3";
    if (!aSuccess)
        aMessage += " (" + formatError(aSuccess.meError) + ")";
    else if (aSuccess.maValue.isScalar())
        aMessage += " (actual '" + formatScalarValue(aSuccess.maValue.maValue) + "')";
    return aMessage;
}

} // namespace

int main(int argc, char** argv)
{
    using spreadsheetengine::standalone::test::fail;

    std::vector<std::filesystem::path> aWorkbooks;
    bool bSummary = false;
    bool bPreflight = false;
    bool bSawPathArgument = false;
    if (argc > 1)
    {
        for (int nIndex = 1; nIndex < argc; ++nIndex)
        {
            if (std::string_view(argv[nIndex]) == "--summary")
            {
                bSummary = true;
                continue;
            }
            if (std::string_view(argv[nIndex]) == "--preflight")
            {
                bPreflight = true;
                continue;
            }

            const auto aMatches = collectFodsFiles(argv[nIndex]);
            aWorkbooks.insert(aWorkbooks.end(), aMatches.begin(), aMatches.end());
            bSawPathArgument = true;
        }
    }

    if (argc == 1 || !bSawPathArgument)
    {
        aWorkbooks = collectDefaultReplayCorpus();
    }

    if (aWorkbooks.empty() && bSummary)
        aWorkbooks = collectDefaultReplayCorpus();
    if (aWorkbooks.empty() && bPreflight)
        aWorkbooks = collectDefaultReplayCorpus();

    if (aWorkbooks.empty())
        return fail("spreadsheetengine_fods_replay_tests", "no FODS workbooks found");

    if (bSummary)
    {
        ReplaySummary aSummary;
        for (const auto& rWorkbookPath : aWorkbooks)
        {
            const auto aLoadResult = loadWorkbook(rWorkbookPath.string());
            if (!aLoadResult)
                return fail("spreadsheetengine_fods_replay_tests", "failed to load workbook for summary");

            mergeSummary(aSummary, summarizeWorkbook(rWorkbookPath, aLoadResult.maValue.maWorkbook));
        }

        printSummary(aSummary);
        return EXIT_SUCCESS;
    }

    if (bPreflight)
    {
        PreflightSummary aSummary;
        for (const auto& rWorkbookPath : aWorkbooks)
        {
            const auto aLoadResult = loadWorkbook(rWorkbookPath.string());
            if (!aLoadResult)
                return fail("spreadsheetengine_fods_replay_tests", "failed to load workbook for preflight");

            mergePreflightSummary(
                aSummary, preflightWorkbook(rWorkbookPath, aLoadResult.maValue.maWorkbook));
        }

        printPreflightSummary(aSummary);
        return EXIT_SUCCESS;
    }

    for (const auto& rWorkbook : aWorkbooks)
    {
        if (const auto oFailure = replayWorkbook(rWorkbook))
            return fail("spreadsheetengine_fods_replay_tests", oFailure->c_str());
    }

    std::cout << "spreadsheetengine raw FODS replay passed for " << aWorkbooks.size()
              << " workbook(s)\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
