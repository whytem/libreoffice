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

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Lookup.hxx>

namespace spreadsheetengine::api::lookupcache
{

enum class Result : sal_uInt8
{
    NotCached,
    CriteriaDifferent,
    NotAvailable,
    Found
};

enum class QueryOp : sal_uInt8
{
    Unknown,
    Equal,
    LessEqual,
    GreaterEqual
};

using SearchMode = spreadsheetengine::api::lookup::SearchMode;

struct QueryCriteria
{
    double mfValue = 0.0;
    String maString;
    QueryOp meOp = QueryOp::Unknown;
    SearchMode meSearchMode = SearchMode::Forward;
    bool mbString = false;

    [[nodiscard]] constexpr bool operator==(const QueryCriteria& rOther) const = default;

    [[nodiscard]] constexpr bool isEmptyStringQuery() const
    {
        return meOp == QueryOp::Equal && mbString && maString.empty();
    }

    [[nodiscard]] static constexpr QueryCriteria fromDouble(
        QueryOp eOp, SearchMode eSearchMode, double fValue)
    {
        QueryCriteria aCriteria;
        aCriteria.mfValue = fValue;
        aCriteria.meOp = eOp;
        aCriteria.meSearchMode = eSearchMode;
        return aCriteria;
    }

    [[nodiscard]] static QueryCriteria fromString(
        QueryOp eOp, SearchMode eSearchMode, StringView rValue)
    {
        QueryCriteria aCriteria;
        aCriteria.maString = String(rValue);
        aCriteria.meOp = eOp;
        aCriteria.meSearchMode = eSearchMode;
        aCriteria.mbString = true;
        return aCriteria;
    }
};

struct QueryKey
{
    RowIndex mnRow = 0;
    SheetId mnSheet = 0;
    QueryOp meOp = QueryOp::Unknown;
    SearchMode meSearchMode = SearchMode::Forward;

    [[nodiscard]] constexpr bool operator==(const QueryKey& rOther) const = default;

    struct Hash
    {
        [[nodiscard]] constexpr std::size_t operator()(const QueryKey& rKey) const
        {
            const auto nSearchMode
                = static_cast<std::size_t>(static_cast<sal_Int32>(rKey.meSearchMode) & 0xFF);
            return (static_cast<std::size_t>(rKey.mnSheet) << 24)
                   ^ (static_cast<std::size_t>(rKey.meOp) << 22) ^ (nSearchMode << 20)
                   ^ static_cast<std::size_t>(rKey.mnRow);
        }
    };
};

struct CacheEntry
{
    QueryCriteria maCriteria;
    CellAddress maAddress;
    bool mbAvailable = true;

    [[nodiscard]] constexpr bool operator==(const CacheEntry& rOther) const = default;
};

[[nodiscard]] constexpr QueryKey makeQueryKey(
    const CellAddress& rAddress, QueryOp eOp, SearchMode eSearchMode)
{
    return { rAddress.mnRow, rAddress.mnSheet, eOp, eSearchMode };
}

[[nodiscard]] constexpr CacheEntry makeCacheEntry(
    const QueryCriteria& rCriteria, const CellAddress& rAddress, bool bAvailable)
{
    return { rCriteria, rAddress, bAvailable };
}

[[nodiscard]] inline Result classifyLookup(CellAddress& o_rResultAddress,
    const QueryCriteria& rCriteria, const CacheEntry* pEntry)
{
    if (!pEntry)
        return Result::NotCached;
    if (!(pEntry->maCriteria == rCriteria))
        return Result::CriteriaDifferent;
    if (!pEntry->mbAvailable)
        return Result::NotAvailable;

    o_rResultAddress = pEntry->maAddress;
    return Result::Found;
}

[[nodiscard]] inline const CacheEntry& cacheEntryFromMappedValue(const CacheEntry& rValue)
{
    return rValue;
}

template <typename T>
[[nodiscard]] inline auto cacheEntryFromMappedValue(const T& rValue) -> decltype((rValue.maEntry))
{
    return rValue.maEntry;
}

template <typename Iterator>
[[nodiscard]] inline RowIndex findCachedRowForCriteria(
    Iterator itBegin, Iterator itEnd, const QueryCriteria& rCriteria)
{
    const auto it = std::find_if(itBegin, itEnd, [&rCriteria](const auto& rEntry) {
        return cacheEntryFromMappedValue(rEntry.second).maCriteria == rCriteria;
    });

    return it == itEnd ? -1 : it->first.mnRow;
}

} // namespace spreadsheetengine::api::lookupcache

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
