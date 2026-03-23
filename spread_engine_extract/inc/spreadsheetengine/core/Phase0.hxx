/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string_view>

#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core
{
inline constexpr std::string_view kLibraryName = "spreadsheetengine";

SPREADSHEETENGINE_DLLPUBLIC bool isLibraryLinked();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
