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

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::datetime
{

SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::DateParts defaultNullDate();

SPREADSHEETENGINE_DLLPUBLIC std::optional<spreadsheetengine::api::NumberParseResult>
parseStandaloneNumberText(spreadsheetengine::api::StringView rValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> parseStoredDateValue(
    spreadsheetengine::api::StringView rValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> parseOdfTimeDuration(
    spreadsheetengine::api::StringView rValue);

SPREADSHEETENGINE_DLLPUBLIC std::optional<spreadsheetengine::api::DateSerial> coerceToDateSerial(
    const spreadsheetengine::api::CellValue& rValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> shiftMonthSerial(
    spreadsheetengine::api::DateSerial nDateSerial, sal_Int32 nMonthOffset, bool bEndOfMonth);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeWeeksDifference(
    spreadsheetengine::api::DateSerial nStartDate, spreadsheetengine::api::DateSerial nEndDate,
    sal_Int16 nMode);

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
