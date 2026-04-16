/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/MathMatrix.hxx>

#include <cmath>
#include <vector>

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

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
