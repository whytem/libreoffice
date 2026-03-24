/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <string_view>

#include <spreadsheetengine/api/Config.hxx>
#include <spreadsheetengine/core/ForceCalculation.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::ForceCalculationMode;
    using spreadsheetengine::standalone::test::fail;

    const auto eNone = spreadsheetengine::core::config::parseForceCalculationMode(std::nullopt);
    if (!eNone || *eNone != ForceCalculationMode::None)
        return fail("spreadsheetengine_config_tests", "empty force-calculation mode mismatch");

    const auto eOpenCL
        = spreadsheetengine::core::config::parseForceCalculationMode(std::string_view("opencl"));
    if (!eOpenCL || *eOpenCL != ForceCalculationMode::OpenCL)
        return fail("spreadsheetengine_config_tests", "opencl force-calculation mode mismatch");

    const auto eThreads
        = spreadsheetengine::core::config::parseForceCalculationMode(std::string_view("threads"));
    if (!eThreads || *eThreads != ForceCalculationMode::Threads)
        return fail("spreadsheetengine_config_tests", "threads force-calculation mode mismatch");

    const auto eCore
        = spreadsheetengine::core::config::parseForceCalculationMode(std::string_view("core"));
    if (!eCore || *eCore != ForceCalculationMode::Core)
        return fail("spreadsheetengine_config_tests", "core force-calculation mode mismatch");

    if (spreadsheetengine::core::config::parseForceCalculationMode(std::string_view("bogus")))
        return fail("spreadsheetengine_config_tests", "invalid force-calculation mode mismatch");

    std::cout << "spreadsheetengine config api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
