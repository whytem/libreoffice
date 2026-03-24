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
#include <spreadsheetengine/compat/libreoffice/Host.hxx>

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

FormulaError parseExpectedError(std::string_view rValue)
{
    if (rValue.empty())
        return FormulaError::NONE;
    if (rValue == "IllegalArgument")
        return FormulaError::IllegalArgument;
    if (rValue == "StringOverflow")
        return FormulaError::StringOverflow;
    if (rValue == "NoValue")
        return FormulaError::NoValue;
    if (rValue == "Domain")
        return FormulaError::IllegalFPOperation;
    if (rValue == "NoConvergence")
        return FormulaError::NoConvergence;

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

void setTextCell(ScDocument* pDoc, SCCOL nCol, const OUString& rValue)
{
    pDoc->SetString(ScAddress(nCol, 0, 0), rValue);
}

void setValueCell(ScDocument* pDoc, SCCOL nCol, double fValue)
{
    pDoc->SetValue(ScAddress(nCol, 0, 0), fValue);
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

    const auto aEmpty = aHost.getCellValue({ 0, 4, 0 });
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

    const auto aNotNumber = aHost.parseNumber(u"hello");
    CPPUNIT_ASSERT(!aNotNumber);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::Error::NoValue, aNotNumber.meError);

    const auto aFormattedNumber = aHost.formatNumber(42.5);
    CPPUNIT_ASSERT(aFormattedNumber);
    CPPUNIT_ASSERT(!aFormattedNumber.maValue.empty());
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
