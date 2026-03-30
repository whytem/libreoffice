/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>
#include <vector>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::lookup
{

struct LookupInput
{
    bool mbScalar = true;
    api::CellValue maScalar = api::CellValue::empty();
    api::ResolvedReference maReference;
    std::vector<api::CellValue> maValues;
    api::MatrixSize mnColumns = 1;
    api::MatrixSize mnRows = 1;
};

class LookupMaterializer
{
public:
    virtual ~LookupMaterializer() = default;

    [[nodiscard]] virtual api::ValueResult<api::CellValue> materialize(
        const LookupInput& rInput, api::MatrixCoordinate aCoordinate) const = 0;
};

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::lookup::VectorLayout> detectLookupLayout(
    const LookupInput& rInput, bool bAllowMajorVector);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::CellValue> materializeLookupInputValue(
    const LookupMaterializer& rMaterializer, const LookupInput& rInput,
    api::lookup::VectorOrientation eOrientation, api::MatrixSize nIndex);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::MatrixSize> resolveTabularLookupIndex(
    const LookupMaterializer& rMaterializer, const api::CellValue& rLookup,
    const LookupInput& rTableInput, api::lookup::VectorOrientation eSearchOrientation,
    bool bApproximate, api::query::SearchType eSearchType);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::MatrixSize> resolveLookupIndex(
    const LookupMaterializer& rMaterializer, const api::CellValue& rLookup,
    const LookupInput& rSearchInput, api::query::SearchType eSearchType);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::MatrixSize> resolveMatchIndex(
    const LookupMaterializer& rMaterializer, const api::CellValue& rLookup,
    const LookupInput& rSearchInput, const api::lookup::MatchSearchMode& rModes,
    api::query::SearchType eSearchType);

SPREADSHEETENGINE_DLLPUBLIC api::ValueResult<api::MatrixSize> resolveExtendedMatchIndex(
    const LookupMaterializer& rMaterializer, const api::CellValue& rLookup,
    const LookupInput& rSearchInput, api::lookup::MatchMode eMatchMode,
    api::lookup::SearchMode eSearchMode, api::query::SearchType eSearchType,
    bool bAllowPatternMatch);

} // namespace spreadsheetengine::core::lookup

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
