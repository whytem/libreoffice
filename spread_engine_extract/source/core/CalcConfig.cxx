/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/CalcConfig.hxx>

#include <array>

namespace spreadsheetengine::core
{

namespace
{

using spreadsheetengine::api::ConfigOpCodeSymbol;

struct ConfigOpCodeSymbolData
{
    ConfigOpCodeSymbol meSymbol;
    spreadsheetengine::api::StringView maCanonicalName;
    spreadsheetengine::api::StringView maCompatAlias;
};

constexpr ConfigOpCodeSymbolData gConfigOpCodeSymbols[] = {
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Add, u"+", u"ADD" },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sub, u"-", u"SUB" },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Mul, u"*", u"MUL" },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Div, u"/", u"DIV" },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Pow, u"^", u"POW" },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Rand, u"RAND", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sin, u"SIN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Cos, u"COS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Tan, u"TAN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Atan, u"ATAN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Exp, u"EXP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Ln, u"LN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sqrt, u"SQRT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::StdNormDistLegacy, u"NORMSDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::StdNormDistMs, u"STD.NORM.DIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SNormInvLegacy, u"NORMSINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SNormInvMs, u"NORM.S.INV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Round, u"ROUND", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Power, u"POWER", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SumProduct, u"SUMPRODUCT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Min, u"MIN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Max, u"MAX", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sum, u"SUM", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Product, u"PRODUCT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Average, u"AVERAGE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Count, u"COUNT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Var, u"VAR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::NormDistLegacy, u"NORMDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::NormDistMs, u"NORM.DIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Match, u"MATCH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::XMatch, u"XMATCH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::CountIf, u"COUNTIF", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SumIf, u"SUMIF", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::AverageIf, u"AVERAGEIF", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::AverageIfs, u"AVERAGEIFS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::CountIfs, u"COUNTIFS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Lookup, u"LOOKUP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::VLookup, u"VLOOKUP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::XLookup, u"XLOOKUP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::HLookup, u"HLOOKUP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Pv, u"PV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Syd, u"SYD", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Ddb, u"DDB", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Db, u"DB", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Vdb, u"VDB", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::PDuration, u"PDURATION", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sln, u"SLN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Pmt, u"PMT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Rri, u"RRI", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Fv, u"FV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Nper, u"NPER", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Rate, u"RATE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Ipmt, u"IPMT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Ppmt, u"PPMT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::CumIpmt, u"CUMIPMT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::CumPrinc, u"CUMPRINC", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Effect, u"EFFECT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Nominal, u"NOMINAL", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::IsPmt, u"ISPMT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SumSq, u"SUMSQ", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::AverageA, u"AVERAGEA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::VarA, u"VARA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::VarP, u"VARP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::VarPA, u"VARPA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::VarPMs, u"VAR.P", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::VarSMs, u"VAR.S", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::StDev, u"STDEV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::StDevA, u"STDEVA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::StDevP, u"STDEVP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::StDevPA, u"STDEVPA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::StDevPMs, u"STDEV.P", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::StDevSMs, u"STDEV.S", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::GeoMean, u"GEOMEAN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::HarMean, u"HARMEAN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::AveDev, u"AVEDEV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DevSq, u"DEVSQ", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Median, u"MEDIAN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Kurt, u"KURT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Skew, u"SKEW", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Skewp, u"SKEWP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ZTest, u"ZTEST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ZTestMs, u"Z.TEST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TTestLegacy, u"TTEST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TTestMs, u"T.TEST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FTestLegacy, u"FTEST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FTestMs, u"F.TEST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::NormInvLegacy, u"NORMINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::NormInvMs, u"NORM.INV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::LogNormDistLegacy, u"LOGNORMDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::LogNormDistMs, u"LOGNORM.DIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::LogInvLegacy, u"LOGINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::LogInvMs, u"LOGNORM.INV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TDistLegacy, u"TDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TDistMs, u"T.DIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TDistRt, u"T.DIST.RT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TDist2T, u"T.DIST.2T", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FDistLegacy, u"FDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FDistMs, u"F.DIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FDistRt, u"F.DIST.RT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChiDistLegacy, u"CHIDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChiDistMs, u"CHISQ.DIST.RT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChiInvLegacy, u"CHIINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChiInvMs, u"CHISQ.INV.RT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChiSqDistLegacy, u"CHISQDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChiSqDistMs, u"CHISQ.DIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChiSqInvLegacy, u"CHISQINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChiSqInvMs, u"CHISQ.INV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::GammaDistLegacy, u"GAMMADIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::GammaDistMs, u"GAMMA.DIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::GammaInvLegacy, u"GAMMAINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::GammaInvMs, u"GAMMA.INV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TInvLegacy, u"TINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TInvMs, u"T.INV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TInv2T, u"T.INV.2T", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FInvLegacy, u"FINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FInvMs, u"F.INV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FInvRt, u"F.INV.RT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Rsq, u"RSQ", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Steyx, u"STEYX", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Intercept, u"INTERCEPT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Forecast, u"FORECAST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbSum, u"DSUM", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbCount, u"DCOUNT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbCountA, u"DCOUNTA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbAverage, u"DAVERAGE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbGet, u"DGET", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbMax, u"DMAX", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbMin, u"DMIN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbProduct, u"DPRODUCT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbStdDev, u"DSTDEV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbStdDevP, u"DSTDEVP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbVar, u"DVAR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::DbVarP, u"DVARP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Abs, u"ABS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Int, u"INT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Pi, u"PI", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Phi, u"PHI", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Gauss, u"GAUSS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Na, u"NA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::IsEven, u"ISEVEN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::IsOdd, u"ISODD", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Log10, u"LOG10", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Atan2, u"ATAN2", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Ceiling, u"CEILING", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Floor, u"FLOOR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::RoundUp, u"ROUNDUP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::RoundDown, u"ROUNDDOWN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Trunc, u"TRUNC", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Log, u"LOG", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Mod, u"MOD", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SumX2My2, u"SUMX2MY2", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SumX2Py2, u"SUMX2PY2", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SumXMy2, u"SUMXMY2", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::MinA, u"MINA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::MaxA, u"MAXA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::CountA, u"COUNTA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Npv, u"NPV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Irr, u"IRR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Mirr, u"MIRR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::B, u"B", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ExponDist, u"EXPONDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::BinomDist, u"BINOMDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Poisson, u"POISSON", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Combin, u"COMBIN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::CombinA, u"COMBINA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Permut, u"PERMUT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::PermutationA, u"PERMUTATIONA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Filter, u"FILTER", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sort, u"SORT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SortBy, u"SORTBY", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChooseCols, u"CHOOSECOLS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ChooseRows, u"CHOOSEROWS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Drop, u"DROP", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Expand, u"EXPAND", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::HStack, u"HSTACK", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::VStack, u"VSTACK", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Take, u"TAKE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::TextSplit, u"TEXTSPLIT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ToCol, u"TOCOL", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::ToRow, u"TOROW", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Unique, u"UNIQUE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::WrapCols, u"WRAPCOLS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::WrapRows, u"WRAPROWS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sequence, u"SEQUENCE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::HypGeomDist, u"HYPGEOMDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Weibull, u"WEIBULL", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::NegBinomDist, u"NEGBINOMDIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::CritBinom, u"CRITBINOM", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Standardize, u"STANDARDIZE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::BetaDist, u"BETADIST", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::BetaInv, u"BETAINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Fisher, u"FISHER", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::FisherInv, u"FISHERINV", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Gamma, u"GAMMA", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::GammaLn, u"GAMMALN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Acos, u"ACOS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Asin, u"ASIN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Acot, u"ACOT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sinh, u"SINH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Cosh, u"COSH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Tanh, u"TANH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Coth, u"COTH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Asinh, u"ASINH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Acosh, u"ACOSH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Atanh, u"ATANH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Acoth, u"ACOTH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Csc, u"CSC", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sec, u"SEC", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Csch, u"CSCH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Sech, u"SECH", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Cot, u"COT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Degrees, u"DEGREES", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Radians, u"RADIANS", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Confidence, u"CONFIDENCE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::BitAnd, u"BITAND", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::BitOr, u"BITOR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::BitXor, u"BITXOR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::BitRShift, u"BITRSHIFT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::BitLShift, u"BITLSHIFT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Fact, u"FACT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Even, u"EVEN", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Odd, u"ODD", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Not, u"NOT", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::And, u"AND", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Or, u"OR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Xor, u"XOR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::RandArray, u"RANDARRAY", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Correl, u"CORREL", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Covar, u"COVAR", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Pearson, u"PEARSON", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::Slope, u"SLOPE", {} },
    ConfigOpCodeSymbolData{ ConfigOpCodeSymbol::SumIfs, u"SUMIFS", {} },
};

}

spreadsheetengine::api::StringView configOpCodeSymbolName(
    const spreadsheetengine::api::ConfigOpCodeSymbol eSymbol)
{
    for (const auto& rEntry : gConfigOpCodeSymbols)
    {
        if (rEntry.meSymbol == eSymbol)
            return rEntry.maCanonicalName;
    }

    return {};
}

std::optional<spreadsheetengine::api::ConfigOpCodeSymbol> findConfigOpCodeSymbol(
    const spreadsheetengine::api::StringView rToken)
{
    for (const auto& rEntry : gConfigOpCodeSymbols)
    {
        if (rEntry.maCanonicalName == rToken
            || (!rEntry.maCompatAlias.empty() && rEntry.maCompatAlias == rToken))
            return rEntry.meSymbol;
    }

    return std::nullopt;
}

spreadsheetengine::api::String configOpCodeSymbolListToString(const ConfigOpCodeSymbolList& rSymbols)
{
    spreadsheetengine::api::String aResult;
    bool bFirst = true;
    for (const auto eSymbol : rSymbols)
    {
        if (!bFirst)
            aResult.push_back(u';');
        aResult.append(configOpCodeSymbolName(eSymbol));
        bFirst = false;
    }
    return aResult;
}

const ConfigOpCodeSymbolList& defaultOpenCLSubsetConfigOpCodes()
{
    static const ConfigOpCodeSymbolList aDefaultOpenCLSubsetOpCodes = {
        ConfigOpCodeSymbol::Add,        ConfigOpCodeSymbol::Sub,      ConfigOpCodeSymbol::Mul,
        ConfigOpCodeSymbol::Div,        ConfigOpCodeSymbol::Pow,      ConfigOpCodeSymbol::Rand,
        ConfigOpCodeSymbol::Sin,        ConfigOpCodeSymbol::Cos,      ConfigOpCodeSymbol::Tan,
        ConfigOpCodeSymbol::Atan,       ConfigOpCodeSymbol::Exp,      ConfigOpCodeSymbol::Ln,
        ConfigOpCodeSymbol::Sqrt,       ConfigOpCodeSymbol::StdNormDistLegacy,
        ConfigOpCodeSymbol::SNormInvLegacy, ConfigOpCodeSymbol::Round,
        ConfigOpCodeSymbol::Power,      ConfigOpCodeSymbol::SumProduct,
        ConfigOpCodeSymbol::Min,        ConfigOpCodeSymbol::Max,      ConfigOpCodeSymbol::Sum,
        ConfigOpCodeSymbol::Product,    ConfigOpCodeSymbol::Average,  ConfigOpCodeSymbol::Count,
        ConfigOpCodeSymbol::Var,        ConfigOpCodeSymbol::NormDistLegacy,
        ConfigOpCodeSymbol::VLookup,    ConfigOpCodeSymbol::Correl,   ConfigOpCodeSymbol::Covar,
        ConfigOpCodeSymbol::Pearson,    ConfigOpCodeSymbol::Slope,    ConfigOpCodeSymbol::SumIfs
    };
    return aDefaultOpenCLSubsetOpCodes;
}

const SymbolicOpCodeList& defaultOpenCLSubsetSymbolicOpCodes()
{
    static const SymbolicOpCodeList aDefaultOpenCLSubsetOpCodes = [] {
        SymbolicOpCodeList aResult;
        aResult.reserve(defaultOpenCLSubsetConfigOpCodes().size());
        for (const auto eSymbol : defaultOpenCLSubsetConfigOpCodes())
            aResult.emplace_back(configOpCodeSymbolName(eSymbol));
        return aResult;
    }();
    return aDefaultOpenCLSubsetOpCodes;
}

spreadsheetengine::api::String symbolicOpCodeListToString(const SymbolicOpCodeList& rOpCodes)
{
    spreadsheetengine::api::String aResult;
    bool bFirst = true;
    for (const auto& rOpCode : rOpCodes)
    {
        if (!bFirst)
            aResult.push_back(u';');
        aResult.append(rOpCode);
        bFirst = false;
    }
    return aResult;
}

SymbolicOpCodeList stringToSymbolicOpCodeList(std::u16string_view rOpCodes)
{
    SymbolicOpCodeList aResult;
    std::size_t nFromIndex = 0;
    while (nFromIndex <= rOpCodes.size())
    {
        const std::size_t nSemicolon = rOpCodes.find(u';', nFromIndex);
        const std::size_t nTokenLength
            = nSemicolon == std::u16string_view::npos ? rOpCodes.size() - nFromIndex
                                                      : nSemicolon - nFromIndex;
        if (nTokenLength > 0)
            aResult.emplace_back(rOpCodes.substr(nFromIndex, nTokenLength));
        if (nSemicolon == std::u16string_view::npos)
            break;
        nFromIndex = nSemicolon + 1;
    }
    return aResult;
}

} // namespace spreadsheetengine::core

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
