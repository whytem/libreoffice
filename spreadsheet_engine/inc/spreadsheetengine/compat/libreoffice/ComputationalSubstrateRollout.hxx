/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdlib>
#include <cstring>

#include <document.hxx>

namespace spreadsheetengine::compat::libreoffice::substraterollout
{

enum class RolloutSurface : sal_uInt8
{
    Authority,
    Lifecycle,
    Structural,
    GlobalNamedRangeStructural
};

namespace detail
{

[[nodiscard]] inline bool envValueEnabled(const char* pValue)
{
    return pValue && *pValue && std::strcmp(pValue, "0") != 0;
}

[[nodiscard]] inline bool resolveSurfaceGate(const char* pSpecificName)
{
    if (const char* pSpecificValue = std::getenv(pSpecificName))
        return envValueEnabled(pSpecificValue);

    return envValueEnabled(std::getenv("SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT"));
}

[[nodiscard]] inline bool resolveExplicitSurfaceGate(const char* pSpecificName)
{
    return envValueEnabled(std::getenv(pSpecificName));
}

} // namespace detail

[[nodiscard]] inline bool isSurfaceEnabled(const ScDocument& rDoc, RolloutSurface eSurface)
{
    if (rDoc.GetAutoCalc())
        return false;

    switch (eSurface)
    {
        case RolloutSurface::Authority:
            return detail::resolveSurfaceGate("SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY");
        case RolloutSurface::Lifecycle:
            return detail::resolveSurfaceGate("SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE");
        case RolloutSurface::Structural:
            return detail::resolveSurfaceGate("SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL");
        case RolloutSurface::GlobalNamedRangeStructural:
            return detail::resolveExplicitSurfaceGate(
                "SPREADSHEET_ENGINE_COMPUTATIONAL_GLOBAL_NAMED_RANGE");
    }

    return false;
}

} // namespace spreadsheetengine::compat::libreoffice::substraterollout

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
