/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIr.hxx>

namespace spreadsheetengine::detail::substrate
{

struct ExecutionIrLoweringContext
{
    ShadowCellId maId;
    std::optional<ShadowFormulaGroupId> moFormulaGroup;
    ExecutionIrFormulaSource maSource;
    bool mbInFormulaTree = false;
    bool mbInFormulaTrack = false;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrLoweringContext& rOther) const
        = default;
};

struct ExecutionIrLoweringResult
{
    ExecutionIrFormulaRecord maFormula;
    sal_Int32 mnFailureIndex = -1;
    api::String maFailureMessage;

    explicit operator bool() const { return maFailureMessage.empty(); }
};

namespace irdetail
{

[[nodiscard]] inline ExecutionIrInstructionKind lowerInstructionKind(const token::Token& rToken)
{
    switch (rToken.meKind)
    {
        case token::Kind::PlainOpcode:
            switch (rToken.mnOpCode)
            {
                case compiler::detail::kLoweredOpFunctionCall:
                    return ExecutionIrInstructionKind::FunctionCallMarker;
                case compiler::detail::kLoweredOpUnaryPlus:
                    return ExecutionIrInstructionKind::UnaryPlusMarker;
                case compiler::detail::kLoweredOpRangeConstructor:
                    return ExecutionIrInstructionKind::RangeConstructorMarker;
                case compiler::detail::kLoweredOpReferenceList:
                    return ExecutionIrInstructionKind::ReferenceListMarker;
                default:
                    return ExecutionIrInstructionKind::PlainOpcode;
            }
        case token::Kind::Missing:
            return ExecutionIrInstructionKind::MissingArgument;
        case token::Kind::Byte:
            return ExecutionIrInstructionKind::ByteLiteral;
        case token::Kind::Value:
            return ExecutionIrInstructionKind::NumberLiteral;
        case token::Kind::String:
            return ExecutionIrInstructionKind::StringLiteral;
        case token::Kind::StringName:
            return ExecutionIrInstructionKind::StringNameLiteral;
        case token::Kind::SingleRef:
            return ExecutionIrInstructionKind::SingleReference;
        case token::Kind::DoubleRef:
            return ExecutionIrInstructionKind::RangeReference;
        case token::Kind::RangeName:
            return ExecutionIrInstructionKind::RangeNameReference;
        case token::Kind::DatabaseRange:
            return ExecutionIrInstructionKind::DatabaseRangeReference;
        case token::Kind::ExternalSingleRef:
            return ExecutionIrInstructionKind::ExternalSingleReference;
        case token::Kind::ExternalDoubleRef:
            return ExecutionIrInstructionKind::ExternalRangeReference;
        case token::Kind::ExternalName:
            return ExecutionIrInstructionKind::ExternalNameReference;
        case token::Kind::Matrix:
            return ExecutionIrInstructionKind::MatrixLiteral;
        case token::Kind::ColRowName:
            return ExecutionIrInstructionKind::ColumnRowNameReference;
        case token::Kind::TableRef:
            return ExecutionIrInstructionKind::TableReference;
        case token::Kind::Error:
            return ExecutionIrInstructionKind::ErrorLiteral;
        case token::Kind::Jump:
            return ExecutionIrInstructionKind::JumpTable;
        case token::Kind::Whitespace:
            return ExecutionIrInstructionKind::Whitespace;
    }
    return ExecutionIrInstructionKind::PlainOpcode;
}

template <typename T>
[[nodiscard]] inline const T* payloadIf(const token::Token& rToken)
{
    return std::get_if<T>(&rToken.maPayload);
}

[[nodiscard]] inline ExecutionIrPayload lowerInstructionPayload(
    const token::Token& rToken, api::String& rFailureMessage)
{
    switch (rToken.meKind)
    {
        case token::Kind::PlainOpcode:
        case token::Kind::Missing:
            return {};
        case token::Kind::Byte:
            if (const auto* pData = payloadIf<token::ByteData>(rToken))
                return ExecutionIrByteData { pData->mnByte, pData->mnInForceArray };
            break;
        case token::Kind::Value:
            if (const auto* pValue = payloadIf<double>(rToken))
                return *pValue;
            break;
        case token::Kind::String:
        case token::Kind::StringName:
            if (const auto* pData = payloadIf<token::StringData>(rToken))
                return ExecutionIrStringData { pData->maText, pData->maCaseFolded };
            break;
        case token::Kind::SingleRef:
        case token::Kind::ColRowName:
            if (const auto* pData = payloadIf<api::refdata::SingleRefData>(rToken))
                return *pData;
            break;
        case token::Kind::DoubleRef:
            if (const auto* pData = payloadIf<api::refdata::ComplexRefData>(rToken))
                return *pData;
            break;
        case token::Kind::RangeName:
            if (const auto* pData = payloadIf<token::NameData>(rToken))
                return ExecutionIrNameData { pData->mnSheet, pData->mnIndex };
            break;
        case token::Kind::DatabaseRange:
            if (const auto* pData = payloadIf<token::DatabaseRangeData>(rToken))
                return ExecutionIrDatabaseRangeData { pData->mnIndex };
            break;
        case token::Kind::ExternalSingleRef:
            if (const auto* pData = payloadIf<token::ExternalSingleRefData>(rToken))
            {
                return ExecutionIrExternalSingleRefData {
                    pData->mnFileId, pData->maTabName, pData->maReference
                };
            }
            break;
        case token::Kind::ExternalDoubleRef:
            if (const auto* pData = payloadIf<token::ExternalDoubleRefData>(rToken))
            {
                return ExecutionIrExternalDoubleRefData {
                    pData->mnFileId, pData->maTabName, pData->maReference
                };
            }
            break;
        case token::Kind::ExternalName:
            if (const auto* pData = payloadIf<token::ExternalNameData>(rToken))
                return ExecutionIrExternalNameData { pData->mnFileId, pData->maName };
            break;
        case token::Kind::Matrix:
            if (const auto* pData = payloadIf<token::MatrixData>(rToken))
                return ExecutionIrMatrixData { pData->mnColumns, pData->mnRows, pData->maValues };
            break;
        case token::Kind::TableRef:
            if (const auto* pData = payloadIf<token::TableRefData>(rToken))
                return ExecutionIrTableRefData { pData->mnIndex, pData->meItem };
            break;
        case token::Kind::Error:
            if (const auto* pData = payloadIf<token::ErrorCode>(rToken))
                return *pData;
            break;
        case token::Kind::Jump:
            if (const auto* pData = payloadIf<token::JumpData>(rToken))
                return ExecutionIrJumpData { pData->maJumps, pData->mnInForceArray };
            break;
        case token::Kind::Whitespace:
            if (const auto* pData = payloadIf<token::WhitespaceData>(rToken))
                return ExecutionIrWhitespaceData { pData->mnCount, pData->mcChar };
            break;
    }

    rFailureMessage = u"token payload did not match token kind during IR lowering";
    return {};
}

[[nodiscard]] inline ExecutionIrFormulaSource makeFormulaSource(
    const token::CompiledFormula& rFormula, const ExecutionIrFormulaSource& rFallback)
{
    if (!rFormula.moXmlFormulaSource)
        return rFallback;

    return { rFormula.moXmlFormulaSource->maFormula, rFormula.moXmlFormulaSource->maNamespace };
}

} // namespace irdetail

[[nodiscard]] inline ExecutionIrLoweringResult lowerCompiledFormulaToExecutionIr(
    const token::CompiledFormula& rFormula, const ExecutionIrLoweringContext& rContext)
{
    ExecutionIrLoweringResult aResult;
    aResult.maFormula.maId = rContext.maId;
    aResult.maFormula.moFormulaGroup = rContext.moFormulaGroup;
    aResult.maFormula.maSource = irdetail::makeFormulaSource(rFormula, rContext.maSource);
    aResult.maFormula.mnCodeError = rFormula.mnCodeError;
    aResult.maFormula.mbHyperLink = rFormula.mbHyperLink;
    aResult.maFormula.mbFromRangeName = rFormula.mbFromRangeName;
    aResult.maFormula.mbShareable = rFormula.mbShareable;
    aResult.maFormula.meVectorState = rFormula.meVectorState;
    aResult.maFormula.mbOpenCLEnabled = rFormula.mbOpenCLEnabled;
    aResult.maFormula.mbThreadingEnabled = rFormula.mbThreadingEnabled;
    aResult.maFormula.mbInFormulaTree = rContext.mbInFormulaTree;
    aResult.maFormula.mbInFormulaTrack = rContext.mbInFormulaTrack;
    aResult.maFormula.maInstructions.reserve(rFormula.maTokens.size());

    for (std::size_t nIndex = 0; nIndex < rFormula.maTokens.size(); ++nIndex)
    {
        const auto& rToken = rFormula.maTokens[nIndex];
        ExecutionIrInstruction aInstruction;
        aInstruction.meKind = irdetail::lowerInstructionKind(rToken);
        aInstruction.mnOpCode = rToken.mnOpCode;
        aInstruction.maPayload = irdetail::lowerInstructionPayload(rToken, aResult.maFailureMessage);
        if (!aResult)
        {
            aResult.mnFailureIndex = static_cast<sal_Int32>(nIndex);
            aResult.maFormula.maInstructions.clear();
            return aResult;
        }

        aResult.maFormula.maInstructions.push_back(std::move(aInstruction));
    }

    return aResult;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
