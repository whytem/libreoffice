/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::api
{

enum class Error
{
    None,
    IllegalArgument,
    DivisionByZero,
    Domain,
    StringOverflow,
    NoValue,
    NoConvergence
};

template <typename T> struct ValueResult
{
    T maValue {};
    Error meError = Error::None;

    [[nodiscard]] constexpr bool ok() const { return meError == Error::None; }

    constexpr explicit operator bool() const { return ok(); }

    static constexpr ValueResult success(const T& rValue)
    {
        return { rValue, Error::None };
    }

    static constexpr ValueResult failure(Error eError)
    {
        return { T {}, eError };
    }
};

} // namespace spreadsheetengine::api

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
