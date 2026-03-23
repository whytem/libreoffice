/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/ustring.hxx>
#include <unotools/charclass.hxx>

#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::text
{

SPREADSHEETENGINE_DLLPUBLIC OUString uppercase(
    const CharClass& rCharClass, const OUString& rInput);
SPREADSHEETENGINE_DLLPUBLIC OUString lowercase(
    const CharClass& rCharClass, const OUString& rInput);
SPREADSHEETENGINE_DLLPUBLIC OUString propercase(
    const CharClass& rCharClass, const OUString& rInput);

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
