/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <lookupcache.hxx>
#include <document.hxx>
#include <lookupsearchmode.hxx>
#include <queryentry.hxx>
#include <brdcst.hxx>

#include <spreadsheetengine/api/LookupCache.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

#include <sal/log.hxx>

namespace selookup = spreadsheetengine::api::lookup;
namespace selookupcache = spreadsheetengine::api::lookupcache;

namespace
{

selookup::SearchMode toApiSearchMode(LookupSearchMode eSearchMode)
{
    switch (eSearchMode)
    {
        case LookupSearchMode::Forward:
            return selookup::SearchMode::Forward;
        case LookupSearchMode::Reverse:
            return selookup::SearchMode::Reverse;
        case LookupSearchMode::BinaryAscending:
            return selookup::SearchMode::BinaryAscending;
        case LookupSearchMode::BinaryDescending:
            return selookup::SearchMode::BinaryDescending;
    }

    return selookup::SearchMode::Forward;
}

selookupcache::QueryOp toApiQueryOp(ScLookupCache::QueryOp eOp)
{
    switch (eOp)
    {
        case ScLookupCache::EQUAL:
            return selookupcache::QueryOp::Equal;
        case ScLookupCache::LESS_EQUAL:
            return selookupcache::QueryOp::LessEqual;
        case ScLookupCache::GREATER_EQUAL:
            return selookupcache::QueryOp::GreaterEqual;
        case ScLookupCache::UNKNOWN:
            break;
    }

    return selookupcache::QueryOp::Unknown;
}

selookupcache::QueryCriteria toApiQueryCriteria(const ScLookupCache::QueryCriteria& rCriteria)
{
    const auto eOp = toApiQueryOp(rCriteria.getQueryOp());
    const auto eSearchMode = toApiSearchMode(rCriteria.getSearchMode());
    if (rCriteria.isStringQuery())
    {
        const OUString* pString = rCriteria.getStringValue();
        return selookupcache::QueryCriteria::fromString(
            eOp, eSearchMode,
            pString ? spreadsheetengine::compat::libreoffice::toApiString(*pString)
                    : spreadsheetengine::api::StringView {});
    }

    return selookupcache::QueryCriteria::fromDouble(
        eOp, eSearchMode, rCriteria.getDoubleValue());
}

ScLookupCache::Result toCalcLookupResult(selookupcache::Result eResult)
{
    switch (eResult)
    {
        case selookupcache::Result::NotCached:
            return ScLookupCache::NOT_CACHED;
        case selookupcache::Result::CriteriaDifferent:
            return ScLookupCache::CRITERIA_DIFFERENT;
        case selookupcache::Result::NotAvailable:
            return ScLookupCache::NOT_AVAILABLE;
        case selookupcache::Result::Found:
            return ScLookupCache::FOUND;
    }

    return ScLookupCache::NOT_CACHED;
}

} // end anonymous namespace

ScLookupCache::QueryCriteria::QueryCriteria( const ScQueryEntry& rEntry, LookupSearchMode nSearchMode ) :
    mfVal(0.0), mbAlloc(false), mbString(false), meSearchMode(nSearchMode)
{
    switch (rEntry.eOp)
    {
        case SC_EQUAL :
            meOp = EQUAL;
            break;
        case SC_LESS_EQUAL :
            meOp = LESS_EQUAL;
            break;
        case SC_GREATER_EQUAL :
            meOp = GREATER_EQUAL;
            break;
        default:
            meOp = UNKNOWN;
            SAL_WARN( "sc.core", "ScLookupCache::QueryCriteria not prepared for this ScQueryOp");
    }

    const ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
    if (rItem.meType == ScQueryEntry::ByString)
        setString(rItem.maString.getString());
    else
        setDouble(rItem.mfVal);
}

ScLookupCache::QueryCriteria::QueryCriteria( const ScLookupCache::QueryCriteria & r ) :
    mfVal( r.mfVal),
    mbAlloc( false),
    mbString( false),
    meOp( r.meOp),
    meSearchMode( r.meSearchMode)
{
    if (r.mbString && r.mpStr)
    {
        mpStr = new OUString( *r.mpStr);
        mbAlloc = mbString = true;
    }
}

ScLookupCache::QueryCriteria::~QueryCriteria()
{
    deleteString();
}

bool ScLookupCache::QueryCriteria::operator==( const QueryCriteria & r ) const
{
    return toApiQueryCriteria(*this) == toApiQueryCriteria(r);
}

bool ScLookupCache::QueryCriteria::isEmptyStringQuery() const
{
    return toApiQueryCriteria(*this).isEmptyStringQuery();
}

ScLookupCache::QueryKey::QueryKey(
    const ScAddress & rAddress, const QueryOp eOp, LookupSearchMode eSearchMode )
    : mnRow( rAddress.Row())
    , mnTab( rAddress.Tab())
    , meOp( eOp)
    , meSearchMode( eSearchMode)
{
}

bool ScLookupCache::QueryKey::operator==( const QueryKey & r ) const
{
    if (meOp == UNKNOWN || r.meOp == UNKNOWN)
        return false;

    const auto aLeft = selookupcache::makeQueryKey(
        spreadsheetengine::api::CellAddress { mnTab, 0, mnRow }, toApiQueryOp(meOp),
        toApiSearchMode(meSearchMode));
    const auto aRight = selookupcache::makeQueryKey(
        spreadsheetengine::api::CellAddress { r.mnTab, 0, r.mnRow }, toApiQueryOp(r.meOp),
        toApiSearchMode(r.meSearchMode));
    return aLeft == aRight;
}

size_t ScLookupCache::QueryKey::Hash::operator()( const QueryKey & r ) const
{
    const auto aKey = selookupcache::makeQueryKey(
        spreadsheetengine::api::CellAddress { r.mnTab, 0, r.mnRow }, toApiQueryOp(r.meOp),
        toApiSearchMode(r.meSearchMode));
    return selookupcache::QueryKey::Hash {}(aKey);
}

ScLookupCache::Result ScLookupCache::lookup( ScAddress & o_rResultAddress,
        const QueryCriteria & rCriteria, const ScAddress & rQueryAddress ) const
{
    auto it( maQueryMap.find( QueryKey( rQueryAddress,
                    rCriteria.getQueryOp(), rCriteria.getSearchMode())));
    if (it == maQueryMap.end())
        return NOT_CACHED;

    spreadsheetengine::api::CellAddress aResultAddress;
    const auto eResult
        = selookupcache::classifyLookup(aResultAddress, toApiQueryCriteria(rCriteria), &it->second.maEntry);
    if (eResult == selookupcache::Result::Found)
        o_rResultAddress = spreadsheetengine::compat::libreoffice::toLibreOfficeAddress(aResultAddress);
    return toCalcLookupResult(eResult);
}

SCROW ScLookupCache::lookup( const QueryCriteria & rCriteria ) const
{
    return selookupcache::findCachedRowForCriteria(
        maQueryMap.begin(), maQueryMap.end(), toApiQueryCriteria(rCriteria));
}

bool ScLookupCache::insert( const ScAddress & rResultAddress,
        const QueryCriteria & rCriteria, const ScAddress & rQueryAddress,
        const bool bAvailable )
{
    QueryKey aKey( rQueryAddress, rCriteria.getQueryOp(), rCriteria.getSearchMode() );
    QueryCriteriaAndResult aResult(selookupcache::makeCacheEntry(toApiQueryCriteria(rCriteria),
        spreadsheetengine::compat::libreoffice::toApiCellAddress(rResultAddress), bAvailable));
    bool bInserted = maQueryMap.insert( ::std::pair< const QueryKey,
            QueryCriteriaAndResult>( aKey, aResult)).second;

    return bInserted;
}

void ScLookupCache::Notify( const SfxHint& rHint )
{
    if (!mpDoc->IsInDtorClear())
    {
        if (rHint.GetId() == SfxHintId::ScDataChanged || rHint.GetId() == SfxHintId::ScAreaChanged)
        {
            mpDoc->RemoveLookupCache( *this);
            // this ScLookupCache is deleted by RemoveLookupCache
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
