/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/runtime/ScalarCoercion.hxx>

namespace spreadsheetengine::core::util
{

using spreadsheetengine::core::coercion::uppercaseAscii;
using spreadsheetengine::core::coercion::parseAsciiDouble;
using spreadsheetengine::core::coercion::coerceToNumber;
using spreadsheetengine::core::coercion::toWholeNumber;

[[nodiscard]] inline api::ValueResult<double> makeFiniteResult(double fValue)
{
    if (!std::isfinite(fValue))
        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    return api::ValueResult<double>::success(fValue);
}

} // namespace spreadsheetengine::core::util

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
