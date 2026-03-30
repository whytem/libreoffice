/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/FormulaEvaluator.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>

#include "FormulaEvaluatorUtils.hxx"

#include <rtl/math.hxx>

namespace spreadsheetengine::core::eval
{
namespace
{

namespace semath = spreadsheetengine::core::math;

[[nodiscard]] bool formulaContainsAggregateLike(const formula::Node& rNode)
{
    if (rNode.meKind == formula::NodeKind::FunctionCall)
    {
        const api::String aName = detail::normalizeFunctionName(rNode.maPrimaryText);
        if (aName == u"AGGREGATE" || aName == u"SUBTOTAL")
            return true;
    }

    for (const auto& pChild : rNode.maChildren)
    {
        if (formulaContainsAggregateLike(*pChild))
            return true;
    }

    return false;
}

[[nodiscard]] bool cellContainsAggregateLike(const workbook::Cell& rCell)
{
    if (!rCell.hasFormula())
        return false;

    const formula::ParseResult aParsed = formula::parseFormula(rCell.maFormula);
    return aParsed && formulaContainsAggregateLike(*aParsed.mpRoot);
}

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateAggregateFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const bool bRankedAggregate
        = rFunctionName == u"LARGE" || rFunctionName == u"SMALL"
          || rFunctionName == u"PERCENTILE" || rFunctionName == u"PERCENTILE.INC"
          || rFunctionName == u"COM.MICROSOFT.PERCENTILE.INC"
          || rFunctionName == u"PERCENTILE.EXC"
          || rFunctionName == u"COM.MICROSOFT.PERCENTILE.EXC"
          || rFunctionName == u"QUARTILE" || rFunctionName == u"QUARTILE.INC"
          || rFunctionName == u"COM.MICROSOFT.QUARTILE.INC"
          || rFunctionName == u"QUARTILE.EXC"
          || rFunctionName == u"COM.MICROSOFT.QUARTILE.EXC";
    if (!(rFunctionName == u"MAX" || rFunctionName == u"MIN" || rFunctionName == u"MAXA"
            || rFunctionName == u"MINA" || rFunctionName == u"SUBTOTAL"
            || rFunctionName == u"AGGREGATE" || bRankedAggregate || rFunctionName == u"SKEW"
            || rFunctionName == u"SKEWP"))
    {
        return std::nullopt;
    }

    auto visitFlattenedValues
        = [&](const auto& self, const formula::Node& rArgument,
              const auto& rVisitor) -> api::ValueResult<bool> {
        if (rArgument.meKind == formula::NodeKind::ArrayConstant
            || rArgument.meKind == formula::NodeKind::ReferenceList)
        {
            for (const auto& pChild : rArgument.maChildren)
            {
                const auto aChild = self(self, *pChild, rVisitor);
                if (!aChild)
                    return aChild;
            }
            return api::ValueResult<bool>::success(true);
        }

        const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                    || rArgument.meKind == formula::NodeKind::RangeReference
                                    || rArgument.meKind == formula::NodeKind::NamedReference;

        EvaluationResult aValue = bReferenceLike ? evaluateReferenceNode(rArgument, rCurrentAddress)
                                                 : evaluateNode(rArgument, rCurrentAddress);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);

        auto visitScalar = [&](const api::CellValue& rValue,
                               bool bFromReference) -> api::ValueResult<bool> {
            return rVisitor(rValue, bFromReference);
        };

        if (aValue.maValue.isMatrixReference())
        {
            const auto& rReference = aValue.maValue.maReference;
            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                        return api::ValueResult<bool>::failure(aCell.meError);

                    const auto aVisited = visitScalar(aCell.maValue.maValue, true);
                    if (!aVisited)
                        return aVisited;
                }
            }

            return api::ValueResult<bool>::success(true);
        }

        return visitScalar(aValue.maValue.maValue, bReferenceLike);
    };

    auto collectVarianceArguments = [&](bool bTextAsZero)
        -> api::ValueResult<std::vector<double>> {
        std::vector<double> aNumbers;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
                [&](const api::CellValue& rValue,
                    bool bFromReference) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);

                    if (rValue.isEmpty())
                        return api::ValueResult<bool>::success(true);

                    if (rValue.isText())
                    {
                        if (bTextAsZero)
                        {
                            aNumbers.push_back(0.0);
                            return api::ValueResult<bool>::success(true);
                        }

                        if (bFromReference)
                            return api::ValueResult<bool>::success(true);
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
                    }

                    if (rValue.isBoolean() && bFromReference && !bTextAsZero)
                        return api::ValueResult<bool>::success(true);

                    const auto aNumber = detail::coerceToNumber(rValue);
                    if (!aNumber)
                        return api::ValueResult<bool>::failure(aNumber.meError);
                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(aNumbers);
    };

    auto collectAggregateScanFromArgument = [&](const formula::Node& rArgument)
        -> api::ValueResult<semath::AggregateScan> {
        semath::AggregateScan aScan;
        const auto aVisited = visitFlattenedValues(
            visitFlattenedValues, rArgument,
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isNumber())
                    aScan.maNumbers.push_back(rValue.mfNumber);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<semath::AggregateScan>::failure(aVisited.meError);
        return api::ValueResult<semath::AggregateScan>::success(std::move(aScan));
    };

    auto collectExtremaArguments = [&]() -> api::ValueResult<std::vector<double>> {
        std::vector<double> aNumbers;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
                [&](const api::CellValue& rValue,
                    bool bFromReference) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);

                    if (bFromReference)
                    {
                        if (rValue.isNumber())
                            aNumbers.push_back(rValue.mfNumber);
                        return api::ValueResult<bool>::success(true);
                    }

                    const auto aNumber = detail::coerceToNumber(rValue);
                    if (!aNumber)
                        return api::ValueResult<bool>::failure(aNumber.meError);
                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(std::move(aNumbers));
    };

    auto evaluateNumericArgument = [&](const formula::Node& rArgument,
                                      std::optional<double> oDefaultForEmpty)
        -> api::ValueResult<double> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        EvaluationResult aValue
            = detail::ensureScalarValue(*this, evaluateNode(rArgument, rCurrentAddress));
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);

        const auto aNumber = detail::coerceToNumber(aValue.maValue.maValue);
        if (!aNumber)
            return api::ValueResult<double>::failure(aNumber.meError);
        return aNumber;
    };

    if (rFunctionName == u"MAX" || rFunctionName == u"MIN")
    {
        if (rNode.maChildren.empty())
            return detail::makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = collectExtremaArguments();
        if (!aNumbers)
            return detail::makeFailure(aNumbers.meError);

        const auto aExtrema
            = semath::evaluateExtremaNumbers(aNumbers.maValue, rFunctionName == u"MAX", true);
        if (!aExtrema)
            return detail::makeFailure(aExtrema.meError);
        return detail::makeScalarResult(api::CellValue::number(aExtrema.maValue));
    }

    if (rFunctionName == u"MAXA" || rFunctionName == u"MINA")
    {
        if (rNode.maChildren.empty())
            return detail::makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = collectVarianceArguments(true);
        if (!aNumbers)
            return detail::makeFailure(aNumbers.meError);

        const auto aExtrema
            = semath::evaluateExtremaNumbers(aNumbers.maValue, rFunctionName == u"MAXA", false);
        if (!aExtrema)
            return detail::makeFailure(aExtrema.meError);
        return detail::makeScalarResult(api::CellValue::number(aExtrema.maValue));
    }

    if (rFunctionName == u"SUBTOTAL")
    {
        if (rNode.maChildren.size() < 2)
            return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aFunctionCode
            = detail::ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aFunctionCode)
        {
            return aFunctionCode.maCyclePath.empty()
                       ? detail::makeScalarResult(api::CellValue::error(aFunctionCode.meError))
                       : aFunctionCode;
        }

        const auto aFunctionNumber = detail::coerceToNumber(aFunctionCode.maValue.maValue);
        if (!aFunctionNumber)
            return detail::makeScalarResult(api::CellValue::error(aFunctionNumber.meError));

        const auto oFunctionCode = detail::toWholeNumber(aFunctionNumber.maValue);
        if (!oFunctionCode)
            return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        bool bIgnoreHiddenRows = false;
        sal_Int32 nAggregateFunction = 0;
        if (*oFunctionCode >= 1 && *oFunctionCode <= 11)
            nAggregateFunction = *oFunctionCode;
        else if (*oFunctionCode >= 101 && *oFunctionCode <= 111)
        {
            nAggregateFunction = *oFunctionCode - 100;
            bIgnoreHiddenRows = true;
        }
        else
            return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        semath::AggregateScan aScan;
        auto consumeSubtotalValue = [&](const api::CellValue& rValue) -> api::ValueResult<bool> {
            if (rValue.isError())
            {
                if (nAggregateFunction == 2)
                    return api::ValueResult<bool>::success(true);
                if (nAggregateFunction == 3)
                {
                    ++aScan.mnNonEmptyCount;
                    return api::ValueResult<bool>::success(true);
                }
                return api::ValueResult<bool>::failure(rValue.meError);
            }

            if (!rValue.isEmpty())
                ++aScan.mnNonEmptyCount;

            if (rValue.isNumber())
                aScan.maNumbers.push_back(rValue.mfNumber);
            return api::ValueResult<bool>::success(true);
        };

        auto scanSubtotalArgument = [&](const formula::Node& rArgument) -> api::ValueResult<bool> {
            const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                        || rArgument.meKind == formula::NodeKind::RangeReference
                                        || rArgument.meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike ? evaluateReferenceNode(rArgument, rCurrentAddress)
                                                        : evaluateNode(rArgument, rCurrentAddress);
            if (!aArgument)
            {
                if (nAggregateFunction == 2)
                    return api::ValueResult<bool>::success(true);
                if (nAggregateFunction == 3)
                {
                    ++aScan.mnNonEmptyCount;
                    return api::ValueResult<bool>::success(true);
                }
                return api::ValueResult<bool>::failure(aArgument.meError);
            }

            if (!aArgument.maValue.isMatrixReference())
                return consumeSubtotalValue(aArgument.maValue.maValue);

            const auto& rReference = aArgument.maValue.maReference;
            const workbook::Sheet* pSheet = getSheet(rReference.maRange.maStart.mnSheet);
            if (!pSheet)
                return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    const api::CellAddress aAddress = rReference.addressAt(nCol, nRow);
                    const bool bFilteredRow = pSheet->isRowFiltered(aAddress.mnRow);
                    const bool bManuallyHiddenRow = pSheet->isRowHidden(aAddress.mnRow);
                    if (bFilteredRow || (bIgnoreHiddenRows && bManuallyHiddenRow))
                        continue;

                    const workbook::Cell* pReferencedCell = getCell(aAddress);
                    if (pReferencedCell && cellContainsAggregateLike(*pReferencedCell))
                        continue;

                    EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                    {
                        if (nAggregateFunction == 2)
                            continue;
                        if (nAggregateFunction == 3)
                        {
                            ++aScan.mnNonEmptyCount;
                            continue;
                        }
                        return api::ValueResult<bool>::failure(aCell.meError);
                    }

                    if (!aCell.maValue.isScalar())
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                    const auto aConsumed = consumeSubtotalValue(aCell.maValue.maValue);
                    if (!aConsumed)
                        return aConsumed;
                }
            }
            return api::ValueResult<bool>::success(true);
        };

        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aScanned = scanSubtotalArgument(*rNode.maChildren[nIndex]);
            if (!aScanned)
                return detail::makeScalarResult(api::CellValue::error(aScanned.meError));
        }

        const auto aAggregate = semath::evaluateAggregateNumbers(nAggregateFunction, aScan);
        if (!aAggregate)
            return detail::makeScalarResult(api::CellValue::error(aAggregate.meError));
        return detail::makeScalarResult(api::CellValue::number(aAggregate.maValue));
    }

    if (rFunctionName == u"AGGREGATE")
    {
        if (rNode.maChildren.size() < 3)
            return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aFunctionCode
            = detail::ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aFunctionCode)
        {
            return aFunctionCode.maCyclePath.empty()
                       ? detail::makeScalarResult(api::CellValue::error(aFunctionCode.meError))
                       : aFunctionCode;
        }

        EvaluationResult aOptionCode
            = detail::ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aOptionCode)
        {
            return aOptionCode.maCyclePath.empty()
                       ? detail::makeScalarResult(api::CellValue::error(aOptionCode.meError))
                       : aOptionCode;
        }

        const auto aFunctionNumber = detail::coerceToNumber(aFunctionCode.maValue.maValue);
        if (!aFunctionNumber)
            return detail::makeScalarResult(api::CellValue::error(aFunctionNumber.meError));
        const auto aOptionNumber = detail::coerceToNumber(aOptionCode.maValue.maValue);
        if (!aOptionNumber)
            return detail::makeScalarResult(api::CellValue::error(aOptionNumber.meError));

        const auto oFunction = detail::toWholeNumber(aFunctionNumber.maValue);
        const auto oOption = detail::toWholeNumber(aOptionNumber.maValue);
        if (!oFunction || !oOption || *oFunction < 1 || *oFunction > 19)
            return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto oOptions = semath::decodeAggregateOptions(*oOption);
        if (!oOptions)
            return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        semath::AggregateScan aScan;
        auto consumeAggregateValue = [&](const api::CellValue& rValue) -> api::ValueResult<bool> {
            if (rValue.isError())
            {
                if (oOptions->mbIgnoreErrors)
                    return api::ValueResult<bool>::success(true);
                if (*oFunction == 2)
                    return api::ValueResult<bool>::success(true);
                if (*oFunction == 3)
                {
                    ++aScan.mnNonEmptyCount;
                    return api::ValueResult<bool>::success(true);
                }
                return api::ValueResult<bool>::failure(rValue.meError);
            }

            if (!rValue.isEmpty())
                ++aScan.mnNonEmptyCount;

            if (rValue.isNumber())
                aScan.maNumbers.push_back(rValue.mfNumber);
            return api::ValueResult<bool>::success(true);
        };

        auto scanAggregateArgument = [&](const formula::Node& rArgument) -> api::ValueResult<bool> {
            const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                        || rArgument.meKind == formula::NodeKind::RangeReference
                                        || rArgument.meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike ? evaluateReferenceNode(rArgument, rCurrentAddress)
                                                        : evaluateNode(rArgument, rCurrentAddress);
            if (!aArgument)
            {
                if (oOptions->mbIgnoreErrors && aArgument.maCyclePath.empty())
                    return api::ValueResult<bool>::success(true);
                if (*oFunction == 2)
                    return api::ValueResult<bool>::success(true);
                if (*oFunction == 3)
                {
                    ++aScan.mnNonEmptyCount;
                    return api::ValueResult<bool>::success(true);
                }
                return api::ValueResult<bool>::failure(aArgument.meError);
            }

            if (!aArgument.maValue.isMatrixReference())
                return consumeAggregateValue(aArgument.maValue.maValue);

            const auto& rReference = aArgument.maValue.maReference;
            const workbook::Sheet* pSheet = getSheet(rReference.maRange.maStart.mnSheet);
            if (!pSheet)
                return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    const api::CellAddress aAddress = rReference.addressAt(nCol, nRow);
                    const bool bFilteredRow = pSheet->isRowFiltered(aAddress.mnRow);
                    const bool bManuallyHiddenRow = pSheet->isRowHidden(aAddress.mnRow);
                    if (bFilteredRow || (oOptions->mbIgnoreHiddenRows && bManuallyHiddenRow))
                        continue;

                    const workbook::Cell* pReferencedCell = getCell(aAddress);
                    if (pReferencedCell && oOptions->mbIgnoreNestedAggregates
                        && cellContainsAggregateLike(*pReferencedCell))
                    {
                        continue;
                    }

                    EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                    {
                        if (oOptions->mbIgnoreErrors && aCell.maCyclePath.empty())
                            continue;
                        if (*oFunction == 2)
                            continue;
                        if (*oFunction == 3)
                        {
                            ++aScan.mnNonEmptyCount;
                            continue;
                        }
                        return api::ValueResult<bool>::failure(aCell.meError);
                    }

                    if (!aCell.maValue.isScalar())
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                    const auto aConsumed = consumeAggregateValue(aCell.maValue.maValue);
                    if (!aConsumed)
                        return aConsumed;
                }
            }
            return api::ValueResult<bool>::success(true);
        };

        const bool bRankedFunction = *oFunction >= 14;
        if (bRankedFunction)
        {
            if (rNode.maChildren.size() != 4)
                return detail::makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
            const auto aScanned = scanAggregateArgument(*rNode.maChildren[2]);
            if (!aScanned)
                return detail::makeScalarResult(api::CellValue::error(aScanned.meError));

            EvaluationResult aRank
                = detail::ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aRank)
            {
                return aRank.maCyclePath.empty()
                           ? detail::makeScalarResult(api::CellValue::error(aRank.meError))
                           : aRank;
            }
            const auto aRankNumber = detail::coerceToNumber(aRank.maValue.maValue);
            if (!aRankNumber)
                return detail::makeScalarResult(api::CellValue::error(aRankNumber.meError));

            const auto aAggregate
                = semath::evaluateAggregateRankedNumbers(*oFunction, aScan, aRankNumber.maValue);
            if (!aAggregate)
                return detail::makeScalarResult(api::CellValue::error(aAggregate.meError));
            return detail::makeScalarResult(api::CellValue::number(aAggregate.maValue));
        }

        for (std::size_t nIndex = 2; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aScanned = scanAggregateArgument(*rNode.maChildren[nIndex]);
            if (!aScanned)
                return detail::makeScalarResult(api::CellValue::error(aScanned.meError));
        }

        const auto aAggregate = semath::evaluateAggregateNumbers(*oFunction, aScan);
        if (!aAggregate)
            return detail::makeScalarResult(api::CellValue::error(aAggregate.meError));
        return detail::makeScalarResult(api::CellValue::number(aAggregate.maValue));
    }

    if (bRankedAggregate)
    {
        if (rNode.maChildren.size() != 2)
            return detail::makeFailure(api::Error::IllegalArgument);

        const auto aScan = collectAggregateScanFromArgument(*rNode.maChildren[0]);
        if (!aScan)
            return detail::makeFailure(aScan.meError);

        const auto aRank = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aRank)
            return detail::makeFailure(aRank.meError);

        sal_Int32 nAggregateFunction = 0;
        if (rFunctionName == u"LARGE")
            nAggregateFunction = 14;
        else if (rFunctionName == u"SMALL")
            nAggregateFunction = 15;
        else if (rFunctionName == u"PERCENTILE" || rFunctionName == u"PERCENTILE.INC"
                 || rFunctionName == u"COM.MICROSOFT.PERCENTILE.INC")
        {
            nAggregateFunction = 16;
        }
        else if (rFunctionName == u"QUARTILE" || rFunctionName == u"QUARTILE.INC"
                 || rFunctionName == u"COM.MICROSOFT.QUARTILE.INC")
        {
            nAggregateFunction = 17;
        }
        else if (rFunctionName == u"PERCENTILE.EXC"
                 || rFunctionName == u"COM.MICROSOFT.PERCENTILE.EXC")
        {
            nAggregateFunction = 18;
        }
        else
        {
            nAggregateFunction = 19;
        }

        const auto aAggregate
            = semath::evaluateAggregateRankedNumbers(nAggregateFunction, aScan.maValue, aRank.maValue);
        if (!aAggregate)
            return detail::makeFailure(aAggregate.meError);
        return detail::makeScalarResult(api::CellValue::number(aAggregate.maValue));
    }

    semath::AggregateScan aScan;
    for (const auto& pChild : rNode.maChildren)
    {
        const auto aCollected = collectAggregateScanFromArgument(*pChild);
        if (!aCollected)
            return detail::makeFailure(aCollected.meError);
        aScan.maNumbers.insert(aScan.maNumbers.end(), aCollected.maValue.maNumbers.begin(),
            aCollected.maValue.maNumbers.end());
    }

    const auto aSkew = semath::evaluateSkewNumbers(aScan.maNumbers, rFunctionName == u"SKEWP");
    if (!aSkew)
        return detail::makeFailure(aSkew.meError);
    return detail::makeScalarResult(api::CellValue::number(aSkew.maValue));
}

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
