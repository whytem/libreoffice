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
#include <spreadsheetengine/core/NumeralConversion.hxx>

namespace spreadsheetengine::api::numeral
{

inline api::ValueResult<api::String> toBase(
    double fValue, double fBase, std::optional<double> ofMinLength = std::nullopt)
{
    const auto aResult = spreadsheetengine::core::convert::convertToBase(fValue, fBase, ofMinLength);
    switch (aResult.meError)
    {
        case spreadsheetengine::core::convert::NumeralStringError::None:
            return api::ValueResult<api::String>::success(aResult.maValue);
        case spreadsheetengine::core::convert::NumeralStringError::StringOverflow:
            return api::ValueResult<api::String>::failure(api::Error::StringOverflow);
        default:
            return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
    }
}

inline api::ValueResult<double> fromBase(api::StringView rText, double fBase)
{
    if (auto oValue = spreadsheetengine::core::convert::convertFromBase(rText, fBase))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<api::String> toRoman(
    double fValue, std::optional<double> ofMode = std::nullopt)
{
    if (auto oValue = spreadsheetengine::core::convert::convertToRoman(fValue, ofMode))
        return api::ValueResult<api::String>::success(*oValue);
    return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<sal_Int32> fromRoman(api::StringView rRoman)
{
    if (auto oValue = spreadsheetengine::core::convert::convertFromRoman(rRoman))
        return api::ValueResult<sal_Int32>::success(*oValue);
    return api::ValueResult<sal_Int32>::failure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::api::numeral

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
