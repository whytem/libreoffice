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

#pragma once

#include "analysishelper.hxx"

#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Workday.hxx>

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

inline bool isFreqInvalid(sal_Int32 nFreq) { return nFreq != 1 && nFreq != 2 && nFreq != 4; }
inline double finiteOrThrow(double d)
{
    if (!std::isfinite(d))
        throw css::lang::IllegalArgumentException();
    return d;
}

inline spreadsheetengine::api::DateParts makeNullDatePartsFromSerial(sal_Int32 nNullDate)
{
    sal_uInt16 nDay = 0;
    sal_uInt16 nMonth = 0;
    sal_uInt16 nYear = 0;
    sca::analysis::DaysToDate(nNullDate, nDay, nMonth, nYear);
    return { static_cast<std::int16_t>(nYear), static_cast<std::int16_t>(nMonth),
        static_cast<std::int16_t>(nDay) };
}

struct HostDateContext
{
    sal_Int32 mnNullDate = 0;
    spreadsheetengine::api::DateParts maNullDate;
};

struct AddInDateServiceContext
{
    HostDateContext maHostDate;
    std::vector<spreadsheetengine::api::DateSerial> maHolidaySerials;
};

inline sal_Int32 getRequiredHostNullDate(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt)
{
    // Host-only: this comes from the live document's NullDate property.
    return sca::analysis::GetNullDate(xOpt);
}

inline HostDateContext getHostDateContext(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt)
{
    const sal_Int32 nNullDate = getRequiredHostNullDate(xOpt);
    return { nNullDate, makeNullDatePartsFromSerial(nNullDate) };
}

inline AddInDateServiceContext getAddInDateServiceContext(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt)
{
    AddInDateServiceContext aContext;
    aContext.maHostDate = getHostDateContext(xOpt);
    return aContext;
}

inline void populateHostHolidayList(sca::analysis::ScaAnyConverter& rAnyConv,
    const css::uno::Reference<css::beans::XPropertySet>& xOpt, const css::uno::Any& rHolidayAny,
    sal_Int32 nNullDate, sca::analysis::SortedIndividualInt32List& rHolidayList)
{
    // Host-only: holiday expansion depends on document-facing add-in inputs.
    rHolidayList.InsertHolidayList(rAnyConv, xOpt, rHolidayAny, nNullDate);
}

inline std::vector<spreadsheetengine::api::DateSerial> collectHostHolidaySerialsFromAddInInputs(
    sca::analysis::ScaAnyConverter& rAnyConv, const css::uno::Reference<css::beans::XPropertySet>& xOpt,
    const css::uno::Any& rHolidayAny, sal_Int32 nNullDate)
{
    // Host-only: this still depends on UNO/add-in holiday inputs from the live document.
    sca::analysis::SortedIndividualInt32List aHolidayList;
    populateHostHolidayList(rAnyConv, xOpt, rHolidayAny, nNullDate, aHolidayList);

    std::vector<spreadsheetengine::api::DateSerial> aHolidaySerials;
    aHolidaySerials.reserve(aHolidayList.Count());
    for (sal_uInt32 nIndex = 0; nIndex < aHolidayList.Count(); ++nIndex)
    {
        aHolidaySerials.push_back(static_cast<spreadsheetengine::api::DateSerial>(
            aHolidayList.Get(nIndex) - nNullDate));
    }
    return aHolidaySerials;
}

inline AddInDateServiceContext getAddInDateServiceContext(sca::analysis::ScaAnyConverter& rAnyConv,
    const css::uno::Reference<css::beans::XPropertySet>& xOpt, const css::uno::Any& rHolidayAny)
{
    AddInDateServiceContext aContext = getAddInDateServiceContext(xOpt);
    aContext.maHolidaySerials = collectHostHolidaySerialsFromAddInInputs(
        rAnyConv, xOpt, rHolidayAny, aContext.maHostDate.mnNullDate);
    return aContext;
}

inline double valueOrThrow(spreadsheetengine::api::ValueResult<double> aResult)
{
    if (!aResult)
        throw css::lang::IllegalArgumentException();
    return finiteOrThrow(aResult.maValue);
}

template <typename Function, typename... Args>
inline double evaluateFinancialWithDateMode(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt, sal_Int32 nBasis,
    Function&& rFunction, Args&&... rArgs)
{
    const auto aDateContext = getAddInDateServiceContext(xOpt);
    return valueOrThrow(std::invoke(std::forward<Function>(rFunction),
        aDateContext.maHostDate.maNullDate, std::forward<Args>(rArgs)..., nBasis));
}

template <typename Function, typename... Args>
inline double evaluateFinancialWithNullDate(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt, Function&& rFunction,
    Args&&... rArgs)
{
    const auto aDateContext = getAddInDateServiceContext(xOpt);
    return valueOrThrow(std::invoke(std::forward<Function>(rFunction),
        aDateContext.maHostDate.maNullDate, std::forward<Args>(rArgs)...));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
