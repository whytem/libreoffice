/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Types.hxx>

#include <optional>

#include <spreadsheetengine/api/Grammar.hxx>
#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/detail/TokenModel.hxx>

namespace spreadsheetengine::detail::compiler
{

enum class ExtendedErrorDetection : std::uint8_t
{
    None = 0,
    NameBreak,
    NameNoBreak,
};

struct CompileContext
{
    api::Grammar maGrammar;
    api::CellAddress maBaseAddress;
    bool mbForPersistence = false;
    bool mbAllowExternalReferences = true;
    bool mbComputeImplicitIntersection = false;
    bool mbMatrixFormula = false;
    ExtendedErrorDetection meExtendedErrorDetection = ExtendedErrorDetection::None;

    [[nodiscard]] constexpr bool operator==(const CompileContext& rOther) const = default;
};

class NameResolver
{
public:
    virtual ~NameResolver() = default;

    [[nodiscard]] virtual std::optional<token::NameData> lookupRangeName(
        api::StringView rName, std::optional<api::SheetId> onSheet,
        const CompileContext& rContext) const = 0;
};

class DatabaseRangeResolver
{
public:
    virtual ~DatabaseRangeResolver() = default;

    [[nodiscard]] virtual std::optional<token::DatabaseRangeData> lookupDatabaseRange(
        api::StringView rName, const CompileContext& rContext) const = 0;
};

class TableRefResolver
{
public:
    virtual ~TableRefResolver() = default;

    [[nodiscard]] virtual std::optional<token::TableRefData> lookupTableReference(
        api::StringView rTableName, api::StringView rItemName,
        const CompileContext& rContext) const = 0;
};

class ColRowNameResolver
{
public:
    virtual ~ColRowNameResolver() = default;

    [[nodiscard]] virtual std::optional<api::refdata::SingleRefData> lookupColRowName(
        api::StringView rName, const CompileContext& rContext) const = 0;
};

class ExternalNameResolver
{
public:
    virtual ~ExternalNameResolver() = default;

    [[nodiscard]] virtual std::optional<token::ExternalNameData> lookupExternalName(
        api::StringView rSymbol, const CompileContext& rContext) const = 0;
};

struct CompileHosts
{
    const NameResolver* mpNameResolver = nullptr;
    const DatabaseRangeResolver* mpDatabaseRangeResolver = nullptr;
    const TableRefResolver* mpTableRefResolver = nullptr;
    const ColRowNameResolver* mpColRowNameResolver = nullptr;
    const ExternalNameResolver* mpExternalNameResolver = nullptr;
};

} // namespace spreadsheetengine::detail::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
