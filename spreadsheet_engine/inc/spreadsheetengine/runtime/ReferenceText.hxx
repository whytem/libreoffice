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
#include <string>

#include <spreadsheetengine/api/Host.hxx>

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
