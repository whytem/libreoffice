/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/LookupCache.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Numeral.hxx>
#include <spreadsheetengine/api/Parsing.hxx>
#include <spreadsheetengine/api/Reference.hxx>
#include <spreadsheetengine/api/Text.hxx>
#include <spreadsheetengine/core/InMemoryHost.hxx>

int main()
{
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::api::CellRange;
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::lookup::SearchMode;
    using spreadsheetengine::api::lookupcache::QueryCriteria;
    using spreadsheetengine::api::lookupcache::QueryOp;
    using spreadsheetengine::api::lookupcache::Result;
    using spreadsheetengine::core::host::InMemoryEvaluationHost;

    if (spreadsheetengine::api::math::abs(-5.0) != 5.0)
        return 1;

    if (spreadsheetengine::api::text::countCodePoints(u"Libre") != 5)
        return 1;

    const auto aRoman = spreadsheetengine::api::numeral::fromRoman(u"XIV");
    if (!aRoman || aRoman.maValue != 14)
        return 1;

    InMemoryEvaluationHost aHost;
    aHost.setLocaleTag(u"en-US");
    aHost.setParsedNumber(u"42.5", 42.5, 11);
    aHost.setParsedNumber(u"2024-01-15", 45306.0, 15,
        spreadsheetengine::api::NumberParseResult::Kind::Date);
    aHost.setParsedNumber(u"06:00", 0.25, 20,
        spreadsheetengine::api::NumberParseResult::Kind::Time,
        spreadsheetengine::api::NumberParseMode::LaxTime);
    aHost.setFormattedNumber(20.0, 7, u"20.00");

    const auto nSheet = aHost.addSheet(u"Sheet1");
    if (!aHost.setCellValue({ nSheet, 0, 0 }, CellValue::text(u"apple"))
        || !aHost.setCellValue({ nSheet, 1, 0 }, CellValue::number(10.0))
        || !aHost.setCellValue({ nSheet, 0, 1 }, CellValue::text(u"banana"))
        || !aHost.setCellValue({ nSheet, 1, 1 }, CellValue::number(20.0))
        || !aHost.setCellValue({ nSheet, 0, 2 }, CellValue::text(u"citrus"))
        || !aHost.setCellValue({ nSheet, 1, 2 }, CellValue::number(30.0)))
    {
        return 1;
    }

    const auto aParsedValue = spreadsheetengine::api::parsing::valueFromText(aHost, u"42.5");
    const auto aParsedDate
        = spreadsheetengine::api::parsing::dateValueFromText(aHost, u"2024-01-15");
    const auto aParsedTime = spreadsheetengine::api::parsing::timeValueFromText(aHost, u"06:00");
    if (!aParsedValue || aParsedValue.maValue != 42.5 || !aParsedDate
        || aParsedDate.maValue != 45306.0 || !aParsedTime || aParsedTime.maValue != 0.25)
    {
        return 1;
    }

    if (spreadsheetengine::api::logic::selectIfBranch(
            aParsedValue.maValue > 40.0, false, true, false)
        != spreadsheetengine::api::logic::IfBranchAction::ThenPath)
    {
        return 1;
    }

    const CellRange aValueRange { CellAddress { nSheet, 1, 0 }, CellAddress { nSheet, 1, 2 } };
    const auto aSelection = spreadsheetengine::api::reference::planIndexReferenceSelection(
        aValueRange, 2, 1, 3);
    if (!aSelection || !aSelection.maValue.maRange.isSingleCell())
        return 1;

    const auto aLookupValue = aHost.getRangeValue(aSelection.maValue.maRange, 0, 0);
    if (!aLookupValue || !aLookupValue.maValue.isNumber() || aLookupValue.maValue.mfNumber != 20.0)
        return 1;

    const auto aCriteria = QueryCriteria::fromString(QueryOp::Equal, SearchMode::Forward, u"banana");
    const auto aCacheEntry = spreadsheetengine::api::lookupcache::makeCacheEntry(
        aCriteria, CellAddress { nSheet, 1, 1 }, true);
    CellAddress aFoundAddress;
    if (spreadsheetengine::api::lookupcache::classifyLookup(aFoundAddress, aCriteria, &aCacheEntry)
            != Result::Found
        || !(aFoundAddress == aSelection.maValue.maRange.maStart))
    {
        return 1;
    }

    const auto aFormatted = aHost.formatNumber(aLookupValue.maValue.mfNumber, 7);
    if (!aFormatted || aFormatted.maValue != u"20.00")
        return 1;

    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
