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
#include <vector>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::math
{

struct AggregateOptions
{
    bool mbIgnoreHiddenRows = false;
    bool mbIgnoreErrors = false;
    bool mbIgnoreNestedAggregates = false;
};

struct AggregateScan
{
    std::vector<double> maNumbers;
    sal_Int32 mnNonEmptyCount = 0;
};

SPREADSHEETENGINE_DLLPUBLIC std::optional<AggregateOptions> decodeAggregateOptions(
    sal_Int32 nOption);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateExtremaNumbers(
    const std::vector<double>& rNumbers, bool bFindMaximum, bool bDefaultZeroIfEmpty);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateVarianceNumbers(
    const std::vector<double>& rNumbers, bool bSample, bool bReturnStdDev);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateTrimmean(
    std::vector<double> aValues, double fPercent);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateGeometricMeanNumbers(
    const std::vector<double>& rValues);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateHarmonicMeanNumbers(
    const std::vector<double>& rValues);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateModeSingle(
    const std::vector<double>& rValues);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<std::vector<double>> evaluateModeValues(
    const std::vector<double>& rValues);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateHypergeometricDistribution(
    double fX, double fTrials, double fSuccesses, double fPopulation, bool bCumulative);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluatePercentrank(
    std::vector<double> aValues, double fValue, bool bInclusive, sal_Int32 nSignificance);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateSkewNumbers(
    const std::vector<double>& rValues, bool bPopulation);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateKurtosisNumbers(
    const std::vector<double>& rValues);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateAggregateNumbers(
    sal_Int32 nFunction, const AggregateScan& rScan);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateAggregateRankedNumbers(
    sal_Int32 nFunction, const AggregateScan& rScan, double fRankValue);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
