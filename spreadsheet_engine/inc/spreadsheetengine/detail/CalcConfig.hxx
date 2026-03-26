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
#include <string_view>
#include <vector>

#include <spreadsheetengine/api/Config.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core
{

using ConfigOpCodeSymbolList = std::vector<spreadsheetengine::api::ConfigOpCodeSymbol>;
using SymbolicOpCodeList = std::vector<spreadsheetengine::api::String>;

SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::StringView
configOpCodeSymbolName(spreadsheetengine::api::ConfigOpCodeSymbol eSymbol);
SPREADSHEETENGINE_DLLPUBLIC std::optional<spreadsheetengine::api::ConfigOpCodeSymbol>
findConfigOpCodeSymbol(spreadsheetengine::api::StringView rToken);
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String
configOpCodeSymbolListToString(const ConfigOpCodeSymbolList& rSymbols);
SPREADSHEETENGINE_DLLPUBLIC const ConfigOpCodeSymbolList&
defaultOpenCLSubsetConfigOpCodes();
SPREADSHEETENGINE_DLLPUBLIC spreadsheetengine::api::String
symbolicOpCodeListToString(const SymbolicOpCodeList& rOpCodes);
SPREADSHEETENGINE_DLLPUBLIC SymbolicOpCodeList
stringToSymbolicOpCodeList(std::u16string_view rOpCodes);
SPREADSHEETENGINE_DLLPUBLIC const SymbolicOpCodeList&
defaultOpenCLSubsetSymbolicOpCodes();

} // namespace spreadsheetengine::core

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
