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

#include "analysisdefs.hxx"
#include "analysis.hxx"
#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx>

#include <rtl/ustring.hxx>

using namespace sca::analysis;

namespace sefinanceexec = spreadsheetengine::compat::libreoffice::financialaddinexecution;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;

namespace
{
std::vector<double> toDoubleVector(const ScaDoubleList& rValues)
{
    std::vector<double> aValues;
    aValues.reserve(rValues.Count());
    for (sal_uInt32 nIndex = 0; nIndex < rValues.Count(); ++nIndex)
        aValues.push_back(rValues.Get(nIndex));
    return aValues;
}

sefinanceexec::DirectFinancialAddInAdapter makeDateModeFinancialAdapter(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt, sal_Int32 nBasis)
{
    return sefinanceexec::DirectFinancialAddInAdapter(
        getAddInDateServiceContext(xOpt).maHostDate.maNullDate, nBasis);
}

sefinanceexec::DirectFinancialAddInAdapter makeNullDateFinancialAdapter(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt)
{
    return sefinanceexec::DirectFinancialAddInAdapter(
        getAddInDateServiceContext(xOpt).maHostDate.maNullDate);
}

[[noreturn]] void throwDeferredOddFirstCouponFunction(const OUString& rFunctionName)
{
    throw css::uno::RuntimeException(
        u"Analysis add-in function "_ustr + rFunctionName
        + u" remains deferred because odd first coupon pricing and yield do not yet have a shared spreadsheet engine runtime."_ustr);
}
}

double SAL_CALL AnalysisAddIn::getAmordegrc( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    double fCost, sal_Int32 nDate, sal_Int32 nFirstPer, double fRestVal,
    double fPer, double fRate, const css::uno::Any& rOB )
{
    if( nDate > nFirstPer || fRate <= 0.0 || fRestVal > fCost ||
        fCost <= 0.0 || fRestVal < 0 || fPer < 0 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateAmordegrc(fCost, nDate, nFirstPer, fRestVal, fPer, fRate));
}


double SAL_CALL AnalysisAddIn::getAmorlinc( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    double fCost, sal_Int32 nDate, sal_Int32 nFirstPer, double fRestVal,
    double fPer, double fRate, const css::uno::Any& rOB )
{
    if ( nDate > nFirstPer || fRate <= 0.0 || fRestVal > fCost ||
         fCost <= 0.0 || fRestVal < 0 || fPer < 0 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateAmorlinc(fCost, nDate, nFirstPer, fRestVal, fPer, fRate));
}


double SAL_CALL AnalysisAddIn::getAccrint( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nIssue, sal_Int32 /*nFirstInter*/, sal_Int32 nSettle, double fRate,
    const css::uno::Any &rVal, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    double      fVal = aAnyConv.getDouble( xOpt, rVal, 1000.0 );

    if( fRate <= 0.0 || fVal <= 0.0 || isFreqInvalid(nFreq) || nIssue >= nSettle)
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateAccrint(nIssue, nSettle, fRate, fVal, nFreq));
}


double SAL_CALL AnalysisAddIn::getAccrintm( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nIssue, sal_Int32 nSettle, double fRate, const css::uno::Any& rVal, const css::uno::Any& rOB )
{
    double      fVal = aAnyConv.getDouble( xOpt, rVal, 1000.0 );
    if( fRate <= 0.0 || fVal <= 0.0 || nIssue >= nSettle )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateAccrintm(nIssue, nSettle, fRate, fVal));
}


double SAL_CALL AnalysisAddIn::getReceived( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fInvest, double fDisc, const css::uno::Any& rOB )
{
    if( fInvest <= 0.0 || fDisc <= 0.0 || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateReceived(nSettle, nMat, fInvest, fDisc));
}


double SAL_CALL AnalysisAddIn::getDisc( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fPrice, double fRedemp, const css::uno::Any& rOB )
{
    if( fPrice <= 0.0 || fRedemp <= 0.0 || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateDisc(nSettle, nMat, fPrice, fRedemp));
}


double SAL_CALL AnalysisAddIn::getDuration( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fCoup, double fYield, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if( fCoup < 0.0 || fYield < 0.0 || isFreqInvalid(nFreq) || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateDuration(nSettle, nMat, fCoup, fYield, nFreq));
}


double SAL_CALL AnalysisAddIn::getEffect( double fNominal, sal_Int32 nPeriods )
{
    if( nPeriods < 1 || fNominal <= 0.0 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(sefinanceexec::DirectFinancialAddInAdapter::evaluateEffect(
        fNominal, nPeriods));
}


double SAL_CALL AnalysisAddIn::getCumprinc( double fRate, sal_Int32 nNumPeriods, double fVal,
    sal_Int32 nStartPer, sal_Int32 nEndPer, sal_Int32 nPayType )
{
    if( nStartPer < 1 || nEndPer < nStartPer || fRate <= 0.0 || nEndPer > nNumPeriods ||
        fVal <= 0.0 || ( nPayType != 0 && nPayType != 1 ) )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        sefinanceexec::DirectFinancialAddInAdapter::evaluateCumulativePrincipal(
            fRate, nNumPeriods, fVal, nStartPer, nEndPer, nPayType != 0));
}


double SAL_CALL AnalysisAddIn::getCumipmt( double fRate, sal_Int32 nNumPeriods, double fVal,
    sal_Int32 nStartPer, sal_Int32 nEndPer, sal_Int32 nPayType )
{
    if( nStartPer < 1 || nEndPer < nStartPer || fRate <= 0.0 || nEndPer > nNumPeriods ||
        fVal <= 0.0 || ( nPayType != 0 && nPayType != 1 ) )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        sefinanceexec::DirectFinancialAddInAdapter::evaluateCumulativeInterest(
            fRate, nNumPeriods, fVal, nStartPer, nEndPer, nPayType != 0));
}


double SAL_CALL AnalysisAddIn::getPrice( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fRate, double fYield, double fRedemp, sal_Int32 nFreq,
    const css::uno::Any& rOB )
{
    if( fYield < 0.0 || fRate < 0.0 || fRedemp <= 0.0 || isFreqInvalid(nFreq) || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluatePrice(nSettle, nMat, fRate, fYield, fRedemp, nFreq));
}


double SAL_CALL AnalysisAddIn::getPricedisc( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fDisc, double fRedemp, const css::uno::Any& rOB )
{
    if( fDisc <= 0.0 || fRedemp <= 0.0 || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluatePricedisc(nSettle, nMat, fDisc, fRedemp));
}


double SAL_CALL AnalysisAddIn::getPricemat( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nIssue, double fRate, double fYield, const css::uno::Any& rOB )
{
    if( fRate < 0.0 || fYield < 0.0 || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluatePricemat(nSettle, nMat, nIssue, fRate, fYield));
}


double SAL_CALL AnalysisAddIn::getMduration( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fCoup, double fYield, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if( fCoup < 0.0 || fYield < 0.0 || isFreqInvalid(nFreq) || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateModifiedDuration(nSettle, nMat, fCoup, fYield, nFreq));
}


double SAL_CALL AnalysisAddIn::getNominal( double fRate, sal_Int32 nPeriods )
{
    if( fRate <= 0.0 || nPeriods < 0 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        sefinanceexec::DirectFinancialAddInAdapter::evaluateNominal(fRate, nPeriods));
}


double SAL_CALL AnalysisAddIn::getDollarfr( double fDollarDec, sal_Int32 nFrac )
{
    if( nFrac <= 0 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        sefinanceexec::DirectFinancialAddInAdapter::evaluateDollarFraction(fDollarDec, nFrac));
}


double SAL_CALL AnalysisAddIn::getDollarde( double fDollarFrac, sal_Int32 nFrac )
{
    if( nFrac <= 0 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        sefinanceexec::DirectFinancialAddInAdapter::evaluateDollarDecimal(fDollarFrac, nFrac));
}


double SAL_CALL AnalysisAddIn::getYield( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fCoup, double fPrice, double fRedemp, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if( fCoup < 0.0 || fPrice <= 0.0 || fRedemp <= 0.0 || isFreqInvalid(nFreq) || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateYield(nSettle, nMat, fCoup, fPrice, fRedemp, nFreq));
}


double SAL_CALL AnalysisAddIn::getYielddisc( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fPrice, double fRedemp, const css::uno::Any& rOB )
{
    if( fPrice <= 0.0 || fRedemp <= 0.0 || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateYielddisc(nSettle, nMat, fPrice, fRedemp));
}


double SAL_CALL AnalysisAddIn::getYieldmat( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nIssue, double fRate, double fPrice, const css::uno::Any& rOB )
{
    if( fPrice <= 0.0 || fRate < 0.0 || nSettle >= nMat || nSettle < nIssue)
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateYieldmat(nSettle, nMat, nIssue, fRate, fPrice));
}


double SAL_CALL AnalysisAddIn::getTbilleq( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fDisc )
{
    nMat++;
    const auto aHostContext = getAddInDateServiceContext(xOpt);
    sal_Int32 nDiff = static_cast<sal_Int32>(spreadsheetengine::api::calendar::diffDate360(
        aHostContext.maHostDate.maNullDate, nSettle, nMat, false));
    if( fDisc <= 0.0 || nSettle >= nMat || nDiff > 360 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeNullDateFinancialAdapter(xOpt).evaluateTbillEq(nSettle, nMat, fDisc));
}


double SAL_CALL AnalysisAddIn::getTbillprice( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fDisc )
{
    if( fDisc <= 0.0 || nSettle > nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeNullDateFinancialAdapter(xOpt).evaluateTbillPrice(nSettle, nMat, fDisc));
}


double SAL_CALL AnalysisAddIn::getTbillyield( const css::uno::Reference< css::beans::XPropertySet >& xOpt, sal_Int32 nSettle, sal_Int32 nMat, double fPrice )
{
    const auto aHostContext = getAddInDateServiceContext(xOpt);
    sal_Int32 nDiff = static_cast<sal_Int32>(spreadsheetengine::api::calendar::diffDate360(
        aHostContext.maHostDate.maNullDate, nSettle, nMat, false));
    nDiff++;
    if( fPrice <= 0.0 || nSettle >= nMat || nDiff > 360 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeNullDateFinancialAdapter(xOpt).evaluateTbillYield(nSettle, nMat, fPrice));
}

double SAL_CALL AnalysisAddIn::getOddfprice( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nIssue, sal_Int32 nFirstCoup,
    double fRate, double fYield, double fRedemp, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if( fRate < 0.0 || fYield < 0.0 || isFreqInvalid(nFreq) || nMat <= nFirstCoup || nFirstCoup <= nSettle || nSettle <= nIssue )
        throw css::lang::IllegalArgumentException();
    (void)xOpt;
    (void)rOB;
    (void)fRedemp;
    throwDeferredOddFirstCouponFunction(u"ODDFPRICE"_ustr);
}

double SAL_CALL AnalysisAddIn::getOddfyield( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nIssue, sal_Int32 nFirstCoup,
    double fRate, double fPrice, double fRedemp, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if( fRate < 0.0 || fPrice <= 0.0 || isFreqInvalid(nFreq) || nMat <= nFirstCoup || nFirstCoup <= nSettle || nSettle <= nIssue )
        throw css::lang::IllegalArgumentException();
    (void)xOpt;
    (void)rOB;
    (void)fRedemp;
    throwDeferredOddFirstCouponFunction(u"ODDFYIELD"_ustr);
}

double SAL_CALL AnalysisAddIn::getOddlprice( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nLastInterest,
    double fRate, double fYield, double fRedemp, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if( fRate <= 0.0 || fYield < 0.0 || fRedemp <= 0.0 || isFreqInvalid(nFreq) || nMat <= nSettle || nSettle <= nLastInterest )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateOddlprice(
                                nSettle, nMat, nLastInterest, fRate, fYield, fRedemp, nFreq));
}


double SAL_CALL AnalysisAddIn::getOddlyield( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nLastInterest,
    double fRate, double fPrice, double fRedemp, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if( fRate <= 0.0 || fPrice <= 0.0 || fRedemp <= 0.0 || isFreqInvalid(nFreq) || nMat <= nSettle || nSettle <= nLastInterest )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateOddlyield(
                                nSettle, nMat, nLastInterest, fRate, fPrice, fRedemp, nFreq));
}

double SAL_CALL AnalysisAddIn::getXirr(
    const css::uno::Reference< css::beans::XPropertySet >& xOpt, const css::uno::Sequence< css::uno::Sequence< double > >& rValues, const css::uno::Sequence< css::uno::Sequence< sal_Int32 > >& rDates, const css::uno::Any& rGuessRate )
{
    ScaDoubleList aValues, aDates;
    aValues.Append( rValues );
    aDates.Append( rDates );

    if( (aValues.Count() < 2) || (aValues.Count() != aDates.Count()) )
        throw css::lang::IllegalArgumentException();

    const double fGuessRate = aAnyConv.getDouble( xOpt, rGuessRate, 0.1 );
    if( fGuessRate <= -1 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(sefinanceexec::DirectFinancialAddInAdapter::evaluateXirrNumbers(
        toDoubleVector(aValues), selibreoffice::toApiDateSerials(toDoubleVector(aDates)),
        fGuessRate));
}


double SAL_CALL AnalysisAddIn::getXnpv(
    double fRate, const css::uno::Sequence< css::uno::Sequence< double > >& rValues, const css::uno::Sequence< css::uno::Sequence< sal_Int32 > >& rDates )
{
    ScaDoubleList aValList;
    ScaDoubleList aDateList;

    aValList.Append( rValues );
    aDateList.Append( rDates );

    sal_uInt32 nNum = aValList.Count();
    if( nNum != aDateList.Count() || nNum < 2 )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(sefinanceexec::DirectFinancialAddInAdapter::evaluateXnpvNumbers(
        fRate, toDoubleVector(aValList), selibreoffice::toApiDateSerials(toDoubleVector(aDateList))));
}


double SAL_CALL AnalysisAddIn::getIntrate( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, double fInvest, double fRedemp, const css::uno::Any& rOB )
{
    if( fInvest <= 0.0 || fRedemp <= 0.0 || nSettle >= nMat )
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
                            .evaluateIntrate(nSettle, nMat, fInvest, fRedemp));
}


double SAL_CALL AnalysisAddIn::getCoupncd( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if (isFreqInvalid(nFreq))
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateCoupncd(nSettle, nMat, nFreq));
}


double SAL_CALL AnalysisAddIn::getCoupdays( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if (isFreqInvalid(nFreq))
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateCoupdays(nSettle, nMat, nFreq));
}


double SAL_CALL AnalysisAddIn::getCoupdaysnc( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if (isFreqInvalid(nFreq))
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateCoupdaysnc(nSettle, nMat, nFreq));
}


double SAL_CALL AnalysisAddIn::getCoupdaybs( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if (isFreqInvalid(nFreq))
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateCoupdaybs(nSettle, nMat, nFreq));
}


double SAL_CALL AnalysisAddIn::getCouppcd( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if (isFreqInvalid(nFreq))
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateCouppcd(nSettle, nMat, nFreq));
}


double SAL_CALL AnalysisAddIn::getCoupnum( const css::uno::Reference< css::beans::XPropertySet >& xOpt,
    sal_Int32 nSettle, sal_Int32 nMat, sal_Int32 nFreq, const css::uno::Any& rOB )
{
    if (isFreqInvalid(nFreq))
        throw css::lang::IllegalArgumentException();
    return valueOrThrow(
        makeDateModeFinancialAdapter(xOpt, getDateMode(xOpt, rOB))
            .evaluateCoupnum(nSettle, nMat, nFreq));
}


double SAL_CALL AnalysisAddIn::getFvschedule( double fPrinc, const css::uno::Sequence< css::uno::Sequence< double > >& rSchedule )
{
    ScaDoubleList aSchedList;

    aSchedList.Append( rSchedule );
    return valueOrThrow(
        sefinanceexec::DirectFinancialAddInAdapter::evaluateFutureValueSchedule(
            fPrinc, toDoubleVector(aSchedList)));
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
