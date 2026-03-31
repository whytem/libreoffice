/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/ConversionRuntime.hxx>
#include <cstdint>

#include <spreadsheetengine/runtime/NumeralConversion.hxx>

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace spreadsheetengine::core::convert
{
namespace
{

struct UnitConversionFactor
{
    api::StringView maFromUnit;
    api::StringView maToUnit;
    double mfFactor = 1.0;
};

struct EuroCurrencyInfo
{
    api::StringView maCode;
    double mfRate = 1.0;
    std::int32_t mnDecimals = 2;
};

constexpr UnitConversionFactor kKnownConversions[] = {
    { u"uk_acre", u"us_acre", 0.999996000004 },
    { u"us_acre", u"ang2", 4.04687260987425E+023 },
    { u"ang2", u"ar", 1E-22 },
    { u"ar", u"ft2", 1076.39104167097 },
    { u"ft2", u"ha", 9.290304E-06 },
    { u"ha", u"in2", 15500031.000062 },
    { u"in2", u"ly2", 7.20836355779189E-36 },
    { u"ly2", u"m2", 8.9501590038784E+031 },
    { u"m2", u"Morgen", 0.0004 },
    { u"Morgen", u"mi2", 0.000965255396356 },
    { u"mi2", u"Nmi2", 0.755119708987773 },
    { u"Nmi2", u"Pica2", 27560019740839.5 },
    { u"ang", u"ell", 8.748906E-11 },
    { u"ell", u"ft", 3.75000016575001 },
    { u"ft", u"in", 12 },
    { u"in", u"ly", 2.68483957766416E-18 },
    { u"ly", u"m", 9.460528E+015 },
    { u"m", u"mi", 0.000621371192237 },
    { u"mi", u"Nmi", 0.868976241900648 },
    { u"Nmi", u"parsec", 6.001922708E-14 },
    { u"parsec", u"Pica", 8.74680337440886E+019 },
    { u"survey_mi", u"yd", 1760.00352000704 },
    { u"BTU", u"c", 252.165488508169 },
    { u"c", u"cal", 0.99933031528756 },
    { u"cal", u"e", 41867948.4613929 },
    { u"e", u"eV", 624145700000 },
    { u"eV", u"flb", 3.80206452103493E-18 },
    { u"flb", u"HPh", 1.5697407642781E-08 },
    { u"HPh", u"J", 2684519.71705162 },
    { u"J", u"Wh", 0.000277777777778 },
    { u"dyn", u"N", 1E-05 },
    { u"N", u"lbf", 0.224808923655339 },
    { u"lbf", u"pond", 453.5923144952 },
    { u"ga", u"T", 0.0001 },
    { u"g", u"grain", 15.43236 },
    { u"cwt", u"uk_cwt", 0.892857142857143 },
    { u"uk_cwt", u"lbm", 112.000014877089 },
    { u"lbm", u"stone", 0.071428541793075 },
    { u"stone", u"ton", 0.007 },
    { u"ton", u"ozm", 32000.017962592 },
    { u"ozm", u"sg", 0.001942566898708 },
    { u"sg", u"u", 8.78861184032002E+027 },
    { u"HP", u"PS", 1.0138700185381 },
    { u"PS", u"W", 735.498542977386 },
    { u"atm", u"mmHg", 760 },
    { u"mmHg", u"Pa", 133.322363925 },
    { u"Pa", u"psi", 0.0001450377 },
    { u"psi", u"Torr", 51.7150920071126 },
    { u"admkn", u"kn", 0.999999913606911 },
    { u"kn", u"m/h", 1852 },
    { u"m/h", u"m/s", 0.000277777777778 },
    { u"m/s", u"mph", 2.2369362920544 },
    { u"C", u"F", 33.8 },
    { u"F", u"K", 255.927777777778 },
    { u"K", u"Rank", 1.8 },
    { u"Rank", u"Reau", -218.075555555556 },
    { u"d", u"hr", 24 },
    { u"hr", u"mn", 60 },
    { u"mn", u"sec", 60 },
    { u"sec", u"yr", 3.16880878140289E-08 },
    { u"ang3", u"barrel", 6.28981077043211E-30 },
    { u"barrel", u"bushel", 4.51167627067586 },
    { u"bushel", u"cup", 148.946856929372 },
    { u"cup", u"ft3", 0.008355034722222 },
    { u"ft3", u"gal", 7.48051948051948 },
    { u"in3", u"l", 0.016387064 },
    { u"in3", u"gal", 0.00432900432900433 },
    { u"l", u"ly3", 1.18101081256238E-51 },
    { u"l", u"ml", 1000.0 },
    { u"ly3", u"m3", 8.46732298606437E+047 },
    { u"m3", u"yd3", 1.30795061931439 },
    { u"m3", u"mi3", 2.39912758578928E-10 },
    { u"mi3", u"Nmi3", 0.65618108690130639 },
    { u"mi3", u"MTON", 5887918080000 },
    { u"MTON", u"Nmi3", 1.1144534927043455E-13 },
    { u"Nmi3", u"oz", 214792833387555 },
    { u"oz", u"Pica3", 673596 },
    { u"pt", u"qt", 0.5 },
    { u"qt", u"tbs", 64 },
    { u"tbs", u"tsp", 3 },
    { u"tsp", u"tspm", 0.98578431875 },
    { u"tspm", u"ml", 5.0 },
    { u"tspm", u"uk_gal", 0.001099846241495 },
    { u"uk_gal", u"uk_pt", 8 },
    { u"uk_pt", u"uk_qt", 0.5 },
    { u"uk_qt", u"yd3", 0.00148651530774 },
    { u"Pica2", u"picapt2", 1 },
    { u"picapt2", u"yd2", 1.48843545191282E-07 },
    { u"Pica3", u"picapt3", 1 },
    { u"picapt3", u"pica3", 0.000578703703705 },
    { u"pica3", u"pt", 0.000160333493666 },
    { u"gal", u"GRT", 0.001336805679661 },
    { u"GRT", u"in3", 172799.98395775 },
    { u"u", u"uk_ton", 1.63431440967062E-30 },
    { u"Pica", u"pica", 0.083333333333353 },
    { u"pica", u"survey_mi", 2.63046611952801E-06 },
};

constexpr EuroCurrencyInfo kEuroCurrencies[] = {
    { u"EUR", 1.0, 2 },       { u"ATS", 13.7603, 2 },  { u"DEM", 1.95583, 2 },
    { u"BEF", 40.3399, 0 },   { u"ESP", 166.386, 0 },  { u"FIM", 5.94573, 2 },
    { u"FRF", 6.55957, 2 },   { u"IEP", 0.787564, 2 }, { u"ITL", 1936.27, 0 },
    { u"LUF", 40.3399, 0 },   { u"NLG", 2.20371, 2 },  { u"PTE", 200.482, 1 },
    { u"GRD", 340.75, 0 },    { u"SIT", 239.64, 0 },   { u"MTL", 0.4293, 2 },
    { u"CYP", 0.585274, 2 },  { u"SKK", 30.126, 1 },
};

[[nodiscard]] api::Error mapNumeralStringError(NumeralStringError eError)
{
    switch (eError)
    {
        case NumeralStringError::None:
            return api::Error::None;
        case NumeralStringError::StringOverflow:
            return api::Error::StringOverflow;
        default:
            return api::Error::IllegalArgument;
    }
}

[[nodiscard]] api::String normalizeUnitSymbol(api::StringView rUnit)
{
    api::String aResult;
    aResult.reserve(rUnit.size());
    for (std::size_t nIndex = 0; nIndex < rUnit.size(); ++nIndex)
    {
        const char16_t cChar = rUnit[nIndex];
        if (cChar == u'^' && nIndex + 1 < rUnit.size()
            && rUnit[nIndex + 1] >= u'0' && rUnit[nIndex + 1] <= u'9')
        {
            continue;
        }

        aResult.push_back(cChar);
    }
    return aResult;
}

[[nodiscard]] api::String normalizeAsciiUpper(api::StringView rValue)
{
    api::String aResult;
    aResult.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar >= u'a' && cChar <= u'z')
            aResult.push_back(static_cast<char16_t>(cChar - (u'a' - u'A')));
        else
            aResult.push_back(cChar);
    }
    return aResult;
}

[[nodiscard]] std::optional<EuroCurrencyInfo> lookupEuroCurrency(
    api::StringView rCode, bool bCaseInsensitive)
{
    const api::String aNormalized
        = bCaseInsensitive ? normalizeAsciiUpper(rCode) : api::String(rCode);
    for (const auto& rCurrency : kEuroCurrencies)
    {
        if (aNormalized == rCurrency.maCode)
            return rCurrency;
    }
    return std::nullopt;
}

[[nodiscard]] double roundToDecimalPlaces(double fValue, std::int32_t nDecimals)
{
    if (nDecimals < 0)
        return fValue;
    const double fScale = std::pow(10.0, static_cast<double>(nDecimals));
    return std::round(fValue * fScale) / fScale;
}

[[nodiscard]] bool equalUnitSymbol(api::StringView rLeft, api::StringView rRight)
{
    return normalizeUnitSymbol(rLeft) == normalizeUnitSymbol(rRight);
}

[[nodiscard]] std::optional<double> convertTemperatureUnit(
    double fValue, api::StringView rFromUnit, api::StringView rToUnit)
{
    const api::String aNormalizedFrom = normalizeUnitSymbol(rFromUnit);
    const api::String aNormalizedTo = normalizeUnitSymbol(rToUnit);

    const auto toKelvin = [&](api::StringView rUnit) -> std::optional<double> {
        if (rUnit == u"C")
            return fValue + 273.15;
        if (rUnit == u"F")
            return (fValue + 459.67) * (5.0 / 9.0);
        if (rUnit == u"K")
            return fValue;
        if (rUnit == u"Rank" || rUnit == u"RANK")
            return fValue * (5.0 / 9.0);
        if (rUnit == u"Reau" || rUnit == u"REAU")
            return fValue * 1.25 + 273.15;
        return std::nullopt;
    };

    const auto oKelvin = toKelvin(aNormalizedFrom);
    if (!oKelvin)
        return std::nullopt;

    if (aNormalizedTo == u"C")
        return *oKelvin - 273.15;
    if (aNormalizedTo == u"F")
        return *oKelvin * (9.0 / 5.0) - 459.67;
    if (aNormalizedTo == u"K")
        return *oKelvin;
    if (aNormalizedTo == u"Rank" || aNormalizedTo == u"RANK")
        return *oKelvin * (9.0 / 5.0);
    if (aNormalizedTo == u"Reau" || aNormalizedTo == u"REAU")
        return (*oKelvin - 273.15) * 0.8;
    return std::nullopt;
}

} // namespace

api::ValueResult<double> evaluateEuroConvertValue(
    double fValue, api::StringView rFromCurrency, api::StringView rToCurrency,
    bool bCaseInsensitive, bool bRoundToTargetDecimals)
{
    const auto oFrom = lookupEuroCurrency(rFromCurrency, bCaseInsensitive);
    const auto oTo = lookupEuroCurrency(rToCurrency, bCaseInsensitive);
    if (!oFrom || !oTo)
        return api::ValueResult<double>::failure(api::Error::NotAvailable);

    double fResult = fValue;
    if (oFrom->maCode != oTo->maCode)
    {
        if (oFrom->maCode == u"EUR")
            fResult *= oTo->mfRate;
        else if (oTo->maCode == u"EUR")
            fResult /= oFrom->mfRate;
        else
            fResult = (fValue / oFrom->mfRate) * oTo->mfRate;
    }

    if (bRoundToTargetDecimals)
        fResult = roundToDecimalPlaces(fResult, oTo->mnDecimals);

    return api::ValueResult<double>::success(fResult);
}

api::ValueResult<double> evaluateConvertValue(
    double fValue, api::StringView rFromUnit, api::StringView rToUnit)
{
    const api::String aNormalizedFrom = normalizeUnitSymbol(rFromUnit);
    const api::String aNormalizedTo = normalizeUnitSymbol(rToUnit);
    if (aNormalizedFrom == aNormalizedTo)
        return api::ValueResult<double>::success(fValue);

    if (const auto oTemperature = convertTemperatureUnit(fValue, rFromUnit, rToUnit))
        return api::ValueResult<double>::success(*oTemperature);

    for (const auto& rConversion : kKnownConversions)
    {
        if (equalUnitSymbol(rConversion.maFromUnit, rFromUnit)
            && equalUnitSymbol(rConversion.maToUnit, rToUnit))
        {
            return api::ValueResult<double>::success(fValue * rConversion.mfFactor);
        }
    }

    for (const auto& rConversion : kKnownConversions)
    {
        if (equalUnitSymbol(rConversion.maFromUnit, rToUnit)
            && equalUnitSymbol(rConversion.maToUnit, rFromUnit))
        {
            return api::ValueResult<double>::success(fValue / rConversion.mfFactor);
        }
    }

    struct PendingUnitConversion
    {
        api::String maUnit;
        double mfFactor = 1.0;
    };

    std::vector<PendingUnitConversion> aPending{ { aNormalizedFrom, 1.0 } };
    std::vector<api::String> aVisited{ aNormalizedFrom };

    for (std::size_t nIndex = 0; nIndex < aPending.size(); ++nIndex)
    {
        const PendingUnitConversion& rCurrent = aPending[nIndex];
        if (rCurrent.maUnit == aNormalizedTo)
            return api::ValueResult<double>::success(fValue * rCurrent.mfFactor);

        for (const auto& rConversion : kKnownConversions)
        {
            api::String aNextUnit;
            double fNextFactor = 1.0;
            if (normalizeUnitSymbol(rConversion.maFromUnit) == rCurrent.maUnit)
            {
                aNextUnit = normalizeUnitSymbol(rConversion.maToUnit);
                fNextFactor = rCurrent.mfFactor * rConversion.mfFactor;
            }
            else if (normalizeUnitSymbol(rConversion.maToUnit) == rCurrent.maUnit)
            {
                aNextUnit = normalizeUnitSymbol(rConversion.maFromUnit);
                fNextFactor = rCurrent.mfFactor / rConversion.mfFactor;
            }
            else
            {
                continue;
            }

            if (std::find(aVisited.begin(), aVisited.end(), aNextUnit) != aVisited.end())
                continue;

            aVisited.push_back(aNextUnit);
            aPending.push_back({ std::move(aNextUnit), fNextFactor });
        }
    }

    return api::ValueResult<double>::failure(api::Error::NotAvailable);
}

api::ValueResult<double> evaluateDecimalValue(api::StringView rText, double fBase)
{
    if (auto oValue = convertFromBase(rText, fBase))
        return api::ValueResult<double>::success(*oValue);
    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

api::ValueResult<api::String> evaluateBaseValue(
    double fValue, double fBase, std::optional<double> ofMinLength)
{
    const auto aResult = convertToBase(fValue, fBase, ofMinLength);
    if (aResult.meError != NumeralStringError::None)
        return api::ValueResult<api::String>::failure(mapNumeralStringError(aResult.meError));
    return api::ValueResult<api::String>::success(aResult.maValue);
}

api::ValueResult<api::String> evaluateRomanValue(double fValue, std::optional<double> ofMode)
{
    if (auto oValue = convertToRoman(fValue, ofMode))
        return api::ValueResult<api::String>::success(*oValue);
    return api::ValueResult<api::String>::failure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::convert

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
