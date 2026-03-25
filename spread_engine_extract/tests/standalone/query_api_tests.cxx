/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/Query.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::query::CellClass;
    using spreadsheetengine::api::query::ComparisonRoute;
    using spreadsheetengine::api::query::OperandKind;
    using spreadsheetengine::api::query::Operator;
    using spreadsheetengine::api::query::SearchType;
    using spreadsheetengine::api::query::StringIdentity;
    using spreadsheetengine::standalone::test::fail;

    if (!spreadsheetengine::api::query::isPartialTextMatchOp(Operator::Contains)
        || !spreadsheetengine::api::query::isPartialTextMatchOp(Operator::EndsWith)
        || spreadsheetengine::api::query::isPartialTextMatchOp(Operator::Equal))
    {
        return fail("spreadsheetengine_query_tests", "partial text operator classification mismatch");
    }

    if (!spreadsheetengine::api::query::isTextMatchOp(Operator::Equal)
        || !spreadsheetengine::api::query::isTextMatchOp(Operator::NotEqual)
        || !spreadsheetengine::api::query::isTextMatchOp(Operator::BeginsWith)
        || spreadsheetengine::api::query::isTextMatchOp(Operator::LessEqual))
    {
        return fail("spreadsheetengine_query_tests", "text operator classification mismatch");
    }

    if (!spreadsheetengine::api::query::isEndsWithOp(Operator::EndsWith)
        || !spreadsheetengine::api::query::isEndsWithOp(Operator::DoesNotEndWith)
        || spreadsheetengine::api::query::isEndsWithOp(Operator::Contains))
    {
        return fail("spreadsheetengine_query_tests", "ends-with operator classification mismatch");
    }

    if (!spreadsheetengine::api::query::isMatchWholeCell(true, Operator::Equal)
        || spreadsheetengine::api::query::isMatchWholeCell(true, Operator::Contains)
        || spreadsheetengine::api::query::isMatchWholeCell(false, Operator::GreaterEqual))
    {
        return fail("spreadsheetengine_query_tests", "match-whole-cell policy mismatch");
    }

    if (!spreadsheetengine::api::query::isRealWildOrRegExp(SearchType::Wildcard, Operator::Equal)
        || spreadsheetengine::api::query::isRealWildOrRegExp(SearchType::Normal, Operator::Equal)
        || !spreadsheetengine::api::query::isTestWildOrRegExp(
               true, SearchType::Regex, Operator::GreaterEqual)
        || spreadsheetengine::api::query::isTestWildOrRegExp(
               false, SearchType::Regex, Operator::GreaterEqual)
        || spreadsheetengine::api::query::isTestWildOrRegExp(
               true, SearchType::Wildcard, Operator::Equal))
    {
        return fail("spreadsheetengine_query_tests", "wildcard or regexp policy mismatch");
    }

    const CellClass aNumericCell { true, false, false };
    const CellClass aStringCell { false, true, false };
    const CellClass aFormulaErrorCell { true, false, true };
    const CellClass aEmptyCell { false, false, false };

    if (!spreadsheetengine::api::query::isQueryByValueForCell(aNumericCell)
        || spreadsheetengine::api::query::isQueryByValueForCell(aFormulaErrorCell))
    {
        return fail("spreadsheetengine_query_tests", "value-cell classification mismatch");
    }

    if (!spreadsheetengine::api::query::isQueryByValue(
            Operator::LessEqual, OperandKind::Value, aNumericCell)
        || spreadsheetengine::api::query::isQueryByValue(
               Operator::Contains, OperandKind::Value, aNumericCell)
        || spreadsheetengine::api::query::isQueryByValue(
               Operator::Equal, OperandKind::Text, aNumericCell))
    {
        return fail("spreadsheetengine_query_tests", "query-by-value policy mismatch");
    }

    if (!spreadsheetengine::api::query::isQueryByString(
               Operator::Equal, OperandKind::Value, aEmptyCell)
        || !spreadsheetengine::api::query::isQueryByString(
               Operator::Contains, OperandKind::Value, aEmptyCell)
        || !spreadsheetengine::api::query::isQueryByString(
               Operator::LessEqual, OperandKind::Text, aStringCell)
        || spreadsheetengine::api::query::isQueryByString(
               Operator::Greater, OperandKind::Text, aFormulaErrorCell)
        || spreadsheetengine::api::query::isQueryByString(
               Operator::Greater, OperandKind::Value, aNumericCell))
    {
        return fail("spreadsheetengine_query_tests", "query-by-string policy mismatch");
    }

    if (spreadsheetengine::api::query::classifyComparisonRoute(
            Operator::Equal, OperandKind::TextColor, aNumericCell, true)
            != ComparisonRoute::TextColor
        || spreadsheetengine::api::query::classifyComparisonRoute(
               Operator::Equal, OperandKind::BackgroundColor, aNumericCell, true)
               != ComparisonRoute::BackgroundColor
        || spreadsheetengine::api::query::classifyComparisonRoute(
               Operator::LessEqual, OperandKind::Value, aNumericCell, true)
               != ComparisonRoute::Value
        || spreadsheetengine::api::query::classifyComparisonRoute(
               Operator::Equal, OperandKind::Value, aEmptyCell, false)
               != ComparisonRoute::String
        || spreadsheetengine::api::query::classifyComparisonRoute(
               Operator::GreaterEqual, OperandKind::Value, aStringCell, true)
               != ComparisonRoute::RangeLookup
        || spreadsheetengine::api::query::classifyComparisonRoute(
               Operator::GreaterEqual, OperandKind::Value, aStringCell, false)
               != ComparisonRoute::None)
    {
        return fail("spreadsheetengine_query_tests", "comparison route classification mismatch");
    }

    if (!spreadsheetengine::api::query::shouldStopAfterItemResult(
            { true, true }, true)
        || spreadsheetengine::api::query::shouldStopAfterItemResult(
            { true, false }, true)
        || !spreadsheetengine::api::query::shouldStopAfterItemResult(
            { true, false }, false)
        || !spreadsheetengine::api::query::shouldShortCircuitAndEntry(
            true, false, true, false)
        || spreadsheetengine::api::query::shouldShortCircuitAndEntry(
            true, true, true, false)
        || spreadsheetengine::api::query::shouldShortCircuitAndEntry(
            false, false, true, false)
        || spreadsheetengine::api::query::combineConnectedResult(
               true, { true, true }, { false, true })
               != spreadsheetengine::api::query::QueryResult { false, true }
        || spreadsheetengine::api::query::combineConnectedResult(
               false, { false, false }, { true, false })
               != spreadsheetengine::api::query::QueryResult { true, false })
    {
        return fail("spreadsheetengine_query_tests", "query result aggregation mismatch");
    }

    if (!spreadsheetengine::api::query::shouldTryMultiEqualityFastPath(Operator::Equal, 10)
        || spreadsheetengine::api::query::shouldTryMultiEqualityFastPath(Operator::Greater, 10)
        || spreadsheetengine::api::query::shouldTryMultiEqualityFastPath(Operator::Equal, 9)
        || !spreadsheetengine::api::query::shouldUseFastStringEqualityPath(
               Operator::Equal, false, false, true)
        || spreadsheetengine::api::query::shouldUseFastStringEqualityPath(
               Operator::Equal, true, false, true)
        || spreadsheetengine::api::query::shouldUseFastStringEqualityPath(
               Operator::Equal, false, true, true)
        || spreadsheetengine::api::query::shouldUseFastStringEqualityPath(
               Operator::Equal, false, false, false)
        || !spreadsheetengine::api::query::shouldUseExactStringEqualityPath(true, false)
        || !spreadsheetengine::api::query::shouldUseExactStringEqualityPath(false, true)
        || spreadsheetengine::api::query::shouldUseExactStringEqualityPath(false, false)
        || !spreadsheetengine::api::query::shouldRunPatternSearchPrepass(
            false, true, false)
        || !spreadsheetengine::api::query::shouldRunPatternSearchPrepass(
            false, false, true)
        || spreadsheetengine::api::query::shouldRunPatternSearchPrepass(
            true, true, true)
        || !spreadsheetengine::api::query::shouldRunPostPatternStringComparison(
            false, false)
        || spreadsheetengine::api::query::shouldRunPostPatternStringComparison(
            false, true)
        || !spreadsheetengine::api::query::shouldUseTextMatchComparisonPath(
            false, Operator::Equal)
        || spreadsheetengine::api::query::shouldUseTextMatchComparisonPath(
            false, Operator::Less)
        || !spreadsheetengine::api::query::shouldUseSortedItemCache(100)
        || spreadsheetengine::api::query::shouldUseSortedItemCache(99))
    {
        return fail("spreadsheetengine_query_tests", "query cache threshold mismatch");
    }

    if (!spreadsheetengine::api::query::shouldUseStringIdentityMultiEqualityFastPath(
            true, Operator::Equal, 10)
        || spreadsheetengine::api::query::shouldUseStringIdentityMultiEqualityFastPath(
            false, Operator::Equal, 10)
        || spreadsheetengine::api::query::shouldUseStringIdentityMultiEqualityFastPath(
            true, Operator::Equal, 9)
        || !spreadsheetengine::api::query::shouldCompareValueOperandAsString(aStringCell)
        || spreadsheetengine::api::query::shouldCompareValueOperandAsString(aNumericCell)
        || !spreadsheetengine::api::query::shouldCompareValueOperandAsString(aFormulaErrorCell)
        || !spreadsheetengine::api::query::shouldIncludeOperandInStringIdentityCache(
            OperandKind::Text, false)
        || spreadsheetengine::api::query::shouldIncludeOperandInStringIdentityCache(
            OperandKind::Value, false)
        || !spreadsheetengine::api::query::shouldIncludeOperandInStringIdentityCache(
            OperandKind::Value, true))
    {
        return fail("spreadsheetengine_query_tests", "string identity fast path policy mismatch");
    }

    if (!spreadsheetengine::api::query::isWholeCellSearchMatch(true, true, 0, 4, 4)
        || spreadsheetengine::api::query::isWholeCellSearchMatch(true, true, 1, 4, 4)
        || spreadsheetengine::api::query::makePatternSearchPlan(Operator::Contains, 6)
               != spreadsheetengine::api::query::PatternSearchPlan { false, 0, 6 }
        || spreadsheetengine::api::query::makePatternSearchPlan(Operator::EndsWith, 6)
               != spreadsheetengine::api::query::PatternSearchPlan { true, 6, 0 }
        || !spreadsheetengine::api::query::shouldRejectAssignedEmptyStringQuery(
               OperandKind::Value, true)
        || spreadsheetengine::api::query::shouldRejectAssignedEmptyStringQuery(
               OperandKind::Text, true)
        || spreadsheetengine::api::query::computeSubstringSearchStart(
               Operator::Contains, 6, 2)
               != 0
        || spreadsheetengine::api::query::computeSubstringSearchStart(
               Operator::EndsWith, 6, 2)
               != 4
        || !spreadsheetengine::api::query::evaluatePatternSearchMatch(
               Operator::Contains, true, 2, 4, 6)
        || !spreadsheetengine::api::query::evaluatePatternSearchMatch(
               Operator::DoesNotContain, false, 0, 0, 6)
        || !spreadsheetengine::api::query::evaluatePatternSearchMatch(
               Operator::BeginsWith, true, 0, 2, 6)
        || !spreadsheetengine::api::query::evaluatePatternSearchMatch(
               Operator::EndsWith, true, 4, 6, 6)
        || !spreadsheetengine::api::query::evaluateEqualityMatch(Operator::Equal, true)
        || !spreadsheetengine::api::query::evaluateEqualityMatch(Operator::NotEqual, false)
        || !spreadsheetengine::api::query::evaluateSubstringMatch(Operator::Contains, 3)
        || !spreadsheetengine::api::query::evaluateSubstringMatch(Operator::DoesNotContain, -1)
        || !spreadsheetengine::api::query::evaluateSubstringMatch(Operator::BeginsWith, 0)
        || !spreadsheetengine::api::query::evaluateSubstringMatch(Operator::EndsWith, 2)
        || spreadsheetengine::api::query::evaluatePatternSearchOutcome(
               Operator::Contains, true, false, true, 2, 4, 6)
               != spreadsheetengine::api::query::PatternSearchOutcome { true, false }
        || spreadsheetengine::api::query::evaluatePatternSearchOutcome(
               Operator::LessEqual, false, true, true, 0, 6, 6)
               != spreadsheetengine::api::query::PatternSearchOutcome { false, true }
        || spreadsheetengine::api::query::evaluateOrderedStringCompare(Operator::Less, -1)
                   != spreadsheetengine::api::query::OrderedCompareResult { true, false }
        || spreadsheetengine::api::query::evaluateOrderedStringCompare(Operator::GreaterEqual, 0)
                   != spreadsheetengine::api::query::OrderedCompareResult { true, true })
    {
        return fail("spreadsheetengine_query_tests", "string match policy mismatch");
    }

    if (!spreadsheetengine::api::query::isRangeLookupStringOperand(OperandKind::Text)
        || spreadsheetengine::api::query::isRangeLookupStringOperand(OperandKind::Value)
        || !spreadsheetengine::api::query::isRangeLookupComparisonSupported(
               Operator::LessEqual, OperandKind::Text)
        || spreadsheetengine::api::query::isRangeLookupComparisonSupported(
               Operator::GreaterEqual, OperandKind::Text)
        || !spreadsheetengine::api::query::isRangeLookupComparisonSupported(
               Operator::GreaterEqual, OperandKind::Value)
        || spreadsheetengine::api::query::isRangeLookupComparisonSupported(
               Operator::LessEqual, OperandKind::Value)
        || !spreadsheetengine::api::query::evaluateRangeLookupMatch(
               Operator::LessEqual, OperandKind::Text, aNumericCell)
        || spreadsheetengine::api::query::evaluateRangeLookupMatch(
               Operator::LessEqual, OperandKind::Text, aFormulaErrorCell)
        || !spreadsheetengine::api::query::evaluateRangeLookupMatch(
               Operator::GreaterEqual, OperandKind::Value, aStringCell)
        || spreadsheetengine::api::query::evaluateRangeLookupMatch(
               Operator::GreaterEqual, OperandKind::Value, aNumericCell))
    {
        return fail("spreadsheetengine_query_tests", "range lookup policy mismatch");
    }

    struct NumericItem
    {
        bool mbNumeric;
        double mfValue;
    };
    const std::vector<NumericItem> aNumericItems
        = { { true, 7.0 }, { false, 0.0 }, { true, 3.0 }, { true, 5.0 } };
    const auto aSortedNumeric = spreadsheetengine::api::query::collectSortedNumericValues(
        aNumericItems.begin(), aNumericItems.end(),
        [](const NumericItem& rItem) { return rItem.mbNumeric; },
        [](const NumericItem& rItem) { return rItem.mfValue; });
    if (aSortedNumeric.size() != 3 || aSortedNumeric[0] != 3.0 || aSortedNumeric[1] != 5.0
        || aSortedNumeric[2] != 7.0
        || !spreadsheetengine::api::query::containsSortedNumericValue(aSortedNumeric, 5.0)
        || spreadsheetengine::api::query::containsSortedNumericValue(aSortedNumeric, 4.0)
        || !spreadsheetengine::api::query::containsLinearNumericValue(
               aNumericItems.begin(), aNumericItems.end(), 7.0,
               [](const NumericItem& rItem) { return rItem.mbNumeric; },
               [](const NumericItem& rItem) { return rItem.mfValue; }))
    {
        return fail("spreadsheetengine_query_tests", "numeric query cache helper mismatch");
    }

    struct StringItem
    {
        bool mbString;
        StringIdentity mpIdentity;
    };
    static const char aFoo[] = "foo";
    static const char aBar[] = "bar";
    static const char aBaz[] = "baz";
    const std::vector<StringItem> aStringItems
        = { { true, static_cast<StringIdentity>(aFoo) },
            { false, nullptr },
            { true, static_cast<StringIdentity>(aBar) },
            { true, static_cast<StringIdentity>(aBaz) } };
    const auto aSortedStrings = spreadsheetengine::api::query::collectSortedStringIdentities(
        aStringItems.begin(), aStringItems.end(),
        [](const StringItem& rItem) { return rItem.mbString; },
        [](const StringItem& rItem) { return rItem.mpIdentity; });
    if (aSortedStrings.size() != 3
        || !spreadsheetengine::api::query::containsSortedStringIdentity(
               aSortedStrings, static_cast<StringIdentity>(aBar))
        || spreadsheetengine::api::query::containsSortedStringIdentity(
               aSortedStrings, static_cast<StringIdentity>("qux"))
        || !spreadsheetengine::api::query::containsLinearStringIdentity(
               aStringItems.begin(), aStringItems.end(), static_cast<StringIdentity>(aFoo),
               [](const StringItem& rItem) { return rItem.mbString; },
               [](const StringItem& rItem) { return rItem.mpIdentity; }))
    {
        return fail("spreadsheetengine_query_tests", "string query cache helper mismatch");
    }

    std::cout << "spreadsheetengine query api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
