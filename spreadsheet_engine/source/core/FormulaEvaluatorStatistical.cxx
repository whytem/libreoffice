/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace spreadsheetengine::core::eval
{
namespace
{

template <std::size_t N>
[[nodiscard]] bool matchesFunctionRegistry(
    api::StringView rFunctionName, const std::array<api::StringView, N>& rRegistry)
{
    return std::find(rRegistry.begin(), rRegistry.end(), rFunctionName) != rRegistry.end();
}

[[nodiscard]] double roundToSignificantDigits(double fValue, int nDigits)
{
    if (!std::isfinite(fValue) || fValue == 0.0 || nDigits <= 0)
        return fValue;

    const double fScale = std::pow(
        10.0, static_cast<double>(nDigits) - 1.0 - std::floor(std::log10(std::fabs(fValue))));
    if (!std::isfinite(fScale) || fScale == 0.0)
        return fValue;

    return std::round(fValue * fScale) / fScale;
}

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateStatisticalRuntimeFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kStatisticalFunctions{
        api::StringView(u"T.TEST"),
        api::StringView(u"TTEST"),
        api::StringView(u"ZTEST"),
        api::StringView(u"Z.TEST"),
        api::StringView(u"FISHER"),
        api::StringView(u"FISHERINV"),
        api::StringView(u"GAUSS"),
        api::StringView(u"GAMMALN"),
        api::StringView(u"GAMMALN.PRECISE"),
        api::StringView(u"COM.MICROSOFT.GAMMALN.PRECISE"),
        api::StringView(u"PERMUTATIONA"),
        api::StringView(u"PERMUT"),
        api::StringView(u"SLOPE"),
        api::StringView(u"KURT"),
        api::StringView(u"RANK"),
        api::StringView(u"RANK.EQ"),
        api::StringView(u"COM.MICROSOFT.RANK.EQ"),
        api::StringView(u"RANK.AVG"),
        api::StringView(u"COM.MICROSOFT.RANK.AVG"),
        api::StringView(u"GEOMEAN"),
        api::StringView(u"HARMEAN"),
        api::StringView(u"POISSON"),
        api::StringView(u"POISSON.DIST"),
        api::StringView(u"LEGACY.CHIDIST"),
        api::StringView(u"CHISQDIST"),
        api::StringView(u"CHISQ.DIST"),
        api::StringView(u"CHISQ.DIST.RT"),
        api::StringView(u"COM.MICROSOFT.CHISQ.DIST.RT"),
        api::StringView(u"CHISQ.INV"),
        api::StringView(u"COM.MICROSOFT.CHISQ.INV"),
        api::StringView(u"CHISQINV"),
        api::StringView(u"CHISQ.INV.RT"),
        api::StringView(u"COM.MICROSOFT.CHISQ.INV.RT"),
        api::StringView(u"LEGACY.CHIINV"),
        api::StringView(u"CHIINV"),
        api::StringView(u"LEGACY.NORMSDIST"),
        api::StringView(u"NORMSDIST"),
        api::StringView(u"NORM.S.DIST"),
        api::StringView(u"COM.MICROSOFT.NORM.S.DIST"),
        api::StringView(u"LEGACY.NORMSINV"),
        api::StringView(u"NORMSINV"),
        api::StringView(u"NORM.S.INV"),
        api::StringView(u"COM.MICROSOFT.NORM.S.INV"),
        api::StringView(u"NORMINV"),
        api::StringView(u"NORM.INV"),
        api::StringView(u"COM.MICROSOFT.NORM.INV"),
        api::StringView(u"STANDARDIZE"),
        api::StringView(u"LOGINV"),
        api::StringView(u"LOGNORM.INV"),
        api::StringView(u"COM.MICROSOFT.LOGNORM.INV"),
        api::StringView(u"GAMMAINV"),
        api::StringView(u"GAMMA.INV"),
        api::StringView(u"COM.MICROSOFT.GAMMA.INV"),
        api::StringView(u"ERF"),
        api::StringView(u"ERF.PRECISE"),
        api::StringView(u"COM.MICROSOFT.ERF.PRECISE"),
        api::StringView(u"ERFC"),
        api::StringView(u"ERFC.PRECISE"),
        api::StringView(u"COM.MICROSOFT.ERFC.PRECISE"),
        api::StringView(u"NORMDIST"),
        api::StringView(u"NORM.DIST"),
        api::StringView(u"LOGNORMDIST"),
        api::StringView(u"LOGNORM.DIST"),
        api::StringView(u"COM.MICROSOFT.LOGNORM.DIST"),
        api::StringView(u"GAMMADIST"),
        api::StringView(u"GAMMA.DIST"),
        api::StringView(u"GAMMA"),
        api::StringView(u"COM.MICROSOFT.GAMMA"),
        api::StringView(u"BETA.INV"),
        api::StringView(u"COM.MICROSOFT.BETA.INV"),
        api::StringView(u"BETAINV"),
        api::StringView(u"TINV"),
        api::StringView(u"T.INV"),
        api::StringView(u"COM.MICROSOFT.T.INV"),
        api::StringView(u"T.INV.2T"),
        api::StringView(u"COM.MICROSOFT.T.INV.2T"),
        api::StringView(u"T.DIST.2T"),
        api::StringView(u"COM.MICROSOFT.T.DIST.2T"),
        api::StringView(u"T.DIST.RT"),
        api::StringView(u"COM.MICROSOFT.T.DIST.RT"),
        api::StringView(u"LEGACY.TDIST"),
        api::StringView(u"CORREL"),
        api::StringView(u"PEARSON"),
        api::StringView(u"RSQ"),
        api::StringView(u"STEYX"),
        api::StringView(u"INTERCEPT"),
        api::StringView(u"FORECAST"),
        api::StringView(u"FTEST"),
        api::StringView(u"DEVSQ"),
        api::StringView(u"FDIST"),
        api::StringView(u"LEGACY.FDIST"),
        api::StringView(u"F.DIST.RT"),
        api::StringView(u"COM.MICROSOFT.F.DIST.RT"),
        api::StringView(u"CONFIDENCE"),
        api::StringView(u"CONFIDENCE.NORM"),
        api::StringView(u"COM.MICROSOFT.CONFIDENCE.NORM"),
        api::StringView(u"CONFIDENCE.T"),
        api::StringView(u"COM.MICROSOFT.CONFIDENCE.T"),
        api::StringView(u"FINV"),
        api::StringView(u"LEGACY.FINV"),
        api::StringView(u"F.INV.RT"),
        api::StringView(u"COM.MICROSOFT.F.INV.RT"),
        api::StringView(u"F.INV"),
        api::StringView(u"COM.MICROSOFT.F.INV"),
        api::StringView(u"VAR"),
        api::StringView(u"VAR.S"),
        api::StringView(u"VARP"),
        api::StringView(u"VAR.P"),
        api::StringView(u"VARA"),
        api::StringView(u"VARPA"),
        api::StringView(u"STDEV"),
        api::StringView(u"STDEV.S"),
        api::StringView(u"STDEVP"),
        api::StringView(u"STDEV.P"),
        api::StringView(u"STDEVA"),
        api::StringView(u"STDEVPA"),
        api::StringView(u"BINOMDIST"),
        api::StringView(u"BINOM.DIST"),
        api::StringView(u"CRITBINOM"),
        api::StringView(u"BINOM.INV"),
        api::StringView(u"BINOM.DIST.RANGE"),
        api::StringView(u"B"),
        api::StringView(u"NEGBINOMDIST"),
        api::StringView(u"NEGBINOM.DIST"),
        api::StringView(u"COM.MICROSOFT.NEGBINOM.DIST"),
        api::StringView(u"EXPONDIST"),
        api::StringView(u"EXPON.DIST"),
        api::StringView(u"COM.MICROSOFT.EXPON.DIST"),
        api::StringView(u"WEIBULL"),
        api::StringView(u"WEIBULL.DIST"),
        api::StringView(u"COM.MICROSOFT.WEIBULL.DIST"),
        api::StringView(u"HYPGEOMDIST"),
        api::StringView(u"HYPGEOM.DIST"),
        api::StringView(u"PERCENTRANK"),
        api::StringView(u"PERCENTRANK.INC"),
        api::StringView(u"PERCENTRANK.EXC"),
        api::StringView(u"MODE.SNGL"),
        api::StringView(u"PROB"),
        api::StringView(u"TRIMMEAN"),
        api::StringView(u"CHISQ.TEST"),
        api::StringView(u"LEGACY.CHITEST"),
        api::StringView(u"BETADIST"),
        api::StringView(u"BETA.DIST"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kStatisticalFunctions))
        return std::nullopt;
    return evaluateStatisticalRuntimeFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateStatisticalRuntimeFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    FunctionEvalContext aContext { *this, rNode, rCurrentAddress };
    const auto makeCellError = [&](api::Error eError) -> EvaluationResult {
        return makeScalarResult(api::CellValue::error(eError));
    };

    struct MatrixOperand
    {
        api::MatrixSize mnColumns = 1;
        api::MatrixSize mnRows = 1;
        std::vector<api::CellValue> maValues;
    };

    const auto materializeMatrixOperand = [&](const formula::Node& rArgument)
        -> api::ValueResult<MatrixOperand> {
        EvaluationResult aValue = evaluateNode(rArgument, rCurrentAddress);
        if (!aValue)
            return api::ValueResult<MatrixOperand>::failure(aValue.meError);

        MatrixOperand aOperand;
        if (aValue.maValue.isScalar())
        {
            aOperand.maValues.push_back(aValue.maValue.maValue);
            return api::ValueResult<MatrixOperand>::success(std::move(aOperand));
        }

        const auto aDimensions = aValue.maValue.maReference.matrixDimensions();
        aOperand.mnColumns = aDimensions.mnColumns;
        aOperand.mnRows = aDimensions.mnRows;
        aOperand.maValues.reserve(static_cast<std::size_t>(aOperand.mnColumns)
                                  * static_cast<std::size_t>(aOperand.mnRows));
        for (api::MatrixSize nRow = 0; nRow < aOperand.mnRows; ++nRow)
        {
            for (api::MatrixSize nColumn = 0; nColumn < aOperand.mnColumns; ++nColumn)
            {
                EvaluationResult aElement
                    = materializeReferenceValue(aValue.maValue.maReference, nColumn, nRow);
                if (!aElement)
                    return api::ValueResult<MatrixOperand>::failure(aElement.meError);
                if (!aElement.maValue.isScalar())
                    return api::ValueResult<MatrixOperand>::failure(api::Error::IllegalArgument);
                aOperand.maValues.push_back(aElement.maValue.maValue);
            }
        }

        return api::ValueResult<MatrixOperand>::success(std::move(aOperand));
    };

    const auto collectNumericSampleValues = [&](const formula::Node& rArgument)
        -> api::ValueResult<std::vector<double>> {
        std::vector<double> aValues;
        const auto aVisited = aContext.visitFlattenedValues(
            rArgument,
            [&](const api::CellValue& rValue, bool bFromReference) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isEmpty())
                    return api::ValueResult<bool>::success(true);
                if (rValue.isText())
                {
                    if (bFromReference)
                        return api::ValueResult<bool>::success(true);
                    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
                }
                if (rValue.isBoolean() && bFromReference)
                    return api::ValueResult<bool>::success(true);

                const auto aNumber = coerceToNumber(rValue);
                if (!aNumber)
                    return api::ValueResult<bool>::failure(aNumber.meError);
                aValues.push_back(aNumber.maValue);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        return api::ValueResult<std::vector<double>>::success(std::move(aValues));
    };

    struct RegressionStats
    {
        double fCount = 0.0;
        double fMeanX = 0.0;
        double fMeanY = 0.0;
        fp::KahanSum fSumDeltaXDeltaY = 0.0;
        fp::KahanSum fSumSqrDeltaX = 0.0;
        fp::KahanSum fSumSqrDeltaY = 0.0;
    };

    const auto collectRegressionStats
        = [&](const formula::Node& rKnownYNode, const formula::Node& rKnownXNode)
        -> api::ValueResult<RegressionStats> {
        const auto collectValueSequence = [&](const formula::Node& rArgument)
            -> api::ValueResult<std::vector<api::CellValue>> {
            std::vector<api::CellValue> aValues;
            const auto aVisited = aContext.visitFlattenedValues(
                rArgument, [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    aValues.push_back(rValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<api::CellValue>>::failure(aVisited.meError);
            return api::ValueResult<std::vector<api::CellValue>>::success(std::move(aValues));
        };

        const auto aKnownY = collectValueSequence(rKnownYNode);
        if (!aKnownY)
            return api::ValueResult<RegressionStats>::failure(aKnownY.meError);
        const auto aKnownX = collectValueSequence(rKnownXNode);
        if (!aKnownX)
            return api::ValueResult<RegressionStats>::failure(aKnownX.meError);

        if (aKnownY.maValue.size() != aKnownX.maValue.size())
            return api::ValueResult<RegressionStats>::failure(api::Error::IllegalArgument);

        RegressionStats aStats;
        fp::KahanSum fSumX = 0.0;
        fp::KahanSum fSumY = 0.0;
        for (std::size_t nIndex = 0; nIndex < aKnownY.maValue.size(); ++nIndex)
        {
            const api::CellValue& rY = aKnownY.maValue[nIndex];
            const api::CellValue& rX = aKnownX.maValue[nIndex];
            if (rY.isError())
                return api::ValueResult<RegressionStats>::failure(rY.meError);
            if (rX.isError())
                return api::ValueResult<RegressionStats>::failure(rX.meError);
            if ((rY.isText() || rY.isEmpty()) || (rX.isText() || rX.isEmpty()))
                continue;

            const auto aY = coerceToNumber(rY);
            if (!aY)
                return api::ValueResult<RegressionStats>::failure(aY.meError);
            const auto aX = coerceToNumber(rX);
            if (!aX)
                return api::ValueResult<RegressionStats>::failure(aX.meError);

            fSumX += aX.maValue;
            fSumY += aY.maValue;
            aStats.fCount += 1.0;
        }

        if (aStats.fCount < 1.0)
            return api::ValueResult<RegressionStats>::failure(api::Error::NoValue);

        aStats.fMeanX = fSumX.get() / aStats.fCount;
        aStats.fMeanY = fSumY.get() / aStats.fCount;
        for (std::size_t nIndex = 0; nIndex < aKnownY.maValue.size(); ++nIndex)
        {
            const api::CellValue& rY = aKnownY.maValue[nIndex];
            const api::CellValue& rX = aKnownX.maValue[nIndex];
            if ((rY.isText() || rY.isEmpty()) || (rX.isText() || rX.isEmpty()))
                continue;

            const auto aY = coerceToNumber(rY);
            if (!aY)
                return api::ValueResult<RegressionStats>::failure(aY.meError);
            const auto aX = coerceToNumber(rX);
            if (!aX)
                return api::ValueResult<RegressionStats>::failure(aX.meError);

            const double fDeltaX = aX.maValue - aStats.fMeanX;
            const double fDeltaY = aY.maValue - aStats.fMeanY;
            aStats.fSumDeltaXDeltaY += fDeltaX * fDeltaY;
            aStats.fSumSqrDeltaX += fDeltaX * fDeltaX;
            aStats.fSumSqrDeltaY += fDeltaY * fDeltaY;
        }

        return api::ValueResult<RegressionStats>::success(std::move(aStats));
    };

    if (aFunctionName == u"T.TEST" || aFunctionName == u"TTEST")
    {
        if (rNode.maChildren.size() != 4)
            return makeCellError(api::Error::IllegalArgument);

        const auto aTails = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[2]);
        if (!aTails)
            return makeCellError(aTails.meError);
        const auto aType = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[3]);
        if (!aType)
            return makeCellError(aType.meError);
        if ((aTails.maValue != 1 && aTails.maValue != 2)
            || (aType.maValue < 1 || aType.maValue > 3))
        {
            return makeCellError(api::Error::IllegalArgument);
        }

        double fT = 0.0;
        double fF = 0.0;
        if (aType.maValue == 1)
        {
            const auto aSample1 = materializeMatrixOperand(*rNode.maChildren[0]);
            if (!aSample1)
                return makeCellError(aSample1.meError);
            const auto aSample2 = materializeMatrixOperand(*rNode.maChildren[1]);
            if (!aSample2)
                return makeCellError(aSample2.meError);
            if (aSample1.maValue.mnColumns != aSample2.maValue.mnColumns
                || aSample1.maValue.mnRows != aSample2.maValue.mnRows)
            {
                return makeCellError(api::Error::IllegalArgument);
            }

            double fCount = 0.0;
            fp::KahanSum fSum1 = 0.0;
            fp::KahanSum fSum2 = 0.0;
            fp::KahanSum fSumSqrD = 0.0;
            for (std::size_t nIndex = 0; nIndex < aSample1.maValue.maValues.size(); ++nIndex)
            {
                const api::CellValue& rValue1 = aSample1.maValue.maValues[nIndex];
                const api::CellValue& rValue2 = aSample2.maValue.maValues[nIndex];
                if (rValue1.isError())
                    return makeCellError(rValue1.meError);
                if (rValue2.isError())
                    return makeCellError(rValue2.meError);
                if ((rValue1.isText() || rValue1.isEmpty())
                    || (rValue2.isText() || rValue2.isEmpty()))
                {
                    continue;
                }

                const auto aValue1 = coerceToNumber(rValue1);
                if (!aValue1)
                    return makeCellError(aValue1.meError);
                const auto aValue2 = coerceToNumber(rValue2);
                if (!aValue2)
                    return makeCellError(aValue2.meError);

                fSum1 += aValue1.maValue;
                fSum2 += aValue2.maValue;
                fSumSqrD += (aValue1.maValue - aValue2.maValue)
                            * (aValue1.maValue - aValue2.maValue);
                fCount += 1.0;
            }
            if (fCount < 1.0)
                return makeCellError(api::Error::NoValue);

            const double fSumD = fSum1.get() - fSum2.get();
            const double fDivider = fSumSqrD.get() * fCount - fSumD * fSumD;
            if (fp::approxEqual(fDivider, 0.0))
                return makeCellError(api::Error::DivisionByZero);

            fT = std::abs(fSumD) * std::sqrt((fCount - 1.0) / fDivider);
            fF = fCount - 1.0;
        }
        else
        {
            const auto aSample1 = collectNumericSampleValues(*rNode.maChildren[0]);
            if (!aSample1)
                return makeCellError(aSample1.meError);
            const auto aSample2 = collectNumericSampleValues(*rNode.maChildren[1]);
            if (!aSample2)
                return makeCellError(aSample2.meError);
            if (aSample1.maValue.size() < 2 || aSample2.maValue.size() < 2)
                return makeCellError(api::Error::NoValue);

            const auto calculateTest = [&](bool bTemplin) -> api::ValueResult<bool> {
                double fCount1 = 0.0;
                double fCount2 = 0.0;
                fp::KahanSum fSum1 = 0.0;
                fp::KahanSum fSumSqr1 = 0.0;
                fp::KahanSum fSum2 = 0.0;
                fp::KahanSum fSumSqr2 = 0.0;
                for (double fValue : aSample1.maValue)
                {
                    fSum1 += fValue;
                    fSumSqr1 += fValue * fValue;
                    fCount1 += 1.0;
                }
                for (double fValue : aSample2.maValue)
                {
                    fSum2 += fValue;
                    fSumSqr2 += fValue * fValue;
                    fCount2 += 1.0;
                }

                if (bTemplin)
                {
                    const double fS1
                        = (fSumSqr1.get() - fSum1.get() * fSum1.get() / fCount1)
                          / (fCount1 - 1.0) / fCount1;
                    const double fS2
                        = (fSumSqr2.get() - fSum2.get() * fSum2.get() / fCount2)
                          / (fCount2 - 1.0) / fCount2;
                    if (fp::approxEqual(fS1 + fS2, 0.0))
                        return api::ValueResult<bool>::failure(api::Error::NoValue);
                    fT = std::abs(fSum1.get() / fCount1 - fSum2.get() / fCount2)
                         / std::sqrt(fS1 + fS2);
                    const double c = fS1 / (fS1 + fS2);
                    fF = 1.0 / (c * c / (fCount1 - 1.0)
                                + (1.0 - c) * (1.0 - c) / (fCount2 - 1.0));
                }
                else
                {
                    const double fS1
                        = (fSumSqr1.get() - fSum1.get() * fSum1.get() / fCount1)
                          / (fCount1 - 1.0);
                    const double fS2
                        = (fSumSqr2.get() - fSum2.get() * fSum2.get() / fCount2)
                          / (fCount2 - 1.0);
                    fT = std::abs(fSum1.get() / fCount1 - fSum2.get() / fCount2)
                         / std::sqrt((fCount1 - 1.0) * fS1 + (fCount2 - 1.0) * fS2)
                         * std::sqrt(
                             fCount1 * fCount2 * (fCount1 + fCount2 - 2.0) / (fCount1 + fCount2));
                    fF = fCount1 + fCount2 - 2.0;
                }
                return api::ValueResult<bool>::success(true);
            };

            const auto aTest = calculateTest(aType.maValue == 3);
            if (!aTest)
                return makeCellError(aTest.meError);
        }

        const auto aDistribution
            = semath::evaluateStudentDistribution(fT, fF, aTails.maValue);
        if (!aDistribution)
            return makeCellError(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"ZTEST" || aFunctionName == u"Z.TEST")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aValues = collectNumericSampleValues(*rNode.maChildren[0]);
        if (!aValues)
            return makeCellError(aValues.meError);
        if (aValues.maValue.size() <= 1)
            return makeCellError(api::Error::DivisionByZero);

        const auto aX = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aX)
            return makeCellError(aX.meError);

        fp::KahanSum fSum = 0.0;
        fp::KahanSum fSumSqr = 0.0;
        double fCount = 0.0;
        for (double fValue : aValues.maValue)
        {
            fSum += fValue;
            fSumSqr += fValue * fValue;
            fCount += 1.0;
        }

        const double fMean = fSum.get() / fCount;
        double fZ = 0.0;
        if (rNode.maChildren.size() == 3)
        {
            const auto aSigma = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
            if (!aSigma)
                return makeCellError(aSigma.meError);
            if (!(aSigma.maValue > 0.0))
                return makeCellError(api::Error::IllegalArgument);
            fZ = (fMean - aX.maValue) * std::sqrt(fCount) / aSigma.maValue;
        }
        else
        {
            const double fSigma
                = (fSumSqr.get() - fSum.get() * fSum.get() / fCount) / (fCount - 1.0);
            if (fp::approxEqual(fSigma, 0.0))
                return makeCellError(api::Error::DivisionByZero);
            fZ = (fMean - aX.maValue) / std::sqrt(fSigma / fCount);
        }

        return makeScalarResult(api::CellValue::number(0.5 - semath::gaussValue(fZ)));
    }

    if (aFunctionName == u"FISHER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        const auto aFisher = semath::fisherTransform(aNumber.maValue);
        if (!aFisher)
            return makeFailure(aFisher.meError);
        return makeScalarResult(api::CellValue::number(aFisher.maValue));
    }

    if (aFunctionName == u"FISHERINV")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        return makeScalarResult(
            api::CellValue::number(semath::inverseFisherTransform(aNumber.maValue)));
    }

    if (aFunctionName == u"GAUSS")
    {
        if (rNode.maChildren.size() != 1)
            return makeCellError(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeCellError(aArgument.meError);
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeCellError(aNumber.meError);

        return makeScalarResult(api::CellValue::number(semath::gaussValue(aNumber.maValue)));
    }

    if (aFunctionName == u"PERMUT")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aN = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aN)
            return makeCellError(aN.meError);
        const auto aK = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aK)
            return makeCellError(aK.meError);

        return aContext.makeNumericOrErrorResult(
            semath::evaluatePermutationValue(aN.maValue, aK.maValue));
    }

    if (aFunctionName == u"PERMUTATIONA")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aN = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aN)
            return makeCellError(aN.meError);
        const auto aK = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[1]);
        if (!aK)
            return makeCellError(aK.meError);

        return aContext.makeNumericOrErrorResult(
            semath::evaluatePermutationAValue(aN.maValue, aK.maValue));
    }

    if (aFunctionName == u"SLOPE")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aStats = collectRegressionStats(*rNode.maChildren[0], *rNode.maChildren[1]);
        if (!aStats)
            return makeCellError(aStats.meError);

        if (fp::approxEqual(aStats.maValue.fSumSqrDeltaX.get(), 0.0))
            return makeCellError(api::Error::DivisionByZero);

        const long double fSlope
            = static_cast<long double>(aStats.maValue.fSumDeltaXDeltaY.get())
              / static_cast<long double>(aStats.maValue.fSumSqrDeltaX.get());
        return makeScalarResult(api::CellValue::number(
            roundToSignificantDigits(static_cast<double>(fSlope), 15)));
    }

    if (aFunctionName == u"CORREL" || aFunctionName == u"PEARSON" || aFunctionName == u"RSQ"
        || aFunctionName == u"STEYX")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aStats = collectRegressionStats(*rNode.maChildren[0], *rNode.maChildren[1]);
        if (!aStats)
            return makeCellError(aStats.meError);

        if (aFunctionName == u"STEYX" && aStats.maValue.fCount < 3.0)
            return makeCellError(api::Error::NoValue);

        if (aStats.maValue.fSumSqrDeltaX.get() < std::numeric_limits<double>::min()
            || ((aFunctionName == u"CORREL" || aFunctionName == u"PEARSON"
                 || aFunctionName == u"RSQ")
                && aStats.maValue.fSumSqrDeltaY.get() < std::numeric_limits<double>::min()))
        {
            return makeCellError(api::Error::DivisionByZero);
        }

        if (aFunctionName == u"STEYX")
        {
            const double fResidual
                = aStats.maValue.fSumSqrDeltaY.get()
                  - (aStats.maValue.fSumDeltaXDeltaY.get()
                     * aStats.maValue.fSumDeltaXDeltaY.get()
                     / aStats.maValue.fSumSqrDeltaX.get());
            return makeScalarResult(api::CellValue::number(
                std::sqrt(fResidual / (aStats.maValue.fCount - 2.0))));
        }

        const double fPearson = aStats.maValue.fSumDeltaXDeltaY.get()
                                / std::sqrt(aStats.maValue.fSumSqrDeltaX.get()
                                            * aStats.maValue.fSumSqrDeltaY.get());
        if (aFunctionName == u"RSQ")
            return makeScalarResult(api::CellValue::number(fPearson * fPearson));
        return makeScalarResult(api::CellValue::number(fPearson));
    }

    if (aFunctionName == u"INTERCEPT" || aFunctionName == u"FORECAST")
    {
        if (rNode.maChildren.size() != 2 + (aFunctionName == u"FORECAST" ? 1 : 0))
            return makeCellError(api::Error::IllegalArgument);

        const auto aStats = collectRegressionStats(*rNode.maChildren[0], *rNode.maChildren[1]);
        if (!aStats)
            return makeCellError(aStats.meError);
        if (fp::approxEqual(aStats.maValue.fSumSqrDeltaX.get(), 0.0))
            return makeCellError(api::Error::DivisionByZero);

        const double fSlope = aStats.maValue.fSumDeltaXDeltaY.get()
                              / aStats.maValue.fSumSqrDeltaX.get();
        const double fIntercept
            = aStats.maValue.fMeanY - fSlope * aStats.maValue.fMeanX;
        if (aFunctionName == u"INTERCEPT")
            return makeScalarResult(api::CellValue::number(fIntercept));

        const auto aForecastX = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aForecastX)
            return makeCellError(aForecastX.meError);
        return makeScalarResult(
            api::CellValue::number(aStats.maValue.fMeanY + fSlope * (aForecastX.maValue
                                                                     - aStats.maValue.fMeanX)));
    }

    if (aFunctionName == u"FTEST")
    {
        if (rNode.maChildren.size() != 2)
            return makeCellError(api::Error::IllegalArgument);

        const auto aSample1 = collectNumericSampleValues(*rNode.maChildren[0]);
        if (!aSample1)
            return makeCellError(aSample1.meError);
        const auto aSample2 = collectNumericSampleValues(*rNode.maChildren[1]);
        if (!aSample2)
            return makeCellError(aSample2.meError);
        if (aSample1.maValue.size() < 2 || aSample2.maValue.size() < 2)
            return makeCellError(api::Error::NoValue);

        auto computeSampleVariance = [](const std::vector<double>& rSample) {
            fp::KahanSum fSum = 0.0;
            fp::KahanSum fSumSqr = 0.0;
            for (const double fValue : rSample)
            {
                fSum += fValue;
                fSumSqr += fValue * fValue;
            }

            const double fCount = static_cast<double>(rSample.size());
            return (fSumSqr.get() - fSum.get() * fSum.get() / fCount) / (fCount - 1.0);
        };

        const double fVariance1 = computeSampleVariance(aSample1.maValue);
        const double fVariance2 = computeSampleVariance(aSample2.maValue);
        if (fp::approxEqual(fVariance1, 0.0) || fp::approxEqual(fVariance2, 0.0))
            return makeCellError(api::Error::NoValue);

        double fRatio = 0.0;
        double fDf1 = 0.0;
        double fDf2 = 0.0;
        if (fVariance1 > fVariance2)
        {
            fRatio = fVariance1 / fVariance2;
            fDf1 = static_cast<double>(aSample1.maValue.size() - 1);
            fDf2 = static_cast<double>(aSample2.maValue.size() - 1);
        }
        else
        {
            fRatio = fVariance2 / fVariance1;
            fDf1 = static_cast<double>(aSample2.maValue.size() - 1);
            fDf2 = static_cast<double>(aSample1.maValue.size() - 1);
        }

        const auto aFdist = semath::evaluateFRightTailDistribution(fRatio, fDf1, fDf2);
        if (!aFdist)
            return makeCellError(aFdist.meError);

        return makeScalarResult(
            api::CellValue::number(2.0 * std::min(aFdist.maValue, 1.0 - aFdist.maValue)));
    }

    if (aFunctionName == u"DEVSQ")
    {
        if (rNode.maChildren.empty())
            return makeCellError(api::Error::IllegalArgument);

        const auto aNumbers = aContext.collectVarianceArguments(false);
        if (!aNumbers)
            return makeCellError(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeCellError(api::Error::DivisionByZero);

        double fCount = 0.0;
        fp::KahanSum fSum = 0.0;
        for (const double fValue : aNumbers.maValue)
        {
            fSum += fValue;
            fCount += 1.0;
        }

        const double fMean = fSum.get() / fCount;
        fp::KahanSum fSquaredDeviation = 0.0;
        for (const double fValue : aNumbers.maValue)
        {
            const double fDelta = fp::approxSub(fValue, fMean);
            fSquaredDeviation += fDelta * fDelta;
        }
        return makeScalarResult(api::CellValue::number(fSquaredDeviation.get()));
    }

    if (aFunctionName == u"RANK" || aFunctionName == u"RANK.EQ"
        || aFunctionName == u"COM.MICROSOFT.RANK.EQ"
        || aFunctionName == u"RANK.AVG" || aFunctionName == u"COM.MICROSOFT.RANK.AVG")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aValue = aContext.evaluateRequiredNumberArgument(*rNode.maChildren[0]);
        if (!aValue)
            return makeCellError(aValue.meError);

        std::vector<double> aSortArray;
        const auto aVisited = aContext.visitFlattenedValues(
            *rNode.maChildren[1],
            [&](const api::CellValue& rValue, bool bFromReference) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isEmpty())
                    return api::ValueResult<bool>::success(true);
                if (bFromReference && rValue.isText())
                    return api::ValueResult<bool>::success(true);

                const auto aNumber = coerceToNumber(rValue);
                if (!aNumber)
                    return api::ValueResult<bool>::failure(aNumber.meError);
                aSortArray.push_back(aNumber.maValue);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return makeCellError(aVisited.meError);
        if (aSortArray.empty())
            return makeCellError(api::Error::NoValue);

        bool bAscending = false;
        if (rNode.maChildren.size() == 3)
        {
            const auto aAscending = aContext.evaluatePayTypeArgument(*rNode.maChildren[2], false);
            if (!aAscending)
                return makeCellError(aAscending.meError);
            bAscending = aAscending.maValue;
        }

        std::sort(aSortArray.begin(), aSortArray.end());
        if (aValue.maValue < aSortArray.front() || aValue.maValue > aSortArray.back())
            return makeCellError(api::Error::NotAvailable);

        double fFirstPos = -1.0;
        double fLastPos = 0.0;
        bool bFinished = false;
        std::size_t nIndex = 0;
        for (; nIndex < aSortArray.size() && !bFinished; ++nIndex)
        {
            if (fp::approxEqual(aSortArray[nIndex], aValue.maValue))
            {
                if (fFirstPos < 0.0)
                    fFirstPos = static_cast<double>(nIndex) + 1.0;
            }
            else if (aSortArray[nIndex] > aValue.maValue)
            {
                fLastPos = static_cast<double>(nIndex);
                bFinished = true;
            }
        }
        if (!bFinished)
            fLastPos = static_cast<double>(nIndex);
        if (fFirstPos <= 0.0)
            return makeCellError(api::Error::NotAvailable);

        const bool bAverage
            = aFunctionName == u"RANK.AVG" || aFunctionName == u"COM.MICROSOFT.RANK.AVG";
        const double fSize = static_cast<double>(aSortArray.size());
        if (!bAverage)
        {
            return makeScalarResult(api::CellValue::number(
                bAscending ? fFirstPos : fSize + 1.0 - fLastPos));
        }

        return makeScalarResult(api::CellValue::number(
            bAscending ? (fFirstPos + fLastPos) / 2.0
                       : fSize + 1.0 - (fFirstPos + fLastPos) / 2.0));
    }

    if (aFunctionName == u"GAMMALN" || aFunctionName == u"GAMMALN.PRECISE")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        if (!(aNumber.maValue > 0.0))
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(std::lgamma(aNumber.maValue)));
    }

    if (aFunctionName == u"GEOMEAN" || aFunctionName == u"HARMEAN")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = aContext.collectNumericArguments(true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aResult = (aFunctionName == u"GEOMEAN")
                                 ? semath::evaluateGeometricMeanNumbers(aNumbers.maValue)
                                 : semath::evaluateHarmonicMeanNumbers(aNumbers.maValue);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"POISSON" || aFunctionName == u"POISSON.DIST")
    {
        const bool bLegacyPoisson = aFunctionName == u"POISSON";
        if ((bLegacyPoisson && (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3))
            || (!bLegacyPoisson && rNode.maChildren.size() != 3))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);

        EvaluationResult aLambdaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aLambdaResult)
            return aLambdaResult;
        const auto aLambdaNumber = coerceToNumber(aLambdaResult.maValue.maValue);
        if (!aLambdaNumber)
            return makeFailure(aLambdaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aPoisson = semath::evaluatePoissonDistribution(
            aXNumber.maValue, aLambdaNumber.maValue, bCumulative);
        if (!aPoisson)
            return makeFailure(aPoisson.meError);
        return makeScalarResult(api::CellValue::number(aPoisson.maValue));
    }

    if (aFunctionName == u"LEGACY.CHIDIST")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aChiResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aChiResult)
            return aChiResult;
        EvaluationResult aDfResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDfResult)
            return aDfResult;

        const auto aChiNumber = coerceToNumber(aChiResult.maValue.maValue);
        if (!aChiNumber)
            return makeFailure(aChiNumber.meError);
        const auto aDfNumber = coerceToNumber(aDfResult.maValue.maValue);
        if (!aDfNumber)
            return makeFailure(aDfNumber.meError);

        const auto aChiDist = semath::evaluateLegacyChiDist(
            aChiNumber.maValue, fp::approxFloor(aDfNumber.maValue));
        if (!aChiDist)
            return makeFailure(aChiDist.meError);
        return makeScalarResult(api::CellValue::number(aChiDist.maValue));
    }

    if (aFunctionName == u"CHISQDIST" || aFunctionName == u"CHISQ.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName == u"CHISQ.DIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 3)
            || (!bMicrosoftSyntax
                && (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)))
        {
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        }

        const auto aXNumber = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aDfNumber = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        if (!aXNumber)
            return makeScalarResult(api::CellValue::error(aXNumber.meError));
        if (!aDfNumber)
            return makeScalarResult(api::CellValue::error(aDfNumber.meError));

        bool bCumulative = true;
        if (rNode.maChildren.size() == 3)
        {
            const auto aCumulativeValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[2]);
            if (!aCumulativeValue)
                return makeScalarResult(api::CellValue::error(aCumulativeValue.meError));
            const auto aCumulativeBool = coerceToBoolean(aCumulativeValue.maValue);
            if (!aCumulativeBool)
                return makeScalarResult(api::CellValue::error(aCumulativeBool.meError));
            bCumulative = aCumulativeBool.maValue;
        }

        return aContext.makeNumericOrErrorResult(semath::evaluateChiSquareDistribution(
            aXNumber.maValue, fp::approxFloor(aDfNumber.maValue), bCumulative,
            bMicrosoftSyntax));
    }

    if (aFunctionName == u"CHISQ.DIST.RT" || aFunctionName == u"COM.MICROSOFT.CHISQ.DIST.RT")
    {
        if (rNode.maChildren.size() != 2)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aX = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aDegreesFreedom = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        if (!aX)
            return makeScalarResult(api::CellValue::error(aX.meError));
        if (!aDegreesFreedom)
            return makeScalarResult(api::CellValue::error(aDegreesFreedom.meError));

        const auto aCdf = semath::evaluateChiSquareDistribution(
            aX.maValue, fp::approxFloor(aDegreesFreedom.maValue), true, true);
        if (!aCdf)
            return makeScalarResult(api::CellValue::error(aCdf.meError));
        return makeScalarResult(api::CellValue::number(1.0 - aCdf.maValue));
    }

    if (aFunctionName == u"CHISQ.INV" || aFunctionName == u"COM.MICROSOFT.CHISQ.INV"
        || aFunctionName == u"CHISQINV" || aFunctionName == u"CHISQ.INV.RT"
        || aFunctionName == u"COM.MICROSOFT.CHISQ.INV.RT"
        || aFunctionName == u"LEGACY.CHIINV" || aFunctionName == u"CHIINV")
    {
        if (rNode.maChildren.size() != 2)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aProbability = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aDegreesFreedom = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        if (!aProbability)
            return makeScalarResult(api::CellValue::error(aProbability.meError));
        if (!aDegreesFreedom)
            return makeScalarResult(api::CellValue::error(aDegreesFreedom.meError));

        const double fDegreesFreedom = fp::approxFloor(aDegreesFreedom.maValue);
        if (aFunctionName == u"LEGACY.CHIINV" || aFunctionName == u"CHIINV")
        {
            return aContext.makeNumericOrErrorResult(
                semath::evaluateLegacyChiInverse(aProbability.maValue, fDegreesFreedom));
        }

        const bool bRightTail = aFunctionName == u"CHISQ.INV.RT"
                                || aFunctionName == u"COM.MICROSOFT.CHISQ.INV.RT";
        const double fLeftTailProbability
            = bRightTail ? 1.0 - aProbability.maValue : aProbability.maValue;
        return aContext.makeNumericOrErrorResult(
            semath::evaluateChiSquareInverse(fLeftTailProbability, fDegreesFreedom));
    }

    if (aFunctionName == u"LEGACY.NORMSDIST" || aFunctionName == u"NORMSDIST"
        || aFunctionName == u"NORM.S.DIST" || aFunctionName == u"COM.MICROSOFT.NORM.S.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName == u"NORM.S.DIST"
                                      || aFunctionName == u"COM.MICROSOFT.NORM.S.DIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 2)
            || (!bMicrosoftSyntax && rNode.maChildren.size() != 1))
        {
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        }

        const auto aX = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        if (!aX)
            return makeScalarResult(api::CellValue::error(aX.meError));

        bool bCumulative = true;
        if (bMicrosoftSyntax)
        {
            const auto aCumulativeValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[1]);
            if (!aCumulativeValue)
                return makeScalarResult(api::CellValue::error(aCumulativeValue.meError));
            const auto aCumulativeBool = coerceToBoolean(aCumulativeValue.maValue);
            if (!aCumulativeBool)
                return makeScalarResult(api::CellValue::error(aCumulativeBool.meError));
            bCumulative = aCumulativeBool.maValue;
        }

        return aContext.makeNumericOrErrorResult(
            semath::evaluateNormalDistribution(aX.maValue, 0.0, 1.0, bCumulative));
    }

    if (aFunctionName == u"LEGACY.NORMSINV" || aFunctionName == u"NORMSINV"
        || aFunctionName == u"NORM.S.INV" || aFunctionName == u"COM.MICROSOFT.NORM.S.INV")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aProbability = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        if (!aProbability)
            return makeScalarResult(api::CellValue::error(aProbability.meError));

        return aContext.makeNumericOrErrorResult(
            semath::evaluateStandardNormalInverse(aProbability.maValue));
    }

    if (aFunctionName == u"NORMINV" || aFunctionName == u"NORM.INV"
        || aFunctionName == u"COM.MICROSOFT.NORM.INV")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aProbability = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aMean = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aSigma = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aProbability)
            return makeScalarResult(api::CellValue::error(aProbability.meError));
        if (!aMean)
            return makeScalarResult(api::CellValue::error(aMean.meError));
        if (!aSigma)
            return makeScalarResult(api::CellValue::error(aSigma.meError));

        return aContext.makeNumericOrErrorResult(
            semath::evaluateNormalInverse(aProbability.maValue, aMean.maValue, aSigma.maValue));
    }

    if (aFunctionName == u"STANDARDIZE")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aX = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aMean = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aSigma = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aX)
            return makeScalarResult(api::CellValue::error(aX.meError));
        if (!aMean)
            return makeScalarResult(api::CellValue::error(aMean.meError));
        if (!aSigma)
            return makeScalarResult(api::CellValue::error(aSigma.meError));
        if (aSigma.maValue < 0.0)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        if (fp::approxEqual(aSigma.maValue, 0.0))
            return makeScalarResult(api::CellValue::error(api::Error::DivisionByZero));

        return makeScalarResult(
            api::CellValue::number((aX.maValue - aMean.maValue) / aSigma.maValue));
    }

    if (aFunctionName == u"LOGINV" || aFunctionName == u"LOGNORM.INV"
        || aFunctionName == u"COM.MICROSOFT.LOGNORM.INV")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aProbability = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        if (!aProbability)
            return makeScalarResult(api::CellValue::error(aProbability.meError));

        const auto aMean = rNode.maChildren.size() >= 2
                               ? aContext.evaluateAnchoredNumericArgument(*rNode.maChildren[1], 0.0)
                               : api::ValueResult<double>::success(0.0);
        const auto aSigma = rNode.maChildren.size() == 3
                                ? aContext.evaluateAnchoredNumericArgument(*rNode.maChildren[2], 1.0)
                                : api::ValueResult<double>::success(1.0);
        if (!aMean)
            return makeScalarResult(api::CellValue::error(aMean.meError));
        if (!aSigma)
            return makeScalarResult(api::CellValue::error(aSigma.meError));

        return aContext.makeNumericOrErrorResult(
            semath::evaluateLogNormalInverse(aProbability.maValue, aMean.maValue, aSigma.maValue));
    }

    if (aFunctionName == u"GAMMAINV" || aFunctionName == u"GAMMA.INV"
        || aFunctionName == u"COM.MICROSOFT.GAMMA.INV")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aProbability = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aAlpha = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aBeta = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aProbability)
            return makeScalarResult(api::CellValue::error(aProbability.meError));
        if (!aAlpha)
            return makeScalarResult(api::CellValue::error(aAlpha.meError));
        if (!aBeta)
            return makeScalarResult(api::CellValue::error(aBeta.meError));

        return aContext.makeNumericOrErrorResult(
            semath::evaluateGammaInverse(aProbability.maValue, aAlpha.maValue, aBeta.maValue));
    }

    if (aFunctionName == u"GAMMALN" || aFunctionName == u"GAMMALN.PRECISE"
        || aFunctionName == u"COM.MICROSOFT.GAMMALN.PRECISE")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aValue = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        if (!aValue)
            return makeScalarResult(api::CellValue::error(aValue.meError));
        return aContext.makeNumericOrErrorResult(semath::evaluateLogGammaValue(aValue.maValue));
    }

    if (aFunctionName == u"ERF" || aFunctionName == u"ERF.PRECISE"
        || aFunctionName == u"COM.MICROSOFT.ERF.PRECISE")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aValue = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        if (!aValue)
            return makeScalarResult(api::CellValue::error(aValue.meError));
        return aContext.makeNumericOrErrorResult(semath::evaluateErrorFunction(aValue.maValue));
    }

    if (aFunctionName == u"ERFC" || aFunctionName == u"ERFC.PRECISE"
        || aFunctionName == u"COM.MICROSOFT.ERFC.PRECISE")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aValue = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        if (!aValue)
            return makeScalarResult(api::CellValue::error(aValue.meError));
        return aContext.makeNumericOrErrorResult(
            semath::evaluateComplementaryErrorFunction(aValue.maValue));
    }

    if (aFunctionName == u"NORMDIST" || aFunctionName == u"NORM.DIST")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aMeanResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aSigmaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aMeanResult)
            return aMeanResult;
        if (!aSigmaResult)
            return aSigmaResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aMeanNumber = coerceToNumber(aMeanResult.maValue.maValue);
        const auto aSigmaNumber = coerceToNumber(aSigmaResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aMeanNumber)
            return makeFailure(aMeanNumber.meError);
        if (!aSigmaNumber)
            return makeFailure(aSigmaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateNormalDistribution(
            aXNumber.maValue, aMeanNumber.maValue, aSigmaNumber.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"LOGNORMDIST" || aFunctionName == u"LOGNORM.DIST"
        || aFunctionName == u"COM.MICROSOFT.LOGNORM.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName != u"LOGNORMDIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 4)
            || (!bMicrosoftSyntax
                && (rNode.maChildren.empty() || rNode.maChildren.size() > 4)))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        const auto aXNumber = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);

        const auto aMeanNumber = rNode.maChildren.size() >= 2
                                     ? aContext.evaluateNumericArgument(*rNode.maChildren[1], 0.0)
                                     : api::ValueResult<double>::success(0.0);
        if (!aMeanNumber)
            return makeFailure(aMeanNumber.meError);

        const auto aSigmaNumber = rNode.maChildren.size() >= 3
                                      ? aContext.evaluateNumericArgument(*rNode.maChildren[2], 1.0)
                                      : api::ValueResult<double>::success(1.0);
        if (!aSigmaNumber)
            return makeFailure(aSigmaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateLogNormalDistribution(
            aXNumber.maValue, aMeanNumber.maValue, aSigmaNumber.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"GAMMADIST" || aFunctionName == u"GAMMA.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName == u"GAMMA.DIST";
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aAlphaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aBetaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aAlphaResult)
            return aAlphaResult;
        if (!aBetaResult)
            return aBetaResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aAlphaNumber = coerceToNumber(aAlphaResult.maValue.maValue);
        const auto aBetaNumber = coerceToNumber(aBetaResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aAlphaNumber)
            return makeFailure(aAlphaNumber.meError);
        if (!aBetaNumber)
            return makeFailure(aBetaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateGammaDistribution(
            aXNumber.maValue, aAlphaNumber.maValue, aBetaNumber.maValue, bCumulative,
            bMicrosoftSyntax);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"GAMMA" || aFunctionName == u"COM.MICROSOFT.GAMMA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumber = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aNumber)
            return makeFailure(aNumber.meError);

        const auto aGammaValue = semath::evaluateGammaValue(aNumber.maValue);
        if (!aGammaValue)
            return makeFailure(aGammaValue.meError);
        return makeScalarResult(api::CellValue::number(aGammaValue.maValue));
    }

    if (aFunctionName == u"BETA.INV" || aFunctionName == u"COM.MICROSOFT.BETA.INV"
        || aFunctionName == u"BETAINV")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aProbability = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aAlpha = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aBeta = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aProbability)
            return makeScalarResult(api::CellValue::error(aProbability.meError));
        if (!aAlpha)
            return makeScalarResult(api::CellValue::error(aAlpha.meError));
        if (!aBeta)
            return makeScalarResult(api::CellValue::error(aBeta.meError));

        const auto aLower = rNode.maChildren.size() >= 4
                                ? aContext.evaluateAnchoredNumericArgument(*rNode.maChildren[3], 0.0)
                                : api::ValueResult<double>::success(0.0);
        const auto aUpper = rNode.maChildren.size() == 5
                                ? aContext.evaluateAnchoredNumericArgument(*rNode.maChildren[4], 1.0)
                                : api::ValueResult<double>::success(1.0);
        if (!aLower)
            return makeScalarResult(api::CellValue::error(aLower.meError));
        if (!aUpper)
            return makeScalarResult(api::CellValue::error(aUpper.meError));

        return aContext.makeNumericOrErrorResult(semath::evaluateBetaInverse(
            aProbability.maValue, aAlpha.maValue, aBeta.maValue, aLower.maValue,
            aUpper.maValue));
    }

    if (aFunctionName == u"TINV" || aFunctionName == u"T.INV.2T"
        || aFunctionName == u"COM.MICROSOFT.T.INV.2T"
        || aFunctionName == u"T.INV" || aFunctionName == u"COM.MICROSOFT.T.INV")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aProbability = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aProbability)
            return makeFailure(aProbability.meError);
        if (!aDegreesFreedom)
            return makeFailure(aDegreesFreedom.meError);

        const auto aInverse = semath::evaluateTInverse(
            aProbability.maValue, fp::approxFloor(aDegreesFreedom.maValue),
            (aFunctionName == u"T.INV" || aFunctionName == u"COM.MICROSOFT.T.INV") ? 4 : 2);
        if (!aInverse)
            return makeFailure(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"FDIST" || aFunctionName == u"LEGACY.FDIST"
        || aFunctionName == u"F.DIST.RT" || aFunctionName == u"COM.MICROSOFT.F.DIST.RT")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aX = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aDegreesFreedom1
            = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aDegreesFreedom2
            = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aX)
            return makeCellError(aX.meError);
        if (!aDegreesFreedom1)
            return makeCellError(aDegreesFreedom1.meError);
        if (!aDegreesFreedom2)
            return makeCellError(aDegreesFreedom2.meError);

        const auto aDistribution = semath::evaluateFRightTailDistribution(
            aX.maValue, aDegreesFreedom1.maValue, aDegreesFreedom2.maValue);
        if (!aDistribution)
            return makeCellError(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"T.DIST.2T" || aFunctionName == u"COM.MICROSOFT.T.DIST.2T")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aX = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aX)
            return makeFailure(aX.meError);
        if (!aDegreesFreedom)
            return makeFailure(aDegreesFreedom.meError);
        if (aX.maValue < 0.0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDistribution = semath::evaluateStudentDistribution(
            aX.maValue, fp::approxFloor(aDegreesFreedom.maValue), 2);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"T.DIST.RT" || aFunctionName == u"COM.MICROSOFT.T.DIST.RT")
    {
        if (rNode.maChildren.size() != 2)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aX = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aDegreesFreedom = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        if (!aX)
            return makeScalarResult(api::CellValue::error(aX.meError));
        if (!aDegreesFreedom)
            return makeScalarResult(api::CellValue::error(aDegreesFreedom.meError));

        const auto aDistribution = semath::evaluateStudentDistribution(
            aX.maValue, fp::approxFloor(aDegreesFreedom.maValue), 1);
        if (!aDistribution)
            return makeScalarResult(api::CellValue::error(aDistribution.meError));
        const double fValue
            = aX.maValue < 0.0 ? 1.0 - aDistribution.maValue : aDistribution.maValue;
        return makeScalarResult(api::CellValue::number(fValue));
    }

    if (aFunctionName == u"LEGACY.TDIST")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aX = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aDegreesFreedom = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aTails = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[2]);
        if (!aX)
            return makeCellError(aX.meError);
        if (!aDegreesFreedom)
            return makeCellError(aDegreesFreedom.meError);
        if (!aTails)
            return makeCellError(aTails.meError);
        if (aX.maValue < 0.0 || (aTails.maValue != 1 && aTails.maValue != 2))
            return makeCellError(api::Error::IllegalArgument);

        const auto aDistribution = semath::evaluateStudentDistribution(
            aX.maValue, fp::approxFloor(aDegreesFreedom.maValue), aTails.maValue);
        if (!aDistribution)
            return makeCellError(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"CONFIDENCE" || aFunctionName == u"CONFIDENCE.NORM"
        || aFunctionName == u"COM.MICROSOFT.CONFIDENCE.NORM"
        || aFunctionName == u"CONFIDENCE.T" || aFunctionName == u"COM.MICROSOFT.CONFIDENCE.T")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aAlpha = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aSigma = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aSampleSize = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aAlpha)
            return makeScalarResult(api::CellValue::error(aAlpha.meError));
        if (!aSigma)
            return makeScalarResult(api::CellValue::error(aSigma.meError));
        if (!aSampleSize)
            return makeScalarResult(api::CellValue::error(aSampleSize.meError));

        const double fSampleSize = fp::approxFloor(aSampleSize.maValue);
        if (aFunctionName == u"CONFIDENCE.T" || aFunctionName == u"COM.MICROSOFT.CONFIDENCE.T")
        {
            return aContext.makeNumericOrErrorResult(
                semath::evaluateConfidenceT(aAlpha.maValue, aSigma.maValue, fSampleSize));
        }

        return aContext.makeNumericOrErrorResult(
            semath::evaluateConfidence(aAlpha.maValue, aSigma.maValue, fSampleSize));
    }

    if (aFunctionName == u"FINV" || aFunctionName == u"LEGACY.FINV"
        || aFunctionName == u"F.INV.RT" || aFunctionName == u"COM.MICROSOFT.F.INV.RT"
        || aFunctionName == u"F.INV" || aFunctionName == u"COM.MICROSOFT.F.INV")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aProbability = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom1 = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aDegreesFreedom2 = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aProbability)
            return makeCellError(aProbability.meError);
        if (!aDegreesFreedom1)
            return makeCellError(aDegreesFreedom1.meError);
        if (!aDegreesFreedom2)
            return makeCellError(aDegreesFreedom2.meError);

        const double fDegreesFreedom1 = fp::approxFloor(aDegreesFreedom1.maValue);
        const double fDegreesFreedom2 = fp::approxFloor(aDegreesFreedom2.maValue);
        const bool bLeftTail = aFunctionName == u"FINV" || aFunctionName == u"F.INV"
                               || aFunctionName == u"COM.MICROSOFT.F.INV";
        if (bLeftTail && (aProbability.maValue <= 0.0 || aProbability.maValue >= 1.0))
            return makeCellError(api::Error::IllegalArgument);
        const double fRightTailProbability
            = bLeftTail ? 1.0 - aProbability.maValue : aProbability.maValue;
        const auto aInverse = semath::evaluateFInverseRightTail(
            fRightTailProbability, fDegreesFreedom1, fDegreesFreedom2);
        if (!aInverse)
            return makeCellError(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"VAR" || aFunctionName == u"VAR.S" || aFunctionName == u"VARP"
        || aFunctionName == u"VAR.P" || aFunctionName == u"VARA" || aFunctionName == u"VARPA"
        || aFunctionName == u"STDEV" || aFunctionName == u"STDEV.S"
        || aFunctionName == u"STDEVP" || aFunctionName == u"STDEV.P"
        || aFunctionName == u"STDEVA" || aFunctionName == u"STDEVPA")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const bool bTextAsZero = aFunctionName == u"VARA" || aFunctionName == u"VARPA"
                                 || aFunctionName == u"STDEVA" || aFunctionName == u"STDEVPA";
        const auto aNumbers = aContext.collectVarianceArguments(bTextAsZero);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);

        const bool bSample = aFunctionName == u"VAR" || aFunctionName == u"VAR.S"
                             || aFunctionName == u"VARA" || aFunctionName == u"STDEV"
                             || aFunctionName == u"STDEV.S" || aFunctionName == u"STDEVA";
        const bool bReturnStdDev = aFunctionName == u"STDEV" || aFunctionName == u"STDEV.S"
                                   || aFunctionName == u"STDEVP"
                                   || aFunctionName == u"STDEV.P"
                                   || aFunctionName == u"STDEVA"
                                   || aFunctionName == u"STDEVPA";
        const auto aVariance = semath::evaluateVarianceNumbers(
            aNumbers.maValue, bSample, bReturnStdDev);
        if (!aVariance)
            return makeFailure(aVariance.meError);
        return makeScalarResult(api::CellValue::number(aVariance.maValue));
    }

    if (aFunctionName == u"KURT")
    {
        if (rNode.maChildren.empty())
            return makeCellError(api::Error::IllegalArgument);

        const auto aNumbers = aContext.collectVarianceArguments(false);
        if (!aNumbers)
            return makeCellError(aNumbers.meError);

        const auto aKurtosis = semath::evaluateKurtosisNumbers(aNumbers.maValue);
        if (!aKurtosis)
            return makeCellError(aKurtosis.meError);
        return makeScalarResult(api::CellValue::number(aKurtosis.maValue));
    }

    if (aFunctionName == u"BINOMDIST" || aFunctionName == u"BINOM.DIST")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aNResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aPResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        EvaluationResult aCumulativeResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aNResult)
            return aNResult;
        if (!aPResult)
            return aPResult;
        if (!aCumulativeResult)
            return aCumulativeResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aNNumber = coerceToNumber(aNResult.maValue.maValue);
        const auto aPNumber = coerceToNumber(aPResult.maValue.maValue);
        const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aNNumber)
            return makeFailure(aNNumber.meError);
        if (!aPNumber)
            return makeFailure(aPNumber.meError);
        if (!aCumulativeBool)
            return makeFailure(aCumulativeBool.meError);

        const auto aBinomial = semath::evaluateBinomialDistribution(
            aXNumber.maValue, aNNumber.maValue, aPNumber.maValue, aCumulativeBool.maValue);
        if (!aBinomial)
            return makeFailure(aBinomial.meError);
        return makeScalarResult(api::CellValue::number(aBinomial.maValue));
    }

    if (aFunctionName == u"BINOM.INV")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTrials = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aProbability = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aAlpha = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aTrials)
            return makeFailure(aTrials.meError);
        if (!aProbability)
            return makeFailure(aProbability.meError);
        if (!aAlpha)
            return makeFailure(aAlpha.meError);

        const auto aInverse = semath::evaluateBinomialInverse(
            aTrials.maValue, aProbability.maValue, aAlpha.maValue);
        if (!aInverse)
            return makeFailure(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"CRITBINOM")
    {
        if (rNode.maChildren.size() != 3)
            return makeCellError(api::Error::IllegalArgument);

        const auto aTrials = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aProbability = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aAlpha = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aTrials)
            return makeCellError(aTrials.meError);
        if (!aProbability)
            return makeCellError(aProbability.meError);
        if (!aAlpha)
            return makeCellError(aAlpha.meError);

        const auto aInverse = semath::evaluateBinomialInverse(
            aTrials.maValue, aProbability.maValue, aAlpha.maValue);
        if (!aInverse)
            return makeCellError(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"BINOM.DIST.RANGE" || aFunctionName == u"B")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aPResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aStartResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aNResult)
            return aNResult;
        if (!aPResult)
            return aPResult;
        if (!aStartResult)
            return aStartResult;

        const auto aNNumber = coerceToNumber(aNResult.maValue.maValue);
        const auto aPNumber = coerceToNumber(aPResult.maValue.maValue);
        const auto aStartNumber = coerceToNumber(aStartResult.maValue.maValue);
        if (!aNNumber)
            return makeFailure(aNNumber.meError);
        if (!aPNumber)
            return makeFailure(aPNumber.meError);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);

        double fEnd = aStartNumber.maValue;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aEndResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aEndResult)
                return aEndResult;
            const auto aEndNumber = coerceToNumber(aEndResult.maValue.maValue);
            if (!aEndNumber)
                return makeFailure(aEndNumber.meError);
            fEnd = aEndNumber.maValue;
        }

        const auto aRange = semath::evaluateBinomialRangeDistribution(
            aNNumber.maValue, aPNumber.maValue, aStartNumber.maValue, fEnd);
        if (!aRange)
            return makeFailure(aRange.meError);
        return makeScalarResult(api::CellValue::number(aRange.maValue));
    }

    if (aFunctionName == u"NEGBINOMDIST" || aFunctionName == u"NEGBINOM.DIST"
        || aFunctionName == u"COM.MICROSOFT.NEGBINOM.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName != u"NEGBINOMDIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 4)
            || (!bMicrosoftSyntax && rNode.maChildren.size() != 3))
        {
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        }

        const auto aFailures = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aSuccesses = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aProbability = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aFailures)
            return makeScalarResult(api::CellValue::error(aFailures.meError));
        if (!aSuccesses)
            return makeScalarResult(api::CellValue::error(aSuccesses.meError));
        if (!aProbability)
            return makeScalarResult(api::CellValue::error(aProbability.meError));

        bool bCumulative = false;
        if (bMicrosoftSyntax)
        {
            const auto aCumulativeValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[3]);
            if (!aCumulativeValue)
                return makeScalarResult(api::CellValue::error(aCumulativeValue.meError));
            const auto aCumulativeBool = coerceToBoolean(aCumulativeValue.maValue);
            if (!aCumulativeBool)
                return makeScalarResult(api::CellValue::error(aCumulativeBool.meError));
            bCumulative = aCumulativeBool.maValue;
        }

        return aContext.makeNumericOrErrorResult(semath::evaluateNegativeBinomialDistribution(
            aFailures.maValue, aSuccesses.maValue, aProbability.maValue, bCumulative,
            bMicrosoftSyntax));
    }

    if (aFunctionName == u"EXPONDIST" || aFunctionName == u"EXPON.DIST"
        || aFunctionName == u"COM.MICROSOFT.EXPON.DIST")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aX = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aLambda = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        if (!aX)
            return makeScalarResult(api::CellValue::error(aX.meError));
        if (!aLambda)
            return makeScalarResult(api::CellValue::error(aLambda.meError));

        const auto aCumulativeValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[2]);
        if (!aCumulativeValue)
            return makeScalarResult(api::CellValue::error(aCumulativeValue.meError));
        const auto aCumulativeBool = coerceToBoolean(aCumulativeValue.maValue);
        if (!aCumulativeBool)
            return makeScalarResult(api::CellValue::error(aCumulativeBool.meError));

        return aContext.makeNumericOrErrorResult(semath::evaluateExponentialDistribution(
            aX.maValue, aLambda.maValue, aCumulativeBool.maValue));
    }

    if (aFunctionName == u"WEIBULL" || aFunctionName == u"WEIBULL.DIST"
        || aFunctionName == u"COM.MICROSOFT.WEIBULL.DIST")
    {
        if (rNode.maChildren.size() != 4)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aX = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[0]);
        const auto aAlpha = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[1]);
        const auto aBeta = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aX)
            return makeScalarResult(api::CellValue::error(aX.meError));
        if (!aAlpha)
            return makeScalarResult(api::CellValue::error(aAlpha.meError));
        if (!aBeta)
            return makeScalarResult(api::CellValue::error(aBeta.meError));

        const auto aCumulativeValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[3]);
        if (!aCumulativeValue)
            return makeScalarResult(api::CellValue::error(aCumulativeValue.meError));
        const auto aCumulativeBool = coerceToBoolean(aCumulativeValue.maValue);
        if (!aCumulativeBool)
            return makeScalarResult(api::CellValue::error(aCumulativeBool.meError));

        return aContext.makeNumericOrErrorResult(semath::evaluateWeibullDistribution(
            aX.maValue, aAlpha.maValue, aBeta.maValue, aCumulativeBool.maValue));
    }

    if (aFunctionName == u"HYPGEOMDIST" || aFunctionName == u"HYPGEOM.DIST")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aX = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aTrials = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aSuccesses = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        const auto aPopulation = aContext.evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
        if (!aX)
            return makeFailure(aX.meError);
        if (!aTrials)
            return makeFailure(aTrials.meError);
        if (!aSuccesses)
            return makeFailure(aSuccesses.meError);
        if (!aPopulation)
            return makeFailure(aPopulation.meError);

        bool bCumulative = false;
        if (rNode.maChildren.size() == 5)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[4], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateHypergeometricDistribution(
            aX.maValue, aTrials.maValue, aSuccesses.maValue, aPopulation.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"PROB")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeCellError(api::Error::IllegalArgument);

        const auto aUpper = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[2]);
        if (!aUpper)
            return makeCellError(aUpper.meError);

        double fLower = aUpper.maValue;
        if (rNode.maChildren.size() == 4)
        {
            const auto aLower
                = aContext.evaluateRequiredAnchoredNumberArgument(*rNode.maChildren[3]);
            if (!aLower)
                return makeCellError(aLower.meError);
            fLower = aLower.maValue;
        }

        const auto aProbabilities = materializeMatrixOperand(*rNode.maChildren[0]);
        if (!aProbabilities)
            return makeCellError(aProbabilities.meError);
        const auto aValues = materializeMatrixOperand(*rNode.maChildren[1]);
        if (!aValues)
            return makeCellError(aValues.meError);
        if (aProbabilities.maValue.mnColumns != aValues.maValue.mnColumns
            || aProbabilities.maValue.mnRows != aValues.maValue.mnRows
            || aProbabilities.maValue.maValues.empty())
        {
            return makeCellError(api::Error::NotAvailable);
        }

        std::vector<double> aProbabilityValues;
        std::vector<double> aDataValues;
        aProbabilityValues.reserve(aProbabilities.maValue.maValues.size());
        aDataValues.reserve(aValues.maValue.maValues.size());
        for (std::size_t nIndex = 0; nIndex < aProbabilities.maValue.maValues.size(); ++nIndex)
        {
            const api::CellValue& rProbability = aProbabilities.maValue.maValues[nIndex];
            const api::CellValue& rValue = aValues.maValue.maValues[nIndex];
            if (rProbability.isError())
                return makeCellError(rProbability.meError);
            if (rValue.isError())
                return makeCellError(rValue.meError);
            if (!rProbability.isNumber() || !rValue.isNumber())
                return makeCellError(api::Error::IllegalArgument);

            aProbabilityValues.push_back(rProbability.mfNumber);
            aDataValues.push_back(rValue.mfNumber);
        }

        return aContext.makeNumericOrErrorResult(
            semath::evaluateProbability(aProbabilityValues, aDataValues, fLower, aUpper.maValue));
    }

    if (aFunctionName == u"PERCENTRANK" || aFunctionName == u"PERCENTRANK.INC"
        || aFunctionName == u"PERCENTRANK.EXC")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aScan = aContext.collectAggregateScanFromArgument(*rNode.maChildren[0]);
        if (!aScan)
            return makeFailure(aScan.meError);

        const auto aValue = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        std::int32_t nSignificance = 3;
        if (rNode.maChildren.size() == 3)
        {
            const auto aSignificance = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aSignificance)
                return makeFailure(aSignificance.meError);
            nSignificance = static_cast<std::int32_t>(fp::approxFloor(aSignificance.maValue));
        }

        const bool bInclusive = aFunctionName != u"PERCENTRANK.EXC";
        const auto aRank = semath::evaluatePercentrank(
            aScan.maValue.maNumbers, aValue.maValue, bInclusive, nSignificance);
        if (!aRank)
            return makeFailure(aRank.meError);
        return makeScalarResult(api::CellValue::number(aRank.maValue));
    }

    if (aFunctionName == u"MODE.SNGL")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        AggregateScan aScan;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aCollected = aContext.collectAggregateScanFromArgument(*pChild);
            if (!aCollected)
                return makeFailure(aCollected.meError);
            aScan.maNumbers.insert(aScan.maNumbers.end(), aCollected.maValue.maNumbers.begin(),
                aCollected.maValue.maNumbers.end());
        }

        const auto aMode = semath::evaluateModeSingle(aScan.maNumbers);
        if (!aMode)
            return makeFailure(aMode.meError);
        return makeScalarResult(api::CellValue::number(aMode.maValue));
    }

    if (aFunctionName == u"TRIMMEAN")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aScan = aContext.collectAggregateScanFromArgument(*rNode.maChildren[0]);
        if (!aScan)
            return makeFailure(aScan.meError);

        const auto aPercent = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aPercent)
            return makeFailure(aPercent.meError);

        const auto aTrimmean = semath::evaluateTrimmean(aScan.maValue.maNumbers, aPercent.maValue);
        if (!aTrimmean)
            return makeFailure(aTrimmean.meError);
        return makeScalarResult(api::CellValue::number(aTrimmean.maValue));
    }

    if (aFunctionName == u"CHISQ.TEST" || aFunctionName == u"LEGACY.CHITEST")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aObservedInput = aContext.evaluateLookupInputNode(*rNode.maChildren[0]);
        const auto aExpectedInput = aContext.evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aObservedInput)
            return makeFailure(aObservedInput.meError);
        if (!aExpectedInput)
            return makeFailure(aExpectedInput.meError);

        const api::MatrixSize nObservedColumns = aObservedInput.maValue.mbScalar
                                                     ? 1
                                                     : aObservedInput.maValue.mnColumns;
        const api::MatrixSize nObservedRows = aObservedInput.maValue.mbScalar ? 1
                                                                              : aObservedInput.maValue.mnRows;
        const api::MatrixSize nExpectedColumns = aExpectedInput.maValue.mbScalar
                                                     ? 1
                                                     : aExpectedInput.maValue.mnColumns;
        const api::MatrixSize nExpectedRows = aExpectedInput.maValue.mbScalar ? 1
                                                                              : aExpectedInput.maValue.mnRows;
        if (nObservedColumns != nExpectedColumns || nObservedRows != nExpectedRows)
            return makeFailure(api::Error::IllegalArgument);

        auto materializeInputCell = [&](const LookupInput& rInput, api::MatrixSize nColumn,
                                        api::MatrixSize nRow)
            -> api::ValueResult<api::CellValue> {
            if (rInput.mbScalar)
            {
                if (nColumn != 0 || nRow != 0)
                    return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
                return api::ValueResult<api::CellValue>::success(rInput.maScalar);
            }

            const std::size_t nIndex = static_cast<std::size_t>(nRow * rInput.mnColumns + nColumn);
            if (!rInput.maValues.empty())
            {
                if (nIndex >= rInput.maValues.size())
                    return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
                return api::ValueResult<api::CellValue>::success(rInput.maValues[nIndex]);
            }

            EvaluationResult aCell = materializeReferenceValue(rInput.maReference, nColumn, nRow);
            if (!aCell)
                return api::ValueResult<api::CellValue>::failure(aCell.meError);
            if (!aCell.maValue.isScalar())
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(aCell.maValue.maValue);
        };

        fp::KahanSum fChi = 0.0;
        bool bSawNonEmptyPair = false;
        for (api::MatrixSize nColumn = 0; nColumn < nObservedColumns; ++nColumn)
        {
            for (api::MatrixSize nRow = 0; nRow < nObservedRows; ++nRow)
            {
                const auto aObservedCell = materializeInputCell(aObservedInput.maValue, nColumn, nRow);
                const auto aExpectedCell = materializeInputCell(aExpectedInput.maValue, nColumn, nRow);
                if (!aObservedCell)
                    return makeFailure(aObservedCell.meError);
                if (!aExpectedCell)
                    return makeFailure(aExpectedCell.meError);

                if (aObservedCell.maValue.isEmpty() || aExpectedCell.maValue.isEmpty())
                    continue;

                bSawNonEmptyPair = true;
                if (aObservedCell.maValue.isText() || aExpectedCell.maValue.isText())
                    return makeFailure(api::Error::IllegalArgument);
                if (aObservedCell.maValue.isError())
                    return makeFailure(aObservedCell.maValue.meError);
                if (aExpectedCell.maValue.isError())
                    return makeFailure(aExpectedCell.maValue.meError);

                const auto aObservedNumber = coerceToNumber(aObservedCell.maValue);
                const auto aExpectedNumber = coerceToNumber(aExpectedCell.maValue);
                if (!aObservedNumber)
                    return makeFailure(aObservedNumber.meError);
                if (!aExpectedNumber)
                    return makeFailure(aExpectedNumber.meError);
                if (fp::approxEqual(aExpectedNumber.maValue, 0.0))
                    return makeFailure(api::Error::DivisionByZero);

                const double fDifference = aObservedNumber.maValue - aExpectedNumber.maValue;
                const double fTerm = (fDifference * fDifference) / aExpectedNumber.maValue;
                if (std::isinf(fTerm))
                    return makeFailure(api::Error::NoConvergence);
                fChi += fTerm;
            }
        }

        if (!bSawNonEmptyPair)
            return makeFailure(api::Error::IllegalArgument);

        double fDegreesFreedom = 0.0;
        if (nObservedColumns == 1 || nObservedRows == 1)
        {
            fDegreesFreedom = static_cast<double>(nObservedColumns * nObservedRows - 1);
            if (fp::approxEqual(fDegreesFreedom, 0.0))
                return makeFailure(api::Error::NotAvailable);
        }
        else
        {
            fDegreesFreedom
                = static_cast<double>(nObservedColumns - 1) * static_cast<double>(nObservedRows - 1);
        }

        const auto aChiDist = semath::evaluateLegacyChiDist(fChi.get(), fDegreesFreedom);
        if (!aChiDist)
            return makeFailure(aChiDist.meError);
        return makeScalarResult(api::CellValue::number(aChiDist.maValue));
    }

    if (aFunctionName == u"BETADIST" || aFunctionName == u"BETA.DIST")
    {
        const bool bMicrosoftOrder = aFunctionName == u"BETA.DIST";
        if ((bMicrosoftOrder && (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6))
            || (!bMicrosoftOrder && (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)))
        {
            return makeCellError(api::Error::IllegalArgument);
        }

        auto evaluateScalarNumber = [&](std::size_t nIndex) -> api::ValueResult<double> {
            EvaluationResult aValue
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress));
            if (!aValue)
                return api::ValueResult<double>::failure(aValue.meError);
            return coerceToNumber(aValue.maValue.maValue);
        };

        const auto aXNumber = evaluateScalarNumber(0);
        const auto aAlphaNumber = evaluateScalarNumber(1);
        const auto aBetaNumber = evaluateScalarNumber(2);
        if (!aXNumber)
            return makeCellError(aXNumber.meError);
        if (!aAlphaNumber)
            return makeCellError(aAlphaNumber.meError);
        if (!aBetaNumber)
            return makeCellError(aBetaNumber.meError);

        bool bCumulative = true;
        double fLowerBound = 0.0;
        double fUpperBound = 1.0;
        if (bMicrosoftOrder)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return makeCellError(aCumulativeResult.meError);
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeCellError(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
            if (rNode.maChildren.size() >= 5)
            {
                const auto aLower = evaluateScalarNumber(4);
                if (!aLower)
                    return makeCellError(aLower.meError);
                fLowerBound = aLower.maValue;
            }
            if (rNode.maChildren.size() >= 6)
            {
                const auto aUpper = evaluateScalarNumber(5);
                if (!aUpper)
                    return makeCellError(aUpper.meError);
                fUpperBound = aUpper.maValue;
            }
        }
        else
        {
            if (rNode.maChildren.size() >= 4)
            {
                const auto aLower = evaluateScalarNumber(3);
                if (!aLower)
                    return makeCellError(aLower.meError);
                fLowerBound = aLower.maValue;
            }
            if (rNode.maChildren.size() >= 5)
            {
                const auto aUpper = evaluateScalarNumber(4);
                if (!aUpper)
                    return makeCellError(aUpper.meError);
                fUpperBound = aUpper.maValue;
            }
            if (rNode.maChildren.size() == 6)
            {
                EvaluationResult aCumulativeResult = ensureScalarValue(
                    *this, evaluateNode(*rNode.maChildren[5], rCurrentAddress));
                if (!aCumulativeResult)
                    return makeCellError(aCumulativeResult.meError);
                const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
                if (!aCumulativeBool)
                    return makeCellError(aCumulativeBool.meError);
                bCumulative = aCumulativeBool.maValue;
            }
        }

        const auto aBetaDistribution = semath::evaluateBetaDistribution(aXNumber.maValue,
            aAlphaNumber.maValue, aBetaNumber.maValue, fLowerBound, fUpperBound, bCumulative,
            bMicrosoftOrder);
        if (!aBetaDistribution)
            return makeCellError(aBetaDistribution.meError);
        return makeScalarResult(api::CellValue::number(aBetaDistribution.maValue));
    }

        return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
