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
#include <utility>
#include <vector>

#include <broadcast.hxx>
#include <document.hxx>
#include <dociter.hxx>
#include <formulacell.hxx>
#include <grouparealistener.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>

namespace spreadsheetengine::compat::libreoffice::substrateobs
{

enum class ListenerKind : sal_uInt8
{
    FormulaCell,
    FormulaGroup,
    HostUnknown
};

struct ListenerSnapshot
{
    ListenerKind meKind = ListenerKind::HostUnknown;
    api::CellAddress maAnchor;
    sal_Int32 mnLength = 0;

    [[nodiscard]] constexpr bool operator==(const ListenerSnapshot& rOther) const = default;
};

struct CellBroadcasterSnapshot
{
    api::CellAddress maBroadcaster;
    std::vector<ListenerSnapshot> maListeners;

    [[nodiscard]] constexpr bool operator==(const CellBroadcasterSnapshot& rOther) const = default;
};

struct AreaBroadcasterSnapshot
{
    api::CellRange maBroadcaster;
    std::vector<ListenerSnapshot> maListeners;

    [[nodiscard]] constexpr bool operator==(const AreaBroadcasterSnapshot& rOther) const = default;
};

struct BroadcasterStateSnapshot
{
    std::vector<CellBroadcasterSnapshot> maCellBroadcasters;
    std::vector<AreaBroadcasterSnapshot> maAreaBroadcasters;

    [[nodiscard]] constexpr bool operator==(const BroadcasterStateSnapshot& rOther) const = default;
};

struct LiveComputationalStateSnapshot
{
    std::vector<api::CellAddress> maFormulaTree;
    std::vector<api::CellAddress> maFormulaTrack;
    BroadcasterStateSnapshot maBroadcasters;

    [[nodiscard]] constexpr bool operator==(const LiveComputationalStateSnapshot& rOther) const = default;
};

namespace detail
{

struct AddressLess
{
    [[nodiscard]] bool operator()(const api::CellAddress& rLeft,
        const api::CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

struct RangeLess
{
    [[nodiscard]] bool operator()(const api::CellRange& rLeft,
        const api::CellRange& rRight) const
    {
        return AddressLess {}(rLeft.maStart, rRight.maStart)
               || (!(rLeft.maStart == rRight.maStart)
                   ? false
                   : AddressLess {}(rLeft.maEnd, rRight.maEnd));
    }
};

struct ListenerLess
{
    [[nodiscard]] bool operator()(const ListenerSnapshot& rLeft,
        const ListenerSnapshot& rRight) const
    {
        if (rLeft.meKind != rRight.meKind)
            return rLeft.meKind < rRight.meKind;
        if (!(rLeft.maAnchor == rRight.maAnchor))
            return AddressLess {}(rLeft.maAnchor, rRight.maAnchor);
        return rLeft.mnLength < rRight.mnLength;
    }
};

[[nodiscard]] inline std::vector<api::CellAddress> normalizeAddresses(
    std::vector<api::CellAddress> aAddresses)
{
    std::sort(aAddresses.begin(), aAddresses.end(), AddressLess {});
    aAddresses.erase(std::unique(aAddresses.begin(), aAddresses.end()), aAddresses.end());
    return aAddresses;
}

[[nodiscard]] inline std::vector<ListenerSnapshot> normalizeListeners(
    std::vector<ListenerSnapshot> aListeners)
{
    std::sort(aListeners.begin(), aListeners.end(), ListenerLess {});
    return aListeners;
}

[[nodiscard]] inline ListenerSnapshot makeFormulaCellListenerSnapshot(const ScFormulaCell& rCell)
{
    ListenerSnapshot aSnapshot;
    aSnapshot.meKind = ListenerKind::FormulaCell;
    aSnapshot.maAnchor = toApiCellAddress(rCell.aPos);
    aSnapshot.mnLength = 1;
    return aSnapshot;
}

[[nodiscard]] inline ListenerSnapshot makeFormulaGroupListenerSnapshot(
    const sc::FormulaGroupAreaListener& rListener)
{
    ListenerSnapshot aSnapshot;
    aSnapshot.meKind = ListenerKind::FormulaGroup;

    if (const ScFormulaCell* pTopCell = rListener.getTopCell())
    {
        aSnapshot.maAnchor = toApiCellAddress(pTopCell->aPos);
        if (const auto xGroup = pTopCell->GetCellGroup(); xGroup)
            aSnapshot.mnLength = xGroup->mnLength;
    }

    if (aSnapshot.mnLength <= 0)
    {
        const ScRange aRange = rListener.getListeningRange();
        aSnapshot.maAnchor = toApiCellAddress(aRange.aStart);
        aSnapshot.mnLength = aRange.aEnd.Row() - aRange.aStart.Row() + 1;
    }

    return aSnapshot;
}

[[nodiscard]] inline ListenerSnapshot makeUnknownListenerSnapshot()
{
    return ListenerSnapshot {};
}

[[nodiscard]] inline ListenerSnapshot makeListenerSnapshot(
    const sc::BroadcasterState::CellListener& rListener)
{
    if (std::holds_alternative<const ScFormulaCell*>(rListener.pData))
        return makeFormulaCellListenerSnapshot(*std::get<const ScFormulaCell*>(rListener.pData));
    return makeUnknownListenerSnapshot();
}

[[nodiscard]] inline ListenerSnapshot makeListenerSnapshot(
    const sc::BroadcasterState::AreaListener& rListener)
{
    if (std::holds_alternative<const ScFormulaCell*>(rListener.pData))
        return makeFormulaCellListenerSnapshot(*std::get<const ScFormulaCell*>(rListener.pData));
    if (std::holds_alternative<const sc::FormulaGroupAreaListener*>(rListener.pData))
    {
        return makeFormulaGroupListenerSnapshot(
            *std::get<const sc::FormulaGroupAreaListener*>(rListener.pData));
    }
    return makeUnknownListenerSnapshot();
}

template <typename LinkAccessor>
[[nodiscard]] inline std::vector<api::CellAddress> collectTrackedFormulaAddresses(
    const ScDocument& rDoc, LinkAccessor pPreviousAccessor, LinkAccessor pNextAccessor,
    bool (ScDocument::*pMembershipCheck)(const ScFormulaCell*) const)
{
    ScFormulaCell* pHead = nullptr;
    std::vector<api::CellAddress> aFallback;

    for (SCTAB nTab = 0; nTab < rDoc.GetTableCount(); ++nTab)
    {
        ScCellIterator aIter(const_cast<ScDocument&>(rDoc),
            ScRange(0, 0, nTab, rDoc.MaxCol(), rDoc.MaxRow(), nTab));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            if (aIter.getType() != CELLTYPE_FORMULA)
                continue;

            ScFormulaCell* pCell = aIter.getFormulaCell();
            if (!pCell || !(rDoc.*pMembershipCheck)(pCell))
                continue;

            aFallback.push_back(toApiCellAddress(pCell->aPos));
            if (!(pCell->*pPreviousAccessor)())
                pHead = pCell;
        }
    }

    if (!pHead)
        return normalizeAddresses(std::move(aFallback));

    std::vector<api::CellAddress> aAddresses;
    for (const ScFormulaCell* pCell = pHead; pCell != nullptr; pCell = (pCell->*pNextAccessor)())
        aAddresses.push_back(toApiCellAddress(pCell->aPos));
    return aAddresses;
}

} // namespace detail

[[nodiscard]] inline std::vector<api::CellAddress> collectFormulaTreeAddresses(const ScDocument& rDoc)
{
    return detail::collectTrackedFormulaAddresses(
        rDoc, &ScFormulaCell::GetPrevious, &ScFormulaCell::GetNext, &ScDocument::IsInFormulaTree);
}

[[nodiscard]] inline std::vector<api::CellAddress> collectFormulaTrackAddresses(const ScDocument& rDoc)
{
    return detail::collectTrackedFormulaAddresses(rDoc, &ScFormulaCell::GetPreviousTrack,
        &ScFormulaCell::GetNextTrack, &ScDocument::IsInFormulaTrack);
}

[[nodiscard]] inline BroadcasterStateSnapshot collectBroadcasterStateSnapshot(const ScDocument& rDoc)
{
    const sc::BroadcasterState aState = rDoc.GetBroadcasterState();
    BroadcasterStateSnapshot aSnapshot;

    aSnapshot.maCellBroadcasters.reserve(aState.aCellListenerStore.size());
    for (const auto& [rAddress, rListeners] : aState.aCellListenerStore)
    {
        CellBroadcasterSnapshot aEntry;
        aEntry.maBroadcaster = toApiCellAddress(rAddress);
        aEntry.maListeners.reserve(rListeners.size());
        for (const auto& rListener : rListeners)
            aEntry.maListeners.push_back(detail::makeListenerSnapshot(rListener));
        aEntry.maListeners = detail::normalizeListeners(std::move(aEntry.maListeners));
        aSnapshot.maCellBroadcasters.push_back(std::move(aEntry));
    }

    aSnapshot.maAreaBroadcasters.reserve(aState.aAreaListenerStore.size());
    for (const auto& [rRange, rListeners] : aState.aAreaListenerStore)
    {
        AreaBroadcasterSnapshot aEntry;
        aEntry.maBroadcaster = toApiCellRange(rRange);
        aEntry.maListeners.reserve(rListeners.size());
        for (const auto& rListener : rListeners)
            aEntry.maListeners.push_back(detail::makeListenerSnapshot(rListener));
        aEntry.maListeners = detail::normalizeListeners(std::move(aEntry.maListeners));
        aSnapshot.maAreaBroadcasters.push_back(std::move(aEntry));
    }

    std::sort(aSnapshot.maCellBroadcasters.begin(), aSnapshot.maCellBroadcasters.end(),
        [](const CellBroadcasterSnapshot& rLeft, const CellBroadcasterSnapshot& rRight) {
            return detail::AddressLess {}(rLeft.maBroadcaster, rRight.maBroadcaster);
        });
    std::sort(aSnapshot.maAreaBroadcasters.begin(), aSnapshot.maAreaBroadcasters.end(),
        [](const AreaBroadcasterSnapshot& rLeft, const AreaBroadcasterSnapshot& rRight) {
            return detail::RangeLess {}(rLeft.maBroadcaster, rRight.maBroadcaster);
        });

    return aSnapshot;
}

[[nodiscard]] inline LiveComputationalStateSnapshot collectLiveComputationalState(
    const ScDocument& rDoc)
{
    LiveComputationalStateSnapshot aSnapshot;
    aSnapshot.maFormulaTree = collectFormulaTreeAddresses(rDoc);
    aSnapshot.maFormulaTrack = collectFormulaTrackAddresses(rDoc);
    aSnapshot.maBroadcasters = collectBroadcasterStateSnapshot(rDoc);
    return aSnapshot;
}

} // namespace spreadsheetengine::compat::libreoffice::substrateobs

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
