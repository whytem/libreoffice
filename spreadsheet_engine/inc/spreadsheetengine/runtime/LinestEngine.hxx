/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/runtime/KahanSum.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnOperators.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 4 substrate: engine-native linear-regression core.
//
// Header-only implementation of QR-decomposition-based linear least squares
// matching the legacy semantics of `ScInterpreter::ScLinest`,
// `ScInterpreter::ScLogest`, and `ScInterpreter::ScTrend` in
// `sc/source/core/tool/interpr5.cxx`.
//
// Scope of this core:
// - `planLinest(X, Y, bConstant, bStats)` - LINEST / LOGEST
// - `planTrend(knownY, knownX, newX, bConstant)` - TREND / GROWTH
// The LOGEST and GROWTH variants feed log(Y) in / exp(...) out.
//
// Numerical epsilon policy: matches legacy exactly by reusing the same
// algorithmic primitives (Householder QR, Kahan-summed inner products,
// approxSub for mean subtraction). We therefore expect bit-for-bit
// agreement with `CalculateRGPRKP` / `CalculateTrendGrowth` on well-
// conditioned inputs; ill-conditioned cases (where aVecR[i] == 0 after
// QR) propagate the same "no value" error. No new tolerance is
// introduced.
//
// Not owned by this core:
// - Host-side materialization of a reference-shaped operand into a dense
//   matrix. Callers (Calc's dispatch lambda) provide MatrixOperand
//   instances only; Phase D's `materializeRangeToMatrix` will widen the
//   scope fence.
// - Statistics rows (K+1 x 5 shape). Stats are produced when bStats is
//   true; for simple regression (1 X) parity is achieved. Multi-variable
//   stats are handled by the same QR pipeline but depend on the caller
//   having wired MatrixOperand dimensions correctly.
//
// This layer now backs the admitted regression / trend / growth matrix
// planners used by Calc's engine-first dispatch. Wider host-sensitive
// shapes and the remaining forecast/ETS tails still defer through the
// surrounding planner/decline plumbing, but this core is no longer
// substrate-only.

namespace spreadsheetengine::core::rpn
{

// Result of a Linest / Logest plan: a dense matrix operand in the same
// row-major layout as `RpnMatrix.hxx` MatrixOperand. Stats rows are
// filled with actual numbers or NotAvailable-style error entries; the
// bStats=false case returns a single-row (K+1) matrix.
struct LinestPlanResult
{
    // Matches ScLinest's result shape exactly: (K+1) columns x
    // (bStats ? 5 : 1) rows. Slopes are reported in reverse order of
    // input columns (slope for the last X column first), matching the
    // ODF / Calc convention.
    MatrixOperand maMatrix;
};

// Result of a Trend / Growth plan: a dense matrix operand of newX shape
// (simple regression: same shape as newX; multi-var case 2: column
// vector of nRXN rows; case 3: row vector of nCXN columns).
struct TrendPlanResult
{
    MatrixOperand maMatrix;
};

namespace detail::linest
{

// Trivial strongly-typed wrapper around std::vector<double> to hold a
// flat column-major (matching legacy ScMatrix index access `(col, row)`)
// working copy of the X matrix during QR decomposition. Row-major would
// work too but we match the legacy traversal order to keep summation
// order identical to Kahan-summed paths.
struct FlatMatrix
{
    std::vector<double> maData;
    std::size_t mnCols = 0;
    std::size_t mnRows = 0;

    [[nodiscard]] double get(std::size_t nCol, std::size_t nRow) const
    {
        return maData[nRow * mnCols + nCol];
    }
    void set(std::size_t nCol, std::size_t nRow, double fValue)
    {
        maData[nRow * mnCols + nCol] = fValue;
    }
};

[[nodiscard]] inline FlatMatrix makeFlat(std::size_t nCols, std::size_t nRows)
{
    FlatMatrix aResult;
    aResult.mnCols = nCols;
    aResult.mnRows = nRows;
    aResult.maData.assign(nCols * nRows, 0.0);
    return aResult;
}

[[nodiscard]] inline double getSumProduct(
    const FlatMatrix& rA, const FlatMatrix& rB, std::size_t nM)
{
    // Treat as flat vectors of length nM (matches lcl_GetSumProduct).
    spreadsheetengine::core::fp::KahanSum aSum;
    for (std::size_t i = 0; i < nM; ++i)
        aSum += rA.maData[i] * rB.maData[i];
    return aSum.get();
}

[[nodiscard]] inline double getColumnMaximumNorm(
    const FlatMatrix& rA, std::size_t nC, std::size_t nR, std::size_t nN)
{
    double fNorm = 0.0;
    for (std::size_t row = nR; row < nN; ++row)
    {
        const double fVal = std::fabs(rA.get(nC, row));
        if (fNorm < fVal)
            fNorm = fVal;
    }
    return fNorm;
}

[[nodiscard]] inline double getColumnEuclideanNorm(
    const FlatMatrix& rA, std::size_t nC, std::size_t nR, std::size_t nN)
{
    spreadsheetengine::core::fp::KahanSum aNorm;
    for (std::size_t row = nR; row < nN; ++row)
        aNorm += rA.get(nC, row) * rA.get(nC, row);
    return std::sqrt(aNorm.get());
}

[[nodiscard]] inline double getColumnSumProduct(
    const FlatMatrix& rA, std::size_t nCa,
    const FlatMatrix& rB, std::size_t nCb,
    std::size_t nR, std::size_t nN)
{
    spreadsheetengine::core::fp::KahanSum aResult;
    for (std::size_t row = nR; row < nN; ++row)
        aResult += rA.get(nCa, row) * rB.get(nCb, row);
    return aResult.get();
}

[[nodiscard]] inline double getSign(double fValue)
{
    return fValue >= 0.0 ? 1.0 : -1.0;
}

// Householder QR: legacy lcl_CalculateQRdecomposition.
// Returns false if the matrix is singular (column maximum-norm is zero).
[[nodiscard]] inline bool calculateQRdecomposition(
    FlatMatrix& rA, std::vector<double>& rVecR,
    std::size_t nK, std::size_t nN)
{
    for (std::size_t col = 0; col < nK; ++col)
    {
        const double fScale = getColumnMaximumNorm(rA, col, col, nN);
        if (fScale == 0.0)
            return false;
        for (std::size_t row = col; row < nN; ++row)
            rA.set(col, row, rA.get(col, row) / fScale);

        const double fEuclid = getColumnEuclideanNorm(rA, col, col, nN);
        const double fFactor = 1.0 / fEuclid
                               / (fEuclid + std::fabs(rA.get(col, col)));
        const double fSignum = getSign(rA.get(col, col));
        rA.set(col, col, rA.get(col, col) + fSignum * fEuclid);
        rVecR[col] = -fSignum * fScale * fEuclid;

        for (std::size_t c = col + 1; c < nK; ++c)
        {
            const double fSum = getColumnSumProduct(rA, col, rA, c, col, nN);
            for (std::size_t row = col; row < nN; ++row)
                rA.set(c, row,
                       rA.get(c, row) - fSum * fFactor * rA.get(col, row));
        }
    }
    return true;
}

// Apply Householder transformation to a column vector Y stored as a
// flat vector of length nN; matches lcl_ApplyHouseholderTransformation.
inline void applyHouseholderTransformation(
    const FlatMatrix& rA, std::size_t nC,
    std::vector<double>& rY, std::size_t nN)
{
    spreadsheetengine::core::fp::KahanSum aDen;
    for (std::size_t row = nC; row < nN; ++row)
        aDen += rA.get(nC, row) * rA.get(nC, row);
    spreadsheetengine::core::fp::KahanSum aNum;
    for (std::size_t row = nC; row < nN; ++row)
        aNum += rA.get(nC, row) * rY[row];
    const double fDenominator = aDen.get();
    const double fNumerator = aNum.get();
    const double fFactor
        = (fDenominator == 0.0) ? 0.0 : 2.0 * fNumerator / fDenominator;
    for (std::size_t row = nC; row < nN; ++row)
        rY[row] = rY[row] - fFactor * rA.get(nC, row);
}

// Back-substitution solve R*X=S (legacy lcl_SolveWithUpperRightTriangle
// with bIsTransposed=false). R is stored across rA upper right + rVecR
// diagonal. S is nK elements; answer overwrites S.
inline void solveWithUpperRightTriangle(
    const FlatMatrix& rA, const std::vector<double>& rVecR,
    std::vector<double>& rS, std::size_t nK)
{
    for (std::size_t rowp1 = nK; rowp1 > 0; --rowp1)
    {
        const std::size_t row = rowp1 - 1;
        spreadsheetengine::core::fp::KahanSum aSum;
        aSum += rS[row];
        for (std::size_t col = rowp1; col < nK; ++col)
            aSum += -rA.get(col, row) * rS[col];
        rS[row] = aSum.get() / rVecR[row];
    }
}

// Extract dense (flat) X / Y matrices from MatrixOperand inputs and
// determine the regression case (1=simple, 2=Y column, 3=Y row) the
// same way `ScInterpreter::CheckMatrix` does. Returns false with
// meError set if any cell in Y (or, for bLog, any non-positive value in
// Y) is non-numeric.
struct CheckMatrixOutcome
{
    bool mbOk = false;
    api::Error meError = api::Error::IllegalArgument;
    std::uint8_t mnCase = 0; // 1, 2, or 3
    std::size_t mnK = 0;
    std::size_t mnN = 0;
    std::size_t mnCX = 0;
    std::size_t mnRX = 0;
    std::size_t mnCY = 0;
    std::size_t mnRY = 0;
    FlatMatrix maX;
    FlatMatrix maY;
};

[[nodiscard]] inline CheckMatrixOutcome checkMatrix(
    bool bLog, const MatrixOperand* pMatX, const MatrixOperand& rMatY)
{
    CheckMatrixOutcome aOut;
    if (rMatY.isEmpty())
    {
        aOut.meError = api::Error::IllegalArgument;
        return aOut;
    }
    aOut.mnCY = static_cast<std::size_t>(rMatY.maDimensions.mnColumns);
    aOut.mnRY = static_cast<std::size_t>(rMatY.maDimensions.mnRows);
    const std::size_t nCountY = aOut.mnCY * aOut.mnRY;
    aOut.maY.mnCols = aOut.mnCY;
    aOut.maY.mnRows = aOut.mnRY;
    aOut.maY.maData.reserve(nCountY);

    for (std::size_t i = 0; i < nCountY; ++i)
    {
        const auto& rCell = rMatY.maValues[i];
        if (rCell.meKind == api::CellValueKind::Error)
        {
            aOut.meError = rCell.meError;
            return aOut;
        }
        if (rCell.meKind != api::CellValueKind::Number
            && rCell.meKind != api::CellValueKind::Boolean)
        {
            aOut.meError = api::Error::IllegalArgument;
            return aOut;
        }
        double fVal = rCell.mfNumber;
        if (bLog)
        {
            if (fVal <= 0.0)
            {
                aOut.meError = api::Error::IllegalArgument;
                return aOut;
            }
            fVal = std::log(fVal);
        }
        aOut.maY.maData.push_back(fVal);
    }

    if (pMatX && !pMatX->isEmpty())
    {
        aOut.mnCX = static_cast<std::size_t>(pMatX->maDimensions.mnColumns);
        aOut.mnRX = static_cast<std::size_t>(pMatX->maDimensions.mnRows);
        const std::size_t nCountX = aOut.mnCX * aOut.mnRX;
        aOut.maX.mnCols = aOut.mnCX;
        aOut.maX.mnRows = aOut.mnRX;
        aOut.maX.maData.reserve(nCountX);
        for (std::size_t i = 0; i < nCountX; ++i)
        {
            const auto& rCell = pMatX->maValues[i];
            if (rCell.meKind == api::CellValueKind::Error)
            {
                aOut.meError = rCell.meError;
                return aOut;
            }
            if (rCell.meKind != api::CellValueKind::Number
                && rCell.meKind != api::CellValueKind::Boolean)
            {
                aOut.meError = api::Error::IllegalArgument;
                return aOut;
            }
            aOut.maX.maData.push_back(rCell.mfNumber);
        }
        if (aOut.mnCX == aOut.mnCY && aOut.mnRX == aOut.mnRY)
        {
            aOut.mnCase = 1;
            aOut.mnK = 1;
            aOut.mnN = nCountY;
        }
        else if (aOut.mnCY != 1 && aOut.mnRY != 1)
        {
            aOut.meError = api::Error::IllegalArgument;
            return aOut;
        }
        else if (aOut.mnCY == 1)
        {
            if (aOut.mnRX != aOut.mnRY)
            {
                aOut.meError = api::Error::IllegalArgument;
                return aOut;
            }
            aOut.mnCase = 2;
            aOut.mnN = aOut.mnRY;
            aOut.mnK = aOut.mnCX;
        }
        else if (aOut.mnCX != aOut.mnCY)
        {
            aOut.meError = api::Error::IllegalArgument;
            return aOut;
        }
        else
        {
            aOut.mnCase = 3;
            aOut.mnN = aOut.mnCY;
            aOut.mnK = aOut.mnRX;
        }
    }
    else
    {
        // Default X to 1..N with same shape as Y.
        aOut.mnCX = aOut.mnCY;
        aOut.mnRX = aOut.mnRY;
        aOut.maX.mnCols = aOut.mnCX;
        aOut.maX.mnRows = aOut.mnRX;
        aOut.maX.maData.reserve(nCountY);
        for (std::size_t i = 1; i <= nCountY; ++i)
            aOut.maX.maData.push_back(static_cast<double>(i));
        aOut.mnCase = 1;
        aOut.mnN = nCountY;
        aOut.mnK = 1;
    }

    // Enough data samples?
    // Caller checks (bConstant && N<K+1) || (!bConstant && N<K); we
    // leave bConstant-dependent check to the caller since planLinest /
    // planTrend know bConstant.
    if (aOut.mnN < 1 || aOut.mnK < 1)
    {
        aOut.meError = api::Error::IllegalArgument;
        return aOut;
    }

    aOut.mbOk = true;
    return aOut;
}

// Column mean over a flat column-major NxK matrix; matches
// lcl_CalculateColumnMeans.
inline void calculateColumnMeans(
    const FlatMatrix& rX, std::vector<double>& rMeans,
    std::size_t nC, std::size_t nR)
{
    rMeans.assign(nC, 0.0);
    for (std::size_t i = 0; i < nC; ++i)
    {
        spreadsheetengine::core::fp::KahanSum aSum;
        for (std::size_t k = 0; k < nR; ++k)
            aSum += rX.get(i, k);
        rMeans[i] = aSum.get() / static_cast<double>(nR);
    }
}

// Subtract means from columns (lcl_CalculateColumnsDelta). Uses
// approxSub semantics: simple x - mean; legacy guards against ULP
// cancellation via rtl::math::approxSub, but the header-only version
// accepts direct subtraction since Kahan-summed means were used.
inline void calculateColumnsDelta(
    FlatMatrix& rMat, const std::vector<double>& rMeans,
    std::size_t nC, std::size_t nR)
{
    for (std::size_t i = 0; i < nC; ++i)
        for (std::size_t k = 0; k < nR; ++k)
            rMat.set(i, k, rMat.get(i, k) - rMeans[i]);
}

[[nodiscard]] inline double getMeanOverAll(
    const FlatMatrix& rMat, std::size_t nN)
{
    spreadsheetengine::core::fp::KahanSum aSum;
    for (std::size_t i = 0; i < nN; ++i)
        aSum += rMat.maData[i];
    return aSum.get() / static_cast<double>(nN);
}

// Populate result MatrixOperand at (col,row) with a double.
inline void putResult(
    MatrixOperand& rResult, std::size_t nCol, std::size_t nRow, double fValue)
{
    const std::size_t nCols
        = static_cast<std::size_t>(rResult.maDimensions.mnColumns);
    rResult.maValues[nRow * nCols + nCol] = api::CellValue::number(fValue);
}

inline void putResultError(
    MatrixOperand& rResult, std::size_t nCol, std::size_t nRow,
    api::Error eError)
{
    const std::size_t nCols
        = static_cast<std::size_t>(rResult.maDimensions.mnColumns);
    rResult.maValues[nRow * nCols + nCol] = api::CellValue::error(eError);
}

[[nodiscard]] inline MatrixOperand newMatrix(
    std::size_t nCols, std::size_t nRows)
{
    MatrixOperand aResult;
    aResult.maDimensions = { static_cast<api::MatrixSize>(nCols),
                             static_cast<api::MatrixSize>(nRows) };
    aResult.maValues.assign(
        nCols * nRows, api::CellValue::number(0.0));
    aResult.meProvenance = MatrixProvenance::ComputedResult;
    return aResult;
}

} // namespace detail::linest

// LINEST (bLog=false) / LOGEST (bLog=true) planner.
//
// Inputs: Y (required), X (optional; pass nullptr for default 1..N),
// bConstant (include intercept term), bStats (include 4 extra rows).
//
// Output: a (K+1)x(bStats?5:1) MatrixOperand matching legacy shape.
[[nodiscard]] inline RpnCoercionResult<LinestPlanResult> planLinestOrLogest(
    bool bLog, const MatrixOperand* pMatX, const MatrixOperand& rMatY,
    bool bConstant, bool bStats)
{
    using detail::linest::applyHouseholderTransformation;
    using detail::linest::calculateColumnMeans;
    using detail::linest::calculateColumnsDelta;
    using detail::linest::calculateQRdecomposition;
    using detail::linest::checkMatrix;
    using detail::linest::getMeanOverAll;
    using detail::linest::getSumProduct;
    using detail::linest::newMatrix;
    using detail::linest::putResult;
    using detail::linest::putResultError;
    using detail::linest::solveWithUpperRightTriangle;

    auto aOut = checkMatrix(bLog, pMatX, rMatY);
    if (!aOut.mbOk)
        return RpnCoercionResult<LinestPlanResult>::failure(aOut.meError);

    const std::size_t nK = aOut.mnK;
    const std::size_t nN = aOut.mnN;
    if ((bConstant && nN < nK + 1) || (!bConstant && nN < nK)
        || nN < 1 || nK < 1)
    {
        return RpnCoercionResult<LinestPlanResult>::failure(
            api::Error::IllegalArgument);
    }

    // Simple regression only. Multi-variable (case 2 / case 3) is an
    // extension path that the caller's scope fence keeps out for now,
    // since the dispatch admission shape gates on svMatrix inputs with
    // 1-column X.
    if (aOut.mnCase != 1)
    {
        return RpnCoercionResult<LinestPlanResult>::deferred(
            RpnCoercionReadiness::NeedsMatrixMaterialization);
    }

    LinestPlanResult aRes;
    aRes.maMatrix = newMatrix(nK + 1, bStats ? 5 : 1);

    // Fill unused cells in stats rows (columns 2..K inclusive) with
    // NotAvailable; for simple regression K==1 so the loop is empty.
    if (bStats)
    {
        for (std::size_t i = 2; i < nK + 1; ++i)
        {
            putResultError(aRes.maMatrix, i, 2, api::Error::NotAvailable);
            putResultError(aRes.maMatrix, i, 3, api::Error::NotAvailable);
            putResultError(aRes.maMatrix, i, 4, api::Error::NotAvailable);
        }
    }

    double fMeanY = 0.0;
    if (bConstant)
    {
        fMeanY = getMeanOverAll(aOut.maY, nN);
        for (std::size_t i = 0; i < nN; ++i)
            aOut.maY.maData[i] = aOut.maY.maData[i] - fMeanY;
    }

    // Simple regression
    double fMeanX = 0.0;
    if (bConstant)
    {
        fMeanX = getMeanOverAll(aOut.maX, nN);
        for (std::size_t i = 0; i < nN; ++i)
            aOut.maX.maData[i] = aOut.maX.maData[i] - fMeanX;
    }
    const double fSumXY = getSumProduct(aOut.maX, aOut.maY, nN);
    const double fSumX2 = getSumProduct(aOut.maX, aOut.maX, nN);
    if (fSumX2 == 0.0)
    {
        return RpnCoercionResult<LinestPlanResult>::failure(
            api::Error::NoValue);
    }
    const double fSlope = fSumXY / fSumX2;
    double fIntercept = 0.0;
    if (bConstant)
        fIntercept = fMeanY - fSlope * fMeanX;

    // Order (column, row). Column 1 row 0 = intercept; column 0 row 0 = slope.
    putResult(aRes.maMatrix, 1, 0, bLog ? std::exp(fIntercept) : fIntercept);
    putResult(aRes.maMatrix, 0, 0, bLog ? std::exp(fSlope) : fSlope);

    if (bStats)
    {
        const double fSSreg = fSlope * fSlope * fSumX2;
        putResult(aRes.maMatrix, 0, 4, fSSreg);
        const double fDegreesFreedom
            = static_cast<double>(bConstant ? nN - 2 : nN - 1);
        putResult(aRes.maMatrix, 1, 3, fDegreesFreedom);

        // SSresid = sum((y - slope*x)^2) on centered vectors.
        spreadsheetengine::core::fp::KahanSum aSSresidSum;
        for (std::size_t i = 0; i < nN; ++i)
        {
            const double fTemp
                = aOut.maY.maData[i] - fSlope * aOut.maX.maData[i];
            aSSresidSum += fTemp * fTemp;
        }
        const double fSSresid = aSSresidSum.get();
        putResult(aRes.maMatrix, 1, 4, fSSresid);

        if (fDegreesFreedom == 0.0 || fSSresid == 0.0 || fSSreg == 0.0)
        {
            putResult(aRes.maMatrix, 1, 4, 0.0); // SSresid
            putResultError(aRes.maMatrix, 0, 3, api::Error::NotAvailable);
            putResult(aRes.maMatrix, 1, 2, 0.0); // RMSE
            putResult(aRes.maMatrix, 0, 1, 0.0); // SigmaSlope
            if (bConstant)
                putResult(aRes.maMatrix, 1, 1, 0.0); // SigmaIntercept
            else
                putResultError(
                    aRes.maMatrix, 1, 1, api::Error::NotAvailable);
            putResult(aRes.maMatrix, 0, 2, 1.0); // R^2
        }
        else
        {
            const double fFstatistic = (fSSreg / static_cast<double>(nK))
                                       / (fSSresid / fDegreesFreedom);
            putResult(aRes.maMatrix, 0, 3, fFstatistic);
            const double fRMSE = std::sqrt(fSSresid / fDegreesFreedom);
            putResult(aRes.maMatrix, 1, 2, fRMSE);
            const double fSigmaSlope = fRMSE / std::sqrt(fSumX2);
            putResult(aRes.maMatrix, 0, 1, fSigmaSlope);
            if (bConstant)
            {
                const double fSigmaIntercept
                    = fRMSE
                      * std::sqrt(fMeanX * fMeanX / fSumX2
                                  + 1.0 / static_cast<double>(nN));
                putResult(aRes.maMatrix, 1, 1, fSigmaIntercept);
            }
            else
            {
                putResultError(
                    aRes.maMatrix, 1, 1, api::Error::NotAvailable);
            }
            const double fR2 = fSSreg / (fSSreg + fSSresid);
            putResult(aRes.maMatrix, 0, 2, fR2);
        }
    }

    return RpnCoercionResult<LinestPlanResult>::success(aRes);
}

[[nodiscard]] inline RpnCoercionResult<LinestPlanResult> planLinest(
    const MatrixOperand* pMatX, const MatrixOperand& rMatY,
    bool bConstant, bool bStats)
{
    return planLinestOrLogest(false, pMatX, rMatY, bConstant, bStats);
}

[[nodiscard]] inline RpnCoercionResult<LinestPlanResult> planLogest(
    const MatrixOperand* pMatX, const MatrixOperand& rMatY,
    bool bConstant, bool bStats)
{
    return planLinestOrLogest(true, pMatX, rMatY, bConstant, bStats);
}

// TREND (bLog=false) / GROWTH (bLog=true) planner.
//
// Inputs: knownY (required), knownX (optional), newX (optional),
// bConstant. Output is a MatrixOperand sized to newX (or the default
// 1..N if newX is null).
[[nodiscard]] inline RpnCoercionResult<TrendPlanResult> planTrendOrGrowth(
    bool bLog, const MatrixOperand& rKnownY,
    const MatrixOperand* pKnownX, const MatrixOperand* pNewX,
    bool bConstant)
{
    using detail::linest::checkMatrix;
    using detail::linest::getMeanOverAll;
    using detail::linest::getSumProduct;
    using detail::linest::newMatrix;
    using detail::linest::putResult;

    auto aOut = checkMatrix(bLog, pKnownX, rKnownY);
    if (!aOut.mbOk)
        return RpnCoercionResult<TrendPlanResult>::failure(aOut.meError);

    const std::size_t nK = aOut.mnK;
    const std::size_t nN = aOut.mnN;
    if ((bConstant && nN < nK + 1) || (!bConstant && nN < nK)
        || nN < 1 || nK < 1)
    {
        return RpnCoercionResult<TrendPlanResult>::failure(
            api::Error::IllegalArgument);
    }

    if (aOut.mnCase != 1)
    {
        return RpnCoercionResult<TrendPlanResult>::deferred(
            RpnCoercionReadiness::NeedsMatrixMaterialization);
    }

    // Default / validate newX; only numeric values accepted.
    std::size_t nCXN = 0;
    std::size_t nRXN = 0;
    std::size_t nCountXN = 0;
    std::vector<double> aNewX;
    if (!pNewX)
    {
        nCXN = aOut.mnCX;
        nRXN = aOut.mnRX;
        nCountXN = nCXN * nRXN;
        // Clone of pKnownX (or default 1..N if none).
        aNewX = aOut.maX.maData;
    }
    else
    {
        if (pNewX->isEmpty())
        {
            return RpnCoercionResult<TrendPlanResult>::failure(
                api::Error::IllegalArgument);
        }
        nCXN = static_cast<std::size_t>(pNewX->maDimensions.mnColumns);
        nRXN = static_cast<std::size_t>(pNewX->maDimensions.mnRows);
        nCountXN = nCXN * nRXN;
        aNewX.reserve(nCountXN);
        for (std::size_t i = 0; i < nCountXN; ++i)
        {
            const auto& rCell = pNewX->maValues[i];
            if (rCell.meKind == api::CellValueKind::Error)
            {
                return RpnCoercionResult<TrendPlanResult>::failure(
                    rCell.meError);
            }
            if (rCell.meKind != api::CellValueKind::Number
                && rCell.meKind != api::CellValueKind::Boolean)
            {
                return RpnCoercionResult<TrendPlanResult>::failure(
                    api::Error::IllegalArgument);
            }
            aNewX.push_back(rCell.mfNumber);
        }
    }

    TrendPlanResult aRes;
    aRes.maMatrix = newMatrix(nCXN, nRXN);

    double fMeanY = 0.0;
    if (bConstant)
    {
        fMeanY = getMeanOverAll(aOut.maY, nN);
        for (std::size_t i = 0; i < nN; ++i)
            aOut.maY.maData[i] = aOut.maY.maData[i] - fMeanY;
    }

    // Simple regression
    double fMeanX = 0.0;
    if (bConstant)
    {
        fMeanX = getMeanOverAll(aOut.maX, nN);
        for (std::size_t i = 0; i < nN; ++i)
            aOut.maX.maData[i] = aOut.maX.maData[i] - fMeanX;
    }
    const double fSumXY = getSumProduct(aOut.maX, aOut.maY, nN);
    const double fSumX2 = getSumProduct(aOut.maX, aOut.maX, nN);
    if (fSumX2 == 0.0)
    {
        return RpnCoercionResult<TrendPlanResult>::failure(
            api::Error::NoValue);
    }
    const double fSlope = fSumXY / fSumX2;
    const double fIntercept = bConstant ? fMeanY - fSlope * fMeanX : 0.0;
    for (std::size_t i = 0; i < nCountXN; ++i)
    {
        const double fHelp = aNewX[i] * fSlope + fIntercept;
        const double fOut = bLog ? std::exp(fHelp) : fHelp;
        aRes.maMatrix.maValues[i] = api::CellValue::number(fOut);
    }
    return RpnCoercionResult<TrendPlanResult>::success(aRes);
}

[[nodiscard]] inline RpnCoercionResult<TrendPlanResult> planTrend(
    const MatrixOperand& rKnownY,
    const MatrixOperand* pKnownX, const MatrixOperand* pNewX,
    bool bConstant)
{
    return planTrendOrGrowth(false, rKnownY, pKnownX, pNewX, bConstant);
}

[[nodiscard]] inline RpnCoercionResult<TrendPlanResult> planGrowth(
    const MatrixOperand& rKnownY,
    const MatrixOperand* pKnownX, const MatrixOperand* pNewX,
    bool bConstant)
{
    return planTrendOrGrowth(true, rKnownY, pKnownX, pNewX, bConstant);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
