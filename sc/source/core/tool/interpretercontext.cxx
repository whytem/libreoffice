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

#include <interpretercontext.hxx>
#include <spreadsheetengine/detail/ExecutionContext.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>

#include <document.hxx>
#include <comphelper/random.hxx>
#include <formula/token.hxx>
#include <lookupcache.hxx>
#include <rangecache.hxx>
#include <algorithm>

ScInterpreterContextPool ScInterpreterContextPool::aThreadedInterpreterPool(true);
ScInterpreterContextPool ScInterpreterContextPool::aNonThreadedInterpreterPool(false);

ScInterpreterContext::ScInterpreterContext(const ScDocument& rDoc, SvNumberFormatter* pFormatter)
    : mpDoc(&rDoc)
    , mnTokenCachePos(0)
    , maTokens(TOKEN_CACHE_SIZE, nullptr)
    // create a per-interpreter Random Number Generator, seeded from the global rng, so we don't have
    // to lock a mutex to generate a random number
    , aRNG(comphelper::rng::uniform_uint_distribution(0, std::numeric_limits<sal_uInt32>::max()))
    , pInterpreter(nullptr)
    , mpFormatter(pFormatter)
{
    if (!pFormatter)
    {
        mpFormatData = nullptr;
        mpNatNum = nullptr;
    }
    else
        prepFormatterForRoMode(pFormatter);
}

ScInterpreterContext::~ScInterpreterContext() { ResetTokens(); }

void ScInterpreterContext::ResetTokens()
{
    spreadsheetengine::core::execution::resetTokenCache(
        maTokens, mnTokenCachePos,
        [](formula::FormulaTypedDoubleToken* pToken) { pToken->DecRef(); });
}

void ScInterpreterContext::SetDocAndFormatter(const ScDocument& rDoc, SvNumberFormatter* pFormatter)
{
    const auto aPlan = spreadsheetengine::core::execution::planContextRebind(
        mpDoc, &rDoc, mpFormatter, pFormatter);
    if (aPlan.mbResetLookupCache)
    {
        mxScLookupCache.reset();
        mpDoc = &rDoc;
    }
    if (aPlan.mbFormatterChanged)
    {
        mpFormatter = pFormatter;

        // formatter has changed
        prepFormatterForRoMode(pFormatter);

        if (aPlan.mbResetRecentCaches)
        {
            spreadsheetengine::core::execution::resetRecentCache(maNFBuiltInCache);
            spreadsheetengine::core::execution::resetRecentCache(maNFTypeCache);
        }
    }
}

void ScInterpreterContext::prepFormatterForRoMode(SvNumberFormatter* pFormatter)
{
    pFormatter->PrepForRoMode();
    mpFormatData = &pFormatter->GetROFormatData();
    mpNatNum = &pFormatter->GetNatNum();
    mxLanguageData.reset(new SvNFLanguageData(pFormatter->GetROLanguageData()));
    mxAuxFormatKeyMap.reset(new SvNFFormatData::DefaultFormatKeysMap);
    maROPolicy = SvNFEngine::GetROPolicy(*mpFormatData, *mxAuxFormatKeyMap);
}

void ScInterpreterContext::initFormatTable()
{
    mpFormatter = mpDoc->GetFormatTable(); // will assert if not main thread
    prepFormatterForRoMode(mpFormatter);
}

void ScInterpreterContext::MergeDefaultFormatKeys(SvNumberFormatter& rFormatter) const
{
    rFormatter.MergeDefaultFormatKeys(*mxAuxFormatKeyMap);
}

void ScInterpreterContext::Cleanup()
{
    // Do not disturb mxScLookupCache.
    spreadsheetengine::core::execution::cleanupScratchState(
        maConditions, maDelayedSetNumberFormat, maTokens, mnTokenCachePos,
        [](formula::FormulaTypedDoubleToken* pToken) { pToken->DecRef(); });
}

void ScInterpreterContext::ClearLookupCache(const ScDocument* pDoc)
{
    spreadsheetengine::core::execution::clearDocBoundStateIfMatches(
        pDoc, mpDoc, mxScLookupCache, mxLanguageData, mxAuxFormatKeyMap, mpFormatter, mpFormatData,
        mpNatNum);
}

SvNumFormatType ScInterpreterContext::NFGetType(sal_uInt32 nFIndex) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetType(nFIndex);

    const auto& rEntry = spreadsheetengine::core::execution::getOrInsertRecentCacheEntry(
        maNFTypeCache, [nFIndex](const NFType& e) { return e.nKey == nFIndex; },
        [this, nFIndex]() {
            NFType aEntry;
            aEntry.nKey = nFIndex;
            aEntry.eType = mpFormatData->GetType(nFIndex);
            return aEntry;
        });

    return rEntry.eType;
}

const SvNumberformat* ScInterpreterContext::NFGetFormatEntry(sal_uInt32 nKey) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetEntry(nKey);
    return mpFormatData->GetFormatEntry(nKey);
}

bool ScInterpreterContext::NFIsTextFormat(sal_uInt32 nFIndex) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->IsTextFormat(nFIndex);
    return mpFormatData->IsTextFormat(nFIndex);
}

const Date& ScInterpreterContext::NFGetNullDate() const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetNullDate();
    return mxLanguageData->GetNullDate();
}

sal_uInt32 ScInterpreterContext::NFGetTimeFormat(double fNumber, LanguageType eLnge,
                                                 bool bForceDuration) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetTimeFormat(fNumber, eLnge, bForceDuration);
    return SvNFEngine::GetTimeFormat(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy, fNumber,
                                     eLnge, bForceDuration);
}

sal_uInt32 ScInterpreterContext::NFGetFormatIndex(NfIndexTableOffset nTabOff,
                                                  LanguageType eLnge) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetFormatIndex(nTabOff, eLnge);
    return SvNFEngine::GetFormatIndex(*mxLanguageData, maROPolicy, *mpNatNum, nTabOff, eLnge);
}
OUString ScInterpreterContext::NFGetFormatDecimalSep(sal_uInt32 nFormat) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetFormatDecimalSep(nFormat);
    return SvNFEngine::GetFormatDecimalSep(*mxLanguageData, *mpFormatData, nFormat);
}

sal_uInt16 ScInterpreterContext::NFGetFormatPrecision(sal_uInt32 nFormat) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetFormatPrecision(nFormat);
    return SvNFEngine::GetFormatPrecision(*mxLanguageData, *mpFormatData, nFormat);
}

sal_uInt32 ScInterpreterContext::NFGetFormatForLanguageIfBuiltIn(sal_uInt32 nFormat,
                                                                 LanguageType eLnge) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetFormatForLanguageIfBuiltIn(nFormat, eLnge);

    sal_uInt64 nKey = spreadsheetengine::core::execution::composeHighLowCacheKey(nFormat,
                                                                                 eLnge.get());

    const auto& rEntry = spreadsheetengine::core::execution::getOrInsertRecentCacheEntry(
        maNFBuiltInCache, [nKey](const NFBuiltIn& e) { return e.nKey == nKey; },
        [this, nKey, nFormat, eLnge]() {
            NFBuiltIn aEntry;
            aEntry.nKey = nKey;
            aEntry.nFormat = SvNFEngine::GetFormatForLanguageIfBuiltIn(*mxLanguageData, *mpNatNum,
                                                                       maROPolicy, nFormat, eLnge);
            return aEntry;
        });

    return rEntry.nFormat;
}

sal_uInt32 ScInterpreterContext::NFGetStandardFormat(SvNumFormatType eType, LanguageType eLnge)
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetStandardFormat(eType, eLnge);
    return SvNFEngine::GetStandardFormat(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                         eType, eLnge);
}

sal_uInt32 ScInterpreterContext::NFGetStandardFormat(sal_uInt32 nFIndex, SvNumFormatType eType,
                                                     LanguageType eLnge)
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetStandardFormat(nFIndex, eType, eLnge);
    return SvNFEngine::GetStandardFormat(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                         nFIndex, eType, eLnge);
}

OUString ScInterpreterContext::NFGetInputLineString(const double& fOutNumber, sal_uInt32 nFIndex,
                                                    bool bFiltering, bool bForceSystemLocale) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetInputLineString(fOutNumber, nFIndex, bFiltering,
                                                    bForceSystemLocale);
    return SvNFEngine::GetInputLineString(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                          fOutNumber, nFIndex, bFiltering, bForceSystemLocale);
}
void ScInterpreterContext::NFGetOutputString(const double& fOutNumber, sal_uInt32 nFIndex,
                                             OUString& sOutString, const Color** ppColor,
                                             bool bUseStarFormat) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetOutputString(fOutNumber, nFIndex, sOutString, ppColor,
                                                 bUseStarFormat);
    return SvNFEngine::GetOutputString(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                       fOutNumber, nFIndex, sOutString, ppColor, bUseStarFormat);
}

void ScInterpreterContext::NFGetOutputString(const OUString& sString, sal_uInt32 nFIndex,
                                             OUString& sOutString, const Color** ppColor,
                                             bool bUseStarFormat) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetOutputString(sString, nFIndex, sOutString, ppColor,
                                                 bUseStarFormat);
    return SvNFEngine::GetOutputString(*mxLanguageData, *mpFormatData, sString, nFIndex, sOutString,
                                       ppColor, bUseStarFormat);
}

sal_uInt32 ScInterpreterContext::NFGetStandardIndex(LanguageType eLnge) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetStandardIndex(eLnge);
    return SvNFEngine::GetStandardIndex(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                        eLnge);
}

bool ScInterpreterContext::NFGetPreviewString(const OUString& sFormatString, double fPreviewNumber,
                                              OUString& sOutString, const Color** ppColor,
                                              LanguageType eLnge)
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetPreviewString(sFormatString, fPreviewNumber, sOutString,
                                                  ppColor, eLnge);
    return SvNFEngine::GetPreviewString(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                        sFormatString, fPreviewNumber, sOutString, ppColor, eLnge,
                                        false);
}
bool ScInterpreterContext::NFGetPreviewString(const OUString& sFormatString,
                                              const OUString& sPreviewString, OUString& sOutString,
                                              const Color** ppColor, LanguageType eLnge)
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetPreviewString(sFormatString, sPreviewString, sOutString,
                                                  ppColor, eLnge);
    return SvNFEngine::GetPreviewString(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                        sFormatString, sPreviewString, sOutString, ppColor, eLnge);
}

bool ScInterpreterContext::NFGetPreviewStringGuess(const OUString& sFormatString,
                                                   double fPreviewNumber, OUString& sOutString,
                                                   const Color** ppColor, LanguageType eLnge)
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetPreviewStringGuess(sFormatString, fPreviewNumber, sOutString,
                                                       ppColor, eLnge);
    return SvNFEngine::GetPreviewStringGuess(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                             sFormatString, fPreviewNumber, sOutString, ppColor,
                                             eLnge);
}

OUString ScInterpreterContext::NFGenerateFormat(sal_uInt32 nIndex, LanguageType eLnge,
                                                bool bThousand, bool bIsRed, sal_uInt16 nPrecision,
                                                sal_uInt16 nLeadingCnt)
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GenerateFormat(nIndex, eLnge, bThousand, bIsRed, nPrecision,
                                                nLeadingCnt);
    return SvNFEngine::GenerateFormat(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy, nIndex,
                                      eLnge, bThousand, bIsRed, nPrecision, nLeadingCnt);
}
OUString ScInterpreterContext::NFGetCalcCellReturn(sal_uInt32 nFormat) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->GetCalcCellReturn(nFormat);
    return mpFormatData->GetCalcCellReturn(nFormat);
}

sal_uInt16 ScInterpreterContext::NFExpandTwoDigitYear(sal_uInt16 nYear) const
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->ExpandTwoDigitYear(nYear);
    return mxLanguageData->ExpandTwoDigitYear(nYear);
}

bool ScInterpreterContext::NFIsNumberFormat(const OUString& sString, sal_uInt32& F_Index,
                                            double& fOutNumber, SvNumInputOptions eInputOptions)
{
    if (!mpDoc->IsThreadedGroupCalcInProgress())
        return GetFormatTable()->IsNumberFormat(sString, F_Index, fOutNumber, eInputOptions);
    return SvNFEngine::IsNumberFormat(*mxLanguageData, *mpFormatData, *mpNatNum, maROPolicy,
                                      sString, F_Index, fOutNumber, eInputOptions);
}

/* ScInterpreterContextPool */

// Threaded version
void ScInterpreterContextPool::Init(size_t nNumThreads, const ScDocument& rDoc,
                                    SvNumberFormatter* pFormatter)
{
    assert(mbThreaded);
    size_t nOldSize = maPool.size();
    maPool.resize(nNumThreads);
    for (size_t nIdx = 0; nIdx < nNumThreads; ++nIdx)
    {
        const auto aPlan
            = spreadsheetengine::core::execution::planThreadedPoolSlot(nOldSize, nNumThreads, nIdx);
        assert(aPlan.mbValid);
        if (aPlan.mbCreateNew)
            maPool[nIdx].reset(new ScInterpreterContext(rDoc, pFormatter));
        else
            maPool[nIdx]->SetDocAndFormatter(rDoc, pFormatter);
    }
}

ScInterpreterContext*
ScInterpreterContextPool::GetInterpreterContextForThreadIdx(size_t nThreadIdx) const
{
    assert(mbThreaded);
    assert(spreadsheetengine::core::execution::isValidThreadedPoolIndex(maPool.size(),
                                                                        nThreadIdx));
    return maPool[nThreadIdx].get();
}

// Non-Threaded version
void ScInterpreterContextPool::Init(const ScDocument& rDoc, SvNumberFormatter* pFormatter)
{
    assert(!mbThreaded);
    const auto aPlan
        = spreadsheetengine::core::execution::planNonThreadedPoolAcquire(maPool.size(), mnNextFree);
    assert(aPlan.mbValid);
    const size_t nCurrIdx = aPlan.mnSlotIndex;
    if (aPlan.mbCreateNew)
    {
        maPool.resize(maPool.size() + 1);
        maPool[nCurrIdx].reset(new ScInterpreterContext(rDoc, pFormatter));
    }
    else
        maPool[nCurrIdx]->SetDocAndFormatter(rDoc, pFormatter);

    mnNextFree = aPlan.mnNextFreeAfterAcquire;
}

ScInterpreterContext* ScInterpreterContextPool::GetInterpreterContext() const
{
    assert(!mbThreaded);
    assert(spreadsheetengine::core::execution::hasActiveNonThreadedPoolContext(
        maPool.size(), mnNextFree));
    return maPool[spreadsheetengine::core::execution::activeNonThreadedPoolContextIndex(
                      maPool.size(), mnNextFree)]
        .get();
}

void ScInterpreterContextPool::ReturnToPool()
{
    if (mbThreaded)
    {
        spreadsheetengine::core::execution::forEachLivePoolContext(maPool,
                                                                   [](ScInterpreterContext& rCtx) {
                                                                       rCtx.Cleanup();
                                                                   });
    }
    else
    {
        const auto aPlan = spreadsheetengine::core::execution::planNonThreadedPoolRelease(
            maPool.size(), mnNextFree);
        assert(aPlan.mbValid);
        mnNextFree = aPlan.mnNextFreeAfterRelease;
        maPool[aPlan.mnReleasedIndex]->Cleanup();
    }
}

// static
void ScInterpreterContextPool::ClearLookupCaches(const ScDocument* pDoc)
{
    spreadsheetengine::core::execution::forEachLivePoolContext(
        aThreadedInterpreterPool.maPool,
        [pDoc](ScInterpreterContext& rCtx) { rCtx.ClearLookupCache(pDoc); });
    spreadsheetengine::core::execution::forEachLivePoolContext(
        aNonThreadedInterpreterPool.maPool,
        [pDoc](ScInterpreterContext& rCtx) { rCtx.ClearLookupCache(pDoc); });
}

// static
void ScInterpreterContextPool::ModuleExiting()
{
    spreadsheetengine::core::execution::forEachLivePoolContext(
        aThreadedInterpreterPool.maPool,
        [](ScInterpreterContext& rCtx) { rCtx.mxLanguageData.reset(); });
    spreadsheetengine::core::execution::forEachLivePoolContext(
        aNonThreadedInterpreterPool.maPool,
        [](ScInterpreterContext& rCtx) { rCtx.mxLanguageData.reset(); });
}

/* ScThreadedInterpreterContextGetterGuard */

ScThreadedInterpreterContextGetterGuard::ScThreadedInterpreterContextGetterGuard(
    size_t nNumThreads, const ScDocument& rDoc, SvNumberFormatter* pFormatter)
    : rPool(ScInterpreterContextPool::aThreadedInterpreterPool)
{
    rPool.Init(nNumThreads, rDoc, pFormatter);
}

ScThreadedInterpreterContextGetterGuard::~ScThreadedInterpreterContextGetterGuard()
{
    rPool.ReturnToPool();
}

ScInterpreterContext*
ScThreadedInterpreterContextGetterGuard::GetInterpreterContextForThreadIdx(size_t nThreadIdx) const
{
    return rPool.GetInterpreterContextForThreadIdx(nThreadIdx);
}

/* ScInterpreterContextGetterGuard */

ScInterpreterContextGetterGuard::ScInterpreterContextGetterGuard(const ScDocument& rDoc,
                                                                 SvNumberFormatter* pFormatter)
    : rPool(ScInterpreterContextPool::aNonThreadedInterpreterPool)
#if !defined NDEBUG
    , nContextIdx(rPool.mnNextFree)
#endif
{
    rPool.Init(rDoc, pFormatter);
}

ScInterpreterContextGetterGuard::~ScInterpreterContextGetterGuard()
{
    assert(nContextIdx == spreadsheetengine::core::execution::activeNonThreadedPoolContextIndex(
                              rPool.maPool.size(), rPool.mnNextFree));
    rPool.ReturnToPool();
}

ScInterpreterContext* ScInterpreterContextGetterGuard::GetInterpreterContext() const
{
    return rPool.GetInterpreterContext();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
