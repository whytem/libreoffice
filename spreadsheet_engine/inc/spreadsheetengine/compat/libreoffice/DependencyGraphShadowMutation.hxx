/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/compat/libreoffice/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowMutation.hxx>

namespace spreadsheetengine::compat::libreoffice
{

[[nodiscard]] inline spreadsheetengine::detail::substrate::GraphShadowMutationState
rebuildDependencyGraphShadowAfterMutation(
    const spreadsheetengine::detail::facade::WorkbookFacade& rFacade, const ScDocument& rDoc,
    const spreadsheetengine::detail::facade::MutationEvent& rMutation)
{
    const auto aObservation
        = makeComputationalObservationState(substrateobs::collectLiveComputationalState(rDoc));
    return spreadsheetengine::detail::substrate::rebuildDependencyGraphShadowAfterMutation(
        rFacade, aObservation, rMutation);
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
