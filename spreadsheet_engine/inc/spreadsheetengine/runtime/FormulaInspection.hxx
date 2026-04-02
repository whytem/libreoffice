/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::runtime::formulainspection
{

class Provider
{
public:
    virtual ~Provider() = default;

    [[nodiscard]] virtual bool isFormulaCell(const api::CellAddress& rAddress) const = 0;

    [[nodiscard]] virtual api::ValueResult<api::String> formulaTextForCell(
        const api::CellAddress& rAddress) const = 0;
};

[[nodiscard]] inline api::CellValue isFormulaValue(
    const Provider& rProvider, const api::CellAddress& rAddress)
{
    return api::CellValue::boolean(rProvider.isFormulaCell(rAddress));
}

[[nodiscard]] inline api::ValueResult<api::String> formulaTextValue(
    const Provider& rProvider, const api::CellAddress& rAddress)
{
    return rProvider.formulaTextForCell(rAddress);
}

} // namespace spreadsheetengine::runtime::formulainspection

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
