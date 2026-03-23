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

#include <rtl/ustring.hxx>
#include <sal/types.h>

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
    OUString maValue;
};

SPREADSHEETENGINE_DLLPUBLIC NumeralStringResult convertToBase(
    double fValue, double fBase, std::optional<double> ofMinLength);

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> convertFromBase(
    const OUString& rText, double fBase);

SPREADSHEETENGINE_DLLPUBLIC std::optional<OUString> convertToRoman(
    double fValue, std::optional<double> ofMode);

SPREADSHEETENGINE_DLLPUBLIC std::optional<sal_Int32> convertFromRoman(
    const OUString& rRoman);

} // namespace spreadsheetengine::core::convert

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
