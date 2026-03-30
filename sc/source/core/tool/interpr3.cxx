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

void ScInterpreter::ScNoName()
{
    PushError(FormulaError::NoName);
}

void ScInterpreter::ScBadName()
{
    short nParamCount = GetByte();
    while (nParamCount-- > 0)
    {
        PopError();
    }
    PushError( FormulaError::NoName);
}

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

void ScInterpreter::ScChiSqDist()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;
    bool bCumulative;
    if (nParamCount == 3)
        bCumulative = GetBool();
    else
        bCumulative = true;
    double fDF = ::rtl::math::approxFloor(GetDouble());
    if (fDF < 1.0)
        PushIllegalArgument();
    else
    {
        double fX = GetDouble();
        if (bCumulative)
            PushDouble(GetChiSqDistCDF(fX,fDF));
        else
            PushDouble(GetChiSqDistPDF(fX,fDF));
    }
}

void ScInterpreter::ScChiSqDist_MS()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 3 ) )
        return;
    bool bCumulative = GetBool();
    double fDF = ::rtl::math::approxFloor( GetDouble() );
    if ( fDF < 1.0 || fDF > 1E10 )
        PushIllegalArgument();
    else
    {
        double fX = GetDouble();
        if ( fX < 0 )
            PushIllegalArgument();
        else
        {
            if ( bCumulative )
                PushDouble( GetChiSqDistCDF( fX, fDF ) );
            else
                PushDouble( GetChiSqDistPDF( fX, fDF ) );
        }
    }
}

void ScInterpreter::ScGamma()
{
    double x = GetDouble();
    const auto aResult = semath::evaluateGammaValue(x);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScLogGamma()
{
    double x = GetDouble();
    const auto aResult = semath::evaluateLogGammaValue(x);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
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

void ScInterpreter::ScBetaDist()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 6 ) ) // expanded, see #i91547#
        return;
    double fLowerBound, fUpperBound;
    double alpha, beta, x;
    bool bIsCumulative;
    if (nParamCount == 6)
        bIsCumulative = GetBool();
    else
        bIsCumulative = true;
    if (nParamCount >= 5)
        fUpperBound = GetDouble();
    else
        fUpperBound = 1.0;
    if (nParamCount >= 4)
        fLowerBound = GetDouble();
    else
        fLowerBound = 0.0;
    beta = GetDouble();
    alpha = GetDouble();
    x = GetDouble();
    const auto aResult = semath::evaluateBetaDistribution(
        x, alpha, beta, fLowerBound, fUpperBound, bIsCumulative, false);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

/**
  Microsoft version has parameters in different order
  Also, upper and lowerbound are optional and have default values
  and different constraints apply.
  Basically, function is identical with ScInterpreter::ScBetaDist()
*/
void ScInterpreter::ScBetaDist_MS()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 4, 6 ) )
        return;
    double fLowerBound, fUpperBound;
    double alpha, beta, x;
    bool bIsCumulative;
    if (nParamCount == 6)
        fUpperBound = GetDouble();
    else
        fUpperBound = 1.0;
    if (nParamCount >= 5)
        fLowerBound = GetDouble();
    else
        fLowerBound = 0.0;
    bIsCumulative = GetBool();
    beta = GetDouble();
    alpha = GetDouble();
    x = GetDouble();
    const auto aResult = semath::evaluateBetaDistribution(
        x, alpha, beta, fLowerBound, fUpperBound, bIsCumulative, true);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScPhi()
{
    PushDouble(semath::evaluateNormalDistribution(GetDouble(), 0.0, 1.0, false).maValue);
}

void ScInterpreter::ScGauss()
{
    PushDouble(semath::gaussValue(GetDouble()));
}

void ScInterpreter::ScFisher()
{
    const auto aResult = semath::fisherTransform(GetDouble());
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScFisherInv()
{
    PushDouble(semath::inverseFisherTransform(GetDouble()));
}

void ScInterpreter::ScFact()
{
    const auto aResult = semath::evaluateFactorialValue(GetDouble());
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScCombin()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        const double k = GetDouble();
        const double n = GetDouble();
        const auto aResult = semath::evaluateCombinValue(n, k, false);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
}

void ScInterpreter::ScCombinA()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        const double k = GetDouble();
        const double n = GetDouble();
        const auto aResult = semath::evaluateCombinValue(n, k, true);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
}

void ScInterpreter::ScPermut()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;

    const double k = GetDouble();
    const double n = GetDouble();
    const auto aResult = semath::evaluatePermutationValue(n, k);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScPermutationA()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        const double k = GetDouble();
        const double n = GetDouble();
        const auto aResult = semath::evaluatePermutationAValue(n, k);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
}

void ScInterpreter::ScB()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 4 ) )
        return ;
    if (nParamCount == 3)   // mass function
    {
        const double x = GetDouble();
        const double p = GetDouble();
        const double n = GetDouble();
        const auto aResult = semath::evaluateBinomialDistribution(x, n, p, false);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
    else
    {   // nParamCount == 4
        const double xe = GetDouble();
        const double xs = GetDouble();
        const double p = GetDouble();
        const double n = GetDouble();
        const auto aResult = semath::evaluateBinomialRangeDistribution(n, p, xs, xe);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
}

void ScInterpreter::ScBinomDist()
{
    if ( !MustHaveParamCount( GetByte(), 4 ) )
        return;

    bool bIsCum   = GetBool();     // false=mass function; true=cumulative
    double p      = GetDouble();
    double n      = GetDouble();
    double x      = GetDouble();
    const auto aResult = semath::evaluateBinomialDistribution(x, n, p, bIsCum);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScCritBinom()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;

    double alpha  = GetDouble();
    double p      = GetDouble();
    double n      = GetDouble();
    const auto aResult = semath::evaluateBinomialInverse(n, p, alpha);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScNegBinomDist()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;

    const double p = GetDouble();
    const double s = GetDouble();
    const double f = GetDouble();
    const auto aResult = semath::evaluateNegativeBinomialDistribution(f, s, p, false, false);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScNegBinomDist_MS()
{
    if ( !MustHaveParamCount( GetByte(), 4 ) )
        return;

    const bool bCumulative = GetBool();
    const double p = GetDouble();
    const double s = GetDouble();
    const double f = GetDouble();
    const auto aResult = semath::evaluateNegativeBinomialDistribution(
        f, s, p, bCumulative, true);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScNormDist( int nMinParamCount )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, nMinParamCount, 4 ) )
        return;
    bool bCumulative = nParamCount != 4 || GetBool();
    double sigma = GetDouble();                 // standard deviation
    double mue = GetDouble();                   // mean
    double x = GetDouble();                     // x
    const auto aResult = semath::evaluateNormalDistribution(x, mue, sigma, bCumulative);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScLogNormDist( int nMinParamCount ) //expanded, see #i100119# and fdo72158
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, nMinParamCount, 4 ) )
        return;
    bool bCumulative = nParamCount != 4 || GetBool(); // cumulative
    double sigma = nParamCount >= 3 ? GetDouble() : 1.0; // standard deviation
    double mue = nParamCount >= 2 ? GetDouble() : 0.0;   // mean
    double x = GetDouble();                              // x
    const auto aResult = semath::evaluateLogNormalDistribution(x, mue, sigma, bCumulative);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScStdNormDist()
{
    PushDouble(semath::evaluateNormalDistribution(GetDouble(), 0.0, 1.0, true).maValue);
}

void ScInterpreter::ScStdNormDist_MS()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2 ) )
        return;
    bool bCumulative = GetBool();                        // cumulative
    double x = GetDouble();                              // x

    if (bCumulative)
        PushDouble(semath::evaluateNormalDistribution(x, 0.0, 1.0, true).maValue);
    else
        PushDouble(semath::evaluateNormalDistribution(x, 0.0, 1.0, false).maValue);
}

void ScInterpreter::ScExpDist()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;

    const bool bCumulative = GetDouble() != 0.0;
    const double lambda = GetDouble();
    const double x = GetDouble();
    const auto aResult = semath::evaluateExponentialDistribution(x, lambda, bCumulative);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScTDist()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fFlag = ::rtl::math::approxFloor(GetDouble());
    double fDF   = ::rtl::math::approxFloor(GetDouble());
    double T     = GetDouble();
    if (fDF < 1.0 || T < 0.0 || (fFlag != 1.0 && fFlag != 2.0) )
    {
        PushIllegalArgument();
        return;
    }
    PushDouble( GetTDist( T, fDF, static_cast<int>(fFlag) ) );
}

void ScInterpreter::ScTDist_T( int nTails )
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fDF = ::rtl::math::approxFloor( GetDouble() );
    double fT  = GetDouble();
    if ( fDF < 1.0 || ( nTails == 2 && fT < 0.0 ) )
    {
        PushIllegalArgument();
        return;
    }
    double fRes = GetTDist( fT, fDF, nTails );
    if ( nTails == 1 && fT < 0.0 )
        PushDouble( 1.0 - fRes ); // tdf#105937, right tail, negative X
    else
        PushDouble( fRes );
}

void ScInterpreter::ScTDist_MS()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    bool   bCumulative = GetBool();
    double fDF = ::rtl::math::approxFloor( GetDouble() );
    double T   = GetDouble();
    if ( fDF < 1.0 )
    {
        PushIllegalArgument();
        return;
    }
    PushDouble( GetTDist( T, fDF, ( bCumulative ? 4 : 3 ) ) );
}

void ScInterpreter::ScFDist()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fF2 = ::rtl::math::approxFloor(GetDouble());
    double fF1 = ::rtl::math::approxFloor(GetDouble());
    double fF  = GetDouble();
    if (fF < 0.0 || fF1 < 1.0 || fF2 < 1.0 || fF1 >= 1.0E10 || fF2 >= 1.0E10)
    {
        PushIllegalArgument();
        return;
    }
    PushDouble(GetFDist(fF, fF1, fF2));
}

void ScInterpreter::ScFDist_LT()
{
    int nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 4 ) )
        return;
    bool bCum;
    if ( nParamCount == 3 )
        bCum = true;
    else if ( IsMissing() )
    {
        bCum = true;
        Pop();
    }
    else
        bCum = GetBool();
    double fF2 = ::rtl::math::approxFloor( GetDouble() );
    double fF1 = ::rtl::math::approxFloor( GetDouble() );
    double fF  = GetDouble();
    if ( fF < 0.0 || fF1 < 1.0 || fF2 < 1.0 || fF1 >= 1.0E10 || fF2 >= 1.0E10 )
    {
        PushIllegalArgument();
        return;
    }
    if ( bCum )
    {
        // left tail cumulative distribution
        PushDouble( 1.0 - GetFDist( fF, fF1, fF2 ) );
    }
    else
    {
        // probability density function
        PushDouble( pow( fF1 / fF2, fF1 / 2 ) * pow( fF, ( fF1 / 2 ) - 1 ) /
                    ( pow( ( 1 + ( fF * fF1 / fF2 ) ), ( fF1 + fF2 ) / 2 ) *
                      GetBeta( fF1 / 2, fF2 / 2 ) ) );
    }
}

void ScInterpreter::ScChiDist( bool bODFF )
{
    double fResult;
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fDF  = ::rtl::math::approxFloor(GetDouble());
    double fChi = GetDouble();
    if ( fDF < 1.0 // x<=0 returns 1, see ODFF1.2 6.18.11
       || ( !bODFF && fChi < 0 ) ) // Excel does not accept negative fChi
    {
        PushIllegalArgument();
        return;
    }
    fResult = GetChiDist( fChi, fDF);
    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;
    }
    PushDouble(fResult);
}

void ScInterpreter::ScWeibull()
{
    if ( !MustHaveParamCount( GetByte(), 4 ) )
        return;

    const bool bCumulative = GetDouble() != 0.0;
    const double beta = GetDouble();
    const double alpha = GetDouble();
    const double x = GetDouble();
    const auto aResult = semath::evaluateWeibullDistribution(x, alpha, beta, bCumulative);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScPoissonDist( bool bODFF )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, ( bODFF ? 2 : 3 ), 3 ) )
        return;

    bool bCumulative = nParamCount != 3 || GetBool(); // default cumulative
    double lambda = GetDouble();                      // Mean
    double x = GetDouble();                           // discrete distribution
    const auto aResult = semath::evaluatePoissonDistribution(x, lambda, bCumulative);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScHypGeomDist( int nMinParamCount )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, nMinParamCount, 5 ) )
        return;

    bool bCumulative = ( nParamCount == 5 && GetBool() );
    double N = ::rtl::math::approxFloor(GetDouble());
    double M = ::rtl::math::approxFloor(GetDouble());
    double n = ::rtl::math::approxFloor(GetDouble());
    double x = ::rtl::math::approxFloor(GetDouble());

    if ( (x < 0.0) || (n < x) || (N < n) || (N < M) || (M < 0.0) )
    {
        PushIllegalArgument();
        return;
    }

    const auto aResult = semath::evaluateHypergeometricDistribution(x, n, M, N, bCumulative);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScGammaDist( bool bODFF )
{
    sal_uInt8 nMinParamCount = ( bODFF ? 3 : 4 );
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, nMinParamCount, 4 ) )
        return;
    bool bCumulative;
    if (nParamCount == 4)
        bCumulative = GetBool();
    else
        bCumulative = true;
    double fBeta = GetDouble();                 // scale
    double fAlpha = GetDouble();                // shape
    double fX = GetDouble();                    // x
    if ((!bODFF && fX < 0) || fAlpha <= 0.0 || fBeta <= 0.0)
        PushIllegalArgument();
    else
    {
        if (bCumulative)                        // distribution
            PushDouble( GetGammaDist( fX, fAlpha, fBeta));
        else                                    // density
            PushDouble( GetGammaDistPDF( fX, fAlpha, fBeta));
    }
}

void ScInterpreter::ScNormInv()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double sigma = GetDouble();
        double mue   = GetDouble();
        double x     = GetDouble();
        const auto aResult = semath::evaluateNormalInverse(x, mue, sigma);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
}

void ScInterpreter::ScSNormInv()
{
    double x = GetDouble();
    const auto aResult = semath::evaluateStandardNormalInverse(x);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScLogNormInv()
{
    sal_uInt8 nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 1, 3 ) )
    {
        double fSigma = ( nParamCount == 3 ? GetDouble() : 1.0 );  // Stddev
        double fMue = ( nParamCount >= 2 ? GetDouble() : 0.0 );    // Mean
        double fP = GetDouble();                                   // p
        const auto aResult = semath::evaluateLogNormalInverse(fP, fMue, fSigma);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
}

void ScInterpreter::ScGammaInv()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fBeta  = GetDouble();
    double fAlpha = GetDouble();
    double fP = GetDouble();
    const auto aResult = semath::evaluateGammaInverse(fP, fAlpha, fBeta);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScBetaInv()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 5 ) )
        return;
    double fP, fA, fB, fAlpha, fBeta;
    if (nParamCount == 5)
        fB = GetDouble();
    else
        fB = 1.0;
    if (nParamCount >= 4)
        fA = GetDouble();
    else
        fA = 0.0;
    fBeta  = GetDouble();
    fAlpha = GetDouble();
    fP     = GetDouble();
    const auto aResult = semath::evaluateBetaInverse(fP, fAlpha, fBeta, fA, fB);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScTInv( int nType )
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fDF  = ::rtl::math::approxFloor(GetDouble());
    double fP = GetDouble();
    if (fDF < 1.0 || fP <= 0.0 || fP > 1.0 )
    {
        PushIllegalArgument();
        return;
    }
    if ( nType == 4 ) // left-tailed cumulative t-distribution
    {
        if ( fP == 1.0 )
            PushIllegalArgument();
        else if ( fP < 0.5 )
            PushDouble( -GetTInv( 1 - fP, fDF, nType ) );
        else
            PushDouble( GetTInv( fP, fDF, nType ) );
    }
    else
        PushDouble( GetTInv( fP, fDF, nType ) );
};

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

void ScInterpreter::ScFInv()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fF2 = ::rtl::math::approxFloor(GetDouble());
    double fF1 = ::rtl::math::approxFloor(GetDouble());
    double fP  = GetDouble();
    if (fP <= 0.0 || fF1 < 1.0 || fF2 < 1.0 || fF1 >= 1.0E10 || fF2 >= 1.0E10 || fP > 1.0)
    {
        PushIllegalArgument();
        return;
    }

    const auto aResult = semath::evaluateFInverseRightTail(fP, fF1, fF2);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScFInv_LT()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;
    double fF2 = ::rtl::math::approxFloor(GetDouble());
    double fF1 = ::rtl::math::approxFloor(GetDouble());
    double fP  = GetDouble();
    if (fP <= 0.0 || fF1 < 1.0 || fF2 < 1.0 || fF1 >= 1.0E10 || fF2 >= 1.0E10 || fP > 1.0)
    {
        PushIllegalArgument();
        return;
    }

    const auto aResult = semath::evaluateFInverseRightTail(1.0 - fP, fF1, fF2);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScChiInv()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fDF  = ::rtl::math::approxFloor(GetDouble());
    double fP = GetDouble();
    const auto aResult = semath::evaluateLegacyChiInverse(fP, fDF);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScChiSqInv()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fDF  = ::rtl::math::approxFloor(GetDouble());
    double fP = GetDouble();
    const auto aResult = semath::evaluateChiSquareInverse(fP, fDF);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScConfidence()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double n     = ::rtl::math::approxFloor(GetDouble());
        double sigma = GetDouble();
        double alpha = GetDouble();
        const auto aResult = semath::evaluateConfidence(alpha, sigma, n);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
}

void ScInterpreter::ScConfidenceT()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double n     = ::rtl::math::approxFloor(GetDouble());
        double sigma = GetDouble();
        double alpha = GetDouble();
        const auto aResult = semath::evaluateConfidenceT(alpha, sigma, n);
        if (!aResult)
        {
            PushError(lcl_ToCalcMathFormulaError(aResult.meError));
            return;
        }
        PushDouble(aResult.maValue);
    }
}

void ScInterpreter::ScZTest()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;
    double sigma = 0.0, x;
    if (nParamCount == 3)
    {
        sigma = GetDouble();
        if (sigma <= 0.0)
        {
            PushIllegalArgument();
            return;
        }
    }
    x = GetDouble();

    KahanSum fSum    = 0.0;
    KahanSum fSumSqr = 0.0;
    double fVal;
    double rValCount = 0.0;
    switch (GetStackType())
    {
        case svDouble :
        {
            fVal = GetDouble();
            fSum    += fVal;
            fSumSqr += fVal*fVal;
            rValCount++;
        }
        break;
        case svSingleRef :
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            ScRefCellValue aCell(mrDoc, aAdr);
            if (aCell.hasNumeric())
            {
                fVal = GetCellValue(aAdr, aCell);
                fSum += fVal;
                fSumSqr += fVal*fVal;
                rValCount++;
            }
        }
        break;
        case svRefList :
        case svDoubleRef :
        {
            short nParam = 1;
            size_t nRefInList = 0;
            while (nParam-- > 0)
            {
                ScRange aRange;
                FormulaError nErr = FormulaError::NONE;
                PopDoubleRef( aRange, nParam, nRefInList);
                ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags );
                if (aValIter.GetFirst(fVal, nErr))
                {
                    fSum += fVal;
                    fSumSqr += fVal*fVal;
                    rValCount++;
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(fVal, nErr))
                    {
                        fSum += fVal;
                        fSumSqr += fVal*fVal;
                        rValCount++;
                    }
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
            if (pMat)
            {
                SCSIZE nCount = pMat->GetElementCount();
                if (pMat->IsNumeric())
                {
                    for ( SCSIZE i = 0; i < nCount; i++ )
                    {
                        fVal= pMat->GetDouble(i);
                        fSum += fVal;
                        fSumSqr += fVal * fVal;
                        rValCount++;
                    }
                }
                else
                {
                    for (SCSIZE i = 0; i < nCount; i++)
                        if (!pMat->IsStringOrEmpty(i))
                        {
                            fVal= pMat->GetDouble(i);
                            fSum += fVal;
                            fSumSqr += fVal * fVal;
                            rValCount++;
                        }
                }
            }
        }
        break;
        default : SetError(FormulaError::IllegalParameter); break;
    }
    if (rValCount <= 1.0)
        PushError( FormulaError::DivisionByZero);
    else
    {
        double mue = fSum.get()/rValCount;

        if (nParamCount != 3)
        {
            sigma = (fSumSqr - fSum*fSum/rValCount).get()/(rValCount-1.0);
            if (sigma == 0.0)
            {
                PushError(FormulaError::DivisionByZero);
                return;
            }
            PushDouble(0.5 - semath::gaussValue((mue-x) / sqrt(sigma / rValCount)));
        }
        else
            PushDouble(0.5 - semath::gaussValue((mue-x) * sqrt(rValCount) / sigma));
    }
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
void ScInterpreter::ScTTest()
{
    if ( !MustHaveParamCount( GetByte(), 4 ) )
        return;
    double fTyp   = ::rtl::math::approxFloor(GetDouble());
    double fTails = ::rtl::math::approxFloor(GetDouble());
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
    double fT, fF;
    SCSIZE nC1, nC2;
    SCSIZE nR1, nR2;
    SCSIZE i, j;
    pMat1->GetDimensions(nC1, nR1);
    pMat2->GetDimensions(nC2, nR2);
    if (fTyp == 1.0)
    {
        if (nC1 != nC2 || nR1 != nR2)
        {
            PushIllegalArgument();
            return;
        }
        double fCount   = 0.0;
        KahanSum fSum1    = 0.0;
        KahanSum fSum2    = 0.0;
        KahanSum fSumSqrD = 0.0;
        double fVal1, fVal2;
        for (i = 0; i < nC1; i++)
            for (j = 0; j < nR1; j++)
            {
                if (!pMat1->IsStringOrEmpty(i,j) && !pMat2->IsStringOrEmpty(i,j))
                {
                    fVal1 = pMat1->GetDouble(i,j);
                    fVal2 = pMat2->GetDouble(i,j);
                    fSum1    += fVal1;
                    fSum2    += fVal2;
                    fSumSqrD += (fVal1 - fVal2)*(fVal1 - fVal2);
                    fCount++;
                }
            }
        if (fCount < 1.0)
        {
            PushNoValue();
            return;
        }
        KahanSum fSumD = fSum1 - fSum2;
        double fDivider = ( fSumSqrD*fCount - fSumD*fSumD ).get();
        if ( fDivider == 0.0 )
        {
            PushError(FormulaError::DivisionByZero);
            return;
        }
        fT = std::abs(fSumD.get()) * sqrt((fCount-1.0) / fDivider);
        fF = fCount - 1.0;
    }
    else if (fTyp == 2.0)
    {
        if (!CalculateTest(false,nC1, nC2,nR1, nR2,pMat1,pMat2,fT,fF))
            return;     // error was pushed
    }
    else if (fTyp == 3.0)
    {
        if (!CalculateTest(true,nC1, nC2,nR1, nR2,pMat1,pMat2,fT,fF))
            return;     // error was pushed
    }
    else
    {
        PushIllegalArgument();
        return;
    }
    PushDouble( GetTDist( fT, fF, static_cast<int>(fTails) ) );
}

void ScInterpreter::ScFTest()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat2 = GetMatrix();
    ScMatrixRef pMat1 = GetMatrix();
    if (!pMat1 || !pMat2)
    {
        PushIllegalParameter();
        return;
    }

    auto aVal1 = pMat1->CollectKahan(sc::op::kOpSumAndSumSquare);
    auto aVal2 = pMat2->CollectKahan(sc::op::kOpSumAndSumSquare);
    double fCount1  = aVal1.mnCount;
    double fCount2  = aVal2.mnCount;
    KahanSum fSum1    = aVal1.maAccumulator[0];
    KahanSum fSumSqr1 = aVal1.maAccumulator[1];
    KahanSum fSum2    = aVal2.maAccumulator[0];
    KahanSum fSumSqr2 = aVal2.maAccumulator[1];

    if (fCount1 < 2.0 || fCount2 < 2.0)
    {
        PushNoValue();
        return;
    }
    double fS1 = (fSumSqr1-fSum1*fSum1/fCount1).get() / (fCount1-1.0);
    double fS2 = (fSumSqr2-fSum2*fSum2/fCount2).get() / (fCount2-1.0);
    if (fS1 == 0.0 || fS2 == 0.0)
    {
        PushNoValue();
        return;
    }
    double fF, fF1, fF2;
    if (fS1 > fS2)
    {
        fF = fS1/fS2;
        fF1 = fCount1-1.0;
        fF2 = fCount2-1.0;
    }
    else
    {
        fF = fS2/fS1;
        fF1 = fCount2-1.0;
        fF2 = fCount1-1.0;
    }
    double fFcdf = GetFDist(fF, fF1, fF2);
    PushDouble(2.0*std::min(fFcdf, 1.0 - fFcdf));
}

void ScInterpreter::ScChiTest()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    ScMatrixRef pMat2 = GetMatrix();
    ScMatrixRef pMat1 = GetMatrix();
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
    KahanSum fChi = 0.0;
    bool bEmpty = true;
    for (SCSIZE i = 0; i < nC1; i++)
    {
        for (SCSIZE j = 0; j < nR1; j++)
        {
            if (!(pMat1->IsEmpty(i,j) || pMat2->IsEmpty(i,j)))
            {
                bEmpty = false;
                if (!pMat1->IsStringOrEmpty(i,j) && !pMat2->IsStringOrEmpty(i,j))
                {
                    double fValX = pMat1->GetDouble(i,j);
                    double fValE = pMat2->GetDouble(i,j);
                    if ( fValE == 0.0 )
                    {
                        PushError(FormulaError::DivisionByZero);
                        return;
                    }
                    // These fTemp values guard against a failure when compiled
                    // with optimization (using g++ 4.8.2 on tinderbox 71-TDF),
                    // where ((fValX - fValE) * (fValX - fValE)) with
                    // fValE==1e+308 should had produced Infinity but did
                    // not, instead the result of divide() then was 1e+308.
                    volatile double fTemp1 = (fValX - fValE) * (fValX - fValE);
                    double fTemp2 = fTemp1;
                    if (std::isinf(fTemp2))
                    {
                        PushError(FormulaError::NoConvergence);
                        return;
                    }
                    fChi += sc::divide( fTemp2, fValE);
                }
                else
                {
                    PushIllegalArgument();
                    return;
                }
            }
        }
    }
    if ( bEmpty )
    {
        // not in ODFF1.2, but for interoperability with Excel
        PushIllegalArgument();
        return;
    }
    double fDF;
    if (nC1 == 1 || nR1 == 1)
    {
        fDF = static_cast<double>(nC1*nR1 - 1);
        if (fDF == 0.0)
        {
            PushNoValue();
            return;
        }
    }
    else
        fDF = static_cast<double>(nC1-1)*static_cast<double>(nR1-1);
    PushDouble(GetChiDist(fChi.get(), fDF));
}

void ScInterpreter::ScKurt()
{
    KahanSum fSum;
    double fCount;
    std::vector<double> values;
    if ( !CalculateSkew(fSum, fCount, values) )
        return;

    const auto aResult = semath::evaluateKurtosisNumbers(values);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScHarMean()
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
            case svDouble    :
            {
                double x = GetDouble();
                if (x > 0.0)
                    aValues.push_back(x);
                else
                    SetError( FormulaError::IllegalArgument);
                break;
            }
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasNumeric())
                {
                    double x = GetCellValue(aAdr, aCell);
                    if (x > 0.0)
                        aValues.push_back(x);
                    else
                        SetError( FormulaError::IllegalArgument);
                }
                break;
            }
            case svDoubleRef :
            case svRefList :
            {
                FormulaError nErr = FormulaError::NONE;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                double nCellVal;
                ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags );
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    if (nCellVal > 0.0)
                        aValues.push_back(nCellVal);
                    else
                        SetError( FormulaError::IllegalArgument);
                    SetError(nErr);
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(nCellVal, nErr))
                    {
                        if (nCellVal > 0.0)
                            aValues.push_back(nCellVal);
                        else
                            SetError( FormulaError::IllegalArgument);
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
                            double x = pMat->GetDouble(nElem);
                            if (x > 0.0)
                                aValues.push_back(x);
                            else
                                SetError( FormulaError::IllegalArgument);
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            if (!pMat->IsStringOrEmpty(nElem))
                            {
                                double x = pMat->GetDouble(nElem);
                                if (x > 0.0)
                                    aValues.push_back(x);
                                else
                                    SetError( FormulaError::IllegalArgument);
                            }
                    }
                }
            }
            break;
            default : SetError(FormulaError::IllegalParameter); break;
        }
    }
    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;
    }

    const auto aResult = semath::evaluateHarmonicMeanNumbers(aValues);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScGeoMean()
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
            case svDouble    :
            {
                double x = GetDouble();
                if (x > 0.0)
                    aValues.push_back(x);
                else if ( x == 0.0 )
                {
                    // value of 0 means that function result will be 0
                    while ( nParamCount-- > 0 )
                        PopError();
                    PushDouble( 0.0 );
                    return;
                }
                else
                    SetError( FormulaError::IllegalArgument);
                break;
            }
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasNumeric())
                {
                    double x = GetCellValue(aAdr, aCell);
                    if (x > 0.0)
                        aValues.push_back(x);
                    else if ( x == 0.0 )
                    {
                        // value of 0 means that function result will be 0
                        while ( nParamCount-- > 0 )
                            PopError();
                        PushDouble( 0.0 );
                        return;
                    }
                    else
                        SetError( FormulaError::IllegalArgument);
                }
                break;
            }
            case svDoubleRef :
            case svRefList :
            {
                FormulaError nErr = FormulaError::NONE;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                double nCellVal;
                ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    if (nCellVal > 0.0)
                        aValues.push_back(nCellVal);
                    else if ( nCellVal == 0.0 )
                    {
                        // value of 0 means that function result will be 0
                        while ( nParamCount-- > 0 )
                            PopError();
                        PushDouble( 0.0 );
                        return;
                    }
                    else
                        SetError( FormulaError::IllegalArgument);
                    SetError(nErr);
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(nCellVal, nErr))
                    {
                        if (nCellVal > 0.0)
                            aValues.push_back(nCellVal);
                        else if ( nCellVal == 0.0 )
                        {
                            // value of 0 means that function result will be 0
                            while ( nParamCount-- > 0 )
                                PopError();
                            PushDouble( 0.0 );
                            return;
                        }
                        else
                            SetError( FormulaError::IllegalArgument);
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
                        for (SCSIZE ui = 0; ui < nCount; ui++)
                        {
                            double x = pMat->GetDouble(ui);
                            if (x > 0.0)
                                aValues.push_back(x);
                            else if ( x == 0.0 )
                            {
                                // value of 0 means that function result will be 0
                                while ( nParamCount-- > 0 )
                                    PopError();
                                PushDouble( 0.0 );
                                return;
                            }
                            else
                                SetError( FormulaError::IllegalArgument);
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
                                else if ( x == 0.0 )
                                {
                                    // value of 0 means that function result will be 0
                                    while ( nParamCount-- > 0 )
                                        PopError();
                                    PushDouble( 0.0 );
                                    return;
                                }
                                else
                                    SetError( FormulaError::IllegalArgument);
                            }
                        }
                    }
                }
            }
            break;
            default : SetError(FormulaError::IllegalParameter); break;
        }
    }
    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;
    }

    const auto aResult = semath::evaluateGeometricMeanNumbers(aValues);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScStandard()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double sigma = GetDouble();
        double mue   = GetDouble();
        double x     = GetDouble();
        if (sigma < 0.0)
            PushError( FormulaError::IllegalArgument);
        else if (sigma == 0.0)
            PushError( FormulaError::DivisionByZero);
        else
            PushDouble((x-mue)/sigma);
    }
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

void ScInterpreter::ScSkew()
{
    KahanSum fSum;
    double fCount = 0.0;
    std::vector<double> aValues;
    if (!CalculateSkew(fSum, fCount, aValues))
        return;

    const auto aResult = semath::evaluateSkewNumbers(aValues, false);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScSkewp()
{
    KahanSum fSum;
    double fCount = 0.0;
    std::vector<double> aValues;
    if (!CalculateSkew(fSum, fCount, aValues))
        return;

    const auto aResult = semath::evaluateSkewNumbers(aValues, true);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScMedian()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 )  )
        return;
    std::vector<double> aArray;
    GetNumberSequenceArray( nParamCount, aArray, false );
    if (aArray.empty() || nGlobalError != FormulaError::NONE)
    {
        PushNoValue();
        return;
    }

    semath::AggregateScan aScan;
    aScan.maNumbers = std::move(aArray);
    const auto aResult = semath::evaluateAggregateNumbers(12, aScan);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

double ScInterpreter::GetPercentile(std::vector<double> & rArray, double fPercentile )
{
    semath::AggregateScan aScan;
    aScan.maNumbers = rArray;
    const auto aResult = semath::evaluateAggregateRankedNumbers(16, aScan, fPercentile);
    return aResult ? aResult.maValue : 0.0;
}

void ScInterpreter::ScPercentile( bool bInclusive )
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double alpha = GetDouble();
    if ( bInclusive ? ( alpha < 0.0 || alpha > 1.0 ) : ( alpha <= 0.0 || alpha >= 1.0 ) )
    {
        PushIllegalArgument();
        return;
    }
    std::vector<double> aArray;
    GetNumberSequenceArray( 1, aArray, false );
    if ( aArray.empty() || nGlobalError != FormulaError::NONE )
    {
        PushNoValue();
        return;
    }
    semath::AggregateScan aScan;
    aScan.maNumbers = std::move(aArray);
    const auto aResult = semath::evaluateAggregateRankedNumbers(
        bInclusive ? 16 : 18, aScan, alpha);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScQuartile( bool bInclusive )
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double fFlag = ::rtl::math::approxFloor(GetDouble());
    if ( bInclusive ? ( fFlag < 0.0 || fFlag > 4.0 ) : ( fFlag <= 0.0 || fFlag >= 4.0 ) )
    {
        PushIllegalArgument();
        return;
    }
    std::vector<double> aArray;
    GetNumberSequenceArray( 1, aArray, false );
    if ( aArray.empty() || nGlobalError != FormulaError::NONE )
    {
        PushNoValue();
        return;
    }
    semath::AggregateScan aScan;
    aScan.maNumbers = std::move(aArray);
    const auto aResult = semath::evaluateAggregateRankedNumbers(
        bInclusive ? 17 : 19, aScan, fFlag);
    if (!aResult)
    {
        PushError(lcl_ToCalcMathFormulaError(aResult.meError));
        return;
    }
    PushDouble(aResult.maValue);
}

void ScInterpreter::ScModalValue()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 ) )
        return;
    std::vector<double> aArray;
    GetNumberSequenceArray( nParamCount, aArray, false );
    if (aArray.empty() || nGlobalError != FormulaError::NONE)
    {
        PushNoValue();
        return;
    }

    const auto aModes = semath::evaluateModeValues(aArray);
    if (!aModes)
    {
        PushError(lcl_ToCalcMathFormulaError(aModes.meError));
        return;
    }
    PushDouble(*std::min_element(aModes.maValue.begin(), aModes.maValue.end()));
}

void ScInterpreter::ScModalValue_MS( bool bSingle )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 ) )
        return;
    std::vector<double> aArray;
    GetNumberSequenceArray( nParamCount, aArray, false );
    if ( aArray.empty() || nGlobalError != FormulaError::NONE )
        PushNoValue();
    else
    {
        const auto aModes = semath::evaluateModeValues(aArray);
        if (!aModes)
        {
            PushError(lcl_ToCalcMathFormulaError(aModes.meError));
            return;
        }

        if ( bSingle )
            PushDouble( aModes.maValue.front() );
        else
        {
            ScMatrixRef pResMatrix = GetNewMat( 1, aModes.maValue.size(), true );
            pResMatrix->PutDoubleVector( aModes.maValue, 0, 0 );
            PushMatrix( pResMatrix );
        }
    }
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

void ScInterpreter::ScLarge()
{
    CalculateSmallLarge(false);
}

void ScInterpreter::ScSmall()
{
    CalculateSmallLarge(true);
}

void ScInterpreter::ScPercentrank( bool bInclusive )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;
    double fSignificance = ( nParamCount == 3 ? ::rtl::math::approxFloor( GetDouble() ) : 3.0 );
    if ( fSignificance < 1.0 )
    {
        PushIllegalArgument();
        return;
    }
    double fNum = GetDouble();
    std::vector<double> aSortArray;
    GetSortArray( 1, aSortArray, nullptr, false, false );
    SCSIZE nSize = aSortArray.size();
    if ( nSize == 0 || nGlobalError != FormulaError::NONE )
        PushNoValue();
    else
    {
        if ( fNum < aSortArray[ 0 ] || fNum > aSortArray[ nSize - 1 ] )
            PushNoValue();
        else
        {
            double fRes;
            if ( nSize == 1 )
                fRes = 1.0;            // fNum == aSortArray[ 0 ], see test above
            else
                fRes = GetPercentrank( aSortArray, fNum, bInclusive );
            if ( fRes != 0.0 )
            {
                double fExp = ::rtl::math::approxFloor( log10( fRes ) ) + 1.0 - fSignificance;
                fRes = ::rtl::math::round( fRes * pow( 10, -fExp ) ) / pow( 10, -fExp );
            }
            PushDouble( fRes );
        }
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

void ScInterpreter::ScTrimMean()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;
    double alpha = GetDouble();
    if (alpha < 0.0 || alpha >= 1.0)
    {
        PushIllegalArgument();
        return;
    }
    std::vector<double> aSortArray;
    GetSortArray( 1, aSortArray, nullptr, false, false );
    SCSIZE nSize = aSortArray.size();
    if (nSize == 0 || nGlobalError != FormulaError::NONE)
        PushNoValue();
    else
    {
        sal_uLong nIndex = static_cast<sal_uLong>(::rtl::math::approxFloor(alpha*static_cast<double>(nSize)));
        if (nIndex % 2 != 0)
            nIndex--;
        nIndex /= 2;
        OSL_ENSURE(nIndex < nSize, "ScTrimMean: wrong index");
        KahanSum fSum = 0.0;
        for (SCSIZE i = nIndex; i < nSize-nIndex; i++)
            fSum += aSortArray[i];
        PushDouble(fSum.get()/static_cast<double>(nSize-2*nIndex));
    }
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

void ScInterpreter::ScRank( bool bAverage )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;
    bool bAscending;
    if ( nParamCount == 3 )
        bAscending = GetBool();
    else
        bAscending = false;

    std::vector<double> aSortArray;
    GetSortArray( 1, aSortArray, nullptr, false, false );
    double fVal = GetDouble();
    SCSIZE nSize = aSortArray.size();
    if ( nSize == 0 || nGlobalError != FormulaError::NONE )
        PushNoValue();
    else
    {
        if ( fVal < aSortArray[ 0 ] || fVal > aSortArray[ nSize - 1 ] )
            PushError( FormulaError::NotAvailable);
        else
        {
            double fLastPos = 0;
            double fFirstPos = -1.0;
            bool bFinished = false;
            SCSIZE i;
            for (i = 0; i < nSize && !bFinished; i++)
            {
                if ( aSortArray[ i ] == fVal )
                {
                    if ( fFirstPos < 0 )
                        fFirstPos = i + 1.0;
                }
                else
                {
                    if ( aSortArray[ i ] > fVal )
                    {
                        fLastPos = i;
                        bFinished = true;
                    }
                }
            }
            if ( !bFinished )
                fLastPos = i;
            if (fFirstPos <= 0)
            {
                PushError( FormulaError::NotAvailable);
            }
            else if ( !bAverage )
            {
                if ( bAscending )
                    PushDouble( fFirstPos );
                else
                    PushDouble( nSize + 1.0 - fLastPos );
            }
            else
            {
                if ( bAscending )
                    PushDouble( ( fFirstPos + fLastPos ) / 2.0 );
                else
                    PushDouble( nSize + 1.0 - ( fFirstPos + fLastPos ) / 2.0 );
            }
        }
    }
}

void ScInterpreter::ScAveDev()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 1 ) )
        return;
    sal_uInt16 SaveSP = sp;
    double nMiddle = 0.0;
    KahanSum rVal = 0.0;
    double rValCount = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    short nParam = nParamCount;
    size_t nRefInList = 0;
    while (nParam-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
                rVal += GetDouble();
                rValCount++;
                break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasNumeric())
                {
                    rVal += GetCellValue(aAdr, aCell);
                    rValCount++;
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                FormulaError nErr = FormulaError::NONE;
                double nCellVal;
                PopDoubleRef( aRange, nParam, nRefInList);
                ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags );
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    rVal += nCellVal;
                    rValCount++;
                    SetError(nErr);
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(nCellVal, nErr))
                    {
                        rVal += nCellVal;
                        rValCount++;
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
                            rVal += pMat->GetDouble(nElem);
                            rValCount++;
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                            if (!pMat->IsStringOrEmpty(nElem))
                            {
                                rVal += pMat->GetDouble(nElem);
                                rValCount++;
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
        return;
    }
    nMiddle = rVal.get() / rValCount;
    sp = SaveSP;
    rVal = 0.0;
    nParam = nParamCount;
    nRefInList = 0;
    while (nParam-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
                rVal += std::abs(GetDouble() - nMiddle);
                break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScRefCellValue aCell(mrDoc, aAdr);
                if (aCell.hasNumeric())
                    rVal += std::abs(GetCellValue(aAdr, aCell) - nMiddle);
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                FormulaError nErr = FormulaError::NONE;
                double nCellVal;
                PopDoubleRef( aRange, nParam, nRefInList);
                ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags );
                if (aValIter.GetFirst(nCellVal, nErr))
                {
                    rVal += std::abs(nCellVal - nMiddle);
                    while (aValIter.GetNext(nCellVal, nErr))
                         rVal += std::abs(nCellVal - nMiddle);
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
                            rVal += std::abs(pMat->GetDouble(nElem) - nMiddle);
                        }
                    }
                    else
                    {
                        for (SCSIZE nElem = 0; nElem < nCount; nElem++)
                        {
                            if (!pMat->IsStringOrEmpty(nElem))
                                rVal += std::abs(pMat->GetDouble(nElem) - nMiddle);
                        }
                    }
                }
            }
            break;
            default : SetError(FormulaError::IllegalParameter); break;
        }
    }
    PushDouble(rVal.get() / rValCount);
}

void ScInterpreter::ScDevSq()
{
    auto VarResult = []( double fVal, size_t /*nValCount*/ )
    {
        return fVal;
    };
    GetStVarParams( false /*bTextAsZero*/, VarResult);
}

void ScInterpreter::ScProbability()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 4 ) )
        return;
    double fUp, fLo;
    fUp = GetDouble();
    if (nParamCount == 4)
        fLo = GetDouble();
    else
        fLo = fUp;
    if (fLo > fUp)
        std::swap( fLo, fUp );
    ScMatrixRef pMatP = GetMatrix();
    ScMatrixRef pMatW = GetMatrix();
    if (!pMatP || !pMatW)
        PushIllegalParameter();
    else
    {
        SCSIZE nC1, nC2;
        SCSIZE nR1, nR2;
        pMatP->GetDimensions(nC1, nR1);
        pMatW->GetDimensions(nC2, nR2);
        if (nC1 != nC2 || nR1 != nR2 || nC1 == 0 || nR1 == 0 ||
            nC2 == 0 || nR2 == 0)
            PushNA();
        else
        {
            KahanSum fSum = 0.0;
            KahanSum fRes = 0.0;
            bool bStop = false;
            double fP, fW;
            for ( SCSIZE i = 0; i < nC1 && !bStop; i++ )
            {
                for (SCSIZE j = 0; j < nR1 && !bStop; ++j )
                {
                    if (pMatP->IsValue(i,j) && pMatW->IsValue(i,j))
                    {
                        fP = pMatP->GetDouble(i,j);
                        fW = pMatW->GetDouble(i,j);
                        if (fP < 0.0 || fP > 1.0)
                            bStop = true;
                        else
                        {
                            fSum += fP;
                            if (fW >= fLo && fW <= fUp)
                                fRes += fP;
                        }
                    }
                    else
                        SetError( FormulaError::IllegalArgument);
                }
            }
            if (bStop || std::abs((fSum -1.0).get()) > 1.0E-7)
                PushNoValue();
            else
                PushDouble(fRes.get());
        }
    }
}

void ScInterpreter::ScCorrel()
{
    // This is identical to ScPearson()
    ScPearson();
}

void ScInterpreter::ScCovarianceP()
{
    CalculatePearsonCovar( false, false, false );
}

void ScInterpreter::ScCovarianceS()
{
    CalculatePearsonCovar( false, false, true );
}

void ScInterpreter::ScPearson()
{
    CalculatePearsonCovar( true, false, false );
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

void ScInterpreter::ScRSQ()
{
    // Same as ScPearson()*ScPearson()
    ScPearson();
    if (nGlobalError != FormulaError::NONE)
        return;

    switch (GetStackType())
    {
        case svDouble:
            {
                double fVal = PopDouble();
                PushDouble( fVal * fVal);
            }
            break;
        default:
            PopError();
            PushNoValue();
    }
}

void ScInterpreter::ScSTEYX()
{
    CalculatePearsonCovar( true, true, false );
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

void ScInterpreter::ScSlope()
{
    CalculateSlopeIntercept(true);
}

void ScInterpreter::ScIntercept()
{
    CalculateSlopeIntercept(false);
}

void ScInterpreter::ScForecast()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
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
    double fVal = GetDouble();
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
            PushDouble( fMeanY + fSumDeltaXDeltaY.get() / fSumSqrDeltaX.get() * (fVal - fMeanX));
    }
}

static void lcl_roundUpNearestPow2(SCSIZE& nNum, SCSIZE& nNumBits)
{
    // Find the least power of 2 that is less than or equal to nNum.
    SCSIZE nPow2(1);
    nNumBits = std::numeric_limits<SCSIZE>::digits;
    nPow2 <<= (nNumBits - 1);
    while (nPow2 >= 1)
    {
        if (nNum & nPow2)
            break;

        --nNumBits;
        nPow2 >>= 1;
    }

    if (nPow2 != nNum)
    {
        assert(nPow2 < 1UL << (std::numeric_limits<unsigned long>::digits - 1));
        nNum = nPow2 ? (nPow2 << 1) : 1;
    }
    else
        --nNumBits;
}

static SCSIZE lcl_bitReverse(SCSIZE nIn, SCSIZE nBound)
{
    SCSIZE nOut = 0;
    for (SCSIZE nMask = 1; nMask < nBound; nMask <<= 1)
    {
        nOut <<= 1;

        if (nIn & nMask)
            nOut |= 1;
    }

    return nOut;
}

namespace {

// Computes and stores twiddle factors for computing DFT later.
struct ScTwiddleFactors
{
    ScTwiddleFactors(SCSIZE nN, bool bInverse) :
        mfWReal(nN),
        mfWImag(nN),
        mnN(nN),
        mbInverse(bInverse)
    {}

    void Compute();

    void Conjugate()
    {
        mbInverse = !mbInverse;
        for (SCSIZE nIdx = 0; nIdx < mnN; ++nIdx)
            mfWImag[nIdx] = -mfWImag[nIdx];
    }

    std::vector<double> mfWReal;
    std::vector<double> mfWImag;
    SCSIZE mnN;
    bool mbInverse;
};

}

void ScTwiddleFactors::Compute()
{
    mfWReal.resize(mnN);
    mfWImag.resize(mnN);

    double nW = (mbInverse ? 2 : -2)*M_PI/static_cast<double>(mnN);

    if (mnN == 1)
    {
        mfWReal[0] = 1.0;
        mfWImag[0] = 0.0;
    }
    else if (mnN == 2)
    {
        mfWReal[0] = 1;
        mfWImag[0] = 0;

        mfWReal[1] = -1;
        mfWImag[1] = 0;
    }
    else if (mnN == 4)
    {
        mfWReal[0] = 1;
        mfWImag[0] = 0;

        mfWReal[1] = 0;
        mfWImag[1] = (mbInverse ? 1.0 : -1.0);

        mfWReal[2] = -1;
        mfWImag[2] = 0;

        mfWReal[3] = 0;
        mfWImag[3] = (mbInverse ? -1.0 : 1.0);
    }
    else if ((mnN % 4) == 0)
    {
        const SCSIZE nQSize = mnN >> 2;
        // Compute cos of the start quadrant.
        // This is the first quadrant if mbInverse == true, else it is the fourth quadrant.
        for (SCSIZE nIdx = 0; nIdx <= nQSize; ++nIdx)
            mfWReal[nIdx] = cos(nW*static_cast<double>(nIdx));

        if (mbInverse)
        {
            const SCSIZE nQ1End = nQSize;
            // First quadrant
            for (SCSIZE nIdx = 0; nIdx <= nQ1End; ++nIdx)
                mfWImag[nIdx] = mfWReal[nQ1End-nIdx];

            // Second quadrant
            const SCSIZE nQ2End = nQ1End << 1;
            for (SCSIZE nIdx = nQ1End+1; nIdx <= nQ2End; ++nIdx)
            {
                mfWReal[nIdx] = -mfWReal[nQ2End - nIdx];
                mfWImag[nIdx] =  mfWImag[nQ2End - nIdx];
            }

            // Third quadrant
            const SCSIZE nQ3End = nQ2End + nQ1End;
            for (SCSIZE nIdx = nQ2End+1; nIdx <= nQ3End; ++nIdx)
            {
                mfWReal[nIdx] = -mfWReal[nIdx - nQ2End];
                mfWImag[nIdx] = -mfWImag[nIdx - nQ2End];
            }

            // Fourth Quadrant
            for (SCSIZE nIdx = nQ3End+1; nIdx < mnN; ++nIdx)
            {
                mfWReal[nIdx] =  mfWReal[mnN - nIdx];
                mfWImag[nIdx] = -mfWImag[mnN - nIdx];
            }
        }
        else
        {
            const SCSIZE nQ4End = nQSize;
            const SCSIZE nQ3End = nQSize << 1;
            const SCSIZE nQ2End = nQ3End + nQSize;

            // Fourth quadrant.
            for (SCSIZE nIdx = 0; nIdx <= nQ4End; ++nIdx)
                mfWImag[nIdx] = -mfWReal[nQ4End - nIdx];

            // Third quadrant.
            for (SCSIZE nIdx = nQ4End+1; nIdx <= nQ3End; ++nIdx)
            {
                mfWReal[nIdx] = -mfWReal[nQ3End - nIdx];
                mfWImag[nIdx] =  mfWImag[nQ3End - nIdx];
            }

            // Second quadrant.
            for (SCSIZE nIdx = nQ3End+1; nIdx <= nQ2End; ++nIdx)
            {
                mfWReal[nIdx] = -mfWReal[nIdx - nQ3End];
                mfWImag[nIdx] = -mfWImag[nIdx - nQ3End];
            }

            // First quadrant.
            for (SCSIZE nIdx = nQ2End+1; nIdx < mnN; ++nIdx)
            {
                mfWReal[nIdx] =  mfWReal[mnN - nIdx];
                mfWImag[nIdx] = -mfWImag[mnN - nIdx];
            }
        }
    }
    else
    {
        for (SCSIZE nIdx = 0; nIdx < mnN; ++nIdx)
        {
            double fAngle = nW*static_cast<double>(nIdx);
            mfWReal[nIdx] = cos(fAngle);
            mfWImag[nIdx] = sin(fAngle);
        }
    }
}

namespace {

// A radix-2 decimation in time FFT algorithm for complex valued input.
class ScComplexFFT2
{
public:
    // rfArray.size() would always be even and a power of 2. (asserted in prepare())
    // rfArray's first half contains the real parts and the later half contains the imaginary parts.
    ScComplexFFT2(std::vector<double>& raArray, bool bInverse, bool bPolar, double fMinMag,
                  ScTwiddleFactors& rTF, bool bSubSampleTFs = false, bool bDisableNormalize = false) :
        mrArray(raArray),
        mfWReal(rTF.mfWReal),
        mfWImag(rTF.mfWImag),
        mnPoints(raArray.size()/2),
        mnStages(0),
        mfMinMag(fMinMag),
        mbInverse(bInverse),
        mbPolar(bPolar),
        mbDisableNormalize(bDisableNormalize),
        mbSubSampleTFs(bSubSampleTFs)
    {}

    void Compute();

private:

    void prepare();

    double getReal(SCSIZE nIdx)
    {
        return mrArray[nIdx];
    }

    void setReal(double fVal, SCSIZE nIdx)
    {
        mrArray[nIdx] = fVal;
    }

    double getImag(SCSIZE nIdx)
    {
        return mrArray[mnPoints + nIdx];
    }

    void setImag(double fVal, SCSIZE nIdx)
    {
        mrArray[mnPoints + nIdx] = fVal;
    }

    SCSIZE getTFactorIndex(SCSIZE nPtIndex, SCSIZE nTfIdxScaleBits)
    {
        return ( ( nPtIndex << nTfIdxScaleBits ) & ( mnPoints - 1 ) ); // (x & (N-1)) is same as (x % N) but faster.
    }

    void computeFly(SCSIZE nTopIdx, SCSIZE nBottomIdx, SCSIZE nWIdx1, SCSIZE nWIdx2)
    {
        if (mbSubSampleTFs)
        {
            nWIdx1 <<= 1;
            nWIdx2 <<= 1;
        }

        const double x1r = getReal(nTopIdx);
        const double x2r = getReal(nBottomIdx);

        const double& w1r = mfWReal[nWIdx1];
        const double& w1i = mfWImag[nWIdx1];

        const double& w2r = mfWReal[nWIdx2];
        const double& w2i = mfWImag[nWIdx2];

        const double x1i = getImag(nTopIdx);
        const double x2i = getImag(nBottomIdx);

        setReal(x1r + x2r*w1r - x2i*w1i, nTopIdx);
        setImag(x1i + x2i*w1r + x2r*w1i, nTopIdx);

        setReal(x1r + x2r*w2r - x2i*w2i, nBottomIdx);
        setImag(x1i + x2i*w2r + x2r*w2i, nBottomIdx);
    }

    std::vector<double>& mrArray;
    std::vector<double>& mfWReal;
    std::vector<double>& mfWImag;
    SCSIZE mnPoints;
    SCSIZE mnStages;
    double mfMinMag;
    bool mbInverse:1;
    bool mbPolar:1;
    bool mbDisableNormalize:1;
    bool mbSubSampleTFs:1;
};

}

void ScComplexFFT2::prepare()
{
    SCSIZE nPoints = mnPoints;
    lcl_roundUpNearestPow2(nPoints, mnStages);
    assert(nPoints == mnPoints);

    // Reorder array by bit-reversed indices.
    for (SCSIZE nIdx = 0; nIdx < mnPoints; ++nIdx)
    {
        SCSIZE nRevIdx = lcl_bitReverse(nIdx, mnPoints);
        if (nIdx < nRevIdx)
        {
            double fTmp = getReal(nIdx);
            setReal(getReal(nRevIdx), nIdx);
            setReal(fTmp, nRevIdx);

            fTmp = getImag(nIdx);
            setImag(getImag(nRevIdx), nIdx);
            setImag(fTmp, nRevIdx);
        }
    }
}

static void lcl_normalize(std::vector<double>& rCmplxArray, bool bScaleOnlyReal)
{
    const SCSIZE nPoints = rCmplxArray.size()/2;
    const double fScale = 1.0/static_cast<double>(nPoints);

    // Scale the real part
    for (SCSIZE nIdx = 0; nIdx < nPoints; ++nIdx)
        rCmplxArray[nIdx] *= fScale;

    if (!bScaleOnlyReal)
    {
        const SCSIZE nLen = nPoints*2;
        for (SCSIZE nIdx = nPoints; nIdx < nLen; ++nIdx)
            rCmplxArray[nIdx] *= fScale;
    }
}

static void lcl_convertToPolar(std::vector<double>& rCmplxArray, double fMinMag)
{
    const SCSIZE nPoints = rCmplxArray.size()/2;
    double fMag, fPhase, fR, fI;
    for (SCSIZE nIdx = 0; nIdx < nPoints; ++nIdx)
    {
        fR = rCmplxArray[nIdx];
        fI = rCmplxArray[nPoints+nIdx];
        fMag = std::hypot(fR, fI);
        if (fMag < fMinMag)
        {
            fMag = 0.0;
            fPhase = 0.0;
        }
        else
        {
            fPhase = atan2(fI, fR);
        }

        rCmplxArray[nIdx] = fMag;
        rCmplxArray[nPoints+nIdx] = fPhase;
    }
}

void ScComplexFFT2::Compute()
{
    prepare();

    const SCSIZE nFliesInStage = mnPoints/2;
    for (SCSIZE nStage = 0; nStage < mnStages; ++nStage)
    {
        const SCSIZE nTFIdxScaleBits = mnStages - nStage - 1;  // Twiddle factor index's scale factor in bits.
        const SCSIZE nFliesInGroup = SCSIZE(1) << nStage;
        const SCSIZE nGroups = nFliesInStage/nFliesInGroup;
        const SCSIZE nFlyWidth = nFliesInGroup;
        for (SCSIZE nGroup = 0, nFlyTopIdx = 0; nGroup < nGroups; ++nGroup)
        {
            for (SCSIZE nFly = 0; nFly < nFliesInGroup; ++nFly, ++nFlyTopIdx)
            {
                SCSIZE nFlyBottomIdx = nFlyTopIdx + nFlyWidth;
                SCSIZE nWIdx1 = getTFactorIndex(nFlyTopIdx, nTFIdxScaleBits);
                SCSIZE nWIdx2 = getTFactorIndex(nFlyBottomIdx, nTFIdxScaleBits);

                computeFly(nFlyTopIdx, nFlyBottomIdx, nWIdx1, nWIdx2);
            }

            nFlyTopIdx += nFlyWidth;
        }
    }

    if (mbPolar)
        lcl_convertToPolar(mrArray, mfMinMag);

    // Normalize after converting to polar, so we have a chance to
    // save O(mnPoints) flops.
    if (mbInverse && !mbDisableNormalize)
        lcl_normalize(mrArray, mbPolar);
}

namespace {

// Bluestein's algorithm or chirp z-transform algorithm that can be used to
// compute DFT of a complex valued input of any length N in O(N lgN) time.
class ScComplexBluesteinFFT
{
public:

    ScComplexBluesteinFFT(std::vector<double>& rArray, bool bReal, bool bInverse,
                          bool bPolar, double fMinMag, bool bDisableNormalize = false) :
        mrArray(rArray),
        mnPoints(rArray.size()/2), // rArray should have space for imaginary parts even if real input.
        mfMinMag(fMinMag),
        mbReal(bReal),
        mbInverse(bInverse),
        mbPolar(bPolar),
        mbDisableNormalize(bDisableNormalize)
    {}

    void Compute();

private:
    std::vector<double>& mrArray;
    const SCSIZE mnPoints;
    double mfMinMag;
    bool mbReal:1;
    bool mbInverse:1;
    bool mbPolar:1;
    bool mbDisableNormalize:1;
};

}

void ScComplexBluesteinFFT::Compute()
{
    std::vector<double> aRealScalars(mnPoints);
    std::vector<double> aImagScalars(mnPoints);
    double fW = (mbInverse ? 2 : -2)*M_PI/static_cast<double>(mnPoints);
    for (SCSIZE nIdx = 0; nIdx < mnPoints; ++nIdx)
    {
        double fAngle = 0.5*fW*static_cast<double>(nIdx*nIdx);
        aRealScalars[nIdx] = cos(fAngle);
        aImagScalars[nIdx] = sin(fAngle);
    }

    SCSIZE nMinSize = mnPoints*2 - 1;
    SCSIZE nExtendedLength = nMinSize, nTmp = 0;
    lcl_roundUpNearestPow2(nExtendedLength, nTmp);
    std::vector<double> aASignal(nExtendedLength*2); // complex valued
    std::vector<double> aBSignal(nExtendedLength*2); // complex valued

    double fReal, fImag;
    for (SCSIZE nIdx = 0; nIdx < mnPoints; ++nIdx)
    {
        // Real part of A signal.
        aASignal[nIdx] = mrArray[nIdx]*aRealScalars[nIdx] + (mbReal ? 0.0 : -mrArray[mnPoints+nIdx]*aImagScalars[nIdx]);
        // Imaginary part of A signal.
        aASignal[nExtendedLength + nIdx] = mrArray[nIdx]*aImagScalars[nIdx] + (mbReal ? 0.0 : mrArray[mnPoints+nIdx]*aRealScalars[nIdx]);

        // Real part of B signal.
        aBSignal[nIdx] = fReal = aRealScalars[nIdx];
        // Imaginary part of B signal.
        aBSignal[nExtendedLength + nIdx] = fImag = -aImagScalars[nIdx]; // negative sign because B signal is the conjugation of the scalars.

        if (nIdx)
        {
            // B signal needs a mirror of its part in 0 < n < mnPoints at the tail end.
            aBSignal[nExtendedLength - nIdx] = fReal;
            aBSignal[(nExtendedLength<<1) - nIdx] = fImag;
        }
    }

    {
        ScTwiddleFactors aTF(nExtendedLength, false /*not inverse*/);
        aTF.Compute();

        // Do complex-FFT2 of both A and B signal.
        ScComplexFFT2 aFFT2A(aASignal, false /*not inverse*/, false /*no polar*/, 0.0 /* no clipping */,
                             aTF, false /*no subsample*/, true /* disable normalize */);
        aFFT2A.Compute();

        ScComplexFFT2 aFFT2B(aBSignal, false /*not inverse*/, false /*no polar*/, 0.0 /* no clipping */,
                             aTF, false /*no subsample*/, true /* disable normalize */);
        aFFT2B.Compute();

        double fAR, fAI, fBR, fBI;
        for (SCSIZE nIdx = 0; nIdx < nExtendedLength; ++nIdx)
        {
            fAR = aASignal[nIdx];
            fAI = aASignal[nExtendedLength + nIdx];
            fBR = aBSignal[nIdx];
            fBI = aBSignal[nExtendedLength + nIdx];

            // Do point-wise product inplace in A signal.
            aASignal[nIdx] = fAR*fBR - fAI*fBI;
            aASignal[nExtendedLength + nIdx] = fAR*fBI + fAI*fBR;
        }

        // Do complex-inverse-FFT2 of aASignal.
        aTF.Conjugate();
        ScComplexFFT2 aFFT2AI(aASignal, true /*inverse*/, false /*no polar*/, 0.0 /* no clipping */, aTF); // Need normalization here.
        aFFT2AI.Compute();
    }

    // Point-wise multiply with scalars.
    for (SCSIZE nIdx = 0; nIdx < mnPoints; ++nIdx)
    {
        fReal = aASignal[nIdx];
        fImag = aASignal[nExtendedLength + nIdx];
        mrArray[nIdx] = fReal*aRealScalars[nIdx] - fImag*aImagScalars[nIdx]; // no conjugation needed here.
        mrArray[mnPoints + nIdx] = fReal*aImagScalars[nIdx] + fImag*aRealScalars[nIdx];
    }

    // Normalize/Polar operations
    if (mbPolar)
        lcl_convertToPolar(mrArray, mfMinMag);

    // Normalize after converting to polar, so we have a chance to
    // save O(mnPoints) flops.
    if (mbInverse && !mbDisableNormalize)
        lcl_normalize(mrArray, mbPolar);
}

namespace {

// Computes DFT of an even length(N) real-valued input by using a
// ScComplexFFT2 if N == 2^k for some k or else by using a ScComplexBluesteinFFT
// with a complex valued input of length = N/2.
class ScRealFFT
{
public:

    ScRealFFT(std::vector<double>& rInArray, std::vector<double>& rOutArray, bool bInverse,
              bool bPolar, double fMinMag) :
        mrInArray(rInArray),
        mrOutArray(rOutArray),
        mfMinMag(fMinMag),
        mbInverse(bInverse),
        mbPolar(bPolar)
    {}

    void Compute();

private:
    std::vector<double>& mrInArray;
    std::vector<double>& mrOutArray;
    double mfMinMag;
    bool mbInverse:1;
    bool mbPolar:1;
};

}

void ScRealFFT::Compute()
{
    // input length has to be even to do this optimization.
    assert(mrInArray.size() % 2 == 0);
    assert(mrInArray.size()*2 == mrOutArray.size());
    // nN is the number of points in the complex-fft input
    // which will be half of the number of points in real array.
    const SCSIZE nN = mrInArray.size()/2;
    if (nN == 0)
    {
        mrOutArray[0] = mrInArray[0];
        mrOutArray[1] = 0.0;
        return;
    }

    // work array should be the same length as mrInArray
    std::vector<double> aWorkArray(nN*2);
    for (SCSIZE nIdx = 0; nIdx < nN; ++nIdx)
    {
        SCSIZE nDoubleIdx = 2*nIdx;
        // Use even elements as real part
        aWorkArray[nIdx] = mrInArray[nDoubleIdx];
        // and odd elements as imaginary part of the contrived complex sequence.
        aWorkArray[nN+nIdx] = mrInArray[nDoubleIdx+1];
    }

    ScTwiddleFactors aTFs(nN*2, mbInverse);
    aTFs.Compute();
    SCSIZE nNextPow2 = nN, nTmp = 0;
    lcl_roundUpNearestPow2(nNextPow2, nTmp);

    if (nNextPow2 == nN)
    {
        ScComplexFFT2 aFFT2(aWorkArray, mbInverse, false /*disable polar*/, 0.0 /* no clipping */,
                            aTFs, true /*subsample tf*/, true /*disable normalize*/);
        aFFT2.Compute();
    }
    else
    {
        ScComplexBluesteinFFT aFFT(aWorkArray, false /*complex input*/, mbInverse, false /*disable polar*/,
                                   0.0 /* no clipping */, true /*disable normalize*/);
        aFFT.Compute();
    }

    // Post process aWorkArray to populate mrOutArray

    const SCSIZE nTwoN = 2*nN, nThreeN = 3*nN;
    double fY1R, fY2R, fY1I, fY2I, fResR, fResI, fWR, fWI;
    for (SCSIZE nIdx = 0; nIdx < nN; ++nIdx)
    {
        const SCSIZE nIdxRev = nIdx ? (nN - nIdx) : 0;
        fY1R = aWorkArray[nIdx];
        fY2R = aWorkArray[nIdxRev];
        fY1I = aWorkArray[nN + nIdx];
        fY2I = aWorkArray[nN + nIdxRev];
        fWR  = aTFs.mfWReal[nIdx];
        fWI  = aTFs.mfWImag[nIdx];

        // mrOutArray has length = 4*nN
        // Real part of the final output (only half of the symmetry around Nyquist frequency)
        // Fills the first quarter.
        mrOutArray[nIdx] = fResR = 0.5*(
            fY1R + fY2R +
            fWR * (fY1I + fY2I) +
            fWI * (fY1R - fY2R) );
        // Imaginary part of the final output (only half of the symmetry around Nyquist frequency)
        // Fills the third quarter.
        mrOutArray[nTwoN + nIdx] = fResI = 0.5*(
            fY1I - fY2I +
            fWI * (fY1I + fY2I) -
            fWR * (fY1R - fY2R) );

        // Fill the missing 2 quarters using symmetry argument.
        if (nIdx)
        {
            // Fills the 2nd quarter.
            mrOutArray[nN + nIdxRev] = fResR;
            // Fills the 4th quarter.
            mrOutArray[nThreeN + nIdxRev] = -fResI;
        }
        else
        {
            mrOutArray[nN] = fY1R - fY1I;
            mrOutArray[nThreeN] = 0.0;
        }
    }

    // Normalize/Polar operations
    if (mbPolar)
        lcl_convertToPolar(mrOutArray, mfMinMag);

    // Normalize after converting to polar, so we have a chance to
    // save O(mnPoints) flops.
    if (mbInverse)
        lcl_normalize(mrOutArray, mbPolar);
}

using ScMatrixGenerator = ScMatrixRef(SCSIZE, SCSIZE, std::vector<double>&);

namespace {

// Generic FFT class that decides which FFT implementation to use.
class ScFFT
{
public:

    ScFFT(ScMatrixRef& pMat, bool bReal, bool bInverse, bool bPolar, double fMinMag) :
        mpInputMat(pMat),
        mfMinMag(fMinMag),
        mbReal(bReal),
        mbInverse(bInverse),
        mbPolar(bPolar)
    {}

    ScMatrixRef Compute(const std::function<ScMatrixGenerator>& rMatGenFunc);

private:
    ScMatrixRef& mpInputMat;
    double mfMinMag;
    bool mbReal:1;
    bool mbInverse:1;
    bool mbPolar:1;
};

}

ScMatrixRef ScFFT::Compute(const std::function<ScMatrixGenerator>& rMatGenFunc)
{
    std::vector<double> aArray;
    mpInputMat->GetDoubleArray(aArray);
    SCSIZE nPoints = mbReal ? aArray.size() : (aArray.size()/2);
    if (nPoints == 1)
    {
        std::vector<double> aOutArray(2);
        aOutArray[0] = aArray[0];
        aOutArray[1] = mbReal ? 0.0 : aArray[1];
        if (mbPolar)
            lcl_convertToPolar(aOutArray, mfMinMag);
        return rMatGenFunc(2, 1, aOutArray);
    }

    if (mbReal && (nPoints % 2) == 0)
    {
        std::vector<double> aOutArray(nPoints*2);
        ScRealFFT aFFT(aArray, aOutArray, mbInverse, mbPolar, mfMinMag);
        aFFT.Compute();
        return rMatGenFunc(2, nPoints, aOutArray);
    }

    SCSIZE nNextPow2 = nPoints, nTmp = 0;
    lcl_roundUpNearestPow2(nNextPow2, nTmp);
    if (nNextPow2 == nPoints && !mbReal)
    {
        ScTwiddleFactors aTF(nPoints, mbInverse);
        aTF.Compute();
        ScComplexFFT2 aFFT2(aArray, mbInverse, mbPolar, mfMinMag, aTF);
        aFFT2.Compute();
        return rMatGenFunc(2, nPoints, aArray);
    }

    if (mbReal)
        aArray.resize(nPoints*2, 0.0);
    ScComplexBluesteinFFT aFFT(aArray, mbReal, mbInverse, mbPolar, mfMinMag);
    aFFT.Compute();
    return rMatGenFunc(2, nPoints, aArray);
}

void ScInterpreter::ScFourier()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 5 ) )
        return;

    bool bInverse = false;
    bool bPolar = false;
    double fMinMag = 0.0;

    if (nParamCount == 5)
    {
        if (IsMissing())
            Pop();
        else
            fMinMag = GetDouble();
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

    bool bGroupedByColumn = GetBool();

    ScMatrixRef pInputMat = GetMatrix();
    if (!pInputMat)
    {
        PushIllegalParameter();
        return;
    }

    SCSIZE nC, nR;
    pInputMat->GetDimensions(nC, nR);

    if ((bGroupedByColumn && nC > 2) || (!bGroupedByColumn && nR > 2))
    {
        // There can be no more than 2 columns (real, imaginary) if data grouped by columns.
        // and no more than 2 rows if data is grouped by rows.
        PushIllegalArgument();
        return;
    }

    if (!pInputMat->IsNumeric())
    {
        PushNoValue();
        return;
    }

    bool bRealInput = true;
    if (!bGroupedByColumn)
    {
        pInputMat->MatTrans(*pInputMat);
        bRealInput = (nR == 1);
    }
    else
    {
        bRealInput = (nC == 1);
    }

    ScFFT aFFT(pInputMat, bRealInput, bInverse, bPolar, fMinMag);
    std::function<ScMatrixGenerator> aFunc = [this](SCSIZE nCol, SCSIZE nRow, std::vector<double>& rVec) -> ScMatrixRef
    {
        return this->GetNewMat(nCol, nRow, rVec);
    };
    ScMatrixRef pOut = aFFT.Compute(aFunc);
    PushMatrix(pOut);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
