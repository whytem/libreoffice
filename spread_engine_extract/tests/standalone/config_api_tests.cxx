/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <algorithm>
#include <iostream>
#include <string_view>

#include <spreadsheetengine/api/Config.hxx>
#include <spreadsheetengine/core/CalcConfig.hxx>
#include <spreadsheetengine/core/ForceCalculation.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::ConfigOpCodeSymbol;
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

    const auto aSymbolicTokens
        = spreadsheetengine::core::stringToSymbolicOpCodeList(u"ADD;;42;SUB");
    if (aSymbolicTokens.size() != 3 || aSymbolicTokens[0] != u"ADD" || aSymbolicTokens[1] != u"42"
        || aSymbolicTokens[2] != u"SUB")
    {
        return fail("spreadsheetengine_config_tests", "symbolic opcode tokenization mismatch");
    }

    const auto aRoundTrip
        = spreadsheetengine::core::symbolicOpCodeListToString(aSymbolicTokens);
    if (aRoundTrip != u"ADD;42;SUB")
        return fail("spreadsheetengine_config_tests", "symbolic opcode formatting mismatch");

    if (spreadsheetengine::core::configOpCodeSymbolName(ConfigOpCodeSymbol::SumIfs) != u"SUMIFS"
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"SUMIFS")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::SumIfs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"+")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Add)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"ADD")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Add)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"NORMSDIST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::StdNormDistLegacy)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"STD.NORM.DIST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::StdNormDistMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"COUNTIFS")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::CountIfs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"XLOOKUP")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::XLookup)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"PV")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Pv)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"RATE")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Rate)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"VDB")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Vdb)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"STDEV")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::StDev)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"GEOMEAN")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::GeoMean)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"RSQ")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Rsq)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"INTERCEPT")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Intercept)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"DSUM")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::DbSum)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"DSTDEVP")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::DbStdDevP)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"VAR.P")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::VarPMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"STDEV.S")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::StDevSMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"NORM.INV")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::NormInvMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"T.DIST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::TDistMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"F.DIST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::FDistMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"CHISQ.DIST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::ChiSqDistMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"CHISQ.INV")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::ChiSqInvMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"Z.TEST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::ZTestMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"T.TEST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::TTestMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"F.TEST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::FTestMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"GAMMA.DIST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::GammaDistMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"GAMMA.INV")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::GammaInvMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"T.INV")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::TInvMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"F.INV")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::FInvMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"CHISQ.INV.RT")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::ChiInvMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"LOGNORM.DIST")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::LogNormDistMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"LOGNORM.INV")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::LogInvMs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"ABS")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Abs)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"TRUNC")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Trunc)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"NA")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Na)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"CSC")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Csc)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"FILTER")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Filter)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"SEQUENCE")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::Sequence)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"RANDARRAY")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::RandArray)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"BITAND")
               != std::optional<ConfigOpCodeSymbol>(ConfigOpCodeSymbol::BitAnd)
        || spreadsheetengine::core::findConfigOpCodeSymbol(u"BOGUS"))
    {
        return fail("spreadsheetengine_config_tests", "config opcode symbol mismatch");
    }

    const auto aKnownRoundTrip = spreadsheetengine::core::configOpCodeSymbolListToString(
        { ConfigOpCodeSymbol::Add, ConfigOpCodeSymbol::Abs, ConfigOpCodeSymbol::Pv,
            ConfigOpCodeSymbol::Trunc, ConfigOpCodeSymbol::VarPMs, ConfigOpCodeSymbol::Filter,
            ConfigOpCodeSymbol::Sequence, ConfigOpCodeSymbol::RandArray,
            ConfigOpCodeSymbol::ChiSqInvMs, ConfigOpCodeSymbol::XLookup });
    if (aKnownRoundTrip != u"+;ABS;PV;TRUNC;VAR.P;FILTER;SEQUENCE;RANDARRAY;CHISQ.INV;XLOOKUP")
        return fail("spreadsheetengine_config_tests", "config opcode list formatting mismatch");

    const auto& rDefaultOpenCLConfigSymbols
        = spreadsheetengine::core::defaultOpenCLSubsetConfigOpCodes();
    if (rDefaultOpenCLConfigSymbols.size() < 10
        || std::find(rDefaultOpenCLConfigSymbols.begin(), rDefaultOpenCLConfigSymbols.end(),
               ConfigOpCodeSymbol::SumIfs)
               == rDefaultOpenCLConfigSymbols.end()
        || std::find(rDefaultOpenCLConfigSymbols.begin(), rDefaultOpenCLConfigSymbols.end(),
               ConfigOpCodeSymbol::VLookup)
               == rDefaultOpenCLConfigSymbols.end())
    {
        return fail("spreadsheetengine_config_tests", "default config opcode subset mismatch");
    }

    const auto& rDefaultOpenCLSubset = spreadsheetengine::core::defaultOpenCLSubsetSymbolicOpCodes();
    if (rDefaultOpenCLSubset.size() < 10
        || rDefaultOpenCLSubset.front() != u"+"
        || std::find(rDefaultOpenCLSubset.begin(), rDefaultOpenCLSubset.end(), u"SUMIFS")
               == rDefaultOpenCLSubset.end()
        || std::find(rDefaultOpenCLSubset.begin(), rDefaultOpenCLSubset.end(), u"VLOOKUP")
               == rDefaultOpenCLSubset.end()
        || std::find(rDefaultOpenCLSubset.begin(), rDefaultOpenCLSubset.end(), u"NORMSDIST")
               == rDefaultOpenCLSubset.end()
        || std::find(rDefaultOpenCLSubset.begin(), rDefaultOpenCLSubset.end(), u"STD.NORM.DIST")
               != rDefaultOpenCLSubset.end())
    {
        return fail("spreadsheetengine_config_tests", "default OpenCL subset mismatch");
    }

    if (spreadsheetengine::core::config::parseForceCalculationMode(std::string_view("bogus")))
        return fail("spreadsheetengine_config_tests", "invalid force-calculation mode mismatch");

    std::cout << "spreadsheetengine config api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
