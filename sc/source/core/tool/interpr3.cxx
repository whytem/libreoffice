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

#include <tools/solar.h>
#include <stdlib.h>

#include <interpre.hxx>
#include <global.hxx>
#include <document.hxx>
#include <dociter.hxx>
#include <matrixoperators.hxx>
#include <scmatrix.hxx>
#include <columniterator.hxx>
#include <unotools/collatorwrapper.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <spreadsheetengine/runtime/MathFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/InterpreterCompatDispatch.hxx>

#include <cassert>
#include <cmath>
#include <memory>
#include <set>
#include <vector>
#include <algorithm>
#include <comphelper/random.hxx>
#include <o3tl/float_int_conversion.hxx>
#include <osl/diagnose.h>

using namespace formula;
namespace semath = spreadsheetengine::core::math;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;
namespace seinterpcompatdispatch
    = spreadsheetengine::compat::libreoffice::interpretercompatdispatch;

/// Two columns of data should be sortable with GetSortArray() and QuickSort()
// This is an arbitrary limit.
static size_t MAX_COUNT_DOUBLE_FOR_SORT(const ScSheetLimits& rSheetLimits)
{
    return rSheetLimits.GetMaxRowCount() * 2;
}

namespace {

FormulaError lcl_ToCalcMathFormulaError(spreadsheetengine::api::Error eError)
{
    // Preserve Calc's existing statistical-function contract for domain failures.
    if (eError == spreadsheetengine::api::Error::Domain)
        return FormulaError::IllegalArgument;
    return selibreoffice::toFormulaError(eError);
}

}

// General functions

//  #i26836# new gaussinv implementation by Martin Eitzenberger <m.eitzenberger@unix.net>

double ScInterpreter::gaussinv(double x)
{
    const auto aResult = semath::evaluateStandardNormalInverse(x);
    return aResult ? aResult.maValue : HUGE_VAL;
}

double ScInterpreter::GetFDist(double x, double fF1, double fF2)
{
    const auto aResult = semath::evaluateFRightTailDistribution(x, fF1, fF2);
    if (!aResult)
    {
        SetError(lcl_ToCalcMathFormulaError(aResult.meError));
        return HUGE_VAL;
    }
    return aResult.maValue;
}

double ScInterpreter::GetTDist( double T, double fDF, int nType )
{
    const auto aResult = semath::evaluateStudentDistribution(T, fDF, nType);
    if (!aResult)
    {
        SetError(lcl_ToCalcMathFormulaError(aResult.meError));
        return HUGE_VAL;
    }
    return aResult.maValue;
}

// for LEGACY.CHIDIST, returns right tail, fDF=degrees of freedom
/** You must ensure fDF>0.0 */
double ScInterpreter::GetChiDist(double fX, double fDF)
{
    const auto aResult = semath::evaluateLegacyChiDist(fX, fDF);
    if (!aResult)
    {
        SetError(lcl_ToCalcMathFormulaError(aResult.meError));
        return HUGE_VAL;
    }
    return aResult.maValue;
}

// ready for ODF 1.2
// for ODF CHISQDIST; cumulative distribution function, fDF=degrees of freedom
// returns left tail
/** You must ensure fDF>0.0 */
double ScInterpreter::GetChiSqDistCDF(double fX, double fDF)
{
    const auto aResult = semath::evaluateChiSquareDistribution(fX, fDF, true, false);
    return aResult ? aResult.maValue : HUGE_VAL;
}

double ScInterpreter::GetChiSqDistPDF(double fX, double fDF)
{
    const auto aResult = semath::evaluateChiSquareDistribution(fX, fDF, false, false);
    return aResult ? aResult.maValue : HUGE_VAL;
}


double ScInterpreter::GetBeta(double fAlpha, double fBeta)
{
    return semath::betaValue(fAlpha, fBeta);
}

// Same as GetBeta but with logarithm
double ScInterpreter::GetLogBeta(double fAlpha, double fBeta)
{
    return semath::logBetaValue(fAlpha, fBeta);
}

// cumulative distribution function, normalized
double ScInterpreter::GetBetaDist(double fXin, double fAlpha, double fBeta)
{
    return semath::betaCdf(fXin, fAlpha, fBeta);
}

double ScInterpreter::GetTInv( double fAlpha, double fSize, int nType )
{
    const auto aResult = semath::evaluateTInverse(fAlpha, fSize, nType);
    if (!aResult)
    {
        SetError(lcl_ToCalcMathFormulaError(aResult.meError));
        return HUGE_VAL;
    }
    return aResult.maValue;
}

bool ScInterpreter::CalculateTest(bool _bTemplin
                                  ,const SCSIZE nC1, const SCSIZE nC2,const SCSIZE nR1,const SCSIZE nR2
                                  ,const ScMatrixRef& pMat1,const ScMatrixRef& pMat2
                                  ,double& fT,double& fF)
{
    double fCount1    = 0.0;
    double fCount2    = 0.0;
    KahanSum fSum1    = 0.0;
    KahanSum fSumSqr1 = 0.0;
    KahanSum fSum2    = 0.0;
    KahanSum fSumSqr2 = 0.0;
    double fVal;
    SCSIZE i,j;
    for (i = 0; i < nC1; i++)
        for (j = 0; j < nR1; j++)
        {
            if (!pMat1->IsStringOrEmpty(i,j))
            {
                fVal = pMat1->GetDouble(i,j);
                fSum1    += fVal;
                fSumSqr1 += fVal * fVal;
                fCount1++;
            }
        }
    for (i = 0; i < nC2; i++)
        for (j = 0; j < nR2; j++)
        {
            if (!pMat2->IsStringOrEmpty(i,j))
            {
                fVal = pMat2->GetDouble(i,j);
                fSum2    += fVal;
                fSumSqr2 += fVal * fVal;
                fCount2++;
            }
        }
    if (fCount1 < 2.0 || fCount2 < 2.0)
    {
        PushNoValue();
        return false;
    } // if (fCount1 < 2.0 || fCount2 < 2.0)
    if ( _bTemplin )
    {
        double fS1 = (fSumSqr1-fSum1*fSum1/fCount1).get() / (fCount1-1.0) / fCount1;
        double fS2 = (fSumSqr2-fSum2*fSum2/fCount2).get() / (fCount2-1.0) / fCount2;
        if (fS1 + fS2 == 0.0)
        {
            PushNoValue();
            return false;
        }
        fT = std::abs(( fSum1/fCount1 - fSum2/fCount2 ).get())/sqrt(fS1+fS2);
        double c = fS1/(fS1+fS2);
    //  GetTDist is calculated via GetBetaDist and also works with non-integral
    // degrees of freedom. The result matches Excel
        fF = 1.0/(c*c/(fCount1-1.0)+(1.0-c)*(1.0-c)/(fCount2-1.0));
    }
    else
    {
        //  according to Bronstein-Semendjajew
        double fS1 = (fSumSqr1 - fSum1*fSum1/fCount1).get() / (fCount1 - 1.0);    // Variance
        double fS2 = (fSumSqr2 - fSum2*fSum2/fCount2).get() / (fCount2 - 1.0);
        fT = std::abs( fSum1.get()/fCount1 - fSum2.get()/fCount2 ) /
             sqrt( (fCount1-1.0)*fS1 + (fCount2-1.0)*fS2 ) *
             sqrt( fCount1*fCount2*(fCount1+fCount2-2)/(fCount1+fCount2) );
        fF = fCount1 + fCount2 - 2;
    }
    return true;
}
bool ScInterpreter::CalculateSkew(KahanSum& fSum, double& fCount, std::vector<double>& values)
{
    short nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 )  )
        return false;

    fSum   = 0.0;
    fCount = 0.0;
    double fVal = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
            {
                fVal = GetDouble();
                fSum += fVal;
                values.push_back(fVal);
                fCount++;
            }
                break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasNumeric())
                {
                    fVal = GetCellValue(aAdr, aCell);
                    fSum += fVal;
                    values.push_back(fVal);
                    fCount++;
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                PopDoubleRef( aRange, nParamCount, nRefInList);
                FormulaError nErr = FormulaError::NONE;
                ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags );
                if (aValIter.GetFirst(fVal, nErr))
                {
                    fSum += fVal;
                    values.push_back(fVal);
                    fCount++;
                    SetError(nErr);
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(fVal, nErr))
                    {
                        fSum += fVal;
                        values.push_back(fVal);
                        fCount++;
                    }
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
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
                            fVal = pMat->GetDouble(nElem);
                            fSum += fVal;
                            values.push_back(fVal);
                            fCount++;
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            if (!pMat->IsStringOrEmpty(nElem))
                            {
                                fVal = pMat->GetDouble(nElem);
                                fSum += fVal;
                                values.push_back(fVal);
                                fCount++;
                            }
                    }
                }
            }
            break;
            default :
                SetError(FormulaError::IllegalParameter);
            break;
        }
    }

    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return false;
    } // if (nGlobalError != FormulaError::NONE)
    return true;
}

double ScInterpreter::GetPercentile(std::vector<double> & rArray, double fPercentile )
{
    semath::AggregateScan aScan;
    aScan.maNumbers = rArray;
    const auto aResult = semath::evaluateAggregateRankedNumbers(16, aScan, fPercentile);
    return aResult ? aResult.maValue : 0.0;
}

void ScInterpreter::CalculateSmallLarge(bool bSmall)
{
    if ( !MustHaveParamCount( GetByte(), 2 )  )
        return;

    SCSIZE nCol = 0, nRow = 0;
    const auto aArray = GetRankNumberArray(nCol, nRow);
    const size_t nRankArraySize = aArray.size();
    if (nRankArraySize == 0 || nGlobalError != FormulaError::NONE)
    {
        PushNoValue();
        return;
    }
    assert(nRankArraySize == nCol * nRow);

    std::vector<SCSIZE> aRankArray;
    aRankArray.reserve(nRankArraySize);
    std::transform(aArray.begin(), aArray.end(), std::back_inserter(aRankArray),
            [bSmall](double f) {
                f = (bSmall ? rtl::math::approxFloor(f) : rtl::math::approxCeil(f));
                // Valid ranks are >= 1.
                if (f < 1.0 || !o3tl::convertsToAtMost(f, std::numeric_limits<SCSIZE>::max()))
                    return static_cast<SCSIZE>(0);
                return static_cast<SCSIZE>(f);
            });

    std::vector<double> aSortArray;
    GetNumberSequenceArray(1, aSortArray, false );
    const SCSIZE nSize = aSortArray.size();
    if (nSize == 0 || nGlobalError != FormulaError::NONE)
        PushNoValue();
    else if (nRankArraySize == 1)
    {
        const SCSIZE k = aRankArray[0];
        if (k < 1 || nSize < k)
        {
            if (!std::isfinite(aArray[0]))
                PushDouble(aArray[0]);  // propagates error
            else
                PushNoValue();
        }
        else
        {
            std::vector<double>::iterator iPos = aSortArray.begin() + (bSmall ? k-1 : nSize-k);
            ::std::nth_element( aSortArray.begin(), iPos, aSortArray.end());
            PushDouble( *iPos);
        }
    }
    else
    {
        std::set<SCSIZE> aIndices;
        for (SCSIZE n : aRankArray)
        {
            if (1 <= n && n <= nSize)
                aIndices.insert(bSmall ? n-1 : nSize-n);
        }
        // We can spare sorting when the total number of ranks is small enough.
        // Find only the elements at given indices if, arbitrarily, the index size is
        // smaller than 1/3 of the haystack array's size; just sort it squarely, otherwise.
        if (aIndices.size() < nSize/3)
        {
            auto itBegin = aSortArray.begin();
            for (SCSIZE i : aIndices)
            {
                auto it = aSortArray.begin() + i;
                std::nth_element(itBegin, it, aSortArray.end());
                itBegin = ++it;
            }
        }
        else
            std::sort(aSortArray.begin(), aSortArray.end());

        std::vector<double> aResultArray;
        aResultArray.reserve(nRankArraySize);
        for (size_t i = 0; i < nRankArraySize; ++i)
        {
            const SCSIZE n = aRankArray[i];
            if (1 <= n && n <= nSize)
                aResultArray.push_back( aSortArray[bSmall ? n-1 : nSize-n]);
            else if (!std::isfinite( aArray[i]))
                aResultArray.push_back( aArray[i]);  // propagate error
            else
                aResultArray.push_back( CreateDoubleError( FormulaError::IllegalArgument));
        }
        ScMatrixRef pResult = GetNewMat(nCol, nRow, aResultArray);
        PushMatrix(pResult);
    }
}

double ScInterpreter::GetPercentrank(std::vector<double> & rArray, double fVal, bool bInclusive )
{
    SCSIZE nSize = rArray.size();
    double fRes;
    if ( fVal == rArray[ 0 ] )
    {
        if ( bInclusive )
            fRes = 0.0;
        else
            fRes = 1.0 / static_cast<double>( nSize + 1 );
    }
    else
    {
        SCSIZE nOldCount = 0;
        double fOldVal = rArray[ 0 ];
        SCSIZE i;
        for ( i = 1; i < nSize && rArray[ i ] < fVal; i++ )
        {
            if ( rArray[ i ] != fOldVal )
            {
                nOldCount = i;
                fOldVal = rArray[ i ];
            }
        }
        if ( rArray[ i ] != fOldVal )
            nOldCount = i;
        if ( fVal == rArray[ i ] )
        {
            if ( bInclusive )
                fRes = div( nOldCount, nSize - 1 );
            else
                fRes = static_cast<double>( i + 1 ) / static_cast<double>( nSize + 1 );
        }
        else
        {
            //  nOldCount is the count of smaller entries
            //  fVal is between rArray[ nOldCount - 1 ] and rArray[ nOldCount ]
            //  use linear interpolation to find a position between the entries
            if ( nOldCount == 0 )
            {
                OSL_FAIL( "should not happen" );
                fRes = 0.0;
            }
            else
            {
                double fFract = ( fVal - rArray[ nOldCount - 1 ] ) /
                    ( rArray[ nOldCount ] - rArray[ nOldCount - 1 ] );
                if ( bInclusive )
                    fRes = div( static_cast<double>( nOldCount - 1 ) + fFract, nSize - 1 );
                else
                    fRes = ( static_cast<double>(nOldCount) + fFract ) / static_cast<double>( nSize + 1 );
            }
        }
    }
    return fRes;
}

std::vector<double> ScInterpreter::GetRankNumberArray( SCSIZE& rCol, SCSIZE& rRow )
{
    std::vector<double> aArray;
    switch (GetStackType())
    {
        case svDouble:
            aArray.push_back(PopDouble());
            rCol = rRow = 1;
        break;
        case svSingleRef:
        {
            ScAddress aAdr;
            PopSingleRef(aAdr);
            ScRefCellValue aCell(mrDoc, aAdr);
            if (aCell.hasNumeric())
            {
                aArray.push_back(GetCellValue(aAdr, aCell));
                rCol = rRow = 1;
            }
        }
        break;
        case svDoubleRef:
        {
            ScRange aRange;
            PopDoubleRef(aRange, true);
            if (nGlobalError != FormulaError::NONE)
                break;

            // give up unless the start and end are in the same sheet
            if (aRange.aStart.Tab() != aRange.aEnd.Tab())
            {
                SetError(FormulaError::IllegalParameter);
                break;
            }

            // the range already is in order
            assert(aRange.aStart.Col() <= aRange.aEnd.Col());
            assert(aRange.aStart.Row() <= aRange.aEnd.Row());
            rCol = aRange.aEnd.Col() - aRange.aStart.Col() + 1;
            rRow = aRange.aEnd.Row() - aRange.aStart.Row() + 1;
            aArray.reserve(rCol * rRow);

            FormulaError nErr = FormulaError::NONE;
            double fCellVal;
            ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
            if (aValIter.GetFirst(fCellVal, nErr))
            {
                do
                    aArray.push_back(fCellVal);
                while (aValIter.GetNext(fCellVal, nErr) && nErr == FormulaError::NONE);
            }
            // Note that SMALL() and LARGE() rank parameters (2nd) have
            // ParamClass::Value, so in array mode this is never hit and
            // argument was converted to matrix instead, but for normal
            // evaluation any non-numeric value including empty cell will
            // result in error anyway, so just clear and propagate an existing
            // error here already.
            if (aArray.size() != rCol * rRow)
            {
                aArray.clear();
                SetError(nErr);
            }
        }
        break;
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
        {
            ScMatrixRef pMat = GetMatrix();
            if (!pMat)
                break;

            const SCSIZE nCount = pMat->GetElementCount();
            aArray.reserve(nCount);
            // Do not propagate errors from matrix elements as global error.
            pMat->SetErrorInterpreter(nullptr);
            if (pMat->IsNumeric())
            {
                for (SCSIZE i = 0; i < nCount; ++i)
                    aArray.push_back(pMat->GetDouble(i));
            }
            else
            {
                for (SCSIZE i = 0; i < nCount; ++i)
                {
                    if (pMat->IsValue(i))
                        aArray.push_back( pMat->GetDouble(i));
                    else
                        aArray.push_back( CreateDoubleError( FormulaError::NoValue));
                }
            }
            pMat->GetDimensions(rCol, rRow);
        }
        break;
        default:
            PopError();
            SetError(FormulaError::IllegalParameter);
        break;
    }
    return aArray;
}

void ScInterpreter::GetNumberSequenceArray( sal_uInt8 nParamCount, std::vector<double>& rArray, bool bConvertTextInArray )
{
    ScAddress aAdr;
    ScRange aRange;
    const bool bIgnoreErrVal = bool(mnSubTotalFlags & SubtotalFlags::IgnoreErrVal);
    short nParam = nParamCount;
    size_t nRefInList = 0;
    ReverseStack( nParamCount );
    while (nParam-- > 0)
    {
        const StackVar eStackType = GetStackType();
        switch (eStackType)
        {
            case svDouble :
                rArray.push_back( PopDouble());
            break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (bIgnoreErrVal && aCell.hasError())
                    ;   // nothing
                else if (aCell.hasNumeric())
                    rArray.push_back(GetCellValue(aAdr, aCell));
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                PopDoubleRef( aRange, nParam, nRefInList);
                if (nGlobalError != FormulaError::NONE)
                    break;

                aRange.PutInOrder();
                SCSIZE nCellCount = aRange.aEnd.Col() - aRange.aStart.Col() + 1;
                nCellCount *= aRange.aEnd.Row() - aRange.aStart.Row() + 1;
                rArray.reserve( rArray.size() + nCellCount);

                FormulaError nErr = FormulaError::NONE;
                double fCellVal;
                ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags );
                if (aValIter.GetFirst( fCellVal, nErr))
                {
                    if (bIgnoreErrVal)
                    {
                        if (nErr == FormulaError::NONE)
                            rArray.push_back( fCellVal);
                        while (aValIter.GetNext( fCellVal, nErr))
                        {
                            if (nErr == FormulaError::NONE)
                                rArray.push_back( fCellVal);
                        }
                    }
                    else
                    {
                        rArray.push_back( fCellVal);
                        SetError(nErr);
                        while ((nErr == FormulaError::NONE) && aValIter.GetNext( fCellVal, nErr))
                            rArray.push_back( fCellVal);
                        SetError(nErr);
                    }
                }
            }
            break;
            case svMatrix :
            case svExternalSingleRef:
            case svExternalDoubleRef:
            {
                ScMatrixRef pMat = GetMatrix();
                if (!pMat)
                    break;

                SCSIZE nCount = pMat->GetElementCount();
                rArray.reserve( rArray.size() + nCount);
                if (pMat->IsNumeric())
                {
                    if (bIgnoreErrVal)
                    {
                        for (SCSIZE i = 0; i < nCount; ++i)
                        {
                            const double fVal = pMat->GetDouble(i);
                            if (nGlobalError == FormulaError::NONE)
                                rArray.push_back( fVal);
                            else
                                nGlobalError = FormulaError::NONE;
                        }
                    }
                    else
                    {
                        for (SCSIZE i = 0; i < nCount; ++i)
                            rArray.push_back( pMat->GetDouble(i));
                    }
                }
                else if (bConvertTextInArray && eStackType == svMatrix)
                {
                    for (SCSIZE i = 0; i < nCount; ++i)
                    {
                        if ( pMat->IsValue( i ) )
                        {
                            if (bIgnoreErrVal)
                            {
                                const double fVal = pMat->GetDouble(i);
                                if (nGlobalError == FormulaError::NONE)
                                    rArray.push_back( fVal);
                                else
                                    nGlobalError = FormulaError::NONE;
                            }
                            else
                                rArray.push_back( pMat->GetDouble(i));
                        }
                        else
                        {
                            // tdf#88547 try to convert string to (date)value
                            OUString aStr = pMat->GetString( i ).getString();
                            if ( aStr.getLength() > 0 )
                            {
                                FormulaError nErr = nGlobalError;
                                nGlobalError = FormulaError::NONE;
                                double fVal = ConvertStringToValue( aStr );
                                if ( nGlobalError == FormulaError::NONE )
                                {
                                    rArray.push_back( fVal );
                                    nGlobalError = nErr;
                                }
                                else
                                {
                                    if (!bIgnoreErrVal)
                                        rArray.push_back( CreateDoubleError( FormulaError::NoValue));
                                    // Propagate previous error if any, else
                                    // the current #VALUE! error, unless
                                    // ignoring error values.
                                    if (nErr != FormulaError::NONE)
                                        nGlobalError = nErr;
                                    else if (!bIgnoreErrVal)
                                        nGlobalError = FormulaError::NoValue;
                                    else
                                        nGlobalError = FormulaError::NONE;
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (bIgnoreErrVal)
                    {
                        for (SCSIZE i = 0; i < nCount; ++i)
                        {
                            if (pMat->IsValue(i))
                            {
                                const double fVal = pMat->GetDouble(i);
                                if (nGlobalError == FormulaError::NONE)
                                    rArray.push_back( fVal);
                                else
                                    nGlobalError = FormulaError::NONE;
                            }
                        }
                    }
                    else
                    {
                        for (SCSIZE i = 0; i < nCount; ++i)
                        {
                            if (pMat->IsValue(i))
                                rArray.push_back( pMat->GetDouble(i));
                        }
                    }
                }
            }
            break;
            default :
                PopError();
                SetError( FormulaError::IllegalParameter);
            break;
        }
        if (nGlobalError != FormulaError::NONE)
            break;  // while
    }
    // nParam > 0 in case of error, clean stack environment and obtain earlier
    // error if there was one.
    while (nParam-- > 0)
        PopError();
}

void ScInterpreter::DecoladeRow( ScSortInfoArray* pArray, SCROW nRow1, SCROW nRow2 )
{
    SCROW nRow;
    int nMax = nRow2 - nRow1;
    for (SCROW i = nRow1; (i + 4) <= nRow2; i += 4)
    {
        nRow = comphelper::rng::uniform_int_distribution(0, nMax - 1);
        pArray->Swap(i, nRow1 + nRow);
    }
}

std::unique_ptr<ScSortInfoArray> ScInterpreter::CreateFastSortInfoArray(
    const ScSortParam& rSortParam, bool bMatrix, SCCOLROW nInd1, SCCOLROW nInd2 )
{
    sal_uInt16 nUsedSorts = 1;
    while (nUsedSorts < rSortParam.GetSortKeyCount() && rSortParam.maKeyState[nUsedSorts].bDoSort)
        nUsedSorts++;
    std::unique_ptr<ScSortInfoArray> pArray(new ScSortInfoArray(nUsedSorts, nInd1, nInd2));

    if (rSortParam.bByRow)
    {
        for (sal_uInt16 nSort = 0; nSort < nUsedSorts; nSort++)
        {
            if (!bMatrix)
            {
                SCCOL nCol = static_cast<SCCOL>(rSortParam.maKeyState[nSort].nField);
                std::optional<sc::ColumnIterator> pIter = mrDoc.GetColumnIterator(rSortParam.nSourceTab, nCol, nInd1, nInd2);
                assert(pIter->hasCell());

                for (SCROW nRow = nInd1; nRow <= nInd2; nRow++, pIter->next())
                {
                    ScSortInfo& rInfo = pArray->Get(nSort, nRow);
                    rInfo.maCell = pIter->getCell();
                    rInfo.nOrg = nRow;
                }
            }
            else
            {
                for (SCROW nRow = nInd1; nRow <= nInd2; nRow++)
                {
                    ScSortInfo& rInfo = pArray->Get(nSort, nRow);
                    rInfo.nOrg = nRow;
                }
            }
        }
    }
    else
    {
        for (sal_uInt16 nSort = 0; nSort < nUsedSorts; nSort++)
        {
            if (!bMatrix)
            {
                SCROW nRow = rSortParam.maKeyState[nSort].nField;
                for (SCCOL nCol = static_cast<SCCOL>(nInd1);
                    nCol <= static_cast<SCCOL>(nInd2); nCol++)
                {
                    ScSortInfo& rInfo = pArray->Get(nSort, nCol);
                    rInfo.maCell = mrDoc.GetRefCellValue(ScAddress(nCol, nRow, rSortParam.nSourceTab));
                    rInfo.nOrg = nCol;
                }
            }
            else
            {
                for (SCCOL nCol = static_cast<SCCOL>(nInd1);
                    nCol <= static_cast<SCCOL>(nInd2); nCol++)
                {
                    ScSortInfo& rInfo = pArray->Get(nSort, nCol);
                    rInfo.nOrg = nCol;
                }
            }
        }
    }
    return pArray;
}

std::vector<SCCOLROW> ScInterpreter::GetSortOrder( const ScSortParam& rSortParam, const ScMatrixRef& pMatSrc )
{
    std::vector<SCCOLROW> aOrderIndices;
    aSortParam = rSortParam;
    if (rSortParam.bByRow)
    {
        const SCROW nLastRow = rSortParam.nRow2;
        const SCROW nRow1 = (rSortParam.bHasHeader ? rSortParam.nRow1 + 1 : rSortParam.nRow1);
        if (nRow1 < nLastRow)
        {
            std::unique_ptr<ScSortInfoArray> pArray(CreateFastSortInfoArray(
                aSortParam, (pMatSrc != nullptr), nRow1, nLastRow));

            if (nLastRow - nRow1 > 255)
                DecoladeRow(pArray.get(), nRow1, nLastRow);

            QuickSort(pArray.get(), pMatSrc, nRow1, nLastRow);
            aOrderIndices = pArray->GetOrderIndices();
        }
    }
    else
    {
        const SCCOL nLastCol = rSortParam.nCol2;
        const SCCOL nCol1 = (rSortParam.bHasHeader ? rSortParam.nCol1 + 1 : rSortParam.nCol1);
        if (nCol1 < nLastCol)
        {
            std::unique_ptr<ScSortInfoArray> pArray(CreateFastSortInfoArray(
                aSortParam, (pMatSrc != nullptr), nCol1, nLastCol));

            QuickSort(pArray.get(), pMatSrc, nCol1, nLastCol);
            aOrderIndices = pArray->GetOrderIndices();
        }
    }
    return aOrderIndices;
}

ScMatrixRef ScInterpreter::CreateSortedMatrix( const ScSortParam& rSortParam, const ScMatrixRef& pMatSrc,
    const ScRange& rSourceRange, const std::vector<SCCOLROW>& rSortArray, SCSIZE nsC, SCSIZE nsR )
{
    SCCOLROW nStartPos = (!rSortParam.bByRow ? rSortParam.nCol1 : rSortParam.nRow1);
    size_t nCount = rSortArray.size();
    std::vector<SCCOLROW> aPosTable(nCount);

    for (size_t i = 0; i < nCount; ++i)
        aPosTable[rSortArray[i] - nStartPos] = i;

    ScMatrixRef pResMat = nullptr;
    if (!rSortArray.empty())
    {
        pResMat = GetNewMat(nsC, nsR, /*bEmpty*/true);
        if (!pMatSrc)
        {
            ScCellIterator aCellIter(mrDoc, rSourceRange);
            for (bool bHas = aCellIter.first(); bHas; bHas = aCellIter.next())
            {
                SCSIZE nThisCol = static_cast<SCSIZE>(aCellIter.GetPos().Col() - rSourceRange.aStart.Col());
                SCSIZE nThisRow = static_cast<SCSIZE>(aCellIter.GetPos().Row() - rSourceRange.aStart.Row());

                ScRefCellValue aCell = aCellIter.getRefCellValue();
                if (aCell.hasNumeric())
                {
                    if (rSortParam.bByRow)
                        pResMat->PutDouble(GetCellValue(aCellIter.GetPos(), aCell), nThisCol, aPosTable[nThisRow]);
                    else
                        pResMat->PutDouble(GetCellValue(aCellIter.GetPos(), aCell), aPosTable[nThisCol], nThisRow);
                }
                else
                {
                    svl::SharedString aStr;
                    GetCellString(aStr, aCell);
                    if (rSortParam.bByRow)
                        pResMat->PutString(aStr, nThisCol, aPosTable[nThisRow]);
                    else
                        pResMat->PutString(aStr, aPosTable[nThisCol], nThisRow);
                }
            }
        }
        else
        {
            for (SCCOL ci = rSourceRange.aStart.Col(); ci <= rSourceRange.aEnd.Col(); ci++)
            {
                for (SCROW rj = rSourceRange.aStart.Row(); rj <= rSourceRange.aEnd.Row(); rj++)
                {
                    if (pMatSrc->IsEmptyCell(ci, rj))
                    {
                        if (rSortParam.bByRow)
                            pResMat->PutEmpty(ci, aPosTable[rj]);
                        else
                            pResMat->PutEmpty(aPosTable[ci], rj);
                    }
                    else if (pMatSrc->IsStringOrEmpty(ci, rj))
                    {
                        if (rSortParam.bByRow)
                            pResMat->PutString(pMatSrc->GetString(ci, rj), ci, aPosTable[rj]);
                        else
                            pResMat->PutString(pMatSrc->GetString(ci, rj), aPosTable[ci], rj);
                    }
                    else
                    {
                        if (rSortParam.bByRow)
                            pResMat->PutDouble(pMatSrc->GetDouble(ci, rj), ci, aPosTable[rj]);
                        else
                            pResMat->PutDouble(pMatSrc->GetDouble(ci, rj), aPosTable[ci], rj);
                    }
                }
            }
        }
    }

    return pResMat;
}

void ScInterpreter::QuickSort( ScSortInfoArray* pArray, const ScMatrixRef& pMatSrc, SCCOLROW nLo, SCCOLROW nHi )
{
    if ((nHi - nLo) == 1)
    {
        if (Compare(pArray, pMatSrc, nLo, nHi) > 0)
            pArray->Swap( nLo, nHi );
    }
    else
    {
        SCCOLROW ni = nLo;
        SCCOLROW nj = nHi;
        do
        {
            while ((ni <= nHi) && (Compare(pArray, pMatSrc, ni, nLo)) < 0)
                ni++;
            while ((nj >= nLo) && (Compare(pArray, pMatSrc, nLo, nj)) < 0)
                nj--;
            if (ni <= nj)
            {
                if (ni != nj)
                    pArray->Swap( ni, nj );
                ni++;
                nj--;
            }
        } while (ni < nj);
        if ((nj - nLo) < (nHi - ni))
        {
            if (nLo < nj)
                QuickSort(pArray, pMatSrc, nLo, nj);
            if (ni < nHi)
                QuickSort(pArray, pMatSrc, ni, nHi);
        }
        else
        {
            if (ni < nHi)
                QuickSort(pArray, pMatSrc, ni, nHi);
            if (nLo < nj)
                QuickSort(pArray, pMatSrc, nLo, nj);
        }
    }
}

short ScInterpreter::Compare( ScSortInfoArray* pArray, const ScMatrixRef& pMatSrc, SCCOLROW nIndex1, SCCOLROW nIndex2 ) const
{
    short nRes;
    sal_uInt16 nSort = 0;
    do
    {
        ScSortInfo& rInfo1 = pArray->Get( nSort, nIndex1 );
        ScSortInfo& rInfo2 = pArray->Get( nSort, nIndex2 );
        if (!pMatSrc)
        {
            nRes = CompareCell(nSort, rInfo1.maCell, rInfo2.maCell);
        }
        else
        {
            if (aSortParam.bByRow)
                nRes = CompareMatrixCell( pMatSrc, nSort,
                    static_cast<SCCOL>(aSortParam.maKeyState[nSort].nField), rInfo1.nOrg,
                    static_cast<SCCOL>(aSortParam.maKeyState[nSort].nField), rInfo2.nOrg );
            else
                nRes = CompareMatrixCell( pMatSrc, nSort,
                    static_cast<SCCOL>(rInfo1.nOrg), aSortParam.maKeyState[nSort].nField,
                    static_cast<SCCOL>(rInfo2.nOrg), aSortParam.maKeyState[nSort].nField );
        }
    } while ( nRes == 0 && ++nSort < pArray->GetUsedSorts() );
    if( nRes == 0 )
    {
        ScSortInfo& rInfo1 = pArray->Get( 0, nIndex1 );
        ScSortInfo& rInfo2 = pArray->Get( 0, nIndex2 );
        if( rInfo1.nOrg < rInfo2.nOrg )
            nRes = -1;
        else if( rInfo1.nOrg > rInfo2.nOrg )
            nRes = 1;
    }
    return nRes;
}

short ScInterpreter::CompareCell( sal_uInt16 nSort,
    ScRefCellValue& rCell1, ScRefCellValue& rCell2 ) const
{
    short nRes = 0;

    CellType eType1 = rCell1.getType(), eType2 = rCell2.getType();

    if (!rCell1.isEmpty())
    {
        if (!rCell2.isEmpty())
        {
            bool bErr1 = false;
            bool bStr1 = ( eType1 != CELLTYPE_VALUE );
            if (eType1 == CELLTYPE_FORMULA)
            {
                if (rCell1.getFormula()->GetErrCode() != FormulaError::NONE)
                {
                    bErr1 = true;
                    bStr1 = false;
                }
                else if (rCell1.getFormula()->IsValue())
                {
                    bStr1 = false;
                }
            }

            bool bErr2 = false;
            bool bStr2 = ( eType2 != CELLTYPE_VALUE );
            if (eType2 == CELLTYPE_FORMULA)
            {
                if (rCell2.getFormula()->GetErrCode() != FormulaError::NONE)
                {
                    bErr2 = true;
                    bStr2 = false;
                }
                else if (rCell2.getFormula()->IsValue())
                {
                    bStr2 = false;
                }
            }

            if ( bStr1 && bStr2 )           // only compare strings as strings!
            {
                OUString aStr1;
                OUString aStr2;

                if (eType1 == CELLTYPE_STRING)
                    aStr1 = rCell1.getSharedString()->getString();
                else
                    aStr1 = rCell1.getString(mrDoc);

                if (eType2 == CELLTYPE_STRING)
                    aStr2 = rCell2.getSharedString()->getString();
                else
                    aStr2 = rCell2.getString(mrDoc);

                CollatorWrapper& rSortCollator = ScGlobal::GetCollator(aSortParam.bCaseSens);
                nRes = static_cast<short>( rSortCollator.compareString( aStr1, aStr2 ) );
            }
            else if ( bStr1 )               // String <-> Number or Error
            {
                if (bErr2)
                    nRes = -1;              // String in front of Error
                else
                    nRes = 1;               // Number in front of String
            }
            else if ( bStr2 )               // Number or Error <-> String
            {
                if (bErr1)
                    nRes = 1;               // String in front of Error
                else
                    nRes = -1;              // Number in front of String
            }
            else if (bErr1 && bErr2)
            {
                // nothing, two Errors are equal
            }
            else if (bErr1)                 // Error <-> Number
            {
                nRes = 1;                   // Number in front of Error
            }
            else if (bErr2)                 // Number <-> Error
            {
                nRes = -1;                  // Number in front of Error
            }
            else                            // Mixed numbers
            {
                double nVal1 = rCell1.getValue();
                double nVal2 = rCell2.getValue();
                if (nVal1 < nVal2)
                    nRes = -1;
                else if (nVal1 > nVal2)
                    nRes = 1;
            }
            if ( !aSortParam.maKeyState[nSort].bAscending )
                nRes = -nRes;
        }
        else
            nRes = -1;
    }
    else
    {
        if (!rCell2.isEmpty())
            nRes = 1;
        else
            nRes = 0;                   // both empty
    }
    return nRes;
}

short ScInterpreter::CompareMatrixCell( const ScMatrixRef& pMatSrc, sal_uInt16 nSort, SCCOL nCell1Col, SCROW nCell1Row,
    SCCOL nCell2Col, SCROW nCell2Row ) const
{
    short nRes = 0;

    // 1st value
    bool bCell1Empty = false;
    bool bCell1Value = false;
    if (pMatSrc->IsEmpty(nCell1Col, nCell1Row))
        bCell1Empty = true;
    else if (pMatSrc->IsStringOrEmpty(nCell1Col, nCell1Row))
        bCell1Value = false;
    else
        bCell1Value = true;

    // 2nd value
    bool bCell2Empty = false;
    bool bCell2Value = false;
    if (pMatSrc->IsEmpty(nCell2Col, nCell2Row))
        bCell2Empty = true;
    else if (pMatSrc->IsStringOrEmpty(nCell2Col, nCell2Row))
        bCell2Value = false;
    else
        bCell2Value = true;

    if (!bCell1Empty)
    {
        if (!bCell2Empty)
        {
            if ( !bCell1Value && !bCell2Value )           // only compare strings as strings!
            {
                OUString aStr1 = pMatSrc->GetString(nCell1Col, nCell1Row).getString();
                OUString aStr2 = pMatSrc->GetString(nCell2Col, nCell2Row).getString();

                CollatorWrapper& rSortCollator = ScGlobal::GetCollator(aSortParam.bCaseSens);
                nRes = static_cast<short>( rSortCollator.compareString( aStr1, aStr2 ) );
            }
            else if ( !bCell1Value )        // String <-> Number or Error
            {
                nRes = 1;                   // Number in front of String
            }
            else if ( !bCell2Value )        // Number or Error <-> String
            {
                nRes = -1;                  // Number in front of String
            }
            else                            // Mixed numbers
            {
                double nVal1 = pMatSrc->GetDouble(nCell1Col, nCell1Row);
                double nVal2 = pMatSrc->GetDouble(nCell2Col, nCell2Row);
                if (nVal1 < nVal2)
                    nRes = -1;
                else if (nVal1 > nVal2)
                    nRes = 1;
            }
            if ( !aSortParam.maKeyState[nSort].bAscending )
                nRes = -nRes;
        }
        else
            nRes = -1;
    }
    else
    {
        if (!bCell2Empty)
            nRes = 1;
        else
            nRes = 0;                   // both empty
    }
    return nRes;
}

void ScInterpreter::GetSortArray( sal_uInt8 nParamCount, std::vector<double>& rSortArray, std::vector<tools::Long>* pIndexOrder, bool bConvertTextInArray, bool bAllowEmptyArray )
{
    GetNumberSequenceArray( nParamCount, rSortArray, bConvertTextInArray );
    if (rSortArray.size() > MAX_COUNT_DOUBLE_FOR_SORT(mrDoc.GetSheetLimits()))
        SetError( FormulaError::MatrixSize);
    else if ( rSortArray.empty() )
    {
        if ( bAllowEmptyArray )
            return;
        SetError( FormulaError::NoValue);
    }

    if (nGlobalError == FormulaError::NONE)
        QuickSort( rSortArray, pIndexOrder);
}

static void lcl_QuickSort( tools::Long nLo, tools::Long nHi, std::vector<double>& rSortArray, std::vector<tools::Long>* pIndexOrder )
{
    // If pIndexOrder is not NULL, we assume rSortArray.size() == pIndexOrder->size().

    if (nHi - nLo == 1)
    {
        if (rSortArray[nLo] > rSortArray[nHi])
        {
            std::swap(rSortArray[nLo],  rSortArray[nHi]);
            if (pIndexOrder)
                std::swap(pIndexOrder->at(nLo), pIndexOrder->at(nHi));
        }
        return;
    }

    tools::Long ni = nLo;
    tools::Long nj = nHi;
    do
    {
        double fLo = rSortArray[nLo];
        while (ni <= nHi && rSortArray[ni] < fLo) ni++;
        while (nj >= nLo && fLo < rSortArray[nj]) nj--;
        if (ni <= nj)
        {
            if (ni != nj)
            {
                std::swap(rSortArray[ni],  rSortArray[nj]);
                if (pIndexOrder)
                    std::swap(pIndexOrder->at(ni), pIndexOrder->at(nj));
            }

            ++ni;
            --nj;
        }
    }
    while (ni < nj);

    if ((nj - nLo) < (nHi - ni))
    {
        if (nLo < nj) lcl_QuickSort(nLo, nj, rSortArray, pIndexOrder);
        if (ni < nHi) lcl_QuickSort(ni, nHi, rSortArray, pIndexOrder);
    }
    else
    {
        if (ni < nHi) lcl_QuickSort(ni, nHi, rSortArray, pIndexOrder);
        if (nLo < nj) lcl_QuickSort(nLo, nj, rSortArray, pIndexOrder);
    }
}

void ScInterpreter::QuickSort(std::vector<double>& rSortArray, std::vector<tools::Long>* pIndexOrder )
{
    tools::Long n = static_cast<tools::Long>(rSortArray.size());

    if (pIndexOrder)
    {
        pIndexOrder->clear();
        pIndexOrder->reserve(n);
        for (tools::Long i = 0; i < n; ++i)
            pIndexOrder->push_back(i);
    }

    if (n < 2)
        return;

    size_t nValCount = rSortArray.size();
    for (size_t i = 0; (i + 4) <= nValCount-1; i += 4)
    {
        size_t nInd = comphelper::rng::uniform_size_distribution(0, nValCount-2);
        std::swap( rSortArray[i], rSortArray[nInd]);
        if (pIndexOrder)
            std::swap( pIndexOrder->at(i), pIndexOrder->at(nInd));
    }

    lcl_QuickSort(0, n-1, rSortArray, pIndexOrder);
}

void ScInterpreter::CalculatePearsonCovar( bool _bPearson, bool _bStexy, bool _bSample )
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    /* #i78250#
     * (sum((X-MeanX)(Y-MeanY)))/N equals (SumXY)/N-MeanX*MeanY mathematically,
     * but the latter produces wrong results if the absolute values are high,
     * for example above 10^8
     */
    double fCount           = 0.0;
    KahanSum fSumX          = 0.0;
    KahanSum fSumY          = 0.0;

    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsStringOrEmpty(i,j) && !pMat2->IsStringOrEmpty(i,j))
            {
                fSumX += pMat1->GetDouble(i,j);
                fSumY += pMat2->GetDouble(i,j);
                fCount++;
            }
        }
    }
    if (fCount < (_bStexy ? 3.0 : (_bSample ? 2.0 : 1.0)))
        PushNoValue();
    else
    {
        KahanSum fSumDeltaXDeltaY = 0.0; // sum of (ValX-MeanX)*(ValY-MeanY)
        KahanSum fSumSqrDeltaX    = 0.0; // sum of (ValX-MeanX)^2
        KahanSum fSumSqrDeltaY    = 0.0; // sum of (ValY-MeanY)^2
        const double fMeanX = fSumX.get() / fCount;
        const double fMeanY = fSumY.get() / fCount;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsStringOrEmpty(i,j) && !pMat2->IsStringOrEmpty(i,j))
                {
                    const double fValX = pMat1->GetDouble(i,j);
                    const double fValY = pMat2->GetDouble(i,j);
                    fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                    if ( _bPearson )
                    {
                        fSumSqrDeltaX    += (fValX - fMeanX) * (fValX - fMeanX);
                        fSumSqrDeltaY    += (fValY - fMeanY) * (fValY - fMeanY);
                    }
                }
            }
        }
        if ( _bPearson )
        {
            // tdf#94962 - Values below the numerical limit lead to unexpected results
            if (fSumSqrDeltaX < ::std::numeric_limits<double>::min()
                || (!_bStexy && fSumSqrDeltaY < ::std::numeric_limits<double>::min()))
                PushError( FormulaError::DivisionByZero);
            else if ( _bStexy )
                PushDouble( sqrt( ( fSumSqrDeltaY - fSumDeltaXDeltaY *
                            fSumDeltaXDeltaY / fSumSqrDeltaX ).get() / (fCount-2)));
            else
                PushDouble( fSumDeltaXDeltaY.get() / sqrt( fSumSqrDeltaX.get() * fSumSqrDeltaY.get() ));
        }
        else
        {
            if ( _bSample )
                PushDouble( fSumDeltaXDeltaY.get() / ( fCount - 1 ) );
            else
                PushDouble( fSumDeltaXDeltaY.get() / fCount);
        }
    }
}

void ScInterpreter::CalculateSlopeIntercept(bool bSlope)
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat1 = GetMatrix();
    ScMatrixRef pMat2 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (nR1 != nR2 || nC1 != nC2)
    {
        PushIllegalArgument();
        return;
    }
    // #i78250# numerical stability improved
    double fCount           = 0.0;
    KahanSum fSumX          = 0.0;
    KahanSum fSumY          = 0.0;

    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!pMat1->IsStringOrEmpty(i,j) && !pMat2->IsStringOrEmpty(i,j))
            {
                fSumX += pMat1->GetDouble(i,j);
                fSumY += pMat2->GetDouble(i,j);
                fCount++;
            }
        }
    }
    if (fCount < 1.0)
        PushNoValue();
    else
    {
        KahanSum fSumDeltaXDeltaY = 0.0; // sum of (ValX-MeanX)*(ValY-MeanY)
        KahanSum fSumSqrDeltaX    = 0.0; // sum of (ValX-MeanX)^2
        double fMeanX = fSumX.get() / fCount;
        double fMeanY = fSumY.get() / fCount;
        for (SCSIZE i = 0; i < nC1; i++)
        {
            for (SCSIZE j = 0; j < nR1; j++)
            {
                if (!pMat1->IsStringOrEmpty(i,j) && !pMat2->IsStringOrEmpty(i,j))
                {
                    double fValX = pMat1->GetDouble(i,j);
                    double fValY = pMat2->GetDouble(i,j);
                    fSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
                    fSumSqrDeltaX    += (fValX - fMeanX) * (fValX - fMeanX);
                }
            }
        }
        if (fSumSqrDeltaX == 0.0)
            PushError( FormulaError::DivisionByZero);
        else
        {
            if ( bSlope )
                PushDouble( fSumDeltaXDeltaY.get() / fSumSqrDeltaX.get());
            else
                PushDouble( fMeanY - fSumDeltaXDeltaY.get() / fSumSqrDeltaX.get() * fMeanX);
        }
    }
}

void ScInterpreter::ExecuteFourierTerminal()
{
    seinterpcompatdispatch::Dispatcher::fourier(*this);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
