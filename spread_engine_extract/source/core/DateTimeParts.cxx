/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/DateTimeParts.hxx>

#include <cmath>
#include <optional>

#include <rtl/math.hxx>
#include <tools/date.hxx>
#include <tools/long.hxx>
#include <tools/time.hxx>

namespace spreadsheetengine::core::datetime
{

std::optional<double> makeDateSerial(
    const Date& rNullDate, sal_Int16 nYear, sal_Int16 nMonth, sal_Int16 nDay, bool bStrict)
{
    sal_Int16 nCalcYear = nYear;
    sal_Int16 nCalcMonth = nMonth;
    sal_Int16 nCalcDay = nDay;
    if (!bStrict)
    {
        if (nMonth > 0)
        {
            nCalcYear = nYear + (nMonth - 1) / 12;
            nCalcMonth = ((nMonth - 1) % 12) + 1;
        }
        else
        {
            nCalcYear = nYear + (nMonth - 12) / 12;
            nCalcMonth = 12 - (-nMonth) % 12;
        }
        nCalcDay = 1;
    }

    Date aDate(nCalcDay, nCalcMonth, nCalcYear);
    if (!bStrict)
        aDate.AddDays(nDay - 1);

    if (!aDate.IsValidAndGregorian())
        return std::nullopt;

    return static_cast<double>(aDate - rNullDate);
}

double extractYear(const Date& rNullDate, sal_Int32 nDays)
{
    Date aDate = rNullDate;
    aDate.AddDays(nDays);
    return static_cast<double>(aDate.GetYear());
}

double extractMonth(const Date& rNullDate, sal_Int32 nDays)
{
    Date aDate = rNullDate;
    aDate.AddDays(nDays);
    return static_cast<double>(aDate.GetMonth());
}

std::optional<double> extractDay(const Date& rNullDate, sal_Int32 nDays)
{
    Date aDate = rNullDate;
    if (!aDate.CheckedAddDays(nDays))
        return std::nullopt;

    return static_cast<double>(aDate.GetDay());
}

namespace
{

void splitClock(double fTimeValue, sal_uInt16& rHour, sal_uInt16& rMinute, sal_uInt16& rSecond,
    double& rFractionOfSecond)
{
    tools::Time::GetClock(fTimeValue, rHour, rMinute, rSecond, rFractionOfSecond, 0);
}

}

double extractMinute(double fTimeValue)
{
    sal_uInt16 nHour, nMinute, nSecond;
    double fFractionOfSecond;
    splitClock(fTimeValue, nHour, nMinute, nSecond, fFractionOfSecond);
    return nMinute;
}

double extractSecond(double fTimeValue)
{
    sal_uInt16 nHour, nMinute, nSecond;
    double fFractionOfSecond;
    splitClock(fTimeValue, nHour, nMinute, nSecond, fFractionOfSecond);
    if (fFractionOfSecond >= 0.5)
        nSecond = (nSecond + 1) % 60;
    return nSecond;
}

double extractHour(double fTimeValue)
{
    sal_uInt16 nHour, nMinute, nSecond;
    double fFractionOfSecond;
    splitClock(fTimeValue, nHour, nMinute, nSecond, fFractionOfSecond);
    return nHour;
}

std::optional<double> makeTimeSerial(double fHour, double fMinute, double fSecond)
{
    const double fDaySeconds = static_cast<double>(::tools::Time::secondPerDay);
    const double fTime = std::fmod(
                             (fHour * ::tools::Time::secondPerHour)
                                 + (fMinute * ::tools::Time::secondPerMinute) + fSecond,
                             fDaySeconds)
                         / fDaySeconds;
    if (fTime < 0.0)
        return std::nullopt;

    return fTime;
}

std::optional<double> computeEasterSundaySerial(const Date& rNullDate, sal_Int16 nYear)
{
    if (nYear < 1583 || nYear > 9956)
        return std::nullopt;

    const int N = nYear % 19;
    const int B = int(nYear / 100);
    const int C = nYear % 100;
    const int D = int(B / 4);
    const int E = B % 4;
    const int F = int((B + 8) / 25);
    const int G = int((B - F + 1) / 3);
    const int H = (19 * N + B - D - G + 15) % 30;
    const int I = int(C / 4);
    const int K = C % 4;
    const int L = (32 + 2 * E + 2 * I - H - K) % 7;
    const int M = int((N + 11 * H + 22 * L) / 451);
    const int O = H + L - 7 * M + 114;
    const sal_Int16 nDay = sal::static_int_cast<sal_Int16>(O % 31 + 1);
    const sal_Int16 nMonth = sal::static_int_cast<sal_Int16>(int(O / 31));
    return makeDateSerial(rNullDate, nYear, nMonth, nDay, true);
}

double computeDiffDate(double fDate1, double fDate2)
{
    return fDate1 - fDate2;
}

double computeDiffDate360(
    const Date& rNullDate, sal_Int32 nDate1, sal_Int32 nDate2, bool bEuropeanMethod)
{
    sal_Int32 nSign = 1;
    if (bEuropeanMethod && (nDate2 < nDate1))
    {
        std::swap(nDate1, nDate2);
        nSign = -1;
    }

    Date aDate1 = rNullDate;
    aDate1.AddDays(nDate1);
    Date aDate2 = rNullDate;
    aDate2.AddDays(nDate2);
    if (aDate1.GetDay() == 31)
        aDate1.AddDays(-1);
    else if (!bEuropeanMethod && aDate1.GetMonth() == 2)
    {
        switch (aDate1.GetDay())
        {
            case 28:
                if (!aDate1.IsLeapYear())
                    aDate1.SetDay(30);
                break;
            case 29:
                aDate1.SetDay(30);
                break;
        }
    }

    if (aDate2.GetDay() == 31)
    {
        if (!bEuropeanMethod)
        {
            if (aDate1.GetDay() == 30)
                aDate2.AddDays(-1);
        }
        else
            aDate2.SetDay(30);
    }

    return static_cast<double>(nSign)
           * (static_cast<double>(aDate2.GetDay())
              + static_cast<double>(aDate2.GetMonth()) * 30.0
              + static_cast<double>(aDate2.GetYear()) * 360.0
              - static_cast<double>(aDate1.GetDay())
              - static_cast<double>(aDate1.GetMonth()) * 30.0
              - static_cast<double>(aDate1.GetYear()) * 360.0);
}

std::optional<double> computeDateDif(
    const Date& rNullDate, sal_Int32 nDate1, sal_Int32 nDate2, const OUString& rInterval)
{
    if (nDate1 > nDate2)
        return std::nullopt;

    const double fDaysDifference = nDate2 - nDate1;
    if (fDaysDifference == 0.0 || rInterval.equalsIgnoreAsciiCase("d"))
        return fDaysDifference;

    Date aDate1(rNullDate);
    aDate1.AddDays(nDate1);
    Date aDate2(rNullDate);
    aDate2.AddDays(nDate2);

    sal_uInt16 d1 = aDate1.GetDay();
    sal_uInt16 m1 = aDate1.GetMonth();
    sal_uInt16 d2 = aDate2.GetDay();
    sal_uInt16 m2 = aDate2.GetMonth();
    sal_Int16 y1 = aDate1.GetYear();
    sal_Int16 y2 = aDate2.GetYear();

    if (y1 < 0 && y2 > 0)
        ++y1;
    else if (y1 > 0 && y2 < 0)
        ++y2;

    if (rInterval.equalsIgnoreAsciiCase("m"))
    {
        int nMonths = m2 - m1 + 12 * (y2 - y1);
        if (d1 > d2)
            --nMonths;
        return static_cast<double>(nMonths);
    }

    if (rInterval.equalsIgnoreAsciiCase("y"))
    {
        int nYears;
        if (y2 > y1)
        {
            if (m2 > m1 || (m2 == m1 && d2 >= d1))
                nYears = y2 - y1;
            else
                nYears = y2 - y1 - 1;
        }
        else
            nYears = 0;
        return static_cast<double>(nYears);
    }

    if (rInterval.equalsIgnoreAsciiCase("md"))
    {
        tools::Long nDays;
        if (d1 <= d2)
            nDays = d2 - d1;
        else
        {
            if (m2 == 1)
            {
                aDate1.SetYear(y2 == 1 ? -1 : y2 - 1);
                aDate1.SetMonth(12);
            }
            else
            {
                aDate1.SetYear(y2);
                aDate1.SetMonth(m2 - 1);
            }
            aDate1.Normalize();
            nDays = aDate2 - aDate1;
        }
        return static_cast<double>(nDays);
    }

    if (rInterval.equalsIgnoreAsciiCase("ym"))
    {
        int nMonths = m2 - m1 + 12 * (y2 - y1);
        if (d1 > d2)
            --nMonths;
        return static_cast<double>(nMonths % 12);
    }

    if (rInterval.equalsIgnoreAsciiCase("yd"))
    {
        if (m2 > m1 || (m2 == m1 && d2 >= d1))
            aDate1.SetYear(y2);
        else
            aDate1.SetYear(y2 - 1);
        aDate1.Normalize();
        return static_cast<double>(aDate2 - aDate1);
    }

    return std::nullopt;
}

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
