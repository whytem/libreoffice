/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>

#include <spreadsheetengine/api/Compiler.hxx>
#include <spreadsheetengine/api/Grammar.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::compiler
{

SPREADSHEETENGINE_DLLPUBLIC const std::array<spreadsheetengine::api::CompilerCharFlags, 128>&
getCharTable(spreadsheetengine::api::AddressConvention eConv);

} // namespace spreadsheetengine::core::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
