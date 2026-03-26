/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <spreadsheetengine/detail/FodsEvaluator.hxx>
#include <spreadsheetengine/detail/FodsLoader.hxx>

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
using spreadsheetengine::core::fods::EvaluationResult;
using spreadsheetengine::core::fods::Evaluator;
using spreadsheetengine::core::fods::loadWorkbook;
using spreadsheetengine::core::workbook::Sheet;
using spreadsheetengine::core::workbook::Workbook;

constexpr ColumnIndex SUCCESS_COLUMN = 1; // B
constexpr RowIndex SUCCESS_ROW = 2; // 3rd row

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
    for (const char* pWorkbook : { "add.fods", "product.fods", "rawsubtract.fods", "round.fods",
             "rounddown.fods", "roundup.fods", "sub.fods", "sum.fods", "trunc.fods",
             "mod.fods", "abs.fods", "int.fods", "sign.fods", "even.fods", "odd.fods",
             "pi.fods", "ln.fods", "exp.fods", "log10.fods", "power.fods", "quotient.fods",
             "gcd.fods", "lcm.fods", "radians.fods", "degrees.fods", "sqrt.fods",
             "log.fods", "fact.fods", "combin.fods", "combina.fods", "multinomial.fods",
             "floor.precise.fods", "ceiling.precise.fods", "sqrtpi.fods", "mround.fods",
             "floor.xcl.fods", "ceiling.xcl.fods", "iso.ceiling.fods", "roundsig.fods",
             "sec.fods", "sech.fods" })
    {
        aFiles.push_back(aMathRoot / pWorkbook);
    }

    return aFiles;
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
    if (argc > 1)
    {
        for (int nIndex = 1; nIndex < argc; ++nIndex)
        {
            const auto aMatches = collectFodsFiles(argv[nIndex]);
            aWorkbooks.insert(aWorkbooks.end(), aMatches.begin(), aMatches.end());
        }
    }
    else
    {
        aWorkbooks = collectDefaultReplayCorpus();
    }

    if (aWorkbooks.empty())
        return fail("spreadsheetengine_fods_replay_tests", "no FODS workbooks found");

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
