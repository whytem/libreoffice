/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string>

#include <spreadsheetengine/detail/TokenModel.hxx>

namespace spreadsheetengine::detail::tokenstringifier
{

namespace setoken = spreadsheetengine::detail::token;

inline void appendAscii(api::String& rOut, std::string_view rText)
{
    rOut.reserve(rOut.size() + rText.size());
    for (char cChar : rText)
        rOut.push_back(static_cast<char16_t>(cChar));
}

template <typename T> inline void appendNumber(api::String& rOut, T nValue)
{
    appendAscii(rOut, std::to_string(nValue));
}

inline void appendBool(api::String& rOut, bool bValue)
{
    appendAscii(rOut, bValue ? "true" : "false");
}

inline void appendQuoted(api::String& rOut, api::StringView rText)
{
    rOut.push_back(u'"');
    rOut.append(rText);
    rOut.push_back(u'"');
}

inline api::String singleRefToDiagnosticString(const api::refdata::SingleRefData& rReference)
{
    api::String aText;
    appendAscii(aText, "{c=");
    appendNumber(aText, rReference.mnColumn);
    appendAscii(aText, ",r=");
    appendNumber(aText, rReference.mnRow);
    appendAscii(aText, ",s=");
    appendNumber(aText, rReference.mnSheet);
    appendAscii(aText, ",rel=[");
    appendBool(aText, rReference.maFlags.mbColumnRelative);
    appendAscii(aText, ",");
    appendBool(aText, rReference.maFlags.mbRowRelative);
    appendAscii(aText, ",");
    appendBool(aText, rReference.maFlags.mbSheetRelative);
    appendAscii(aText, "]}");
    return aText;
}

inline api::String tokenPayloadToDiagnosticString(const setoken::Token& rToken)
{
    using setoken::Kind;

    api::String aText;
    switch (rToken.meKind)
    {
        case Kind::PlainOpcode:
        case Kind::Missing:
            break;
        case Kind::Byte:
        {
            const auto& rData = std::get<setoken::ByteData>(rToken.maPayload);
            appendAscii(aText, " byte=");
            appendNumber(aText, rData.mnByte);
            break;
        }
        case Kind::Value:
            appendAscii(aText, " value=");
            appendAscii(aText, std::to_string(std::get<double>(rToken.maPayload)));
            break;
        case Kind::String:
        case Kind::StringName:
            appendAscii(aText, " text=");
            appendQuoted(aText, std::get<setoken::StringData>(rToken.maPayload).maText);
            break;
        case Kind::SingleRef:
        case Kind::ColRowName:
            appendAscii(aText, " ref=");
            aText += singleRefToDiagnosticString(
                std::get<api::refdata::SingleRefData>(rToken.maPayload));
            break;
        case Kind::DoubleRef:
        {
            const auto& rData = std::get<api::refdata::ComplexRefData>(rToken.maPayload);
            appendAscii(aText, " range=");
            aText += singleRefToDiagnosticString(rData.maRef1);
            appendAscii(aText, ":");
            aText += singleRefToDiagnosticString(rData.maRef2);
            break;
        }
        case Kind::RangeName:
        {
            const auto& rData = std::get<setoken::NameData>(rToken.maPayload);
            appendAscii(aText, " name(sheet=");
            appendNumber(aText, rData.mnSheet);
            appendAscii(aText, ",index=");
            appendNumber(aText, rData.mnIndex);
            appendAscii(aText, ")");
            break;
        }
        case Kind::DatabaseRange:
            appendAscii(aText, " dbrange=");
            appendNumber(aText, std::get<setoken::DatabaseRangeData>(rToken.maPayload).mnIndex);
            break;
        case Kind::ExternalSingleRef:
        {
            const auto& rData = std::get<setoken::ExternalSingleRefData>(rToken.maPayload);
            appendAscii(aText, " extref(file=");
            appendNumber(aText, rData.mnFileId);
            appendAscii(aText, ",tab=");
            appendQuoted(aText, rData.maTabName);
            appendAscii(aText, ",ref=");
            aText += singleRefToDiagnosticString(rData.maReference);
            appendAscii(aText, ")");
            break;
        }
        case Kind::ExternalDoubleRef:
        {
            const auto& rData = std::get<setoken::ExternalDoubleRefData>(rToken.maPayload);
            appendAscii(aText, " extrange(file=");
            appendNumber(aText, rData.mnFileId);
            appendAscii(aText, ",tab=");
            appendQuoted(aText, rData.maTabName);
            appendAscii(aText, ")");
            break;
        }
        case Kind::ExternalName:
        {
            const auto& rData = std::get<setoken::ExternalNameData>(rToken.maPayload);
            appendAscii(aText, " extname(file=");
            appendNumber(aText, rData.mnFileId);
            appendAscii(aText, ",name=");
            appendQuoted(aText, rData.maName);
            appendAscii(aText, ")");
            break;
        }
        case Kind::Matrix:
        {
            const auto& rData = std::get<setoken::MatrixData>(rToken.maPayload);
            appendAscii(aText, " matrix(");
            appendNumber(aText, rData.mnColumns);
            appendAscii(aText, "x");
            appendNumber(aText, rData.mnRows);
            appendAscii(aText, ")");
            break;
        }
        case Kind::TableRef:
        {
            const auto& rData = std::get<setoken::TableRefData>(rToken.maPayload);
            appendAscii(aText, " tableref(index=");
            appendNumber(aText, rData.mnIndex);
            appendAscii(aText, ",item=");
            appendNumber(aText, static_cast<sal_uInt16>(rData.meItem));
            appendAscii(aText, ")");
            break;
        }
        case Kind::Error:
            appendAscii(aText, " error=");
            appendNumber(aText, std::get<setoken::ErrorCode>(rToken.maPayload));
            break;
        case Kind::Jump:
        {
            const auto& rData = std::get<setoken::JumpData>(rToken.maPayload);
            appendAscii(aText, " jump(count=");
            appendNumber(aText, rData.maJumps.size());
            appendAscii(aText, ",values=[");
            for (std::size_t nIndex = 0; nIndex < rData.maJumps.size(); ++nIndex)
            {
                if (nIndex)
                    appendAscii(aText, ",");
                appendNumber(aText, rData.maJumps[nIndex]);
            }
            appendAscii(aText, "])");
            break;
        }
        case Kind::Whitespace:
        {
            const auto& rData = std::get<setoken::WhitespaceData>(rToken.maPayload);
            appendAscii(aText, " ws(count=");
            appendNumber(aText, rData.mnCount);
            appendAscii(aText, ",char=");
            appendNumber(aText, rData.mcChar);
            appendAscii(aText, ")");
            break;
        }
    }

    return aText;
}

inline api::String tokenToDiagnosticString(const setoken::Token& rToken)
{
    api::String aText(setoken::kindName(rToken.meKind));
    appendAscii(aText, "(op=");
    appendNumber(aText, rToken.mnOpCode);
    appendAscii(aText, ")");
    aText += tokenPayloadToDiagnosticString(rToken);
    return aText;
}

inline api::String compiledFormulaToDiagnosticString(const setoken::CompiledFormula& rFormula)
{
    api::String aText;
    appendAscii(aText, "codeError=");
    appendNumber(aText, rFormula.mnCodeError);
    appendAscii(aText, " tokens=[");
    for (std::size_t i = 0; i < rFormula.maTokens.size(); ++i)
    {
        if (i)
            appendAscii(aText, "; ");
        aText += tokenToDiagnosticString(rFormula.maTokens[i]);
    }
    appendAscii(aText, "]");
    if (rFormula.moXmlFormulaSource)
    {
        appendAscii(aText, " xml=");
        appendQuoted(aText, rFormula.moXmlFormulaSource->maFormula);
    }
    return aText;
}

} // namespace spreadsheetengine::detail::tokenstringifier

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
