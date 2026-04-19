/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/MathMatrix.hxx>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include <spreadsheetengine/runtime/KahanSum.hxx>

namespace spreadsheetengine::core::math
{
namespace
{

[[nodiscard]] int lupDecompose(std::vector<double>& rMatrix, std::size_t nDimension,
    std::vector<std::size_t>& rPermutation)
{
    int nSign = 1;
    std::vector<double> aScale(nDimension);
    for (std::size_t nRow = 0; nRow < nDimension; ++nRow)
    {
        double fMax = 0.0;
        for (std::size_t nColumn = 0; nColumn < nDimension; ++nColumn)
        {
            const double fValue = std::fabs(rMatrix[nRow * nDimension + nColumn]);
            if (fMax < fValue)
                fMax = fValue;
        }
        if (fMax == 0.0)
            return 0;
        aScale[nRow] = 1.0 / fMax;
        rPermutation[nRow] = nRow;
    }

    for (std::size_t nPivot = 0; nPivot + 1 < nDimension; ++nPivot)
    {
        double fMax = 0.0;
        std::size_t nPivotRow = nPivot;
        for (std::size_t nRow = nPivot; nRow < nDimension; ++nRow)
        {
            const double fCandidate
                = aScale[nRow] * std::fabs(rMatrix[nRow * nDimension + nPivot]);
            if (fMax < fCandidate)
            {
                fMax = fCandidate;
                nPivotRow = nRow;
            }
        }
        if (fMax == 0.0)
            return 0;

        if (nPivot != nPivotRow)
        {
            std::swap(rPermutation[nPivot], rPermutation[nPivotRow]);
            std::swap(aScale[nPivot], aScale[nPivotRow]);
            for (std::size_t nColumn = 0; nColumn < nDimension; ++nColumn)
            {
                std::swap(rMatrix[nPivot * nDimension + nColumn],
                    rMatrix[nPivotRow * nDimension + nColumn]);
            }
            nSign = -nSign;
        }

        const double fPivot = rMatrix[nPivot * nDimension + nPivot];
        for (std::size_t nRow = nPivot + 1; nRow < nDimension; ++nRow)
        {
            const std::size_t nLeftIndex = nRow * nDimension + nPivot;
            const double fNumerator = rMatrix[nLeftIndex];
            rMatrix[nLeftIndex] = fNumerator / fPivot;
            for (std::size_t nColumn = nPivot + 1; nColumn < nDimension; ++nColumn)
            {
                const std::size_t nIndex = nRow * nDimension + nColumn;
                rMatrix[nIndex]
                    = (rMatrix[nIndex] * fPivot
                        - fNumerator * rMatrix[nPivot * nDimension + nColumn])
                      / fPivot;
            }
        }
    }

    for (std::size_t nIndex = 0; nIndex < nDimension; ++nIndex)
    {
        if (rMatrix[nIndex * nDimension + nIndex] == 0.0)
            return 0;
    }

    return nSign;
}

} // namespace

api::ValueResult<double> evaluateMatrixDeterminant(
    const std::vector<double>& rValues, std::size_t nDimension)
{
    if (nDimension == 0
        || rValues.size() != nDimension * nDimension)
    {
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    }

    std::vector<double> aMatrix(rValues);
    std::vector<std::size_t> aPermutation(nDimension);
    const int nSign = lupDecompose(aMatrix, nDimension, aPermutation);
    if (nSign == 0)
        return api::ValueResult<double>::success(0.0);

    double fDeterminant = static_cast<double>(nSign);
    for (std::size_t nIndex = 0; nIndex < nDimension; ++nIndex)
        fDeterminant *= aMatrix[nIndex * nDimension + nIndex];

    return api::ValueResult<double>::success(fDeterminant);
}

api::ValueResult<std::vector<double>> evaluateMatrixMultiply(
    const std::vector<double>& rLeft, std::size_t nLeftRows, std::size_t nInner,
    const std::vector<double>& rRight, std::size_t nRightColumns)
{
    if (nLeftRows == 0 || nInner == 0 || nRightColumns == 0
        || rLeft.size() != nLeftRows * nInner
        || rRight.size() != nInner * nRightColumns)
    {
        return api::ValueResult<std::vector<double>>::failure(api::Error::IllegalArgument);
    }

    // Summation order matches legacy `ScInterpreter::ScMatMult`
    // (sc/source/core/tool/interpr5.cxx): outer loop on result row i,
    // then result column j, then inner index k ascending. Compensated
    // summation uses the same Kahan accumulator the legacy path uses.
    std::vector<double> aResult(nLeftRows * nRightColumns, 0.0);
    for (std::size_t i = 0; i < nLeftRows; ++i)
    {
        for (std::size_t j = 0; j < nRightColumns; ++j)
        {
            fp::KahanSum aSum;
            for (std::size_t k = 0; k < nInner; ++k)
            {
                aSum.add(rLeft[i * nInner + k] * rRight[k * nRightColumns + j]);
            }
            aResult[i * nRightColumns + j] = aSum.get();
        }
    }

    return api::ValueResult<std::vector<double>>::success(std::move(aResult));
}

namespace
{

// Parallel to legacy lcl_LUP_solve in interpr5.cxx: solve Ax = b with a
// LUP-decomposed LU matrix. Accepts the same row-major flat layout as
// lupDecompose above.
void lupSolve(const std::vector<double>& rLU, std::size_t nDimension,
    const std::vector<std::size_t>& rPermutation,
    const std::vector<double>& rRhs, std::vector<double>& rOut)
{
    std::size_t nFirst = static_cast<std::size_t>(-1);
    for (std::size_t i = 0; i < nDimension; ++i)
    {
        fp::KahanSum aSum(rRhs[rPermutation[i]]);
        if (nFirst != static_cast<std::size_t>(-1))
        {
            for (std::size_t j = nFirst; j < i; ++j)
                aSum.subtract(rLU[i * nDimension + j] * rOut[j]);
        }
        else if (aSum.get() != 0.0)
        {
            nFirst = i;
        }
        rOut[i] = aSum.get();
    }
    for (std::size_t i = nDimension; i-- > 0;)
    {
        fp::KahanSum aSum(rOut[i]);
        for (std::size_t j = i + 1; j < nDimension; ++j)
            aSum.subtract(rLU[i * nDimension + j] * rOut[j]);
        rOut[i] = aSum.get() / rLU[i * nDimension + i];
    }
}

} // namespace

api::ValueResult<std::vector<double>> evaluateMatrixInverse(
    const std::vector<double>& rValues, std::size_t nDimension)
{
    if (nDimension == 0
        || rValues.size() != nDimension * nDimension)
    {
        return api::ValueResult<std::vector<double>>::failure(api::Error::IllegalArgument);
    }

    // Singular-matrix threshold is identical to legacy `ScMatInv`:
    // `lupDecompose` returns 0 for any row whose absolute maximum is
    // exactly zero, or for a zero on the diagonal after decomposition.
    // No epsilon widening: matches `interpr5.cxx::lcl_LUP_decompose`.
    std::vector<double> aLU(rValues);
    std::vector<std::size_t> aPermutation(nDimension);
    const int nSign = lupDecompose(aLU, nDimension, aPermutation);
    if (nSign == 0)
    {
        return api::ValueResult<std::vector<double>>::failure(api::Error::IllegalArgument);
    }

    std::vector<double> aResult(nDimension * nDimension, 0.0);
    std::vector<double> aRhs(nDimension, 0.0);
    std::vector<double> aSolution(nDimension, 0.0);
    // Solve the linear system for each column j of the identity matrix
    // — matches legacy column-by-column iteration in ScMatInv.
    for (std::size_t j = 0; j < nDimension; ++j)
    {
        std::fill(aRhs.begin(), aRhs.end(), 0.0);
        aRhs[j] = 1.0;
        lupSolve(aLU, nDimension, aPermutation, aRhs, aSolution);
        for (std::size_t i = 0; i < nDimension; ++i)
            aResult[i * nDimension + j] = aSolution[i];
    }

    return api::ValueResult<std::vector<double>>::success(std::move(aResult));
}

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
