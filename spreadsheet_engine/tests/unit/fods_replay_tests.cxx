/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <map>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <spreadsheetengine/detail/FodsEvaluator.hxx>
#include <spreadsheetengine/detail/FodsCompilerPreflight.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>
#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>

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

struct NativeLoweringSummary
{
    std::size_t mnFormulaCells = 0;
    std::size_t mnPreflightReady = 0;
    std::size_t mnPreflightExpectedError = 0;
    std::size_t mnLowered = 0;
    std::map<std::string, std::size_t> maPreflightReasonCounts;
    std::map<std::string, std::string> maPreflightReasonExamples;
    std::map<std::string, std::size_t> maLoweringReasonCounts;
    std::map<std::string, std::string> maLoweringReasonExamples;
};

struct CompiledDiffSummary
{
    std::size_t mnFormulaCells = 0;
    std::size_t mnEligibleFormulaCells = 0;
    std::size_t mnSkippedFormulaCells = 0;
    std::size_t mnMatchedFormulaCells = 0;
    std::size_t mnCachedFallbackOnly = 0;
    std::string maCachedFallbackExample;
};

std::string columnLabel(ColumnIndex nColumn);

using FormulaAddressKey = std::tuple<SheetId, ColumnIndex, RowIndex>;
using ReadyFormulaSet = std::set<FormulaAddressKey>;

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

std::string formatEvaluationResult(const EvaluationResult& rResult)
{
    if (!rResult)
    {
        std::string aResult = "error:" + formatError(rResult.meError);
        if (!rResult.maCyclePath.empty())
        {
            aResult += " cycle=";
            for (std::size_t nIndex = 0; nIndex < rResult.maCyclePath.size(); ++nIndex)
            {
                if (nIndex > 0)
                    aResult += "->";
                aResult += columnLabel(rResult.maCyclePath[nIndex].mnColumn)
                           + std::to_string(rResult.maCyclePath[nIndex].mnRow + 1);
            }
        }
        return aResult;
    }

    std::string aPrefix = rResult.mbUsedCachedValue ? "cached:" : "live:";
    if (!rResult.maValue.isScalar())
    {
        const auto aDims = rResult.maValue.maReference.matrixDimensions();
        return aPrefix + "range:" + std::to_string(aDims.mnColumns) + "x"
               + std::to_string(aDims.mnRows);
    }

    return aPrefix + formatScalarValue(rResult.maValue.maValue);
}

bool evaluationResultsEqual(const EvaluationResult& rLeft, const EvaluationResult& rRight)
{
    if (rLeft.ok() != rRight.ok())
        return false;
    if (!rLeft)
        return rLeft.meError == rRight.meError && rLeft.maCyclePath == rRight.maCyclePath;
    if (rLeft.maValue.isScalar() != rRight.maValue.isScalar())
        return false;
    if (!rLeft.maValue.isScalar())
        return rLeft.maValue.maReference.maRange == rRight.maValue.maReference.maRange;

    const auto& rLeftValue = rLeft.maValue.maValue;
    const auto& rRightValue = rRight.maValue.maValue;
    if (rLeftValue.meKind != rRightValue.meKind)
        return false;
    if (rLeftValue.isNumber() || rLeftValue.isBoolean())
        return spreadsheetengine::standalone::test::almostEqual(rLeftValue.mfNumber, rRightValue.mfNumber);
    if (rLeftValue.isText())
        return rLeftValue.maString == rRightValue.maString;
    if (rLeftValue.isError())
        return rLeftValue.meError == rRightValue.meError;
    return true;
}

bool isCachedFallbackOnlyMismatch(const EvaluationResult& rLeft, const EvaluationResult& rRight)
{
    return evaluationResultsEqual(rLeft, rRight) && rLeft.ok() == rRight.ok()
           && rLeft.mbUsedCachedValue != rRight.mbUsedCachedValue;
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

bool isCompiledReplayPromotedFamily(const std::string& rFamily)
{
    return rFamily == "logical" || rFamily == "mathematical" || rFamily == "text"
           || rFamily == "date_time" || rFamily == "information" || rFamily == "spreadsheet";
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

NativeLoweringSummary nativeLowerWorkbook(
    const std::filesystem::path& rWorkbookPath, const Workbook& rWorkbook)
{
    NativeLoweringSummary aSummary;
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
            const auto aPreflight = spreadsheetengine::detail::compiler::preflightFormulaSource(
                rCell.maFormula, aHost, aContext);
            if (!aPreflight)
            {
                const bool bExpectedErrorPath = cellCachesError(rCell);
                if (bExpectedErrorPath)
                    ++aSummary.mnPreflightExpectedError;

                const std::string aReason
                    = preflightBucketName(aPreflight.meReason, bExpectedErrorPath);
                auto& rReasonCounts = aSummary.maPreflightReasonCounts;
                auto& rReasonExamples = aSummary.maPreflightReasonExamples;
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
                continue;
            }

            ++aSummary.mnPreflightReady;
            const auto aLowering = spreadsheetengine::detail::compiler::lowerFormulaSource(
                rCell.maFormula, aHost, aContext);
            if (aLowering)
            {
                ++aSummary.mnLowered;
                continue;
            }

            const std::string aReason
                = toUtf8(spreadsheetengine::detail::compiler::loweringReasonName(aLowering.meReason));
            ++aSummary.maLoweringReasonCounts[aReason];
            if (!aSummary.maLoweringReasonExamples.contains(aReason))
            {
                std::string aExample
                    = rWorkbookPath.filename().string() + " " + toUtf8(rSheet.maName) + "."
                      + columnLabel(rKey.first) + std::to_string(rKey.second + 1) + " "
                      + toUtf8(rCell.maFormula);
                if (!aLowering.maDetail.empty())
                    aExample += " => " + toUtf8(aLowering.maDetail);
                aSummary.maLoweringReasonExamples[aReason] = std::move(aExample);
            }
        }
    }

    return aSummary;
}

void mergeNativeLoweringSummary(NativeLoweringSummary& rInto, const NativeLoweringSummary& rFrom)
{
    rInto.mnFormulaCells += rFrom.mnFormulaCells;
    rInto.mnPreflightReady += rFrom.mnPreflightReady;
    rInto.mnPreflightExpectedError += rFrom.mnPreflightExpectedError;
    rInto.mnLowered += rFrom.mnLowered;

    for (const auto& [rReason, nCount] : rFrom.maPreflightReasonCounts)
        rInto.maPreflightReasonCounts[rReason] += nCount;
    for (const auto& [rReason, rExample] : rFrom.maPreflightReasonExamples)
    {
        if (!rInto.maPreflightReasonExamples.contains(rReason))
            rInto.maPreflightReasonExamples[rReason] = rExample;
    }

    for (const auto& [rReason, nCount] : rFrom.maLoweringReasonCounts)
        rInto.maLoweringReasonCounts[rReason] += nCount;
    for (const auto& [rReason, rExample] : rFrom.maLoweringReasonExamples)
    {
        if (!rInto.maLoweringReasonExamples.contains(rReason))
            rInto.maLoweringReasonExamples[rReason] = rExample;
    }
}

void printNativeLoweringSummary(const NativeLoweringSummary& rSummary)
{
    const std::size_t nPreflightHardBlockers
        = rSummary.mnFormulaCells - rSummary.mnPreflightReady - rSummary.mnPreflightExpectedError;
    const std::size_t nLoweringFailures = rSummary.mnPreflightReady - rSummary.mnLowered;

    std::cout << "native_lower_formula_cells=" << rSummary.mnFormulaCells << '\n';
    std::cout << "native_lower_preflight_ready=" << rSummary.mnPreflightReady << '\n';
    std::cout << "native_lower_preflight_expected_error=" << rSummary.mnPreflightExpectedError
              << '\n';
    std::cout << "native_lower_preflight_hard_blockers=" << nPreflightHardBlockers << '\n';
    std::cout << "native_lower_lowered=" << rSummary.mnLowered << '\n';
    std::cout << "native_lower_lowering_failures=" << nLoweringFailures << '\n';
    std::cout << "native_lower_ready_lowered_rate="
              << (rSummary.mnPreflightReady
                      ? (100.0 * static_cast<double>(rSummary.mnLowered)
                            / static_cast<double>(rSummary.mnPreflightReady))
                      : 0.0)
              << '\n';
    std::cout << "native_lower_overall_rate="
              << (rSummary.mnFormulaCells
                      ? (100.0 * static_cast<double>(rSummary.mnLowered)
                            / static_cast<double>(rSummary.mnFormulaCells))
                      : 0.0)
              << '\n';

    std::cout << "native_lower_preflight_reasons=";
    bool bFirst = true;
    for (const auto& [rReason, nCount] : rSummary.maPreflightReasonCounts)
    {
        if (!bFirst)
            std::cout << ",";
        bFirst = false;
        std::cout << rReason << ":" << nCount;
    }
    std::cout << '\n';

    std::cout << "native_lower_lowering_reasons=";
    bFirst = true;
    for (const auto& [rReason, nCount] : rSummary.maLoweringReasonCounts)
    {
        if (!bFirst)
            std::cout << ",";
        bFirst = false;
        std::cout << rReason << ":" << nCount;
    }
    std::cout << '\n';

    std::cout << "native_lower_preflight_examples=";
    bFirst = true;
    for (const auto& [rReason, rExample] : rSummary.maPreflightReasonExamples)
    {
        if (!bFirst)
            std::cout << " | ";
        bFirst = false;
        std::cout << rReason << ":" << rExample;
    }
    std::cout << '\n';

    std::cout << "native_lower_lowering_examples=";
    bFirst = true;
    for (const auto& [rReason, rExample] : rSummary.maLoweringReasonExamples)
    {
        if (!bFirst)
            std::cout << " | ";
        bFirst = false;
        std::cout << rReason << ":" << rExample;
    }
    std::cout << '\n';
}

std::optional<std::string> diffCompiledWorkbook(const std::filesystem::path& rWorkbookPath,
    const Workbook& rWorkbook, CompiledDiffSummary& rSummary)
{
    Evaluator aAstEvaluator(rWorkbook);
    Evaluator aCompiledEvaluator(rWorkbook);
    spreadsheetengine::detail::compiler::WorkbookCompileHost aHost(rWorkbook);

    for (SheetId nSheet = 0; nSheet < static_cast<SheetId>(rWorkbook.maSheets.size()); ++nSheet)
    {
        const Sheet& rSheet = rWorkbook.maSheets[nSheet];
        for (const auto& [rKey, rCell] : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;

            ++rSummary.mnFormulaCells;
            const auto aContext = spreadsheetengine::detail::compiler::makeWorkbookCompileContext(
                { nSheet, rKey.first, rKey.second });
            const auto aPreflight = spreadsheetengine::detail::compiler::preflightFormulaSource(
                rCell.maFormula, aHost, aContext);
            if (!aPreflight)
            {
                ++rSummary.mnSkippedFormulaCells;
                continue;
            }

            ++rSummary.mnEligibleFormulaCells;
            const CellAddress aAddress { nSheet, rKey.first, rKey.second };
            const EvaluationResult aAstResult = aAstEvaluator.evaluateCell(aAddress);
            const EvaluationResult aCompiledResult = aCompiledEvaluator.evaluateCellViaCompiledTokens(aAddress);
            if (!evaluationResultsEqual(aAstResult, aCompiledResult))
            {
                return rWorkbookPath.filename().string() + " " + toUtf8(rSheet.maName) + "."
                       + columnLabel(rKey.first) + std::to_string(rKey.second + 1) + " "
                       + toUtf8(rCell.maFormula) + " AST='" + formatEvaluationResult(aAstResult)
                       + "' compiled='" + formatEvaluationResult(aCompiledResult) + "'";
            }

            if (isCachedFallbackOnlyMismatch(aAstResult, aCompiledResult))
            {
                ++rSummary.mnCachedFallbackOnly;
                if (rSummary.maCachedFallbackExample.empty())
                {
                    rSummary.maCachedFallbackExample
                        = rWorkbookPath.filename().string() + " " + toUtf8(rSheet.maName) + "."
                          + columnLabel(rKey.first) + std::to_string(rKey.second + 1) + " "
                          + toUtf8(rCell.maFormula) + " AST='"
                          + formatEvaluationResult(aAstResult) + "' compiled='"
                          + formatEvaluationResult(aCompiledResult) + "'";
                }
            }

            ++rSummary.mnMatchedFormulaCells;
        }
    }

    return std::nullopt;
}

void printCompiledDiffSummary(const CompiledDiffSummary& rSummary)
{
    std::cout << "compiled_diff_formula_cells=" << rSummary.mnFormulaCells << '\n';
    std::cout << "compiled_diff_eligible=" << rSummary.mnEligibleFormulaCells << '\n';
    std::cout << "compiled_diff_skipped=" << rSummary.mnSkippedFormulaCells << '\n';
    std::cout << "compiled_diff_matched=" << rSummary.mnMatchedFormulaCells << '\n';
    std::cout << "compiled_diff_cached_fallback_only=" << rSummary.mnCachedFallbackOnly << '\n';
    std::cout << "compiled_diff_match_rate="
              << (rSummary.mnEligibleFormulaCells
                      ? (100.0 * static_cast<double>(rSummary.mnMatchedFormulaCells)
                            / static_cast<double>(rSummary.mnEligibleFormulaCells))
                      : 0.0)
              << '\n';
    std::cout << "compiled_diff_cached_fallback_example=" << rSummary.maCachedFallbackExample
              << '\n';
}

ReadyFormulaSet collectPreflightReadyCells(const Workbook& rWorkbook)
{
    ReadyFormulaSet aReady;
    spreadsheetengine::detail::compiler::WorkbookCompileHost aHost(rWorkbook);

    for (SheetId nSheet = 0; nSheet < static_cast<SheetId>(rWorkbook.maSheets.size()); ++nSheet)
    {
        const Sheet& rSheet = rWorkbook.maSheets[nSheet];
        for (const auto& [rKey, rCell] : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;

            const auto aContext = spreadsheetengine::detail::compiler::makeWorkbookCompileContext(
                { nSheet, rKey.first, rKey.second });
            const auto aPreflight = spreadsheetengine::detail::compiler::preflightFormulaSource(
                rCell.maFormula, aHost, aContext);
            if (aPreflight)
                aReady.emplace(nSheet, rKey.first, rKey.second);
        }
    }

    return aReady;
}

EvaluationResult evaluateReplayCell(Evaluator& rAstEvaluator, Evaluator& rCompiledEvaluator,
    const ReadyFormulaSet* pReadyCells, const CellAddress& rAddress)
{
    if (pReadyCells && pReadyCells->contains({ rAddress.mnSheet, rAddress.mnColumn, rAddress.mnRow }))
        return rCompiledEvaluator.evaluateCellViaCompiledTokens(rAddress);
    return rAstEvaluator.evaluateCell(rAddress);
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

std::string readEvaluatedCell(Evaluator& rAstEvaluator, Evaluator& rCompiledEvaluator,
    const ReadyFormulaSet* pReadyCells, const Sheet& rSheet, SheetId nSheet, ColumnIndex nColumn,
    RowIndex nRow)
{
    if (!rSheet.findCell(nColumn, nRow))
        return "";

    const EvaluationResult aResult
        = evaluateReplayCell(rAstEvaluator, rCompiledEvaluator, pReadyCells, { nSheet, nColumn, nRow });
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

std::string collectColumnGroup(Evaluator& rAstEvaluator, Evaluator& rCompiledEvaluator,
    const ReadyFormulaSet* pReadyCells, const Sheet& rSheet, SheetId nSheet,
    ColumnIndex nStartColumn, ColumnIndex nCount, RowIndex nRow)
{
    std::string aJoined;
    for (ColumnIndex nOffset = 0; nOffset < nCount; ++nOffset)
    {
        if (nOffset > 0)
            aJoined += ", ";
        aJoined += readEvaluatedCell(
            rAstEvaluator, rCompiledEvaluator, pReadyCells, rSheet, nSheet, nStartColumn + nOffset, nRow);
    }
    return aJoined;
}

std::optional<std::string> findFirstFailure(
    const std::filesystem::path& rWorkbookPath, const Workbook& rWorkbook, Evaluator& rAstEvaluator,
    Evaluator& rCompiledEvaluator, const ReadyFormulaSet* pReadyCells)
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

            const EvaluationResult aCorrect = evaluateReplayCell(
                rAstEvaluator, rCompiledEvaluator, pReadyCells, { nSheet, *oCorrect, nRow });
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
                rAstEvaluator, rCompiledEvaluator, pReadyCells, rSheet, nSheet, 0, nResultColumnCount, nRow);
            const std::string aExpected = collectColumnGroup(rAstEvaluator, rCompiledEvaluator,
                pReadyCells, rSheet, nSheet, *oExpected, nExpectedColumnCount, nRow);
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

    const bool bPreferCompiled = isCompiledReplayPromotedFamily(familyNameForWorkbook(rWorkbookPath));
    ReadyFormulaSet aReadyCells;
    const ReadyFormulaSet* pReadyCells = nullptr;
    if (bPreferCompiled)
    {
        aReadyCells = collectPreflightReadyCells(aLoadResult.maValue.maWorkbook);
        pReadyCells = &aReadyCells;
    }

    Evaluator aAstEvaluator(aLoadResult.maValue.maWorkbook);
    Evaluator aCompiledEvaluator(aLoadResult.maValue.maWorkbook);
    const EvaluationResult aSuccess = evaluateReplayCell(
        aAstEvaluator, aCompiledEvaluator, pReadyCells, { 0, SUCCESS_COLUMN, SUCCESS_ROW });
    if (isSuccessLike(aSuccess))
        return std::nullopt;

    if (const auto oFailure
        = findFirstFailure(rWorkbookPath, aLoadResult.maValue.maWorkbook, aAstEvaluator,
            aCompiledEvaluator, pReadyCells))
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
    bool bNativeLowerSmoke = false;
    bool bCompiledDiff = false;
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
            if (std::string_view(argv[nIndex]) == "--native-lower-smoke")
            {
                bNativeLowerSmoke = true;
                continue;
            }
            if (std::string_view(argv[nIndex]) == "--compiled-diff")
            {
                bCompiledDiff = true;
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
    if (aWorkbooks.empty() && bNativeLowerSmoke)
        aWorkbooks = collectDefaultReplayCorpus();
    if (aWorkbooks.empty() && bCompiledDiff)
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

    if (bNativeLowerSmoke)
    {
        NativeLoweringSummary aSummary;
        for (const auto& rWorkbookPath : aWorkbooks)
        {
            const auto aLoadResult = loadWorkbook(rWorkbookPath.string());
            if (!aLoadResult)
            {
                return fail("spreadsheetengine_fods_replay_tests",
                    "failed to load workbook for native lower smoke");
            }

            mergeNativeLoweringSummary(
                aSummary, nativeLowerWorkbook(rWorkbookPath, aLoadResult.maValue.maWorkbook));
        }

        printNativeLoweringSummary(aSummary);
        return EXIT_SUCCESS;
    }

    if (bCompiledDiff)
    {
        CompiledDiffSummary aSummary;
        for (const auto& rWorkbookPath : aWorkbooks)
        {
            const auto aLoadResult = loadWorkbook(rWorkbookPath.string());
            if (!aLoadResult)
            {
                return fail("spreadsheetengine_fods_replay_tests",
                    "failed to load workbook for compiled diff");
            }

            if (const auto oFailure = diffCompiledWorkbook(
                    rWorkbookPath, aLoadResult.maValue.maWorkbook, aSummary))
            {
                return fail("spreadsheetengine_fods_replay_tests", oFailure->c_str());
            }
        }

        printCompiledDiffSummary(aSummary);
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
