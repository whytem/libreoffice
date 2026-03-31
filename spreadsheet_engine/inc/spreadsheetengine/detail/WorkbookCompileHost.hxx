/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <limits>
#include <optional>
#include <vector>

#include <spreadsheetengine/detail/BuiltinExternalNames.hxx>
#include <spreadsheetengine/detail/CompileHost.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>

namespace spreadsheetengine::detail::compiler
{

enum class LookupSupport : std::uint8_t
{
    Supported = 0,
    Unsupported
};

struct WorkbookCompileHostSupport
{
    LookupSupport meRangeNames = LookupSupport::Supported;
    LookupSupport meDatabaseRanges = LookupSupport::Unsupported;
    LookupSupport meTableRefs = LookupSupport::Unsupported;
    LookupSupport meColRowNames = LookupSupport::Unsupported;
    LookupSupport meExternalNames = LookupSupport::Supported;
};

struct WorkbookExternalNameBinding
{
    api::String maSymbol;
    token::ExternalNameData maData;
};

struct WorkbookCompileHostOptions
{
    WorkbookCompileHostSupport maSupport;
    std::vector<WorkbookExternalNameBinding> maExternalNames;
};

inline constexpr api::Grammar kDefaultWorkbookCompileGrammar {
    api::FormulaLanguage::Odff,
    api::AddressConvention::OdfA1,
    false,
};

[[nodiscard]] inline CompileContext makeWorkbookCompileContext(
    const api::CellAddress& rBaseAddress, api::Grammar aGrammar = kDefaultWorkbookCompileGrammar,
    bool bForPersistence = false, bool bAllowExternalReferences = false,
    bool bComputeImplicitIntersection = false, bool bMatrixFormula = false,
    ExtendedErrorDetection eExtendedErrorDetection = ExtendedErrorDetection::None)
{
    CompileContext aContext;
    aContext.maGrammar = aGrammar;
    aContext.maBaseAddress = rBaseAddress;
    aContext.mbForPersistence = bForPersistence;
    aContext.mbAllowExternalReferences = bAllowExternalReferences;
    aContext.mbComputeImplicitIntersection = bComputeImplicitIntersection;
    aContext.mbMatrixFormula = bMatrixFormula;
    aContext.meExtendedErrorDetection = eExtendedErrorDetection;
    return aContext;
}

[[nodiscard]] inline std::optional<CompileContext> makeWorkbookCompileContext(
    const core::workbook::Workbook& rWorkbook, api::StringView rSheetName,
    api::ColumnIndex nColumn, api::RowIndex nRow,
    api::Grammar aGrammar = kDefaultWorkbookCompileGrammar, bool bForPersistence = false,
    bool bAllowExternalReferences = false, bool bComputeImplicitIntersection = false,
    bool bMatrixFormula = false,
    ExtendedErrorDetection eExtendedErrorDetection = ExtendedErrorDetection::None)
{
    const auto oSheetId = rWorkbook.findSheetId(rSheetName);
    if (!oSheetId)
        return std::nullopt;

    return makeWorkbookCompileContext({ *oSheetId, nColumn, nRow }, aGrammar, bForPersistence,
        bAllowExternalReferences, bComputeImplicitIntersection, bMatrixFormula,
        eExtendedErrorDetection);
}

class WorkbookCompileHost final : public NameResolver
    , public DatabaseRangeResolver
    , public TableRefResolver
    , public ColRowNameResolver
    , public ExternalNameResolver
{
public:
    explicit WorkbookCompileHost(
        const core::workbook::Workbook& rWorkbook, WorkbookCompileHostOptions aOptions = {})
        : mrWorkbook(rWorkbook)
        , maSupport(aOptions.maSupport)
        , maExternalNames(std::move(aOptions.maExternalNames))
    {
    }

    [[nodiscard]] CompileHosts hosts() const
    {
        return { this, this, this, this, this };
    }

    [[nodiscard]] constexpr const WorkbookCompileHostSupport& support() const
    {
        return maSupport;
    }

    [[nodiscard]] constexpr const core::workbook::Workbook& workbook() const
    {
        return mrWorkbook;
    }

    [[nodiscard]] std::optional<api::SheetId> lookupSheetId(api::StringView rSheetName) const
    {
        return mrWorkbook.findSheetId(rSheetName);
    }

    [[nodiscard]] std::optional<token::NameData> lookupRangeName(
        api::StringView rName, std::optional<api::SheetId> onSheet,
        const CompileContext&) const override
    {
        if (onSheet && *onSheet >= 0
            && static_cast<std::size_t>(*onSheet) < mrWorkbook.maSheets.size())
        {
            const auto oLocalIndex = lookupRangeNameIndex(rName, localScopeName(*onSheet));
            if (oLocalIndex)
            {
                return token::NameData {
                    static_cast<std::int16_t>(*onSheet),
                    *oLocalIndex,
                };
            }
        }

        const auto oGlobalIndex = lookupRangeNameIndex(rName, {});
        if (!oGlobalIndex)
            return std::nullopt;

        return token::NameData { -1, *oGlobalIndex };
    }

    [[nodiscard]] std::optional<token::DatabaseRangeData> lookupDatabaseRange(
        api::StringView, const CompileContext&) const override
    {
        return std::nullopt;
    }

    [[nodiscard]] std::optional<token::TableRefData> lookupTableReference(
        api::StringView, api::StringView, const CompileContext&) const override
    {
        return std::nullopt;
    }

    [[nodiscard]] std::optional<api::refdata::SingleRefData> lookupColRowName(
        api::StringView, const CompileContext&) const override
    {
        return std::nullopt;
    }

    [[nodiscard]] std::optional<token::ExternalNameData> lookupExternalName(
        api::StringView rSymbol, const CompileContext&) const override
    {
        if (const auto oBuiltin = lookupBuiltinExternalName(rSymbol))
            return token::ExternalNameData { kBuiltinExternalNameCatalogId, *oBuiltin };

        for (const auto& rBinding : maExternalNames)
        {
            if (equalLookupText(rBinding.maSymbol, rSymbol))
                return rBinding.maData;
        }

        return std::nullopt;
    }

private:
    const core::workbook::Workbook& mrWorkbook;
    WorkbookCompileHostSupport maSupport;
    std::vector<WorkbookExternalNameBinding> maExternalNames;

    [[nodiscard]] api::String localScopeName(api::SheetId nSheet) const
    {
        if (nSheet < 0 || static_cast<std::size_t>(nSheet) >= mrWorkbook.maSheets.size())
            return {};
        return mrWorkbook.maSheets[nSheet].maName;
    }

    [[nodiscard]] std::optional<std::uint16_t> lookupRangeNameIndex(
        api::StringView rName, api::StringView rScopeSheetName) const
    {
        for (std::size_t nIndex = 0; nIndex < mrWorkbook.maNamedRanges.size(); ++nIndex)
        {
            const auto& rRange = mrWorkbook.maNamedRanges[nIndex];
            if (!equalLookupText(rRange.maName, rName)
                || !equalLookupText(rRange.maScopeSheetName, rScopeSheetName))
            {
                continue;
            }

            if (nIndex >= std::numeric_limits<std::uint16_t>::max())
                return std::nullopt;

            return static_cast<std::uint16_t>(nIndex + 1);
        }

        return std::nullopt;
    }
};

} // namespace spreadsheetengine::detail::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
