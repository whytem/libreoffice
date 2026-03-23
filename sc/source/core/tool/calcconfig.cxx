/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ostream>

#include <rtl/ustring.hxx>
#include <spreadsheetengine/bridge/CalcPhase0Bridge.hxx>
#include <spreadsheetengine/core/CalcConfig.hxx>
#include <comphelper/configuration.hxx>

#include <calcconfig.hxx>

#include <comphelper/configurationlistener.hxx>

using comphelper::ConfigurationListener;

static rtl::Reference<ConfigurationListener> const & getMiscListener()
{
    static rtl::Reference<ConfigurationListener> xListener(new ConfigurationListener(u"/org.openoffice.Office.Common/Misc"_ustr));
    return xListener;
}

static rtl::Reference<ConfigurationListener> const & getFormulaCalculationListener()
{
    static rtl::Reference<ConfigurationListener> xListener(new ConfigurationListener(u"/org.openoffice.Office.Calc/Formula/Calculation"_ustr));
    return xListener;
}

static ForceCalculationType toScForceCalculationType(
    spreadsheetengine::core::ForceCalculationMode eMode)
{
    switch (eMode)
    {
        case spreadsheetengine::core::ForceCalculationMode::OpenCL:
            return ForceCalculationOpenCL;
        case spreadsheetengine::core::ForceCalculationMode::Threads:
            return ForceCalculationThreads;
        case spreadsheetengine::core::ForceCalculationMode::Core:
            return ForceCalculationCore;
        case spreadsheetengine::core::ForceCalculationMode::None:
        default:
            return ForceCalculationNone;
    }
}

ForceCalculationType ScCalcConfig::getForceCalculationType()
{
    static_assert(spreadsheetengine::bridge::kCalcBridgeEnabled);
    static const ForceCalculationType type
        = toScForceCalculationType(spreadsheetengine::core::getForceCalculationModeFromEnv());
    return type;
}

bool ScCalcConfig::isOpenCLEnabled()
{
    if (comphelper::IsFuzzing())
        return false;
    static ForceCalculationType force = getForceCalculationType();
    if( force != ForceCalculationNone )
        return force == ForceCalculationOpenCL;
    static comphelper::ConfigurationListenerProperty<bool> gOpenCLEnabled(getMiscListener(), u"UseOpenCL"_ustr);
    return gOpenCLEnabled.get();
}

bool ScCalcConfig::isThreadingEnabled()
{
    if (comphelper::IsFuzzing())
        return false;
    static ForceCalculationType force = getForceCalculationType();
    if( force != ForceCalculationNone )
        return force == ForceCalculationThreads;
    static comphelper::ConfigurationListenerProperty<bool> gThreadingEnabled(getFormulaCalculationListener(), u"UseThreadedCalculationForFormulaGroups"_ustr);
    return gThreadingEnabled.get();
}

ScCalcConfig::ScCalcConfig() :
    meStringRefAddressSyntax(formula::FormulaGrammar::CONV_UNSPECIFIED),
    meStringConversion(StringConversion::LOCALE),     // old LibreOffice behavior
    mbEmptyStringAsZero(false),
    mbHasStringRefSyntax(false)
{
    setOpenCLConfigToDefault();
}

void ScCalcConfig::setOpenCLConfigToDefault()
{
    // Keep in order of opcode value, is that clearest? (Random order,
    // at least, would make no sense at all.)
    static const OpCodeSet pDefaultOpenCLSubsetOpCodes(new o3tl::sorted_vector<OpCode>({
        ocAdd,
        ocSub,
        ocNegSub,
        ocMul,
        ocDiv,
        ocPow,
        ocRandom,
        ocSin,
        ocCos,
        ocTan,
        ocArcTan,
        ocExp,
        ocLn,
        ocSqrt,
        ocStdNormDist,
        ocSNormInv,
        ocRound,
        ocPower,
        ocSumProduct,
        ocMin,
        ocMax,
        ocSum,
        ocProduct,
        ocAverage,
        ocCount,
        ocVar,
        ocNormDist,
        ocVLookup,
        ocCorrel,
        ocCovar,
        ocPearson,
        ocSlope,
        ocSumIfs}));

    // Note that these defaults better be kept in sync with those in
    // officecfg/registry/schema/org/openoffice/Office/Calc.xcs.
    // Crazy.
    mbOpenCLSubsetOnly = true;
    mbOpenCLAutoSelect = true;
    mnOpenCLMinimumFormulaGroupSize = 100;
    mpOpenCLSubsetOpCodes = pDefaultOpenCLSubsetOpCodes;
}

void ScCalcConfig::reset()
{
    *this = ScCalcConfig();
}

void ScCalcConfig::MergeDocumentSpecific( const ScCalcConfig& r )
{
    // String conversion options are per document.
    meStringConversion       = r.meStringConversion;
    mbEmptyStringAsZero      = r.mbEmptyStringAsZero;
    // INDIRECT ref syntax is per document.
    meStringRefAddressSyntax = r.meStringRefAddressSyntax;
    mbHasStringRefSyntax      = r.mbHasStringRefSyntax;
}

void ScCalcConfig::SetStringRefSyntax( formula::FormulaGrammar::AddressConvention eConv )
{
    meStringRefAddressSyntax = eConv;
    mbHasStringRefSyntax = true;
}

bool ScCalcConfig::operator== (const ScCalcConfig& r) const
{
    return meStringRefAddressSyntax == r.meStringRefAddressSyntax &&
           meStringConversion == r.meStringConversion &&
           mbEmptyStringAsZero == r.mbEmptyStringAsZero &&
           mbHasStringRefSyntax == r.mbHasStringRefSyntax &&
           mbOpenCLSubsetOnly == r.mbOpenCLSubsetOnly &&
           mbOpenCLAutoSelect == r.mbOpenCLAutoSelect &&
           maOpenCLDevice == r.maOpenCLDevice &&
           mnOpenCLMinimumFormulaGroupSize == r.mnOpenCLMinimumFormulaGroupSize &&
           *mpOpenCLSubsetOpCodes == *r.mpOpenCLSubsetOpCodes;
}

bool ScCalcConfig::operator!= (const ScCalcConfig& r) const
{
    return !operator==(r);
}

OUString ScOpCodeSetToSymbolicString(const ScCalcConfig::OpCodeSet& rOpCodes)
{
    return spreadsheetengine::core::formulaOpCodeSetToSymbolicString(rOpCodes);
}

ScCalcConfig::OpCodeSet ScStringToOpCodeSet(std::u16string_view rOpCodes)
{
    return spreadsheetengine::core::stringToFormulaOpCodeSet(rOpCodes);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
