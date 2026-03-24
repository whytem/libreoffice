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

#include <sal/types.h>
#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::datetime
{

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> makeDateSerial(
    const spreadsheetengine::api::DateParts& rNullDate, sal_Int16 nYear, sal_Int16 nMonth,
    sal_Int16 nDay, bool bStrict);

SPREADSHEETENGINE_DLLPUBLIC double extractYear(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays);

SPREADSHEETENGINE_DLLPUBLIC double extractMonth(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> extractDay(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays);

SPREADSHEETENGINE_DLLPUBLIC double extractMinute(double fTimeValue);

SPREADSHEETENGINE_DLLPUBLIC double extractSecond(double fTimeValue);

SPREADSHEETENGINE_DLLPUBLIC double extractHour(double fTimeValue);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> makeTimeSerial(
    double fHour, double fMinute, double fSecond);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeEasterSundaySerial(
    const spreadsheetengine::api::DateParts& rNullDate, sal_Int16 nYear);

SPREADSHEETENGINE_DLLPUBLIC double computeDiffDate(double fDate1, double fDate2);

SPREADSHEETENGINE_DLLPUBLIC double computeDiffDate360(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDate1,
    spreadsheetengine::api::DateSerial nDate2, bool bEuropeanMethod);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeDateDif(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDate1,
    spreadsheetengine::api::DateSerial nDate2, spreadsheetengine::api::StringView rInterval);

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
