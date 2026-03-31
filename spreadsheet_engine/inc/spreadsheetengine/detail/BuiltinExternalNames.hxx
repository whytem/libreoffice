/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <spreadsheetengine/api/String.hxx>

namespace spreadsheetengine::detail::compiler
{

inline constexpr sal_uInt16 kBuiltinExternalNameCatalogId = 1;

[[nodiscard]] constexpr sal_Unicode foldAscii(sal_Unicode c)
{
    return (c >= u'A' && c <= u'Z') ? static_cast<sal_Unicode>(c - u'A' + u'a') : c;
}

[[nodiscard]] inline bool equalLookupText(api::StringView rLeft, api::StringView rRight)
{
    if (rLeft.size() != rRight.size())
        return false;

    for (std::size_t nIndex = 0; nIndex < rLeft.size(); ++nIndex)
    {
        if (foldAscii(rLeft[nIndex]) != foldAscii(rRight[nIndex]))
            return false;
    }

    return true;
}

[[nodiscard]] inline std::optional<api::String> lookupBuiltinExternalName(api::StringView rSymbol)
{
    if (equalLookupText(rSymbol, u"WORKDAY"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETWORKDAY");
    if (equalLookupText(rSymbol, u"YEARFRAC"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETYEARFRAC");
    if (equalLookupText(rSymbol, u"YEARS") || equalLookupText(rSymbol, u"ORG.OPENOFFICE.YEARS"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETDIFFYEARS");
    if (equalLookupText(rSymbol, u"WEEKS") || equalLookupText(rSymbol, u"ORG.OPENOFFICE.WEEKS"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETDIFFWEEKS");
    if (equalLookupText(rSymbol, u"WEEKSINYEAR")
        || equalLookupText(rSymbol, u"ORG.OPENOFFICE.WEEKSINYEAR"))
    {
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETWEEKSINYEAR");
    }
    if (equalLookupText(rSymbol, u"SERIESSUM"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETSERIESSUM");
    if (equalLookupText(rSymbol, u"QUOTIENT"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETQUOTIENT");
    if (equalLookupText(rSymbol, u"CONVERT")
        || equalLookupText(rSymbol, u"ORG.OPENOFFICE.CONVERT"))
    {
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCONVERT");
    }
    if (equalLookupText(rSymbol, u"AMORLINC"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETAMORLINC");
    if (equalLookupText(rSymbol, u"ODDLYIELD"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETODDLYIELD");
    if (equalLookupText(rSymbol, u"DEC2HEX"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDEC2HEX");
    if (equalLookupText(rSymbol, u"MROUND"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETMROUND");
    if (equalLookupText(rSymbol, u"MULTINOMIAL"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETMULTINOMIAL");
    if (equalLookupText(rSymbol, u"SQRTPI"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETSQRTPI");
    if (equalLookupText(rSymbol, u"RANDBETWEEN"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETRANDBETWEEN");

    return std::nullopt;
}

[[nodiscard]] inline std::optional<api::String> lookupBuiltinExternalSymbol(api::StringView rName)
{
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETWORKDAY"))
        return api::String(u"WORKDAY");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETYEARFRAC"))
        return api::String(u"YEARFRAC");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETDIFFYEARS"))
        return api::String(u"YEARS");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETDIFFWEEKS"))
        return api::String(u"WEEKS");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETWEEKSINYEAR"))
        return api::String(u"WEEKSINYEAR");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETSERIESSUM"))
        return api::String(u"SERIESSUM");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETQUOTIENT"))
        return api::String(u"QUOTIENT");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCONVERT"))
        return api::String(u"CONVERT");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETAMORLINC"))
        return api::String(u"AMORLINC");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETODDLYIELD"))
        return api::String(u"ODDLYIELD");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDEC2HEX"))
        return api::String(u"DEC2HEX");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETMROUND"))
        return api::String(u"MROUND");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETMULTINOMIAL"))
        return api::String(u"MULTINOMIAL");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETSQRTPI"))
        return api::String(u"SQRTPI");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETRANDBETWEEN"))
        return api::String(u"RANDBETWEEN");

    return std::nullopt;
}

} // namespace spreadsheetengine::detail::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
