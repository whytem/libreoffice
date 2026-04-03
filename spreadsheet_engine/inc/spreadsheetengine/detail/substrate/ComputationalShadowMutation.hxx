/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx>

namespace spreadsheetengine::detail::substrate
{

struct ShadowMutationState
{
    facade::MutationEvent maMutation;
    ComputationalObservationState maObservation;
    ComputationalWorkbookShadow maShadow;

    [[nodiscard]] constexpr bool operator==(const ShadowMutationState& rOther) const = default;
};

[[nodiscard]] inline ShadowMutationState rebuildComputationalShadowAfterMutation(
    const facade::WorkbookFacade& rFacade,
    const ComputationalObservationState& rObservation,
    const facade::MutationEvent& rMutation)
{
    ShadowMutationState aState;
    aState.maMutation = rMutation;
    aState.maObservation = rObservation;
    aState.maShadow = buildComputationalWorkbookShadow(rFacade, rObservation);
    return aState;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
