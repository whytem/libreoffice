/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/compat/libreoffice/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrMutation.hxx>

namespace spreadsheetengine::compat::libreoffice
{

[[nodiscard]] inline spreadsheetengine::detail::substrate::ExecutionIrShadowMutationState
rebuildExecutionIrShadowAfterMutation(
    const spreadsheetengine::detail::facade::WorkbookFacade& rFacade, ScDocument& rDoc,
    const spreadsheetengine::detail::facade::MutationEvent& rMutation,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    const auto aShadow = buildComputationalWorkbookShadow(rFacade, rDoc);
    spreadsheetengine::compat::libreoffice::DocumentCompileHost aHost(rDoc);
    const auto eGrammar = toLibreOfficeGrammar(aShadow.maGrammar);
    return spreadsheetengine::detail::substrate::rebuildExecutionIrShadowAfterMutation(aShadow,
        rMutation, irexecdetail::makeCalcExecutionIrCompileCallback(
                       rDoc, aHost, eGrammar, rExternalLinks));
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
