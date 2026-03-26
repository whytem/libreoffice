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

#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/runtime/TextServices.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::text
{

enum class NumberValueStatus
{
    Ok,
    NoValue,
    IllegalArgument,
};

struct NumberValueResult
{
    NumberValueStatus meStatus = NumberValueStatus::NoValue;
    double mfValue = 0.0;
};

SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String trimRepeatedSpaces(
    spreadsheetengine::api::StringView rInput);
SPREADSHEETENGINE_DLLPUBLIC sal_Int32 countCodePoints(
    spreadsheetengine::api::StringView rInput);
SPREADSHEETENGINE_DLLPUBLIC NumberValueResult parseNumberValue(
    spreadsheetengine::api::StringView rInput,
    const std::optional<spreadsheetengine::api::String>& roDecimalSeparator,
    const std::optional<spreadsheetengine::api::String>& roGroupSeparator, bool bEmptyStringAsZero);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String cleanPrintable(
    spreadsheetengine::api::StringView rInput);
SPREADSHEETENGINE_DLLPUBLIC sal_Int32 codeFromText(
    const SingleByteEncodingService& rEncodingService, spreadsheetengine::api::StringView rInput);
SPREADSHEETENGINE_DLLPUBLIC std::optional<spreadsheetengine::api::String> charFromValue(
    const SingleByteEncodingService& rEncodingService, double fValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> unicodeFromText(
    spreadsheetengine::api::StringView rInput);
SPREADSHEETENGINE_DLLPUBLIC std::optional<spreadsheetengine::api::String> unicharFromCodePoint(
    sal_uInt32 nCodePoint);

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
