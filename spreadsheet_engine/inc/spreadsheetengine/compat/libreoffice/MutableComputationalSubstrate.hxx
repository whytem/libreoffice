/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <document.hxx>

#include <spreadsheetengine/compat/libreoffice/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx>

namespace spreadsheetengine::compat::libreoffice
{

[[nodiscard]] inline spreadsheetengine::detail::substrate::MutableComputationalSubstrateState
bootstrapMutableComputationalSubstrateState(const spreadsheetengine::detail::facade::WorkbookFacade& rFacade,
    const ScDocument& rDoc)
{
    const auto aObservation
        = makeComputationalObservationState(substrateobs::collectLiveComputationalState(rDoc));
    return spreadsheetengine::detail::substrate::bootstrapMutableComputationalSubstrateState(
        rFacade, aObservation);
}

[[nodiscard]] inline spreadsheetengine::detail::substrate::MutableComputationalSubstrateState
bootstrapMutableComputationalSubstrateState(const ScDocument& rDoc, sal_Int64 nGeneration = 0)
{
    const CalcWorkbookFacade aFacade(rDoc, nGeneration);
    return bootstrapMutableComputationalSubstrateState(aFacade, rDoc);
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
