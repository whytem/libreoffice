/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/substrate/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx>

namespace spreadsheetengine::detail::substrate
{

struct ExecutionIrShadowMutationState
{
    facade::MutationEvent maMutation;
    ComputationalWorkbookShadow maComputationalShadow;
    ExecutionIrWorkbookShadow maIrShadow;

    [[nodiscard]] constexpr bool operator==(const ExecutionIrShadowMutationState& rOther) const
        = default;
};

[[nodiscard]] inline ExecutionIrShadowMutationState rebuildExecutionIrShadowAfterMutation(
    const ComputationalWorkbookShadow& rShadow, const facade::MutationEvent& rMutation,
    const ExecutionIrCompileCallback& rCompile)
{
    ExecutionIrShadowMutationState aState;
    aState.maMutation = rMutation;
    aState.maComputationalShadow = rShadow;
    aState.maIrShadow = buildExecutionIrWorkbookShadow(rShadow, rCompile);
    return aState;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
