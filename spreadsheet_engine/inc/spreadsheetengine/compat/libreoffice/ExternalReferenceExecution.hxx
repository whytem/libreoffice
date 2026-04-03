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
#include <types.hxx>

namespace spreadsheetengine::compat::libreoffice::externalreferenceexecution
{

struct ExternalSingleRefFetch
{
    FormulaError meError = FormulaError::NONE;
    ScExternalRefCache::TokenRef mxToken;
    ScExternalRefCache::CellFormat maFormat;
};

struct ExternalDoubleRefFetch
{
    FormulaError meError = FormulaError::NONE;
    ScExternalRefCache::TokenArrayRef mxArray;
};

struct ExternalDoubleRefMatrixProjection
{
    FormulaError meError = FormulaError::NONE;
    ScMatrixRef mxMatrix;
};

[[nodiscard]] inline ExternalSingleRefFetch fetchExternalSingleRef(const ScDocument& rDoc,
    const ScAddress& rFormulaPos, sal_uInt16 nFileId, const OUString& rTabName,
    const ScSingleRefData& rRef)
{
    ExternalSingleRefFetch aFetch;
    ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
    if (!pRefMgr || !pRefMgr->getExternalFileName(nFileId))
    {
        aFetch.meError = FormulaError::NoName;
        return aFetch;
    }

    if (rRef.IsTabRel())
    {
        aFetch.meError = FormulaError::NoRef;
        return aFetch;
    }

    ScAddress aAddress = rRef.toAbs(rDoc, rFormulaPos);
    ScExternalRefCache::TokenRef xToken = pRefMgr->getSingleRefToken(
        nFileId, rTabName, aAddress, &rFormulaPos, nullptr, &aFetch.maFormat);
    if (!xToken)
    {
        aFetch.meError = FormulaError::NoRef;
        return aFetch;
    }
    if (xToken->GetType() == formula::svError)
    {
        aFetch.meError = xToken->GetError();
        return aFetch;
    }

    aFetch.mxToken = std::move(xToken);
    return aFetch;
}

[[nodiscard]] inline ExternalDoubleRefFetch fetchExternalDoubleRef(const ScDocument& rDoc,
    const ScAddress& rFormulaPos, sal_uInt16 nFileId, const OUString& rTabName,
    const ScComplexRefData& rData)
{
    ExternalDoubleRefFetch aFetch;
    ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
    if (!pRefMgr || !pRefMgr->getExternalFileName(nFileId))
    {
        aFetch.meError = FormulaError::NoName;
        return aFetch;
    }

    if (rData.Ref1.IsTabRel() || rData.Ref2.IsTabRel())
    {
        aFetch.meError = FormulaError::NoRef;
        return aFetch;
    }

    ScComplexRefData aData(rData);
    ScRange aRange = aData.toAbs(rDoc, rFormulaPos);
    if (!rDoc.ValidColRow(aRange.aStart.Col(), aRange.aStart.Row())
        || !rDoc.ValidColRow(aRange.aEnd.Col(), aRange.aEnd.Row()))
    {
        aFetch.meError = FormulaError::NoRef;
        return aFetch;
    }

    ScExternalRefCache::TokenArrayRef xArray
        = pRefMgr->getDoubleRefTokens(nFileId, rTabName, aRange, &rFormulaPos);
    if (!xArray)
    {
        aFetch.meError = FormulaError::IllegalArgument;
        return aFetch;
    }

    formula::FormulaTokenArrayPlainIterator aIter(*xArray);
    formula::FormulaToken* pToken = aIter.First();
    assert(pToken);
    if (pToken->GetType() == formula::svError)
    {
        aFetch.meError = pToken->GetError();
        return aFetch;
    }
    if (pToken->GetType() != formula::svMatrix)
    {
        aFetch.meError = FormulaError::IllegalArgument;
        return aFetch;
    }
    if (aIter.Next())
    {
        aFetch.meError = FormulaError::IllegalArgument;
        return aFetch;
    }

    aFetch.mxArray = std::move(xArray);
    return aFetch;
}

[[nodiscard]] inline ExternalDoubleRefMatrixProjection projectExternalDoubleRefMatrix(
    const ScExternalRefCache::TokenArrayRef& xArray)
{
    ExternalDoubleRefMatrixProjection aProjection;
    if (!xArray)
    {
        aProjection.meError = FormulaError::IllegalArgument;
        return aProjection;
    }

    formula::FormulaToken* pToken = xArray->FirstToken();
    if (!pToken || pToken->GetType() != formula::svMatrix)
    {
        aProjection.meError = FormulaError::IllegalArgument;
        return aProjection;
    }

    aProjection.mxMatrix = pToken->GetMatrix();
    if (!aProjection.mxMatrix)
        aProjection.meError = FormulaError::UnknownVariable;

    return aProjection;
}

} // namespace spreadsheetengine::compat::libreoffice::externalreferenceexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
