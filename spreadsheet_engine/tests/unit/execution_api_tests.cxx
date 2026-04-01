/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <array>
#include <memory>
#include <vector>

#include <spreadsheetengine/detail/ExecutionContext.hxx>
#include <spreadsheetengine/runtime/ScalarCoercion.hxx>

#include "TestSupport.hxx"

namespace
{

struct ReleaseToken
{
    bool mbReleased = false;
};

struct CacheEntry
{
    int mnValue = -1;

    bool operator==(const CacheEntry& rOther) const { return mnValue == rOther.mnValue; }
};

struct CachedToken
{
    int mnRefCount = 0;
    bool mbReleased = false;
    bool mbRetained = false;
};

struct PoolEntry
{
    int mnValue = 0;
};

} // namespace

int main()
{
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::api::Error;
    using spreadsheetengine::core::coercion::coerceToBoolean;
    using spreadsheetengine::core::coercion::coerceToNumber;
    using spreadsheetengine::core::coercion::coerceToString;
    using spreadsheetengine::core::coercion::normalizeNonNegativeLengthArgument;
    using spreadsheetengine::core::coercion::normalizeOneBasedStringPositionArgument;
    using spreadsheetengine::core::coercion::normalizeStringPositionArgument;
    using spreadsheetengine::standalone::test::fail;

    const auto aNumericText = coerceToNumber(CellValue::text(u"12.5"));
    const auto aInvalidNumericText = coerceToNumber(CellValue::text(u"abc"));
    const auto aBooleanTrue = coerceToBoolean(CellValue::text(u"TRUE"));
    const auto aBooleanNumeric = coerceToBoolean(CellValue::number(2.0));
    const auto aStringNumber = coerceToString(CellValue::number(12.5));
    const auto aStringBoolean = coerceToString(CellValue::boolean(true));
    const auto aPosition = normalizeStringPositionArgument(2.9);
    const auto aZeroLength = normalizeNonNegativeLengthArgument(0.9);
    const auto aOneBasedPosition = normalizeOneBasedStringPositionArgument(1.9);
    const auto aInvalidPosition = normalizeStringPositionArgument(-1.0);
    const auto aInvalidOneBased = normalizeOneBasedStringPositionArgument(0.0);

    if (!aNumericText || aNumericText.maValue != 12.5 || aInvalidNumericText
        || aInvalidNumericText.meError != Error::IllegalArgument || !aBooleanTrue
        || !aBooleanTrue.maValue || !aBooleanNumeric || !aBooleanNumeric.maValue || !aStringNumber
        || aStringNumber.maValue != u"12.5" || !aStringBoolean
        || aStringBoolean.maValue != u"TRUE" || !aPosition || aPosition.maValue != 2
        || !aZeroLength || aZeroLength.maValue != 0 || !aOneBasedPosition
        || aOneBasedPosition.maValue != 1 || aInvalidPosition
        || aInvalidPosition.meError != Error::IllegalArgument || aInvalidOneBased
        || aInvalidOneBased.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_execution_tests", "scalar coercion helper mismatch");
    }

    std::size_t nReleasedCount = 0;
    std::vector<ReleaseToken*> aTokens(4, nullptr);
    ReleaseToken aToken1;
    ReleaseToken aToken2;
    aTokens[1] = &aToken1;
    aTokens[3] = &aToken2;
    std::size_t nTokenCachePos = 3;

    spreadsheetengine::core::execution::resetTokenCache(
        aTokens, nTokenCachePos, [&](ReleaseToken* pToken) {
            pToken->mbReleased = true;
            ++nReleasedCount;
        });

    if (!aToken1.mbReleased || !aToken2.mbReleased || nReleasedCount != 2 || nTokenCachePos != 0
        || aTokens[1] != nullptr || aTokens[3] != nullptr)
    {
        return fail("spreadsheetengine_execution_tests", "token cache reset mismatch");
    }

    std::vector<CachedToken*> aCachedTokens(3, nullptr);
    CachedToken aCachedToken1 { 2, false, false };
    CachedToken aCachedToken2 { 1, false, false };
    aCachedTokens[0] = &aCachedToken1;
    aCachedTokens[1] = &aCachedToken2;
    if (spreadsheetengine::core::execution::findReusableCachedToken(
            aCachedTokens, [](CachedToken* pToken) { return pToken->mnRefCount == 1; })
        != &aCachedToken2)
    {
        return fail("spreadsheetengine_execution_tests", "reusable token lookup mismatch");
    }

    std::size_t nReplacementPos = 1;
    CachedToken aReplacement { 1, false, false };
    spreadsheetengine::core::execution::replaceCachedToken(
        aCachedTokens, nReplacementPos, &aReplacement,
        [](CachedToken* pToken) { pToken->mbReleased = true; },
        [](CachedToken* pToken) { pToken->mbRetained = true; });
    if (!aCachedToken2.mbReleased || !aReplacement.mbRetained || aCachedTokens[1] != &aReplacement
        || nReplacementPos != 2)
    {
        return fail("spreadsheetengine_execution_tests", "token cache replacement mismatch");
    }

    const int nDoc1 = 1;
    const int nDoc2 = 2;
    const int nFormatter1 = 11;
    const int nFormatter2 = 12;
    const auto aNoRebindPlan = spreadsheetengine::core::execution::planContextRebind(
        &nDoc1, &nDoc1, &nFormatter1, &nFormatter1);
    const auto aDocRebindPlan = spreadsheetengine::core::execution::planContextRebind(
        &nDoc1, &nDoc2, &nFormatter1, &nFormatter1);
    const auto aFormatterRebindPlan = spreadsheetengine::core::execution::planContextRebind(
        &nDoc1, &nDoc1, &nFormatter1, &nFormatter2);
    if (aNoRebindPlan.mbDocChanged || aNoRebindPlan.mbFormatterChanged
        || aNoRebindPlan.mbResetLookupCache || aNoRebindPlan.mbResetRecentCaches
        || !aDocRebindPlan.mbDocChanged || !aDocRebindPlan.mbResetLookupCache
        || aDocRebindPlan.mbFormatterChanged || aDocRebindPlan.mbResetRecentCaches
        || !aFormatterRebindPlan.mbFormatterChanged
        || !aFormatterRebindPlan.mbResetRecentCaches || aFormatterRebindPlan.mbDocChanged
        || aFormatterRebindPlan.mbResetLookupCache)
    {
        return fail("spreadsheetengine_execution_tests", "context rebind plan mismatch");
    }

    if (spreadsheetengine::core::execution::composeHighLowCacheKey(0x01234567u, 0x89ABu)
        != 0x01234567000089ABULL)
    {
        return fail("spreadsheetengine_execution_tests", "cache key composition mismatch");
    }

    const auto aThreadedReusePlan
        = spreadsheetengine::core::execution::planThreadedPoolSlot(2, 4, 1);
    const auto aThreadedCreatePlan
        = spreadsheetengine::core::execution::planThreadedPoolSlot(2, 4, 3);
    if (!aThreadedReusePlan.mbValid || aThreadedReusePlan.mbCreateNew
        || aThreadedReusePlan.mnSlotIndex != 1 || !aThreadedCreatePlan.mbValid
        || !aThreadedCreatePlan.mbCreateNew || aThreadedCreatePlan.mnSlotIndex != 3
        || spreadsheetengine::core::execution::planThreadedPoolSlot(2, 4, 4).mbValid
        || !spreadsheetengine::core::execution::isValidThreadedPoolIndex(4, 3)
        || spreadsheetengine::core::execution::isValidThreadedPoolIndex(4, 4))
    {
        return fail("spreadsheetengine_execution_tests", "threaded pool plan mismatch");
    }

    const auto aAcquirePlan
        = spreadsheetengine::core::execution::planNonThreadedPoolAcquire(2, 1);
    if (!aAcquirePlan.mbValid || aAcquirePlan.mnSlotIndex != 1
        || aAcquirePlan.mnNextFreeAfterAcquire != 2 || aAcquirePlan.mbCreateNew)
    {
        return fail("spreadsheetengine_execution_tests", "pool acquire plan mismatch");
    }

    const auto aGrowAcquirePlan
        = spreadsheetengine::core::execution::planNonThreadedPoolAcquire(2, 2);
    if (!aGrowAcquirePlan.mbValid || !aGrowAcquirePlan.mbCreateNew
        || aGrowAcquirePlan.mnSlotIndex != 2 || aGrowAcquirePlan.mnNextFreeAfterAcquire != 3)
    {
        return fail("spreadsheetengine_execution_tests", "pool grow-acquire plan mismatch");
    }

    if (!spreadsheetengine::core::execution::hasActiveNonThreadedPoolContext(3, 2)
        || spreadsheetengine::core::execution::hasActiveNonThreadedPoolContext(3, 0)
        || spreadsheetengine::core::execution::activeNonThreadedPoolContextIndex(3, 2) != 1)
    {
        return fail("spreadsheetengine_execution_tests", "pool active-context mismatch");
    }

    const auto aReleasePlan
        = spreadsheetengine::core::execution::planNonThreadedPoolRelease(3, 2);
    if (!aReleasePlan.mbValid || aReleasePlan.mnReleasedIndex != 1
        || aReleasePlan.mnNextFreeAfterRelease != 1
        || spreadsheetengine::core::execution::planNonThreadedPoolRelease(3, 0).mbValid)
    {
        return fail("spreadsheetengine_execution_tests", "pool release plan mismatch");
    }

    int nVisitedSum = 0;
    std::vector<std::unique_ptr<PoolEntry>> aPool;
    aPool.emplace_back(std::make_unique<PoolEntry>(PoolEntry { 2 }));
    aPool.emplace_back(nullptr);
    aPool.emplace_back(std::make_unique<PoolEntry>(PoolEntry { 5 }));
    spreadsheetengine::core::execution::forEachLivePoolContext(
        aPool, [&nVisitedSum](PoolEntry& rEntry) { nVisitedSum += rEntry.mnValue; });
    if (nVisitedSum != 7)
        return fail("spreadsheetengine_execution_tests", "pool iteration mismatch");

    std::array<CacheEntry, 4> aCache { CacheEntry { 3 }, CacheEntry { 5 }, CacheEntry { 8 },
                                       CacheEntry { 13 } };
    const auto aFoundCacheEntry = spreadsheetengine::core::execution::findRecentCacheEntry(
        aCache, [](const CacheEntry& rEntry) { return rEntry.mnValue == 8; });
    if (aFoundCacheEntry == aCache.end() || aFoundCacheEntry->mnValue != 8)
        return fail("spreadsheetengine_execution_tests", "recent cache lookup mismatch");

    spreadsheetengine::core::execution::pushRecentCacheEntry(aCache, CacheEntry { 21 });
    if (aCache[0].mnValue != 21 || aCache[1].mnValue != 3 || aCache[2].mnValue != 5
        || aCache[3].mnValue != 8)
    {
        return fail("spreadsheetengine_execution_tests", "recent cache promotion mismatch");
    }

    int nRecentCacheBuildCount = 0;
    auto& rInsertedCacheEntry = spreadsheetengine::core::execution::getOrInsertRecentCacheEntry(
        aCache, [](const CacheEntry& rEntry) { return rEntry.mnValue == 34; },
        [&nRecentCacheBuildCount]() {
            ++nRecentCacheBuildCount;
            return CacheEntry { 34 };
        });
    if (nRecentCacheBuildCount != 1 || rInsertedCacheEntry.mnValue != 34 || aCache[0].mnValue != 34)
    {
        return fail("spreadsheetengine_execution_tests", "recent cache insert mismatch");
    }

    auto& rHitCacheEntry = spreadsheetengine::core::execution::getOrInsertRecentCacheEntry(
        aCache, [](const CacheEntry& rEntry) { return rEntry.mnValue == 34; },
        [&nRecentCacheBuildCount]() {
            ++nRecentCacheBuildCount;
            return CacheEntry { 55 };
        });
    if (nRecentCacheBuildCount != 1 || rHitCacheEntry.mnValue != 34 || &rHitCacheEntry != &aCache[0])
    {
        return fail("spreadsheetengine_execution_tests", "recent cache hit mismatch");
    }

    spreadsheetengine::core::execution::resetRecentCache(aCache);
    if (aCache[0].mnValue != -1 || aCache[3].mnValue != -1)
        return fail("spreadsheetengine_execution_tests", "recent cache reset mismatch");

    std::vector<unsigned char> aConditions { 1, 2, 3 };
    std::vector<int> aDelayedState { 7, 9 };
    ReleaseToken aToken3;
    aTokens[0] = &aToken3;
    nTokenCachePos = 2;
    spreadsheetengine::core::execution::cleanupScratchState(
        aConditions, aDelayedState, aTokens, nTokenCachePos, [&](ReleaseToken* pToken) {
            pToken->mbReleased = true;
        });

    if (!aConditions.empty() || !aDelayedState.empty() || nTokenCachePos != 0
        || aTokens[0] != nullptr || !aToken3.mbReleased)
    {
        return fail("spreadsheetengine_execution_tests", "scratch cleanup mismatch");
    }

    const int nDocA = 1;
    const int nDocB = 2;
    auto xLookupCache = std::make_unique<int>(5);
    auto xLanguageData = std::make_unique<int>(6);
    auto xAuxFormatKeyMap = std::make_unique<int>(7);
    int nFormatter = 11;
    int nFormatData = 12;
    int nNatNum = 13;
    int* pFormatter = &nFormatter;
    int* pFormatData = &nFormatData;
    int* pNatNum = &nNatNum;

    if (spreadsheetengine::core::execution::clearDocBoundStateIfMatches(
            &nDocB, &nDocA, xLookupCache, xLanguageData, xAuxFormatKeyMap, pFormatter, pFormatData,
            pNatNum))
    {
        return fail("spreadsheetengine_execution_tests", "doc-bound state should not clear");
    }

    if (!spreadsheetengine::core::execution::clearDocBoundStateIfMatches(
            &nDocA, &nDocA, xLookupCache, xLanguageData, xAuxFormatKeyMap, pFormatter, pFormatData,
            pNatNum))
    {
        return fail("spreadsheetengine_execution_tests", "doc-bound state clear mismatch");
    }

    if (xLookupCache || xLanguageData || xAuxFormatKeyMap || pFormatter || pFormatData || pNatNum)
        return fail("spreadsheetengine_execution_tests", "doc-bound state not cleared");

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
