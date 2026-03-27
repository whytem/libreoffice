/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/FodsLoader.hxx>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <unicode/ustring.h>

namespace spreadsheetengine::core::fods
{
namespace
{

constexpr const char* pOfficeNs = "urn:oasis:names:tc:opendocument:xmlns:office:1.0";
constexpr const char* pTableNs = "urn:oasis:names:tc:opendocument:xmlns:table:1.0";
constexpr const char* pTextNs = "urn:oasis:names:tc:opendocument:xmlns:text:1.0";
constexpr const char* pChartNs = "urn:oasis:names:tc:opendocument:xmlns:chart:1.0";
constexpr const char* pDrawNs = "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0";
constexpr const char* pXLinkNs = "http://www.w3.org/1999/xlink";
constexpr const char* pCalcExtNs = "urn:org:documentfoundation:names:experimental:calc:xmlns:calcext:1.0";

using XmlDocument = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
using XmlString = std::unique_ptr<xmlChar, decltype(xmlFree)>;

[[nodiscard]] bool matchesNode(const xmlNode* pNode, std::string_view rNamespaceHref,
    std::string_view rLocalName)
{
    if (!pNode || pNode->type != XML_ELEMENT_NODE || !pNode->ns || !pNode->ns->href
        || !pNode->name)
    {
        return false;
    }

    return reinterpret_cast<const char*>(pNode->ns->href) == rNamespaceHref
           && reinterpret_cast<const char*>(pNode->name) == rLocalName;
}

[[nodiscard]] api::String toApiString(const xmlChar* pText)
{
    if (!pText)
        return {};

    const char* pUtf8 = reinterpret_cast<const char*>(pText);
    UErrorCode eStatus = U_ZERO_ERROR;
    int32_t nLength = 0;
    u_strFromUTF8(nullptr, 0, &nLength, pUtf8, -1, &eStatus);
    if (eStatus != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(eStatus))
        return {};

    api::String aResult(static_cast<std::size_t>(nLength), u'\0');
    eStatus = U_ZERO_ERROR;
    u_strFromUTF8(reinterpret_cast<UChar*>(aResult.data()), nLength, nullptr, pUtf8, -1, &eStatus);
    if (U_FAILURE(eStatus))
        return {};

    return aResult;
}

[[nodiscard]] std::optional<unsigned char> decodeCp1252Byte(char16_t cChar)
{
    if (cChar >= 0x0080 && cChar <= 0x00BF)
        return static_cast<unsigned char>(cChar);

    switch (cChar)
    {
        case 0x20AC: return 0x80;
        case 0x201A: return 0x82;
        case 0x0192: return 0x83;
        case 0x201E: return 0x84;
        case 0x2026: return 0x85;
        case 0x2020: return 0x86;
        case 0x2021: return 0x87;
        case 0x02C6: return 0x88;
        case 0x2030: return 0x89;
        case 0x0160: return 0x8A;
        case 0x2039: return 0x8B;
        case 0x0152: return 0x8C;
        case 0x017D: return 0x8E;
        case 0x2018: return 0x91;
        case 0x2019: return 0x92;
        case 0x201C: return 0x93;
        case 0x201D: return 0x94;
        case 0x2022: return 0x95;
        case 0x2013: return 0x96;
        case 0x2014: return 0x97;
        case 0x02DC: return 0x98;
        case 0x2122: return 0x99;
        case 0x0161: return 0x9A;
        case 0x203A: return 0x9B;
        case 0x0153: return 0x9C;
        case 0x017E: return 0x9E;
        case 0x0178: return 0x9F;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] api::String repairC2Mojibake(api::StringView rText)
{
    api::String aResult;
    aResult.reserve(rText.size());

    for (std::size_t nIndex = 0; nIndex < rText.size(); ++nIndex)
    {
        if (rText[nIndex] == u'\u00C2' && nIndex + 1 < rText.size())
        {
            if (const auto oByte = decodeCp1252Byte(rText[nIndex + 1]))
            {
                aResult.push_back(static_cast<char16_t>(*oByte));
                ++nIndex;
                continue;
            }
        }

        aResult.push_back(rText[nIndex]);
    }

    return aResult;
}

[[nodiscard]] std::string toUtf8String(api::StringView rText)
{
    if (rText.empty())
        return {};

    UErrorCode eStatus = U_ZERO_ERROR;
    int32_t nLength = 0;
    u_strToUTF8(nullptr, 0, &nLength, reinterpret_cast<const UChar*>(rText.data()),
        static_cast<int32_t>(rText.size()), &eStatus);
    if (eStatus != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(eStatus))
        return {};

    std::string aResult(static_cast<std::size_t>(nLength), '\0');
    eStatus = U_ZERO_ERROR;
    u_strToUTF8(aResult.data(), nLength, nullptr, reinterpret_cast<const UChar*>(rText.data()),
        static_cast<int32_t>(rText.size()), &eStatus);
    if (U_FAILURE(eStatus))
        return {};

    return aResult;
}

[[nodiscard]] XmlString getProp(
    const xmlNode* pNode, const char* pNamespaceHref, const char* pLocalName)
{
    return XmlString(xmlGetNsProp(pNode, BAD_CAST pLocalName, BAD_CAST pNamespaceHref), xmlFree);
}

[[nodiscard]] api::String getPropString(
    const xmlNode* pNode, const char* pNamespaceHref, const char* pLocalName)
{
    return toApiString(getProp(pNode, pNamespaceHref, pLocalName).get());
}

[[nodiscard]] std::size_t parseRepeatCount(
    const xmlNode* pNode, const char* pNamespaceHref, const char* pLocalName)
{
    const auto pValue = getProp(pNode, pNamespaceHref, pLocalName);
    if (!pValue)
        return 1;

    const long nRepeat = std::strtol(reinterpret_cast<const char*>(pValue.get()), nullptr, 10);
    return nRepeat > 0 ? static_cast<std::size_t>(nRepeat) : 1;
}

[[nodiscard]] bool parseBoolean(const api::String& rValue)
{
    return rValue == u"true" || rValue == u"TRUE" || rValue == u"1";
}

[[nodiscard]] bool isCanonicalBoolean(api::StringView rValue)
{
    return rValue == u"true" || rValue == u"TRUE" || rValue == u"false" || rValue == u"FALSE"
           || rValue == u"1" || rValue == u"0";
}

[[nodiscard]] double parseAsciiDouble(api::StringView rValue)
{
    std::string aAscii;
    aAscii.reserve(rValue.size());
    for (const char16_t cChar : rValue)
        aAscii.push_back(static_cast<char>(cChar));
    return std::strtod(aAscii.c_str(), nullptr);
}

[[nodiscard]] api::Error mapDisplayedError(api::StringView rText)
{
    if (rText == u"#N/A")
        return api::Error::NotAvailable;
    if (rText == u"#DIV/0!")
        return api::Error::DivisionByZero;
    if (rText == u"#VALUE!")
        return api::Error::NoValue;
    if (rText == u"#NUM!")
        return api::Error::NoConvergence;
    if (rText == u"#NAME?" || rText == u"#REF!" || rText == u"#NULL!")
        return api::Error::IllegalArgument;
    if (rText.substr(0, 4) == u"Err:")
    {
        const api::StringView aCode = rText.substr(4);
        bool bDigitsOnly = !aCode.empty();
        for (const char16_t cChar : aCode)
        {
            if (cChar < u'0' || cChar > u'9')
            {
                bDigitsOnly = false;
                break;
            }
        }

        if (bDigitsOnly)
        {
            if (aCode == u"503" || aCode == u"523")
                return api::Error::NoConvergence;
            if (aCode == u"513")
                return api::Error::StringOverflow;
            if (aCode == u"519")
                return api::Error::NoValue;
            if (aCode == u"532")
                return api::Error::DivisionByZero;
            return api::Error::IllegalArgument;
        }
    }

    return api::Error::NoValue;
}

[[nodiscard]] std::optional<api::Error> tryParseDisplayedError(api::StringView rText)
{
    if (rText == u"#N/A")
        return api::Error::NotAvailable;
    if (rText == u"#DIV/0!")
        return api::Error::DivisionByZero;
    if (rText == u"#VALUE!")
        return api::Error::NoValue;
    if (rText == u"#NUM!")
        return api::Error::NoConvergence;
    if (rText == u"#NAME?" || rText == u"#REF!" || rText == u"#NULL!")
        return api::Error::IllegalArgument;
    if (rText.substr(0, 4) == u"Err:")
        return mapDisplayedError(rText);
    return std::nullopt;
}

void appendNodeText(const xmlNode* pNode, api::String& rText)
{
    for (const xmlNode* pCurrent = pNode; pCurrent; pCurrent = pCurrent->next)
    {
        if (pCurrent->type == XML_TEXT_NODE || pCurrent->type == XML_CDATA_SECTION_NODE)
        {
            rText += toApiString(pCurrent->content);
            continue;
        }

        if (pCurrent->type != XML_ELEMENT_NODE)
            continue;

        if (matchesNode(pCurrent, pTextNs, "s"))
        {
            rText.append(parseRepeatCount(pCurrent, pTextNs, "c"), u' ');
            continue;
        }

        if (matchesNode(pCurrent, pTextNs, "tab"))
        {
            rText.push_back(u'\t');
            continue;
        }

        if (matchesNode(pCurrent, pTextNs, "line-break"))
        {
            rText.push_back(u'\n');
            continue;
        }

        appendNodeText(pCurrent->children, rText);
    }
}

[[nodiscard]] api::String getNodeText(const xmlNode* pNode)
{
    api::String aText;
    appendNodeText(pNode ? pNode->children : nullptr, aText);
    return aText;
}

void countIgnoredFeatures(const xmlNode* pNode, IgnoredFeatureSummary& rSummary)
{
    for (const xmlNode* pChild = pNode; pChild; pChild = pChild->next)
    {
        if (pChild->type == XML_ELEMENT_NODE)
        {
            if (matchesNode(pChild, pChartNs, "chart"))
                ++rSummary.mnChartElementCount;
            else if (pChild->ns && pChild->ns->href
                     && std::string_view(reinterpret_cast<const char*>(pChild->ns->href))
                            == pDrawNs)
            {
                ++rSummary.mnDrawElementCount;
            }
            else if (matchesNode(pChild, pTableNs, "content-validation"))
            {
                ++rSummary.mnContentValidationCount;
            }
            else if (matchesNode(pChild, pOfficeNs, "annotation"))
            {
                ++rSummary.mnAnnotationCount;
            }
        }

        countIgnoredFeatures(pChild->children, rSummary);
    }
}

[[nodiscard]] workbook::SheetSourceMode parseSheetSourceMode(api::StringView rMode)
{
    return rMode == u"copy-results-only" ? workbook::SheetSourceMode::CopyResultsOnly
                                         : workbook::SheetSourceMode::Unknown;
}

[[nodiscard]] bool isCollapsedRow(api::StringView rVisibility)
{
    return rVisibility == u"collapse";
}

void parseCalculationSettings(const xmlNode* pSettingsNode, workbook::Workbook& rWorkbook)
{
    // Calc's XML import defaults missing formula-search settings to regex mode.
    rWorkbook.meFormulaSearchType = workbook::FormulaSearchType::Regex;

    const api::String aUseRegex
        = getPropString(pSettingsNode, pTableNs, "use-regular-expressions");
    if (aUseRegex == u"false" || aUseRegex == u"FALSE")
        rWorkbook.meFormulaSearchType = workbook::FormulaSearchType::Normal;

    const api::String aUseWildcards = getPropString(pSettingsNode, pTableNs, "use-wildcards");
    if (aUseWildcards == u"true" || aUseWildcards == u"TRUE")
        rWorkbook.meFormulaSearchType = workbook::FormulaSearchType::Wildcard;
}

[[nodiscard]] workbook::Cell parseCell(const xmlNode* pCellNode)
{
    workbook::Cell aCell;
    aCell.mbCovered = matchesNode(pCellNode, pTableNs, "covered-table-cell");
    aCell.maFormula = getPropString(pCellNode, pTableNs, "formula");

    const api::String aOfficeValueType = getPropString(pCellNode, pOfficeNs, "value-type");
    const api::String aCalcExtValueType = getPropString(pCellNode, pCalcExtNs, "value-type");
    aCell.maRawValueType = !aCalcExtValueType.empty() ? aCalcExtValueType : aOfficeValueType;

    if (aCell.mbCovered)
        return aCell;

    if (aCell.maRawValueType == u"error")
    {
        aCell.maRawValue = getNodeText(pCellNode);
        aCell.maValue = api::CellValue::error(mapDisplayedError(aCell.maRawValue));
        return aCell;
    }

    if (aOfficeValueType == u"float" || aOfficeValueType == u"currency" || aOfficeValueType == u"percentage")
    {
        aCell.maRawValue = getPropString(pCellNode, pOfficeNs, "value");
        if (!aCell.maRawValue.empty())
            aCell.maValue = api::CellValue::number(parseAsciiDouble(aCell.maRawValue));
        return aCell;
    }

    if (aOfficeValueType == u"boolean")
    {
        aCell.maRawValue = getPropString(pCellNode, pOfficeNs, "boolean-value");
        if (isCanonicalBoolean(aCell.maRawValue))
            aCell.maValue = api::CellValue::boolean(parseBoolean(aCell.maRawValue));
        else
            aCell.maValue = api::CellValue::number(parseAsciiDouble(aCell.maRawValue));
        return aCell;
    }

    if (aOfficeValueType == u"string")
    {
        const api::String aStringValue = getPropString(pCellNode, pOfficeNs, "string-value");
        const api::String aDisplayedText = getNodeText(pCellNode);
        if (!aCell.maFormula.empty() && aStringValue.empty())
        {
            if (const auto oError = tryParseDisplayedError(aDisplayedText))
            {
                aCell.maRawValueType = u"error";
                aCell.maRawValue = aDisplayedText;
                aCell.maValue = api::CellValue::error(*oError);
                return aCell;
            }
        }
        aCell.maRawValue = repairC2Mojibake(
            !aStringValue.empty() ? aStringValue : aDisplayedText);
        aCell.maValue = api::CellValue::text(aCell.maRawValue);
        return aCell;
    }

    if (aOfficeValueType == u"date")
    {
        aCell.maRawValue = getPropString(pCellNode, pOfficeNs, "date-value");
        if (aCell.maRawValue.empty())
            aCell.maRawValue = getNodeText(pCellNode);
        aCell.maValue = api::CellValue::text(aCell.maRawValue);
        return aCell;
    }

    if (aOfficeValueType == u"time")
    {
        aCell.maRawValue = getPropString(pCellNode, pOfficeNs, "time-value");
        if (aCell.maRawValue.empty())
            aCell.maRawValue = getNodeText(pCellNode);
        aCell.maValue = api::CellValue::text(aCell.maRawValue);
        return aCell;
    }

    aCell.maRawValue = getNodeText(pCellNode);
    if (!aCell.maRawValue.empty())
        aCell.maValue = api::CellValue::text(aCell.maRawValue);

    return aCell;
}

void parseNamedExpressions(const xmlNode* pNamedExpressions, api::StringView rScopeSheetName,
    workbook::Workbook& rWorkbook)
{
    for (const xmlNode* pChild = pNamedExpressions ? pNamedExpressions->children : nullptr; pChild;
         pChild = pChild->next)
    {
        if (!matchesNode(pChild, pTableNs, "named-range"))
            continue;

        rWorkbook.maNamedRanges.push_back({ getPropString(pChild, pTableNs, "name"),
            api::String(rScopeSheetName), getPropString(pChild, pTableNs, "base-cell-address"),
            getPropString(pChild, pTableNs, "cell-range-address") });
    }
}

void parseSheet(const xmlNode* pTableNode, workbook::Workbook& rWorkbook)
{
    workbook::Sheet aSheet;
    aSheet.maName = getPropString(pTableNode, pTableNs, "name");
    api::RowIndex nRow = 0;

    for (const xmlNode* pChild = pTableNode->children; pChild; pChild = pChild->next)
    {
        if (matchesNode(pChild, pTableNs, "table-source"))
        {
            aSheet.moSource = workbook::SheetSource { getPropString(pChild, pXLinkNs, "href"),
                getPropString(pChild, pTableNs, "table-name"),
                parseSheetSourceMode(getPropString(pChild, pTableNs, "mode")) };
            continue;
        }

        if (matchesNode(pChild, pTableNs, "named-expressions"))
        {
            parseNamedExpressions(pChild, aSheet.maName, rWorkbook);
            continue;
        }

        if (!matchesNode(pChild, pTableNs, "table-row"))
            continue;

        const std::size_t nRowRepeat
            = parseRepeatCount(pChild, pTableNs, "number-rows-repeated");
        const api::RowIndex nBaseRow = nRow;
        const bool bCollapsedRow = isCollapsedRow(getPropString(pChild, pTableNs, "visibility"));

        for (std::size_t nRowOffset = 0; nRowOffset < nRowRepeat; ++nRowOffset)
        {
            if (bCollapsedRow)
            {
                aSheet.setRowHidden(
                    nBaseRow + static_cast<api::RowIndex>(nRowOffset));
            }

            api::ColumnIndex nColumn = 0;
            for (const xmlNode* pCellNode = pChild->children; pCellNode; pCellNode = pCellNode->next)
            {
                if (!matchesNode(pCellNode, pTableNs, "table-cell")
                    && !matchesNode(pCellNode, pTableNs, "covered-table-cell"))
                {
                    continue;
                }

                const std::size_t nColRepeat
                    = parseRepeatCount(pCellNode, pTableNs, "number-columns-repeated");
                const workbook::Cell aCell = parseCell(pCellNode);
                const bool bStoreCell = aCell.mbCovered || aCell.hasFormula()
                                        || !aCell.maRawValueType.empty() || !aCell.maRawValue.empty()
                                        || !aCell.maValue.isEmpty();

                if (bStoreCell)
                {
                    for (std::size_t nColOffset = 0; nColOffset < nColRepeat; ++nColOffset)
                    {
                        aSheet.setCell(
                            nColumn + static_cast<api::ColumnIndex>(nColOffset),
                            nBaseRow + static_cast<api::RowIndex>(nRowOffset), aCell);
                    }
                }

                nColumn += static_cast<api::ColumnIndex>(nColRepeat);
            }
        }

        nRow += static_cast<api::RowIndex>(nRowRepeat);
    }

    rWorkbook.maSheets.push_back(std::move(aSheet));
}

void mergeIgnoredFeatures(IgnoredFeatureSummary& rTarget, const IgnoredFeatureSummary& rSource)
{
    rTarget.mnChartElementCount += rSource.mnChartElementCount;
    rTarget.mnDrawElementCount += rSource.mnDrawElementCount;
    rTarget.mnContentValidationCount += rSource.mnContentValidationCount;
    rTarget.mnAnnotationCount += rSource.mnAnnotationCount;
}

[[nodiscard]] std::filesystem::path normalizePath(const std::filesystem::path& rPath)
{
    return std::filesystem::absolute(rPath).lexically_normal();
}

[[nodiscard]] std::optional<std::filesystem::path> resolveLinkedWorkbookPath(
    const std::filesystem::path& rWorkbookPath, api::StringView rHref)
{
    const std::string aHrefUtf8 = toUtf8String(rHref);
    if (aHrefUtf8.empty())
        return std::nullopt;

    const std::filesystem::path aHrefPath(aHrefUtf8);
    const std::filesystem::path aBaseDir = rWorkbookPath.parent_path();

    std::vector<std::filesystem::path> aCandidates;
    if (aHrefPath.is_absolute())
        aCandidates.push_back(normalizePath(aHrefPath));
    else
        aCandidates.push_back(normalizePath(aBaseDir / aHrefPath));

    std::vector<std::filesystem::path> aSegments;
    for (const auto& rPart : aHrefPath)
    {
        if (rPart == ".")
            continue;
        aSegments.push_back(rPart);
    }

    for (std::size_t nSkip = 1; nSkip < aSegments.size(); ++nSkip)
    {
        std::filesystem::path aSuffix;
        for (std::size_t nIndex = nSkip; nIndex < aSegments.size(); ++nIndex)
            aSuffix /= aSegments[nIndex];
        if (!aSuffix.empty())
            aCandidates.push_back(normalizePath(aBaseDir / aSuffix));
    }

    for (const auto& rCandidate : aCandidates)
    {
        std::error_code aError;
        if (std::filesystem::exists(rCandidate, aError) && !aError
            && rCandidate.extension() == ".fods")
        {
            return rCandidate;
        }
    }

    return std::nullopt;
}

[[nodiscard]] api::ValueResult<LoadResult> loadWorkbookRecursive(
    const std::filesystem::path& rPath, std::set<std::filesystem::path>& rActiveLoads);

void resolveImportedSheets(LoadResult& rResult, const std::filesystem::path& rPath,
    std::set<std::filesystem::path>& rActiveLoads)
{
    for (auto& rSheet : rResult.maWorkbook.maSheets)
    {
        if (!rSheet.moSource || !rSheet.moSource->isCopyResultsOnly() || rSheet.moSource->maHref.empty()
            || rSheet.moSource->maTableName.empty())
        {
            continue;
        }

        const auto oLinkedPath = resolveLinkedWorkbookPath(rPath, rSheet.moSource->maHref);
        if (!oLinkedPath || rActiveLoads.contains(*oLinkedPath))
            continue;

        const auto aLinkedResult = loadWorkbookRecursive(*oLinkedPath, rActiveLoads);
        if (!aLinkedResult)
            continue;

        mergeIgnoredFeatures(rResult.maIgnoredFeatures, aLinkedResult.maValue.maIgnoredFeatures);

        const workbook::Sheet* pSourceSheet
            = aLinkedResult.maValue.maWorkbook.findSheet(rSheet.moSource->maTableName);
        if (!pSourceSheet)
            continue;

        rSheet.maCells = pSourceSheet->maCells;
        rSheet.maHiddenRows = pSourceSheet->maHiddenRows;
    }
}

api::ValueResult<LoadResult> loadWorkbookRecursive(
    const std::filesystem::path& rPath, std::set<std::filesystem::path>& rActiveLoads)
{
    const std::filesystem::path aNormalizedPath = normalizePath(rPath);
    if (rActiveLoads.contains(aNormalizedPath))
        return api::ValueResult<LoadResult>::failure(api::Error::IllegalArgument);

    rActiveLoads.insert(aNormalizedPath);
    const auto aRemovePath = [&]() { rActiveLoads.erase(aNormalizedPath); };

    xmlInitParser();
    XmlDocument pDoc(
        xmlReadFile(aNormalizedPath.string().c_str(), nullptr, XML_PARSE_NONET | XML_PARSE_NOBLANKS),
        &xmlFreeDoc);
    if (!pDoc)
    {
        aRemovePath();
        return api::ValueResult<LoadResult>::failure(api::Error::IllegalArgument);
    }

    const xmlNode* pRoot = xmlDocGetRootElement(pDoc.get());
    if (!pRoot)
    {
        aRemovePath();
        return api::ValueResult<LoadResult>::failure(api::Error::IllegalArgument);
    }

    const xmlNode* pBody = nullptr;
    const xmlNode* pSpreadsheet = nullptr;
    for (const xmlNode* pChild = pRoot->children; pChild; pChild = pChild->next)
    {
        if (matchesNode(pChild, pOfficeNs, "body"))
        {
            pBody = pChild;
            break;
        }
    }

    for (const xmlNode* pChild = pBody ? pBody->children : nullptr; pChild; pChild = pChild->next)
    {
        if (matchesNode(pChild, pOfficeNs, "spreadsheet"))
        {
            pSpreadsheet = pChild;
            break;
        }
    }

    if (!pSpreadsheet)
    {
        aRemovePath();
        return api::ValueResult<LoadResult>::failure(api::Error::IllegalArgument);
    }

    LoadResult aResult;
    countIgnoredFeatures(pSpreadsheet->children, aResult.maIgnoredFeatures);

    for (const xmlNode* pChild = pSpreadsheet->children; pChild; pChild = pChild->next)
    {
        if (matchesNode(pChild, pTableNs, "calculation-settings"))
        {
            parseCalculationSettings(pChild, aResult.maWorkbook);
            continue;
        }

        if (matchesNode(pChild, pTableNs, "named-expressions"))
        {
            parseNamedExpressions(pChild, {}, aResult.maWorkbook);
            continue;
        }

        if (matchesNode(pChild, pTableNs, "table"))
            parseSheet(pChild, aResult.maWorkbook);
    }

    resolveImportedSheets(aResult, aNormalizedPath, rActiveLoads);
    aRemovePath();
    return api::ValueResult<LoadResult>::success(aResult);
}

} // namespace

api::ValueResult<LoadResult> loadWorkbook(std::string_view rPath)
{
    std::set<std::filesystem::path> aActiveLoads;
    return loadWorkbookRecursive(std::filesystem::path(std::string(rPath)), aActiveLoads);
}

} // namespace spreadsheetengine::core::fods

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
