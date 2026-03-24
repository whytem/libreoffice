/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <algorithm>
#include <ostream>
#include <optional>

#include <com/sun/star/sheet/FormulaLanguage.hpp>
#include <formula/FormulaCompiler.hxx>
#include <sal/log.hxx>
#include <rtl/ustring.hxx>
#include <spreadsheetengine/bridge/CalcPhase0Bridge.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/core/ForceCalculation.hxx>
#include <spreadsheetengine/core/CalcConfig.hxx>
#include <comphelper/configuration.hxx>

#include <calcconfig.hxx>

#include <comphelper/configurationlistener.hxx>

using comphelper::ConfigurationListener;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;

namespace
{

std::optional<OpCode> toCalcConfigOpCode(spreadsheetengine::api::ConfigOpCodeSymbol eSymbol)
{
    switch (eSymbol)
    {
        case spreadsheetengine::api::ConfigOpCodeSymbol::Add:
            return ocAdd;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sub:
            return ocSub;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Mul:
            return ocMul;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Div:
            return ocDiv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Pow:
            return ocPow;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Rand:
            return ocRandom;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sin:
            return ocSin;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Cos:
            return ocCos;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Tan:
            return ocTan;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Atan:
            return ocArcTan;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Exp:
            return ocExp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Ln:
            return ocLn;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sqrt:
            return ocSqrt;
        case spreadsheetengine::api::ConfigOpCodeSymbol::StdNormDistLegacy:
            return ocStdNormDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::StdNormDistMs:
            return ocStdNormDist_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SNormInvLegacy:
            return ocSNormInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SNormInvMs:
            return ocSNormInv_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Round:
            return ocRound;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Power:
            return ocPower;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SumProduct:
            return ocSumProduct;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Min:
            return ocMin;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Max:
            return ocMax;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sum:
            return ocSum;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Product:
            return ocProduct;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Average:
            return ocAverage;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Count:
            return ocCount;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Var:
            return ocVar;
        case spreadsheetengine::api::ConfigOpCodeSymbol::NormDistLegacy:
            return ocNormDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::NormDistMs:
            return ocNormDist_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Match:
            return ocMatch;
        case spreadsheetengine::api::ConfigOpCodeSymbol::XMatch:
            return ocXMatch;
        case spreadsheetengine::api::ConfigOpCodeSymbol::CountIf:
            return ocCountIf;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SumIf:
            return ocSumIf;
        case spreadsheetengine::api::ConfigOpCodeSymbol::AverageIf:
            return ocAverageIf;
        case spreadsheetengine::api::ConfigOpCodeSymbol::AverageIfs:
            return ocAverageIfs;
        case spreadsheetengine::api::ConfigOpCodeSymbol::CountIfs:
            return ocCountIfs;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Lookup:
            return ocLookup;
        case spreadsheetengine::api::ConfigOpCodeSymbol::VLookup:
            return ocVLookup;
        case spreadsheetengine::api::ConfigOpCodeSymbol::XLookup:
            return ocXLookup;
        case spreadsheetengine::api::ConfigOpCodeSymbol::HLookup:
            return ocHLookup;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Pv:
            return ocPV;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Syd:
            return ocSYD;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Ddb:
            return ocDDB;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Db:
            return ocDB;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Vdb:
            return ocVBD;
        case spreadsheetengine::api::ConfigOpCodeSymbol::PDuration:
            return ocPDuration;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sln:
            return ocSLN;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Pmt:
            return ocPMT;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Rri:
            return ocRRI;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Fv:
            return ocFV;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Nper:
            return ocNper;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Rate:
            return ocRate;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Ipmt:
            return ocIpmt;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Ppmt:
            return ocPpmt;
        case spreadsheetengine::api::ConfigOpCodeSymbol::CumIpmt:
            return ocCumIpmt;
        case spreadsheetengine::api::ConfigOpCodeSymbol::CumPrinc:
            return ocCumPrinc;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Effect:
            return ocEffect;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Nominal:
            return ocNominal;
        case spreadsheetengine::api::ConfigOpCodeSymbol::IsPmt:
            return ocISPMT;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SumSq:
            return ocSumSQ;
        case spreadsheetengine::api::ConfigOpCodeSymbol::AverageA:
            return ocAverageA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::VarA:
            return ocVarA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::VarP:
            return ocVarP;
        case spreadsheetengine::api::ConfigOpCodeSymbol::VarPA:
            return ocVarPA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::VarPMs:
            return ocVarP_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::VarSMs:
            return ocVarS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::StDev:
            return ocStDev;
        case spreadsheetengine::api::ConfigOpCodeSymbol::StDevA:
            return ocStDevA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::StDevP:
            return ocStDevP;
        case spreadsheetengine::api::ConfigOpCodeSymbol::StDevPA:
            return ocStDevPA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::StDevPMs:
            return ocStDevP_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::StDevSMs:
            return ocStDevS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::GeoMean:
            return ocGeoMean;
        case spreadsheetengine::api::ConfigOpCodeSymbol::HarMean:
            return ocHarMean;
        case spreadsheetengine::api::ConfigOpCodeSymbol::AveDev:
            return ocAveDev;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DevSq:
            return ocDevSq;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Median:
            return ocMedian;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Kurt:
            return ocKurt;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Skew:
            return ocSkew;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Skewp:
            return ocSkewp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ZTest:
            return ocZTest;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ZTestMs:
            return ocZTest_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TTestLegacy:
            return ocTTest;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TTestMs:
            return ocTTest_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FTestLegacy:
            return ocFTest;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FTestMs:
            return ocFTest_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::NormInvLegacy:
            return ocNormInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::NormInvMs:
            return ocNormInv_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::LogNormDistLegacy:
            return ocLogNormDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::LogNormDistMs:
            return ocLogNormDist_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::LogInvLegacy:
            return ocLogInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::LogInvMs:
            return ocLogInv_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TDistLegacy:
            return ocTDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TDistMs:
            return ocTDist_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TDistRt:
            return ocTDist_RT;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TDist2T:
            return ocTDist_2T;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FDistLegacy:
            return ocFDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FDistMs:
            return ocFDist_LT;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FDistRt:
            return ocFDist_RT;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChiDistLegacy:
            return ocChiDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChiDistMs:
            return ocChiDist_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChiInvLegacy:
            return ocChiInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChiInvMs:
            return ocChiInv_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChiSqDistLegacy:
            return ocChiSqDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChiSqDistMs:
            return ocChiSqDist_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChiSqInvLegacy:
            return ocChiSqInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChiSqInvMs:
            return ocChiSqInv_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::GammaDistLegacy:
            return ocGammaDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::GammaDistMs:
            return ocGammaDist_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::GammaInvLegacy:
            return ocGammaInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::GammaInvMs:
            return ocGammaInv_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TInvLegacy:
            return ocTInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TInvMs:
            return ocTInv_MS;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TInv2T:
            return ocTInv_2T;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FInvLegacy:
            return ocFInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FInvMs:
            return ocFInv_LT;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FInvRt:
            return ocFInv_RT;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Rsq:
            return ocRSQ;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Steyx:
            return ocSTEYX;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Intercept:
            return ocIntercept;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Forecast:
            return ocForecast;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbSum:
            return ocDBSum;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbCount:
            return ocDBCount;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbCountA:
            return ocDBCount2;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbAverage:
            return ocDBAverage;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbGet:
            return ocDBGet;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbMax:
            return ocDBMax;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbMin:
            return ocDBMin;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbProduct:
            return ocDBProduct;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbStdDev:
            return ocDBStdDev;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbStdDevP:
            return ocDBStdDevP;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbVar:
            return ocDBVar;
        case spreadsheetengine::api::ConfigOpCodeSymbol::DbVarP:
            return ocDBVarP;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Abs:
            return ocAbs;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Int:
            return ocInt;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Pi:
            return ocPi;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Phi:
            return ocPhi;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Gauss:
            return ocGauss;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Na:
            return ocNotAvail;
        case spreadsheetengine::api::ConfigOpCodeSymbol::IsEven:
            return ocIsEven;
        case spreadsheetengine::api::ConfigOpCodeSymbol::IsOdd:
            return ocIsOdd;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Log10:
            return ocLog10;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Atan2:
            return ocArcTan2;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Ceiling:
            return ocCeil;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Floor:
            return ocFloor;
        case spreadsheetengine::api::ConfigOpCodeSymbol::RoundUp:
            return ocRoundUp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::RoundDown:
            return ocRoundDown;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Trunc:
            return ocTrunc;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Log:
            return ocLog;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Mod:
            return ocMod;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SumX2My2:
            return ocSumX2MY2;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SumX2Py2:
            return ocSumX2DY2;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SumXMy2:
            return ocSumXMY2;
        case spreadsheetengine::api::ConfigOpCodeSymbol::MinA:
            return ocMinA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::MaxA:
            return ocMaxA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::CountA:
            return ocCount2;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Npv:
            return ocNPV;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Irr:
            return ocIRR;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Mirr:
            return ocMIRR;
        case spreadsheetengine::api::ConfigOpCodeSymbol::B:
            return ocB;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ExponDist:
            return ocExpDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::BinomDist:
            return ocBinomDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Poisson:
            return ocPoissonDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Combin:
            return ocCombin;
        case spreadsheetengine::api::ConfigOpCodeSymbol::CombinA:
            return ocCombinA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Permut:
            return ocPermut;
        case spreadsheetengine::api::ConfigOpCodeSymbol::PermutationA:
            return ocPermutationA;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Filter:
            return ocFilter;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sort:
            return ocSort;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SortBy:
            return ocSortBy;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChooseCols:
            return ocChooseCols;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ChooseRows:
            return ocChooseRows;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Drop:
            return ocDrop;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Expand:
            return ocExpand;
        case spreadsheetengine::api::ConfigOpCodeSymbol::HStack:
            return ocHStack;
        case spreadsheetengine::api::ConfigOpCodeSymbol::VStack:
            return ocVStack;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Take:
            return ocTake;
        case spreadsheetengine::api::ConfigOpCodeSymbol::TextSplit:
            return ocTextSplit;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ToCol:
            return ocToCol;
        case spreadsheetengine::api::ConfigOpCodeSymbol::ToRow:
            return ocToRow;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Unique:
            return ocUnique;
        case spreadsheetengine::api::ConfigOpCodeSymbol::WrapCols:
            return ocWrapCols;
        case spreadsheetengine::api::ConfigOpCodeSymbol::WrapRows:
            return ocWrapRows;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sequence:
            return ocMatSequence;
        case spreadsheetengine::api::ConfigOpCodeSymbol::HypGeomDist:
            return ocHypGeomDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Weibull:
            return ocWeibull;
        case spreadsheetengine::api::ConfigOpCodeSymbol::NegBinomDist:
            return ocNegBinomVert;
        case spreadsheetengine::api::ConfigOpCodeSymbol::CritBinom:
            return ocCritBinom;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Standardize:
            return ocStandard;
        case spreadsheetengine::api::ConfigOpCodeSymbol::BetaDist:
            return ocBetaDist;
        case spreadsheetengine::api::ConfigOpCodeSymbol::BetaInv:
            return ocBetaInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Fisher:
            return ocFisher;
        case spreadsheetengine::api::ConfigOpCodeSymbol::FisherInv:
            return ocFisherInv;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Gamma:
            return ocGamma;
        case spreadsheetengine::api::ConfigOpCodeSymbol::GammaLn:
            return ocGammaLn;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Acos:
            return ocArcCos;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Asin:
            return ocArcSin;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Acot:
            return ocArcCot;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sinh:
            return ocSinHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Cosh:
            return ocCosHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Tanh:
            return ocTanHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Coth:
            return ocCotHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Asinh:
            return ocArcSinHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Acosh:
            return ocArcCosHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Atanh:
            return ocArcTanHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Acoth:
            return ocArcCotHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Csc:
            return ocCosecant;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sec:
            return ocSecant;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Csch:
            return ocCosecantHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Sech:
            return ocSecantHyp;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Cot:
            return ocCot;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Degrees:
            return ocDeg;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Radians:
            return ocRad;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Confidence:
            return ocConfidence;
        case spreadsheetengine::api::ConfigOpCodeSymbol::BitAnd:
            return ocBitAnd;
        case spreadsheetengine::api::ConfigOpCodeSymbol::BitOr:
            return ocBitOr;
        case spreadsheetengine::api::ConfigOpCodeSymbol::BitXor:
            return ocBitXor;
        case spreadsheetengine::api::ConfigOpCodeSymbol::BitRShift:
            return ocBitRshift;
        case spreadsheetengine::api::ConfigOpCodeSymbol::BitLShift:
            return ocBitLshift;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Fact:
            return ocFact;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Even:
            return ocEven;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Odd:
            return ocOdd;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Not:
            return ocNot;
        case spreadsheetengine::api::ConfigOpCodeSymbol::And:
            return ocAnd;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Or:
            return ocOr;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Xor:
            return ocXor;
        case spreadsheetengine::api::ConfigOpCodeSymbol::RandArray:
            return ocRandArray;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Correl:
            return ocCorrel;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Covar:
            return ocCovar;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Pearson:
            return ocPearson;
        case spreadsheetengine::api::ConfigOpCodeSymbol::Slope:
            return ocSlope;
        case spreadsheetengine::api::ConfigOpCodeSymbol::SumIfs:
            return ocSumIfs;
    }

    return std::nullopt;
}

std::optional<spreadsheetengine::api::ConfigOpCodeSymbol> toConfigOpCodeSymbol(const OpCode eOp)
{
    switch (eOp)
    {
        case ocAdd:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Add;
        case ocSub:
        case ocNegSub:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sub;
        case ocMul:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Mul;
        case ocDiv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Div;
        case ocPow:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Pow;
        case ocRandom:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Rand;
        case ocSin:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sin;
        case ocCos:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Cos;
        case ocTan:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Tan;
        case ocArcTan:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Atan;
        case ocExp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Exp;
        case ocLn:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Ln;
        case ocSqrt:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sqrt;
        case ocStdNormDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::StdNormDistLegacy;
        case ocStdNormDist_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::StdNormDistMs;
        case ocSNormInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SNormInvLegacy;
        case ocSNormInv_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SNormInvMs;
        case ocRound:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Round;
        case ocPower:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Power;
        case ocSumProduct:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SumProduct;
        case ocMin:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Min;
        case ocMax:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Max;
        case ocSum:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sum;
        case ocProduct:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Product;
        case ocAverage:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Average;
        case ocCount:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Count;
        case ocVar:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Var;
        case ocNormDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::NormDistLegacy;
        case ocNormDist_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::NormDistMs;
        case ocMatch:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Match;
        case ocXMatch:
            return spreadsheetengine::api::ConfigOpCodeSymbol::XMatch;
        case ocCountIf:
            return spreadsheetengine::api::ConfigOpCodeSymbol::CountIf;
        case ocSumIf:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SumIf;
        case ocAverageIf:
            return spreadsheetengine::api::ConfigOpCodeSymbol::AverageIf;
        case ocAverageIfs:
            return spreadsheetengine::api::ConfigOpCodeSymbol::AverageIfs;
        case ocCountIfs:
            return spreadsheetengine::api::ConfigOpCodeSymbol::CountIfs;
        case ocLookup:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Lookup;
        case ocVLookup:
            return spreadsheetengine::api::ConfigOpCodeSymbol::VLookup;
        case ocXLookup:
            return spreadsheetengine::api::ConfigOpCodeSymbol::XLookup;
        case ocHLookup:
            return spreadsheetengine::api::ConfigOpCodeSymbol::HLookup;
        case ocPV:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Pv;
        case ocSYD:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Syd;
        case ocDDB:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Ddb;
        case ocDB:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Db;
        case ocVBD:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Vdb;
        case ocPDuration:
            return spreadsheetengine::api::ConfigOpCodeSymbol::PDuration;
        case ocSLN:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sln;
        case ocPMT:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Pmt;
        case ocRRI:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Rri;
        case ocFV:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Fv;
        case ocNper:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Nper;
        case ocRate:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Rate;
        case ocIpmt:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Ipmt;
        case ocPpmt:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Ppmt;
        case ocCumIpmt:
            return spreadsheetengine::api::ConfigOpCodeSymbol::CumIpmt;
        case ocCumPrinc:
            return spreadsheetengine::api::ConfigOpCodeSymbol::CumPrinc;
        case ocEffect:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Effect;
        case ocNominal:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Nominal;
        case ocISPMT:
            return spreadsheetengine::api::ConfigOpCodeSymbol::IsPmt;
        case ocSumSQ:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SumSq;
        case ocAverageA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::AverageA;
        case ocVarA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::VarA;
        case ocVarP:
            return spreadsheetengine::api::ConfigOpCodeSymbol::VarP;
        case ocVarPA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::VarPA;
        case ocVarP_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::VarPMs;
        case ocVarS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::VarSMs;
        case ocStDev:
            return spreadsheetengine::api::ConfigOpCodeSymbol::StDev;
        case ocStDevA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::StDevA;
        case ocStDevP:
            return spreadsheetengine::api::ConfigOpCodeSymbol::StDevP;
        case ocStDevPA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::StDevPA;
        case ocStDevP_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::StDevPMs;
        case ocStDevS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::StDevSMs;
        case ocGeoMean:
            return spreadsheetengine::api::ConfigOpCodeSymbol::GeoMean;
        case ocHarMean:
            return spreadsheetengine::api::ConfigOpCodeSymbol::HarMean;
        case ocAveDev:
            return spreadsheetengine::api::ConfigOpCodeSymbol::AveDev;
        case ocDevSq:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DevSq;
        case ocMedian:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Median;
        case ocKurt:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Kurt;
        case ocSkew:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Skew;
        case ocSkewp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Skewp;
        case ocZTest:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ZTest;
        case ocZTest_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ZTestMs;
        case ocTTest:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TTestLegacy;
        case ocTTest_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TTestMs;
        case ocFTest:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FTestLegacy;
        case ocFTest_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FTestMs;
        case ocNormInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::NormInvLegacy;
        case ocNormInv_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::NormInvMs;
        case ocLogNormDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::LogNormDistLegacy;
        case ocLogNormDist_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::LogNormDistMs;
        case ocLogInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::LogInvLegacy;
        case ocLogInv_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::LogInvMs;
        case ocTDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TDistLegacy;
        case ocTDist_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TDistMs;
        case ocTDist_RT:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TDistRt;
        case ocTDist_2T:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TDist2T;
        case ocFDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FDistLegacy;
        case ocFDist_LT:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FDistMs;
        case ocFDist_RT:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FDistRt;
        case ocChiDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChiDistLegacy;
        case ocChiDist_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChiDistMs;
        case ocChiInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChiInvLegacy;
        case ocChiInv_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChiInvMs;
        case ocChiSqDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChiSqDistLegacy;
        case ocChiSqDist_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChiSqDistMs;
        case ocChiSqInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChiSqInvLegacy;
        case ocChiSqInv_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChiSqInvMs;
        case ocGammaDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::GammaDistLegacy;
        case ocGammaDist_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::GammaDistMs;
        case ocGammaInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::GammaInvLegacy;
        case ocGammaInv_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::GammaInvMs;
        case ocTInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TInvLegacy;
        case ocTInv_MS:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TInvMs;
        case ocTInv_2T:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TInv2T;
        case ocFInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FInvLegacy;
        case ocFInv_LT:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FInvMs;
        case ocFInv_RT:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FInvRt;
        case ocRSQ:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Rsq;
        case ocSTEYX:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Steyx;
        case ocIntercept:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Intercept;
        case ocForecast:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Forecast;
        case ocDBSum:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbSum;
        case ocDBCount:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbCount;
        case ocDBCount2:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbCountA;
        case ocDBAverage:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbAverage;
        case ocDBGet:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbGet;
        case ocDBMax:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbMax;
        case ocDBMin:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbMin;
        case ocDBProduct:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbProduct;
        case ocDBStdDev:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbStdDev;
        case ocDBStdDevP:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbStdDevP;
        case ocDBVar:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbVar;
        case ocDBVarP:
            return spreadsheetengine::api::ConfigOpCodeSymbol::DbVarP;
        case ocAbs:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Abs;
        case ocInt:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Int;
        case ocPi:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Pi;
        case ocPhi:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Phi;
        case ocGauss:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Gauss;
        case ocNotAvail:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Na;
        case ocIsEven:
            return spreadsheetengine::api::ConfigOpCodeSymbol::IsEven;
        case ocIsOdd:
            return spreadsheetengine::api::ConfigOpCodeSymbol::IsOdd;
        case ocLog10:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Log10;
        case ocArcTan2:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Atan2;
        case ocCeil:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Ceiling;
        case ocFloor:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Floor;
        case ocRoundUp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::RoundUp;
        case ocRoundDown:
            return spreadsheetengine::api::ConfigOpCodeSymbol::RoundDown;
        case ocTrunc:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Trunc;
        case ocLog:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Log;
        case ocMod:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Mod;
        case ocSumX2MY2:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SumX2My2;
        case ocSumX2DY2:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SumX2Py2;
        case ocSumXMY2:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SumXMy2;
        case ocMinA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::MinA;
        case ocMaxA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::MaxA;
        case ocCount2:
            return spreadsheetengine::api::ConfigOpCodeSymbol::CountA;
        case ocNPV:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Npv;
        case ocIRR:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Irr;
        case ocMIRR:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Mirr;
        case ocB:
            return spreadsheetengine::api::ConfigOpCodeSymbol::B;
        case ocExpDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ExponDist;
        case ocBinomDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::BinomDist;
        case ocPoissonDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Poisson;
        case ocCombin:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Combin;
        case ocCombinA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::CombinA;
        case ocPermut:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Permut;
        case ocPermutationA:
            return spreadsheetengine::api::ConfigOpCodeSymbol::PermutationA;
        case ocFilter:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Filter;
        case ocSort:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sort;
        case ocSortBy:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SortBy;
        case ocChooseCols:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChooseCols;
        case ocChooseRows:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ChooseRows;
        case ocDrop:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Drop;
        case ocExpand:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Expand;
        case ocHStack:
            return spreadsheetengine::api::ConfigOpCodeSymbol::HStack;
        case ocVStack:
            return spreadsheetengine::api::ConfigOpCodeSymbol::VStack;
        case ocTake:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Take;
        case ocTextSplit:
            return spreadsheetengine::api::ConfigOpCodeSymbol::TextSplit;
        case ocToCol:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ToCol;
        case ocToRow:
            return spreadsheetengine::api::ConfigOpCodeSymbol::ToRow;
        case ocUnique:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Unique;
        case ocWrapCols:
            return spreadsheetengine::api::ConfigOpCodeSymbol::WrapCols;
        case ocWrapRows:
            return spreadsheetengine::api::ConfigOpCodeSymbol::WrapRows;
        case ocMatSequence:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sequence;
        case ocHypGeomDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::HypGeomDist;
        case ocWeibull:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Weibull;
        case ocNegBinomVert:
            return spreadsheetengine::api::ConfigOpCodeSymbol::NegBinomDist;
        case ocCritBinom:
            return spreadsheetengine::api::ConfigOpCodeSymbol::CritBinom;
        case ocStandard:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Standardize;
        case ocBetaDist:
            return spreadsheetengine::api::ConfigOpCodeSymbol::BetaDist;
        case ocBetaInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::BetaInv;
        case ocFisher:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Fisher;
        case ocFisherInv:
            return spreadsheetengine::api::ConfigOpCodeSymbol::FisherInv;
        case ocGamma:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Gamma;
        case ocGammaLn:
            return spreadsheetengine::api::ConfigOpCodeSymbol::GammaLn;
        case ocArcCos:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Acos;
        case ocArcSin:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Asin;
        case ocArcCot:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Acot;
        case ocSinHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sinh;
        case ocCosHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Cosh;
        case ocTanHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Tanh;
        case ocCotHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Coth;
        case ocArcSinHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Asinh;
        case ocArcCosHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Acosh;
        case ocArcTanHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Atanh;
        case ocArcCotHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Acoth;
        case ocCosecant:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Csc;
        case ocSecant:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sec;
        case ocCosecantHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Csch;
        case ocSecantHyp:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Sech;
        case ocCot:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Cot;
        case ocDeg:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Degrees;
        case ocRad:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Radians;
        case ocConfidence:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Confidence;
        case ocBitAnd:
            return spreadsheetengine::api::ConfigOpCodeSymbol::BitAnd;
        case ocBitOr:
            return spreadsheetengine::api::ConfigOpCodeSymbol::BitOr;
        case ocBitXor:
            return spreadsheetengine::api::ConfigOpCodeSymbol::BitXor;
        case ocBitRshift:
            return spreadsheetengine::api::ConfigOpCodeSymbol::BitRShift;
        case ocBitLshift:
            return spreadsheetengine::api::ConfigOpCodeSymbol::BitLShift;
        case ocFact:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Fact;
        case ocEven:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Even;
        case ocOdd:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Odd;
        case ocNot:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Not;
        case ocAnd:
            return spreadsheetengine::api::ConfigOpCodeSymbol::And;
        case ocOr:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Or;
        case ocXor:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Xor;
        case ocRandArray:
            return spreadsheetengine::api::ConfigOpCodeSymbol::RandArray;
        case ocCorrel:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Correl;
        case ocCovar:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Covar;
        case ocPearson:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Pearson;
        case ocSlope:
            return spreadsheetengine::api::ConfigOpCodeSymbol::Slope;
        case ocSumIfs:
            return spreadsheetengine::api::ConfigOpCodeSymbol::SumIfs;
        default:
            return std::nullopt;
    }
}

}

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
    spreadsheetengine::api::ForceCalculationMode eMode)
{
    switch (eMode)
    {
        case spreadsheetengine::api::ForceCalculationMode::OpenCL:
            return ForceCalculationOpenCL;
        case spreadsheetengine::api::ForceCalculationMode::Threads:
            return ForceCalculationThreads;
        case spreadsheetengine::api::ForceCalculationMode::Core:
            return ForceCalculationCore;
        case spreadsheetengine::api::ForceCalculationMode::None:
        default:
            return ForceCalculationNone;
    }
}

ForceCalculationType ScCalcConfig::getForceCalculationType()
{
    static_assert(spreadsheetengine::bridge::kCalcBridgeEnabled);
    static const ForceCalculationType type
        = toScForceCalculationType(spreadsheetengine::core::config::getForceCalculationModeFromEnv());
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
    static const OpCodeSet pDefaultOpenCLSubsetOpCodes = ScStringToOpCodeSet(
        spreadsheetengine::core::configOpCodeSymbolListToString(
            spreadsheetengine::core::defaultOpenCLSubsetConfigOpCodes()));

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
    spreadsheetengine::core::SymbolicOpCodeList aSymbols;
    aSymbols.reserve(rOpCodes->size());
    std::vector<spreadsheetengine::api::ConfigOpCodeSymbol> aSeenSymbols;

    formula::FormulaCompiler aCompiler;
    formula::FormulaCompiler::OpCodeMapPtr pOpCodeMap;

    for (auto i = rOpCodes->begin(); i != rOpCodes->end(); ++i)
    {
        if (const auto eSymbol = toConfigOpCodeSymbol(*i))
        {
            if (std::find(aSeenSymbols.begin(), aSeenSymbols.end(), *eSymbol) != aSeenSymbols.end())
                continue;
            aSeenSymbols.push_back(*eSymbol);
            aSymbols.emplace_back(spreadsheetengine::core::configOpCodeSymbolName(*eSymbol));
        }
        else
        {
            if (!pOpCodeMap)
                pOpCodeMap = aCompiler.GetOpCodeMap(css::sheet::FormulaLanguage::ENGLISH);
            aSymbols.push_back(selibreoffice::toApiString(pOpCodeMap->getSymbol(*i)));
        }
    }

    return selibreoffice::toLibreOfficeString(
        spreadsheetengine::core::symbolicOpCodeListToString(aSymbols));
}

ScCalcConfig::OpCodeSet ScStringToOpCodeSet(std::u16string_view rOpCodes)
{
    ScCalcConfig::OpCodeSet aResult = std::make_shared<o3tl::sorted_vector<OpCode>>();

    formula::FormulaCompiler aCompiler;
    formula::FormulaCompiler::OpCodeMapPtr pOpCodeMap;

    const auto aTokens = spreadsheetengine::core::stringToSymbolicOpCodeList(rOpCodes);
    for (const auto& rToken : aTokens)
    {
        if (const auto eSymbol = spreadsheetengine::core::findConfigOpCodeSymbol(rToken))
        {
            if (const auto eOpCode = toCalcConfigOpCode(*eSymbol))
            {
                aResult->insert(*eOpCode);
                continue;
            }
        }

        const OUString aElement = selibreoffice::toLibreOfficeString(rToken);
        const sal_Int32 nValue = aElement.toInt32();
        if (nValue > 0 || (nValue == 0 && aElement == "0"))
        {
            aResult->insert(static_cast<OpCode>(nValue));
            continue;
        }

        if (!pOpCodeMap)
            pOpCodeMap = aCompiler.GetOpCodeMap(css::sheet::FormulaLanguage::ENGLISH);
        const formula::OpCodeHashMap& rHashMap(pOpCodeMap->getHashMap());
        auto it = rHashMap.find(aElement);
        if (it != rHashMap.end())
            aResult->insert(it->second);
        else
            SAL_WARN("sc.opencl", "Unrecognized OpCode " << aElement << " in OpCode set string");
    }

    if (aResult->find(ocSub) != aResult->end())
        aResult->insert(ocNegSub);

    return aResult;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
