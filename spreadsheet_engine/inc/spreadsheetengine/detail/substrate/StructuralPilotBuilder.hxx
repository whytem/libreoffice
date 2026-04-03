/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>
#include <set>

#include <spreadsheetengine/runtime/ReferenceText.hxx>
#include <spreadsheetengine/api/ReferenceUpdate.hxx>
#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>
#include <spreadsheetengine/detail/dependency/RecalcPlanner.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilot.hxx>

namespace spreadsheetengine::detail::substrate
{

namespace structuralbuilddetail
{

using spreadsheetengine::detail::substrate::detail::AddressLess;
using spreadsheetengine::detail::substrate::detail::collectShadowCellAddresses;
using spreadsheetengine::detail::substrate::detail::collectShadowFormulaGroups;
using spreadsheetengine::detail::substrate::detail::sortNamedRanges;

[[nodiscard]] inline api::refdata::SheetLimits makeStructuralSheetLimits()
{
    return { 1023, 65535, 15 };
}

[[nodiscard]] inline bool isOrdinaryScalarFormulaCell(const ShadowCellRecord& rCell)
{
    return rCell.hasFormula() && rCell.moFormula->meKind == facade::FormulaCellKind::Ordinary
           && !rCell.moFormulaGroup.has_value();
}

[[nodiscard]] inline bool isAdmittedStructuralSlice(const ComputationalWorkbookShadow& rShadow)
{
    if (!rShadow.maNamedRanges.empty() || !rShadow.maFormulaGroups.empty())
        return false;

    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;
            if (!isOrdinaryScalarFormulaCell(rCell))
                return false;
        }
    }

    return true;
}

[[nodiscard]] inline std::optional<api::SheetId> findSheetIdByName(
    const ComputationalWorkbookShadow& rShadow, api::StringView rSheetName)
{
    for (const auto& rSheet : rShadow.maSheets)
    {
        if (rSheet.maSheet.maName == rSheetName)
            return rSheet.maSheet.mnId;
    }
    return std::nullopt;
}

[[nodiscard]] inline const facade::SheetDescriptor* findSheetDescriptor(
    const ComputationalWorkbookShadow& rShadow, api::SheetId nSheet)
{
    for (const auto& rSheet : rShadow.maSheets)
    {
        if (rSheet.maSheet.mnId == nSheet)
            return &rSheet.maSheet;
    }
    return nullptr;
}

[[nodiscard]] inline std::optional<api::ColumnIndex> parseStructuralColumnName(
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
    const ComputationalWorkbookShadow& rShadow, api::StringView rToken, api::SheetId nImplicitSheet)
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
            const auto oSheet
                = findSheetIdByName(rShadow, runtime::referencetext::unquoteSheetName(aSheetToken));
            if (!oSheet)
                return std::nullopt;
            nSheet = *oSheet;
        }
        aAddressToken = rToken.substr(nDotPos + 1);
    }

    if (!isAbsoluteAddressToken(aAddressToken))
        return std::nullopt;

    aAddressToken.remove_prefix(1); // leading $
    std::size_t nColumnEnd = 0;
    while (nColumnEnd < aAddressToken.size())
    {
        const char16_t cChar = aAddressToken[nColumnEnd];
        const bool bAlpha = (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
        if (!bAlpha)
            break;
        ++nColumnEnd;
    }

    const auto oColumn = parseStructuralColumnName(aAddressToken.substr(0, nColumnEnd));
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
    const ComputationalWorkbookShadow& rShadow, const facade::NamedRangeDescriptor& rNamedRange)
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
            rShadow, rNamedRange.maTargetExpression, rNamedRange.maBaseAddress.mnSheet);
        if (!oSingle)
            return std::nullopt;
        return api::CellRange { *oSingle, *oSingle };
    }

    const auto oStart = parseAbsoluteNamedRangeAddressToken(
        rShadow, rNamedRange.maTargetExpression.substr(0, nColonPos), rNamedRange.maBaseAddress.mnSheet);
    if (!oStart)
        return std::nullopt;

    const auto oEnd = parseAbsoluteNamedRangeAddressToken(
        rShadow, rNamedRange.maTargetExpression.substr(nColonPos + 1), oStart->mnSheet);
    if (!oEnd)
        return std::nullopt;

    api::CellRange aRange { *oStart, *oEnd };
    if (aRange.maStart.mnSheet != aRange.maEnd.mnSheet)
        return std::nullopt;

    return dependency::detail::normalizeRange(aRange);
}

[[nodiscard]] inline api::String formatAbsoluteNamedRangeAddressToken(
    const ComputationalWorkbookShadow& rShadow, const api::CellAddress& rAddress, bool bIncludeSheet)
{
    api::String aToken;
    if (bIncludeSheet)
    {
        const auto* pSheet = findSheetDescriptor(rShadow, rAddress.mnSheet);
        if (pSheet)
        {
            aToken.push_back(u'$');
            aToken += runtime::referencetext::quoteSheetNameForFormula(pSheet->maName);
            aToken.push_back(u'.');
        }
    }

    aToken.push_back(u'$');
    aToken += runtime::referencetext::columnNameFromIndex(rAddress.mnColumn);
    aToken.push_back(u'$');
    aToken += runtime::referencetext::formatPositiveInteger(
        static_cast<std::int64_t>(rAddress.mnRow) + 1);
    return aToken;
}

[[nodiscard]] inline api::String formatSingleAreaNamedRangeTarget(
    const ComputationalWorkbookShadow& rShadow,
    const facade::NamedRangeDescriptor& rNamedRange,
    const api::CellRange& rRange)
{
    const std::size_t nColonPos = rNamedRange.maTargetExpression.find(u':');
    const api::StringView aStartToken
        = nColonPos == api::StringView::npos ? api::StringView(rNamedRange.maTargetExpression)
                                             : api::StringView(rNamedRange.maTargetExpression)
                                                   .substr(0, nColonPos);
    const bool bIncludeStartSheet = aStartToken.rfind(u'.') != api::StringView::npos;

    api::String aResult = formatAbsoluteNamedRangeAddressToken(
        rShadow, rRange.maStart, bIncludeStartSheet);
    if (rRange.maStart == rRange.maEnd)
        return aResult;

    aResult.push_back(u':');
    aResult += formatAbsoluteNamedRangeAddressToken(
        rShadow, rRange.maEnd, rRange.maEnd.mnSheet != rRange.maStart.mnSheet);
    return aResult;
}

[[nodiscard]] inline std::optional<facade::NamedRangeDescriptor> findNamedRangeById(
    const std::vector<facade::NamedRangeDescriptor>& rNamedRanges,
    const facade::NamedRangeId& rId)
{
    auto it = std::find_if(rNamedRanges.begin(), rNamedRanges.end(),
        [&rId](const facade::NamedRangeDescriptor& rDescriptor) {
            return rDescriptor.maId == rId;
        });
    if (it == rNamedRanges.end())
        return std::nullopt;
    return *it;
}

[[nodiscard]] inline bool hasNamedRangeScopeAmbiguity(
    const std::vector<facade::NamedRangeDescriptor>& rNamedRanges)
{
    std::set<api::String> aSeenNames;
    for (const auto& rNamedRange : rNamedRanges)
    {
        api::String aFolded = rNamedRange.maName;
        std::transform(aFolded.begin(), aFolded.end(), aFolded.begin(), [](char16_t cChar) {
            if (cChar >= u'a' && cChar <= u'z')
                return static_cast<char16_t>(cChar - u'a' + u'A');
            return cChar;
        });

        if (!aSeenNames.insert(aFolded).second)
            return true;
    }

    return false;
}

[[nodiscard]] inline bool isNamedRangeStructuralValidationSlice(
    const ComputationalWorkbookShadow& rShadow)
{
    if (rShadow.maNamedRanges.empty() || !rShadow.maFormulaGroups.empty()
        || hasNamedRangeScopeAmbiguity(rShadow.maNamedRanges))
    {
        return false;
    }

    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
        {
            if (!rCell.hasFormula())
                continue;
            if (!isOrdinaryScalarFormulaCell(rCell))
                return false;
        }
    }

    return std::all_of(rShadow.maNamedRanges.begin(), rShadow.maNamedRanges.end(),
        [&rShadow](const facade::NamedRangeDescriptor& rNamedRange) {
            return parseSingleAreaNamedRangeTarget(rShadow, rNamedRange).has_value();
        });
}

[[nodiscard]] inline std::optional<api::CellAddress> shiftAddress(
    const facade::MutationEvent& rMutation, const api::CellAddress& rAddress)
{
    if (rAddress.mnSheet != rMutation.mnSheet)
        return rAddress;

    switch (rMutation.meKind)
    {
        case facade::MutationKind::InsertRows:
            if (rAddress.mnRow >= rMutation.maAddress.mnRow)
                return api::CellAddress { rAddress.mnSheet, rAddress.mnColumn,
                    static_cast<api::RowIndex>(rAddress.mnRow + rMutation.mnCount) };
            return rAddress;
        case facade::MutationKind::DeleteRows:
            if (rAddress.mnRow >= rMutation.maAddress.mnRow
                && rAddress.mnRow < rMutation.maAddress.mnRow + rMutation.mnCount)
            {
                return std::nullopt;
            }
            if (rAddress.mnRow >= rMutation.maAddress.mnRow + rMutation.mnCount)
            {
                return api::CellAddress { rAddress.mnSheet, rAddress.mnColumn,
                    static_cast<api::RowIndex>(rAddress.mnRow - rMutation.mnCount) };
            }
            return rAddress;
        case facade::MutationKind::InsertColumns:
            if (rAddress.mnColumn >= rMutation.maAddress.mnColumn)
                return api::CellAddress { rAddress.mnSheet,
                    static_cast<api::ColumnIndex>(rAddress.mnColumn + rMutation.mnCount),
                    rAddress.mnRow };
            return rAddress;
        case facade::MutationKind::DeleteColumns:
            if (rAddress.mnColumn >= rMutation.maAddress.mnColumn
                && rAddress.mnColumn < rMutation.maAddress.mnColumn + rMutation.mnCount)
            {
                return std::nullopt;
            }
            if (rAddress.mnColumn >= rMutation.maAddress.mnColumn + rMutation.mnCount)
            {
                return api::CellAddress { rAddress.mnSheet,
                    static_cast<api::ColumnIndex>(rAddress.mnColumn - rMutation.mnCount),
                    rAddress.mnRow };
            }
            return rAddress;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] inline std::optional<api::CellRange> shiftRange(
    const facade::MutationEvent& rMutation, api::CellRange aRange)
{
    const auto oPlan = makeExecutionIrStructuralUpdatePlan(rMutation, makeStructuralSheetLimits());
    if (!oPlan)
        return std::nullopt;

    const auto eResult = api::refupdate::updateReference(oPlan->meMode, oPlan->maWhere, oPlan->mnDx,
        oPlan->mnDy, oPlan->mnDz, 1023, 65535, 15, oPlan->mbExpandRefs, aRange);
    if (eResult == api::refupdate::UpdateResult::Invalid)
        return std::nullopt;
    return dependency::detail::normalizeRange(aRange);
}

[[nodiscard]] inline std::optional<ListenerAnchorId> shiftListenerAnchor(
    const facade::MutationEvent& rMutation, const ListenerAnchorId& rAnchor)
{
    if (rAnchor.meKind != ListenerAnchorKind::FormulaCell)
        return std::nullopt;

    const auto oShifted = shiftAddress(rMutation, rAnchor.maAnchor);
    if (!oShifted)
        return std::nullopt;

    ListenerAnchorId aShifted = rAnchor;
    aShifted.maAnchor = *oShifted;
    return aShifted;
}

inline void sortComputationalShadowForComparison(ComputationalWorkbookShadow& rShadow)
{
    for (auto& rSheet : rShadow.maSheets)
    {
        std::sort(rSheet.maCells.begin(), rSheet.maCells.end(),
            [](const ShadowCellRecord& rLeft, const ShadowCellRecord& rRight) {
                return AddressLess {}(rLeft.maId.maAddress, rRight.maId.maAddress);
            });
    }

    std::sort(rShadow.maFormulaTree.begin(), rShadow.maFormulaTree.end(), AddressLess {});
    std::sort(rShadow.maFormulaTrack.begin(), rShadow.maFormulaTrack.end(), AddressLess {});
    std::sort(rShadow.maCellBroadcasters.begin(), rShadow.maCellBroadcasters.end(),
        [](const CellBroadcasterRecord& rLeft, const CellBroadcasterRecord& rRight) {
            return AddressLess {}(rLeft.maBroadcaster, rRight.maBroadcaster);
        });
    std::sort(rShadow.maAreaBroadcasters.begin(), rShadow.maAreaBroadcasters.end(),
        [](const AreaBroadcasterRecord& rLeft, const AreaBroadcasterRecord& rRight) {
            if (!(rLeft.maBroadcaster.maStart == rRight.maBroadcaster.maStart))
                return AddressLess {}(rLeft.maBroadcaster.maStart, rRight.maBroadcaster.maStart);
            return AddressLess {}(rLeft.maBroadcaster.maEnd, rRight.maBroadcaster.maEnd);
        });
    for (auto& rRecord : rShadow.maCellBroadcasters)
        graphmapping::sortAndUnique(rRecord.maListeners, graphmapping::ListenerAnchorIdLess {});
    for (auto& rRecord : rShadow.maAreaBroadcasters)
        graphmapping::sortAndUnique(rRecord.maListeners, graphmapping::ListenerAnchorIdLess {});
}

[[nodiscard]] inline ComputationalWorkbookShadow buildPredictedStructuralComputationalShadow(
    const ComputationalWorkbookShadow& rBefore, const facade::MutationEvent& rMutation,
    const ComputationalWorkbookShadow& rObservedAfter)
{
    ComputationalWorkbookShadow aPredicted = rBefore;
    aPredicted.maSnapshot = rObservedAfter.maSnapshot;
    aPredicted.maFormulaGroups.clear();
    aPredicted.maCellBroadcasters.clear();
    aPredicted.maAreaBroadcasters.clear();
    aPredicted.maFormulaTree.clear();
    aPredicted.maFormulaTrack.clear();

    for (auto& rSheet : aPredicted.maSheets)
        rSheet.maCells.clear();

    for (const auto& rSheet : rBefore.maSheets)
    {
        auto& rTargetSheet = aPredicted.maSheets[static_cast<std::size_t>(rSheet.maSheet.mnId)];
        for (const auto& rCell : rSheet.maCells)
        {
            const auto oShifted = shiftAddress(rMutation, rCell.maId.maAddress);
            if (!oShifted)
                continue;

            ShadowCellRecord aShifted = rCell;
            aShifted.maId.maAddress = *oShifted;
            aShifted.maCell.maAddress = *oShifted;
            if (aShifted.moFormula)
                aShifted.moFormula->maId.maAddress = *oShifted;
            rTargetSheet.maCells.push_back(std::move(aShifted));
        }
    }

    for (const auto& rAddress : rBefore.maFormulaTree)
    {
        if (const auto oShifted = shiftAddress(rMutation, rAddress))
            aPredicted.maFormulaTree.push_back(*oShifted);
    }

    for (const auto& rAddress : rBefore.maFormulaTrack)
    {
        if (const auto oShifted = shiftAddress(rMutation, rAddress))
            aPredicted.maFormulaTrack.push_back(*oShifted);
    }

    aPredicted.maNamedRanges.clear();
    if (!rBefore.maNamedRanges.empty())
    {
        for (const auto& rNamedRange : rBefore.maNamedRanges)
        {
            const auto oObservedAfter
                = findNamedRangeById(rObservedAfter.maNamedRanges, rNamedRange.maId);
            const auto oTarget = parseSingleAreaNamedRangeTarget(rBefore, rNamedRange);
            if (!oObservedAfter || !oTarget)
            {
                aPredicted.maNamedRanges.clear();
                return aPredicted;
            }

            const auto oShiftedTarget = shiftRange(rMutation, *oTarget);
            if (!oShiftedTarget)
            {
                aPredicted.maNamedRanges.clear();
                return aPredicted;
            }

            facade::NamedRangeDescriptor aShifted = *oObservedAfter;
            aShifted.maTargetExpression
                = formatSingleAreaNamedRangeTarget(rObservedAfter, rNamedRange, *oShiftedTarget);
            aPredicted.maNamedRanges.push_back(std::move(aShifted));
        }
    }

    for (const auto& rBroadcaster : rBefore.maCellBroadcasters)
    {
        const auto oBroadcaster = shiftAddress(rMutation, rBroadcaster.maBroadcaster);
        if (!oBroadcaster)
            continue;

        CellBroadcasterRecord aShifted;
        aShifted.maBroadcaster = *oBroadcaster;
        for (const auto& rListener : rBroadcaster.maListeners)
        {
            if (const auto oListener = shiftListenerAnchor(rMutation, rListener))
                aShifted.maListeners.push_back(*oListener);
        }
        if (!aShifted.maListeners.empty())
            aPredicted.maCellBroadcasters.push_back(std::move(aShifted));
    }

    for (const auto& rBroadcaster : rBefore.maAreaBroadcasters)
    {
        const auto oBroadcaster = shiftRange(rMutation, rBroadcaster.maBroadcaster);
        if (!oBroadcaster)
            continue;

        AreaBroadcasterRecord aShifted;
        aShifted.maBroadcaster = *oBroadcaster;
        for (const auto& rListener : rBroadcaster.maListeners)
        {
            if (const auto oListener = shiftListenerAnchor(rMutation, rListener))
                aShifted.maListeners.push_back(*oListener);
        }
        if (!aShifted.maListeners.empty())
            aPredicted.maAreaBroadcasters.push_back(std::move(aShifted));
    }

    sortComputationalShadowForComparison(aPredicted);
    return aPredicted;
}

[[nodiscard]] inline bool matchesPredictedStructuralPopulation(
    const ComputationalWorkbookShadow& rPredicted,
    const ComputationalWorkbookShadow& rObserved)
{
    return collectShadowCellAddresses(rPredicted) == collectShadowCellAddresses(rObserved)
           && collectShadowFormulaGroups(rPredicted) == collectShadowFormulaGroups(rObserved)
           && sortNamedRanges(rPredicted.maNamedRanges) == sortNamedRanges(rObserved.maNamedRanges);
}

[[nodiscard]] inline ExecutionIrReferenceUpdateSummary updateShiftedFormulaReferences(
    ExecutionIrFormulaRecord& rFormula, const api::CellAddress& rOldPosition,
    const api::CellAddress& rNewPosition, const ExecutionIrStructuralUpdatePlan& rPlan)
{
    ExecutionIrReferenceUpdateSummary aSummary;
    const auto aLimits = makeStructuralSheetLimits();

    for (auto& rInstruction : rFormula.maInstructions)
    {
        api::refupdate::UpdateResult eResult = api::refupdate::UpdateResult::Nothing;
        bool bChanged = false;

        auto lUpdateSingle = [&](api::refdata::SingleRefData& rReference) {
            const auto aBefore = rReference;
            const auto aAbsolute
                = api::refdata::toAbsoluteAddress(rReference, aLimits, rOldPosition);
            api::CellRange aRange { aAbsolute, aAbsolute };
            eResult = api::refupdate::updateReference(rPlan.meMode, rPlan.maWhere, rPlan.mnDx,
                rPlan.mnDy, rPlan.mnDz, aLimits.mnMaxColumn, aLimits.mnMaxRow,
                aLimits.mnMaxSheet, rPlan.mbExpandRefs, aRange);
            api::refdata::setAddress(rReference, aLimits, aRange.maStart, rNewPosition);
            bChanged = rReference != aBefore;
        };

        auto lUpdateRange = [&](api::refdata::ComplexRefData& rReference) {
            const auto aBefore = rReference;
            auto aAbsolute = api::refdata::toAbsoluteRange(rReference, aLimits, rOldPosition);
            eResult = api::refupdate::updateReference(rPlan.meMode, rPlan.maWhere, rPlan.mnDx,
                rPlan.mnDy, rPlan.mnDz, aLimits.mnMaxColumn, aLimits.mnMaxRow,
                aLimits.mnMaxSheet, rPlan.mbExpandRefs, aAbsolute);
            api::refdata::setRange(rReference, aLimits, aAbsolute, rNewPosition);
            bChanged = rReference != aBefore;
        };

        switch (rInstruction.meKind)
        {
            case ExecutionIrInstructionKind::SingleReference:
            case ExecutionIrInstructionKind::ColumnRowNameReference:
                if (auto* pReference = std::get_if<api::refdata::SingleRefData>(&rInstruction.maPayload))
                    lUpdateSingle(*pReference);
                break;
            case ExecutionIrInstructionKind::RangeReference:
                if (auto* pReference = std::get_if<api::refdata::ComplexRefData>(&rInstruction.maPayload))
                    lUpdateRange(*pReference);
                break;
            case ExecutionIrInstructionKind::ExternalSingleReference:
                if (auto* pReference = std::get_if<ExecutionIrExternalSingleRefData>(&rInstruction.maPayload))
                    lUpdateSingle(pReference->maReference);
                break;
            case ExecutionIrInstructionKind::ExternalRangeReference:
                if (auto* pReference = std::get_if<ExecutionIrExternalDoubleRefData>(&rInstruction.maPayload))
                    lUpdateRange(pReference->maReference);
                break;
            default:
                break;
        }

        irrefdetail::foldUpdateResult(aSummary, eResult, bChanged);
    }

    return aSummary;
}

[[nodiscard]] inline ExecutionIrWorkbookShadow buildPredictedStructuralIrShadow(
    const StructuralPilotInput& rInput)
{
    ExecutionIrWorkbookShadow aPredicted;
    aPredicted.maSnapshot = rInput.maObservedAfterIrShadow.maSnapshot;
    aPredicted.maGrammar = rInput.maIrShadow.maGrammar;
    aPredicted.maFormulaGroups = rInput.maObservedAfterIrShadow.maFormulaGroups;
    aPredicted.maBuildFailures = rInput.maObservedAfterIrShadow.maBuildFailures;

    const auto oPlan
        = makeExecutionIrStructuralUpdatePlan(rInput.maMutation, makeStructuralSheetLimits());

    std::map<api::CellAddress, const ExecutionIrFormulaRecord*, AddressLess> aObservedAfterByAddress;
    for (const auto& rFormula : rInput.maObservedAfterIrShadow.maFormulaRecords)
        aObservedAfterByAddress[rFormula.maId.maAddress] = &rFormula;

    for (const auto& rFormula : rInput.maIrShadow.maFormulaRecords)
    {
        const auto oShifted = shiftAddress(rInput.maMutation, rFormula.maId.maAddress);
        if (!oShifted)
            continue;

        ExecutionIrFormulaRecord aPredictedRecord = rFormula;
        if (const auto itObserved = aObservedAfterByAddress.find(*oShifted);
            itObserved != aObservedAfterByAddress.end())
        {
            aPredictedRecord = *itObserved->second;
        }

        aPredictedRecord.maId.maAddress = *oShifted;
        if (oPlan)
        {
            aPredictedRecord.maInstructions = rFormula.maInstructions;
            const auto aSummary = updateShiftedFormulaReferences(
                aPredictedRecord, rFormula.maId.maAddress, *oShifted, *oPlan);
            (void)aSummary;
        }

        aPredictedRecord.mbInFormulaTree
            = std::find(rInput.maObservedAfterComputationalShadow.maFormulaTree.begin(),
                   rInput.maObservedAfterComputationalShadow.maFormulaTree.end(),
                   *oShifted)
              != rInput.maObservedAfterComputationalShadow.maFormulaTree.end();
        aPredictedRecord.mbInFormulaTrack
            = std::find(rInput.maObservedAfterComputationalShadow.maFormulaTrack.begin(),
                   rInput.maObservedAfterComputationalShadow.maFormulaTrack.end(),
                   *oShifted)
              != rInput.maObservedAfterComputationalShadow.maFormulaTrack.end();
        aPredicted.maFormulaRecords.push_back(std::move(aPredictedRecord));
    }

    irdetail::normalizeFormulaRecords(aPredicted.maFormulaRecords);
    return aPredicted;
}

} // namespace structuralbuilddetail

[[nodiscard]] inline StructuralPilotTransition buildStructuralPilotTransition(
    const StructuralPilotInput& rInput, const facade::WorkbookFacade& rAfterFacade,
    const ComputationalObservationState&,
    StructuralPilotBuildMode eBuildMode = StructuralPilotBuildMode::AuthorityOnly)
{
    StructuralPilotTransition aTransition;
    aTransition.maInput = rInput;
    aTransition.maContract = structuraldetail::classifyStructuralMutation(rInput.maMutation);

    if (aTransition.maContract.mbRequiresCleanBaseline && !rInput.mbCleanBaseline)
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedDirtyBaseline;
        aTransition.maReason = u"dirty_baseline";
        return aTransition;
    }

    const bool bAdmittedStructuralSlice
        = structuralbuilddetail::isAdmittedStructuralSlice(rInput.maComputationalShadow)
        && structuralbuilddetail::isAdmittedStructuralSlice(rInput.maObservedAfterComputationalShadow);
    const bool bNamedRangeValidationSlice
        = structuralbuilddetail::isNamedRangeStructuralValidationSlice(rInput.maComputationalShadow)
        && structuralbuilddetail::isNamedRangeStructuralValidationSlice(
            rInput.maObservedAfterComputationalShadow);

    if (!bAdmittedStructuralSlice)
    {
        if (eBuildMode == StructuralPilotBuildMode::Validation && bNamedRangeValidationSlice)
            aTransition.maContract.meMutationClass = StructuralMutationClass::ValidationOnly;
        else
        {
            aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
            aTransition.maReason = u"structural_slice_out_of_contract";
            return aTransition;
        }
    }

    aTransition.maVerification = structuraldetail::makeStructuralVerification(aTransition.maContract);
    if (!aTransition.maContract.isAllowedInBuildMode(eBuildMode))
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"mutation_out_of_contract";
        return aTransition;
    }

    if (!rInput.maIrShadow.maBuildFailures.empty()
        || !rInput.maObservedAfterIrShadow.maBuildFailures.empty())
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"structural_slice_out_of_contract";
        return aTransition;
    }

    StructuralSyncAction aSyncAction;
    aSyncAction.mnSheet = rInput.maMutation.mnSheet;
    aSyncAction.mnStartRow = rInput.maMutation.maAddress.mnRow;
    aSyncAction.mnStartColumn = rInput.maMutation.maAddress.mnColumn;
    aSyncAction.mnCount = rInput.maMutation.mnCount;
    switch (rInput.maMutation.meKind)
    {
        case facade::MutationKind::InsertRows:
            aSyncAction.meKind = StructuralSyncActionKind::InsertRows;
            break;
        case facade::MutationKind::DeleteRows:
            aSyncAction.meKind = StructuralSyncActionKind::DeleteRows;
            break;
        case facade::MutationKind::InsertColumns:
            aSyncAction.meKind = StructuralSyncActionKind::InsertColumns;
            break;
        case facade::MutationKind::DeleteColumns:
            aSyncAction.meKind = StructuralSyncActionKind::DeleteColumns;
            break;
        default:
            aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
            aTransition.maReason = u"unsupported_structural_sync";
            return aTransition;
    }
    aTransition.maSyncActions.push_back(aSyncAction);

    const auto aPredictedComputational
        = structuralbuilddetail::buildPredictedStructuralComputationalShadow(
            rInput.maComputationalShadow, rInput.maMutation, rInput.maObservedAfterComputationalShadow);
    if (!structuralbuilddetail::matchesPredictedStructuralPopulation(
            aPredictedComputational, rInput.maObservedAfterComputationalShadow))
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"structural_population_mismatch";
        return aTransition;
    }

    aTransition.maIrAfter = structuralbuilddetail::buildPredictedStructuralIrShadow(rInput);
    const auto aIrComparison
        = compareExecutionIrWorkbookShadow(aTransition.maIrAfter, rInput.maObservedAfterIrShadow);
    if (aIrComparison.meKind == ExecutionIrComparisonKind::Mismatch)
    {
        aTransition.meVerdict = StructuralPilotVerdict::RepairDetected;
        aTransition.maReason = u"structural_reference_update_mismatch";
        aTransition.mbRequiresRollback = true;
        return aTransition;
    }

    if (const auto oPlan = makeExecutionIrStructuralUpdatePlan(
            rInput.maMutation, structuralbuilddetail::makeStructuralSheetLimits()))
    {
        for (const auto& rFormula : rInput.maIrShadow.maFormulaRecords)
        {
            StructuralReferenceUpdateRecord aReferenceUpdate;
            aReferenceUpdate.maBeforeId = rFormula.maId;
            if (const auto oShifted
                = structuralbuilddetail::shiftAddress(rInput.maMutation, rFormula.maId.maAddress))
            {
                aReferenceUpdate.moAfterId = ShadowCellId { *oShifted };
                auto aTmp = rFormula;
                aReferenceUpdate.maSummary = structuralbuilddetail::updateShiftedFormulaReferences(
                    aTmp, rFormula.maId.maAddress, *oShifted, *oPlan);
            }
            else
            {
                aReferenceUpdate.mbRemovedByStructure = true;
            }
            aTransition.maReferenceUpdates.push_back(std::move(aReferenceUpdate));
        }
    }

    aTransition.maDependencySnapshot = dependency::buildDependencySnapshot(rAfterFacade);
    if (aTransition.maDependencySnapshot.maReport.mnOpaqueNodeCount > 0
        || aTransition.maDependencySnapshot.maReport.mnOpaqueEdgeCount > 0)
    {
        aTransition.meVerdict = StructuralPilotVerdict::RejectedOutOfContract;
        aTransition.maReason = u"opaque_dependency_surface";
        return aTransition;
    }

    aTransition.maInvalidationPlan
        = dependency::planInvalidation(aTransition.maDependencySnapshot, rInput.maMutation);
    aTransition.maRecalcPlan
        = dependency::buildRecalcPlan(aTransition.maDependencySnapshot, aTransition.maInvalidationPlan);

    const auto aPredictedObservation = lifecyclebuilddetail::buildLifecycleObservationState(
        aTransition.maDependencySnapshot, aTransition.maRecalcPlan);
    aTransition.maComputationalAfter
        = buildComputationalWorkbookShadow(rAfterFacade, aPredictedObservation);
    aTransition.maGraphAfter
        = buildDependencyGraphShadow(aTransition.maComputationalAfter, aPredictedObservation);
    aTransition.meVerdict = StructuralPilotVerdict::Applicable;
    aTransition.maReason = u"ready";
    return aTransition;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
