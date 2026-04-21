/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>

#include <spreadsheetengine/api/Grammar.hxx>
#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::api
{

enum class RangeResolutionKind : std::uint8_t
{
    DirectReference,
    NamedReference,
    IndirectText
};

enum class ResolvedRangeBindingKind : std::uint8_t
{
    LocalRange,
    ExternalRange,
    TokenBackedSymbol
};

struct RangeResolutionRequest
{
    RangeResolutionKind meKind = RangeResolutionKind::DirectReference;
    CellAddress maBaseAddress;
    String maPrimaryText;
    String maSecondaryText;
    AddressConvention meConvention = AddressConvention::OooA1;
    bool mbTryXlA1 = false;

    [[nodiscard]] constexpr bool operator==(const RangeResolutionRequest& rOther) const = default;
};

struct ResolvedRangeBinding
{
    ResolvedRangeBindingKind meKind = ResolvedRangeBindingKind::LocalRange;
    CellRange maRange;
    sal_uInt16 mnFileId = 0;
    String maTabName;
    String maSymbol;

    [[nodiscard]] constexpr bool operator==(const ResolvedRangeBinding& rOther) const = default;

    [[nodiscard]] constexpr bool isSingleCell() const
    {
        return maRange.isSingleCell();
    }
};

class RangeResolver
{
public:
    virtual ~RangeResolver() = default;

    [[nodiscard]] virtual ValueResult<ResolvedRangeBinding> resolveRange(
        const RangeResolutionRequest& rRequest) const = 0;
};

} // namespace spreadsheetengine::api

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
