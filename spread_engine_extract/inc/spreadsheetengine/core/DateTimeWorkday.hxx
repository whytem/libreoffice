/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>

#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::datetime
{

SPREADSHEETENGINE_DLLPUBLIC void setDefaultWeekendMask(bool bWeekendMask[7]);

SPREADSHEETENGINE_DLLPUBLIC bool applyWeekendMaskSequence(
    const std::vector<double>& rWeekendDays, bool bWeekendMask[7]);

SPREADSHEETENGINE_DLLPUBLIC bool applyWeekendMaskMsSpec(
    const OUString& rWeekendDays, bool bWorkdayFunction, bool bWeekendMask[7]);

SPREADSHEETENGINE_DLLPUBLIC sal_Int32 countWorkdays(
    sal_Int32 nDate1, sal_Int32 nDate2, const std::vector<double>& rSortedHolidays,
    const bool bWeekendMask[7]);

SPREADSHEETENGINE_DLLPUBLIC sal_Int32 advanceWorkday(
    sal_Int32 nDate, sal_Int32 nDays, const std::vector<double>& rSortedHolidays,
    const bool bWeekendMask[7]);

} // namespace spreadsheetengine::core::datetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
