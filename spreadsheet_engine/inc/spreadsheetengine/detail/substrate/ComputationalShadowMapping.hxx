/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/substrate/ComputationalShadow.hxx>

namespace spreadsheetengine::detail::substrate::mapping
{

/// Stable Phase 1 cell identity. The shadow stores only address-based ids.
[[nodiscard]] inline ShadowCellId makeShadowCellId(const api::CellAddress& rAddress)
{
    return ShadowCellId { rAddress };
}

/// Stable Phase 1 formula-group identity. Length participates so that a
/// rebuilt shadow cannot silently merge unrelated groups that share an anchor.
[[nodiscard]] inline ShadowFormulaGroupId makeShadowFormulaGroupId(
    const facade::FormulaGroupDescriptor& rDescriptor)
{
    return ShadowFormulaGroupId { rDescriptor.maAnchor, rDescriptor.mnLength };
}

/// Stable Phase 1 listener-anchor identity derived from normalized observation
/// capture, never from broadcaster container pointers.
[[nodiscard]] inline ListenerAnchorId makeListenerAnchorId(ListenerAnchorKind eKind,
    const api::CellAddress& rAnchor, sal_Int32 nLength)
{
    return ListenerAnchorId { eKind, rAnchor, nLength };
}

} // namespace spreadsheetengine::detail::substrate::mapping

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
