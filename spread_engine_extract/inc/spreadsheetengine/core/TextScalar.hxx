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

SPREADSHEETENGINE_DLLPUBLIC OUString trimRepeatedSpaces(const OUString& rInput);
SPREADSHEETENGINE_DLLPUBLIC sal_Int32 countCodePoints(const OUString& rInput);
SPREADSHEETENGINE_DLLPUBLIC NumberValueResult parseNumberValue(
    const OUString& rInput, const std::optional<OUString>& roDecimalSeparator,
    const std::optional<OUString>& roGroupSeparator, bool bEmptyStringAsZero);
SPREADSHEETENGINE_DLLPUBLIC OUString cleanPrintable(const OUString& rInput);
SPREADSHEETENGINE_DLLPUBLIC sal_Int32 codeFromText(const OUString& rInput);
SPREADSHEETENGINE_DLLPUBLIC std::optional<OUString> charFromValue(double fValue);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> unicodeFromText(const OUString& rInput);
SPREADSHEETENGINE_DLLPUBLIC std::optional<OUString> unicharFromCodePoint(sal_uInt32 nCodePoint);

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
