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

#include <algorithm>
#include <memory>
#include <interpre.hxx>

#include <o3tl/float_int_conversion.hxx>
#include <o3tl/string_view.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/linkmgr.hxx>
#include <sfx2/objsh.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>
#include <sal/macros.h>
#include <osl/diagnose.h>

#include <sc.hrc>
#include <ddelink.hxx>
#include <scmatrix.hxx>
#include <formulacell.hxx>
#include <document.hxx>
#include <dociter.hxx>
#include <docsh.hxx>
#include <unitconv.hxx>
#include <hints.hxx>
#include <dpobject.hxx>
#include <tokenarray.hxx>
#include <globalnames.hxx>
#include <stlpool.hxx>
#include <stlsheet.hxx>
#include <dpcache.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/DateTimeWeek.hxx>
#include <spreadsheetengine/runtime/DateTimeWorkday.hxx>
#include <spreadsheetengine/runtime/MathFinancial.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <spreadsheetengine/runtime/MathScalar.hxx>
#include <spreadsheetengine/runtime/NumeralConversion.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx>

#include <com/sun/star/sheet/DataPilotFieldFilter.hpp>

#include <string.h>

using namespace com::sun::star;
using namespace formula;
namespace sedatetime = spreadsheetengine::core::datetime;
namespace semath = spreadsheetengine::core::math;
namespace seconvert = spreadsheetengine::core::convert;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;

#define SCdEpsilon                1.0E-7

namespace
{

}

// Date and Time

double ScInterpreter::GetDateSerial( sal_Int16 nYear, sal_Int16 nMonth, sal_Int16 nDay,
        bool bStrict )
{
    if ( nYear < 100 && !bStrict )
        nYear = mrContext.NFExpandTwoDigitYear( nYear );
    if (std::optional<double> fSerial = sedatetime::makeDateSerial(
            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), nYear, nMonth, nDay,
            bStrict))
        return *fSerial;

    SetError(FormulaError::NoValue);
    return 0;
}

FormulaError ScInterpreter::GetWeekendAndHolidayMasks(
    const sal_uInt8 nParamCount, const sal_Int32 nNullDate, std::vector< double >& rSortArray,
    bool bWeekendMask[ 7 ] )
{
    if ( nParamCount == 4 )
    {
        std::vector< double > nWeekendDays;
        GetNumberSequenceArray( 1, nWeekendDays, false );
        if ( nGlobalError != FormulaError::NONE )
            return nGlobalError;
        auto aWeekendMask = selibreoffice::toApiWeekendMask(bWeekendMask);
        if (!sedatetime::applyWeekendMaskSequence(nWeekendDays, aWeekendMask))
            return FormulaError::IllegalArgument;
        selibreoffice::toLibreOfficeWeekendMask(aWeekendMask, bWeekendMask);
    }
    else
    {
        auto aWeekendMask = selibreoffice::toApiWeekendMask(bWeekendMask);
        sedatetime::setDefaultWeekendMask(aWeekendMask);
        selibreoffice::toLibreOfficeWeekendMask(aWeekendMask, bWeekendMask);
    }

    if ( nParamCount >= 3 )
    {
        GetSortArray( 1, rSortArray, nullptr, true, true );
        size_t nMax = rSortArray.size();
        for ( size_t i = 0; i < nMax; i++ )
            rSortArray.at( i ) = ::rtl::math::approxFloor( rSortArray.at( i ) ) + nNullDate;
    }

    return nGlobalError;
}

FormulaError ScInterpreter::GetWeekendAndHolidayMasks_MS(
    const sal_uInt8 nParamCount, const sal_Int32 nNullDate, std::vector< double >& rSortArray,
    bool bWeekendMask[ 7 ], bool bWorkdayFunction )
{
    FormulaError nErr = FormulaError::NONE;
    OUString aWeekendDays;
    if ( nParamCount == 4 )
    {
        GetSortArray( 1, rSortArray, nullptr, true, true );
        size_t nMax = rSortArray.size();
        for ( size_t i = 0; i < nMax; i++ )
            rSortArray.at( i ) = ::rtl::math::approxFloor( rSortArray.at( i ) ) + nNullDate;
    }

    if ( nParamCount >= 3 )
    {
        if ( IsMissing() )
            Pop();
        else
        {
            switch ( GetStackType() )
            {
                case svDoubleRef :
                case svExternalDoubleRef :
                    return FormulaError::NoValue;

                default :
                    {
                        double fDouble;
                        svl::SharedString aSharedString;
                        bool bDouble = GetDoubleOrString( fDouble, aSharedString);
                        if ( bDouble )
                        {
                            if ( fDouble >= 1.0 && fDouble <= 17 )
                                aWeekendDays = OUString::number( fDouble );
                            else
                                return FormulaError::NoValue;
                        }
                        else
                        {
                            if ( aSharedString.isEmpty() || aSharedString.getLength() != 7 ||
                                 ( bWorkdayFunction && aSharedString.getString() == "1111111" ) )
                                return FormulaError::NoValue;
                            else
                                aWeekendDays = aSharedString.getString();
                        }
                    }
                    break;
            }
        }
    }

    if ( aWeekendDays.isEmpty() )
    {
        auto aWeekendMask = selibreoffice::toApiWeekendMask(bWeekendMask);
        sedatetime::setDefaultWeekendMask(aWeekendMask);
        selibreoffice::toLibreOfficeWeekendMask(aWeekendMask, bWeekendMask);
    }
    else
    {
        auto aWeekendMask = selibreoffice::toApiWeekendMask(bWeekendMask);
        if (!sedatetime::applyWeekendMaskMsSpec(
                selibreoffice::toApiString(aWeekendDays), bWorkdayFunction, aWeekendMask))
            nErr = FormulaError::IllegalArgument;
        selibreoffice::toLibreOfficeWeekendMask(aWeekendMask, bWeekendMask);
    }
    return nErr;
}

void ScInterpreter::RoundNumber( rtl_math_RoundingMode eMode )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    double fVal = 0.0;
    if (nParamCount == 1)
        fVal = ::rtl::math::round( GetDouble(), 0, eMode );
    else
    {
        const sal_Int16 nDec = GetInt16();
        const double fX = GetDouble();
        if (nGlobalError == FormulaError::NONE)
            fVal = semath::roundToDecimals( fX, nDec, eMode );
    }
    PushDouble(fVal);
}

void ScInterpreter::RoundSignificant( double fX, double fDigits, double &fRes )
{
    fRes = semath::roundToSignificantDigits( fX, fDigits );
}

/** tdf69552 ODFF1.2 function CEILING and Excel function CEILING.MATH
    In essence, the difference between the two is that ODFF-CEILING needs to
    have arguments value and significance of the same sign and with
    CEILING.MATH the sign of argument significance is irrevelevant.
    This is why ODFF-CEILING is exported to Excel as CEILING.MATH and
    CEILING.MATH is imported in Calc as CEILING.MATH
 */
/** tdf69552 ODFF1.2 function FLOOR and Excel function FLOOR.MATH
    In essence, the difference between the two is that ODFF-FLOOR needs to
    have arguments value and significance of the same sign and with
    FLOOR.MATH the sign of argument significance is irrevelevant.
    This is why ODFF-FLOOR is exported to Excel as FLOOR.MATH and
    FLOOR.MATH is imported in Calc as FLOOR.MATH
 */
// financial functions
double ScInterpreter::ScGetPV(double fRate, double fNper, double fPmt,
                              double fFv, bool bPayInAdvance)
{
    return semath::computePresentValue(fRate, fNper, fPmt, fFv, bPayInAdvance);
}

double ScInterpreter::ScGetDDB(double fCost, double fSalvage, double fLife,
                double fPeriod, double fFactor)
{
    return semath::computeDoubleDecliningBalance(fCost, fSalvage, fLife, fPeriod, fFactor);
}

double ScInterpreter::ScInterVDB(double fCost, double fSalvage, double fLife,
                             double fLife1, double fPeriod, double fFactor)
{
    return semath::computeVariableDecliningBalanceSegment(
        fCost, fSalvage, fLife, fLife1, fPeriod, fFactor);
}

double ScInterpreter::ScGetPMT(double fRate, double fNper, double fPv,
                       double fFv, bool bPayInAdvance)
{
    return semath::computePayment(fRate, fNper, fPv, fFv, bPayInAdvance);
}

double ScInterpreter::ScGetFV(double fRate, double fNper, double fPmt,
                              double fPv, bool bPayInAdvance)
{
    return semath::computeFutureValue(fRate, fNper, fPmt, fPv, bPayInAdvance);
}

double ScInterpreter::ScGetIpmt(double fRate, double fPer, double fNper, double fPv,
                                 double fFv, bool bPayInAdvance, double& fPmt)
{
    const semath::FinancialInterestPayment aResult = semath::computeInterestPayment(
        fRate, fPer, fNper, fPv, fFv, bPayInAdvance);
    fPmt = aResult.mfPayment;
    nFuncFmtType = SvNumFormatType::CURRENCY;
    return aResult.mfInterest;
}

static ScDdeLink* lcl_GetDdeLink( const sfx2::LinkManager* pLinkMgr,
                                std::u16string_view rA, std::u16string_view rT, std::u16string_view rI, sal_uInt8 nM )
{
    size_t nCount = pLinkMgr->GetLinks().size();
    for (size_t i=0; i<nCount; i++ )
    {
        ::sfx2::SvBaseLink* pBase = pLinkMgr->GetLinks()[i].get();
        if (ScDdeLink* pLink = dynamic_cast<ScDdeLink*>(pBase))
        {
            if ( pLink->GetAppl() == rA &&
                 pLink->GetTopic() == rT &&
                 pLink->GetItem() == rI &&
                 pLink->GetMode() == nM )
                return pLink;
        }
    }

    return nullptr;
}

void ScInterpreter::ScDde()
{
    //  application, file, scope
    //  application, Topic, Item

    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 4 ) )
        return;

    sal_uInt8 nMode = SC_DDE_DEFAULT;
    if (nParamCount == 4)
    {
        sal_Int32 nTmp = GetInt32();
        if (nGlobalError != FormulaError::NONE || nTmp < 0 || nTmp > SAL_MAX_UINT8)
        {
            PushIllegalArgument();
            return;
        }
        nMode = static_cast<sal_uInt8>(nTmp);
    }
    OUString aItem  = GetString().getString();
    OUString aTopic = GetString().getString();
    OUString aAppl  = GetString().getString();

    if (nMode > SC_DDE_TEXT)
        nMode = SC_DDE_DEFAULT;

    //  temporary documents (ScFunctionAccess) have no DocShell
    //  and no LinkManager -> abort

    //sfx2::LinkManager* pLinkMgr = mrDoc.GetLinkManager();
    if (!mpLinkManager)
    {
        PushNoValue();
        return;
    }

    // Need to reinterpret after loading (build links)
    pArr->AddRecalcMode( ScRecalcMode::ONLOAD_LENIENT );

        //  while the link is not evaluated, idle must be disabled (to avoid circular references)

    bool bOldEnabled = mrDoc.IsIdleEnabled();
    mrDoc.EnableIdle(false);

        // Get/ Create link object

    ScDdeLink* pLink = lcl_GetDdeLink( mpLinkManager, aAppl, aTopic, aItem, nMode );

    //TODO: Save Dde-links (in addition) more efficient at document !!!!!
    //      ScDdeLink* pLink = mrDoc.GetDdeLink( aAppl, aTopic, aItem );

    bool bWasError = ( pMyFormulaCell && pMyFormulaCell->GetRawError() != FormulaError::NONE );

    if (!pLink)
    {
        pLink = new ScDdeLink( mrDoc, aAppl, aTopic, aItem, nMode );
        mpLinkManager->InsertDDELink( pLink, aAppl, aTopic, aItem );
        if ( mpLinkManager->GetLinks().size() == 1 )                    // the first one?
        {
            SfxBindings* pBindings = mrDoc.GetViewBindings();
            if (pBindings)
                pBindings->Invalidate( SID_LINKS );             // Link-Manager enabled
        }

        //if the document was just loaded, but the ScDdeLink entry was missing, then
        //don't update this link until the links are updated in response to the users
        //decision
        if (!mrDoc.HasLinkFormulaNeedingCheck())
        {
                                //TODO: evaluate asynchron ???
            pLink->TryUpdate(); //  TryUpdate doesn't call Update multiple times
        }

        if (pMyFormulaCell)
        {
            // StartListening after the Update to avoid circular references
            pMyFormulaCell->StartListening( *pLink );
        }
    }
    else
    {
        if (pMyFormulaCell)
            pMyFormulaCell->StartListening( *pLink );
    }

    //  If a new Error from Reschedule appears when the link is executed then reset the errorflag


    if ( pMyFormulaCell && pMyFormulaCell->GetRawError() != FormulaError::NONE && !bWasError )
        pMyFormulaCell->SetErrCode(FormulaError::NONE);

        //  check the value

    const ScMatrix* pLinkMat = pLink->GetResult();
    if (pLinkMat)
    {
        SCSIZE nC, nR;
        pLinkMat->GetDimensions(nC, nR);
        ScMatrixRef pNewMat = GetNewMat( nC, nR, /*bEmpty*/true);
        if (pNewMat)
        {
            pLinkMat->MatCopy(*pNewMat);        // copy
            PushMatrix( pNewMat );
        }
        else
            PushIllegalArgument();
    }
    else
        PushNA();

    mrDoc.EnableIdle(bOldEnabled);
    mpLinkManager->CloseCachedComps();
}

void ScInterpreter::ScHyperLink()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    double fVal = 0.0;
    svl::SharedString aStr;
    ScMatValType nResultType = ScMatValType::String;

    if ( nParamCount == 2 )
    {
        switch ( GetStackType() )
        {
            case svDouble:
                fVal = GetDouble();
                nResultType = ScMatValType::Value;
            break;
            case svString:
                aStr = GetString();
            break;
            case svSingleRef:
            case svDoubleRef:
            {
                ScAddress aAdr;
                if ( !PopDoubleRefOrSingleRef( aAdr ) )
                    break;

                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasEmptyValue())
                    nResultType = ScMatValType::Empty;
                else
                {
                    FormulaError nErr = GetCellErrCode(aCell);
                    if (nErr != FormulaError::NONE)
                        SetError( nErr);
                    else if (aCell.hasNumeric())
                    {
                        fVal = GetCellValue(aAdr, aCell);
                        nResultType = ScMatValType::Value;
                    }
                    else
                        GetCellString(aStr, aCell);
                }
            }
            break;
            case svMatrix:
                nResultType = GetDoubleOrStringFromMatrix( fVal, aStr);
            break;
            case svMissing:
            case svEmptyCell:
                Pop();
                // mimic xcl
                fVal = 0.0;
                nResultType = ScMatValType::Value;
            break;
            default:
                PopError();
                SetError( FormulaError::IllegalArgument);
        }
    }
    svl::SharedString aUrl = GetString();
    ScMatrixRef pResMat = GetNewMat( 1, 2);
    if (nGlobalError != FormulaError::NONE)
    {
        fVal = CreateDoubleError( nGlobalError);
        nResultType = ScMatValType::Value;
    }
    if (nParamCount == 2 || nGlobalError != FormulaError::NONE)
    {
        if (ScMatrix::IsValueType( nResultType))
            pResMat->PutDouble( fVal, 0);
        else if (ScMatrix::IsRealStringType( nResultType))
            pResMat->PutString(aStr, 0);
        else    // EmptyType, EmptyPathType, mimic xcl
            pResMat->PutDouble( 0.0, 0 );
    }
    else
        pResMat->PutString(aUrl, 0);
    pResMat->PutString(aUrl, 1);
    bMatrixFormula = true;
    PushMatrix(pResMat);
}

void ScInterpreter::ScGetPivotData()
{
    sal_uInt8 nParamCount = GetByte();

    if (!MustHaveParamCountMin(nParamCount, 2) || (nParamCount % 2) == 1)
    {
        PushError(FormulaError::NoRef);
        return;
    }

    bool bOldSyntax = false;
    if (nParamCount == 2)
    {
        // if the first parameter is a ref, assume old syntax
        StackVar eFirstType = GetStackType(2);
        if (eFirstType == svSingleRef || eFirstType == svDoubleRef)
            bOldSyntax = true;
    }

    std::vector<sheet::DataPilotFieldFilter> aFilters;
    OUString aDataFieldName;
    ScRange aBlock;

    if (bOldSyntax)
    {
        aDataFieldName = GetString().getString();

        switch (GetStackType())
        {
            case svDoubleRef :
                PopDoubleRef(aBlock);
            break;
            case svSingleRef :
            {
                ScAddress aAddr;
                PopSingleRef(aAddr);
                aBlock = aAddr;
            }
            break;
            default:
                PushError(FormulaError::NoRef);
                return;
        }
    }
    else
    {
        // Standard syntax: separate name/value pairs

        sal_uInt16 nFilterCount = nParamCount / 2 - 1;
        aFilters.resize(nFilterCount);

        sal_uInt16 i = nFilterCount;
        while (i > 0)
        {
            --i;
            /* TODO: also, in case of numeric the entire filter match should
             * not be on a (even if locale independent) formatted string down
             * below in pDPObj->GetPivotData(). */

            bool bEvaluateFormatIndex;
            switch (GetRawStackType())
            {
                case svSingleRef:
                case svDoubleRef:
                    bEvaluateFormatIndex = true;
                break;
                default:
                    bEvaluateFormatIndex = false;
            }

            double fDouble;
            svl::SharedString aSharedString;
            bool bDouble = GetDoubleOrString( fDouble, aSharedString);
            if (nGlobalError != FormulaError::NONE)
            {
                PushError( nGlobalError);
                return;
            }

            if (bDouble)
            {
                sal_uInt32 nNumFormat;
                if (bEvaluateFormatIndex && nCurFmtIndex)
                    nNumFormat = nCurFmtIndex;
                else
                {
                    if (nCurFmtType == SvNumFormatType::UNDEFINED)
                        nNumFormat = 0;
                    else
                        nNumFormat = mrContext.NFGetStandardFormat( nCurFmtType, ScGlobal::eLnge);
                }
                const Color* pColor;
                mrContext.NFGetOutputString( fDouble, nNumFormat, aFilters[i].MatchValueName, &pColor);
                aFilters[i].MatchValue = ScDPCache::GetLocaleIndependentFormattedString(
                        fDouble, mrContext, nNumFormat);
            }
            else
            {
                aFilters[i].MatchValueName = aSharedString.getString();

                // Parse possible number from MatchValueName and format
                // locale independent as MatchValue.
                sal_uInt32 nNumFormat = 0;
                double fValue;
                if (mrContext.NFIsNumberFormat( aFilters[i].MatchValueName, nNumFormat, fValue))
                    aFilters[i].MatchValue = ScDPCache::GetLocaleIndependentFormattedString(
                            fValue, mrContext, nNumFormat);
                else
                    aFilters[i].MatchValue = aFilters[i].MatchValueName;
            }

            aFilters[i].FieldName = GetString().getString();
        }

        switch (GetStackType())
        {
            case svDoubleRef :
                PopDoubleRef(aBlock);
            break;
            case svSingleRef :
            {
                ScAddress aAddr;
                PopSingleRef(aAddr);
                aBlock = aAddr;
            }
            break;
            default:
                PushError(FormulaError::NoRef);
                return;
        }

        aDataFieldName = GetString().getString(); // First parameter is data field name.
    }

    // Early bail-out, don't grind through data pilot cache and all.
    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;
    }

    // NOTE : MS Excel docs claim to use the 'most recent' which is not
    // exactly the same as what we do in ScDocument::GetDPAtBlock
    // However we do need to use GetDPABlock
    ScDPObject* pDPObj = mrDoc.GetDPAtBlock(aBlock);
    if (!pDPObj)
    {
        PushError(FormulaError::NoRef);
        return;
    }

    if (bOldSyntax)
    {
        OUString aFilterStr = aDataFieldName;
        std::vector<sal_Int16> aFilterFuncs;
        if (!pDPObj->ParseFilters(aDataFieldName, aFilters, aFilterFuncs, aFilterStr))
        {
            PushError(FormulaError::NoRef);
            return;
        }

        // TODO : For now, we ignore filter functions since we couldn't find a
        // live example of how they are supposed to be used. We'll support
        // this again once we come across a real-world example.
    }

    double fVal = pDPObj->GetPivotData(aDataFieldName, aFilters);
    if (std::isnan(fVal))
    {
        PushError(FormulaError::NoRef);
        return;
    }
    PushDouble(fVal);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
