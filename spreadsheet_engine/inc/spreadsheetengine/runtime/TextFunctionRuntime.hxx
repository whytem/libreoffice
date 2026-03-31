/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::text
{

SPREADSHEETENGINE_DLLPUBLIC std::optional<sal_Int32> findText(
    spreadsheetengine::api::StringView rNeedle, spreadsheetengine::api::StringView rHaystack,
    sal_Int32 nStart, bool bCaseInsensitive);
SPREADSHEETENGINE_DLLPUBLIC std::optional<std::size_t> findByteText(
    spreadsheetengine::api::StringView rNeedle, spreadsheetengine::api::StringView rHaystack,
    std::size_t nStartIndex, bool bCaseInsensitive);

SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String sliceText(
    spreadsheetengine::api::StringView rText, sal_Int32 nCodePointStart,
    sal_Int32 nCodePointLength);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String sliceTextLeftRight(
    spreadsheetengine::api::StringView rText, sal_Int32 nLength, bool bFromRight);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String replaceText(
    spreadsheetengine::api::StringView rSource, sal_Int32 nCodePointStart,
    sal_Int32 nCodePointLength, spreadsheetengine::api::StringView rReplacement);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String replaceByteText(
    spreadsheetengine::api::StringView rSource, std::size_t nStartIndex,
    std::size_t nReplaceLength, spreadsheetengine::api::StringView rReplacement);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String substituteText(
    spreadsheetengine::api::StringView rSource, spreadsheetengine::api::StringView rOldText,
    spreadsheetengine::api::StringView rNewText, std::optional<sal_Int32> oInstance);

SPREADSHEETENGINE_DLLPUBLIC std::optional<spreadsheetengine::api::String> textAfter(
    spreadsheetengine::api::StringView rText,
    const std::vector<spreadsheetengine::api::String>& rDelimiters, sal_Int32 nInstance,
    bool bCaseInsensitive, bool bMatchEnd);
SPREADSHEETENGINE_DLLPUBLIC std::optional<spreadsheetengine::api::String> textBefore(
    spreadsheetengine::api::StringView rText,
    const std::vector<spreadsheetengine::api::String>& rDelimiters, sal_Int32 nInstance,
    bool bCaseInsensitive, bool bMatchEnd);

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
