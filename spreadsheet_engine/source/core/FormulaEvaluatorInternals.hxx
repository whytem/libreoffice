/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/FormulaEvaluator.hxx>

#include <spreadsheetengine/api/Array.hxx>
#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/api/Text.hxx>
#include <spreadsheetengine/api/Workday.hxx>
#include <spreadsheetengine/runtime/ConversionRuntime.hxx>
#include <spreadsheetengine/runtime/DateTimeParse.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/FinancialRuntime.hxx>
#include <spreadsheetengine/runtime/FloatingPoint.hxx>
#include <spreadsheetengine/runtime/LookupRuntime.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <spreadsheetengine/runtime/MathFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/KahanSum.hxx>
#include <spreadsheetengine/runtime/ReferenceText.hxx>
#include <spreadsheetengine/runtime/TextCase.hxx>
#include <spreadsheetengine/runtime/TextFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/TextRuntimeSupport.hxx>
#include <spreadsheetengine/runtime/TextScalar.hxx>

#include "DateAlgorithms.hxx"
#include "FormulaEvaluatorUtils.hxx"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <optional>
#include <vector>

namespace spreadsheetengine::core::eval
{

namespace seconvert = spreadsheetengine::core::convert;
namespace sedate = spreadsheetengine::core::detail::date;
namespace sedatetime = spreadsheetengine::core::datetime;
namespace sefinance = spreadsheetengine::core::finance;
namespace semath = spreadsheetengine::core::math;
namespace selookup = spreadsheetengine::core::lookup;
namespace sequery = spreadsheetengine::core::query;
namespace setext = spreadsheetengine::core::text;
namespace seutil = spreadsheetengine::core::util;

using detail::coerceToBoolean;
using detail::coerceToNumber;
using detail::coerceToString;
using detail::ensureScalarValue;
using detail::formatNumber;
using detail::formatQuotedString;
using detail::hasFunctionPrefix;
using detail::makeFailure;
using detail::makeReferenceResult;
using detail::makeScalarResult;
using detail::normalizeDisplayFunctionName;
using detail::normalizeFunctionName;
using detail::normalizeNonNegativeLengthArgument;
using detail::normalizeOneBasedStringPositionArgument;
using detail::normalizeStringPositionArgument;
using detail::parseAsciiDouble;
using detail::toWholeNumber;
using detail::uppercaseAscii;

using AggregateOptions = semath::AggregateOptions;
using AggregateScan = semath::AggregateScan;
using CriteriaAggregateInput = sequery::CriteriaAggregateInput;
using CriteriaAggregateKind = sequery::CriteriaAggregateKind;
using CriteriaPredicate = sequery::CriteriaPredicate;
using LookupInput = selookup::LookupInput;
using spreadsheetengine::runtime::referencetext::formatAddressFunctionResult;

[[nodiscard]] api::query::SearchType toQuerySearchType(workbook::FormulaSearchType eSearchType);

[[nodiscard]] bool usesMicrosoftCompatibilityName(api::StringView rName);

[[nodiscard]] std::optional<std::int16_t> classifyOdfErrorTypeLiteral(api::StringView rText);

[[nodiscard]] std::optional<std::int16_t> classifyOdfErrorType(api::Error eError);

[[nodiscard]] std::optional<std::int32_t> classifyLegacyErrorTypeLiteral(api::StringView rText);

[[nodiscard]] std::optional<std::int32_t> classifyLegacyErrorType(api::Error eError);

[[nodiscard]] bool isLiteralArrayWeekendNode(const formula::Node& rNode);

[[nodiscard]] api::String formatBasisDateTime(double fSerialValue);

[[nodiscard]] std::optional<LookupInput> makeLookupInput(const EvaluationResult& rResult);

[[nodiscard]] LookupInput makeLookupScalarError(api::Error eError);

[[nodiscard]] std::optional<CriteriaAggregateInput> makeCriteriaAggregateInput(
    const EvaluationResult& rResult);

struct Evaluator::FunctionEvalContext
{
    Evaluator& mrEvaluator;
    const formula::Node& mrNode;
    const api::CellAddress& mrCurrentAddress;

    struct MaterializedMatrixInput
    {
        api::MatrixDimensions maDimensions { 1, 1 };
        std::vector<api::CellValue> maValues;
        std::optional<api::ResolvedReference> moReference;

        [[nodiscard]] const api::CellValue& valueAt(
            api::MatrixSize nColumn, api::MatrixSize nRow) const
        {
            const std::size_t nLinearIndex
                = static_cast<std::size_t>(nRow) * maDimensions.mnColumns
                  + static_cast<std::size_t>(nColumn);
            return maValues[nLinearIndex];
        }
    };

    template <typename Visitor>
    [[nodiscard]] api::ValueResult<bool> visitFlattenedValues(
        const formula::Node& rArgument, const Visitor& rVisitor) const
    {
        if (rArgument.meKind == formula::NodeKind::ArrayConstant
            || rArgument.meKind == formula::NodeKind::ReferenceList)
        {
            for (const auto& pChild : rArgument.maChildren)
            {
                const auto aChild = visitFlattenedValues(*pChild, rVisitor);
                if (!aChild)
                    return aChild;
            }
            return api::ValueResult<bool>::success(true);
        }

        const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                    || rArgument.meKind == formula::NodeKind::RangeReference
                                    || rArgument.meKind == formula::NodeKind::NamedReference;

        EvaluationResult aValue = bReferenceLike
                                      ? mrEvaluator.evaluateReferenceNode(rArgument, mrCurrentAddress)
                                      : mrEvaluator.evaluateNode(rArgument, mrCurrentAddress);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);

        if (aValue.maValue.isMatrixReference())
        {
            const auto& rReference = aValue.maValue.maReference;
            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    EvaluationResult aCell
                        = mrEvaluator.materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                        return api::ValueResult<bool>::failure(aCell.meError);

                    const auto aVisited = rVisitor(aCell.maValue.maValue, true);
                    if (!aVisited)
                        return aVisited;
                }
            }

            return api::ValueResult<bool>::success(true);
        }

        return rVisitor(aValue.maValue.maValue, bReferenceLike);
    }

    [[nodiscard]] api::ValueResult<std::vector<double>> collectNumericArguments(
        bool bIgnoreTextAndEmptyFromReferences, bool bTreatScalarEmptyAsZero = false) const
    {
        std::vector<double> aNumbers;
        for (const auto& pChild : mrNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                *pChild,
                [&](const api::CellValue& rValue,
                    bool bFromReference) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);

                    if (bFromReference && bIgnoreTextAndEmptyFromReferences
                        && (rValue.isEmpty() || rValue.isText()))
                    {
                        return api::ValueResult<bool>::success(true);
                    }

                    if (rValue.isEmpty())
                    {
                        if (bTreatScalarEmptyAsZero)
                            aNumbers.push_back(0.0);
                        return api::ValueResult<bool>::success(true);
                    }

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                    {
                        if (bFromReference && bIgnoreTextAndEmptyFromReferences
                            && rValue.isText())
                        {
                            return api::ValueResult<bool>::success(true);
                        }

                        return api::ValueResult<bool>::failure(aNumber.meError);
                    }

                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(std::move(aNumbers));
    }

    [[nodiscard]] api::ValueResult<MaterializedMatrixInput> materializeMatrixInput(
        const formula::Node& rArgument) const
    {
        MaterializedMatrixInput aInput;

        if (rArgument.meKind == formula::NodeKind::ArrayConstant)
        {
            if (rArgument.mnArrayColumns < 1 || rArgument.mnArrayRows < 1
                || static_cast<sal_Int32>(rArgument.maChildren.size())
                       != rArgument.mnArrayColumns * rArgument.mnArrayRows)
            {
                return api::ValueResult<MaterializedMatrixInput>::failure(
                    api::Error::IllegalArgument);
            }

            aInput.maDimensions = { rArgument.mnArrayColumns, rArgument.mnArrayRows };
            aInput.maValues.reserve(rArgument.maChildren.size());
            for (const auto& pChild : rArgument.maChildren)
            {
                const auto aValue = evaluateScalarArgumentValue(*pChild);
                if (!aValue)
                    return api::ValueResult<MaterializedMatrixInput>::failure(aValue.meError);
                aInput.maValues.push_back(aValue.maValue);
            }
            return api::ValueResult<MaterializedMatrixInput>::success(std::move(aInput));
        }

        const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                    || rArgument.meKind == formula::NodeKind::RangeReference
                                    || rArgument.meKind == formula::NodeKind::NamedReference;
        EvaluationResult aValue
            = bReferenceLike ? mrEvaluator.evaluateReferenceNode(rArgument, mrCurrentAddress)
                             : mrEvaluator.evaluateNode(rArgument, mrCurrentAddress);
        if (!aValue)
            return api::ValueResult<MaterializedMatrixInput>::failure(aValue.meError);

        if (aValue.maValue.isScalar())
        {
            aInput.maValues.push_back(aValue.maValue.maValue);
            return api::ValueResult<MaterializedMatrixInput>::success(std::move(aInput));
        }

        aInput.moReference = aValue.maValue.maReference;
        aInput.maDimensions = aValue.maValue.maReference.matrixDimensions();
        if (aInput.maDimensions.mnColumns < 1 || aInput.maDimensions.mnRows < 1)
            return api::ValueResult<MaterializedMatrixInput>::failure(api::Error::IllegalArgument);

        aInput.maValues.reserve(
            static_cast<std::size_t>(aInput.maDimensions.mnColumns) * aInput.maDimensions.mnRows);
        for (api::MatrixSize nRow = 0; nRow < aInput.maDimensions.mnRows; ++nRow)
        {
            for (api::MatrixSize nColumn = 0; nColumn < aInput.maDimensions.mnColumns; ++nColumn)
            {
                EvaluationResult aCell = mrEvaluator.materializeReferenceValue(
                    *aInput.moReference, nColumn, nRow);
                if (!aCell)
                    return api::ValueResult<MaterializedMatrixInput>::failure(aCell.meError);
                if (!aCell.maValue.isScalar())
                {
                    return api::ValueResult<MaterializedMatrixInput>::failure(
                        api::Error::IllegalArgument);
                }
                aInput.maValues.push_back(aCell.maValue.maValue);
            }
        }

        return api::ValueResult<MaterializedMatrixInput>::success(std::move(aInput));
    }

    [[nodiscard]] api::ValueResult<std::vector<double>> collectVarianceArguments(
        bool bTextAsZero) const
    {
        std::vector<double> aNumbers;
        for (const auto& pChild : mrNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                *pChild,
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

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                        return api::ValueResult<bool>::failure(aNumber.meError);
                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(std::move(aNumbers));
    }

    [[nodiscard]] api::ValueResult<api::CellValue> evaluateScalarArgumentValue(
        const formula::Node& rArgument) const
    {
        EvaluationResult aValue
            = ensureScalarValue(mrEvaluator, mrEvaluator.evaluateNode(rArgument, mrCurrentAddress));
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        return api::ValueResult<api::CellValue>::success(aValue.maValue.maValue);
    }

    [[nodiscard]] api::ValueResult<api::CellValue> evaluateAnchoredScalarArgumentValue(
        const formula::Node& rArgument) const
    {
        EvaluationResult aValue = mrEvaluator.evaluateNode(rArgument, mrCurrentAddress);
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        if (aValue.maValue.isMatrixReference())
            aValue = mrEvaluator.materializeReferenceValue(aValue.maValue.maReference, 0, 0);
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        if (!aValue.maValue.isScalar())
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(aValue.maValue.maValue);
    }

    [[nodiscard]] api::ValueResult<double> evaluateNumericArgument(
        const formula::Node& rArgument, std::optional<double> oDefaultForEmpty) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<double>::failure(aNumber.meError);
        return aNumber;
    }

    [[nodiscard]] api::ValueResult<double> evaluateAnchoredNumericArgument(
        const formula::Node& rArgument, std::optional<double> oDefaultForEmpty) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aValue = evaluateAnchoredScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);
        if (aValue.maValue.isEmpty() && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<double>::failure(aNumber.meError);
        return aNumber;
    }

    [[nodiscard]] api::ValueResult<double> evaluateRequiredNumberArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        return coerceToNumber(aValue.maValue);
    }

    [[nodiscard]] api::ValueResult<double> evaluateRequiredAnchoredNumberArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateAnchoredScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        return coerceToNumber(aValue.maValue);
    }

    [[nodiscard]] api::ValueResult<api::DateSerial> evaluateRequiredDateArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<api::DateSerial>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<api::DateSerial>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<api::DateSerial>::failure(api::Error::IllegalArgument);

        const auto oDateSerial = sedatetime::coerceToDateSerial(aValue.maValue);
        if (!oDateSerial)
            return api::ValueResult<api::DateSerial>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::DateSerial>::success(*oDateSerial);
    }

    [[nodiscard]] api::ValueResult<std::int32_t> evaluateOptionalWholeNumberArgument(
        const formula::Node& rArgument, std::int32_t nDefaultValue) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::int32_t>::success(nDefaultValue);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<std::int32_t>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<std::int32_t>::success(nDefaultValue);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<std::int32_t>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole)
            return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);
        return api::ValueResult<std::int32_t>::success(*oWhole);
    }

    [[nodiscard]] api::ValueResult<std::int32_t> evaluateRequiredWholeNumberArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<std::int32_t>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<std::int32_t>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole)
            return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);
        return api::ValueResult<std::int32_t>::success(*oWhole);
    }

    [[nodiscard]] EvaluationResult makeNumericOrErrorResult(api::ValueResult<double> aValue) const
    {
        if (!aValue)
            return makeScalarResult(api::CellValue::error(aValue.meError));
        return makeScalarResult(api::CellValue::number(aValue.maValue));
    }

    [[nodiscard]] api::ValueResult<AggregateScan> collectAggregateScanFromArgument(
        const formula::Node& rArgument) const
    {
        AggregateScan aScan;
        const auto aVisited = visitFlattenedValues(
            rArgument, [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isNumber())
                    aScan.maNumbers.push_back(rValue.mfNumber);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<AggregateScan>::failure(aVisited.meError);
        return api::ValueResult<AggregateScan>::success(std::move(aScan));
    }

    [[nodiscard]] api::ValueResult<std::vector<double>> collectExtremaArguments() const
    {
        std::vector<double> aNumbers;
        for (const auto& pChild : mrNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                *pChild,
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

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                        return api::ValueResult<bool>::failure(aNumber.meError);
                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(std::move(aNumbers));
    }

    [[nodiscard]] api::ValueResult<LookupInput> evaluateLookupInputNode(
        const formula::Node& rLookupNode) const
    {
        auto evaluateMatrixOperand = [&](const formula::Node& rOperand)
            -> api::ValueResult<LookupInput> {
            EvaluationResult aValue = mrEvaluator.evaluateNode(rOperand, mrCurrentAddress);
            if (!aValue)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(aValue.meError));

            if (aValue.maValue.isScalar())
            {
                const auto aNumber = coerceToNumber(aValue.maValue.maValue);
                if (!aNumber)
                    return api::ValueResult<LookupInput>::success(
                        makeLookupScalarError(aNumber.meError));

                LookupInput aInput;
                aInput.maScalar = api::CellValue::number(aNumber.maValue);
                return api::ValueResult<LookupInput>::success(aInput);
            }

            LookupInput aInput;
            aInput.mbScalar = false;
            const auto aDimensions = aValue.maValue.maReference.matrixDimensions();
            aInput.mnColumns = aDimensions.mnColumns;
            aInput.mnRows = aDimensions.mnRows;
            aInput.maValues.reserve(
                static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnColumns, 0))
                * static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnRows, 0)));
            for (api::MatrixSize nRow = 0; nRow < aInput.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aInput.mnColumns; ++nCol)
                {
                    EvaluationResult aElement
                        = mrEvaluator.materializeReferenceValue(aValue.maValue.maReference, nCol,
                            nRow);
                    if (!aElement)
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(aElement.meError));
                    }
                    if (!aElement.maValue.isScalar())
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(api::Error::IllegalArgument));
                    }

                    const auto aNumber = coerceToNumber(aElement.maValue.maValue);
                    if (!aNumber)
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(aNumber.meError));
                    }
                    aInput.maValues.push_back(api::CellValue::number(aNumber.maValue));
                }
            }
            return api::ValueResult<LookupInput>::success(aInput);
        };

        if (rLookupNode.meKind == formula::NodeKind::FunctionCall
            && normalizeFunctionName(rLookupNode.maPrimaryText) == u"ISNUMBER")
        {
            if (rLookupNode.maChildren.size() != 1)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));

            EvaluationResult aValue
                = mrEvaluator.evaluateNode(*rLookupNode.maChildren[0], mrCurrentAddress);
            if (!aValue)
            {
                LookupInput aScalar;
                aScalar.maScalar = api::CellValue::boolean(false);
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            if (aValue.maValue.isScalar())
            {
                LookupInput aScalar;
                aScalar.maScalar = api::CellValue::boolean(aValue.maValue.maValue.isNumber());
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            LookupInput aInput;
            aInput.mbScalar = false;
            const auto aDimensions = aValue.maValue.maReference.matrixDimensions();
            aInput.mnColumns = aDimensions.mnColumns;
            aInput.mnRows = aDimensions.mnRows;
            aInput.maValues.reserve(
                static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnColumns, 0))
                * static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnRows, 0)));
            for (api::MatrixSize nRow = 0; nRow < aInput.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aInput.mnColumns; ++nCol)
                {
                    EvaluationResult aElement = mrEvaluator.materializeReferenceValue(
                        aValue.maValue.maReference, nCol, nRow);
                    const bool bIsNumber = aElement && aElement.maValue.isScalar()
                                           && aElement.maValue.maValue.isNumber();
                    aInput.maValues.push_back(api::CellValue::boolean(bIsNumber));
                }
            }

            if (aInput.mnColumns == 1 && aInput.mnRows == 1 && !aInput.maValues.empty())
            {
                LookupInput aScalar;
                aScalar.maScalar = aInput.maValues.front();
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            return api::ValueResult<LookupInput>::success(aInput);
        }

        if (rLookupNode.meKind == formula::NodeKind::ArrayConstant)
        {
            if (rLookupNode.mnArrayColumns < 1 || rLookupNode.mnArrayRows < 1
                || static_cast<sal_Int32>(rLookupNode.maChildren.size())
                       != rLookupNode.mnArrayColumns * rLookupNode.mnArrayRows)
            {
                return api::ValueResult<LookupInput>::failure(api::Error::IllegalArgument);
            }

            LookupInput aInput;
            aInput.mbScalar = false;
            aInput.mnColumns = rLookupNode.mnArrayColumns;
            aInput.mnRows = rLookupNode.mnArrayRows;
            aInput.maValues.reserve(rLookupNode.maChildren.size());
            for (const auto& pChild : rLookupNode.maChildren)
            {
                const auto aValue = evaluateScalarArgumentValue(*pChild);
                if (!aValue)
                    return api::ValueResult<LookupInput>::failure(aValue.meError);
                aInput.maValues.push_back(aValue.maValue);
            }

            if (aInput.mnColumns == 1 && aInput.mnRows == 1 && !aInput.maValues.empty())
            {
                LookupInput aScalar;
                aScalar.maScalar = aInput.maValues.front();
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            return api::ValueResult<LookupInput>::success(aInput);
        }

        if (rLookupNode.meKind == formula::NodeKind::FunctionCall
            && rLookupNode.maPrimaryText == u"MMULT")
        {
            if (rLookupNode.maChildren.size() != 2)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));

            const auto aLeft = evaluateMatrixOperand(*rLookupNode.maChildren[0]);
            const auto aRight = evaluateMatrixOperand(*rLookupNode.maChildren[1]);
            if (!aLeft)
                return aLeft;
            if (!aRight)
                return aRight;
            if (aLeft.maValue.mbScalar && aLeft.maValue.maScalar.isError())
                return api::ValueResult<LookupInput>::success(aLeft.maValue);
            if (aRight.maValue.mbScalar && aRight.maValue.maScalar.isError())
                return api::ValueResult<LookupInput>::success(aRight.maValue);

            const api::MatrixSize nLeftColumns
                = aLeft.maValue.mbScalar ? 1 : aLeft.maValue.mnColumns;
            const api::MatrixSize nLeftRows = aLeft.maValue.mbScalar ? 1 : aLeft.maValue.mnRows;
            const api::MatrixSize nRightColumns
                = aRight.maValue.mbScalar ? 1 : aRight.maValue.mnColumns;
            const api::MatrixSize nRightRows = aRight.maValue.mbScalar ? 1 : aRight.maValue.mnRows;
            if (nLeftColumns != nRightRows)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));

            auto getValueAt = [](const LookupInput& rInput, api::MatrixSize nColumn,
                                 api::MatrixSize nRow) -> double {
                if (rInput.mbScalar)
                    return rInput.maScalar.mfNumber;
                const std::size_t nIndex
                    = static_cast<std::size_t>(nRow * rInput.mnColumns + nColumn);
                return rInput.maValues[nIndex].mfNumber;
            };

            LookupInput aResult;
            aResult.mbScalar = false;
            aResult.mnColumns = nRightColumns;
            aResult.mnRows = nLeftRows;
            aResult.maValues.reserve(static_cast<std::size_t>(aResult.mnColumns)
                                     * static_cast<std::size_t>(aResult.mnRows));
            for (api::MatrixSize nRow = 0; nRow < aResult.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aResult.mnColumns; ++nCol)
                {
                    double fSum = 0.0;
                    for (api::MatrixSize nIndex = 0; nIndex < nLeftColumns; ++nIndex)
                    {
                        fSum += getValueAt(aLeft.maValue, nIndex, nRow)
                                * getValueAt(aRight.maValue, nCol, nIndex);
                    }
                    aResult.maValues.push_back(api::CellValue::number(fSum));
                }
            }

            if (aResult.mnColumns == 1 && aResult.mnRows == 1)
            {
                LookupInput aScalar;
                aScalar.maScalar = aResult.maValues.front();
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            return api::ValueResult<LookupInput>::success(aResult);
        }

        EvaluationResult aValue = mrEvaluator.evaluateNode(rLookupNode, mrCurrentAddress);
        if (!aValue)
            return api::ValueResult<LookupInput>::failure(aValue.meError);

        const auto oInput = makeLookupInput(aValue);
        if (!oInput)
            return api::ValueResult<LookupInput>::failure(api::Error::IllegalArgument);
        return api::ValueResult<LookupInput>::success(*oInput);
    }

    [[nodiscard]] api::ValueResult<bool> evaluatePayTypeArgument(
        const formula::Node& rArgument, bool bDefaultValue) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<bool>::success(bDefaultValue);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<bool>::success(bDefaultValue);

        return coerceToBoolean(aValue.maValue);
    }

    [[nodiscard]] api::ValueResult<bool> evaluateStrictPaymentTypeArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<bool>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole || (*oWhole != 0 && *oWhole != 1))
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
        return api::ValueResult<bool>::success(*oWhole != 0);
    }

    [[nodiscard]] EvaluationResult makeFiniteNumberResult(double fValue) const
    {
        if (!std::isfinite(fValue))
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(fValue));
    }

    [[nodiscard]] api::ValueResult<api::WeekendMask> evaluateWeekendMaskArgument(
        const formula::Node* pArgument, bool bWorkdayFunction, bool bAllowSequence = false) const
    {
        if (!pArgument || pArgument->meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<api::WeekendMask>::success(api::workday::defaultWeekendMask());

        if (bAllowSequence)
        {
            std::vector<double> aWeekendSequence;
            bool bSequenceCompatible = true;
            const auto aVisited = visitFlattenedValues(
                *pArgument, [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);
                    if (rValue.isEmpty())
                        return api::ValueResult<bool>::success(true);

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                    {
                        bSequenceCompatible = false;
                        return api::ValueResult<bool>::success(true);
                    }

                    aWeekendSequence.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<api::WeekendMask>::failure(aVisited.meError);

            if (bSequenceCompatible && aWeekendSequence.size() == 7)
                return api::workday::weekendMaskFromSequence(aWeekendSequence);

            if (bSequenceCompatible && aWeekendSequence.size() > 1)
                return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
        }

        EvaluationResult aWeekendValue;
        if (pArgument->meKind == formula::NodeKind::ArrayConstant)
        {
            if (pArgument->maChildren.empty())
                return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            for (const auto& pChild : pArgument->maChildren)
            {
                if (!isLiteralArrayWeekendNode(*pChild))
                    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            }

            aWeekendValue = ensureScalarValue(
                mrEvaluator, mrEvaluator.evaluateNode(*pArgument->maChildren.front(), mrCurrentAddress));
        }
        else
        {
            aWeekendValue = mrEvaluator.evaluateNode(*pArgument, mrCurrentAddress);
            if (aWeekendValue && aWeekendValue.maValue.isMatrixReference())
            {
                if (!aWeekendValue.maValue.maReference.isSingleCell())
                    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
                aWeekendValue
                    = mrEvaluator.materializeReferenceValue(aWeekendValue.maValue.maReference, 0, 0);
            }
        }

        if (!aWeekendValue)
            return api::ValueResult<api::WeekendMask>::failure(aWeekendValue.meError);
        if (!aWeekendValue.maValue.isScalar())
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);

        const api::CellValue& rValue = aWeekendValue.maValue.maValue;
        if (rValue.isError())
            return api::ValueResult<api::WeekendMask>::failure(rValue.meError);
        if (rValue.isEmpty())
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);

        if (rValue.isText())
        {
            if (rValue.maString.size() != 7)
                return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            const auto aMask
                = api::workday::weekendMaskFromMsSpec(rValue.maString, bWorkdayFunction);
            if (!aMask)
                return api::ValueResult<api::WeekendMask>::failure(aMask.meError);
            return aMask;
        }

        const auto aNumber = coerceToNumber(rValue);
        if (!aNumber)
            return api::ValueResult<api::WeekendMask>::failure(aNumber.meError);
        const auto oWholeNumber = toWholeNumber(aNumber.maValue);
        if (!oWholeNumber)
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
        if ((*oWholeNumber < 1 || *oWholeNumber > 7)
            && (*oWholeNumber < 11 || *oWholeNumber > 17))
        {
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
        }

        const auto aMask = api::workday::weekendMaskFromMsSpec(
            formatNumber(static_cast<double>(*oWholeNumber)), bWorkdayFunction);
        if (!aMask)
            return api::ValueResult<api::WeekendMask>::failure(aMask.meError);
        return aMask;
    }

    [[nodiscard]] api::ValueResult<std::vector<api::DateSerial>> collectHolidaySerials(
        const formula::Node* pArgument) const
    {
        std::vector<api::DateSerial> aHolidays;
        if (!pArgument || pArgument->meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::vector<api::DateSerial>>::success(aHolidays);

        const auto aVisited = visitFlattenedValues(
            *pArgument,
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isEmpty())
                    return api::ValueResult<bool>::success(true);

                const auto oDateSerial = sedatetime::coerceToDateSerial(rValue);
                if (!oDateSerial)
                    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                aHolidays.push_back(*oDateSerial);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<std::vector<api::DateSerial>>::failure(aVisited.meError);

        std::sort(aHolidays.begin(), aHolidays.end());
        aHolidays.erase(std::unique(aHolidays.begin(), aHolidays.end()), aHolidays.end());
        return api::ValueResult<std::vector<api::DateSerial>>::success(std::move(aHolidays));
    }
};

class EvaluatorCriteriaAggregateMaterializer final
    : public sequery::CriteriaAggregateMaterializer
{
    Evaluator& mrEvaluator;

public:
    explicit EvaluatorCriteriaAggregateMaterializer(Evaluator& rEvaluator)
        : mrEvaluator(rEvaluator)
    {
    }

    [[nodiscard]] api::ValueResult<api::CellValue> materialize(
        const CriteriaAggregateInput& rInput, api::MatrixCoordinate aCoordinate) const override
    {
        if (rInput.mbScalar)
        {
            if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(rInput.maScalar);
        }

        if (!rInput.maValues.empty())
        {
            const std::int64_t nLinearIndex
                = static_cast<std::int64_t>(aCoordinate.mnRow) * rInput.mnColumns
                  + aCoordinate.mnColumn;
            if (nLinearIndex < 0
                || static_cast<std::size_t>(nLinearIndex) >= rInput.maValues.size())
            {
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            }
            return api::ValueResult<api::CellValue>::success(
                rInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
        }

        EvaluationResult aResult = mrEvaluator.materializeReferenceValue(
            rInput.maReference, aCoordinate.mnColumn, aCoordinate.mnRow);
        if (!aResult)
            return api::ValueResult<api::CellValue>::failure(aResult.meError);
        if (!aResult.maValue.isScalar())
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(aResult.maValue.maValue);
    }
};

class EvaluatorLookupMaterializer final : public selookup::LookupMaterializer
{
    Evaluator& mrEvaluator;

public:
    explicit EvaluatorLookupMaterializer(Evaluator& rEvaluator)
        : mrEvaluator(rEvaluator)
    {
    }

    [[nodiscard]] api::ValueResult<api::CellValue> materialize(
        const LookupInput& rInput, api::MatrixCoordinate aCoordinate) const override
    {
        if (rInput.mbScalar)
        {
            if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(rInput.maScalar);
        }

        if (!rInput.maValues.empty())
        {
            const std::int64_t nLinearIndex
                = static_cast<std::int64_t>(aCoordinate.mnRow) * rInput.mnColumns
                  + aCoordinate.mnColumn;
            if (nLinearIndex < 0
                || static_cast<std::size_t>(nLinearIndex) >= rInput.maValues.size())
            {
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            }
            return api::ValueResult<api::CellValue>::success(
                rInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
        }

        EvaluationResult aResult = mrEvaluator.materializeReferenceValue(
            rInput.maReference, aCoordinate.mnColumn, aCoordinate.mnRow);
        if (!aResult)
            return api::ValueResult<api::CellValue>::failure(aResult.meError);
        if (!aResult.maValue.isScalar())
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(aResult.maValue.maValue);
    }
};

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
