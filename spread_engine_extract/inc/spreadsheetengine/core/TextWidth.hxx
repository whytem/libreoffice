/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/core/TextServices.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::text
{

SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String convertIntoHalfWidth(
    const WidthConversionService& rWidthService, spreadsheetengine::api::StringView rInput);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String convertIntoFullWidth(
    const WidthConversionService& rWidthService, spreadsheetengine::api::StringView rInput);

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
