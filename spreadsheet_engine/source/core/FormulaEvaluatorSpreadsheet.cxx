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
#include <numeric>
#include <optional>
#include <vector>

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

[[nodiscard]] bool containsReplayVolatileFunction(const formula::Node& rNode)
{
    if (rNode.meKind == formula::NodeKind::FunctionCall)
    {
        const api::String aFunctionName = normalizeFunctionName(rNode.maPrimaryText);
        if (aFunctionName == u"TODAY" || aFunctionName == u"NOW")
            return true;
    }

    for (const auto& pChild : rNode.maChildren)
    {
        if (containsReplayVolatileFunction(*pChild))
            return true;
    }
    return false;
}

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateSpreadsheetFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kSpreadsheetFunctions{
        api::StringView(u"TODAY"),
        api::StringView(u"NOW"),
        api::StringView(u"SEQUENCE"),
        api::StringView(u"TAKE"),
        api::StringView(u"DROP"),
        api::StringView(u"EXPAND"),
        api::StringView(u"SORT"),
        api::StringView(u"SORTBY"),
        api::StringView(u"TEXTSPLIT"),
        api::StringView(u"INDEX"),
        api::StringView(u"GETPIVOTDATA"),
        api::StringView(u"UNIQUE"),
        api::StringView(u"HSTACK"),
        api::StringView(u"VSTACK"),
        api::StringView(u"WRAPCOLS"),
        api::StringView(u"WRAPROWS"),
        api::StringView(u"FILTER"),
        api::StringView(u"TOROW"),
        api::StringView(u"TOCOL"),
        api::StringView(u"RANDARRAY"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kSpreadsheetFunctions))
        return std::nullopt;
    return evaluateSpreadsheetFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateSpreadsheetFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    FunctionEvalContext aContext { *this, rNode, rCurrentAddress };
    const auto replayStoredOrFailure = [&](api::Error eError) -> EvaluationResult {
        if (canUseStoredReplayValue())
        {
            if (const auto oStoredValue = tryGetStoredCellValue(rCurrentAddress))
                return makeScalarResult(*oStoredValue);
        }
        return makeFailure(eError);
    };
    const auto findWorkbookVolatileSnapshot
        = [&](api::StringView rVolatileFunction) -> std::optional<api::CellValue> {
        api::String aExactFormula(u"of:=");
        aExactFormula += api::String(rVolatileFunction);
        aExactFormula.push_back(u'(');
        aExactFormula.push_back(u')');

        for (sal_Int32 nSheet = 0;
             nSheet < static_cast<sal_Int32>(mrWorkbook.maSheets.size()); ++nSheet)
        {
            const auto& rSheet = mrWorkbook.maSheets[static_cast<std::size_t>(nSheet)];
            for (const auto& [rKey, rCell] : rSheet.maCells)
            {
                if (rCell.maFormula != aExactFormula)
                    continue;
                if (auto oStored = tryGetStoredCellValue({ nSheet, rKey.first, rKey.second }))
                    return oStored;
            }
        }
        return std::nullopt;
    };

    if (aFunctionName == u"GETPIVOTDATA" || aFunctionName == u"UNIQUE" || aFunctionName == u"HSTACK"
        || aFunctionName == u"VSTACK" || aFunctionName == u"WRAPCOLS"
        || aFunctionName == u"WRAPROWS" || aFunctionName == u"FILTER"
        || aFunctionName == u"TOROW" || aFunctionName == u"TOCOL"
        || aFunctionName == u"RANDARRAY")
    {
        return replayStoredOrFailure(api::Error::IllegalArgument);
    }

    struct MaterializedMatrixInput
    {
        api::MatrixDimensions maDimensions;
        std::vector<api::CellValue> maValues;
        std::optional<api::ResolvedReference> moReference;

        [[nodiscard]] const api::CellValue& valueAt(
            api::MatrixSize nColumn, api::MatrixSize nRow) const
        {
            const std::size_t nLinearIndex = static_cast<std::size_t>(nRow) * maDimensions.mnColumns
                                             + static_cast<std::size_t>(nColumn);
            return maValues[nLinearIndex];
        }
    };

    const auto materializeMatrixInput
        = [&](const formula::Node& rArgument) -> api::ValueResult<MaterializedMatrixInput> {
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
                const auto aValue = aContext.evaluateScalarArgumentValue(*pChild);
                if (!aValue)
                    return api::ValueResult<MaterializedMatrixInput>::failure(aValue.meError);
                aInput.maValues.push_back(aValue.maValue);
            }
            return api::ValueResult<MaterializedMatrixInput>::success(std::move(aInput));
        }

        const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                    || rArgument.meKind == formula::NodeKind::RangeReference
                                    || rArgument.meKind == formula::NodeKind::NamedReference;
        EvaluationResult aValue = bReferenceLike ? evaluateReferenceNode(rArgument, rCurrentAddress)
                                                 : evaluateNode(rArgument, rCurrentAddress);
        if (!aValue)
            return api::ValueResult<MaterializedMatrixInput>::failure(aValue.meError);

        if (aValue.maValue.isScalar())
        {
            aInput.maDimensions = { 1, 1 };
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
                EvaluationResult aCell = materializeReferenceValue(
                    *aInput.moReference, nColumn, nRow);
                if (!aCell)
                    return api::ValueResult<MaterializedMatrixInput>::failure(aCell.meError);
                if (!aCell.maValue.isScalar())
                    return api::ValueResult<MaterializedMatrixInput>::failure(
                        api::Error::IllegalArgument);
                aInput.maValues.push_back(aCell.maValue.maValue);
            }
        }

        return api::ValueResult<MaterializedMatrixInput>::success(std::move(aInput));
    };

    const auto evaluateOptionalSignedWholeArgument
        = [&](const formula::Node& rArgument) -> api::ValueResult<std::optional<sal_Int32>> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::optional<sal_Int32>>::success(std::nullopt);

        const auto aValue = aContext.evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<std::optional<sal_Int32>>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<std::optional<sal_Int32>>::success(std::nullopt);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<std::optional<sal_Int32>>::failure(aNumber.meError);
        const auto oWholeNumber = toWholeNumber(aNumber.maValue);
        if (!oWholeNumber)
            return api::ValueResult<std::optional<sal_Int32>>::failure(
                api::Error::IllegalArgument);
        return api::ValueResult<std::optional<sal_Int32>>::success(
            static_cast<sal_Int32>(*oWholeNumber));
    };

    const auto evaluateOptionalNonNegativeWholeArgument
        = [&](const formula::Node& rArgument) -> api::ValueResult<std::optional<sal_Int32>> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::optional<sal_Int32>>::success(std::nullopt);

        const auto aValue = aContext.evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<std::optional<sal_Int32>>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<std::optional<sal_Int32>>::success(std::nullopt);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<std::optional<sal_Int32>>::failure(aNumber.meError);
        const auto oWholeNumber = toWholeNumber(aNumber.maValue);
        if (!oWholeNumber || *oWholeNumber < 0)
            return api::ValueResult<std::optional<sal_Int32>>::failure(
                api::Error::IllegalArgument);
        return api::ValueResult<std::optional<sal_Int32>>::success(
            static_cast<sal_Int32>(*oWholeNumber));
    };

    const auto collectTextVector
        = [&](const formula::Node& rArgument) -> api::ValueResult<std::vector<api::String>> {
        std::vector<api::String> aValues;
        const auto aVisited = aContext.visitFlattenedValues(
            rArgument, [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                const auto aText = coerceToString(rValue);
                if (!aText)
                    return api::ValueResult<bool>::failure(aText.meError);
                aValues.push_back(aText.maValue);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<std::vector<api::String>>::failure(aVisited.meError);
        return api::ValueResult<std::vector<api::String>>::success(std::move(aValues));
    };

    const auto compareSortValues = [&](const api::CellValue& rLeft,
                                       const api::CellValue& rRight) -> api::ValueResult<int> {
        if (rLeft.isError() && rRight.isError())
            return api::ValueResult<int>::success(
                static_cast<int>(rLeft.meError) - static_cast<int>(rRight.meError));
        if (rLeft.isError())
            return api::ValueResult<int>::success(1);
        if (rRight.isError())
            return api::ValueResult<int>::success(-1);

        if (rLeft.isEmpty() && rRight.isEmpty())
            return api::ValueResult<int>::success(0);
        if (rLeft.isEmpty())
            return api::ValueResult<int>::success(1);
        if (rRight.isEmpty())
            return api::ValueResult<int>::success(-1);

        const bool bLeftText = rLeft.isText();
        const bool bRightText = rRight.isText();
        if (bLeftText && bRightText)
            return api::ValueResult<int>::success(
                sequery::compareFoldedText(rLeft.maString, rRight.maString));
        if (bLeftText)
            return api::ValueResult<int>::success(1);
        if (bRightText)
            return api::ValueResult<int>::success(-1);

        const auto aLeftNumber = coerceToNumber(rLeft);
        const auto aRightNumber = coerceToNumber(rRight);
        if (!aLeftNumber || !aRightNumber)
            return api::ValueResult<int>::failure(api::Error::IllegalArgument);
        if (fp::approxEqual(aLeftNumber.maValue, aRightNumber.maValue))
            return api::ValueResult<int>::success(0);
        return api::ValueResult<int>::success(
            aLeftNumber.maValue < aRightNumber.maValue ? -1 : 1);
    };

    const auto splitText = [&](const api::String& rText,
                               const std::vector<api::String>& rDelimiters, bool bIgnoreEmpty,
                               bool bMatchMode) {
        std::vector<api::String> aParts;
        if (rDelimiters.empty() || rText.empty())
        {
            aParts.push_back(rText);
            return aParts;
        }

        const api::String aSearchText
            = bMatchMode ? api::text::lowercase(setext::defaultCaseMappingService(), rText)
                         : rText;
        std::size_t nStart = 0;
        while (nStart < rText.size())
        {
            std::size_t nBestIndex = rText.size();
            std::size_t nBestLength = 0;
            for (const auto& rDelimiter : rDelimiters)
            {
                if (rDelimiter.empty())
                    continue;
                const api::String aSearchDelimiter = bMatchMode
                                                         ? api::text::lowercase(
                                                               setext::defaultCaseMappingService(),
                                                               rDelimiter)
                                                         : rDelimiter;
                const std::size_t nIndex = aSearchText.find(aSearchDelimiter, nStart);
                if (nIndex != api::String::npos && nIndex < nBestIndex)
                {
                    nBestIndex = nIndex;
                    nBestLength = rDelimiter.size();
                }
            }

            const std::size_t nSliceEnd
                = nBestIndex == api::String::npos ? rText.size() : nBestIndex;
            api::String aPart = rText.substr(nStart, nSliceEnd - nStart);
            if (!bIgnoreEmpty || !aPart.empty())
                aParts.push_back(std::move(aPart));

            if (nBestIndex == api::String::npos || nBestIndex >= rText.size())
                break;
            nStart = nBestIndex + nBestLength;
        }

        return aParts;
    };

    if (aFunctionName == u"TODAY" || aFunctionName == u"NOW")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        if (auto oStored = findWorkbookVolatileSnapshot(aFunctionName))
            return makeScalarResult(*oStored);
        return makeFailure(api::Error::IllegalArgument);
    }

    if (aFunctionName == u"SEQUENCE")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);
        if (containsReplayVolatileFunction(rNode))
        {
            // Promoted replay needs the workbook snapshot for volatile starts like TODAY()/NOW()
            // until the evaluator grows an explicit replay-time clock.
            if (const auto oStoredValue = tryGetStoredCellValue(rCurrentAddress))
                return makeScalarResult(*oStoredValue);
            return makeFailure(api::Error::IllegalArgument);
        }

        const auto aRowsValue
            = aContext.evaluateAnchoredNumericArgument(*rNode.maChildren[0], 1.0);
        if (!aRowsValue)
            return makeFailure(aRowsValue.meError);
        const auto oRows = toWholeNumber(aRowsValue.maValue);
        if (!oRows || *oRows < 1)
            return makeFailure(api::Error::IllegalArgument);

        double fColumns = 1.0;
        if (rNode.maChildren.size() >= 2)
        {
            const auto aColumnsValue
                = aContext.evaluateAnchoredNumericArgument(*rNode.maChildren[1], 1.0);
            if (!aColumnsValue)
                return makeFailure(aColumnsValue.meError);
            fColumns = aColumnsValue.maValue;
        }
        const auto oColumns = toWholeNumber(fColumns);
        if (!oColumns || *oColumns < 1)
            return makeFailure(api::Error::IllegalArgument);

        double fStart = 1.0;
        if (rNode.maChildren.size() >= 3)
        {
            const auto aStartValue
                = aContext.evaluateAnchoredNumericArgument(*rNode.maChildren[2], 1.0);
            if (!aStartValue)
                return makeFailure(aStartValue.meError);
            fStart = aStartValue.maValue;
        }

        return makeScalarResult(api::CellValue::number(fStart));
    }

    if (aFunctionName == u"TAKE" || aFunctionName == u"DROP")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aSource = materializeMatrixInput(*rNode.maChildren[0]);
        if (!aSource)
            return makeFailure(aSource.meError);
        const auto oRows = rNode.maChildren.size() >= 2
                               ? evaluateOptionalSignedWholeArgument(*rNode.maChildren[1])
                               : api::ValueResult<std::optional<sal_Int32>>::success(std::nullopt);
        if (!oRows)
            return makeFailure(oRows.meError);
        const auto oColumns = rNode.maChildren.size() >= 3
                                  ? evaluateOptionalSignedWholeArgument(*rNode.maChildren[2])
                                  : api::ValueResult<std::optional<sal_Int32>>::success(std::nullopt);
        if (!oColumns)
            return makeFailure(oColumns.meError);

        const auto aSlice = api::array::planTakeDropSlice(
            aSource.maValue.maDimensions, aFunctionName == u"TAKE", oRows.maValue, oColumns.maValue);
        if (!aSlice)
            return makeScalarResult(api::CellValue::error(aSlice.meError));

        if (aSource.maValue.moReference)
        {
            api::ResolvedReference aSliceReference;
            aSliceReference.maRange.maStart = aSource.maValue.moReference->addressAt(
                aSlice.maValue.maStart.mnColumn, aSlice.maValue.maStart.mnRow);
            aSliceReference.maRange.maEnd = aSource.maValue.moReference->addressAt(
                aSlice.maValue.maStart.mnColumn + aSlice.maValue.maDimensions.mnColumns - 1,
                aSlice.maValue.maStart.mnRow + aSlice.maValue.maDimensions.mnRows - 1);
            if (aSliceReference.isSingleCell())
                return materializeReferenceValue(aSliceReference, 0, 0);
            return makeReferenceResult(aSliceReference);
        }

        return makeScalarResult(aSource.maValue.valueAt(
            aSlice.maValue.maStart.mnColumn, aSlice.maValue.maStart.mnRow));
    }

    if (aFunctionName == u"INDEX")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aSource = materializeMatrixInput(*rNode.maChildren[0]);
        if (!aSource)
            return makeFailure(aSource.meError);

        const auto oIndex1 = evaluateOptionalNonNegativeWholeArgument(*rNode.maChildren[1]);
        if (!oIndex1)
            return makeFailure(oIndex1.meError);

        std::optional<sal_Int32> oRowNumber;
        std::optional<sal_Int32> oColumnNumber;

        if (rNode.maChildren.size() == 2)
        {
            if (!oIndex1.maValue)
                return makeFailure(api::Error::IllegalArgument);

            if (aSource.maValue.maDimensions.mnRows == 1 && aSource.maValue.maDimensions.mnColumns > 1)
            {
                oRowNumber = 1;
                oColumnNumber = oIndex1.maValue;
            }
            else if (aSource.maValue.maDimensions.mnColumns == 1)
            {
                oRowNumber = oIndex1.maValue;
                oColumnNumber = 1;
            }
            else
            {
                oRowNumber = oIndex1.maValue;
                oColumnNumber = 0;
            }
        }
        else
        {
            oRowNumber = oIndex1.maValue;
            const auto oIndex2 = evaluateOptionalNonNegativeWholeArgument(*rNode.maChildren[2]);
            if (!oIndex2)
                return makeFailure(oIndex2.meError);
            oColumnNumber = oIndex2.maValue;
        }

        if (!oRowNumber)
            oRowNumber = 0;
        if (!oColumnNumber)
            oColumnNumber = 0;
        if (*oRowNumber == 0 && *oColumnNumber == 0)
            return makeFailure(api::Error::IllegalArgument);

        const api::MatrixDimensions aSourceDimensions = aSource.maValue.maDimensions;
        const api::MatrixSize nStartRow = *oRowNumber == 0 ? 0 : static_cast<api::MatrixSize>(*oRowNumber - 1);
        const api::MatrixSize nStartColumn
            = *oColumnNumber == 0 ? 0 : static_cast<api::MatrixSize>(*oColumnNumber - 1);
        const api::MatrixDimensions aSliceDimensions{
            *oColumnNumber == 0 ? aSourceDimensions.mnColumns : 1,
            *oRowNumber == 0 ? aSourceDimensions.mnRows : 1
        };
        if (nStartRow >= aSourceDimensions.mnRows || nStartColumn >= aSourceDimensions.mnColumns
            || nStartRow + aSliceDimensions.mnRows > aSourceDimensions.mnRows
            || nStartColumn + aSliceDimensions.mnColumns > aSourceDimensions.mnColumns)
        {
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        if (aSource.maValue.moReference)
        {
            api::ResolvedReference aSliceReference;
            aSliceReference.maRange.maStart
                = aSource.maValue.moReference->addressAt(nStartColumn, nStartRow);
            aSliceReference.maRange.maEnd = aSource.maValue.moReference->addressAt(
                nStartColumn + aSliceDimensions.mnColumns - 1,
                nStartRow + aSliceDimensions.mnRows - 1);
            if (aSliceReference.isSingleCell())
                return materializeReferenceValue(aSliceReference, 0, 0);
            return makeReferenceResult(aSliceReference);
        }

        return makeScalarResult(aSource.maValue.valueAt(nStartColumn, nStartRow));
    }

    if (aFunctionName == u"EXPAND")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aSource = materializeMatrixInput(*rNode.maChildren[0]);
        if (!aSource)
            return makeFailure(aSource.meError);
        const auto oRows = evaluateOptionalSignedWholeArgument(*rNode.maChildren[1]);
        if (!oRows)
            return makeFailure(oRows.meError);
        const auto oColumns = rNode.maChildren.size() >= 3
                                  ? evaluateOptionalSignedWholeArgument(*rNode.maChildren[2])
                                  : api::ValueResult<std::optional<sal_Int32>>::success(std::nullopt);
        if (!oColumns)
            return makeFailure(oColumns.meError);
        if (rNode.maChildren.size() == 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aPadValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[3]);
            if (!aPadValue)
                return makeFailure(aPadValue.meError);
        }

        const auto aExpanded = api::array::planExpandDimensions(
            aSource.maValue.maDimensions, oRows.maValue, oColumns.maValue);
        if (!aExpanded)
            return makeScalarResult(api::CellValue::error(aExpanded.meError));

        if (aSource.maValue.moReference && aExpanded.maValue == aSource.maValue.maDimensions)
            return makeReferenceResult(*aSource.maValue.moReference);

        return makeScalarResult(aSource.maValue.valueAt(0, 0));
    }

    if (aFunctionName == u"SORT")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aSource = materializeMatrixInput(*rNode.maChildren[0]);
        if (!aSource)
            return makeFailure(aSource.meError);

        std::int32_t nSortIndex = 1;
        if (rNode.maChildren.size() >= 2)
        {
            const auto aIndexValue
                = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[1], 1);
            if (!aIndexValue)
                return makeFailure(aIndexValue.meError);
            nSortIndex = aIndexValue.maValue;
            if (nSortIndex < 1)
                return makeFailure(api::Error::IllegalArgument);
        }

        std::int32_t nSortOrder = 1;
        if (rNode.maChildren.size() >= 3)
        {
            const auto aOrderValue
                = aContext.evaluateOptionalWholeNumberArgument(*rNode.maChildren[2], 1);
            if (!aOrderValue)
                return makeFailure(aOrderValue.meError);
            nSortOrder = aOrderValue.maValue;
            if (nSortOrder != 1 && nSortOrder != -1)
                return makeFailure(api::Error::IllegalArgument);
        }

        bool bByColumn = false;
        if (rNode.maChildren.size() == 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aByColumnValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[3]);
            if (!aByColumnValue)
                return makeFailure(aByColumnValue.meError);
            const auto aBoolean = coerceToBoolean(aByColumnValue.maValue);
            if (!aBoolean)
                return makeFailure(aBoolean.meError);
            bByColumn = aBoolean.maValue;
        }

        const bool bSortRows = !bByColumn;
        const api::MatrixSize nAxisLength
            = bSortRows ? aSource.maValue.maDimensions.mnRows : aSource.maValue.maDimensions.mnColumns;
        const api::MatrixSize nKeyLimit
            = bSortRows ? aSource.maValue.maDimensions.mnColumns : aSource.maValue.maDimensions.mnRows;
        if (nSortIndex > nKeyLimit)
            return makeFailure(api::Error::IllegalArgument);
        if (nAxisLength <= 1)
            return makeScalarResult(aSource.maValue.valueAt(0, 0));

        std::vector<api::MatrixSize> aOrder(static_cast<std::size_t>(nAxisLength));
        std::iota(aOrder.begin(), aOrder.end(), 0);
        std::stable_sort(aOrder.begin(), aOrder.end(),
            [&](api::MatrixSize nLeft, api::MatrixSize nRight) {
                const api::CellValue& rLeftValue = bSortRows
                                                       ? aSource.maValue.valueAt(nSortIndex - 1, nLeft)
                                                       : aSource.maValue.valueAt(nLeft, nSortIndex - 1);
                const api::CellValue& rRightValue = bSortRows
                                                        ? aSource.maValue.valueAt(nSortIndex - 1, nRight)
                                                        : aSource.maValue.valueAt(nRight, nSortIndex - 1);
                const auto aCompare = compareSortValues(rLeftValue, rRightValue);
                if (!aCompare || aCompare.maValue == 0)
                    return nLeft < nRight;
                return nSortOrder > 0 ? aCompare.maValue < 0 : aCompare.maValue > 0;
            });

        return makeScalarResult(
            bSortRows ? aSource.maValue.valueAt(0, aOrder.front())
                      : aSource.maValue.valueAt(aOrder.front(), 0));
    }

    if (aFunctionName == u"SORTBY")
    {
        if (rNode.maChildren.size() < 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aSource = materializeMatrixInput(*rNode.maChildren[0]);
        if (!aSource)
            return makeFailure(aSource.meError);

        struct SortKeyInput
        {
            MaterializedMatrixInput maMatrix;
            std::int32_t mnOrder = 1;
        };

        std::vector<SortKeyInput> aKeys;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size();)
        {
            const auto aKeyMatrix = materializeMatrixInput(*rNode.maChildren[nIndex]);
            if (!aKeyMatrix)
                return makeFailure(aKeyMatrix.meError);
            ++nIndex;

            std::int32_t nSortOrder = 1;
            if (nIndex < rNode.maChildren.size())
            {
                if (rNode.maChildren[nIndex]->meKind != formula::NodeKind::EmptyArgument)
                {
                    const auto aOrderValue
                        = aContext.evaluateRequiredWholeNumberArgument(*rNode.maChildren[nIndex]);
                    if (!aOrderValue)
                        return makeFailure(aOrderValue.meError);
                    nSortOrder = aOrderValue.maValue;
                    if (nSortOrder != 1 && nSortOrder != -1)
                        return makeFailure(api::Error::IllegalArgument);
                }
                ++nIndex;
            }

            aKeys.push_back({ aKeyMatrix.maValue, nSortOrder });
        }

        if (aKeys.empty())
            return makeFailure(api::Error::IllegalArgument);

        const MaterializedMatrixInput& rPrimaryKey = aKeys.front().maMatrix;
        const bool bSortRows = rPrimaryKey.maDimensions.mnColumns == 1 && rPrimaryKey.maDimensions.mnRows > 1;
        const bool bSortColumns
            = rPrimaryKey.maDimensions.mnRows == 1 && rPrimaryKey.maDimensions.mnColumns > 1;
        if (!bSortRows && !bSortColumns)
        {
            if (rPrimaryKey.maDimensions == api::MatrixDimensions { 1, 1 })
                return makeScalarResult(aSource.maValue.valueAt(0, 0));
            return makeFailure(api::Error::IllegalArgument);
        }

        const api::MatrixSize nAxisLength = bSortRows ? rPrimaryKey.maDimensions.mnRows
                                                      : rPrimaryKey.maDimensions.mnColumns;
        for (const auto& rKey : aKeys)
        {
            if (bSortRows)
            {
                if (rKey.maMatrix.maDimensions.mnColumns != 1
                    || rKey.maMatrix.maDimensions.mnRows != nAxisLength)
                {
                    return makeFailure(api::Error::IllegalArgument);
                }
            }
            else if (rKey.maMatrix.maDimensions.mnRows != 1
                     || rKey.maMatrix.maDimensions.mnColumns != nAxisLength)
            {
                return makeFailure(api::Error::IllegalArgument);
            }
        }

        if ((bSortRows && aSource.maValue.maDimensions.mnRows != nAxisLength)
            || (bSortColumns && aSource.maValue.maDimensions.mnColumns != nAxisLength))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        std::vector<api::MatrixSize> aOrder(static_cast<std::size_t>(nAxisLength));
        std::iota(aOrder.begin(), aOrder.end(), 0);
        std::stable_sort(aOrder.begin(), aOrder.end(),
            [&](api::MatrixSize nLeft, api::MatrixSize nRight) {
                for (const auto& rKey : aKeys)
                {
                    const api::CellValue& rLeftValue = bSortRows
                                                           ? rKey.maMatrix.valueAt(0, nLeft)
                                                           : rKey.maMatrix.valueAt(nLeft, 0);
                    const api::CellValue& rRightValue = bSortRows
                                                            ? rKey.maMatrix.valueAt(0, nRight)
                                                            : rKey.maMatrix.valueAt(nRight, 0);
                    const auto aCompare = compareSortValues(rLeftValue, rRightValue);
                    if (!aCompare || aCompare.maValue == 0)
                        continue;
                    return rKey.mnOrder > 0 ? aCompare.maValue < 0 : aCompare.maValue > 0;
                }
                return nLeft < nRight;
            });

        return makeScalarResult(
            bSortRows ? aSource.maValue.valueAt(0, aOrder.front())
                      : aSource.maValue.valueAt(aOrder.front(), 0));
    }

    if (aFunctionName == u"TEXTSPLIT")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTextValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTextValue)
            return makeFailure(aTextValue.meError);
        const auto aText = coerceToString(aTextValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        if (aText.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        std::vector<api::String> aColumnDelimiters;
        if (rNode.maChildren.size() >= 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aDelimiters = collectTextVector(*rNode.maChildren[1]);
            if (!aDelimiters)
                return makeFailure(aDelimiters.meError);
            aColumnDelimiters = aDelimiters.maValue;
        }

        std::vector<api::String> aRowDelimiters;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aDelimiters = collectTextVector(*rNode.maChildren[2]);
            if (!aDelimiters)
                return makeFailure(aDelimiters.meError);
            aRowDelimiters = aDelimiters.maValue;
        }

        bool bIgnoreEmpty = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aIgnoreValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[3]);
            if (!aIgnoreValue)
                return makeFailure(aIgnoreValue.meError);
            const auto aBoolean = coerceToBoolean(aIgnoreValue.maValue);
            if (!aBoolean)
                return makeFailure(aBoolean.meError);
            bIgnoreEmpty = aBoolean.maValue;
        }

        bool bMatchMode = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[4]);
            if (!aMatchValue)
                return makeFailure(aMatchValue.meError);
            const auto aBoolean = coerceToBoolean(aMatchValue.maValue);
            if (!aBoolean)
                return makeFailure(aBoolean.meError);
            bMatchMode = aBoolean.maValue;
        }

        std::optional<api::String> oPadWith;
        if (rNode.maChildren.size() == 6
            && rNode.maChildren[5]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aPadValue = aContext.evaluateAnchoredScalarArgumentValue(*rNode.maChildren[5]);
            if (!aPadValue)
                return makeFailure(aPadValue.meError);
            const auto aPadText = coerceToString(aPadValue.maValue);
            if (!aPadText)
                return makeFailure(aPadText.meError);
            oPadWith = aPadText.maValue;
        }

        const auto aRows = splitText(aText.maValue, aRowDelimiters, bIgnoreEmpty, bMatchMode);
        if (aRows.empty())
        {
            if (oPadWith)
                return makeScalarResult(api::CellValue::text(*oPadWith));
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        const auto aColumns = splitText(aRows.front(), aColumnDelimiters, bIgnoreEmpty, bMatchMode);
        if (aColumns.empty())
        {
            if (oPadWith)
                return makeScalarResult(api::CellValue::text(*oPadWith));
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        if (aColumns.front().empty())
            return makeScalarResult(api::CellValue::empty());
        return makeScalarResult(api::CellValue::text(aColumns.front()));
    }

    return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
