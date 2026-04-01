/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <address.hxx>
#include <document.hxx>
#include <interpretercontext.hxx>
#include <scmatrix.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/runtime/LookupRuntime.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::compat::libreoffice::lookupexecution
{

using LookupInput = spreadsheetengine::core::lookup::LookupInput;

struct LookupInputSource
{
    std::optional<ScRange> moRange;
    ScMatrixRef mpMatrix;
};

using LookupVectorSource = LookupInputSource;

struct MatchExecutionRequest
{
    spreadsheetengine::api::CellValue maLookupValue
        = spreadsheetengine::api::CellValue::empty();
    LookupVectorSource maSearchSource;
    spreadsheetengine::api::lookup::MatchSearchMode maLegacyModes;
    spreadsheetengine::api::lookup::MatchMode meMatchMode
        = spreadsheetengine::api::lookup::MatchMode::ExactOrNotAvailable;
    spreadsheetengine::api::lookup::SearchMode meSearchMode
        = spreadsheetengine::api::lookup::SearchMode::Forward;
    spreadsheetengine::api::query::SearchType meSearchType
        = spreadsheetengine::api::query::SearchType::Normal;
    bool mbExtended = false;
    bool mbAllowPatternMatch = false;
};

struct LookupExecutionResult
{
    enum class Kind : std::uint8_t
    {
        Scalar,
        Reference,
        Matrix
    };

    Kind meKind = Kind::Scalar;
    spreadsheetengine::api::CellValue maScalar = spreadsheetengine::api::CellValue::empty();
    ScRange maRange;
    ScMatrixRef mpMatrix;

    [[nodiscard]] bool isSingleCellReference() const
    {
        return meKind == Kind::Reference && maRange.aStart == maRange.aEnd;
    }
};

struct LegacyLookupRequest
{
    spreadsheetengine::api::CellValue maLookupValue
        = spreadsheetengine::api::CellValue::empty();
    LookupInput maDataInput;
    std::optional<LookupInput> moResultInput;
    spreadsheetengine::api::query::SearchType meSearchType
        = spreadsheetengine::api::query::SearchType::Normal;
};

struct TabularLookupRequest
{
    spreadsheetengine::api::CellValue maLookupValue
        = spreadsheetengine::api::CellValue::empty();
    LookupInput maTableInput;
    spreadsheetengine::api::lookup::VectorOrientation meSearchOrientation
        = spreadsheetengine::api::lookup::VectorOrientation::Column;
    spreadsheetengine::api::MatrixSize mnResultIndex = 0;
    bool mbApproximate = true;
    spreadsheetengine::api::query::SearchType meSearchType
        = spreadsheetengine::api::query::SearchType::Normal;
};

struct XLookupExecutionRequest
{
    spreadsheetengine::api::CellValue maLookupValue
        = spreadsheetengine::api::CellValue::empty();
    LookupInput maSearchInput;
    LookupInput maResultInput;
    spreadsheetengine::api::lookup::MatchMode meMatchMode
        = spreadsheetengine::api::lookup::MatchMode::ExactOrNotAvailable;
    spreadsheetengine::api::lookup::SearchMode meSearchMode
        = spreadsheetengine::api::lookup::SearchMode::Forward;
    spreadsheetengine::api::query::SearchType meSearchType
        = spreadsheetengine::api::query::SearchType::Normal;
    bool mbAllowPatternMatch = false;
};

namespace detail
{

using spreadsheetengine::core::lookup::LookupMaterializer;

[[nodiscard]] inline spreadsheetengine::api::CellValue toApiCellValue(const ScMatrixValue& rValue)
{
    if (ScMatrix::IsBooleanType(rValue.nType))
        return spreadsheetengine::api::CellValue::boolean(rValue.GetBoolean());

    if (ScMatrix::IsValueType(rValue.nType))
    {
        if (const FormulaError eError = rValue.GetError(); eError != FormulaError::NONE)
            return spreadsheetengine::api::CellValue::error(toApiError(eError));
        return spreadsheetengine::api::CellValue::number(rValue.fVal);
    }

    if (ScMatrix::IsRealStringType(rValue.nType))
        return spreadsheetengine::api::CellValue::text(toApiString(rValue.GetString().getString()));

    return spreadsheetengine::api::CellValue::empty();
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<LookupInput> buildLookupInput(
    const LookupInputSource& rSource)
{
    LookupInput aInput;
    aInput.mbScalar = false;

    if (rSource.moRange)
    {
        aInput.maReference.maRange = toApiCellRange(*rSource.moRange);
        aInput.mnColumns = rSource.moRange->aEnd.Col() - rSource.moRange->aStart.Col() + 1;
        aInput.mnRows = rSource.moRange->aEnd.Row() - rSource.moRange->aStart.Row() + 1;
        return spreadsheetengine::api::ValueResult<LookupInput>::success(aInput);
    }

    if (!rSource.mpMatrix)
        return spreadsheetengine::api::ValueResult<LookupInput>::failure(
            spreadsheetengine::api::Error::IllegalArgument);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rSource.mpMatrix->GetDimensions(nColumns, nRows);
    if (nColumns == 0 || nRows == 0)
        return spreadsheetengine::api::ValueResult<LookupInput>::failure(
            spreadsheetengine::api::Error::IllegalArgument);

    aInput.mnColumns = static_cast<spreadsheetengine::api::MatrixSize>(nColumns);
    aInput.mnRows = static_cast<spreadsheetengine::api::MatrixSize>(nRows);
    aInput.maValues.reserve(nColumns * nRows);
    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            aInput.maValues.push_back(toApiCellValue(rSource.mpMatrix->Get(nColumn, nRow)));
    }
    return spreadsheetengine::api::ValueResult<LookupInput>::success(std::move(aInput));
}

[[nodiscard]] inline LookupInput buildLookupScalarInput(
    const spreadsheetengine::api::CellValue& rValue)
{
    LookupInput aInput;
    aInput.maScalar = rValue;
    return aInput;
}

class CalcLookupMaterializer final : public LookupMaterializer
{
    DocumentEvaluationHost maHost;

public:
    CalcLookupMaterializer(const ScDocument& rDoc, ScInterpreterContext& rContext)
        : maHost(rDoc, rContext)
    {
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue> materialize(
        const LookupInput& rInput,
        spreadsheetengine::api::MatrixCoordinate aCoordinate) const override
    {
        if (rInput.mbScalar)
        {
            if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::failure(
                    spreadsheetengine::api::Error::IllegalArgument);
            }
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(rInput.maScalar);
        }

        if (!rInput.maValues.empty())
        {
            const std::int64_t nLinearIndex
                = static_cast<std::int64_t>(aCoordinate.mnRow) * rInput.mnColumns
                  + aCoordinate.mnColumn;
            if (nLinearIndex < 0
                || static_cast<std::size_t>(nLinearIndex) >= rInput.maValues.size())
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::failure(
                    spreadsheetengine::api::Error::IllegalArgument);
            }
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(
                rInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
        }

        return maHost.getRangeValue(
            rInput.maReference.maRange, aCoordinate.mnColumn, aCoordinate.mnRow);
    }
};

[[nodiscard]] inline LookupExecutionResult makeScalarResult(
    const spreadsheetengine::api::CellValue& rValue)
{
    LookupExecutionResult aResult;
    aResult.maScalar = rValue;
    return aResult;
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<LookupExecutionResult>
makeCoordinateResult(const CalcLookupMaterializer& rMaterializer, const LookupInput& rInput,
    spreadsheetengine::api::MatrixCoordinate aCoordinate)
{
    if (rInput.mbScalar)
    {
        if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
        {
            return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
        }
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::success(
            makeScalarResult(rInput.maScalar));
    }

    const spreadsheetengine::api::MatrixDimensions aDimensions { rInput.mnColumns, rInput.mnRows };
    if (!spreadsheetengine::api::isValidCoordinate(aDimensions, aCoordinate))
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            spreadsheetengine::api::Error::IllegalArgument);
    }

    if (!rInput.maValues.empty())
    {
        const auto aValue = rMaterializer.materialize(rInput, aCoordinate);
        if (!aValue)
        {
            return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
                aValue.meError);
        }
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::success(
            makeScalarResult(aValue.maValue));
    }

    LookupExecutionResult aResult;
    aResult.meKind = LookupExecutionResult::Kind::Reference;
    aResult.maRange = ScRange(
        rInput.maReference.maRange.maStart.mnColumn + aCoordinate.mnColumn,
        rInput.maReference.maRange.maStart.mnRow + aCoordinate.mnRow,
        rInput.maReference.maRange.maStart.mnSheet,
        rInput.maReference.maRange.maStart.mnColumn + aCoordinate.mnColumn,
        rInput.maReference.maRange.maStart.mnRow + aCoordinate.mnRow,
        rInput.maReference.maRange.maStart.mnSheet);
    return spreadsheetengine::api::ValueResult<LookupExecutionResult>::success(aResult);
}

inline void putApiCellValue(
    const spreadsheetengine::api::CellValue& rValue, const ScMatrixRef& pMatrix, SCSIZE nColumn,
    SCSIZE nRow)
{
    if (rValue.isError())
        pMatrix->PutError(toFormulaError(rValue.meError), nColumn, nRow);
    else if (rValue.isText())
        pMatrix->PutString(svl::SharedString(toLibreOfficeString(rValue.maString)), nColumn, nRow);
    else if (rValue.isBoolean())
        pMatrix->PutBoolean(rValue.mfNumber != 0.0, nColumn, nRow);
    else if (rValue.isNumber())
        pMatrix->PutDouble(rValue.mfNumber, nColumn, nRow);
    else
        pMatrix->PutEmpty(nColumn, nRow);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<LookupExecutionResult> makeSliceResult(
    const CalcLookupMaterializer& rMaterializer, const LookupInput& rInput,
    const spreadsheetengine::api::lookup::VectorSlice& rSlice)
{
    if (rInput.mbScalar)
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::success(
            makeScalarResult(rInput.maScalar));

    if (rSlice.isSingleCell())
        return makeCoordinateResult(rMaterializer, rInput, rSlice.maStart);

    if (rInput.maValues.empty())
    {
        LookupExecutionResult aResult;
        aResult.meKind = LookupExecutionResult::Kind::Reference;
        aResult.maRange = ScRange(
            rInput.maReference.maRange.maStart.mnColumn + rSlice.maStart.mnColumn,
            rInput.maReference.maRange.maStart.mnRow + rSlice.maStart.mnRow,
            rInput.maReference.maRange.maStart.mnSheet,
            rInput.maReference.maRange.maStart.mnColumn + rSlice.maStart.mnColumn
                + rSlice.maDimensions.mnColumns - 1,
            rInput.maReference.maRange.maStart.mnRow + rSlice.maStart.mnRow
                + rSlice.maDimensions.mnRows - 1,
            rInput.maReference.maRange.maStart.mnSheet);
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::success(aResult);
    }

    ScMatrixRef pMatrix = new ScMatrix(
        static_cast<SCSIZE>(rSlice.maDimensions.mnColumns),
        static_cast<SCSIZE>(rSlice.maDimensions.mnRows));
    for (spreadsheetengine::api::MatrixSize nRow = 0; nRow < rSlice.maDimensions.mnRows; ++nRow)
    {
        for (spreadsheetengine::api::MatrixSize nColumn = 0; nColumn < rSlice.maDimensions.mnColumns;
             ++nColumn)
        {
            const spreadsheetengine::api::MatrixCoordinate aCoordinate {
                rSlice.maStart.mnColumn + nColumn, rSlice.maStart.mnRow + nRow
            };
            const auto aValue = rMaterializer.materialize(rInput, aCoordinate);
            if (!aValue)
            {
                return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
                    aValue.meError);
            }
            putApiCellValue(aValue.maValue, pMatrix, static_cast<SCSIZE>(nColumn),
                static_cast<SCSIZE>(nRow));
        }
    }

    LookupExecutionResult aResult;
    aResult.meKind = LookupExecutionResult::Kind::Matrix;
    aResult.mpMatrix = pMatrix;
    return spreadsheetengine::api::ValueResult<LookupExecutionResult>::success(aResult);
}

} // namespace detail

[[nodiscard]] inline spreadsheetengine::api::ValueResult<spreadsheetengine::api::MatrixSize>
resolveMatchIndex(const ScDocument& rDoc, ScInterpreterContext& rContext,
    const MatchExecutionRequest& rRequest)
{
    const auto aSearchInput = detail::buildLookupInput(rRequest.maSearchSource);
    if (!aSearchInput)
    {
        return spreadsheetengine::api::ValueResult<
            spreadsheetengine::api::MatrixSize>::failure(aSearchInput.meError);
    }

    const detail::CalcLookupMaterializer aMaterializer(rDoc, rContext);
    if (!rRequest.mbExtended)
    {
        return spreadsheetengine::core::lookup::resolveMatchIndex(
            aMaterializer, rRequest.maLookupValue, aSearchInput.maValue, rRequest.maLegacyModes,
            rRequest.meSearchType);
    }

    return spreadsheetengine::core::lookup::resolveExtendedMatchIndex(aMaterializer,
        rRequest.maLookupValue, aSearchInput.maValue, rRequest.meMatchMode,
        rRequest.meSearchMode, rRequest.meSearchType, rRequest.mbAllowPatternMatch);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<LookupExecutionResult>
resolveLookupResult(const ScDocument& rDoc, ScInterpreterContext& rContext,
    const LegacyLookupRequest& rRequest)
{
    if (rRequest.maLookupValue.isError())
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            rRequest.maLookupValue.meError);
    }
    if (rRequest.maLookupValue.isEmpty())
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            spreadsheetengine::api::Error::NotAvailable);
    }

    const auto aDataLayout
        = spreadsheetengine::core::lookup::detectLookupLayout(rRequest.maDataInput, true);
    if (!aDataLayout)
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            aDataLayout.meError);
    }

    const detail::CalcLookupMaterializer aMaterializer(rDoc, rContext);
    std::optional<spreadsheetengine::api::lookup::VectorLayout> oResultLayout;
    if (rRequest.moResultInput)
    {
        const auto aResultLayout
            = spreadsheetengine::core::lookup::detectLookupLayout(*rRequest.moResultInput, false);
        if (!aResultLayout)
        {
            return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
                aResultLayout.meError);
        }
        oResultLayout = aResultLayout.maValue;
    }

    const auto aResolvedIndex = spreadsheetengine::core::lookup::resolveLookupIndex(
        aMaterializer, rRequest.maLookupValue, rRequest.maDataInput, rRequest.meSearchType);
    if (!aResolvedIndex)
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(aResolvedIndex.meError);

    const auto aMatchedSearchValue = spreadsheetengine::core::lookup::materializeLookupInputValue(
        aMaterializer, rRequest.maDataInput, aDataLayout.maValue.meOrientation,
        aResolvedIndex.maValue);
    if (!aMatchedSearchValue)
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            aMatchedSearchValue.meError);
    }

    if (rRequest.maLookupValue.isText() && (aMatchedSearchValue.maValue.isNumber()
                                            || aMatchedSearchValue.maValue.isBoolean()))
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            spreadsheetengine::api::Error::NotAvailable);
    }

    if (rRequest.moResultInput)
    {
        if (rRequest.moResultInput->mbScalar)
        {
            if (aResolvedIndex.maValue != 0)
            {
                return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
                    spreadsheetengine::api::Error::NotAvailable);
            }
            return spreadsheetengine::api::ValueResult<LookupExecutionResult>::success(
                detail::makeScalarResult(rRequest.moResultInput->maScalar));
        }

        const auto aCoordinate = spreadsheetengine::api::lookup::planVectorElement(
            oResultLayout->meOrientation, aResolvedIndex.maValue,
            { rRequest.moResultInput->mnColumns, rRequest.moResultInput->mnRows });
        if (!aCoordinate)
        {
            return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
                aCoordinate.meError);
        }
        return detail::makeCoordinateResult(aMaterializer, *rRequest.moResultInput,
            aCoordinate.maValue);
    }

    if (rRequest.maDataInput.mbScalar)
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::success(
            detail::makeScalarResult(rRequest.maDataInput.maScalar));
    }

    const spreadsheetengine::api::MatrixDimensions aDimensions {
        rRequest.maDataInput.mnColumns, rRequest.maDataInput.mnRows
    };
    const spreadsheetengine::api::MatrixSize nResultIndex
        = aDataLayout.maValue.meOrientation
              == spreadsheetengine::api::lookup::VectorOrientation::Column
              ? aDimensions.mnColumns - 1
              : aDimensions.mnRows - 1;
    const auto aCoordinate = spreadsheetengine::api::lookup::planTabularLookupResult(
        aDataLayout.maValue.meOrientation, aResolvedIndex.maValue, nResultIndex, aDimensions);
    if (!aCoordinate)
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            aCoordinate.meError);
    }
    return detail::makeCoordinateResult(aMaterializer, rRequest.maDataInput, aCoordinate.maValue);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<LookupExecutionResult>
resolveTabularLookupResult(const ScDocument& rDoc, ScInterpreterContext& rContext,
    const TabularLookupRequest& rRequest)
{
    if (rRequest.maLookupValue.isError())
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            rRequest.maLookupValue.meError);
    }

    const spreadsheetengine::api::MatrixDimensions aDimensions {
        rRequest.maTableInput.mnColumns, rRequest.maTableInput.mnRows
    };
    if (aDimensions.isEmpty())
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            spreadsheetengine::api::Error::IllegalArgument);
    }

    if (rRequest.meSearchOrientation == spreadsheetengine::api::lookup::VectorOrientation::Column)
    {
        if (rRequest.mnResultIndex < 0 || rRequest.mnResultIndex >= aDimensions.mnColumns)
        {
            return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
        }
    }
    else if (rRequest.mnResultIndex < 0 || rRequest.mnResultIndex >= aDimensions.mnRows)
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            spreadsheetengine::api::Error::IllegalArgument);
    }

    const detail::CalcLookupMaterializer aMaterializer(rDoc, rContext);
    const auto aResolvedIndex = spreadsheetengine::core::lookup::resolveTabularLookupIndex(
        aMaterializer, rRequest.maLookupValue, rRequest.maTableInput, rRequest.meSearchOrientation,
        rRequest.mbApproximate, rRequest.meSearchType);
    if (!aResolvedIndex)
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(aResolvedIndex.meError);

    const auto aMatchedSearchValue = spreadsheetengine::core::lookup::materializeLookupInputValue(
        aMaterializer, rRequest.maTableInput, rRequest.meSearchOrientation, aResolvedIndex.maValue);
    if (!aMatchedSearchValue)
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            aMatchedSearchValue.meError);
    }

    if (rRequest.mbApproximate && rRequest.maLookupValue.isText()
        && (aMatchedSearchValue.maValue.isNumber()
            || aMatchedSearchValue.maValue.isBoolean()))
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            spreadsheetengine::api::Error::NotAvailable);
    }

    const auto aCoordinate = spreadsheetengine::api::lookup::planTabularLookupResult(
        rRequest.meSearchOrientation, aResolvedIndex.maValue, rRequest.mnResultIndex, aDimensions);
    if (!aCoordinate)
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            aCoordinate.meError);
    }
    return detail::makeCoordinateResult(aMaterializer, rRequest.maTableInput, aCoordinate.maValue);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<LookupExecutionResult>
resolveXLookupResult(const ScDocument& rDoc, ScInterpreterContext& rContext,
    const XLookupExecutionRequest& rRequest)
{
    if (rRequest.maLookupValue.isError())
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            rRequest.maLookupValue.meError);
    }
    if (rRequest.maLookupValue.isEmpty())
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            spreadsheetengine::api::Error::NotAvailable);
    }

    const auto aSearchLayout
        = spreadsheetengine::core::lookup::detectLookupLayout(rRequest.maSearchInput, false);
    if (!aSearchLayout)
    {
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(
            aSearchLayout.meError);
    }

    const auto aShape = spreadsheetengine::api::lookup::validateXLookupResultShape(
        { rRequest.maSearchInput.mnColumns, rRequest.maSearchInput.mnRows },
        { rRequest.maResultInput.mnColumns, rRequest.maResultInput.mnRows });
    if (!aShape)
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(aShape.meError);

    const detail::CalcLookupMaterializer aMaterializer(rDoc, rContext);
    const auto aResolvedIndex = spreadsheetengine::core::lookup::resolveExtendedMatchIndex(
        aMaterializer, rRequest.maLookupValue, rRequest.maSearchInput, rRequest.meMatchMode,
        rRequest.meSearchMode, rRequest.meSearchType, rRequest.mbAllowPatternMatch);
    if (!aResolvedIndex)
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(aResolvedIndex.meError);

    const auto aResultSlice = spreadsheetengine::api::lookup::planXLookupResultSlice(
        aSearchLayout.maValue.meOrientation, aResolvedIndex.maValue,
        { rRequest.maResultInput.mnColumns, rRequest.maResultInput.mnRows });
    if (!aResultSlice)
        return spreadsheetengine::api::ValueResult<LookupExecutionResult>::failure(aResultSlice.meError);

    return detail::makeSliceResult(aMaterializer, rRequest.maResultInput, aResultSlice.maValue);
}

} // namespace spreadsheetengine::compat::libreoffice::lookupexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
