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

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateLookupFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kLookupFunctions{
        api::StringView(u"VLOOKUP"),
        api::StringView(u"HLOOKUP"),
        api::StringView(u"LOOKUP"),
        api::StringView(u"MATCH"),
        api::StringView(u"XMATCH"),
        api::StringView(u"XLOOKUP"),
        api::StringView(u"CHOOSECOLS"),
        api::StringView(u"CHOOSEROWS"),
        api::StringView(u"ADDRESS"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kLookupFunctions))
        return std::nullopt;
    return evaluateLookupFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateLookupFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    FunctionEvalContext aContext { *this, rNode, rCurrentAddress };
    const auto oStoredReplayValue = tryGetStoredCellValue(rCurrentAddress);
    const auto replayStoredOrFailure = [&](api::Error eError) -> EvaluationResult {
        if (oStoredReplayValue)
            return makeScalarResult(*oStoredReplayValue);
        return makeFailure(eError);
    };

if (aFunctionName == u"VLOOKUP" || aFunctionName == u"HLOOKUP")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aLookupValue)
            return aLookupValue;

        EvaluationResult aTable = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        if (!aTable)
            return aTable;
        if (!aTable.maValue.isMatrixReference())
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aIndex
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aIndex)
            return aIndex;
        if (aIndex.maValue.maValue.isEmpty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aIndexNumber = coerceToNumber(aIndex.maValue.maValue);
        if (!aIndexNumber)
            return makeFailure(aIndexNumber.meError);
        const auto oWholeIndex = toWholeNumber(aIndexNumber.maValue);
        if (!oWholeIndex || *oWholeIndex <= 0)
            return makeFailure(api::Error::IllegalArgument);

        bool bApproximate = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aMode)
                return aMode;

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            bApproximate = !fp::approxEqual(aModeNumber.maValue, 0.0);
        }

        LookupInput aTableInput;
        aTableInput.mbScalar = false;
        aTableInput.maReference = aTable.maValue.maReference;
        const auto aDimensions = aTableInput.maReference.matrixDimensions();
        aTableInput.mnColumns = aDimensions.mnColumns;
        aTableInput.mnRows = aDimensions.mnRows;
        const auto eOrientation = aFunctionName == u"VLOOKUP"
                                      ? api::lookup::VectorOrientation::Column
                                      : api::lookup::VectorOrientation::Row;

        const api::MatrixSize nSearchLength
            = eOrientation == api::lookup::VectorOrientation::Column ? aDimensions.mnRows
                                                                     : aDimensions.mnColumns;
        const api::MatrixSize nResultIndex = *oWholeIndex - 1;
        if (nSearchLength <= 0)
            return makeFailure(api::Error::IllegalArgument);
        if (eOrientation == api::lookup::VectorOrientation::Column
            && nResultIndex >= aDimensions.mnColumns)
        {
            return makeFailure(api::Error::IllegalArgument);
        }
        if (eOrientation == api::lookup::VectorOrientation::Row && nResultIndex >= aDimensions.mnRows)
            return makeFailure(api::Error::IllegalArgument);

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);

        const EvaluatorLookupMaterializer aLookupMaterializer(*this);
        const auto aResolvedIndex = selookup::resolveTabularLookupIndex(aLookupMaterializer,
            rLookup, aTableInput, eOrientation, bApproximate,
            toQuerySearchType(mrWorkbook.meFormulaSearchType));
        if (!aResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const auto aMatchedSearchValue = selookup::materializeLookupInputValue(
            aLookupMaterializer, aTableInput, eOrientation, aResolvedIndex.maValue);
        if (!aMatchedSearchValue)
            return makeFailure(aMatchedSearchValue.meError);
        if (bApproximate && rLookup.isText()
            && (aMatchedSearchValue.maValue.isNumber()
                || aMatchedSearchValue.maValue.isBoolean()))
        {
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        const auto aResultCoordinate = api::lookup::planTabularLookupResult(
            eOrientation, aResolvedIndex.maValue, nResultIndex, aDimensions);
        if (!aResultCoordinate)
            return makeFailure(aResultCoordinate.meError);

        return materializeReferenceValue(
            aTableInput.maReference, aResultCoordinate.maValue.mnColumn,
            aResultCoordinate.maValue.mnRow);
    }

    if (aFunctionName == u"LOOKUP")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);
        if (rLookup.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const auto aDataInput = aContext.evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aDataInput)
            return replayStoredOrFailure(aDataInput.meError);
        const LookupInput& rDataInput = aDataInput.maValue;
        if (rDataInput.mbScalar && rDataInput.maScalar.isError())
            return makeScalarResult(api::CellValue::error(rDataInput.maScalar.meError));

        const auto aDataLayout = selookup::detectLookupLayout(rDataInput, true);
        if (!aDataLayout)
            return replayStoredOrFailure(aDataLayout.meError);
        const EvaluatorLookupMaterializer aLookupMaterializer(*this);

        std::optional<LookupInput> oResultInput;
        std::optional<api::lookup::VectorLayout> oResultLayout;
        if (rNode.maChildren.size() == 3)
        {
            const auto aResultInput = aContext.evaluateLookupInputNode(*rNode.maChildren[2]);
            if (!aResultInput)
                return replayStoredOrFailure(aResultInput.meError);
            oResultInput = aResultInput.maValue;
            if (oResultInput->mbScalar && oResultInput->maScalar.isError())
                return makeScalarResult(api::CellValue::error(oResultInput->maScalar.meError));

            const auto aResultLayout = selookup::detectLookupLayout(*oResultInput, false);
            if (!aResultLayout)
                return replayStoredOrFailure(aResultLayout.meError);
            oResultLayout = aResultLayout.maValue;
        }

        auto materializeResultAt = [&](api::MatrixSize nIndex) -> EvaluationResult {
            if (oResultInput)
            {
                if (oResultInput->mbScalar)
                {
                if (nIndex != 0)
                    return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
                return makeScalarResult(oResultInput->maScalar);
            }

                const auto aValue = selookup::materializeLookupInputValue(
                    aLookupMaterializer, *oResultInput, oResultLayout->meOrientation, nIndex);
                if (!aValue)
                    return replayStoredOrFailure(aValue.meError);
                return makeScalarResult(aValue.maValue);
            }

            if (rDataInput.mbScalar)
                return makeScalarResult(rDataInput.maScalar);

            const api::MatrixDimensions aDimensions { rDataInput.mnColumns, rDataInput.mnRows };
            const api::MatrixSize nResultIndex
                = aDataLayout.maValue.meOrientation == api::lookup::VectorOrientation::Column
                      ? aDimensions.mnColumns - 1
                      : aDimensions.mnRows - 1;
            const auto aResultCoordinate = api::lookup::planTabularLookupResult(
                aDataLayout.maValue.meOrientation, nIndex, nResultIndex, aDimensions);
            if (!aResultCoordinate)
                return replayStoredOrFailure(aResultCoordinate.meError);

            if (!rDataInput.maValues.empty())
            {
                const std::int64_t nLinearIndex
                    = static_cast<std::int64_t>(aResultCoordinate.maValue.mnRow) * rDataInput.mnColumns
                      + aResultCoordinate.maValue.mnColumn;
                if (nLinearIndex < 0
                    || static_cast<std::size_t>(nLinearIndex) >= rDataInput.maValues.size())
                {
                    return replayStoredOrFailure(api::Error::IllegalArgument);
                }

                return makeScalarResult(
                    rDataInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
            }

            return materializeReferenceValue(rDataInput.maReference,
                aResultCoordinate.maValue.mnColumn, aResultCoordinate.maValue.mnRow);
        };

        const auto aResolvedIndex = selookup::resolveLookupIndex(aLookupMaterializer, rLookup,
            rDataInput, toQuerySearchType(mrWorkbook.meFormulaSearchType));
        if (!aResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const auto aMatchedSearchValue = selookup::materializeLookupInputValue(
            aLookupMaterializer, rDataInput, aDataLayout.maValue.meOrientation,
            aResolvedIndex.maValue);
        if (!aMatchedSearchValue)
            return makeFailure(aMatchedSearchValue.meError);
        if (rLookup.isText() && (aMatchedSearchValue.maValue.isNumber()
                                 || aMatchedSearchValue.maValue.isBoolean()))
        {
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        return materializeResultAt(aResolvedIndex.maValue);
    }

    if (aFunctionName == u"MATCH")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return replayStoredOrFailure(rLookup.meError);

        EvaluationResult aSearchValue = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        const auto oSearchInput = makeLookupInput(aSearchValue);
        if (!oSearchInput)
            return replayStoredOrFailure(api::Error::IllegalArgument);

        api::lookup::MatchSearchMode aModes;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return replayStoredOrFailure(aMode.meError);
            if (aMode.maValue.maValue.isEmpty())
                return replayStoredOrFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return replayStoredOrFailure(aModeNumber.meError);

            const auto aNormalized = api::lookup::normalizeMatchType(aModeNumber.maValue);
            if (!aNormalized)
                return replayStoredOrFailure(aNormalized.meError);
            aModes = aNormalized.maValue;
        }
        else
        {
            aModes.meMatchMode = api::lookup::MatchMode::ExactOrNextSmaller;
            aModes.meSearchMode = api::lookup::SearchMode::BinaryAscending;
        }
        const EvaluatorLookupMaterializer aLookupMaterializer(*this);
        const auto aResolvedIndex = selookup::resolveMatchIndex(aLookupMaterializer, rLookup,
            *oSearchInput, aModes, toQuerySearchType(mrWorkbook.meFormulaSearchType));
        if (!aResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(aResolvedIndex.maValue + 1)));
    }

    if (aFunctionName == u"XMATCH")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);

        const auto aSearchInput = aContext.evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aSearchInput)
            return makeFailure(aSearchInput.meError);

        api::lookup::MatchMode eMatchMode = api::lookup::MatchMode::ExactOrNotAvailable;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeExtendedMatchMode(static_cast<std::int16_t>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eMatchMode = aNormalized.maValue;
        }

        api::lookup::SearchMode eSearchMode = api::lookup::SearchMode::Forward;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeSearchMode(static_cast<std::int16_t>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eSearchMode = aNormalized.maValue;
        }

        const EvaluatorLookupMaterializer aLookupMaterializer(*this);
        const auto aResolvedIndex = selookup::resolveExtendedMatchIndex(
            aLookupMaterializer, rLookup, aSearchInput.maValue, eMatchMode, eSearchMode,
            toQuerySearchType(mrWorkbook.meFormulaSearchType), true);
        if (!aResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(aResolvedIndex.maValue + 1)));
    }

    if (aFunctionName == u"XLOOKUP")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);
        if (rLookup.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        EvaluationResult aSearchValue = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        EvaluationResult aReturnValue = evaluateNode(*rNode.maChildren[2], rCurrentAddress);
        const auto oSearchInput = makeLookupInput(aSearchValue);
        const auto oReturnInput = makeLookupInput(aReturnValue);
        if (!oSearchInput || !oReturnInput)
            return makeFailure(api::Error::IllegalArgument);

        const api::MatrixDimensions aSearchDimensions { oSearchInput->mnColumns, oSearchInput->mnRows };
        const api::MatrixDimensions aReturnDimensions { oReturnInput->mnColumns, oReturnInput->mnRows };
        const auto aSearchLayout = selookup::detectLookupLayout(*oSearchInput, false);
        if (!aSearchLayout)
            return makeFailure(aSearchLayout.meError);
        if (aReturnDimensions.isEmpty())
            return makeFailure(api::Error::IllegalArgument);
        if (aSearchLayout.maValue.meOrientation == api::lookup::VectorOrientation::Column)
        {
            if (aReturnDimensions.mnRows != aSearchDimensions.mnRows)
                return makeFailure(api::Error::IllegalArgument);
        }
        else if (aReturnDimensions.mnColumns != aSearchDimensions.mnColumns)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        api::lookup::MatchMode eMatchMode = api::lookup::MatchMode::ExactOrNotAvailable;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[4], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized = api::lookup::normalizeExtendedMatchMode(
                static_cast<std::int16_t>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eMatchMode = aNormalized.maValue;
        }

        api::lookup::SearchMode eSearchMode = api::lookup::SearchMode::Forward;
        if (rNode.maChildren.size() >= 6
            && rNode.maChildren[5]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[5], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeSearchMode(static_cast<std::int16_t>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eSearchMode = aNormalized.maValue;
        }

        const EvaluatorLookupMaterializer aLookupMaterializer(*this);
        const auto aResolvedIndex = selookup::resolveExtendedMatchIndex(
            aLookupMaterializer, rLookup, *oSearchInput, eMatchMode, eSearchMode,
            toQuerySearchType(mrWorkbook.meFormulaSearchType), true);
        if (!aResolvedIndex)
        {
            if (rNode.maChildren.size() >= 4
                && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
            {
                return evaluateNode(*rNode.maChildren[3], rCurrentAddress);
            }
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        const auto aResultSlice = api::lookup::planXLookupResultSlice(
            aSearchLayout.maValue.meOrientation, aResolvedIndex.maValue, aReturnDimensions);
        if (!aResultSlice)
            return makeFailure(aResultSlice.meError);

        if (oReturnInput->mbScalar)
            return makeScalarResult(oReturnInput->maScalar);

        api::ResolvedReference aSliceReference;
        aSliceReference.maRange.maStart = oReturnInput->maReference.addressAt(
            aResultSlice.maValue.maStart.mnColumn, aResultSlice.maValue.maStart.mnRow);
        aSliceReference.maRange.maEnd = oReturnInput->maReference.addressAt(
            aResultSlice.maValue.maStart.mnColumn + aResultSlice.maValue.maDimensions.mnColumns - 1,
            aResultSlice.maValue.maStart.mnRow + aResultSlice.maValue.maDimensions.mnRows - 1);

        if (aSliceReference.isSingleCell())
            return materializeReferenceValue(aSliceReference, 0, 0);

        return makeReferenceResult(aSliceReference);
    }

    if (aFunctionName == u"CHOOSECOLS" || aFunctionName == u"CHOOSEROWS")
    {
        if (rNode.maChildren.size() < 2)
            return makeFailure(api::Error::IllegalArgument);

        const bool bChooseColumns = aFunctionName == u"CHOOSECOLS";
        EvaluationResult aSource = evaluateReferenceNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aSource)
            return aSource;
        if (!aSource.maValue.isMatrixReference())
            return makeFailure(api::Error::IllegalArgument);

        const auto aSourceDimensions = aSource.maValue.maReference.matrixDimensions();
        if (aSourceDimensions.mnColumns < 1 || aSourceDimensions.mnRows < 1)
            return makeFailure(api::Error::IllegalArgument);

        std::optional<api::MatrixSize> oFirstSelection;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            if (rNode.maChildren[nIndex]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);

            const auto aVisited = aContext.visitFlattenedValues( *rNode.maChildren[nIndex],
                [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);
                    if (rValue.isEmpty() || rValue.isText())
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber || !std::isfinite(aNumber.maValue)
                        || aNumber.maValue < static_cast<double>(std::numeric_limits<std::int32_t>::min())
                        || aNumber.maValue > static_cast<double>(std::numeric_limits<std::int32_t>::max()))
                    {
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
                    }

                    const std::int32_t nRequestedIndex = static_cast<std::int32_t>(aNumber.maValue);
                    const auto aSelection = api::array::normalizeSelectionIndex(
                        nRequestedIndex,
                        bChooseColumns ? aSourceDimensions.mnColumns : aSourceDimensions.mnRows);
                    if (!aSelection)
                        return api::ValueResult<bool>::failure(aSelection.meError);

                    if (!oFirstSelection)
                        oFirstSelection = aSelection.maValue;
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return makeFailure(aVisited.meError);
        }

        if (!oFirstSelection)
            return makeFailure(api::Error::IllegalArgument);

        return materializeReferenceValue(aSource.maValue.maReference,
            bChooseColumns ? *oFirstSelection : 0, bChooseColumns ? 0 : *oFirstSelection);
    }

    if (aFunctionName == u"ADDRESS")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRowNumber = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aRowNumber)
            return makeFailure(aRowNumber.meError);
        const auto aColumnNumber = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aColumnNumber)
            return makeFailure(aColumnNumber.meError);

        const auto oWholeRow = toWholeNumber(aRowNumber.maValue);
        const auto oWholeColumn = toWholeNumber(aColumnNumber.maValue);
        if (!oWholeRow || !oWholeColumn || *oWholeRow < 1 || *oWholeColumn < 1)
            return makeFailure(api::Error::IllegalArgument);

        std::int32_t nAbsMode = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aAbsMode = aContext.evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aAbsMode)
                return makeFailure(aAbsMode.meError);
            const auto oWholeAbsMode = toWholeNumber(aAbsMode.maValue);
            if (!oWholeAbsMode || *oWholeAbsMode < 1 || *oWholeAbsMode > 4)
                return makeFailure(api::Error::IllegalArgument);
            nAbsMode = *oWholeAbsMode;
        }

        bool bA1Style = true;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aA1Argument = aContext.evaluateScalarArgumentValue(*rNode.maChildren[3]);
            if (!aA1Argument)
                return makeFailure(aA1Argument.meError);
            if (!aA1Argument.maValue.isEmpty())
            {
                const auto aA1Bool = coerceToBoolean(aA1Argument.maValue);
                if (!aA1Bool)
                    return makeFailure(aA1Bool.meError);
                bA1Style = aA1Bool.maValue;
            }
        }

        api::String aSheetName;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aSheetArgument = aContext.evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aSheetArgument)
                return makeFailure(aSheetArgument.meError);
            const auto aSheetText = coerceToString(aSheetArgument.maValue);
            if (!aSheetText)
                return makeFailure(aSheetText.meError);
            aSheetName = aSheetText.maValue;
        }

        return makeScalarResult(api::CellValue::text(formatAddressFunctionResult(
            static_cast<api::RowIndex>(*oWholeRow - 1),
            static_cast<api::ColumnIndex>(*oWholeColumn - 1), nAbsMode, bA1Style, aSheetName)));
    }

        return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
