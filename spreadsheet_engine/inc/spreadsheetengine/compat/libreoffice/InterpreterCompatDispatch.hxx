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
#include <spreadsheetengine/compat/libreoffice/LookupExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/TextServices.hxx>
#include <spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx>
#include <spreadsheetengine/runtime/ForecastEngine.hxx>
#include <spreadsheetengine/runtime/ForecastEtsEngine.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnRandom.hxx>

namespace spreadsheetengine::compat::libreoffice::interpretercompatdispatch
{

namespace selibreoffice = spreadsheetengine::compat::libreoffice;
namespace selookupexec = spreadsheetengine::compat::libreoffice::lookupexecution;
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
    static void matrixDeterminant(ScInterpreter& rCalc);
    static void random(ScInterpreter& rCalc);
    static void randArray(ScInterpreter& rCalc);
    static void randbetween(ScInterpreter& rCalc);
    static void matrixSumXMY2(ScInterpreter& rCalc);
    static void matrixFrequency(ScInterpreter& rCalc);
    static void fourier(ScInterpreter& rCalc);
    static void aggregateFunction(ScInterpreter& rCalc);
    static void subtotalFunction(ScInterpreter& rCalc);
    static void probability(ScInterpreter& rCalc);
    static void zTest(ScInterpreter& rCalc);
    static void tTest(ScInterpreter& rCalc);
    static void fTest(ScInterpreter& rCalc);
    static void chiTest(ScInterpreter& rCalc);
    static void forecast(ScInterpreter& rCalc);
    static void forecastEts(ScInterpreter& rCalc, ScETSType eETSType);
    static void growth(ScInterpreter& rCalc) { CalculateTrendGrowth(true); }
};

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
