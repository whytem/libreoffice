/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#pragma once

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/String.hxx>

#include "TestSupport.hxx"

namespace spreadsheetengine::standalone::test
{

struct SharedCaseRow
{
    std::vector<std::string> maColumns;
    std::size_t mnLineNumber = 0;
};

inline std::filesystem::path testRootPath()
{
#ifdef SPREADSHEETENGINE_TEST_ROOT
    return SPREADSHEETENGINE_TEST_ROOT;
#else
    return std::filesystem::current_path();
#endif
}

inline std::filesystem::path sharedCasePath(std::string_view rRelativePath)
{
    return testRootPath() / "tests" / "parity" / std::filesystem::path(rRelativePath);
}

inline std::vector<std::string> splitTsvLine(const std::string& rLine)
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

inline std::vector<SharedCaseRow> loadSharedCaseRows(std::string_view rRelativePath)
{
    const auto aPath = sharedCasePath(rRelativePath);
    std::ifstream aInput(aPath);
    if (!aInput.is_open())
        throw std::runtime_error("unable to open shared case file: " + aPath.string());

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

inline int hexDigitValue(char cDigit)
{
    if (cDigit >= '0' && cDigit <= '9')
        return cDigit - '0';
    if (cDigit >= 'a' && cDigit <= 'f')
        return 10 + (cDigit - 'a');
    if (cDigit >= 'A' && cDigit <= 'F')
        return 10 + (cDigit - 'A');
    return -1;
}

inline void appendCodePoint(api::String& rOutput, char32_t nCodePoint)
{
    if (nCodePoint <= 0xFFFF)
    {
        rOutput.push_back(static_cast<char16_t>(nCodePoint));
        return;
    }

    nCodePoint -= 0x10000;
    rOutput.push_back(static_cast<char16_t>(0xD800 + ((nCodePoint >> 10) & 0x3FF)));
    rOutput.push_back(static_cast<char16_t>(0xDC00 + (nCodePoint & 0x3FF)));
}

inline bool appendEscapedCodePoint(
    api::String& rOutput, std::string_view rInput, std::size_t& rnIndex)
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

inline api::String decodeUtf8TestString(std::string_view rInput)
{
    api::String aOutput;
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
                        aOutput.push_back(u'\\');
                        ++i;
                        continue;
                    case 't':
                        aOutput.push_back(u'\t');
                        ++i;
                        continue;
                    case 'n':
                        aOutput.push_back(u'\n');
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
            throw std::runtime_error("invalid UTF-8 lead byte in shared case");

        for (std::size_t j = 0; j < nExtraBytes; ++j)
        {
            if (i + 1 >= rInput.size())
                throw std::runtime_error("truncated UTF-8 sequence in shared case");

            const unsigned char nCont = static_cast<unsigned char>(rInput[++i]);
            if ((nCont & 0xC0u) != 0x80u)
                throw std::runtime_error("invalid UTF-8 continuation byte in shared case");
            nCodePoint = (nCodePoint << 6) | (nCont & 0x3Fu);
        }

        appendCodePoint(aOutput, nCodePoint);
    }

    return aOutput;
}

inline double parseDouble(std::string_view rValue)
{
    std::string aString(rValue);
    return std::stod(aString);
}

inline bool parseBool(std::string_view rValue)
{
    std::string aUpper(rValue);
    for (char& rChar : aUpper)
        rChar = static_cast<char>(std::toupper(static_cast<unsigned char>(rChar)));
    return aUpper == "TRUE" || aUpper == "1" || aUpper == "YES";
}

inline api::Error parseExpectedError(std::string_view rValue)
{
    if (rValue.empty())
        return api::Error::None;
    if (rValue == "IllegalArgument")
        return api::Error::IllegalArgument;
    if (rValue == "DivisionByZero")
        return api::Error::DivisionByZero;
    if (rValue == "StringOverflow")
        return api::Error::StringOverflow;
    if (rValue == "NoValue")
        return api::Error::NoValue;
    if (rValue == "NoName")
        return api::Error::NoName;
    if (rValue == "Domain")
        return api::Error::Domain;
    if (rValue == "NoConvergence")
        return api::Error::NoConvergence;
    if (rValue == "NotAvailable")
        return api::Error::NotAvailable;

    throw std::runtime_error("unknown expected error token in shared case");
}

inline int failSharedCase(
    const char* pTestName, const SharedCaseRow& rRow, const char* pMessage)
{
    std::ostringstream aStream;
    aStream << pMessage << " (shared case line " << rRow.mnLineNumber << ")";
    return fail(pTestName, aStream.str().c_str());
}

} // namespace spreadsheetengine::standalone::test

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
