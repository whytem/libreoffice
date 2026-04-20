/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/runtime/MathMatrix.hxx>
#include <spreadsheetengine/runtime/RpnOperators.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 4 substrate: engine-native matrix-operand decision layer.
//
// This header defines the dense matrix operand shape the engine RPN
// evaluator needs for MINVERSE / MMULT / MDETERM / TRANSPOSE / MUNIT /
// MSEQUENCE / SUMPRODUCT / SUMX2MY2 / SUMX2DY2 / SUMXMY2 plus the
// regression / forecast family (LINEST, LOGEST, TREND, GROWTH,
// FORECAST, FOURIER).
//
// The substrate deliberately does NOT own:
// - host-side materialization of a reference-shaped operand into a
//   dense matrix (that belongs to the Host facade, which will grow a
//   forEachCellInRange primitive)
// - matrix-frame state (bMatrixFormula, JumpMatrix) which stays on
//   Calc's interpreter until the RPN loop subsumes the whole matrix
//   broadcast protocol
// - numerical primitives like LU / QR decomposition (those live in
//   core::math already and are called from the planners below)
//
// This layer now backs the admitted svMatrix-only paths for MUNIT /
// MSEQUENCE / TRANSPOSE / MDETERM / MMULT / MINVERSE plus the first
// regression/forecast matrix admissions. Reference-to-matrix widening and
// full matrix-frame ownership still defer to later host/materialization work.

namespace spreadsheetengine::core::rpn
{

// Provenance tags the source of a MatrixOperand so downstream code can
// route correctly: inline literal matrices preserve formatting, while
// materialized reference operands may require a secondary resolution
// round-trip through the host.
enum class MatrixProvenance : std::uint8_t
{
    // Result of an inline matrix literal (e.g. `={1,2;3,4}`).
    InlineLiteral,
    // A reference that was materialized into a dense grid via the
    // host's forEachCellInRange primitive.
    MaterializedReference,
    // Produced by a previous engine computation (MMULT result, etc.).
    ComputedResult
};

struct MatrixOperand
{
    api::MatrixDimensions maDimensions { 0, 0 };
    std::vector<api::CellValue> maValues; // row-major, size = rows * columns
    MatrixProvenance meProvenance = MatrixProvenance::ComputedResult;

    [[nodiscard]] constexpr bool isEmpty() const
    {
        return maDimensions.mnColumns == 0 || maDimensions.mnRows == 0;
    }

    [[nodiscard]] constexpr std::size_t cellCount() const
    {
        return static_cast<std::size_t>(maDimensions.mnColumns) * maDimensions.mnRows;
    }

    [[nodiscard]] bool linearIndexOf(
        api::MatrixCoordinate aCoord, std::size_t& rLinearIndex) const
    {
        if (!api::isValidCoordinate(maDimensions, aCoord))
            return false;
        rLinearIndex = static_cast<std::size_t>(aCoord.mnRow) * maDimensions.mnColumns
                       + static_cast<std::size_t>(aCoord.mnColumn);
        return true;
    }

    [[nodiscard]] const api::CellValue* at(api::MatrixCoordinate aCoord) const
    {
        std::size_t nIndex = 0;
        if (!linearIndexOf(aCoord, nIndex) || nIndex >= maValues.size())
            return nullptr;
        return &maValues[nIndex];
    }
};

// Build an MUNIT identity matrix of side n. Pure computation, no host
// access required.
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planIdentityMatrix(
    std::size_t nDimension)
{
    if (nDimension == 0)
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    MatrixOperand aResult;
    aResult.maDimensions
        = { static_cast<api::MatrixSize>(nDimension), static_cast<api::MatrixSize>(nDimension) };
    aResult.maValues.assign(nDimension * nDimension, api::CellValue::number(0.0));
    aResult.meProvenance = MatrixProvenance::ComputedResult;
    for (std::size_t i = 0; i < nDimension; ++i)
        aResult.maValues[i * nDimension + i] = api::CellValue::number(1.0);
    return RpnCoercionResult<MatrixOperand>::success(aResult);
}

// Build an MSEQUENCE matrix (rows x columns) starting from fStart with
// stride fStep. Pure computation.
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planSequenceMatrix(
    std::size_t nRows, std::size_t nColumns, double fStart, double fStep)
{
    if (nRows == 0 || nColumns == 0)
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    MatrixOperand aResult;
    aResult.maDimensions
        = { static_cast<api::MatrixSize>(nColumns), static_cast<api::MatrixSize>(nRows) };
    aResult.maValues.reserve(nRows * nColumns);
    double fValue = fStart;
    for (std::size_t i = 0; i < nRows * nColumns; ++i)
    {
        aResult.maValues.push_back(api::CellValue::number(fValue));
        fValue += fStep;
    }
    aResult.meProvenance = MatrixProvenance::ComputedResult;
    return RpnCoercionResult<MatrixOperand>::success(aResult);
}

// Transpose: swaps rows <-> columns, flips cell layout. Pure.
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planTranspose(
    const MatrixOperand& rSource)
{
    if (rSource.isEmpty())
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);

    MatrixOperand aResult;
    aResult.maDimensions = { rSource.maDimensions.mnRows, rSource.maDimensions.mnColumns };
    aResult.maValues.resize(rSource.cellCount());
    aResult.meProvenance = MatrixProvenance::ComputedResult;

    for (api::MatrixSize r = 0; r < rSource.maDimensions.mnRows; ++r)
    {
        for (api::MatrixSize c = 0; c < rSource.maDimensions.mnColumns; ++c)
        {
            const std::size_t nSrc
                = static_cast<std::size_t>(r) * rSource.maDimensions.mnColumns + c;
            const std::size_t nDst
                = static_cast<std::size_t>(c) * rSource.maDimensions.mnRows + r;
            aResult.maValues[nDst] = rSource.maValues[nSrc];
        }
    }
    return RpnCoercionResult<MatrixOperand>::success(aResult);
}

namespace detail
{

// Flatten a MatrixOperand row-major to a std::vector<double>. Numeric
// and boolean cells coerce via `CellValue.mfNumber`; empty cells map
// to `0.0`; text cells surface as `IllegalArgument`; error cells
// propagate their `meError`.
[[nodiscard]] inline bool tryFlattenNumericMatrix(
    const MatrixOperand& rMatrix, std::vector<double>& rOut, api::Error& rError)
{
    rOut.clear();
    rOut.reserve(rMatrix.cellCount());
    for (const auto& rValue : rMatrix.maValues)
    {
        if (rValue.meKind == api::CellValueKind::Error)
        {
            rError = rValue.meError;
            return false;
        }
        if (rValue.meKind == api::CellValueKind::Number
            || rValue.meKind == api::CellValueKind::Boolean)
        {
            rOut.push_back(rValue.mfNumber);
        }
        else if (rValue.meKind == api::CellValueKind::Empty)
        {
            rOut.push_back(0.0);
        }
        else
        {
            // Text cells are invalid input for numeric matrix ops.
            rError = api::Error::IllegalArgument;
            return false;
        }
    }
    return true;
}

} // namespace detail

// Determinant: bridges to core::math::evaluateMatrixDeterminant. The
// source matrix must be square and numeric; non-numeric cells coerce
// via CellValue.mfNumber directly (empty = 0, errors propagate).
[[nodiscard]] inline RpnCoercionResult<double> planDeterminant(const MatrixOperand& rSource)
{
    if (rSource.isEmpty()
        || rSource.maDimensions.mnColumns != rSource.maDimensions.mnRows)
    {
        return RpnCoercionResult<double>::failure(api::Error::IllegalArgument);
    }

    std::vector<double> aFlat;
    api::Error eFlattenError = api::Error::None;
    if (!detail::tryFlattenNumericMatrix(rSource, aFlat, eFlattenError))
        return RpnCoercionResult<double>::failure(eFlattenError);

    const auto aResult = core::math::evaluateMatrixDeterminant(
        aFlat, static_cast<std::size_t>(rSource.maDimensions.mnColumns));
    if (!aResult)
        return RpnCoercionResult<double>::failure(aResult.meError);
    return RpnCoercionResult<double>::success(aResult.maValue);
}

// Matrix product: LEFT (m x k) * RIGHT (k x n) -> (m x n). The engine
// `evaluateMatrixMultiply` walks the same summation order as
// `ScInterpreter::ScMatMult`. Non-square shapes with compatible inner
// dimensions are accepted.
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planMatrixMultiply(
    const MatrixOperand& rLeft, const MatrixOperand& rRight)
{
    if (rLeft.isEmpty() || rRight.isEmpty()
        || rLeft.maDimensions.mnColumns != rRight.maDimensions.mnRows)
    {
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    }

    std::vector<double> aLeftFlat;
    api::Error eLeftError = api::Error::None;
    if (!detail::tryFlattenNumericMatrix(rLeft, aLeftFlat, eLeftError))
        return RpnCoercionResult<MatrixOperand>::failure(eLeftError);

    std::vector<double> aRightFlat;
    api::Error eRightError = api::Error::None;
    if (!detail::tryFlattenNumericMatrix(rRight, aRightFlat, eRightError))
        return RpnCoercionResult<MatrixOperand>::failure(eRightError);

    const std::size_t nLeftRows = static_cast<std::size_t>(rLeft.maDimensions.mnRows);
    const std::size_t nInner = static_cast<std::size_t>(rLeft.maDimensions.mnColumns);
    const std::size_t nRightCols = static_cast<std::size_t>(rRight.maDimensions.mnColumns);

    const auto aResult = core::math::evaluateMatrixMultiply(
        aLeftFlat, nLeftRows, nInner, aRightFlat, nRightCols);
    if (!aResult)
        return RpnCoercionResult<MatrixOperand>::failure(aResult.meError);

    MatrixOperand aOut;
    aOut.maDimensions = { static_cast<api::MatrixSize>(nRightCols),
                          static_cast<api::MatrixSize>(nLeftRows) };
    aOut.maValues.reserve(nLeftRows * nRightCols);
    for (double fValue : aResult.maValue)
        aOut.maValues.push_back(api::CellValue::number(fValue));
    aOut.meProvenance = MatrixProvenance::ComputedResult;
    return RpnCoercionResult<MatrixOperand>::success(std::move(aOut));
}

// Matrix inverse via LUP decomposition. Source must be square and
// strictly numeric (empty = 0, text = IllegalArgument, error =
// propagated). Singular inputs surface as `Error::IllegalArgument`
// matching the legacy `ScMatInv` PushIllegalArgument path (Calc maps
// it to `#VALUE!`).
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planMatrixInverse(
    const MatrixOperand& rSource)
{
    if (rSource.isEmpty()
        || rSource.maDimensions.mnColumns != rSource.maDimensions.mnRows)
    {
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    }

    std::vector<double> aFlat;
    api::Error eFlattenError = api::Error::None;
    if (!detail::tryFlattenNumericMatrix(rSource, aFlat, eFlattenError))
        return RpnCoercionResult<MatrixOperand>::failure(eFlattenError);

    const std::size_t nDimension = static_cast<std::size_t>(rSource.maDimensions.mnColumns);
    const auto aResult = core::math::evaluateMatrixInverse(aFlat, nDimension);
    if (!aResult)
        return RpnCoercionResult<MatrixOperand>::failure(aResult.meError);

    MatrixOperand aOut;
    aOut.maDimensions = rSource.maDimensions;
    aOut.maValues.reserve(nDimension * nDimension);
    for (double fValue : aResult.maValue)
        aOut.maValues.push_back(api::CellValue::number(fValue));
    aOut.meProvenance = MatrixProvenance::ComputedResult;
    return RpnCoercionResult<MatrixOperand>::success(std::move(aOut));
}

// Broadcast binary: MMULT / Add / Subtract etc. broadcast rules are
// Calc-specific. This pair handles the two canonical shapes:
//   - scalar op matrix: scalar broadcast to every cell
//   - matrix op matrix (same dimensions): element-wise
// Matrix-matrix with different but broadcast-compatible dimensions
// (e.g. vector broadcast) defers — it's a Calc-specific protocol that
// the matrix-frame machinery owns.
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planBroadcastScalarOverMatrix(
    BinaryScalarOperator eOperator, const RpnValue& rScalar,
    const MatrixOperand& rMatrix)
{
    if (rMatrix.isEmpty())
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    if (!rScalar.isScalar())
        return RpnCoercionResult<MatrixOperand>::deferred(
            RpnCoercionReadiness::NeedsMatrixMaterialization);

    MatrixOperand aResult;
    aResult.maDimensions = rMatrix.maDimensions;
    aResult.maValues.reserve(rMatrix.cellCount());
    aResult.meProvenance = MatrixProvenance::ComputedResult;

    for (const auto& rCell : rMatrix.maValues)
    {
        const RpnValue aCellRpn = RpnValue::fromCellValue(rCell);
        const auto aPair = evaluateBinaryScalarOperator(eOperator, rScalar, aCellRpn);
        if (aPair.meReadiness != RpnCoercionReadiness::Ready)
            return RpnCoercionResult<MatrixOperand>::deferred(aPair.meReadiness);
        if (!aPair)
            aResult.maValues.push_back(api::CellValue::error(aPair.meError));
        else
            aResult.maValues.push_back(aPair.maValue.maScalar);
    }
    return RpnCoercionResult<MatrixOperand>::success(aResult);
}

[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planElementwiseBinary(
    BinaryScalarOperator eOperator, const MatrixOperand& rLeft,
    const MatrixOperand& rRight)
{
    if (rLeft.isEmpty() || rRight.isEmpty()
        || rLeft.maDimensions.mnColumns != rRight.maDimensions.mnColumns
        || rLeft.maDimensions.mnRows != rRight.maDimensions.mnRows)
    {
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    }

    MatrixOperand aResult;
    aResult.maDimensions = rLeft.maDimensions;
    aResult.maValues.reserve(rLeft.cellCount());
    aResult.meProvenance = MatrixProvenance::ComputedResult;

    for (std::size_t i = 0; i < rLeft.maValues.size(); ++i)
    {
        const RpnValue aL = RpnValue::fromCellValue(rLeft.maValues[i]);
        const RpnValue aR = RpnValue::fromCellValue(rRight.maValues[i]);
        const auto aPair = evaluateBinaryScalarOperator(eOperator, aL, aR);
        if (aPair.meReadiness != RpnCoercionReadiness::Ready)
            return RpnCoercionResult<MatrixOperand>::deferred(aPair.meReadiness);
        if (!aPair)
            aResult.maValues.push_back(api::CellValue::error(aPair.meError));
        else
            aResult.maValues.push_back(aPair.maValue.maScalar);
    }
    return RpnCoercionResult<MatrixOperand>::success(aResult);
}

// SUMPRODUCT family reductions. SumProduct: sum of product of
// corresponding numeric cells across N parallel same-shape matrices.
// SumX2MY2: sum of (x_i^2 - y_i^2). SumX2PY2: sum of (x_i^2 + y_i^2).
// SumXMY2: sum of (x_i - y_i)^2.
enum class SumReductionKind : std::uint8_t
{
    SumProduct,
    SumX2MinusY2,
    SumX2PlusY2,
    SumXMinusY2
};

[[nodiscard]] inline RpnCoercionResult<double> planSumReductionPair(
    SumReductionKind eKind, const MatrixOperand& rX, const MatrixOperand& rY)
{
    if (rX.isEmpty() || rY.isEmpty()
        || rX.maDimensions.mnColumns != rY.maDimensions.mnColumns
        || rX.maDimensions.mnRows != rY.maDimensions.mnRows)
    {
        return RpnCoercionResult<double>::failure(api::Error::IllegalArgument);
    }

    double fSum = 0.0;
    for (std::size_t i = 0; i < rX.maValues.size(); ++i)
    {
        const auto& rXv = rX.maValues[i];
        const auto& rYv = rY.maValues[i];
        if (rXv.meKind == api::CellValueKind::Error)
            return RpnCoercionResult<double>::failure(rXv.meError);
        if (rYv.meKind == api::CellValueKind::Error)
            return RpnCoercionResult<double>::failure(rYv.meError);

        // Non-numeric cells are skipped in classic Calc semantics.
        const bool bXNumeric = rXv.meKind == api::CellValueKind::Number
                               || rXv.meKind == api::CellValueKind::Boolean;
        const bool bYNumeric = rYv.meKind == api::CellValueKind::Number
                               || rYv.meKind == api::CellValueKind::Boolean;
        if (!bXNumeric || !bYNumeric)
            continue;

        const double x = rXv.mfNumber;
        const double y = rYv.mfNumber;
        switch (eKind)
        {
            case SumReductionKind::SumProduct:
                fSum += x * y;
                break;
            case SumReductionKind::SumX2MinusY2:
                fSum += x * x - y * y;
                break;
            case SumReductionKind::SumX2PlusY2:
                fSum += x * x + y * y;
                break;
            case SumReductionKind::SumXMinusY2:
            {
                const double d = x - y;
                fSum += d * d;
                break;
            }
        }
    }
    return RpnCoercionResult<double>::success(fSum);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
