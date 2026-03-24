/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <string_view>

#include <formula/opcode.hxx>
#include <o3tl/sorted_vector.hxx>
#include <rtl/ustring.hxx>
#include <spreadsheetengine/api/Config.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core
{

using FormulaOpCodeSet = std::shared_ptr<o3tl::sorted_vector<OpCode>>;

SPREADSHEETENGINE_DLLPUBLIC OUString formulaOpCodeSetToSymbolicString(const FormulaOpCodeSet& rOpCodes);
SPREADSHEETENGINE_DLLPUBLIC FormulaOpCodeSet stringToFormulaOpCodeSet(std::u16string_view rOpCodes);

} // namespace spreadsheetengine::core

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
