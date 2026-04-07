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
#include <set>
#include <vector>

#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacade.hxx>
#include <spreadsheetengine/runtime/ReferenceText.hxx>

namespace spreadsheetengine::detail::facade::consumers
{

/// Result of a formula-cell enumeration shadow check.
struct FormulaCellEnumerationResult
{
    sal_Int32 mnTotalFormulaCells = 0;
    sal_Int32 mnOrdinaryCells = 0;
    sal_Int32 mnSharedGroupMembers = 0;
    sal_Int32 mnMatrixOrigins = 0;
    sal_Int32 mnMatrixMembers = 0;
    sal_Int32 mnDirtyCells = 0;
};

/// Enumerate all formula cells through the facade and produce a summary.
/// This is the first shadow consumer — it proves the facade can enumerate
/// the formula-cell population of a workbook.
[[nodiscard]] inline FormulaCellEnumerationResult
enumerateFormulaCells(const WorkbookFacade& rFacade)
{
    FormulaCellEnumerationResult aResult;

    rFacade.visitAllFormulaCells([&aResult](const FormulaCellDescriptor& rDesc) {
        ++aResult.mnTotalFormulaCells;

        switch (rDesc.meKind)
        {
            case FormulaCellKind::Ordinary:
                ++aResult.mnOrdinaryCells;
                break;
            case FormulaCellKind::SharedGroupMember:
                ++aResult.mnSharedGroupMembers;
                break;
            case FormulaCellKind::MatrixOrigin:
                ++aResult.mnMatrixOrigins;
                break;
            case FormulaCellKind::MatrixMember:
                ++aResult.mnMatrixMembers;
                break;
        }

        if (rDesc.mbDirty)
            ++aResult.mnDirtyCells;

        return true;
    });

    return aResult;
}

/// Result of a shared-formula group comparison.
struct SharedFormulaGroupSummary
{
    sal_Int32 mnGroupCount = 0;
    sal_Int32 mnTotalGroupLength = 0;
    sal_Int32 mnShareableGroups = 0;
};

[[nodiscard]] inline std::vector<FormulaGroupDescriptor>
collectFormulaGroupDescriptors(const WorkbookFacade& rFacade)
{
    std::vector<FormulaGroupDescriptor> aGroups;

    rFacade.visitAllFormulaCells([&](const FormulaCellDescriptor& rDesc) {
        auto oGroup = rFacade.getFormulaGroupDescriptor(rDesc.maId.maAddress);
        if (!oGroup)
            return true;

        if (std::find(aGroups.begin(), aGroups.end(), *oGroup) == aGroups.end())
            aGroups.push_back(*oGroup);
        return true;
    });

    std::sort(aGroups.begin(), aGroups.end(),
        [](const FormulaGroupDescriptor& rLeft, const FormulaGroupDescriptor& rRight) {
            if (rLeft.maAnchor.mnSheet != rRight.maAnchor.mnSheet)
                return rLeft.maAnchor.mnSheet < rRight.maAnchor.mnSheet;
            if (rLeft.maAnchor.mnColumn != rRight.maAnchor.mnColumn)
                return rLeft.maAnchor.mnColumn < rRight.maAnchor.mnColumn;
            if (rLeft.maAnchor.mnRow != rRight.maAnchor.mnRow)
                return rLeft.maAnchor.mnRow < rRight.maAnchor.mnRow;
            if (rLeft.mnLength != rRight.mnLength)
                return rLeft.mnLength < rRight.mnLength;
            return rLeft.mbShareable < rRight.mbShareable;
        });
    return aGroups;
}

enum class SharedFormulaGroupTransitionKind : std::uint8_t
{
    None,
    Preserve,
    Rebuild,
    Split
};

struct SharedFormulaGroupTransition
{
    SharedFormulaGroupTransitionKind meKind = SharedFormulaGroupTransitionKind::None;
    sal_Int32 mnBeforeGroupCount = 0;
    sal_Int32 mnAfterGroupCount = 0;
    bool mbShareableChanged = false;

    [[nodiscard]] constexpr bool operator==(const SharedFormulaGroupTransition& rOther) const
        = default;
};

enum class SharedFormulaMutationFamily : std::uint8_t
{
    None,
    SameTextPreserve,
    MemberExit,
    Regroup,
    OneSidedInsert,
    Merge,
    ReplacementMerge,
    MultiGroupCollapse,
    StructuralPreserve,
    StructuralSplit,
    StructuralRebuild
};

struct SharedFormulaMutationClassification
{
    SharedFormulaGroupTransition maTransition;
    SharedFormulaMutationFamily meFamily = SharedFormulaMutationFamily::None;
    bool mbTouchedAddressSharedBefore = false;
    bool mbTouchedAddressSharedAfter = false;
    sal_Int32 mnBeforeNeighborhoodGroupCount = 0;
    sal_Int32 mnAfterNeighborhoodGroupCount = 0;

    [[nodiscard]] constexpr bool operator==(const SharedFormulaMutationClassification& rOther) const
        = default;
};

enum class SharedFormulaNamedRangeMutationBoundary : std::uint8_t
{
    None,
    GlobalSingleAreaSameSheet,
    Deferred
};

struct SharedFormulaNamedRangeMutationClassification
{
    SharedFormulaNamedRangeMutationBoundary meBoundary
        = SharedFormulaNamedRangeMutationBoundary::None;
    sal_Int32 mnNamedRangeCount = 0;
    bool mbDescriptorsStable = false;
    bool mbAllConsumersStayOnSheet = false;

    [[nodiscard]] constexpr bool operator==(
        const SharedFormulaNamedRangeMutationClassification& rOther) const = default;
};

namespace detail
{

struct AddressLess
{
    [[nodiscard]] bool operator()(const api::CellAddress& rLeft, const api::CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

[[nodiscard]] inline std::optional<FormulaGroupDescriptor> shiftFormulaGroupDescriptor(
    const FormulaGroupDescriptor& rDescriptor, const MutationEvent& rMutation)
{
    FormulaGroupDescriptor aShifted = rDescriptor;
    switch (rMutation.meKind)
    {
        case MutationKind::InsertRows:
            if (aShifted.maAnchor.mnSheet == rMutation.mnSheet
                && aShifted.maAnchor.mnRow >= rMutation.maAddress.mnRow)
            {
                aShifted.maAnchor.mnRow = static_cast<api::RowIndex>(
                    aShifted.maAnchor.mnRow + rMutation.mnCount);
            }
            return aShifted;
        case MutationKind::DeleteRows:
            if (aShifted.maAnchor.mnSheet == rMutation.mnSheet)
            {
                if (aShifted.maAnchor.mnRow >= rMutation.maAddress.mnRow
                    && aShifted.maAnchor.mnRow < rMutation.maAddress.mnRow + rMutation.mnCount)
                {
                    return std::nullopt;
                }
                if (aShifted.maAnchor.mnRow >= rMutation.maAddress.mnRow + rMutation.mnCount)
                {
                    aShifted.maAnchor.mnRow = static_cast<api::RowIndex>(
                        aShifted.maAnchor.mnRow - rMutation.mnCount);
                }
            }
            return aShifted;
        case MutationKind::InsertColumns:
            if (aShifted.maAnchor.mnSheet == rMutation.mnSheet
                && aShifted.maAnchor.mnColumn >= rMutation.maAddress.mnColumn)
            {
                aShifted.maAnchor.mnColumn = static_cast<api::ColumnIndex>(
                    aShifted.maAnchor.mnColumn + rMutation.mnCount);
            }
            return aShifted;
        case MutationKind::DeleteColumns:
            if (aShifted.maAnchor.mnSheet == rMutation.mnSheet)
            {
                if (aShifted.maAnchor.mnColumn >= rMutation.maAddress.mnColumn
                    && aShifted.maAnchor.mnColumn
                           < rMutation.maAddress.mnColumn + rMutation.mnCount)
                {
                    return std::nullopt;
                }
                if (aShifted.maAnchor.mnColumn
                    >= rMutation.maAddress.mnColumn + rMutation.mnCount)
                {
                    aShifted.maAnchor.mnColumn = static_cast<api::ColumnIndex>(
                        aShifted.maAnchor.mnColumn - rMutation.mnCount);
                }
            }
            return aShifted;
        default:
            return aShifted;
    }
}

[[nodiscard]] inline std::optional<FormulaGroupDescriptor> findFormulaGroupContainingAddress(
    const std::vector<FormulaGroupDescriptor>& rGroups, const api::CellAddress& rAddress)
{
    const auto it = std::find_if(rGroups.begin(), rGroups.end(),
        [&rAddress](const FormulaGroupDescriptor& rGroup) {
            return rGroup.maAnchor.mnSheet == rAddress.mnSheet
                   && rGroup.maAnchor.mnColumn == rAddress.mnColumn
                   && rAddress.mnRow >= rGroup.maAnchor.mnRow
                   && rAddress.mnRow
                          < static_cast<api::RowIndex>(rGroup.maAnchor.mnRow + rGroup.mnLength);
        });
    return it == rGroups.end() ? std::nullopt : std::optional<FormulaGroupDescriptor>(*it);
}

[[nodiscard]] inline std::vector<FormulaGroupDescriptor> collectNeighborhoodGroups(
    const WorkbookFacade& rFacade, const api::CellAddress& rAddress)
{
    std::vector<FormulaGroupDescriptor> aGroups;
    auto lCollect = [&](api::RowIndex nRow) {
        if (nRow < 0)
            return;

        const api::CellAddress aCandidate { rAddress.mnSheet, rAddress.mnColumn, nRow };
        const auto oGroup = rFacade.getFormulaGroupDescriptor(aCandidate);
        if (!oGroup)
            return;
        if (std::find(aGroups.begin(), aGroups.end(), *oGroup) == aGroups.end())
            aGroups.push_back(*oGroup);
    };

    lCollect(rAddress.mnRow);
    lCollect(static_cast<api::RowIndex>(rAddress.mnRow - 1));
    lCollect(static_cast<api::RowIndex>(rAddress.mnRow + 1));
    return aGroups;
}

[[nodiscard]] inline std::optional<api::ColumnIndex> parseNamedRangeColumnName(
    api::StringView rColumnName)
{
    if (rColumnName.empty())
        return std::nullopt;

    std::int64_t nColumn = 0;
    for (char16_t cChar : rColumnName)
    {
        if (cChar >= u'a' && cChar <= u'z')
            cChar = static_cast<char16_t>(cChar - u'a' + u'A');
        if (cChar < u'A' || cChar > u'Z')
            return std::nullopt;
        nColumn = (nColumn * 26) + (cChar - u'A' + 1);
    }

    return static_cast<api::ColumnIndex>(nColumn - 1);
}

[[nodiscard]] inline bool isAbsoluteAddressToken(api::StringView rToken)
{
    if (rToken.empty())
        return false;

    const std::size_t nDotPos = rToken.rfind(u'.');
    api::StringView aAddressToken
        = nDotPos == api::StringView::npos ? rToken : rToken.substr(nDotPos + 1);
    if (aAddressToken.empty() || aAddressToken.front() != u'$')
        return false;

    aAddressToken.remove_prefix(1);
    std::size_t nColumnEnd = 0;
    while (nColumnEnd < aAddressToken.size())
    {
        const char16_t cChar = aAddressToken[nColumnEnd];
        const bool bAlpha = (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
        if (!bAlpha)
            break;
        ++nColumnEnd;
    }

    if (nColumnEnd == 0 || nColumnEnd >= aAddressToken.size() || aAddressToken[nColumnEnd] != u'$')
        return false;

    api::StringView aRowToken = aAddressToken.substr(nColumnEnd + 1);
    if (aRowToken.empty())
        return false;

    return std::all_of(aRowToken.begin(), aRowToken.end(), [](char16_t cChar) {
        return cChar >= u'0' && cChar <= u'9';
    });
}

[[nodiscard]] inline std::optional<api::CellAddress> parseAbsoluteNamedRangeAddressToken(
    const WorkbookFacade& rFacade, api::StringView rToken, api::SheetId nImplicitSheet)
{
    const std::size_t nDotPos = rToken.rfind(u'.');
    api::SheetId nSheet = nImplicitSheet;
    api::StringView aAddressToken = rToken;
    if (nDotPos != api::StringView::npos)
    {
        api::StringView aSheetToken = rToken.substr(0, nDotPos);
        while (!aSheetToken.empty() && aSheetToken.front() == u'$')
            aSheetToken.remove_prefix(1);
        if (!aSheetToken.empty())
        {
            const auto oSheet = rFacade.findSheetId(
                runtime::referencetext::unquoteSheetName(aSheetToken));
            if (!oSheet)
                return std::nullopt;
            nSheet = *oSheet;
        }
        aAddressToken = rToken.substr(nDotPos + 1);
    }

    if (!isAbsoluteAddressToken(aAddressToken))
        return std::nullopt;

    aAddressToken.remove_prefix(1);
    std::size_t nColumnEnd = 0;
    while (nColumnEnd < aAddressToken.size())
    {
        const char16_t cChar = aAddressToken[nColumnEnd];
        const bool bAlpha = (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
        if (!bAlpha)
            break;
        ++nColumnEnd;
    }

    const auto oColumn = parseNamedRangeColumnName(aAddressToken.substr(0, nColumnEnd));
    if (!oColumn)
        return std::nullopt;

    api::StringView aRowToken = aAddressToken.substr(nColumnEnd + 1);
    std::int64_t nRow = 0;
    for (const char16_t cChar : aRowToken)
        nRow = (nRow * 10) + (cChar - u'0');

    if (nRow <= 0)
        return std::nullopt;

    return api::CellAddress { nSheet, *oColumn, static_cast<api::RowIndex>(nRow - 1) };
}

[[nodiscard]] inline std::optional<api::CellRange> parseSingleAreaNamedRangeTarget(
    const WorkbookFacade& rFacade, const NamedRangeDescriptor& rNamedRange)
{
    if (rNamedRange.maTargetExpression.find(u'~') != api::StringView::npos
        || rNamedRange.maTargetExpression.find(u';') != api::StringView::npos
        || rNamedRange.maTargetExpression.find(u',') != api::StringView::npos)
    {
        return std::nullopt;
    }

    const std::size_t nColonPos = rNamedRange.maTargetExpression.find(u':');
    if (nColonPos != api::StringView::npos
        && rNamedRange.maTargetExpression.find(u':', nColonPos + 1) != api::StringView::npos)
    {
        return std::nullopt;
    }

    if (nColonPos == api::StringView::npos)
    {
        const auto oSingle = parseAbsoluteNamedRangeAddressToken(
            rFacade, rNamedRange.maTargetExpression, rNamedRange.maBaseAddress.mnSheet);
        if (!oSingle)
            return std::nullopt;
        return api::CellRange { *oSingle, *oSingle };
    }

    const auto oStart = parseAbsoluteNamedRangeAddressToken(
        rFacade, rNamedRange.maTargetExpression.substr(0, nColonPos),
        rNamedRange.maBaseAddress.mnSheet);
    if (!oStart)
        return std::nullopt;

    const auto oEnd = parseAbsoluteNamedRangeAddressToken(
        rFacade, rNamedRange.maTargetExpression.substr(nColonPos + 1), oStart->mnSheet);
    if (!oEnd)
        return std::nullopt;

    api::CellRange aRange { *oStart, *oEnd };
    if (aRange.maStart.mnSheet != aRange.maEnd.mnSheet)
        return std::nullopt;

    return dependency::detail::normalizeRange(aRange);
}

[[nodiscard]] inline std::optional<NamedRangeDescriptor> findNamedRangeDescriptorById(
    const WorkbookFacade& rFacade, const NamedRangeId& rId)
{
    const auto aRanges = rFacade.getNamedRangeDescriptors();
    const auto it = std::find_if(aRanges.begin(), aRanges.end(),
        [&rId](const NamedRangeDescriptor& rDescriptor) { return rDescriptor.maId == rId; });
    if (it == aRanges.end())
        return std::nullopt;
    return *it;
}

[[nodiscard]] inline std::vector<api::CellAddress> collectRelevantSharedFormulaNeighborhoodAddresses(
    const WorkbookFacade& rFacade, const api::CellAddress& rAddress)
{
    std::set<api::CellAddress, AddressLess> aAddresses;
    auto lCollect = [&](api::RowIndex nRow) {
        if (nRow < 0)
            return;

        const api::CellAddress aCandidate { rAddress.mnSheet, rAddress.mnColumn, nRow };
        if (rFacade.getFormulaCellDescriptor(aCandidate))
            aAddresses.insert(aCandidate);

        const auto oGroup = rFacade.getFormulaGroupDescriptor(aCandidate);
        if (!oGroup)
            return;

        for (sal_Int32 nOffset = 0; nOffset < oGroup->mnLength; ++nOffset)
        {
            aAddresses.insert({ oGroup->maAnchor.mnSheet, oGroup->maAnchor.mnColumn,
                static_cast<api::RowIndex>(oGroup->maAnchor.mnRow + nOffset) });
        }
    };

    lCollect(static_cast<api::RowIndex>(rAddress.mnRow - 1));
    lCollect(rAddress.mnRow);
    lCollect(static_cast<api::RowIndex>(rAddress.mnRow + 1));
    return { aAddresses.begin(), aAddresses.end() };
}

[[nodiscard]] inline std::vector<NamedRangeId> collectNamedRangeDependenciesForFormulaAddresses(
    const dependency::DependencySnapshot& rSnapshot, const std::vector<api::CellAddress>& rAddresses)
{
    std::vector<NamedRangeId> aNamedRangeIds;
    for (const auto& rAddress : rAddresses)
    {
        const auto oFormulaNode = rSnapshot.findFormulaCellNode(rAddress);
        if (!oFormulaNode)
            continue;

        for (const auto& rDependency : rSnapshot.getDependencies(*oFormulaNode))
        {
            if (rDependency.maSource.meKind != dependency::DependencySourceKind::NamedRange)
                continue;

            if (std::find(aNamedRangeIds.begin(), aNamedRangeIds.end(),
                    rDependency.maSource.maNamedRangeId)
                == aNamedRangeIds.end())
            {
                aNamedRangeIds.push_back(rDependency.maSource.maNamedRangeId);
            }
        }
    }

    return aNamedRangeIds;
}

[[nodiscard]] inline bool namedRangeConsumersStayOnSheet(
    const dependency::DependencySnapshot& rSnapshot, const NamedRangeId& rId, api::SheetId nSheet)
{
    const auto oNamedRangeNode = rSnapshot.findNamedRangeNode(rId);
    if (!oNamedRangeNode)
        return false;

    for (const auto aDependentId : rSnapshot.getReverseDependents(*oNamedRangeNode))
    {
        const auto* pDependent = rSnapshot.getNode(aDependentId);
        if (!pDependent || pDependent->meKind != dependency::DependencyNodeKind::FormulaCell
            || !pDependent->moOutputAddress)
        {
            return false;
        }

        if (pDependent->moOutputAddress->mnSheet != nSheet)
            return false;
    }

    return true;
}

} // namespace detail

[[nodiscard]] inline SharedFormulaGroupTransition classifyFormulaGroupTransition(
    std::vector<FormulaGroupDescriptor> aBeforeGroups,
    std::vector<FormulaGroupDescriptor> aAfterGroups,
    const std::optional<MutationEvent>& oMutation = std::nullopt)
{
    SharedFormulaGroupTransition aTransition;
    aTransition.mnBeforeGroupCount = static_cast<sal_Int32>(aBeforeGroups.size());
    aTransition.mnAfterGroupCount = static_cast<sal_Int32>(aAfterGroups.size());

    auto lSort = [](std::vector<FormulaGroupDescriptor>& rGroups) {
        std::sort(rGroups.begin(), rGroups.end(),
            [](const FormulaGroupDescriptor& rLeft, const FormulaGroupDescriptor& rRight) {
                if (rLeft.maAnchor.mnSheet != rRight.maAnchor.mnSheet)
                    return rLeft.maAnchor.mnSheet < rRight.maAnchor.mnSheet;
                if (rLeft.maAnchor.mnColumn != rRight.maAnchor.mnColumn)
                    return rLeft.maAnchor.mnColumn < rRight.maAnchor.mnColumn;
                if (rLeft.maAnchor.mnRow != rRight.maAnchor.mnRow)
                    return rLeft.maAnchor.mnRow < rRight.maAnchor.mnRow;
                if (rLeft.mnLength != rRight.mnLength)
                    return rLeft.mnLength < rRight.mnLength;
                return rLeft.mbShareable < rRight.mbShareable;
            });
    };

    std::vector<FormulaGroupDescriptor> aExpectedGroups;
    aExpectedGroups.reserve(aBeforeGroups.size());
    for (const auto& rGroup : aBeforeGroups)
    {
        const auto oShifted = oMutation ? detail::shiftFormulaGroupDescriptor(rGroup, *oMutation)
                                        : std::optional<FormulaGroupDescriptor>(rGroup);
        if (!oShifted)
            continue;
        aExpectedGroups.push_back(*oShifted);
    }

    lSort(aExpectedGroups);
    lSort(aAfterGroups);

    if (aExpectedGroups.empty() && aAfterGroups.empty())
        return aTransition;

    if (aExpectedGroups == aAfterGroups)
    {
        aTransition.meKind = SharedFormulaGroupTransitionKind::Preserve;
        return aTransition;
    }

    const auto bSameCount = aExpectedGroups.size() == aAfterGroups.size();
    bool bAnyShareableChanged = false;
    if (bSameCount)
    {
        for (std::size_t nIndex = 0; nIndex < aExpectedGroups.size(); ++nIndex)
        {
            if (aExpectedGroups[nIndex].mbShareable != aAfterGroups[nIndex].mbShareable)
            {
                bAnyShareableChanged = true;
                break;
            }
        }
    }
    aTransition.mbShareableChanged = bAnyShareableChanged;

    if (aAfterGroups.empty() || aAfterGroups.size() < aExpectedGroups.size())
    {
        aTransition.meKind = SharedFormulaGroupTransitionKind::Split;
        return aTransition;
    }

    aTransition.meKind = SharedFormulaGroupTransitionKind::Rebuild;
    return aTransition;
}

[[nodiscard]] inline SharedFormulaMutationClassification classifySharedFormulaMutation(
    const WorkbookFacade& rBeforeFacade, const WorkbookFacade& rAfterFacade,
    const MutationEvent& rMutation)
{
    SharedFormulaMutationClassification aClassification;
    const auto aBeforeGroups = collectFormulaGroupDescriptors(rBeforeFacade);
    const auto aAfterGroups = collectFormulaGroupDescriptors(rAfterFacade);
    aClassification.maTransition
        = classifyFormulaGroupTransition(aBeforeGroups, aAfterGroups, rMutation);

    aClassification.mbTouchedAddressSharedBefore
        = rBeforeFacade.getFormulaGroupDescriptor(rMutation.maAddress).has_value();
    aClassification.mbTouchedAddressSharedAfter
        = rAfterFacade.getFormulaGroupDescriptor(rMutation.maAddress).has_value();
    const auto oTouchedGroupBefore = rBeforeFacade.getFormulaGroupDescriptor(rMutation.maAddress);
    const auto oTouchedGroupAfter = rAfterFacade.getFormulaGroupDescriptor(rMutation.maAddress);
    aClassification.mnBeforeNeighborhoodGroupCount = static_cast<sal_Int32>(
        detail::collectNeighborhoodGroups(rBeforeFacade, rMutation.maAddress).size());
    aClassification.mnAfterNeighborhoodGroupCount = static_cast<sal_Int32>(
        detail::collectNeighborhoodGroups(rAfterFacade, rMutation.maAddress).size());

    switch (rMutation.meKind)
    {
        case MutationKind::SetFormula:
        {
            const auto oBeforeFormula = rBeforeFacade.getFormulaCellDescriptor(rMutation.maAddress);
            if (aClassification.maTransition.meKind == SharedFormulaGroupTransitionKind::Preserve
                && oBeforeFormula && oBeforeFormula->maFormulaSource == rMutation.maText)
            {
                aClassification.meFamily = SharedFormulaMutationFamily::SameTextPreserve;
            }
            else if (!aClassification.mbTouchedAddressSharedAfter
                     && aClassification.mbTouchedAddressSharedBefore)
            {
                aClassification.meFamily = SharedFormulaMutationFamily::MemberExit;
            }
            else if (!aClassification.mbTouchedAddressSharedBefore
                     && aClassification.mbTouchedAddressSharedAfter
                     && aClassification.maTransition.meKind
                            == SharedFormulaGroupTransitionKind::Rebuild
                     && aClassification.mnBeforeNeighborhoodGroupCount == 1
                     && aClassification.mnAfterNeighborhoodGroupCount == 1)
            {
                aClassification.meFamily = SharedFormulaMutationFamily::OneSidedInsert;
            }
            else if (!aClassification.mbTouchedAddressSharedBefore
                     && aClassification.mbTouchedAddressSharedAfter
                     && aClassification.maTransition.meKind
                            == SharedFormulaGroupTransitionKind::Rebuild
                     && aClassification.mnBeforeNeighborhoodGroupCount == 2
                     && aClassification.mnAfterNeighborhoodGroupCount == 1)
            {
                aClassification.meFamily = SharedFormulaMutationFamily::Merge;
            }
            else if (aClassification.mbTouchedAddressSharedBefore
                     && aClassification.mbTouchedAddressSharedAfter
                     && aClassification.maTransition.meKind
                            == SharedFormulaGroupTransitionKind::Rebuild
                     && aClassification.mnBeforeNeighborhoodGroupCount == 3
                     && aClassification.mnAfterNeighborhoodGroupCount == 1
                     && oTouchedGroupBefore && oTouchedGroupAfter
                     && *oTouchedGroupBefore != *oTouchedGroupAfter)
            {
                aClassification.meFamily = SharedFormulaMutationFamily::MultiGroupCollapse;
            }
            else if (aClassification.mbTouchedAddressSharedBefore
                     && aClassification.mbTouchedAddressSharedAfter
                     && aClassification.maTransition.meKind
                            == SharedFormulaGroupTransitionKind::Rebuild
                     && aClassification.mnBeforeNeighborhoodGroupCount == 2
                     && aClassification.mnAfterNeighborhoodGroupCount == 1
                     && oTouchedGroupBefore && oTouchedGroupAfter
                     && *oTouchedGroupBefore != *oTouchedGroupAfter)
            {
                aClassification.meFamily = SharedFormulaMutationFamily::ReplacementMerge;
            }
            else if (oTouchedGroupBefore && oTouchedGroupAfter
                     && *oTouchedGroupBefore != *oTouchedGroupAfter)
            {
                aClassification.meFamily = SharedFormulaMutationFamily::Regroup;
            }
            break;
        }
        case MutationKind::SetScalarValue:
        case MutationKind::ClearCell:
            if (!aClassification.mbTouchedAddressSharedAfter
                && aClassification.mbTouchedAddressSharedBefore)
            {
                aClassification.meFamily = SharedFormulaMutationFamily::MemberExit;
            }
            break;
        case MutationKind::InsertRows:
        case MutationKind::DeleteRows:
        case MutationKind::InsertColumns:
        case MutationKind::DeleteColumns:
            switch (aClassification.maTransition.meKind)
            {
                case SharedFormulaGroupTransitionKind::Preserve:
                    aClassification.meFamily = SharedFormulaMutationFamily::StructuralPreserve;
                    break;
                case SharedFormulaGroupTransitionKind::Split:
                    aClassification.meFamily = SharedFormulaMutationFamily::StructuralSplit;
                    break;
                case SharedFormulaGroupTransitionKind::Rebuild:
                    aClassification.meFamily = SharedFormulaMutationFamily::StructuralRebuild;
                    break;
                case SharedFormulaGroupTransitionKind::None:
                    break;
            }
            break;
        default:
            break;
    }

    return aClassification;
}

[[nodiscard]] inline SharedFormulaNamedRangeMutationClassification
classifySharedFormulaNamedRangeMutationBoundary(
    const WorkbookFacade& rBeforeFacade, const WorkbookFacade& rAfterFacade,
    const MutationEvent& rMutation)
{
    SharedFormulaNamedRangeMutationClassification aClassification;

    const auto aBeforeRelevantAddresses
        = detail::collectRelevantSharedFormulaNeighborhoodAddresses(rBeforeFacade, rMutation.maAddress);
    const auto aAfterRelevantAddresses
        = detail::collectRelevantSharedFormulaNeighborhoodAddresses(rAfterFacade, rMutation.maAddress);
    const auto aBeforeSnapshot = dependency::buildDependencySnapshot(rBeforeFacade);
    const auto aAfterSnapshot = dependency::buildDependencySnapshot(rAfterFacade);
    auto aNamedRangeIds
        = detail::collectNamedRangeDependenciesForFormulaAddresses(aBeforeSnapshot, aBeforeRelevantAddresses);
    const auto aAfterNamedRangeIds
        = detail::collectNamedRangeDependenciesForFormulaAddresses(aAfterSnapshot, aAfterRelevantAddresses);
    for (const auto& rId : aAfterNamedRangeIds)
    {
        if (std::find(aNamedRangeIds.begin(), aNamedRangeIds.end(), rId) == aNamedRangeIds.end())
            aNamedRangeIds.push_back(rId);
    }

    if (aNamedRangeIds.empty())
        return aClassification;

    aClassification.mnNamedRangeCount = static_cast<sal_Int32>(aNamedRangeIds.size());
    aClassification.mbDescriptorsStable = true;
    aClassification.mbAllConsumersStayOnSheet = true;
    for (const auto& rId : aNamedRangeIds)
    {
        const auto oBeforeDescriptor = detail::findNamedRangeDescriptorById(rBeforeFacade, rId);
        const auto oAfterDescriptor = detail::findNamedRangeDescriptorById(rAfterFacade, rId);
        if (!oBeforeDescriptor || !oAfterDescriptor || *oBeforeDescriptor != *oAfterDescriptor)
        {
            aClassification.mbDescriptorsStable = false;
            aClassification.meBoundary = SharedFormulaNamedRangeMutationBoundary::Deferred;
            return aClassification;
        }

        if (oBeforeDescriptor->meScope != NamedRangeScope::Global)
        {
            aClassification.meBoundary = SharedFormulaNamedRangeMutationBoundary::Deferred;
            return aClassification;
        }

        const auto oTarget
            = detail::parseSingleAreaNamedRangeTarget(rBeforeFacade, *oBeforeDescriptor);
        if (!oTarget || oTarget->maStart.mnSheet != rMutation.maAddress.mnSheet
            || oTarget->maEnd.mnSheet != rMutation.maAddress.mnSheet)
        {
            aClassification.meBoundary = SharedFormulaNamedRangeMutationBoundary::Deferred;
            return aClassification;
        }

        if (!detail::namedRangeConsumersStayOnSheet(
                aBeforeSnapshot, rId, rMutation.maAddress.mnSheet)
            || !detail::namedRangeConsumersStayOnSheet(
                aAfterSnapshot, rId, rMutation.maAddress.mnSheet))
        {
            aClassification.mbAllConsumersStayOnSheet = false;
            aClassification.meBoundary = SharedFormulaNamedRangeMutationBoundary::Deferred;
            return aClassification;
        }
    }

    aClassification.meBoundary
        = SharedFormulaNamedRangeMutationBoundary::GlobalSingleAreaSameSheet;
    return aClassification;
}

/// Scan all formula cells and summarize shared-formula groups.
/// This is a low-risk direct consumer that uses the facade's group
/// descriptor query.
[[nodiscard]] inline SharedFormulaGroupSummary
summarizeFormulaGroups(const WorkbookFacade& rFacade)
{
    SharedFormulaGroupSummary aSummary;
    for (const auto& rGroup : collectFormulaGroupDescriptors(rFacade))
    {
        ++aSummary.mnGroupCount;
        aSummary.mnTotalGroupLength += rGroup.mnLength;
        if (rGroup.mbShareable)
            ++aSummary.mnShareableGroups;
    }

    return aSummary;
}

/// Result of a named-range shadow inventory.
struct NamedRangeInventory
{
    sal_Int32 mnGlobalCount = 0;
    sal_Int32 mnSheetLocalCount = 0;
};

/// Inventory all named ranges through the facade.
[[nodiscard]] inline NamedRangeInventory
inventoryNamedRanges(const WorkbookFacade& rFacade)
{
    NamedRangeInventory aInventory;

    const auto aRanges = rFacade.getNamedRangeDescriptors();
    for (const auto& rRange : aRanges)
    {
        if (rRange.meScope == NamedRangeScope::Global)
            ++aInventory.mnGlobalCount;
        else
            ++aInventory.mnSheetLocalCount;
    }

    return aInventory;
}

/// Snapshot comparison result for differential validation.
struct SnapshotComparison
{
    bool mbSheetCountMatch = false;
    bool mbFormulaCellCountMatch = false;
    bool mbFullMatch = false;
};

/// Compare two facade snapshots for differential validation.
[[nodiscard]] inline SnapshotComparison
compareSnapshots(const WorkbookSnapshotInfo& rLeft, const WorkbookSnapshotInfo& rRight)
{
    SnapshotComparison aResult;
    aResult.mbSheetCountMatch = (rLeft.mnSheetCount == rRight.mnSheetCount);
    aResult.mbFormulaCellCountMatch
        = (rLeft.mnFormulaCellCount == rRight.mnFormulaCellCount);
    aResult.mbFullMatch = (rLeft == rRight);
    return aResult;
}

/// Collect formula source strings for a representative corpus, suitable
/// for feeding into compile-diff or shadow compiler harnesses.
[[nodiscard]] inline std::vector<std::pair<api::CellAddress, api::String>>
collectFormulaCorpus(const WorkbookFacade& rFacade, sal_Int32 nMaxFormulas = -1)
{
    std::vector<std::pair<api::CellAddress, api::String>> aCorpus;
    sal_Int32 nCollected = 0;

    rFacade.visitAllFormulaCells(
        [&aCorpus, &nCollected, nMaxFormulas](const FormulaCellDescriptor& rDesc) {
            aCorpus.emplace_back(rDesc.maId.maAddress, rDesc.maFormulaSource);
            ++nCollected;
            if (nMaxFormulas >= 0 && nCollected >= nMaxFormulas)
                return false;
            return true;
        });

    return aCorpus;
}

} // namespace spreadsheetengine::detail::facade::consumers

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
