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

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/runtime/TextCase.hxx>
#include <spreadsheetengine/runtime/TextScalar.hxx>
#include <spreadsheetengine/runtime/TextServices.hxx>
#include <spreadsheetengine/runtime/TextWidth.hxx>

namespace spreadsheetengine::api::text
{

inline api::String trimRepeatedSpaces(api::StringView rInput)
{
    return spreadsheetengine::core::text::trimRepeatedSpaces(rInput);
}

inline sal_Int32 countCodePoints(api::StringView rInput)
{
    return spreadsheetengine::core::text::countCodePoints(rInput);
}

inline api::ValueResult<double> parseNumberValue(api::StringView rInput,
    const std::optional<api::String>& roDecimalSeparator,
    const std::optional<api::String>& roGroupSeparator, bool bEmptyStringAsZero)
{
    const auto aResult = spreadsheetengine::core::text::parseNumberValue(
        rInput, roDecimalSeparator, roGroupSeparator, bEmptyStringAsZero);
    switch (aResult.meStatus)
    {
        case spreadsheetengine::core::text::NumberValueStatus::Ok:
            return api::ValueResult<double>::success(aResult.mfValue);
        case spreadsheetengine::core::text::NumberValueStatus::NoValue:
            return api::ValueResult<double>::failure(api::Error::NoValue);
        default:
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }
}

inline api::String cleanPrintable(api::StringView rInput)
{
    return spreadsheetengine::core::text::cleanPrintable(rInput);
}

inline sal_Int32 codeFromText(
    const spreadsheetengine::core::text::SingleByteEncodingService& rEncodingService,
    api::StringView rInput)
{
    return spreadsheetengine::core::text::codeFromText(rEncodingService, rInput);
}

inline api::ValueResult<api::String> charFromValue(
    const spreadsheetengine::core::text::SingleByteEncodingService& rEncodingService, double fValue)
{
    if (auto oValue = spreadsheetengine::core::text::charFromValue(rEncodingService, fValue))
        return api::ValueResult<api::String>::success(*oValue);
    return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<double> unicodeFromText(api::StringView rInput)
{
    if (auto oValue = spreadsheetengine::core::text::unicodeFromText(rInput))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

inline api::ValueResult<api::String> unicharFromCodePoint(sal_uInt32 nCodePoint)
{
    if (auto oValue = spreadsheetengine::core::text::unicharFromCodePoint(nCodePoint))
        return api::ValueResult<api::String>::success(*oValue);
    return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
}

inline api::String uppercase(
    const spreadsheetengine::core::text::CaseMappingService& rCaseService, api::StringView rInput)
{
    return spreadsheetengine::core::text::uppercase(rCaseService, rInput);
}

inline api::String lowercase(
    const spreadsheetengine::core::text::CaseMappingService& rCaseService, api::StringView rInput)
{
    return spreadsheetengine::core::text::lowercase(rCaseService, rInput);
}

inline api::String propercase(
    const spreadsheetengine::core::text::CaseMappingService& rCaseService, api::StringView rInput)
{
    return spreadsheetengine::core::text::propercase(rCaseService, rInput);
}

inline api::String convertIntoHalfWidth(
    const spreadsheetengine::core::text::WidthConversionService& rWidthService,
    api::StringView rInput)
{
    return spreadsheetengine::core::text::convertIntoHalfWidth(rWidthService, rInput);
}

inline api::String convertIntoFullWidth(
    const spreadsheetengine::core::text::WidthConversionService& rWidthService,
    api::StringView rInput)
{
    return spreadsheetengine::core::text::convertIntoFullWidth(rWidthService, rInput);
}

} // namespace spreadsheetengine::api::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
