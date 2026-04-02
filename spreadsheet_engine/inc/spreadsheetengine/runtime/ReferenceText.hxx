/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>

namespace spreadsheetengine::runtime::referencetext
{

[[nodiscard]] inline bool needsQuotedSheetName(api::StringView rSheetName)
{
    if (rSheetName.empty())
        return false;

    for (const char16_t cChar : rSheetName)
    {
        const bool bAlphaNum = (cChar >= u'0' && cChar <= u'9')
                               || (cChar >= u'A' && cChar <= u'Z')
                               || (cChar >= u'a' && cChar <= u'z') || cChar == u'_';
        if (!bAlphaNum)
            return true;
    }

    return false;
}

[[nodiscard]] inline api::String quoteSheetNameForFormula(api::StringView rSheetName)
{
    if (!needsQuotedSheetName(rSheetName))
        return api::String(rSheetName);

    api::String aQuoted;
    aQuoted.reserve(rSheetName.size() + 2);
    aQuoted.push_back(u'\'');
    for (const char16_t cChar : rSheetName)
    {
        if (cChar == u'\'')
            aQuoted.push_back(u'\'');
        aQuoted.push_back(cChar);
    }
    aQuoted.push_back(u'\'');
    return aQuoted;
}

[[nodiscard]] inline api::String unquoteSheetName(api::StringView rSheetName)
{
    if (rSheetName.size() < 2 || rSheetName.front() != u'\'' || rSheetName.back() != u'\'')
        return api::String(rSheetName);

    api::String aResult;
    aResult.reserve(rSheetName.size() - 2);
    for (std::size_t nIndex = 1; nIndex + 1 < rSheetName.size(); ++nIndex)
    {
        if (rSheetName[nIndex] == u'\''
            && nIndex + 1 < rSheetName.size() - 1
            && rSheetName[nIndex + 1] == u'\'')
        {
            aResult.push_back(u'\'');
            ++nIndex;
            continue;
        }

        aResult.push_back(rSheetName[nIndex]);
    }
    return aResult;
}

[[nodiscard]] inline api::String formatPositiveInteger(std::int64_t nValue)
{
    const std::string aAscii = std::to_string(nValue);
    api::String aResult;
    aResult.reserve(aAscii.size());
    for (const char cDigit : aAscii)
        aResult.push_back(static_cast<char16_t>(cDigit));
    return aResult;
}

[[nodiscard]] inline api::String columnNameFromIndex(api::ColumnIndex nColumn)
{
    api::String aName;
    api::ColumnIndex nCurrent = nColumn;
    do
    {
        const api::ColumnIndex nRemainder = nCurrent % 26;
        aName.insert(aName.begin(), static_cast<char16_t>(u'A' + nRemainder));
        nCurrent = (nCurrent / 26) - 1;
    } while (nCurrent >= 0);
    return aName;
}

[[nodiscard]] inline bool looksLikeA1AddressToken(api::StringView rToken)
{
    if (rToken.empty())
        return false;

    while (!rToken.empty() && rToken.front() == u'$')
        rToken.remove_prefix(1);

    std::size_t nColumnEnd = 0;
    while (nColumnEnd < rToken.size())
    {
        const char16_t cChar = rToken[nColumnEnd];
        if (!((cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z')))
            break;
        ++nColumnEnd;
    }
    if (nColumnEnd == 0 || nColumnEnd >= rToken.size())
        return false;

    api::StringView aRowToken = rToken.substr(nColumnEnd);
    if (!aRowToken.empty() && aRowToken.front() == u'$')
        aRowToken.remove_prefix(1);
    if (aRowToken.empty())
        return false;

    for (const char16_t cChar : aRowToken)
    {
        if (cChar < u'0' || cChar > u'9')
            return false;
    }

    return true;
}

[[nodiscard]] inline api::String prefixImplicitSheet(api::StringView rToken)
{
    api::String aResult = u".";
    aResult += rToken;
    return aResult;
}

[[nodiscard]] inline std::optional<api::String> normalizeIndirectA1ReferenceText(
    api::StringView rText)
{
    const std::size_t nBangPos = rText.rfind(u'!');
    api::StringView aSheetToken;
    api::StringView aAddressToken = rText;
    if (nBangPos != api::StringView::npos)
    {
        aSheetToken = rText.substr(0, nBangPos);
        aAddressToken = rText.substr(nBangPos + 1);
    }

    const auto normalizeRangePart = [&](api::StringView rPart) -> std::optional<api::String> {
        if (rPart.find(u'.') != api::StringView::npos)
            return api::String(rPart);
        if (!looksLikeA1AddressToken(rPart))
            return std::nullopt;
        return prefixImplicitSheet(rPart);
    };

    const std::size_t nColonPos = aAddressToken.find(u':');
    api::String aNormalized;
    if (!aSheetToken.empty())
    {
        aNormalized += aSheetToken;
        aNormalized.push_back(u'.');
    }

    if (nColonPos == api::StringView::npos)
    {
        if (!aSheetToken.empty())
        {
            if (!looksLikeA1AddressToken(aAddressToken))
                return std::nullopt;
            aNormalized += aAddressToken;
            return aNormalized;
        }

        return normalizeRangePart(aAddressToken);
    }

    const auto oStart = normalizeRangePart(aAddressToken.substr(0, nColonPos));
    const auto oEnd = normalizeRangePart(aAddressToken.substr(nColonPos + 1));
    if (!oStart || !oEnd)
        return std::nullopt;

    if (!aSheetToken.empty())
    {
        aNormalized += aAddressToken.substr(0, nColonPos);
        aNormalized.push_back(u':');
        aNormalized += *oEnd;
        return aNormalized;
    }

    aNormalized = *oStart;
    aNormalized.push_back(u':');
    aNormalized += *oEnd;
    return aNormalized;
}

[[nodiscard]] inline std::optional<api::ResolvedReference> parseIndirectR1C1ReferenceText(
    api::StringView rText, const core::workbook::Workbook& rWorkbook, api::SheetId nImplicitSheet)
{
    const auto parsePositiveIndex = [](api::StringView rDigits) -> std::optional<std::int64_t> {
        if (rDigits.empty())
            return std::nullopt;
        std::int64_t nValue = 0;
        for (const char16_t cChar : rDigits)
        {
            if (cChar < u'0' || cChar > u'9')
                return std::nullopt;
            nValue = nValue * 10 + (cChar - u'0');
        }
        return nValue > 0 ? std::optional<std::int64_t>(nValue) : std::nullopt;
    };

    const std::size_t nBangPos = rText.rfind(u'!');
    api::StringView aSheetToken;
    api::StringView aAddressToken = rText;
    if (nBangPos != api::StringView::npos)
    {
        aSheetToken = rText.substr(0, nBangPos);
        aAddressToken = rText.substr(nBangPos + 1);
    }

    if (aAddressToken.size() < 4 || (aAddressToken[0] != u'R' && aAddressToken[0] != u'r'))
        return std::nullopt;

    std::size_t nIndex = 1;
    const std::size_t nRowStart = nIndex;
    while (nIndex < aAddressToken.size() && aAddressToken[nIndex] >= u'0'
           && aAddressToken[nIndex] <= u'9')
    {
        ++nIndex;
    }
    if (nIndex == nRowStart || nIndex >= aAddressToken.size()
        || (aAddressToken[nIndex] != u'C' && aAddressToken[nIndex] != u'c'))
    {
        return std::nullopt;
    }

    const auto oRow = parsePositiveIndex(aAddressToken.substr(nRowStart, nIndex - nRowStart));
    if (!oRow || *oRow < 1)
        return std::nullopt;

    ++nIndex;
    const std::size_t nColumnStart = nIndex;
    while (nIndex < aAddressToken.size() && aAddressToken[nIndex] >= u'0'
           && aAddressToken[nIndex] <= u'9')
    {
        ++nIndex;
    }
    if (nIndex != aAddressToken.size() || nIndex == nColumnStart)
        return std::nullopt;

    const auto oColumn = parsePositiveIndex(
        aAddressToken.substr(nColumnStart, nIndex - nColumnStart));
    if (!oColumn || *oColumn < 1)
        return std::nullopt;

    api::SheetId nSheet = nImplicitSheet;
    if (!aSheetToken.empty())
    {
        const auto oSheetId = rWorkbook.findSheetId(unquoteSheetName(aSheetToken));
        if (!oSheetId)
            return std::nullopt;
        nSheet = *oSheetId;
    }

    api::CellAddress aAddress {
        nSheet,
        static_cast<api::ColumnIndex>(*oColumn - 1),
        static_cast<api::RowIndex>(*oRow - 1),
    };
    return api::ResolvedReference { { aAddress, aAddress } };
}

[[nodiscard]] inline api::String formatAddressFunctionResult(api::RowIndex nRow,
    api::ColumnIndex nColumn, std::int32_t nAbsMode, bool bA1Style, api::StringView rSheetName)
{
    api::String aResult;
    if (!rSheetName.empty())
    {
        aResult = quoteSheetNameForFormula(rSheetName);
        aResult.push_back(bA1Style ? u'.' : u'!');
    }

    const bool bRowAbsolute = nAbsMode == 1 || nAbsMode == 2;
    const bool bColumnAbsolute = nAbsMode == 1 || nAbsMode == 3;
    if (bA1Style)
    {
        if (bColumnAbsolute)
            aResult.push_back(u'$');
        aResult += columnNameFromIndex(nColumn);
        if (bRowAbsolute)
            aResult.push_back(u'$');
        aResult += formatPositiveInteger(static_cast<std::int64_t>(nRow) + 1);
        return aResult;
    }

    aResult.push_back(u'R');
    if (bRowAbsolute)
    {
        aResult += formatPositiveInteger(static_cast<std::int64_t>(nRow) + 1);
    }
    else
    {
        aResult.push_back(u'[');
        aResult += formatPositiveInteger(static_cast<std::int64_t>(nRow) + 1);
        aResult.push_back(u']');
    }

    aResult.push_back(u'C');
    if (bColumnAbsolute)
    {
        aResult += formatPositiveInteger(static_cast<std::int64_t>(nColumn) + 1);
    }
    else
    {
        aResult.push_back(u'[');
        aResult += formatPositiveInteger(static_cast<std::int64_t>(nColumn) + 1);
        aResult.push_back(u']');
    }

    return aResult;
}

} // namespace spreadsheetengine::runtime::referencetext

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
