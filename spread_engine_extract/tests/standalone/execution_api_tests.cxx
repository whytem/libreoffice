/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <array>
#include <memory>
#include <vector>

#include <spreadsheetengine/core/ExecutionContext.hxx>

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

} // namespace

int main()
{
    using spreadsheetengine::standalone::test::fail;

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

    std::array<CacheEntry, 4> aCache { CacheEntry { 3 }, CacheEntry { 5 }, CacheEntry { 8 },
                                       CacheEntry { 13 } };
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
