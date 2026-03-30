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

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::convert
{

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateEuroConvertValue(
    double fValue, api::StringView rFromCurrency, api::StringView rToCurrency,
    bool bCaseInsensitive, bool bRoundToTargetDecimals);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateConvertValue(
    double fValue, api::StringView rFromUnit, api::StringView rToUnit);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateDecimalValue(
    api::StringView rText, double fBase);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::String> evaluateBaseValue(
    double fValue, double fBase, std::optional<double> ofMinLength = std::nullopt);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::String> evaluateRomanValue(
    double fValue, std::optional<double> ofMode = std::nullopt);

} // namespace spreadsheetengine::core::convert

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
