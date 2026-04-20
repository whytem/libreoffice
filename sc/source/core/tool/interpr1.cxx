/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <interpre.hxx>

#include <optional>
#include <scitems.hxx>
#include <editeng/langitem.hxx>
#include <editeng/justifyitem.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/temporary.hxx>
#include <osl/thread.h>
#include <unotools/textsearch.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>
#include <svl/zformat.hxx>
#include <tools/urlobj.hxx>
#include <unotools/charclass.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/printer.hxx>
#include <unotools/collatorwrapper.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <rtl/character.hxx>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <unicode/uchar.h>
#include <unicode/regex.h>
#include <i18nlangtag/mslangid.hxx>

#include <patattr.hxx>
#include <global.hxx>
#include <document.hxx>
#include <dociter.hxx>
#include <docsh.hxx>
#include <sfx2/linkmgr.hxx>
#include <formulacell.hxx>
#include <scmatrix.hxx>
#include <docoptio.hxx>
#include <attrib.hxx>
#include <jumpmatrix.hxx>
#include <cellkeytranslator.hxx>
#include <lookupcache.hxx>
#include <rangenam.hxx>
#include <spreadsheetengine/api/Array.hxx>
#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Reference.hxx>
#include <spreadsheetengine/api/StringReference.hxx>
#include <spreadsheetengine/compat/libreoffice/CellInspectionExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/FormulaInspectionExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/InfoInspectionExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/IndirectExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpreterDispatch.hxx>
#include <spreadsheetengine/compat/libreoffice/JumpExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/JumpMatrixExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/LetExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/LookupExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/ReferenceExecution.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <spreadsheetengine/runtime/MathBitwise.hxx>
#include <spreadsheetengine/runtime/MathTranscendental.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/Grammar.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/TextServices.hxx>
#include <rangeutl.hxx>
#include <compiler.hxx>
#include <externalrefmgr.hxx>
#include <doubleref.hxx>
#include <queryparam.hxx>
#include <queryiter.hxx>
#include <tokenarray.hxx>
#include <compare.hxx>
#include <comphelper/lok.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/string.hxx>
#include <svl/sharedstringpool.hxx>

#include <stdlib.h>
#include <memory>
#include <vector>
#include <limits>
#include <string_view>
#include <cmath>

ScCalcConfig *ScInterpreter::mpGlobalConfig = nullptr;

using namespace formula;
namespace semath = spreadsheetengine::core::math;
namespace searray = spreadsheetengine::api::array;
namespace selogic = spreadsheetengine::api::logic;
namespace selookup = spreadsheetengine::api::lookup;
namespace seref = spreadsheetengine::api::reference;
namespace sestringref = spreadsheetengine::api::stringreference;
namespace secellexec = spreadsheetengine::compat::libreoffice::cellinspectionexecution;
namespace seindirectexec = spreadsheetengine::compat::libreoffice::indirectexecution;
namespace seinterpre = spreadsheetengine::compat::libreoffice::interpreterdispatch;
namespace sejumpexec = spreadsheetengine::compat::libreoffice::jumpexecution;
namespace sejumpmatrixexec = spreadsheetengine::compat::libreoffice::jumpmatrixexecution;
namespace seletexec = spreadsheetengine::compat::libreoffice::letexecution;
namespace seformulainspect = spreadsheetengine::compat::libreoffice::formulainspection;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;
namespace selookupexec = spreadsheetengine::compat::libreoffice::lookupexecution;
namespace setaileval = spreadsheetengine::compat::libreoffice::interprettaileval;
namespace serefexec = spreadsheetengine::compat::libreoffice::referenceexecution;
namespace setextparseexec = spreadsheetengine::compat::libreoffice::textparsingexecution;

namespace
{

spreadsheetengine::api::query::SearchType toApiSearchType(utl::SearchParam::SearchType eSearchType)
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

[[nodiscard]] std::optional<OUString> lclGetQuarantinedHardRoutedFormula(
    const ScFormulaCell* pCell, const ScDocument& rDoc, ScInterpreterContext& rContext)
{
    if (!pCell)
        return std::nullopt;

    if (pCell->IsIterCell() || pCell->GetMatrixFlag() != ScMatrixMode::NONE
        || pCell->IsHyperLinkCell() || rDoc.IsThreadedGroupCalcInProgress())
    {
        return std::nullopt;
    }

    OUString aFormulaSource = pCell->GetFormula(FormulaGrammar::GRAM_ODFF, &rContext);
    if (!setaileval::isHardRoutedFormula(
            std::u16string_view(aFormulaSource.getStr(), aFormulaSource.getLength())))
    {
        return std::nullopt;
    }

    return aFormulaSource;
}

}

void ScInterpreter::PushLookupScalarValue(const spreadsheetengine::api::CellValue& rValue)
{
    if (rValue.isError())
    {
        PushError(selibreoffice::toFormulaError(rValue.meError));
        return;
    }

    if (rValue.isText())
    {
        PushString(selibreoffice::toLibreOfficeString(rValue.maString));
        return;
    }

    if (rValue.isBoolean())
        nFuncFmtType = SvNumFormatType::LOGICAL;

    if (rValue.isNumber() || rValue.isBoolean())
    {
        PushDouble(rValue.mfNumber);
        return;
    }

    PushTempToken(new ScEmptyCellToken(false, false));
}

void ScInterpreter::PushLookupExecutionResult(
    const selookupexec::LookupExecutionResult& rResult, bool bPreserveSingleReference)
{
    switch (rResult.meKind)
    {
        case selookupexec::LookupExecutionResult::Kind::Scalar:
            PushLookupScalarValue(rResult.maScalar);
            return;
        case selookupexec::LookupExecutionResult::Kind::Reference:
            if (rResult.isSingleCellReference())
            {
                if (bPreserveSingleReference)
                    PushSingleRef(rResult.maRange.aStart.Col(), rResult.maRange.aStart.Row(),
                        rResult.maRange.aStart.Tab());
                else
                    PushCellResultToken(true, rResult.maRange.aStart, nullptr, nullptr);
            }
            else
            {
                PushDoubleRef(rResult.maRange.aStart.Col(), rResult.maRange.aStart.Row(),
                    rResult.maRange.aStart.Tab(), rResult.maRange.aEnd.Col(),
                    rResult.maRange.aEnd.Row(), rResult.maRange.aEnd.Tab());
            }
            return;
        case selookupexec::LookupExecutionResult::Kind::Matrix:
            PushMatrix(rResult.mpMatrix);
            return;
    }
}

void ScInterpreter::PushReferenceAxisPlan(const serefexec::AxisReferencePlan& rPlan)
{
    if (!rPlan.requiresMatrixResult())
    {
        PushDouble(rPlan.mfStart);
        return;
    }

    const auto aDimensions = rPlan.resultDimensions();
    ScMatrixRef pResultMatrix
        = GetNewMat(static_cast<SCSIZE>(aDimensions.mnColumns), static_cast<SCSIZE>(aDimensions.mnRows),
            /*bEmpty*/ true);
    if (!pResultMatrix)
    {
        PushIllegalArgument();
        return;
    }

    for (sal_Int32 nIndex = 0; nIndex < rPlan.mnLength; ++nIndex)
    {
        const double fValue = rPlan.mfStart + static_cast<double>(nIndex);
        if (rPlan.meAxis == spreadsheetengine::api::reference::ReferenceAxis::Column)
            pResultMatrix->PutDouble(fValue, static_cast<SCSIZE>(nIndex), 0);
        else
            pResultMatrix->PutDouble(fValue, 0, static_cast<SCSIZE>(nIndex));
    }

    PushMatrix(pResultMatrix);
}

spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue>
ScInterpreter::PopLookupExecutionValue(bool bAllowEmpty, bool bUseRawStackType)
{
    const auto makeTextValue = [](const OUString& rText) {
        return spreadsheetengine::api::CellValue::text(selibreoffice::toApiString(rText));
    };

    switch (bUseRawStackType ? GetRawStackType() : GetStackType())
    {
        case svMissing:
        case svEmptyCell:
            if (!bAllowEmpty)
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::failure(
                    spreadsheetengine::api::Error::IllegalArgument);
            }
            Pop();
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(
                spreadsheetengine::api::CellValue::empty());
        case svDouble:
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(
                spreadsheetengine::api::CellValue::number(GetDouble()));
        case svString:
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(makeTextValue(GetString().getString()));
        case svDoubleRef:
        case svSingleRef:
        {
            ScAddress aAddress;
            if (!PopDoubleRefOrSingleRef(aAddress))
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::failure(
                    spreadsheetengine::api::Error::IllegalArgument);
            }

            ScRefCellValue aCell(mrDoc, aAddress);
            if (aCell.hasNumeric())
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::success(
                    spreadsheetengine::api::CellValue::number(GetCellValue(aAddress, aCell)));
            }

            svl::SharedString aString;
            GetCellString(aString, aCell);
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(makeTextValue(aString.getString()));
        }
        case svExternalSingleRef:
        {
            ScExternalRefCache::TokenRef pToken;
            PopExternalSingleRef(pToken);
            if (nGlobalError != FormulaError::NONE)
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::failure(
                    selibreoffice::toApiError(nGlobalError));
            }

            if (pToken->GetType() == svDouble)
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::success(
                    spreadsheetengine::api::CellValue::number(pToken->GetDouble()));
            }

            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(
                makeTextValue(pToken->GetString().getString()));
        }
        case svExternalDoubleRef:
        case svMatrix:
        {
            double fValue = 0.0;
            svl::SharedString aString;
            const ScMatValType nType = GetDoubleOrStringFromMatrix(fValue, aString);
            if (nGlobalError != FormulaError::NONE)
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::failure(
                    selibreoffice::toApiError(nGlobalError));
            }

            if (ScMatrix::IsNonValueType(nType))
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::success(makeTextValue(aString.getString()));
            }

            if (ScMatrix::IsBooleanType(nType))
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::success(
                    spreadsheetengine::api::CellValue::boolean(fValue != 0.0));
            }

            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(
                spreadsheetengine::api::CellValue::number(fValue));
        }
        default:
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
    }
}

spreadsheetengine::api::ValueResult<selookupexec::LookupInput>
ScInterpreter::PopLookupExecutionInput(bool bAllowScalar, bool bRequireVector)
{
    auto validateInput = [bRequireVector](const selookupexec::LookupInput& rInput) {
        if (!bRequireVector || rInput.mbScalar)
        {
            return spreadsheetengine::api::ValueResult<selookupexec::LookupInput>::success(
                rInput);
        }

        const auto aLayout = selookup::detectVectorLayout({ rInput.mnColumns, rInput.mnRows });
        if (!aLayout)
        {
            return spreadsheetengine::api::ValueResult<selookupexec::LookupInput>::failure(
                aLayout.meError);
        }
        return spreadsheetengine::api::ValueResult<selookupexec::LookupInput>::success(rInput);
    };

    auto buildRangeInput = [&](const ScRange& rRange) {
        selookupexec::LookupInputSource aSource;
        aSource.moRange = rRange;
        const auto aInput = selookupexec::detail::buildLookupInput(aSource);
        if (!aInput)
            return spreadsheetengine::api::ValueResult<selookupexec::LookupInput>::failure(aInput.meError);
        return validateInput(aInput.maValue);
    };

    auto buildMatrixInput = [&](const ScMatrixRef& pMatrix) {
        if (!pMatrix)
        {
            return spreadsheetengine::api::ValueResult<selookupexec::LookupInput>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
        }
        selookupexec::LookupInputSource aSource;
        aSource.mpMatrix = pMatrix;
        const auto aInput = selookupexec::detail::buildLookupInput(aSource);
        if (!aInput)
            return spreadsheetengine::api::ValueResult<selookupexec::LookupInput>::failure(aInput.meError);
        return validateInput(aInput.maValue);
    };

    switch (GetStackType())
    {
        case svSingleRef:
        {
            SCCOL nCol = 0;
            SCROW nRow = 0;
            SCTAB nTab = 0;
            PopSingleRef(nCol, nRow, nTab);
            return buildRangeInput(ScRange(nCol, nRow, nTab, nCol, nRow, nTab));
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
            if (nTab1 != nTab2)
            {
                return spreadsheetengine::api::ValueResult<selookupexec::LookupInput>::failure(
                    spreadsheetengine::api::Error::IllegalArgument);
            }
            return buildRangeInput(ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2));
        }
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
            return buildMatrixInput(GetMatrix());
        case svDouble:
            if (!bAllowScalar)
                break;
            return validateInput(selookupexec::detail::buildLookupScalarInput(
                spreadsheetengine::api::CellValue::number(GetDouble())));
        case svString:
            if (!bAllowScalar)
                break;
            return validateInput(selookupexec::detail::buildLookupScalarInput(
                spreadsheetengine::api::CellValue::text(
                    selibreoffice::toApiString(GetString().getString()))));
        default:
            break;
    }

    return spreadsheetengine::api::ValueResult<selookupexec::LookupInput>::failure(
        spreadsheetengine::api::Error::IllegalArgument);
}

void ScInterpreter::ScIfJump()
{
    const short* pJump = pCur->GetJump();
    short nJumpCount = pJump[ 0 ];
    MatrixJumpConditionToMatrix();
    if ( GetStackType() != svMatrix )
    {
        ScIfJumpNotMatrix(pJump, nJumpCount);
        return;
    }

    ScMatrixRef pMat = PopMatrix();
    if ( !pMat )
    {
        PushIllegalParameter();
        return;
    }

    FormulaConstTokenRef xNew;
    ScTokenMatrixMap::const_iterator aMapIter;
    // DoubleError handled by JumpMatrix
    pMat->SetErrorInterpreter( nullptr);
    SCSIZE nCols, nRows;
    pMat->GetDimensions( nCols, nRows );
    if ( nCols == 0 || nRows == 0 )
    {
        PushIllegalArgument();
        return;
    }

    if ((aMapIter = maTokenMatrixMap.find( pCur)) != maTokenMatrixMap.end())
        xNew = (*aMapIter).second;
    else
    {
        std::shared_ptr<ScJumpMatrix> pJumpMat( std::make_shared<ScJumpMatrix>(
                    pCur->GetOpCode(), nCols, nRows));
        pMat->IfJump(*pJumpMat, pJump, nJumpCount);
        xNew = new ScJumpMatrixToken(std::move(pJumpMat));
        GetTokenMatrixMap().emplace(pCur, xNew);
    }
    if (!xNew)
    {
        PushIllegalArgument();
        return;
    }
    PushTokenRef( xNew);
    // set endpoint of path for main code line
    aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
}

void ScInterpreter::ScIfJumpNotMatrix( const short* pJump, short nJumpCount )
{
    const bool bCondition = GetBool();
    switch (selogic::selectIfBranch(bCondition, nGlobalError != FormulaError::NONE,
                nJumpCount >= 2, nJumpCount == 3))
    {
        case selogic::IfBranchAction::PropagateError:
            PushError(nGlobalError);
            aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
            break;
        case selogic::IfBranchAction::ThenPath:
            aCode.Jump( pJump[ 1 ], pJump[ nJumpCount ] );
            break;
        case selogic::IfBranchAction::ElsePath:
            aCode.Jump( pJump[ 2 ], pJump[ nJumpCount ] );
            break;
        case selogic::IfBranchAction::ReturnTrue:
            nFuncFmtType = SvNumFormatType::LOGICAL;
            PushInt(1);
            aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
            break;
        case selogic::IfBranchAction::ReturnFalse:
            nFuncFmtType = SvNumFormatType::LOGICAL;
            PushInt(0);
            aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
            break;
    }
}

void ScInterpreter::ScChooseJump()
{
    // We have to set a jump, if there was none chosen because of an error set
    // it to endpoint.
    bool bHaveJump = false;
    const short* pJump = pCur->GetJump();
    short nJumpCount = pJump[ 0 ];
    MatrixJumpConditionToMatrix();
    switch ( GetStackType() )
    {
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                PushIllegalParameter();
            else
            {
                FormulaConstTokenRef xNew;
                ScTokenMatrixMap::const_iterator aMapIter;
                // DoubleError handled by JumpMatrix
                pMat->SetErrorInterpreter( nullptr);
                SCSIZE nCols, nRows;
                pMat->GetDimensions( nCols, nRows );
                if ( nCols == 0 || nRows == 0 )
                    PushIllegalParameter();
                else if ((aMapIter = maTokenMatrixMap.find(
                                    pCur)) != maTokenMatrixMap.end())
                    xNew = (*aMapIter).second;
                else
                {
                    std::shared_ptr<ScJumpMatrix> pJumpMat( std::make_shared<ScJumpMatrix>(
                                pCur->GetOpCode(), nCols, nRows));
                    sejumpexec::initializeChooseJumpMatrix(*pMat, *pJumpMat, pJump, nJumpCount);
                    xNew = new ScJumpMatrixToken(std::move(pJumpMat));
                    GetTokenMatrixMap().emplace(pCur, xNew);
                }
                if (xNew)
                {
                    PushTokenRef( xNew);
                    // set endpoint of path for main code line
                    aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
                    bHaveJump = true;
                }
            }
        }
        break;
        default:
        {
            sal_Int16 nJumpIndex = GetInt16();
            const auto aJumpDecision = selogic::chooseJumpIndex(nJumpIndex, nJumpCount);
            if (nGlobalError == FormulaError::NONE && aJumpDecision)
            {
                aCode.Jump( pJump[ static_cast<short>(aJumpDecision.maValue) ], pJump[ nJumpCount ] );
                bHaveJump = true;
            }
            else
                PushIllegalArgument();
        }
    }
    if (!bHaveJump)
        aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
}

bool ScInterpreter::JumpMatrix( short nStackLevel )
{
    pJumpMatrix = pStack[sp-nStackLevel]->GetJumpMatrix();
    bool bHasResMat = pJumpMatrix->HasResultMatrix();
    SCSIZE nC, nR;
    if ( nStackLevel == 2 )
    {
        if ( aCode.HasStacked() )
            aCode.Pop();    // pop what Jump() pushed
        else
        {
            assert(!"pop goes the weasel");
        }

        if ( !bHasResMat )
        {
            Pop();
            SetError( FormulaError::UnknownStackVariable );
        }
        else
        {
            pJumpMatrix->GetPos( nC, nR );
            switch ( GetStackType() )
            {
                case svDouble:
                {
                    double fVal = GetDouble();
                    if ( nGlobalError != FormulaError::NONE )
                    {
                        fVal = CreateDoubleError( nGlobalError );
                        nGlobalError = FormulaError::NONE;
                    }
                    pJumpMatrix->PutResultDouble( fVal, nC, nR );
                }
                break;
                case svString:
                {
                    svl::SharedString aStr = GetString();
                    if ( nGlobalError != FormulaError::NONE )
                    {
                        pJumpMatrix->PutResultDouble( CreateDoubleError( nGlobalError),
                                nC, nR);
                        nGlobalError = FormulaError::NONE;
                    }
                    else
                        pJumpMatrix->PutResultString(aStr, nC, nR);
                }
                break;
                case svSingleRef:
                {
                    FormulaConstTokenRef xRef = pStack[sp-1];
                    ScAddress aAdr;
                    PopSingleRef( aAdr );
                    if ( nGlobalError != FormulaError::NONE )
                    {
                        pJumpMatrix->PutResultDouble( CreateDoubleError( nGlobalError),
                                nC, nR);
                        nGlobalError = FormulaError::NONE;
                    }
                    else
                    {
                        ScRefCellValue aCell(mrDoc, aAdr);
                        if (aCell.hasEmptyValue())
                            pJumpMatrix->PutResultEmpty( nC, nR );
                        else if (aCell.hasNumeric())
                        {
                            double fVal = GetCellValue(aAdr, aCell);
                            if ( nGlobalError != FormulaError::NONE )
                            {
                                fVal = CreateDoubleError(
                                        nGlobalError);
                                nGlobalError = FormulaError::NONE;
                            }
                            pJumpMatrix->PutResultDouble( fVal, nC, nR );
                        }
                        else
                        {
                            svl::SharedString aStr;
                            GetCellString(aStr, aCell);
                            if ( nGlobalError != FormulaError::NONE )
                            {
                                pJumpMatrix->PutResultDouble( CreateDoubleError(
                                            nGlobalError), nC, nR);
                                nGlobalError = FormulaError::NONE;
                            }
                            else
                                pJumpMatrix->PutResultString(aStr, nC, nR);
                        }
                    }

                    formula::ParamClass eReturnType = ScParameterClassification::GetParameterType( pCur, SAL_MAX_UINT16);
                    if (eReturnType == ParamClass::Reference)
                    {
                        /* TODO: What about error handling and do we actually
                         * need the result matrix above at all in this case? */
                        ScComplexRefData aRef;
                        aRef.Ref1 = aRef.Ref2 = *(xRef->GetSingleRef());
                        pJumpMatrix->GetRefList().push_back( aRef);
                    }
                }
                break;
                case svDoubleRef:
                {   // upper left plus offset within matrix
                    FormulaConstTokenRef xRef = pStack[sp-1];
                    double fVal;
                    ScRange aRange;
                    PopDoubleRef( aRange );
                    if ( nGlobalError != FormulaError::NONE )
                    {
                        fVal = CreateDoubleError( nGlobalError );
                        nGlobalError = FormulaError::NONE;
                        pJumpMatrix->PutResultDouble( fVal, nC, nR );
                    }
                    else
                    {
                        // Do not modify the original range because we use it
                        // to adjust the size of the result matrix if necessary.
                        ScAddress aAdr( aRange.aStart);
                        sal_uLong nCol = static_cast<sal_uLong>(aAdr.Col()) + nC;
                        sal_uLong nRow = static_cast<sal_uLong>(aAdr.Row()) + nR;
                        if ((nCol > o3tl::make_unsigned(aRange.aEnd.Col()) &&
                                    aRange.aEnd.Col() != aRange.aStart.Col())
                                || (nRow > o3tl::make_unsigned(aRange.aEnd.Row()) &&
                                    aRange.aEnd.Row() != aRange.aStart.Row()))
                        {
                            fVal = CreateDoubleError( FormulaError::NotAvailable );
                            pJumpMatrix->PutResultDouble( fVal, nC, nR );
                        }
                        else
                        {
                            // Replicate column and/or row of a vector if it is
                            // one. Note that this could be a range reference
                            // that in fact consists of only one cell, e.g. A1:A1
                            if (aRange.aEnd.Col() == aRange.aStart.Col())
                                nCol = aRange.aStart.Col();
                            if (aRange.aEnd.Row() == aRange.aStart.Row())
                                nRow = aRange.aStart.Row();
                            aAdr.SetCol( static_cast<SCCOL>(nCol) );
                            aAdr.SetRow( static_cast<SCROW>(nRow) );
                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (aCell.hasEmptyValue())
                                pJumpMatrix->PutResultEmpty( nC, nR );
                            else if (aCell.hasNumeric())
                            {
                                double fCellVal = GetCellValue(aAdr, aCell);
                                if ( nGlobalError != FormulaError::NONE )
                                {
                                    fCellVal = CreateDoubleError(
                                            nGlobalError);
                                    nGlobalError = FormulaError::NONE;
                                }
                                pJumpMatrix->PutResultDouble( fCellVal, nC, nR );
                            }
                            else
                            {
                                svl::SharedString aStr;
                                GetCellString(aStr, aCell);
                                if ( nGlobalError != FormulaError::NONE )
                                {
                                    pJumpMatrix->PutResultDouble( CreateDoubleError(
                                                nGlobalError), nC, nR);
                                    nGlobalError = FormulaError::NONE;
                                }
                                else
                                    pJumpMatrix->PutResultString(aStr, nC, nR);
                            }
                        }
                        SCSIZE nParmCols = aRange.aEnd.Col() - aRange.aStart.Col() + 1;
                        SCSIZE nParmRows = aRange.aEnd.Row() - aRange.aStart.Row() + 1;
                        sejumpmatrixexec::adjustResultMatrixDimensions(
                            *pJumpMatrix, nParmCols, nParmRows);
                    }

                    formula::ParamClass eReturnType = ScParameterClassification::GetParameterType( pCur, SAL_MAX_UINT16);
                    if (eReturnType == ParamClass::Reference)
                    {
                        /* TODO: What about error handling and do we actually
                         * need the result matrix above at all in this case? */
                        pJumpMatrix->GetRefList().push_back( *(xRef->GetDoubleRef()));
                    }
                }
                break;
                case svExternalSingleRef:
                {
                    ScExternalRefCache::TokenRef pToken;
                    PopExternalSingleRef(pToken);
                    if (nGlobalError != FormulaError::NONE)
                    {
                        pJumpMatrix->PutResultDouble( CreateDoubleError( nGlobalError), nC, nR );
                        nGlobalError = FormulaError::NONE;
                    }
                    else
                    {
                        switch (pToken->GetType())
                        {
                            case svDouble:
                                pJumpMatrix->PutResultDouble( pToken->GetDouble(), nC, nR );
                            break;
                            case svString:
                                pJumpMatrix->PutResultString( pToken->GetString(), nC, nR );
                            break;
                            case svEmptyCell:
                                pJumpMatrix->PutResultEmpty( nC, nR );
                            break;
                            default:
                                // svError was already handled (set by
                                // PopExternalSingleRef()) with nGlobalError
                                // above.
                                assert(!"unhandled svExternalSingleRef case");
                                pJumpMatrix->PutResultDouble( CreateDoubleError(
                                            FormulaError::UnknownStackVariable), nC, nR );
                        }
                    }
                }
                break;
                case svExternalDoubleRef:
                case svMatrix:
                {   // match matrix offsets
                    double fVal;
                    ScMatrixRef pMat = GetMatrix();
                    if ( nGlobalError != FormulaError::NONE )
                    {
                        fVal = CreateDoubleError( nGlobalError );
                        nGlobalError = FormulaError::NONE;
                        pJumpMatrix->PutResultDouble( fVal, nC, nR );
                    }
                    else if ( !pMat )
                    {
                        fVal = CreateDoubleError( FormulaError::UnknownVariable );
                        pJumpMatrix->PutResultDouble( fVal, nC, nR );
                    }
                    else
                    {
                        SCSIZE nCols, nRows;
                        pMat->GetDimensions( nCols, nRows );
                        if ((nCols <= nC && nCols != 1) ||
                            (nRows <= nR && nRows != 1))
                        {
                            fVal = CreateDoubleError( FormulaError::NotAvailable );
                            pJumpMatrix->PutResultDouble( fVal, nC, nR );
                        }
                        else
                        {
                            // GetMatrix() does SetErrorInterpreter() at the
                            // matrix, do not propagate an error from
                            // matrix->GetValue() as global error.
                            pMat->SetErrorInterpreter(nullptr);
                            sejumpexec::storeJumpMatrixResult(*pMat, *pJumpMatrix, nC, nR);
                        }
                        sejumpmatrixexec::adjustResultMatrixDimensions(
                            *pJumpMatrix, nCols, nRows);
                    }
                }
                break;
                case svError:
                {
                    PopError();
                    double fVal = CreateDoubleError( nGlobalError);
                    nGlobalError = FormulaError::NONE;
                    pJumpMatrix->PutResultDouble( fVal, nC, nR );
                }
                break;
                default:
                {
                    Pop();
                    double fVal = CreateDoubleError( FormulaError::IllegalArgument);
                    pJumpMatrix->PutResultDouble( fVal, nC, nR );
                }
            }
        }
    }
    const auto aPendingJump = sejumpmatrixexec::advanceToPendingJump(*pJumpMatrix, bHasResMat);
    if (aPendingJump.mbHasPendingJump)
    {
        const ScTokenVec & rParams = pJumpMatrix->GetJumpParameters();
        for ( auto const & i : rParams )
        {
            // This is not the current state of the interpreter, so
            // push without error, and elements' errors are coded into
            // double.
            PushWithoutError(*i);
        }
        aCode.Jump(aPendingJump.mnStart, aPendingJump.mnNext, aPendingJump.mnStop);
        return false;
    }
    // We're done with it, throw away jump matrix, keep result.
    // For an intermediate result of Reference use the array of references
    // if there are more than one reference and the current ForceArray
    // context is ReferenceOrRefArray.
    // Else (also for a final result of Reference) use the matrix.
    // Treat the result of a jump command as final and use the matrix (see
    // tdf#115493 for why).
    if (sejumpmatrixexec::shouldReturnReferenceList(
            pCur->GetInForceArray() == ParamClass::ReferenceOrRefArray,
            pJumpMatrix->GetRefList().size(),
            ScParameterClassification::GetParameterType(pCur, SAL_MAX_UINT16)
                == ParamClass::Reference,
            FormulaCompiler::IsOpCodeJumpCommand(pJumpMatrix->GetOpCode()),
            aCode.PeekNextOperator()))
    {
        FormulaTokenRef xRef = new ScRefListToken(true);
        *(xRef->GetRefList()) = pJumpMatrix->GetRefList();
        pJumpMatrix = nullptr;
        Pop();
        PushTokenRef( xRef);
        maTokenMatrixMap.erase( pCur);
        // There's no result matrix to remember in this case.
    }
    else
    {
        ScMatrix* pResMat = pJumpMatrix->GetResultMatrix();
        pJumpMatrix = nullptr;
        Pop();
        PushMatrix( pResMat );
        // Remove jump matrix from map and remember result matrix in case it
        // could be reused in another path of the same condition.
        maTokenMatrixMap.erase( pCur);
        maTokenMatrixMap.emplace(pCur, pStack[sp-1]);
    }
    return true;
}

double ScInterpreter::Compare( ScQueryOp eOp )
{
    sc::Compare aComp;
    aComp.meOp = eOp;
    aComp.mbIgnoreCase = mrDoc.GetDocOptions().IsIgnoreCase();
    for( short i = 1; i >= 0; i-- )
    {
        sc::Compare::Cell& rCell = aComp.maCells[i];

        switch ( GetRawStackType() )
        {
            case svEmptyCell:
                Pop();
                rCell.mbEmpty = true;
                break;
            case svMissing:
            case svDouble:
                rCell.mfValue = GetDouble();
                rCell.mbValue = true;
                break;
            case svString:
                rCell.maStr = GetString();
                rCell.mbValue = false;
                break;
            case svDoubleRef :
            case svSingleRef :
            {
                ScAddress aAdr;
                if ( !PopDoubleRefOrSingleRef( aAdr ) )
                    break;
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasEmptyValue())
                    rCell.mbEmpty = true;
                else if (aCell.hasString())
                {
                    svl::SharedString aStr;
                    GetCellString(aStr, aCell);
                    rCell.maStr = std::move(aStr);
                    rCell.mbValue = false;
                }
                else
                {
                    rCell.mfValue = GetCellValue(aAdr, aCell);
                    rCell.mbValue = true;
                }
            }
            break;
            case svExternalSingleRef:
            {
                ScMatrixRef pMat = GetMatrix();
                if (!pMat)
                {
                    SetError( FormulaError::IllegalParameter);
                    break;
                }

                SCSIZE nC, nR;
                pMat->GetDimensions(nC, nR);
                if (!nC || !nR)
                {
                    SetError( FormulaError::IllegalParameter);
                    break;
                }
                if (pMat->IsEmpty(0, 0))
                    rCell.mbEmpty = true;
                else if (pMat->IsStringOrEmpty(0, 0))
                {
                    rCell.maStr = pMat->GetString(0, 0);
                    rCell.mbValue = false;
                }
                else
                {
                    rCell.mfValue = pMat->GetDouble(0, 0);
                    rCell.mbValue = true;
                }
            }
            break;
            case svExternalDoubleRef:
                // TODO: Find out how to handle this...
                // Xcl generates a position dependent intersection using
                // col/row, as it seems to do for all range references, not
                // only in compare context. We'd need a general implementation
                // for that behavior similar to svDoubleRef in scalar and array
                // mode. Which also means we'd have to change all places where
                // it currently is handled along with svMatrix.
            default:
                PopError();
                SetError( FormulaError::IllegalParameter);
            break;
        }
    }
    if( nGlobalError != FormulaError::NONE )
        return 0;
    nCurFmtType = nFuncFmtType = SvNumFormatType::LOGICAL;
    return sc::CompareFunc(aComp);
}

sc::RangeMatrix ScInterpreter::CompareMat( ScQueryOp eOp, sc::CompareOptions* pOptions )
{
    sc::Compare aComp;
    aComp.meOp = eOp;
    aComp.mbIgnoreCase = mrDoc.GetDocOptions().IsIgnoreCase();
    sc::RangeMatrix aMat[2];
    ScAddress aAdr;
    for( short i = 1; i >= 0; i-- )
    {
        sc::Compare::Cell& rCell = aComp.maCells[i];

        switch (GetRawStackType())
        {
            case svEmptyCell:
                Pop();
                rCell.mbEmpty = true;
                break;
            case svMissing:
            case svDouble:
                rCell.mfValue = GetDouble();
                rCell.mbValue = true;
                break;
            case svString:
                rCell.maStr = GetString();
                rCell.mbValue = false;
                break;
            case svSingleRef:
            {
                PopSingleRef( aAdr );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasEmptyValue())
                    rCell.mbEmpty = true;
                else if (aCell.hasString())
                {
                    svl::SharedString aStr;
                    GetCellString(aStr, aCell);
                    rCell.maStr = std::move(aStr);
                    rCell.mbValue = false;
                }
                else
                {
                    rCell.mfValue = GetCellValue(aAdr, aCell);
                    rCell.mbValue = true;
                }
            }
            break;
            case svExternalSingleRef:
            case svExternalDoubleRef:
            case svDoubleRef:
            case svMatrix:
                aMat[i] = GetRangeMatrix();
                if (!aMat[i].mpMat)
                    SetError( FormulaError::IllegalParameter);
                else
                    aMat[i].mpMat->SetErrorInterpreter(nullptr);
                    // errors are transported as DoubleError inside matrix
                break;
            default:
                PopError();
                SetError( FormulaError::IllegalParameter);
            break;
        }
    }

    sc::RangeMatrix aRes;

    if (nGlobalError != FormulaError::NONE)
    {
        nCurFmtType = nFuncFmtType = SvNumFormatType::LOGICAL;
        return aRes;
    }

    if (aMat[0].mpMat && aMat[1].mpMat)
    {
        SCSIZE nC0, nC1;
        SCSIZE nR0, nR1;
        aMat[0].mpMat->GetDimensions(nC0, nR0);
        aMat[1].mpMat->GetDimensions(nC1, nR1);
        SCSIZE nC = std::max( nC0, nC1 );
        SCSIZE nR = std::max( nR0, nR1 );
        aRes.mpMat = GetNewMat( nC, nR, /*bEmpty*/true );
        if (!aRes.mpMat)
            return aRes;
        for ( SCSIZE j=0; j<nC; j++ )
        {
            for ( SCSIZE k=0; k<nR; k++ )
            {
                SCSIZE nCol = j, nRow = k;
                if (aMat[0].mpMat->ValidColRowOrReplicated(nCol, nRow) &&
                    aMat[1].mpMat->ValidColRowOrReplicated(nCol, nRow))
                {
                    for ( short i=1; i>=0; i-- )
                    {
                        sc::Compare::Cell& rCell = aComp.maCells[i];

                        if (aMat[i].mpMat->IsStringOrEmpty(j, k))
                        {
                            rCell.mbValue = false;
                            rCell.maStr = aMat[i].mpMat->GetString(j, k);
                            rCell.mbEmpty = aMat[i].mpMat->IsEmpty(j, k);
                        }
                        else
                        {
                            rCell.mbValue = true;
                            rCell.mfValue = aMat[i].mpMat->GetDouble(j, k);
                            rCell.mbEmpty = false;
                        }
                    }
                    aRes.mpMat->PutDouble( sc::CompareFunc( aComp, pOptions), j, k);
                }
                else
                    aRes.mpMat->PutError( FormulaError::NoValue, j, k);
            }
        }

        switch (eOp)
        {
            case SC_EQUAL:
                aRes.mpMat->CompareEqual();
                break;
            case SC_LESS:
                aRes.mpMat->CompareLess();
                break;
            case SC_GREATER:
                aRes.mpMat->CompareGreater();
                break;
            case SC_LESS_EQUAL:
                aRes.mpMat->CompareLessEqual();
                break;
            case SC_GREATER_EQUAL:
                aRes.mpMat->CompareGreaterEqual();
                break;
            case SC_NOT_EQUAL:
                aRes.mpMat->CompareNotEqual();
                break;
            default:
                SAL_WARN("sc",  "ScInterpreter::QueryMat: unhandled comparison operator: " << static_cast<int>(eOp));
                aRes.mpMat.reset();
                return aRes;
        }
    }
    else if (aMat[0].mpMat || aMat[1].mpMat)
    {
        size_t i = ( aMat[0].mpMat ? 0 : 1);

        aRes.mnCol1 = aMat[i].mnCol1;
        aRes.mnRow1 = aMat[i].mnRow1;
        aRes.mnTab1 = aMat[i].mnTab1;
        aRes.mnCol2 = aMat[i].mnCol2;
        aRes.mnRow2 = aMat[i].mnRow2;
        aRes.mnTab2 = aMat[i].mnTab2;

        ScMatrix& rMat = *aMat[i].mpMat;
        aRes.mpMat = rMat.CompareMatrix(aComp, i, pOptions);
        if (!aRes.mpMat)
            return aRes;
    }

    nCurFmtType = nFuncFmtType = SvNumFormatType::LOGICAL;
    return aRes;
}

ScMatrixRef ScInterpreter::QueryMat( const ScMatrixRef& pMat, sc::CompareOptions& rOptions )
{
    SvNumFormatType nSaveCurFmtType = nCurFmtType;
    SvNumFormatType nSaveFuncFmtType = nFuncFmtType;
    PushMatrix( pMat);
    const ScQueryEntry::Item& rItem = rOptions.aQueryEntry.GetQueryItem();
    if (rItem.meType == ScQueryEntry::ByString)
        PushString(rItem.maString.getString());
    else
        PushDouble(rItem.mfVal);
    ScMatrixRef pResultMatrix = CompareMat(rOptions.aQueryEntry.eOp, &rOptions).mpMat;
    nCurFmtType = nSaveCurFmtType;
    nFuncFmtType = nSaveFuncFmtType;
    if (nGlobalError != FormulaError::NONE || !pResultMatrix)
    {
        SetError( FormulaError::IllegalParameter);
        return pResultMatrix;
    }

    return pResultMatrix;
}

void ScInterpreter::ScCompareOp(seinterpre::ComparisonMode eMode, ScQueryOp eOp)
{
    if (GetStackType(1) == svMatrix || GetStackType(2) == svMatrix)
    {
        sc::RangeMatrix aMat = CompareMat(eOp);
        if (!aMat.mpMat)
        {
            PushIllegalParameter();
            return;
        }

        PushMatrix(aMat);
        return;
    }

    PushInt(int(seinterpre::matchesComparisonResult(Compare(eOp), eMode)));
}

void ScInterpreter::ScLogicalFoldOp(seinterpre::LogicalFoldMode eMode)
{
    nFuncFmtType = SvNumFormatType::LOGICAL;
    short nParamCount = GetByte();
    if (!MustHaveParamCountMin(nParamCount, 1))
        return;

    bool bHaveValue = false;
    bool bRes = seinterpre::initialLogicalFoldValue(eMode);
    size_t nRefInList = 0;

    const auto foldValue = [&](bool bValue) {
        bHaveValue = true;
        bRes = seinterpre::foldLogicalValue(eMode, bRes, bValue);
    };

    while (nParamCount-- > 0)
    {
        if (nGlobalError == FormulaError::NONE)
        {
            switch (GetStackType())
            {
                case svDouble:
                    foldValue(PopDouble() != 0.0);
                break;
                case svString:
                    Pop();
                    SetError(FormulaError::NoValue);
                break;
                case svSingleRef:
                {
                    ScAddress aAdr;
                    PopSingleRef(aAdr);
                    if (nGlobalError == FormulaError::NONE)
                    {
                        ScRefCellValue aCell(mrDoc, aAdr);
                        if (aCell.hasNumeric())
                            foldValue(GetCellValue(aAdr, aCell) != 0.0);
                    }
                }
                break;
                case svDoubleRef:
                case svRefList:
                {
                    ScRange aRange;
                    PopDoubleRef(aRange, nParamCount, nRefInList);
                    if (nGlobalError == FormulaError::NONE)
                    {
                        double fVal;
                        FormulaError nErr = FormulaError::NONE;
                        ScValueIterator aValIter(mrContext, aRange);
                        if (aValIter.GetFirst(fVal, nErr))
                        {
                            do
                            {
                                foldValue(fVal != 0.0);
                            } while (nErr == FormulaError::NONE && aValIter.GetNext(fVal, nErr));
                        }
                        SetError(nErr);
                    }
                }
                break;
                case svExternalSingleRef:
                case svExternalDoubleRef:
                case svMatrix:
                {
                    ScMatrixRef pMat = GetMatrix();
                    if (pMat)
                    {
                        double fVal = 0.0;
                        switch (eMode)
                        {
                            case seinterpre::LogicalFoldMode::And:
                                fVal = pMat->And();
                            break;
                            case seinterpre::LogicalFoldMode::Or:
                                fVal = pMat->Or();
                            break;
                            case seinterpre::LogicalFoldMode::Xor:
                                fVal = pMat->Xor();
                            break;
                        }

                        FormulaError nErr = GetDoubleErrorValue(fVal);
                        if (nErr != FormulaError::NONE)
                        {
                            SetError(nErr);
                            bRes = false;
                        }
                        else
                            foldValue(fVal != 0.0);
                    }
                }
                break;
                default:
                    PopError();
                    SetError(FormulaError::IllegalParameter);
            }
        }
        else
            Pop();
    }

    if (bHaveValue)
        PushInt(int(bRes));
    else
        PushNoValue();
}

void ScInterpreter::ScUnaryMatrixOrScalarOp(seinterpre::UnaryMatrixScalarMode eMode)
{
    switch (GetStackType())
    {
        case svMatrix:
        {
            ScMatrixRef pMat = GetMatrix();
            if (!pMat)
                PushIllegalParameter();
            else
            {
                SCSIZE nC, nR;
                pMat->GetDimensions(nC, nR);
                ScMatrixRef pResMat = GetNewMat(nC, nR, /*bEmpty*/true);
                if (!pResMat)
                    PushIllegalArgument();
                else
                {
                    if (eMode == seinterpre::UnaryMatrixScalarMode::Negate)
                        pMat->NegOp(*pResMat);
                    else
                        pMat->NotOp(*pResMat);
                    PushMatrix(pResMat);
                }
            }
        }
        break;
        default:
            if (eMode == seinterpre::UnaryMatrixScalarMode::Negate)
                PushDouble(-GetDouble());
            else
                PushInt(int(GetDouble() == 0.0));
    }
}

void ScInterpreter::ScSyntheticBinaryOp(OpCode eOpCode, void (ScInterpreter::*pOperation)())
{
    const FormulaToken* pSaveCur = pCur;
    const sal_uInt8 nSavePar = cPar;
    cPar = 2;
    FormulaByteToken aOperation(eOpCode, cPar);
    pCur = &aOperation;
    (this->*pOperation)();
    pCur = pSaveCur;
    cPar = nSavePar;
}

void ScInterpreter::ScMatchOp(bool bExtended)
{
    const sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, bExtended ? 4 : 3))
        return;

    selookupexec::MatchExecutionRequest aRequest;
    aRequest.mbExtended = bExtended;
    aRequest.mbAllowPatternMatch = bExtended;
    aRequest.meSearchType = toApiSearchType(mrDoc.GetDocOptions().GetFormulaSearchType());

    if (bExtended)
    {
        if (nParamCount == 4)
        {
            const auto aSearchMode = selookup::normalizeSearchMode(GetInt16());
            if (!aSearchMode)
            {
                PushIllegalParameter();
                return;
            }
            aRequest.meSearchMode = aSearchMode.maValue;
        }

        if (nParamCount >= 3)
        {
            const auto aMatchMode = selookup::normalizeExtendedMatchMode(GetInt16());
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
        const auto aModePlan = selookup::normalizeMatchType(nParamCount == 3 ? GetDouble() : 1.0);
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
        }
        break;
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
        }
        break;
        case svMatrix:
        {
            aRequest.maSearchSource.mpMatrix = PopMatrix();
            if (!aRequest.maSearchSource.mpMatrix)
            {
                PushIllegalParameter();
                return;
            }
        }
        break;
        case svExternalDoubleRef:
        {
            PopExternalDoubleRef(aRequest.maSearchSource.mpMatrix);
            if (!aRequest.maSearchSource.mpMatrix)
            {
                PushIllegalParameter();
                return;
            }
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

    switch (bExtended ? GetRawStackType() : GetStackType())
    {
        case svMissing:
        case svEmptyCell:
        {
            if (!bExtended)
            {
                PushIllegalParameter();
                return;
            }
            Pop();
            aRequest.maLookupValue = spreadsheetengine::api::CellValue::empty();
        }
        break;
        case svDouble:
            aRequest.maLookupValue = spreadsheetengine::api::CellValue::number(GetDouble());
        break;
        case svString:
            aRequest.maLookupValue = makeTextValue(GetString().getString());
        break;
        case svDoubleRef:
        case svSingleRef:
        {
            ScAddress aAddress;
            if (!PopDoubleRefOrSingleRef(aAddress))
            {
                PushInt(0);
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
                GetCellString(aString, aCell);
                aRequest.maLookupValue = makeTextValue(aString.getString());
            }
        }
        break;
        case svExternalSingleRef:
        {
            ScExternalRefCache::TokenRef pToken;
            PopExternalSingleRef(pToken);
            if (nGlobalError != FormulaError::NONE)
            {
                PushError(nGlobalError);
                return;
            }
            if (pToken->GetType() == svDouble)
                aRequest.maLookupValue
                    = spreadsheetengine::api::CellValue::number(pToken->GetDouble());
            else
                aRequest.maLookupValue = makeTextValue(pToken->GetString().getString());
        }
        break;
        case svExternalDoubleRef:
        case svMatrix:
        {
            double fValue = 0.0;
            svl::SharedString aString;
            const ScMatValType nType = GetDoubleOrStringFromMatrix(fValue, aString);
            if (nGlobalError != FormulaError::NONE)
            {
                PushError(nGlobalError);
                return;
            }

            if (ScMatrix::IsNonValueType(nType))
                aRequest.maLookupValue = makeTextValue(aString.getString());
            else if (ScMatrix::IsBooleanType(nType))
                aRequest.maLookupValue
                    = spreadsheetengine::api::CellValue::boolean(fValue != 0.0);
            else
                aRequest.maLookupValue = spreadsheetengine::api::CellValue::number(fValue);
        }
        break;
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

void ScInterpreter::ScRandomImpl( const std::function<double( double fFirst, double fLast )>& RandomFunc,
        double fFirst, double fLast )
{
    if (bMatrixFormula)
    {
        SCCOL nCols = 0;
        SCROW nRows = 0;
        // In JumpMatrix context use its dimensions for the return matrix; the
        // formula cell range selected may differ, for example if the result is
        // to be transposed.
        if (GetStackType(1) == svJumpMatrix)
        {
            SCSIZE nC, nR;
            pStack[sp-1]->GetJumpMatrix()->GetDimensions( nC, nR);
            nCols = std::max<SCCOL>(0, static_cast<SCCOL>(nC));
            nRows = std::max<SCROW>(0, static_cast<SCROW>(nR));
        }
        else if (pMyFormulaCell)
            pMyFormulaCell->GetMatColsRows( nCols, nRows);

        if (nCols == 1 && nRows == 1)
        {
            // For compatibility with existing
            // com.sun.star.sheet.FunctionAccess.callFunction() calls that per
            // default are executed in array context unless
            // FA.setPropertyValue("IsArrayFunction",False) was set, return a
            // scalar double instead of a 1x1 matrix object. tdf#128218
            PushDouble( RandomFunc( fFirst, fLast));
            return;
        }

        // ScViewFunc::EnterMatrix() might be asking for
        // ScFormulaCell::GetResultDimensions(), which here are none so create
        // a 1x1 matrix at least which exactly is the case when EnterMatrix()
        // asks for a not selected range.
        if (nCols == 0)
            nCols = 1;
        if (nRows == 0)
            nRows = 1;
        ScMatrixRef pResMat = GetNewMat( static_cast<SCSIZE>(nCols), static_cast<SCSIZE>(nRows), /*bEmpty*/true );
        if (!pResMat)
            PushError( FormulaError::MatrixSize);
        else
        {
            for (SCCOL i=0; i < nCols; ++i)
            {
                for (SCROW j=0; j < nRows; ++j)
                {
                    pResMat->PutDouble( RandomFunc( fFirst, fLast),
                            static_cast<SCSIZE>(i), static_cast<SCSIZE>(j));
                }
            }
            PushMatrix( pResMat);
        }
    }
    else
    {
        PushDouble( RandomFunc( fFirst, fLast));
    }
}

void ScInterpreter::ScRandom()
{
    auto RandomFunc = [this]( double, double )
    {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(mrContext.aRNG);
    };
    ScRandomImpl( RandomFunc, 0.0, 0.0 );
}

void ScInterpreter::ScRandArray()
{
    sal_uInt8 nParamCount = GetByte();
    // optional 5th para:
    // TRUE for a whole number
    // FALSE for a decimal number - default.
    bool bWholeNumber = false;
    if (nParamCount == 5)
        bWholeNumber = GetBoolWithDefault(false);

    // optional 4th para: The maximum value of the random numbers
    double fMax = 1.0;
    if (nParamCount >= 4)
        fMax = GetDoubleWithDefault(1.0);

    // optional 3rd para: The minimum value of the random numbers
    double fMin = 0.0;
    if (nParamCount >= 3)
        fMin = GetDoubleWithDefault(0.0);

    // optional 2nd para: The number of columns of the return array
    SCCOL nCols = 1;
    if (nParamCount >= 2)
        nCols = static_cast<SCCOL>(GetInt32WithDefault(1));

    // optional 1st para: The number of rows of the return array
    SCROW nRows = 1;
    if (nParamCount >= 1)
        nRows = static_cast<SCROW>(GetInt32WithDefault(1));

    if (bWholeNumber)
    {
        fMax = rtl::math::round(fMax, 0, rtl_math_RoundingMode_Up);
        fMin = rtl::math::round(fMin, 0, rtl_math_RoundingMode_Up);
    }

    if (nGlobalError != FormulaError::NONE || fMin > fMax || nCols <= 0 || nRows <= 0)
    {
        PushIllegalArgument();
        return;
    }

    if (bWholeNumber)
        fMax = std::nextafter(fMax + 1, -DBL_MAX);
    else
        fMax = std::nextafter(fMax, DBL_MAX);

    auto RandomFunc = [this](double fFirst, double fLast, bool bWholeNum)
        {
            std::uniform_real_distribution<double> dist(fFirst, fLast);
            if (bWholeNum)
                return floor(dist(mrContext.aRNG));
            else
                return dist(mrContext.aRNG);
        };

    if (nCols == 1 && nRows == 1)
    {
        PushDouble(RandomFunc(fMin, fMax, bWholeNumber));
        return;
    }

    ScMatrixRef pResMat = GetNewMat(static_cast<SCSIZE>(nCols), static_cast<SCSIZE>(nRows), /*bEmpty*/true);
    if (!pResMat)
        PushError(FormulaError::MatrixSize);
    else
    {
        for (SCCOL i = 0; i < nCols; ++i)
        {
            for (SCROW j = 0; j < nRows; ++j)
            {
                pResMat->PutDouble(RandomFunc(fMin, fMax, bWholeNumber),
                    static_cast<SCSIZE>(i), static_cast<SCSIZE>(j));
            }
        }
        PushMatrix(pResMat);
    }
}

void ScInterpreter::ScRandbetween()
{
    if (!MustHaveParamCount( GetByte(), 2))
        return;

    // Same like scaddins/source/analysis/analysis.cxx
    // AnalysisAddIn::getRandbetween()
    double fMax = rtl::math::round( GetDouble(), 0, rtl_math_RoundingMode_Up);
    double fMin = rtl::math::round( GetDouble(), 0, rtl_math_RoundingMode_Up);
    if (nGlobalError != FormulaError::NONE || fMin > fMax)
    {
        PushIllegalArgument();
        return;
    }
    fMax = std::nextafter( fMax+1, -DBL_MAX);
    auto RandomFunc = [this]( double fFirst, double fLast )
    {
        std::uniform_real_distribution<double> dist(fFirst, fLast);
        return floor(dist(mrContext.aRNG));
    };
    ScRandomImpl( RandomFunc, fMin, fMax);
}

bool ScInterpreter::IsString()
{
    nFuncFmtType = SvNumFormatType::LOGICAL;
    bool bRes = false;
    switch ( GetRawStackType() )
    {
        case svString:
            Pop();
            bRes = true;
        break;
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;

            ScRefCellValue aCell(mrDoc, aAdr);
            if (GetCellErrCode(aCell) == FormulaError::NONE)
            {
                switch (aCell.getType())
                {
                    case CELLTYPE_STRING :
                    case CELLTYPE_EDIT :
                        bRes = true;
                        break;
                    case CELLTYPE_FORMULA :
                        bRes = (!aCell.getFormula()->IsValue() && !aCell.getFormula()->IsEmpty());
                        break;
                    default:
                        ; // nothing
                }
            }
        }
        break;
        case svExternalSingleRef:
        {
            ScExternalRefCache::TokenRef pToken;
            PopExternalSingleRef(pToken);
            if (nGlobalError == FormulaError::NONE && pToken->GetType() == svString)
                bRes = true;
        }
        break;
        case svExternalDoubleRef:
        case svMatrix:
        {
            ScMatrixRef pMat = GetMatrix();
            if ( !pMat )
                ;   // nothing
            else if ( !pJumpMatrix )
                bRes = pMat->IsStringOrEmpty(0, 0) && !pMat->IsEmpty(0, 0);
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                    bRes = pMat->IsStringOrEmpty( nC, nR) && !pMat->IsEmpty( nC, nR);
            }
        }
        break;
        default:
            Pop();
    }
    nGlobalError = FormulaError::NONE;
    return bRes;
}

void ScInterpreter::ScType()
{
    short nType = 0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;

            ScRefCellValue aCell(mrDoc, aAdr);
            if (GetCellErrCode(aCell) == FormulaError::NONE)
            {
                switch (aCell.getType())
                {
                    // NOTE: this is Xcl nonsense!
                    case CELLTYPE_STRING :
                    case CELLTYPE_EDIT :
                        nType = 2;
                        break;
                    case CELLTYPE_VALUE :
                    {
                        sal_uInt32 nFormat = GetCellNumberFormat(aAdr, aCell);
                        if (mrContext.NFGetType(nFormat) == SvNumFormatType::LOGICAL)
                            nType = 4;
                        else
                            nType = 1;
                    }
                    break;
                    case CELLTYPE_NONE:
                        // always 1, s. tdf#73078
                        nType = 1;
                        break;
                    case CELLTYPE_FORMULA :
                        nType = 8;
                        break;
                    default:
                        PushIllegalArgument();
                }
            }
            else
                nType = 16;
        }
        break;
        case svString:
            PopError();
            if ( nGlobalError != FormulaError::NONE )
            {
                nType = 16;
                nGlobalError = FormulaError::NONE;
            }
            else
                nType = 2;
        break;
        case svMatrix:
            PopMatrix();
            if ( nGlobalError != FormulaError::NONE )
            {
                nType = 16;
                nGlobalError = FormulaError::NONE;
            }
            else
                nType = 64;
                // we could return the type of one element if in JumpMatrix or
                // ForceArray mode, but Xcl doesn't ...
        break;
        default:
            PopError();
            if ( nGlobalError != FormulaError::NONE )
            {
                nType = 16;
                nGlobalError = FormulaError::NONE;
            }
            else
                nType = 1;
    }
    PushInt( nType );
}

namespace {

FormulaGrammar::AddressConvention resolveCellInfoAddressConvention(
    const ScCalcConfig& rConfig, const ScDocument& rDoc)
{
    FormulaGrammar::AddressConvention eConv = rConfig.meStringRefAddressSyntax;
    switch (eConv)
    {
        default:
            return rDoc.GetAddressConvention();
        case FormulaGrammar::CONV_OOO:
        case FormulaGrammar::CONV_XL_A1:
        case FormulaGrammar::CONV_XL_R1C1:
            return eConv;
    }
}

}

void ScInterpreter::ScCell()
{   // ATTRIBUTE ; [REF]
    sal_uInt8 nParamCount = GetByte();
    if( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    ScAddress aCellPos( aPos );
    if( nParamCount == 2 )
    {
        switch (GetStackType())
        {
            case svExternalSingleRef:
            case svExternalDoubleRef:
            {
                // Let's handle external reference separately...
                ScCellExternal();
                return;
            }
            case svDoubleRef:
            {
                // Exceptionally not an intersecting position but top left.
                // See ODF v1.3 part 4 OpenFormula 6.13.3 CELL
                ScRange aRange;
                PopDoubleRef( aRange);
                aCellPos = aRange.aStart;
            }
            break;
            case svSingleRef:
                PopSingleRef( aCellPos);
            break;
            default:
                PopError();
                SetError( FormulaError::NoRef);
        }
    }
    OUString aInfoType = GetString().getString();
    if (nGlobalError != FormulaError::NONE)
        PushIllegalParameter();
    else
    {
        ScRefCellValue aCell(mrDoc, aCellPos);

        ScCellKeywordTranslator::transKeyword(aInfoType, ScGlobal::GetLocale(), ocCell);
        const auto pushApiCellValue = [&](const spreadsheetengine::api::CellValue& rValue) {
            if (rValue.isError())
            {
                PushError(selibreoffice::toFormulaError(rValue.meError));
                return;
            }
            if (rValue.isText())
            {
                PushString(selibreoffice::toLibreOfficeString(rValue.maString));
                return;
            }
            PushDouble(rValue.mfNumber);
        };
        const FormulaGrammar::AddressConvention eAddressConvention
            = resolveCellInfoAddressConvention(maCalcConfig, mrDoc);
        secellexec::DirectCellInspectionAdapter aDirectCellAdapter(
            mrDoc, aPos, eAddressConvention);
        const auto aDirectEvaluation = aDirectCellAdapter.evaluateLocalInfo(
            aInfoType, aCellPos,
            selibreoffice::readHostDocumentCellValue(
                mrDoc, aCellPos, aCell, selibreoffice::HostCellStringKind::Display));
        const auto eBoundedInfoKind = aDirectEvaluation.meKind;
        if (aDirectEvaluation.mbHandled)
        {
            if (!aDirectEvaluation.maResult)
                PushError(selibreoffice::toFormulaError(aDirectEvaluation.maResult.meError));
            else
                pushApiCellValue(aDirectEvaluation.maResult.maValue);
            return;
        }

        secellexec::DirectHostCellInspectionAdapter aHostCellAdapter(mrDoc, mrContext);
        secellexec::LocalHostCellInfoRequest aHostRequest;
        aHostRequest.maCellPos = aCellPos;
        aHostRequest.mbHasString = aCell.hasString();
        aHostRequest.meConvention = eAddressConvention;
        aHostRequest.mnFormat = mrDoc.GetNumberFormat(ScRange(aCellPos));
        const auto aHostEvaluation = aHostCellAdapter.evaluateLocalInfo(aInfoType, aHostRequest);
        if (aHostEvaluation.mbHandled)
        {
            pushApiCellValue(aHostEvaluation.maValue);
            return;
        }

        switch (eBoundedInfoKind)
        {
            case secellexec::InfoKind::Unsupported:
            case secellexec::InfoKind::Column:
            case secellexec::InfoKind::Row:
            case secellexec::InfoKind::Sheet:
            case secellexec::InfoKind::Address:
            case secellexec::InfoKind::Contents:
            case secellexec::InfoKind::Type:
            case secellexec::InfoKind::Filename:
            case secellexec::InfoKind::Coord:
            case secellexec::InfoKind::Width:
            case secellexec::InfoKind::Prefix:
            case secellexec::InfoKind::Protect:
            case secellexec::InfoKind::Format:
            case secellexec::InfoKind::Color:
            case secellexec::InfoKind::Parentheses:
                PushIllegalArgument();
                break;
        }
    }
}

void ScInterpreter::ScCellExternal()
{
    sal_uInt16 nFileId;
    OUString aTabName;
    ScSingleRefData aRef;
    ScExternalRefCache::TokenRef pToken;
    ScExternalRefCache::CellFormat aFmt;
    PopExternalSingleRef(nFileId, aTabName, aRef, pToken, &aFmt);
    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;
    }

    OUString aInfoType = GetString().getString();
    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;
    }

    SCCOL nCol;
    SCROW nRow;
    SCTAB nTab;
    aRef.SetAbsTab(0); // external ref has a tab index of -1, which SingleRefToVars() don't like.
    SingleRefToVars(aRef, nCol, nRow, nTab);
    if (nGlobalError != FormulaError::NONE)
    {
        PushIllegalParameter();
        return;
    }
    aRef.SetAbsTab(-1); // revert the value.

    ScCellKeywordTranslator::transKeyword(aInfoType, ScGlobal::GetLocale(), ocCell);
    const auto pushApiCellValue = [&](const spreadsheetengine::api::CellValue& rValue) {
        if (rValue.isError())
        {
            PushError(selibreoffice::toFormulaError(rValue.meError));
            return;
        }
        if (rValue.isText())
        {
            PushString(selibreoffice::toLibreOfficeString(rValue.maString));
            return;
        }
        PushDouble(rValue.mfNumber);
    };
    const FormulaGrammar::AddressConvention eAddressConvention
        = resolveCellInfoAddressConvention(maCalcConfig, mrDoc);
    secellexec::DirectExternalCellInspectionAdapter aDirectExternalAdapter(mrDoc, aPos);
    secellexec::ExternalCellInfoRequest aExternalRequest;
    aExternalRequest.maAddress = { 0, nCol, nRow };
    aExternalRequest.mnFileId = nFileId;
    aExternalRequest.maTabName = aTabName;
    aExternalRequest.maReference = aRef;
    aExternalRequest.mxToken = pToken;
    aExternalRequest.maFormat = aFmt;
    aExternalRequest.meConvention = eAddressConvention;
    const auto aDirectEvaluation = aDirectExternalAdapter.evaluateInfo(aInfoType, aExternalRequest);
    if (aDirectEvaluation.mbHandled)
    {
        if (aDirectEvaluation.meError != FormulaError::NONE)
        {
            PushError(aDirectEvaluation.meError);
            return;
        }
        pushApiCellValue(aDirectEvaluation.maValue);
        return;
    }

    secellexec::DirectExternalHostCellInspectionAdapter aHostExternalAdapter(mrContext);
    secellexec::ExternalHostCellInfoRequest aHostExternalRequest;
    aHostExternalRequest.mnFormat = aFmt.mbIsSet ? aFmt.mnIndex : 0;
    const auto aHostEvaluation
        = aHostExternalAdapter.evaluateInfo(aInfoType, aHostExternalRequest);
    if (aHostEvaluation.mbHandled)
    {
        pushApiCellValue(aHostEvaluation.maValue);
        return;
    }

    PushIllegalParameter();
}

bool ScInterpreter::IsEven()
{
    nFuncFmtType = SvNumFormatType::LOGICAL;
    bool bRes = false;
    double fVal = 0.0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;

            ScRefCellValue aCell(mrDoc, aAdr);
            FormulaError nErr = GetCellErrCode(aCell);
            if (nErr != FormulaError::NONE)
                SetError(nErr);
            else
            {
                switch (aCell.getType())
                {
                    case CELLTYPE_VALUE :
                        fVal = GetCellValue(aAdr, aCell);
                        bRes = true;
                    break;
                    case CELLTYPE_FORMULA :
                        if (aCell.getFormula()->IsValue())
                        {
                            fVal = GetCellValue(aAdr, aCell);
                            bRes = true;
                        }
                    break;
                    default:
                        ; // nothing
                }
            }
        }
        break;
        case svDouble:
        {
            fVal = PopDouble();
            bRes = true;
        }
        break;
        case svExternalSingleRef:
        {
            ScExternalRefCache::TokenRef pToken;
            PopExternalSingleRef(pToken);
            if (nGlobalError == FormulaError::NONE && pToken->GetType() == svDouble)
            {
                fVal = pToken->GetDouble();
                bRes = true;
            }
        }
        break;
        case svExternalDoubleRef:
        case svMatrix:
        {
            ScMatrixRef pMat = GetMatrix();
            if ( !pMat )
                ;   // nothing
            else if ( !pJumpMatrix )
            {
                bRes = pMat->IsValue( 0, 0);
                if ( bRes )
                    fVal = pMat->GetDouble( 0, 0);
            }
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                {
                    bRes = pMat->IsValue( nC, nR);
                    if ( bRes )
                        fVal = pMat->GetDouble( nC, nR);
                }
                else
                    SetError( FormulaError::NoValue);
            }
        }
        break;
        default:
            ; // nothing
    }
    if ( !bRes )
        SetError( FormulaError::IllegalParameter);
    else
        bRes = ( fmod( ::rtl::math::approxFloor( fabs( fVal ) ), 2.0 ) < 0.5 );
    return bRes;
}

void ScInterpreter::ScN()
{
    FormulaError nErr = nGlobalError;
    nGlobalError = FormulaError::NONE;
    // Temporarily override the ConvertStringToValue() error for
    // GetCellValue() / GetCellValueOrZero()
    FormulaError nSErr = mnStringNoValueError;
    mnStringNoValueError = FormulaError::CellNoValue;
    double fVal = GetDouble();
    mnStringNoValueError = nSErr;
    if (nErr != FormulaError::NONE)
        nGlobalError = nErr;    // preserve previous error if any
    else if (nGlobalError == FormulaError::CellNoValue)
        nGlobalError = FormulaError::NONE;       // reset temporary detection error
    PushDouble(fVal);
}

/* ODFF:
 * Summary: Converts half-width to full-width ASCII and katakana characters.
 * Semantics: Conversion is done for half-width ASCII and katakana characters,
 * other characters are simply copied from T to the result. This is the
 * complementary function to ASC.
 * For references regarding halfwidth and fullwidth characters see
 * http://www.unicode.org/reports/tr11/
 * http://www.unicode.org/charts/charindex2.html#H
 * http://www.unicode.org/charts/charindex2.html#F
 */
/* ODFF:
 * Summary: Converts full-width to half-width ASCII and katakana characters.
 * Semantics: Conversion is done for full-width ASCII and katakana characters,
 * other characters are simply copied from T to the result. This is the
 * complementary function to JIS.
 */
bool ScInterpreter::SwitchToArrayRefList( ScMatrixRef& xResMat, SCSIZE nMatRows, double fCurrent,
        const std::function<void( SCSIZE i, double fCurrent )>& MatOpFunc, bool bDoMatOp )
{
    const ScRefListToken* p = dynamic_cast<const ScRefListToken*>(pStack[sp-1]);
    if (!p || !p->IsArrayResult())
        return false;

    if (!xResMat)
    {
        // Create and init all elements with current value.
        assert(nMatRows > 0);
        xResMat = GetNewMat( 1, nMatRows, true);
        xResMat->FillDouble( fCurrent, 0,0, 0,nMatRows-1);
    }
    else if (bDoMatOp)
    {
        // Current value and values from vector are operands
        // for each vector position.
        for (SCSIZE i=0; i < nMatRows; ++i)
        {
            MatOpFunc( i, fCurrent);
        }
    }
    return true;
}

void ScInterpreter::GetStVarParams( bool bTextAsZero, double(*VarResult)( double fVal, size_t nValCount ) )
{
    short nParamCount = GetByte();
    const SCSIZE nMatRows = GetRefListArrayMaxSize( nParamCount);

    struct ArrayRefListValue
    {
        std::vector<double> mvValues;
        KahanSum mfSum;
        ArrayRefListValue() = default;
        double get() const { return mfSum.get(); }
    };
    std::vector<ArrayRefListValue> vArrayValues;

    std::vector<double> values;
    KahanSum fSum    = 0.0;
    double fVal = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nGlobalError == FormulaError::NONE && nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
            {
                fVal = GetDouble();
                if (nGlobalError == FormulaError::NONE)
                {
                    values.push_back(fVal);
                    fSum    += fVal;
                }
            }
            break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasNumeric())
                {
                    fVal = GetCellValue(aAdr, aCell);
                    if (nGlobalError == FormulaError::NONE)
                    {
                        values.push_back(fVal);
                        fSum += fVal;
                    }
                }
                else if (bTextAsZero && aCell.hasString())
                {
                    values.push_back(0.0);
                }
            }
            break;
            case svRefList :
            {
                const ScRefListToken* p = dynamic_cast<const ScRefListToken*>(pStack[sp-1]);
                if (p && p->IsArrayResult())
                {
                    size_t nRefArrayPos = nRefInList;
                    if (vArrayValues.empty())
                    {
                        // Create and init all elements with current value.
                        assert(nMatRows > 0);
                        vArrayValues.resize(nMatRows);
                        for (ArrayRefListValue & it : vArrayValues)
                        {
                            it.mvValues = values;
                            it.mfSum = fSum;
                        }
                    }
                    else
                    {
                        // Current value and values from vector are operands
                        // for each vector position.
                        for (ArrayRefListValue & it : vArrayValues)
                        {
                            it.mvValues.insert( it.mvValues.end(), values.begin(), values.end());
                            it.mfSum += fSum;
                        }
                    }
                    ArrayRefListValue& rArrayValue = vArrayValues[nRefArrayPos];
                    FormulaError nErr = FormulaError::NONE;
                    PopDoubleRef( aRange, nParamCount, nRefInList);
                    ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags, bTextAsZero );
                    if (aValIter.GetFirst(fVal, nErr))
                    {
                        do
                        {
                            rArrayValue.mvValues.push_back(fVal);
                            rArrayValue.mfSum += fVal;
                        }
                        while ((nErr == FormulaError::NONE) && aValIter.GetNext(fVal, nErr));
                    }
                    if ( nErr != FormulaError::NONE )
                    {
                        rArrayValue.mfSum = CreateDoubleError( nErr);
                    }
                    // Reset.
                    std::vector<double>().swap(values);
                    fSum = 0.0;
                    break;
                }
            }
            [[fallthrough]];
            case svDoubleRef :
            {
                FormulaError nErr = FormulaError::NONE;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags, bTextAsZero );
                if (aValIter.GetFirst(fVal, nErr))
                {
                    do
                    {
                        values.push_back(fVal);
                        fSum += fVal;
                    }
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(fVal, nErr));
                }
                if ( nErr != FormulaError::NONE )
                {
                    SetError(nErr);
                }
            }
            break;
            case svExternalSingleRef :
            case svExternalDoubleRef :
            case svMatrix :
            {
                ScMatrixRef pMat = GetMatrix();
                if (pMat)
                {
                    const bool bIgnoreErrVal = bool(mnSubTotalFlags & SubtotalFlags::IgnoreErrVal);
                    SCSIZE nC, nR;
                    pMat->GetDimensions(nC, nR);
                    for (SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++)
                    {
                        for (SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++)
                        {
                            if (!pMat->IsStringOrEmpty(nMatCol,nMatRow))
                            {
                                fVal= pMat->GetDouble(nMatCol,nMatRow);
                                if (nGlobalError == FormulaError::NONE)
                                {
                                    values.push_back(fVal);
                                    fSum += fVal;
                                }
                                else if (bIgnoreErrVal)
                                    nGlobalError = FormulaError::NONE;
                            }
                            else if ( bTextAsZero )
                            {
                                values.push_back(0.0);
                            }
                        }
                    }
                }
            }
            break;
            case svString :
            {
                Pop();
                if ( bTextAsZero )
                {
                    values.push_back(0.0);
                }
                else
                    SetError(FormulaError::IllegalParameter);
            }
            break;
            default :
                PopError();
                SetError(FormulaError::IllegalParameter);
        }
    }

    if (!vArrayValues.empty())
    {
        // Include value of last non-references-array type and calculate final result.
        if (!values.empty())
        {
            for (auto & it : vArrayValues)
            {
                it.mvValues.insert( it.mvValues.end(), values.begin(), values.end());
                it.mfSum += fSum;
            }
        }
        ScMatrixRef xResMat = GetNewMat( 1, nMatRows, true);
        for (SCSIZE r=0; r < nMatRows; ++r)
        {
            ::std::vector<double>::size_type n = vArrayValues[r].mvValues.size();
            if (!n)
                xResMat->PutError( FormulaError::DivisionByZero, 0, r);
            else
            {
                ArrayRefListValue& rArrayValue = vArrayValues[r];
                double vSum = 0.0;
                const double vMean = rArrayValue.get() / n;
                for (::std::vector<double>::size_type i = 0; i < n; i++)
                    vSum += ::rtl::math::approxSub( rArrayValue.mvValues[i], vMean) *
                        ::rtl::math::approxSub( rArrayValue.mvValues[i], vMean);
                xResMat->PutDouble( VarResult( vSum, n), 0, r);
            }
        }
        PushMatrix( xResMat);
    }
    else
    {
        ::std::vector<double>::size_type n = values.size();
        if (!n)
            SetError( FormulaError::DivisionByZero);
        double vSum = 0.0;
        if (nGlobalError == FormulaError::NONE)
        {
            const double vMean = fSum.get() / n;
            for (::std::vector<double>::size_type i = 0; i < n; i++)
                vSum += ::rtl::math::approxSub( values[i], vMean) * ::rtl::math::approxSub( values[i], vMean);
        }
        PushDouble( VarResult( vSum, n));
    }
}

void ScInterpreter::IterateParametersIf( ScIterFuncIf eFunc )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;

    SCCOL nCol3 = 0;
    SCROW nRow3 = 0;
    SCTAB nTab3 = 0;

    ScMatrixRef pSumExtraMatrix;
    bool bSumExtraRange = (nParamCount == 3);
    if (bSumExtraRange)
    {
        // Save only the upperleft cell in case of cell range.  The geometry
        // of the 3rd parameter is taken from the 1st parameter.

        switch ( GetStackType() )
        {
            case svDoubleRef :
                {
                    SCCOL nColJunk = 0;
                    SCROW nRowJunk = 0;
                    SCTAB nTabJunk = 0;
                    PopDoubleRef( nCol3, nRow3, nTab3, nColJunk, nRowJunk, nTabJunk );
                    if ( nTabJunk != nTab3 )
                    {
                        PushError( FormulaError::IllegalParameter);
                        return;
                    }
                }
                break;
            case svSingleRef :
                PopSingleRef( nCol3, nRow3, nTab3 );
                break;
            case svMatrix:
                pSumExtraMatrix = PopMatrix();
                // nCol3, nRow3, nTab3 remain 0
                break;
            case svExternalSingleRef:
                {
                    pSumExtraMatrix = GetNewMat(1,1);
                    ScExternalRefCache::TokenRef pToken;
                    PopExternalSingleRef(pToken);
                    if (nGlobalError != FormulaError::NONE)
                    {
                        PushError( nGlobalError);
                        return;
                    }

                    if (pToken->GetType() == svDouble)
                        pSumExtraMatrix->PutDouble(pToken->GetDouble(), 0, 0);
                    else
                        pSumExtraMatrix->PutString(pToken->GetString(), 0, 0);
                }
                break;
            case svExternalDoubleRef:
                PopExternalDoubleRef(pSumExtraMatrix);
                break;
            default:
                PushError( FormulaError::IllegalParameter);
                return;
        }
    }

    svl::SharedString aString;
    double fVal = 0.0;
    bool bIsString = true;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
            {
                ScAddress aAdr;
                if ( !PopDoubleRefOrSingleRef( aAdr ) )
                {
                    PushError( nGlobalError);
                    return;
                }

                ScRefCellValue aCell(mrDoc, aAdr);
                switch (aCell.getType())
                {
                    case CELLTYPE_VALUE :
                        fVal = GetCellValue(aAdr, aCell);
                        bIsString = false;
                        break;
                    case CELLTYPE_FORMULA :
                        if (aCell.getFormula()->IsValue())
                        {
                            fVal = GetCellValue(aAdr, aCell);
                            bIsString = false;
                        }
                        else
                            GetCellString(aString, aCell);
                        break;
                    case CELLTYPE_STRING :
                    case CELLTYPE_EDIT :
                        GetCellString(aString, aCell);
                        break;
                    default:
                        fVal = 0.0;
                        bIsString = false;
                }
            }
            break;
        case svString:
            aString = GetString();
            break;
        case svMatrix :
        case svExternalDoubleRef:
            {
                ScMatValType nType = GetDoubleOrStringFromMatrix( fVal, aString);
                bIsString = ScMatrix::IsRealStringType( nType);
            }
            break;
        case svExternalSingleRef:
            {
                ScExternalRefCache::TokenRef pToken;
                PopExternalSingleRef(pToken);
                if (nGlobalError == FormulaError::NONE)
                {
                    if (pToken->GetType() == svDouble)
                    {
                        fVal = pToken->GetDouble();
                        bIsString = false;
                    }
                    else
                        aString = pToken->GetString();
                }
            }
            break;
        default:
            {
                fVal = GetDouble();
                bIsString = false;
            }
    }

    KahanSum fSum = 0.0;
    double fRes = 0.0;
    double fCount = 0.0;
    short nParam = 1;
    const SCSIZE nMatRows = GetRefListArrayMaxSize( nParam);
    // There's either one RefList and nothing else, or none.
    ScMatrixRef xResMat = (nMatRows ? GetNewMat( 1, nMatRows, /*bEmpty*/true ) : nullptr);
    SCSIZE nRefListArrayPos = 0;
    size_t nRefInList = 0;
    while (nParam-- > 0)
    {
        SCCOL nCol1 = 0;
        SCROW nRow1 = 0;
        SCTAB nTab1 = 0;
        SCCOL nCol2 = 0;
        SCROW nRow2 = 0;
        SCTAB nTab2 = 0;
        ScMatrixRef pQueryMatrix;
        switch ( GetStackType() )
        {
            case svRefList :
                if (bSumExtraRange)
                {
                    /* TODO: this could resolve if all refs are of the same size */
                    SetError( FormulaError::IllegalParameter);
                }
                else
                {
                    nRefListArrayPos = nRefInList;
                    ScRange aRange;
                    PopDoubleRef( aRange, nParam, nRefInList);
                    aRange.GetVars( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                }
                break;
            case svDoubleRef :
                PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                break;
            case svSingleRef :
                PopSingleRef( nCol1, nRow1, nTab1 );
                nCol2 = nCol1;
                nRow2 = nRow1;
                nTab2 = nTab1;
                break;
            case svMatrix:
            case svExternalSingleRef:
            case svExternalDoubleRef:
                {
                    pQueryMatrix = GetMatrix();
                    if (!pQueryMatrix)
                    {
                        PushError( FormulaError::IllegalParameter);
                        return;
                    }
                    nCol1 = 0;
                    nRow1 = 0;
                    nTab1 = 0;
                    SCSIZE nC, nR;
                    pQueryMatrix->GetDimensions( nC, nR);
                    nCol2 = static_cast<SCCOL>(nC - 1);
                    nRow2 = static_cast<SCROW>(nR - 1);
                    nTab2 = 0;
                }
                break;
            default:
                SetError( FormulaError::IllegalParameter);
        }
        if ( nTab1 != nTab2 )
        {
            SetError( FormulaError::IllegalParameter);
        }

        if (bSumExtraRange)
        {
            // Take the range geometry of the 1st parameter and apply it to
            // the 3rd. If parts of the resulting range would point outside
            // the sheet, don't complain but silently ignore and simply cut
            // them away, this is what Xcl does :-/

            // For the cut-away part we also don't need to determine the
            // criteria match, so shrink the source range accordingly,
            // instead of the result range.
            SCCOL nColDelta = nCol2 - nCol1;
            SCROW nRowDelta = nRow2 - nRow1;
            SCCOL nMaxCol;
            SCROW nMaxRow;
            if (pSumExtraMatrix)
            {
                SCSIZE nC, nR;
                pSumExtraMatrix->GetDimensions( nC, nR);
                nMaxCol = static_cast<SCCOL>(nC - 1);
                nMaxRow = static_cast<SCROW>(nR - 1);
            }
            else
            {
                nMaxCol = mrDoc.MaxCol();
                nMaxRow = mrDoc.MaxRow();
            }
            if (nCol3 + nColDelta > nMaxCol)
            {
                SCCOL nNewDelta = nMaxCol - nCol3;
                nCol2 = nCol1 + nNewDelta;
            }

            if (nRow3 + nRowDelta > nMaxRow)
            {
                SCROW nNewDelta = nMaxRow - nRow3;
                nRow2 = nRow1 + nNewDelta;
            }
        }
        else
        {
            nCol3 = nCol1;
            nRow3 = nRow1;
            nTab3 = nTab1;
        }

        if (nGlobalError == FormulaError::NONE)
        {
            ScQueryParam rParam;
            rParam.nRow1       = nRow1;
            rParam.nRow2       = nRow2;

            ScQueryEntry& rEntry = rParam.GetEntry(0);
            ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
            rEntry.bDoQuery = true;
            if (!bIsString)
            {
                rItem.meType = ScQueryEntry::ByValue;
                rItem.mfVal = fVal;
                rEntry.eOp = SC_EQUAL;
            }
            else
            {
                rParam.FillInExcelSyntax(mrDoc.GetSharedStringPool(), aString.getString(), 0, &mrContext);
                if (rItem.meType == ScQueryEntry::ByString)
                    rParam.eSearchType = DetectSearchType(rItem.maString.getString(), mrDoc);
            }
            ScAddress aAdr;
            aAdr.SetTab( nTab3 );
            rParam.nCol1  = nCol1;
            rParam.nCol2  = nCol2;
            rEntry.nField = nCol1;
            SCCOL nColDiff = nCol3 - nCol1;
            SCROW nRowDiff = nRow3 - nRow1;
            if (pQueryMatrix)
            {
                // Never case-sensitive.
                sc::CompareOptions aOptions( mrDoc, rEntry, rParam.eSearchType);
                ScMatrixRef pResultMatrix = QueryMat( pQueryMatrix, aOptions);
                if (nGlobalError != FormulaError::NONE || !pResultMatrix)
                {
                    PushIllegalParameter();
                    return;
                }

                if (pSumExtraMatrix)
                {
                    for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
                    {
                        for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
                        {
                            if (pResultMatrix->IsValue( nCol, nRow) &&
                                    pResultMatrix->GetDouble( nCol, nRow))
                            {
                                SCSIZE nC = nCol + nColDiff;
                                SCSIZE nR = nRow + nRowDiff;
                                if (pSumExtraMatrix->IsValue( nC, nR))
                                {
                                    fVal = pSumExtraMatrix->GetDouble( nC, nR);
                                    ++fCount;
                                    fSum += fVal;
                                }
                            }
                        }
                    }
                }
                else if (!bSumExtraRange)
                {
                    for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
                    {
                        for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
                        {
                            if (pResultMatrix->IsValue( nCol, nRow) &&
                                    pResultMatrix->GetDouble( nCol, nRow))
                            {
                                if (pQueryMatrix->IsValue( nCol, nRow))
                                {
                                    fVal = pQueryMatrix->GetDouble( nCol, nRow);
                                    ++fCount;
                                    fSum += fVal;
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
                    {
                        for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
                        {
                            if (pResultMatrix->GetDouble( nCol, nRow))
                            {
                                aAdr.SetCol( nCol + nColDiff);
                                aAdr.SetRow( nRow + nRowDiff);
                                ScRefCellValue aCell(mrDoc, aAdr);
                                if (aCell.hasNumeric())
                                {
                                    fVal = GetCellValue(aAdr, aCell);
                                    ++fCount;
                                    fSum += fVal;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                ScQueryCellIteratorDirect aCellIter(mrDoc, mrContext, nTab1, rParam, false, false);
                // Increment Entry.nField in iterator when switching to next column.
                aCellIter.SetAdvanceQueryParamEntryField( true );
                if ( aCellIter.GetFirst() )
                {
                    if (pSumExtraMatrix)
                    {
                        do
                        {
                            SCSIZE nC = aCellIter.GetCol() + nColDiff;
                            SCSIZE nR = aCellIter.GetRow() + nRowDiff;
                            if (pSumExtraMatrix->IsValue( nC, nR))
                            {
                                fVal = pSumExtraMatrix->GetDouble( nC, nR);
                                ++fCount;
                                fSum += fVal;
                            }
                        } while ( aCellIter.GetNext() );
                    }
                    else
                    {
                        do
                        {
                            aAdr.SetCol( aCellIter.GetCol() + nColDiff);
                            aAdr.SetRow( aCellIter.GetRow() + nRowDiff);
                            ScRefCellValue aCell(mrDoc, aAdr);
                            if (aCell.hasNumeric())
                            {
                                fVal = GetCellValue(aAdr, aCell);
                                ++fCount;
                                fSum += fVal;
                            }
                        } while ( aCellIter.GetNext() );
                    }
                }
            }
        }
        else
        {
            PushError( FormulaError::IllegalParameter);
            return;
        }

        switch( eFunc )
        {
            case ifSUMIF:     fRes = fSum.get(); break;
            case ifAVERAGEIF: fRes = div( fSum.get(), fCount ); break;
        }
        if (xResMat)
        {
            if (nGlobalError == FormulaError::NONE)
                xResMat->PutDouble( fRes, 0, nRefListArrayPos);
            else
            {
                xResMat->PutError( nGlobalError, 0, nRefListArrayPos);
                nGlobalError = FormulaError::NONE;
            }
            fRes = fCount = 0.0;
            fSum = 0;
        }
    }
    if (xResMat)
        PushMatrix( xResMat);
    else
        PushDouble( fRes);
}

void ScInterpreter::IterateParametersIfs( double(*ResultFunc)( const sc::ParamIfsResult& rRes ) )
{
    sal_uInt8 nParamCount = GetByte();
    sal_uInt8 nQueryCount = nParamCount / 2;

    std::vector<sal_uInt8>& vConditions = mrContext.maConditions;
    // vConditions is cached, although it is clear'ed after every cell is interpreted,
    // if the SUMIFS/COUNTIFS are part of a matrix formula, then that is not enough because
    // with a single InterpretTail() call it results in evaluation of all the cells in the
    // matrix formula.
    vConditions.clear();

    // Range-reduce optimization
    SCCOL nStartColDiff = 0;
    SCCOL nEndColDiff = 0;
    SCROW nStartRowDiff = 0;
    SCROW nEndRowDiff = 0;
    bool bRangeReduce = false;
    ScRange aMainRange;

    bool bHasDoubleRefCriteriaRanges = true;
    // Do not attempt main-range reduce if any of the criteria-ranges are not double-refs.
    // For COUNTIFS queries it's possible to range-reduce too, if the query is not supposed
    // to match empty cells (will be checked and undone later if needed), so simply treat
    // the first criteria range as the main range for purposes of detecting if this can be done.
    for (sal_uInt16 nParamIdx = 2; nParamIdx < nParamCount; nParamIdx += 2 )
    {
        const formula::FormulaToken* pCriteriaRangeToken = pStack[ sp-nParamIdx ];
        if (pCriteriaRangeToken->GetType() != svDoubleRef )
        {
            bHasDoubleRefCriteriaRanges = false;
            break;
        }
    }

    // Probe the main range token, and try if we can shrink the range without altering results.
    const formula::FormulaToken* pMainRangeToken = pStack[ sp-nParamCount ];
    if (pMainRangeToken->GetType() == svDoubleRef && bHasDoubleRefCriteriaRanges)
    {
        const ScComplexRefData* pRefData = pMainRangeToken->GetDoubleRef();
        if (!pRefData->IsDeleted())
        {
            DoubleRefToRange( *pRefData, aMainRange);
            if (aMainRange.aStart.Tab() == aMainRange.aEnd.Tab())
            {
                // Shrink the range to actual data content.
                ScRange aSubRange = aMainRange;
                mrDoc.GetDataAreaSubrange(aSubRange);
                nStartColDiff = aSubRange.aStart.Col() - aMainRange.aStart.Col();
                nStartRowDiff = aSubRange.aStart.Row() - aMainRange.aStart.Row();
                nEndColDiff = aSubRange.aEnd.Col() - aMainRange.aEnd.Col();
                nEndRowDiff = aSubRange.aEnd.Row() - aMainRange.aEnd.Row();
                bRangeReduce = nStartColDiff || nStartRowDiff || nEndColDiff || nEndRowDiff;
            }
        }
    }

    double fVal = 0.0;
    SCCOL nDimensionCols = 0;
    SCROW nDimensionRows = 0;
    const SCSIZE nRefArrayRows = GetRefListArrayMaxSize( nParamCount);
    std::vector<std::vector<sal_uInt8>> vRefArrayConditions;

    while (nParamCount > 1 && nGlobalError == FormulaError::NONE)
    {
        // take criteria
        svl::SharedString aString;
        fVal = 0.0;
        bool bIsString = true;
        switch ( GetStackType() )
        {
            case svDoubleRef :
            case svSingleRef :
                {
                    ScAddress aAdr;
                    if ( !PopDoubleRefOrSingleRef( aAdr ) )
                    {
                        PushError( nGlobalError);
                        return;
                    }

                    ScRefCellValue aCell(mrDoc, aAdr);
                    switch (aCell.getType())
                    {
                        case CELLTYPE_VALUE :
                            fVal = GetCellValue(aAdr, aCell);
                            bIsString = false;
                            break;
                        case CELLTYPE_FORMULA :
                            if (aCell.getFormula()->IsValue())
                            {
                                fVal = GetCellValue(aAdr, aCell);
                                bIsString = false;
                            }
                            else
                                GetCellString(aString, aCell);
                            break;
                        case CELLTYPE_STRING :
                        case CELLTYPE_EDIT :
                            GetCellString(aString, aCell);
                            break;
                        default:
                            fVal = 0.0;
                            bIsString = false;
                    }
                }
                break;
            case svString:
                aString = GetString();
                break;
            case svMatrix :
            case svExternalDoubleRef:
                {
                    ScMatValType nType = GetDoubleOrStringFromMatrix( fVal, aString);
                    bIsString = ScMatrix::IsRealStringType( nType);
                }
                break;
            case svExternalSingleRef:
                {
                    ScExternalRefCache::TokenRef pToken;
                    PopExternalSingleRef(pToken);
                    if (nGlobalError == FormulaError::NONE)
                    {
                        if (pToken->GetType() == svDouble)
                        {
                            fVal = pToken->GetDouble();
                            bIsString = false;
                        }
                        else
                            aString = pToken->GetString();
                    }
                }
                break;
            default:
                {
                    fVal = GetDouble();
                    bIsString = false;
                }
        }

        if (nGlobalError != FormulaError::NONE)
        {
            PushError( nGlobalError);
            return;   // and bail out, no need to evaluate other arguments
        }

        // take range
        short nParam = nParamCount;
        size_t nRefInList = 0;
        size_t nRefArrayPos = std::numeric_limits<size_t>::max();
        SCCOL nCol1 = 0;
        SCROW nRow1 = 0;
        SCTAB nTab1 = 0;
        SCCOL nCol2 = 0;
        SCROW nRow2 = 0;
        SCTAB nTab2 = 0;
        ScMatrixRef pQueryMatrix;
        while (nParam-- == nParamCount)
        {
            const ScComplexRefData* refData = nullptr;
            switch ( GetStackType() )
            {
                case svRefList :
                    {
                        const ScRefListToken* p = dynamic_cast<const ScRefListToken*>(pStack[sp-1]);
                        if (p && p->IsArrayResult())
                        {
                            if (nRefInList == 0)
                            {
                                if (vRefArrayConditions.empty())
                                    vRefArrayConditions.resize( nRefArrayRows);
                                if (!vConditions.empty())
                                {
                                    // Similar to other reference list array
                                    // handling, add/op the current value to
                                    // all array positions.
                                    for (auto & rVec : vRefArrayConditions)
                                    {
                                        if (rVec.empty())
                                            rVec = vConditions;
                                        else
                                        {
                                            assert(rVec.size() == vConditions.size());  // see dimensions below
                                            for (size_t i=0, n = rVec.size(); i < n; ++i)
                                            {
                                                rVec[i] += vConditions[i];
                                            }
                                        }
                                    }
                                    // Reset condition results.
                                    std::for_each( vConditions.begin(), vConditions.end(),
                                            [](sal_uInt8 & r){ r = 0.0; } );
                                }
                            }
                            nRefArrayPos = nRefInList;
                        }
                        refData = GetStackDoubleRef(nRefInList);
                        ScRange aRange;
                        PopDoubleRef( aRange, nParam, nRefInList);
                        aRange.GetVars( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                    }
                break;
                case svDoubleRef :
                    refData = GetStackDoubleRef();
                    PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                break;
                case svSingleRef :
                    PopSingleRef( nCol1, nRow1, nTab1 );
                    nCol2 = nCol1;
                    nRow2 = nRow1;
                    nTab2 = nTab1;
                break;
                case svMatrix:
                case svExternalSingleRef:
                case svExternalDoubleRef:
                    {
                        pQueryMatrix = GetMatrix();
                        if (!pQueryMatrix)
                        {
                            PushError( FormulaError::IllegalParameter);
                            return;
                        }
                        nCol1 = 0;
                        nRow1 = 0;
                        nTab1 = 0;
                        SCSIZE nC, nR;
                        pQueryMatrix->GetDimensions( nC, nR);
                        nCol2 = static_cast<SCCOL>(nC - 1);
                        nRow2 = static_cast<SCROW>(nR - 1);
                        nTab2 = 0;
                    }
                break;
                default:
                    PushError( FormulaError::IllegalParameter);
                    return;
            }
            if ( nTab1 != nTab2 )
            {
                PushError( FormulaError::IllegalArgument);
                return;
            }

            ScQueryParam rParam;
            ScQueryEntry& rEntry = rParam.GetEntry(0);
            ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
            rEntry.bDoQuery = true;
            if (!bIsString)
            {
                rItem.meType = ScQueryEntry::ByValue;
                rItem.mfVal = fVal;
                rEntry.eOp = SC_EQUAL;
            }
            else
            {
                rParam.FillInExcelSyntax(mrDoc.GetSharedStringPool(), aString.getString(), 0, &mrContext);
                if (rItem.meType == ScQueryEntry::ByString)
                    rParam.eSearchType = DetectSearchType(rItem.maString.getString(), mrDoc);
            }

            // Undo bRangeReduce if asked to match empty cells for COUNTIFS (which should be rare).
            assert(rEntry.GetQueryItems().size() == 1);
            const bool isCountIfs = (nParamCount % 2) == 0;
            if(isCountIfs && (rEntry.IsQueryByEmpty() || rItem.mbMatchEmpty) && bRangeReduce)
            {
                bRangeReduce = false;
                // All criteria ranges are svDoubleRef's, so only vConditions needs adjusting.
                assert(vRefArrayConditions.empty());
                if(!vConditions.empty())
                {
                    std::vector<sal_uInt8> newConditions;
                    SCCOL newDimensionCols = nCol2 - nCol1 + 1;
                    SCROW newDimensionRows = nRow2 - nRow1 + 1;
                    newConditions.reserve( newDimensionCols * newDimensionRows );
                    SCCOL col = nCol1;
                    for(; col < nCol1 + nStartColDiff; ++col)
                        newConditions.insert( newConditions.end(), newDimensionRows, 0 );
                    for(; col <= nCol2 - nStartColDiff; ++col)
                    {
                        newConditions.insert( newConditions.end(), nStartRowDiff, 0 );
                        SCCOL oldCol = col - ( nCol1 + nStartColDiff );
                        size_t nIndex = oldCol * nDimensionRows;
                        if (nIndex < vConditions.size())
                        {
                            auto it = vConditions.begin() + nIndex;
                            newConditions.insert( newConditions.end(), it, it + nDimensionRows );
                        }
                        else
                            newConditions.insert( newConditions.end(), nDimensionRows, 0 );
                        newConditions.insert( newConditions.end(), -nEndRowDiff, 0 );
                    }
                    for(; col <= nCol2; ++col)
                        newConditions.insert( newConditions.end(), newDimensionRows, 0 );
                    assert( newConditions.size() == o3tl::make_unsigned( newDimensionCols * newDimensionRows ));
                    vConditions = std::move( newConditions );
                    nDimensionCols = newDimensionCols;
                    nDimensionRows = newDimensionRows;
                }
            }

            if (bRangeReduce)
            {
                // All reference ranges must be of the same size as the main range.
                if( aMainRange.aEnd.Col() - aMainRange.aStart.Col() != nCol2 - nCol1
                    || aMainRange.aEnd.Row() - aMainRange.aStart.Row() != nRow2 - nRow1)
                {
                    PushError ( FormulaError::IllegalArgument);
                    return;
                }
                nCol1 += nStartColDiff;
                nRow1 += nStartRowDiff;

                nCol2 += nEndColDiff;
                nRow2 += nEndRowDiff;
            }

            // All reference ranges must be of same dimension and size.
            if (!nDimensionCols)
                nDimensionCols = nCol2 - nCol1 + 1;
            if (!nDimensionRows)
                nDimensionRows = nRow2 - nRow1 + 1;
            if ((nDimensionCols != (nCol2 - nCol1 + 1)) || (nDimensionRows != (nRow2 - nRow1 + 1)))
            {
                PushError ( FormulaError::IllegalArgument);
                return;
            }

            // recalculate matrix values
            if (nGlobalError != FormulaError::NONE)
            {
                PushError( nGlobalError);
                return;
            }

            // initialize temporary result matrix
            if (vConditions.empty())
                vConditions.resize( nDimensionCols * nDimensionRows, 0);

            rParam.nRow1  = nRow1;
            rParam.nRow2  = nRow2;
            rParam.nCol1  = nCol1;
            rParam.nCol2  = nCol2;
            rEntry.nField = nCol1;
            SCCOL nColDiff = -nCol1;
            SCROW nRowDiff = -nRow1;
            if (pQueryMatrix)
            {
                // Never case-sensitive.
                sc::CompareOptions aOptions(mrDoc, rEntry, rParam.eSearchType);
                ScMatrixRef pResultMatrix = QueryMat( pQueryMatrix, aOptions);
                if (nGlobalError != FormulaError::NONE || !pResultMatrix)
                {
                    PushError( FormulaError::IllegalParameter);
                    return;
                }

                // result matrix is filled with boolean values.
                std::vector<double> aResValues;
                pResultMatrix->GetDoubleArray(aResValues);
                if (vConditions.size() != aResValues.size())
                {
                    PushError( FormulaError::IllegalParameter);
                    return;
                }

                std::vector<double>::const_iterator itThisRes = aResValues.begin();
                for (auto& rCondition : vConditions)
                {
                    rCondition += *itThisRes;
                    ++itThisRes;
                }
            }
            else
            {
                if( ScQueryCellIteratorSortedCache::CanBeUsed( mrDoc, rParam, nTab1, pMyFormulaCell,
                        refData, mrContext ))
                {
                    ScQueryCellIteratorSortedCache aCellIter(mrDoc, mrContext, nTab1, rParam, false, false);
                    // Increment Entry.nField in iterator when switching to next column.
                    aCellIter.SetAdvanceQueryParamEntryField( true );
                    if ( aCellIter.GetFirst() )
                    {
                        do
                        {
                            size_t nC = aCellIter.GetCol() + nColDiff;
                            size_t nR = aCellIter.GetRow() + nRowDiff;
                            ++vConditions[nC * nDimensionRows + nR];
                        } while ( aCellIter.GetNext() );
                    }
                }
                else
                {
                    ScQueryCellIteratorDirect aCellIter(mrDoc, mrContext, nTab1, rParam, false, false);
                    // Increment Entry.nField in iterator when switching to next column.
                    aCellIter.SetAdvanceQueryParamEntryField( true );
                    if ( aCellIter.GetFirst() )
                    {
                        do
                        {
                            size_t nC = aCellIter.GetCol() + nColDiff;
                            size_t nR = aCellIter.GetRow() + nRowDiff;
                            ++vConditions[nC * nDimensionRows + nR];
                        } while ( aCellIter.GetNext() );
                    }
                }
            }
            if (nRefArrayPos != std::numeric_limits<size_t>::max())
            {
                // Apply condition result to reference list array result position.
                std::vector<sal_uInt8>& rVec = vRefArrayConditions[nRefArrayPos];
                if (rVec.empty())
                    rVec = vConditions;
                else
                {
                    assert(rVec.size() == vConditions.size());  // see dimensions above
                    for (size_t i=0, n = rVec.size(); i < n; ++i)
                    {
                        rVec[i] += vConditions[i];
                    }
                }
                // Reset conditions vector.
                // When leaving an svRefList this has to be emptied not set to
                // 0.0 because it's checked when entering an svRefList.
                if (nRefInList == 0)
                    std::vector<sal_uInt8>().swap( vConditions);
                else
                    std::for_each( vConditions.begin(), vConditions.end(), [](sal_uInt8 & r){ r = 0; } );
            }
        }
        nParamCount -= 2;
    }

    if (!vRefArrayConditions.empty() && !vConditions.empty())
    {
        // Add/op the last current value to all array positions.
        for (auto & rVec : vRefArrayConditions)
        {
            if (rVec.empty())
                rVec = vConditions;
            else
            {
                assert(rVec.size() == vConditions.size());  // see dimensions above
                for (size_t i=0, n = rVec.size(); i < n; ++i)
                {
                    rVec[i] += vConditions[i];
                }
            }
        }
    }

    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;   // bail out
    }

    sc::ParamIfsResult aRes;
    ScMatrixRef xResMat;

    // main range - only for AVERAGEIFS, SUMIFS, MINIFS and MAXIFS
    if (nParamCount == 1)
    {
        short nParam = nParamCount;
        size_t nRefInList = 0;
        size_t nRefArrayPos = std::numeric_limits<size_t>::max();
        bool bRefArrayMain = false;
        while (nParam-- == nParamCount)
        {
            SCCOL nMainCol1 = 0;
            SCROW nMainRow1 = 0;
            SCTAB nMainTab1 = 0;
            SCCOL nMainCol2 = 0;
            SCROW nMainRow2 = 0;
            SCTAB nMainTab2 = 0;
            ScMatrixRef pMainMatrix;
            switch ( GetStackType() )
            {
                case svRefList :
                    {
                        const ScRefListToken* p = dynamic_cast<const ScRefListToken*>(pStack[sp-1]);
                        if (p && p->IsArrayResult())
                        {
                            if (vRefArrayConditions.empty())
                            {
                                // Replicate conditions if there wasn't a
                                // reference list array for criteria
                                // evaluation.
                                vRefArrayConditions.resize( nRefArrayRows);
                                for (auto & rVec : vRefArrayConditions)
                                {
                                    rVec = vConditions;
                                }
                            }

                            bRefArrayMain = true;
                            nRefArrayPos = nRefInList;
                        }
                        ScRange aRange;
                        PopDoubleRef( aRange, nParam, nRefInList);
                        aRange.GetVars( nMainCol1, nMainRow1, nMainTab1, nMainCol2, nMainRow2, nMainTab2);
                    }
                break;
                case svDoubleRef :
                    PopDoubleRef( nMainCol1, nMainRow1, nMainTab1, nMainCol2, nMainRow2, nMainTab2 );
                break;
                case svSingleRef :
                    PopSingleRef( nMainCol1, nMainRow1, nMainTab1 );
                    nMainCol2 = nMainCol1;
                    nMainRow2 = nMainRow1;
                    nMainTab2 = nMainTab1;
                break;
                case svMatrix:
                case svExternalSingleRef:
                case svExternalDoubleRef:
                    {
                        pMainMatrix = GetMatrix();
                        if (!pMainMatrix)
                        {
                            PushError( FormulaError::IllegalParameter);
                            return;
                        }
                        nMainCol1 = 0;
                        nMainRow1 = 0;
                        nMainTab1 = 0;
                        SCSIZE nC, nR;
                        pMainMatrix->GetDimensions( nC, nR);
                        nMainCol2 = static_cast<SCCOL>(nC - 1);
                        nMainRow2 = static_cast<SCROW>(nR - 1);
                        nMainTab2 = 0;
                    }
                break;
                // Treat a scalar value as 1x1 matrix.
                case svDouble:
                    pMainMatrix = GetNewMat(1,1);
                    nMainCol1 = nMainCol2 = 0;
                    nMainRow1 = nMainRow2 = 0;
                    nMainTab1 = nMainTab2 = 0;
                    pMainMatrix->PutDouble( GetDouble(), 0, 0);
                break;
                case svString:
                    pMainMatrix = GetNewMat(1,1);
                    nMainCol1 = nMainCol2 = 0;
                    nMainRow1 = nMainRow2 = 0;
                    nMainTab1 = nMainTab2 = 0;
                    pMainMatrix->PutString( GetString(), 0, 0);
                break;
                default:
                    PopError();
                    PushError( FormulaError::IllegalParameter);
                    return;
            }
            if ( nMainTab1 != nMainTab2 )
            {
                PushError( FormulaError::IllegalArgument);
                return;
            }

            if (bRangeReduce)
            {
                nMainCol1 += nStartColDiff;
                nMainRow1 += nStartRowDiff;

                nMainCol2 += nEndColDiff;
                nMainRow2 += nEndRowDiff;
            }

            // All reference ranges must be of same dimension and size.
            if ((nDimensionCols != (nMainCol2 - nMainCol1 + 1)) || (nDimensionRows != (nMainRow2 - nMainRow1 + 1)))
            {
                PushError ( FormulaError::IllegalArgument);
                return;
            }

            if (nGlobalError != FormulaError::NONE)
            {
                PushError( nGlobalError);
                return;   // bail out
            }

            // end-result calculation

            // This gets weird... if conditions were calculated using a
            // reference list array but the main calculation range is not a
            // reference list array, then the conditions of the array are
            // applied to the main range each in turn to form the array result.

            size_t nRefArrayMainPos = (bRefArrayMain ? nRefArrayPos :
                    (vRefArrayConditions.empty() ? std::numeric_limits<size_t>::max() : 0));
            const bool bAppliedArray = (!bRefArrayMain && nRefArrayMainPos == 0);

            if (nRefArrayMainPos == 0)
                xResMat = GetNewMat( 1, nRefArrayRows, /*bEmpty*/true );

            if (pMainMatrix)
            {
                std::vector<double> aMainValues;
                pMainMatrix->GetDoubleArray(aMainValues, false); // Map empty values to NaN's.

                do
                {
                    if (nRefArrayMainPos < vRefArrayConditions.size())
                        vConditions = vRefArrayConditions[nRefArrayMainPos];

                    if (vConditions.size() != aMainValues.size())
                    {
                        PushError( FormulaError::IllegalArgument);
                        return;
                    }

                    std::vector<sal_uInt8>::const_iterator itRes = vConditions.begin(), itResEnd = vConditions.end();
                    std::vector<double>::const_iterator itMain = aMainValues.begin();
                    for (; itRes != itResEnd; ++itRes, ++itMain)
                    {
                        if (*itRes != nQueryCount)
                            continue;

                        fVal = *itMain;
                        if (GetDoubleErrorValue(fVal) == FormulaError::ElementNaN)
                            continue;

                        ++aRes.mfCount;
                        aRes.mfSum += fVal;
                        if ( aRes.mfMin > fVal )
                            aRes.mfMin = fVal;
                        if ( aRes.mfMax < fVal )
                            aRes.mfMax = fVal;
                    }
                    if (nRefArrayMainPos != std::numeric_limits<size_t>::max())
                    {
                        xResMat->PutDouble( ResultFunc( aRes), 0, nRefArrayMainPos);
                        aRes = sc::ParamIfsResult();
                    }
                }
                while (bAppliedArray && ++nRefArrayMainPos < nRefArrayRows);
            }
            else
            {
                ScAddress aAdr;
                aAdr.SetTab( nMainTab1 );
                do
                {
                    if (nRefArrayMainPos < vRefArrayConditions.size())
                        vConditions = vRefArrayConditions[nRefArrayMainPos];

                    SAL_WARN_IF(nDimensionCols && nDimensionRows && vConditions.empty(), "sc",  "ScInterpreter::IterateParametersIfs vConditions is empty");
                    if (!vConditions.empty())
                    {
                        std::vector<sal_uInt8>::const_iterator itRes = vConditions.begin();
                        for (SCCOL nCol = 0; nCol < nDimensionCols; ++nCol)
                        {
                            for (SCROW nRow = 0; nRow < nDimensionRows; ++nRow, ++itRes)
                            {
                                if (*itRes == nQueryCount)
                                {
                                    aAdr.SetCol( nCol + nMainCol1);
                                    aAdr.SetRow( nRow + nMainRow1);
                                    ScRefCellValue aCell(mrDoc, aAdr);
                                    if (aCell.hasNumeric())
                                    {
                                        fVal = GetCellValue(aAdr, aCell);
                                        ++aRes.mfCount;
                                        aRes.mfSum += fVal;
                                        if ( aRes.mfMin > fVal )
                                            aRes.mfMin = fVal;
                                        if ( aRes.mfMax < fVal )
                                            aRes.mfMax = fVal;
                                    }
                                }
                            }
                        }
                    }
                    if (nRefArrayMainPos != std::numeric_limits<size_t>::max())
                    {
                        xResMat->PutDouble( ResultFunc( aRes), 0, nRefArrayMainPos);
                        aRes = sc::ParamIfsResult();
                    }
                }
                while (bAppliedArray && ++nRefArrayMainPos < nRefArrayRows);
            }
        }
    }
    else
    {
        // COUNTIFS only.
        if (vRefArrayConditions.empty())
        {
            // The code below is this but optimized for most elements not matching.
            // for (auto const & rCond : vConditions)
            //     if (rCond == nQueryCount)
            //         ++aRes.mfCount;
            static_assert(sizeof(vConditions[0]) == 1);
            const sal_uInt8* pos = vConditions.data();
            const sal_uInt8* end = pos + vConditions.size();
            for(;;)
            {
                pos = static_cast< const sal_uInt8* >( memchr( pos, nQueryCount, end - pos ));
                if( pos == nullptr )
                    break;
                ++aRes.mfCount;
                ++pos;
            }
        }
        else
        {
            xResMat = GetNewMat( 1, nRefArrayRows, /*bEmpty*/true );
            for (size_t i=0, n = vRefArrayConditions.size(); i < n; ++i)
            {
                double fCount = 0.0;
                for (auto const & rCond : vRefArrayConditions[i])
                {
                    if (rCond == nQueryCount)
                        ++fCount;
                }
                xResMat->PutDouble( fCount, 0, i);
            }
        }
    }

    if (xResMat)
        PushMatrix( xResMat);
    else
        PushDouble( ResultFunc( aRes));
}

void ScInterpreter::ScLookup()
{
    const std::optional<OUString> oQuarantinedFormula
        = lclGetQuarantinedHardRoutedFormula(pMyFormulaCell, mrDoc, mrContext);
    if (oQuarantinedFormula)
    {
        SAL_WARN("sc.core",
            "hard-routed LOOKUP reached ScInterpreter for " << *oQuarantinedFormula);
        OSL_FAIL("hard-routed LOOKUP reached ScInterpreter");
    }

    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;

    selookupexec::LegacyLookupRequest aRequest;
    if (nParamCount == 3)
    {
        const auto aResultInput = PopLookupExecutionInput(true, true);
        if (!aResultInput)
        {
            PushIllegalParameter();
            return;
        }
        aRequest.moResultInput = aResultInput.maValue;
    }

    const auto aDataInput = PopLookupExecutionInput(true, false);
    if (!aDataInput)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maDataInput = aDataInput.maValue;

    const auto aLookupValue = PopLookupExecutionValue(false);
    if (!aLookupValue)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maLookupValue = aLookupValue.maValue;
    if (aRequest.maLookupValue.isText())
    {
        aRequest.meSearchType = toApiSearchType(
            DetectSearchType(selibreoffice::toLibreOfficeString(aRequest.maLookupValue.maString),
                mrDoc));
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

    PushLookupExecutionResult(aResult.maValue, false);
}

void ScInterpreter::CalculateLookup(bool bHLookup)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 3, 4))
        return;

    bool bSorted = true;
    if (nParamCount == 4)
        bSorted = GetBool();

    const double fIndex = ::rtl::math::approxFloor(GetDouble()) - 1.0;
    if (fIndex < 0.0)
    {
        PushIllegalArgument();
        return;
    }

    const auto aTableInput = PopLookupExecutionInput(false, false);
    if (!aTableInput)
    {
        PushIllegalParameter();
        return;
    }

    const auto aLookupValue = PopLookupExecutionValue(false);
    if (!aLookupValue)
    {
        PushIllegalParameter();
        return;
    }

    const spreadsheetengine::api::MatrixDimensions aDimensions {
        aTableInput.maValue.mnColumns, aTableInput.maValue.mnRows
    };
    const spreadsheetengine::api::MatrixSize nResultIndex
        = static_cast<spreadsheetengine::api::MatrixSize>(fIndex);
    if ((bHLookup && nResultIndex >= aDimensions.mnRows)
        || (!bHLookup && nResultIndex >= aDimensions.mnColumns))
    {
        PushIllegalArgument();
        return;
    }

    selookupexec::TabularLookupRequest aRequest;
    aRequest.maLookupValue = aLookupValue.maValue;
    aRequest.maTableInput = aTableInput.maValue;
    aRequest.meSearchOrientation = bHLookup ? selookup::VectorOrientation::Row
                                            : selookup::VectorOrientation::Column;
    aRequest.mnResultIndex = nResultIndex;
    aRequest.mbApproximate = bSorted;
    if (aRequest.maLookupValue.isText())
    {
        aRequest.meSearchType = toApiSearchType(
            DetectSearchType(selibreoffice::toLibreOfficeString(aRequest.maLookupValue.maString),
                mrDoc));
    }

    const auto aResult = selookupexec::resolveTabularLookupResult(mrDoc, mrContext, aRequest);
    if (!aResult)
    {
        if (aResult.meError == spreadsheetengine::api::Error::NotAvailable)
            PushNA();
        else
            PushError(selibreoffice::toFormulaError(aResult.meError));
        return;
    }

    PushLookupExecutionResult(aResult.maValue, false);
}

bool ScInterpreter::FillEntry(ScQueryEntry& rEntry)
{
    ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
    switch ( GetStackType() )
    {
        case svDouble:
        {
            rItem.meType = ScQueryEntry::ByValue;
            rItem.mfVal = GetDouble();
        }
        break;
        case svString:
        {
            rItem.meType = ScQueryEntry::ByString;
            rItem.maString = GetString();
        }
        break;
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
            {
                PushInt(0);
                return false;
            }
            ScRefCellValue aCell(mrDoc, aAdr);
            if (aCell.hasNumeric())
            {
                rItem.meType = ScQueryEntry::ByValue;
                rItem.mfVal = GetCellValue(aAdr, aCell);
            }
            else
            {
                GetCellString(rItem.maString, aCell);
                rItem.meType = ScQueryEntry::ByString;
            }
        }
        break;
        case svExternalDoubleRef:
        case svExternalSingleRef:
        case svMatrix:
        {
            svl::SharedString aStr;
            const ScMatValType nType = GetDoubleOrStringFromMatrix(rItem.mfVal, aStr);
            rItem.maString = std::move(aStr);
            rItem.meType = ScMatrix::IsNonValueType(nType) ?
                ScQueryEntry::ByString : ScQueryEntry::ByValue;
        }
        break;
        default:
        {
            PushIllegalParameter();
            return false;
        }
    } // switch ( GetStackType() )
    return true;
}

void ScInterpreter::ScXLookup()
{
    const std::optional<OUString> oQuarantinedFormula
        = lclGetQuarantinedHardRoutedFormula(pMyFormulaCell, mrDoc, mrContext);
    if (oQuarantinedFormula)
    {
        SAL_WARN("sc.core",
            "hard-routed XLOOKUP reached ScInterpreter for " << *oQuarantinedFormula);
        OSL_FAIL("hard-routed XLOOKUP reached ScInterpreter");
    }

    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 6 ) )
        return;

    selookupexec::XLookupExecutionRequest aRequest;
    aRequest.mbAllowPatternMatch = true;

    if ( nParamCount == 6 )
    {
        const auto aSearchMode = selookup::normalizeSearchMode(GetInt16());
        if (!aSearchMode)
        {
            PushIllegalParameter();
            return;
        }
        aRequest.meSearchMode = aSearchMode.maValue;
    }

    if ( nParamCount >= 5 )
    {
        const auto aMatchMode = selookup::normalizeExtendedMatchMode(GetInt16());
        if (!aMatchMode)
        {
            PushIllegalParameter();
            return;
        }
        aRequest.meMatchMode = aMatchMode.maValue;
    }

    // Optional 4th argument to set return values if not found (default is #N/A)
    formula::FormulaConstTokenRef xNotFound;
    FormulaError nFirstMatchError = FormulaError::NONE;
    if ( nParamCount >= 4 && GetStackType() != svEmptyCell )
    {
        xNotFound = PopToken();
        nFirstMatchError = xNotFound->GetError();
        nGlobalError = FormulaError::NONE; // propagate only for match or active result path
    }

    const auto aResultInput = PopLookupExecutionInput(false, false);
    if (!aResultInput)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maResultInput = aResultInput.maValue;

    const auto aSearchInput = PopLookupExecutionInput(false, true);
    if (!aSearchInput)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maSearchInput = aSearchInput.maValue;

    const auto aLookupValue = PopLookupExecutionValue(true, true);
    if (!aLookupValue)
    {
        PushIllegalParameter();
        return;
    }
    aRequest.maLookupValue = aLookupValue.maValue;
    if (aRequest.maLookupValue.isText())
    {
        aRequest.meSearchType = toApiSearchType(
            DetectSearchType(selibreoffice::toLibreOfficeString(aRequest.maLookupValue.maString),
                mrDoc));
    }

    const auto aResult = selookupexec::resolveXLookupResult(mrDoc, mrContext, aRequest);
    if (!aResult)
    {
        if (aResult.meError == spreadsheetengine::api::Error::NotAvailable)
        {
            if (xNotFound && (xNotFound->GetType() != svMissing))
            {
                nGlobalError = nFirstMatchError;
                PushTokenRef(xNotFound);
            }
            else
            {
                PushNA();
            }
        }
        else if (aResult.meError == spreadsheetengine::api::Error::NoValue)
        {
            PushNoValue();
        }
        else
        {
            PushError(selibreoffice::toFormulaError(aResult.meError));
        }
        return;
    }

    PushLookupExecutionResult(aResult.maValue, true);
}

void ScInterpreter::ScFilter()
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, 3))
        return;

    // Optional 3th argument to set the value to return if all values
    // in the included array are empty (filter returns nothing)
    formula::FormulaConstTokenRef xNotFound;
    if (nParamCount == 3 && GetStackType() != svEmptyCell)
        xNotFound = PopToken();

    SCCOL nCondResultColEnd = 0;
    SCROW nCondResultRowEnd = 0;
    ScMatrixRef pCondResultMatrix = nullptr;
    std::vector<double> aResValues;
    size_t nMatch = 0;
    // take 2nd argument criteria bool array
    switch ( GetStackType() )
    {
        case svMatrix :
        case svExternalDoubleRef:
        case svExternalSingleRef:
        case svDoubleRef:
        case svSingleRef:
            {
                pCondResultMatrix = GetMatrix();
                if (!pCondResultMatrix)
                {
                    PushError(FormulaError::IllegalParameter);
                    return;
                }
                SCSIZE nC, nR;
                pCondResultMatrix->GetDimensions(nC, nR);
                nCondResultColEnd = static_cast<SCCOL>(nC - 1);
                nCondResultRowEnd = static_cast<SCROW>(nR - 1);

                // only 1 dimension of filtering allowed (also in excel)
                if (nCondResultColEnd > 0 && nCondResultRowEnd > 0)
                {
                    PushError(FormulaError::NoValue);
                    return;
                }

                // result matrix is filled with boolean values.
                pCondResultMatrix->GetDoubleArray(aResValues);

                FormulaError nError = FormulaError::NONE;
                auto matchNum = [&nMatch, &nError](double i) {
                    nError = GetDoubleErrorValue(i);
                    if (nError != FormulaError::NONE)
                    {
                        return true;
                    }
                    else
                    {
                        if (i > 0)
                            nMatch++;
                        return false;
                    }
                };

                if (auto it = std::find_if(aResValues.begin(), aResValues.end(), matchNum); it != aResValues.end())
                {
                    PushError(nError);
                    return;
                }
            }
            break;

        default:
            {
                PushIllegalParameter();
                return;
            }
    }

    // bail out, no need to evaluate other arguments
    if (nGlobalError != FormulaError::NONE)
    {
        PushError(nGlobalError);
        return;
    }

    SCCOL nQueryCol1 = 0;
    SCROW nQueryRow1 = 0;
    SCCOL nQueryCol2 = 0;
    SCROW nQueryRow2 = 0;
    ScMatrixRef pQueryMatrix = nullptr;
    // take 1st argument range
    switch ( GetStackType() )
    {
        case svSingleRef:
        case svDoubleRef:
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
            {
                pQueryMatrix = GetMatrix();
                if (!pQueryMatrix)
                {
                    PushError( FormulaError::IllegalParameter);
                    return;
                }
                SCSIZE nC, nR;
                pQueryMatrix->GetDimensions( nC, nR);
                nQueryCol2 = static_cast<SCCOL>(nC - 1);
                nQueryRow2 = static_cast<SCROW>(nR - 1);
            }
        break;
        default:
            PushError( FormulaError::IllegalParameter);
            return;
    }

    // bail out, no need to set a matrix if we have no result
    if (!nMatch)
    {
        if (xNotFound && (xNotFound->GetType() != svMissing))
            PushTokenRef(xNotFound);
        else
            PushError(FormulaError::NestedArray);
        return;
    }

    SCSIZE nResPos = 0;
    ScMatrixRef pResMat = nullptr;
    if (nQueryCol2 == nCondResultColEnd && nCondResultColEnd > 0)
    {
        pResMat = GetNewMat(nMatch, nQueryRow2 + 1 , /*bEmpty*/true);
        for (SCROW iR = nQueryRow1; iR <= nQueryRow2; iR++)
        {
            for (size_t iC = 0; iC < aResValues.size(); iC++)
            {
                if (aResValues[iC] > 0)
                {
                    if (pQueryMatrix->IsEmptyCell(iC, iR))
                        pResMat->PutEmptyTrans(nResPos++);
                    else if (pQueryMatrix->IsStringOrEmpty(iC, iR))
                        pResMat->PutStringTrans(pQueryMatrix->GetString(iC, iR), nResPos++);
                    else
                        pResMat->PutDoubleTrans(pQueryMatrix->GetDouble(iC, iR), nResPos++);
                }
            }
        }
    }
    else if (nQueryRow2 == nCondResultRowEnd && nCondResultRowEnd > 0)
    {
        pResMat = GetNewMat(nQueryCol2 + 1, nMatch, /*bEmpty*/true);
        for (SCCOL iC = nQueryCol1; iC <= nQueryCol2; iC++)
        {
            for (size_t iR = 0; iR < aResValues.size(); iR++)
            {
                if (aResValues[iR] > 0)
                {
                    if (pQueryMatrix->IsEmptyCell(iC, iR))
                        pResMat->PutEmpty(nResPos++);
                    else if (pQueryMatrix->IsStringOrEmpty(iC, iR))
                        pResMat->PutString(pQueryMatrix->GetString(iC, iR), nResPos++);
                    else
                        pResMat->PutDouble(pQueryMatrix->GetDouble(iC, iR), nResPos++);
                }
            }
        }
    }
    else
    {
        PushError(FormulaError::IllegalParameter);
        return;
    }

    if (pResMat)
        PushMatrix(pResMat);
    else
        PushError(FormulaError::NestedArray);
}

void ScInterpreter::ScSort()
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 1, 4))
        return;

    // Create sort data
    ScSortParam aSortData;

    // 4th argument optional
    aSortData.bByRow = true; // default: By_Col = false --> bByRow = true
    if (nParamCount == 4)
        aSortData.bByRow = !GetBool();

    // 3rd argument optional
    std::vector<double> aSortOrderValues{ 1.0 }; // default: 1 = asc, -1 = desc
    if (nParamCount >= 3)
    {
        bool bMissing = IsMissing();
        ScMatrixRef pSortOrder = GetMatrix();
        if (!bMissing)
        {
            aSortOrderValues.clear();
            pSortOrder->GetDoubleArray(aSortOrderValues);
            for (const double& sortOrder : aSortOrderValues)
            {
                if (sortOrder != 1.0 && sortOrder != -1.0)
                {
                    PushIllegalParameter();
                    return;
                }
            }
        }
    }

    // 2nd argument optional
    std::vector<double> aSortIndexValues{ 0.0 }; // default: first column or row
    if (nParamCount >= 2)
    {
        bool bMissing = IsMissing();
        ScMatrixRef pSortIndex = GetMatrix();
        if (!bMissing)
        {
            aSortIndexValues.clear();
            pSortIndex->GetDoubleArray(aSortIndexValues);
            for (double& sortIndex : aSortIndexValues)
            {
                if (sortIndex < 1)
                {
                    PushIllegalParameter();
                    return;
                }
                else
                    sortIndex--;
            }
        }
    }

    if (aSortIndexValues.size() != aSortOrderValues.size() && aSortOrderValues.size() > 1)
    {
        PushIllegalParameter();
        return;
    }

    // 1st argument is vector to be sorted
    SCSIZE nsC = 0, nsR = 0;
    ScMatrixRef pMatSrc = nullptr;
    switch ( GetStackType() )
    {
        case svSingleRef:
            PopSingleRef(aSortData.nCol1, aSortData.nRow1, aSortData.nSourceTab);
            aSortData.nCol2   = aSortData.nCol1;
            aSortData.nRow2   = aSortData.nRow1;
            nsC = aSortData.nCol2 - aSortData.nCol1 + 1;
            nsR = aSortData.nRow2 - aSortData.nRow1 + 1;
        break;
        case svDoubleRef:
        {
            SCTAB nTab2 = 0;
            PopDoubleRef(aSortData.nCol1, aSortData.nRow1, aSortData.nSourceTab, aSortData.nCol2, aSortData.nRow2, nTab2);
            if (aSortData.nSourceTab != nTab2)
            {
                PushIllegalParameter();
                return;
            }
            nsC = aSortData.nCol2 - aSortData.nCol1 + 1;
            nsR = aSortData.nRow2 - aSortData.nRow1 + 1;
        }
        break;
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            pMatSrc = GetMatrix();
            if (!pMatSrc)
            {
                PushIllegalParameter();
                return;
            }
            pMatSrc->GetDimensions(nsC, nsR);
            if (nsC == 0 || nsR == 0)
            {
                PushIllegalArgument();
                return;
            }
            aSortData.nCol2 = nsC - 1; // aSortData.nCol1 = 0
            aSortData.nRow2 = nsR - 1; // aSortData.nRow1 = 0
        }
        break;

        default:
            PushIllegalParameter();
            return;
    }

    aSortData.maKeyState.resize(aSortIndexValues.size());
    for (size_t i = 0; i < aSortIndexValues.size(); i++)
    {
        SCSIZE fIndex = static_cast<SCSIZE>(double_to_int32(aSortIndexValues[i]));
        if ((aSortData.bByRow && fIndex + 1 > nsC) || (!aSortData.bByRow && fIndex + 1 > nsR))
        {
            PushIllegalParameter();
            return;
        }

        aSortData.maKeyState[i].bDoSort = true;
        aSortData.maKeyState[i].nField = (aSortData.bByRow ? aSortData.nCol1 : aSortData.nRow1) + fIndex;

        if (aSortIndexValues.size() == aSortOrderValues.size())
            aSortData.maKeyState[i].bAscending = (aSortOrderValues[i] == 1.0);
        else
            aSortData.maKeyState[i].bAscending = (aSortOrderValues[0] == 1.0);
    }

    if ((aSortData.bByRow && nsR == 1) || (!aSortData.bByRow && nsC == 1))
    {
        // No need to sort
        if (pMatSrc)
            PushMatrix(pMatSrc);
        else
            PushDoubleRef(aSortData.nCol1, aSortData.nRow1, aSortData.nSourceTab,
                aSortData.nCol2, aSortData.nRow2, aSortData.nSourceTab);
    }
    else
    {
        // sorting...
        std::vector<SCCOLROW> aOrderIndices = GetSortOrder(aSortData, pMatSrc);
        // create sorted matrix
        ScMatrixRef pResMat = CreateSortedMatrix(aSortData, pMatSrc,
            ScRange(aSortData.nCol1, aSortData.nRow1, aSortData.nSourceTab,
                aSortData.nCol2, aSortData.nRow2, aSortData.nSourceTab),
            aOrderIndices, nsC, nsR);

        if (pResMat)
            PushMatrix(pResMat);
        else
            PushIllegalParameter();
    }
}

void ScInterpreter::ScSortBy()
{
    sal_uInt8 nParamCount = GetByte();

    if (nParamCount < 2/*|| (nParamCount % 2 != 1)*/)
    {
        PushError(FormulaError::ParameterExpected);
        return;
    }

    sal_uInt8 nSortCount = nParamCount / 2;

    ScSortParam aSortData;
    aSortData.maKeyState.resize(nSortCount);
    bool bNoNeedToSort = false;

    // 127th, ..., 3rd and 2nd argument: sort by range/array and sort orders pair
    sal_uInt8 nSortBy = nSortCount;
    ScMatrixRef pFullMatSortBy = nullptr;
    while (nSortBy-- > 0 && nGlobalError == FormulaError::NONE)
    {
        // 3rd argument sort_order optional: default ascending
        if (nParamCount >= 3 && (nParamCount % 2 == 1))
        {
            sal_Int8 nSortOrder = static_cast<sal_Int8>(GetInt32WithDefault(1));
            if (nSortOrder != 1 && nSortOrder != -1)
            {
                PushIllegalParameter();
                return;
            }
            aSortData.maKeyState[nSortBy].bAscending = (nSortOrder == 1);
            nParamCount--;
        }

        // 2nd argument: take sort by ranges
        ScMatrixRef pMatSortBy = nullptr;
        SCSIZE nbyC = 0, nbyR = 0;
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
                    pFullMatSortBy->GetDimensions(nbyC, nbyR);
                }
                else
                {
                    pMatSortBy = GetMatrix();
                    if (!pMatSortBy)
                    {
                        PushIllegalParameter();
                        return;
                    }
                    pMatSortBy->GetDimensions(nbyC, nbyR);
                }

                // last->first (backward) sortby array
                if (nSortBy == nSortCount - 1)
                {
                    if (nbyC == 1 && nbyR > 1)
                        aSortData.bByRow = true;
                    else if (nbyR == 1 && nbyC > 1)
                        aSortData.bByRow = false;
                    else if (nbyC == 1 && nbyR == 1)
                        bNoNeedToSort = true;
                    else
                    {
                        PushIllegalParameter();
                        return;
                    }

                    if (nSortCount > 1)
                    {
                        pFullMatSortBy = GetNewMat(aSortData.bByRow ? (nbyC * nSortCount) : nbyC,
                            aSortData.bByRow ? nbyR : (nbyR * nSortCount), /*bEmpty*/true);
                    }
                }
            }
            break;

            default:
                PushIllegalParameter();
                return;
        }

        // ..., penultimate sortby arrays
        if (nSortCount > 1 && nSortBy <= nSortCount - 1)
        {
            SCSIZE nCheckCol = 0, nCheckRow = 0;
            pFullMatSortBy->GetDimensions(nCheckCol, nCheckRow);
            if ((aSortData.bByRow && nbyR == nCheckRow && nbyC == 1) ||
                (!aSortData.bByRow && nbyC == nCheckCol && nbyR == 1))
            {
                for (SCSIZE ci = 0; ci < nbyC; ci++)//col
                {
                    for (SCSIZE rj = 0; rj < nbyR; rj++)//row
                    {
                        if (pMatSortBy->IsEmptyCell(ci, rj))
                        {
                            if (aSortData.bByRow)
                                pFullMatSortBy->PutEmpty(ci + nSortBy, rj);
                            else
                                pFullMatSortBy->PutEmpty(ci, rj + nSortBy);
                        }
                        else if (pMatSortBy->IsStringOrEmpty(ci, rj))
                        {
                            if (aSortData.bByRow)
                                pFullMatSortBy->PutString(pMatSortBy->GetString(ci, rj), ci + nSortBy, rj);
                            else
                                pFullMatSortBy->PutString(pMatSortBy->GetString(ci, rj), ci, rj + nSortBy);
                        }
                        else
                        {
                            if (aSortData.bByRow)
                                pFullMatSortBy->PutDouble(pMatSortBy->GetDouble(ci, rj), ci + nSortBy, rj);
                            else
                                pFullMatSortBy->PutDouble(pMatSortBy->GetDouble(ci, rj), ci, rj + nSortBy);
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

        nParamCount--;
    }

    // 1st argument is the range/array to be sorted
    SCSIZE nsC = 0, nsR = 0;
    SCCOL nSortCol1 = 0, nSortCol2 = 0;
    SCROW nSortRow1 = 0, nSortRow2 = 0;
    SCTAB nSortTab1 = 0, nSortTab2 = 0;
    ScMatrixRef pMatSrc = nullptr;
    switch ( GetStackType() )
    {
        case svSingleRef:
            PopSingleRef(nSortCol1, nSortRow1, nSortTab1);
            nSortCol2 = nSortCol1;
            nSortRow2 = nSortRow1;
            nsC = nSortCol2 - nSortCol1 + 1;
            nsR = nSortRow2 - nSortRow1 + 1;
        break;
        case svDoubleRef:
        {
            PopDoubleRef(nSortCol1, nSortRow1, nSortTab1, nSortCol2, nSortRow2, nSortTab2);
            if (nSortTab1 != nSortTab2)
            {
                PushIllegalParameter();
                return;
            }
            nsC = nSortCol2 - nSortCol1 + 1;
            nsR = nSortRow2 - nSortRow1 + 1;
        }
        break;
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            pMatSrc = GetMatrix();
            if (!pMatSrc)
            {
                PushIllegalParameter();
                return;
            }
            pMatSrc->GetDimensions(nsC, nsR);
            if (nsC == 0 || nsR == 0)
            {
                PushIllegalArgument();
                return;
            }
            nSortCol2 = nsC - 1; // nSortCol1 = 0
            nSortRow2 = nsR - 1; // nSortRow1 = 0
        }
        break;

        default:
            PushIllegalParameter();
            return;
    }

    SCSIZE nCheckMatrixCol = 0, nCheckMatrixRow = 0;
    pFullMatSortBy->GetDimensions(nCheckMatrixCol, nCheckMatrixRow);
    if (nGlobalError != FormulaError::NONE)
    {
        PushError(nGlobalError);
        return;
    }
    else if ((aSortData.bByRow && nsR != nCheckMatrixRow) ||
        (!aSortData.bByRow && nsC != nCheckMatrixCol))
    {
        PushIllegalParameter();
        return;
    }
    else
    {
        aSortData.nCol2 = nCheckMatrixCol - 1;
        aSortData.nRow2 = nCheckMatrixRow - 1;
    }

    if (bNoNeedToSort)
    {
        // no need to sort
        if (pMatSrc)
            PushMatrix(pMatSrc);
        else
            PushDoubleRef(nSortCol1, nSortRow1, nSortTab1, nSortCol2, nSortRow2, nSortTab2);
    }
    else
    {
        // sorting...
        std::vector<SCCOLROW> aOrderIndices = GetSortOrder(aSortData, pFullMatSortBy);
        // create sorted matrix
        ScMatrixRef pResMat = CreateSortedMatrix(aSortData, pMatSrc,
            ScRange(nSortCol1, nSortRow1, nSortTab1, nSortCol2, nSortRow2, nSortTab2),
            aOrderIndices, nsC, nsR);

        if (pResMat)
            PushMatrix(pResMat);
        else
            PushIllegalParameter();
    }
}

static void lcl_FillCell(const ScMatrixRef& pMatSource, const ScMatrixRef& pMatDest, SCSIZE nsC, SCSIZE nsR, SCSIZE ndC, SCSIZE ndR)
{
    if (pMatSource->IsEmptyCell(nsC, nsR))
    {
        pMatDest->PutEmpty(ndC, ndR);
    }
    else if (!pMatSource->IsStringOrEmpty(nsC, nsR))
    {
        pMatDest->PutDouble(pMatSource->GetDouble(nsC, nsR), ndC, ndR);
    }
    else
    {
        pMatDest->PutString(pMatSource->GetString(nsC, nsR), ndC, ndR);
    }
}

void ScInterpreter::ScTakeOrDrop(bool bTake)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 1, 3))
        return;

    // 3rd argument optional - columns
    std::optional<sal_Int32> nArgCols;
    if (nParamCount == 3)
    {
        if (!IsMissing())
            nArgCols = GetInt32();
        else
            Pop();
    }

    // 2nd argument optional - rows
    std::optional<sal_Int32> nArgRows;
    if (nParamCount >= 2)
    {
        if (!IsMissing())
            nArgRows = GetInt32();
        else
            Pop();
    }

    // 1st argument: take unique search range
    ScMatrixRef pMatSource = nullptr;
    SCSIZE nsC = 0, nsR = 0;
    switch (GetStackType())
    {
        case svSingleRef:
        case svDoubleRef:
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            pMatSource = GetMatrix();
            if (!pMatSource)
            {
                PushIllegalParameter();
                return;
            }

            pMatSource->GetDimensions(nsC, nsR);
        }
        break;

        default:
            PushIllegalParameter();
            return;
    }

    if (nGlobalError != FormulaError::NONE || nsC < 1 || nsR < 1)
    {
        PushIllegalArgument();
        return;
    }

    const auto aSlice = searray::planTakeDropSlice(
        { static_cast<sal_Int32>(nsC), static_cast<sal_Int32>(nsR) }, bTake, nArgRows, nArgCols);
    if (!aSlice)
    {
        if (aSlice.meError == spreadsheetengine::api::Error::NotAvailable)
            PushNA();
        else
            PushIllegalArgument();
        return;
    }

    const SCSIZE nColumns = aSlice.maValue.maDimensions.mnColumns;
    const SCSIZE nRows = aSlice.maValue.maDimensions.mnRows;
    ScMatrixRef pResMat = GetNewMat(nColumns, nRows, /*bEmpty*/true);
    if (!pResMat)
    {
        PushIllegalArgument();
        return;
    }

    for (SCSIZE col = 0; col < nColumns; ++col)
    {
        for (SCSIZE row = 0; row < nRows; ++row)
        {
            lcl_FillCell(pMatSource, pResMat, aSlice.maValue.maStart.mnColumn + col,
                         aSlice.maValue.maStart.mnRow + row, col, row);
        }
    }

    PushMatrix(pResMat);
}

void ScInterpreter::ScChooseColsOrRows(bool bCols)
{
    sal_uInt8 nParamCount = GetByte();

    if (!MustHaveParamCountMin( nParamCount, 2))
        return;

    //reverse order of parameter stack to read them from first to last
    ReverseStack(nParamCount);

    // 1st argument: array
    ScMatrixRef pMatSource = nullptr;
    SCSIZE nsC = 0, nsR = 0;
    switch (GetStackType())
    {
        case svSingleRef:
        case svDoubleRef:
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            pMatSource = GetMatrix();
            if (!pMatSource)
            {
                PushIllegalParameter();
                return;
            }

            pMatSource->GetDimensions(nsC, nsR);
        }
        break;

        default:
            PushIllegalParameter();
            return;
    }

    if (nGlobalError != FormulaError::NONE || nsC < 1 || nsR < 1)
    {
        PushIllegalArgument();
        return;
    }

    std::vector<sal_Int32> aParamsVector;
    while (nGlobalError == FormulaError::NONE && nParamCount-- > 1)
    {
        if (IsMissing())
        {
            PushIllegalParameter();
            return;
        }

        ScMatrixRef pRefMatrix = GetMatrix();
        if (!pRefMatrix)
        {
            PushIllegalParameter();
            return;
        }

        SCSIZE nC = 0, nR = 0;
        pRefMatrix->GetDimensions(nC, nR);
        for (SCSIZE col = 0; col < nC; col++)
        {
            for (SCSIZE row = 0; row < nR; row++)
            {
                if (!pRefMatrix->IsStringOrEmpty(col, row))
                {
                    const auto aSelection = searray::normalizeSelectionIndex(
                        double_to_int32(pRefMatrix->GetDouble(col, row)),
                        bCols ? static_cast<sal_Int32>(nsC) : static_cast<sal_Int32>(nsR));
                    if (!aSelection)
                    {
                        PushIllegalParameter();
                        return;
                    }
                    aParamsVector.push_back(aSelection.maValue);
                }
                else
                {
                    PushIllegalParameter();
                    return;
                }
            }
        }
    }

    const auto aResultDimensions = searray::planChooseResultDimensions(
        { static_cast<sal_Int32>(nsC), static_cast<sal_Int32>(nsR) },
        static_cast<sal_Int32>(aParamsVector.size()),
        bCols ? searray::Axis::Columns : searray::Axis::Rows);
    if (!aResultDimensions)
    {
        PushIllegalArgument();
        return;
    }

    SCSIZE nColumns = aResultDimensions.maValue.mnColumns;
    SCSIZE nRows = aResultDimensions.maValue.mnRows;
    ScMatrixRef pResMat = GetNewMat(nColumns, nRows, /*bEmpty*/true);
    if (!pResMat)
    {
        PushIllegalArgument();
        return;
    }

    for (SCSIZE col = 0; col < nColumns; ++col)
    {
        for(SCSIZE row = 0; row < nRows; ++row)
        {
            if (bCols)
                lcl_FillCell(pMatSource, pResMat, aParamsVector[col], row, col, row);
            else
                lcl_FillCell(pMatSource, pResMat, col, aParamsVector[row], col, row);
        }
    }

    PushMatrix(pResMat);
}

void ScInterpreter::ScExpand()
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, 4))
        return;

    // 4rd argument optional - pad_with
    std::optional<bool> bDouble;
    double fNumber(0.0);
    svl::SharedString aString;
    if (nParamCount == 4)
        bDouble = GetDoubleOrString(fNumber, aString);

    // 3rd argument optional - columns
    std::optional<sal_Int32> nArgCols;
    if (nParamCount >= 3)
    {
        if (!IsMissing())
            nArgCols = GetInt32();
        else
            Pop();
    }

    // 2nd argument - rows
    std::optional<sal_Int32> nArgRows;
    if (nParamCount >= 2)
    {
        if (!IsMissing())
            nArgRows = GetInt32();
        else
            Pop();
    }

    // 1st argument: take unique search range
    ScMatrixRef pMatSource = nullptr;
    SCSIZE nsC = 0, nsR = 0;
    switch (GetStackType())
    {
        case svSingleRef:
        case svDoubleRef:
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            pMatSource = GetMatrix();
            if (!pMatSource)
            {
                PushIllegalParameter();
                return;
            }

            pMatSource->GetDimensions(nsC, nsR);
        }
        break;

        default:
            PushIllegalParameter();
            return;
    }

    if (nGlobalError != FormulaError::NONE || nsC < 1 || nsR < 1)
    {
        PushIllegalArgument();
        return;
    }

    const auto aExpandDimensions = searray::planExpandDimensions(
        { static_cast<sal_Int32>(nsC), static_cast<sal_Int32>(nsR) }, nArgRows, nArgCols);
    if (!aExpandDimensions)
    {
        PushIllegalArgument();
        return;
    }
    const SCSIZE nColumns = aExpandDimensions.maValue.mnColumns;
    const SCSIZE nRows = aExpandDimensions.maValue.mnRows;

    ScMatrixRef pResMat = GetNewMat(nColumns, nRows, /*bEmpty*/true);
    if (!pResMat)
    {
        PushIllegalArgument();
        return;
    }

    for (SCSIZE col = 0; col < nColumns; ++col)
    {
        for (SCSIZE row = 0; row < nRows; ++row)
        {
            if (col < nsC && row < nsR)
                lcl_FillCell(pMatSource, pResMat, col, row, col, row);
            else
            {
                if (bDouble.has_value())
                {
                    if (bDouble.value())
                        pResMat->PutDouble(fNumber, col, row);
                    else
                        pResMat->PutString(aString, col, row);
                }
                else
                    pResMat->PutError(FormulaError::NotAvailable, col, row);
            }
        }
    }

    PushMatrix(pResMat);
}

void ScInterpreter::ScHorizontalOrVerticalStack(bool bHorizontal)
{
    sal_uInt8 nParamCount = GetByte();

    if (!MustHaveParamCountMin( nParamCount, 1))
        return;

    //reverse order of parameter stack to read them from first to last
    ReverseStack(nParamCount);

    spreadsheetengine::api::MatrixDimensions aStackDimensions;
    std::vector<ScMatrixRef> aResMatrix;
    while (nGlobalError == FormulaError::NONE && nParamCount-- > 0)
    {
        if (IsMissing())
        {
            PushIllegalParameter();
            return;
        }

        ScMatrixRef pRefMatrix = GetMatrix();
        if (!pRefMatrix)
        {
            PushIllegalParameter();
            return;
        }

        SCSIZE nC = 0, nR = 0;
        pRefMatrix->GetDimensions(nC, nR);
        aStackDimensions = searray::appendStackDimensions(
            aStackDimensions, { static_cast<sal_Int32>(nC), static_cast<sal_Int32>(nR) },
            bHorizontal ? searray::StackDirection::Horizontal : searray::StackDirection::Vertical);
        aResMatrix.emplace_back(pRefMatrix);
    }

    // No result
    if (aResMatrix.size() == 0)
    {
        PushNA();
        return;
    }

    ScMatrixRef pResMat = GetNewMat(aStackDimensions.mnColumns, aStackDimensions.mnRows, /*bEmpty*/true);
    if (!pResMat)
    {
        PushIllegalArgument();
        return;
    }

    SCSIZE nCount = 0;
    for (const ScMatrixRef& rMatrix : aResMatrix)
    {
        SCSIZE nC = 0, nR = 0;
        rMatrix->GetDimensions(nC, nR);
        if (bHorizontal)
        {
            for (SCSIZE col = 0; col < nC; ++col)
            {
                for (SCSIZE row = 0; row < aStackDimensions.mnRows; ++row)
                {
                    if (row < nR)
                        lcl_FillCell(rMatrix, pResMat, col, row, nCount, row);
                    else
                        pResMat->PutError(FormulaError::NotAvailable, nCount, row);
                }
                ++nCount;
            }
        }
        else
        {
            for (SCSIZE row = 0; row < nR; ++row)
            {
                for (SCSIZE col = 0; col < aStackDimensions.mnColumns; ++col)
                {
                    if (col < nC)
                        lcl_FillCell(rMatrix, pResMat, col, row, col, nCount);
                    else
                        pResMat->PutError(FormulaError::NotAvailable, col, nCount);
                }
                ++nCount;
            }
        }
    }

    PushMatrix(pResMat);
}

static std::vector<OUString> lcl_SplitText(const OUString& rText, const std::vector<svl::SharedString>& rDelimiters,
        bool bIgnoreEmpty, bool bMatchMode)
{
    std::vector<OUString> aResStr;
    if (!rDelimiters.size() || rText.isEmpty())
    {
        aResStr.push_back(rText);
    }
    else
    {
        const sal_Int32 nLength (rText.getLength());
        sal_Int32 nStart(0);
        while (nStart < nLength)
        {
            sal_Int32 nIndex = nLength;
            sal_Int32 nDelLength(0);

            // Find the first delimiter
            for (auto& rDelimiter : rDelimiters)
            {
                if (rDelimiter.isEmpty())
                    continue;

                OUString sDelimiter = rDelimiter.getString();
                sal_Int32 nDelimiterIndex;
                if (bMatchMode)
                {
                    nDelimiterIndex = ScGlobal::getCharClass().lowercase(rText).indexOf(
                            ScGlobal::getCharClass().lowercase(sDelimiter), nStart);
                }
                else
                    nDelimiterIndex = rText.indexOf(sDelimiter, nStart);

                if (nDelimiterIndex != -1 && nDelimiterIndex < nIndex)
                {
                    nDelLength = sDelimiter.getLength();
                    nIndex = nDelimiterIndex;
                }
            }

            OUString sRes(rText.copy(nStart, nIndex - nStart));
            if (!bIgnoreEmpty || !sRes.isEmpty())
            {
                aResStr.push_back(sRes);
            }
            nStart = nIndex + nDelLength;
        }
    }
    return aResStr;
}

void ScInterpreter::ScTextSplit()
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 1, 6))
        return;

    // 6rd argument optional - pad_with
    std::optional<svl::SharedString> aPadWith;
    if (nParamCount == 6)
        aPadWith = GetString();

    // 5rd argument optional - match_mode
    bool bMatchMode = false;
    if (nParamCount >= 5)
    {
        if (!IsMissing())
        {
            bMatchMode = GetBool();
        }
        else
            Pop();
    }

    // 4rd argument optional - ignore_empty
    bool bIgnoreEmpty = false;
    if (nParamCount >= 4)
    {
        if (!IsMissing())
            bIgnoreEmpty = GetBool();
        else
            Pop();
    }

    // 3rd argument optional - row_delimiter
    std::vector<svl::SharedString> aRowDelimiters;
    if (nParamCount >= 3)
    {
        ScMatrixRef pMatSource = nullptr;
        SCSIZE nsC = 0, nsR = 0;
        switch (GetStackType())
        {
            case svSingleRef:
            case svDoubleRef:
            case svMatrix:
            case svExternalSingleRef:
            case svExternalDoubleRef:
            {
                pMatSource = GetMatrix();
                if (!pMatSource)
                {
                    PushIllegalParameter();
                    return;
                }

                pMatSource->GetDimensions(nsC, nsR);
                for (SCSIZE i = 0; i < nsC; i++)
                {
                    for (SCSIZE j = 0; j < nsR; j++)
                    {
                        aRowDelimiters.push_back(pMatSource->GetString(i,j));
                    }
                }
            }
            break;

            default:
                aRowDelimiters.push_back(GetString());
        }
    }

    // 2nd argument optional - col_delimiter
    std::vector<svl::SharedString> aColDelimiters;
    if (nParamCount >= 2)
    {
        ScMatrixRef pMatSource = nullptr;
        SCSIZE nsC = 0, nsR = 0;
        switch (GetStackType())
        {
            case svSingleRef:
            case svDoubleRef:
            case svMatrix:
            case svExternalSingleRef:
            case svExternalDoubleRef:
            {
                pMatSource = GetMatrix();
                if (!pMatSource)
                {
                    PushIllegalParameter();
                    return;
                }

                pMatSource->GetDimensions(nsC, nsR);
                for (SCSIZE i = 0; i < nsC; i++)
                {
                    for (SCSIZE j = 0; j < nsR; j++)
                    {
                        aColDelimiters.push_back(pMatSource->GetString(i,j));
                    }
                }
            }
            break;

            default:
                aColDelimiters.push_back(GetString());
        }
    }

    // 1st argument: text
    svl::SharedString sText = GetString();
    if (sText.isEmpty())
    {
        PushIllegalParameter();
        return;
    }

    std::vector<OUString> aRowStrs = lcl_SplitText(sText.getString(), aRowDelimiters, bIgnoreEmpty, bMatchMode);
    std::vector<std::vector<OUString>> aRes;

    SCSIZE nCols = 1;
    SCSIZE nRows = aRowStrs.size();
    for (auto& rRow : aRowStrs)
    {
        std::vector<OUString> aColStrs = lcl_SplitText(rRow, aColDelimiters, bIgnoreEmpty, bMatchMode);
        nCols = std::max(nCols, aColStrs.size());
        aRes.push_back(std::move(aColStrs));
    }

    ScMatrixRef pResMat = GetNewMat(nCols, nRows, /*bEmpty*/true);
    for (SCSIZE col = 0; col < nCols; ++col)
    {
        for (SCSIZE row = 0; row < nRows; ++row)
        {
            if (col < aRes[row].size())
            {
                pResMat->PutString(mrStrPool.intern(aRes[row][col]), col, row);
            }
            else
            {
                if (!aPadWith.has_value())
                    pResMat->PutError(FormulaError::NotAvailable, col, row);
                else
                    pResMat->PutString(aPadWith.value(), col, row);
            }
        }
    }

    PushMatrix(pResMat);
}

void ScInterpreter::ScToColOrRow(bool bCol)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 1, 3))
        return;

    // 3rd argument optional - Scan_by_column: default FALSE
    bool bByColumn = false;
    if (nParamCount == 3)
        bByColumn = GetBoolWithDefault(false);

    // 2nd argument optional - Ignore: default keep all values
    IgnoreValues eIgnoreValues = IgnoreValues::DEFAULT;
    if (nParamCount >= 2)
    {
        sal_Int32 k = GetInt32WithDefault(0);
        if (k >= 0 && k <= 3)
            eIgnoreValues = static_cast<IgnoreValues>(k);
        else
        {
            PushIllegalParameter();
            return;
        }
    }

    // 1st argument: take unique search range
    ScMatrixRef pMatSource = nullptr;
    SCSIZE nsC = 0, nsR = 0;
    switch (GetStackType())
    {
        case svSingleRef:
        case svDoubleRef:
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            pMatSource = GetMatrix();
            if (!pMatSource)
            {
                PushIllegalParameter();
                return;
            }

            pMatSource->GetDimensions(nsC, nsR);
        }
        break;

        default:
            PushIllegalParameter();
            return;
    }

    if (nGlobalError != FormulaError::NONE || nsC < 1 || nsR < 1)
    {
        PushIllegalArgument();
        return;
    }

    std::vector<std::pair<SCSIZE, SCSIZE>> aResPos;
    SCSIZE nOut = bByColumn ? nsC : nsR;
    SCSIZE nIn = bByColumn ? nsR : nsC;

    for (SCSIZE i = 0; i < nOut; i++)
    {
        for (SCSIZE j = 0; j < nIn; j++)
        {
            SCSIZE nCol = bByColumn ? i : j;
            SCSIZE nRow = bByColumn ? j : i;
            if (searray::shouldIncludeFlattenedValue(
                    static_cast<searray::FlattenIgnore>(eIgnoreValues),
                    pMatSource->IsEmptyCell(nCol, nRow),
                    pMatSource->GetError(nCol, nRow) != FormulaError::NONE))
                aResPos.emplace_back(nCol, nRow);
        }

    }
    const auto aFlattenDimensions
        = searray::planFlattenOutputDimensions(aResPos.size(), bCol);
    if (!aFlattenDimensions)
    {
        PushNA();
        return;
    }

    SCSIZE nColumns = aFlattenDimensions.maValue.mnColumns;
    SCSIZE nRows = aFlattenDimensions.maValue.mnRows;

    ScMatrixRef pResMat = GetNewMat(nColumns, nRows, /*bEmpty*/true);
    if (!pResMat)
    {
        PushIllegalArgument();
        return;
    }

    // fill result matrix to the same column
    for (SCSIZE iPos = 0; iPos < aResPos.size(); ++iPos)
    {
        const auto aDest = searray::flattenDestination(iPos, bCol);
        lcl_FillCell(pMatSource, pResMat, aResPos[iPos].first, aResPos[iPos].second,
                     aDest.mnColumn, aDest.mnRow);
    }

    PushMatrix(pResMat);
}

void ScInterpreter::ScUnique()
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 1, 3))
        return;

    // 3rd argument optional - Exactly_once: default FALSE
    bool bExactly_once = false;
    if (nParamCount == 3)
        bExactly_once = GetBoolWithDefault(false);

    // 2nd argument optional - default: By_Col = false --> bByRow = true
    bool bByRow = true;
    if (nParamCount >= 2)
        bByRow = !GetBoolWithDefault(false);

    // 1st argument: take unique search range
    ScMatrixRef pMatSource = nullptr;
    SCSIZE nsC = 0, nsR = 0;
    switch (GetStackType())
    {
        case svSingleRef:
        case svDoubleRef:
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            pMatSource = GetMatrix();
            if (!pMatSource)
            {
                PushIllegalParameter();
                return;
            }

            pMatSource->GetDimensions(nsC, nsR);
        }
        break;

        default:
            PushIllegalParameter();
            return;
    }

    if (nGlobalError != FormulaError::NONE || nsC < 1 || nsR < 1)
    {
        PushIllegalArgument();
        return;
    }

    // Create unique dataset
    std::unordered_set<OUString> aStrSet;
    std::vector<std::pair<SCSIZE, OUString>> aResPos;
    SCSIZE nOut = bByRow ? nsR : nsC;
    SCSIZE nIn = bByRow ? nsC : nsR;

    for (SCSIZE i = 0; i < nOut; i++)
    {
        OUString aStr;
        for (SCSIZE j = 0; j < nIn; j++)
        {
            OUString aCellStr = bByRow ? pMatSource->GetString(mrContext, j, i).getString() :
                pMatSource->GetString(mrContext, i, j).getString();
            aStr += aCellStr + u"\x0001";
        }

        if (aStrSet.insert(ScGlobal::getCharClass().lowercase(aStr)).second) // unique if inserted
        {
            aResPos.emplace_back(std::make_pair(i, aStr));
        }
        else
        {
            if (bExactly_once)
            {
                auto it = std::find_if(aResPos.begin(), aResPos.end(),
                    [str = ScGlobal::getCharClass().lowercase(aStr)](const std::pair<SCSIZE, OUString>& aRes)
                    {
                        return ScGlobal::getCharClass().lowercase(aRes.second).equals(str);
                    }
                );
                if (it != aResPos.end())
                    aResPos.erase(it);
            }
        }
    }
    // No result
    if (aResPos.size() == 0)
    {
        PushNA();
        return;
    }

    ScMatrixRef pResMat = bByRow ? GetNewMat(nsC, aResPos.size(), /*bEmpty*/true) :
        GetNewMat(aResPos.size(), nsR, /*bEmpty*/true);
    if (!pResMat)
    {
        PushIllegalArgument();
        return;
    }
    // fill result matrix with unique values
    for (SCSIZE iPos = 0; iPos < aResPos.size(); iPos++)
    {
        if (bByRow)
        {
            for (SCSIZE col = 0; col < nsC; col++)
            {
                lcl_FillCell(pMatSource, pResMat, col, aResPos[iPos].first, col, iPos);
            }
        }
        else
        {
            for (SCSIZE row = 0; row < nsR; row++)
            {
                lcl_FillCell(pMatSource, pResMat, aResPos[iPos].first, row, iPos, row);
            }
        }
    }

    PushMatrix(pResMat);
}

void ScInterpreter::ScLet()
{
    const short* pJump = pCur->GetJump();
    short nJumpCount = pJump[0];
    short nOrgJumpCount = nJumpCount;

    if (nJumpCount < 3 || (nJumpCount % 2 != 1))
    {
        PushError(FormulaError::ParameterExpected);
        aCode.Jump(pJump[nOrgJumpCount], pJump[nOrgJumpCount]);
        return;
    }

    OUString aStrName;
    std::unordered_map<OUString, formula::FormulaToken*> nResultIndexes;
    formula::FormulaTokenArrayPlainIterator aIter(*pArr);
    // clone tokens for replacing string name tokens
    ScTokenArray aValueTokens = pArr->CloneValue();

    // name and function pairs parameter
    while (nJumpCount > 1)
    {
        if (nJumpCount == nOrgJumpCount)
        {
            aStrName = GetString().getString();
        }
        else if ((nOrgJumpCount - nJumpCount + 1) % 2 == 1)
        {
            aIter.Jump(pJump[static_cast<short>(nOrgJumpCount - nJumpCount + 1)] - 1);
            FormulaToken* t = aIter.NextRPN();
            aStrName = t->GetString().getString();
        }
        else
        {
            PushError(FormulaError::ParameterExpected);
            aCode.Jump(pJump[nOrgJumpCount], pJump[nOrgJumpCount]);
            return;
        }
        nJumpCount--;

        // replace names with result tokens
        seletexec::replaceNamesToResult(
            nResultIndexes, aValueTokens, pJump[nOrgJumpCount - nJumpCount],
            pJump[nOrgJumpCount - nJumpCount + 1]);

        ScTokenArray aTempTokens = seletexec::copyTokenSlice(
            mrDoc, aValueTokens, pJump[nOrgJumpCount - nJumpCount],
            pJump[nOrgJumpCount - nJumpCount + 1]);

        // calculate the inner results unless we already have a push result token
        if (aTempTokens.GetLen() == 0)
        {
            PushIllegalParameter();
            aCode.Jump(pJump[nOrgJumpCount], pJump[nOrgJumpCount]);
            return;
        }
        else if (aTempTokens.GetLen() == 1 && aTempTokens.GetArray()[0]->GetOpCode() == ocPush)
        {
            if (!nResultIndexes.insert(std::make_pair(aStrName, aTempTokens.GetArray()[0]->Clone())).second)
            {
                PushIllegalParameter();
                aCode.Jump(pJump[nOrgJumpCount], pJump[nOrgJumpCount]);
                return;
            }
        }
        else
        {
            ScInterpreter aInt(mrDoc.GetFormulaCell(aPos), mrDoc, mrContext, aPos, aValueTokens);
            aInt.aCode.Jump(pJump[nOrgJumpCount - nJumpCount], pJump[nOrgJumpCount - nJumpCount + 1], pJump[nOrgJumpCount - nJumpCount + 1]);
            while (aInt.aCode.HasStacked())
                aInt.aCode.FrontPop();
            aInt.aCode.Lambda(true);

            sfx2::LinkManager aNewLinkMgr(mrDoc.GetDocumentShell());
            aInt.SetLinkManager(&aNewLinkMgr);

            formula::StackVar aIntType = aInt.Interpret();

            if (aIntType == formula::svMatrixCell)
            {
                ScConstMatrixRef xMat(aInt.GetResultToken()->GetMatrix());
                if (!nResultIndexes.insert(std::make_pair(aStrName, new ScMatrixToken(xMat->Clone()))).second)
                {
                    PushIllegalParameter();
                    aCode.Jump(pJump[nOrgJumpCount], pJump[nOrgJumpCount]);
                    return;
                }
            }
            else
            {
                const FormulaConstTokenRef& xTok(aInt.GetResultToken());
                if (!nResultIndexes.insert(std::make_pair(aStrName, xTok->Clone())).second)
                {
                    PushIllegalParameter();
                    aCode.Jump(pJump[nOrgJumpCount], pJump[nOrgJumpCount]);
                    return;
                }
            }
        }
        nJumpCount--;
    }

    // last parameter: calculation
    // replace names with result tokens
    seletexec::replaceNamesToResult(
        nResultIndexes, aValueTokens, pJump[nOrgJumpCount - nJumpCount],
        pJump[nOrgJumpCount - nJumpCount + 1]);

    // calculate the final result
    ScInterpreter aInt(mrDoc.GetFormulaCell(aPos), mrDoc, mrContext, aPos, aValueTokens);
    aInt.aCode.Jump(pJump[nOrgJumpCount - nJumpCount], pJump[nOrgJumpCount - nJumpCount + 1], pJump[nOrgJumpCount - nJumpCount + 1]);
    while (aInt.aCode.HasStacked())
        aInt.aCode.FrontPop();
    aInt.aCode.Lambda(true);

    sfx2::LinkManager aNewLinkMgr(mrDoc.GetDocumentShell());
    aInt.SetLinkManager(&aNewLinkMgr);
    formula::StackVar aIntType = aInt.Interpret();

    if (aIntType == formula::svMatrixCell)
    {
        ScConstMatrixRef xMat(aInt.GetResultToken()->GetMatrix());
        PushTokenRef(new ScMatrixToken(xMat->Clone()));
    }
    else
    {
        const formula::FormulaConstTokenRef& xLambdaResult(aInt.GetResultToken());
        if (xLambdaResult)
        {
            nGlobalError = xLambdaResult->GetError();
            if (nGlobalError == FormulaError::NONE)
                PushTokenRef(xLambdaResult);
            else
                PushError(nGlobalError);
        }
    }

    nJumpCount--;
    aCode.Jump(pJump[nOrgJumpCount], pJump[nOrgJumpCount]);
}

void ScInterpreter::ScSubTotal()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCountMinWithStackCheck( nParamCount, 2 ) )
        return;

    // We must fish the 1st parameter deep from the stack! And push it on top.
    const FormulaToken* p = pStack[ sp - nParamCount ];
    PushWithoutError( *p );
    sal_Int32 nFunc = GetInt32();
    mnSubTotalFlags |= SubtotalFlags::IgnoreNestedStAg | SubtotalFlags::IgnoreFiltered;
    if (nFunc > 100)
    {
        // For opcodes 101 through 111, we need to skip hidden cells.
        // Other than that these opcodes are identical to 1 through 11.
        mnSubTotalFlags |= SubtotalFlags::IgnoreHidden;
        nFunc -= 100;
    }

    if ( nGlobalError != FormulaError::NONE || nFunc < 1 || nFunc > 11 )
        PushIllegalArgument();  // simulate return on stack, not SetError(...)
    else
    {
        cPar = nParamCount - 1;
        const auto pushSubtotalMin = [&]() {
            short nSubParamCount = GetByte();
            if (!MustHaveParamCountMin(nSubParamCount, 1))
                return;

            ScMatrixRef xResMat;
            double nMin = ::std::numeric_limits<double>::max();
            auto MatOpFunc = [&xResMat](SCSIZE i, double fCurMin) {
                double fVecRes = xResMat->GetDouble(0, i);
                if (fVecRes > fCurMin)
                    xResMat->PutDouble(fCurMin, 0, i);
            };
            const SCSIZE nMatRows = GetRefListArrayMaxSize(nSubParamCount);
            size_t nRefArrayPos = std::numeric_limits<size_t>::max();

            double nVal = 0.0;
            ScAddress aAdr;
            ScRange aRange;
            size_t nRefInList = 0;
            while (nSubParamCount-- > 0)
            {
                switch (GetStackType())
                {
                    case svDouble:
                    {
                        nVal = GetDouble();
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
                    }
                    break;
                    case svRefList:
                    {
                        if (SwitchToArrayRefList(
                                xResMat, nMatRows, nMin, MatOpFunc,
                                nRefArrayPos == std::numeric_limits<size_t>::max()))
                            nRefArrayPos = nRefInList;
                    }
                    [[fallthrough]];
                    case svDoubleRef:
                    {
                        FormulaError nErr = FormulaError::NONE;
                        PopDoubleRef(aRange, nSubParamCount, nRefInList);
                        ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags, false);
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
                                false, bool(mnSubTotalFlags & SubtotalFlags::IgnoreErrVal));
                            if (nMin > nVal)
                                nMin = nVal;
                        }
                    }
                    break;
                    case svString:
                        Pop();
                        SetError(FormulaError::IllegalParameter);
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
                PushError(GetDoubleErrorValue(nVal));
            else if (nVal < nMin)
                PushDouble(0.0);
            else
                PushDouble(nMin);
        };
        const auto pushSubtotalMax = [&]() {
            short nSubParamCount = GetByte();
            if (!MustHaveParamCountMin(nSubParamCount, 1))
                return;

            ScMatrixRef xResMat;
            double nMax = std::numeric_limits<double>::lowest();
            auto MatOpFunc = [&xResMat](SCSIZE i, double fCurMax) {
                double fVecRes = xResMat->GetDouble(0, i);
                if (fVecRes < fCurMax)
                    xResMat->PutDouble(fCurMax, 0, i);
            };
            const SCSIZE nMatRows = GetRefListArrayMaxSize(nSubParamCount);
            size_t nRefArrayPos = std::numeric_limits<size_t>::max();

            double nVal = 0.0;
            ScAddress aAdr;
            ScRange aRange;
            size_t nRefInList = 0;
            while (nSubParamCount-- > 0)
            {
                switch (GetStackType())
                {
                    case svDouble:
                    {
                        nVal = GetDouble();
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
                    }
                    break;
                    case svRefList:
                    {
                        if (SwitchToArrayRefList(
                                xResMat, nMatRows, nMax, MatOpFunc,
                                nRefArrayPos == std::numeric_limits<size_t>::max()))
                            nRefArrayPos = nRefInList;
                    }
                    [[fallthrough]];
                    case svDoubleRef:
                    {
                        FormulaError nErr = FormulaError::NONE;
                        PopDoubleRef(aRange, nSubParamCount, nRefInList);
                        ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags, false);
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
                                false, bool(mnSubTotalFlags & SubtotalFlags::IgnoreErrVal));
                            if (nMax < nVal)
                                nMax = nVal;
                        }
                    }
                    break;
                    case svString:
                        Pop();
                        SetError(FormulaError::IllegalParameter);
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
                PushError(GetDoubleErrorValue(nVal));
            else if (nVal > nMax)
                PushDouble(0.0);
            else
                PushDouble(nMax);
        };
        const auto pushSubtotalVar = [&](bool bPopulation) {
            if (bPopulation)
            {
                auto VarResult = [](double fVal, size_t nValCount) {
                    return sc::div(fVal, nValCount);
                };
                GetStVarParams(false, VarResult);
            }
            else
            {
                auto VarResult = [](double fVal, size_t nValCount) {
                    if (nValCount <= 1)
                        return CreateDoubleError(FormulaError::DivisionByZero);
                    return fVal / (nValCount - 1);
                };
                GetStVarParams(false, VarResult);
            }
        };
        const auto pushSubtotalStDev = [&](bool bPopulation) {
            if (bPopulation)
            {
                auto VarResult = [](double fVal, size_t nValCount) {
                    if (nValCount == 0)
                        return CreateDoubleError(FormulaError::DivisionByZero);
                    return sqrt(fVal / nValCount);
                };
                GetStVarParams(false, VarResult);
            }
            else
            {
                auto VarResult = [](double fVal, size_t nValCount) {
                    if (nValCount <= 1)
                        return CreateDoubleError(FormulaError::DivisionByZero);
                    return sqrt(fVal / (nValCount - 1));
                };
                GetStVarParams(false, VarResult);
            }
        };
        switch( nFunc )
        {
            case SUBTOTAL_FUNC_AVE  : IterateParameters(ifAVERAGE); break;
            case SUBTOTAL_FUNC_CNT  : IterateParameters(ifCOUNT);   break;
            case SUBTOTAL_FUNC_CNT2 : IterateParameters(ifCOUNT2);  break;
            case SUBTOTAL_FUNC_MAX  : pushSubtotalMax();            break;
            case SUBTOTAL_FUNC_MIN  : pushSubtotalMin();            break;
            case SUBTOTAL_FUNC_PROD : IterateParameters(ifPRODUCT); break;
            case SUBTOTAL_FUNC_STD  : pushSubtotalStDev(false);     break;
            case SUBTOTAL_FUNC_STDP : pushSubtotalStDev(true);      break;
            case SUBTOTAL_FUNC_SUM  : IterateParameters(ifSUM);     break;
            case SUBTOTAL_FUNC_VAR  : pushSubtotalVar(false);       break;
            case SUBTOTAL_FUNC_VARP : pushSubtotalVar(true);        break;
            default : PushIllegalArgument();       break;
        }
    }
    mnSubTotalFlags = SubtotalFlags::NONE;
    // Get rid of the 1st (fished) parameter.
    FormulaConstTokenRef xRef( PopToken());
    Pop();
    PushTokenRef( xRef);
}

void ScInterpreter::ScWrapColsOrRows(bool bCols)
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, 3))
        return;

    // 3rd argument optional - pad_with
    std::optional<bool> bDouble;
    double fNumber(0.0);
    svl::SharedString aString;
    if (nParamCount == 3)
        bDouble = GetDoubleOrString(fNumber, aString);

    // 2nd argument - wrap_count
    SCSIZE nWrap = GetInt32WithDefault(0);
    if (nWrap <= 0)
    {
        PushIllegalParameter();
        return;
    }

    // 1st argument: take range
    ScMatrixRef pMatSource = nullptr;
    SCSIZE nsC = 0, nsR = 0;
    switch (GetStackType())
    {
        case svSingleRef:
        case svDoubleRef:
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            pMatSource = GetMatrix();
            if (!pMatSource)
            {
                PushIllegalParameter();
                return;
            }

            pMatSource->GetDimensions(nsC, nsR);
        }
        break;

        default:
            PushIllegalParameter();
            return;
    }

    if (nGlobalError != FormulaError::NONE || nsC < 1 || nsR < 1)
    {
        PushIllegalArgument();
        return;
    }

    const auto aWrapDimensions = searray::planWrapOutputDimensions(
        { static_cast<sal_Int32>(nsC), static_cast<sal_Int32>(nsR) }, nWrap, bCols);
    if (!aWrapDimensions)
    {
        PushIllegalArgument();
        return;
    }

    SCSIZE nColumns = aWrapDimensions.maValue.mnColumns;
    SCSIZE nRows = aWrapDimensions.maValue.mnRows;
    ScMatrixRef pResMat = GetNewMat(nColumns, nRows, /*bEmpty*/true);
    if (!pResMat)
    {
        PushIllegalArgument();
        return;
    }

    const SCSIZE nElementCount = nsC * nsR;
    for (SCSIZE iPos = 0; iPos < nElementCount; ++iPos)
    {
        const auto aDest = searray::wrapDestination(iPos, nWrap, bCols);
        const SCSIZE nSourceCol = nsC == 1 ? 0 : iPos;
        const SCSIZE nSourceRow = nsC == 1 ? iPos : 0;
        lcl_FillCell(pMatSource, pResMat, nSourceCol, nSourceRow, aDest.mnColumn, aDest.mnRow);
    }

    for (SCSIZE col = 0; col < nColumns; ++col)
    {
        for (SCSIZE row = 0; row < nRows; ++row)
        {
            const SCSIZE nLinearIndex = bCols ? (col * nWrap + row) : (row * nWrap + col);
            if (nLinearIndex < nElementCount)
                continue;
            if (bDouble.has_value())
            {
                if (bDouble.value())
                    pResMat->PutDouble(fNumber, col, row);
                else
                    pResMat->PutString(aString, col, row);
            }
            else
                pResMat->PutError(FormulaError::NotAvailable, col, row);
        }
    }

    PushMatrix(pResMat);
}

std::unique_ptr<ScDBQueryParamBase> ScInterpreter::GetDBParams( bool& rMissingField )
{
    bool bAllowMissingField = false;
    if ( rMissingField )
    {
        bAllowMissingField = true;
        rMissingField = false;
    }
    if ( GetByte() == 3 )
    {
        // First, get the query criteria range.
        ::std::unique_ptr<ScDBRangeBase> pQueryRef( PopDBDoubleRef() );
        if (!pQueryRef)
            return nullptr;

        bool    bByVal = true;
        double  nVal = 0.0;
        svl::SharedString  aStr;
        ScRange aMissingRange;
        bool bRangeFake = false;
        switch (GetStackType())
        {
            case svDouble :
                nVal = ::rtl::math::approxFloor( GetDouble() );
                if ( bAllowMissingField && nVal == 0.0 )
                    rMissingField = true;   // fake missing parameter
                break;
            case svString :
                bByVal = false;
                aStr = GetString();
                break;
            case svSingleRef :
                {
                    ScAddress aAdr;
                    PopSingleRef( aAdr );
                    ScRefCellValue aCell(mrDoc, aAdr);
                    if (aCell.hasNumeric())
                        nVal = GetCellValue(aAdr, aCell);
                    else
                    {
                        bByVal = false;
                        GetCellString(aStr, aCell);
                    }
                }
                break;
            case svDoubleRef :
                if ( bAllowMissingField )
                {   // fake missing parameter for old SO compatibility
                    bRangeFake = true;
                    PopDoubleRef( aMissingRange );
                }
                else
                {
                    PopError();
                    SetError( FormulaError::IllegalParameter );
                }
                break;
            case svMissing :
                PopError();
                if ( bAllowMissingField )
                    rMissingField = true;
                else
                    SetError( FormulaError::IllegalParameter );
                break;
            default:
                PopError();
                SetError( FormulaError::IllegalParameter );
        }

        if (nGlobalError != FormulaError::NONE)
            return nullptr;

        std::unique_ptr<ScDBRangeBase> pDBRef( PopDBDoubleRef() );

        if (nGlobalError != FormulaError::NONE || !pDBRef)
            return nullptr;

        if ( bRangeFake )
        {
            // range parameter must match entire database range
            if (pDBRef->isRangeEqual(aMissingRange))
                rMissingField = true;
            else
                SetError( FormulaError::IllegalParameter );
        }

        if (nGlobalError != FormulaError::NONE)
            return nullptr;

        SCCOL nField = pDBRef->getFirstFieldColumn();
        if (rMissingField)
            ; // special case
        else if (bByVal)
            nField = pDBRef->findFieldColumn(static_cast<SCCOL>(nVal));
        else
        {
            FormulaError nErr = FormulaError::NONE;
            nField = pDBRef->findFieldColumn(aStr.getString(), &nErr);
            SetError(nErr);
        }

        if (!mrDoc.ValidCol(nField))
            return nullptr;

        std::unique_ptr<ScDBQueryParamBase> pParam( pDBRef->createQueryParam(pQueryRef.get()) );

        if (pParam)
        {
            // An allowed missing field parameter sets the result field
            // to any of the query fields, just to be able to return
            // some cell from the iterator.
            if ( rMissingField )
                nField = static_cast<SCCOL>(pParam->GetEntry(0).nField);
            pParam->mnField = nField;

            SCSIZE nCount = pParam->GetEntryCount();
            for ( SCSIZE i=0; i < nCount; i++ )
            {
                ScQueryEntry& rEntry = pParam->GetEntry(i);
                if (!rEntry.bDoQuery)
                    break;

                ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
                sal_uInt32 nIndex = 0;
                OUString aQueryStr = rItem.maString.getString();
                bool bNumber = mrContext.NFIsNumberFormat(
                    aQueryStr, nIndex, rItem.mfVal);
                rItem.meType = bNumber ? ScQueryEntry::ByValue : ScQueryEntry::ByString;

                if (!bNumber && pParam->eSearchType == utl::SearchParam::SearchType::Normal)
                    pParam->eSearchType = DetectSearchType(aQueryStr, mrDoc);
            }
            return pParam;
        }
    }
    return nullptr;
}

void ScInterpreter::DBIterator( ScIterFunc eFunc )
{
    double fRes = 0;
    KahanSum fErg = 0;
    sal_uLong nCount = 0;
    bool bMissingField = false;
    std::unique_ptr<ScDBQueryParamBase> pQueryParam( GetDBParams(bMissingField) );
    if (pQueryParam)
    {
        if (!pQueryParam->IsValidFieldIndex())
        {
            SetError(FormulaError::NoValue);
            return;
        }
        ScDBQueryDataIterator aValIter(mrDoc, mrContext, std::move(pQueryParam));
        ScDBQueryDataIterator::Value aValue;
        if ( aValIter.GetFirst(aValue) && aValue.mnError == FormulaError::NONE )
        {
            switch( eFunc )
            {
                case ifPRODUCT: fRes = 1; break;
                case ifMAX:     fRes = -MAXDOUBLE; break;
                case ifMIN:     fRes = MAXDOUBLE; break;
                default: ; // nothing
            }

            do
            {
                nCount++;
                switch( eFunc )
                {
                    case ifAVERAGE:
                    case ifSUM:
                        fErg += aValue.mfValue;
                        break;
                    case ifSUMSQ:
                        fErg += aValue.mfValue * aValue.mfValue;
                        break;
                    case ifPRODUCT:
                        fRes *= aValue.mfValue;
                        break;
                    case ifMAX:
                        if( aValue.mfValue > fRes ) fRes = aValue.mfValue;
                        break;
                    case ifMIN:
                        if( aValue.mfValue < fRes ) fRes = aValue.mfValue;
                        break;
                    default: ; // nothing
                }
            }
            while ( aValIter.GetNext(aValue) && aValue.mnError == FormulaError::NONE );
        }
        SetError(aValue.mnError);
    }
    else
        SetError( FormulaError::IllegalParameter);
    switch( eFunc )
    {
        case ifCOUNT:   fRes = nCount; break;
        case ifSUM:     fRes = fErg.get(); break;
        case ifSUMSQ:   fRes = fErg.get(); break;
        case ifAVERAGE: fRes = div(fErg.get(), nCount); break;
        default: ; // nothing
    }
    PushDouble( fRes );
}

void ScInterpreter::GetDBStVarParams( std::vector<double>& rValues )
{
    rValues.clear();
    bool bMissingField = false;
    std::unique_ptr<ScDBQueryParamBase> pQueryParam( GetDBParams(bMissingField) );
    if (pQueryParam)
    {
        if (!pQueryParam->IsValidFieldIndex())
        {
            SetError(FormulaError::NoValue);
            return;
        }
        ScDBQueryDataIterator aValIter(mrDoc, mrContext, std::move(pQueryParam));
        ScDBQueryDataIterator::Value aValue;
        if (aValIter.GetFirst(aValue) && aValue.mnError == FormulaError::NONE)
        {
            do
            {
                rValues.push_back(aValue.mfValue);
            }
            while ((aValue.mnError == FormulaError::NONE) && aValIter.GetNext(aValue));
        }
        SetError(aValue.mnError);
    }
    else
        SetError( FormulaError::IllegalParameter);
}

void ScInterpreter::ScIndirect()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 )  )
        return;

    const bool bForceR1C1 = (nParamCount == 2 && 0.0 == GetDouble());
    const auto aSyntaxPolicy = sestringref::resolveIndirectAddressSyntaxPolicy(
        selibreoffice::toApiAddressConvention(maCalcConfig.meStringRefAddressSyntax),
        selibreoffice::toApiAddressConvention(mrDoc.GetAddressConvention()),
        maCalcConfig.meStringRefAddressSyntax == FormulaGrammar::CONV_A1_XL_A1, bForceR1C1);
    FormulaGrammar::AddressConvention eConv
        = selibreoffice::toLibreOfficeAddressConvention(aSyntaxPolicy.mePrimary);
    const bool bTryXlA1 = aSyntaxPolicy.moFallback == spreadsheetengine::api::AddressConvention::XlA1;

    svl::SharedString sSharedRefStr = GetString();
    if (sSharedRefStr.getString().isEmpty())
    {
        // Bail out early for empty cells, rely on "we do have a string" below.
        PushError( FormulaError::NoRef);
        return;
    }

    const auto oResolved = seindirectexec::resolveIndirectReference(
        mrDoc, aPos, sSharedRefStr, eConv, bTryXlA1);
    if (!oResolved)
    {
        PushError(FormulaError::NoRef);
        return;
    }

    switch (oResolved->meKind)
    {
        case seindirectexec::IndirectExecutionResult::Kind::SingleRef:
            PushSingleRef(oResolved->maRef1);
            return;
        case seindirectexec::IndirectExecutionResult::Kind::DoubleRef:
            PushDoubleRef(oResolved->maRef1, oResolved->maRef2);
            return;
        case seindirectexec::IndirectExecutionResult::Kind::ExternalSingleRef:
            PushExternalSingleRef(oResolved->mnFileId, oResolved->maTabName,
                oResolved->maRef1.Col(), oResolved->maRef1.Row(), oResolved->maRef1.Tab());
            return;
        case seindirectexec::IndirectExecutionResult::Kind::ExternalDoubleRef:
            PushExternalDoubleRef(oResolved->mnFileId, oResolved->maTabName,
                oResolved->maRef1.Col(), oResolved->maRef1.Row(), oResolved->maRef1.Tab(),
                oResolved->maRef2.Col(), oResolved->maRef2.Row(), oResolved->maRef2.Tab());
            return;
        case seindirectexec::IndirectExecutionResult::Kind::Token:
            PushTokenRef(oResolved->mxToken);
            return;
    }
}

void ScInterpreter::ScAddressFunc()
{
    OUString sTabStr;

    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 2, 5))
        return;

    if (nParamCount >= 5)
        sTabStr = GetString().getString();

    const bool bForceR1C1 = (nParamCount >= 4 && 0.0 == GetDoubleWithDefault( 1.0));
    const FormulaGrammar::AddressConvention eConv
        = selibreoffice::toLibreOfficeAddressConvention(
            sestringref::resolveAddressFunctionConvention(
                selibreoffice::toApiAddressConvention(maCalcConfig.meStringRefAddressSyntax),
                selibreoffice::toApiAddressConvention(mrDoc.GetAddressConvention()), bForceR1C1));

    ScRefFlags nFlags = ScRefFlags::COL_ABS | ScRefFlags::ROW_ABS; // default
    sal_Int32 nAbsMode = 1;
    if (nParamCount >= 3)
    {
        sal_Int32 n = GetInt32WithDefault(1);
        switch (n)
        {
            default:
                PushNoValue();
                return;

            case 5:
            case 1:
                nAbsMode = n;
                break;
            case 6:
            case 2:
                nAbsMode = n;
                nFlags = ScRefFlags::ROW_ABS;
                break;
            case 7:
            case 3:
                nAbsMode = n;
                nFlags = ScRefFlags::COL_ABS;
                break;
            case 8:
            case 4:
                nAbsMode = n;
                nFlags = ScRefFlags::ZERO;
                break; // both relative
        }
    }
    nFlags |= ScRefFlags::VALID | ScRefFlags::ROW_VALID | ScRefFlags::COL_VALID;

    SCCOL nCol = static_cast<SCCOL>(GetInt16());
    SCROW nRow = static_cast<SCROW>(GetInt32());
    if( eConv == FormulaGrammar::CONV_XL_R1C1 )
    {
        // YUCK!  The XL interface actually treats rel R1C1 refs differently
        // than A1
        if( !(nFlags & ScRefFlags::COL_ABS) )
            nCol += aPos.Col() + 1;
        if( !(nFlags & ScRefFlags::ROW_ABS) )
            nRow += aPos.Row() + 1;
    }

    --nCol;
    --nRow;
    if (nGlobalError != FormulaError::NONE || !mrDoc.ValidCol( nCol) || !mrDoc.ValidRow( nRow))
    {
        PushIllegalArgument();
        return;
    }

    serefexec::AddressFunctionRequest aRequest;
    aRequest.mnRow = nRow;
    aRequest.mnColumn = nCol;
    aRequest.mnAbsMode = nAbsMode;
    aRequest.mbA1Style = eConv != FormulaGrammar::CONV_XL_R1C1;
    aRequest.maSheetToken = sTabStr;
    aRequest.meConvention = eConv;
    const auto aAddress = serefexec::formatAddressFunctionResult(aRequest);
    if (!aAddress)
        PushIllegalArgument();
    else
        PushString(aAddress.maValue);
}


void ScInterpreter::ScIndex()
{
    const std::optional<OUString> oQuarantinedFormula
        = lclGetQuarantinedHardRoutedFormula(pMyFormulaCell, mrDoc, mrContext);
    if (oQuarantinedFormula)
    {
        SAL_WARN("sc.core",
            "hard-routed INDEX reached ScInterpreter for " << *oQuarantinedFormula);
        OSL_FAIL("hard-routed INDEX reached ScInterpreter");
    }

    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 4 ) )
        return;

    sal_Int32 nArea;
    size_t nAreaCount;
    SCCOL nCol;
    SCROW nRow;
    if (nParamCount == 4)
        nArea = GetInt32();
    else
        nArea = 1;
    bool bColMissing;
    if (nParamCount >= 3)
    {
        bColMissing = IsMissing();
        nCol = static_cast<SCCOL>(GetInt16());
    }
    else
    {
        bColMissing = false;
        nCol = 0;
    }
    if (nParamCount >= 2)
        nRow = static_cast<SCROW>(GetInt32());
    else
        nRow = 0;
    if (nArea < 1 || nCol < 0 || nRow < 0)
    {
        PushIllegalArgument();
        return;
    }
    if (GetStackType() == svRefList)
        nAreaCount = (sp ? pStack[sp-1]->GetRefList()->size() : 0);
    else
        nAreaCount = 1;     // one reference or array or whatever
    const auto aAreaSelection = serefexec::normalizeAreaSelection(nArea, nAreaCount);
    if (nGlobalError != FormulaError::NONE || !aAreaSelection)
    {
        PushError( FormulaError::NoRef);
        return;
    }
    switch (GetStackType())
    {
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
            {
                assert(nArea == 1);
                sal_uInt16 nOldSp = sp;
                ScMatrixRef pMat = GetMatrix();
                if (!pMat)
                    PushError(FormulaError::NoRef);
                else
                {
                    SCSIZE nC, nR;
                    pMat->GetDimensions(nC, nR);
                    const auto aSelection = seref::planIndexMatrixSelection(
                        { static_cast<sal_Int32>(nC), static_cast<sal_Int32>(nR) }, nRow, nCol,
                        bColMissing, nParamCount);
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
                        if (pMat->IsStringOrEmpty(nMatrixColumn, nMatrixRow))
                            PushString(pMat->GetString(nMatrixColumn, nMatrixRow).getString());
                        else
                            PushDouble(pMat->GetDouble(nMatrixColumn, nMatrixRow));
                    }
                    else
                    {
                        const auto& rSelection = aSelection.maValue;
                        ScMatrixRef pResMat = GetNewMat(
                            rSelection.maDimensions.mnColumns, rSelection.maDimensions.mnRows,
                            /*bEmpty*/true);
                        if (pResMat)
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
                                    if (!pMat->IsStringOrEmpty(nMatrixColumn, nMatrixRow))
                                    {
                                        pResMat->PutDouble(
                                            pMat->GetDouble(nMatrixColumn, nMatrixRow), nResultCol,
                                            nResultRow);
                                    }
                                    else
                                    {
                                        pResMat->PutString(
                                            pMat->GetString(nMatrixColumn, nMatrixRow), nResultCol,
                                            nResultRow);
                                    }
                                }
                            }
                            PushMatrix(pResMat);
                        }
                        else
                            PushError( FormulaError::NoRef);
                    }
                }
            }
            break;
        case svSingleRef:
            {
                SCCOL nCol1 = 0;
                SCROW nRow1 = 0;
                SCTAB nTab1 = 0;
                PopSingleRef( nCol1, nRow1, nTab1);
                const auto aSelection = serefexec::planIndexReferenceSelection(
                    ScRange(nCol1, nRow1, nTab1, nCol1, nRow1, nTab1),
                    nRow, nCol, nParamCount);
                if (!aSelection)
                    PushError(FormulaError::NoRef);
                else
                {
                    const ScRange aRange = selibreoffice::toLibreOfficeRange(aSelection.maValue.maRange);
                    PushSingleRef(aRange.aStart.Col(), aRange.aStart.Row(), aRange.aStart.Tab());
                }
            }
            break;
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
                        PushError( FormulaError::NoRef);
                        return;
                    }
                    ScRange aRange( ScAddress::UNINITIALIZED);
                    DoubleRefToRange((*(xRef->GetRefList()))[aAreaSelection.maValue], aRange);
                    aRange.GetVars( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                }
                else {
                    PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                }
                const auto aSelection = serefexec::planIndexReferenceSelection(
                    ScRange(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2),
                    nRow, nCol, nParamCount);
                if (!aSelection)
                    PushError( FormulaError::NoRef);
                else
                {
                    const ScRange aRange
                        = selibreoffice::toLibreOfficeRange(aSelection.maValue.maRange);
                    if (aSelection.maValue.maRange.isSingleCell())
                        PushSingleRef(aRange.aStart.Col(), aRange.aStart.Row(), aRange.aStart.Tab());
                    else
                        PushDoubleRef(aRange.aStart.Col(), aRange.aStart.Row(), aRange.aStart.Tab(),
                                      aRange.aEnd.Col(), aRange.aEnd.Row(), aRange.aEnd.Tab());
                }
            }
            break;
        default:
            PopError();
            PushError( FormulaError::NoRef);
    }
}

void ScInterpreter::ScMultiArea()
{
    // Legacy support, convert to RefList
    sal_uInt8 nParamCount = GetByte();
    if (MustHaveParamCountMin( nParamCount, 1))
    {
        while (nGlobalError == FormulaError::NONE && nParamCount-- > 1)
        {
            ScUnionFunc();
        }
    }
}


FormulaError ScInterpreter::GetErrorType()
{
    FormulaError nErr;
    FormulaError nOldError = nGlobalError;
    nGlobalError = FormulaError::NONE;
    switch ( GetStackType() )
    {
        case svRefList :
        {
            FormulaConstTokenRef x = PopToken();
            if (nGlobalError != FormulaError::NONE)
                nErr = nGlobalError;
            else
            {
                const ScRefList* pRefList = x->GetRefList();
                size_t n = pRefList->size();
                if (!n)
                    nErr = FormulaError::NoRef;
                else if (n > 1)
                    nErr = FormulaError::NoValue;
                else
                {
                    ScRange aRange;
                    DoubleRefToRange( (*pRefList)[0], aRange);
                    if (nGlobalError != FormulaError::NONE)
                        nErr = nGlobalError;
                    else
                    {
                        ScAddress aAdr;
                        if ( DoubleRefToPosSingleRef( aRange, aAdr ) )
                            nErr = mrDoc.GetErrCode( aAdr );
                        else
                            nErr = nGlobalError;
                    }
                }
            }
        }
        break;
        case svDoubleRef :
        {
            ScRange aRange;
            PopDoubleRef( aRange );
            if ( nGlobalError != FormulaError::NONE )
                nErr = nGlobalError;
            else
            {
                ScAddress aAdr;
                if ( DoubleRefToPosSingleRef( aRange, aAdr ) )
                    nErr = mrDoc.GetErrCode( aAdr );
                else
                    nErr = nGlobalError;
            }
        }
        break;
        case svSingleRef :
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            if ( nGlobalError != FormulaError::NONE )
                nErr = nGlobalError;
            else
                nErr = mrDoc.GetErrCode( aAdr );
        }
        break;
        default:
            PopError();
            nErr = nGlobalError;
    }
    nGlobalError = nOldError;
    return nErr;
}

static bool MayBeRegExp( std::u16string_view rStr )
{
    if ( rStr.empty() || (rStr.size() == 1 && rStr[0] != '.') )
        return false;   // single meta characters can not be a regexp
    // First two characters are wildcard '?' and '*' characters.
    std::u16string_view cre(u"?*+.[]^$\\<>()|");
    return rStr.find_first_of(cre) != std::u16string_view::npos;
}

static bool MayBeWildcard( std::u16string_view rStr )
{
    // Wildcards with '~' escape, if there are no wildcards then an escaped
    // character does not make sense, but it modifies the search pattern in an
    // Excel compatible wildcard search...
    std::u16string_view cw(u"*?~");
    return rStr.find_first_of(cw) != std::u16string_view::npos;
}

utl::SearchParam::SearchType ScInterpreter::DetectSearchType( std::u16string_view rStr, const ScDocument& rDoc )
{
    const auto eType = rDoc.GetDocOptions().GetFormulaSearchType();
    if ((eType == utl::SearchParam::SearchType::Wildcard && MayBeWildcard(rStr))
        || (eType == utl::SearchParam::SearchType::Regexp && MayBeRegExp(rStr)))
        return eType;
    return utl::SearchParam::SearchType::Normal;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
