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

#include <spreadsheetengine/api/Grammar.hxx>

namespace spreadsheetengine::api::stringreference
{

struct AddressSyntaxPolicy
{
    AddressConvention mePrimary = AddressConvention::OooA1;
    std::optional<AddressConvention> moFallback;

    [[nodiscard]] constexpr bool operator==(const AddressSyntaxPolicy& rOther) const = default;
};

[[nodiscard]] constexpr AddressConvention normalizeAddressConvention(
    AddressConvention eConfigured, AddressConvention eDocument)
{
    if (eConfigured != AddressConvention::Unknown)
        return eConfigured;
    if (eDocument != AddressConvention::Unknown)
        return eDocument;
    return AddressConvention::OooA1;
}

[[nodiscard]] constexpr AddressSyntaxPolicy resolveIndirectAddressSyntaxPolicy(
    AddressConvention eConfigured, AddressConvention eDocument, bool bTryOooThenXlA1,
    bool bForceR1C1)
{
    if (bForceR1C1)
        return { AddressConvention::XlR1C1, std::nullopt };

    if (bTryOooThenXlA1)
        return { AddressConvention::OooA1, AddressConvention::XlA1 };

    return { normalizeAddressConvention(eConfigured, eDocument), std::nullopt };
}

[[nodiscard]] constexpr AddressConvention resolveAddressFunctionConvention(
    AddressConvention eConfigured, AddressConvention eDocument, bool bForceR1C1)
{
    if (bForceR1C1)
        return AddressConvention::XlR1C1;

    const AddressConvention eResolved = normalizeAddressConvention(eConfigured, eDocument);
    switch (eResolved)
    {
        case AddressConvention::XlA1:
        case AddressConvention::XlR1C1:
            return AddressConvention::XlA1;
        case AddressConvention::OooA1:
        case AddressConvention::OdfA1:
        case AddressConvention::XlOox:
        case AddressConvention::Unknown:
        default:
            return AddressConvention::OooA1;
    }
}

} // namespace spreadsheetengine::api::stringreference

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
