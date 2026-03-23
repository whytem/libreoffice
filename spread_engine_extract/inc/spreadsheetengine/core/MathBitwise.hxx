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

#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::math
{

SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeBitAnd(double fLeft, double fRight);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeBitOr(double fLeft, double fRight);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeBitXor(double fLeft, double fRight);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeBitLeftShift(double fValue, double fShift);
SPREADSHEETENGINE_DLLPUBLIC std::optional<double> computeBitRightShift(double fValue, double fShift);

} // namespace spreadsheetengine::core::math

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
