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
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::datetime
{

struct WeekdayResult
{
    int mnValue = 0;
    bool mbValid = true;
};

SPREADSHEETENGINE_DLLPUBLIC WeekdayResult computeDayOfWeek(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays,
    sal_Int16 nFlag);

SPREADSHEETENGINE_DLLPUBLIC int computeWeeknumOOo(const spreadsheetengine::api::DateParts& rNullDate,
    spreadsheetengine::api::DateSerial nDays, sal_Int16 nFlag);

SPREADSHEETENGINE_DLLPUBLIC std::optional<int> computeWeekOfYear(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays,
    sal_Int16 nFlag);

SPREADSHEETENGINE_DLLPUBLIC int computeIsoWeekOfYear(
    const spreadsheetengine::api::DateParts& rNullDate, spreadsheetengine::api::DateSerial nDays);

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
