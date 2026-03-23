/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

class Date;

namespace spreadsheetengine::core::datetime
{

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> makeDateSerial(
    const Date& rNullDate, sal_Int16 nYear, sal_Int16 nMonth, sal_Int16 nDay, bool bStrict);

SPREADSHEETENGINE_DLLPUBLIC double extractYear(const Date& rNullDate, sal_Int32 nDays);

SPREADSHEETENGINE_DLLPUBLIC double extractMonth(const Date& rNullDate, sal_Int32 nDays);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> extractDay(const Date& rNullDate, sal_Int32 nDays);

SPREADSHEETENGINE_DLLPUBLIC double extractMinute(double fTimeValue);

SPREADSHEETENGINE_DLLPUBLIC double extractSecond(double fTimeValue);

SPREADSHEETENGINE_DLLPUBLIC double extractHour(double fTimeValue);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> makeTimeSerial(
    double fHour, double fMinute, double fSecond);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeEasterSundaySerial(
    const Date& rNullDate, sal_Int16 nYear);

SPREADSHEETENGINE_DLLPUBLIC double computeDiffDate(double fDate1, double fDate2);

SPREADSHEETENGINE_DLLPUBLIC double computeDiffDate360(
    const Date& rNullDate, sal_Int32 nDate1, sal_Int32 nDate2, bool bEuropeanMethod);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeDateDif(
    const Date& rNullDate, sal_Int32 nDate1, sal_Int32 nDate2, const OUString& rInterval);

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
