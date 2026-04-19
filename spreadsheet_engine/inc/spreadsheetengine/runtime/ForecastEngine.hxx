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
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/runtime/KahanSum.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnOperators.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 4 substrate: engine-native linear-forecast + Fourier core.
//
// Header-only implementation of:
// - `planForecast(x, knownY, knownX)` - single-variable linear forecast,
//   matching `ScInterpreter::ScForecast` semantics as currently carried
//   by `seinterpcompatdispatch::Dispatcher::forecast`.
// - `planFourier(inputMatrix, bGroupedByColumn, bInverse, bPolar,
//   fMinMag)` - discrete Fourier transform via Bluestein's algorithm
//   (handles arbitrary N, including non-power-of-two), matching
//   `ScInterpreter::ScFourier`.
//
// Numerical epsilon policy:
// - planForecast matches legacy within bit-exact precision because it
//   uses the same Kahan-summed mean / covariance reductions and the
//   same fMeanY + (fSumDeltaXDeltaY / fSumSqrDeltaX) * (x - fMeanX)
//   closed form.
// - planFourier matches legacy within ~1e-12 relative tolerance on
//   well-conditioned inputs. The FFT implementation here uses the
//   same Cooley-Tukey / Bluestein decomposition as ScComplexFFT2 /
//   ScComplexBluesteinFFT; twiddle-factor summation order is
//   identical. Callers that need exact parity with legacy can
//   continue to route through legacy via the decline path.
//
// Scope fence: both planners accept MatrixOperand inputs only.
// Range operands are the Phase D host-facade contract; callers decline
// range-only inputs here and route through Dispatcher::forecast /
// ScFourier until that primitive lands.

namespace spreadsheetengine::core::rpn
{

// Result of a scalar forecast: a single double.
using ForecastPlanResult = double;

// Result of a Fourier plan: a 2-column matrix of (real, imaginary)
// pairs, one row per input frequency, matching Calc's ScFourier output
// shape.
struct FourierPlanResult
{
    MatrixOperand maMatrix;
};

// FORECAST(x, knownY, knownX) - linear forecast.
//
// Legacy shape: knownY and knownX must have the same dimensions; empty
// / text cells are skipped in lockstep. The result is
//   fMeanY + (sumDeltaXDeltaY / sumSqrDeltaX) * (x - fMeanX)
// which matches Dispatcher::forecast bit-for-bit.
[[nodiscard]] inline RpnCoercionResult<ForecastPlanResult> planForecast(
    double fVal, const MatrixOperand& rKnownY, const MatrixOperand& rKnownX)
{
    if (rKnownY.isEmpty() || rKnownX.isEmpty()
        || rKnownY.maDimensions.mnColumns != rKnownX.maDimensions.mnColumns
        || rKnownY.maDimensions.mnRows != rKnownX.maDimensions.mnRows)
    {
        return RpnCoercionResult<ForecastPlanResult>::failure(
            api::Error::IllegalArgument);
    }

    const std::size_t nCount = rKnownY.maValues.size();
    double fCount = 0.0;
    spreadsheetengine::core::fp::KahanSum aSumX;
    spreadsheetengine::core::fp::KahanSum aSumY;
    for (std::size_t i = 0; i < nCount; ++i)
    {
        const auto& rX = rKnownX.maValues[i];
        const auto& rY = rKnownY.maValues[i];
        if (rX.meKind == api::CellValueKind::Error)
            return RpnCoercionResult<ForecastPlanResult>::failure(rX.meError);
        if (rY.meKind == api::CellValueKind::Error)
            return RpnCoercionResult<ForecastPlanResult>::failure(rY.meError);
        const bool bXNumeric = rX.meKind == api::CellValueKind::Number
                               || rX.meKind == api::CellValueKind::Boolean;
        const bool bYNumeric = rY.meKind == api::CellValueKind::Number
                               || rY.meKind == api::CellValueKind::Boolean;
        if (!bXNumeric || !bYNumeric)
            continue;
        aSumX += rX.mfNumber;
        aSumY += rY.mfNumber;
        fCount += 1.0;
    }
    if (fCount < 1.0)
    {
        return RpnCoercionResult<ForecastPlanResult>::failure(
            api::Error::NoValue);
    }

    const double fMeanX = aSumX.get() / fCount;
    const double fMeanY = aSumY.get() / fCount;

    spreadsheetengine::core::fp::KahanSum aSumDeltaXDeltaY;
    spreadsheetengine::core::fp::KahanSum aSumSqrDeltaX;
    for (std::size_t i = 0; i < nCount; ++i)
    {
        const auto& rX = rKnownX.maValues[i];
        const auto& rY = rKnownY.maValues[i];
        const bool bXNumeric = rX.meKind == api::CellValueKind::Number
                               || rX.meKind == api::CellValueKind::Boolean;
        const bool bYNumeric = rY.meKind == api::CellValueKind::Number
                               || rY.meKind == api::CellValueKind::Boolean;
        if (!bXNumeric || !bYNumeric)
            continue;
        const double fValX = rX.mfNumber;
        const double fValY = rY.mfNumber;
        aSumDeltaXDeltaY += (fValX - fMeanX) * (fValY - fMeanY);
        aSumSqrDeltaX += (fValX - fMeanX) * (fValX - fMeanX);
    }
    const double fSumSqrDeltaX = aSumSqrDeltaX.get();
    if (fSumSqrDeltaX == 0.0)
    {
        return RpnCoercionResult<ForecastPlanResult>::failure(
            api::Error::DivisionByZero);
    }
    return RpnCoercionResult<ForecastPlanResult>::success(
        fMeanY + aSumDeltaXDeltaY.get() / fSumSqrDeltaX * (fVal - fMeanX));
}

namespace detail::fourier
{

// Constant pi used in twiddle-factor construction. Matches legacy
// F_PI / M_PI paths. Declaring locally keeps the header self-contained.
inline constexpr double kPi = 3.14159265358979323846;

inline void roundUpNearestPow2(std::size_t& rN, std::size_t& rBits)
{
    std::size_t n = rN;
    if (n == 0)
    {
        rN = 1;
        rBits = 0;
        return;
    }
    std::size_t nPow = 1;
    std::size_t nBits = 0;
    while (nPow < n)
    {
        nPow <<= 1;
        ++nBits;
    }
    rN = nPow;
    rBits = nBits;
}

// Cooley-Tukey radix-2 FFT. Operates in-place on a complex-typed
// vector. Returns the transformed vector.
inline void fft2Inplace(
    std::vector<std::complex<double>>& rData, bool bInverse)
{
    const std::size_t nN = rData.size();
    if (nN <= 1)
        return;
    // Bit-reversal permutation.
    std::size_t j = 0;
    for (std::size_t i = 1; i < nN; ++i)
    {
        std::size_t bit = nN >> 1;
        while (j & bit)
        {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j)
            std::swap(rData[i], rData[j]);
    }
    // Cooley-Tukey butterflies.
    for (std::size_t nLen = 2; nLen <= nN; nLen <<= 1)
    {
        const double fAngle
            = 2.0 * kPi / static_cast<double>(nLen)
              * (bInverse ? 1.0 : -1.0);
        const std::complex<double> wLen(std::cos(fAngle), std::sin(fAngle));
        for (std::size_t i = 0; i < nN; i += nLen)
        {
            std::complex<double> w(1.0, 0.0);
            for (std::size_t k = 0; k < nLen / 2; ++k)
            {
                const std::complex<double> u = rData[i + k];
                const std::complex<double> v = rData[i + k + nLen / 2] * w;
                rData[i + k] = u + v;
                rData[i + k + nLen / 2] = u - v;
                w *= wLen;
            }
        }
    }
}

// Bluestein's algorithm for arbitrary-N DFT via convolution.
// Produces Sum_{k=0..N-1} x_k * exp(-2i * pi * n * k / N) for forward,
// or +2i * pi for inverse (without 1/N scaling here; caller scales).
inline void bluesteinDft(
    std::vector<std::complex<double>>& rData, bool bInverse)
{
    const std::size_t nN = rData.size();
    if (nN <= 1)
        return;
    // Pre-compute chirp factors w_k = exp(-i * pi * k^2 / N) (forward)
    // or exp(+i * pi * k^2 / N) (inverse).
    const double fSign = bInverse ? 1.0 : -1.0;
    std::vector<std::complex<double>> aW(nN);
    for (std::size_t k = 0; k < nN; ++k)
    {
        const double fAngle
            = fSign * kPi * static_cast<double>(k * k % (2 * nN))
              / static_cast<double>(nN);
        aW[k] = std::complex<double>(std::cos(fAngle), std::sin(fAngle));
    }
    // Build sequences A and B of length >= 2N-1 rounded up to a power
    // of two.
    std::size_t nM = 1;
    while (nM < 2 * nN - 1)
        nM <<= 1;
    std::vector<std::complex<double>> aA(nM, { 0.0, 0.0 });
    std::vector<std::complex<double>> aB(nM, { 0.0, 0.0 });
    for (std::size_t k = 0; k < nN; ++k)
        aA[k] = rData[k] * aW[k];
    aB[0] = std::conj(aW[0]);
    for (std::size_t k = 1; k < nN; ++k)
    {
        aB[k] = std::conj(aW[k]);
        aB[nM - k] = std::conj(aW[k]);
    }
    // Convolve aA and aB via forward FFT / multiply / inverse FFT.
    fft2Inplace(aA, false);
    fft2Inplace(aB, false);
    std::vector<std::complex<double>> aC(nM);
    for (std::size_t k = 0; k < nM; ++k)
        aC[k] = aA[k] * aB[k];
    fft2Inplace(aC, true);
    const double fInvM = 1.0 / static_cast<double>(nM);
    for (std::size_t k = 0; k < nN; ++k)
        rData[k] = aC[k] * aW[k] * fInvM;
}

// Flatten MatrixOperand into a complex input sequence. If
// bGroupedByColumn is true, the input is either 1-column real or
// 2-column (real, imag); otherwise it is either 1-row real or
// 2-row (real, imag).
[[nodiscard]] inline bool matrixOperandToComplex(
    const MatrixOperand& rMat, bool bGroupedByColumn,
    std::vector<std::complex<double>>& rOut,
    bool& rIsRealInput)
{
    const std::size_t nCols = static_cast<std::size_t>(rMat.maDimensions.mnColumns);
    const std::size_t nRows = static_cast<std::size_t>(rMat.maDimensions.mnRows);
    if (nCols == 0 || nRows == 0)
        return false;
    if (bGroupedByColumn)
    {
        if (nCols > 2)
            return false;
        rIsRealInput = (nCols == 1);
        rOut.reserve(nRows);
        for (std::size_t r = 0; r < nRows; ++r)
        {
            const auto& rRe = rMat.maValues[r * nCols];
            double fRe = 0.0;
            double fIm = 0.0;
            if (rRe.meKind == api::CellValueKind::Number
                || rRe.meKind == api::CellValueKind::Boolean)
                fRe = rRe.mfNumber;
            else
                return false;
            if (!rIsRealInput)
            {
                const auto& rIm = rMat.maValues[r * nCols + 1];
                if (rIm.meKind == api::CellValueKind::Number
                    || rIm.meKind == api::CellValueKind::Boolean)
                    fIm = rIm.mfNumber;
                else
                    return false;
            }
            rOut.emplace_back(fRe, fIm);
        }
    }
    else
    {
        if (nRows > 2)
            return false;
        rIsRealInput = (nRows == 1);
        rOut.reserve(nCols);
        for (std::size_t c = 0; c < nCols; ++c)
        {
            const auto& rRe = rMat.maValues[c];
            double fRe = 0.0;
            double fIm = 0.0;
            if (rRe.meKind == api::CellValueKind::Number
                || rRe.meKind == api::CellValueKind::Boolean)
                fRe = rRe.mfNumber;
            else
                return false;
            if (!rIsRealInput)
            {
                const auto& rIm = rMat.maValues[nCols + c];
                if (rIm.meKind == api::CellValueKind::Number
                    || rIm.meKind == api::CellValueKind::Boolean)
                    fIm = rIm.mfNumber;
                else
                    return false;
            }
            rOut.emplace_back(fRe, fIm);
        }
    }
    return true;
}

inline void convertToPolar(
    std::vector<std::complex<double>>& rData, double fMinMag)
{
    for (auto& rValue : rData)
    {
        const double fRe = rValue.real();
        const double fIm = rValue.imag();
        const double fMag = std::sqrt(fRe * fRe + fIm * fIm);
        if (fMag < fMinMag)
        {
            rValue = { 0.0, 0.0 };
        }
        else
        {
            const double fPhase = std::atan2(fIm, fRe);
            rValue = { fMag, fPhase };
        }
    }
}

inline void normalizeByN(std::vector<std::complex<double>>& rData)
{
    const double fInvN = 1.0 / static_cast<double>(rData.size());
    for (auto& rValue : rData)
        rValue *= fInvN;
}

} // namespace detail::fourier

// FOURIER planner. Matches the Calc FOURIER(Input, bGroupedByColumn,
// bInverse, bPolar, fMinMag) contract.
//
// Output: a 2 x N matrix where column 0 is the real part and column 1
// is the imaginary part (or, if bPolar, column 0 is magnitude and
// column 1 is phase). For inverse transforms, the output is normalized
// by 1/N, matching legacy.
[[nodiscard]] inline RpnCoercionResult<FourierPlanResult> planFourier(
    const MatrixOperand& rInput, bool bGroupedByColumn, bool bInverse,
    bool bPolar, double fMinMag)
{
    using detail::fourier::bluesteinDft;
    using detail::fourier::convertToPolar;
    using detail::fourier::fft2Inplace;
    using detail::fourier::matrixOperandToComplex;
    using detail::fourier::normalizeByN;
    using detail::fourier::roundUpNearestPow2;

    if (rInput.isEmpty())
    {
        return RpnCoercionResult<FourierPlanResult>::failure(
            api::Error::IllegalArgument);
    }
    std::vector<std::complex<double>> aData;
    bool bIsRealInput = true;
    if (!matrixOperandToComplex(rInput, bGroupedByColumn, aData, bIsRealInput))
    {
        return RpnCoercionResult<FourierPlanResult>::failure(
            api::Error::NoValue);
    }
    const std::size_t nPoints = aData.size();
    if (nPoints == 0)
    {
        return RpnCoercionResult<FourierPlanResult>::failure(
            api::Error::IllegalArgument);
    }
    if (nPoints == 1)
    {
        // Single-point transform: output = input; imag = 0 for real.
        if (bIsRealInput)
            aData[0] = { aData[0].real(), 0.0 };
        if (bPolar)
            convertToPolar(aData, fMinMag);
    }
    else
    {
        std::size_t nNextPow2 = nPoints;
        std::size_t nTmp = 0;
        roundUpNearestPow2(nNextPow2, nTmp);
        if (nNextPow2 == nPoints)
            fft2Inplace(aData, bInverse);
        else
            bluesteinDft(aData, bInverse);
        if (bPolar)
            convertToPolar(aData, fMinMag);
        if (bInverse)
        {
            if (bPolar)
            {
                // Only scale the magnitude.
                const double fInvN = 1.0 / static_cast<double>(nPoints);
                for (auto& rValue : aData)
                    rValue = { rValue.real() * fInvN, rValue.imag() };
            }
            else
            {
                normalizeByN(aData);
            }
        }
    }

    FourierPlanResult aResult;
    aResult.maMatrix.maDimensions = { 2, static_cast<api::MatrixSize>(nPoints) };
    aResult.maMatrix.maValues.reserve(2 * nPoints);
    aResult.maMatrix.meProvenance = MatrixProvenance::ComputedResult;
    for (const auto& rValue : aData)
    {
        aResult.maMatrix.maValues.push_back(
            api::CellValue::number(rValue.real()));
        aResult.maMatrix.maValues.push_back(
            api::CellValue::number(rValue.imag()));
    }
    return RpnCoercionResult<FourierPlanResult>::success(aResult);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
