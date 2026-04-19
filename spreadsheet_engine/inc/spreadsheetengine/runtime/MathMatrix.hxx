/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>
#include <vector>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::math
{

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<double> evaluateMatrixDeterminant(
    const std::vector<double>& rValues, std::size_t nDimension);

// Matrix-matrix product. `rLeft` is (nLeftRows x nInner) and `rRight` is
// (nInner x nRightColumns); both are dense row-major. Result is
// (nLeftRows x nRightColumns), row-major.
//
// Summation order matches legacy `ScInterpreter::ScMatMult`
// (sc/source/core/tool/interpr5.cxx): for each output cell (i, j),
// accumulate k = 0 ... nInner - 1 in ascending order using a Kahan-
// style compensated sum so the engine reproduces the bit pattern
// produced by the legacy `KahanSum` accumulator. Mismatched dimensions
// yield `Error::IllegalArgument`.
SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<std::vector<double>> evaluateMatrixMultiply(
    const std::vector<double>& rLeft, std::size_t nLeftRows, std::size_t nInner,
    const std::vector<double>& rRight, std::size_t nRightColumns);

// Matrix inverse via LUP decomposition. Input is a square dense
// row-major matrix of side `nDimension`. Output is the same shape.
//
// Singular-matrix policy matches legacy `ScMatInv` exactly: the LUP
// decomposition rejects a row whose absolute maximum element is `0.0`
// (returns `0` from `lcl_LUP_decompose`) and, after decomposition,
// any diagonal element that equals exactly `0.0` also triggers
// singularity. Both cases surface as `Error::IllegalArgument`, which
// Calc's PushIllegalArgument maps to `FormulaError::IllegalArgument`
// (`#VALUE!`).
//
// No separate epsilon is applied. The legacy code does not scale
// before the equality-to-zero test, so neither does the engine. This
// keeps parity for the narrow regime where a near-singular but
// non-zero pivot still produces an inverse in legacy. Callers who
// need a tighter condition check must perform it above this layer.
SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<std::vector<double>> evaluateMatrixInverse(
    const std::vector<double>& rValues, std::size_t nDimension);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
