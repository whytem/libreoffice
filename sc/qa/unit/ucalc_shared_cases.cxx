/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helper/qahelper.hxx"

#include <scopetools.hxx>

#include <formula/errorcodes.hxx>
#include <global.hxx>
#include <interpretercontext.hxx>
#include <rangenam.hxx>
#include <spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/FormulaInspectionExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx>
#include <spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx>
#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Parsing.hxx>
#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/HostValueAccess.hxx>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{

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

struct SharedCaseRow
{
    std::vector<std::string> maColumns;
    std::size_t mnLineNumber = 0;
};

std::vector<std::string> splitTsvLine(const std::string& rLine)
{
    std::vector<std::string> aColumns;
    std::size_t nStart = 0;
    while (nStart <= rLine.size())
    {
        const std::size_t nTab = rLine.find('\t', nStart);
        if (nTab == std::string::npos)
        {
            aColumns.emplace_back(rLine.substr(nStart));
            break;
        }

        aColumns.emplace_back(rLine.substr(nStart, nTab - nStart));
        nStart = nTab + 1;
        if (nStart == rLine.size())
        {
            aColumns.emplace_back(std::string());
            break;
        }
    }
    return aColumns;
}

std::vector<SharedCaseRow> loadSharedCaseRows(std::string_view rRelativePath)
{
    const std::string aPath
        = std::string(SPREADSHEETENGINE_SHARED_CASE_ROOT) + "/" + std::string(rRelativePath);
    std::ifstream aInput(aPath);
    CPPUNIT_ASSERT_MESSAGE(("unable to open shared case file: " + aPath).c_str(), aInput.is_open());

    std::vector<SharedCaseRow> aRows;
    std::string aLine;
    std::size_t nLineNumber = 0;
    while (std::getline(aInput, aLine))
    {
        ++nLineNumber;
        if (!aLine.empty() && aLine.back() == '\r')
            aLine.pop_back();
        if (aLine.empty() || aLine[0] == '#')
            continue;

        aRows.push_back({ splitTsvLine(aLine), nLineNumber });
    }

    return aRows;
}

int hexDigitValue(char cDigit)
{
    if (cDigit >= '0' && cDigit <= '9')
        return cDigit - '0';
    if (cDigit >= 'a' && cDigit <= 'f')
        return 10 + (cDigit - 'a');
    if (cDigit >= 'A' && cDigit <= 'F')
        return 10 + (cDigit - 'A');
    return -1;
}

void appendCodePoint(OUStringBuffer& rOutput, char32_t nCodePoint)
{
    if (nCodePoint <= 0xFFFF)
    {
        rOutput.append(static_cast<sal_Unicode>(nCodePoint));
        return;
    }

    nCodePoint -= 0x10000;
    rOutput.append(static_cast<sal_Unicode>(0xD800 + ((nCodePoint >> 10) & 0x3FF)));
    rOutput.append(static_cast<sal_Unicode>(0xDC00 + (nCodePoint & 0x3FF)));
}

bool appendEscapedCodePoint(
    OUStringBuffer& rOutput, std::string_view rInput, std::size_t& rnIndex)
{
    if (rnIndex + 1 >= rInput.size() || rInput[rnIndex] != '\\')
        return false;

    const char cEscapeType = rInput[rnIndex + 1];
    const std::size_t nHexDigits = cEscapeType == 'u' ? 4 : (cEscapeType == 'U' ? 8 : 0);
    if (!nHexDigits || rnIndex + 2 + nHexDigits > rInput.size())
        return false;

    char32_t nCodePoint = 0;
    for (std::size_t i = 0; i < nHexDigits; ++i)
    {
        const int nDigit = hexDigitValue(rInput[rnIndex + 2 + i]);
        if (nDigit < 0)
            return false;
        nCodePoint = (nCodePoint << 4) | static_cast<char32_t>(nDigit);
    }

    appendCodePoint(rOutput, nCodePoint);
    rnIndex += 1 + nHexDigits;
    return true;
}

OUString decodeUtf8TestString(std::string_view rInput)
{
    OUStringBuffer aOutput;
    for (std::size_t i = 0; i < rInput.size(); ++i)
    {
        if (rInput[i] == '\\')
        {
            if (appendEscapedCodePoint(aOutput, rInput, i))
                continue;

            if (i + 1 < rInput.size())
            {
                switch (rInput[i + 1])
                {
                    case '\\':
                        aOutput.append(u'\\');
                        ++i;
                        continue;
                    case 't':
                        aOutput.append(u'\t');
                        ++i;
                        continue;
                    case 'n':
                        aOutput.append(u'\n');
                        ++i;
                        continue;
                    default:
                        break;
                }
            }
        }

        const unsigned char nByte = static_cast<unsigned char>(rInput[i]);
        char32_t nCodePoint = 0;
        std::size_t nExtraBytes = 0;
        if ((nByte & 0x80u) == 0)
            nCodePoint = nByte;
        else if ((nByte & 0xE0u) == 0xC0u)
        {
            nCodePoint = nByte & 0x1Fu;
            nExtraBytes = 1;
        }
        else if ((nByte & 0xF0u) == 0xE0u)
        {
            nCodePoint = nByte & 0x0Fu;
            nExtraBytes = 2;
        }
        else if ((nByte & 0xF8u) == 0xF0u)
        {
            nCodePoint = nByte & 0x07u;
            nExtraBytes = 3;
        }
        else
            CPPUNIT_FAIL("invalid UTF-8 lead byte in shared case");

        for (std::size_t j = 0; j < nExtraBytes; ++j)
        {
            CPPUNIT_ASSERT_MESSAGE("truncated UTF-8 sequence in shared case", i + 1 < rInput.size());

            const unsigned char nCont = static_cast<unsigned char>(rInput[++i]);
            CPPUNIT_ASSERT_MESSAGE(
                "invalid UTF-8 continuation byte in shared case", (nCont & 0xC0u) == 0x80u);
            nCodePoint = (nCodePoint << 6) | (nCont & 0x3Fu);
        }

        appendCodePoint(aOutput, nCodePoint);
    }

    return aOutput.makeStringAndClear();
}

double parseDouble(std::string_view rValue)
{
    std::string aString(rValue);
    return std::stod(aString);
}

bool parseBool(std::string_view rValue)
{
    std::string aUpper(rValue);
    for (char& rChar : aUpper)
        rChar = static_cast<char>(std::toupper(static_cast<unsigned char>(rChar)));
    return aUpper == "TRUE" || aUpper == "1" || aUpper == "YES";
}

FormulaError parseExpectedError(std::string_view rValue)
{
    if (rValue.empty())
        return FormulaError::NONE;
    if (rValue == "IllegalArgument")
        return FormulaError::IllegalArgument;
    if (rValue == "DivisionByZero")
        return FormulaError::DivisionByZero;
    if (rValue == "StringOverflow")
        return FormulaError::StringOverflow;
    if (rValue == "NoValue")
        return FormulaError::NoValue;
    if (rValue == "Domain")
        return FormulaError::IllegalFPOperation;
    if (rValue == "NoConvergence")
        return FormulaError::NoConvergence;
    if (rValue == "NotAvailable")
        return FormulaError::NotAvailable;

    CPPUNIT_FAIL("unknown expected error token in shared case");
    return FormulaError::NONE;
}

std::string failSharedCase(const SharedCaseRow& rRow, const std::string& rMessage)
{
    std::ostringstream aStream;
    aStream << rMessage << " (shared case line " << rRow.mnLineNumber << ")";
    return aStream.str();
}

OUString makeDateFormula(std::string_view rToken)
{
    if (rToken.find('-') == std::string_view::npos)
        return OUString::number(parseDouble(rToken));

    const std::string aToken(rToken);
    const std::size_t nDash1 = aToken.find('-');
    const std::size_t nDash2 = aToken.find('-', nDash1 + 1);
    CPPUNIT_ASSERT(nDash1 != std::string::npos && nDash2 != std::string::npos);

    return u"DATE("_ustr
           + OUString::number(std::stoi(aToken.substr(0, nDash1))) + u";"_ustr
           + OUString::number(std::stoi(aToken.substr(nDash1 + 1, nDash2 - nDash1 - 1)))
           + u";"_ustr + OUString::number(std::stoi(aToken.substr(nDash2 + 1))) + u")"_ustr;
}

OUString makeSharedCaseErrorFormula(std::string_view rToken)
{
    if (rToken == "DivisionByZero")
        return u"1/0"_ustr;
    if (rToken == "NotAvailable")
        return u"NA()"_ustr;

    CPPUNIT_FAIL("unknown shared-case error token");
    return OUString();
}

void setTextCell(ScDocument* pDoc, SCCOL nCol, const OUString& rValue)
{
    pDoc->SetTextCell(ScAddress(nCol, 0, 0), rValue);
}

void setValueCell(ScDocument* pDoc, SCCOL nCol, double fValue)
{
    pDoc->SetValue(ScAddress(nCol, 0, 0), fValue);
}

void setCellNumberFormat(ScDocument* pDoc, const ScAddress& rPos, const OUString& rFormat)
{
    SvNumberFormatter* pFormatter = pDoc->GetFormatTable();
    CPPUNIT_ASSERT(pFormatter);

    sal_Int32 nCheckPos = 0;
    SvNumFormatType eType = SvNumFormatType::ALL;
    sal_uInt32 nFormat = 0;
    OUString aFormat = rFormat;
    pFormatter->PutEntry(aFormat, nCheckPos, eType, nFormat);
    pDoc->SetNumberFormat(rPos, nFormat);
}

OUString evaluateFormulaString(ScDocument* pDoc, const OUString& rFormula)
{
    const ScAddress aFormulaPos(5, 0, 0);
    pDoc->SetString(aFormulaPos, rFormula);
    CPPUNIT_ASSERT_EQUAL(FormulaError::NONE, pDoc->GetErrCode(aFormulaPos));
    return pDoc->GetString(aFormulaPos);
}

double evaluateFormulaValue(ScDocument* pDoc, const OUString& rFormula)
{
    const ScAddress aFormulaPos(5, 0, 0);
    pDoc->SetString(aFormulaPos, rFormula);
    CPPUNIT_ASSERT_EQUAL(FormulaError::NONE, pDoc->GetErrCode(aFormulaPos));
    return pDoc->GetValue(aFormulaPos);
}

FormulaError evaluateFormulaError(ScDocument* pDoc, const OUString& rFormula)
{
    const ScAddress aFormulaPos(5, 0, 0);
    pDoc->SetString(aFormulaPos, rFormula);
    return pDoc->GetErrCode(aFormulaPos);
}

double encodeDateTokenAsYmdNumber(std::string_view rToken)
{
    const std::string aToken(rToken);
    const std::size_t nDash1 = aToken.find('-');
    const std::size_t nDash2 = aToken.find('-', nDash1 + 1);
    CPPUNIT_ASSERT(nDash1 != std::string::npos && nDash2 != std::string::npos);

    return static_cast<double>(std::stoi(aToken.substr(0, nDash1)) * 10000
                               + std::stoi(aToken.substr(nDash1 + 1, nDash2 - nDash1 - 1)) * 100
                               + std::stoi(aToken.substr(nDash2 + 1)));
}

std::vector<std::vector<double>> parseNumericMatrixSpec(std::string_view rSpec)
{
    std::vector<std::vector<double>> aMatrix;
    std::size_t nRowStart = 0;
    while (nRowStart <= rSpec.size())
    {
        const std::size_t nRowEnd = rSpec.find('|', nRowStart);
        const std::string_view aRowToken = nRowEnd == std::string_view::npos
                                               ? rSpec.substr(nRowStart)
                                               : rSpec.substr(nRowStart, nRowEnd - nRowStart);

        std::vector<double> aRow;
        std::size_t nValueStart = 0;
        while (nValueStart <= aRowToken.size())
        {
            const std::size_t nValueEnd = aRowToken.find(',', nValueStart);
            const std::string_view aValueToken
                = nValueEnd == std::string_view::npos
                      ? aRowToken.substr(nValueStart)
                      : aRowToken.substr(nValueStart, nValueEnd - nValueStart);
            if (!aValueToken.empty())
                aRow.push_back(parseDouble(aValueToken));

            if (nValueEnd == std::string_view::npos)
                break;

            nValueStart = nValueEnd + 1;
        }

        if (!aRow.empty())
            aMatrix.push_back(std::move(aRow));

        if (nRowEnd == std::string_view::npos)
            break;

        nRowStart = nRowEnd + 1;
    }

    return aMatrix;
}

void setNumberBlock(ScDocument* pDoc, SCCOL nStartCol, SCROW nStartRow,
                    std::initializer_list<std::initializer_list<double>> aRows)
{
    SCROW nRow = nStartRow;
    for (const auto& rRow : aRows)
    {
        SCCOL nCol = nStartCol;
        for (double fValue : rRow)
        {
            pDoc->SetValue(ScAddress(nCol, nRow, 0), fValue);
            ++nCol;
        }
        ++nRow;
    }
}

void assertNumericMatrixResult(ScDocument* pDoc, SCCOL nStartCol, SCROW nStartRow,
                               const std::vector<std::vector<double>>& rExpected,
                               const SharedCaseRow& rRow, const char* pMismatch)
{
    for (std::size_t nRow = 0; nRow < rExpected.size(); ++nRow)
    {
        for (std::size_t nCol = 0; nCol < rExpected[nRow].size(); ++nCol)
        {
            const ScAddress aAddress(static_cast<SCCOL>(nStartCol + nCol),
                                     static_cast<SCROW>(nStartRow + nRow), 0);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(failSharedCase(rRow, pMismatch).c_str(), FormulaError::NONE,
                                         pDoc->GetErrCode(aAddress));
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(failSharedCase(rRow, pMismatch).c_str(),
                                                 rExpected[nRow][nCol], pDoc->GetValue(aAddress),
                                                 1e-9);
        }
    }
}

class TestSharedCases : public ScUcalcTestBase
{
};

CPPUNIT_TEST_FIXTURE(TestSharedCases, testCalcHostAdapter)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"Host"_ustr);

    spreadsheetengine::compat::libreoffice::DocumentEvaluationHost aHost(*m_pDoc, u"en-US");

    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aHost.sheetCount());
    CPPUNIT_ASSERT(aHost.hasSheet(0));
    CPPUNIT_ASSERT(!aHost.hasSheet(1));

    const auto aSheetName = aHost.getSheetName(0);
    CPPUNIT_ASSERT(aSheetName);
    CPPUNIT_ASSERT_EQUAL(u"Host"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aSheetName.maValue));

    const auto aNullDate = aHost.getNullDate();
    CPPUNIT_ASSERT_EQUAL(static_cast<std::int16_t>(1899), aNullDate.mnYear);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::int16_t>(12), aNullDate.mnMonth);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::int16_t>(30), aNullDate.mnDay);
    CPPUNIT_ASSERT_EQUAL(u"en-US"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aHost.getLocaleTag()));

    setValueCell(m_pDoc, 0, 42.5);
    setTextCell(m_pDoc, 1, u"hello"_ustr);
    m_pDoc->SetFormula(ScAddress(2, 0, 0), u"=1/0"_ustr, formula::FormulaGrammar::GRAM_NATIVE_UI);
    setTextCell(m_pDoc, 4, u"42.5"_ustr);

    const auto aNumber = aHost.getCellValue({ 0, 0, 0 });
    CPPUNIT_ASSERT(aNumber);
    CPPUNIT_ASSERT(aNumber.maValue.isNumber());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, aNumber.maValue.mfNumber, 1e-12);

    const auto aText = aHost.getCellValue({ 0, 1, 0 });
    CPPUNIT_ASSERT(aText);
    CPPUNIT_ASSERT(aText.maValue.isText());
    CPPUNIT_ASSERT_EQUAL(u"hello"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aText.maValue.maString));

    const auto aError = aHost.getCellValue({ 0, 2, 0 });
    CPPUNIT_ASSERT(aError);
    CPPUNIT_ASSERT(aError.maValue.isError());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::DivisionByZero, aError.maValue.meError);

    const auto aEmpty = aHost.getCellValue({ 0, 5, 0 });
    CPPUNIT_ASSERT(aEmpty);
    CPPUNIT_ASSERT(aEmpty.maValue.isEmpty());

    const auto aRangeValue = aHost.getRangeValue({ { 0, 0, 0 }, { 0, 1, 0 } }, 1, 0);
    CPPUNIT_ASSERT(aRangeValue);
    CPPUNIT_ASSERT(aRangeValue.maValue.isText());
    CPPUNIT_ASSERT_EQUAL(u"hello"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aRangeValue.maValue.maString));

    const auto aBadOffset = aHost.getRangeValue({ { 0, 0, 0 }, { 0, 1, 0 } }, 2, 0);
    CPPUNIT_ASSERT(!aBadOffset);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::IllegalArgument, aBadOffset.meError);

    const auto aResolved = aHost.resolveReference({ { 0, 0, 0 }, { 0, 1, 0 } });
    CPPUNIT_ASSERT(aResolved);
    CPPUNIT_ASSERT(aResolved.maValue.isNormalized());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aResolved.maValue.matrixDimensions().mnColumns);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aResolved.maValue.matrixDimensions().mnRows);

    const auto aParsedNumber = aHost.parseNumber(u"42.5");
    CPPUNIT_ASSERT(aParsedNumber);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, aParsedNumber.maValue.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::NumberParseResult::Kind::Number, aParsedNumber.maValue.meKind);

    const auto aNotNumber = aHost.parseNumber(u"hello");
    CPPUNIT_ASSERT(!aNotNumber);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::NoValue, aNotNumber.meError);

    const auto aFormattedNumber = aHost.formatNumber(42.5);
    CPPUNIT_ASSERT(aFormattedNumber);
    CPPUNIT_ASSERT(!aFormattedNumber.maValue.empty());

    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    spreadsheetengine::compat::libreoffice::DocumentEvaluationHost aContextHost(*m_pDoc, rContext,
                                                                                 u"en-US");

    const auto aContextParsedNumber = aContextHost.parseNumber(u"42.5");
    CPPUNIT_ASSERT(aContextParsedNumber);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, aContextParsedNumber.maValue.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::NumberParseResult::Kind::Number,
        aContextParsedNumber.maValue.meKind);

    const auto aContextParsedDate = aContextHost.parseNumber(u"1954-07-20");
    CPPUNIT_ASSERT(aContextParsedDate);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::NumberParseResult::Kind::Date, aContextParsedDate.maValue.meKind);

    const auto aContextParsedDateTime = aContextHost.parseNumber(u"1954-07-20 16:30:01");
    CPPUNIT_ASSERT(aContextParsedDateTime);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::NumberParseResult::Kind::DateTime,
        aContextParsedDateTime.maValue.meKind);

    const auto aContextParsedTime = aContextHost.parseNumber(
        u"16:30:01", spreadsheetengine::api::NumberParseMode::LaxTime);
    CPPUNIT_ASSERT(aContextParsedTime);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::NumberParseResult::Kind::Time, aContextParsedTime.maValue.meKind);

    const auto aContextFormattedNumber = aContextHost.formatNumber(42.5);
    CPPUNIT_ASSERT(aContextFormattedNumber);
    CPPUNIT_ASSERT(!aContextFormattedNumber.maValue.empty());

    const auto aScalarView = spreadsheetengine::core::host::readValueView(
        aHost, { { 0, 0, 0 }, { 0, 0, 0 } });
    CPPUNIT_ASSERT(aScalarView);
    CPPUNIT_ASSERT(aScalarView.maValue.isScalar());

    const auto aMatrixView = spreadsheetengine::core::host::readValueView(
        aHost, { { 0, 0, 0 }, { 0, 1, 0 } });
    CPPUNIT_ASSERT(aMatrixView);
    CPPUNIT_ASSERT(aMatrixView.maValue.isMatrixReference());

    const auto aScalarElement
        = spreadsheetengine::core::host::readValueViewElement(aHost, aScalarView.maValue);
    CPPUNIT_ASSERT(aScalarElement);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, aScalarElement.maValue.mfNumber, 1e-12);

    const auto aMatrixElement
        = spreadsheetengine::core::host::readValueViewElement(aHost, aMatrixView.maValue, 1, 0);
    CPPUNIT_ASSERT(aMatrixElement);
    CPPUNIT_ASSERT(aMatrixElement.maValue.isText());
    CPPUNIT_ASSERT_EQUAL(u"hello"_ustr,
                         spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                             aMatrixElement.maValue.maString));

    const auto aCoercedNumber = spreadsheetengine::core::host::coerceToNumber(
        aHost, spreadsheetengine::api::CellValue::text(u"42.5"));
    CPPUNIT_ASSERT(aCoercedNumber);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, aCoercedNumber.maValue.mfValue, 1e-12);

    const auto aCoercedEmpty = spreadsheetengine::core::host::coerceToNumber(
        aHost, spreadsheetengine::api::CellValue::empty());
    CPPUNIT_ASSERT(!aCoercedEmpty);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::NoValue, aCoercedEmpty.meError);

    const auto aFormattedValue = spreadsheetengine::core::host::formatValue(
        aHost, spreadsheetengine::api::CellValue::number(42.5));
    CPPUNIT_ASSERT(aFormattedValue);
    CPPUNIT_ASSERT(!aFormattedValue.maValue.empty());

    const auto aTextNumberView = spreadsheetengine::core::host::readValueView(
        aHost, { { 0, 4, 0 }, { 0, 4, 0 } });
    CPPUNIT_ASSERT(aTextNumberView);
    CPPUNIT_ASSERT(aTextNumberView.maValue.isScalar());

    const auto aCoercedViewNumber = spreadsheetengine::core::host::coerceValueViewElementToNumber(
        aHost, aTextNumberView.maValue);
    CPPUNIT_ASSERT(aCoercedViewNumber);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, aCoercedViewNumber.maValue.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testLocaleParsingSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"LocaleParsing"_ustr);

    for (const auto& rRow : loadSharedCaseRows("locale_parsing_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 5, 5, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "locale parsing shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 6);

        const auto& rFunction = rRow.maColumns[0];
        const auto aInput = decodeUtf8TestString(rRow.maColumns[1]);
        const FormulaError eExpectedError = parseExpectedError(rRow.maColumns[5]);
        setTextCell(m_pDoc, 0, aInput);

        OUString aFormula;
        if (rFunction == "VALUE")
            aFormula = u"=VALUE(A1)"_ustr;
        else if (rFunction == "DATEVALUE")
            aFormula = u"=DATEVALUE(A1)"_ustr;
        else if (rFunction == "TIMEVALUE")
            aFormula = u"=TIMEVALUE(A1)"_ustr;
        else
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown locale parsing function").c_str());

        if (eExpectedError != FormulaError::NONE)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "locale parsing error mismatch").c_str(), eExpectedError,
                evaluateFormulaError(m_pDoc, aFormula));
        }
        else
        {
            const ScAddress aFormulaPos(5, 0, 0);
            m_pDoc->SetString(aFormulaPos, aFormula);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "locale parsing unexpected error").c_str(),
                FormulaError::NONE, m_pDoc->GetErrCode(aFormulaPos));
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "locale parsing value mismatch").c_str(),
                parseDouble(rRow.maColumns[4]), m_pDoc->GetValue(aFormulaPos), 1e-12);
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testDirectTextParsingAdapter)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"DirectTextParsing"_ustr);

    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    spreadsheetengine::compat::libreoffice::textparsingexecution::DirectTextParsingAdapter
        aAdapter(*m_pDoc, rContext, false);

    const auto aValue = aAdapter.evaluateValue(u"42.5"_ustr);
    CPPUNIT_ASSERT(aValue);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.5, aValue.maValue, 1e-12);

    const auto aDateValue = aAdapter.evaluateDateValue(u"1954-07-20"_ustr);
    CPPUNIT_ASSERT(aDateValue);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(19925.0, aDateValue.maValue, 1e-12);

    const auto aTimeValue = aAdapter.evaluateTimeValue(u"16:30:01"_ustr);
    CPPUNIT_ASSERT(aTimeValue);
    CPPUNIT_ASSERT_DOUBLES_EQUAL((16.0 * 3600.0 + 30.0 * 60.0 + 1.0) / 86400.0,
        aTimeValue.maValue, 1e-12);

    const auto aNumberValue
        = aAdapter.evaluateNumberValue(
            u"1,234.5%"_ustr, std::optional(u"."_ustr), std::optional(u","_ustr));
    CPPUNIT_ASSERT(aNumberValue);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(12.345, aNumberValue.maValue, 1e-12);

    const auto aInvalidDate = aAdapter.evaluateDateValue(u"not a date"_ustr);
    CPPUNIT_ASSERT(!aInvalidDate);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::IllegalArgument, aInvalidDate.meError);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    m_pDoc->SetTextCell(ScAddress(0, 0, 0), u"1954-07-20"_ustr);
    m_pDoc->SetTextCell(ScAddress(0, 1, 0), u"07-20"_ustr);
    m_pDoc->SetValue(0, 2, 0, 42.0);
    m_pDoc->SetTextCell(ScAddress(1, 0, 0), u"16:30:01"_ustr);
    m_pDoc->SetTextCell(ScAddress(0, 3, 0), u"1899-12-27 12:00:00"_ustr);
    m_pDoc->SetValue(0, 4, 0, 20.0);
    m_pDoc->SetValue(1, 4, 0, 10.0);
    m_pDoc->SetTextCell(ScAddress(2, 4, 0), u"ten"_ustr);
    m_pDoc->SetValue(1, 5, 0, 20.0);
    m_pDoc->SetTextCell(ScAddress(2, 5, 0), u"twenty"_ustr);
    m_pDoc->SetValue(1, 6, 0, 30.0);
    m_pDoc->SetTextCell(ScAddress(2, 6, 0), u"thirty"_ustr);
    m_pDoc->SetTextCell(ScAddress(0, 7, 0), u"1,234.5"_ustr);
    m_pDoc->SetTextCell(ScAddress(1, 7, 0), u"."_ustr);
    m_pDoc->SetTextCell(ScAddress(2, 7, 0), u","_ustr);
    m_pDoc->SetValue(0, 9, 0, -3.5);
    setCellNumberFormat(m_pDoc, ScAddress(0, 9, 0), u"YYYY-MM-DD HH:MM:SS"_ustr);
    m_pDoc->SetValue(6, 10, 0, -3.5);
    m_pDoc->SetString(0, 10, 0, u"=BASISODATETIME(G11)"_ustr);
    m_pDoc->SetValue(1, 11, 0, 1.0);
    m_pDoc->SetValue(2, 11, 0, 2.0);
    m_pDoc->SetValue(3, 11, 0, 3.0);
    m_pDoc->SetValue(1, 12, 0, 3.0);
    m_pDoc->SetValue(2, 12, 0, 6.0);
    m_pDoc->SetValue(3, 12, 0, 9.0);
    m_pDoc->SetValue(5, 14, 0, 1.0);
    m_pDoc->SetTextCell(ScAddress(6, 14, 0), u"apple"_ustr);
    m_pDoc->SetValue(5, 15, 0, 2.0);
    m_pDoc->SetTextCell(ScAddress(6, 15, 0), u"banana"_ustr);
    m_pDoc->SetValue(5, 16, 0, 3.0);
    m_pDoc->SetTextCell(ScAddress(6, 16, 0), u"cherry"_ustr);
    m_pDoc->SetValue(5, 17, 0, 4.0);
    m_pDoc->SetTextCell(ScAddress(6, 17, 0), u"date"_ustr);
    m_pDoc->SetValue(7, 14, 0, 3.0);
    m_pDoc->SetValue(7, 15, 0, 2.0);
    m_pDoc->SetValue(7, 16, 0, 4.0);
    m_pDoc->SetValue(7, 17, 0, 1.0);
    m_pDoc->SetValue(8, 14, 0, 1.0);
    m_pDoc->SetValue(8, 15, 0, 2.0);
    m_pDoc->SetValue(8, 16, 0, 3.0);
    m_pDoc->SetValue(8, 17, 0, 4.0);
    m_pDoc->SetValue(9, 14, 0, 1.0);
    m_pDoc->SetValue(9, 15, 0, 2.0);
    m_pDoc->SetValue(9, 16, 0, 3.0);
    m_pDoc->SetValue(9, 17, 0, 4.0);
    m_pDoc->SetValue(10, 0, 0, 1.0);
    m_pDoc->SetValue(10, 1, 0, 2.0);
    m_pDoc->SetValue(10, 2, 0, 3.0);
    m_pDoc->SetValue(10, 3, 0, 5.0);
    m_pDoc->SetValue(11, 0, 0, 10.0);
    m_pDoc->SetValue(11, 1, 0, 20.0);
    m_pDoc->SetValue(11, 2, 0, 30.0);
    m_pDoc->SetValue(11, 3, 0, 50.0);
    m_pDoc->SetString(12, 0, 0, u"=L1*10"_ustr);
    m_pDoc->SetString(12, 1, 0, u"=L2*10"_ustr);
    m_pDoc->SetString(12, 2, 0, u"=L3*10"_ustr);
    m_pDoc->SetString(12, 3, 0, u"=L4*10"_ustr);
    m_pDoc->SetValue(20, 0, 0, 0.0);
    m_pDoc->SetValue(20, 1, 0, 1.0);
    m_pDoc->SetValue(20, 2, 0, 2.0);
    m_pDoc->SetValue(20, 3, 0, 3.0);
    m_pDoc->SetValue(21, 0, 0, 0.0);
    m_pDoc->SetValue(21, 1, 0, 11.0);
    m_pDoc->SetValue(21, 2, 0, 22.0);
    m_pDoc->SetValue(21, 3, 0, 33.0);
    m_pDoc->SetString(22, 0, 0, u"=V1&\" 2\""_ustr);
    m_pDoc->SetString(22, 1, 0, u"=V2&\" 2\""_ustr);
    m_pDoc->SetString(22, 2, 0, u"=V3&\" 2\""_ustr);
    m_pDoc->SetString(22, 3, 0, u"=V4&\" 2\""_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"MyTimeName"_ustr, u"$InterpretTailHelper.$B$1"_ustr)));
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"column1"_ustr, u"$InterpretTailHelper.$H$15:$H$18"_ustr)));
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"table"_ustr, u"$InterpretTailHelper.$F$15:$G$18"_ustr)));
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"range"_ustr, u"$InterpretTailHelper.$I$15:$I$18"_ustr)));

    const auto aDate = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=DATEVALUE(A1)", false);
    CPPUNIT_ASSERT(aDate.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aDate.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(19925.0, aDate.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aDate.meFormatType);

    const auto aConcatDate = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=DATEVALUE(\"1954-\"&A2)", false);
    CPPUNIT_ASSERT(aConcatDate.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aConcatDate.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(19925.0, aConcatDate.maResult.mfValue, 1e-12);

    const auto aDateFromDateTimeRef = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 9, 0), u"=DATEVALUE(A10)", false);
    CPPUNIT_ASSERT(aDateFromDateTimeRef.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aDateFromDateTimeRef.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-3.0, aDateFromDateTimeRef.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aDateFromDateTimeRef.meFormatType);

    const auto aDateFromIsoDateTimeText = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 3, 0), u"=DATEVALUE(A4)", false);
    CPPUNIT_ASSERT(aDateFromIsoDateTimeText.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aDateFromIsoDateTimeText.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-3.0, aDateFromIsoDateTimeText.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aDateFromIsoDateTimeText.meFormatType);

    const auto aDateFromMonthNameText = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 14, 0), u"=DATEVALUE(\"Jan1, 2015\")", false);
    CPPUNIT_ASSERT(aDateFromMonthNameText.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aDateFromMonthNameText.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42005.0, aDateFromMonthNameText.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aDateFromMonthNameText.meFormatType);

    const auto aImportedDateFromMonthNameText = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 14, 0), u"=DATEVALUE(\"Jan1, 2015\")", false, nullptr,
        u"of:=DATEVALUE(\"Jan1, 2015\")");
    CPPUNIT_ASSERT(aImportedDateFromMonthNameText.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedDateFromMonthNameText.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedDateFromMonthNameText.maResult.meError);

    const auto aDateFromBasisFormulaRef = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 10, 0), u"=DATEVALUE(A11)", false);
    CPPUNIT_ASSERT(aDateFromBasisFormulaRef.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aDateFromBasisFormulaRef.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-4.0, aDateFromBasisFormulaRef.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aDateFromBasisFormulaRef.meFormatType);

    const auto aTimeFromName = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=TIMEVALUE(MyTimeName)", false);
    CPPUNIT_ASSERT(aTimeFromName.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aTimeFromName.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL((16.0 * 3600.0 + 30.0 * 60.0 + 1.0) / 86400.0,
        aTimeFromName.maResult.mfValue, 1e-12);

    const auto aSignedValue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=VALUE(-A3)", false);
    CPPUNIT_ASSERT(aSignedValue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSignedValue.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-42.0, aSignedValue.maResult.mfValue, 1e-12);

    const auto aSum = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SUM({1;2;3})", false);
    CPPUNIT_ASSERT(aSum.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aSum.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSum.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6.0, aSum.maResult.mfValue, 1e-12);

    const auto aProduct = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=PRODUCT(A5:B5)", false);
    CPPUNIT_ASSERT(aProduct.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aProduct.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aProduct.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(200.0, aProduct.maResult.mfValue, 1e-12);

    const auto aAverage = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=AVERAGE(L1:L4)", false);
    CPPUNIT_ASSERT(aAverage.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aAverage.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aAverage.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.75, aAverage.maResult.mfValue, 1e-12);

    const auto aSumSq = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SUMSQ({\"a\";-4;-5})", false);
    CPPUNIT_ASSERT(aSumSq.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aSumSq.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSumSq.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(41.0, aSumSq.maResult.mfValue, 1e-12);

    const auto aDevSq = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=DEVSQ({1;2;3;4;5})", false);
    CPPUNIT_ASSERT(aDevSq.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aDevSq.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aDevSq.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, aDevSq.maResult.mfValue, 1e-12);

    const auto aMultinomial = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MULTINOMIAL({2;3;4})", false);
    CPPUNIT_ASSERT(aMultinomial.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aMultinomial.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMultinomial.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1260.0, aMultinomial.maResult.mfValue, 1e-12);

    const auto aSumX2My2 = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SUMX2MY2({3;4};{1;2})", false);
    CPPUNIT_ASSERT(aSumX2My2.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aSumX2My2.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSumX2My2.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(20.0, aSumX2My2.maResult.mfValue, 1e-12);

    const auto aDirectReference = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=B5", false);
    CPPUNIT_ASSERT(aDirectReference.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aDirectReference.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, aDirectReference.maResult.mfValue, 1e-12);

    const auto aScalarArithmetic = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=A5+B5", false);
    CPPUNIT_ASSERT(aScalarArithmetic.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aScalarArithmetic.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(30.0, aScalarArithmetic.maResult.mfValue, 1e-12);

    const auto aScalarComparison = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=A5=B6", false);
    CPPUNIT_ASSERT(aScalarComparison.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aScalarComparison.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aScalarComparison.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aScalarComparison.meFormatType);

    const auto aScalarComparisonLess = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=A5<B7", false);
    CPPUNIT_ASSERT(aScalarComparisonLess.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aScalarComparisonLess.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aScalarComparisonLess.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aScalarComparisonLess.meFormatType);

    const auto aScalarConcat = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=\"A\"&\"B\"", false);
    CPPUNIT_ASSERT(aScalarConcat.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aScalarConcat.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"AB"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aScalarConcat.maResult.maString));

    const auto aRound = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ROUND(12.345;2)", false);
    CPPUNIT_ASSERT(aRound.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aRound.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(12.35, aRound.maResult.mfValue, 1e-12);

    m_pDoc->SetValue(34, 0, 0, 3.0);
    const auto aRate = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=RATE(AI1;-10;900)", false);
    CPPUNIT_ASSERT(aRate.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aRate.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.75626593687807, aRate.maResult.mfValue, 1e-12);

    const auto aRoundedRate = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ROUND(RATE(AI1;-10;900);6)", false);
    CPPUNIT_ASSERT(aRoundedRate.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aRoundedRate.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.756266, aRoundedRate.maResult.mfValue, 1e-12);

    const auto aAbs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ABS(-7.25)", false);
    CPPUNIT_ASSERT(aAbs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aAbs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.25, aAbs.maResult.mfValue, 1e-12);

    const auto aDegreesArcCos = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=DEGREES(ACOS(-0.5))", false);
    CPPUNIT_ASSERT(aDegreesArcCos.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aDegreesArcCos.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(120.0, aDegreesArcCos.maResult.mfValue, 1e-12);

    const auto aAtanhOverPi = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ATANH(0)/PI()", false);
    CPPUNIT_ASSERT(aAtanhOverPi.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aAtanhOverPi.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aAtanhOverPi.maResult.mfValue, 1e-12);

    const auto aCeilingMath = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=CEILING.MATH(-5.5;2;-1)", false);
    CPPUNIT_ASSERT(aCeilingMath.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aCeilingMath.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-6.0, aCeilingMath.maResult.mfValue, 1e-12);

    const auto aBitShift = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=BITLSHIFT(6;1)", false);
    CPPUNIT_ASSERT(aBitShift.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aBitShift.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(12.0, aBitShift.maResult.mfValue, 1e-12);

    const auto aMod = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MOD(22;3)", false);
    CPPUNIT_ASSERT(aMod.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMod.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMod.maResult.mfValue, 1e-12);

    const auto aTrunc = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=TRUNC(1.234;2)", false);
    CPPUNIT_ASSERT(aTrunc.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aTrunc.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.23, aTrunc.maResult.mfValue, 1e-12);

    m_pDoc->SetValue(30, 29, 0, 6.0);
    m_pDoc->SetValue(30, 30, 0, 10.0);
    m_pDoc->SetValue(30, 31, 0, 14.0);
    const auto aGcdRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=GCD(AE30:AE32)", false);
    CPPUNIT_ASSERT(aGcdRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aGcdRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aGcdRange.maResult.mfValue, 1e-12);

    const auto aLcmRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=LCM(AE30:AE32)", false);
    CPPUNIT_ASSERT(aLcmRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aLcmRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(210.0, aLcmRange.maResult.mfValue, 1e-12);

    m_pDoc->SetString(31, 29, 0, u"=ABS(-1)"_ustr);
    m_pDoc->SetString(31, 30, 0, u"=MOD(22;3)"_ustr);
    m_pDoc->SetString(31, 31, 0, u"=BITLSHIFT(3;1)"_ustr);
    m_pDoc->SetValue(32, 29, 0, 1.0);
    m_pDoc->SetValue(32, 30, 0, 1.0);
    m_pDoc->SetValue(32, 31, 0, 6.0);
    m_pDoc->SetString(33, 29, 0, u"=AF30=AG30"_ustr);
    m_pDoc->SetString(33, 30, 0, u"=AF31=AG31"_ustr);
    m_pDoc->SetString(33, 31, 0, u"=AF32=AG32"_ustr);
    for (SCROW nRow = 29; nRow <= 31; ++nRow)
    {
        ScFormulaCell* pMath = m_pDoc->GetFormulaCell(ScAddress(31, nRow, 0));
        CPPUNIT_ASSERT(pMath);
        pMath->SetDirty();
        ScFormulaCell* pCompare = m_pDoc->GetFormulaCell(ScAddress(33, nRow, 0));
        CPPUNIT_ASSERT(pCompare);
        pCompare->SetDirty();
    }

    const auto aMathBackedAndRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(34, 29, 0), u"=AND(AH30:AH32)", false);
    CPPUNIT_ASSERT(aMathBackedAndRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMathBackedAndRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMathBackedAndRange.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aMathBackedAndRange.meFormatType);

    const auto aIsError = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ISERROR(1/0)", false);
    CPPUNIT_ASSERT(aIsError.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aIsError.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aIsError.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aIsError.meFormatType);

    const auto aAnd = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=AND(A5=B6;A5<B7)", false);
    CPPUNIT_ASSERT(aAnd.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aAnd.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aAnd.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aAnd.meFormatType);

    const auto aNot = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=NOT(A5=B6)", false);
    CPPUNIT_ASSERT(aNot.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aNot.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aNot.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aNot.meFormatType);

    m_pDoc->SetValue(0, 29, 0, 1.0);
    m_pDoc->SetValue(0, 30, 0, 1.0);
    m_pDoc->SetValue(0, 31, 0, 0.0);
    m_pDoc->SetValue(1, 29, 0, 1.0);
    m_pDoc->SetValue(1, 30, 0, 0.0);
    m_pDoc->SetValue(1, 31, 0, 0.0);

    const auto aAndRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=AND(A30:A32)", false);
    CPPUNIT_ASSERT(aAndRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aAndRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aAndRange.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aAndRange.meFormatType);

    const auto aOrRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=OR(A30:A32)", false);
    CPPUNIT_ASSERT(aOrRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aOrRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aOrRange.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aOrRange.meFormatType);

    const auto aXorRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=XOR(B30:B32)", false);
    CPPUNIT_ASSERT(aXorRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aXorRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aXorRange.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aXorRange.meFormatType);

    m_pDoc->SetString(2, 29, 0, u"=1=1"_ustr);
    m_pDoc->SetString(2, 30, 0, u"=2=2"_ustr);
    const auto aAndBooleanFormulaRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=AND(C30:C32)", false);
    CPPUNIT_ASSERT(aAndBooleanFormulaRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aAndBooleanFormulaRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aAndBooleanFormulaRange.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aAndBooleanFormulaRange.meFormatType);

    const auto aImportedAndRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=AND(A30:A32)", false, nullptr,
        u"of:=AND([.A30:.A32])");
    CPPUNIT_ASSERT(aImportedAndRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedAndRange.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedAndRange.maResult.meError);

    const auto aImportedAbsReference = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ABS(A30)", false, nullptr, u"of:=ABS([.A30])");
    CPPUNIT_ASSERT(aImportedAbsReference.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedAbsReference.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedAbsReference.maResult.meError);

    m_pDoc->SetString(35, 29, 0, u"=1/0"_ustr);
    ScFormulaCell* pImportedError = m_pDoc->GetFormulaCell(ScAddress(35, 29, 0));
    CPPUNIT_ASSERT(pImportedError);
    pImportedError->SetHybridString(
        m_pDoc->GetSharedStringPool().intern(ScGlobal::GetErrorString(FormulaError::DivisionByZero)));
    pImportedError->ResetDirty();

    m_pDoc->SetString(36, 29, 0, u"=NA()"_ustr);
    ScFormulaCell* pImportedNa = m_pDoc->GetFormulaCell(ScAddress(36, 29, 0));
    CPPUNIT_ASSERT(pImportedNa);
    pImportedNa->SetHybridString(
        m_pDoc->GetSharedStringPool().intern(ScGlobal::GetErrorString(FormulaError::NotAvailable)));
    pImportedNa->ResetDirty();

    m_pDoc->SetString(37, 29, 0, u"=\"hello\""_ustr);
    ScFormulaCell* pImportedText = m_pDoc->GetFormulaCell(ScAddress(37, 29, 0));
    CPPUNIT_ASSERT(pImportedText);
    pImportedText->SetHybridString(m_pDoc->GetSharedStringPool().intern(u"hello"_ustr));
    pImportedText->ResetDirty();

    m_pDoc->SetString(38, 29, 0, u"=\"\""_ustr);
    ScFormulaCell* pImportedBlank = m_pDoc->GetFormulaCell(ScAddress(38, 29, 0));
    CPPUNIT_ASSERT(pImportedBlank);
    pImportedBlank->SetHybridEmptyDisplayedAsString();
    pImportedBlank->ResetDirty();

    const auto aImportedIsErrorRef = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ISERROR(AJ30)", false, nullptr,
        u"of:=ISERROR([.AJ30])");
    CPPUNIT_ASSERT(aImportedIsErrorRef.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedIsErrorRef.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedIsErrorRef.maResult.meError);

    const auto aImportedIsErrRef = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ISERR(AJ30)", false, nullptr,
        u"of:=ISERR([.AJ30])");
    CPPUNIT_ASSERT(aImportedIsErrRef.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedIsErrRef.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedIsErrRef.maResult.meError);

    const auto aImportedIsNaRef = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ISNA(AK30)", false, nullptr,
        u"of:=ISNA([.AK30])");
    CPPUNIT_ASSERT(aImportedIsNaRef.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedIsNaRef.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedIsNaRef.maResult.meError);

    const auto aImportedIsTextRef = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ISTEXT(AL30)", false, nullptr,
        u"of:=ISTEXT([.AL30])");
    CPPUNIT_ASSERT(aImportedIsTextRef.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedIsTextRef.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedIsTextRef.maResult.meError);

    const auto aImportedIsNonTextRef = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ISNONTEXT(AL30)", false, nullptr,
        u"of:=ISNONTEXT([.AL30])");
    CPPUNIT_ASSERT(aImportedIsNonTextRef.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedIsNonTextRef.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedIsNonTextRef.maResult.meError);

    const auto aImportedIsBlankRef = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ISBLANK(AM30)", false, nullptr,
        u"of:=ISBLANK([.AM30])");
    CPPUNIT_ASSERT(aImportedIsBlankRef.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aImportedIsBlankRef.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::VariableExpected,
        aImportedIsBlankRef.maResult.meError);

    const auto aRoundUpFractionalDigits = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ROUNDUP(31415.92654;3.3)", false);
    CPPUNIT_ASSERT(aRoundUpFractionalDigits.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aRoundUpFractionalDigits.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(31415.927, aRoundUpFractionalDigits.maResult.mfValue, 1e-12);

    const auto aRoundDownFractionalDigits = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ROUNDDOWN(31415.92654;3.3)", false);
    CPPUNIT_ASSERT(aRoundDownFractionalDigits.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aRoundDownFractionalDigits.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(31415.926, aRoundDownFractionalDigits.maResult.mfValue, 1e-12);

    m_pDoc->SetTextCell(ScAddress(25, 0, 0), u"12 potatoes"_ustr);
    m_pDoc->SetTextCell(ScAddress(25, 1, 0), u"1"_ustr);
    m_pDoc->SetTextCell(ScAddress(25, 2, 0), u"2000-01-01"_ustr);
    const auto aRangeValue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(26, 5, 0), u"=VALUE(Z1:Z3)", false);
    CPPUNIT_ASSERT(aRangeValue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error, aRangeValue.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::IllegalArgument, aRangeValue.maResult.meError);

    const auto aEmptyValue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0), u"=VALUE(A9)", false);
    CPPUNIT_ASSERT(aEmptyValue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aEmptyValue.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aEmptyValue.maResult.mfValue, 1e-12);

    const auto aNumber = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=NUMBERVALUE(A8;B8;C8)", false);
    CPPUNIT_ASSERT(aNumber.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aNumber.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1234.5, aNumber.maResult.mfValue, 1e-12);

    const auto aErrorLiteral = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=of:#N/A", false);
    CPPUNIT_ASSERT(aErrorLiteral.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error, aErrorLiteral.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::NotAvailable, aErrorLiteral.maResult.meError);

    const auto aBracketedErrorLiteral = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=[.OF:.ERR]:502", false);
    CPPUNIT_ASSERT(aBracketedErrorLiteral.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Error,
        aBracketedErrorLiteral.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::IllegalArgument,
        aBracketedErrorLiteral.maResult.meError);

    const auto aMatch = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 4, 0), u"=MATCH(A5;B5:B7;0)", false);
    CPPUNIT_ASSERT(aMatch.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMatch.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aMatch.maResult.mfValue, 1e-12);

    const auto aVLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 4, 0), u"=VLOOKUP(A5;B5:C7;2;0)", false);
    CPPUNIT_ASSERT(aVLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aVLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(aVLookup.maResult.maString));

    const auto aVLookupDuplicateExact = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 4, 0),
        u"=VLOOKUP(2;{1;\"one\"|2;\"first\"|2;\"second\"};2;0)", false);
    CPPUNIT_ASSERT(aVLookupDuplicateExact.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aVLookupDuplicateExact.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"second"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupDuplicateExact.maResult.maString));

    const auto aVLookupExplicitFalse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(4, 4, 0), u"=VLOOKUP(A5;B5:C7;2;FALSE())", false);
    CPPUNIT_ASSERT(aVLookupExplicitFalse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aVLookupExplicitFalse.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupExplicitFalse.maResult.maString));

    const auto aLookupArray = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=LOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"})", false);
    CPPUNIT_ASSERT(aLookupArray.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aLookupArray.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLookupArray.maResult.maString));

    const auto aLookupScalarSearchVector = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=LOOKUP(\"B\";{\"A\"};{\"Andy\";\"Bruce\";\"Charlie\"})", false);
    CPPUNIT_ASSERT(aLookupScalarSearchVector.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aLookupScalarSearchVector.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"Andy"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLookupScalarSearchVector.maResult.maString));

    const auto aLookupArrayFormSquare = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0), u"=LOOKUP(5;{1;2;3|4;5;6|7;8;9})", false);
    CPPUNIT_ASSERT(aLookupArrayFormSquare.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aLookupArrayFormSquare.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6.0, aLookupArrayFormSquare.maResult.mfValue, 1e-12);

    const auto aLookupDifferentDirectionVectors = setaileval::tryEvaluateFormula(*m_pDoc,
        rContext, ScAddress(3, 8, 0), u"=LOOKUP(\"D\";{\"B\";\"C\";\"D\"};{\"X\"|\"Y\"|\"Z\"})",
        false);
    CPPUNIT_ASSERT(aLookupDifferentDirectionVectors.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aLookupDifferentDirectionVectors.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"Z"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLookupDifferentDirectionVectors.maResult.maString));

    const auto aLookupShortResultVector = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=LOOKUP(\"D\";{\"B\";\"C\";\"D\"};{\"X\";\"Y\"})", false);
    CPPUNIT_ASSERT(aLookupShortResultVector.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aLookupShortResultVector.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::NotAvailable,
        aLookupShortResultVector.maResult.meError);

    m_pDoc->SetString(18, 30, 0, u"A"_ustr);
    m_pDoc->SetString(18, 31, 0, u"B"_ustr);
    m_pDoc->SetString(18, 32, 0, u"C"_ustr);
    m_pDoc->SetString(18, 33, 0, u"D"_ustr);
    m_pDoc->SetString(18, 34, 0, u"E"_ustr);
    m_pDoc->SetString(19, 30, 0, u"=CONCATENATE(\"Res\";S31)"_ustr);
    m_pDoc->SetString(19, 31, 0, u"=CONCATENATE(\"Res\";S32)"_ustr);
    m_pDoc->SetString(19, 32, 0, u"=CONCATENATE(\"Res\";S33)"_ustr);
    m_pDoc->SetTextCell(ScAddress(19, 34, 0), u"OUT OF BOUND"_ustr);

    const auto aLookupShortResultRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(20, 30, 0), u"=LOOKUP(\"E\";S31:S35;T31:T33)", false);
    CPPUNIT_ASSERT(aLookupShortResultRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aLookupShortResultRange.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"OUT OF BOUND"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLookupShortResultRange.maResult.maString));

    m_pDoc->SetValue(20, 40, 0, 1.0);
    m_pDoc->SetValue(20, 41, 0, 2.0);
    m_pDoc->SetValue(20, 42, 0, 3.0);
    m_pDoc->SetString(21, 40, 0, u"=CONCATENATE(\"Res\";U41)"_ustr);
    m_pDoc->SetString(21, 41, 0, u"=CONCATENATE(\"Res\";U42)"_ustr);
    m_pDoc->SetString(21, 42, 0, u"=CONCATENATE(\"Res\";U43)"_ustr);

    const auto aLookupFormulaBackedRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(22, 40, 0), u"=LOOKUP(2;U41:V43)", false);
    CPPUNIT_ASSERT(aLookupFormulaBackedRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aLookupFormulaBackedRange.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"Res2"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLookupFormulaBackedRange.maResult.maString));

    for (SCROW nRow = 40; nRow <= 42; ++nRow)
    {
        ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(21, nRow, 0));
        CPPUNIT_ASSERT(pFormula);
        pFormula->SetDirty();
    }

    const auto aLookupDirtyFormulaBackedRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(22, 41, 0), u"=LOOKUP(2;U41:V43)", false);
    CPPUNIT_ASSERT(aLookupDirtyFormulaBackedRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aLookupDirtyFormulaBackedRange.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"Res2"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLookupDirtyFormulaBackedRange.maResult.maString));

    const auto aLookupMatrixArithmetic = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(4, 11, 0), u"=LOOKUP(4;B12:D12*2;B13:D13/3)", false);
    CPPUNIT_ASSERT(aLookupMatrixArithmetic.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aLookupMatrixArithmetic.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aLookupMatrixArithmetic.maResult.mfValue, 1e-12);

    m_pDoc->SetValue(14, 20, 0, 7.0);
    m_pDoc->SetTextCell(ScAddress(15, 20, 0), u"text"_ustr);
    m_pDoc->SetValue(16, 20, 0, 9.0);
    m_pDoc->SetTextCell(ScAddress(17, 20, 0), u"tail"_ustr);

    const auto aXMatchIsNumber = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(18, 20, 0), u"=XMATCH(1;ISNUMBER(O21:R21);0;-1)", false);
    CPPUNIT_ASSERT(aXMatchIsNumber.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchIsNumber.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aXMatchIsNumber.maResult.mfValue, 1e-12);

    m_pDoc->SetTextCell(ScAddress(24, 20, 0), u"Amy"_ustr);
    m_pDoc->SetTextCell(ScAddress(24, 21, 0), u"Beth"_ustr);
    m_pDoc->SetTextCell(ScAddress(24, 22, 0), u"Clara"_ustr);
    m_pDoc->SetTextCell(ScAddress(24, 23, 0), u"Diana"_ustr);
    m_pDoc->SetString(24, 24, 0, u""_ustr);

    const auto aXMatchEmptyLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(25, 20, 0), u"=XMATCH(;Y21:Y25)", false);
    CPPUNIT_ASSERT(aXMatchEmptyLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchEmptyLookup.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, aXMatchEmptyLookup.maResult.mfValue, 1e-12);

    const auto aXMatchTextNextLargerEmpty = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(25, 21, 0), u"=XMATCH(\"Susan\";Y21:Y25;1)", false);
    CPPUNIT_ASSERT(aXMatchTextNextLargerEmpty.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchTextNextLargerEmpty.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, aXMatchTextNextLargerEmpty.maResult.mfValue, 1e-12);

    const auto aNamedVLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(9, 14, 0), u"=VLOOKUP(column1;table;2;0)", false);
    CPPUNIT_ASSERT(aNamedVLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aNamedVLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"cherry"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aNamedVLookup.maResult.maString));

    const auto aIndexNamedColumn = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(13, 14, 0), u"=INDEX(range;2;1)", false);
    CPPUNIT_ASSERT(aIndexNamedColumn.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aIndexNamedColumn.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aIndexNamedColumn.maResult.mfValue, 1e-12);

    const auto aMmultLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(12, 0, 0), u"=LOOKUP(4;MMULT(K1:K4;1);L1:L4)", false);
    CPPUNIT_ASSERT(aMmultLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMmultLookup.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(30.0, aMmultLookup.maResult.mfValue, 1e-12);

    const auto aMmultFormulaResultLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(13, 0, 0), u"=LOOKUP(4;MMULT(K1:K4;1);M1:M4)", false);
    CPPUNIT_ASSERT(aMmultFormulaResultLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMmultFormulaResultLookup.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(300.0, aMmultFormulaResultLookup.maResult.mfValue, 1e-12);

    const auto aMmultArrayLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(12, 1, 0), u"=LOOKUP(4;MMULT(K1:K4;1))", false);
    CPPUNIT_ASSERT(aMmultArrayLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMmultArrayLookup.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aMmultArrayLookup.maResult.mfValue, 1e-12);

    const auto aInvalidMmultLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(12, 2, 0), u"=LOOKUP(4;MMULT(K1:L4;1))", false);
    CPPUNIT_ASSERT(aInvalidMmultLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error,
        aInvalidMmultLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::IllegalArgument, aInvalidMmultLookup.maResult.meError);

    const auto aLookupFormulaBacked2D = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(23, 1, 0), u"=LOOKUP(1;U1:W4)", false);
    CPPUNIT_ASSERT(aLookupFormulaBacked2D.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aLookupFormulaBacked2D.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"11 2"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLookupFormulaBacked2D.maResult.maString));

    for (SCROW nRow = 0; nRow <= 3; ++nRow)
    {
        ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(22, nRow, 0));
        CPPUNIT_ASSERT(pFormula);
        pFormula->SetDirty();
    }

    const auto aLookupDirtyFormulaBacked2D = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(23, 2, 0), u"=LOOKUP(1;U1:W4)", false);
    CPPUNIT_ASSERT(aLookupDirtyFormulaBacked2D.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aLookupDirtyFormulaBacked2D.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"11 2"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLookupDirtyFormulaBacked2D.maResult.maString));

    const auto aMatchArray = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0), u"=MATCH(2;{1;2;3};0)", false);
    CPPUNIT_ASSERT(aMatchArray.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMatchArray.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aMatchArray.maResult.mfValue, 1e-12);

    const auto aMatchRangeLookupValue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 4, 0), u"=MATCH(B5:B7;B5:B7;0)", false);
    CPPUNIT_ASSERT(aMatchRangeLookupValue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchRangeLookupValue.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMatchRangeLookupValue.maResult.mfValue, 1e-12);

    const auto aMatchRangeLookupValueOffRow = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 5, 0), u"=MATCH(B5:B7;B5:B7;0)", false);
    CPPUNIT_ASSERT(aMatchRangeLookupValueOffRow.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchRangeLookupValueOffRow.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMatchRangeLookupValueOffRow.maResult.mfValue, 1e-12);

    const auto aMatchArrayLookupValue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 4, 0), u"=MATCH({1;2;3};{1;2;3};0)", false);
    CPPUNIT_ASSERT(aMatchArrayLookupValue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchArrayLookupValue.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMatchArrayLookupValue.maResult.mfValue, 1e-12);

    const auto aMatchDuplicateExact = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0), u"=MATCH(0;{0;0;1};0)", false);
    CPPUNIT_ASSERT(aMatchDuplicateExact.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchDuplicateExact.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMatchDuplicateExact.maResult.mfValue, 1e-12);

    const auto aMatchDuplicateExactText = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(40, 40, 0), u"=MATCH(\"C\";{\"A\";\"A\";\"B\";\"B\";\"C\";\"C\"};0)",
        false);
    CPPUNIT_ASSERT(aMatchDuplicateExactText.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchDuplicateExactText.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, aMatchDuplicateExactText.maResult.mfValue, 1e-12);

    m_pDoc->SetString(30, 39, 0, u"A"_ustr);
    m_pDoc->SetString(31, 39, 0, u"B"_ustr);
    m_pDoc->SetString(32, 39, 0, u"C"_ustr);
    m_pDoc->SetString(33, 39, 0, u"D"_ustr);
    const auto aMatchWholeRowApprox = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(0, 40, 0), u"=MATCH(AH40;A40:XFD40;1)", false);
    CPPUNIT_ASSERT(aMatchWholeRowApprox.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchWholeRowApprox.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, aMatchWholeRowApprox.maResult.mfValue, 1e-12);

    const auto aMatchArrayApproxAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(1, 40, 0), u"=MATCH(25;{10;20;30};1)", false);
    CPPUNIT_ASSERT(aMatchArrayApproxAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchArrayApproxAscending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aMatchArrayApproxAscending.maResult.mfValue, 1e-12);

    const auto aMatchArrayApproxDefault = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(2, 41, 0), u"=MATCH(25;{10;20;30})", false);
    CPPUNIT_ASSERT(aMatchArrayApproxDefault.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchArrayApproxDefault.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aMatchArrayApproxDefault.maResult.mfValue, 1e-12);

    const auto aMatchArrayApproxDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(2, 40, 0), u"=MATCH(25;{30;20;10};-1)", false);
    CPPUNIT_ASSERT(aMatchArrayApproxDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchArrayApproxDescending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMatchArrayApproxDescending.maResult.mfValue, 1e-12);

    const auto aXMatchArray = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0), u"=XMATCH(2;{1;2;3})", false);
    CPPUNIT_ASSERT(aXMatchArray.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aXMatchArray.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArray.maResult.mfValue, 1e-12);

    const auto aXMatchArrayExplicitExact = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(4, 8, 0), u"=XMATCH(2;{1;2;3};0)", false);
    CPPUNIT_ASSERT(aXMatchArrayExplicitExact.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayExplicitExact.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayExplicitExact.maResult.mfValue, 1e-12);

    const auto aXMatchArrayExplicitForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(5, 8, 0), u"=XMATCH(2;{1;2;3};0;1)", false);
    CPPUNIT_ASSERT(aXMatchArrayExplicitForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayExplicitForward.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayExplicitForward.maResult.mfValue, 1e-12);

    const auto aXMatchArrayExplicitReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 8, 0), u"=XMATCH(2;{1;2;3};0;-1)", false);
    CPPUNIT_ASSERT(aXMatchArrayExplicitReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayExplicitReverse.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayExplicitReverse.maResult.mfValue, 1e-12);

    const auto aXMatchArrayBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(7, 8, 0), u"=XMATCH(2;{1;2;3};0;2)", false);
    CPPUNIT_ASSERT(aXMatchArrayBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayBinaryAscending.maResult.mfValue, 1e-12);

    const auto aXMatchArrayBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(8, 8, 0), u"=XMATCH(2;{3;2;1};0;-2)", false);
    CPPUNIT_ASSERT(aXMatchArrayBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayBinaryDescending.maResult.mfValue, 1e-12);

    const auto aXMatchArrayDefaultForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(9, 8, 0), u"=XMATCH(2;{1;2;3};;1)", false);
    CPPUNIT_ASSERT(aXMatchArrayDefaultForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayDefaultForward.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayDefaultForward.maResult.mfValue, 1e-12);

    const auto aXMatchArrayDefaultReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(10, 8, 0), u"=XMATCH(2;{1;2;3};;-1)", false);
    CPPUNIT_ASSERT(aXMatchArrayDefaultReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayDefaultReverse.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayDefaultReverse.maResult.mfValue, 1e-12);

    const auto aXMatchArrayDefaultBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(11, 8, 0), u"=XMATCH(2;{1;2;3};;2)", false);
    CPPUNIT_ASSERT(aXMatchArrayDefaultBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayDefaultBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayDefaultBinaryAscending.maResult.mfValue, 1e-12);

    const auto aXMatchArrayDefaultBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(12, 8, 0), u"=XMATCH(2;{3;2;1};;-2)", false);
    CPPUNIT_ASSERT(aXMatchArrayDefaultBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayDefaultBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayDefaultBinaryDescending.maResult.mfValue, 1e-12);

    const auto aXMatchArrayApproxNextLarger = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(13, 8, 0), u"=XMATCH(25;{10;20;30};1)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextLarger.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextLarger.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aXMatchArrayApproxNextLarger.maResult.mfValue, 1e-12);

    const auto aXMatchArrayApproxNextSmaller = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(14, 8, 0), u"=XMATCH(25;{10;20;30};-1)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextSmaller.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextSmaller.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayApproxNextSmaller.maResult.mfValue, 1e-12);

    const auto aXMatchArrayApproxNextLargerForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(15, 8, 0), u"=XMATCH(25;{10;20;30};1;1)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextLargerForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextLargerForward.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aXMatchArrayApproxNextLargerForward.maResult.mfValue, 1e-12);

    const auto aXMatchArrayApproxNextSmallerForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(16, 8, 0), u"=XMATCH(25;{10;20;30};-1;1)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextSmallerForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextSmallerForward.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayApproxNextSmallerForward.maResult.mfValue, 1e-12);

    const auto aXMatchArrayApproxNextLargerReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(17, 8, 0), u"=XMATCH(25;{10;20;30};1;-1)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextLargerReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextLargerReverse.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aXMatchArrayApproxNextLargerReverse.maResult.mfValue, 1e-12);

    const auto aXMatchArrayApproxNextSmallerReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(18, 8, 0), u"=XMATCH(25;{10;20;30};-1;-1)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextSmallerReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextSmallerReverse.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayApproxNextSmallerReverse.maResult.mfValue, 1e-12);

    const auto aXMatchArrayApproxNextLargerBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(19, 8, 0), u"=XMATCH(25;{10;20;30};1;2)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextLargerBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextLargerBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aXMatchArrayApproxNextLargerBinaryAscending.maResult.mfValue,
        1e-12);

    const auto aXMatchArrayApproxNextSmallerBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(20, 8, 0), u"=XMATCH(25;{10;20;30};-1;2)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextSmallerBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextSmallerBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayApproxNextSmallerBinaryAscending.maResult.mfValue,
        1e-12);

    const auto aXMatchArrayApproxNextLargerBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(21, 8, 0), u"=XMATCH(25;{30;20;10};1;-2)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextLargerBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextLargerBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aXMatchArrayApproxNextLargerBinaryDescending.maResult.mfValue,
        1e-12);

    const auto aXMatchArrayApproxNextSmallerBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(22, 8, 0), u"=XMATCH(25;{30;20;10};-1;-2)", false);
    CPPUNIT_ASSERT(aXMatchArrayApproxNextSmallerBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchArrayApproxNextSmallerBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aXMatchArrayApproxNextSmallerBinaryDescending.maResult.mfValue,
        1e-12);

    const auto aXMatchRangeLookupValue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 4, 0), u"=XMATCH(B5:B7;B5:B7)", false);
    CPPUNIT_ASSERT(aXMatchRangeLookupValue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aXMatchRangeLookupValue.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aXMatchRangeLookupValue.maResult.mfValue, 1e-12);

    const auto aVLookupArray = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=VLOOKUP(2;{1;\"one\"|2;\"two\"|3;\"three\"};2;0)", false);
    CPPUNIT_ASSERT(aVLookupArray.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aVLookupArray.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupArray.maResult.maString));

    const auto aVLookupApproxDefault = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(4, 8, 0),
        u"=VLOOKUP(25;{10;\"ten\"|20;\"twenty\"|30;\"thirty\"};2)", false);
    CPPUNIT_ASSERT(aVLookupApproxDefault.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aVLookupApproxDefault.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupApproxDefault.maResult.maString));

    const auto aVLookupApproxTrue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(5, 8, 0),
        u"=VLOOKUP(25;{10;\"ten\"|20;\"twenty\"|30;\"thirty\"};2;TRUE())", false);
    CPPUNIT_ASSERT(aVLookupApproxTrue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aVLookupApproxTrue.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupApproxTrue.maResult.maString));

    const auto aVLookupRangeLookupValue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 4, 0), u"=VLOOKUP(B5:B7;B5:C7;2;0)", false);
    CPPUNIT_ASSERT(aVLookupRangeLookupValue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aVLookupRangeLookupValue.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"ten"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupRangeLookupValue.maResult.maString));

    const auto aVLookupArrayLookupValue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 4, 0),
        u"=VLOOKUP({2;3};{1;\"one\"|2;\"two\"|3;\"three\"};2;0)", false);
    CPPUNIT_ASSERT(aVLookupArrayLookupValue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aVLookupArrayLookupValue.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupArrayLookupValue.maResult.maString));

    m_pDoc->SetString(26, 30, 0, u"ABC"_ustr);
    m_pDoc->SetString(27, 30, 0, u"ABC"_ustr);
    m_pDoc->SetString(26, 31, 0, u"abcd"_ustr);
    m_pDoc->SetString(27, 31, 0, u"AB"_ustr);
    m_pDoc->SetString(26, 32, 0, u"cot\u00E9"_ustr);
    m_pDoc->SetString(27, 32, 0, u"ABCD"_ustr);
    m_pDoc->SetString(26, 33, 0, u"c\u00F4te"_ustr);
    m_pDoc->SetString(27, 33, 0, u"cot\u00E9"_ustr);
    m_pDoc->SetString(26, 34, 0, u"c\u00F4t\u00E9"_ustr);
    m_pDoc->SetString(27, 34, 0, u"c\u00F4te"_ustr);
    m_pDoc->SetString(26, 35, 0, u"cote "_ustr);
    m_pDoc->SetString(27, 35, 0, u"c\u00F4t\u00E9"_ustr);
    m_pDoc->SetString(26, 36, 0, u"D\u00FCrst"_ustr);
    m_pDoc->SetString(27, 36, 0, u"cote "_ustr);
    m_pDoc->SetString(26, 37, 0, u"DUERST"_ustr);
    m_pDoc->SetString(27, 37, 0, u"D\u00FCrst"_ustr);

    const auto aVLookupApproxAccent = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(38, 38, 0), u"=VLOOKUP(\"c\u00F4ted\";AA31:AB38;1;1)", false);
    CPPUNIT_ASSERT(aVLookupApproxAccent.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aVLookupApproxAccent.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"cote "_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupApproxAccent.maResult.maString));

    const auto aVLookupApproxDuerst = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(39, 38, 0), u"=VLOOKUP(\"D\u00FCrs\";AA31:AB38;1;1)", false);
    CPPUNIT_ASSERT(aVLookupApproxDuerst.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aVLookupApproxDuerst.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"cote "_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aVLookupApproxDuerst.maResult.maString));

    const auto aHLookupArray = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=HLOOKUP(2;{1;2;3|\"one\";\"two\";\"three\"};2;0)", false);
    CPPUNIT_ASSERT(aHLookupArray.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aHLookupArray.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aHLookupArray.maResult.maString));

    const auto aHLookupArrayFalse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(4, 8, 0),
        u"=HLOOKUP(2;{1;2;3|\"one\";\"two\";\"three\"};2;FALSE())", false);
    CPPUNIT_ASSERT(aHLookupArrayFalse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aHLookupArrayFalse.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aHLookupArrayFalse.maResult.maString));

    const auto aHLookupApproxDefault = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(5, 8, 0),
        u"=HLOOKUP(25;{10;20;30|\"ten\";\"twenty\";\"thirty\"};2)", false);
    CPPUNIT_ASSERT(aHLookupApproxDefault.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aHLookupApproxDefault.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aHLookupApproxDefault.maResult.maString));

    const auto aHLookupApproxTrue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 8, 0),
        u"=HLOOKUP(25;{10;20;30|\"ten\";\"twenty\";\"thirty\"};2;TRUE())", false);
    CPPUNIT_ASSERT(aHLookupApproxTrue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aHLookupApproxTrue.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aHLookupApproxTrue.maResult.maString));

    const auto aIndexArrayTwoArgs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(4, 4, 0), u"=INDEX({1;2|3;4};1)", false);
    CPPUNIT_ASSERT(aIndexArrayTwoArgs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aIndexArrayTwoArgs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aIndexArrayTwoArgs.maResult.mfValue, 1e-12);

    const auto aIndex = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 4, 0), u"=INDEX(B5:C7;2;2)", false);
    CPPUNIT_ASSERT(aIndex.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aIndex.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(aIndex.maResult.maString));

    const auto aIndexArray = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0), u"=INDEX({1;2|3;4};2;2)", false);
    CPPUNIT_ASSERT(aIndexArray.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aIndexArray.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, aIndexArray.maResult.mfValue, 1e-12);

    m_pDoc->SetValue(1, 23, 0, 11.0);
    m_pDoc->SetValue(2, 23, 0, 12.0);
    m_pDoc->SetValue(1, 24, 0, 21.0);
    m_pDoc->SetValue(2, 24, 0, 22.0);

    const auto aIndexReferenceRowSlice = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(0, 23, 0), u"=INDEX(B24:C25;1)", false);
    CPPUNIT_ASSERT(aIndexReferenceRowSlice.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aIndexReferenceRowSlice.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(11.0, aIndexReferenceRowSlice.maResult.mfValue, 1e-12);

    const auto aIndexArrayColumnSlice = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(0, 25, 0), u"=INDEX({1;2|3;4};0;2)", false);
    CPPUNIT_ASSERT(aIndexArrayColumnSlice.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aIndexArrayColumnSlice.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aIndexArrayColumnSlice.maResult.mfValue, 1e-12);

    const auto aIndexArrayRowSlice = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(1, 25, 0), u"=INDEX({1;2|3;4};2;0)", false);
    CPPUNIT_ASSERT(aIndexArrayRowSlice.mbSupported);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aIndexArrayRowSlice.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aIndexArrayRowSlice.maResult.mfValue, 1e-12);

    const auto aXLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 4, 0), u"=XLOOKUP(A5;B5:B7;C5:C7)", false);
    CPPUNIT_ASSERT(aXLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup, aXLookup.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aXLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(aXLookup.maResult.maString));

    const auto aXLookupLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(13, 14, 0),
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\")", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallback.maResult.maString));

    const auto aXLookupArrayExactForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(14, 14, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;0;1)", false);
    CPPUNIT_ASSERT(aXLookupArrayExactForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupArrayExactForward.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArrayExactForward.maResult.maString));

    const auto aXLookupArrayDefaultForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(14, 15, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;;1)", false);
    CPPUNIT_ASSERT(aXLookupArrayDefaultForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupArrayDefaultForward.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArrayDefaultForward.maResult.maString));

    const auto aXLookupArrayExactReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(15, 14, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;0;-1)", false);
    CPPUNIT_ASSERT(aXLookupArrayExactReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupArrayExactReverse.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArrayExactReverse.maResult.maString));

    const auto aXLookupArrayDefaultReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(15, 15, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;;-1)", false);
    CPPUNIT_ASSERT(aXLookupArrayDefaultReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupArrayDefaultReverse.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArrayDefaultReverse.maResult.maString));

    const auto aXLookupArrayBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(16, 14, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;0;2)", false);
    CPPUNIT_ASSERT(aXLookupArrayBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupArrayBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArrayBinaryAscending.maResult.maString));

    const auto aXLookupArrayDefaultBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(16, 15, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;;2)", false);
    CPPUNIT_ASSERT(aXLookupArrayDefaultBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupArrayDefaultBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArrayDefaultBinaryAscending.maResult.maString));

    const auto aXLookupArrayBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(17, 14, 0),
        u"=XLOOKUP(2;{3;2;1};{\"three\";\"two\";\"one\"};;0;-2)", false);
    CPPUNIT_ASSERT(aXLookupArrayBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupArrayBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArrayBinaryDescending.maResult.maString));

    const auto aXLookupArrayDefaultBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(17, 15, 0),
        u"=XLOOKUP(2;{3;2;1};{\"three\";\"two\";\"one\"};;;-2)", false);
    CPPUNIT_ASSERT(aXLookupArrayDefaultBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupArrayDefaultBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArrayDefaultBinaryDescending.maResult.maString));

    const auto aLookupLiteralVector = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(18, 14, 0), u"=LOOKUP(2;{1;2;3})", false);
    CPPUNIT_ASSERT(aLookupLiteralVector.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aLookupLiteralVector.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aLookupLiteralVector.maResult.mfValue, 1e-12);

    const auto aXLookupLiteralFallbackExplicitExact = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(19, 14, 0),
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";0)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackExplicitExact.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackExplicitExact.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackExplicitExact.maResult.maString));

    const auto aXLookupLiteralFallbackForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(20, 14, 0),
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";0;1)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackForward.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackForward.maResult.maString));

    const auto aXLookupLiteralFallbackDefaultForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(20, 15, 0),
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";;1)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackDefaultForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackDefaultForward.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackDefaultForward.maResult.maString));

    const auto aXLookupLiteralFallbackReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(21, 14, 0),
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";0;-1)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackReverse.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackReverse.maResult.maString));

    const auto aXLookupLiteralFallbackDefaultReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(21, 15, 0),
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";;-1)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackDefaultReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackDefaultReverse.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackDefaultReverse.maResult.maString));

    const auto aXLookupLiteralFallbackBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(22, 14, 0),
        u"=XLOOKUP(4;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";0;2)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackBinaryAscending.maResult.maString));

    const auto aXLookupLiteralFallbackDefaultBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(22, 15, 0),
        u"=XLOOKUP(4;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";;2)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackDefaultBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackDefaultBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackDefaultBinaryAscending.maResult.maString));

    const auto aXLookupLiteralFallbackBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(23, 14, 0),
        u"=XLOOKUP(4;{3;2;1};{\"three\";\"two\";\"one\"};\"missing\";0;-2)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackBinaryDescending.maResult.maString));

    const auto aXLookupLiteralFallbackDefaultBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(23, 15, 0),
        u"=XLOOKUP(4;{3;2;1};{\"three\";\"two\";\"one\"};\"missing\";;-2)", false);
    CPPUNIT_ASSERT(aXLookupLiteralFallbackDefaultBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupLiteralFallbackDefaultBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupLiteralFallbackDefaultBinaryDescending.maResult.maString));

    const auto aXLookupApproxNextLarger = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(24, 14, 0),
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLarger.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLarger.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"thirty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLarger.maResult.maString));

    const auto aXLookupApproxNextLargerLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(24, 15, 0),
        u"=XLOOKUP(35;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextSmaller = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(25, 14, 0),
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;-1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmaller.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmaller.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmaller.maResult.maString));

    const auto aXLookupApproxNextSmallerLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(25, 15, 0),
        u"=XLOOKUP(5;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";-1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextLargerForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(26, 14, 0),
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;1;1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerForward.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"thirty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerForward.maResult.maString));

    const auto aXLookupApproxNextLargerForwardLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(26, 15, 0),
        u"=XLOOKUP(35;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";1;1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerForwardLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerForwardLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerForwardLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextSmallerForward = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(27, 14, 0),
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;-1;1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerForward.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerForward.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerForward.maResult.maString));

    const auto aXLookupApproxNextSmallerForwardLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(27, 15, 0),
        u"=XLOOKUP(5;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";-1;1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerForwardLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerForwardLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerForwardLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextLargerReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(28, 14, 0),
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;1;-1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerReverse.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"thirty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerReverse.maResult.maString));

    const auto aXLookupApproxNextLargerReverseLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(28, 15, 0),
        u"=XLOOKUP(35;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";1;-1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerReverseLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerReverseLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerReverseLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextSmallerReverse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(29, 14, 0),
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;-1;-1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerReverse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerReverse.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerReverse.maResult.maString));

    const auto aXLookupApproxNextSmallerReverseLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(29, 15, 0),
        u"=XLOOKUP(5;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";-1;-1)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerReverseLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerReverseLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerReverseLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextLargerBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(30, 14, 0),
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;1;2)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"thirty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerBinaryAscending.maResult.maString));

    const auto aXLookupApproxNextLargerBinaryAscendingLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(30, 15, 0),
        u"=XLOOKUP(35;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";1;2)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerBinaryAscendingLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerBinaryAscendingLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerBinaryAscendingLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextSmallerBinaryAscending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(31, 14, 0),
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;-1;2)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerBinaryAscending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerBinaryAscending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerBinaryAscending.maResult.maString));

    const auto aXLookupApproxNextSmallerBinaryAscendingLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(31, 15, 0),
        u"=XLOOKUP(5;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";-1;2)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerBinaryAscendingLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerBinaryAscendingLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerBinaryAscendingLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextLargerBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(32, 14, 0),
        u"=XLOOKUP(25;{30;20;10};{\"thirty\";\"twenty\";\"ten\"};;1;-2)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"thirty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerBinaryDescending.maResult.maString));

    const auto aXLookupApproxNextLargerBinaryDescendingLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(32, 15, 0),
        u"=XLOOKUP(35;{30;20;10};{\"thirty\";\"twenty\";\"ten\"};\"missing\";1;-2)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextLargerBinaryDescendingLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextLargerBinaryDescendingLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextLargerBinaryDescendingLiteralFallback.maResult.maString));

    const auto aXLookupApproxNextSmallerBinaryDescending = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(33, 14, 0),
        u"=XLOOKUP(25;{30;20;10};{\"thirty\";\"twenty\";\"ten\"};;-1;-2)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerBinaryDescending.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerBinaryDescending.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"twenty"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerBinaryDescending.maResult.maString));

    const auto aXLookupApproxNextSmallerBinaryDescendingLiteralFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(33, 15, 0),
        u"=XLOOKUP(5;{30;20;10};{\"thirty\";\"twenty\";\"ten\"};\"missing\";-1;-2)", false);
    CPPUNIT_ASSERT(aXLookupApproxNextSmallerBinaryDescendingLiteralFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupApproxNextSmallerBinaryDescendingLiteralFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupApproxNextSmallerBinaryDescendingLiteralFallback.maResult.maString));

    const auto aXLookupFallback = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 4, 0),
        u"=XLOOKUP(25;B5:B7;C5:C7;\"missing\")", false);
    CPPUNIT_ASSERT(aXLookupFallback.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup, aXLookupFallback.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aXLookupFallback.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupFallback.maResult.maString));

    const auto aXLookupArray = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"})", false);
    CPPUNIT_ASSERT(aXLookupArray.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup, aXLookupArray.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aXLookupArray.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupArray.maResult.maString));

    const auto aXLookupDescNextSmaller = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=XLOOKUP(4;{6;5;3;2;1};{\"a6\";\"b5\";\"c3\";\"d2\";\"e1\"};;-1;-2)", false);
    CPPUNIT_ASSERT(aXLookupDescNextSmaller.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup,
        aXLookupDescNextSmaller.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupDescNextSmaller.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"c3"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupDescNextSmaller.maResult.maString));

    const auto aXLookupDescNextLarger = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=XLOOKUP(4;{6;5;3;2;1};{\"a6\";\"b5\";\"c3\";\"d2\";\"e1\"};;1;-2)", false);
    CPPUNIT_ASSERT(aXLookupDescNextLarger.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup,
        aXLookupDescNextLarger.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupDescNextLarger.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"b5"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupDescNextLarger.maResult.maString));

    const auto aXLookupDescDuplicateLarger = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=XLOOKUP(5;{10;9;8;7;6;6;4;3;2;1;0};"
        u"{\"CN\";\"IN\";\"US\";\"NG2\";\"ID\";\"BR\";\"PK\";\"NG\";\"BD\";\"RU\";\"MX\"};0;1;-2)",
        false);
    CPPUNIT_ASSERT(aXLookupDescDuplicateLarger.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup,
        aXLookupDescDuplicateLarger.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupDescDuplicateLarger.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"BR"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupDescDuplicateLarger.maResult.maString));

    const auto aXLookupDescTextNextSmaller = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=XLOOKUP(\"D\";{\"F\";\"E\";\"C\";\"B\";\"A\"};{\"fF\";\"eE\";\"cC\";\"bB\";\"aA\"};;-1;-2)",
        false);
    CPPUNIT_ASSERT(aXLookupDescTextNextSmaller.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup,
        aXLookupDescTextNextSmaller.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupDescTextNextSmaller.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"cC"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupDescTextNextSmaller.maResult.maString));

    const auto aXLookupDescTextNextLarger = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=XLOOKUP(\"D\";{\"F\";\"E\";\"C\";\"B\";\"A\"};{\"fF\";\"eE\";\"cC\";\"bB\";\"aA\"};;1;-2)",
        false);
    CPPUNIT_ASSERT(aXLookupDescTextNextLarger.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup,
        aXLookupDescTextNextLarger.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupDescTextNextLarger.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"eE"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupDescTextNextLarger.maResult.maString));

    m_pDoc->SetValue(1, 26, 0, 1.0);
    m_pDoc->SetValue(2, 26, 0, 2.0);
    m_pDoc->SetValue(3, 26, 0, 3.0);
    m_pDoc->SetString(1, 27, 0, u"one"_ustr);
    m_pDoc->SetString(2, 27, 0, u"two"_ustr);
    m_pDoc->SetString(3, 27, 0, u"three"_ustr);
    m_pDoc->SetString(1, 28, 0, u"ONE"_ustr);
    m_pDoc->SetString(2, 28, 0, u"TWO"_ustr);
    m_pDoc->SetString(3, 28, 0, u"THREE"_ustr);

    const auto aXLookupReferenceColumnSlice = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(0, 26, 0), u"=XLOOKUP(2;B27:D27;B28:D29)", false);
    CPPUNIT_ASSERT(aXLookupReferenceColumnSlice.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup, aXLookupReferenceColumnSlice.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::String,
        aXLookupReferenceColumnSlice.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aXLookupReferenceColumnSlice.maResult.maString));

    const auto aNestedXLookupRowSlice = setaileval::tryEvaluateFormula(*m_pDoc, rContext,
        ScAddress(3, 8, 0),
        u"=XLOOKUP(\"Sales\";{\"Product\";\"Sales\";\"Profit\"};XLOOKUP(\"B\";{\"A\"|\"B\"};{10;20;30|40;50;60}))",
        false);
    CPPUNIT_ASSERT(aNestedXLookupRowSlice.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup, aNestedXLookupRowSlice.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aNestedXLookupRowSlice.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(50.0, aNestedXLookupRowSlice.maResult.mfValue, 1e-12);

    const auto aXLookupIndexColumnSlice = setaileval::tryEvaluateFormula(*m_pDoc, rContext,
        ScAddress(3, 8, 0),
        u"=XLOOKUP(2;{1|2|3};INDEX({10;100|20;200|30;300};0;2))", false);
    CPPUNIT_ASSERT(aXLookupIndexColumnSlice.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup, aXLookupIndexColumnSlice.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aXLookupIndexColumnSlice.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(200.0, aXLookupIndexColumnSlice.maResult.mfValue, 1e-12);

    const auto aMatchIndexColumnSlice = setaileval::tryEvaluateFormula(*m_pDoc, rContext,
        ScAddress(3, 8, 0),
        u"=MATCH(200;INDEX({10;100|20;200|30;300};0;2);0)", false);
    CPPUNIT_ASSERT(aMatchIndexColumnSlice.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Match, aMatchIndexColumnSlice.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aMatchIndexColumnSlice.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aMatchIndexColumnSlice.maResult.mfValue, 1e-12);

    const auto aLiteralXMatch = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0), u"=XMATCH(2;{1;2;3})", false);
    CPPUNIT_ASSERT(aLiteralXMatch.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XMatch, aLiteralXMatch.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aLiteralXMatch.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aLiteralXMatch.maResult.mfValue, 1e-12);

    const auto aLiteralLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=LOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"})", false);
    CPPUNIT_ASSERT(aLiteralLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Lookup, aLiteralLookup.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aLiteralLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLiteralLookup.maResult.maString));

    m_pDoc->SetValue(1, 30, 0, 1.0);
    m_pDoc->SetString(2, 30, 0, u"one"_ustr);
    m_pDoc->SetValue(1, 31, 0, 2.0);
    m_pDoc->SetString(2, 31, 0, u"two"_ustr);
    m_pDoc->SetValue(1, 32, 0, 3.0);
    m_pDoc->SetString(2, 32, 0, u"three"_ustr);

    const auto aRangeLookupArrayForm = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0), u"=LOOKUP(2;B31:C33)", false);
    CPPUNIT_ASSERT(aRangeLookupArrayForm.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Lookup, aRangeLookupArrayForm.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aRangeLookupArrayForm.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aRangeLookupArrayForm.maResult.maString));

    const auto aLiteralVLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=VLOOKUP(2;{1;\"one\"|2;\"first\"|2;\"second\"};2;0)", false);
    CPPUNIT_ASSERT(aLiteralVLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::VLookup, aLiteralVLookup.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aLiteralVLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"second"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLiteralVLookup.maResult.maString));

    const auto aLiteralXLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"})", false);
    CPPUNIT_ASSERT(aLiteralXLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup, aLiteralXLookup.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aLiteralXLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLiteralXLookup.maResult.maString));

    const auto aLiteralXLookupExplicitExact = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 8, 0),
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;0)", false);
    CPPUNIT_ASSERT(aLiteralXLookupExplicitExact.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup,
        aLiteralXLookupExplicitExact.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aLiteralXLookupExplicitExact.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"two"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aLiteralXLookupExplicitExact.maResult.maString));

    const auto aIfErrorWrappedLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 4, 0),
        u"=IFERROR(VLOOKUP(25;B5:C7;2;0);\"missing\")", false);
    CPPUNIT_ASSERT(aIfErrorWrappedLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::VLookup, aIfErrorWrappedLookup.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aIfErrorWrappedLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aIfErrorWrappedLookup.maResult.maString));

    const auto aIfNaWrappedXLookup = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(3, 4, 0),
        u"=IFNA(XLOOKUP(25;B5:B7;C5:C7);\"missing\")", false);
    CPPUNIT_ASSERT(aIfNaWrappedXLookup.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::XLookup, aIfNaWrappedXLookup.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String,
        aIfNaWrappedXLookup.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"missing"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aIfNaWrappedXLookup.maResult.maString));

    const auto aTrue = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(4, 4, 0), u"=TRUE()", false);
    CPPUNIT_ASSERT(aTrue.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::LogicalConstant, aTrue.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aTrue.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aTrue.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aTrue.meFormatType);

    const auto aFalse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(5, 4, 0), u"=FALSE()", false);
    CPPUNIT_ASSERT(aFalse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::LogicalConstant, aFalse.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aFalse.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aFalse.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aFalse.meFormatType);

    const auto aLocalizedError = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(6, 4, 0), u"=of:chyba:511", false);
    CPPUNIT_ASSERT(aLocalizedError.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Unknown, aLocalizedError.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error, aLocalizedError.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::IllegalArgument,
        aLocalizedError.maResult.meError);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorNumericAggregateHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailAggregateHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    m_pDoc->SetValue(0, 4, 0, 20.0);
    m_pDoc->SetValue(1, 4, 0, 10.0);
    m_pDoc->SetValue(5, 14, 0, 1.0);
    m_pDoc->SetValue(5, 15, 0, 2.0);
    m_pDoc->SetValue(5, 16, 0, 3.0);
    m_pDoc->SetValue(5, 17, 0, 4.0);
    m_pDoc->SetValue(10, 0, 0, 1.0);
    m_pDoc->SetValue(10, 1, 0, 2.0);
    m_pDoc->SetValue(10, 2, 0, 3.0);
    m_pDoc->SetValue(10, 3, 0, 5.0);

    const auto aSum = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SUM({1;2;3})", false);
    CPPUNIT_ASSERT(aSum.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aSum.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSum.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6.0, aSum.maResult.mfValue, 1e-12);

    const auto aProduct = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=PRODUCT(A5:B5)", false);
    CPPUNIT_ASSERT(aProduct.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aProduct.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aProduct.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(200.0, aProduct.maResult.mfValue, 1e-12);

    const auto aAverage = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=AVERAGE(K1:K4)", false);
    CPPUNIT_ASSERT(aAverage.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aAverage.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aAverage.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.75, aAverage.maResult.mfValue, 1e-12);

    const auto aSumSq = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SUMSQ({\"a\";-4;-5})", false);
    CPPUNIT_ASSERT(aSumSq.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aSumSq.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSumSq.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(41.0, aSumSq.maResult.mfValue, 1e-12);

    const auto aDevSq = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=DEVSQ({1;2;3;4;5})", false);
    CPPUNIT_ASSERT(aDevSq.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aDevSq.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aDevSq.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, aDevSq.maResult.mfValue, 1e-12);

    const auto aMultinomial = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MULTINOMIAL({2;3;4})", false);
    CPPUNIT_ASSERT(aMultinomial.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aMultinomial.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMultinomial.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1260.0, aMultinomial.maResult.mfValue, 1e-12);

    const auto aSumX2My2 = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SUMX2MY2({3;4};{1;2})", false);
    CPPUNIT_ASSERT(aSumX2My2.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::NumericAggregate, aSumX2My2.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSumX2My2.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(20.0, aSumX2My2.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorRankedAggregateHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailRankedAggregateHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetValue(0, 2, 0, 3.0);
    m_pDoc->SetValue(0, 3, 0, 4.0);
    m_pDoc->SetValue(1, 0, 0, 7.0);
    m_pDoc->SetValue(1, 1, 0, 8.0);
    m_pDoc->SetValue(1, 2, 0, 9.0);
    m_pDoc->SetValue(1, 3, 0, 10.0);

    const auto aQuartile = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=QUARTILE(B1:B4;3)", false);
    CPPUNIT_ASSERT(aQuartile.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::RankedAggregate, aQuartile.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aQuartile.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(9.25, aQuartile.maResult.mfValue, 1e-12);

    const auto aQuartileExc = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COM.MICROSOFT.QUARTILE.EXC(B1:B4;3)", false);
    CPPUNIT_ASSERT(aQuartileExc.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::RankedAggregate, aQuartileExc.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aQuartileExc.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(9.75, aQuartileExc.maResult.mfValue, 1e-12);

    const auto aPercentRank = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=PERCENTRANK(A1:A4;3.5;2)", false);
    CPPUNIT_ASSERT(aPercentRank.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::RankedAggregate, aPercentRank.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aPercentRank.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.83, aPercentRank.maResult.mfValue, 1e-12);

    const auto aPercentRankExc = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COM.MICROSOFT.PERCENTRANK.EXC(A1:A4;3.5;2)", false);
    CPPUNIT_ASSERT(aPercentRankExc.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::RankedAggregate, aPercentRankExc.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aPercentRankExc.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.7, aPercentRankExc.maResult.mfValue, 1e-12);

    const auto aLarge = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=LARGE(A1:A4;2)", false);
    CPPUNIT_ASSERT(aLarge.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::RankedAggregate, aLarge.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aLarge.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aLarge.maResult.mfValue, 1e-12);

    const auto aSmall = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SMALL(B1:B4;2)", false);
    CPPUNIT_ASSERT(aSmall.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::RankedAggregate, aSmall.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSmall.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, aSmall.maResult.mfValue, 1e-12);

    const auto aRankEq = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=RANK.EQ(3;A1:A4)", false);
    CPPUNIT_ASSERT(aRankEq.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::RankedAggregate, aRankEq.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aRankEq.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aRankEq.maResult.mfValue, 1e-12);

    const auto aRankAvg = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=RANK.AVG(2;{1;2;2;4})", false);
    CPPUNIT_ASSERT(aRankAvg.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::RankedAggregate, aRankAvg.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aRankAvg.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.5, aRankAvg.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorStatisticalAggregateHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailStatisticalAggregateHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetValue(0, 2, 0, 3.0);
    m_pDoc->SetValue(0, 3, 0, 4.0);

    const auto aMax = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MAX(A1:A4)", false);
    CPPUNIT_ASSERT(aMax.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalAggregate, aMax.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMax.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, aMax.maResult.mfValue, 1e-12);

    const auto aMinA = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MINA({TRUE;2;\"x\"})", false);
    CPPUNIT_ASSERT(aMinA.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalAggregate, aMinA.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMinA.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aMinA.maResult.mfValue, 1e-12);

    const auto aMedian = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MEDIAN({1;9;3;5})", false);
    CPPUNIT_ASSERT(aMedian.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalAggregate, aMedian.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMedian.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, aMedian.maResult.mfValue, 1e-12);

    const auto aGeoMean = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=GEOMEAN({1;4;16})", false);
    CPPUNIT_ASSERT(aGeoMean.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalAggregate, aGeoMean.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aGeoMean.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, aGeoMean.maResult.mfValue, 1e-12);

    const auto aHarMean = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=HARMEAN({1;2;4})", false);
    CPPUNIT_ASSERT(aHarMean.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalAggregate, aHarMean.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aHarMean.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(12.0 / 7.0, aHarMean.maResult.mfValue, 1e-12);

    const auto aVarS = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=VAR.S(A1:A4)", false);
    CPPUNIT_ASSERT(aVarS.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalAggregate, aVarS.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aVarS.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0 / 3.0, aVarS.maResult.mfValue, 1e-12);

    const auto aStdevP = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=STDEV.P(A1:A4)", false);
    CPPUNIT_ASSERT(aStdevP.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalAggregate, aStdevP.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aStdevP.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(std::sqrt(1.25), aStdevP.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorRoundSigHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailRoundSigHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    m_pDoc->SetValue(0, 0, 0, 1234.567);
    m_pDoc->SetValue(1, 0, 0, 3.0);

    const auto aRoundSig = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ROUNDSIG(A1;B1)", false);
    CPPUNIT_ASSERT(aRoundSig.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::MathScalar, aRoundSig.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aRoundSig.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1230.0, aRoundSig.maResult.mfValue, 1e-12);

    const auto aAlias = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ORG.LIBREOFFICE.ROUNDSIG(1234.567;3)", false);
    CPPUNIT_ASSERT(aAlias.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::MathScalar, aAlias.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aAlias.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1230.0, aAlias.maResult.mfValue, 1e-12);

    CPPUNIT_ASSERT(setaileval::isFamilyLocalDefaultOnFormula(u"=ROUNDSIG(1234.567;3)"));
    CPPUNIT_ASSERT(setaileval::isFamilyLocalDefaultOnFormula(
        u"=ORG.LIBREOFFICE.ROUNDSIG(1234.567;3)"));
    CPPUNIT_ASSERT(!setaileval::isFamilyLocalDefaultOnFormula(u"=ABS(12)"));
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorConversionHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailConversionHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    const auto aDistance = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=CONVERT(1;\"m\";\"mi\")", false);
    CPPUNIT_ASSERT(aDistance.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conversion, aDistance.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aDistance.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.000621371192237, aDistance.maResult.mfValue, 1e-12);

    const auto aTemperature = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=CONVERT(2;\"C\";\"F\")", false);
    CPPUNIT_ASSERT(aTemperature.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conversion, aTemperature.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aTemperature.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(35.6, aTemperature.maResult.mfValue, 1e-12);

    const auto aAlias = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ORG.OPENOFFICE.CONVERT(100;\"ATS\";\"EUR\")", false);
    CPPUNIT_ASSERT(aAlias.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conversion, aAlias.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aAlias.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.26728341678597, aAlias.maResult.mfValue, 1e-12);

    const auto aEuroConvert = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=EUROCONVERT(100;\"ATS\";\"EUR\")", false);
    CPPUNIT_ASSERT(aEuroConvert.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conversion, aEuroConvert.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aEuroConvert.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.27, aEuroConvert.maResult.mfValue, 1e-12);

    const auto aEuroConvertFullPrecision = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=EUROCONVERT(100;\"ATS\";\"EUR\";TRUE())", false);
    CPPUNIT_ASSERT(aEuroConvertFullPrecision.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conversion, aEuroConvertFullPrecision.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aEuroConvertFullPrecision.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.26728341678597, aEuroConvertFullPrecision.maResult.mfValue,
        1e-12);

    const auto aEuroConvertPrecision = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=EUROCONVERT(100;\"EUR\";\"SIT\";;3)", false);
    CPPUNIT_ASSERT(aEuroConvertPrecision.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conversion, aEuroConvertPrecision.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aEuroConvertPrecision.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(23964.0, aEuroConvertPrecision.maResult.mfValue, 1e-12);

    CPPUNIT_ASSERT(setaileval::isFamilyLocalDefaultOnFormula(u"=CONVERT(1;\"m\";\"mi\")"));
    CPPUNIT_ASSERT(setaileval::isFamilyLocalDefaultOnFormula(
        u"=ORG.OPENOFFICE.CONVERT(100;\"ATS\";\"EUR\")"));
    CPPUNIT_ASSERT(setaileval::isFamilyLocalDefaultOnFormula(
        u"=EUROCONVERT(100;\"ATS\";\"EUR\")"));
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorAggregateHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailAggregateHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(7, 0, 0);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetValue(0, 2, 0, 4.0);
    m_pDoc->SetValue(0, 3, 0, 8.0);
    m_pDoc->SetValue(0, 4, 0, 16.0);
    m_pDoc->SetValue(0, 5, 0, 32.0);
    m_pDoc->SetRowHidden(2, 4, 0, true);
    m_pDoc->SetValue(1, 0, 0, 10.0);
    m_pDoc->SetValue(1, 1, 0, 30.0);
    m_pDoc->SetString(1, 2, 0, u"=AGGREGATE(9;6;B1:B2)"_ustr);

    const auto aHiddenSum = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COM.MICROSOFT.AGGREGATE(9;5;A1:A6)", false);
    CPPUNIT_ASSERT(aHiddenSum.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Aggregate, aHiddenSum.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aHiddenSum.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(35.0, aHiddenSum.maResult.mfValue, 1e-12);

    const auto aNestedSkip = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=AGGREGATE(1;0;B1:B2;B3)", false);
    CPPUNIT_ASSERT(aNestedSkip.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Aggregate, aNestedSkip.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aNestedSkip.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(20.0, aNestedSkip.maResult.mfValue, 1e-12);

    const auto aScalarSum = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COM.MICROSOFT.AGGREGATE(9;6;3;4;5)", false);
    CPPUNIT_ASSERT(aScalarSum.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Aggregate, aScalarSum.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aScalarSum.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(12.0, aScalarSum.maResult.mfValue, 1e-12);

    const auto aLarge = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COM.MICROSOFT.AGGREGATE(14;6;{1;4;8;16};2)", false);
    CPPUNIT_ASSERT(aLarge.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Aggregate, aLarge.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aLarge.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, aLarge.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorCriteriaAggregateHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailCriteriaAggregateHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(6, 0, 0);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetValue(0, 2, 0, 2.0);
    m_pDoc->SetValue(0, 3, 0, 3.0);
    m_pDoc->SetValue(0, 4, 0, 4.0);
    m_pDoc->SetValue(1, 0, 0, 10.0);
    m_pDoc->SetValue(1, 1, 0, 20.0);
    m_pDoc->SetValue(1, 2, 0, 30.0);
    m_pDoc->SetValue(1, 3, 0, 40.0);
    m_pDoc->SetValue(1, 4, 0, 50.0);
    m_pDoc->SetValue(7, 0, 0, 2.0);
    m_pDoc->SetString(7, 1, 0, u">2"_ustr);

    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(new ScRangeData(
        *m_pDoc, u"CriteriaData"_ustr,
        u"$InterpretTailCriteriaAggregateHelper.$A$1:$A$5"_ustr)));
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(new ScRangeData(
        *m_pDoc, u"ValueData"_ustr,
        u"$InterpretTailCriteriaAggregateHelper.$B$1:$B$5"_ustr)));

    const auto aCountIf = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COUNTIF(A1:A5;H1)", false);
    CPPUNIT_ASSERT(aCountIf.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate, aCountIf.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aCountIf.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aCountIf.maResult.mfValue, 1e-12);

    const auto aCountIfs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COUNTIFS(A1:A5;\">1\";B1:B5;\"<50\")", false);
    CPPUNIT_ASSERT(aCountIfs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate, aCountIfs.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aCountIfs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, aCountIfs.maResult.mfValue, 1e-12);

    const auto aSumIf = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SUMIF(CriteriaData;H1;ValueData)", false);
    CPPUNIT_ASSERT(aSumIf.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate, aSumIf.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aSumIf.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(50.0, aSumIf.maResult.mfValue, 1e-12);

    const auto aAverageIf = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=AVERAGEIF(A1:A5;H2;B1:B5)", false);
    CPPUNIT_ASSERT(aAverageIf.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate, aAverageIf.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aAverageIf.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(45.0, aAverageIf.maResult.mfValue, 1e-12);

    const auto aMaxIfs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MAXIFS(ValueData;CriteriaData;\">1\")", false);
    CPPUNIT_ASSERT(aMaxIfs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate, aMaxIfs.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMaxIfs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(50.0, aMaxIfs.maResult.mfValue, 1e-12);

    const auto aMinIfs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MINIFS(ValueData;CriteriaData;\">1\";ValueData;\">25\")",
        false);
    CPPUNIT_ASSERT(aMinIfs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate, aMinIfs.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMinIfs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(30.0, aMinIfs.maResult.mfValue, 1e-12);

    m_pDoc->SetValue(2, 0, 0, 1.0); // C1
    m_pDoc->SetValue(2, 1, 0, 0.0); // C2
    m_pDoc->SetValue(3, 0, 0, 3.0); // D1
    m_pDoc->SetValue(3, 1, 0, 4.0); // D2
    m_pDoc->SetValue(4, 0, 0, 2.0); // E1
    m_pDoc->SetValue(4, 1, 0, 2.0); // E2
    m_pDoc->SetValue(5, 0, 0, 11.0); // F1
    m_pDoc->SetValue(5, 1, 0, 12.0); // F2
    m_pDoc->SetValue(5, 2, 0, 11.0); // F3
    m_pDoc->SetValue(5, 3, 0, 13.0); // F4
    m_pDoc->SetValue(5, 4, 0, 14.0); // F5

    const auto aArraySumIf = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=SUMIF({-10|10|20|30};\">0\")", false);
    CPPUNIT_ASSERT(aArraySumIf.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate, aArraySumIf.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aArraySumIf.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(60.0, aArraySumIf.maResult.mfValue, 1e-12);

    const auto aMatrixCountIf = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COUNTIF(IF(C1:C2=1;D1:D2;E1:E2);2)", false);
    CPPUNIT_ASSERT(aMatrixCountIf.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate, aMatrixCountIf.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMatrixCountIf.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMatrixCountIf.maResult.mfValue, 1e-12);

    const auto aRangeCriteriaCountIf = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COUNTIF(F1:F5;F1:F5)", false);
    CPPUNIT_ASSERT(aRangeCriteriaCountIf.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate,
        aRangeCriteriaCountIf.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aRangeCriteriaCountIf.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aRangeCriteriaCountIf.maResult.mfValue, 1e-12);

    const auto aMatrixCountIfs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COUNTIFS(IF(C1:C2=1;D1:D2;E1:E2);2)", false);
    CPPUNIT_ASSERT(aMatrixCountIfs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate,
        aMatrixCountIfs.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMatrixCountIfs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aMatrixCountIfs.maResult.mfValue, 1e-12);

    const auto aRangeCriteriaCountIfs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COUNTIFS(F1:F5;F1:F5)", false);
    CPPUNIT_ASSERT(aRangeCriteriaCountIfs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CriteriaAggregate,
        aRangeCriteriaCountIfs.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value,
        aRangeCriteriaCountIfs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aRangeCriteriaCountIfs.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorConditionalHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailConditionalHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(5, 0, 0);

    m_pDoc->SetValue(0, 0, 0, 2.0); // A1
    m_pDoc->SetValue(1, 0, 0, 10.0); // B1
    m_pDoc->SetValue(2, 0, 0, 20.0); // C1
    m_pDoc->SetValue(3, 0, 0, 5.0); // D1
    m_pDoc->SetString(4, 0, 0, u""_ustr); // E1

    const auto aTrueBranch = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=IF(TRUE;42;99)", false);
    CPPUNIT_ASSERT(aTrueBranch.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conditional, aTrueBranch.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aTrueBranch.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42.0, aTrueBranch.maResult.mfValue, 1e-12);

    const auto aFalseBranch = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=IF(FALSE;42;99)", false);
    CPPUNIT_ASSERT(aFalseBranch.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conditional, aFalseBranch.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aFalseBranch.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(99.0, aFalseBranch.maResult.mfValue, 1e-12);

    const auto aOmittedElse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=IF(FALSE;42)", false);
    CPPUNIT_ASSERT(aOmittedElse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conditional, aOmittedElse.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aOmittedElse.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aOmittedElse.maResult.mfValue, 1e-12);

    const auto aLazyElse = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=IF(TRUE;7;1/0)", false);
    CPPUNIT_ASSERT(aLazyElse.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conditional, aLazyElse.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aLazyElse.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.0, aLazyElse.maResult.mfValue, 1e-12);

    const auto aNestedCondition = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=IF(ISBLANK(E1);ABS(B1-C1)<D1;0)", false);
    CPPUNIT_ASSERT(aNestedCondition.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conditional, aNestedCondition.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aNestedCondition.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aNestedCondition.maResult.mfValue, 1e-12);

    const auto aReferenceBranch = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=IF(A1=2;B1;C1)", false);
    CPPUNIT_ASSERT(aReferenceBranch.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::Conditional, aReferenceBranch.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aReferenceBranch.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, aReferenceBranch.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorFormulaTextHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailFormulaTextHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();

    m_pDoc->SetString(0, 0, 0, u"=SUM(1;2)"_ustr); // A1
    m_pDoc->SetValue(0, 1, 0, 7.0); // A2
    m_pDoc->SetString(0, 2, 0, u"=A1*2"_ustr); // A3

    m_pDoc->SetString(1, 0, 0, u"=FORMULA(A1)"_ustr); // B1
    m_pDoc->SetString(1, 2, 0, u"=FORMULA(A1:A3)"_ustr); // B3

    const OUString aExpectedScalar = m_pDoc->GetString(1, 0, 0);
    const OUString aExpectedIntersect = m_pDoc->GetString(1, 2, 0);

    const auto aScalar = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(1, 0, 0), u"=FORMULA(A1)", false);
    CPPUNIT_ASSERT(aScalar.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::FormulaText, aScalar.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aScalar.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(aExpectedScalar,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(aScalar.maResult.maString));

    const auto aNonFormula = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(1, 1, 0), u"=FORMULA(A2)", false);
    CPPUNIT_ASSERT(aNonFormula.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::FormulaText, aNonFormula.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Error, aNonFormula.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::NotAvailable, aNonFormula.maResult.meError);

    const auto aIntersect = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, ScAddress(1, 2, 0), u"=FORMULA(A1:A3)", false);
    CPPUNIT_ASSERT(aIntersect.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::FormulaText, aIntersect.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aIntersect.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(aExpectedIntersect,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(aIntersect.maResult.maString));

    CPPUNIT_ASSERT(setaileval::isFamilyLocalDefaultOnFormula(u"=FORMULA(A1)"));
    CPPUNIT_ASSERT(!setaileval::isFamilyLocalDefaultOnFormula(u"=SUM(1;2)"));
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorTextUtilityHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailTextUtilityHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(6, 0, 0);

    m_pDoc->SetString(0, 0, 0, u"Abc"_ustr); // A1
    m_pDoc->SetString(1, 0, 0, u"  Hello  "_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"abc"_ustr); // C1

    const auto aConcat = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=CONCATENATE(\"ab\";\"cd\")", false);
    CPPUNIT_ASSERT(aConcat.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aConcat.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aConcat.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        u"abcd"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aConcat.maResult.maString));

    const auto aClean = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=CLEAN(\"A\"&CHAR(1)&\"B\")", false);
    CPPUNIT_ASSERT(aClean.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aClean.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aClean.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        u"AB"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aClean.maResult.maString));

    const auto aChar = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=CHAR(65)", false);
    CPPUNIT_ASSERT(aChar.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aChar.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aChar.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        u"A"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aChar.maResult.maString));

    const auto aCode = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=CODE(\"A\")", false);
    CPPUNIT_ASSERT(aCode.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aCode.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aCode.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, aCode.maResult.mfValue, 1e-12);

    const auto aUnichar = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=UNICHAR(9731)", false);
    CPPUNIT_ASSERT(aUnichar.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aUnichar.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aUnichar.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"\u2603"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(aUnichar.maResult.maString));

    const auto aUpper = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=UPPER(A1)", false);
    CPPUNIT_ASSERT(aUpper.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aUpper.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aUpper.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        u"ABC"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aUpper.maResult.maString));

    const auto aLower = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=LOWER(A1)", false);
    CPPUNIT_ASSERT(aLower.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aLower.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aLower.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        u"abc"_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aLower.maResult.maString));

    const auto aLen = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=LEN(B1)", false);
    CPPUNIT_ASSERT(aLen.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aLen.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aLen.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(9.0, aLen.maResult.mfValue, 1e-12);

    const auto aRight = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=RIGHT(\"LibreOffice\";6)", false);
    CPPUNIT_ASSERT(aRight.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aRight.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aRight.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(u"Office"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(aRight.maResult.maString));

    const auto aT = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=T(123)", false);
    CPPUNIT_ASSERT(aT.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aT.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::String, aT.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        u""_ustr, spreadsheetengine::compat::libreoffice::toLibreOfficeString(aT.maResult.maString));

    const auto aExact = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=EXACT(A1;C1)", false);
    CPPUNIT_ASSERT(aExact.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::TextUtility, aExact.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aExact.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, aExact.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorStatisticalDistributionHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailStatisticalDistributionHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(6, 0, 0);

    const auto aFisher
        = setaileval::tryEvaluateFormula(*m_pDoc, rContext, aFormulaPos, u"=FISHER(0.5)", false);
    CPPUNIT_ASSERT(aFisher.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalDistribution, aFisher.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aFisher.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5493061443340549, aFisher.maResult.mfValue, 1e-12);

    const auto aFisherInv = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=FISHERINV(0.5)", false);
    CPPUNIT_ASSERT(aFisherInv.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        setaileval::FunctionKind::StatisticalDistribution, aFisherInv.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aFisherInv.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.46211715726000974, aFisherInv.maResult.mfValue, 1e-12);

    const auto aPoisson = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=POISSON(1;1;FALSE())", false);
    CPPUNIT_ASSERT(aPoisson.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalDistribution, aPoisson.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aPoisson.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.36787944117144233, aPoisson.maResult.mfValue, 1e-12);

    const auto aBinom = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=BINOMDIST(2;5;0.5;FALSE())", false);
    CPPUNIT_ASSERT(aBinom.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalDistribution, aBinom.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aBinom.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.3125, aBinom.maResult.mfValue, 1e-12);

    const auto aRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=BINOM.DIST.RANGE(5;0.5;1;2)", false);
    CPPUNIT_ASSERT(aRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalDistribution, aRange.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.46875, aRange.maResult.mfValue, 1e-12);

    const auto aBeta = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=BETADIST(0.5;2;3)", false);
    CPPUNIT_ASSERT(aBeta.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::StatisticalDistribution, aBeta.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aBeta.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.6875, aBeta.maResult.mfValue, 1e-12);

    const auto aBetaDist = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=BETA.DIST(0.5;2;3;FALSE())", false);
    CPPUNIT_ASSERT(aBetaDist.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        setaileval::FunctionKind::StatisticalDistribution, aBetaDist.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aBetaDist.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.5, aBetaDist.maResult.mfValue, 1e-12);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorBusinessDayHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    ScopedEnvironmentOverride aBusinessDay(
        "SPREADSHEET_ENGINE_INTERPRET_TAIL_ENABLE_BUSINESSDAY", "1");
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailBusinessDayHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);
    m_pDoc->SetString(10, 1, 0, u"=DATE(2014;11;1)"_ustr); // K2
    m_pDoc->SetValue(10, 2, 0, 5.0); // K3
    m_pDoc->SetString(10, 3, 0, u"=DATE(2014;11;2)"_ustr); // K4
    m_pDoc->SetString(10, 4, 0, u"=DATE(2014;11;3)"_ustr); // K5
    m_pDoc->SetString(10, 5, 0, u"=DATE(2014;11;4)"_ustr); // K6
    m_pDoc->SetString(10, 6, 0, u"=DATE(2014;11;30)"_ustr); // K7
    m_pDoc->SetValue(11, 1, 0, 5.0); // L2
    m_pDoc->SetValue(12, 0, 0, 0.0); // M1
    m_pDoc->SetValue(12, 1, 0, 0.0); // M2
    m_pDoc->SetValue(12, 2, 0, 0.0); // M3
    m_pDoc->SetValue(12, 3, 0, 0.0); // M4
    m_pDoc->SetValue(12, 4, 0, 0.0); // M5
    m_pDoc->SetValue(12, 5, 0, 1.0); // M6
    m_pDoc->SetValue(12, 6, 0, 1.0); // M7
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(new ScRangeData(
        *m_pDoc, u"BusinessStart"_ustr, u"$InterpretTailBusinessDayHelper.$K$2"_ustr)));
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(new ScRangeData(
        *m_pDoc, u"BusinessEnd"_ustr, u"$InterpretTailBusinessDayHelper.$K$7"_ustr)));
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(new ScRangeData(
        *m_pDoc, u"BusinessHolidays"_ustr, u"$InterpretTailBusinessDayHelper.$K$4:$K$6"_ustr)));
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(new ScRangeData(
        *m_pDoc, u"BusinessWeekend"_ustr, u"$InterpretTailBusinessDayHelper.$M$1:$M$7"_ustr)));

    const auto aWorkday = setaileval::tryEvaluateFormula(*m_pDoc, rContext, aFormulaPos,
        u"=WORKDAY(DATE(2014;11;1);5;{\"2014-11-2\";\"2014-11-3\";\"2014-11-4\"})", false);
    CPPUNIT_ASSERT(aWorkday.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aWorkday.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aWorkday.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(41954.0, aWorkday.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aWorkday.meFormatType);

    const auto aNetworkDays = setaileval::tryEvaluateFormula(*m_pDoc, rContext, aFormulaPos,
        u"=NETWORKDAYS(DATE(2014;11;1);DATE(2014;11;30);{\"2014-11-11\";\"2014-11-28\";\"2014-11-27\"})",
        false);
    CPPUNIT_ASSERT(aNetworkDays.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aNetworkDays.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aNetworkDays.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(17.0, aNetworkDays.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aNetworkDays.meFormatType);

    const auto aWorkdayIntl = setaileval::tryEvaluateFormula(*m_pDoc, rContext, aFormulaPos,
        u"=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);2;5;{\"2014-11-2\";\"2014-11-3\";\"2014-11-4\"})",
        false);
    CPPUNIT_ASSERT(aWorkdayIntl.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aWorkdayIntl.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aWorkdayIntl.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(41951.0, aWorkdayIntl.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aWorkdayIntl.meFormatType);

    const auto aWorkdayIntlAllWeekend = setaileval::tryEvaluateFormula(*m_pDoc, rContext,
        aFormulaPos,
        u"=COM.MICROSOFT.WORKDAY.INTL(DATE(2014;11;1);2;{1;1;1;1;1;1;1};{\"2014-11-2\"})",
        false);
    CPPUNIT_ASSERT(aWorkdayIntlAllWeekend.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aWorkdayIntlAllWeekend.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Error,
        aWorkdayIntlAllWeekend.maResult.meType);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::IllegalArgument, aWorkdayIntlAllWeekend.maResult.meError);

    const auto aNetworkDaysIntl = setaileval::tryEvaluateFormula(*m_pDoc, rContext, aFormulaPos,
        u"=COM.MICROSOFT.NETWORKDAYS.INTL(DATE(2014;11;1);DATE(2014;11;30);1;{\"2014-11-11\";\"2014-11-28\";\"2014-11-27\"})",
        false);
    CPPUNIT_ASSERT(aNetworkDaysIntl.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aNetworkDaysIntl.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aNetworkDaysIntl.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(17.0, aNetworkDaysIntl.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aNetworkDaysIntl.meFormatType);

    const auto aWorkdayFromRefs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=WORKDAY(K2;K3;K4:K6)", false);
    CPPUNIT_ASSERT(aWorkdayFromRefs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aWorkdayFromRefs.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aWorkdayFromRefs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(41954.0, aWorkdayFromRefs.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aWorkdayFromRefs.meFormatType);

    const auto aWorkdayIntlFromRefs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COM.MICROSOFT.WORKDAY.INTL(K2;2;L2;K4:K6)", false);
    CPPUNIT_ASSERT(aWorkdayIntlFromRefs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aWorkdayIntlFromRefs.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aWorkdayIntlFromRefs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(41951.0, aWorkdayIntlFromRefs.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aWorkdayIntlFromRefs.meFormatType);

    const auto aNetworkDaysFromRefs = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=NETWORKDAYS(K2;K7;K4:K6)", false);
    CPPUNIT_ASSERT(aNetworkDaysFromRefs.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aNetworkDaysFromRefs.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aNetworkDaysFromRefs.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(18.0, aNetworkDaysFromRefs.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aNetworkDaysFromRefs.meFormatType);

    const auto aNetworkDaysIntlFromRange = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos,
        u"=COM.MICROSOFT.NETWORKDAYS.INTL(K2;K7;M1:M7;K4:K6)", false);
    CPPUNIT_ASSERT(aNetworkDaysIntlFromRange.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        setaileval::FunctionKind::BusinessDay, aNetworkDaysIntlFromRange.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aNetworkDaysIntlFromRange.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(18.0, aNetworkDaysIntlFromRange.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aNetworkDaysIntlFromRange.meFormatType);

    const auto aNetworkDaysIntlBinaryEnd = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=COM.MICROSOFT.NETWORKDAYS.INTL(K2;K2+6;M1:M7)", false);
    CPPUNIT_ASSERT(aNetworkDaysIntlBinaryEnd.mbSupported);
    CPPUNIT_ASSERT_EQUAL(
        setaileval::FunctionKind::BusinessDay, aNetworkDaysIntlBinaryEnd.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aNetworkDaysIntlBinaryEnd.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, aNetworkDaysIntlBinaryEnd.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aNetworkDaysIntlBinaryEnd.meFormatType);

    const auto aNamedNetworkDaysIntl = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos,
        u"=COM.MICROSOFT.NETWORKDAYS.INTL(BusinessStart;BusinessEnd;BusinessWeekend;BusinessHolidays)",
        false);
    CPPUNIT_ASSERT(aNamedNetworkDaysIntl.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::BusinessDay, aNamedNetworkDaysIntl.meFunction);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::formulavalue::ValueType::Value,
        aNamedNetworkDaysIntl.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(18.0, aNamedNetworkDaysIntl.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aNamedNetworkDaysIntl.meFormatType);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorCalendarUtilityHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailCalendarUtilityHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    m_pDoc->SetString(0, 0, 0, u"2016-02-11"_ustr);
    m_pDoc->SetString(0, 1, 0, u"2016-01-04"_ustr);
    m_pDoc->SetString(0, 2, 0, u"2015-01-01"_ustr);
    m_pDoc->SetValue(1, 0, 0, 2015.0);

    const auto aDaysInMonth = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ORG.OPENOFFICE.DAYSINMONTH(A1)", false);
    CPPUNIT_ASSERT(aDaysInMonth.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CalendarUtility, aDaysInMonth.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aDaysInMonth.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(29.0, aDaysInMonth.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aDaysInMonth.meFormatType);

    const auto aDaysInYear = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ORG.OPENOFFICE.DAYSINYEAR(A1)", false);
    CPPUNIT_ASSERT(aDaysInYear.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CalendarUtility, aDaysInYear.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aDaysInYear.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(366.0, aDaysInYear.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aDaysInYear.meFormatType);

    const auto aIsLeapYear = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ORG.OPENOFFICE.ISLEAPYEAR(A1)", false);
    CPPUNIT_ASSERT(aIsLeapYear.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CalendarUtility, aIsLeapYear.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aIsLeapYear.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aIsLeapYear.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::LOGICAL, aIsLeapYear.meFormatType);

    const auto aIsoWeek = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ISOWEEKNUM(A2)", false);
    CPPUNIT_ASSERT(aIsoWeek.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CalendarUtility, aIsoWeek.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aIsoWeek.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aIsoWeek.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aIsoWeek.meFormatType);

    const auto aEasterSunday = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ORG.OPENOFFICE.EASTERSUNDAY(B1)", false);
    CPPUNIT_ASSERT(aEasterSunday.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CalendarUtility, aEasterSunday.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aEasterSunday.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(42099.0, aEasterSunday.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aEasterSunday.meFormatType);

    const auto aWeeksInYear = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ORG.OPENOFFICE.WEEKSINYEAR(A3)", false);
    CPPUNIT_ASSERT(aWeeksInYear.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::CalendarUtility, aWeeksInYear.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aWeeksInYear.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(53.0, aWeeksInYear.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aWeeksInYear.meFormatType);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorDateDifferenceHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailDateDifferenceHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    m_pDoc->SetString(0, 0, 0, u"2010-04-03"_ustr);
    m_pDoc->SetString(1, 0, 0, u"2011-06-17"_ustr);
    m_pDoc->SetString(0, 1, 0, u"2021-11-14"_ustr);
    m_pDoc->SetString(1, 1, 0, u"2021-11-15"_ustr);
    m_pDoc->SetValue(2, 0, 0, 0.0);
    m_pDoc->SetValue(2, 1, 0, 1.0);

    const auto aMonths = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=ORG.OPENOFFICE.MONTHS(A1;B1;C1)", false);
    CPPUNIT_ASSERT(aMonths.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::DateDifference, aMonths.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMonths.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(14.0, aMonths.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aMonths.meFormatType);

    const auto aYears = setaileval::tryEvaluateFormula(*m_pDoc, rContext, aFormulaPos,
        u"=ORG.OPENOFFICE.YEARS(DATE(2014;1;15);DATE(2016;4;1);0)", false);
    CPPUNIT_ASSERT(aYears.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::DateDifference, aYears.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aYears.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aYears.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aYears.meFormatType);

    const auto aWeeks = setaileval::tryEvaluateFormula(*m_pDoc, rContext, aFormulaPos,
        u"=ORG.OPENOFFICE.WEEKS(A2;B2;C2)", false);
    CPPUNIT_ASSERT(aWeeks.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::DateDifference, aWeeks.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aWeeks.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aWeeks.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aWeeks.meFormatType);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorDateConstructExtractHelper)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;

    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"InterpretTailDateConstructExtractHelper"_ustr);
    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    const ScAddress aFormulaPos(3, 0, 0);

    m_pDoc->SetValue(0, 0, 0, 2020.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->SetValue(2, 0, 0, 29.0);
    m_pDoc->SetString(0, 1, 0, u"2016-02-11"_ustr);

    const auto aDate = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=DATE(A1;B1;C1)", false);
    CPPUNIT_ASSERT(aDate.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::DateConstructExtract, aDate.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aDate.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(43890.0, aDate.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::DATE, aDate.meFormatType);

    const auto aYear = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=YEAR(DATE(2016;2;11))", false);
    CPPUNIT_ASSERT(aYear.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::DateConstructExtract, aYear.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aYear.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2016.0, aYear.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aYear.meFormatType);

    const auto aMonth = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=MONTH(A2)", false);
    CPPUNIT_ASSERT(aMonth.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::DateConstructExtract, aMonth.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aMonth.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aMonth.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aMonth.meFormatType);

    const auto aDay = setaileval::tryEvaluateFormula(
        *m_pDoc, rContext, aFormulaPos, u"=DAY(A2)", false);
    CPPUNIT_ASSERT(aDay.mbSupported);
    CPPUNIT_ASSERT_EQUAL(setaileval::FunctionKind::DateConstructExtract, aDay.meFunction);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::formulavalue::ValueType::Value, aDay.maResult.meType);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(11.0, aDay.maResult.mfValue, 1e-12);
    CPPUNIT_ASSERT_EQUAL(SvNumFormatType::NUMBER, aDay.meFormatType);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testInterpretTailEngineEvaluatorSourceNormalization)
{
    namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;
    using spreadsheetengine::compat::libreoffice::toLibreOfficeString;

    CPPUNIT_ASSERT_EQUAL(
        u"of:=DATEVALUE([.A1])"_ustr,
        toLibreOfficeString(setaileval::detail::normalizeFormulaSource(
            u"oooc:=DATEVALUE([.A1])")));
    CPPUNIT_ASSERT_EQUAL(
        u"of:=VLOOKUP([.A1];[.B1:.C2];2;0)"_ustr,
        toLibreOfficeString(setaileval::detail::normalizeFormulaSource(
            u"oooc:=VLOOKUP([.A1];[.B1:.C2];2;0)")));
    CPPUNIT_ASSERT_EQUAL(
        u"of:=COM.MICROSOFT.XLOOKUP([.A1];[.B1:.B3];[.C1:.C3])"_ustr,
        toLibreOfficeString(setaileval::detail::normalizeFormulaSource(
            u"oooc:=COM.MICROSOFT.XLOOKUP([.A1];[.B1:.B3];[.C1:.C3])")));
    CPPUNIT_ASSERT_EQUAL(
        u"of:=VALUE(\"4321\")"_ustr,
        toLibreOfficeString(
            setaileval::detail::normalizeFormulaSource(u"=VALUE(\"4321\")")));
    CPPUNIT_ASSERT_EQUAL(
        u"of:#ERR511!"_ustr,
        toLibreOfficeString(
            setaileval::detail::normalizeFormulaSource(u"=of:chyba:511")));
    CPPUNIT_ASSERT_EQUAL(
        u"of:#ERR504!"_ustr,
        toLibreOfficeString(
            setaileval::detail::normalizeFormulaSource(u"of:Err:504")));
    const auto aParse = spreadsheetengine::core::formula::parseFormula(
        setaileval::detail::normalizeFormulaSource(
            u"=IFERROR(VLOOKUP(A1;B1:C3;2;0);\"missing\")"));
    CPPUNIT_ASSERT(aParse && aParse.mpRoot);
    CPPUNIT_ASSERT_EQUAL(
        setaileval::FunctionKind::VLookup,
        setaileval::detail::classifyDelegatedFunctionNode(*aParse.mpRoot));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=TRUE()"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=FALSE()"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=VALUE(\"4321\")"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=DATEVALUE(\"1954-07-20\")"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=TIMEVALUE(\"16:30:01\")"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=NUMBERVALUE(\"1,234.5\";\".\";\",\")"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=MATCH(2;{1;2;3};0)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=MATCH(\"C\";{\"A\";\"A\";\"B\";\"B\";\"C\";\"C\"};0)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=MATCH(25;{10;20;30};1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=MATCH(25;{10;20;30})"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=MATCH(25;{30;20;10};-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{1;2;3})"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{1;2;3};0)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{1;2;3};0;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{1;2;3};0;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{1;2;3};0;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{3;2;1};0;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{1;2;3};;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{1;2;3};;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{1;2;3};;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(2;{3;2;1};;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{10;20;30};1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{10;20;30};-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{10;20;30};1;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{10;20;30};-1;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{10;20;30};1;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{10;20;30};-1;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{10;20;30};1;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{10;20;30};-1;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{30;20;10};1;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=XMATCH(25;{30;20;10};-1;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=LOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"})"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=LOOKUP(2;{1;2;3})"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=LOOKUP(5;{1;2;3|4;5;6|7;8;9})"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=VLOOKUP(2;{1;\"one\"|2;\"first\"|2;\"second\"};2;0)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=VLOOKUP(2;{1;\"one\"|2;\"first\"|2;\"second\"};2;FALSE())"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=VLOOKUP(25;{10;\"ten\"|20;\"twenty\"|30;\"thirty\"};2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=VLOOKUP(25;{10;\"ten\"|20;\"twenty\"|30;\"thirty\"};2;TRUE())"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=HLOOKUP(2;{1;2;3|\"one\";\"two\";\"three\"};2;0)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=HLOOKUP(2;{1;2;3|\"one\";\"two\";\"three\"};2;FALSE())"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=HLOOKUP(25;{10;20;30|\"ten\";\"twenty\";\"thirty\"};2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=HLOOKUP(25;{10;20;30|\"ten\";\"twenty\";\"thirty\"};2;TRUE())"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"})"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\")"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;0)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";0)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;0;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";0;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;0;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";0;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;0;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{1;2;3};{\"one\";\"two\";\"three\"};;;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(4;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";0;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(4;{1;2;3};{\"one\";\"two\";\"three\"};\"missing\";;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{3;2;1};{\"three\";\"two\";\"one\"};;0;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(2;{3;2;1};{\"three\";\"two\";\"one\"};;;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(4;{3;2;1};{\"three\";\"two\";\"one\"};\"missing\";0;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(4;{3;2;1};{\"three\";\"two\";\"one\"};\"missing\";;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(35;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(5;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;1;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(35;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";1;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;-1;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(5;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";-1;1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;1;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(35;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";1;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;-1;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(5;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";-1;-1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;1;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(35;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";1;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};;-1;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(5;{10;20;30};{\"ten\";\"twenty\";\"thirty\"};\"missing\";-1;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{30;20;10};{\"thirty\";\"twenty\";\"ten\"};;1;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(35;{30;20;10};{\"thirty\";\"twenty\";\"ten\"};\"missing\";1;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(25;{30;20;10};{\"thirty\";\"twenty\";\"ten\"};;-1;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(
        u"=XLOOKUP(5;{30;20;10};{\"thirty\";\"twenty\";\"ten\"};\"missing\";-1;-2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=INDEX({1;2|3;4};2;2)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=INDEX({1;2|3;4};1)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=INDEX({1;2|3;4};2;0)"));
    CPPUNIT_ASSERT(setaileval::isHardRoutedFormula(u"=INDEX({1;2|3;4};0;2)"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=TRUE(1)"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=MATCH(2;A1:A3;0)"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=MATCH(2;{1;2;3})"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=XMATCH(2;A1:A3)"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=XLOOKUP(2;A1:A3;B1:B3)"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=INDEX(A1:B2;2;2)"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=VALUE(A1)"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=DATEVALUE(A1)"));
    CPPUNIT_ASSERT(!setaileval::isHardRoutedFormula(u"=TIMEVALUE(MyTimeName)"));
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testDirectFormulaInspectionAdapter)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"DirectFormulaInspection"_ustr);

    m_pDoc->SetString(0, 0, 0, u"=1+1"_ustr);
    setValueCell(m_pDoc, 1, 7.0);

    ScInterpreterContext& rContext = m_pDoc->GetNonThreadedContext();
    spreadsheetengine::compat::libreoffice::formulainspection::DirectFormulaInspectionAdapter
        aAdapter(*m_pDoc, rContext);

    CPPUNIT_ASSERT(aAdapter.isFormulaCell(ScAddress(0, 0, 0)));
    CPPUNIT_ASSERT(!aAdapter.isFormulaCell(ScAddress(1, 0, 0)));

    const auto aFormulaText = aAdapter.formulaTextForCell(ScAddress(0, 0, 0));
    CPPUNIT_ASSERT(aFormulaText);
    CPPUNIT_ASSERT_EQUAL(u"=1+1"_ustr, aFormulaText.maValue);

    const auto aMissingFormulaText = aAdapter.formulaTextForCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(!aMissingFormulaText);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::api::Error::NotAvailable, aMissingFormulaText.meError);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testDirectCellInspectionAdapter)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"DirectCellInspection"_ustr);
    m_pDoc->InsertTab(1, u"bar"_ustr);

    setValueCell(m_pDoc, 0, 42.5);
    setTextCell(m_pDoc, 1, u"hello"_ustr);

    const ScAddress aFormulaPos(0, 0, 0);
    const ScAddress aValuePos(0, 0, 0);
    const ScAddress aTextPos(1, 0, 0);
    const ScAddress aOtherSheetPos(2, 9, 1);

    spreadsheetengine::compat::libreoffice::cellinspectionexecution::
        DirectCellInspectionAdapter aAdapter(
            *m_pDoc, aFormulaPos, formula::FormulaGrammar::CONV_OOO);

    auto getCellValue = [this](const ScAddress& rPos,
                            spreadsheetengine::compat::libreoffice::HostCellStringKind eKind
                                = spreadsheetengine::compat::libreoffice::HostCellStringKind::Raw)
    {
        const auto aValue
            = spreadsheetengine::compat::libreoffice::readHostDocumentCellValue(*m_pDoc, rPos, eKind);
        CPPUNIT_ASSERT(aValue);
        return aValue.maValue;
    };

    const auto aColumn = aAdapter.evaluateLocalInfo(
        u"COL"_ustr, aValuePos, getCellValue(aValuePos));
    CPPUNIT_ASSERT(aColumn.mbHandled);
    CPPUNIT_ASSERT(aColumn.maResult);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aColumn.maResult.maValue.mfNumber, 1e-12);

    const auto aAddress = aAdapter.evaluateLocalInfo(
        u"ADDRESS"_ustr, aValuePos, getCellValue(aValuePos));
    CPPUNIT_ASSERT(aAddress.mbHandled);
    CPPUNIT_ASSERT(aAddress.maResult);
    CPPUNIT_ASSERT_EQUAL(u"$A$1"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aAddress.maResult.maValue.maString));

    const auto aContents = aAdapter.evaluateLocalInfo(
        u"CONTENTS"_ustr, aTextPos,
        getCellValue(aTextPos, spreadsheetengine::compat::libreoffice::HostCellStringKind::Display));
    CPPUNIT_ASSERT(aContents.mbHandled);
    CPPUNIT_ASSERT(aContents.maResult);
    CPPUNIT_ASSERT_EQUAL(u"hello"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aContents.maResult.maValue.maString));

    const auto aType = aAdapter.evaluateLocalInfo(
        u"TYPE"_ustr, aTextPos,
        getCellValue(aTextPos, spreadsheetengine::compat::libreoffice::HostCellStringKind::Display));
    CPPUNIT_ASSERT(aType.mbHandled);
    CPPUNIT_ASSERT(aType.maResult);
    CPPUNIT_ASSERT_EQUAL(u"l"_ustr,
        spreadsheetengine::compat::libreoffice::toLibreOfficeString(
            aType.maResult.maValue.maString));

    const auto aSheet = aAdapter.evaluateLocalInfo(
        u"SHEET"_ustr, aOtherSheetPos,
        getCellValue(aOtherSheetPos,
            spreadsheetengine::compat::libreoffice::HostCellStringKind::Display));
    CPPUNIT_ASSERT(aSheet.mbHandled);
    CPPUNIT_ASSERT(aSheet.maResult);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aSheet.maResult.maValue.mfNumber, 1e-12);

    const auto aDeferred = aAdapter.evaluateLocalInfo(
        u"WIDTH"_ustr, aValuePos, getCellValue(aValuePos));
    CPPUNIT_ASSERT(!aDeferred.mbHandled);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::compat::libreoffice::cellinspectionexecution::InfoKind::Width,
        aDeferred.meKind);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testLogicSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    for (const auto& rRow : loadSharedCaseRows("logic_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 5, 5, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "logic shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 8);

        const auto& rFunction = rRow.maColumns[0];
        const auto aExpectedValue = decodeUtf8TestString(rRow.maColumns[6]);
        const FormulaError eExpectedError = parseExpectedError(rRow.maColumns[7]);
        OUString aFormula;

        if (rFunction == "IF")
        {
            setTextCell(m_pDoc, 1, decodeUtf8TestString(rRow.maColumns[2]));
            setTextCell(m_pDoc, 2, decodeUtf8TestString(rRow.maColumns[3]));
            aFormula = parseBool(rRow.maColumns[1]) ? u"=IF(TRUE;B1;C1)"_ustr
                                                    : u"=IF(FALSE;B1;C1)"_ustr;
        }
        else if (rFunction == "IF2")
        {
            setTextCell(m_pDoc, 1, decodeUtf8TestString(rRow.maColumns[2]));
            aFormula = parseBool(rRow.maColumns[1]) ? u"=IF(TRUE;B1)"_ustr
                                                    : u"=IF(FALSE;B1)"_ustr;
        }
        else if (rFunction == "IFERROR" || rFunction == "IFNA")
        {
            setTextCell(m_pDoc, 2, decodeUtf8TestString(rRow.maColumns[3]));
            const OUString aPrimaryExpr = rRow.maColumns[1].empty()
                                              ? (setTextCell(
                                                     m_pDoc, 1,
                                                     decodeUtf8TestString(rRow.maColumns[2])),
                                                  u"B1"_ustr)
                                              : makeSharedCaseErrorFormula(rRow.maColumns[1]);

            aFormula = OUString::Concat(u"=")
                       + (rFunction == "IFNA" ? u"IFNA("_ustr : u"IFERROR("_ustr) + aPrimaryExpr
                       + u";C1)"_ustr;
        }
        else if (rFunction == "CHOOSE")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setTextCell(m_pDoc, 1, decodeUtf8TestString(rRow.maColumns[2]));
            setTextCell(m_pDoc, 2, decodeUtf8TestString(rRow.maColumns[3]));
            setTextCell(m_pDoc, 3, decodeUtf8TestString(rRow.maColumns[4]));
            aFormula = u"=CHOOSE(A1;B1;C1;D1)"_ustr;
        }
        else if (rFunction == "IFS")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            setTextCell(m_pDoc, 2, decodeUtf8TestString(rRow.maColumns[3]));
            setValueCell(m_pDoc, 3, parseDouble(rRow.maColumns[4]));
            setTextCell(m_pDoc, 4, decodeUtf8TestString(rRow.maColumns[5]));
            aFormula = u"=IFS(A1=B1;C1;A1=D1;E1)"_ustr;
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown logic shared-case function").c_str());
        }

        if (eExpectedError != FormulaError::NONE)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "logic error mismatch").c_str(), eExpectedError,
                evaluateFormulaError(m_pDoc, aFormula));
        }
        else
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "logic value mismatch").c_str(), aExpectedValue,
                evaluateFormulaString(m_pDoc, aFormula));
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testLookupSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    for (const auto& rRow : loadSharedCaseRows("lookup_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 16, 8, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "lookup shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 6);

        const auto& rFunction = rRow.maColumns[0];
        const FormulaError eExpectedError = parseExpectedError(rRow.maColumns[5]);

        setNumberBlock(m_pDoc, 1, 0, { { 10, 100 }, { 20, 200 }, { 30, 300 } });
        setNumberBlock(m_pDoc, 1, 4, { { 10, 20, 30 }, { 100, 200, 300 } });

        OUString aFormula;
        if (rFunction == "MATCH")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 3, parseDouble(rRow.maColumns[2]));
            aFormula = u"=MATCH(A1;B1:B3;D1)"_ustr;
        }
        else if (rFunction == "XMATCH")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 3, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 4, parseDouble(rRow.maColumns[3]));
            aFormula = u"=XMATCH(A1;B1:B3;D1;E1)"_ustr;
        }
        else if (rFunction == "LOOKUP")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=LOOKUP(A1;B1:B3;C1:C3)"_ustr;
        }
        else if (rFunction == "VLOOKUP")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 3, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 4, parseDouble(rRow.maColumns[3]));
            aFormula = u"=VLOOKUP(A1;B1:C3;D1;E1)"_ustr;
        }
        else if (rFunction == "HLOOKUP")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 3, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 4, parseDouble(rRow.maColumns[3]));
            aFormula = u"=HLOOKUP(A1;B5:D6;D1;E1)"_ustr;
        }
        else if (rFunction == "XLOOKUP")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=XLOOKUP(A1;B1:B3;C1:C3)"_ustr;
        }
        else if (rFunction == "XLOOKUP_ROW")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=XLOOKUP(A1;B5:D5;B6:D6)"_ustr;
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown lookup shared-case function").c_str());
        }

        if (eExpectedError != FormulaError::NONE)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "lookup error mismatch").c_str(), eExpectedError,
                evaluateFormulaError(m_pDoc, aFormula));
        }
        else
        {
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "lookup value mismatch").c_str(),
                parseDouble(rRow.maColumns[4]), evaluateFormulaValue(m_pDoc, aFormula), 1e-9);
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testReferenceSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    for (const auto& rRow : loadSharedCaseRows("reference_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 8, 8, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "reference shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 7);

        setNumberBlock(m_pDoc, 0, 0, { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } });
        setNumberBlock(m_pDoc, 12, 0, { { 10, 20, 30 } });

        const auto& rFunction = rRow.maColumns[0];
        OUString aFormula;
        if (rFunction == "INDEX")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            aFormula = u"=INDEX(A1:C3;H1;I1)"_ustr;
        }
        else if (rFunction == "INDEX_ROWVECTOR")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            aFormula = u"=INDEX(M1:O1;0;H1)"_ustr;
        }
        else if (rFunction == "OFFSET_VALUE")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            aFormula = u"=OFFSET(A1;H1;I1)"_ustr;
        }
        else if (rFunction == "OFFSET_SUM")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 9, parseDouble(rRow.maColumns[3]));
            setValueCell(m_pDoc, 10, parseDouble(rRow.maColumns[4]));
            aFormula = u"=SUM(OFFSET(A1;H1;I1;J1;K1))"_ustr;
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown reference shared-case function").c_str());
        }

        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
            failSharedCase(rRow, "reference value mismatch").c_str(), parseDouble(rRow.maColumns[5]),
            evaluateFormulaValue(m_pDoc, aFormula), 1e-9);
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testArraySharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);
    ScMarkData aMark(m_pDoc->GetSheetLimits());
    aMark.SelectOneTable(0);

    for (const auto& rRow : loadSharedCaseRows("array_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 40, 20, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "array shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 6);

        setNumberBlock(m_pDoc, 0, 0, { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } });
        setNumberBlock(m_pDoc, 3, 0, { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } });
        setNumberBlock(m_pDoc, 20, 0, { { 10, 11 }, { 12, 13 } });
        setNumberBlock(m_pDoc, 23, 0, { { 20, 21 }, { 22, 23 } });

        const auto& rFunction = rRow.maColumns[0];
        const auto aExpected = parseNumericMatrixSpec(rRow.maColumns[4]);
        CPPUNIT_ASSERT(!aExpected.empty());
        const SCCOL nStartCol = 30;
        const SCROW nStartRow = 0;
        const SCCOL nEndCol = static_cast<SCCOL>(nStartCol + aExpected[0].size() - 1);
        const SCROW nEndRow = static_cast<SCROW>(nStartRow + aExpected.size() - 1);

        OUString aFormula;
        if (rFunction == "TAKE")
        {
            aFormula = u"=TAKE(A1:C3;"_ustr + OUString::number(parseDouble(rRow.maColumns[1]))
                       + u";"_ustr + OUString::number(parseDouble(rRow.maColumns[2])) + u")"_ustr;
        }
        else if (rFunction == "DROP")
        {
            aFormula = u"=DROP(A1:C3;"_ustr + OUString::number(parseDouble(rRow.maColumns[1]))
                       + u";"_ustr + OUString::number(parseDouble(rRow.maColumns[2])) + u")"_ustr;
        }
        else if (rFunction == "EXPAND")
        {
            aFormula = u"=EXPAND(A1:B2;"_ustr + OUString::number(parseDouble(rRow.maColumns[1]))
                       + u";"_ustr + OUString::number(parseDouble(rRow.maColumns[2])) + u";"_ustr
                       + OUString::number(parseDouble(rRow.maColumns[3])) + u")"_ustr;
        }
        else if (rFunction == "CHOOSECOLS")
        {
            aFormula = u"=CHOOSECOLS(A1:C3;"_ustr
                       + OUString::number(parseDouble(rRow.maColumns[1])) + u";"_ustr
                       + OUString::number(parseDouble(rRow.maColumns[2])) + u")"_ustr;
        }
        else if (rFunction == "CHOOSEROWS")
        {
            aFormula = u"=CHOOSEROWS(A1:C3;"_ustr
                       + OUString::number(parseDouble(rRow.maColumns[1])) + u";"_ustr
                       + OUString::number(parseDouble(rRow.maColumns[2])) + u")"_ustr;
        }
        else if (rFunction == "TOCOL")
            aFormula = u"=TOCOL(A1:B2)"_ustr;
        else if (rFunction == "TOROW")
            aFormula = u"=TOROW(A1:B2)"_ustr;
        else if (rFunction == "WRAPROWS")
        {
            aFormula = u"=WRAPROWS(D1:D5;"_ustr + OUString::number(parseDouble(rRow.maColumns[1]))
                       + u";"_ustr + OUString::number(parseDouble(rRow.maColumns[2])) + u")"_ustr;
        }
        else if (rFunction == "WRAPCOLS")
        {
            aFormula = u"=WRAPCOLS(D1:D5;"_ustr + OUString::number(parseDouble(rRow.maColumns[1]))
                       + u";"_ustr + OUString::number(parseDouble(rRow.maColumns[2])) + u")"_ustr;
        }
        else if (rFunction == "HSTACK")
            aFormula = u"=HSTACK(U1:V2;X1:Y2)"_ustr;
        else if (rFunction == "VSTACK")
            aFormula = u"=VSTACK(U1:V2;X1:Y2)"_ustr;
        else
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown array shared-case function").c_str());

        m_pDoc->InsertMatrixFormula(nStartCol, nStartRow, nEndCol, nEndRow, aMark, aFormula);
        assertNumericMatrixResult(m_pDoc, nStartCol, nStartRow, aExpected, rRow,
                                  "array value mismatch");
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testDynamicArrayHelperFunctions)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"ArrayHelpers"_ustr);
    ScMarkData aMark(m_pDoc->GetSheetLimits());
    aMark.SelectOneTable(0);

    auto setNumberBlock = [this](SCCOL nStartCol, SCROW nStartRow,
                                 std::initializer_list<std::initializer_list<double>> aRows) {
        SCROW nRow = nStartRow;
        for (const auto& rRow : aRows)
        {
            SCCOL nCol = nStartCol;
            for (double fValue : rRow)
            {
                m_pDoc->SetValue(ScAddress(nCol, nRow, 0), fValue);
                ++nCol;
            }
            ++nRow;
        }
    };

    auto assertValueCell = [this](SCCOL nCol, SCROW nRow, double fExpected) {
        CPPUNIT_ASSERT_EQUAL(FormulaError::NONE, m_pDoc->GetErrCode(ScAddress(nCol, nRow, 0)));
        CPPUNIT_ASSERT_DOUBLES_EQUAL(fExpected, m_pDoc->GetValue(ScAddress(nCol, nRow, 0)), 1e-12);
    };

    setNumberBlock(0, 0, { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } });
    setNumberBlock(20, 0, { { 10, 11 }, { 12, 13 } });
    setNumberBlock(23, 0, { { 20, 21 }, { 22, 23 } });
    setNumberBlock(3, 0, { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } });

    m_pDoc->InsertMatrixFormula(5, 0, 6, 1, aMark, u"=TAKE(A1:C3;2;2)"_ustr);
    assertValueCell(5, 0, 1);
    assertValueCell(6, 0, 2);
    assertValueCell(5, 1, 4);
    assertValueCell(6, 1, 5);

    m_pDoc->InsertMatrixFormula(8, 0, 9, 1, aMark, u"=DROP(A1:C3;1;1)"_ustr);
    assertValueCell(8, 0, 5);
    assertValueCell(9, 0, 6);
    assertValueCell(8, 1, 8);
    assertValueCell(9, 1, 9);

    m_pDoc->InsertMatrixFormula(5, 4, 7, 6, aMark, u"=EXPAND(A1:B2;3;3;0)"_ustr);
    assertValueCell(5, 4, 1);
    assertValueCell(6, 4, 2);
    assertValueCell(7, 4, 0);
    assertValueCell(5, 5, 4);
    assertValueCell(6, 5, 5);
    assertValueCell(7, 6, 0);

    m_pDoc->InsertMatrixFormula(9, 4, 10, 6, aMark, u"=CHOOSECOLS(A1:C3;3;1)"_ustr);
    assertValueCell(9, 4, 3);
    assertValueCell(10, 4, 1);
    assertValueCell(9, 6, 9);
    assertValueCell(10, 6, 7);

    m_pDoc->InsertMatrixFormula(13, 4, 15, 5, aMark, u"=CHOOSEROWS(A1:C3;3;1)"_ustr);
    assertValueCell(13, 4, 7);
    assertValueCell(14, 4, 8);
    assertValueCell(15, 4, 9);
    assertValueCell(13, 5, 1);
    assertValueCell(15, 5, 3);

    m_pDoc->InsertMatrixFormula(5, 9, 5, 12, aMark, u"=TOCOL(A1:B2)"_ustr);
    assertValueCell(5, 9, 1);
    assertValueCell(5, 10, 2);
    assertValueCell(5, 11, 4);
    assertValueCell(5, 12, 5);

    m_pDoc->InsertMatrixFormula(7, 9, 10, 9, aMark, u"=TOROW(A1:B2)"_ustr);
    assertValueCell(7, 9, 1);
    assertValueCell(8, 9, 2);
    assertValueCell(9, 9, 4);
    assertValueCell(10, 9, 5);

    m_pDoc->InsertMatrixFormula(12, 9, 13, 11, aMark, u"=WRAPROWS(D1:D5;2;0)"_ustr);
    assertValueCell(12, 9, 1);
    assertValueCell(13, 9, 2);
    assertValueCell(12, 10, 3);
    assertValueCell(13, 10, 4);
    assertValueCell(12, 11, 5);
    assertValueCell(13, 11, 0);

    m_pDoc->InsertMatrixFormula(15, 9, 17, 10, aMark, u"=WRAPCOLS(D1:D5;2;0)"_ustr);
    assertValueCell(15, 9, 1);
    assertValueCell(15, 10, 2);
    assertValueCell(16, 9, 3);
    assertValueCell(16, 10, 4);
    assertValueCell(17, 9, 5);
    assertValueCell(17, 10, 0);

    m_pDoc->InsertMatrixFormula(5, 14, 8, 15, aMark, u"=HSTACK(U1:V2;X1:Y2)"_ustr);
    assertValueCell(5, 14, 10);
    assertValueCell(6, 14, 11);
    assertValueCell(7, 14, 20);
    assertValueCell(8, 14, 21);
    assertValueCell(5, 15, 12);
    assertValueCell(8, 15, 23);

    m_pDoc->InsertMatrixFormula(5, 18, 6, 21, aMark, u"=VSTACK(U1:V2;X1:Y2)"_ustr);
    assertValueCell(5, 18, 10);
    assertValueCell(6, 18, 11);
    assertValueCell(5, 19, 12);
    assertValueCell(6, 19, 13);
    assertValueCell(5, 20, 20);
    assertValueCell(6, 21, 23);
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testMathScalarSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    for (const auto& rRow : loadSharedCaseRows("math_scalar_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 5, 5, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "math shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 6);

        const auto& rFunction = rRow.maColumns[0];
        const FormulaError eExpectedError = parseExpectedError(rRow.maColumns[5]);
        OUString aFormula;

        if (rFunction == "MOD")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=MOD(A1;B1)"_ustr;
        }
        else if (rFunction == "LN")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=LN(A1)"_ustr;
        }
        else if (rFunction == "LOG10")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=LOG10(A1)"_ustr;
        }
        else if (rFunction == "LOG")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=LOG(A1;B1)"_ustr;
        }
        else if (rFunction == "EVEN")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=EVEN(A1)"_ustr;
        }
        else if (rFunction == "ODD")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=ODD(A1)"_ustr;
        }
        else if (rFunction == "BITAND")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=BITAND(A1;B1)"_ustr;
        }
        else if (rFunction == "BITOR")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=BITOR(A1;B1)"_ustr;
        }
        else if (rFunction == "BITXOR")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=BITXOR(A1;B1)"_ustr;
        }
        else if (rFunction == "BITLSHIFT")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=BITLSHIFT(A1;B1)"_ustr;
        }
        else if (rFunction == "BITRSHIFT")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=BITRSHIFT(A1;B1)"_ustr;
        }
        else if (rFunction == "CEILING.MATH")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=CEILING.MATH(A1;B1)"_ustr;
        }
        else if (rFunction == "FLOOR.MATH")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=FLOOR.MATH(A1;B1)"_ustr;
        }
        else if (rFunction == "SQRT")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=SQRT(A1)"_ustr;
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown math shared-case function").c_str());
        }

        if (eExpectedError != FormulaError::NONE)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "math error mismatch").c_str(), eExpectedError,
                evaluateFormulaError(m_pDoc, aFormula));
        }
        else
        {
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "math value mismatch").c_str(),
                parseDouble(rRow.maColumns[4]), evaluateFormulaValue(m_pDoc, aFormula), 1e-9);
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testFinancialSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    for (const auto& rRow : loadSharedCaseRows("financial_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 13, 5, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "financial shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 9);

        const auto& rFunction = rRow.maColumns[0];
        OUString aFormula;

        if (rFunction == "PMT" || rFunction == "FV" || rFunction == "PV")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 9, parseDouble(rRow.maColumns[3]));
            setValueCell(m_pDoc, 10, parseDouble(rRow.maColumns[4]));
            setValueCell(m_pDoc, 11, parseDouble(rRow.maColumns[5]));
            aFormula = OUString::Concat(u"=") + decodeUtf8TestString(rFunction)
                       + u"(H1;I1;J1;K1;L1)"_ustr;
        }
        else if (rFunction == "EFFECT" || rFunction == "NOMINAL")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            aFormula = OUString::Concat(u"=") + decodeUtf8TestString(rFunction) + u"(H1;I1)"_ustr;
        }
        else if (rFunction == "SLN")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 9, parseDouble(rRow.maColumns[3]));
            aFormula = u"=SLN(H1;I1;J1)"_ustr;
        }
        else if (rFunction == "SYD")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 9, parseDouble(rRow.maColumns[3]));
            setValueCell(m_pDoc, 10, parseDouble(rRow.maColumns[4]));
            aFormula = u"=SYD(H1;I1;J1;K1)"_ustr;
        }
        else if (rFunction == "RRI")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 9, parseDouble(rRow.maColumns[3]));
            aFormula = u"=RRI(H1;I1;J1)"_ustr;
        }
        else if (rFunction == "RATE")
        {
            setValueCell(m_pDoc, 7, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 8, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 9, parseDouble(rRow.maColumns[3]));
            setValueCell(m_pDoc, 10, parseDouble(rRow.maColumns[4]));
            setValueCell(m_pDoc, 11, parseDouble(rRow.maColumns[5]));
            setValueCell(m_pDoc, 12, parseDouble(rRow.maColumns[6]));
            aFormula = u"=RATE(H1;I1;J1;K1;L1;M1)"_ustr;
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown financial shared-case function").c_str());
        }

        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
            failSharedCase(rRow, "financial value mismatch").c_str(),
            parseDouble(rRow.maColumns[7]), evaluateFormulaValue(m_pDoc, aFormula), 1e-9);
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testNumeralSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    for (const auto& rRow : loadSharedCaseRows("numeral_conversion_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 5, 5, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "numeral shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 6);

        const auto& rFunction = rRow.maColumns[0];
        const FormulaError eExpectedError = parseExpectedError(rRow.maColumns[5]);

        if (rFunction == "BASE")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            OUString aFormula = u"=BASE(A1;B1"_ustr;
            if (!rRow.maColumns[3].empty())
            {
                setValueCell(m_pDoc, 2, parseDouble(rRow.maColumns[3]));
                aFormula += u";C1"_ustr;
            }
            aFormula += u")"_ustr;

            if (eExpectedError != FormulaError::NONE)
            {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    failSharedCase(rRow, "BASE error mismatch").c_str(), eExpectedError,
                    evaluateFormulaError(m_pDoc, aFormula));
            }
            else
            {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    failSharedCase(rRow, "BASE value mismatch").c_str(),
                    decodeUtf8TestString(rRow.maColumns[4]), evaluateFormulaString(m_pDoc, aFormula));
            }
        }
        else if (rFunction == "DECIMAL")
        {
            setTextCell(m_pDoc, 0, decodeUtf8TestString(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "DECIMAL value mismatch").c_str(),
                parseDouble(rRow.maColumns[4]), evaluateFormulaValue(m_pDoc, u"=DECIMAL(A1;B1)"_ustr),
                1e-12);
        }
        else if (rFunction == "ROMAN")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "ROMAN value mismatch").c_str(),
                decodeUtf8TestString(rRow.maColumns[4]), evaluateFormulaString(m_pDoc, u"=ROMAN(A1)"_ustr));
        }
        else if (rFunction == "ARABIC")
        {
            setTextCell(m_pDoc, 0, decodeUtf8TestString(rRow.maColumns[1]));
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "ARABIC value mismatch").c_str(),
                parseDouble(rRow.maColumns[4]), evaluateFormulaValue(m_pDoc, u"=ARABIC(A1)"_ustr),
                1e-12);
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown numeral shared-case function").c_str());
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testTextSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    for (const auto& rRow : loadSharedCaseRows("text_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 5, 5, 0));
        CPPUNIT_ASSERT_MESSAGE(failSharedCase(rRow, "text shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 7);

        const auto& rFunction = rRow.maColumns[0];
        const auto aInputA = decodeUtf8TestString(rRow.maColumns[1]);
        const auto aInputB = decodeUtf8TestString(rRow.maColumns[2]);
        const auto aInputC = decodeUtf8TestString(rRow.maColumns[3]);
        const auto aExpected = decodeUtf8TestString(rRow.maColumns[5]);
        const FormulaError eExpectedError = parseExpectedError(rRow.maColumns[6]);

        if (rFunction == "TRIM")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "TRIM mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=TRIM(A1)"_ustr));
        }
        else if (rFunction == "LEN")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "LEN mismatch").c_str(), parseDouble(rRow.maColumns[5]),
                evaluateFormulaValue(m_pDoc, u"=LEN(A1)"_ustr), 1e-12);
        }
        else if (rFunction == "NUMBERVALUE")
        {
            setTextCell(m_pDoc, 0, aInputA);
            setTextCell(m_pDoc, 1, aInputB);
            setTextCell(m_pDoc, 2, aInputC);
            const OUString aFormula = u"=NUMBERVALUE(A1;B1;C1)"_ustr;
            if (eExpectedError != FormulaError::NONE)
            {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(
                    failSharedCase(rRow, "NUMBERVALUE error mismatch").c_str(), eExpectedError,
                    evaluateFormulaError(m_pDoc, aFormula));
            }
            else
            {
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                    failSharedCase(rRow, "NUMBERVALUE value mismatch").c_str(),
                    parseDouble(rRow.maColumns[5]), evaluateFormulaValue(m_pDoc, aFormula), 1e-12);
            }
        }
        else if (rFunction == "CLEAN")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "CLEAN mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=CLEAN(A1)"_ustr));
        }
        else if (rFunction == "CODE")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "CODE mismatch").c_str(), parseDouble(rRow.maColumns[5]),
                evaluateFormulaValue(m_pDoc, u"=CODE(A1)"_ustr), 1e-12);
        }
        else if (rFunction == "CHAR")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "CHAR mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=CHAR(A1)"_ustr));
        }
        else if (rFunction == "UNICODE")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "UNICODE mismatch").c_str(), parseDouble(rRow.maColumns[5]),
                evaluateFormulaValue(m_pDoc, u"=UNICODE(A1)"_ustr), 1e-12);
        }
        else if (rFunction == "UNICHAR")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "UNICHAR mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=UNICHAR(A1)"_ustr));
        }
        else if (rFunction == "UPPER")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "UPPER mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=UPPER(A1)"_ustr));
        }
        else if (rFunction == "LOWER")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "LOWER mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=LOWER(A1)"_ustr));
        }
        else if (rFunction == "PROPER")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "PROPER mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=PROPER(A1)"_ustr));
        }
        else if (rFunction == "ASC")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "ASC mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=ASC(A1)"_ustr));
        }
        else if (rFunction == "JIS")
        {
            setTextCell(m_pDoc, 0, aInputA);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                failSharedCase(rRow, "JIS mismatch").c_str(), aExpected,
                evaluateFormulaString(m_pDoc, u"=JIS(A1)"_ustr));
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown text shared-case function").c_str());
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testCalendarSharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    for (const auto& rRow : loadSharedCaseRows("calendar_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 5, 5, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "calendar shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 7);

        const auto& rFunction = rRow.maColumns[0];
        OUString aFormula;

        if (rFunction == "DATE")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 2, parseDouble(rRow.maColumns[3]));
            aFormula = u"=DATE(A1;B1;C1)"_ustr;
        }
        else if (rFunction == "YEARFROM")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=YEAR(A1)"_ustr;
        }
        else if (rFunction == "MONTHFROM")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=MONTH(A1)"_ustr;
        }
        else if (rFunction == "DAYFROM")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=DAY(A1)"_ustr;
        }
        else if (rFunction == "TIME")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            setValueCell(m_pDoc, 2, parseDouble(rRow.maColumns[3]));
            aFormula = u"=TIME(A1;B1;C1)"_ustr;
        }
        else if (rFunction == "WEEKDAY")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=WEEKDAY(A1;B1)"_ustr;
        }
        else if (rFunction == "WEEKNUM_OOO")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            setValueCell(m_pDoc, 1, parseDouble(rRow.maColumns[2]));
            aFormula = u"=WEEKNUM(A1;B1)"_ustr;
        }
        else if (rFunction == "ISOWEEKNUM")
            aFormula = u"=ISOWEEKNUM("_ustr + makeDateFormula(rRow.maColumns[1]) + u")"_ustr;
        else if (rFunction == "EASTERSUNDAY")
        {
            setValueCell(m_pDoc, 0, parseDouble(rRow.maColumns[1]));
            aFormula = u"=YEAR(EASTERSUNDAY(A1))*10000+MONTH(EASTERSUNDAY(A1))*100+DAY(EASTERSUNDAY(A1))"_ustr;
        }
        else if (rFunction == "DAYS360")
        {
            aFormula = u"=DAYS360("_ustr + makeDateFormula(rRow.maColumns[1]) + u";"_ustr
                       + makeDateFormula(rRow.maColumns[2]) + u")"_ustr;
        }
        else if (rFunction == "DATEDIF")
        {
            setTextCell(m_pDoc, 0, decodeUtf8TestString(rRow.maColumns[3]));
            aFormula = u"=DATEDIF("_ustr + makeDateFormula(rRow.maColumns[1]) + u";"_ustr
                       + makeDateFormula(rRow.maColumns[2]) + u";A1)"_ustr;
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown calendar shared-case function").c_str());
        }

        if (rFunction == "EASTERSUNDAY")
        {
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "EASTERSUNDAY numeric mismatch").c_str(),
                encodeDateTokenAsYmdNumber(rRow.maColumns[5]),
                evaluateFormulaValue(m_pDoc, aFormula), 1e-12);
        }
        else
        {
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
                failSharedCase(rRow, "calendar value mismatch").c_str(),
                parseDouble(rRow.maColumns[5]), evaluateFormulaValue(m_pDoc, aFormula), 1e-12);
        }
    }
}

CPPUNIT_TEST_FIXTURE(TestSharedCases, testWorkdaySharedCases)
{
    sc::AutoCalcSwitch aAutoCalc(*m_pDoc, true);
    m_pDoc->InsertTab(0, u"SharedCases"_ustr);

    std::size_t nSkippedHelperRows = 0;
    for (const auto& rRow : loadSharedCaseRows("workday_cases.tsv"))
    {
        clearRange(m_pDoc, ScRange(0, 0, 0, 5, 5, 0));
        CPPUNIT_ASSERT_MESSAGE(
            failSharedCase(rRow, "workday shared case column mismatch").c_str(),
            rRow.maColumns.size() >= 6);

        const auto& rFunction = rRow.maColumns[0];
        if (rFunction == "WEEKENDMASK.DEFAULT")
        {
            ++nSkippedHelperRows;
            continue;
        }

        if (rFunction == "WEEKENDMASK.MS")
        {
            ++nSkippedHelperRows;
            continue;
        }

        if (rFunction == "NETWORKDAYS")
        {
            ++nSkippedHelperRows;
        }
        else if (rFunction == "WORKDAY")
        {
            ++nSkippedHelperRows;
        }
        else
        {
            CPPUNIT_FAIL(failSharedCase(rRow, "unknown workday shared-case function").c_str());
        }
    }

    // The current workday TSV rows use helper-layer contracts that do not map
    // 1:1 onto spreadsheet-facing Calc formulas yet. Keep them in the
    // standalone/helper lane until they are rewritten as spreadsheet-facing
    // parity cases.
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(5), nSkippedHelperRows);
}

} // namespace

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
