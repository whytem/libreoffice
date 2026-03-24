/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace spreadsheetengine::core::execution
{

struct ContextRebindPlan
{
    bool mbDocChanged = false;
    bool mbFormatterChanged = false;
    bool mbResetLookupCache = false;
    bool mbResetRecentCaches = false;
};

struct ThreadedPoolSlotPlan
{
    std::size_t mnSlotIndex = 0;
    bool mbCreateNew = false;
    bool mbValid = false;
};

struct NonThreadedPoolAcquirePlan
{
    std::size_t mnSlotIndex = 0;
    std::size_t mnNextFreeAfterAcquire = 0;
    bool mbCreateNew = false;
    bool mbValid = false;
};

struct NonThreadedPoolReleasePlan
{
    std::size_t mnReleasedIndex = 0;
    std::size_t mnNextFreeAfterRelease = 0;
    bool mbValid = false;
};

template <typename DocPtr, typename FormatterPtr>
[[nodiscard]] constexpr ContextRebindPlan planContextRebind(DocPtr pCurrentDoc, DocPtr pNextDoc,
                                                            FormatterPtr pCurrentFormatter,
                                                            FormatterPtr pNextFormatter)
{
    const bool bDocChanged = pCurrentDoc != pNextDoc;
    const bool bFormatterChanged = pCurrentFormatter != pNextFormatter;
    return { bDocChanged, bFormatterChanged, bDocChanged, bFormatterChanged };
}

template <typename High, typename Low>
[[nodiscard]] constexpr std::uint64_t composeHighLowCacheKey(High nHigh, Low nLow)
{
    return (static_cast<std::uint64_t>(nHigh) << 32) | static_cast<std::uint32_t>(nLow);
}

[[nodiscard]] constexpr ThreadedPoolSlotPlan planThreadedPoolSlot(std::size_t nOldSize,
                                                                  std::size_t nPoolSize,
                                                                  std::size_t nSlotIndex)
{
    if (nSlotIndex >= nPoolSize)
        return {};

    return { nSlotIndex, nSlotIndex >= nOldSize, true };
}

[[nodiscard]] constexpr bool isValidThreadedPoolIndex(std::size_t nPoolSize,
                                                      std::size_t nThreadIdx)
{
    return nThreadIdx < nPoolSize;
}

[[nodiscard]] constexpr NonThreadedPoolAcquirePlan planNonThreadedPoolAcquire(
    std::size_t nPoolSize, std::size_t nNextFree)
{
    if (nNextFree > nPoolSize)
        return {};

    return { nNextFree, nNextFree + 1, nNextFree == nPoolSize, true };
}

[[nodiscard]] constexpr bool hasActiveNonThreadedPoolContext(
    std::size_t nPoolSize, std::size_t nNextFree)
{
    return nNextFree > 0 && nNextFree <= nPoolSize;
}

[[nodiscard]] constexpr std::size_t activeNonThreadedPoolContextIndex(
    std::size_t nPoolSize, std::size_t nNextFree)
{
    return hasActiveNonThreadedPoolContext(nPoolSize, nNextFree) ? (nNextFree - 1) : 0;
}

[[nodiscard]] constexpr NonThreadedPoolReleasePlan planNonThreadedPoolRelease(
    std::size_t nPoolSize, std::size_t nNextFree)
{
    if (!hasActiveNonThreadedPoolContext(nPoolSize, nNextFree))
        return {};

    return { nNextFree - 1, nNextFree - 1, true };
}

template <typename TokenContainer, typename Predicate>
auto findReusableCachedToken(TokenContainer& rTokens, Predicate aPredicate)
    -> typename TokenContainer::value_type
{
    for (auto pToken : rTokens)
    {
        if (pToken && aPredicate(pToken))
            return pToken;
    }

    return nullptr;
}

template <typename TokenContainer, typename Releaser>
void resetTokenCache(TokenContainer& rTokens, std::size_t& rnTokenCachePos, Releaser aReleaser)
{
    for (auto& rpToken : rTokens)
    {
        if (rpToken)
            aReleaser(rpToken);
    }

    rnTokenCachePos = 0;
    std::fill(rTokens.begin(), rTokens.end(), nullptr);
}

template <typename TokenContainer, typename TokenPtr, typename Releaser, typename Retainer>
void replaceCachedToken(TokenContainer& rTokens, std::size_t& rnTokenCachePos, TokenPtr pToken,
                        Releaser aReleaser, Retainer aRetainer)
{
    if (rTokens[rnTokenCachePos])
        aReleaser(rTokens[rnTokenCachePos]);

    rTokens[rnTokenCachePos] = pToken;
    aRetainer(pToken);
    rnTokenCachePos = (rnTokenCachePos + 1) % rTokens.size();
}

template <typename CacheArray>
void resetRecentCache(CacheArray& rCache)
{
    using Entry = typename CacheArray::value_type;
    std::fill(rCache.begin(), rCache.end(), Entry());
}

template <typename CacheArray, typename Predicate>
auto findRecentCacheEntry(CacheArray& rCache, Predicate aPredicate) -> decltype(rCache.begin())
{
    return std::find_if(rCache.begin(), rCache.end(), aPredicate);
}

template <typename CacheArray>
void pushRecentCacheEntry(CacheArray& rCache, const typename CacheArray::value_type& rEntry)
{
    if (rCache.empty())
        return;

    std::move_backward(rCache.begin(), std::next(rCache.begin(), rCache.size() - 1), rCache.end());
    rCache[0] = rEntry;
}

template <typename CacheArray, typename Predicate, typename EntryBuilder>
auto getOrInsertRecentCacheEntry(CacheArray& rCache, Predicate aPredicate, EntryBuilder aEntryBuilder)
    -> typename CacheArray::value_type&
{
    auto aFound = findRecentCacheEntry(rCache, aPredicate);
    if (aFound != rCache.end())
        return *aFound;

    pushRecentCacheEntry(rCache, aEntryBuilder());
    return rCache[0];
}

template <typename ConditionContainer, typename DelayedContainer, typename TokenContainer,
          typename Releaser>
void cleanupScratchState(ConditionContainer& rConditions, DelayedContainer& rDelayedState,
                         TokenContainer& rTokens, std::size_t& rnTokenCachePos,
                         Releaser aReleaser)
{
    rConditions.clear();
    rDelayedState.clear();
    resetTokenCache(rTokens, rnTokenCachePos, aReleaser);
}

template <typename LookupCachePtr, typename LanguageDataPtr, typename AuxFormatKeyMapPtr,
          typename FormatterPtr, typename FormatDataPtr, typename NatNumPtr>
bool clearDocBoundStateIfMatches(const void* pRequestedDoc, const void* pCurrentDoc,
                                 LookupCachePtr& rxLookupCache, LanguageDataPtr& rxLanguageData,
                                 AuxFormatKeyMapPtr& rxAuxFormatKeyMap, FormatterPtr& rpFormatter,
                                 FormatDataPtr& rpFormatData, NatNumPtr& rpNatNum)
{
    if (pRequestedDoc != pCurrentDoc)
        return false;

    rxLookupCache.reset();
    rxLanguageData.reset();
    rxAuxFormatKeyMap.reset();
    rpFormatter = nullptr;
    rpFormatData = nullptr;
    rpNatNum = nullptr;
    return true;
}

template <typename PoolContainer, typename Visitor>
void forEachLivePoolContext(PoolContainer& rPool, Visitor aVisitor)
{
    for (auto& rpContext : rPool)
    {
        if (rpContext)
            aVisitor(*rpContext);
    }
}

} // namespace spreadsheetengine::core::execution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
