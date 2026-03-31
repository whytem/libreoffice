/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/math.hxx>

#include <spreadsheetengine/api/Rounding.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::RoundingMode toApiRoundingMode(rtl_math_RoundingMode eMode)
{
    switch (eMode)
    {
        case rtl_math_RoundingMode_Down:
            return spreadsheetengine::api::RoundingMode::Down;
        case rtl_math_RoundingMode_Up:
            return spreadsheetengine::api::RoundingMode::Up;
        default:
            return spreadsheetengine::api::RoundingMode::Corrected;
    }
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
