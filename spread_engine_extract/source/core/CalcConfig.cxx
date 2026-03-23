/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/CalcConfig.hxx>

#include <cstdlib>
#include <cstring>

#include <com/sun/star/sheet/FormulaLanguage.hpp>
#include <formula/FormulaCompiler.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

namespace spreadsheetengine::core
{

ForceCalculationMode getForceCalculationModeFromEnv()
{
    const char* env = std::getenv("SC_FORCE_CALCULATION");
    if (env == nullptr)
        return ForceCalculationMode::None;

    if (std::strcmp(env, "opencl") == 0)
    {
        SAL_INFO("sc.core.formulagroup", "Forcing calculations to use OpenCL");
        return ForceCalculationMode::OpenCL;
    }
    if (std::strcmp(env, "threads") == 0)
    {
        SAL_INFO("sc.core.formulagroup", "Forcing calculations to use threads");
        return ForceCalculationMode::Threads;
    }
    if (std::strcmp(env, "core") == 0)
    {
        SAL_INFO("sc.core.formulagroup", "Forcing calculations to use core");
        return ForceCalculationMode::Core;
    }

    SAL_WARN("sc.core.formulagroup", "Unrecognized value of SC_FORCE_CALCULATION");
    std::abort();
}

OUString formulaOpCodeSetToSymbolicString(const FormulaOpCodeSet& rOpCodes)
{
    OUStringBuffer result(256);
    formula::FormulaCompiler aCompiler;
    formula::FormulaCompiler::OpCodeMapPtr pOpCodeMap(
        aCompiler.GetOpCodeMap(css::sheet::FormulaLanguage::ENGLISH));

    for (auto i = rOpCodes->begin(); i != rOpCodes->end(); ++i)
    {
        if (i != rOpCodes->begin())
            result.append(';');
        result.append(pOpCodeMap->getSymbol(*i));
    }

    return result.makeStringAndClear();
}

FormulaOpCodeSet stringToFormulaOpCodeSet(std::u16string_view rOpCodes)
{
    FormulaOpCodeSet result = std::make_shared<o3tl::sorted_vector<OpCode>>();
    formula::FormulaCompiler aCompiler;
    formula::FormulaCompiler::OpCodeMapPtr pOpCodeMap(
        aCompiler.GetOpCodeMap(css::sheet::FormulaLanguage::ENGLISH));

    const formula::OpCodeHashMap& rHashMap(pOpCodeMap->getHashMap());

    sal_Int32 fromIndex = 0;
    sal_Int32 semicolon;
    OUString s(OUString::Concat(rOpCodes) + ";");

    while ((semicolon = s.indexOf(';', fromIndex)) >= 0)
    {
        if (semicolon > fromIndex)
        {
            OUString element(s.copy(fromIndex, semicolon - fromIndex));
            sal_Int32 n = element.toInt32();
            if (n > 0 || (n == 0 && element == "0"))
                result->insert(static_cast<OpCode>(n));
            else
            {
                auto opcode = rHashMap.find(element);
                if (opcode != rHashMap.end())
                    result->insert(opcode->second);
                else
                    SAL_WARN("sc.opencl", "Unrecognized OpCode " << element << " in OpCode set string");
            }
        }
        fromIndex = semicolon + 1;
    }

    // Unary and binary minus share a string representation but not an opcode.
    if (result->find(ocSub) != result->end())
        result->insert(ocNegSub);

    return result;
}

} // namespace spreadsheetengine::core

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
