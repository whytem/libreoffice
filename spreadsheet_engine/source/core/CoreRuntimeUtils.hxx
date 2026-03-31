/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/String.hxx>

namespace spreadsheetengine::core::util
{

[[nodiscard]] inline api::String uppercaseAscii(api::StringView rValue)
{
    api::String aResult;
    aResult.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar >= u'a' && cChar <= u'z')
            aResult.push_back(static_cast<char16_t>(cChar - u'a' + u'A'));
        else
            aResult.push_back(cChar);
    }
    return aResult;
}

[[nodiscard]] inline std::optional<double> parseAsciiDouble(api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    std::string aAscii;
    aAscii.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar > 0x7f)
            return std::nullopt;
        aAscii.push_back(static_cast<char>(cChar));
    }

    char* pEnd = nullptr;
    const double fValue = std::strtod(aAscii.c_str(), &pEnd);
    if (!pEnd || *pEnd != '\0')
        return std::nullopt;

    return fValue;
}

[[nodiscard]] inline api::ValueResult<double> coerceToNumber(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<double>::success(0.0);
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return api::ValueResult<double>::success(rValue.mfNumber);
        case api::CellValueKind::Text:
        {
            if (auto oValue = parseAsciiDouble(rValue.maString))
                return api::ValueResult<double>::success(*oValue);
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        }
        case api::CellValueKind::Error:
            return api::ValueResult<double>::failure(rValue.meError);
    }

    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

[[nodiscard]] inline std::optional<std::int32_t> toWholeNumber(double fValue)
{
    if (!std::isfinite(fValue))
        return std::nullopt;

    const double fRounded = std::round(fValue);
    if (std::abs(fValue - fRounded) > 1e-9)
        return std::nullopt;

    return static_cast<std::int32_t>(fRounded);
}

[[nodiscard]] inline api::ValueResult<double> makeFiniteResult(double fValue)
{
    if (!std::isfinite(fValue))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(fValue);
}

} // namespace spreadsheetengine::core::util

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
