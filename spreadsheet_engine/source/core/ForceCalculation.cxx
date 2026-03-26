/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/ForceCalculation.hxx>

#include <cstdlib>

#include <sal/log.hxx>

namespace spreadsheetengine::core::config
{

std::optional<spreadsheetengine::api::ForceCalculationMode> parseForceCalculationMode(
    const std::optional<std::string_view> oValue)
{
    if (!oValue)
        return spreadsheetengine::api::ForceCalculationMode::None;

    if (*oValue == "opencl")
        return spreadsheetengine::api::ForceCalculationMode::OpenCL;
    if (*oValue == "threads")
        return spreadsheetengine::api::ForceCalculationMode::Threads;
    if (*oValue == "core")
        return spreadsheetengine::api::ForceCalculationMode::Core;

    return std::nullopt;
}

spreadsheetengine::api::ForceCalculationMode getForceCalculationModeFromEnv()
{
    const char* env = std::getenv("SC_FORCE_CALCULATION");
    const auto oMode = parseForceCalculationMode(
        env ? std::optional<std::string_view>(env) : std::nullopt);
    if (!oMode)
    {
        SAL_WARN("sc.core.formulagroup", "Unrecognized value of SC_FORCE_CALCULATION");
        std::abort();
    }

    switch (*oMode)
    {
        case spreadsheetengine::api::ForceCalculationMode::OpenCL:
            SAL_INFO("sc.core.formulagroup", "Forcing calculations to use OpenCL");
            break;
        case spreadsheetengine::api::ForceCalculationMode::Threads:
            SAL_INFO("sc.core.formulagroup", "Forcing calculations to use threads");
            break;
        case spreadsheetengine::api::ForceCalculationMode::Core:
            SAL_INFO("sc.core.formulagroup", "Forcing calculations to use core");
            break;
        case spreadsheetengine::api::ForceCalculationMode::None:
        default:
            break;
    }

    return *oMode;
}

} // namespace spreadsheetengine::core::config

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
