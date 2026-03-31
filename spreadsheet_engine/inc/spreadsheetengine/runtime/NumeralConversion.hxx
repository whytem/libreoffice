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

#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::convert
{

enum class NumeralStringError
{
    None,
    IllegalArgument,
    StringOverflow,
};

struct NumeralStringResult
{
    NumeralStringError meError = NumeralStringError::None;
    spreadsheetengine::api::String maValue;
};

SPREADSHEETENGINE_DLLPUBLIC NumeralStringResult convertToBase(
    double fValue, double fBase, std::optional<double> ofMinLength);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> convertFromBase(
    spreadsheetengine::api::StringView rText, double fBase);

SPREADSHEETENGINE_DLLPUBLIC std::optional<spreadsheetengine::api::String> convertToRoman(
    double fValue, std::optional<double> ofMode);

SPREADSHEETENGINE_DLLPUBLIC std::optional<sal_Int32> convertFromRoman(
    spreadsheetengine::api::StringView rRoman);

} // namespace spreadsheetengine::core::convert

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
