/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/LookupCache.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Parsing.hxx>
#include <spreadsheetengine/api/Reference.hxx>
#include <spreadsheetengine/core/Phase0.hxx>
#include <spreadsheetengine/core/InMemoryHost.hxx>

namespace
{

bool almostEqual(double fLeft, double fRight)
{
    const double fScale = std::max({ 1.0, std::fabs(fLeft), std::fabs(fRight) });
    return std::fabs(fLeft - fRight) <= (1.0e-9 * fScale);
}

}

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::api::CellRange;
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::api::math::Sign;
    using spreadsheetengine::api::lookup::SearchMode;
    using spreadsheetengine::api::lookupcache::QueryCriteria;
    using spreadsheetengine::api::lookupcache::QueryOp;
    using spreadsheetengine::api::lookupcache::Result;
    using spreadsheetengine::api::math::arcTan2;
    using spreadsheetengine::api::math::bitAnd;
    using spreadsheetengine::api::math::bitRightShift;
    using spreadsheetengine::api::math::ceiling;
    using spreadsheetengine::api::math::degrees;
    using spreadsheetengine::api::math::futureValue;
    using spreadsheetengine::api::math::modulo;
    using spreadsheetengine::api::math::payment;
    using spreadsheetengine::api::math::presentValue;
    using spreadsheetengine::api::math::radians;
    using spreadsheetengine::api::math::roundToDecimals;
    using spreadsheetengine::api::math::sign;
    using spreadsheetengine::api::math::squareRoot;
    using spreadsheetengine::core::isLibraryLinked;
    using spreadsheetengine::core::host::InMemoryEvaluationHost;

    auto fail = [](const char* pMessage) {
        std::cerr << "spreadsheetengine_smoke: " << pMessage << '\n';
        return EXIT_FAILURE;
    };

    if (!isLibraryLinked())
        return fail("Phase0 library probe failed");

    if (sign(-42.0) != Sign::Negative || sign(0.0) != Sign::Zero
        || sign(7.0) != Sign::Positive)
    {
        return fail("sign() returned unexpected results");
    }

    const auto aMod = modulo(10.0, 3.0);
    if (!aMod || !almostEqual(aMod.maValue, 1.0))
        return fail("modulo() returned unexpected result");

    const auto aBitAnd = bitAnd(6.0, 3.0);
    if (!aBitAnd || !almostEqual(aBitAnd.maValue, 2.0))
        return fail("bitAnd() returned unexpected result");

    const auto aBitShift = bitRightShift(16.0, 2.0);
    if (!aBitShift || !almostEqual(aBitShift.maValue, 4.0))
        return fail("bitRightShift() returned unexpected result");

    if (!almostEqual(arcTan2(0.0, 1.0), 0.0))
        return fail("arcTan2() returned unexpected result");

    if (!almostEqual(degrees(radians(45.0)), 45.0))
        return fail("degree/radian conversion returned unexpected result");

    if (!almostEqual(roundToDecimals(12.345, 2, spreadsheetengine::api::RoundingMode::Corrected), 12.35))
        return fail("roundToDecimals() returned unexpected result");

    const auto aCeiling = ceiling(4.1, 2.0, true, false);
    if (!aCeiling || !almostEqual(aCeiling.maValue, 6.0))
        return fail("ceiling() returned unexpected result");

    const auto aSqrt = squareRoot(81.0);
    if (!aSqrt || !almostEqual(aSqrt.maValue, 9.0))
        return fail("squareRoot() returned unexpected result");

    const auto aNegativeSqrt = squareRoot(-1.0);
    if (aNegativeSqrt || aNegativeSqrt.meError != Error::Domain)
        return fail("squareRoot() should reject negative input with Domain error");

    const double fPmt = payment(0.01, 12.0, 1000.0, 0.0, false);
    const double fFv = futureValue(0.01, 12.0, fPmt, 1000.0, false);
    if (!almostEqual(fFv, 0.0))
        return fail("futureValue()/payment() returned inconsistent result");

    const double fPv = presentValue(0.01, 12.0, fPmt, 0.0, false);
    if (!almostEqual(fPv, 1000.0))
        return fail("presentValue()/payment() returned inconsistent result");

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
        return fail("InMemoryHost setup failed");
    }

    const auto aParsedValue = spreadsheetengine::api::parsing::valueFromText(aHost, u"42.5");
    const auto aParsedDate
        = spreadsheetengine::api::parsing::dateValueFromText(aHost, u"2024-01-15");
    const auto aParsedTime = spreadsheetengine::api::parsing::timeValueFromText(aHost, u"06:00");
    if (!aParsedValue || !almostEqual(aParsedValue.maValue, 42.5) || !aParsedDate
        || !almostEqual(aParsedDate.maValue, 45306.0) || !aParsedTime
        || !almostEqual(aParsedTime.maValue, 0.25))
    {
        return fail("host-backed parsing returned unexpected result");
    }

    if (spreadsheetengine::api::logic::selectIfBranch(
            aParsedValue.maValue > 40.0, false, true, false)
        != spreadsheetengine::api::logic::IfBranchAction::ThenPath)
    {
        return fail("IF branch selection returned unexpected result");
    }

    const CellRange aValueRange { CellAddress { nSheet, 1, 0 }, CellAddress { nSheet, 1, 2 } };
    const auto aSelection = spreadsheetengine::api::reference::planIndexReferenceSelection(
        aValueRange, 2, 1, 3);
    if (!aSelection || !aSelection.maValue.maRange.isSingleCell())
        return fail("INDEX reference planning returned unexpected result");

    const auto aLookupValue = aHost.getRangeValue(aSelection.maValue.maRange, 0, 0);
    if (!aLookupValue || !aLookupValue.maValue.isNumber()
        || !almostEqual(aLookupValue.maValue.mfNumber, 20.0))
    {
        return fail("resolved range lookup returned unexpected result");
    }

    const auto aCriteria = QueryCriteria::fromString(QueryOp::Equal, SearchMode::Forward, u"banana");
    const auto aCacheEntry = spreadsheetengine::api::lookupcache::makeCacheEntry(
        aCriteria, CellAddress { nSheet, 1, 1 }, true);
    CellAddress aFoundAddress;
    if (spreadsheetengine::api::lookupcache::classifyLookup(aFoundAddress, aCriteria, &aCacheEntry)
            != Result::Found
        || !(aFoundAddress == aSelection.maValue.maRange.maStart))
    {
        return fail("lookup cache classification returned unexpected result");
    }

    const auto aFormatted = aHost.formatNumber(aLookupValue.maValue.mfNumber, 7);
    if (!aFormatted || aFormatted.maValue != u"20.00")
        return fail("host-backed formatting returned unexpected result");

    std::cout << "spreadsheetengine smoke test passed\n";
    return EXIT_SUCCESS;
}
