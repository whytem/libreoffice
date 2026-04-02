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

#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::runtime::cellinspection
{

enum class InfoKind : std::uint8_t
{
    Unsupported,
    Column,
    Row,
    Sheet,
    Address,
    Coord,
    Contents,
    Type,
    Filename,
    Width,
    Prefix,
    Protect,
    Format,
    Color,
    Parentheses
};

enum class PrefixStyle : std::uint8_t
{
    None,
    Left,
    Center,
    Right,
    Repeat
};

[[nodiscard]] inline api::String uppercaseAscii(api::StringView rText)
{
    api::String aUpper;
    aUpper.reserve(rText.size());
    for (const char16_t cChar : rText)
    {
        if (cChar >= u'a' && cChar <= u'z')
            aUpper.push_back(static_cast<char16_t>(cChar - u'a' + u'A'));
        else
            aUpper.push_back(cChar);
    }
    return aUpper;
}

[[nodiscard]] inline InfoKind classifyInfoType(api::StringView rInfoType)
{
    const api::String aUpper = uppercaseAscii(rInfoType);
    if (aUpper == u"COL")
        return InfoKind::Column;
    if (aUpper == u"ROW")
        return InfoKind::Row;
    if (aUpper == u"SHEET")
        return InfoKind::Sheet;
    if (aUpper == u"ADDRESS")
        return InfoKind::Address;
    if (aUpper == u"COORD")
        return InfoKind::Coord;
    if (aUpper == u"CONTENTS")
        return InfoKind::Contents;
    if (aUpper == u"TYPE")
        return InfoKind::Type;
    if (aUpper == u"FILENAME")
        return InfoKind::Filename;
    if (aUpper == u"WIDTH")
        return InfoKind::Width;
    if (aUpper == u"PREFIX")
        return InfoKind::Prefix;
    if (aUpper == u"PROTECT")
        return InfoKind::Protect;
    if (aUpper == u"FORMAT")
        return InfoKind::Format;
    if (aUpper == u"COLOR")
        return InfoKind::Color;
    if (aUpper == u"PARENTHESES")
        return InfoKind::Parentheses;
    return InfoKind::Unsupported;
}

[[nodiscard]] inline bool isHostPropertyInfoKind(InfoKind eKind)
{
    switch (eKind)
    {
        case InfoKind::Filename:
        case InfoKind::Width:
        case InfoKind::Prefix:
        case InfoKind::Protect:
        case InfoKind::Format:
        case InfoKind::Color:
        case InfoKind::Parentheses:
            return true;
        case InfoKind::Unsupported:
        case InfoKind::Column:
        case InfoKind::Row:
        case InfoKind::Sheet:
        case InfoKind::Address:
        case InfoKind::Coord:
        case InfoKind::Contents:
        case InfoKind::Type:
            return false;
    }
    return false;
}

[[nodiscard]] inline api::CellValue columnValue(const api::CellAddress& rAddress)
{
    return api::CellValue::number(static_cast<double>(rAddress.mnColumn + 1));
}

[[nodiscard]] inline api::CellValue rowValue(const api::CellAddress& rAddress)
{
    return api::CellValue::number(static_cast<double>(rAddress.mnRow + 1));
}

[[nodiscard]] inline api::CellValue sheetValue(const api::CellAddress& rAddress)
{
    return api::CellValue::number(static_cast<double>(rAddress.mnSheet + 1));
}

[[nodiscard]] inline api::CellValue contentsValue(const api::CellValue& rValue)
{
    if (rValue.isText())
        return api::CellValue::text(rValue.maString);
    if (rValue.isError())
        return api::CellValue::error(rValue.meError);
    if (rValue.isNumber() || rValue.isBoolean())
        return api::CellValue::number(rValue.mfNumber);
    return api::CellValue::number(0.0);
}

[[nodiscard]] inline api::CellValue typeValue(const api::CellValue& rValue)
{
    char16_t cType = u'b';
    if (rValue.isText())
        cType = u'l';
    else if (rValue.isNumber() || rValue.isBoolean())
        cType = u'v';

    api::String aResult;
    aResult.push_back(cType);
    return api::CellValue::text(aResult);
}

[[nodiscard]] inline api::CellValue numericPropertyValue(double fValue)
{
    return api::CellValue::number(fValue);
}

[[nodiscard]] inline api::CellValue textPropertyValue(api::StringView rText)
{
    return api::CellValue::text(api::String(rText));
}

[[nodiscard]] inline api::CellValue prefixValue(PrefixStyle eStyle)
{
    char16_t cPrefix = 0;
    switch (eStyle)
    {
        case PrefixStyle::Left:
            cPrefix = u'\'';
            break;
        case PrefixStyle::Center:
            cPrefix = u'^';
            break;
        case PrefixStyle::Right:
            cPrefix = u'"';
            break;
        case PrefixStyle::Repeat:
            cPrefix = u'\\';
            break;
        case PrefixStyle::None:
            break;
    }

    api::String aResult;
    if (cPrefix != 0)
        aResult.push_back(cPrefix);
    return api::CellValue::text(aResult);
}

} // namespace spreadsheetengine::runtime::cellinspection

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
