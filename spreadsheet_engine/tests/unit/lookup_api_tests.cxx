/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <unordered_map>
#include <vector>

#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/LookupCache.hxx>

#include "SharedCaseSupport.hxx"
#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::MatrixCoordinate;
    using spreadsheetengine::api::MatrixDimensions;
    using spreadsheetengine::api::lookup::ComparisonOp;
    using spreadsheetengine::api::lookup::MatchMode;
    using spreadsheetengine::api::lookup::Operation;
    using spreadsheetengine::api::lookup::PatternMode;
    using spreadsheetengine::api::lookup::SearchMode;
    using spreadsheetengine::api::lookup::VectorOrientation;
    using spreadsheetengine::api::MatrixSize;
    using spreadsheetengine::api::lookupcache::CacheEntry;
    using spreadsheetengine::api::lookupcache::QueryCriteria;
    using spreadsheetengine::api::lookupcache::QueryKey;
    using spreadsheetengine::api::lookupcache::QueryOp;
    using spreadsheetengine::api::lookupcache::Result;
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::loadSharedCaseRows;
    using spreadsheetengine::standalone::test::parseDouble;
    using spreadsheetengine::standalone::test::parseExpectedError;
    using spreadsheetengine::standalone::test::fail;

    const std::vector<double> aLookupKeys { 10.0, 20.0, 30.0 };
    const std::vector<double> aLookupValues { 100.0, 200.0, 300.0 };
    const MatrixDimensions aVerticalTableDimensions { 2, 3 };
    const MatrixDimensions aHorizontalTableDimensions { 3, 2 };

    auto findExactHitOneBased = [&aLookupKeys](double fLookup) -> MatrixSize {
        for (std::size_t i = 0; i < aLookupKeys.size(); ++i)
        {
            if (almostEqual(aLookupKeys[i], fLookup))
                return static_cast<MatrixSize>(i + 1);
        }
        return 0;
    };

    auto findBestFitAscendingZeroBased = [&aLookupKeys](double fLookup) -> std::optional<MatrixSize> {
        std::optional<MatrixSize> oBestFit;
        for (std::size_t i = 0; i < aLookupKeys.size(); ++i)
        {
            if (aLookupKeys[i] <= fLookup)
                oBestFit = static_cast<MatrixSize>(i);
        }
        return oBestFit;
    };

    const auto aMatchDefault = spreadsheetengine::api::lookup::normalizeMatchType(1.0);
    const auto aMatchLower = spreadsheetengine::api::lookup::normalizeMatchType(-1.0);
    const auto aMatchExact = spreadsheetengine::api::lookup::normalizeMatchType(0.9);
    const auto aMatchBad = spreadsheetengine::api::lookup::normalizeMatchType(2.0);
    if (!aMatchDefault || aMatchDefault.maValue.meMatchMode != MatchMode::ExactOrNextSmaller
        || aMatchDefault.maValue.meSearchMode != SearchMode::BinaryAscending || !aMatchLower
        || aMatchLower.maValue.meMatchMode != MatchMode::ExactOrNextLarger
        || aMatchLower.maValue.meSearchMode != SearchMode::BinaryDescending || !aMatchExact
        || aMatchExact.maValue.meMatchMode != MatchMode::ExactOrNotAvailable || aMatchBad
        || aMatchBad.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_lookup_tests", "MATCH mode normalization mismatch");
    }

    const auto aSearchMode = spreadsheetengine::api::lookup::normalizeSearchMode(-2);
    const auto aSearchModeBad = spreadsheetengine::api::lookup::normalizeSearchMode(0);
    const auto aExtendedMatch = spreadsheetengine::api::lookup::normalizeExtendedMatchMode(3);
    const auto aExtendedMatchBad
        = spreadsheetengine::api::lookup::normalizeExtendedMatchMode(4);
    if (!aSearchMode || aSearchMode.maValue != SearchMode::BinaryDescending || aSearchModeBad
        || aSearchModeBad.meError != Error::IllegalArgument || !aExtendedMatch
        || aExtendedMatch.maValue != MatchMode::Regex || aExtendedMatchBad
        || aExtendedMatchBad.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_lookup_tests", "extended mode normalization mismatch");
    }

    const auto aColumnVector
        = spreadsheetengine::api::lookup::detectVectorLayout(MatrixDimensions { 1, 4 });
    const auto aRowVector
        = spreadsheetengine::api::lookup::detectVectorLayout(MatrixDimensions { 5, 1 });
    const auto aNotVector
        = spreadsheetengine::api::lookup::detectVectorLayout(MatrixDimensions { 2, 2 });
    if (!aColumnVector || aColumnVector.maValue.meOrientation != VectorOrientation::Column
        || aColumnVector.maValue.mnLength != 4 || !aRowVector
        || aRowVector.maValue.meOrientation != VectorOrientation::Row
        || aRowVector.maValue.mnLength != 5 || aNotVector
        || aNotVector.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_lookup_tests", "vector layout mismatch");
    }

    if (spreadsheetengine::api::lookup::majorVectorLayout(MatrixDimensions { 2, 5 }).meOrientation
            != VectorOrientation::Column
        || spreadsheetengine::api::lookup::majorVectorLayout(MatrixDimensions { 5, 2 })
               .meOrientation
               != VectorOrientation::Row)
    {
        return fail("spreadsheetengine_lookup_tests", "major vector layout mismatch");
    }

    const auto aXLookupShapeOk = spreadsheetengine::api::lookup::validateXLookupResultShape(
        MatrixDimensions { 1, 4 }, MatrixDimensions { 3, 4 });
    const auto aXLookupShapeBad = spreadsheetengine::api::lookup::validateXLookupResultShape(
        MatrixDimensions { 4, 1 }, MatrixDimensions { 3, 4 });
    if (!aXLookupShapeOk || aXLookupShapeOk.maValue != VectorOrientation::Column
        || aXLookupShapeBad || aXLookupShapeBad.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_lookup_tests", "XLOOKUP shape validation mismatch");
    }

    const auto aMatchPolicy = spreadsheetengine::api::lookup::buildSearchPolicy(
        Operation::Match, MatchMode::ExactOrNotAvailable, SearchMode::Forward, true, false,
        false, false);
    const auto aVBAPolicy = spreadsheetengine::api::lookup::buildSearchPolicy(
        Operation::Match, MatchMode::ExactOrNextSmaller, SearchMode::Forward, true, true, false,
        false);
    const auto aWildcardPolicy = spreadsheetengine::api::lookup::buildSearchPolicy(
        Operation::XLookup, MatchMode::Wildcard, SearchMode::Forward, true, false, true, false);
    const auto aWildcardBinary = spreadsheetengine::api::lookup::buildSearchPolicy(
        Operation::XLookup, MatchMode::Wildcard, SearchMode::BinaryAscending, true, false, true,
        false);
    if (!aMatchPolicy || aMatchPolicy.maValue.meComparison != ComparisonOp::Equal
        || aMatchPolicy.maValue.mePattern != PatternMode::Detect || !aVBAPolicy
        || aVBAPolicy.maValue.meComparison != ComparisonOp::LessEqual
        || aVBAPolicy.maValue.mePattern != PatternMode::Wildcard
        || aVBAPolicy.maValue.mbAllowMatchEmpty || !aWildcardPolicy
        || aWildcardPolicy.maValue.mePattern != PatternMode::Wildcard
        || aWildcardPolicy.maValue.meComparison != ComparisonOp::Equal
        || aWildcardBinary || aWildcardBinary.meError != Error::NoValue)
    {
        return fail("spreadsheetengine_lookup_tests", "search policy mismatch");
    }

    const auto aXLookupIndex
        = spreadsheetengine::api::lookup::resolveSearchResultIndex(Operation::XLookup, 3, {});
    const auto aMatchBestFit = spreadsheetengine::api::lookup::resolveSearchResultIndex(
        Operation::Match, 0, spreadsheetengine::api::MatrixSize(6));
    const auto aNoHit = spreadsheetengine::api::lookup::resolveSearchResultIndex(
        Operation::XMatch, 0, std::nullopt);
    if (!aXLookupIndex || aXLookupIndex.maValue != 2 || !aMatchBestFit
        || aMatchBestFit.maValue != 7 || aNoHit || aNoHit.meError != Error::NotAvailable)
    {
        return fail("spreadsheetengine_lookup_tests", "search result index mismatch");
    }

    const auto aVectorElement = spreadsheetengine::api::lookup::planVectorElement(
        VectorOrientation::Column, 2, MatrixDimensions { 1, 4 });
    const auto aVectorElementBad = spreadsheetengine::api::lookup::planVectorElement(
        VectorOrientation::Row, 2, MatrixDimensions { 2, 1 });
    if (!aVectorElement || aVectorElement.maValue.mnColumn != 0
        || aVectorElement.maValue.mnRow != 2 || aVectorElementBad
        || aVectorElementBad.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_lookup_tests", "vector element planning mismatch");
    }

    const auto aTabularLookup = spreadsheetengine::api::lookup::planTabularLookupResult(
        VectorOrientation::Column, 3, 1, MatrixDimensions { 4, 5 });
    const auto aTabularHLookup = spreadsheetengine::api::lookup::planTabularLookupResult(
        VectorOrientation::Row, 3, 1, MatrixDimensions { 5, 4 });
    if (!aTabularLookup || aTabularLookup.maValue.mnColumn != 1
        || aTabularLookup.maValue.mnRow != 3 || !aTabularHLookup
        || aTabularHLookup.maValue.mnColumn != 3 || aTabularHLookup.maValue.mnRow != 1)
    {
        return fail("spreadsheetengine_lookup_tests", "tabular lookup planning mismatch");
    }

    const auto aXLookupSlice = spreadsheetengine::api::lookup::planXLookupResultSlice(
        VectorOrientation::Column, 2, MatrixDimensions { 3, 4 });
    const auto aXLookupSliceRow = spreadsheetengine::api::lookup::planXLookupResultSlice(
        VectorOrientation::Row, 1, MatrixDimensions { 4, 2 });
    if (!aXLookupSlice || aXLookupSlice.maValue.maStart.mnColumn != 0
        || aXLookupSlice.maValue.maStart.mnRow != 2
        || aXLookupSlice.maValue.maDimensions.mnColumns != 3
        || aXLookupSlice.maValue.maDimensions.mnRows != 1 || !aXLookupSliceRow
        || aXLookupSliceRow.maValue.maStart.mnColumn != 1
        || aXLookupSliceRow.maValue.maStart.mnRow != 0
        || aXLookupSliceRow.maValue.maDimensions.mnColumns != 1
        || aXLookupSliceRow.maValue.maDimensions.mnRows != 2)
    {
        return fail("spreadsheetengine_lookup_tests", "XLOOKUP slice planning mismatch");
    }

    const auto aNumericCriteria
        = QueryCriteria::fromDouble(QueryOp::Equal, SearchMode::Forward, 42.0);
    const auto aStringCriteria
        = QueryCriteria::fromString(QueryOp::Equal, SearchMode::BinaryAscending, u"needle");
    const auto aEmptyCriteria
        = QueryCriteria::fromString(QueryOp::Equal, SearchMode::Forward, u"");
    if (!(aNumericCriteria == QueryCriteria::fromDouble(QueryOp::Equal, SearchMode::Forward, 42.0))
        || aNumericCriteria == aStringCriteria || !aEmptyCriteria.isEmptyStringQuery()
        || aStringCriteria.isEmptyStringQuery())
    {
        return fail("spreadsheetengine_lookup_tests", "lookup cache criteria mismatch");
    }

    const auto aKey
        = spreadsheetengine::api::lookupcache::makeQueryKey({ 3, 2, 9 }, QueryOp::LessEqual,
            SearchMode::BinaryDescending);
    std::unordered_map<QueryKey, CacheEntry, QueryKey::Hash> aCache;
    aCache.emplace(aKey, spreadsheetengine::api::lookupcache::makeCacheEntry(
                             aNumericCriteria, { 3, 7, 11 }, true));
    if (aCache.find(aKey) == aCache.end())
    {
        return fail("spreadsheetengine_lookup_tests", "lookup cache key hashing mismatch");
    }

    const auto nCachedRow
        = spreadsheetengine::api::lookupcache::findCachedRowForCriteria(
            aCache.begin(), aCache.end(), aNumericCriteria);
    const auto nMissingRow
        = spreadsheetengine::api::lookupcache::findCachedRowForCriteria(
            aCache.begin(), aCache.end(), aStringCriteria);
    if (nCachedRow != 9 || nMissingRow != -1)
    {
        return fail("spreadsheetengine_lookup_tests", "lookup cache row search mismatch");
    }

    spreadsheetengine::api::CellAddress aFoundAddress;
    const auto eFound
        = spreadsheetengine::api::lookupcache::classifyLookup(aFoundAddress, aNumericCriteria,
            &aCache.find(aKey)->second);
    const auto eDifferent
        = spreadsheetengine::api::lookupcache::classifyLookup(aFoundAddress, aStringCriteria,
            &aCache.find(aKey)->second);
    const auto aMissingEntry = spreadsheetengine::api::lookupcache::makeCacheEntry(
        aStringCriteria, { 3, 0, 0 }, false);
    const auto eMissing = spreadsheetengine::api::lookupcache::classifyLookup(
        aFoundAddress, aStringCriteria, &aMissingEntry);
    const auto eNotCached = spreadsheetengine::api::lookupcache::classifyLookup(
        aFoundAddress, aStringCriteria, nullptr);
    if (eFound != Result::Found || aFoundAddress.mnSheet != 3 || aFoundAddress.mnColumn != 7
        || aFoundAddress.mnRow != 11 || eDifferent != Result::CriteriaDifferent
        || eMissing != Result::NotAvailable || eNotCached != Result::NotCached)
    {
        return fail("spreadsheetengine_lookup_tests", "lookup cache result mismatch");
    }

    for (const auto& rRow : loadSharedCaseRows("lookup_cases.tsv"))
    {
        if (rRow.maColumns.size() < 6)
            return failSharedCase(
                "spreadsheetengine_lookup_tests", rRow, "lookup shared case column mismatch");

        const auto& rFunction = rRow.maColumns[0];
        const double fLookup = parseDouble(rRow.maColumns[1]);
        const Error eExpectedError = parseExpectedError(rRow.maColumns[5]);
        const double fExpected = parseDouble(rRow.maColumns[4]);

        if (rFunction == "MATCH")
        {
            const auto aModes = spreadsheetengine::api::lookup::normalizeMatchType(
                parseDouble(rRow.maColumns[2]));
            if (!aModes)
                return failSharedCase("spreadsheetengine_lookup_tests", rRow, "MATCH mode mismatch");

            const MatrixSize nHitIndex
                = aModes.maValue.meMatchMode == MatchMode::ExactOrNotAvailable
                      ? findExactHitOneBased(fLookup)
                      : 0;
            const auto aResult = spreadsheetengine::api::lookup::resolveSearchResultIndex(
                Operation::Match, nHitIndex, findBestFitAscendingZeroBased(fLookup));
            if (eExpectedError != Error::None)
            {
                if (aResult || aResult.meError != eExpectedError)
                    return failSharedCase(
                        "spreadsheetengine_lookup_tests", rRow, "MATCH error mismatch");
            }
            else if (!aResult || !almostEqual(static_cast<double>(aResult.maValue), fExpected))
            {
                return failSharedCase(
                    "spreadsheetengine_lookup_tests", rRow, "MATCH value mismatch");
            }
        }
        else if (rFunction == "XMATCH")
        {
            const auto aMatchMode = spreadsheetengine::api::lookup::normalizeExtendedMatchMode(
                static_cast<sal_Int16>(parseDouble(rRow.maColumns[2])));
            const auto aSearchMode = spreadsheetengine::api::lookup::normalizeSearchMode(
                static_cast<sal_Int16>(parseDouble(rRow.maColumns[3])));
            if (!aMatchMode || !aSearchMode)
            {
                return failSharedCase(
                    "spreadsheetengine_lookup_tests", rRow, "XMATCH mode mismatch");
            }

            const auto aResult = spreadsheetengine::api::lookup::resolveSearchResultIndex(
                Operation::XMatch, findExactHitOneBased(fLookup), std::nullopt);
            if (!aResult || !almostEqual(static_cast<double>(aResult.maValue), fExpected))
                return failSharedCase("spreadsheetengine_lookup_tests", rRow, "XMATCH mismatch");
        }
        else if (rFunction == "LOOKUP")
        {
            const auto aIndex = spreadsheetengine::api::lookup::resolveSearchResultIndex(
                Operation::Lookup, 0, findBestFitAscendingZeroBased(fLookup));
            if (!aIndex)
                return failSharedCase("spreadsheetengine_lookup_tests", rRow, "LOOKUP index mismatch");

            const auto aCoordinate = spreadsheetengine::api::lookup::planVectorElement(
                VectorOrientation::Column, aIndex.maValue - 1, MatrixDimensions { 1, 3 });
            if (!aCoordinate
                || !almostEqual(aLookupValues[aCoordinate.maValue.mnRow], fExpected))
            {
                return failSharedCase(
                    "spreadsheetengine_lookup_tests", rRow, "LOOKUP value mismatch");
            }
        }
        else if (rFunction == "VLOOKUP")
        {
            const auto aCoordinate = spreadsheetengine::api::lookup::planTabularLookupResult(
                VectorOrientation::Column, findExactHitOneBased(fLookup) - 1,
                static_cast<MatrixSize>(parseDouble(rRow.maColumns[2]) - 1),
                aVerticalTableDimensions);
            if (!aCoordinate
                || !almostEqual(aLookupValues[aCoordinate.maValue.mnRow], fExpected))
            {
                return failSharedCase(
                    "spreadsheetengine_lookup_tests", rRow, "VLOOKUP value mismatch");
            }
        }
        else if (rFunction == "HLOOKUP")
        {
            const auto aCoordinate = spreadsheetengine::api::lookup::planTabularLookupResult(
                VectorOrientation::Row, findExactHitOneBased(fLookup) - 1,
                static_cast<MatrixSize>(parseDouble(rRow.maColumns[2]) - 1),
                aHorizontalTableDimensions);
            if (!aCoordinate
                || !almostEqual(aLookupValues[aCoordinate.maValue.mnColumn], fExpected))
            {
                return failSharedCase(
                    "spreadsheetengine_lookup_tests", rRow, "HLOOKUP value mismatch");
            }
        }
        else if (rFunction == "XLOOKUP" || rFunction == "XLOOKUP_ROW")
        {
            const MatrixSize nHitIndex = findExactHitOneBased(fLookup) - 1;
            const bool bVertical = rFunction == "XLOOKUP";
            const auto aShape = spreadsheetengine::api::lookup::validateXLookupResultShape(
                bVertical ? MatrixDimensions { 1, 3 } : MatrixDimensions { 3, 1 },
                bVertical ? MatrixDimensions { 1, 3 } : MatrixDimensions { 3, 1 });
            if (!aShape)
            {
                return failSharedCase(
                    "spreadsheetengine_lookup_tests", rRow, "XLOOKUP shape mismatch");
            }

            const auto aSlice = spreadsheetengine::api::lookup::planXLookupResultSlice(
                aShape.maValue, nHitIndex,
                bVertical ? MatrixDimensions { 1, 3 } : MatrixDimensions { 3, 1 });
            if (!aSlice)
                return failSharedCase(
                    "spreadsheetengine_lookup_tests", rRow, "XLOOKUP slice mismatch");

            const MatrixSize nValueIndex
                = bVertical ? aSlice.maValue.maStart.mnRow : aSlice.maValue.maStart.mnColumn;
            if (!almostEqual(aLookupValues[nValueIndex], fExpected))
                return failSharedCase(
                    "spreadsheetengine_lookup_tests", rRow, "XLOOKUP value mismatch");
        }
        else
        {
            return failSharedCase(
                "spreadsheetengine_lookup_tests", rRow, "unknown lookup shared-case function");
        }
    }

    std::cout << "spreadsheetengine lookup api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
