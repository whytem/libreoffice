/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/CompileHost.hxx>

namespace spreadsheetengine::detail::compiler
{

struct FormulaSource
{
    api::String maFormula;
    api::String maNamespace;

    [[nodiscard]] constexpr bool hasNamespace() const
    {
        return !maNamespace.empty();
    }

    [[nodiscard]] constexpr bool operator==(const FormulaSource& rOther) const = default;
};

struct CompileRequest
{
    FormulaSource maSource;
    CompileContext maContext;
    CompileHosts maHosts;

    [[nodiscard]] constexpr bool operator==(const CompileRequest& rOther) const = default;
};

struct CompileStatus
{
    token::CompiledFormula maFormula;
    sal_uInt16 mnFailureIndex = 0;
    api::String maFailureMessage;
    bool mbUsedLegacyBackend = false;

    explicit operator bool() const { return maFailureMessage.empty(); }
};

[[nodiscard]] constexpr bool hasCompleteHostBundle(const CompileHosts& rHosts)
{
    return rHosts.mpNameResolver && rHosts.mpDatabaseRangeResolver && rHosts.mpTableRefResolver
           && rHosts.mpColRowNameResolver && rHosts.mpExternalNameResolver;
}

} // namespace spreadsheetengine::detail::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
