/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <formulacell.hxx>
#include <optional>

#include <spreadsheetengine/api/SharedFormula.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/TokenBridge.hxx>
#include <spreadsheetengine/detail/SharedFormulaToken.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::sharedformula::TokenCompareState toApiTokenCompareState(
    ScFormulaCell::CompareState eState)
{
    switch (eState)
    {
        case ScFormulaCell::NotEqual:
            return spreadsheetengine::api::sharedformula::TokenCompareState::NotEqual;
        case ScFormulaCell::EqualInvariant:
            return spreadsheetengine::api::sharedformula::TokenCompareState::EqualInvariant;
        case ScFormulaCell::EqualRelativeRef:
            return spreadsheetengine::api::sharedformula::TokenCompareState::EqualRelativeRef;
    }

    return spreadsheetengine::api::sharedformula::TokenCompareState::NotEqual;
}

inline ScFormulaCell::CompareState toLibreOfficeTokenCompareState(
    spreadsheetengine::api::sharedformula::TokenCompareState eState)
{
    switch (eState)
    {
        case spreadsheetengine::api::sharedformula::TokenCompareState::NotEqual:
            return ScFormulaCell::NotEqual;
        case spreadsheetengine::api::sharedformula::TokenCompareState::EqualInvariant:
            return ScFormulaCell::EqualInvariant;
        case spreadsheetengine::api::sharedformula::TokenCompareState::EqualRelativeRef:
            return ScFormulaCell::EqualRelativeRef;
    }

    return ScFormulaCell::NotEqual;
}

inline std::optional<ScFormulaCell::CompareState> compareSharedFormulaTokenArrays(
    const ScTokenArray& rLeft, const ScTokenArray& rRight)
{
    namespace sesharedtoken = spreadsheetengine::detail::sharedformulatoken;

    if (!rLeft.IsShareable() || !rRight.IsShareable())
        return ScFormulaCell::NotEqual;
    if (rLeft.GetCodeError() != rRight.GetCodeError())
        return ScFormulaCell::NotEqual;

    const auto aLeftLexical = importTokenSequence(rLeft.Tokens());
    if (!aLeftLexical)
        return std::nullopt;
    const auto aRightLexical = importTokenSequence(rRight.Tokens());
    if (!aRightLexical)
        return std::nullopt;

    if (sesharedtoken::hashSharedFormulaLexicalTokens(aLeftLexical.maTokens)
        != sesharedtoken::hashSharedFormulaLexicalTokens(aRightLexical.maTokens))
    {
        return ScFormulaCell::NotEqual;
    }

    const auto aLeftRpn = importTokenSequence(rLeft.RPNTokens());
    if (!aLeftRpn)
        return std::nullopt;
    const auto aRightRpn = importTokenSequence(rRight.RPNTokens());
    if (!aRightRpn)
        return std::nullopt;

    const auto eRpnState = sesharedtoken::compareSharedFormulaTokenStreams(
        sesharedtoken::StreamKind::Rpn, aLeftRpn.maTokens, aRightRpn.maTokens);
    if (eRpnState == spreadsheetengine::api::sharedformula::TokenCompareState::NotEqual)
        return ScFormulaCell::NotEqual;

    const auto eLexicalState = sesharedtoken::compareSharedFormulaTokenStreams(
        sesharedtoken::StreamKind::Lexical, aLeftLexical.maTokens, aRightLexical.maTokens);
    if (eLexicalState == spreadsheetengine::api::sharedformula::TokenCompareState::NotEqual)
        return ScFormulaCell::NotEqual;

    return eRpnState == spreadsheetengine::api::sharedformula::TokenCompareState::EqualRelativeRef
                   || eLexicalState
                          == spreadsheetengine::api::sharedformula::TokenCompareState::EqualRelativeRef
               ? ScFormulaCell::EqualRelativeRef
               : ScFormulaCell::EqualInvariant;
}

inline std::optional<std::size_t> computeSharedFormulaLexicalHash(const ScTokenArray& rArray)
{
    const auto aLexicalTokens = importTokenSequence(rArray.Tokens());
    if (!aLexicalTokens)
        return std::nullopt;

    return spreadsheetengine::detail::sharedformulatoken::hashSharedFormulaLexicalTokens(
        aLexicalTokens.maTokens);
}

inline spreadsheetengine::api::sharedformula::GroupSingleRefListenPlan makeGroupSingleRefListenPlan(
    const ScAddress& rAddress)
{
    return spreadsheetengine::api::sharedformula::makeGroupSingleRefListenPlan(
        toApiCellAddress(rAddress));
}

inline spreadsheetengine::api::sharedformula::GroupDoubleRefListenPlan makeGroupDoubleRefListenPlan(
    const ScRange& rRange, bool bRef1RowRelative, bool bRef2RowRelative, sal_Int32 nGroupLength)
{
    return spreadsheetengine::api::sharedformula::makeGroupDoubleRefListenPlan(
        toApiCellRange(rRange), bRef1RowRelative, bRef2RowRelative, nGroupLength);
}

inline ScRange toLibreOfficeListenedRange(
    const spreadsheetengine::api::sharedformula::GroupDoubleRefListenPlan& rPlan)
{
    return toLibreOfficeRange(rPlan.maListenedRange);
}

inline ScRange toLibreOfficeOriginalRange(
    const spreadsheetengine::api::sharedformula::GroupDoubleRefListenPlan& rPlan)
{
    return toLibreOfficeRange(rPlan.maOriginalRange);
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
