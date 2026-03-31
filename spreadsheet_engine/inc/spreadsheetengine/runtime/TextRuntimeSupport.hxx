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
#include <spreadsheetengine/runtime/TextServices.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::text
{

SPREADSHEETENGINE_DLLPUBLIC const CaseMappingService& defaultCaseMappingService();
SPREADSHEETENGINE_DLLPUBLIC const SingleByteEncodingService& defaultSingleByteEncodingService();
SPREADSHEETENGINE_DLLPUBLIC const WidthConversionService& defaultWidthConversionService();

SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String substringByCodePoints(
    spreadsheetengine::api::StringView rText, sal_Int32 nCodePointStart,
    sal_Int32 nCodePointLength);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String replaceByCodePoints(
    spreadsheetengine::api::StringView rText, sal_Int32 nCodePointStart,
    sal_Int32 nCodePointLength, spreadsheetengine::api::StringView rReplacement);
SPREADSHEETENGINE_DLLPUBLIC std::optional<sal_Int32> findTextCodePointIndex(
    spreadsheetengine::api::StringView rNeedle, spreadsheetengine::api::StringView rHaystack,
    sal_Int32 nCodePointStart, bool bCaseInsensitive);

SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String expandDbcsByteText(
    spreadsheetengine::api::StringView rText, bool bFoldAscii);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String collapseDbcsByteText(
    spreadsheetengine::api::StringView rExpandedText);
SPREADSHEETENGINE_DLLPUBLIC std::optional<std::size_t> findDbcsExpandedText(
    spreadsheetengine::api::StringView rNeedle, spreadsheetengine::api::StringView rHaystack,
    std::size_t nStartIndex, bool bFoldAscii);

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
