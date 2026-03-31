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

inline constexpr std::uint16_t kBuiltinExternalNameCatalogId = 1;

[[nodiscard]] constexpr char16_t foldAscii(char16_t c)
{
    return (c >= u'A' && c <= u'Z') ? static_cast<char16_t>(c - u'A' + u'a') : c;
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
    if (equalLookupText(rSymbol, u"MONTHS") || equalLookupText(rSymbol, u"ORG.OPENOFFICE.MONTHS"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETDIFFMONTHS");
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
    if (equalLookupText(rSymbol, u"ACCRINTM"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETACCRINTM");
    if (equalLookupText(rSymbol, u"CONVERT")
        || equalLookupText(rSymbol, u"ORG.OPENOFFICE.CONVERT"))
    {
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCONVERT");
    }
    if (equalLookupText(rSymbol, u"NOMINAL") || equalLookupText(rSymbol, u"NOMINAL_ADD"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETNOMINAL");
    if (equalLookupText(rSymbol, u"DOLLARFR"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDOLLARFR");
    if (equalLookupText(rSymbol, u"DOLLARDE"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDOLLARDE");
    if (equalLookupText(rSymbol, u"PRICE"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETPRICE");
    if (equalLookupText(rSymbol, u"PRICEMAT"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETPRICEMAT");
    if (equalLookupText(rSymbol, u"DISC"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDISC");
    if (equalLookupText(rSymbol, u"RECEIVED"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETRECEIVED");
    if (equalLookupText(rSymbol, u"PRICEDISC"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETPRICEDISC");
    if (equalLookupText(rSymbol, u"INTRATE"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETINTRATE");
    if (equalLookupText(rSymbol, u"YIELDDISC"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETYIELDDISC");
    if (equalLookupText(rSymbol, u"MDURATION"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETMDURATION");
    if (equalLookupText(rSymbol, u"YIELD"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETYIELD");
    if (equalLookupText(rSymbol, u"TBILLPRICE"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETTBILLPRICE");
    if (equalLookupText(rSymbol, u"TBILLEQ"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETTBILLEQ");
    if (equalLookupText(rSymbol, u"TBILLYIELD"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETTBILLYIELD");
    if (equalLookupText(rSymbol, u"FVSCHEDULE"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETFVSCHEDULE");
    if (equalLookupText(rSymbol, u"AMORLINC"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETAMORLINC");
    if (equalLookupText(rSymbol, u"AMORDEGRC"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETAMORDEGRC");
    if (equalLookupText(rSymbol, u"ODDLPRICE"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETODDLPRICE");
    if (equalLookupText(rSymbol, u"ODDLYIELD"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETODDLYIELD");
    if (equalLookupText(rSymbol, u"COUPNCD"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPNCD");
    if (equalLookupText(rSymbol, u"COUPDAYS"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPDAYS");
    if (equalLookupText(rSymbol, u"COUPDAYSNC"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPDAYSNC");
    if (equalLookupText(rSymbol, u"COUPDAYBS"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPDAYBS");
    if (equalLookupText(rSymbol, u"COUPPCD"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPPCD");
    if (equalLookupText(rSymbol, u"COUPNUM"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPNUM");
    if (equalLookupText(rSymbol, u"XIRR"))
        return api::String(u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETXIRR");
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
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETDIFFMONTHS"))
        return api::String(u"MONTHS");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETDIFFWEEKS"))
        return api::String(u"WEEKS");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.DATEFUNCTIONS.GETWEEKSINYEAR"))
        return api::String(u"WEEKSINYEAR");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETSERIESSUM"))
        return api::String(u"SERIESSUM");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETQUOTIENT"))
        return api::String(u"QUOTIENT");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETACCRINTM"))
        return api::String(u"ACCRINTM");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCONVERT"))
        return api::String(u"CONVERT");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETNOMINAL"))
        return api::String(u"NOMINAL");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDOLLARFR"))
        return api::String(u"DOLLARFR");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDOLLARDE"))
        return api::String(u"DOLLARDE");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETPRICE"))
        return api::String(u"PRICE");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETPRICEMAT"))
        return api::String(u"PRICEMAT");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETDISC"))
        return api::String(u"DISC");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETRECEIVED"))
        return api::String(u"RECEIVED");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETPRICEDISC"))
        return api::String(u"PRICEDISC");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETINTRATE"))
        return api::String(u"INTRATE");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETYIELDDISC"))
        return api::String(u"YIELDDISC");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETMDURATION"))
        return api::String(u"MDURATION");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETYIELD"))
        return api::String(u"YIELD");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETTBILLPRICE"))
        return api::String(u"TBILLPRICE");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETTBILLEQ"))
        return api::String(u"TBILLEQ");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETTBILLYIELD"))
        return api::String(u"TBILLYIELD");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETFVSCHEDULE"))
        return api::String(u"FVSCHEDULE");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETAMORLINC"))
        return api::String(u"AMORLINC");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETAMORDEGRC"))
        return api::String(u"AMORDEGRC");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETODDLPRICE"))
        return api::String(u"ODDLPRICE");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETODDLYIELD"))
        return api::String(u"ODDLYIELD");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPNCD"))
        return api::String(u"COUPNCD");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPDAYS"))
        return api::String(u"COUPDAYS");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPDAYSNC"))
        return api::String(u"COUPDAYSNC");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPDAYBS"))
        return api::String(u"COUPDAYBS");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPPCD"))
        return api::String(u"COUPPCD");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCOUPNUM"))
        return api::String(u"COUPNUM");
    if (equalLookupText(rName, u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETXIRR"))
        return api::String(u"XIRR");
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
