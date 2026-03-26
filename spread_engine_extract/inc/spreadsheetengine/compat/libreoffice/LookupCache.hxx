/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <lookupcache.hxx>

#include <spreadsheetengine/api/LookupCache.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::lookup::SearchMode toApiSearchMode(LookupSearchMode eSearchMode)
{
    switch (eSearchMode)
    {
        case LookupSearchMode::Forward:
            return spreadsheetengine::api::lookup::SearchMode::Forward;
        case LookupSearchMode::Reverse:
            return spreadsheetengine::api::lookup::SearchMode::Reverse;
        case LookupSearchMode::BinaryAscending:
            return spreadsheetengine::api::lookup::SearchMode::BinaryAscending;
        case LookupSearchMode::BinaryDescending:
            return spreadsheetengine::api::lookup::SearchMode::BinaryDescending;
    }

    return spreadsheetengine::api::lookup::SearchMode::Forward;
}

inline spreadsheetengine::api::lookupcache::QueryOp toApiQueryOp(ScLookupCache::QueryOp eOp)
{
    switch (eOp)
    {
        case ScLookupCache::EQUAL:
            return spreadsheetengine::api::lookupcache::QueryOp::Equal;
        case ScLookupCache::LESS_EQUAL:
            return spreadsheetengine::api::lookupcache::QueryOp::LessEqual;
        case ScLookupCache::GREATER_EQUAL:
            return spreadsheetengine::api::lookupcache::QueryOp::GreaterEqual;
        case ScLookupCache::UNKNOWN:
            break;
    }

    return spreadsheetengine::api::lookupcache::QueryOp::Unknown;
}

inline spreadsheetengine::api::lookupcache::QueryCriteria toApiQueryCriteria(
    const ScLookupCache::QueryCriteria& rCriteria)
{
    const auto eOp = toApiQueryOp(rCriteria.getQueryOp());
    const auto eSearchMode = toApiSearchMode(rCriteria.getSearchMode());
    if (rCriteria.isStringQuery())
    {
        const OUString* pString = rCriteria.getStringValue();
        return spreadsheetengine::api::lookupcache::QueryCriteria::fromString(
            eOp, eSearchMode, pString ? toApiString(*pString) : spreadsheetengine::api::StringView {});
    }

    return spreadsheetengine::api::lookupcache::QueryCriteria::fromDouble(
        eOp, eSearchMode, rCriteria.getDoubleValue());
}

inline ScLookupCache::Result toLibreOfficeLookupResult(
    spreadsheetengine::api::lookupcache::Result eResult)
{
    switch (eResult)
    {
        case spreadsheetengine::api::lookupcache::Result::NotCached:
            return ScLookupCache::NOT_CACHED;
        case spreadsheetengine::api::lookupcache::Result::CriteriaDifferent:
            return ScLookupCache::CRITERIA_DIFFERENT;
        case spreadsheetengine::api::lookupcache::Result::NotAvailable:
            return ScLookupCache::NOT_AVAILABLE;
        case spreadsheetengine::api::lookupcache::Result::Found:
            return ScLookupCache::FOUND;
    }

    return ScLookupCache::NOT_CACHED;
}

inline spreadsheetengine::api::lookupcache::QueryKey makeLookupQueryKey(
    SCROW nRow, SCTAB nTab, ScLookupCache::QueryOp eOp, LookupSearchMode eSearchMode)
{
    return spreadsheetengine::api::lookupcache::makeQueryKey(
        spreadsheetengine::api::CellAddress { nTab, 0, nRow }, toApiQueryOp(eOp),
        toApiSearchMode(eSearchMode));
}

inline ScLookupCache::Result classifyLookup(ScAddress& rResultAddress,
    const ScLookupCache::QueryCriteria& rCriteria,
    const spreadsheetengine::api::lookupcache::CacheEntry* pEntry)
{
    spreadsheetengine::api::CellAddress aResultAddress;
    const auto eResult = spreadsheetengine::api::lookupcache::classifyLookup(
        aResultAddress, toApiQueryCriteria(rCriteria), pEntry);
    if (eResult == spreadsheetengine::api::lookupcache::Result::Found)
        rResultAddress = toLibreOfficeAddress(aResultAddress);
    return toLibreOfficeLookupResult(eResult);
}

template <typename Iterator>
inline SCROW findCachedRowForCriteria(
    Iterator itBegin, Iterator itEnd, const ScLookupCache::QueryCriteria& rCriteria)
{
    return spreadsheetengine::api::lookupcache::findCachedRowForCriteria(
        itBegin, itEnd, toApiQueryCriteria(rCriteria));
}

inline spreadsheetengine::api::lookupcache::CacheEntry makeLookupCacheEntry(
    const ScLookupCache::QueryCriteria& rCriteria, const ScAddress& rResultAddress,
    bool bAvailable)
{
    return spreadsheetengine::api::lookupcache::makeCacheEntry(
        toApiQueryCriteria(rCriteria), toApiCellAddress(rResultAddress), bAvailable);
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
