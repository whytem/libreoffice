/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include <spreadsheetengine/detail/OdfFormulaParser.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/dependency/DependencyTypes.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacade.hxx>

namespace spreadsheetengine::detail::dependency
{

struct DependencySnapshot
{
    std::vector<DependencyNode> maNodes;
    std::vector<std::vector<DependencyEdge>> maDependenciesByNode;
    std::vector<std::vector<DependencyNodeId>> maReverseDependentsByNode;
    DependencyBuildReport maReport;

    [[nodiscard]] const DependencyNode* getNode(DependencyNodeId aNodeId) const
    {
        if (!aNodeId.isValid()
            || static_cast<std::size_t>(aNodeId.mnIndex) >= maNodes.size())
        {
            return nullptr;
        }

        return &maNodes[aNodeId.mnIndex];
    }

    [[nodiscard]] const std::vector<DependencyEdge>& getDependencies(
        DependencyNodeId aNodeId) const
    {
        static const std::vector<DependencyEdge> aEmpty;
        if (!aNodeId.isValid()
            || static_cast<std::size_t>(aNodeId.mnIndex) >= maDependenciesByNode.size())
        {
            return aEmpty;
        }

        return maDependenciesByNode[aNodeId.mnIndex];
    }

    [[nodiscard]] const std::vector<DependencyNodeId>& getReverseDependents(
        DependencyNodeId aNodeId) const
    {
        static const std::vector<DependencyNodeId> aEmpty;
        if (!aNodeId.isValid()
            || static_cast<std::size_t>(aNodeId.mnIndex) >= maReverseDependentsByNode.size())
        {
            return aEmpty;
        }

        return maReverseDependentsByNode[aNodeId.mnIndex];
    }

    [[nodiscard]] std::optional<DependencyNodeId> findFormulaCellNode(
        const api::CellAddress& rAddress) const
    {
        for (const auto& rNode : maNodes)
        {
            if (rNode.meKind != DependencyNodeKind::FormulaCell || !rNode.moOutputAddress)
                continue;
            if (*rNode.moOutputAddress == rAddress)
                return rNode.maId;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<DependencyNodeId> findNamedRangeNode(
        const facade::NamedRangeId& rNamedRangeId) const
    {
        for (const auto& rNode : maNodes)
        {
            if (rNode.meKind != DependencyNodeKind::NamedRange || !rNode.moNamedRangeId)
                continue;
            if (*rNode.moNamedRangeId == rNamedRangeId)
                return rNode.maId;
        }
        return std::nullopt;
    }
};

namespace detail
{

[[nodiscard]] inline bool isAsciiAlpha(char16_t cChar)
{
    return (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
}

[[nodiscard]] inline api::String foldAsciiUpper(api::StringView rText)
{
    api::String aFolded;
    aFolded.reserve(rText.size());
    for (const char16_t cChar : rText)
    {
        if (cChar >= u'a' && cChar <= u'z')
            aFolded.push_back(static_cast<char16_t>(cChar - u'a' + u'A'));
        else
            aFolded.push_back(cChar);
    }
    return aFolded;
}

[[nodiscard]] inline api::CellRange normalizeRange(api::CellRange aRange)
{
    if (aRange.maStart.mnSheet > aRange.maEnd.mnSheet)
        std::swap(aRange.maStart.mnSheet, aRange.maEnd.mnSheet);
    if (aRange.maStart.mnColumn > aRange.maEnd.mnColumn)
        std::swap(aRange.maStart.mnColumn, aRange.maEnd.mnColumn);
    if (aRange.maStart.mnRow > aRange.maEnd.mnRow)
        std::swap(aRange.maStart.mnRow, aRange.maEnd.mnRow);
    return aRange;
}

[[nodiscard]] inline bool rangeContainsCell(const api::CellRange& rRange,
    const api::CellAddress& rAddress)
{
    const api::CellRange aRange = normalizeRange(rRange);
    return rAddress.mnSheet >= aRange.maStart.mnSheet
           && rAddress.mnSheet <= aRange.maEnd.mnSheet
           && rAddress.mnColumn >= aRange.maStart.mnColumn
           && rAddress.mnColumn <= aRange.maEnd.mnColumn
           && rAddress.mnRow >= aRange.maStart.mnRow
           && rAddress.mnRow <= aRange.maEnd.mnRow;
}

[[nodiscard]] inline bool rangesIntersect(
    const api::CellRange& rLeft, const api::CellRange& rRight)
{
    const api::CellRange aLeft = normalizeRange(rLeft);
    const api::CellRange aRight = normalizeRange(rRight);
    return aLeft.maStart.mnSheet <= aRight.maEnd.mnSheet
           && aLeft.maEnd.mnSheet >= aRight.maStart.mnSheet
           && aLeft.maStart.mnColumn <= aRight.maEnd.mnColumn
           && aLeft.maEnd.mnColumn >= aRight.maStart.mnColumn
           && aLeft.maStart.mnRow <= aRight.maEnd.mnRow
           && aLeft.maEnd.mnRow >= aRight.maStart.mnRow;
}

class FacadeReferenceHost
{
    const facade::WorkbookFacade& mrFacade;

public:
    explicit FacadeReferenceHost(const facade::WorkbookFacade& rFacade)
        : mrFacade(rFacade)
    {
    }

    [[nodiscard]] std::optional<api::SheetId> lookupSheetId(api::StringView rSheetName) const
    {
        return mrFacade.findSheetId(rSheetName);
    }
};

[[nodiscard]] inline api::CellRange toApiRange(
    const compiler::detail::ParsedSingleReference& rReference)
{
    using Shape = compiler::detail::ParsedSingleReference::Shape;
    switch (rReference.meShape)
    {
        case Shape::Cell:
        {
            const api::CellAddress aAddress { rReference.mnResolvedSheet,
                rReference.mnResolvedColumn, rReference.mnResolvedRow };
            return { aAddress, aAddress };
        }
        case Shape::WholeRow:
            return { { rReference.mnResolvedSheet, 0, rReference.mnResolvedRow },
                { rReference.mnResolvedSheet, compiler::detail::kSmokeMaxColumn,
                    rReference.mnResolvedRow } };
        case Shape::WholeColumn:
            return { { rReference.mnResolvedSheet, rReference.mnResolvedColumn, 0 },
                { rReference.mnResolvedSheet, rReference.mnResolvedColumn,
                    compiler::detail::kSmokeMaxRow } };
    }

    const api::CellAddress aAddress { rReference.mnResolvedSheet,
        rReference.mnResolvedColumn, rReference.mnResolvedRow };
    return { aAddress, aAddress };
}

[[nodiscard]] inline api::CellRange toApiRange(
    const api::refdata::ComplexRefData& rReference)
{
    return normalizeRange({ { rReference.maRef1.mnSheet, rReference.maRef1.mnColumn,
                                   rReference.maRef1.mnRow },
        { rReference.maRef2.mnSheet, rReference.maRef2.mnColumn, rReference.maRef2.mnRow } });
}

[[nodiscard]] inline bool dependencySourceExists(
    const std::vector<DependencyEdge>& rEdges, const DependencySource& rSource)
{
    return std::any_of(rEdges.begin(), rEdges.end(),
        [&rSource](const DependencyEdge& rEdge) { return rEdge.maSource == rSource; });
}

[[nodiscard]] inline std::optional<facade::NamedRangeDescriptor> resolveNamedRange(
    const facade::WorkbookFacade& rFacade, api::StringView rName, const api::CellAddress& rBaseAddress)
{
    if (rBaseAddress.mnSheet >= 0)
    {
        if (const auto oLocal = rFacade.findNamedRange(rName, rBaseAddress.mnSheet))
            return oLocal;
    }

    return rFacade.findNamedRange(rName, std::nullopt);
}

inline void addEdge(DependencySnapshot& rSnapshot, DependencyNodeId aNodeId,
    DependencyEdgeKind eKind, const DependencySource& rSource)
{
    auto& rEdges = rSnapshot.maDependenciesByNode[aNodeId.mnIndex];
    if (dependencySourceExists(rEdges, rSource))
        return;

    rEdges.push_back({ aNodeId, eKind, rSource });
    switch (eKind)
    {
        case DependencyEdgeKind::DirectCell:
            ++rSnapshot.maReport.mnDirectCellEdgeCount;
            break;
        case DependencyEdgeKind::DirectRange:
            ++rSnapshot.maReport.mnDirectRangeEdgeCount;
            break;
        case DependencyEdgeKind::NamedRange:
            ++rSnapshot.maReport.mnNamedRangeEdgeCount;
            break;
        case DependencyEdgeKind::OpaqueWorkbook:
            ++rSnapshot.maReport.mnOpaqueEdgeCount;
            break;
    }
}

inline void addIssue(DependencySnapshot& rSnapshot, DependencyNodeId aNodeId, api::StringView rMessage)
{
    rSnapshot.maReport.maIssues.push_back({ aNodeId, api::String(rMessage) });
}

inline void addOpaqueDependency(DependencySnapshot& rSnapshot, DependencyNodeId aNodeId,
    api::StringView rDetail)
{
    auto* pNode = const_cast<DependencyNode*>(rSnapshot.getNode(aNodeId));
    if (!pNode)
        return;
    pNode->mbOpaqueDependencies = true;
    addEdge(rSnapshot, aNodeId, DependencyEdgeKind::OpaqueWorkbook,
        DependencySource::opaqueWorkbook(rDetail));
}

inline void addDependencyForRange(DependencySnapshot& rSnapshot, DependencyNodeId aNodeId,
    const api::CellRange& rRange)
{
    if (rRange.isSingleCell())
        addEdge(rSnapshot, aNodeId, DependencyEdgeKind::DirectCell,
            DependencySource::cell(rRange.maStart));
    else
        addEdge(rSnapshot, aNodeId, DependencyEdgeKind::DirectRange,
            DependencySource::range(rRange));
}

[[nodiscard]] inline bool collectDependencyFromRawReferenceText(api::StringView rText,
    const facade::WorkbookFacade& rFacade, DependencySnapshot& rSnapshot, DependencyNodeId aNodeId,
    const api::CellAddress& rBaseAddress)
{
    const FacadeReferenceHost aHost(rFacade);
    const auto aContext = compiler::makeWorkbookCompileContext(
        rBaseAddress, rFacade.getGrammar(), false, true);

    if (const auto oReference = compiler::detail::parseSingleReference(
            rText, aHost, aContext, rBaseAddress.mnSheet))
    {
        addDependencyForRange(rSnapshot, aNodeId, toApiRange(*oReference));
        return true;
    }

    const std::size_t nSeparator = rText.rfind(u':');
    if (nSeparator == api::StringView::npos || nSeparator == 0
        || nSeparator + 1 >= rText.size())
    {
        return false;
    }

    const api::StringView aStartText = rText.substr(0, nSeparator);
    const api::StringView aEndText = rText.substr(nSeparator + 1);
    const auto oStart = compiler::detail::parseSingleReference(
        aStartText, aHost, aContext, rBaseAddress.mnSheet);
    if (!oStart)
        return false;

    std::optional<compiler::detail::ExternalReferenceContext> oExternal;
    if (oStart->mbExternal)
    {
        oExternal = compiler::detail::ExternalReferenceContext {
            oStart->mnFileId, oStart->maExternalTabName };
    }

    const auto oEnd = compiler::detail::parseSingleReference(
        aEndText, aHost, aContext, oStart->mnResolvedSheet, oExternal);
    if (!oEnd)
        return false;

    api::CellRange aRange;
    if (oStart->meShape == compiler::detail::ParsedSingleReference::Shape::Cell
        && oEnd->meShape == compiler::detail::ParsedSingleReference::Shape::Cell)
    {
        aRange = detail::normalizeRange(
            { { oStart->mnResolvedSheet, oStart->mnResolvedColumn, oStart->mnResolvedRow },
                { oEnd->mnResolvedSheet, oEnd->mnResolvedColumn, oEnd->mnResolvedRow } });
    }
    else
    {
        aRange = toApiRange(compiler::detail::mergeExpandedRanges(*oStart, *oEnd));
    }

    addDependencyForRange(rSnapshot, aNodeId, aRange);
    return true;
}

[[nodiscard]] inline bool isOpaqueFunctionHead(api::StringView rHead)
{
    const api::String aFolded = foldAsciiUpper(rHead);
    return aFolded == u"INDIRECT" || aFolded == u"OFFSET";
}

inline void collectDependenciesFromFormulaNode(const core::formula::Node& rNode,
    const facade::WorkbookFacade& rFacade, DependencySnapshot& rSnapshot,
    DependencyNodeId aNodeId, const api::CellAddress& rBaseAddress)
{
    const FacadeReferenceHost aHost(rFacade);
    const auto aContext = compiler::makeWorkbookCompileContext(
        rBaseAddress, rFacade.getGrammar(), false, true);

    using core::formula::NodeKind;
    switch (rNode.meKind)
    {
        case NodeKind::CellReference:
        {
            const auto oReference = compiler::detail::parseSingleReference(
                rNode.maPrimaryText, aHost, aContext, rBaseAddress.mnSheet);
            if (!oReference)
            {
                addOpaqueDependency(rSnapshot, aNodeId, u"unsupported_reference_text");
                addIssue(rSnapshot, aNodeId, u"unsupported_reference_text");
                return;
            }

            addDependencyForRange(rSnapshot, aNodeId, toApiRange(*oReference));
            return;
        }
        case NodeKind::RangeReference:
        {
            const auto oStart = compiler::detail::parseSingleReference(
                rNode.maPrimaryText, aHost, aContext, rBaseAddress.mnSheet);
            if (!oStart)
            {
                addOpaqueDependency(rSnapshot, aNodeId, u"unsupported_range_start");
                addIssue(rSnapshot, aNodeId, u"unsupported_range_start");
                return;
            }

            std::optional<compiler::detail::ExternalReferenceContext> oExternal;
            if (oStart->mbExternal)
                oExternal = compiler::detail::ExternalReferenceContext {
                    oStart->mnFileId, oStart->maExternalTabName };

            const auto oEnd = compiler::detail::parseSingleReference(
                rNode.maSecondaryText, aHost, aContext, oStart->mnResolvedSheet, oExternal);
            if (!oEnd)
            {
                addOpaqueDependency(rSnapshot, aNodeId, u"unsupported_range_end");
                addIssue(rSnapshot, aNodeId, u"unsupported_range_end");
                return;
            }

            api::CellRange aRange;
            if (oStart->meShape == compiler::detail::ParsedSingleReference::Shape::Cell
                && oEnd->meShape == compiler::detail::ParsedSingleReference::Shape::Cell)
            {
                aRange = detail::normalizeRange(
                    { { oStart->mnResolvedSheet, oStart->mnResolvedColumn, oStart->mnResolvedRow },
                        { oEnd->mnResolvedSheet, oEnd->mnResolvedColumn, oEnd->mnResolvedRow } });
            }
            else
            {
                aRange = toApiRange(compiler::detail::mergeExpandedRanges(*oStart, *oEnd));
            }

            addDependencyForRange(rSnapshot, aNodeId, aRange);
            return;
        }
        case NodeKind::NamedReference:
        {
            const auto oNamedRange = resolveNamedRange(rFacade, rNode.maPrimaryText, rBaseAddress);
            if (!oNamedRange
                && collectDependencyFromRawReferenceText(
                    rNode.maPrimaryText, rFacade, rSnapshot, aNodeId, rBaseAddress))
            {
                return;
            }

            if (!oNamedRange)
            {
                addOpaqueDependency(rSnapshot, aNodeId, u"missing_named_reference");
                addIssue(rSnapshot, aNodeId, u"missing_named_reference");
                return;
            }

            addEdge(rSnapshot, aNodeId, DependencyEdgeKind::NamedRange,
                DependencySource::namedRange(oNamedRange->maId));
            return;
        }
        case NodeKind::RangeConstructor:
            addOpaqueDependency(rSnapshot, aNodeId, u"range_constructor");
            addIssue(rSnapshot, aNodeId, u"range_constructor");
            for (const auto& pChild : rNode.maChildren)
            {
                if (pChild)
                    collectDependenciesFromFormulaNode(
                        *pChild, rFacade, rSnapshot, aNodeId, rBaseAddress);
            }
            return;
        case NodeKind::ReferenceList:
            for (const auto& pChild : rNode.maChildren)
            {
                if (pChild)
                    collectDependenciesFromFormulaNode(
                        *pChild, rFacade, rSnapshot, aNodeId, rBaseAddress);
            }
            return;
        case NodeKind::FunctionCall:
            if (isOpaqueFunctionHead(rNode.maPrimaryText))
            {
                addOpaqueDependency(rSnapshot, aNodeId, u"dynamic_reference_function");
                addIssue(rSnapshot, aNodeId, u"dynamic_reference_function");
            }
            for (const auto& pChild : rNode.maChildren)
            {
                if (pChild)
                    collectDependenciesFromFormulaNode(
                        *pChild, rFacade, rSnapshot, aNodeId, rBaseAddress);
            }
            return;
        case NodeKind::UnaryOperation:
        case NodeKind::BinaryOperation:
        case NodeKind::ArrayConstant:
            for (const auto& pChild : rNode.maChildren)
            {
                if (pChild)
                    collectDependenciesFromFormulaNode(
                        *pChild, rFacade, rSnapshot, aNodeId, rBaseAddress);
            }
            return;
        case NodeKind::NumberLiteral:
        case NodeKind::StringLiteral:
        case NodeKind::BooleanLiteral:
        case NodeKind::ErrorLiteral:
        case NodeKind::EmptyArgument:
            return;
    }
}

inline DependencyNodeId addNode(DependencySnapshot& rSnapshot, DependencyNode aNode)
{
    aNode.maId = { static_cast<sal_Int32>(rSnapshot.maNodes.size()) };
    rSnapshot.maDependenciesByNode.emplace_back();
    rSnapshot.maReverseDependentsByNode.emplace_back();
    rSnapshot.maNodes.push_back(std::move(aNode));
    return rSnapshot.maNodes.back().maId;
}

inline void addReverseDependent(DependencySnapshot& rSnapshot, DependencyNodeId aProducer,
    DependencyNodeId aDependent)
{
    if (!aProducer.isValid() || !aDependent.isValid())
        return;

    auto& rDependents = rSnapshot.maReverseDependentsByNode[aProducer.mnIndex];
    if (std::find(rDependents.begin(), rDependents.end(), aDependent) != rDependents.end())
        return;

    rDependents.push_back(aDependent);
    ++rSnapshot.maReport.mnReverseDependencyEdgeCount;
}

[[nodiscard]] inline bool dependencyMatchesProducedFormulaAddress(
    const DependencySource& rSource, const api::CellAddress& rProducedAddress)
{
    switch (rSource.meKind)
    {
        case DependencySourceKind::Cell:
            return rSource.maCellAddress == rProducedAddress;
        case DependencySourceKind::Range:
            return detail::rangeContainsCell(rSource.maCellRange, rProducedAddress);
        case DependencySourceKind::NamedRange:
        case DependencySourceKind::OpaqueWorkbook:
            return false;
    }

    return false;
}

[[nodiscard]] inline bool dependencyMatchesProducedNamedRange(
    const DependencySource& rSource, const facade::NamedRangeId& rNamedRangeId)
{
    return rSource.meKind == DependencySourceKind::NamedRange
           && rSource.maNamedRangeId == rNamedRangeId;
}

inline void buildReverseDependencyIndex(DependencySnapshot& rSnapshot)
{
    for (const auto& rProducer : rSnapshot.maNodes)
    {
        for (const auto& rCandidate : rSnapshot.maNodes)
        {
            if (rProducer.maId == rCandidate.maId)
                continue;

            const auto& rDependencies = rSnapshot.getDependencies(rCandidate.maId);
            bool bMatched = false;
            for (const auto& rDependency : rDependencies)
            {
                if (rProducer.meKind == DependencyNodeKind::FormulaCell
                    && rProducer.moOutputAddress
                    && dependencyMatchesProducedFormulaAddress(
                        rDependency.maSource, *rProducer.moOutputAddress))
                {
                    bMatched = true;
                    break;
                }

                if (rProducer.meKind == DependencyNodeKind::NamedRange
                    && rProducer.moNamedRangeId
                    && dependencyMatchesProducedNamedRange(
                        rDependency.maSource, *rProducer.moNamedRangeId))
                {
                    bMatched = true;
                    break;
                }
            }

            if (bMatched)
                addReverseDependent(rSnapshot, rProducer.maId, rCandidate.maId);
        }
    }
}

inline void populateSharedGroupMetadata(const facade::WorkbookFacade& rFacade,
    const facade::FormulaCellDescriptor& rFormulaCell, DependencyNode& rNode)
{
    const auto oGroup = rFacade.getFormulaGroupDescriptor(rFormulaCell.maId.maAddress);
    if (!oGroup)
        return;

    rNode.moSharedGroupAnchor = oGroup->maAnchor;
    rNode.mnSharedGroupLength = oGroup->mnLength;
    rNode.mbShareableGroup = oGroup->mbShareable;
}

} // namespace detail

[[nodiscard]] inline DependencySnapshot buildDependencySnapshot(
    const facade::WorkbookFacade& rFacade)
{
    DependencySnapshot aSnapshot;

    const auto aNamedRanges = rFacade.getNamedRangeDescriptors();
    for (const auto& rNamedRange : aNamedRanges)
    {
        DependencyNode aNode;
        aNode.meKind = DependencyNodeKind::NamedRange;
        aNode.moNamedRangeId = rNamedRange.maId;
        aNode.maSourceText = rNamedRange.maTargetExpression;
        const auto aNodeId = detail::addNode(aSnapshot, std::move(aNode));
        ++aSnapshot.maReport.mnNamedRangeNodeCount;

        const auto aParseResult = core::formula::parseFormula(rNamedRange.maTargetExpression);
        if (!aParseResult)
        {
            if (!detail::collectDependencyFromRawReferenceText(rNamedRange.maTargetExpression,
                    rFacade, aSnapshot, aNodeId, rNamedRange.maBaseAddress))
            {
                detail::addOpaqueDependency(aSnapshot, aNodeId, u"parse_failure");
                detail::addIssue(aSnapshot, aNodeId, u"parse_failure");
            }
            continue;
        }

        detail::collectDependenciesFromFormulaNode(*aParseResult.mpRoot, rFacade, aSnapshot,
            aNodeId, rNamedRange.maBaseAddress);
    }

    rFacade.visitAllFormulaCells([&](const facade::FormulaCellDescriptor& rFormulaCell) {
        DependencyNode aNode;
        aNode.meKind = DependencyNodeKind::FormulaCell;
        aNode.moFormulaCellId = rFormulaCell.maId;
        aNode.moOutputAddress = rFormulaCell.maId.maAddress;
        aNode.maSourceText = rFormulaCell.maFormulaSource;
        detail::populateSharedGroupMetadata(rFacade, rFormulaCell, aNode);

        const auto aNodeId = detail::addNode(aSnapshot, std::move(aNode));
        ++aSnapshot.maReport.mnFormulaNodeCount;

        const auto aParseResult = core::formula::parseFormula(rFormulaCell.maFormulaSource);
        if (!aParseResult)
        {
            detail::addOpaqueDependency(aSnapshot, aNodeId, u"parse_failure");
            detail::addIssue(aSnapshot, aNodeId, u"parse_failure");
            return true;
        }

        detail::collectDependenciesFromFormulaNode(*aParseResult.mpRoot, rFacade, aSnapshot,
            aNodeId, rFormulaCell.maId.maAddress);
        return true;
    });

    for (const auto& rNode : aSnapshot.maNodes)
    {
        if (rNode.mbOpaqueDependencies)
            ++aSnapshot.maReport.mnOpaqueNodeCount;
    }

    detail::buildReverseDependencyIndex(aSnapshot);

    return aSnapshot;
}

} // namespace spreadsheetengine::detail::dependency

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
