/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <address.hxx>

#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::CellAddress toApiCellAddress(const ScAddress& rAddress)
{
    return { rAddress.Tab(), rAddress.Col(), rAddress.Row() };
}

inline ScAddress toLibreOfficeAddress(const spreadsheetengine::api::CellAddress& rAddress)
{
    return ScAddress(rAddress.mnColumn, rAddress.mnRow, rAddress.mnSheet);
}

inline spreadsheetengine::api::CellRange toApiCellRange(const ScRange& rRange)
{
    return { toApiCellAddress(rRange.aStart), toApiCellAddress(rRange.aEnd) };
}

inline ScRange toLibreOfficeRange(const spreadsheetengine::api::CellRange& rRange)
{
    return ScRange(toLibreOfficeAddress(rRange.maStart), toLibreOfficeAddress(rRange.maEnd));
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
