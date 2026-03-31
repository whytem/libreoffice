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
#include <vector>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx>

namespace spreadsheetengine::detail::dependency
{

struct DependencyNodeId
{
    sal_Int32 mnIndex = -1;

    [[nodiscard]] constexpr bool operator==(const DependencyNodeId& rOther) const = default;
    [[nodiscard]] constexpr bool isValid() const { return mnIndex >= 0; }
};

enum class DependencyNodeKind : sal_uInt8
{
    FormulaCell,
    NamedRange
};

enum class DependencySourceKind : sal_uInt8
{
    Cell,
    Range,
    NamedRange,
    OpaqueWorkbook
};

enum class DependencyEdgeKind : sal_uInt8
{
    DirectCell,
    DirectRange,
    NamedRange,
    OpaqueWorkbook
};

struct DependencySource
{
    DependencySourceKind meKind = DependencySourceKind::Cell;
    api::CellAddress maCellAddress;
    api::CellRange maCellRange;
    facade::NamedRangeId maNamedRangeId;
    api::String maOpaqueDetail;

    [[nodiscard]] constexpr bool operator==(const DependencySource& rOther) const = default;

    [[nodiscard]] static constexpr DependencySource cell(const api::CellAddress& rAddress)
    {
        DependencySource aSource;
        aSource.meKind = DependencySourceKind::Cell;
        aSource.maCellAddress = rAddress;
        aSource.maCellRange = { rAddress, rAddress };
        return aSource;
    }

    [[nodiscard]] static constexpr DependencySource range(const api::CellRange& rRange)
    {
        DependencySource aSource;
        aSource.meKind = DependencySourceKind::Range;
        aSource.maCellRange = rRange;
        return aSource;
    }

    [[nodiscard]] static constexpr DependencySource namedRange(
        const facade::NamedRangeId& rNamedRangeId)
    {
        DependencySource aSource;
        aSource.meKind = DependencySourceKind::NamedRange;
        aSource.maNamedRangeId = rNamedRangeId;
        return aSource;
    }

    [[nodiscard]] static DependencySource opaqueWorkbook(api::StringView rDetail)
    {
        DependencySource aSource;
        aSource.meKind = DependencySourceKind::OpaqueWorkbook;
        aSource.maOpaqueDetail = api::String(rDetail);
        return aSource;
    }
};

struct DependencyEdge
{
    DependencyNodeId maDependent;
    DependencyEdgeKind meKind = DependencyEdgeKind::DirectCell;
    DependencySource maSource;

    [[nodiscard]] constexpr bool operator==(const DependencyEdge& rOther) const = default;
};

struct DependencyNode
{
    DependencyNodeId maId;
    DependencyNodeKind meKind = DependencyNodeKind::FormulaCell;
    std::optional<facade::FormulaCellId> moFormulaCellId;
    std::optional<facade::NamedRangeId> moNamedRangeId;
    std::optional<api::CellAddress> moOutputAddress;
    std::optional<api::CellAddress> moSharedGroupAnchor;
    sal_Int32 mnSharedGroupLength = 0;
    bool mbShareableGroup = false;
    bool mbOpaqueDependencies = false;
    api::String maSourceText;

    [[nodiscard]] constexpr bool operator==(const DependencyNode& rOther) const = default;
};

struct DependencyIssue
{
    DependencyNodeId maNodeId;
    api::String maMessage;

    [[nodiscard]] constexpr bool operator==(const DependencyIssue& rOther) const = default;
};

struct DependencyBuildReport
{
    sal_Int32 mnFormulaNodeCount = 0;
    sal_Int32 mnNamedRangeNodeCount = 0;
    sal_Int32 mnOpaqueNodeCount = 0;
    sal_Int32 mnDirectCellEdgeCount = 0;
    sal_Int32 mnDirectRangeEdgeCount = 0;
    sal_Int32 mnNamedRangeEdgeCount = 0;
    sal_Int32 mnOpaqueEdgeCount = 0;
    sal_Int32 mnReverseDependencyEdgeCount = 0;
    std::vector<DependencyIssue> maIssues;

    [[nodiscard]] constexpr bool operator==(const DependencyBuildReport& rOther) const = default;
};

enum class DirtyReason : sal_uInt8
{
    ScalarValueChanged,
    FormulaChanged,
    CellCleared,
    RangeCleared,
    NamedRangeChanged,
    StructuralMutation,
    DirectDependency,
    TransitiveDependency,
    OpaqueDependency
};

struct DirtyFormulaCell
{
    api::CellAddress maAddress;
    DirtyReason meReason = DirtyReason::DirectDependency;

    [[nodiscard]] constexpr bool operator==(const DirtyFormulaCell& rOther) const = default;
};

struct DirtyNamedRange
{
    facade::NamedRangeId maId;
    DirtyReason meReason = DirtyReason::NamedRangeChanged;

    [[nodiscard]] constexpr bool operator==(const DirtyNamedRange& rOther) const = default;
};

enum class RebuildScopeKind : sal_uInt8
{
    FormulaCell,
    Sheet,
    Workbook,
    NamedRanges
};

struct RebuildScope
{
    RebuildScopeKind meKind = RebuildScopeKind::FormulaCell;
    api::CellAddress maAddress;
    facade::SheetId mnSheet = 0;
    api::String maReason;

    [[nodiscard]] constexpr bool operator==(const RebuildScope& rOther) const = default;
};

struct InvalidationPlan
{
    std::vector<DependencyNodeId> maDirtyNodeIds;
    std::vector<DirtyFormulaCell> maDirtyFormulaCells;
    std::vector<DirtyNamedRange> maDirtyNamedRanges;
    std::vector<RebuildScope> maRebuildScopes;
    bool mbRequiresSnapshotRebuild = false;
    bool mbUsedConservativeWidening = false;

    [[nodiscard]] constexpr bool operator==(const InvalidationPlan& rOther) const = default;
};

} // namespace spreadsheetengine::detail::dependency

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
