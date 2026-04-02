/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <unordered_map>

#include <formula/tokenarray.hxx>
#include <tokenarray.hxx>

namespace spreadsheetengine::compat::libreoffice::letexecution
{

inline void replaceNamesToResult(
    const std::unordered_map<OUString, formula::FormulaToken*>& rResultIndexes,
    ScTokenArray& rTokens, short nStartPos, short nEndPos)
{
    formula::FormulaTokenArrayPlainIterator aIterator(rTokens);
    aIterator.Jump(nStartPos + 1);
    for (formula::FormulaToken* pToken = aIterator.GetNextStringName(); pToken;
         pToken = aIterator.GetNextStringName())
    {
        if (aIterator.GetIndex() > nEndPos)
            break;

        const auto itResult = rResultIndexes.find(pToken->GetString().getString());
        if (itResult != rResultIndexes.end())
            rTokens.ReplaceRPNToken(aIterator.GetIndex() - 1, itResult->second->Clone());
    }
}

inline ScTokenArray copyTokenSlice(
    const ScDocument& rDocument, const ScTokenArray& rTokens, short nStartPos, short nEndPos)
{
    formula::FormulaTokenArrayPlainIterator aIterator(rTokens);
    aIterator.Jump(nStartPos + 1);
    ScTokenArray aSlice(rDocument);
    for (formula::FormulaToken* pToken = aIterator.NextRPN(); pToken;
         pToken = aIterator.NextRPN())
    {
        if (aIterator.GetIndex() > nEndPos)
            break;

        aSlice.AddToken(*pToken->Clone());
    }
    return aSlice;
}

} // namespace spreadsheetengine::compat::libreoffice::letexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
