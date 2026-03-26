/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <calcconfig.hxx>
#include <formula/FormulaCompiler.hxx>

#include <optional>

#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::String toApiConfigOpCodeSymbol(
    const formula::FormulaCompiler::OpCodeMap& rMap, OpCode eOpCode)
{
    return toApiString(rMap.getSymbol(eOpCode));
}

inline std::optional<OpCode> findCalcEnglishOpCode(
    const formula::FormulaCompiler::OpCodeMap& rMap, spreadsheetengine::api::StringView rToken)
{
    const OUString aElement = toLibreOfficeString(spreadsheetengine::api::String(rToken));
    const sal_Int32 nValue = aElement.toInt32();
    if (nValue > 0 || (nValue == 0 && aElement == "0"))
        return static_cast<OpCode>(nValue);

    const formula::OpCodeHashMap& rHashMap(rMap.getHashMap());
    auto it = rHashMap.find(aElement);
    if (it != rHashMap.end())
        return it->second;

    return std::nullopt;
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
