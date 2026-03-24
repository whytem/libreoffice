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

namespace spreadsheetengine::core::execution
{

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

template <typename CacheArray>
void resetRecentCache(CacheArray& rCache)
{
    using Entry = typename CacheArray::value_type;
    std::fill(rCache.begin(), rCache.end(), Entry());
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

} // namespace spreadsheetengine::core::execution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
