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
#include <optional>

#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/TokenModel.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>

namespace spreadsheetengine::detail::compiler
{

[[nodiscard]] std::optional<std::unique_ptr<spreadsheetengine::core::formula::Node>>
inflateCompiledFormulaNode(const spreadsheetengine::detail::token::CompiledFormula& rFormula,
    const spreadsheetengine::core::workbook::Workbook& rWorkbook,
    const spreadsheetengine::api::CellAddress& rCurrentAddress);

} // namespace spreadsheetengine::detail::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
