/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/TextWidth.hxx>

namespace spreadsheetengine::core::text
{

spreadsheetengine::api::String convertIntoHalfWidth(
    const WidthConversionService& rWidthService, spreadsheetengine::api::StringView rInput)
{
    return rWidthService.toHalfWidth(rInput);
}

spreadsheetengine::api::String convertIntoFullWidth(
    const WidthConversionService& rWidthService, spreadsheetengine::api::StringView rInput)
{
    return rWidthService.toFullWidth(rInput);
}

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
