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
#include <spreadsheetengine/runtime/FloatingPoint.hxx>

#include <spreadsheetengine/api/SharedFormula.hxx>
#include <spreadsheetengine/detail/TokenModel.hxx>

namespace spreadsheetengine::detail::sharedformulatoken
{

namespace setoken = spreadsheetengine::detail::token;
namespace seshared = spreadsheetengine::api::sharedformula;

enum class StreamKind
{
    Lexical,
    Rpn,
};

[[nodiscard]] inline bool isSingleReferenceKind(setoken::Kind eKind)
{
    return eKind == setoken::Kind::SingleRef || eKind == setoken::Kind::ColRowName;
}

[[nodiscard]] inline std::size_t hashSharedFormulaSingleRef(
    const api::refdata::SingleRefData& rReference)
{
    std::size_t nHash = 0;
    nHash += static_cast<std::size_t>(rReference.maFlags.mbColumnRelative);
    nHash += static_cast<std::size_t>(rReference.maFlags.mbRowRelative) << 1;
    nHash += static_cast<std::size_t>(rReference.maFlags.mbSheetRelative) << 2;
    return nHash;
}

[[nodiscard]] inline std::size_t hashSharedFormulaLexicalTokens(
    const std::vector<setoken::Token>& rTokens)
{
    std::size_t nHash = 1;
    const std::size_t nCount = std::min<std::size_t>(rTokens.size(), 20);
    for (std::size_t i = 0; i < nCount; ++i)
    {
        const setoken::Token& rToken = rTokens[i];
        if (rToken.mnOpCode == setoken::kOpCodePush)
        {
            switch (rToken.meKind)
            {
                case setoken::Kind::Byte:
                    nHash += std::get<setoken::ByteData>(rToken.maPayload).mnByte;
                    break;
                case setoken::Kind::Value:
                    nHash += std::hash<double> {}(std::get<double>(rToken.maPayload));
                    break;
                case setoken::Kind::String:
                    nHash += std::hash<std::u16string_view> {}(
                        std::get<setoken::StringData>(rToken.maPayload).maText);
                    break;
                case setoken::Kind::SingleRef:
                    nHash += hashSharedFormulaSingleRef(
                        std::get<api::refdata::SingleRefData>(rToken.maPayload));
                    break;
                case setoken::Kind::DoubleRef:
                {
                    const auto& rReference
                        = std::get<api::refdata::ComplexRefData>(rToken.maPayload);
                    nHash += hashSharedFormulaSingleRef(rReference.maRef1);
                    nHash += hashSharedFormulaSingleRef(rReference.maRef2);
                    break;
                }
                default:
                    nHash += static_cast<std::size_t>(rToken.mnOpCode);
                    break;
            }
        }
        else
            nHash += static_cast<std::size_t>(rToken.mnOpCode);

        nHash = (nHash << 4) - nHash;
    }

    return nHash;
}

inline void updateInvariantForReference(
    const api::refdata::SingleRefData& rReference, bool& rbInvariant)
{
    if (rReference.maFlags.mbRowRelative)
        rbInvariant = false;
}

inline void updateInvariantForReference(
    const api::refdata::ComplexRefData& rReference, bool& rbInvariant)
{
    updateInvariantForReference(rReference.maRef1, rbInvariant);
    updateInvariantForReference(rReference.maRef2, rbInvariant);
}

[[nodiscard]] inline bool compareRangeNamePayload(const setoken::Token& rLeft, const setoken::Token& rRight)
{
    return std::get<setoken::NameData>(rLeft.maPayload)
           == std::get<setoken::NameData>(rRight.maPayload);
}

[[nodiscard]] inline bool compareDatabaseRangePayload(
    const setoken::Token& rLeft, const setoken::Token& rRight)
{
    return std::get<setoken::DatabaseRangeData>(rLeft.maPayload)
           == std::get<setoken::DatabaseRangeData>(rRight.maPayload);
}

[[nodiscard]] inline bool compareTableRefPayloadForLegacySharedFormula(
    const setoken::Token& rLeft, const setoken::Token& rRight)
{
    return std::get<setoken::TableRefData>(rLeft.maPayload).mnIndex
           == std::get<setoken::TableRefData>(rRight.maPayload).mnIndex;
}

[[nodiscard]] inline seshared::TokenCompareState compareSharedFormulaTokenStreams(
    StreamKind eStream, const std::vector<setoken::Token>& rLeft,
    const std::vector<setoken::Token>& rRight)
{
    if (rLeft.size() != rRight.size())
        return seshared::TokenCompareState::NotEqual;

    bool bInvariant = true;
    for (std::size_t i = 0; i < rLeft.size(); ++i)
    {
        const setoken::Token& rLeftToken = rLeft[i];
        const setoken::Token& rRightToken = rRight[i];
        if (rLeftToken.meKind != rRightToken.meKind || rLeftToken.mnOpCode != rRightToken.mnOpCode)
            return seshared::TokenCompareState::NotEqual;

        switch (eStream)
        {
            case StreamKind::Rpn:
                switch (rLeftToken.meKind)
                {
                    case setoken::Kind::Matrix:
                    case setoken::Kind::ExternalSingleRef:
                    case setoken::Kind::ExternalDoubleRef:
                        return seshared::TokenCompareState::NotEqual;
                    case setoken::Kind::SingleRef:
                    case setoken::Kind::ColRowName:
                    {
                        const auto& rLeftReference
                            = std::get<api::refdata::SingleRefData>(rLeftToken.maPayload);
                        const auto& rRightReference
                            = std::get<api::refdata::SingleRefData>(rRightToken.maPayload);
                        if (!(rLeftReference == rRightReference))
                            return seshared::TokenCompareState::NotEqual;
                        updateInvariantForReference(rLeftReference, bInvariant);
                        break;
                    }
                    case setoken::Kind::DoubleRef:
                    {
                        const auto& rLeftReference
                            = std::get<api::refdata::ComplexRefData>(rLeftToken.maPayload);
                        const auto& rRightReference
                            = std::get<api::refdata::ComplexRefData>(rRightToken.maPayload);
                        if (!(rLeftReference == rRightReference))
                            return seshared::TokenCompareState::NotEqual;
                        updateInvariantForReference(rLeftReference, bInvariant);
                        break;
                    }
                    case setoken::Kind::Value:
                        if (!spreadsheetengine::core::fp::approxEqual(std::get<double>(rLeftToken.maPayload),
                                std::get<double>(rRightToken.maPayload)))
                        {
                            return seshared::TokenCompareState::NotEqual;
                        }
                        break;
                    case setoken::Kind::String:
                        if (!(std::get<setoken::StringData>(rLeftToken.maPayload)
                              == std::get<setoken::StringData>(rRightToken.maPayload)))
                        {
                            return seshared::TokenCompareState::NotEqual;
                        }
                        break;
                    case setoken::Kind::RangeName:
                        if (!compareRangeNamePayload(rLeftToken, rRightToken))
                            return seshared::TokenCompareState::NotEqual;
                        break;
                    case setoken::Kind::DatabaseRange:
                        if (!compareDatabaseRangePayload(rLeftToken, rRightToken))
                            return seshared::TokenCompareState::NotEqual;
                        break;
                    case setoken::Kind::TableRef:
                        if (!compareTableRefPayloadForLegacySharedFormula(rLeftToken, rRightToken))
                            return seshared::TokenCompareState::NotEqual;
                        break;
                    case setoken::Kind::Byte:
                        if (!(std::get<setoken::ByteData>(rLeftToken.maPayload)
                              == std::get<setoken::ByteData>(rRightToken.maPayload)))
                        {
                            return seshared::TokenCompareState::NotEqual;
                        }
                        break;
                    case setoken::Kind::Error:
                        if (std::get<setoken::ErrorCode>(rLeftToken.maPayload)
                            != std::get<setoken::ErrorCode>(rRightToken.maPayload))
                        {
                            return seshared::TokenCompareState::NotEqual;
                        }
                        break;
                    default:
                        break;
                }
                break;
            case StreamKind::Lexical:
                switch (rLeftToken.meKind)
                {
                    case setoken::Kind::SingleRef:
                    case setoken::Kind::ColRowName:
                    {
                        const auto& rLeftReference
                            = std::get<api::refdata::SingleRefData>(rLeftToken.maPayload);
                        const auto& rRightReference
                            = std::get<api::refdata::SingleRefData>(rRightToken.maPayload);
                        if (!(rLeftReference == rRightReference))
                            return seshared::TokenCompareState::NotEqual;
                        updateInvariantForReference(rLeftReference, bInvariant);
                        break;
                    }
                    case setoken::Kind::DoubleRef:
                    {
                        const auto& rLeftReference
                            = std::get<api::refdata::ComplexRefData>(rLeftToken.maPayload);
                        const auto& rRightReference
                            = std::get<api::refdata::ComplexRefData>(rRightToken.maPayload);
                        if (!(rLeftReference == rRightReference))
                            return seshared::TokenCompareState::NotEqual;
                        updateInvariantForReference(rLeftReference, bInvariant);
                        break;
                    }
                    case setoken::Kind::RangeName:
                        if (!compareRangeNamePayload(rLeftToken, rRightToken))
                            return seshared::TokenCompareState::NotEqual;
                        break;
                    case setoken::Kind::DatabaseRange:
                        if (!compareDatabaseRangePayload(rLeftToken, rRightToken))
                            return seshared::TokenCompareState::NotEqual;
                        break;
                    case setoken::Kind::TableRef:
                        if (!compareTableRefPayloadForLegacySharedFormula(rLeftToken, rRightToken))
                            return seshared::TokenCompareState::NotEqual;
                        break;
                    default:
                        break;
                }
                break;
        }
    }

    return bInvariant ? seshared::TokenCompareState::EqualInvariant
                      : seshared::TokenCompareState::EqualRelativeRef;
}

} // namespace spreadsheetengine::detail::sharedformulatoken

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
