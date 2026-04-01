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
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>

#include <rtl/math.hxx>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::core::coercion
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

[[nodiscard]] inline api::ValueResult<bool> coerceToBoolean(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<bool>::success(false);
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return api::ValueResult<bool>::success(rValue.mfNumber != 0.0);
        case api::CellValueKind::Text:
        {
            const api::String aUpper = uppercaseAscii(rValue.maString);
            if (aUpper == u"TRUE")
                return api::ValueResult<bool>::success(true);
            if (aUpper == u"FALSE")
                return api::ValueResult<bool>::success(false);
            if (auto oNumber = parseAsciiDouble(rValue.maString))
                return api::ValueResult<bool>::success(*oNumber != 0.0);
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
        }
        case api::CellValueKind::Error:
            return api::ValueResult<bool>::failure(rValue.meError);
    }

    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
}

[[nodiscard]] inline api::ValueResult<api::String> coerceToString(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<api::String>::success({});
        case api::CellValueKind::Number:
        {
            char aBuffer[32];
            const int nLength = std::snprintf(aBuffer, sizeof(aBuffer), "%.17G", rValue.mfNumber);
            const std::string aAscii(aBuffer, static_cast<std::size_t>(std::max(nLength, 0)));

            api::String aResult;
            aResult.reserve(aAscii.size());
            for (const char cChar : aAscii)
                aResult.push_back(static_cast<char16_t>(cChar));
            return api::ValueResult<api::String>::success(aResult);
        }
        case api::CellValueKind::Boolean:
            return api::ValueResult<api::String>::success(
                rValue.mfNumber != 0.0 ? api::String(u"TRUE") : api::String(u"FALSE"));
        case api::CellValueKind::Text:
            return api::ValueResult<api::String>::success(rValue.maString);
        case api::CellValueKind::Error:
            return api::ValueResult<api::String>::failure(rValue.meError);
    }

    return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
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

struct StringPositionArgumentCheck
{
    double mfSanitizedValue = 0.0;
    bool mbValid = false;
};

[[nodiscard]] inline StringPositionArgumentCheck checkStringPositionArgument(double fValue)
{
    if (!std::isfinite(fValue))
        return { -1.0, false };

    const double fFloored = rtl::math::approxFloor(fValue);
    if (fFloored < 0.0)
        return { 0.0, false };

    const double fMax = static_cast<double>(std::numeric_limits<sal_Int32>::max());
    if (fFloored > fMax)
        return { fMax, false };

    return { fFloored, true };
}

[[nodiscard]] inline api::ValueResult<sal_Int32> normalizeStringPositionArgument(double fValue)
{
    const auto aChecked = checkStringPositionArgument(fValue);
    if (!aChecked.mbValid)
        return api::ValueResult<sal_Int32>::failure(api::Error::IllegalArgument);
    return api::ValueResult<sal_Int32>::success(static_cast<sal_Int32>(aChecked.mfSanitizedValue));
}

[[nodiscard]] inline api::ValueResult<sal_Int32> normalizeOneBasedStringPositionArgument(
    double fValue)
{
    const auto aChecked = checkStringPositionArgument(fValue);
    if (!aChecked.mbValid || aChecked.mfSanitizedValue < 1.0)
        return api::ValueResult<sal_Int32>::failure(api::Error::IllegalArgument);
    return api::ValueResult<sal_Int32>::success(static_cast<sal_Int32>(aChecked.mfSanitizedValue));
}

[[nodiscard]] inline api::ValueResult<sal_Int32> normalizeNonNegativeLengthArgument(double fValue)
{
    const auto aChecked = checkStringPositionArgument(fValue);
    if (!aChecked.mbValid)
        return api::ValueResult<sal_Int32>::failure(api::Error::IllegalArgument);
    return api::ValueResult<sal_Int32>::success(static_cast<sal_Int32>(aChecked.mfSanitizedValue));
}

} // namespace spreadsheetengine::core::coercion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
