/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <interpre.hxx>
#include <jumpmatrix.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/FormulaInspectionExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/IndirectExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpreterDispatch.hxx>
#include <spreadsheetengine/compat/libreoffice/LetExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/LookupExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/TextServices.hxx>
#include <spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx>
#include <spreadsheetengine/api/StringReference.hxx>
#include <spreadsheetengine/runtime/ForecastEngine.hxx>
#include <spreadsheetengine/runtime/ForecastEtsEngine.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnRandom.hxx>
#include <sfx2/linkmgr.hxx>

namespace spreadsheetengine::compat::libreoffice::interpretercompatdispatch
{

namespace selibreoffice = spreadsheetengine::compat::libreoffice;
namespace seindirectexec = spreadsheetengine::compat::libreoffice::indirectexecution;
namespace seinterpre = spreadsheetengine::compat::libreoffice::interpreterdispatch;
namespace selookup = spreadsheetengine::api::lookup;
namespace selookupexec = spreadsheetengine::compat::libreoffice::lookupexecution;
namespace seletexec = spreadsheetengine::compat::libreoffice::letexecution;
namespace sestringref = spreadsheetengine::api::stringreference;
namespace seref = spreadsheetengine::api::reference;
namespace serefexec = spreadsheetengine::compat::libreoffice::referenceexecution;
namespace semath = spreadsheetengine::core::math;
namespace serpn = spreadsheetengine::core::rpn;
using namespace formula;

#define SEIC rCalc
#define mrDoc SEIC.mrDoc
#define mrContext SEIC.mrContext
#define nGlobalError SEIC.nGlobalError
#define nFuncFmtType SEIC.nFuncFmtType
#define nFuncFmtIndex SEIC.nFuncFmtIndex
#define mnSubTotalFlags SEIC.mnSubTotalFlags
#define cPar SEIC.cPar
#define sp SEIC.sp
#define pStack SEIC.pStack
#define GetByte(...) SEIC.GetByte(__VA_ARGS__)
#define MustHaveParamCount(...) SEIC.MustHaveParamCount(__VA_ARGS__)
#define MustHaveParamCountMin(...) SEIC.MustHaveParamCountMin(__VA_ARGS__)
#define MustHaveParamCountMinWithStackCheck(...) SEIC.MustHaveParamCountMinWithStackCheck(__VA_ARGS__)
#define CalcGetDouble(...) SEIC.GetDouble(__VA_ARGS__)
#define GetDoubleWithDefault(...) SEIC.GetDoubleWithDefault(__VA_ARGS__)
#define GetBool(...) SEIC.GetBool(__VA_ARGS__)
#define GetInt32(...) SEIC.GetInt32(__VA_ARGS__)
#define IsMissing(...) SEIC.IsMissing(__VA_ARGS__)
#define GetStackType(...) SEIC.GetStackType(__VA_ARGS__)
#define PopSingleRef(...) SEIC.PopSingleRef(__VA_ARGS__)
#define PopDoubleRef(...) SEIC.PopDoubleRef(__VA_ARGS__)
#define GetMatrix(...) SEIC.GetMatrix(__VA_ARGS__)
#define PushIllegalArgument(...) SEIC.PushIllegalArgument(__VA_ARGS__)
#define PushIllegalParameter(...) SEIC.PushIllegalParameter(__VA_ARGS__)
#define PushNA(...) SEIC.PushNA(__VA_ARGS__)
#define PushNoValue(...) SEIC.PushNoValue(__VA_ARGS__)
#define PushError(...) SEIC.PushError(__VA_ARGS__)
#define PushDouble(...) SEIC.PushDouble(__VA_ARGS__)
#define PushMatrix(...) SEIC.PushMatrix(__VA_ARGS__)
#define PushTokenRef(...) SEIC.PushTokenRef(__VA_ARGS__)
#define PushWithoutError(...) SEIC.PushWithoutError(__VA_ARGS__)
#define Pop(...) SEIC.Pop(__VA_ARGS__)
#define PopError(...) SEIC.PopError(__VA_ARGS__)
#define PopToken(...) SEIC.PopToken(__VA_ARGS__)
#define SetError(...) SEIC.SetError(__VA_ARGS__)
#define GetCellValue(...) SEIC.GetCellValue(__VA_ARGS__)
#define GetCellErrCode(...) SEIC.GetCellErrCode(__VA_ARGS__)
#define CurFmtToFuncFmt(...) SEIC.CurFmtToFuncFmt(__VA_ARGS__)
#define GetNewMat(...) SEIC.GetNewMat(__VA_ARGS__)
#define GetRefListArrayMaxSize(...) SEIC.GetRefListArrayMaxSize(__VA_ARGS__)
#define SwitchToArrayRefList(...) SEIC.SwitchToArrayRefList(__VA_ARGS__)
#define IterateParameters(...) SEIC.IterateParameters(__VA_ARGS__)
#define GetStVarParams(...) SEIC.GetStVarParams(__VA_ARGS__)
#define CalculateSkew(...) SEIC.CalculateSkew(__VA_ARGS__)
#define GetNumberSequenceArray(...) SEIC.GetNumberSequenceArray(__VA_ARGS__)
#define GetSortArray(...) SEIC.GetSortArray(__VA_ARGS__)
#define GetPercentrank(...) SEIC.GetPercentrank(__VA_ARGS__)
#define CalculatePearsonCovar(...) SEIC.CalculatePearsonCovar(__VA_ARGS__)
#define CalculateTest(...) SEIC.CalculateTest(__VA_ARGS__)
#define CalculateSmallLarge(...) SEIC.CalculateSmallLarge(__VA_ARGS__)
#define GetChiDist(...) SEIC.GetChiDist(__VA_ARGS__)
#define GetFDist(...) SEIC.GetFDist(__VA_ARGS__)
#define GetTDist(...) SEIC.GetTDist(__VA_ARGS__)
#define CalculateTrendGrowth(...) SEIC.CalculateTrendGrowth(__VA_ARGS__)

[[nodiscard]] inline std::optional<serpn::MatrixOperand> matrixRefToMatrixOperand(
    const ScMatrixRef& pMatrix)
{
    if (!pMatrix)
        return std::nullopt;

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    pMatrix->GetDimensions(nColumns, nRows);

    serpn::MatrixOperand aOperand;
    aOperand.maDimensions = { static_cast<spreadsheetengine::api::MatrixSize>(nColumns),
                              static_cast<spreadsheetengine::api::MatrixSize>(nRows) };
    aOperand.meProvenance = serpn::MatrixProvenance::MaterializedReference;
    aOperand.maValues.reserve(static_cast<std::size_t>(nColumns) * nRows);
    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            aOperand.maValues.push_back(selookupexec::detail::toApiCellValue(
                pMatrix->Get(nColumn, nRow)));
    }
    return aOperand;
}

inline void putScalarIntoMatrix(
    const spreadsheetengine::api::CellValue& rValue, const ScMatrixRef& pMatrix,
    SCSIZE nColumn, SCSIZE nRow)
{
    if (rValue.isError())
        pMatrix->PutError(selibreoffice::toFormulaError(rValue.meError), nColumn, nRow);
    else if (rValue.isText())
        pMatrix->PutString(svl::SharedString(selibreoffice::toLibreOfficeString(rValue.maString)),
            nColumn, nRow);
    else if (rValue.isBoolean())
        pMatrix->PutBoolean(rValue.mfNumber != 0.0, nColumn, nRow);
    else if (rValue.isNumber())
        pMatrix->PutDouble(rValue.mfNumber, nColumn, nRow);
    else
        pMatrix->PutEmpty(nColumn, nRow);
}

[[nodiscard]] inline ScMatrixRef matrixOperandToMatrixRef(const serpn::MatrixOperand& rOperand)
{
    ScMatrixRef xMatrix(new ScMatrix(static_cast<SCSIZE>(rOperand.maDimensions.mnColumns),
        static_cast<SCSIZE>(rOperand.maDimensions.mnRows)));
    for (SCSIZE nRow = 0; nRow < static_cast<SCSIZE>(rOperand.maDimensions.mnRows); ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < static_cast<SCSIZE>(rOperand.maDimensions.mnColumns);
             ++nColumn)
        {
            const std::size_t nIndex
                = static_cast<std::size_t>(nRow) * rOperand.maDimensions.mnColumns + nColumn;
            putScalarIntoMatrix(rOperand.maValues[nIndex], xMatrix, nColumn, nRow);
        }
    }
    return xMatrix;
}

[[nodiscard]] inline serpn::MatrixOperand makeScalarMatrixOperand(double fValue)
{
    serpn::MatrixOperand aOperand;
    aOperand.maDimensions = { 1, 1 };
    aOperand.meProvenance = serpn::MatrixProvenance::ComputedResult;
    aOperand.maValues.push_back(spreadsheetengine::api::CellValue::number(fValue));
    return aOperand;
}

[[nodiscard]] inline SCSIZE minBinaryMatrixExtent(SCSIZE nLeft, SCSIZE nRight)
{
    if (nLeft == 1)
        return nRight;
    if (nRight == 1)
        return nLeft;
    return std::min(nLeft, nRight);
}

[[nodiscard]] inline ScMatrixRef binaryMatrixCalculation(
    const ScMatrix& rLeft, const ScMatrix& rRight, ScInterpreter& rCalc,
    const ScMatrix::CalculateOpFunction& rOperation)
{
    SCSIZE nLeftColumns = 0;
    SCSIZE nLeftRows = 0;
    SCSIZE nRightColumns = 0;
    SCSIZE nRightRows = 0;
    rLeft.GetDimensions(nLeftColumns, nLeftRows);
    rRight.GetDimensions(nRightColumns, nRightRows);
    const SCSIZE nResultColumns = minBinaryMatrixExtent(nLeftColumns, nRightColumns);
    const SCSIZE nResultRows = minBinaryMatrixExtent(nLeftRows, nRightRows);
    ScMatrixRef xResult = GetNewMat(nResultColumns, nResultRows, /*bEmpty*/ true);
    if (xResult)
        xResult->ExecuteBinaryOp(nResultColumns, nResultRows, rLeft, rRight, &rCalc, rOperation);
    return xResult;
}

[[nodiscard]] inline double compatMatrixMul(const double& fLeft, const double& fRight)
{
    return fLeft * fRight;
}

[[nodiscard]] inline double compatMatrixDiv(const double& fLeft, const double& fRight)
{
    return ScInterpreter::div(fLeft, fRight);
}

[[nodiscard]] inline double compatMatrixPow(const double& fLeft, const double& fRight)
{
    return sc::power(fLeft, fRight);
}

[[nodiscard]] inline spreadsheetengine::api::query::SearchType toLookupSearchType(
    utl::SearchParam::SearchType eSearchType)
{
    switch (eSearchType)
    {
        case utl::SearchParam::SearchType::Wildcard:
            return spreadsheetengine::api::query::SearchType::Wildcard;
        case utl::SearchParam::SearchType::Regexp:
            return spreadsheetengine::api::query::SearchType::Regex;
        case utl::SearchParam::SearchType::Normal:
        case utl::SearchParam::SearchType::Unknown:
        default:
            return spreadsheetengine::api::query::SearchType::Normal;
    }
}

    [[nodiscard]] constexpr serpn::ForecastEtsVariant toForecastEtsVariant(ScETSType eType)
{
    switch (eType)
    {
        case etsAdd:
            return serpn::ForecastEtsVariant::Add;
        case etsMult:
            return serpn::ForecastEtsVariant::Mult;
        case etsSeason:
            return serpn::ForecastEtsVariant::Seasonality;
        case etsPIAdd:
            return serpn::ForecastEtsVariant::PIAdd;
        case etsPIMult:
            return serpn::ForecastEtsVariant::PIMult;
        case etsStatAdd:
            return serpn::ForecastEtsVariant::StatAdd;
        case etsStatMult:
            return serpn::ForecastEtsVariant::StatMult;
    }
    return serpn::ForecastEtsVariant::Add;
}

    struct Dispatcher
    {
        [[nodiscard]] static serpn::RandomOutputFrame randomOutputFrame(ScInterpreter& rCalc)
        {
            serpn::RandomOutputFrame aFrame;
            if (!SEIC.bMatrixFormula)
                return aFrame;

            aFrame.mbArrayContext = true;
            aFrame.mbSingleCellScalarCompat = true;

            SCCOL nColumns = 0;
            SCROW nRows = 0;
            if (GetStackType(1) == svJumpMatrix)
            {
                SCSIZE nJumpColumns = 0;
                SCSIZE nJumpRows = 0;
                pStack[sp - 1]->GetJumpMatrix()->GetDimensions(nJumpColumns, nJumpRows);
                nColumns = std::max<SCCOL>(0, static_cast<SCCOL>(nJumpColumns));
                nRows = std::max<SCROW>(0, static_cast<SCROW>(nJumpRows));
            }
            else if (SEIC.pMyFormulaCell)
            {
                SEIC.pMyFormulaCell->GetMatColsRows(nColumns, nRows);
            }

            aFrame.maDimensions
                = { static_cast<spreadsheetengine::api::MatrixSize>(std::max<SCCOL>(0, nColumns)),
                    static_cast<spreadsheetengine::api::MatrixSize>(std::max<SCROW>(0, nRows)) };
            return aFrame;
        }

        [[nodiscard]] static FormulaError toCalcMathFormulaError(spreadsheetengine::api::Error eError)
        {
            if (eError == spreadsheetengine::api::Error::Domain)
                return FormulaError::IllegalArgument;
            return selibreoffice::toFormulaError(eError);
        }

        static void pushRandomPlanResult(ScInterpreter& rCalc, const serpn::RandomPlanResult& rResult)
        {
            if (rResult.mbIsScalar)
            {
                PushDouble(rResult.mfScalar);
                return;
            }

            PushMatrix(matrixOperandToMatrixRef(rResult.maMatrix));
        }

        static void pushRandomPlanError(ScInterpreter& rCalc, spreadsheetengine::api::Error eError)
        {
            if (eError == spreadsheetengine::api::Error::IllegalArgument)
            {
                PushIllegalArgument();
                return;
            }

            PushError(selibreoffice::toFormulaError(eError));
        }

    template <typename TResult> static void pushCalcMathValueResult(
        ScInterpreter& rCalc, const TResult& rResult)
    {
        if (!rResult)
        {
            PushError(toCalcMathFormulaError(rResult.meError));
            return;
        }
        PushDouble(rResult.maValue);
    }

 #undef mrDoc
    [[nodiscard]] static FormulaTokenRef rangeReferenceToken(
        ScInterpreter& rCalc, const FormulaToken& rLeft, const FormulaToken& rRight)
    {
        return extendRangeReference(rCalc.mrDoc.GetSheetLimits(),
            const_cast<FormulaToken&>(rLeft), const_cast<FormulaToken&>(rRight), rCalc.aPos,
            false);
    }
#define mrDoc SEIC.mrDoc

    static void textInfoIsBlank(ScInterpreter& rCalc)
    {
        short nRes = 0;
        nFuncFmtType = SvNumFormatType::LOGICAL;
        switch (SEIC.GetRawStackType())
        {
            case svEmptyCell:
            {
                FormulaConstTokenRef p = PopToken();
                if (!static_cast<const ScEmptyCellToken*>(p.get())->IsInherited())
                    nRes = 1;
            }
            break;
            case svDoubleRef:
            case svSingleRef:
            {
                ScAddress aAdr;
                if (!SEIC.PopDoubleRefOrSingleRef(aAdr))
                    break;
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.getType() == CELLTYPE_NONE)
                    nRes = 1;
            }
            break;
            case svExternalSingleRef:
            case svExternalDoubleRef:
            case svMatrix:
            {
                ScMatrixRef pMat = GetMatrix();
                if (!pMat)
                    break;
                if (!SEIC.pJumpMatrix)
                    nRes = pMat->IsEmptyCell(0, 0) ? 1 : 0;
                else
                {
                    SCSIZE nCols, nRows, nC, nR;
                    pMat->GetDimensions(nCols, nRows);
                    SEIC.pJumpMatrix->GetPos(nC, nR);
                    if (nC < nCols && nR < nRows)
                        nRes = pMat->IsEmptyCell(nC, nR) ? 1 : 0;
                }
            }
            break;
            default:
                Pop();
        }
        nGlobalError = FormulaError::NONE;
        SEIC.PushInt(nRes);
    }

    static void textInfoIsText(ScInterpreter& rCalc) { SEIC.PushInt(int(SEIC.IsString())); }

    static void textInfoIsNonText(ScInterpreter& rCalc)
    {
        SEIC.PushInt(int(!SEIC.IsString()));
    }

    static void textInfoIsNumber(ScInterpreter& rCalc)
    {
        nFuncFmtType = SvNumFormatType::LOGICAL;
        bool bRes = false;
        switch (SEIC.GetRawStackType())
        {
            case svDouble:
                Pop();
                bRes = true;
                break;
            case svDoubleRef:
            case svSingleRef:
            {
                ScAddress aAdr;
                if (!SEIC.PopDoubleRefOrSingleRef(aAdr))
                    break;
                ScRefCellValue aCell(mrDoc, aAdr);
                if (GetCellErrCode(aCell) == FormulaError::NONE)
                {
                    switch (aCell.getType())
                    {
                        case CELLTYPE_VALUE:
                            bRes = true;
                            break;
                        case CELLTYPE_FORMULA:
                            bRes = (aCell.getFormula()->IsValue()
                                    && !aCell.getFormula()->IsEmpty());
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
            case svExternalSingleRef:
            {
                ScExternalRefCache::TokenRef pToken;
                SEIC.PopExternalSingleRef(pToken);
                if (nGlobalError == FormulaError::NONE && pToken->GetType() == svDouble)
                    bRes = true;
            }
            break;
            case svExternalDoubleRef:
            case svMatrix:
            {
                ScMatrixRef pMat = GetMatrix();
                if (!pMat)
                    break;
                if (!SEIC.pJumpMatrix)
                {
                    if (pMat->GetErrorIfNotString(0, 0) == FormulaError::NONE)
                        bRes = pMat->IsValue(0, 0);
                }
                else
                {
                    SCSIZE nCols, nRows, nC, nR;
                    pMat->GetDimensions(nCols, nRows);
                    SEIC.pJumpMatrix->GetPos(nC, nR);
                    if (nC < nCols && nR < nRows
                        && pMat->GetErrorIfNotString(nC, nR) == FormulaError::NONE)
                    {
                        bRes = pMat->IsValue(nC, nR);
                    }
                }
            }
            break;
            default:
                Pop();
        }
        nGlobalError = FormulaError::NONE;
        SEIC.PushInt(int(bRes));
    }

    static void textInfoIsNA(ScInterpreter& rCalc)
    {
        nFuncFmtType = SvNumFormatType::LOGICAL;
        bool bRes = false;
        switch (GetStackType())
        {
            case svDoubleRef:
            case svSingleRef:
            {
                ScAddress aAdr;
                const bool bOk = SEIC.PopDoubleRefOrSingleRef(aAdr);
                if (nGlobalError == FormulaError::NotAvailable)
                    bRes = true;
                else if (bOk)
                {
                    ScRefCellValue aCell(mrDoc, aAdr);
                    bRes = (GetCellErrCode(aCell) == FormulaError::NotAvailable);
                }
            }
            break;
            case svExternalSingleRef:
            {
                ScExternalRefCache::TokenRef pToken;
                SEIC.PopExternalSingleRef(pToken);
                if (nGlobalError == FormulaError::NotAvailable
                    || (pToken && pToken->GetType() == svError
                        && pToken->GetError() == FormulaError::NotAvailable))
                {
                    bRes = true;
                }
            }
            break;
            case svExternalDoubleRef:
            case svMatrix:
            {
                ScMatrixRef pMat = GetMatrix();
                if (!pMat)
                    break;
                if (!SEIC.pJumpMatrix)
                    bRes = (pMat->GetErrorIfNotString(0, 0) == FormulaError::NotAvailable);
                else
                {
                    SCSIZE nCols, nRows, nC, nR;
                    pMat->GetDimensions(nCols, nRows);
                    SEIC.pJumpMatrix->GetPos(nC, nR);
                    if (nC < nCols && nR < nRows)
                        bRes = (pMat->GetErrorIfNotString(nC, nR)
                                == FormulaError::NotAvailable);
                }
            }
            break;
            default:
                PopError();
                if (nGlobalError == FormulaError::NotAvailable)
                    bRes = true;
        }
        nGlobalError = FormulaError::NONE;
        SEIC.PushInt(int(bRes));
    }

    static void textInfoIsErrLike(ScInterpreter& rCalc, bool bTreatNAAsError)
    {
        nFuncFmtType = SvNumFormatType::LOGICAL;
        bool bRes = false;
        switch (GetStackType())
        {
            case svDoubleRef:
            case svSingleRef:
            {
                ScAddress aAdr;
                const bool bOk = SEIC.PopDoubleRefOrSingleRef(aAdr);
                if (!bOk || (nGlobalError != FormulaError::NONE
                             && (bTreatNAAsError
                                     || nGlobalError != FormulaError::NotAvailable)))
                {
                    bRes = true;
                }
                else
                {
                    ScRefCellValue aCell(mrDoc, aAdr);
                    const FormulaError nErr = GetCellErrCode(aCell);
                    bRes = bTreatNAAsError ? (nErr != FormulaError::NONE)
                                           : (nErr != FormulaError::NONE
                                              && nErr != FormulaError::NotAvailable);
                }
            }
            break;
            case svExternalSingleRef:
                {
                    ScExternalRefCache::TokenRef pToken;
                    SEIC.PopExternalSingleRef(pToken);
                    if (bTreatNAAsError)
                    {
                        bRes = (nGlobalError != FormulaError::NONE
                                || (pToken && pToken->GetType() == svError));
                    }
                    else if ((nGlobalError != FormulaError::NONE
                              && nGlobalError != FormulaError::NotAvailable)
                         || !pToken
                         || (pToken->GetType() == svError
                             && pToken->GetError() != FormulaError::NotAvailable))
                {
                    bRes = true;
                }
            }
            break;
            case svExternalDoubleRef:
            case svMatrix:
            {
                ScMatrixRef pMat = GetMatrix();
                if (nGlobalError != FormulaError::NONE || !pMat)
                {
                    bRes = bTreatNAAsError
                               ? (nGlobalError != FormulaError::NONE || !pMat)
                               : ((nGlobalError != FormulaError::NONE
                                   && nGlobalError != FormulaError::NotAvailable)
                                  || !pMat);
                }
                else
                {
                    const auto getErr = [&](SCSIZE nC, SCSIZE nR) {
                        return pMat->GetErrorIfNotString(nC, nR);
                    };
                    FormulaError nErr = FormulaError::NONE;
                    if (!SEIC.pJumpMatrix)
                        nErr = getErr(0, 0);
                    else
                    {
                        SCSIZE nCols, nRows, nC, nR;
                        pMat->GetDimensions(nCols, nRows);
                        SEIC.pJumpMatrix->GetPos(nC, nR);
                        if (nC < nCols && nR < nRows)
                            nErr = getErr(nC, nR);
                    }
                    bRes = bTreatNAAsError ? (nErr != FormulaError::NONE)
                                           : (nErr != FormulaError::NONE
                                              && nErr != FormulaError::NotAvailable);
                }
            }
            break;
            default:
                PopError();
                if (bTreatNAAsError)
                    bRes = (nGlobalError != FormulaError::NONE);
                else if (nGlobalError != FormulaError::NONE
                         && nGlobalError != FormulaError::NotAvailable)
                {
                    bRes = true;
                }
        }
        nGlobalError = FormulaError::NONE;
        SEIC.PushInt(int(bRes));
    }

    static void textInfoCode(ScInterpreter& rCalc)
    {
        SEIC.PushInt(selibreoffice::codeFromText(SEIC.GetString().getString()));
    }

    static void textInfoTrim(ScInterpreter& rCalc)
    {
        SEIC.PushString(selibreoffice::trimRepeatedSpaces(SEIC.GetString().getString()));
    }

    static void textInfoLen(ScInterpreter& rCalc)
    {
        PushDouble(selibreoffice::countCodePoints(SEIC.GetString().getString()));
    }

    static void textInfoClean(ScInterpreter& rCalc)
    {
        SEIC.PushString(selibreoffice::cleanPrintable(SEIC.GetString().getString()));
    }

    static void textInfoChar(ScInterpreter& rCalc)
    {
        if (auto aStr = selibreoffice::charFromValue(CalcGetDouble()))
            SEIC.PushString(*aStr);
        else
            PushIllegalArgument();
    }

    static void textInfoJis(ScInterpreter& rCalc)
    {
        if (MustHaveParamCount(GetByte(), 1))
            SEIC.PushString(selibreoffice::convertIntoFullWidth(SEIC.GetString().getString()));
    }

    static void textInfoAsc(ScInterpreter& rCalc)
    {
        if (MustHaveParamCount(GetByte(), 1))
            SEIC.PushString(selibreoffice::convertIntoHalfWidth(SEIC.GetString().getString()));
    }

    static void textInfoUnicode(ScInterpreter& rCalc)
    {
        if (!MustHaveParamCount(GetByte(), 1))
            return;
        if (std::optional<double> fValue
            = selibreoffice::unicodeFromText(SEIC.GetString().getString()))
            PushDouble(*fValue);
        else
            PushIllegalParameter();
    }

    static void textInfoUnichar(ScInterpreter& rCalc)
    {
        if (!MustHaveParamCount(GetByte(), 1))
            return;
        sal_uInt32 nCodePoint = SEIC.GetUInt32();
        if (nGlobalError != FormulaError::NONE)
            PushIllegalArgument();
        else if (auto aStr = selibreoffice::unicharFromCodePoint(nCodePoint))
            SEIC.PushString(*aStr);
        else
            PushIllegalArgument();
    }

    static void textParsingDateValue(ScInterpreter& rCalc)
    {
        const OUString aInputString = SEIC.GetString().getString();
        const auto aResult = spreadsheetengine::compat::libreoffice::textparsingexecution::
            evaluateDateValue(mrDoc, mrContext, aInputString);
        if (aResult)
        {
            nFuncFmtType = SvNumFormatType::DATE;
            PushDouble(aResult.maValue);
        }
        else
            PushIllegalArgument();
    }

    static void textParsingTimeValue(ScInterpreter& rCalc)
    {
        const OUString aInputString = SEIC.GetString().getString();
        const auto aResult = spreadsheetengine::compat::libreoffice::textparsingexecution::
            evaluateTimeValue(mrDoc, mrContext, aInputString);
        if (aResult)
        {
            nFuncFmtType = SvNumFormatType::TIME;
            PushDouble(aResult.maValue);
        }
        else
            PushIllegalArgument();
    }

    static void textParsingValue(ScInterpreter& rCalc)
    {
        OUString aInputString;
        double fVal = 0.0;

        switch (SEIC.GetRawStackType())
        {
            case svMissing:
            case svEmptyCell:
                Pop();
                SEIC.PushInt(0);
                return;
            case svDouble:
                return;
            case svSingleRef:
            case svDoubleRef:
            {
                ScAddress aAdr;
                if (!SEIC.PopDoubleRefOrSingleRef(aAdr))
                {
                    SEIC.PushInt(0);
                    return;
                }
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasString())
                {
                    svl::SharedString aSS;
                    SEIC.GetCellString(aSS, aCell);
                    aInputString = aSS.getString();
                }
                else if (aCell.hasNumeric())
                {
                    PushDouble(GetCellValue(aAdr, aCell));
                    return;
                }
                else
                {
                    PushDouble(0.0);
                    return;
                }
            }
            break;
            case svMatrix:
            {
                svl::SharedString aSS;
                const ScMatValType nType = SEIC.GetDoubleOrStringFromMatrix(fVal, aSS);
                aInputString = aSS.getString();
                switch (nType)
                {
                    case ScMatValType::Empty:
                        fVal = 0.0;
                        [[fallthrough]];
                    case ScMatValType::Value:
                    case ScMatValType::Boolean:
                        PushDouble(fVal);
                        return;
                    case ScMatValType::String:
                        break;
                    default:
                        PushIllegalArgument();
                }
            }
            break;
            default:
                aInputString = SEIC.GetString().getString();
                break;
        }

        const auto aResult = spreadsheetengine::compat::libreoffice::textparsingexecution::
            evaluateValue(mrDoc, mrContext, aInputString);
        if (aResult)
            PushDouble(aResult.maValue);
        else
            PushIllegalArgument();
    }

    static void textParsingNumberValue(ScInterpreter& rCalc, sal_uInt8 nParamCount)
    {
        if (!MustHaveParamCount(nParamCount, 1, 3))
            return;

        std::optional<OUString> oGroupSeparator;
        std::optional<OUString> oDecimalSeparator;
        if (nParamCount == 3)
            oGroupSeparator = SEIC.GetString().getString();
        if (nParamCount >= 2)
            oDecimalSeparator = SEIC.GetString().getString();

        if (GetStackType() == svDouble)
            return;

        OUString aInputString = SEIC.GetString().getString();
        if (nGlobalError != FormulaError::NONE)
        {
            PushError(nGlobalError);
            return;
        }

        const auto aResult = spreadsheetengine::compat::libreoffice::textparsingexecution::
            evaluateNumberValue(mrDoc, mrContext, aInputString, oDecimalSeparator,
                oGroupSeparator, SEIC.maCalcConfig.mbEmptyStringAsZero);
        if (aResult)
        {
            PushDouble(aResult.maValue);
            return;
        }

        switch (aResult.meError)
        {
            case spreadsheetengine::api::Error::IllegalArgument:
                PushIllegalArgument();
                return;
            case spreadsheetengine::api::Error::NoValue:
                PushNoValue();
                return;
            default:
                PushIllegalArgument();
                return;
        }
    }

#undef GetNewMat
    static void formulaInspectionIsFormula(ScInterpreter& rCalc)
    {
        nFuncFmtType = SvNumFormatType::LOGICAL;
        bool bRes = false;
        switch (GetStackType())
        {
            case svDoubleRef:
                if (SEIC.IsInArrayContext())
                {
                    SCCOL nCol1, nCol2;
                    SCROW nRow1, nRow2;
                    SCTAB nTab1, nTab2;
                    PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                    if (nGlobalError != FormulaError::NONE)
                    {
                        PushError(nGlobalError);
                        return;
                    }
                    if (nTab1 != nTab2)
                    {
                        PushIllegalArgument();
                        return;
                    }

                    const auto aMatrixResult
                        = spreadsheetengine::compat::libreoffice::formulainspection::
                            buildIsFormulaMatrix(
                                mrDoc, mrContext,
                                ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2),
                                [&rCalc](SCSIZE nColumns, SCSIZE nRows) {
                                    return rCalc.GetNewMat(nColumns, nRows, true);
                                });
                    if (aMatrixResult.meFailure
                        == spreadsheetengine::compat::libreoffice::formulainspection::
                               MatrixInspectionFailure::IllegalArgument)
                    {
                        PushIllegalArgument();
                        return;
                    }
                    if (aMatrixResult.meFailure
                        == spreadsheetengine::compat::libreoffice::formulainspection::
                               MatrixInspectionFailure::MatrixSize)
                    {
                        PushError(FormulaError::MatrixSize);
                        return;
                    }

                    PushMatrix(aMatrixResult.mpMatrix);
                    return;
                }
                [[fallthrough]];
            case svSingleRef:
            {
                ScAddress aAdr;
                if (!SEIC.PopDoubleRefOrSingleRef(aAdr))
                    break;
                bRes = spreadsheetengine::compat::libreoffice::formulainspection::isFormulaCell(
                    mrDoc, mrContext, aAdr);
            }
            break;
            default:
                Pop();
        }
        nGlobalError = FormulaError::NONE;
        SEIC.PushInt(int(bRes));
    }

    static void formulaInspectionFormulaText(ScInterpreter& rCalc)
    {
        OUString aFormula;
        switch (GetStackType())
        {
            case svDoubleRef:
                if (SEIC.IsInArrayContext())
                {
                    SCCOL nCol1, nCol2;
                    SCROW nRow1, nRow2;
                    SCTAB nTab1, nTab2;
                    PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                    if (nGlobalError != FormulaError::NONE)
                        break;

                    if (nTab1 != nTab2)
                    {
                        SetError(FormulaError::IllegalArgument);
                        break;
                    }

                    const auto aMatrixResult
                        = spreadsheetengine::compat::libreoffice::formulainspection::
                            buildFormulaTextMatrix(
                                mrDoc, mrContext,
                                ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2),
                                SEIC.mrStrPool, [&rCalc](SCSIZE nColumns, SCSIZE nRows) {
                                    return rCalc.GetNewMat(nColumns, nRows, true);
                                });
                    if (aMatrixResult.meFailure
                        == spreadsheetengine::compat::libreoffice::formulainspection::
                               MatrixInspectionFailure::IllegalArgument)
                    {
                        SetError(FormulaError::IllegalArgument);
                        break;
                    }
                    if (aMatrixResult.meFailure
                        == spreadsheetengine::compat::libreoffice::formulainspection::
                               MatrixInspectionFailure::MatrixSize)
                    {
                        break;
                    }

                    PushMatrix(aMatrixResult.mpMatrix);
                    return;
                }
                [[fallthrough]];
            case svSingleRef:
            {
                ScAddress aAdr;
                if (!SEIC.PopDoubleRefOrSingleRef(aAdr))
                    break;

                const auto aFormulaText
                    = spreadsheetengine::compat::libreoffice::formulainspection::formulaTextForCell(
                        mrDoc, mrContext, aAdr);
                if (!aFormulaText)
                    SetError(selibreoffice::toFormulaError(aFormulaText.meError));
                else
                    aFormula = aFormulaText.maValue;
            }
            break;
            default:
                PopError();
                SetError(FormulaError::NotAvailable);
        }

        SEIC.PushString(aFormula);
    }
#define GetNewMat(...) SEIC.GetNewMat(__VA_ARGS__)

    static void statisticalKurt(ScInterpreter& rCalc)
    {
        KahanSum fSum;
        double fCount = 0.0;
        std::vector<double> aValues;
        if (!CalculateSkew(fSum, fCount, aValues))
            return;
        const auto aResult = semath::evaluateKurtosisNumbers(aValues);
        pushCalcMathValueResult(rCalc, aResult);
    }

    static void statisticalHarMean(ScInterpreter& rCalc)
    {
        short nParamCount = GetByte();
        std::vector<double> aValues;
        ScAddress aAdr;
        ScRange aRange;
        size_t nRefInList = 0;
        while ((nGlobalError == FormulaError::NONE) && (nParamCount-- > 0))
        {
            switch (GetStackType())
            {
                case svDouble:
                {
                    double x = CalcGetDouble();
                    if (x > 0.0)
                        aValues.push_back(x);
                    else
                        SetError(FormulaError::IllegalArgument);
                    break;
                }
                case svSingleRef:
                {
                    PopSingleRef(aAdr);
                    ScRefCellValue aCell(mrDoc, aAdr);
                    if (aCell.hasNumeric())
                    {
                        double x = GetCellValue(aAdr, aCell);
                        if (x > 0.0)
                            aValues.push_back(x);
                        else
                            SetError(FormulaError::IllegalArgument);
                    }
                    break;
                }
                case svDoubleRef:
                case svRefList:
                {
                    FormulaError nErr = FormulaError::NONE;
                    PopDoubleRef(aRange, nParamCount, nRefInList);
                    double nCellVal;
                    ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                    if (aValIter.GetFirst(nCellVal, nErr))
                    {
                        if (nCellVal > 0.0)
                            aValues.push_back(nCellVal);
                        else
                            SetError(FormulaError::IllegalArgument);
                        SetError(nErr);
                        while ((nErr == FormulaError::NONE) && aValIter.GetNext(nCellVal, nErr))
                        {
                            if (nCellVal > 0.0)
                                aValues.push_back(nCellVal);
                            else
                                SetError(FormulaError::IllegalArgument);
                        }
                        SetError(nErr);
                    }
                    break;
                }
                case svMatrix:
                case svExternalSingleRef:
                case svExternalDoubleRef:
                {
                    ScMatrixRef pMat = GetMatrix();
                    if (pMat)
                    {
                        SCSIZE nCount = pMat->GetElementCount();
                        if (pMat->IsNumeric())
                        {
                            for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            {
                                double x = pMat->GetDouble(nElem);
                                if (x > 0.0)
                                    aValues.push_back(x);
                                else
                                    SetError(FormulaError::IllegalArgument);
                            }
                        }
                        else
                        {
                            for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            {
                                if (!pMat->IsStringOrEmpty(nElem))
                                {
                                    double x = pMat->GetDouble(nElem);
                                    if (x > 0.0)
                                        aValues.push_back(x);
                                    else
                                        SetError(FormulaError::IllegalArgument);
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    SetError(FormulaError::IllegalParameter);
                    break;
            }
        }
        if (nGlobalError != FormulaError::NONE)
        {
            PushError(nGlobalError);
            return;
        }
        const auto aResult = semath::evaluateHarmonicMeanNumbers(aValues);
        pushCalcMathValueResult(rCalc, aResult);
    }

    static void statisticalGeoMean(ScInterpreter& rCalc)
    {
        short nParamCount = GetByte();
        std::vector<double> aValues;
        ScAddress aAdr;
        ScRange aRange;
        size_t nRefInList = 0;
        while ((nGlobalError == FormulaError::NONE) && (nParamCount-- > 0))
        {
            switch (GetStackType())
            {
                case svDouble:
                {
                    double x = CalcGetDouble();
                    if (x > 0.0)
                        aValues.push_back(x);
                    else if (x == 0.0)
                    {
                        while (nParamCount-- > 0)
                            PopError();
                        PushDouble(0.0);
                        return;
                    }
                    else
                        SetError(FormulaError::IllegalArgument);
                    break;
                }
                case svSingleRef:
                {
                    PopSingleRef(aAdr);
                    ScRefCellValue aCell(mrDoc, aAdr);
                    if (aCell.hasNumeric())
                    {
                        double x = GetCellValue(aAdr, aCell);
                        if (x > 0.0)
                            aValues.push_back(x);
                        else if (x == 0.0)
                        {
                            while (nParamCount-- > 0)
                                PopError();
                            PushDouble(0.0);
                            return;
                        }
                        else
                            SetError(FormulaError::IllegalArgument);
                    }
                    break;
                }
                case svDoubleRef:
                case svRefList:
                {
                    FormulaError nErr = FormulaError::NONE;
                    PopDoubleRef(aRange, nParamCount, nRefInList);
                    double nCellVal;
                    ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                    if (aValIter.GetFirst(nCellVal, nErr))
                    {
                        if (nCellVal > 0.0)
                            aValues.push_back(nCellVal);
                        else if (nCellVal == 0.0)
                        {
                            while (nParamCount-- > 0)
                                PopError();
                            PushDouble(0.0);
                            return;
                        }
                        else
                            SetError(FormulaError::IllegalArgument);
                        SetError(nErr);
                        while ((nErr == FormulaError::NONE) && aValIter.GetNext(nCellVal, nErr))
                        {
                            if (nCellVal > 0.0)
                                aValues.push_back(nCellVal);
                            else if (nCellVal == 0.0)
                            {
                                while (nParamCount-- > 0)
                                    PopError();
                                PushDouble(0.0);
                                return;
                            }
                            else
                                SetError(FormulaError::IllegalArgument);
                        }
                        SetError(nErr);
                    }
                    break;
                }
                case svMatrix:
                case svExternalSingleRef:
                case svExternalDoubleRef:
                {
                    ScMatrixRef pMat = GetMatrix();
                    if (pMat)
                    {
                        SCSIZE nCount = pMat->GetElementCount();
                        if (pMat->IsNumeric())
                        {
                            for (SCSIZE ui = 0; ui < nCount; ui++)
                            {
                                double x = pMat->GetDouble(ui);
                                if (x > 0.0)
                                    aValues.push_back(x);
                                else if (x == 0.0)
                                {
                                    while (nParamCount-- > 0)
                                        PopError();
                                    PushDouble(0.0);
                                    return;
                                }
                                else
                                    SetError(FormulaError::IllegalArgument);
                            }
                        }
                        else
                        {
                            for (SCSIZE ui = 0; ui < nCount; ui++)
                            {
                                if (!pMat->IsStringOrEmpty(ui))
                                {
                                    double x = pMat->GetDouble(ui);
                                    if (x > 0.0)
                                        aValues.push_back(x);
                                    else if (x == 0.0)
                                    {
                                        while (nParamCount-- > 0)
                                            PopError();
                                        PushDouble(0.0);
                                        return;
                                    }
                                    else
                                        SetError(FormulaError::IllegalArgument);
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    SetError(FormulaError::IllegalParameter);
                    break;
            }
        }
        if (nGlobalError != FormulaError::NONE)
        {
            PushError(nGlobalError);
            return;
        }
        const auto aResult = semath::evaluateGeometricMeanNumbers(aValues);
        pushCalcMathValueResult(rCalc, aResult);
    }

    static void statisticalSkew(ScInterpreter& rCalc, bool bPopulation)
    {
        KahanSum fSum;
        double fCount = 0.0;
        std::vector<double> aValues;
        if (!CalculateSkew(fSum, fCount, aValues))
            return;
        const auto aResult = semath::evaluateSkewNumbers(aValues, bPopulation);
        pushCalcMathValueResult(rCalc, aResult);
    }

    static void statisticalMedian(ScInterpreter& rCalc)
    {
        sal_uInt8 nParamCount = GetByte();
        if (!MustHaveParamCountMin(nParamCount, 1))
            return;
        std::vector<double> aArray;
        GetNumberSequenceArray(nParamCount, aArray, false);
        if (aArray.empty() || nGlobalError != FormulaError::NONE)
        {
            PushNoValue();
            return;
        }
        semath::AggregateScan aScan;
        aScan.maNumbers = std::move(aArray);
        const auto aResult = semath::evaluateAggregateNumbers(12, aScan);
        pushCalcMathValueResult(rCalc, aResult);
    }

    static void statisticalMode(ScInterpreter& rCalc, bool bSingle, bool bSmallest = false)
    {
        sal_uInt8 nParamCount = GetByte();
        if (!MustHaveParamCountMin(nParamCount, 1))
            return;
        std::vector<double> aArray;
        GetNumberSequenceArray(nParamCount, aArray, false);
        if (aArray.empty() || nGlobalError != FormulaError::NONE)
        {
            PushNoValue();
            return;
        }
        const auto aModes = semath::evaluateModeValues(aArray);
        if (!aModes)
        {
            PushError(toCalcMathFormulaError(aModes.meError));
            return;
        }
        if (bSingle)
        {
            // MODE (classical) returns the smallest tied mode;
            // MODE.SNGL returns the first-in-input-order mode.
            if (bSmallest)
                PushDouble(*std::min_element(
                    aModes.maValue.begin(), aModes.maValue.end()));
            else
                PushDouble(aModes.maValue.front());
        }
        else
        {
            ScMatrixRef pResMatrix = GetNewMat(1, aModes.maValue.size(), true);
            pResMatrix->PutDoubleVector(aModes.maValue, 0, 0);
            PushMatrix(pResMatrix);
        }
    }

    static void statisticalPercentile(ScInterpreter& rCalc, bool bInclusive)
    {
        if (!MustHaveParamCount(GetByte(), 2))
            return;
        double fAlpha = CalcGetDouble();
        if (bInclusive ? (fAlpha < 0.0 || fAlpha > 1.0) : (fAlpha <= 0.0 || fAlpha >= 1.0))
        {
            PushIllegalArgument();
            return;
        }
        std::vector<double> aArray;
        GetNumberSequenceArray(1, aArray, false);
        if (aArray.empty() || nGlobalError != FormulaError::NONE)
        {
            PushNoValue();
            return;
        }
        semath::AggregateScan aScan;
        aScan.maNumbers = std::move(aArray);
        const auto aResult = semath::evaluateAggregateRankedNumbers(
            bInclusive ? 16 : 18, aScan, fAlpha);
        pushCalcMathValueResult(rCalc, aResult);
    }

    static void statisticalQuartile(ScInterpreter& rCalc, bool bInclusive)
    {
        if (!MustHaveParamCount(GetByte(), 2))
            return;
        double fFlag = ::rtl::math::approxFloor(CalcGetDouble());
        if (bInclusive ? (fFlag < 0.0 || fFlag > 4.0) : (fFlag <= 0.0 || fFlag >= 4.0))
        {
            PushIllegalArgument();
            return;
        }
        std::vector<double> aArray;
        GetNumberSequenceArray(1, aArray, false);
        if (aArray.empty() || nGlobalError != FormulaError::NONE)
        {
            PushNoValue();
            return;
        }
        semath::AggregateScan aScan;
        aScan.maNumbers = std::move(aArray);
        const auto aResult = semath::evaluateAggregateRankedNumbers(
            bInclusive ? 17 : 19, aScan, fFlag);
        pushCalcMathValueResult(rCalc, aResult);
    }

    static void statisticalPercentrank(ScInterpreter& rCalc, bool bInclusive)
    {
        sal_uInt8 nParamCount = GetByte();
        if (!MustHaveParamCount(nParamCount, 2, 3))
            return;
        double fSignificance = (nParamCount == 3 ? ::rtl::math::approxFloor(CalcGetDouble()) : 3.0);
        if (fSignificance < 1.0)
        {
            PushIllegalArgument();
            return;
        }
        double fNum = CalcGetDouble();
        std::vector<double> aSortArray;
        GetSortArray(1, aSortArray, nullptr, false, false);
        SCSIZE nSize = aSortArray.size();
        if (nSize == 0 || nGlobalError != FormulaError::NONE)
        {
            PushNoValue();
            return;
        }
        if (fNum < aSortArray[0] || fNum > aSortArray[nSize - 1])
        {
            PushNoValue();
            return;
        }
        double fRes = nSize == 1 ? 1.0 : GetPercentrank(aSortArray, fNum, bInclusive);
        if (fRes != 0.0)
        {
            double fExp = ::rtl::math::approxFloor(log10(fRes)) + 1.0 - fSignificance;
            fRes = ::rtl::math::round(fRes * pow(10, -fExp)) / pow(10, -fExp);
        }
        PushDouble(fRes);
    }

    static void statisticalTrimMean(ScInterpreter& rCalc)
    {
        if (!MustHaveParamCount(GetByte(), 2))
            return;
        double fAlpha = CalcGetDouble();
        if (fAlpha < 0.0 || fAlpha >= 1.0)
        {
            PushIllegalArgument();
            return;
        }
        std::vector<double> aSortArray;
        GetSortArray(1, aSortArray, nullptr, false, false);
        if (aSortArray.empty() || nGlobalError != FormulaError::NONE)
        {
            PushNoValue();
            return;
        }
        const auto aResult = semath::evaluateTrimmean(std::move(aSortArray), fAlpha);
        pushCalcMathValueResult(rCalc, aResult);
    }

    static void statisticalRank(ScInterpreter& rCalc, bool bAverage)
    {
        sal_uInt8 nParamCount = GetByte();
        if (!MustHaveParamCount(nParamCount, 2, 3))
            return;
        bool bAscending = nParamCount == 3 && CalcGetDouble() != 0.0;
        std::vector<double> aSortArray;
        GetSortArray(1, aSortArray, nullptr, false, false);
        double fVal = CalcGetDouble();
        SCSIZE nSize = aSortArray.size();
        if (nSize == 0 || nGlobalError != FormulaError::NONE)
        {
            PushNoValue();
            return;
        }
        if (fVal < aSortArray[0] || fVal > aSortArray[nSize - 1])
        {
            PushError(FormulaError::NotAvailable);
            return;
        }
        double fLastPos = 0.0;
        double fFirstPos = -1.0;
        bool bFinished = false;
        SCSIZE i = 0;
        for (; i < nSize && !bFinished; i++)
        {
            if (aSortArray[i] == fVal)
            {
                if (fFirstPos < 0.0)
                    fFirstPos = i + 1.0;
            }
            else if (aSortArray[i] > fVal)
            {
                fLastPos = i;
                bFinished = true;
            }
        }
        if (!bFinished)
            fLastPos = i;
        if (fFirstPos <= 0.0)
            PushError(FormulaError::NotAvailable);
        else if (!bAverage)
            PushDouble(bAscending ? fFirstPos : nSize + 1.0 - fLastPos);
        else
        {
            const double fAverage = (fFirstPos + fLastPos) / 2.0;
            PushDouble(bAscending ? fAverage : nSize + 1.0 - fAverage);
        }
    }

    static void statisticalAveDev(ScInterpreter& rCalc)
    {
        sal_uInt8 nParamCount = GetByte();
        if (!MustHaveParamCountMin(nParamCount, 1))
            return;
        sal_uInt16 nSaveSP = sp;
        double fMiddle = 0.0;
        KahanSum fValue = 0.0;
        double fValueCount = 0.0;
        ScAddress aAdr;
        ScRange aRange;
        short nParam = nParamCount;
        size_t nRefInList = 0;
        while (nParam-- > 0)
        {
            switch (GetStackType())
            {
                case svDouble:
                    fValue += CalcGetDouble();
                    fValueCount++;
                    break;
                case svSingleRef:
                {
                    PopSingleRef(aAdr);
                    ScRefCellValue aCell(mrDoc, aAdr);
                    if (aCell.hasNumeric())
                    {
                        fValue += GetCellValue(aAdr, aCell);
                        fValueCount++;
                    }
                    break;
                }
                case svDoubleRef:
                case svRefList:
                {
                    FormulaError nErr = FormulaError::NONE;
                    double nCellVal;
                    PopDoubleRef(aRange, nParam, nRefInList);
                    ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                    if (aValIter.GetFirst(nCellVal, nErr))
                    {
                        fValue += nCellVal;
                        fValueCount++;
                        SetError(nErr);
                        while ((nErr == FormulaError::NONE) && aValIter.GetNext(nCellVal, nErr))
                        {
                            fValue += nCellVal;
                            fValueCount++;
                        }
                        SetError(nErr);
                    }
                    break;
                }
                case svMatrix:
                case svExternalSingleRef:
                case svExternalDoubleRef:
                {
                    ScMatrixRef pMat = GetMatrix();
                    if (pMat)
                    {
                        SCSIZE nCount = pMat->GetElementCount();
                        if (pMat->IsNumeric())
                        {
                            for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            {
                                fValue += pMat->GetDouble(nElem);
                                fValueCount++;
                            }
                        }
                        else
                        {
                            for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            {
                                if (!pMat->IsStringOrEmpty(nElem))
                                {
                                    fValue += pMat->GetDouble(nElem);
                                    fValueCount++;
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    SetError(FormulaError::IllegalParameter);
                    break;
            }
        }
        if (nGlobalError != FormulaError::NONE)
        {
            PushError(nGlobalError);
            return;
        }
        fMiddle = fValue.get() / fValueCount;
        sp = nSaveSP;
        fValue = 0.0;
        nParam = nParamCount;
        nRefInList = 0;
        while (nParam-- > 0)
        {
            switch (GetStackType())
            {
                case svDouble:
                    fValue += std::abs(CalcGetDouble() - fMiddle);
                    break;
                case svSingleRef:
                {
                    PopSingleRef(aAdr);
                    ScRefCellValue aCell(mrDoc, aAdr);
                    if (aCell.hasNumeric())
                        fValue += std::abs(GetCellValue(aAdr, aCell) - fMiddle);
                    break;
                }
                case svDoubleRef:
                case svRefList:
                {
                    FormulaError nErr = FormulaError::NONE;
                    double nCellVal;
                    PopDoubleRef(aRange, nParam, nRefInList);
                    ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                    if (aValIter.GetFirst(nCellVal, nErr))
                    {
                        fValue += std::abs(nCellVal - fMiddle);
                        while (aValIter.GetNext(nCellVal, nErr))
                            fValue += std::abs(nCellVal - fMiddle);
                    }
                    break;
                }
                case svMatrix:
                case svExternalSingleRef:
                case svExternalDoubleRef:
                {
                    ScMatrixRef pMat = GetMatrix();
                    if (pMat)
                    {
                        SCSIZE nCount = pMat->GetElementCount();
                        if (pMat->IsNumeric())
                        {
                            for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                                fValue += std::abs(pMat->GetDouble(nElem) - fMiddle);
                        }
                        else
                        {
                            for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            {
                                if (!pMat->IsStringOrEmpty(nElem))
                                    fValue += std::abs(pMat->GetDouble(nElem) - fMiddle);
                            }
                        }
                    }
                    break;
                }
                default:
                    SetError(FormulaError::IllegalParameter);
                    break;
            }
        }
        PushDouble(fValue.get() / fValueCount);
    }

    static void statisticalDevSq(ScInterpreter& rCalc)
    {
        auto VarResult = [](double fVal, size_t) { return fVal; };
        GetStVarParams(false, VarResult);
    }

    static void statisticalRSQ(ScInterpreter& rCalc)
    {
        CalculatePearsonCovar(true, false, false);
        if (nGlobalError != FormulaError::NONE)
            return;
        switch (GetStackType())
        {
            case svDouble:
            {
                double fVal = SEIC.PopDouble();
                PushDouble(fVal * fVal);
                break;
            }
            default:
                PopError();
                PushNoValue();
                break;
        }
    }

    static void aggregateSumProduct(ScInterpreter& rCalc);
    static void aggregateSumSq(ScInterpreter& rCalc);
    static void aggregateSum(ScInterpreter& rCalc);
    static void aggregateProduct(ScInterpreter& rCalc);
    static void aggregateAverage(ScInterpreter& rCalc, bool bTextAsZero);
    static void aggregateMin(ScInterpreter& rCalc, bool bTextAsZero);
    static void aggregateMax(ScInterpreter& rCalc, bool bTextAsZero);
    static void aggregateVar(ScInterpreter& rCalc, bool bTextAsZero);
    static void aggregateVarP(ScInterpreter& rCalc, bool bTextAsZero);
    static void aggregateStDev(ScInterpreter& rCalc, bool bTextAsZero);
    static void aggregateStDevP(ScInterpreter& rCalc, bool bTextAsZero);
    static void comparisonKernel(
        ScInterpreter& rCalc, seinterpre::ComparisonMode eMode, ScQueryOp eOp);
    static void logicalFoldKernel(
        ScInterpreter& rCalc, seinterpre::LogicalFoldMode eMode);
    static void unaryMatrixOrScalarKernel(
        ScInterpreter& rCalc, seinterpre::UnaryMatrixScalarMode eMode);
    static void binaryMathKernel(
        ScInterpreter& rCalc, spreadsheetengine::core::rpn::BinaryScalarOperator eOperator);
    static void concatKernel(ScInterpreter& rCalc);
    static void letKernel(ScInterpreter& rCalc);
    static void lookupTerminal(ScInterpreter& rCalc);
    static void xlookupTerminal(ScInterpreter& rCalc);
    static void matchOperation(ScInterpreter& rCalc, bool bExtended);
    static void indirectTerminal(ScInterpreter& rCalc);
    static void addressTerminal(ScInterpreter& rCalc);
    static void indexTerminal(ScInterpreter& rCalc);
    static void multiAreaTerminal(ScInterpreter& rCalc);
    static void intersectTerminal(ScInterpreter& rCalc);
    static void rangeReferenceTerminal(ScInterpreter& rCalc);
    static void unionTerminal(ScInterpreter& rCalc);
    static void missingTerminal(ScInterpreter& rCalc);
    static void matrixDeterminant(ScInterpreter& rCalc);
    static void random(ScInterpreter& rCalc);
    static void randArray(ScInterpreter& rCalc);
    static void randbetween(ScInterpreter& rCalc);
    static void matrixSumXMY2(ScInterpreter& rCalc);
    static void matrixFrequency(ScInterpreter& rCalc);
    static void fourier(ScInterpreter& rCalc);
    static void aggregateFunction(ScInterpreter& rCalc);
    static void subtotalFunction(ScInterpreter& rCalc);
    static void sortByTerminal(ScInterpreter& rCalc);
    static void dbAreaTerminal(ScInterpreter& rCalc);
    static void colRowNameAutoTerminal(ScInterpreter& rCalc);
    static void probability(ScInterpreter& rCalc);
    static void zTest(ScInterpreter& rCalc);
    static void tTest(ScInterpreter& rCalc);
    static void fTest(ScInterpreter& rCalc);
    static void chiTest(ScInterpreter& rCalc);
    static void forecast(ScInterpreter& rCalc);
    static void forecastEts(ScInterpreter& rCalc, ScETSType eETSType);
    static void growth(ScInterpreter& rCalc) { CalculateTrendGrowth(true); }
};

inline void Dispatcher::comparisonKernel(
    ScInterpreter& rCalc, seinterpre::ComparisonMode eMode, ScQueryOp eOp)
{
    if (GetStackType(1) == svMatrix || GetStackType(2) == svMatrix)
    {
        sc::RangeMatrix aMatrix = SEIC.CompareMat(eOp);
        if (!aMatrix.mpMat)
        {
            PushIllegalParameter();
            return;
        }

        PushMatrix(aMatrix);
        return;
    }

    SEIC.PushInt(int(seinterpre::matchesComparisonResult(SEIC.Compare(eOp), eMode)));
}

inline void Dispatcher::logicalFoldKernel(
    ScInterpreter& rCalc, seinterpre::LogicalFoldMode eMode)
{
    nFuncFmtType = SvNumFormatType::LOGICAL;
    short nParamCount = GetByte();
    if (!MustHaveParamCountMin(nParamCount, 1))
        return;

    bool bHaveValue = false;
    bool bResult = seinterpre::initialLogicalFoldValue(eMode);
    size_t nRefInList = 0;

    const auto foldValue = [&](bool bValue) {
        bHaveValue = true;
        bResult = seinterpre::foldLogicalValue(eMode, bResult, bValue);
    };

    while (nParamCount-- > 0)
    {
        if (nGlobalError == FormulaError::NONE)
        {
            switch (GetStackType())
            {
                case svDouble:
                    foldValue(SEIC.PopDouble() != 0.0);
                    break;
                case svString:
                    Pop();
                    SetError(FormulaError::NoValue);
                    break;
                case svSingleRef:
                {
                    ScAddress aAddress;
                    PopSingleRef(aAddress);
                    if (nGlobalError == FormulaError::NONE)
                    {
                        ScRefCellValue aCell(mrDoc, aAddress);
                        if (aCell.hasNumeric())
                            foldValue(GetCellValue(aAddress, aCell) != 0.0);
                    }
                    break;
                }
                case svDoubleRef:
                case svRefList:
                {
                    ScRange aRange;
                    PopDoubleRef(aRange, nParamCount, nRefInList);
                    if (nGlobalError == FormulaError::NONE)
                    {
                        double fValue = 0.0;
                        FormulaError eError = FormulaError::NONE;
                        ScValueIterator aValueIter(mrContext, aRange);
                        if (aValueIter.GetFirst(fValue, eError))
                        {
                            do
                            {
                                foldValue(fValue != 0.0);
                            } while (eError == FormulaError::NONE
                                     && aValueIter.GetNext(fValue, eError));
                        }
                        SetError(eError);
                    }
                    break;
                }
                case svExternalSingleRef:
                case svExternalDoubleRef:
                case svMatrix:
                {
                    ScMatrixRef pMatrix = GetMatrix();
                    if (pMatrix)
                    {
                        double fValue = 0.0;
                        switch (eMode)
                        {
                            case seinterpre::LogicalFoldMode::And:
                                fValue = pMatrix->And();
                                break;
                            case seinterpre::LogicalFoldMode::Or:
                                fValue = pMatrix->Or();
                                break;
                            case seinterpre::LogicalFoldMode::Xor:
                                fValue = pMatrix->Xor();
                                break;
                        }

                        const FormulaError eError = GetDoubleErrorValue(fValue);
                        if (eError != FormulaError::NONE)
                        {
                            SetError(eError);
                            bResult = false;
                        }
                        else
                            foldValue(fValue != 0.0);
                    }
                    break;
                }
                default:
                    PopError();
                    SetError(FormulaError::IllegalParameter);
                    break;
            }
        }
        else
            Pop();
    }

    if (bHaveValue)
        SEIC.PushInt(int(bResult));
    else
        PushNoValue();
}

inline void Dispatcher::unaryMatrixOrScalarKernel(
    ScInterpreter& rCalc, seinterpre::UnaryMatrixScalarMode eMode)
{
    switch (GetStackType())
    {
        case svMatrix:
        {
            ScMatrixRef pMatrix = GetMatrix();
            if (!pMatrix)
                PushIllegalParameter();
            else
            {
                SCSIZE nColumns = 0;
                SCSIZE nRows = 0;
                pMatrix->GetDimensions(nColumns, nRows);
                ScMatrixRef pResultMatrix = GetNewMat(nColumns, nRows, /*bEmpty*/ true);
                if (!pResultMatrix)
                    PushIllegalArgument();
                else
                {
                    if (eMode == seinterpre::UnaryMatrixScalarMode::Negate)
                        pMatrix->NegOp(*pResultMatrix);
                    else
                        pMatrix->NotOp(*pResultMatrix);
                    PushMatrix(pResultMatrix);
                }
            }
            break;
        }
        default:
            if (eMode == seinterpre::UnaryMatrixScalarMode::Negate)
                PushDouble(-SEIC.GetDouble());
            else
                SEIC.PushInt(int(SEIC.GetDouble() == 0.0));
            break;
    }
}

inline void Dispatcher::binaryMathKernel(
    ScInterpreter& rCalc, spreadsheetengine::core::rpn::BinaryScalarOperator eOperator)
{
    ScMatrixRef pMatrix1 = nullptr;
    ScMatrixRef pMatrix2 = nullptr;
    double fValue1 = 0.0;
    double fValue2 = 0.0;
    SvNumFormatType eCurrencyType = SEIC.nCurFmtType;
    sal_uLong nCurrencyIndex = SEIC.nCurFmtIndex;
    SvNumFormatType eCurrencyType2 = SvNumFormatType::UNDEFINED;

    const auto pMatrixOperation = [&]() -> double (*)(const double&, const double&) {
        switch (eOperator)
        {
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Multiply:
                return compatMatrixMul;
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Divide:
                return compatMatrixDiv;
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Power:
                return compatMatrixPow;
            default:
                return nullptr;
        }
    }();

    if (GetStackType() == svMatrix)
        pMatrix2 = GetMatrix();
    else
    {
        fValue2 = SEIC.GetDouble();
        if (eOperator == spreadsheetengine::core::rpn::BinaryScalarOperator::Multiply)
        {
            if (SEIC.nCurFmtType == SvNumFormatType::CURRENCY)
            {
                eCurrencyType = SEIC.nCurFmtType;
                nCurrencyIndex = SEIC.nCurFmtIndex;
            }
        }
        else if (eOperator == spreadsheetengine::core::rpn::BinaryScalarOperator::Divide)
            eCurrencyType2 = SEIC.nCurFmtType;
    }

    if (GetStackType() == svMatrix)
        pMatrix1 = GetMatrix();
    else
    {
        fValue1 = SEIC.GetDouble();
        if (eOperator == spreadsheetengine::core::rpn::BinaryScalarOperator::Multiply
            || eOperator == spreadsheetengine::core::rpn::BinaryScalarOperator::Divide)
        {
            if (SEIC.nCurFmtType == SvNumFormatType::CURRENCY)
            {
                eCurrencyType = SEIC.nCurFmtType;
                nCurrencyIndex = SEIC.nCurFmtIndex;
            }
        }
    }

    if (pMatrix1 && pMatrix2)
    {
        ScMatrixRef pResultMatrix = pMatrixOperation
                                        ? binaryMatrixCalculation(
                                              *pMatrix1, *pMatrix2, rCalc, pMatrixOperation)
                                        : nullptr;
        if (!pResultMatrix)
            PushNoValue();
        else
            PushMatrix(pResultMatrix);
    }
    else if (pMatrix1 || pMatrix2)
    {
        double fScalar = 0.0;
        bool bScalarOnLeft = false;
        ScMatrixRef pMatrix = std::move(pMatrix1);
        if (!pMatrix)
        {
            fScalar = fValue1;
            pMatrix = std::move(pMatrix2);
            bScalarOnLeft = true;
        }
        else
            fScalar = fValue2;

        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        pMatrix->GetDimensions(nColumns, nRows);
        ScMatrixRef pResultMatrix = GetNewMat(nColumns, nRows, /*bEmpty*/ true);
        if (!pResultMatrix)
        {
            PushIllegalArgument();
            return;
        }

        switch (eOperator)
        {
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Multiply:
                pMatrix->MulOp(fScalar, *pResultMatrix);
                break;
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Divide:
                pMatrix->DivOp(bScalarOnLeft, fScalar, *pResultMatrix);
                break;
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Power:
                pMatrix->PowOp(bScalarOnLeft, fScalar, *pResultMatrix);
                break;
            default:
                PushIllegalArgument();
                return;
        }
        PushMatrix(pResultMatrix);
    }
    else
    {
        switch (eOperator)
        {
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Multiply:
                if (eCurrencyType == SvNumFormatType::CURRENCY)
                {
                    nFuncFmtType = eCurrencyType;
                    nFuncFmtIndex = nCurrencyIndex;
                }
                PushDouble(fValue1 * fValue2);
                break;
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Divide:
                if (eCurrencyType == SvNumFormatType::CURRENCY
                    && eCurrencyType2 != SvNumFormatType::CURRENCY)
                {
                    nFuncFmtType = eCurrencyType;
                    nFuncFmtIndex = nCurrencyIndex;
                }
                PushDouble(ScInterpreter::div(fValue1, fValue2));
                break;
            case spreadsheetengine::core::rpn::BinaryScalarOperator::Power:
                PushDouble(sc::power(fValue1, fValue2));
                break;
            default:
                PushIllegalArgument();
                break;
        }
    }
}

inline void Dispatcher::concatKernel(ScInterpreter& rCalc)
{
    ScMatrixRef pMatrix1 = nullptr;
    ScMatrixRef pMatrix2 = nullptr;
    OUString aString1;
    OUString aString2;
    if (GetStackType() == svMatrix)
        pMatrix2 = GetMatrix();
    else
        aString2 = SEIC.GetString().getString();
    if (GetStackType() == svMatrix)
        pMatrix1 = GetMatrix();
    else
        aString1 = SEIC.GetString().getString();

    if (pMatrix1 && pMatrix2)
    {
        ScMatrixRef pResultMatrix = SEIC.MatConcat(pMatrix1, pMatrix2);
        if (!pResultMatrix)
            PushNoValue();
        else
            PushMatrix(pResultMatrix);
    }
    else if (pMatrix1 || pMatrix2)
    {
        OUString aScalarString;
        bool bScalarOnLeft = false;
        ScMatrixRef pMatrix = std::move(pMatrix1);
        if (!pMatrix)
        {
            aScalarString = aString1;
            pMatrix = std::move(pMatrix2);
            bScalarOnLeft = true;
        }
        else
            aScalarString = aString2;

        SCSIZE nColumns = 0;
        SCSIZE nRows = 0;
        pMatrix->GetDimensions(nColumns, nRows);
        ScMatrixRef pResultMatrix = GetNewMat(nColumns, nRows, /*bEmpty*/ true);
        if (!pResultMatrix)
        {
            PushIllegalArgument();
            return;
        }

        if (nGlobalError != FormulaError::NONE)
        {
            for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                    pResultMatrix->PutError(nGlobalError, nColumn, nRow);
        }
        else if (bScalarOnLeft)
        {
            for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            {
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    const FormulaError eError = pMatrix->GetErrorIfNotString(nColumn, nRow);
                    if (eError != FormulaError::NONE)
                        pResultMatrix->PutError(eError, nColumn, nRow);
                    else
                    {
                        const OUString aValue
                            = aScalarString + pMatrix->GetString(mrContext, nColumn, nRow).getString();
                        pResultMatrix->PutString(rCalc.mrStrPool.intern(aValue), nColumn, nRow);
                    }
                }
            }
        }
        else
        {
            for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            {
                for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
                {
                    const FormulaError eError = pMatrix->GetErrorIfNotString(nColumn, nRow);
                    if (eError != FormulaError::NONE)
                        pResultMatrix->PutError(eError, nColumn, nRow);
                    else
                    {
                        const OUString aValue
                            = pMatrix->GetString(mrContext, nColumn, nRow).getString() + aScalarString;
                        pResultMatrix->PutString(rCalc.mrStrPool.intern(aValue), nColumn, nRow);
                    }
                }
            }
        }
        PushMatrix(pResultMatrix);
    }
    else
    {
        if (SEIC.CheckStringResultLen(aString1, aString2.getLength()))
            aString1 += aString2;
        SEIC.PushString(aString1);
    }
}

inline void Dispatcher::letKernel(ScInterpreter& rCalc)
{
    const short* pJump = SEIC.pCur->GetJump();
    short nJumpCount = pJump[0];
    const short nOriginalJumpCount = nJumpCount;

    if (nJumpCount < 3 || (nJumpCount % 2 != 1))
    {
        PushError(FormulaError::ParameterExpected);
        SEIC.aCode.Jump(pJump[nOriginalJumpCount], pJump[nOriginalJumpCount]);
        return;
    }

    OUString aName;
    std::unordered_map<OUString, formula::FormulaToken*> aResultIndexes;
    formula::FormulaTokenArrayPlainIterator aIterator(*SEIC.pArr);
    ScTokenArray aValueTokens = SEIC.pArr->CloneValue();

    while (nJumpCount > 1)
    {
        if (nJumpCount == nOriginalJumpCount)
            aName = SEIC.GetString().getString();
        else if ((nOriginalJumpCount - nJumpCount + 1) % 2 == 1)
        {
            aIterator.Jump(pJump[static_cast<short>(nOriginalJumpCount - nJumpCount + 1)] - 1);
            FormulaToken* pToken = aIterator.NextRPN();
            aName = pToken->GetString().getString();
        }
        else
        {
            PushError(FormulaError::ParameterExpected);
            SEIC.aCode.Jump(pJump[nOriginalJumpCount], pJump[nOriginalJumpCount]);
            return;
        }
        --nJumpCount;

        seletexec::replaceNamesToResult(
            aResultIndexes, aValueTokens, pJump[nOriginalJumpCount - nJumpCount],
            pJump[nOriginalJumpCount - nJumpCount + 1]);

        ScTokenArray aTempTokens = seletexec::copyTokenSlice(
            mrDoc, aValueTokens, pJump[nOriginalJumpCount - nJumpCount],
            pJump[nOriginalJumpCount - nJumpCount + 1]);

        if (aTempTokens.GetLen() == 0)
        {
            PushIllegalParameter();
            SEIC.aCode.Jump(pJump[nOriginalJumpCount], pJump[nOriginalJumpCount]);
            return;
        }
        else if (aTempTokens.GetLen() == 1 && aTempTokens.GetArray()[0]->GetOpCode() == ocPush)
        {
            if (!aResultIndexes
                     .insert(std::make_pair(aName, aTempTokens.GetArray()[0]->Clone()))
                     .second)
            {
                PushIllegalParameter();
                SEIC.aCode.Jump(pJump[nOriginalJumpCount], pJump[nOriginalJumpCount]);
                return;
            }
        }
        else
        {
            ScInterpreter aNestedInterpreter(
                mrDoc.GetFormulaCell(SEIC.aPos), mrDoc, mrContext, SEIC.aPos, aValueTokens);
            aNestedInterpreter.aCode.Jump(
                pJump[nOriginalJumpCount - nJumpCount],
                pJump[nOriginalJumpCount - nJumpCount + 1],
                pJump[nOriginalJumpCount - nJumpCount + 1]);
            while (aNestedInterpreter.aCode.HasStacked())
                aNestedInterpreter.aCode.FrontPop();
            aNestedInterpreter.aCode.Lambda(true);

            sfx2::LinkManager aLinkManager(mrDoc.GetDocumentShell());
            aNestedInterpreter.SetLinkManager(&aLinkManager);

            formula::StackVar aInterpreterType = aNestedInterpreter.Interpret();
            if (aInterpreterType == formula::svMatrixCell)
            {
#undef GetMatrix
                ScConstMatrixRef xMatrix(aNestedInterpreter.GetResultToken()->GetMatrix());
#define GetMatrix(...) SEIC.GetMatrix(__VA_ARGS__)
                if (!aResultIndexes
                         .insert(std::make_pair(aName, new ScMatrixToken(xMatrix->Clone())))
                         .second)
                {
                    PushIllegalParameter();
                    SEIC.aCode.Jump(pJump[nOriginalJumpCount], pJump[nOriginalJumpCount]);
                    return;
                }
            }
            else
            {
                const FormulaConstTokenRef& xToken(aNestedInterpreter.GetResultToken());
                if (!aResultIndexes.insert(std::make_pair(aName, xToken->Clone())).second)
                {
                    PushIllegalParameter();
                    SEIC.aCode.Jump(pJump[nOriginalJumpCount], pJump[nOriginalJumpCount]);
                    return;
                }
            }
        }
        --nJumpCount;
    }

    seletexec::replaceNamesToResult(
        aResultIndexes, aValueTokens, pJump[nOriginalJumpCount - nJumpCount],
        pJump[nOriginalJumpCount - nJumpCount + 1]);

    ScInterpreter aNestedInterpreter(
        mrDoc.GetFormulaCell(SEIC.aPos), mrDoc, mrContext, SEIC.aPos, aValueTokens);
    aNestedInterpreter.aCode.Jump(
        pJump[nOriginalJumpCount - nJumpCount],
        pJump[nOriginalJumpCount - nJumpCount + 1],
        pJump[nOriginalJumpCount - nJumpCount + 1]);
    while (aNestedInterpreter.aCode.HasStacked())
        aNestedInterpreter.aCode.FrontPop();
    aNestedInterpreter.aCode.Lambda(true);

    sfx2::LinkManager aLinkManager(mrDoc.GetDocumentShell());
    aNestedInterpreter.SetLinkManager(&aLinkManager);
    const formula::StackVar aInterpreterType = aNestedInterpreter.Interpret();

    if (aInterpreterType == formula::svMatrixCell)
    {
#undef GetMatrix
        ScConstMatrixRef xMatrix(aNestedInterpreter.GetResultToken()->GetMatrix());
#define GetMatrix(...) SEIC.GetMatrix(__VA_ARGS__)
        PushTokenRef(new ScMatrixToken(xMatrix->Clone()));
    }
    else
    {
        const formula::FormulaConstTokenRef& xLambdaResult(aNestedInterpreter.GetResultToken());
        if (xLambdaResult)
        {
            nGlobalError = xLambdaResult->GetError();
            if (nGlobalError == FormulaError::NONE)
                PushTokenRef(xLambdaResult);
            else
                PushError(nGlobalError);
        }
    }

    --nJumpCount;
    SEIC.aCode.Jump(pJump[nOriginalJumpCount], pJump[nOriginalJumpCount]);
}

inline void Dispatcher::intersectTerminal(ScInterpreter& rCalc)
{
    FormulaConstTokenRef xSecond = PopToken();
    FormulaConstTokenRef xFirst = PopToken();

    if (nGlobalError != FormulaError::NONE || !xSecond || !xFirst)
    {
        PushIllegalArgument();
        return;
    }

    const StackVar eFirstType = xFirst->GetType();
    const StackVar eSecondType = xSecond->GetType();
    if (!serefexec::isReferenceOperandType(eFirstType)
        || !serefexec::isReferenceOperandType(eSecondType))
    {
        PushIllegalArgument();
        return;
    }

    const FormulaToken* pFirst = xFirst.get();
    const FormulaToken* pSecond = xSecond.get();
    if (eFirstType == svRefList || eSecondType == svRefList)
    {
        const auto aReferences1 = serefexec::collectReferenceOperandEntries(*pFirst);
        const auto aReferences2 = serefexec::collectReferenceOperandEntries(*pSecond);

        ScTokenRef xResult(new ScRefListToken);
        ScRefList* pResultList = xResult->GetRefList();
        for (const auto& rRef1 : aReferences1)
        {
            const ScAddress aFirstStart = rRef1.Ref1.toAbs(mrDoc, SEIC.aPos);
            const ScAddress aFirstEnd = rRef1.Ref2.toAbs(mrDoc, SEIC.aPos);
            for (const auto& rRef2 : aReferences2)
            {
                const ScAddress aSecondStart = rRef2.Ref1.toAbs(mrDoc, SEIC.aPos);
                const ScAddress aSecondEnd = rRef2.Ref2.toAbs(mrDoc, SEIC.aPos);
                const SCCOL nCol1 = std::max(aFirstStart.Col(), aSecondStart.Col());
                const SCROW nRow1 = std::max(aFirstStart.Row(), aSecondStart.Row());
                const SCTAB nTab1 = std::max(aFirstStart.Tab(), aSecondStart.Tab());
                const SCCOL nCol2 = std::min(aFirstEnd.Col(), aSecondEnd.Col());
                const SCROW nRow2 = std::min(aFirstEnd.Row(), aSecondEnd.Row());
                const SCTAB nTab2 = std::min(aFirstEnd.Tab(), aSecondEnd.Tab());
                if (nCol2 < nCol1 || nRow2 < nRow1 || nTab2 < nTab1)
                    continue;

                ScComplexRefData aRef;
                aRef.InitRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                pResultList->push_back(aRef);
            }
        }

        const std::size_t nResultCount = pResultList->size();
        if (!nResultCount)
            PushError(FormulaError::NoCode);
        else if (nResultCount == 1)
        {
            const ScComplexRefData& rRef = (*pResultList)[0];
            if (rRef.Ref1 == rRef.Ref2)
                SEIC.PushTempToken(new ScSingleRefToken(mrDoc.GetSheetLimits(), rRef.Ref1));
            else
                SEIC.PushTempToken(new ScDoubleRefToken(mrDoc.GetSheetLimits(), rRef));
        }
        else
            PushTokenRef(xResult);
        return;
    }

    const FormulaToken* pTokens[2] = { pFirst, pSecond };
    const StackVar eTypes[2] = { eFirstType, eSecondType };
    SCCOL nColStart[2] = { 0, 0 };
    SCCOL nColEnd[2] = { 0, 0 };
    SCROW nRowStart[2] = { 0, 0 };
    SCROW nRowEnd[2] = { 0, 0 };
    SCTAB nTabStart[2] = { 0, 0 };
    SCTAB nTabEnd[2] = { 0, 0 };
    for (std::size_t i = 0; i < 2; ++i)
    {
        switch (eTypes[i])
        {
            case svSingleRef:
            case svDoubleRef:
            {
                const ScAddress aStart = pTokens[i]->GetSingleRef()->toAbs(mrDoc, SEIC.aPos);
                nColStart[i] = aStart.Col();
                nRowStart[i] = aStart.Row();
                nTabStart[i] = aStart.Tab();
                if (eTypes[i] == svDoubleRef)
                {
                    const ScAddress aEnd = pTokens[i]->GetSingleRef2()->toAbs(mrDoc, SEIC.aPos);
                    nColEnd[i] = aEnd.Col();
                    nRowEnd[i] = aEnd.Row();
                    nTabEnd[i] = aEnd.Tab();
                }
                else
                {
                    nColEnd[i] = nColStart[i];
                    nRowEnd[i] = nRowStart[i];
                    nTabEnd[i] = nTabStart[i];
                }
                break;
            }
            default:
                break;
        }
    }

    const SCCOL nCol1 = std::max(nColStart[0], nColStart[1]);
    const SCROW nRow1 = std::max(nRowStart[0], nRowStart[1]);
    const SCTAB nTab1 = std::max(nTabStart[0], nTabStart[1]);
    const SCCOL nCol2 = std::min(nColEnd[0], nColEnd[1]);
    const SCROW nRow2 = std::min(nRowEnd[0], nRowEnd[1]);
    const SCTAB nTab2 = std::min(nTabEnd[0], nTabEnd[1]);
    if (nCol2 < nCol1 || nRow2 < nRow1 || nTab2 < nTab1)
        PushError(FormulaError::NoCode);
    else if (nCol2 == nCol1 && nRow2 == nRow1 && nTab2 == nTab1)
        SEIC.PushSingleRef(nCol1, nRow1, nTab1);
    else
        SEIC.PushDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
}

inline void Dispatcher::rangeReferenceTerminal(ScInterpreter& rCalc)
{
    FormulaConstTokenRef xSecond = PopToken();
    FormulaConstTokenRef xFirst = PopToken();

    if (nGlobalError != FormulaError::NONE || !xSecond || !xFirst)
    {
        PushIllegalArgument();
        return;
    }

    FormulaTokenRef xResult = rangeReferenceToken(rCalc, *xFirst, *xSecond);
    if (!xResult)
        PushIllegalArgument();
    else
        PushTokenRef(xResult);
}

inline void Dispatcher::unionTerminal(ScInterpreter& rCalc)
{
    FormulaConstTokenRef xSecond = PopToken();
    FormulaConstTokenRef xFirst = PopToken();

    if (nGlobalError != FormulaError::NONE || !xSecond || !xFirst)
    {
        PushIllegalArgument();
        return;
    }

    const StackVar eFirstType = xFirst->GetType();
    const StackVar eSecondType = xSecond->GetType();
    if (!serefexec::isReferenceOperandType(eFirstType)
        || !serefexec::isReferenceOperandType(eSecondType))
    {
        PushIllegalArgument();
        return;
    }

    const FormulaToken* pFirst = xFirst.get();
    const FormulaToken* pSecond = xSecond.get();
    ScTokenRef xResult;
    bool bHandledFirst = false;
    bool bHandledSecond = false;
    if (eFirstType == svRefList)
    {
        xResult = pFirst->Clone();
        bHandledFirst = true;
    }
    else if (eSecondType == svRefList)
    {
        xResult = pSecond->Clone();
        bHandledSecond = true;
    }
    else
        xResult = new ScRefListToken;

    ScRefList* pResultList = xResult->GetRefList();
    if (!bHandledFirst)
        serefexec::appendReferenceOperandEntries(*pResultList, *pFirst);
    if (!bHandledSecond)
        serefexec::appendReferenceOperandEntries(*pResultList, *pSecond);
    SEIC.ValidateRef(*pResultList);
    PushTokenRef(xResult);
}

inline void Dispatcher::multiAreaTerminal(ScInterpreter& rCalc)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCountMin(nParamCount, 1))
        return;

    while (nGlobalError == FormulaError::NONE && nParamCount-- > 1)
        unionTerminal(rCalc);
}

inline void Dispatcher::missingTerminal(ScInterpreter& rCalc)
{
    if (SEIC.aCode.IsEndOfPath())
        SEIC.PushTempToken(new ScEmptyCellToken(false, false));
    else
        SEIC.PushTempToken(new FormulaMissingToken);
}

inline void Dispatcher::lookupTerminal(ScInterpreter& rCalc)
{
    const sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, 3))
        return;

    selookupexec::LegacyLookupRequest aRequest;
    if (nParamCount == 3)
    {
        const auto aResultInput = SEIC.PopLookupExecutionInput(true, true);
        if (!aResultInput)
        {
            PushIllegalParameter();
            return;
        }
        aRequest.moResultInput = aResultInput.maValue;
    }

    const auto aDataInput = SEIC.PopLookupExecutionInput(true, false);
    if (!aDataInput)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maDataInput = aDataInput.maValue;

    const auto aLookupValue = SEIC.PopLookupExecutionValue(false);
    if (!aLookupValue)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maLookupValue = aLookupValue.maValue;
    if (aRequest.maLookupValue.isText())
    {
        aRequest.meSearchType = toLookupSearchType(SEIC.DetectSearchType(
            selibreoffice::toLibreOfficeString(aRequest.maLookupValue.maString), mrDoc));
    }

    const auto aResult = selookupexec::resolveLookupResult(mrDoc, mrContext, aRequest);
    if (!aResult)
    {
        if (aResult.meError == spreadsheetengine::api::Error::NotAvailable)
            PushNA();
        else
            PushError(selibreoffice::toFormulaError(aResult.meError));
        return;
    }

    SEIC.PushLookupExecutionResult(aResult.maValue, false);
}

inline void Dispatcher::xlookupTerminal(ScInterpreter& rCalc)
{
    const sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 3, 6))
        return;

    selookupexec::XLookupExecutionRequest aRequest;
    aRequest.mbAllowPatternMatch = true;

    if (nParamCount == 6)
    {
        const auto aSearchMode = selookup::normalizeSearchMode(SEIC.GetInt16());
        if (!aSearchMode)
        {
            PushIllegalParameter();
            return;
        }
        aRequest.meSearchMode = aSearchMode.maValue;
    }

    if (nParamCount >= 5)
    {
        const auto aMatchMode = selookup::normalizeExtendedMatchMode(SEIC.GetInt16());
        if (!aMatchMode)
        {
            PushIllegalParameter();
            return;
        }
        aRequest.meMatchMode = aMatchMode.maValue;
    }

    FormulaConstTokenRef xNotFound;
    FormulaError eFirstMatchError = FormulaError::NONE;
    if (nParamCount >= 4 && GetStackType() != svEmptyCell)
    {
        xNotFound = PopToken();
        eFirstMatchError = xNotFound->GetError();
        nGlobalError = FormulaError::NONE;
    }

    const auto aResultInput = SEIC.PopLookupExecutionInput(false, false);
    if (!aResultInput)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maResultInput = aResultInput.maValue;

    const auto aSearchInput = SEIC.PopLookupExecutionInput(false, true);
    if (!aSearchInput)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maSearchInput = aSearchInput.maValue;

    const auto aLookupValue = SEIC.PopLookupExecutionValue(true, true);
    if (!aLookupValue)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maLookupValue = aLookupValue.maValue;
    if (aRequest.maLookupValue.isText())
    {
        aRequest.meSearchType = toLookupSearchType(SEIC.DetectSearchType(
            selibreoffice::toLibreOfficeString(aRequest.maLookupValue.maString), mrDoc));
    }

    const auto aResult = selookupexec::resolveXLookupResult(mrDoc, mrContext, aRequest);
    if (!aResult)
    {
        if (aResult.meError == spreadsheetengine::api::Error::NotAvailable)
        {
            if (xNotFound && xNotFound->GetType() != svMissing)
            {
                nGlobalError = eFirstMatchError;
                PushTokenRef(xNotFound);
            }
            else
                PushNA();
        }
        else if (aResult.meError == spreadsheetengine::api::Error::NoValue)
            PushNoValue();
        else
            PushError(selibreoffice::toFormulaError(aResult.meError));
        return;
    }

    SEIC.PushLookupExecutionResult(aResult.maValue, true);
}

inline void Dispatcher::matchOperation(ScInterpreter& rCalc, bool bExtended)
{
    const sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, bExtended ? 4 : 3))
        return;

    selookupexec::MatchExecutionRequest aRequest;
    aRequest.mbExtended = bExtended;
    aRequest.mbAllowPatternMatch = bExtended;
    aRequest.meSearchType = toLookupSearchType(mrDoc.GetDocOptions().GetFormulaSearchType());

    if (bExtended)
    {
        if (nParamCount == 4)
        {
            const auto aSearchMode = selookup::normalizeSearchMode(SEIC.GetInt16());
            if (!aSearchMode)
            {
                PushIllegalParameter();
                return;
            }
            aRequest.meSearchMode = aSearchMode.maValue;
        }

        if (nParamCount >= 3)
        {
            const auto aMatchMode = selookup::normalizeExtendedMatchMode(SEIC.GetInt16());
            if (!aMatchMode)
            {
                PushIllegalParameter();
                return;
            }
            aRequest.meMatchMode = aMatchMode.maValue;
        }
    }
    else
    {
        const auto aModePlan = selookup::normalizeMatchType(nParamCount == 3 ? SEIC.GetDouble() : 1.0);
        if (!aModePlan)
        {
            PushIllegalParameter();
            return;
        }
        aRequest.maLegacyModes = aModePlan.maValue;
    }

    switch (GetStackType())
    {
        case svSingleRef:
        {
            SCCOL nCol = 0;
            SCROW nRow = 0;
            SCTAB nTab = 0;
            PopSingleRef(nCol, nRow, nTab);
            aRequest.maSearchSource.moRange = ScRange(nCol, nRow, nTab, nCol, nRow, nTab);
            break;
        }
        case svDoubleRef:
        {
            SCCOL nCol1 = 0;
            SCROW nRow1 = 0;
            SCTAB nTab1 = 0;
            SCCOL nCol2 = 0;
            SCROW nRow2 = 0;
            SCTAB nTab2 = 0;
            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
            if (nTab1 != nTab2 || (nCol1 != nCol2 && nRow1 != nRow2))
            {
                PushIllegalParameter();
                return;
            }
            aRequest.maSearchSource.moRange = ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
            break;
        }
        case svMatrix:
        {
            aRequest.maSearchSource.mpMatrix = SEIC.PopMatrix();
            if (!aRequest.maSearchSource.mpMatrix)
            {
                PushIllegalParameter();
                return;
            }
            break;
        }
        case svExternalDoubleRef:
            SEIC.PopExternalDoubleRef(aRequest.maSearchSource.mpMatrix);
            if (!aRequest.maSearchSource.mpMatrix)
            {
                PushIllegalParameter();
                return;
            }
            break;
        default:
            PushIllegalParameter();
            return;
    }

    if (nGlobalError != FormulaError::NONE)
    {
        PushIllegalParameter();
        return;
    }

    const auto makeTextValue = [](const OUString& rText) {
        return spreadsheetengine::api::CellValue::text(selibreoffice::toApiString(rText));
    };

    switch (bExtended ? SEIC.GetRawStackType() : GetStackType())
    {
        case svMissing:
        case svEmptyCell:
            if (!bExtended)
            {
                PushIllegalParameter();
                return;
            }
            Pop();
            aRequest.maLookupValue = spreadsheetengine::api::CellValue::empty();
            break;
        case svDouble:
            aRequest.maLookupValue = spreadsheetengine::api::CellValue::number(SEIC.GetDouble());
            break;
        case svString:
            aRequest.maLookupValue = makeTextValue(SEIC.GetString().getString());
            break;
        case svDoubleRef:
        case svSingleRef:
        {
            ScAddress aAddress;
            if (!SEIC.PopDoubleRefOrSingleRef(aAddress))
            {
                SEIC.PushInt(0);
                return;
            }

            ScRefCellValue aCell(mrDoc, aAddress);
            if (aCell.hasNumeric())
            {
                aRequest.maLookupValue
                    = spreadsheetengine::api::CellValue::number(GetCellValue(aAddress, aCell));
            }
            else
            {
                svl::SharedString aString;
                SEIC.GetCellString(aString, aCell);
                aRequest.maLookupValue = makeTextValue(aString.getString());
            }
            break;
        }
        case svExternalSingleRef:
        {
            ScExternalRefCache::TokenRef xToken;
            SEIC.PopExternalSingleRef(xToken);
            if (nGlobalError != FormulaError::NONE)
            {
                PushError(nGlobalError);
                return;
            }
            if (xToken->GetType() == svDouble)
                aRequest.maLookupValue
                    = spreadsheetengine::api::CellValue::number(xToken->GetDouble());
            else
                aRequest.maLookupValue = makeTextValue(xToken->GetString().getString());
            break;
        }
        case svExternalDoubleRef:
        case svMatrix:
        {
            double fValue = 0.0;
            svl::SharedString aString;
            const ScMatValType eType = SEIC.GetDoubleOrStringFromMatrix(fValue, aString);
            if (nGlobalError != FormulaError::NONE)
            {
                PushError(nGlobalError);
                return;
            }

            if (ScMatrix::IsNonValueType(eType))
                aRequest.maLookupValue = makeTextValue(aString.getString());
            else if (ScMatrix::IsBooleanType(eType))
                aRequest.maLookupValue = spreadsheetengine::api::CellValue::boolean(fValue != 0.0);
            else
                aRequest.maLookupValue = spreadsheetengine::api::CellValue::number(fValue);
            break;
        }
        default:
            PushIllegalParameter();
            return;
    }

    const auto aResolvedIndex = selookupexec::resolveMatchIndex(mrDoc, mrContext, aRequest);
    if (!aResolvedIndex)
    {
        if (aResolvedIndex.meError == spreadsheetengine::api::Error::NotAvailable)
            PushNA();
        else
            PushError(selibreoffice::toFormulaError(aResolvedIndex.meError));
        return;
    }

    PushDouble(static_cast<double>(aResolvedIndex.maValue + 1));
}

inline void Dispatcher::indirectTerminal(ScInterpreter& rCalc)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 1, 2))
        return;

    const bool bForceR1C1 = (nParamCount == 2 && 0.0 == SEIC.GetDouble());
    const auto aSyntaxPolicy = sestringref::resolveIndirectAddressSyntaxPolicy(
        selibreoffice::toApiAddressConvention(SEIC.maCalcConfig.meStringRefAddressSyntax),
        selibreoffice::toApiAddressConvention(mrDoc.GetAddressConvention()),
        SEIC.maCalcConfig.meStringRefAddressSyntax == FormulaGrammar::CONV_A1_XL_A1,
        bForceR1C1);
    FormulaGrammar::AddressConvention eConvention
        = selibreoffice::toLibreOfficeAddressConvention(aSyntaxPolicy.mePrimary);
    const bool bTryXlA1
        = aSyntaxPolicy.moFallback == spreadsheetengine::api::AddressConvention::XlA1;

    svl::SharedString aSharedRef = SEIC.GetString();
    if (aSharedRef.getString().isEmpty())
    {
        PushError(FormulaError::NoRef);
        return;
    }

    const auto oResolved = seindirectexec::resolveIndirectReference(
        mrDoc, SEIC.aPos, aSharedRef, eConvention, bTryXlA1);
    if (!oResolved)
    {
        PushError(FormulaError::NoRef);
        return;
    }

    switch (oResolved->meKind)
    {
        case seindirectexec::IndirectExecutionResult::Kind::SingleRef:
            SEIC.PushSingleRef(oResolved->maRef1);
            return;
        case seindirectexec::IndirectExecutionResult::Kind::DoubleRef:
            SEIC.PushDoubleRef(oResolved->maRef1, oResolved->maRef2);
            return;
        case seindirectexec::IndirectExecutionResult::Kind::ExternalSingleRef:
            SEIC.PushExternalSingleRef(oResolved->mnFileId, oResolved->maTabName,
                oResolved->maRef1.Col(), oResolved->maRef1.Row(), oResolved->maRef1.Tab());
            return;
        case seindirectexec::IndirectExecutionResult::Kind::ExternalDoubleRef:
            SEIC.PushExternalDoubleRef(oResolved->mnFileId, oResolved->maTabName,
                oResolved->maRef1.Col(), oResolved->maRef1.Row(), oResolved->maRef1.Tab(),
                oResolved->maRef2.Col(), oResolved->maRef2.Row(), oResolved->maRef2.Tab());
            return;
        case seindirectexec::IndirectExecutionResult::Kind::Token:
            PushTokenRef(oResolved->mxToken);
            return;
    }
}

inline void Dispatcher::addressTerminal(ScInterpreter& rCalc)
{
    OUString aSheetToken;

    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, 5))
        return;

    if (nParamCount >= 5)
        aSheetToken = SEIC.GetString().getString();

    const bool bForceR1C1 = (nParamCount >= 4 && 0.0 == GetDoubleWithDefault(1.0));
    const FormulaGrammar::AddressConvention eConvention
        = selibreoffice::toLibreOfficeAddressConvention(
            sestringref::resolveAddressFunctionConvention(
                selibreoffice::toApiAddressConvention(SEIC.maCalcConfig.meStringRefAddressSyntax),
                selibreoffice::toApiAddressConvention(mrDoc.GetAddressConvention()),
                bForceR1C1));

    ScRefFlags nFlags = ScRefFlags::COL_ABS | ScRefFlags::ROW_ABS;
    sal_Int32 nAbsMode = 1;
    if (nParamCount >= 3)
    {
        const sal_Int32 nMode = SEIC.GetInt32WithDefault(1);
        switch (nMode)
        {
            default:
                PushNoValue();
                return;
            case 5:
            case 1:
                nAbsMode = nMode;
                break;
            case 6:
            case 2:
                nAbsMode = nMode;
                nFlags = ScRefFlags::ROW_ABS;
                break;
            case 7:
            case 3:
                nAbsMode = nMode;
                nFlags = ScRefFlags::COL_ABS;
                break;
            case 8:
            case 4:
                nAbsMode = nMode;
                nFlags = ScRefFlags::ZERO;
                break;
        }
    }
    nFlags |= ScRefFlags::VALID | ScRefFlags::ROW_VALID | ScRefFlags::COL_VALID;

    SCCOL nCol = static_cast<SCCOL>(SEIC.GetInt16());
    SCROW nRow = static_cast<SCROW>(GetInt32());
    if (eConvention == FormulaGrammar::CONV_XL_R1C1)
    {
        if (!(nFlags & ScRefFlags::COL_ABS))
            nCol += SEIC.aPos.Col() + 1;
        if (!(nFlags & ScRefFlags::ROW_ABS))
            nRow += SEIC.aPos.Row() + 1;
    }

    --nCol;
    --nRow;
    if (nGlobalError != FormulaError::NONE || !mrDoc.ValidCol(nCol) || !mrDoc.ValidRow(nRow))
    {
        PushIllegalArgument();
        return;
    }

    serefexec::AddressFunctionRequest aRequest;
    aRequest.mnRow = nRow;
    aRequest.mnColumn = nCol;
    aRequest.mnAbsMode = nAbsMode;
    aRequest.mbA1Style = eConvention != FormulaGrammar::CONV_XL_R1C1;
    aRequest.maSheetToken = aSheetToken;
    aRequest.meConvention = eConvention;
    const auto aAddress = serefexec::formatAddressFunctionResult(aRequest);
    if (!aAddress)
        PushIllegalArgument();
    else
        SEIC.PushString(aAddress.maValue);
}

inline void Dispatcher::indexTerminal(ScInterpreter& rCalc)
{
    const sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 1, 4))
        return;

    sal_Int32 nArea = (nParamCount == 4) ? GetInt32() : 1;
    std::size_t nAreaCount = 0;
    SCCOL nCol = 0;
    SCROW nRow = 0;
    bool bColMissing = false;
    if (nParamCount >= 3)
    {
        bColMissing = IsMissing();
        nCol = static_cast<SCCOL>(SEIC.GetInt16());
    }
    if (nParamCount >= 2)
        nRow = static_cast<SCROW>(GetInt32());

    if (nArea < 1 || nCol < 0 || nRow < 0)
    {
        PushIllegalArgument();
        return;
    }

    if (GetStackType() == svRefList)
        nAreaCount = (sp ? pStack[sp - 1]->GetRefList()->size() : 0);
    else
        nAreaCount = 1;

    const auto aAreaSelection = serefexec::normalizeAreaSelection(nArea, nAreaCount);
    if (nGlobalError != FormulaError::NONE || !aAreaSelection)
    {
        PushError(FormulaError::NoRef);
        return;
    }

    switch (GetStackType())
    {
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            const sal_uInt16 nOldSp = sp;
            ScMatrixRef pMatrix = GetMatrix();
            if (!pMatrix)
                PushError(FormulaError::NoRef);
            else
            {
                SCSIZE nColumns = 0;
                SCSIZE nRows = 0;
                pMatrix->GetDimensions(nColumns, nRows);
                const auto aSelection = seref::planIndexMatrixSelection(
                    { static_cast<sal_Int32>(nColumns), static_cast<sal_Int32>(nRows) }, nRow,
                    nCol, bColMissing, nParamCount);
                if (!aSelection)
                    PushError(FormulaError::NoRef);
                else if (aSelection.maValue.meKind == seref::IndexSelectionKind::KeepSource)
                    sp = nOldSp;
                else if (aSelection.maValue.meKind == seref::IndexSelectionKind::Scalar)
                {
                    const SCSIZE nMatrixColumn
                        = static_cast<SCSIZE>(aSelection.maValue.maStart.mnColumn);
                    const SCSIZE nMatrixRow
                        = static_cast<SCSIZE>(aSelection.maValue.maStart.mnRow);
                    if (pMatrix->IsStringOrEmpty(nMatrixColumn, nMatrixRow))
                        SEIC.PushString(pMatrix->GetString(nMatrixColumn, nMatrixRow).getString());
                    else
                        PushDouble(pMatrix->GetDouble(nMatrixColumn, nMatrixRow));
                }
                else
                {
                    const auto& rSelection = aSelection.maValue;
                    ScMatrixRef pResultMatrix = GetNewMat(
                        rSelection.maDimensions.mnColumns, rSelection.maDimensions.mnRows,
                        /*bEmpty*/ true);
                    if (pResultMatrix)
                    {
                        for (SCSIZE nResultRow = 0;
                             nResultRow < static_cast<SCSIZE>(rSelection.maDimensions.mnRows);
                             ++nResultRow)
                        {
                            for (SCSIZE nResultCol = 0;
                                 nResultCol
                                 < static_cast<SCSIZE>(rSelection.maDimensions.mnColumns);
                                 ++nResultCol)
                            {
                                const SCSIZE nMatrixColumn = static_cast<SCSIZE>(
                                    rSelection.maStart.mnColumn + nResultCol);
                                const SCSIZE nMatrixRow = static_cast<SCSIZE>(
                                    rSelection.maStart.mnRow + nResultRow);
                                if (!pMatrix->IsStringOrEmpty(nMatrixColumn, nMatrixRow))
                                {
                                    pResultMatrix->PutDouble(
                                        pMatrix->GetDouble(nMatrixColumn, nMatrixRow), nResultCol,
                                        nResultRow);
                                }
                                else
                                {
                                    pResultMatrix->PutString(
                                        pMatrix->GetString(nMatrixColumn, nMatrixRow), nResultCol,
                                        nResultRow);
                                }
                            }
                        }
                        PushMatrix(pResultMatrix);
                    }
                    else
                        PushError(FormulaError::NoRef);
                }
            }
            break;
        }
        case svSingleRef:
        {
            SCCOL nCol1 = 0;
            SCROW nRow1 = 0;
            SCTAB nTab1 = 0;
            PopSingleRef(nCol1, nRow1, nTab1);
            const auto aSelection = serefexec::planIndexReferenceSelection(
                ScRange(nCol1, nRow1, nTab1, nCol1, nRow1, nTab1), nRow, nCol, nParamCount);
            if (!aSelection)
                PushError(FormulaError::NoRef);
            else
            {
                const ScRange aRange
                    = selibreoffice::toLibreOfficeRange(aSelection.maValue.maRange);
                SEIC.PushSingleRef(aRange.aStart.Col(), aRange.aStart.Row(), aRange.aStart.Tab());
            }
            break;
        }
        case svDoubleRef:
        case svRefList:
        {
            SCCOL nCol1 = 0;
            SCROW nRow1 = 0;
            SCTAB nTab1 = 0;
            SCCOL nCol2 = 0;
            SCROW nRow2 = 0;
            SCTAB nTab2 = 0;
            if (GetStackType() == svRefList)
            {
                FormulaConstTokenRef xRef = PopToken();
                if (nGlobalError != FormulaError::NONE || !xRef)
                {
                    PushError(FormulaError::NoRef);
                    return;
                }
                ScRange aRange(ScAddress::UNINITIALIZED);
                SEIC.DoubleRefToRange((*(xRef->GetRefList()))[aAreaSelection.maValue], aRange);
                aRange.GetVars(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
            }
            else
                PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);

            const auto aSelection = serefexec::planIndexReferenceSelection(
                ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2), nRow, nCol, nParamCount);
            if (!aSelection)
                PushError(FormulaError::NoRef);
            else
            {
                const ScRange aRange
                    = selibreoffice::toLibreOfficeRange(aSelection.maValue.maRange);
                if (aSelection.maValue.maRange.isSingleCell())
                {
                    SEIC.PushSingleRef(
                        aRange.aStart.Col(), aRange.aStart.Row(), aRange.aStart.Tab());
                }
                else
                {
                    SEIC.PushDoubleRef(
                        aRange.aStart.Col(), aRange.aStart.Row(), aRange.aStart.Tab(),
                        aRange.aEnd.Col(), aRange.aEnd.Row(), aRange.aEnd.Tab());
                }
            }
            break;
        }
        default:
            PopError();
            PushError(FormulaError::NoRef);
            break;
    }
}

inline void Dispatcher::aggregateSumProduct(ScInterpreter& rCalc)
{
    short nParamCount = GetByte();
    if (!MustHaveParamCountMin(nParamCount, 1))
        return;

    size_t nInRefList = 0;
    ScMatrixRef pMatLast = GetMatrix(--nParamCount, nInRefList);
    if (!pMatLast)
    {
        PushIllegalParameter();
        return;
    }

    SCSIZE nCLast = 0;
    SCSIZE nRLast = 0;
    pMatLast->GetDimensions(nCLast, nRLast);
    std::vector<double> aResArray;
    pMatLast->GetDoubleArray(aResArray);

    while (nParamCount--)
    {
        ScMatrixRef pMat = GetMatrix(nParamCount, nInRefList);
        if (!pMat)
        {
            PushIllegalParameter();
            return;
        }
        SCSIZE nC = 0;
        SCSIZE nR = 0;
        pMat->GetDimensions(nC, nR);
        if (nC != nCLast || nR != nRLast)
        {
            PushNoValue();
            return;
        }

        pMat->MergeDoubleArrayMultiply(aResArray);
    }

    KahanSum fSum = 0.0;
    for (double fPosArray : aResArray)
    {
        FormulaError nErr = ::GetDoubleErrorValue(fPosArray);
        if (nErr == FormulaError::NONE)
            fSum += fPosArray;
        else if (nErr != FormulaError::ElementNaN)
        {
            PushError(nErr);
            return;
        }
    }

    PushDouble(fSum.get());
}

inline void Dispatcher::aggregateSumSq(ScInterpreter& rCalc) { IterateParameters(ifSUMSQ); }

inline void Dispatcher::aggregateSum(ScInterpreter& rCalc) { IterateParameters(ifSUM); }

inline void Dispatcher::aggregateProduct(ScInterpreter& rCalc) { IterateParameters(ifPRODUCT); }

inline void Dispatcher::aggregateAverage(ScInterpreter& rCalc, bool bTextAsZero)
{
    IterateParameters(ifAVERAGE, bTextAsZero);
}

inline void Dispatcher::aggregateMin(ScInterpreter& rCalc, bool bTextAsZero)
{
    short nParamCount = GetByte();
    if (!MustHaveParamCountMin(nParamCount, 1))
        return;

    ScMatrixRef xResMat;
    double nMin = ::std::numeric_limits<double>::max();
    auto MatOpFunc = [&xResMat](SCSIZE i, double fCurMin) {
        double fVecRes = xResMat->GetDouble(0, i);
        if (fVecRes > fCurMin)
            xResMat->PutDouble(fCurMin, 0, i);
    };
    const SCSIZE nMatRows = GetRefListArrayMaxSize(nParamCount);
    size_t nRefArrayPos = std::numeric_limits<size_t>::max();

    double nVal = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble:
            {
                nVal = CalcGetDouble();
                if (nMin > nVal)
                    nMin = nVal;
                nFuncFmtType = SvNumFormatType::NUMBER;
            }
            break;
            case svSingleRef:
            {
                PopSingleRef(aAdr);
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasNumeric())
                {
                    nVal = GetCellValue(aAdr, aCell);
                    CurFmtToFuncFmt();
                    if (nMin > nVal)
                        nMin = nVal;
                }
                else if (bTextAsZero && aCell.hasString())
                {
                    if (nMin > 0.0)
                        nMin = 0.0;
                }
            }
            break;
            case svRefList:
            {
                if (SwitchToArrayRefList(xResMat, nMatRows, nMin, MatOpFunc,
                                         nRefArrayPos == std::numeric_limits<size_t>::max()))
                    nRefArrayPos = nRefInList;
            }
            [[fallthrough]];
            case svDoubleRef:
            {
                FormulaError nErr = FormulaError::NONE;
                PopDoubleRef(aRange, nParamCount, nRefInList);
                ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags, bTextAsZero);
                if (aValIter.GetFirst(nVal, nErr))
                {
                    if (nMin > nVal)
                        nMin = nVal;
                    aValIter.GetCurNumFmtInfo(nFuncFmtType, nFuncFmtIndex);
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(nVal, nErr))
                    {
                        if (nMin > nVal)
                            nMin = nVal;
                    }
                    SetError(nErr);
                }
                if (nRefArrayPos != std::numeric_limits<size_t>::max())
                {
                    MatOpFunc(nRefArrayPos, nMin);
                    nMin = std::numeric_limits<double>::max();
                    nVal = 0.0;
                    nRefArrayPos = std::numeric_limits<size_t>::max();
                }
            }
            break;
            case svMatrix:
            case svExternalSingleRef:
            case svExternalDoubleRef:
            {
                ScMatrixRef pMat = GetMatrix();
                if (pMat)
                {
                    nFuncFmtType = SvNumFormatType::NUMBER;
                    nVal = pMat->GetMinValue(
                        bTextAsZero, bool(mnSubTotalFlags & SubtotalFlags::IgnoreErrVal));
                    if (nMin > nVal)
                        nMin = nVal;
                }
            }
            break;
            case svString:
            {
                Pop();
                if (bTextAsZero)
                {
                    if (nMin > 0.0)
                        nMin = 0.0;
                }
                else
                    SetError(FormulaError::IllegalParameter);
            }
            break;
            default:
                PopError();
                SetError(FormulaError::IllegalParameter);
        }
    }

    if (xResMat)
    {
        if (nMin < std::numeric_limits<double>::max())
        {
            for (SCSIZE i = 0; i < nMatRows; ++i)
                MatOpFunc(i, nMin);
        }
        else
        {
            for (SCSIZE i = 0; i < nMatRows; ++i)
            {
                double fVecRes = xResMat->GetDouble(0, i);
                if (fVecRes == std::numeric_limits<double>::max())
                    xResMat->PutDouble(0.0, 0, i);
            }
        }
        PushMatrix(xResMat);
    }
    else if (!std::isfinite(nVal))
        PushError(::GetDoubleErrorValue(nVal));
    else if (nVal < nMin)
        PushDouble(0.0);
    else
        PushDouble(nMin);
}

inline void Dispatcher::aggregateMax(ScInterpreter& rCalc, bool bTextAsZero)
{
    short nParamCount = GetByte();
    if (!MustHaveParamCountMin(nParamCount, 1))
        return;

    ScMatrixRef xResMat;
    double nMax = std::numeric_limits<double>::lowest();
    auto MatOpFunc = [&xResMat](SCSIZE i, double fCurMax) {
        double fVecRes = xResMat->GetDouble(0, i);
        if (fVecRes < fCurMax)
            xResMat->PutDouble(fCurMax, 0, i);
    };
    const SCSIZE nMatRows = GetRefListArrayMaxSize(nParamCount);
    size_t nRefArrayPos = std::numeric_limits<size_t>::max();

    double nVal = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble:
            {
                nVal = CalcGetDouble();
                if (nMax < nVal)
                    nMax = nVal;
                nFuncFmtType = SvNumFormatType::NUMBER;
            }
            break;
            case svSingleRef:
            {
                PopSingleRef(aAdr);
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasNumeric())
                {
                    nVal = GetCellValue(aAdr, aCell);
                    CurFmtToFuncFmt();
                    if (nMax < nVal)
                        nMax = nVal;
                }
                else if (bTextAsZero && aCell.hasString())
                {
                    if (nMax < 0.0)
                        nMax = 0.0;
                }
            }
            break;
            case svRefList:
            {
                if (SwitchToArrayRefList(xResMat, nMatRows, nMax, MatOpFunc,
                                         nRefArrayPos == std::numeric_limits<size_t>::max()))
                    nRefArrayPos = nRefInList;
            }
            [[fallthrough]];
            case svDoubleRef:
            {
                FormulaError nErr = FormulaError::NONE;
                PopDoubleRef(aRange, nParamCount, nRefInList);
                ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags, bTextAsZero);
                if (aValIter.GetFirst(nVal, nErr))
                {
                    if (nMax < nVal)
                        nMax = nVal;
                    aValIter.GetCurNumFmtInfo(nFuncFmtType, nFuncFmtIndex);
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(nVal, nErr))
                    {
                        if (nMax < nVal)
                            nMax = nVal;
                    }
                    SetError(nErr);
                }
                if (nRefArrayPos != std::numeric_limits<size_t>::max())
                {
                    MatOpFunc(nRefArrayPos, nMax);
                    nMax = std::numeric_limits<double>::lowest();
                    nVal = 0.0;
                    nRefArrayPos = std::numeric_limits<size_t>::max();
                }
            }
            break;
            case svMatrix:
            case svExternalSingleRef:
            case svExternalDoubleRef:
            {
                ScMatrixRef pMat = GetMatrix();
                if (pMat)
                {
                    nFuncFmtType = SvNumFormatType::NUMBER;
                    nVal = pMat->GetMaxValue(
                        bTextAsZero, bool(mnSubTotalFlags & SubtotalFlags::IgnoreErrVal));
                    if (nMax < nVal)
                        nMax = nVal;
                }
            }
            break;
            case svString:
            {
                Pop();
                if (bTextAsZero)
                {
                    if (nMax < 0.0)
                        nMax = 0.0;
                }
                else
                    SetError(FormulaError::IllegalParameter);
            }
            break;
            default:
                PopError();
                SetError(FormulaError::IllegalParameter);
        }
    }

    if (xResMat)
    {
        if (nMax > std::numeric_limits<double>::lowest())
        {
            for (SCSIZE i = 0; i < nMatRows; ++i)
                MatOpFunc(i, nMax);
        }
        else
        {
            for (SCSIZE i = 0; i < nMatRows; ++i)
            {
                double fVecRes = xResMat->GetDouble(0, i);
                if (fVecRes == -std::numeric_limits<double>::max())
                    xResMat->PutDouble(0.0, 0, i);
            }
        }
        PushMatrix(xResMat);
    }
    else if (!std::isfinite(nVal))
        PushError(::GetDoubleErrorValue(nVal));
    else if (nVal > nMax)
        PushDouble(0.0);
    else
        PushDouble(nMax);
}

inline void Dispatcher::aggregateVar(ScInterpreter& rCalc, bool bTextAsZero)
{
    auto VarResult = [](double fVal, size_t nValCount) {
        if (nValCount <= 1)
            return CreateDoubleError(FormulaError::DivisionByZero);
        return fVal / (nValCount - 1);
    };
    GetStVarParams(bTextAsZero, VarResult);
}

inline void Dispatcher::aggregateVarP(ScInterpreter& rCalc, bool bTextAsZero)
{
    auto VarResult = [](double fVal, size_t nValCount) { return sc::div(fVal, nValCount); };
    GetStVarParams(bTextAsZero, VarResult);
}

inline void Dispatcher::aggregateStDev(ScInterpreter& rCalc, bool bTextAsZero)
{
    auto VarResult = [](double fVal, size_t nValCount) {
        if (nValCount <= 1)
            return CreateDoubleError(FormulaError::DivisionByZero);
        return sqrt(fVal / (nValCount - 1));
    };
    GetStVarParams(bTextAsZero, VarResult);
}

inline void Dispatcher::aggregateStDevP(ScInterpreter& rCalc, bool bTextAsZero)
{
    auto VarResult = [](double fVal, size_t nValCount) {
        if (nValCount == 0)
            return CreateDoubleError(FormulaError::DivisionByZero);
        return sqrt(fVal / nValCount);
    };
    GetStVarParams(bTextAsZero, VarResult);
}

inline void Dispatcher::matrixDeterminant(ScInterpreter& rCalc)
{
    if (!MustHaveParamCount(GetByte(), 1))
        return;

    ScMatrixRef pMat = GetMatrix();
    if (!pMat)
    {
        PushIllegalParameter();
        return;
    }
    if (!pMat->IsNumeric())
    {
        PushNoValue();
        return;
    }

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    pMat->GetDimensions(nColumns, nRows);
    if (nColumns != nRows || nColumns == 0)
    {
        PushIllegalArgument();
        return;
    }
    if (!ScMatrix::IsSizeAllocatable(nColumns, nRows))
    {
        PushError(FormulaError::MatrixSize);
        return;
    }

    std::vector<double> aValues;
    aValues.reserve(nColumns * nRows);
    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            aValues.push_back(pMat->GetDouble(nColumn, nRow));
    }

    const auto aDeterminant
        = spreadsheetengine::core::math::evaluateMatrixDeterminant(
            aValues, static_cast<std::size_t>(nColumns));
    if (!aDeterminant)
        PushError(selibreoffice::toFormulaError(aDeterminant.meError));
    else
        PushDouble(aDeterminant.maValue);
}

inline void Dispatcher::matrixSumXMY2(ScInterpreter& rCalc)
{
    if (!MustHaveParamCount(GetByte(), 2))
        return;

    ScMatrixRef pRight = GetMatrix();
    ScMatrixRef pLeft = GetMatrix();
    if (!pRight || !pLeft)
    {
        PushIllegalParameter();
        return;
    }

    const auto oLeft = matrixRefToMatrixOperand(pLeft);
    const auto oRight = matrixRefToMatrixOperand(pRight);
    if (!oLeft || !oRight)
    {
        PushIllegalParameter();
        return;
    }

    const auto aResult
        = serpn::planSumReductionPair(serpn::SumReductionKind::SumXMinusY2, *oLeft, *oRight);
    if (!aResult)
    {
        if (aResult.meError == spreadsheetengine::api::Error::IllegalArgument
            || aResult.meError == spreadsheetengine::api::Error::NoValue)
        {
            PushNoValue();
            return;
        }
        PushError(selibreoffice::toFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

inline void Dispatcher::random(ScInterpreter& rCalc)
{
    selibreoffice::DocumentEvaluationHost aHost(mrDoc, mrContext);
    const auto aResult = serpn::planRandom(aHost, randomOutputFrame(rCalc));
    if (!aResult)
    {
        pushRandomPlanError(rCalc, aResult.meError);
        return;
    }

    pushRandomPlanResult(rCalc, aResult.maValue);
}

inline void Dispatcher::randArray(ScInterpreter& rCalc)
{
    const sal_uInt8 nParamCount = GetByte();

    bool bWholeNumber = false;
    if (nParamCount == 5)
        bWholeNumber = SEIC.GetBoolWithDefault(false);

    double fMax = 1.0;
    if (nParamCount >= 4)
        fMax = GetDoubleWithDefault(1.0);

    double fMin = 0.0;
    if (nParamCount >= 3)
        fMin = GetDoubleWithDefault(0.0);

    SCCOL nColumns = 1;
    if (nParamCount >= 2)
        nColumns = static_cast<SCCOL>(SEIC.GetInt32WithDefault(1));

    SCROW nRows = 1;
    if (nParamCount >= 1)
        nRows = static_cast<SCROW>(SEIC.GetInt32WithDefault(1));

    if (nGlobalError != FormulaError::NONE)
    {
        PushIllegalArgument();
        return;
    }

    selibreoffice::DocumentEvaluationHost aHost(mrDoc, mrContext);
    const auto aResult = serpn::planRandArray(aHost,
        { static_cast<spreadsheetengine::api::MatrixSize>(nColumns),
          static_cast<spreadsheetengine::api::MatrixSize>(nRows) },
        fMin, fMax, bWholeNumber);
    if (!aResult)
    {
        pushRandomPlanError(rCalc, aResult.meError);
        return;
    }

    pushRandomPlanResult(rCalc, aResult.maValue);
}

inline void Dispatcher::randbetween(ScInterpreter& rCalc)
{
    if (!MustHaveParamCount(GetByte(), 2))
        return;

    const double fMax = CalcGetDouble();
    const double fMin = CalcGetDouble();
    if (nGlobalError != FormulaError::NONE)
    {
        PushIllegalArgument();
        return;
    }

    selibreoffice::DocumentEvaluationHost aHost(mrDoc, mrContext);
    const auto aResult = serpn::planRandbetween(aHost, randomOutputFrame(rCalc), fMin, fMax);
    if (!aResult)
    {
        pushRandomPlanError(rCalc, aResult.meError);
        return;
    }

    pushRandomPlanResult(rCalc, aResult.maValue);
}

inline void Dispatcher::matrixFrequency(ScInterpreter& rCalc)
{
    if (!MustHaveParamCount(GetByte(), 2))
        return;

    ScMatrixRef pBins = GetMatrix();
    ScMatrixRef pData = GetMatrix();
    if (!pBins || !pData)
    {
        PushIllegalParameter();
        return;
    }

    const auto oBins = matrixRefToMatrixOperand(pBins);
    const auto oData = matrixRefToMatrixOperand(pData);
    if (!oBins || !oData)
    {
        PushIllegalParameter();
        return;
    }

    const auto aResult = serpn::planFrequency(*oData, *oBins);
    if (!aResult)
    {
        if (aResult.meError == spreadsheetengine::api::Error::NoValue)
        {
            PushNoValue();
            return;
        }
        PushError(selibreoffice::toFormulaError(aResult.meError));
        return;
    }
    PushMatrix(matrixOperandToMatrixRef(aResult.maValue));
}

inline void Dispatcher::fourier(ScInterpreter& rCalc)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, 5))
        return;

    bool bInverse = false;
    bool bPolar = false;
    double fMinMag = 0.0;

    if (nParamCount == 5)
    {
        if (IsMissing())
            Pop();
        else
            fMinMag = CalcGetDouble();
    }

    if (nParamCount >= 4)
    {
        if (IsMissing())
            Pop();
        else
            bPolar = GetBool();
    }

    if (nParamCount >= 3)
    {
        if (IsMissing())
            Pop();
        else
            bInverse = GetBool();
    }

    const bool bGroupedByColumn = GetBool();

    ScMatrixRef pInput = GetMatrix();
    if (!pInput)
    {
        PushIllegalParameter();
        return;
    }

    const auto oInput = matrixRefToMatrixOperand(pInput);
    if (!oInput)
    {
        PushIllegalParameter();
        return;
    }

    const auto aResult = serpn::planFourier(
        *oInput, bGroupedByColumn, bInverse, bPolar, fMinMag);
    if (!aResult)
    {
        if (aResult.meError == spreadsheetengine::api::Error::NoValue)
        {
            PushNoValue();
            return;
        }
        PushError(selibreoffice::toFormulaError(aResult.meError));
        return;
    }
    PushMatrix(matrixOperandToMatrixRef(aResult.maValue.maMatrix));
}

inline void Dispatcher::aggregateFunction(ScInterpreter& rCalc)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCountMinWithStackCheck(nParamCount, 3))
        return;

    const FormulaError nErr = nGlobalError;
    nGlobalError = FormulaError::NONE;

    const FormulaToken* pFuncToken = pStack[sp - nParamCount];
    PushWithoutError(*pFuncToken);
    sal_Int32 nFunc = GetInt32();
    const FormulaToken* pOptionToken = pStack[sp - (nParamCount - 1)];
    PushWithoutError(*pOptionToken);
    sal_Int32 nOption = GetInt32();

    if (nGlobalError != FormulaError::NONE || nFunc < 1 || nFunc > 19)
    {
        nGlobalError = nErr;
        PushIllegalArgument();
        return;
    }

    switch (nOption)
    {
        case 0:
            mnSubTotalFlags = SubtotalFlags::IgnoreNestedStAg;
            break;
        case 1:
            mnSubTotalFlags = SubtotalFlags::IgnoreHidden | SubtotalFlags::IgnoreNestedStAg;
            break;
        case 2:
            mnSubTotalFlags = SubtotalFlags::IgnoreErrVal | SubtotalFlags::IgnoreNestedStAg;
            break;
        case 3:
            mnSubTotalFlags = SubtotalFlags::IgnoreHidden | SubtotalFlags::IgnoreErrVal
                              | SubtotalFlags::IgnoreNestedStAg;
            break;
        case 4:
            mnSubTotalFlags = SubtotalFlags::NONE;
            break;
        case 5:
            mnSubTotalFlags = SubtotalFlags::IgnoreHidden;
            break;
        case 6:
            mnSubTotalFlags = SubtotalFlags::IgnoreErrVal;
            break;
        case 7:
            mnSubTotalFlags = SubtotalFlags::IgnoreHidden | SubtotalFlags::IgnoreErrVal;
            break;
        default:
            nGlobalError = nErr;
            PushIllegalArgument();
            return;
    }

    if ((mnSubTotalFlags & SubtotalFlags::IgnoreErrVal) == SubtotalFlags::NONE)
        nGlobalError = nErr;

    cPar = nParamCount - 2;
    switch (nFunc)
    {
        case AGGREGATE_FUNC_AVE:
            aggregateAverage(rCalc, false);
            break;
        case AGGREGATE_FUNC_CNT:
            IterateParameters(ifCOUNT);
            break;
        case AGGREGATE_FUNC_CNT2:
            IterateParameters(ifCOUNT2);
            break;
        case AGGREGATE_FUNC_MAX:
            aggregateMax(rCalc, false);
            break;
        case AGGREGATE_FUNC_MIN:
            aggregateMin(rCalc, false);
            break;
        case AGGREGATE_FUNC_PROD:
            aggregateProduct(rCalc);
            break;
        case AGGREGATE_FUNC_STD:
            aggregateStDev(rCalc, false);
            break;
        case AGGREGATE_FUNC_STDP:
            aggregateStDevP(rCalc, false);
            break;
        case AGGREGATE_FUNC_SUM:
            aggregateSum(rCalc);
            break;
        case AGGREGATE_FUNC_VAR:
            aggregateVar(rCalc, false);
            break;
        case AGGREGATE_FUNC_VARP:
            aggregateVarP(rCalc, false);
            break;
        case AGGREGATE_FUNC_MEDIAN:
            statisticalMedian(rCalc);
            break;
        case AGGREGATE_FUNC_MODSNGL:
            statisticalMode(rCalc, true);
            break;
        case AGGREGATE_FUNC_LARGE:
            CalculateSmallLarge(false);
            break;
        case AGGREGATE_FUNC_SMALL:
            CalculateSmallLarge(true);
            break;
        case AGGREGATE_FUNC_PERCINC:
            statisticalPercentile(rCalc, true);
            break;
        case AGGREGATE_FUNC_QRTINC:
            statisticalQuartile(rCalc, true);
            break;
        case AGGREGATE_FUNC_PERCEXC:
            statisticalPercentile(rCalc, false);
            break;
        case AGGREGATE_FUNC_QRTEXC:
            statisticalQuartile(rCalc, false);
            break;
        default:
            nGlobalError = nErr;
            PushIllegalArgument();
            mnSubTotalFlags = SubtotalFlags::NONE;
            return;
    }
    mnSubTotalFlags = SubtotalFlags::NONE;

    FormulaConstTokenRef xRef(PopToken());
    Pop();
    Pop();
    PushTokenRef(xRef);
}

inline void Dispatcher::subtotalFunction(ScInterpreter& rCalc)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCountMinWithStackCheck(nParamCount, 2))
        return;

    const FormulaToken* pFuncToken = pStack[sp - nParamCount];
    PushWithoutError(*pFuncToken);
    sal_Int32 nFunc = GetInt32();
    mnSubTotalFlags = SubtotalFlags::IgnoreNestedStAg | SubtotalFlags::IgnoreFiltered;
    if (nFunc > 100)
    {
        mnSubTotalFlags |= SubtotalFlags::IgnoreHidden;
        nFunc -= 100;
    }

    if (nGlobalError != FormulaError::NONE || nFunc < 1 || nFunc > 11)
    {
        mnSubTotalFlags = SubtotalFlags::NONE;
        PushIllegalArgument();
        return;
    }

    cPar = nParamCount - 1;
    switch (nFunc)
    {
        case SUBTOTAL_FUNC_AVE:
            aggregateAverage(rCalc, false);
            break;
        case SUBTOTAL_FUNC_CNT:
            IterateParameters(ifCOUNT);
            break;
        case SUBTOTAL_FUNC_CNT2:
            IterateParameters(ifCOUNT2);
            break;
        case SUBTOTAL_FUNC_MAX:
            aggregateMax(rCalc, false);
            break;
        case SUBTOTAL_FUNC_MIN:
            aggregateMin(rCalc, false);
            break;
        case SUBTOTAL_FUNC_PROD:
            aggregateProduct(rCalc);
            break;
        case SUBTOTAL_FUNC_STD:
            aggregateStDev(rCalc, false);
            break;
        case SUBTOTAL_FUNC_STDP:
            aggregateStDevP(rCalc, false);
            break;
        case SUBTOTAL_FUNC_SUM:
            aggregateSum(rCalc);
            break;
        case SUBTOTAL_FUNC_VAR:
            aggregateVar(rCalc, false);
            break;
        case SUBTOTAL_FUNC_VARP:
            aggregateVarP(rCalc, false);
            break;
        default:
            mnSubTotalFlags = SubtotalFlags::NONE;
            PushIllegalArgument();
            return;
    }
    mnSubTotalFlags = SubtotalFlags::NONE;

    FormulaConstTokenRef xRef(PopToken());
    Pop();
    PushTokenRef(xRef);
}

inline void Dispatcher::sortByTerminal(ScInterpreter& rCalc)
{
    sal_uInt8 nParamCount = GetByte();
    if (nParamCount < 2)
    {
        PushError(FormulaError::ParameterExpected);
        return;
    }

    const sal_uInt8 nSortCount = nParamCount / 2;
    ScSortParam aSortData;
    aSortData.maKeyState.resize(nSortCount);
    bool bNoNeedToSort = false;

    sal_uInt8 nSortBy = nSortCount;
    ScMatrixRef pFullMatSortBy = nullptr;
    while (nSortBy-- > 0 && nGlobalError == FormulaError::NONE)
    {
        if (nParamCount >= 3 && (nParamCount % 2 == 1))
        {
            const sal_Int8 nSortOrder
                = static_cast<sal_Int8>(SEIC.GetInt32WithDefault(1));
            if (nSortOrder != 1 && nSortOrder != -1)
            {
                PushIllegalParameter();
                return;
            }
            aSortData.maKeyState[nSortBy].bAscending = (nSortOrder == 1);
            --nParamCount;
        }

        ScMatrixRef pMatSortBy = nullptr;
        SCSIZE nByColumns = 0;
        SCSIZE nByRows = 0;
        switch (GetStackType())
        {
            case svSingleRef:
            case svDoubleRef:
            case svMatrix:
            case svExternalSingleRef:
            case svExternalDoubleRef:
            {
                if (nSortCount == 1)
                {
                    pFullMatSortBy = GetMatrix();
                    if (!pFullMatSortBy)
                    {
                        PushIllegalParameter();
                        return;
                    }
                    pFullMatSortBy->GetDimensions(nByColumns, nByRows);
                }
                else
                {
                    pMatSortBy = GetMatrix();
                    if (!pMatSortBy)
                    {
                        PushIllegalParameter();
                        return;
                    }
                    pMatSortBy->GetDimensions(nByColumns, nByRows);
                }

                if (nSortBy == nSortCount - 1)
                {
                    if (nByColumns == 1 && nByRows > 1)
                        aSortData.bByRow = true;
                    else if (nByRows == 1 && nByColumns > 1)
                        aSortData.bByRow = false;
                    else if (nByColumns == 1 && nByRows == 1)
                        bNoNeedToSort = true;
                    else
                    {
                        PushIllegalParameter();
                        return;
                    }

                    if (nSortCount > 1)
                    {
                        pFullMatSortBy = GetNewMat(
                            aSortData.bByRow ? (nByColumns * nSortCount) : nByColumns,
                            aSortData.bByRow ? nByRows : (nByRows * nSortCount), true);
                    }
                }
                break;
            }
            default:
                PushIllegalParameter();
                return;
        }

        if (nSortCount > 1 && nSortBy <= nSortCount - 1)
        {
            SCSIZE nCheckColumns = 0;
            SCSIZE nCheckRows = 0;
            pFullMatSortBy->GetDimensions(nCheckColumns, nCheckRows);
            if ((aSortData.bByRow && nByRows == nCheckRows && nByColumns == 1)
                || (!aSortData.bByRow && nByColumns == nCheckColumns && nByRows == 1))
            {
                for (SCSIZE nColumn = 0; nColumn < nByColumns; ++nColumn)
                {
                    for (SCSIZE nRow = 0; nRow < nByRows; ++nRow)
                    {
                        if (pMatSortBy->IsEmptyCell(nColumn, nRow))
                        {
                            if (aSortData.bByRow)
                                pFullMatSortBy->PutEmpty(nColumn + nSortBy, nRow);
                            else
                                pFullMatSortBy->PutEmpty(nColumn, nRow + nSortBy);
                        }
                        else if (pMatSortBy->IsStringOrEmpty(nColumn, nRow))
                        {
                            if (aSortData.bByRow)
                            {
                                pFullMatSortBy->PutString(
                                    pMatSortBy->GetString(nColumn, nRow), nColumn + nSortBy,
                                    nRow);
                            }
                            else
                            {
                                pFullMatSortBy->PutString(
                                    pMatSortBy->GetString(nColumn, nRow), nColumn,
                                    nRow + nSortBy);
                            }
                        }
                        else if (aSortData.bByRow)
                        {
                            pFullMatSortBy->PutDouble(
                                pMatSortBy->GetDouble(nColumn, nRow), nColumn + nSortBy, nRow);
                        }
                        else
                        {
                            pFullMatSortBy->PutDouble(
                                pMatSortBy->GetDouble(nColumn, nRow), nColumn, nRow + nSortBy);
                        }
                    }
                }
            }
            else
            {
                PushIllegalParameter();
                return;
            }
        }

        aSortData.maKeyState[nSortBy].bDoSort = true;
        aSortData.maKeyState[nSortBy].nField = nSortBy;
        --nParamCount;
    }

    SCSIZE nSourceColumns = 0;
    SCSIZE nSourceRows = 0;
    SCCOL nSortCol1 = 0;
    SCCOL nSortCol2 = 0;
    SCROW nSortRow1 = 0;
    SCROW nSortRow2 = 0;
    SCTAB nSortTab1 = 0;
    SCTAB nSortTab2 = 0;
    ScMatrixRef pMatSource = nullptr;
    switch (GetStackType())
    {
        case svSingleRef:
            PopSingleRef(nSortCol1, nSortRow1, nSortTab1);
            nSortCol2 = nSortCol1;
            nSortRow2 = nSortRow1;
            nSourceColumns = nSortCol2 - nSortCol1 + 1;
            nSourceRows = nSortRow2 - nSortRow1 + 1;
            break;
        case svDoubleRef:
            PopDoubleRef(nSortCol1, nSortRow1, nSortTab1, nSortCol2, nSortRow2, nSortTab2);
            if (nSortTab1 != nSortTab2)
            {
                PushIllegalParameter();
                return;
            }
            nSourceColumns = nSortCol2 - nSortCol1 + 1;
            nSourceRows = nSortRow2 - nSortRow1 + 1;
            break;
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
            pMatSource = GetMatrix();
            if (!pMatSource)
            {
                PushIllegalParameter();
                return;
            }
            pMatSource->GetDimensions(nSourceColumns, nSourceRows);
            if (nSourceColumns == 0 || nSourceRows == 0)
            {
                PushIllegalArgument();
                return;
            }
            nSortCol2 = nSourceColumns - 1;
            nSortRow2 = nSourceRows - 1;
            break;
        default:
            PushIllegalParameter();
            return;
    }

    SCSIZE nCheckMatrixColumns = 0;
    SCSIZE nCheckMatrixRows = 0;
    pFullMatSortBy->GetDimensions(nCheckMatrixColumns, nCheckMatrixRows);
    if (nGlobalError != FormulaError::NONE)
    {
        PushError(nGlobalError);
        return;
    }
    if ((aSortData.bByRow && nSourceRows != nCheckMatrixRows)
        || (!aSortData.bByRow && nSourceColumns != nCheckMatrixColumns))
    {
        PushIllegalParameter();
        return;
    }

    aSortData.nCol2 = nCheckMatrixColumns - 1;
    aSortData.nRow2 = nCheckMatrixRows - 1;

    if (bNoNeedToSort)
    {
        if (pMatSource)
            PushMatrix(pMatSource);
        else
            SEIC.PushDoubleRef(
                nSortCol1, nSortRow1, nSortTab1, nSortCol2, nSortRow2, nSortTab2);
        return;
    }

    const std::vector<SCCOLROW> aOrderIndices = SEIC.GetSortOrder(aSortData, pFullMatSortBy);
    ScMatrixRef pResultMatrix = SEIC.CreateSortedMatrix(
        aSortData, pMatSource,
        ScRange(nSortCol1, nSortRow1, nSortTab1, nSortCol2, nSortRow2, nSortTab2),
        aOrderIndices, nSourceColumns, nSourceRows);
    if (pResultMatrix)
        PushMatrix(pResultMatrix);
    else
        PushIllegalParameter();
}

inline void Dispatcher::dbAreaTerminal(ScInterpreter& rCalc)
{
    ScDBData* pDBData = mrDoc.GetDBCollection()->getNamedDBs().findByIndex(SEIC.pCur->GetIndex());
    if (!pDBData)
    {
        PushError(FormulaError::NoName);
        return;
    }

    ScComplexRefData aRefData;
    aRefData.InitFlags();
    ScRange aRange;
    pDBData->GetArea(aRange);
    aRange.aEnd.SetTab(aRange.aStart.Tab());
    aRefData.SetRange(mrDoc.GetSheetLimits(), aRange, SEIC.aPos);
    SEIC.PushTempToken(new ScDoubleRefToken(mrDoc.GetSheetLimits(), aRefData));
}

inline void Dispatcher::colRowNameAutoTerminal(ScInterpreter& rCalc)
{
    ScComplexRefData aRefData(*SEIC.pCur->GetDoubleRef());
    ScRange aAbsolute = aRefData.toAbs(mrDoc, SEIC.aPos);
    if (!mrDoc.ValidRange(aAbsolute))
    {
        PushError(FormulaError::NoRef);
        return;
    }

    SCCOL nStartCol = aAbsolute.aStart.Col();
    SCROW nStartRow = aAbsolute.aStart.Row();
    SCCOL nCol2 = aAbsolute.aEnd.Col();
    SCROW nRow2 = aAbsolute.aEnd.Row();
    aAbsolute.aEnd = aAbsolute.aStart;

    {
        SCCOL nDataAreaCol1 = aAbsolute.aStart.Col();
        SCCOL nDataAreaCol2 = aAbsolute.aEnd.Col();
        SCROW nDataAreaRow1 = aAbsolute.aStart.Row();
        SCROW nDataAreaRow2 = aAbsolute.aEnd.Row();
        mrDoc.GetDataArea(aAbsolute.aStart.Tab(), nDataAreaCol1, nDataAreaRow1, nDataAreaCol2,
            nDataAreaRow2, true, false);
        aAbsolute.aEnd.SetCol(nDataAreaCol2);
        aAbsolute.aEnd.SetRow(nDataAreaRow2);
    }

    if (aRefData.Ref1.IsColRel())
    {
        aAbsolute.aEnd.SetCol(nStartCol);
        if (aAbsolute.aEnd.Row() > nRow2)
            aAbsolute.aEnd.SetRow(nRow2);
        if (SEIC.aPos.Col() == nStartCol)
        {
            const SCROW nMyRow = SEIC.aPos.Row();
            if (nStartRow <= nMyRow && nMyRow <= aAbsolute.aEnd.Row())
            {
                if (nMyRow == nStartRow)
                {
                    ++nStartRow;
                    if (nStartRow > mrDoc.MaxRow())
                        nStartRow = mrDoc.MaxRow();
                    aAbsolute.aStart.SetRow(nStartRow);
                }
                else
                {
                    aAbsolute.aEnd.SetRow(nMyRow - 1);
                }
            }
        }
    }
    else
    {
        aAbsolute.aEnd.SetRow(nStartRow);
        if (aAbsolute.aEnd.Col() > nCol2)
            aAbsolute.aEnd.SetCol(nCol2);
        if (SEIC.aPos.Row() == nStartRow)
        {
            const SCCOL nMyCol = SEIC.aPos.Col();
            if (nStartCol <= nMyCol && nMyCol <= aAbsolute.aEnd.Col())
            {
                if (nMyCol == nStartCol)
                {
                    ++nStartCol;
                    if (nStartCol > mrDoc.MaxCol())
                        nStartCol = mrDoc.MaxCol();
                    aAbsolute.aStart.SetCol(nStartCol);
                }
                else
                {
                    aAbsolute.aEnd.SetCol(nMyCol - 1);
                }
            }
        }
    }

    aRefData.SetRange(mrDoc.GetSheetLimits(), aAbsolute, SEIC.aPos);
    SEIC.PushTempToken(new ScDoubleRefToken(mrDoc.GetSheetLimits(), aRefData));
}

inline void Dispatcher::probability(ScInterpreter& rCalc)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 3, 4))
        return;

    double fUpper = CalcGetDouble();
    double fLower = nParamCount == 4 ? CalcGetDouble() : fUpper;
    if (fLower > fUpper)
        std::swap(fLower, fUpper);

    ScMatrixRef pMatProbabilities = GetMatrix();
    ScMatrixRef pMatValues = GetMatrix();
    if (!pMatProbabilities || !pMatValues)
    {
        PushIllegalParameter();
        return;
    }

    SCSIZE nProbCols = 0, nProbRows = 0, nValueCols = 0, nValueRows = 0;
    pMatProbabilities->GetDimensions(nProbCols, nProbRows);
    pMatValues->GetDimensions(nValueCols, nValueRows);
    if (nProbCols != nValueCols || nProbRows != nValueRows || nProbCols == 0 || nProbRows == 0
        || nValueCols == 0 || nValueRows == 0)
    {
        PushNA();
        return;
    }

    KahanSum fSum = 0.0;
    KahanSum fResult = 0.0;
    bool bStop = false;
    for (SCSIZE nColumn = 0; nColumn < nProbCols && !bStop; ++nColumn)
    {
        for (SCSIZE nRow = 0; nRow < nProbRows && !bStop; ++nRow)
        {
            if (pMatProbabilities->IsValue(nColumn, nRow) && pMatValues->IsValue(nColumn, nRow))
            {
                const double fProbability = pMatProbabilities->GetDouble(nColumn, nRow);
                const double fValue = pMatValues->GetDouble(nColumn, nRow);
                if (fProbability < 0.0 || fProbability > 1.0)
                    bStop = true;
                else
                {
                    fSum += fProbability;
                    if (fValue >= fLower && fValue <= fUpper)
                        fResult += fProbability;
                }
            }
            else
                SetError(FormulaError::IllegalArgument);
        }
    }

    if (bStop || std::abs((fSum - 1.0).get()) > 1.0E-7)
        PushNoValue();
    else
        PushDouble(fResult.get());
}

inline void Dispatcher::zTest(ScInterpreter& rCalc)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, 3))
        return;

    double sigma = 0.0;
    if (nParamCount == 3)
    {
        sigma = CalcGetDouble();
        if (sigma <= 0.0)
        {
            PushIllegalArgument();
            return;
        }
    }
    const double x = CalcGetDouble();

    KahanSum fSum = 0.0;
    KahanSum fSumSqr = 0.0;
    double fVal = 0.0;
    double fValCount = 0.0;
    switch (GetStackType())
    {
        case svDouble:
            fVal = CalcGetDouble();
            fSum += fVal;
            fSumSqr += fVal * fVal;
            fValCount++;
            break;
        case svSingleRef:
        {
            ScAddress aAdr;
            PopSingleRef(aAdr);
            ScRefCellValue aCell(mrDoc, aAdr);
            if (aCell.hasNumeric())
            {
                fVal = GetCellValue(aAdr, aCell);
                fSum += fVal;
                fSumSqr += fVal * fVal;
                fValCount++;
            }
        }
        break;
        case svRefList:
        case svDoubleRef:
        {
            short nParam = 1;
            size_t nRefInList = 0;
            while (nParam-- > 0)
            {
                ScRange aRange;
                FormulaError nErr = FormulaError::NONE;
                PopDoubleRef(aRange, nParam, nRefInList);
                ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                if (aValIter.GetFirst(fVal, nErr))
                {
                    fSum += fVal;
                    fSumSqr += fVal * fVal;
                    fValCount++;
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(fVal, nErr))
                    {
                        fSum += fVal;
                        fSumSqr += fVal * fVal;
                        fValCount++;
                    }
                    SetError(nErr);
                }
            }
        }
        break;
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            ScMatrixRef pMat = GetMatrix();
            if (pMat)
            {
                const SCSIZE nCount = pMat->GetElementCount();
                if (pMat->IsNumeric())
                {
                    for (SCSIZE i = 0; i < nCount; i++)
                    {
                        fVal = pMat->GetDouble(i);
                        fSum += fVal;
                        fSumSqr += fVal * fVal;
                        fValCount++;
                    }
                }
                else
                {
                    for (SCSIZE i = 0; i < nCount; i++)
                    {
                        if (!pMat->IsStringOrEmpty(i))
                        {
                            fVal = pMat->GetDouble(i);
                            fSum += fVal;
                            fSumSqr += fVal * fVal;
                            fValCount++;
                        }
                    }
                }
            }
        }
        break;
        default:
            SetError(FormulaError::IllegalParameter);
            break;
    }

    if (fValCount <= 1.0)
    {
        PushError(FormulaError::DivisionByZero);
        return;
    }

    const double fMean = fSum.get() / fValCount;
    if (nParamCount != 3)
    {
        sigma = (fSumSqr - fSum * fSum / fValCount).get() / (fValCount - 1.0);
        if (sigma == 0.0)
        {
            PushError(FormulaError::DivisionByZero);
            return;
        }
        PushDouble(0.5 - semath::gaussValue((fMean - x) / sqrt(sigma / fValCount)));
    }
    else
        PushDouble(0.5 - semath::gaussValue((fMean - x) * sqrt(fValCount) / sigma));
}

inline void Dispatcher::tTest(ScInterpreter& rCalc)
{
    if (!MustHaveParamCount(GetByte(), 4))
        return;

    double fTyp = ::rtl::math::approxFloor(CalcGetDouble());
    double fTails = ::rtl::math::approxFloor(CalcGetDouble());
    if (fTails != 1.0 && fTails != 2.0)
    {
        PushIllegalArgument();
        return;
    }

    ScMatrixRef pMat2 = GetMatrix();
    ScMatrixRef pMat1 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }

    double fT = 0.0;
    double fF = 0.0;
    SCSIZE nC1 = 0, nC2 = 0, nR1 = 0, nR2 = 0;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (fTyp == 1.0)
    {
        if (nC1 != nC2 || nR1 != nR2)
        {
            PushIllegalArgument();
            return;
        }
        double fCount = 0.0;
        KahanSum fSum1 = 0.0;
        KahanSum fSum2 = 0.0;
        KahanSum fSumSqrD = 0.0;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsStringOrEmpty(i, j) && !pMat2->IsStringOrEmpty(i, j))
                {
                    const double fVal1 = pMat1->GetDouble(i, j);
                    const double fVal2 = pMat2->GetDouble(i, j);
                    fSum1 += fVal1;
                    fSum2 += fVal2;
                    fSumSqrD += (fVal1 - fVal2) * (fVal1 - fVal2);
                    fCount++;
                }
            }
        }
        if (fCount < 1.0)
        {
            PushNoValue();
            return;
        }
        KahanSum fSumD = fSum1 - fSum2;
        const double fDivider = (fSumSqrD * fCount - fSumD * fSumD).get();
        if (fDivider == 0.0)
        {
            PushError(FormulaError::DivisionByZero);
            return;
        }
        fT = std::abs(fSumD.get()) * sqrt((fCount - 1.0) / fDivider);
        fF = fCount - 1.0;
    }
    else if (fTyp == 2.0)
    {
        if (!CalculateTest(false, nC1, nC2, nR1, nR2, pMat1, pMat2, fT, fF))
            return;
    }
    else if (fTyp == 3.0)
    {
        if (!CalculateTest(true, nC1, nC2, nR1, nR2, pMat1, pMat2, fT, fF))
            return;
    }
    else
    {
        PushIllegalArgument();
        return;
    }
    PushDouble(GetTDist(fT, fF, static_cast<int>(fTails)));
}

inline void Dispatcher::fTest(ScInterpreter& rCalc)
{
    if (!MustHaveParamCount(GetByte(), 2))
        return;

    ScMatrixRef pMat2 = GetMatrix();
    ScMatrixRef pMat1 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }

    const auto aVal1 = pMat1->CollectKahan(sc::op::kOpSumAndSumSquare);
    const auto aVal2 = pMat2->CollectKahan(sc::op::kOpSumAndSumSquare);
    const double fCount1 = aVal1.mnCount;
    const double fCount2 = aVal2.mnCount;
    const KahanSum fSum1 = aVal1.maAccumulator[0];
    const KahanSum fSumSqr1 = aVal1.maAccumulator[1];
    const KahanSum fSum2 = aVal2.maAccumulator[0];
    const KahanSum fSumSqr2 = aVal2.maAccumulator[1];

    if (fCount1 < 2.0 || fCount2 < 2.0)
    {
        PushNoValue();
        return;
    }
    const double fS1 = (fSumSqr1 - fSum1 * fSum1 / fCount1).get() / (fCount1 - 1.0);
    const double fS2 = (fSumSqr2 - fSum2 * fSum2 / fCount2).get() / (fCount2 - 1.0);
    if (fS1 == 0.0 || fS2 == 0.0)
    {
        PushNoValue();
        return;
    }

    double fF = 0.0;
    double fF1 = 0.0;
    double fF2 = 0.0;
    if (fS1 > fS2)
    {
        fF = fS1 / fS2;
        fF1 = fCount1 - 1.0;
        fF2 = fCount2 - 1.0;
    }
    else
    {
        fF = fS2 / fS1;
        fF1 = fCount2 - 1.0;
        fF2 = fCount1 - 1.0;
    }
    const double fFcdf = GetFDist(fF, fF1, fF2);
    PushDouble(2.0 * std::min(fFcdf, 1.0 - fFcdf));
}

inline void Dispatcher::chiTest(ScInterpreter& rCalc)
{
    if (!MustHaveParamCount(GetByte(), 2))
        return;

    ScMatrixRef pMat2 = GetMatrix();
    ScMatrixRef pMat1 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1 = 0, nC2 = 0, nR1 = 0, nR2 = 0;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    KahanSum fChi = 0.0;
    bool bEmpty = true;
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!(pMat1->IsEmpty(i, j) || pMat2->IsEmpty(i, j)))
            {
                bEmpty = false;
                if (!pMat1->IsStringOrEmpty(i, j) && !pMat2->IsStringOrEmpty(i, j))
                {
                    const double fValX = pMat1->GetDouble(i, j);
                    const double fValE = pMat2->GetDouble(i, j);
                    if (fValE == 0.0)
                    {
                        PushError(FormulaError::DivisionByZero);
                        return;
                    }
                    volatile double fTemp1 = (fValX - fValE) * (fValX - fValE);
                    const double fTemp2 = fTemp1;
                    if (std::isinf(fTemp2))
                    {
                        PushError(FormulaError::NoConvergence);
                        return;
                    }
                    fChi += sc::divide(fTemp2, fValE);
                }
                else
                {
                    PushIllegalArgument();
                    return;
                }
            }
        }
    }
    if (bEmpty)
    {
        PushIllegalArgument();
        return;
    }
    double fDF = 0.0;
    if (nC1 == 1 || nR1 == 1)
    {
        fDF = static_cast<double>(nC1 * nR1 - 1);
        if (fDF == 0.0)
        {
            PushNoValue();
            return;
        }
    }
    else
        fDF = static_cast<double>(nC1 - 1) * static_cast<double>(nR1 - 1);
    PushDouble(GetChiDist(fChi.get(), fDF));
}

inline void Dispatcher::forecast(ScInterpreter& rCalc)
{
    if (!MustHaveParamCount(GetByte(), 3))
        return;

    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1 = 0, nC2 = 0, nR1 = 0, nR2 = 0;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    const double fVal = CalcGetDouble();
    double fCount = 0.0;
    KahanSum fSumX = 0.0;
    KahanSum fSumY = 0.0;

    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsStringOrEmpty(i, j) && !pMat2->IsStringOrEmpty(i, j))
            {
                fSumX += pMat1->GetDouble(i, j);
                fSumY += pMat2->GetDouble(i, j);
                fCount++;
            }
        }
    }
    if (fCount < 1.0)
    {
        PushNoValue();
        return;
    }

    KahanSum fSumDeltaXDeltaY = 0.0;
    KahanSum fSumSqrDeltaX = 0.0;
    const double fMeanX = fSumX.get() / fCount;
    const double fMeanY = fSumY.get() / fCount;
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsStringOrEmpty(i, j) && !pMat2->IsStringOrEmpty(i, j))
            {
                const double fValX = pMat1->GetDouble(i, j);
                const double fValY = pMat2->GetDouble(i, j);
                fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                fSumSqrDeltaX += (fValX - fMeanX) * (fValX - fMeanX);
            }
        }
    }
    if (fSumSqrDeltaX == 0.0)
        PushError(FormulaError::DivisionByZero);
    else
        PushDouble(
            fMeanY + fSumDeltaXDeltaY.get() / fSumSqrDeltaX.get() * (fVal - fMeanX));
}

inline void Dispatcher::forecastEts(ScInterpreter& rCalc, ScETSType eETSType)
{
    sal_uInt8 nParamCount = GetByte();
    switch (eETSType)
    {
        case etsAdd:
        case etsMult:
        case etsStatAdd:
        case etsStatMult:
            if (!MustHaveParamCount(nParamCount, 3, 6))
                return;
            break;
        case etsPIAdd:
        case etsPIMult:
            if (!MustHaveParamCount(nParamCount, 3, 7))
                return;
            break;
        case etsSeason:
            if (!MustHaveParamCount(nParamCount, 2, 4))
                return;
            break;
    }

    serpn::MatrixOperand aAggregation;
    serpn::MatrixOperand aDataCompletion;
    serpn::MatrixOperand aSeasonality;
    serpn::MatrixOperand aConfidence;
    serpn::MatrixOperand* pAggregation = nullptr;
    serpn::MatrixOperand* pDataCompletion = nullptr;
    serpn::MatrixOperand* pSeasonality = nullptr;
    serpn::MatrixOperand* pConfidence = nullptr;

    if ((nParamCount == 6 && eETSType != etsPIAdd && eETSType != etsPIMult)
        || (nParamCount == 4 && eETSType == etsSeason) || nParamCount == 7)
    {
        aAggregation = makeScalarMatrixOperand(GetDoubleWithDefault(1.0));
        pAggregation = &aAggregation;
    }

    if ((nParamCount >= 5 && eETSType != etsPIAdd && eETSType != etsPIMult)
        || (nParamCount >= 3 && eETSType == etsSeason)
        || (nParamCount >= 6 && (eETSType == etsPIAdd || eETSType == etsPIMult)))
    {
        aDataCompletion = makeScalarMatrixOperand(GetDoubleWithDefault(1.0));
        pDataCompletion = &aDataCompletion;
    }

    if (((nParamCount >= 4 && eETSType != etsPIAdd && eETSType != etsPIMult)
         || (nParamCount >= 5 && (eETSType == etsPIAdd || eETSType == etsPIMult)))
        && eETSType != etsSeason)
    {
        aSeasonality = makeScalarMatrixOperand(GetDoubleWithDefault(1.0));
        pSeasonality = &aSeasonality;
    }

    if (eETSType == etsPIAdd || eETSType == etsPIMult)
    {
        aConfidence
            = makeScalarMatrixOperand(nParamCount < 4 ? 0.95 : GetDoubleWithDefault(0.95));
        pConfidence = &aConfidence;
    }

    ScMatrixRef pTargetOrType;
    if (eETSType == etsStatAdd || eETSType == etsStatMult)
    {
        pTargetOrType = GetMatrix();
        if (!pTargetOrType)
        {
            PushIllegalParameter();
            return;
        }
    }

    ScMatrixRef pKnownX = GetMatrix();
    ScMatrixRef pKnownY = GetMatrix();
    if (!pKnownX || !pKnownY)
    {
        PushIllegalParameter();
        return;
    }

    if (eETSType != etsStatAdd && eETSType != etsStatMult && eETSType != etsSeason)
    {
        pTargetOrType = GetMatrix();
        if (!pTargetOrType)
        {
            PushIllegalArgument();
            return;
        }
    }

    const auto oKnownX = matrixRefToMatrixOperand(pKnownX);
    const auto oKnownY = matrixRefToMatrixOperand(pKnownY);
    if (!oKnownX || !oKnownY)
    {
        PushIllegalParameter();
        return;
    }

    serpn::MatrixOperand aTargetOrType;
    if (pTargetOrType)
    {
        const auto oTargetOrType = matrixRefToMatrixOperand(pTargetOrType);
        if (!oTargetOrType)
        {
            PushIllegalParameter();
            return;
        }
        aTargetOrType = *oTargetOrType;
    }

    const auto aResult = serpn::planForecastEts(
        toForecastEtsVariant(eETSType), selibreoffice::toApiDateParts(mrContext.NFGetNullDate()),
        aTargetOrType,
        *oKnownY, *oKnownX, pConfidence, pSeasonality, pDataCompletion, pAggregation);
    if (!aResult)
    {
        PushError(selibreoffice::toFormulaError(aResult.meError));
        return;
    }

    if (aResult.maValue.mbIsScalar)
    {
        PushDouble(aResult.maValue.mfScalar);
        return;
    }

    PushMatrix(matrixOperandToMatrixRef(aResult.maValue.maMatrix));
}

} // namespace spreadsheetengine::compat::libreoffice::interpretercompatdispatch

#undef SEIC
#undef mrDoc
#undef mrContext
#undef nGlobalError
#undef nFuncFmtType
#undef nFuncFmtIndex
#undef mnSubTotalFlags
#undef cPar
#undef sp
#undef pStack
#undef GetByte
#undef MustHaveParamCount
#undef MustHaveParamCountMin
#undef MustHaveParamCountMinWithStackCheck
#undef CalcGetDouble
#undef GetDoubleWithDefault
#undef GetBool
#undef GetInt32
#undef IsMissing
#undef GetStackType
#undef PopSingleRef
#undef PopDoubleRef
#undef GetMatrix
#undef PushIllegalArgument
#undef PushIllegalParameter
#undef PushNA
#undef PushNoValue
#undef PushError
#undef PushDouble
#undef PushMatrix
#undef PushTokenRef
#undef PushWithoutError
#undef Pop
#undef PopError
#undef PopToken
#undef SetError
#undef GetCellValue
#undef GetCellErrCode
#undef CurFmtToFuncFmt
#undef GetNewMat
#undef GetRefListArrayMaxSize
#undef SwitchToArrayRefList
#undef IterateParameters
#undef GetStVarParams
#undef CalculateSkew
#undef GetNumberSequenceArray
#undef GetSortArray
#undef GetPercentrank
#undef CalculatePearsonCovar
#undef CalculateTest
#undef CalculateSmallLarge
#undef GetChiDist
#undef GetFDist
#undef GetTDist
#undef CalculateTrendGrowth
