/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <address.hxx>
#include <document.hxx>
#include <externalrefmgr.hxx>
#include <tokenarray.hxx>

namespace spreadsheetengine::compat::libreoffice::externalreferenceexecution
{

[[nodiscard]] inline FormulaError fetchExternalSingleRefToken(const ScDocument& rDoc,
    const ScAddress& rFormulaPos, sal_uInt16 nFileId, const OUString& rTabName,
    const ScSingleRefData& rRef, ScExternalRefCache::TokenRef& rToken,
    ScExternalRefCache::CellFormat* pFormat = nullptr)
{
    ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
    if (!pRefMgr || !pRefMgr->getExternalFileName(nFileId))
        return FormulaError::NoName;

    if (rRef.IsTabRel())
        return FormulaError::NoRef;

    ScAddress aAddress = rRef.toAbs(rDoc, rFormulaPos);
    ScExternalRefCache::CellFormat aFormat;
    ScExternalRefCache::TokenRef xToken
        = pRefMgr->getSingleRefToken(nFileId, rTabName, aAddress, &rFormulaPos, nullptr, &aFormat);
    if (!xToken)
        return FormulaError::NoRef;
    if (xToken->GetType() == formula::svError)
        return xToken->GetError();

    rToken = std::move(xToken);
    if (pFormat)
        *pFormat = aFormat;
    return FormulaError::NONE;
}

[[nodiscard]] inline FormulaError fetchExternalDoubleRefTokens(const ScDocument& rDoc,
    const ScAddress& rFormulaPos, sal_uInt16 nFileId, const OUString& rTabName,
    const ScComplexRefData& rData, ScExternalRefCache::TokenArrayRef& rArray)
{
    ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
    if (!pRefMgr || !pRefMgr->getExternalFileName(nFileId))
        return FormulaError::NoName;

    if (rData.Ref1.IsTabRel() || rData.Ref2.IsTabRel())
        return FormulaError::NoRef;

    ScComplexRefData aData(rData);
    ScRange aRange = aData.toAbs(rDoc, rFormulaPos);
    if (!rDoc.ValidColRow(aRange.aStart.Col(), aRange.aStart.Row())
        || !rDoc.ValidColRow(aRange.aEnd.Col(), aRange.aEnd.Row()))
    {
        return FormulaError::NoRef;
    }

    ScExternalRefCache::TokenArrayRef xArray
        = pRefMgr->getDoubleRefTokens(nFileId, rTabName, aRange, &rFormulaPos);
    if (!xArray)
        return FormulaError::IllegalArgument;

    formula::FormulaTokenArrayPlainIterator aIter(*xArray);
    formula::FormulaToken* pToken = aIter.First();
    assert(pToken);
    if (pToken->GetType() == formula::svError)
        return pToken->GetError();
    if (pToken->GetType() != formula::svMatrix)
        return FormulaError::IllegalArgument;
    if (aIter.Next())
        return FormulaError::IllegalArgument;

    rArray = std::move(xArray);
    return FormulaError::NONE;
}

} // namespace spreadsheetengine::compat::libreoffice::externalreferenceexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
