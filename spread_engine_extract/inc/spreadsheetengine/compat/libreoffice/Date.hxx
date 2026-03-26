/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <vector>

#include <tools/date.hxx>

#include <spreadsheetengine/api/Date.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::DateParts toApiDateParts(const Date& rDate)
{
    return { rDate.GetYear(), static_cast<std::int16_t>(rDate.GetMonth()),
        static_cast<std::int16_t>(rDate.GetDay()) };
}

inline spreadsheetengine::api::WeekendMask toApiWeekendMask(const bool bWeekendMask[7])
{
    spreadsheetengine::api::WeekendMask aWeekendMask {};
    std::copy_n(bWeekendMask, aWeekendMask.size(), aWeekendMask.begin());
    return aWeekendMask;
}

inline void toLibreOfficeWeekendMask(
    const spreadsheetengine::api::WeekendMask& rWeekendMask, bool bDestination[7])
{
    std::copy(rWeekendMask.begin(), rWeekendMask.end(), bDestination);
}

inline std::vector<spreadsheetengine::api::DateSerial> toApiDateSerials(
    const std::vector<double>& rSerials)
{
    std::vector<spreadsheetengine::api::DateSerial> aDateSerials;
    aDateSerials.reserve(rSerials.size());
    for (double fDate : rSerials)
        aDateSerials.push_back(static_cast<spreadsheetengine::api::DateSerial>(fDate));
    return aDateSerials;
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
