/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>

#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObservation.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowMapping.hxx>

namespace spreadsheetengine::compat::libreoffice
{

namespace builderdetail
{

[[nodiscard]] inline spreadsheetengine::detail::substrate::ListenerAnchorKind toShadowListenerKind(
    substrateobs::ListenerKind eKind)
{
    switch (eKind)
    {
        case substrateobs::ListenerKind::FormulaCell:
            return spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaCell;
        case substrateobs::ListenerKind::FormulaGroup:
            return spreadsheetengine::detail::substrate::ListenerAnchorKind::FormulaGroup;
        case substrateobs::ListenerKind::HostUnknown:
        default:
            return spreadsheetengine::detail::substrate::ListenerAnchorKind::HostUnknown;
    }
}

[[nodiscard]] inline spreadsheetengine::detail::substrate::ListenerAnchorId toShadowListenerAnchor(
    const substrateobs::ListenerSnapshot& rSnapshot)
{
    return spreadsheetengine::detail::substrate::mapping::makeListenerAnchorId(
        toShadowListenerKind(rSnapshot.meKind), rSnapshot.maAnchor, rSnapshot.mnLength);
}

template <typename InputContainer, typename OutputContainer>
void appendListenerAnchors(const InputContainer& rListeners, OutputContainer& rOutput)
{
    rOutput.reserve(rListeners.size());
    for (const auto& rListener : rListeners)
        rOutput.push_back(toShadowListenerAnchor(rListener));
}

} // namespace builderdetail

[[nodiscard]] inline spreadsheetengine::detail::substrate::ComputationalObservationState
makeComputationalObservationState(
    const substrateobs::LiveComputationalStateSnapshot& rSnapshot)
{
    spreadsheetengine::detail::substrate::ComputationalObservationState aState;
    aState.maFormulaTree = rSnapshot.maFormulaTree;
    aState.maFormulaTrack = rSnapshot.maFormulaTrack;

    aState.maCellBroadcasters.reserve(rSnapshot.maBroadcasters.maCellBroadcasters.size());
    for (const auto& rBroadcaster : rSnapshot.maBroadcasters.maCellBroadcasters)
    {
        spreadsheetengine::detail::substrate::CellBroadcasterRecord aRecord;
        aRecord.maBroadcaster = rBroadcaster.maBroadcaster;
        builderdetail::appendListenerAnchors(rBroadcaster.maListeners, aRecord.maListeners);
        aState.maCellBroadcasters.push_back(std::move(aRecord));
    }

    aState.maAreaBroadcasters.reserve(rSnapshot.maBroadcasters.maAreaBroadcasters.size());
    for (const auto& rBroadcaster : rSnapshot.maBroadcasters.maAreaBroadcasters)
    {
        spreadsheetengine::detail::substrate::AreaBroadcasterRecord aRecord;
        aRecord.maBroadcaster = rBroadcaster.maBroadcaster;
        builderdetail::appendListenerAnchors(rBroadcaster.maListeners, aRecord.maListeners);
        aState.maAreaBroadcasters.push_back(std::move(aRecord));
    }

    return aState;
}

[[nodiscard]] inline spreadsheetengine::detail::substrate::ComputationalWorkbookShadow
buildComputationalWorkbookShadow(
    const spreadsheetengine::detail::facade::WorkbookFacade& rFacade, const ScDocument& rDoc)
{
    return spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow(
        rFacade, makeComputationalObservationState(substrateobs::collectLiveComputationalState(rDoc)));
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
