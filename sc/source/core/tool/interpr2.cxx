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
#include <spreadsheetengine/core/DateTimeParts.hxx>
#include <spreadsheetengine/core/DateTimeWeek.hxx>
#include <spreadsheetengine/core/DateTimeWorkday.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/core/MathFinancial.hxx>
#include <spreadsheetengine/core/MathRounding.hxx>
#include <spreadsheetengine/core/MathScalar.hxx>
#include <spreadsheetengine/core/NumeralConversion.hxx>
#include <spreadsheetengine/compat/libreoffice/Date.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

#include <com/sun/star/sheet/DataPilotFieldFilter.hpp>

#include <string.h>

using namespace com::sun::star;
using namespace formula;
namespace sedatetime = spreadsheetengine::core::datetime;
namespace semath = spreadsheetengine::core::math;
namespace seconvert = spreadsheetengine::core::convert;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;

#define SCdEpsilon                1.0E-7

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

void ScInterpreter::ScGetActDate()
{
    nFuncFmtType = SvNumFormatType::DATE;
    Date aActDate( Date::SYSTEM );
    tools::Long nDiff = aActDate - mrContext.NFGetNullDate();
    PushDouble(static_cast<double>(nDiff));
}

void ScInterpreter::ScGetActTime()
{
    nFuncFmtType = SvNumFormatType::DATETIME;
    DateTime aActTime( DateTime::SYSTEM );
    tools::Long nDiff = aActTime - mrContext.NFGetNullDate();
    double fTime = aActTime.GetHour()    / static_cast<double>(::tools::Time::hourPerDay)   +
                   aActTime.GetMin()     / static_cast<double>(::tools::Time::minutePerDay) +
                   aActTime.GetSec()     / static_cast<double>(::tools::Time::secondPerDay) +
                   aActTime.GetNanoSec() / static_cast<double>(::tools::Time::nanoSecPerDay);
    PushDouble( static_cast<double>(nDiff) + fTime );
}

void ScInterpreter::ScGetYear()
{
    PushDouble(sedatetime::extractYear(
        selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32()));
}

void ScInterpreter::ScGetMonth()
{
    PushDouble(sedatetime::extractMonth(
        selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32()));
}

void ScInterpreter::ScGetDay()
{
    if (std::optional<double> fDay
        = sedatetime::extractDay(selibreoffice::toApiDateParts(mrContext.NFGetNullDate()),
              GetFloor32()))
        PushDouble(*fDay);
    else
    {
        SetError(FormulaError::IllegalArgument);
        PushDouble(HUGE_VAL);
    }
}

void ScInterpreter::ScGetMin()
{
    PushDouble(sedatetime::extractMinute(GetDouble()));
}

void ScInterpreter::ScGetSec()
{
    PushDouble(sedatetime::extractSecond(GetDouble()));
}

void ScInterpreter::ScGetHour()
{
    PushDouble(sedatetime::extractHour(GetDouble()));
}

void ScInterpreter::ScGetDateValue()
{
    OUString aInputString = GetString().getString();
    const auto aResult = selibreoffice::parseDateValueFromText(mrDoc, mrContext, aInputString);
    if (aResult)
    {
        nFuncFmtType = SvNumFormatType::DATE;
        PushDouble(aResult.maValue);
    }
    else
        PushIllegalArgument();
}

void ScInterpreter::ScGetDayOfWeek()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    sal_Int16 nFlag;
    if (nParamCount == 2)
        nFlag = GetInt16();
    else
        nFlag = 1;

    const auto aResult
        = sedatetime::computeDayOfWeek(
            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32(), nFlag);
    if (!aResult.mbValid)
        SetError(FormulaError::IllegalArgument);
    PushInt(aResult.mnValue);
}

void ScInterpreter::ScWeeknumOOo()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        sal_Int16 nFlag = GetInt16();
        PushInt(sedatetime::computeWeeknumOOo(
            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32(), nFlag));
    }
}

void ScInterpreter::ScGetWeekOfYear()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    sal_Int16 nFlag = (nParamCount == 1) ? 1 : GetInt16WithDefault(1);
    if (std::optional<int> nWeek
        = sedatetime::computeWeekOfYear(
            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32(), nFlag))
        PushInt(*nWeek);
    else
        PushIllegalArgument();
}

void ScInterpreter::ScGetIsoWeekOfYear()
{
    if ( MustHaveParamCount( GetByte(), 1 ) )
        PushInt(sedatetime::computeIsoWeekOfYear(
            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), GetFloor32()));
}

void ScInterpreter::ScEasterSunday()
{
    nFuncFmtType = SvNumFormatType::DATE;
    if ( !MustHaveParamCount( GetByte(), 1 ) )
        return;

    sal_Int16 nYear = GetInt16();
    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;
    }
    if ( nYear < 100 )
        nYear = mrContext.NFExpandTwoDigitYear( nYear );
    if (std::optional<double> fSerial
        = sedatetime::computeEasterSundaySerial(
            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), nYear))
        PushDouble(*fSerial);
    else
        PushIllegalArgument();
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

void ScInterpreter::ScNetWorkdays( bool bOOXML_Version )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 4 ) )
        return;

    std::vector<double> nSortArray;
    bool bWeekendMask[ 7 ];
    const Date& rNullDate = mrContext.NFGetNullDate();
    sal_Int32 nNullDate = rNullDate.GetAsNormalizedDays();
    FormulaError nErr;
    if ( bOOXML_Version )
    {
        nErr = GetWeekendAndHolidayMasks_MS( nParamCount, nNullDate,
                        nSortArray, bWeekendMask, false );
    }
    else
    {
        nErr = GetWeekendAndHolidayMasks( nParamCount, nNullDate,
                        nSortArray, bWeekendMask );
    }
    if ( nErr != FormulaError::NONE )
        PushError( nErr );
    else
    {
        sal_Int32 nDate2 = GetFloor32();
        sal_Int32 nDate1 = GetFloor32();
        if (nGlobalError != FormulaError::NONE || (nDate1 > SAL_MAX_INT32 - nNullDate) || nDate2 > (SAL_MAX_INT32 - nNullDate))
        {
            PushIllegalArgument();
            return;
        }
        nDate2 += nNullDate;
        nDate1 += nNullDate;
        const auto aHolidaySerials = selibreoffice::toApiDateSerials(nSortArray);
        PushDouble(static_cast<double>(
            sedatetime::countWorkdays(
                nDate1, nDate2, aHolidaySerials,
                selibreoffice::toApiWeekendMask(bWeekendMask))));
    }
}

void ScInterpreter::ScWorkday_MS()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 4 ) )
        return;

    nFuncFmtType = SvNumFormatType::DATE;
    std::vector<double> nSortArray;
    bool bWeekendMask[ 7 ];
    const Date& rNullDate = mrContext.NFGetNullDate();
    sal_Int32 nNullDate = rNullDate.GetAsNormalizedDays();
    FormulaError nErr = GetWeekendAndHolidayMasks_MS( nParamCount, nNullDate,
                        nSortArray, bWeekendMask, true );
    if ( nErr != FormulaError::NONE )
        PushError( nErr );
    else
    {
        sal_Int32 nDays = GetFloor32();
        sal_Int32 nDate = GetFloor32();
        if (nGlobalError != FormulaError::NONE || (nDate > SAL_MAX_INT32 - nNullDate))
        {
            PushIllegalArgument();
            return;
        }
        nDate += nNullDate;

        if ( !nDays )
            PushDouble( static_cast<double>( nDate - nNullDate ) );
        else
        {
            const auto aHolidaySerials = selibreoffice::toApiDateSerials(nSortArray);
            PushDouble(static_cast<double>(
                sedatetime::advanceWorkday(
                    nDate, nDays, aHolidaySerials,
                    selibreoffice::toApiWeekendMask(bWeekendMask))
                - nNullDate));
        }
    }
}

void ScInterpreter::ScGetDate()
{
    nFuncFmtType = SvNumFormatType::DATE;
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;

    sal_Int16 nDay   = GetInt16();
    sal_Int16 nMonth = GetInt16();
    if (IsMissing())
        SetError( FormulaError::ParameterExpected);    // Year must be given.
    sal_Int16 nYear  = GetInt16();
    if (nGlobalError != FormulaError::NONE || nYear < 0)
        PushIllegalArgument();
    else
        PushDouble(GetDateSerial(nYear, nMonth, nDay, false));
}

void ScInterpreter::ScGetTime()
{
    nFuncFmtType = SvNumFormatType::TIME;
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double fSec = GetDouble();
        double fMin = GetDouble();
        double fHour = GetDouble();
        if (std::optional<double> fTime = sedatetime::makeTimeSerial(fHour, fMin, fSec))
            PushDouble(*fTime);
        else
            PushIllegalArgument();
    }
}

void ScInterpreter::ScGetDiffDate()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        double fDate2 = GetDouble();
        double fDate1 = GetDouble();
        PushDouble(sedatetime::computeDiffDate(fDate1, fDate2));
    }
}

void ScInterpreter::ScGetDiffDate360()
{
    /* Implementation follows
     * http://www.bondmarkets.com/eCommerce/SMD_Fields_030802.pdf
     * Appendix B: Day-Count Bases, there are 7 different ways to calculate the
     * 30-days count. That document also claims that Excel implements the "PSA
     * 30" or "NASD 30" method (funny enough they also state that Excel is the
     * only tool that does so).
     *
     * Note that the definition given in
     * http://msdn.microsoft.com/library/en-us/office97/html/SEB7C.asp
     * is _not_ the way how it is actually calculated by Excel (that would not
     * even match any of the 7 methods mentioned above) and would result in the
     * following test cases producing wrong results according to that appendix B:
     *
     * 28-Feb-95  31-Aug-95  181 instead of 180
     * 29-Feb-96  31-Aug-96  181 instead of 180
     * 30-Jan-96  31-Mar-96   61 instead of  60
     * 31-Jan-96  31-Mar-96   61 instead of  60
     *
     * Still, there is a difference between OOoCalc and Excel:
     * In Excel:
     * 02-Feb-99 31-Mar-00 results in  419
     * 31-Mar-00 02-Feb-99 results in -418
     * In Calc the result is 419 respectively -419. I consider the -418 a bug in Excel.
     */

    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;

    bool bFlag = nParamCount == 3 && GetBool();
    sal_Int32 nDate2 = GetFloor32();
    sal_Int32 nDate1 = GetFloor32();
    if (nGlobalError != FormulaError::NONE)
        PushError( nGlobalError);
    else
        PushDouble(sedatetime::computeDiffDate360(
            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), nDate1, nDate2, bFlag));
}

// fdo#44456 function DATEDIF as defined in ODF1.2 (Par. 6.10.3)
void ScInterpreter::ScGetDateDif()
{
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;

    OUString aInterval = GetString().getString();
    sal_Int32 nDate2 = GetFloor32();
    sal_Int32 nDate1 = GetFloor32();

    if (nGlobalError != FormulaError::NONE)
    {
        PushError( nGlobalError);
        return;
    }

    if (std::optional<double> fResult
        = sedatetime::computeDateDif(
            selibreoffice::toApiDateParts(mrContext.NFGetNullDate()), nDate1, nDate2,
            selibreoffice::toApiString(aInterval)))
        PushDouble(*fResult);
    else
        PushIllegalArgument();               // unsupported format
}

void ScInterpreter::ScGetTimeValue()
{
    OUString aInputString = GetString().getString();
    const auto aResult = selibreoffice::parseTimeValueFromText(mrDoc, mrContext, aInputString);
    if (aResult)
    {
        nFuncFmtType = SvNumFormatType::TIME;
        PushDouble(aResult.maValue);
    }
    else
        PushIllegalArgument();
}

void ScInterpreter::ScPlusMinus()
{
    PushInt( semath::computePlusMinus( GetDouble() ) );
}

void ScInterpreter::ScAbs()
{
    PushDouble( semath::computeAbs( GetDouble() ) );
}

void ScInterpreter::ScInt()
{
    PushDouble( semath::computeInt( GetDouble() ) );
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

void ScInterpreter::ScRound()
{
    RoundNumber( rtl_math_RoundingMode_Corrected );
}

void ScInterpreter::ScRoundDown()
{
    RoundNumber( rtl_math_RoundingMode_Down );
}

void ScInterpreter::ScRoundUp()
{
    RoundNumber( rtl_math_RoundingMode_Up );
}

void ScInterpreter::RoundSignificant( double fX, double fDigits, double &fRes )
{
    fRes = semath::roundToSignificantDigits( fX, fDigits );
}

// tdf#105931
void ScInterpreter::ScRoundSignificant()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;

    double fDigits = ::rtl::math::approxFloor( GetDouble() );
    double fX = GetDouble();
    if ( nGlobalError != FormulaError::NONE || fDigits < 1.0 )
    {
        PushIllegalArgument();
        return;
    }

    if ( fX == 0.0 )
        PushDouble( 0.0 );
    else
    {
        double fRes;
        RoundSignificant( fX, fDigits, fRes );
        PushDouble( fRes );
    }
}

/** tdf69552 ODFF1.2 function CEILING and Excel function CEILING.MATH
    In essence, the difference between the two is that ODFF-CEILING needs to
    have arguments value and significance of the same sign and with
    CEILING.MATH the sign of argument significance is irrevelevant.
    This is why ODFF-CEILING is exported to Excel as CEILING.MATH and
    CEILING.MATH is imported in Calc as CEILING.MATH
 */
void ScInterpreter::ScCeil( bool bODFF )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 3 ) )
        return;

    bool bAbs = nParamCount == 3 && GetBool();
    double fDec, fVal;
    if ( nParamCount == 1 )
    {
        fVal = GetDouble();
        fDec = ( fVal < 0 ? -1 : 1 );
    }
    else
    {
        bool bArgumentMissing = IsMissing();
        fDec = GetDouble();
        fVal = GetDouble();
        if ( bArgumentMissing )
            fDec = ( fVal < 0 ? -1 : 1 );
    }
    if ( fVal == 0 || fDec == 0.0 )
        PushInt( 0 );
    else
    {
        if (std::optional<double> fResult = semath::computeCeiling( fVal, fDec, bAbs, bODFF ))
            PushDouble(*fResult);
        else
            PushIllegalArgument();
    }
}

void ScInterpreter::ScCeil_MS()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2 ) )
        return;

    double fDec = GetDouble();
    double fVal = GetDouble();
    if (std::optional<double> fResult = semath::computeCeilingMs( fVal, fDec ))
        PushDouble(*fResult);
    else
        PushIllegalArgument();
}

void ScInterpreter::ScCeil_Precise()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    double fDec, fVal;
    if ( nParamCount == 1 )
    {
        fVal = GetDouble();
        fDec = 1.0;
    }
    else
    {
        fDec = std::abs( GetDoubleWithDefault( 1.0 ));
        fVal = GetDouble();
    }
    if ( fDec == 0.0 || fVal == 0.0 )
        PushInt( 0 );
    else
        PushDouble(semath::computeCeilingPrecise( fVal, fDec ));
}

/** tdf69552 ODFF1.2 function FLOOR and Excel function FLOOR.MATH
    In essence, the difference between the two is that ODFF-FLOOR needs to
    have arguments value and significance of the same sign and with
    FLOOR.MATH the sign of argument significance is irrevelevant.
    This is why ODFF-FLOOR is exported to Excel as FLOOR.MATH and
    FLOOR.MATH is imported in Calc as FLOOR.MATH
 */
void ScInterpreter::ScFloor( bool bODFF )
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 3 ) )
        return;

    bool bAbs = ( nParamCount == 3 && GetBool() );
    double fDec, fVal;
    if ( nParamCount == 1 )
    {
        fVal = GetDouble();
        fDec = ( fVal < 0 ? -1 : 1 );
    }
    else
    {
        bool bArgumentMissing = IsMissing();
        fDec = GetDouble();
        fVal = GetDouble();
        if ( bArgumentMissing )
            fDec = ( fVal < 0 ? -1 : 1 );
    }
    if ( fDec == 0.0 || fVal == 0.0 )
        PushInt( 0 );
    else
    {
        if (std::optional<double> fResult = semath::computeFloor( fVal, fDec, bAbs, bODFF ))
            PushDouble(*fResult);
        else
            PushIllegalArgument();
    }
}

void ScInterpreter::ScFloor_MS()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2 ) )
        return;

    double fDec = GetDouble();
    double fVal = GetDouble();
    if (std::optional<double> fResult = semath::computeFloorMs( fVal, fDec ))
        PushDouble(*fResult);
    else
        PushIllegalArgument();
}

void ScInterpreter::ScFloor_Precise()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    double fDec = nParamCount == 1 ? 1.0 : std::abs( GetDoubleWithDefault( 1.0 ) );
    double fVal = GetDouble();
    if ( fDec == 0.0 || fVal == 0.0 )
        PushInt( 0 );
    else
        PushDouble(semath::computeFloorPrecise( fVal, fDec ));
}

void ScInterpreter::ScEven()
{
    PushDouble(semath::computeEven(GetDouble()));
}

void ScInterpreter::ScOdd()
{
    PushDouble(semath::computeOdd(GetDouble()));
}

void ScInterpreter::ScArcTan2()
{
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        double fVal2 = GetDouble();
        double fVal1 = GetDouble();
        PushDouble( semath::computeArcTan2( fVal2, fVal1 ) );
    }
}

void ScInterpreter::ScLog()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    double fBase = nParamCount == 2 ? GetDouble() : 10.0;
    double fVal = GetDouble();
    if (std::optional<double> fResult = semath::computeLog( fVal, fBase ))
        PushDouble(*fResult);
    else
        PushIllegalArgument();
}

void ScInterpreter::ScLn()
{
    if (std::optional<double> fResult = semath::computeLn( GetDouble() ))
        PushDouble(*fResult);
    else
        PushIllegalArgument();
}

void ScInterpreter::ScLog10()
{
    if (std::optional<double> fResult = semath::computeLog10( GetDouble() ))
        PushDouble(*fResult);
    else
        PushIllegalArgument();
}

void ScInterpreter::ScNPV()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    short nParamCount = GetByte();
    if ( !MustHaveParamCountMin( nParamCount, 2) )
        return;

    KahanSum fVal = 0.0;
    // We turn the stack upside down!
    ReverseStack( nParamCount);
    if (nGlobalError == FormulaError::NONE)
    {
        double  fCount = 1.0;
        double  fRate = GetDouble();
        --nParamCount;
        size_t nRefInList = 0;
        ScRange aRange;
        while (nParamCount-- > 0)
        {
            switch (GetStackType())
            {
                case svDouble :
                {
                    fVal += GetDouble() / pow(1.0 + fRate, fCount);
                    fCount++;
                }
                break;
                case svSingleRef :
                {
                    ScAddress aAdr;
                    PopSingleRef( aAdr );
                    ScRefCellValue aCell(mrDoc, aAdr);
                    if (!aCell.hasEmptyValue() && aCell.hasNumeric())
                    {
                        double fCellVal = GetCellValue(aAdr, aCell);
                        fVal += fCellVal / pow(1.0 + fRate, fCount);
                        fCount++;
                    }
                }
                break;
                case svDoubleRef :
                case svRefList :
                {
                    FormulaError nErr = FormulaError::NONE;
                    double fCellVal;
                    PopDoubleRef( aRange, nParamCount, nRefInList);
                    ScHorizontalValueIterator aValIter( mrDoc, aRange );
                    while ((nErr == FormulaError::NONE) && aValIter.GetNext(fCellVal, nErr))
                    {
                        fVal += fCellVal / pow(1.0 + fRate, fCount);
                        fCount++;
                    }
                    if ( nErr != FormulaError::NONE )
                        SetError(nErr);
                }
                break;
                case svMatrix :
                case svExternalSingleRef:
                case svExternalDoubleRef:
                {
                    ScMatrixRef pMat = GetMatrix();
                    if (pMat)
                    {
                        SCSIZE nC, nR;
                        pMat->GetDimensions(nC, nR);
                        if (nC == 0 || nR == 0)
                        {
                            PushIllegalArgument();
                            return;
                        }
                        else
                        {
                            double fx;
                            for ( SCSIZE j = 0; j < nC; j++ )
                            {
                                for (SCSIZE k = 0; k < nR; ++k)
                                {
                                    if (!pMat->IsValue(j,k))
                                    {
                                        PushIllegalArgument();
                                        return;
                                    }
                                    fx = pMat->GetDouble(j,k);
                                    fVal += fx / pow(1.0 + fRate, fCount);
                                    fCount++;
                                }
                            }
                        }
                    }
                }
                break;
                default : SetError(FormulaError::IllegalParameter); break;
            }
        }
    }
    PushDouble(fVal.get());
}

void ScInterpreter::ScIRR()
{
    nFuncFmtType = SvNumFormatType::PERCENT;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;
    double fEstimated = nParamCount == 2 ? GetDouble() : 0.1;
    double fEps = 1.0;
    // If it's -1 the default result for division by zero else startvalue
    double x = fEstimated == -1.0 ? 0.1 : fEstimated;
    double fValue;

    ScRange aRange;
    ScMatrixRef pMat;
    SCSIZE nC = 0;
    SCSIZE nR = 0;
    bool bIsMatrix = false;
    switch (GetStackType())
    {
        case svDoubleRef:
            PopDoubleRef(aRange);
        break;
        case svMatrix:
        case svExternalSingleRef:
        case svExternalDoubleRef:
            pMat = GetMatrix();
            if (pMat)
            {
                pMat->GetDimensions(nC, nR);
                if (nC == 0 || nR == 0)
                {
                    PushIllegalParameter();
                    return;
                }
                bIsMatrix = true;
            }
            else
            {
                PushIllegalParameter();
                return;
            }
            break;
        default:
        {
            PushIllegalParameter();
            return;
        }
    }
    const sal_uInt16 nIterationsMax = 20;
    sal_uInt16 nItCount = 0;
    FormulaError nIterError = FormulaError::NONE;
    while (fEps > SCdEpsilon && nItCount < nIterationsMax && nGlobalError == FormulaError::NONE)
    {                                       // Newtons method:
        KahanSum fNom = 0.0;
        KahanSum fDenom = 0.0;
        double fCount = 0.0;
        if (bIsMatrix)
        {
            for (SCSIZE j = 0; j < nC && nGlobalError == FormulaError::NONE; j++)
            {
                for (SCSIZE k = 0; k < nR; k++)
                {
                    if (!pMat->IsValue(j, k))
                        continue;
                    fValue = pMat->GetDouble(j, k);
                    if (nGlobalError != FormulaError::NONE)
                        break;

                    fNom   +=           fValue / pow(1.0+x,fCount);
                    fDenom += -fCount * fValue / pow(1.0+x,fCount+1.0);
                    fCount++;
                }
            }
        }
        else
        {
            ScValueIterator aValIter(mrContext, aRange, mnSubTotalFlags);
            bool bLoop = aValIter.GetFirst(fValue, nIterError);
            while (bLoop && nIterError == FormulaError::NONE)
            {
                fNom   +=           fValue / pow(1.0+x,fCount);
                fDenom += -fCount * fValue / pow(1.0+x,fCount+1.0);
                fCount++;

                bLoop = aValIter.GetNext(fValue, nIterError);
            }
            SetError(nIterError);
        }
        double xNew = x - o3tl::div_allow_zero(fNom.get(), fDenom.get());  // x(i+1) = x(i)-f(x(i))/f'(x(i))
        nItCount++;
        fEps = std::abs(xNew - x);
        x = xNew;
    }
    if (fEstimated == 0.0 && std::abs(x) < SCdEpsilon)
        x = 0.0;                        // adjust to zero
    if (fEps < SCdEpsilon)
        PushDouble(x);
    else
        PushError( FormulaError::NoConvergence);
}

void ScInterpreter::ScMIRR()
{   // range_of_values ; rate_invest ; rate_reinvest
    nFuncFmtType = SvNumFormatType::PERCENT;
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;

    double fRate1_reinvest = GetDouble() + 1;
    double fRate1_invest = GetDouble() + 1;

    ScRange aRange;
    ScMatrixRef pMat;
    SCSIZE nC = 0;
    SCSIZE nR = 0;
    bool bIsMatrix = false;
    switch ( GetStackType() )
    {
        case svDoubleRef :
            PopDoubleRef( aRange );
            break;
        case svMatrix :
        case svExternalSingleRef:
        case svExternalDoubleRef:
            {
                pMat = GetMatrix();
                if ( pMat )
                {
                    pMat->GetDimensions( nC, nR );
                    if ( nC == 0 || nR == 0 )
                        SetError( FormulaError::IllegalArgument );
                    bIsMatrix = true;
                }
                else
                    SetError( FormulaError::IllegalArgument );
            }
            break;
        default :
            SetError( FormulaError::IllegalParameter );
            break;
    }

    if ( nGlobalError != FormulaError::NONE )
        PushError( nGlobalError );
    else
    {
        KahanSum fNPV_reinvest = 0.0;
        double fPow_reinvest = 1.0;
        KahanSum fNPV_invest = 0.0;
        double fPow_invest = 1.0;
        sal_uLong nCount = 0;
        bool bHasPosValue = false;
        bool bHasNegValue = false;

        if ( bIsMatrix )
        {
            double fX;
            for ( SCSIZE j = 0; j < nC; j++ )
            {
                for ( SCSIZE k = 0; k < nR; ++k )
                {
                    if ( !pMat->IsValue( j, k ) )
                        continue;
                    fX = pMat->GetDouble( j, k );
                    if ( nGlobalError != FormulaError::NONE )
                        break;

                    if ( fX > 0.0 )
                    {    // reinvestments
                        bHasPosValue = true;
                        fNPV_reinvest += fX * fPow_reinvest;
                    }
                    else if ( fX < 0.0 )
                    {   // investments
                        bHasNegValue = true;
                        fNPV_invest += fX * fPow_invest;
                    }
                    fPow_reinvest /= fRate1_reinvest;
                    fPow_invest /= fRate1_invest;
                    nCount++;
                }
            }
        }
        else
        {
            ScValueIterator aValIter( mrContext, aRange, mnSubTotalFlags );
            double fCellValue;
            FormulaError nIterError = FormulaError::NONE;

            bool bLoop = aValIter.GetFirst( fCellValue, nIterError );
            while( bLoop )
            {
                if( fCellValue > 0.0 )          // reinvestments
                {    // reinvestments
                    bHasPosValue = true;
                    fNPV_reinvest += fCellValue * fPow_reinvest;
                }
                else if( fCellValue < 0.0 )     // investments
                {   // investments
                    bHasNegValue = true;
                    fNPV_invest += fCellValue * fPow_invest;
                }
                fPow_reinvest /= fRate1_reinvest;
                fPow_invest /= fRate1_invest;
                nCount++;

                bLoop = aValIter.GetNext( fCellValue, nIterError );
            }

            if ( nIterError != FormulaError::NONE )
                SetError( nIterError );
        }
        if ( !( bHasPosValue && bHasNegValue ) )
            SetError( FormulaError::IllegalArgument );

        if ( nGlobalError != FormulaError::NONE )
            PushError( nGlobalError );
        else
        {
            double fResult = -o3tl::div_allow_zero(fNPV_reinvest.get(), fNPV_invest.get());
            fResult *= pow( fRate1_reinvest, static_cast<double>( nCount - 1 ) );
            fResult = pow( fResult, div( 1.0, (nCount - 1)) );
            PushDouble( fResult - 1.0 );
        }
    }
}

void ScInterpreter::ScISPMT()
{   // rate ; period ; total_periods ; invest
    if( MustHaveParamCount( GetByte(), 4 ) )
    {
        double fInvest = GetDouble();
        double fTotal = GetDouble();
        double fPeriod = GetDouble();
        double fRate = GetDouble();

        if( nGlobalError != FormulaError::NONE )
            PushError( nGlobalError);
        else
            PushDouble( semath::computeInterestSchedulePayment(
                fRate, fPeriod, fTotal, fInvest ) );
    }
}

// financial functions
double ScInterpreter::ScGetPV(double fRate, double fNper, double fPmt,
                              double fFv, bool bPayInAdvance)
{
    return semath::computePresentValue(fRate, fNper, fPmt, fFv, bPayInAdvance);
}

void ScInterpreter::ScPV()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 5 ) )
        return;

    bool bPayInAdvance = nParamCount == 5 && GetBool();
    double fFv   = nParamCount >= 4 ? GetDouble() : 0;
    double fPmt  = GetDouble();
    double fNper = GetDouble();
    double fRate = GetDouble();
    PushDouble(ScGetPV(fRate, fNper, fPmt, fFv, bPayInAdvance));
}

void ScInterpreter::ScSYD()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    if ( MustHaveParamCount( GetByte(), 4 ) )
    {
        double fPer = GetDouble();
        double fLife = GetDouble();
        double fSalvage = GetDouble();
        double fCost = GetDouble();
        PushDouble(semath::computeSumOfYearsDepreciation(fCost, fSalvage, fLife, fPer));
    }
}

double ScInterpreter::ScGetDDB(double fCost, double fSalvage, double fLife,
                double fPeriod, double fFactor)
{
    return semath::computeDoubleDecliningBalance(fCost, fSalvage, fLife, fPeriod, fFactor);
}

void ScInterpreter::ScDDB()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 4, 5 ) )
        return;

    double fFactor = nParamCount == 5 ? GetDouble() : 2.0;
    double fPeriod = GetDouble();
    double fLife   = GetDouble();
    double fSalvage    = GetDouble();
    double fCost    = GetDouble();
    if (fCost < 0.0 || fSalvage < 0.0 || fFactor <= 0.0 || fSalvage > fCost
                    || fPeriod < 1.0 || fPeriod > fLife)
        PushIllegalArgument();
    else
        PushDouble(ScGetDDB(fCost, fSalvage, fLife, fPeriod, fFactor));
}

void ScInterpreter::ScDB()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 4, 5 ) )
        return ;
    double fMonths = nParamCount == 4 ? 12.0 : ::rtl::math::approxFloor(GetDouble());
    double fPeriod = GetDouble();
    double fLife = GetDouble();
    double fSalvage = GetDouble();
    double fCost = GetDouble();
    if (fMonths < 1.0 || fMonths > 12.0 || fLife > 1200.0 || fSalvage < 0.0 ||
        fPeriod > (fLife + 1.0) || fSalvage > fCost || fCost <= 0.0 ||
        fLife <= 0 || fPeriod <= 0 )
    {
        PushIllegalArgument();
        return;
    }
    PushDouble(semath::computeFixedDecliningBalance(
        fCost, fSalvage, fLife, fPeriod, fMonths));
}

double ScInterpreter::ScInterVDB(double fCost, double fSalvage, double fLife,
                             double fLife1, double fPeriod, double fFactor)
{
    return semath::computeVariableDecliningBalanceSegment(
        fCost, fSalvage, fLife, fLife1, fPeriod, fFactor);
}

void ScInterpreter::ScVDB()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 5, 7 ) )
        return;

    KahanSum fVdb = 0.0;
    bool bNoSwitch = nParamCount == 7 && GetBool();
    double  fFactor = nParamCount >= 6 ? GetDouble() : 2.0;
    double fEnd   = GetDouble();
    double fStart = GetDouble();
    double fLife  = GetDouble();
    double fSalvage   = GetDouble();
    double fCost   = GetDouble();
    if (fStart < 0.0 || fEnd < fStart || fEnd > fLife || fCost < 0.0
                      || fSalvage > fCost || fFactor <= 0.0)
        PushIllegalArgument();
    else
        fVdb = semath::computeVariableDecliningBalance(
            fCost, fSalvage, fLife, fStart, fEnd, fFactor, bNoSwitch);
    PushDouble(fVdb.get());
}

void ScInterpreter::ScPDuration()
{
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double fFuture = GetDouble();
        double fPresent = GetDouble();
        double fRate = GetDouble();
        if ( fFuture <= 0.0 || fPresent <= 0.0 || fRate <= 0.0 )
            PushIllegalArgument();
        else
            PushDouble( semath::computePaybackDuration( fRate, fPresent, fFuture ) );
    }
}

void ScInterpreter::ScSLN()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double fLife = GetDouble();
        double fSalvage = GetDouble();
        double fCost = GetDouble();
        PushDouble(semath::computeStraightLineDepreciation(fCost, fSalvage, fLife));
    }
}

double ScInterpreter::ScGetPMT(double fRate, double fNper, double fPv,
                       double fFv, bool bPayInAdvance)
{
    return semath::computePayment(fRate, fNper, fPv, fFv, bPayInAdvance);
}

void ScInterpreter::ScPMT()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 5 ) )
        return;
    bool bPayInAdvance = nParamCount == 5 && GetBool();
    double fFv   = nParamCount >= 4 ? GetDouble() : 0;
    double fPv   = GetDouble();
    double fNper = GetDouble();
    double fRate = GetDouble();
    PushDouble(ScGetPMT(fRate, fNper, fPv, fFv, bPayInAdvance));
}

void ScInterpreter::ScRRI()
{
    nFuncFmtType = SvNumFormatType::PERCENT;
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double fFutureValue = GetDouble();
        double fPresentValue = GetDouble();
        double fNrOfPeriods = GetDouble();
        if ( fNrOfPeriods <= 0.0  || fPresentValue == 0.0 )
            PushIllegalArgument();
        else
            PushDouble(semath::computeGrowthRateOverPeriods(
                fNrOfPeriods, fPresentValue, fFutureValue));
    }
}

double ScInterpreter::ScGetFV(double fRate, double fNper, double fPmt,
                              double fPv, bool bPayInAdvance)
{
    return semath::computeFutureValue(fRate, fNper, fPmt, fPv, bPayInAdvance);
}

void ScInterpreter::ScFV()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 5 ) )
        return;
    bool bPayInAdvance = nParamCount == 5 && GetBool();
    double fPv   = nParamCount >= 4 ? GetDouble() : 0;
    double fPmt  = GetDouble();
    double fNper = GetDouble();
    double fRate = GetDouble();
    PushDouble(ScGetFV(fRate, fNper, fPmt, fPv, bPayInAdvance));
}

void ScInterpreter::ScNper()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 5 ) )
        return;
    bool bPayInAdvance = nParamCount == 5 && GetBool();
    double fFV   = nParamCount >= 4 ? GetDouble() : 0;
    double fPV   = GetDouble();      // Present Value
    double fPmt  = GetDouble();      // Payment
    double fRate = GetDouble();
    PushDouble(semath::computePeriodsForFutureValue(
        fRate, fPmt, fPV, fFV, bPayInAdvance));
}

// In Calc UI it is the function RATE(Nper;Pmt;Pv;Fv;Type;Guess)
void ScInterpreter::ScRate()
{
    nFuncFmtType = SvNumFormatType::PERCENT;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 6 ) )
        return;

    // defaults for missing arguments, see ODFF spec
    double fGuess = nParamCount == 6 ? GetDouble() : 0.1;
    bool bDefaultGuess = nParamCount != 6;
    bool bPayType = nParamCount >= 5 && GetBool();
    double fFv = nParamCount >= 4 ? GetDouble() : 0;
    double fPv = GetDouble();
    double fPayment = GetDouble();
    double fNper = GetDouble();
    if (fNper <= 0.0) // constraint from ODFF spec
    {
        PushIllegalArgument();
        return;
    }
    const semath::FinancialRateResult aResult = semath::solveRate(
        fNper, fPayment, fPv, fFv, bPayType, fGuess, bDefaultGuess);
    if (!aResult.mbConverged)
        SetError(FormulaError::NoConvergence);
    PushDouble(aResult.mfRate);
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

void ScInterpreter::ScIpmt()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 4, 6 ) )
        return;
    bool bPayInAdvance = nParamCount == 6 && GetBool();
    double fFv   = nParamCount >= 5 ? GetDouble() : 0;
    double fPv   = GetDouble();
    double fNper = GetDouble();
    double fPer  = GetDouble();
    double fRate = GetDouble();
    if (fPer < 1.0 || fPer > fNper)
        PushIllegalArgument();
    else
    {
        double fPmt;
        PushDouble(ScGetIpmt(fRate, fPer, fNper, fPv, fFv, bPayInAdvance, fPmt));
    }
}

void ScInterpreter::ScPpmt()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 4, 6 ) )
        return;
    bool bPayInAdvance = nParamCount == 6 && GetBool();
    double fFv   = nParamCount >= 5 ? GetDouble() : 0;
    double fPv   = GetDouble();
    double fNper = GetDouble();
    double fPer  = GetDouble();
    double fRate = GetDouble();
    if (fPer < 1.0 || fPer > fNper)
        PushIllegalArgument();
    else
    {
        double fPmt;
        double fInterestPer = ScGetIpmt(fRate, fPer, fNper, fPv, fFv, bPayInAdvance, fPmt);
        PushDouble(fPmt - fInterestPer);
    }
}

void ScInterpreter::ScCumIpmt()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    if ( !MustHaveParamCount( GetByte(), 6 ) )
        return;

    double fFlag  = GetDoubleWithDefault( -1.0 );
    double fEnd   = ::rtl::math::approxFloor(GetDouble());
    double fStart = ::rtl::math::approxFloor(GetDouble());
    double fPv    = GetDouble();
    double fNper  = GetDouble();
    double fRate  = GetDouble();
    if (fStart < 1.0 || fEnd < fStart || fRate <= 0.0 ||
        fEnd > fNper  || fNper <= 0.0 || fPv <= 0.0 ||
        ( fFlag != 0.0 && fFlag != 1.0 ))
        PushIllegalArgument();
    else
    {
        bool bPayInAdvance = static_cast<bool>(fFlag);
        PushDouble(semath::computeCumulativeInterest(
            fRate, fStart, fEnd, fNper, fPv, 0.0, bPayInAdvance));
    }
}

void ScInterpreter::ScCumPrinc()
{
    nFuncFmtType = SvNumFormatType::CURRENCY;
    if ( !MustHaveParamCount( GetByte(), 6 ) )
        return;

    double fFlag  = GetDoubleWithDefault( -1.0 );
    double fEnd   = ::rtl::math::approxFloor(GetDouble());
    double fStart = ::rtl::math::approxFloor(GetDouble());
    double fPv    = GetDouble();
    double fNper  = GetDouble();
    double fRate  = GetDouble();
    if (fStart < 1.0 || fEnd < fStart || fRate <= 0.0 ||
        fEnd > fNper  || fNper <= 0.0 || fPv <= 0.0 ||
        ( fFlag != 0.0 && fFlag != 1.0 ))
        PushIllegalArgument();
    else
    {
        bool bPayInAdvance = static_cast<bool>(fFlag);
        PushDouble(semath::computeCumulativePrincipal(
            fRate, fStart, fEnd, fNper, fPv, 0.0, bPayInAdvance));
    }
}

void ScInterpreter::ScEffect()
{
    nFuncFmtType = SvNumFormatType::PERCENT;
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;

    double fPeriods = GetDouble();
    double fNominal = GetDouble();
    if (fPeriods < 1.0 || fNominal < 0.0)
        PushIllegalArgument();
    else if ( fNominal == 0.0 )
        PushDouble( 0.0 );
    else
    {
        fPeriods = ::rtl::math::approxFloor(fPeriods);
        PushDouble(semath::computeEffectiveAnnualRate(fNominal, fPeriods));
    }
}

void ScInterpreter::ScNominal()
{
    nFuncFmtType = SvNumFormatType::PERCENT;
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        double fPeriods = GetDouble();
        double fEffective = GetDouble();
        if (fPeriods < 1.0 || fEffective <= 0.0)
            PushIllegalArgument();
        else
        {
            fPeriods = ::rtl::math::approxFloor(fPeriods);
            PushDouble(semath::computeNominalAnnualRate(fEffective, fPeriods));
        }
    }
}

void ScInterpreter::ScMod()
{
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;

    double fDenom   = GetDouble();   // Denominator
    if ( fDenom == 0.0 )
    {
        PushError(FormulaError::DivisionByZero);
        return;
    }
    double fNum = GetDouble();   // Numerator
    if (std::optional<double> fResult = semath::computeMod( fNum, fDenom ))
        PushDouble(*fResult);
    else
        PushError( FormulaError::NoValue );
}

void ScInterpreter::ScIntersect()
{
    formula::FormulaConstTokenRef p2nd = PopToken();
    formula::FormulaConstTokenRef p1st = PopToken();

    if (nGlobalError != FormulaError::NONE || !p2nd || !p1st)
    {
        PushIllegalArgument();
        return;
    }

    StackVar sv1 = p1st->GetType();
    StackVar sv2 = p2nd->GetType();
    if ((sv1 != svSingleRef && sv1 != svDoubleRef && sv1 != svRefList) ||
        (sv2 != svSingleRef && sv2 != svDoubleRef && sv2 != svRefList))
    {
        PushIllegalArgument();
        return;
    }

    const formula::FormulaToken* x1 = p1st.get();
    const formula::FormulaToken* x2 = p2nd.get();
    if (sv1 == svRefList || sv2 == svRefList)
    {
        // Now this is a bit nasty but it simplifies things, and having
        // intersections with lists isn't too common, if at all...
        // Convert a reference to list.
        const formula::FormulaToken* xt[2] = { x1, x2 };
        StackVar sv[2] = { sv1, sv2 };
        // There may only be one reference; the other is necessarily a list
        // Ensure converted list proper destruction
        std::unique_ptr<formula::FormulaToken> p;
        for (size_t i=0; i<2; ++i)
        {
            if (sv[i] == svSingleRef)
            {
                ScComplexRefData aRef;
                aRef.Ref1 = aRef.Ref2 = *xt[i]->GetSingleRef();
                p.reset(new ScRefListToken);
                p->GetRefList()->push_back( aRef);
                xt[i] = p.get();
            }
            else if (sv[i] == svDoubleRef)
            {
                ScComplexRefData aRef = *xt[i]->GetDoubleRef();
                p.reset(new ScRefListToken);
                p->GetRefList()->push_back( aRef);
                xt[i] = p.get();
            }
        }
        x1 = xt[0];
        x2 = xt[1];

        ScTokenRef xRes = new ScRefListToken;
        ScRefList* pRefList = xRes->GetRefList();
        for (const auto& rRef1 : *x1->GetRefList())
        {
            const ScAddress r11 = rRef1.Ref1.toAbs(mrDoc, aPos);
            const ScAddress r12 = rRef1.Ref2.toAbs(mrDoc, aPos);
            for (const auto& rRef2 : *x2->GetRefList())
            {
                const ScAddress r21 = rRef2.Ref1.toAbs(mrDoc, aPos);
                const ScAddress r22 = rRef2.Ref2.toAbs(mrDoc, aPos);
                SCCOL nCol1 = ::std::max( r11.Col(), r21.Col());
                SCROW nRow1 = ::std::max( r11.Row(), r21.Row());
                SCTAB nTab1 = ::std::max( r11.Tab(), r21.Tab());
                SCCOL nCol2 = ::std::min( r12.Col(), r22.Col());
                SCROW nRow2 = ::std::min( r12.Row(), r22.Row());
                SCTAB nTab2 = ::std::min( r12.Tab(), r22.Tab());
                if (nCol2 < nCol1 || nRow2 < nRow1 || nTab2 < nTab1)
                    ;   // nothing
                else
                {
                    ScComplexRefData aRef;
                    aRef.InitRange( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                    pRefList->push_back( aRef);
                }
            }
        }
        size_t n = pRefList->size();
        if (!n)
            PushError( FormulaError::NoCode);
        else if (n == 1)
        {
            const ScComplexRefData& rRef = (*pRefList)[0];
            if (rRef.Ref1 == rRef.Ref2)
                PushTempToken( new ScSingleRefToken(mrDoc.GetSheetLimits(), rRef.Ref1));
            else
                PushTempToken( new ScDoubleRefToken(mrDoc.GetSheetLimits(), rRef));
        }
        else
            PushTokenRef( xRes);
    }
    else
    {
        const formula::FormulaToken* pt[2] = { x1, x2 };
        StackVar sv[2] = { sv1, sv2 };
        SCCOL nC1[2], nC2[2];
        SCROW nR1[2], nR2[2];
        SCTAB nT1[2], nT2[2];
        for (size_t i=0; i<2; ++i)
        {
            switch (sv[i])
            {
                case svSingleRef:
                case svDoubleRef:
                {
                    {
                        const ScAddress r = pt[i]->GetSingleRef()->toAbs(mrDoc, aPos);
                        nC1[i] = r.Col();
                        nR1[i] = r.Row();
                        nT1[i] = r.Tab();
                    }
                    if (sv[i] == svDoubleRef)
                    {
                        const ScAddress r = pt[i]->GetSingleRef2()->toAbs(mrDoc, aPos);
                        nC2[i] = r.Col();
                        nR2[i] = r.Row();
                        nT2[i] = r.Tab();
                    }
                    else
                    {
                        nC2[i] = nC1[i];
                        nR2[i] = nR1[i];
                        nT2[i] = nT1[i];
                    }
                }
                break;
                default:
                    ;   // nothing, prevent compiler warning
            }
        }
        SCCOL nCol1 = ::std::max( nC1[0], nC1[1]);
        SCROW nRow1 = ::std::max( nR1[0], nR1[1]);
        SCTAB nTab1 = ::std::max( nT1[0], nT1[1]);
        SCCOL nCol2 = ::std::min( nC2[0], nC2[1]);
        SCROW nRow2 = ::std::min( nR2[0], nR2[1]);
        SCTAB nTab2 = ::std::min( nT2[0], nT2[1]);
        if (nCol2 < nCol1 || nRow2 < nRow1 || nTab2 < nTab1)
            PushError( FormulaError::NoCode);
        else if (nCol2 == nCol1 && nRow2 == nRow1 && nTab2 == nTab1)
            PushSingleRef( nCol1, nRow1, nTab1);
        else
            PushDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
    }
}

void ScInterpreter::ScRangeFunc()
{
    formula::FormulaConstTokenRef x2 = PopToken();
    formula::FormulaConstTokenRef x1 = PopToken();

    if (nGlobalError != FormulaError::NONE || !x2 || !x1)
    {
        PushIllegalArgument();
        return;
    }
    // We explicitly tell extendRangeReference() to not reuse the token,
    // casting const away spares two clones.
    FormulaTokenRef xRes = extendRangeReference(
            mrDoc.GetSheetLimits(), const_cast<FormulaToken&>(*x1), const_cast<FormulaToken&>(*x2), aPos, false);
    if (!xRes)
        PushIllegalArgument();
    else
        PushTokenRef( xRes);
}

void ScInterpreter::ScUnionFunc()
{
    formula::FormulaConstTokenRef p2nd = PopToken();
    formula::FormulaConstTokenRef p1st = PopToken();

    if (nGlobalError != FormulaError::NONE || !p2nd || !p1st)
    {
        PushIllegalArgument();
        return;
    }

    StackVar sv1 = p1st->GetType();
    StackVar sv2 = p2nd->GetType();
    if ((sv1 != svSingleRef && sv1 != svDoubleRef && sv1 != svRefList) ||
        (sv2 != svSingleRef && sv2 != svDoubleRef && sv2 != svRefList))
    {
        PushIllegalArgument();
        return;
    }

    const formula::FormulaToken* x1 = p1st.get();
    const formula::FormulaToken* x2 = p2nd.get();

    ScTokenRef xRes;
    // Append to an existing RefList if there is one.
    if (sv1 == svRefList)
    {
        xRes = x1->Clone();
        sv1 = svUnknown;    // mark as handled
    }
    else if (sv2 == svRefList)
    {
        xRes = x2->Clone();
        sv2 = svUnknown;    // mark as handled
    }
    else
        xRes = new ScRefListToken;
    ScRefList* pRes = xRes->GetRefList();
    const formula::FormulaToken* pt[2] = { x1, x2 };
    StackVar sv[2] = { sv1, sv2 };
    for (size_t i=0; i<2; ++i)
    {
        if (pt[i] == xRes)
            continue;
        switch (sv[i])
        {
            case svSingleRef:
                {
                    ScComplexRefData aRef;
                    aRef.Ref1 = aRef.Ref2 = *pt[i]->GetSingleRef();
                    pRes->push_back( aRef);
                }
                break;
            case svDoubleRef:
                pRes->push_back( *pt[i]->GetDoubleRef());
                break;
            case svRefList:
                {
                    const ScRefList* p = pt[i]->GetRefList();
                    for (const auto& rRef : *p)
                    {
                        pRes->push_back( rRef);
                    }
                }
                break;
            default:
                ;   // nothing, prevent compiler warning
        }
    }
    ValidateRef( *pRes);    // set #REF! if needed
    PushTokenRef( xRes);
}

void ScInterpreter::ScCurrent()
{
    FormulaConstTokenRef xTok( PopToken());
    if (xTok)
    {
        PushTokenRef( xTok);
        PushTokenRef( xTok);
    }
    else
        PushError( FormulaError::UnknownStackVariable);
}

void ScInterpreter::ScStyle()
{
    sal_uInt8 nParamCount = GetByte();
    if (!MustHaveParamCount(nParamCount, 1, 3))
        return;

    OUString aStyle2;                           // Style after timer
    if (nParamCount >= 3)
        aStyle2 = GetString().getString();
    tools::Long nTimeOut = 0;                          // timeout
    if (nParamCount >= 2)
        nTimeOut = static_cast<tools::Long>(GetDouble()*1000.0);
    OUString aStyle1 = GetString().getString(); // Style for immediate

    if (nTimeOut < 0)
        nTimeOut = 0;

    // Execute request to apply style
    if ( !mrDoc.IsClipOrUndo() )
    {
        ScDocShell* pShell = mrDoc.GetDocumentShell();
        if (pShell)
        {
            // Normalize style names right here, making sure that character case is correct,
            // and that we only apply anything when there's something to apply
            auto pPool = mrDoc.GetStyleSheetPool();
            if (!aStyle1.isEmpty())
            {
                if (auto pNewStyle = pPool->FindAutoStyle(aStyle1))
                    aStyle1 = pNewStyle->GetName();
                else
                    aStyle1.clear();
            }
            if (!aStyle2.isEmpty())
            {
                if (auto pNewStyle = pPool->FindAutoStyle(aStyle2))
                    aStyle2 = pNewStyle->GetName();
                else
                    aStyle2.clear();
            }
            // notify object shell directly!
            if (!aStyle1.isEmpty() || !aStyle2.isEmpty())
            {
                const ScStyleSheet* pStyle = mrDoc.GetStyle(aPos.Col(), aPos.Row(), aPos.Tab());

                const bool bNotify = !pStyle
                                     || (!aStyle1.isEmpty() && pStyle->GetName() != aStyle1)
                                     || (!aStyle2.isEmpty() && pStyle->GetName() != aStyle2);
                if (bNotify)
                {
                    ScRange aRange(aPos);
                    ScAutoStyleHint aHint(aRange, aStyle1, nTimeOut, aStyle2);
                    pShell->Broadcast(aHint);
                }
            }
        }
    }

    PushDouble(0.0);
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

void ScInterpreter::ScBase()
{   // Value, Base [, MinLen]
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return;

    const std::optional<double> ofMinLength
        = (nParamCount == 3) ? std::optional<double>(GetDouble()) : std::nullopt;
    const double fBase = GetDouble();
    const double fValue = GetDouble();
    if (nGlobalError != FormulaError::NONE)
    {
        PushIllegalArgument();
        return;
    }

    const seconvert::NumeralStringResult aResult
        = seconvert::convertToBase(fValue, fBase, ofMinLength);
    if (aResult.meError == seconvert::NumeralStringError::None)
        PushString(selibreoffice::toLibreOfficeString(aResult.maValue));
    else if (aResult.meError == seconvert::NumeralStringError::StringOverflow)
        PushError(FormulaError::StringOverflow);
    else
        PushIllegalArgument();
}

void ScInterpreter::ScDecimal()
{   // Text, Base
    if ( !MustHaveParamCount( GetByte(), 2 ) )
        return;

    const double fBase = GetDouble();
    const OUString aText = GetString().getString();
    if (nGlobalError != FormulaError::NONE)
    {
        PushIllegalArgument();
        return;
    }

    if (std::optional<double> fValue
        = seconvert::convertFromBase(selibreoffice::toApiString(aText), fBase))
        PushDouble(*fValue);
    else
        PushIllegalArgument();
}

void ScInterpreter::ScConvertOOo()
{   // Value, FromUnit, ToUnit
    if ( !MustHaveParamCount( GetByte(), 3 ) )
        return;

    OUString aToUnit = GetString().getString();
    OUString aFromUnit = GetString().getString();
    double fVal = GetDouble();
    if ( nGlobalError != FormulaError::NONE )
        PushError( nGlobalError);
    else
    {
        // first of all search for the given order; if it can't be found then search for the inverse
        double fConv;
        if ( ScGlobal::GetUnitConverter()->GetValue( fConv, aFromUnit, aToUnit ) )
            PushDouble( fVal * fConv );
        else if ( ScGlobal::GetUnitConverter()->GetValue( fConv, aToUnit, aFromUnit ) )
            PushDouble( fVal / fConv );
        else
            PushNA();
    }
}

void ScInterpreter::ScRoman()
{   // Value [Mode]
    sal_uInt8 nParamCount = GetByte();
    if( !MustHaveParamCount( nParamCount, 1, 2 ) )
        return;

    const std::optional<double> ofMode
        = (nParamCount == 2) ? std::optional<double>(GetDouble()) : std::nullopt;
    const double fValue = GetDouble();
    if( nGlobalError != FormulaError::NONE )
        PushError( nGlobalError);
    else if (auto aRoman = seconvert::convertToRoman(fValue, ofMode))
        PushString(selibreoffice::toLibreOfficeString(*aRoman));
    else
        PushIllegalArgument();
}

void ScInterpreter::ScArabic()
{
    const OUString aRoman = GetString().getString();
    if( nGlobalError != FormulaError::NONE )
        PushError( nGlobalError);
    else if (std::optional<sal_Int32> nValue
             = seconvert::convertFromRoman(selibreoffice::toApiString(aRoman)))
        PushInt(*nValue);
    else
        PushIllegalArgument();
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

/** Resources at the website of the European Commission:
    http://ec.europa.eu/economy_finance/euro/adoption/conversion/
    http://ec.europa.eu/economy_finance/euro/countries/
 */
static bool lclConvertMoney( std::u16string_view aSearchUnit, double& rfRate, int& rnDec )
{
    struct ConvertInfo
    {
        const char* pCurrText;
        double      fRate;
        int         nDec;
    };
    static const ConvertInfo aConvertTable[] = {
        { "EUR", 1.0,      2 },
        { "ATS", 13.7603,  2 },
        { "BEF", 40.3399,  0 },
        { "DEM", 1.95583,  2 },
        { "ESP", 166.386,  0 },
        { "FIM", 5.94573,  2 },
        { "FRF", 6.55957,  2 },
        { "IEP", 0.787564, 2 },
        { "ITL", 1936.27,  0 },
        { "LUF", 40.3399,  0 },
        { "NLG", 2.20371,  2 },
        { "PTE", 200.482,  2 },
        { "GRD", 340.750,  2 },
        { "SIT", 239.640,  2 },
        { "MTL", 0.429300, 2 },
        { "CYP", 0.585274, 2 },
        { "SKK", 30.1260,  2 },
        { "EEK", 15.6466,  2 },
        { "LVL", 0.702804, 2 },
        { "LTL", 3.45280,  2 },
        { "HRK", 7.53450,  2 },
        { "BGN", 1.95583,  2 }
    };

    for (const auto & i : aConvertTable)
        if ( o3tl::equalsIgnoreAsciiCase( aSearchUnit, i.pCurrText ) )
        {
            rfRate = i.fRate;
            rnDec  = i.nDec;
            return true;
        }
    return false;
}

void ScInterpreter::ScEuroConvert()
{   //Value, FromUnit, ToUnit[, FullPrecision, [TriangulationPrecision]]
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 3, 5 ) )
        return;

    double fPrecision = 0.0;
    if ( nParamCount == 5 )
    {
        fPrecision = ::rtl::math::approxFloor(GetDouble());
        if ( fPrecision < 3 )
        {
            PushIllegalArgument();
            return;
        }
    }

    bool bFullPrecision = nParamCount >= 4 && GetBool();
    OUString aToUnit = GetString().getString();
    OUString aFromUnit = GetString().getString();
    double fVal = GetDouble();
    if ( nGlobalError != FormulaError::NONE )
        PushError( nGlobalError);
    else
    {
        double fFromRate;
        double fToRate;
        int    nFromDec;
        int    nToDec;
        if ( lclConvertMoney( aFromUnit, fFromRate, nFromDec )
            && lclConvertMoney( aToUnit, fToRate, nToDec ) )
        {
            double fRes;
            if ( aFromUnit.equalsIgnoreAsciiCase( aToUnit ) )
                fRes = fVal;
            else
            {
                if ( aFromUnit.equalsIgnoreAsciiCase( "EUR" ) )
                   fRes = fVal * fToRate;
                else
                {
                    double fIntermediate = fVal / fFromRate;
                    if ( fPrecision )
                        fIntermediate = ::rtl::math::round( fIntermediate,
                                                        static_cast<int>(fPrecision) );
                    fRes = fIntermediate * fToRate;
                }
                if ( !bFullPrecision )
                    fRes = ::rtl::math::round( fRes, nToDec );
            }
            PushDouble( fRes );
        }
        else
            PushIllegalArgument();
    }
}

// BAHTTEXT
// local functions
namespace {

constexpr std::u16string_view TH_0      = u"ศูนย์";
constexpr std::u16string_view TH_1      = u"หนึ่ง";
constexpr std::u16string_view TH_2      = u"สอง";
constexpr std::u16string_view TH_3      = u"สาม";
constexpr std::u16string_view TH_4      = u"สี่";
constexpr std::u16string_view TH_5      = u"ห้า";
constexpr std::u16string_view TH_6      = u"หก";
constexpr std::u16string_view TH_7      = u"เจ็ด";
constexpr std::u16string_view TH_8      = u"แปด";
constexpr std::u16string_view TH_9      = u"เก้า";
constexpr std::u16string_view TH_10     = u"สิบ";
constexpr std::u16string_view TH_11     = u"เอ็ด";
constexpr std::u16string_view TH_20     = u"ยี่";
constexpr std::u16string_view TH_1E2    = u"ร้อย";
constexpr std::u16string_view TH_1E3    = u"พัน";
constexpr std::u16string_view TH_1E4    = u"หมื่น";
constexpr std::u16string_view TH_1E5    = u"แสน";
constexpr std::u16string_view TH_1E6    = u"ล้าน";
constexpr std::u16string_view TH_DOT0   = u"ถ้วน";
constexpr std::u16string_view TH_BAHT   = u"บาท";
constexpr std::u16string_view TH_SATANG = u"สตางค์";
constexpr std::u16string_view TH_MINUS  = u"ลบ";

/** Appends a digit (1 to 9) to the passed string. */
void lclAppendDigit( OUStringBuffer& rText, char nDigit )
{
    assert(nDigit >= '1' && nDigit <= '9');
    switch( nDigit )
    {
        case '1': rText.append( TH_1 ); break;
        case '2': rText.append( TH_2 ); break;
        case '3': rText.append( TH_3 ); break;
        case '4': rText.append( TH_4 ); break;
        case '5': rText.append( TH_5 ); break;
        case '6': rText.append( TH_6 ); break;
        case '7': rText.append( TH_7 ); break;
        case '8': rText.append( TH_8 ); break;
        case '9': rText.append( TH_9 ); break;
    }
}

/** Appends a value raised to a power of 10: nDigit*10^nPow10.
    @param nDigit  A digit in the range from 1 to 9.
    @param nPow10  A value in the range from 2 to 5.
 */
void lclAppendPow10( OUStringBuffer& rText, char nDigit, sal_Int32 nPow10 )
{
    assert(nPow10 >= 2 && nPow10 <= 5);
    lclAppendDigit( rText, nDigit );
    switch( nPow10 )
    {
        case 2: rText.append( TH_1E2 );   break;
        case 3: rText.append( TH_1E3 );   break;
        case 4: rText.append( TH_1E4 );   break;
        case 5: rText.append( TH_1E5 );   break;
    }
}

/** Appends a block of 6 digits (value from 1 to 999,999) to the passed string. */
void lclAppendBlock( OUStringBuffer& rText, std::string_view block )
{
    assert(block.size() > 0 && block.size() <= 6);
    auto it = block.begin();
    for (size_t pow = block.size() - 1; pow >= 2; --pow)
        if (char ch = *it++; ch != '0')
            lclAppendPow10(rText, ch, pow);

    // Two last characters
    char ten = block.size() > 1 ? *it++ : '0';
    char one = *it;
    if (ten >= '1')
    {
        if (ten >= '3')
            lclAppendDigit(rText, ten);
        else if (ten == '2')
            rText.append( TH_20 );
        rText.append( TH_10 );
    }
    if ((ten > '0') && (one == '1'))
        rText.append( TH_11 );
    else if( one > '0' )
        lclAppendDigit(rText, one);
}

} // namespace

void ScInterpreter::ScBahtText()
{
    sal_uInt8 nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 1 ) )
        return;

    double fValue = GetDouble();
    if( nGlobalError != FormulaError::NONE )
    {
        PushError( nGlobalError);
        return;
    }

    OString number = rtl::math::doubleToString(std::abs(fValue), rtl_math_StringFormat_F, 2, '.');
    assert(number.getLength() > 3); // At least "0.00"
    const sal_Int32 dotPos = number.getLength() - 3;
    assert(number[dotPos] == '.');
    // split Baht and Satang
    std::string_view sBaht(number.subView(0, dotPos)), sSatang(number.subView(dotPos + 1));
    bool noBaht = sBaht == "0", noSatang = sSatang == "00";
    if (noBaht && noSatang)
    {
        return PushString(OUString::Concat(TH_0) + TH_BAHT + TH_DOT0);
    }

    OUStringBuffer aText;
    // sign
    if (fValue < 0.0)
        aText.append(TH_MINUS);

    // generate text for Baht value
    if (!noBaht)
    {
        size_t blocksize = sBaht.size() % 6;
        if (blocksize == 0)
            blocksize = 6;
        while (!sBaht.empty())
        {
            assert(blocksize <= sBaht.size());
            lclAppendBlock(aText, sBaht.substr(0, blocksize));
            sBaht.remove_prefix(blocksize);
            blocksize = 6;
            // add "million", if there will come more blocks
            if (!sBaht.empty())
                aText.append(TH_1E6);
        }
        aText.append(TH_BAHT);
    }

    // generate text for Satang value
    if (noSatang)
    {
        aText.append( TH_DOT0 );
    }
    else
    {
        lclAppendBlock(aText, sSatang);
        aText.append( TH_SATANG );
    }

    PushString(aText.makeStringAndClear());
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
