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

#include <spreadsheetengine/compat/libreoffice/LookupCache.hxx>

#include <sal/log.hxx>

namespace selookupcache = spreadsheetengine::api::lookupcache;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;

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
    return selibreoffice::toApiQueryCriteria(*this) == selibreoffice::toApiQueryCriteria(r);
}

bool ScLookupCache::QueryCriteria::isEmptyStringQuery() const
{
    return selibreoffice::toApiQueryCriteria(*this).isEmptyStringQuery();
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

    const auto aLeft
        = selibreoffice::makeLookupQueryKey(mnRow, mnTab, meOp, meSearchMode);
    const auto aRight
        = selibreoffice::makeLookupQueryKey(r.mnRow, r.mnTab, r.meOp, r.meSearchMode);
    return aLeft == aRight;
}

size_t ScLookupCache::QueryKey::Hash::operator()( const QueryKey & r ) const
{
    const auto aKey
        = selibreoffice::makeLookupQueryKey(r.mnRow, r.mnTab, r.meOp, r.meSearchMode);
    return selookupcache::QueryKey::Hash {}(aKey);
}

ScLookupCache::Result ScLookupCache::lookup( ScAddress & o_rResultAddress,
        const QueryCriteria & rCriteria, const ScAddress & rQueryAddress ) const
{
    auto it( maQueryMap.find( QueryKey( rQueryAddress,
                    rCriteria.getQueryOp(), rCriteria.getSearchMode())));
    if (it == maQueryMap.end())
        return NOT_CACHED;

    return selibreoffice::classifyLookup(o_rResultAddress, rCriteria, &it->second.maEntry);
}

SCROW ScLookupCache::lookup( const QueryCriteria & rCriteria ) const
{
    return selibreoffice::findCachedRowForCriteria(
        maQueryMap.begin(), maQueryMap.end(), rCriteria);
}

bool ScLookupCache::insert( const ScAddress & rResultAddress,
        const QueryCriteria & rCriteria, const ScAddress & rQueryAddress,
        const bool bAvailable )
{
    QueryKey aKey( rQueryAddress, rCriteria.getQueryOp(), rCriteria.getSearchMode() );
    QueryCriteriaAndResult aResult(
        selibreoffice::makeLookupCacheEntry(rCriteria, rResultAddress, bAvailable));
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
