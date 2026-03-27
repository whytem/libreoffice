/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <formula/errorcodes.hxx>
#include <formula/paramclass.hxx>
#include <formula/token.hxx>
#include <formula/tokenarray.hxx>
#include <scmatrix.hxx>
#include <token.hxx>
#include <tokenarray.hxx>
#include <types.hxx>

#include <memory>
#include <optional>
#include <utility>

#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/TokenModel.hxx>

namespace spreadsheetengine::compat::libreoffice
{

namespace setoken = spreadsheetengine::detail::token;

inline setoken::VectorState toEngineVectorState(ScFormulaVectorState eState)
{
    using setoken::VectorState;

    switch (eState)
    {
        case FormulaVectorDisabled:
            return VectorState::Disabled;
        case FormulaVectorDisabledNotInSubSet:
            return VectorState::DisabledNotInSubSet;
        case FormulaVectorDisabledByOpCode:
            return VectorState::DisabledByOpCode;
        case FormulaVectorDisabledByStackVariable:
            return VectorState::DisabledByStackVariable;
        case FormulaVectorEnabled:
            return VectorState::Enabled;
        case FormulaVectorCheckReference:
            return VectorState::CheckReference;
        case FormulaVectorUnknown:
        default:
            return VectorState::Unknown;
    }
}

inline setoken::TableRefItem toEngineTableRefItem(ScTableRefToken::Item eItem)
{
    using setoken::TableRefItem;

    if (eItem == ScTableRefToken::TABLE)
        return TableRefItem::Table;

    TableRefItem eResult = TableRefItem::None;
    const auto nItem = static_cast<sal_uInt16>(eItem);
    if (nItem & ScTableRefToken::ALL)
        eResult |= TableRefItem::All;
    if (nItem & ScTableRefToken::HEADERS)
        eResult |= TableRefItem::Headers;
    if (nItem & ScTableRefToken::DATA)
        eResult |= TableRefItem::Data;
    if (nItem & ScTableRefToken::TOTALS)
        eResult |= TableRefItem::Totals;
    if (nItem & ScTableRefToken::THIS_ROW)
        eResult |= TableRefItem::ThisRow;
    return eResult;
}

inline ScTableRefToken::Item toLibreOfficeTableRefItem(setoken::TableRefItem eItem)
{
    if (eItem == setoken::TableRefItem::None || eItem == setoken::TableRefItem::Table)
        return ScTableRefToken::TABLE;

    sal_uInt16 nItem = 0;
    const auto nEngineItem = static_cast<sal_uInt16>(eItem);
    if (nEngineItem & static_cast<sal_uInt16>(setoken::TableRefItem::All))
        nItem |= ScTableRefToken::ALL;
    if (nEngineItem & static_cast<sal_uInt16>(setoken::TableRefItem::Headers))
        nItem |= ScTableRefToken::HEADERS;
    if (nEngineItem & static_cast<sal_uInt16>(setoken::TableRefItem::Data))
        nItem |= ScTableRefToken::DATA;
    if (nEngineItem & static_cast<sal_uInt16>(setoken::TableRefItem::Totals))
        nItem |= ScTableRefToken::TOTALS;
    if (nEngineItem & static_cast<sal_uInt16>(setoken::TableRefItem::ThisRow))
        nItem |= ScTableRefToken::THIS_ROW;
    return static_cast<ScTableRefToken::Item>(nItem);
}

inline setoken::ParamClassValue toEngineParamClass(formula::ParamClass eClass)
{
    return static_cast<setoken::ParamClassValue>(eClass);
}

inline formula::ParamClass toLibreOfficeParamClass(setoken::ParamClassValue nClass)
{
    return static_cast<formula::ParamClass>(nClass);
}

struct TokenImportStatus
{
    setoken::CompiledFormula maFormula;
    sal_uInt16 mnFailureIndex = 0;
    OUString maFailureMessage;

    explicit operator bool() const { return maFailureMessage.isEmpty(); }
};

struct TokenSequenceImportStatus
{
    std::vector<setoken::Token> maTokens;
    sal_uInt16 mnFailureIndex = 0;
    OUString maFailureMessage;

    explicit operator bool() const { return maFailureMessage.isEmpty(); }
};

struct TokenExportStatus
{
    std::unique_ptr<ScTokenArray> mxTokenArray;
    sal_uInt16 mnFailureIndex = 0;
    OUString maFailureMessage;

    explicit operator bool() const { return mxTokenArray && maFailureMessage.isEmpty(); }
};

namespace detail
{

inline void setFailure(TokenImportStatus& rStatus, sal_uInt16 nIndex, OUString aMessage)
{
    rStatus.mnFailureIndex = nIndex;
    rStatus.maFailureMessage = std::move(aMessage);
}

inline void setFailure(TokenSequenceImportStatus& rStatus, sal_uInt16 nIndex, OUString aMessage)
{
    rStatus.mnFailureIndex = nIndex;
    rStatus.maFailureMessage = std::move(aMessage);
}

inline void setFailure(TokenExportStatus& rStatus, sal_uInt16 nIndex, OUString aMessage)
{
    rStatus.mnFailureIndex = nIndex;
    rStatus.maFailureMessage = std::move(aMessage);
    rStatus.mxTokenArray.reset();
}

inline bool isPureXmlPlaceholderArray(
    const ScTokenArray& rArray, std::optional<setoken::XmlFormulaSource>& roXmlSource)
{
    const sal_uInt16 nLength = rArray.GetLen();
    if (!nLength || nLength > 2)
        return false;

    for (formula::FormulaToken* pToken : rArray.Tokens())
    {
        if (!pToken || pToken->GetType() != formula::svString || pToken->GetOpCode() != ocStringXML)
            return false;
    }

    formula::FormulaToken** pTokens = rArray.GetArray();
    roXmlSource = setoken::XmlFormulaSource {
        toApiString(pTokens[0]->GetString().getString()),
        nLength == 2 ? toApiString(pTokens[1]->GetString().getString()) : api::String(),
    };
    return true;
}

inline bool importMatrix(
    const ScMatrix& rMatrix, setoken::MatrixData& rData, sal_uInt16 nTokenIndex,
    TokenImportStatus& rStatus)
{
    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rMatrix.GetDimensions(nColumns, nRows);

    rData.mnColumns = static_cast<sal_Int32>(nColumns);
    rData.mnRows = static_cast<sal_Int32>(nRows);
    rData.maValues.reserve(nColumns * nRows);

    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
        {
            if (rMatrix.IsEmpty(nColumn, nRow) || rMatrix.IsEmptyCell(nColumn, nRow)
                || rMatrix.IsEmptyResult(nColumn, nRow) || rMatrix.IsEmptyPath(nColumn, nRow))
            {
                setFailure(rStatus, nTokenIndex, u"matrix token contains unsupported empty element"_ustr);
                return false;
            }

            if (rMatrix.IsValue(nColumn, nRow))
            {
                const FormulaError eError = rMatrix.GetError(nColumn, nRow);
                if (eError != FormulaError::NONE)
                    rData.maValues.emplace_back(static_cast<setoken::ErrorCode>(eError));
                else
                    rData.maValues.emplace_back(rMatrix.GetDouble(nColumn, nRow));
                continue;
            }

            if (rMatrix.IsStringOrEmpty(nColumn, nRow))
            {
                rData.maValues.emplace_back(
                    toApiString(rMatrix.GetString(nColumn, nRow).getString()));
                continue;
            }

            setFailure(
                rStatus, nTokenIndex, u"matrix token contains unsupported element type"_ustr);
            return false;
        }
    }

    return true;
}

inline formula::ParamClass extractInForceArray(const formula::FormulaToken& rToken)
{
    return rToken.GetInForceArray();
}

inline bool exportMatrix(
    const setoken::MatrixData& rData, ScTokenArray& rArray, sal_uInt16 nTokenIndex,
    TokenExportStatus& rStatus)
{
    if (rData.mnColumns < 0 || rData.mnRows < 0)
    {
        setFailure(rStatus, nTokenIndex, u"matrix token has invalid dimensions"_ustr);
        return false;
    }

    const std::size_t nExpected = static_cast<std::size_t>(rData.mnColumns)
                                  * static_cast<std::size_t>(rData.mnRows);
    if (rData.maValues.size() != nExpected)
    {
        setFailure(rStatus, nTokenIndex, u"matrix token value count does not match dimensions"_ustr);
        return false;
    }

    ScMatrixRef xMatrix(
        new ScMatrix(static_cast<SCSIZE>(rData.mnColumns), static_cast<SCSIZE>(rData.mnRows)));

    std::size_t nIndex = 0;
    for (sal_Int32 nRow = 0; nRow < rData.mnRows; ++nRow)
    {
        for (sal_Int32 nColumn = 0; nColumn < rData.mnColumns; ++nColumn)
        {
            const auto& rScalar = rData.maValues[nIndex++];
            std::visit(
                [&](const auto& rValue) {
                    using Value = std::decay_t<decltype(rValue)>;
                    if constexpr (std::is_same_v<Value, double>)
                        xMatrix->PutDouble(rValue, nColumn, nRow);
                    else if constexpr (std::is_same_v<Value, api::String>)
                        xMatrix->PutString(
                            svl::SharedString(toLibreOfficeString(rValue)), nColumn, nRow);
                    else
                        xMatrix->PutError(static_cast<FormulaError>(rValue), nColumn, nRow);
                },
                rScalar);
        }
    }

    rArray.AddMatrix(xMatrix);
    return true;
}

inline setoken::Token importSingleToken(
    const formula::FormulaToken& rToken, sal_uInt16 nTokenIndex, TokenImportStatus& rStatus)
{
    using namespace spreadsheetengine::detail::token;

    const OpCodeValue nOpCode = static_cast<OpCodeValue>(rToken.GetOpCode());
    switch (rToken.GetType())
    {
        case formula::svByte:
            if (rToken.GetOpCode() == ocWhitespace)
            {
                return { Kind::Whitespace, nOpCode,
                    WhitespaceData { rToken.GetByte(), rToken.GetChar() } };
            }
            return { Kind::Byte, nOpCode,
                ByteData { rToken.GetByte(), toEngineParamClass(extractInForceArray(rToken)) } };
        case formula::svDouble:
            return { Kind::Value, nOpCode, rToken.GetDouble() };
        case formula::svString:
            return { Kind::String, nOpCode,
                StringData { toApiString(rToken.GetString().getString()),
                    toApiString(rToken.GetString().getString()) } };
        case formula::svStringName:
            return { Kind::StringName, nOpCode,
                StringData { toApiString(rToken.GetString().getString()),
                    toApiString(rToken.GetString().getString()) } };
        case formula::svSingleRef:
        {
            const auto* pReference = rToken.GetSingleRef();
            if (!pReference)
            {
                setFailure(rStatus, nTokenIndex, u"single-ref token is missing reference data"_ustr);
                return {};
            }

            return { rToken.GetOpCode() == ocColRowName ? Kind::ColRowName : Kind::SingleRef,
                nOpCode, pReference->toApiSingleRefData() };
        }
        case formula::svDoubleRef:
        {
            const auto* pReference = rToken.GetDoubleRef();
            if (!pReference)
            {
                setFailure(rStatus, nTokenIndex, u"double-ref token is missing reference data"_ustr);
                return {};
            }

            return { Kind::DoubleRef, nOpCode, pReference->toApiComplexRefData() };
        }
        case formula::svMatrix:
        {
            const auto* pMatrix = rToken.GetMatrix();
            if (!pMatrix)
            {
                setFailure(rStatus, nTokenIndex, u"matrix token is missing matrix payload"_ustr);
                return {};
            }

            MatrixData aData;
            if (!importMatrix(*pMatrix, aData, nTokenIndex, rStatus))
                return {};
            return { Kind::Matrix, nOpCode, std::move(aData) };
        }
        case formula::svIndex:
            switch (rToken.GetOpCode())
            {
                case ocName:
                    return { Kind::RangeName, nOpCode,
                        NameData { rToken.GetSheet(), rToken.GetIndex() } };
                case ocDBArea:
                    return { Kind::DatabaseRange, nOpCode,
                        DatabaseRangeData { rToken.GetIndex() } };
                case ocTableRef:
                {
                    const auto* pTableRef = dynamic_cast<const ScTableRefToken*>(&rToken);
                    if (!pTableRef)
                    {
                        setFailure(
                            rStatus, nTokenIndex, u"table-ref token has unexpected runtime type"_ustr);
                        return {};
                    }

                    return { Kind::TableRef, nOpCode,
                        TableRefData {
                            pTableRef->GetIndex(), toEngineTableRefItem(pTableRef->GetItem())
                        } };
                }
                default:
                    setFailure(
                        rStatus, nTokenIndex, u"svIndex token uses unsupported opcode"_ustr);
                    return {};
            }
        case formula::svJump:
        {
            short* pJump = rToken.GetJump();
            if (!pJump)
            {
                setFailure(rStatus, nTokenIndex, u"jump token is missing jump payload"_ustr);
                return {};
            }

            JumpData aData;
            aData.maJumps.assign(pJump, pJump + pJump[0] + 1);
            aData.mnInForceArray = toEngineParamClass(extractInForceArray(rToken));
            return { Kind::Jump, nOpCode, std::move(aData) };
        }
        case formula::svExternalSingleRef:
        {
            const auto* pReference = rToken.GetSingleRef();
            if (!pReference)
            {
                setFailure(
                    rStatus, nTokenIndex, u"external single-ref token is missing reference data"_ustr);
                return {};
            }

            return { Kind::ExternalSingleRef, nOpCode,
                ExternalSingleRefData { rToken.GetIndex(),
                    toApiString(rToken.GetString().getString()), pReference->toApiSingleRefData() } };
        }
        case formula::svExternalDoubleRef:
        {
            const auto* pReference = rToken.GetDoubleRef();
            if (!pReference)
            {
                setFailure(
                    rStatus, nTokenIndex, u"external double-ref token is missing reference data"_ustr);
                return {};
            }

            return { Kind::ExternalDoubleRef, nOpCode,
                ExternalDoubleRefData { rToken.GetIndex(),
                    toApiString(rToken.GetString().getString()), pReference->toApiComplexRefData() } };
        }
        case formula::svExternalName:
            return { Kind::ExternalName, nOpCode,
                ExternalNameData { rToken.GetIndex(), toApiString(rToken.GetString().getString()) } };
        case formula::svError:
            return { Kind::Error, nOpCode,
                static_cast<ErrorCode>(rToken.GetError()) };
        case formula::svMissing:
            return { Kind::Missing, nOpCode, {} };
        case formula::svSep:
        case formula::svUnknown:
            return { Kind::PlainOpcode, nOpCode, {} };
        case formula::svExternal:
            setFailure(rStatus, nTokenIndex, u"plain external tokens are not bridged yet"_ustr);
            return {};
        case formula::svFAP:
        case formula::svJumpMatrix:
        case formula::svRefList:
        case formula::svEmptyCell:
        case formula::svMatrixCell:
        case formula::svHybridCell:
        case formula::svSingleVectorRef:
        case formula::svDoubleVectorRef:
            setFailure(rStatus, nTokenIndex, u"interpreter-only token kind is unsupported"_ustr);
            return {};
    }

    setFailure(rStatus, nTokenIndex, u"unknown token kind"_ustr);
    return {};
}

inline bool exportSingleToken(
    const setoken::Token& rToken, ScTokenArray& rArray, sal_uInt16 nTokenIndex,
    TokenExportStatus& rStatus)
{
    using namespace spreadsheetengine::detail::token;

    const OpCode eOpCode = static_cast<OpCode>(rToken.mnOpCode);
    switch (rToken.meKind)
    {
        case Kind::PlainOpcode:
            rArray.AddOpCode(eOpCode);
            return true;
        case Kind::Missing:
            rArray.AddToken(formula::FormulaMissingToken());
            return true;
        case Kind::Byte:
        {
            const auto& rData = std::get<ByteData>(rToken.maPayload);
            rArray.AddToken(
                formula::FormulaByteToken(eOpCode, rData.mnByte, toLibreOfficeParamClass(rData.mnInForceArray)));
            return true;
        }
        case Kind::Value:
            rArray.AddDouble(std::get<double>(rToken.maPayload));
            return true;
        case Kind::String:
        {
            const auto& rData = std::get<StringData>(rToken.maPayload);
            const OUString aString = toLibreOfficeString(rData.maText);
            if (eOpCode == ocPush)
                rArray.AddString(svl::SharedString(aString));
            else
                rArray.AddToken(formula::FormulaStringOpToken(eOpCode, svl::SharedString(aString)));
            return true;
        }
        case Kind::StringName:
        {
            if (eOpCode != ocPush)
            {
                setFailure(
                    rStatus, nTokenIndex, u"string-name token uses unsupported opcode"_ustr);
                return false;
            }
            const auto& rData = std::get<StringData>(rToken.maPayload);
            rArray.AddStringName(toLibreOfficeString(rData.maText));
            return true;
        }
        case Kind::SingleRef:
        {
            ScSingleRefData aReference;
            aReference.assignFromApiSingleRefData(
                std::get<api::refdata::SingleRefData>(rToken.maPayload));
            rArray.AddSingleReference(aReference);
            return true;
        }
        case Kind::DoubleRef:
        {
            ScComplexRefData aReference;
            aReference.assignFromApiComplexRefData(
                std::get<api::refdata::ComplexRefData>(rToken.maPayload));
            rArray.AddDoubleReference(aReference);
            return true;
        }
        case Kind::RangeName:
        {
            const auto& rData = std::get<NameData>(rToken.maPayload);
            rArray.AddRangeName(rData.mnIndex, rData.mnSheet);
            return true;
        }
        case Kind::DatabaseRange:
            rArray.AddDBRange(std::get<DatabaseRangeData>(rToken.maPayload).mnIndex);
            return true;
        case Kind::ExternalSingleRef:
        {
            const auto& rData = std::get<ExternalSingleRefData>(rToken.maPayload);
            ScSingleRefData aReference;
            aReference.assignFromApiSingleRefData(rData.maReference);
            rArray.AddExternalSingleReference(
                rData.mnFileId, svl::SharedString(toLibreOfficeString(rData.maTabName)), aReference);
            return true;
        }
        case Kind::ExternalDoubleRef:
        {
            const auto& rData = std::get<ExternalDoubleRefData>(rToken.maPayload);
            ScComplexRefData aReference;
            aReference.assignFromApiComplexRefData(rData.maReference);
            rArray.AddExternalDoubleReference(
                rData.mnFileId, svl::SharedString(toLibreOfficeString(rData.maTabName)), aReference);
            return true;
        }
        case Kind::ExternalName:
        {
            const auto& rData = std::get<ExternalNameData>(rToken.maPayload);
            rArray.AddExternalName(
                rData.mnFileId, svl::SharedString(toLibreOfficeString(rData.maName)));
            return true;
        }
        case Kind::Matrix:
            return exportMatrix(std::get<MatrixData>(rToken.maPayload), rArray, nTokenIndex, rStatus);
        case Kind::ColRowName:
        {
            ScSingleRefData aReference;
            aReference.assignFromApiSingleRefData(
                std::get<api::refdata::SingleRefData>(rToken.maPayload));
            rArray.AddColRowName(aReference);
            return true;
        }
        case Kind::TableRef:
        {
            const auto& rData = std::get<TableRefData>(rToken.maPayload);
            rArray.AddToken(ScTableRefToken(rData.mnIndex, toLibreOfficeTableRefItem(rData.meItem)));
            return true;
        }
        case Kind::Error:
            rArray.AddToken(formula::FormulaErrorToken(
                static_cast<FormulaError>(std::get<ErrorCode>(rToken.maPayload))));
            return true;
        case Kind::Jump:
        {
            const auto& rData = std::get<JumpData>(rToken.maPayload);
            if (rData.maJumps.empty())
            {
                setFailure(rStatus, nTokenIndex, u"jump token is missing jump payload"_ustr);
                return false;
            }

            formula::FormulaJumpToken aJumpToken(eOpCode, rData.maJumps.data());
            aJumpToken.SetInForceArray(toLibreOfficeParamClass(rData.mnInForceArray));
            rArray.AddToken(aJumpToken);
            return true;
        }
        case Kind::Whitespace:
        {
            const auto& rData = std::get<WhitespaceData>(rToken.maPayload);
            rArray.AddToken(formula::FormulaSpaceToken(rData.mnCount, rData.mcChar));
            return true;
        }
    }

    setFailure(rStatus, nTokenIndex, u"unknown engine token kind"_ustr);
    return false;
}

inline void applyFormulaMetadata(ScTokenArray& rArray, const setoken::CompiledFormula& rFormula)
{
    rArray.SetCodeError(static_cast<FormulaError>(rFormula.mnCodeError));
    rArray.SetHyperLink(rFormula.mbHyperLink);
    rArray.SetFromRangeName(rFormula.mbFromRangeName);
    rArray.SetShareable(rFormula.mbShareable);

    constexpr sal_uInt8 nExclusiveMask = static_cast<sal_uInt8>(ScRecalcMode::EMask);
    const sal_uInt8 nRecalcModeBits = rFormula.mnRecalcModeBits;
    const sal_uInt8 nExclusiveBits = nRecalcModeBits & nExclusiveMask;
    const sal_uInt8 nCombinedBits = nRecalcModeBits & ~nExclusiveMask;

    rArray.ClearRecalcMode();
    rArray.SetMaskedRecalcMode(static_cast<ScRecalcMode>(
        nExclusiveBits ? nExclusiveBits : static_cast<sal_uInt8>(ScRecalcMode::NORMAL)));
    rArray.SetCombinedBitsRecalcMode(static_cast<ScRecalcMode>(nCombinedBits));
}

inline bool matricesEqualForBridge(const ScMatrix* pLeft, const ScMatrix* pRight)
{
    if (!pLeft || !pRight)
        return pLeft == pRight;

    SCSIZE nLeftColumns = 0;
    SCSIZE nLeftRows = 0;
    SCSIZE nRightColumns = 0;
    SCSIZE nRightRows = 0;
    pLeft->GetDimensions(nLeftColumns, nLeftRows);
    pRight->GetDimensions(nRightColumns, nRightRows);
    if (nLeftColumns != nRightColumns || nLeftRows != nRightRows)
        return false;

    for (SCSIZE nRow = 0; nRow < nLeftRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nLeftColumns; ++nColumn)
        {
            if (pLeft->IsValue(nColumn, nRow) != pRight->IsValue(nColumn, nRow))
                return false;
            if (pLeft->IsStringOrEmpty(nColumn, nRow) != pRight->IsStringOrEmpty(nColumn, nRow))
                return false;
            if (pLeft->IsEmpty(nColumn, nRow) != pRight->IsEmpty(nColumn, nRow))
                return false;
            if (pLeft->IsEmptyCell(nColumn, nRow) != pRight->IsEmptyCell(nColumn, nRow))
                return false;
            if (pLeft->IsEmptyResult(nColumn, nRow) != pRight->IsEmptyResult(nColumn, nRow))
                return false;
            if (pLeft->IsEmptyPath(nColumn, nRow) != pRight->IsEmptyPath(nColumn, nRow))
                return false;

            if (pLeft->IsValue(nColumn, nRow))
            {
                if (pLeft->GetError(nColumn, nRow) != pRight->GetError(nColumn, nRow))
                    return false;
                if (pLeft->GetError(nColumn, nRow) == FormulaError::NONE
                    && pLeft->GetDouble(nColumn, nRow) != pRight->GetDouble(nColumn, nRow))
                {
                    return false;
                }
                continue;
            }

            if (pLeft->GetString(nColumn, nRow) != pRight->GetString(nColumn, nRow))
                return false;
        }
    }

    return true;
}

inline bool tokensEqualForBridge(const formula::FormulaToken& rLeft, const formula::FormulaToken& rRight)
{
    if (&rLeft == &rRight || rLeft == rRight)
        return true;

    if (rLeft.GetType() == formula::svMatrix && rRight.GetType() == formula::svMatrix
        && rLeft.GetOpCode() == rRight.GetOpCode())
    {
        return matricesEqualForBridge(rLeft.GetMatrix(), rRight.GetMatrix());
    }

    return false;
}

} // namespace detail

inline TokenSequenceImportStatus importTokenSequence(formula::FormulaTokenArrayStandardRange aTokens)
{
    TokenSequenceImportStatus aStatus;
    formula::FormulaToken** pBegin = aTokens.begin();
    formula::FormulaToken** pEnd = aTokens.end();
    if (pBegin && pEnd)
        aStatus.maTokens.reserve(static_cast<std::size_t>(pEnd - pBegin));
    sal_uInt16 nTokenIndex = 0;
    for (formula::FormulaToken* pToken : aTokens)
    {
        if (!pToken)
        {
            detail::setFailure(aStatus, nTokenIndex, u"token array contains null token"_ustr);
            return aStatus;
        }

        TokenImportStatus aTokenStatus;
        setoken::Token aToken = detail::importSingleToken(*pToken, nTokenIndex, aTokenStatus);
        if (!aTokenStatus)
        {
            detail::setFailure(aStatus, aTokenStatus.mnFailureIndex, aTokenStatus.maFailureMessage);
            return aStatus;
        }

        aStatus.maTokens.push_back(std::move(aToken));
        ++nTokenIndex;
    }

    return aStatus;
}

inline std::optional<bool> tokenSequencesEqualCanonical(
    formula::FormulaTokenArrayStandardRange aLeft,
    formula::FormulaTokenArrayStandardRange aRight)
{
    const auto aImportedLeft = importTokenSequence(aLeft);
    if (!aImportedLeft)
        return std::nullopt;

    const auto aImportedRight = importTokenSequence(aRight);
    if (!aImportedRight)
        return std::nullopt;

    return aImportedLeft.maTokens == aImportedRight.maTokens;
}

inline TokenImportStatus importCompiledFormula(const ScTokenArray& rTokenArray)
{
    TokenImportStatus aStatus;
    aStatus.maFormula.mnCodeError = static_cast<setoken::ErrorCode>(rTokenArray.GetCodeError());
    aStatus.maFormula.mnRecalcModeBits = static_cast<sal_uInt8>(rTokenArray.GetRecalcMode());
    aStatus.maFormula.mbHyperLink = rTokenArray.IsHyperLink();
    aStatus.maFormula.mbFromRangeName = rTokenArray.IsFromRangeName();
    aStatus.maFormula.mbShareable = rTokenArray.IsShareable();
    aStatus.maFormula.meVectorState = toEngineVectorState(rTokenArray.GetVectorState());
    aStatus.maFormula.mbOpenCLEnabled = rTokenArray.IsEnabledForOpenCL();
    aStatus.maFormula.mbThreadingEnabled = rTokenArray.IsEnabledForThreading();

    std::optional<setoken::XmlFormulaSource> oXmlSource;
    if (detail::isPureXmlPlaceholderArray(rTokenArray, oXmlSource))
    {
        aStatus.maFormula.moXmlFormulaSource = std::move(oXmlSource);
        return aStatus;
    }

    const auto aTokens = importTokenSequence(rTokenArray.Tokens());
    if (!aTokens)
    {
        detail::setFailure(aStatus, aTokens.mnFailureIndex, aTokens.maFailureMessage);
        return aStatus;
    }
    aStatus.maFormula.maTokens = aTokens.maTokens;

    return aStatus;
}

inline TokenExportStatus exportCompiledFormula(
    const setoken::CompiledFormula& rFormula, const ScDocument& rDocument)
{
    TokenExportStatus aStatus;
    aStatus.mxTokenArray = std::make_unique<ScTokenArray>(rDocument);

    if (rFormula.moXmlFormulaSource)
    {
        if (!rFormula.maTokens.empty())
        {
            detail::setFailure(
                aStatus, 0, u"compiled formula mixes XML placeholder source with token stream"_ustr);
            return aStatus;
        }

        aStatus.mxTokenArray->AssignXMLString(
            toLibreOfficeString(rFormula.moXmlFormulaSource->maFormula),
            toLibreOfficeString(rFormula.moXmlFormulaSource->maNamespace));
        detail::applyFormulaMetadata(*aStatus.mxTokenArray, rFormula);
        return aStatus;
    }

    sal_uInt16 nTokenIndex = 0;
    for (const auto& rToken : rFormula.maTokens)
    {
        if (!detail::exportSingleToken(rToken, *aStatus.mxTokenArray, nTokenIndex, aStatus))
            return aStatus;
        ++nTokenIndex;
    }

    detail::applyFormulaMetadata(*aStatus.mxTokenArray, rFormula);
    return aStatus;
}

inline bool tokenArraysEqualForBridge(const ScTokenArray& rLeft, const ScTokenArray& rRight)
{
    if (rLeft.GetLen() != rRight.GetLen())
        return false;

    for (sal_uInt16 nIndex = 0; nIndex < rLeft.GetLen(); ++nIndex)
    {
        formula::FormulaToken* pLeft = rLeft.TokenAt(nIndex);
        formula::FormulaToken* pRight = rRight.TokenAt(nIndex);
        if (!pLeft || !pRight)
            return pLeft == pRight;
        if (!detail::tokensEqualForBridge(*pLeft, *pRight))
            return false;
    }

    return true;
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
