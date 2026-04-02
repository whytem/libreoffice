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

#include <spreadsheetengine/api/Date.hxx>
#include <spreadsheetengine/api/Error.hxx>

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <cmath>
#include <functional>
#include <utility>

inline bool isFreqInvalid(sal_Int32 nFreq) { return nFreq != 1 && nFreq != 2 && nFreq != 4; }
inline double finiteOrThrow(double d)
{
    if (!std::isfinite(d))
        throw css::lang::IllegalArgumentException();
    return d;
}

inline spreadsheetengine::api::DateParts getNullDateParts(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt)
{
    sal_uInt16 nDay = 0;
    sal_uInt16 nMonth = 0;
    sal_uInt16 nYear = 0;
    sca::analysis::DaysToDate(sca::analysis::GetNullDate(xOpt), nDay, nMonth, nYear);
    return { static_cast<std::int16_t>(nYear), static_cast<std::int16_t>(nMonth),
        static_cast<std::int16_t>(nDay) };
}

struct FinancialDateContext
{
    spreadsheetengine::api::DateParts maNullDate;
    sal_Int32 mnBasis = 0;
};

inline FinancialDateContext getFinancialDateContext(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt, sal_Int32 nBasis)
{
    return { getNullDateParts(xOpt), nBasis };
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
    const auto aContext = getFinancialDateContext(xOpt, nBasis);
    return valueOrThrow(std::invoke(std::forward<Function>(rFunction), aContext.maNullDate,
        std::forward<Args>(rArgs)..., aContext.mnBasis));
}

template <typename Function, typename... Args>
inline double evaluateFinancialWithNullDate(
    const css::uno::Reference<css::beans::XPropertySet>& xOpt, Function&& rFunction,
    Args&&... rArgs)
{
    return valueOrThrow(std::invoke(std::forward<Function>(rFunction), getNullDateParts(xOpt),
        std::forward<Args>(rArgs)...));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
