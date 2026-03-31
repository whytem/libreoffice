/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <cstdint>

#include <algorithm>
#include <cmath>
#include <optional>

#include <spreadsheetengine/runtime/FloatingPoint.hxx>

#include "DateAlgorithms.hxx"

namespace spreadsheetengine::core::datetime
{

namespace sedate = spreadsheetengine::core::detail::date;

namespace
{

bool equalsIgnoreAsciiCase(spreadsheetengine::api::StringView rLeft,
    spreadsheetengine::api::StringView rRight)
{
    if (rLeft.size() != rRight.size())
        return false;

    auto toUpper = [](char16_t c) -> char16_t {
        if (c >= u'a' && c <= u'z')
            return c - (u'a' - u'A');
        return c;
    };

    for (std::size_t i = 0; i < rLeft.size(); ++i)
    {
        if (toUpper(rLeft[i]) != toUpper(rRight[i]))
            return false;
    }
    return true;
}

spreadsheetengine::api::DateParts getDateForSerial(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays)
{
    return sedate::fromAbsoluteDays(sedate::toAbsoluteDays(rNullDate) + nDays);
}

void splitClock(double fTimeInDays, std::uint16_t& nHour, std::uint16_t& nMinute, std::uint16_t& nSecond,
    double& fFractionOfSecond)
{
    constexpr double fSecondsPerMinute = 60.0;
    constexpr double fSecondsPerHour = 3600.0;
    constexpr double fSecondsPerDay = 86400.0;

    const double fTime = fTimeInDays - fp::approxFloor(fTimeInDays);
    if (fTime <= 0.0 || fTime >= 1.0)
    {
        nHour = nMinute = nSecond = 0;
        fFractionOfSecond = 0.0;
        return;
    }

    const double fRawSeconds = fTime * fSecondsPerDay;
    int nDec = 9;
    const double fAbsTimeInDays = std::fabs(fTimeInDays);
    if (fAbsTimeInDays >= 1.0)
    {
        const int nDig = static_cast<int>(std::ceil(std::log10(fAbsTimeInDays)));
        nDec = std::clamp(10 - nDig, 2, 9);
    }
    double fSeconds = fp::round(fRawSeconds, nDec, fp::RoundingMode::Corrected);
    if (fSeconds >= fSecondsPerDay)
        fSeconds = fRawSeconds;

    nHour = static_cast<std::uint16_t>(fSeconds / fSecondsPerHour);
    fSeconds -= nHour * fSecondsPerHour;
    nMinute = static_cast<std::uint16_t>(fSeconds / fSecondsPerMinute);
    fSeconds -= nMinute * fSecondsPerMinute;
    nSecond = static_cast<std::uint16_t>(fSeconds);
    fFractionOfSecond = fSeconds - nSecond;
}

}

std::optional<double> makeDateSerial(const spreadsheetengine::api::DateParts& rNullDate,
    std::int16_t nYear, std::int16_t nMonth, std::int16_t nDay, bool bStrict)
{
    std::int16_t nCalcYear = nYear;
    std::int16_t nCalcMonth = nMonth;
    std::int16_t nCalcDay = nDay;
    if (!bStrict)
    {
        while (nCalcMonth > 12)
        {
            nCalcMonth -= 12;
            ++nCalcYear;
            if (nCalcYear == 0)
                nCalcYear = 1;
        }
        while (nCalcMonth <= 0)
        {
            nCalcMonth += 12;
            --nCalcYear;
            if (nCalcYear == 0)
                nCalcYear = -1;
        }
        nCalcDay = 1;
    }

    if (!sedate::isValidDate(
            static_cast<std::uint16_t>(nCalcDay), static_cast<std::uint16_t>(nCalcMonth), nCalcYear))
    {
        return std::nullopt;
    }

    auto aDate = spreadsheetengine::api::DateParts{ nCalcYear, nCalcMonth, nCalcDay };
    if (!bStrict)
        aDate = sedate::fromAbsoluteDays(sedate::toAbsoluteDays(aDate) + (nDay - 1));

    if (!sedate::isValidAndGregorian(static_cast<std::uint16_t>(aDate.mnDay),
            static_cast<std::uint16_t>(aDate.mnMonth), static_cast<std::int16_t>(aDate.mnYear)))
    {
        return std::nullopt;
    }

    return static_cast<double>(sedate::toAbsoluteDays(aDate) - sedate::toAbsoluteDays(rNullDate));
}

double extractYear(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays)
{
    return static_cast<double>(getDateForSerial(rNullDate, nDays).mnYear);
}

double extractMonth(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays)
{
    return static_cast<double>(getDateForSerial(rNullDate, nDays).mnMonth);
}

std::optional<double> extractDay(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays)
{
    return static_cast<double>(getDateForSerial(rNullDate, nDays).mnDay);
}

double extractMinute(double fTimeValue)
{
    std::uint16_t nHour;
    std::uint16_t nMinute;
    std::uint16_t nSecond;
    double fFractionOfSecond;
    splitClock(fTimeValue, nHour, nMinute, nSecond, fFractionOfSecond);
    return nMinute;
}

double extractSecond(double fTimeValue)
{
    std::uint16_t nHour;
    std::uint16_t nMinute;
    std::uint16_t nSecond;
    double fFractionOfSecond;
    splitClock(fTimeValue, nHour, nMinute, nSecond, fFractionOfSecond);
    if (fFractionOfSecond >= 0.5)
        nSecond = (nSecond + 1) % 60;
    return nSecond;
}

double extractHour(double fTimeValue)
{
    std::uint16_t nHour;
    std::uint16_t nMinute;
    std::uint16_t nSecond;
    double fFractionOfSecond;
    splitClock(fTimeValue, nHour, nMinute, nSecond, fFractionOfSecond);
    return nHour;
}

std::optional<double> makeTimeSerial(double fHour, double fMinute, double fSecond)
{
    constexpr double fSecondsPerMinute = 60.0;
    constexpr double fSecondsPerHour = 3600.0;
    constexpr double fSecondsPerDay = 86400.0;

    const double fTime = std::fmod((fHour * fSecondsPerHour) + (fMinute * fSecondsPerMinute) + fSecond,
                             fSecondsPerDay)
                         / fSecondsPerDay;
    if (fTime < 0.0)
        return std::nullopt;
    return fTime;
}

double normalizeTimeFraction(double fTimeInDays)
{
    constexpr std::uint64_t nNanoSecondsPerSecond = 1000000000ULL;
    constexpr std::uint64_t nNanoSecondsPerDay = 86400ULL * nNanoSecondsPerSecond;
    constexpr std::uint64_t nAccuracyEpsilonNanoseconds = 300ULL;

    const double fTime = fTimeInDays - fp::approxFloor(fTimeInDays);
    if (fTime <= 0.0 || fTime >= 1.0)
        return 0.0;

    std::int64_t nNanoSeconds
        = static_cast<std::int64_t>(fp::approxFloor(fTime * nNanoSecondsPerDay));
    const std::int64_t nRemainder = nNanoSeconds % static_cast<std::int64_t>(nNanoSecondsPerSecond);
    if (nRemainder)
    {
        const std::uint64_t nDistance = std::abs(nRemainder);
        if (nDistance <= nAccuracyEpsilonNanoseconds)
            nNanoSeconds -= nRemainder;
        else if (nDistance >= nNanoSecondsPerSecond - nAccuracyEpsilonNanoseconds)
        {
            nNanoSeconds += static_cast<std::int64_t>(nNanoSecondsPerSecond - nDistance);
            if (nNanoSeconds >= static_cast<std::int64_t>(nNanoSecondsPerDay))
                nNanoSeconds %= static_cast<std::int64_t>(nNanoSecondsPerDay);
        }
    }

    return static_cast<double>(nNanoSeconds) / static_cast<double>(nNanoSecondsPerDay);
}

std::optional<double> computeEasterSundaySerial(
    const spreadsheetengine::api::DateParts& rNullDate, std::int16_t nYear)
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
    const std::int16_t nDay = static_cast<std::int16_t>(O % 31 + 1);
    const std::int16_t nMonth = static_cast<std::int16_t>(int(O / 31));
    return makeDateSerial(rNullDate, nYear, nMonth, nDay, true);
}

double computeDiffDate(double fDate1, double fDate2) { return fDate1 - fDate2; }

double computeDiffDate360(const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nDate1, spreadsheetengine::api::DateSerial nDate2,
    bool bEuropeanMethod)
{
    std::int32_t nSign = 1;
    if (bEuropeanMethod && (nDate2 < nDate1))
    {
        std::swap(nDate1, nDate2);
        nSign = -1;
    }

    auto aDate1 = getDateForSerial(rNullDate, nDate1);
    auto aDate2 = getDateForSerial(rNullDate, nDate2);
    if (aDate1.mnDay == 31)
        aDate1 = sedate::fromAbsoluteDays(sedate::toAbsoluteDays(aDate1) - 1);
    else if (!bEuropeanMethod && aDate1.mnMonth == 2)
    {
        switch (aDate1.mnDay)
        {
            case 28:
                if (!sedate::isLeapYear(static_cast<std::int16_t>(aDate1.mnYear)))
                    aDate1.mnDay = 30;
                break;
            case 29:
                aDate1.mnDay = 30;
                break;
        }
    }

    if (aDate2.mnDay == 31)
    {
        if (!bEuropeanMethod)
        {
            if (aDate1.mnDay == 30)
                aDate2 = sedate::fromAbsoluteDays(sedate::toAbsoluteDays(aDate2) - 1);
        }
        else
            aDate2.mnDay = 30;
    }

    return static_cast<double>(nSign)
           * (static_cast<double>(aDate2.mnDay) + static_cast<double>(aDate2.mnMonth) * 30.0
              + static_cast<double>(aDate2.mnYear) * 360.0
              - static_cast<double>(aDate1.mnDay) - static_cast<double>(aDate1.mnMonth) * 30.0
              - static_cast<double>(aDate1.mnYear) * 360.0);
}

std::optional<double> computeDateDif(const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nDate1, spreadsheetengine::api::DateSerial nDate2,
    spreadsheetengine::api::StringView rInterval)
{
    if (nDate1 > nDate2)
        return std::nullopt;

    const double fDaysDifference = nDate2 - nDate1;
    if (fDaysDifference == 0.0 || equalsIgnoreAsciiCase(rInterval, u"d"))
        return fDaysDifference;

    auto aDate1 = getDateForSerial(rNullDate, nDate1);
    auto aDate2 = getDateForSerial(rNullDate, nDate2);

    std::uint16_t d1 = static_cast<std::uint16_t>(aDate1.mnDay);
    std::uint16_t m1 = static_cast<std::uint16_t>(aDate1.mnMonth);
    std::uint16_t d2 = static_cast<std::uint16_t>(aDate2.mnDay);
    std::uint16_t m2 = static_cast<std::uint16_t>(aDate2.mnMonth);
    std::int16_t y1 = static_cast<std::int16_t>(aDate1.mnYear);
    std::int16_t y2 = static_cast<std::int16_t>(aDate2.mnYear);

    if (y1 < 0 && y2 > 0)
        ++y1;
    else if (y1 > 0 && y2 < 0)
        ++y2;

    if (equalsIgnoreAsciiCase(rInterval, u"m"))
    {
        int nMonths = m2 - m1 + 12 * (y2 - y1);
        if (d1 > d2)
            --nMonths;
        return static_cast<double>(nMonths);
    }

    if (equalsIgnoreAsciiCase(rInterval, u"y"))
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

    if (equalsIgnoreAsciiCase(rInterval, u"md"))
    {
        std::int32_t nDays;
        if (d1 <= d2)
            nDays = d2 - d1;
        else
        {
            if (m2 == 1)
            {
                aDate1.mnYear = (y2 == 1 ? -1 : y2 - 1);
                aDate1.mnMonth = 12;
            }
            else
            {
                aDate1.mnYear = y2;
                aDate1.mnMonth = static_cast<std::int16_t>(m2 - 1);
            }
            auto nDay = static_cast<std::uint16_t>(aDate1.mnDay);
            auto nMonth = static_cast<std::uint16_t>(aDate1.mnMonth);
            auto nYear = static_cast<std::int16_t>(aDate1.mnYear);
            sedate::normalize(nDay, nMonth, nYear);
            aDate1 = { nYear, static_cast<std::int16_t>(nMonth), static_cast<std::int16_t>(nDay) };
            nDays = sedate::toAbsoluteDays(aDate2) - sedate::toAbsoluteDays(aDate1);
        }
        return static_cast<double>(nDays);
    }

    if (equalsIgnoreAsciiCase(rInterval, u"ym"))
    {
        int nMonths = m2 - m1 + 12 * (y2 - y1);
        if (d1 > d2)
            --nMonths;
        return static_cast<double>(nMonths % 12);
    }

    if (equalsIgnoreAsciiCase(rInterval, u"yd"))
    {
        if (m2 > m1 || (m2 == m1 && d2 >= d1))
            aDate1.mnYear = y2;
        else
            aDate1.mnYear = (y2 == 1 ? -1 : y2 - 1);

        auto nDay = static_cast<std::uint16_t>(aDate1.mnDay);
        auto nMonth = static_cast<std::uint16_t>(aDate1.mnMonth);
        auto nYear = static_cast<std::int16_t>(aDate1.mnYear);
        sedate::normalize(nDay, nMonth, nYear);
        aDate1 = { nYear, static_cast<std::int16_t>(nMonth), static_cast<std::int16_t>(nDay) };
        return static_cast<double>(sedate::toAbsoluteDays(aDate2) - sedate::toAbsoluteDays(aDate1));
    }

    return std::nullopt;
}

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
