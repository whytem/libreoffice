/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>

#include <document.hxx>
#include <dociter.hxx>
#include <formulacell.hxx>

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx>

namespace spreadsheetengine::compat::libreoffice::substrateformulalifetime
{

enum class FormulaCellLifetimeResultKind : sal_uInt8
{
    Applied,
    RejectedOutOfContract
};

struct FormulaCellLifetimeResult
{
    FormulaCellLifetimeResultKind meKind = FormulaCellLifetimeResultKind::RejectedOutOfContract;
    api::String maReason;
    sal_Int32 mnFormulaCellsRemoved = 0;
    sal_Int32 mnFormulaCellsRealized = 0;
};

namespace detail
{

[[nodiscard]] inline bool isAdmittedFormulaCell(const ScFormulaCell& rCell)
{
    return rCell.GetMatrixFlag() == ScMatrixMode::NONE;
}

[[nodiscard]] inline bool collectAdmittedLiveFormulaAddresses(
    ScDocument& rDoc, std::vector<ScAddress>& rAddresses, api::String& rReason)
{
    for (SCTAB nTab = 0; nTab < rDoc.GetTableCount(); ++nTab)
    {
        ScCellIterator aIter(rDoc, ScRange(0, 0, nTab, rDoc.MaxCol(), rDoc.MaxRow(), nTab));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            if (aIter.getType() != CELLTYPE_FORMULA)
                continue;

            ScFormulaCell* pCell = aIter.getFormulaCell();
            if (!pCell || !isAdmittedFormulaCell(*pCell))
            {
                rReason = u"formula_shape_out_of_contract";
                return false;
            }

            rAddresses.push_back(aIter.GetPos());
        }
    }

    return true;
}

inline void clearRemovedFormulaCells(ScDocument& rDoc,
    const std::vector<ScAddress>& rLiveFormulaAddresses,
    const spreadsheetengine::detail::substrate::AdmittedFormulaCellLifetime& rStore,
    FormulaCellLifetimeResult& rResult)
{
    for (const auto& rAddress : rLiveFormulaAddresses)
    {
        if (rStore.findFormulaCell(toApiCellAddress(rAddress)))
            continue;

        rDoc.SetEmptyCell(rAddress);
        ++rResult.mnFormulaCellsRemoved;
    }
}

inline void realizeFormulaCellRecord(ScDocument& rDoc,
    const spreadsheetengine::detail::substrate::AdmittedFormulaCellLifetimeRecord& rRecord,
    FormulaCellLifetimeResult& rResult)
{
    if (rRecord.maFormulaSource.empty())
    {
        rResult.maReason = u"formula_source_out_of_contract";
        return;
    }

    const ScAddress aAddress = toLibreOfficeAddress(rRecord.maId.maAddress);
    if (ScFormulaCell* pCell = rDoc.GetFormulaCell(aAddress))
    {
        if (!isAdmittedFormulaCell(*pCell))
        {
            rResult.maReason = u"formula_shape_out_of_contract";
            return;
        }
        if (pCell->GetFormula() == toLibreOfficeString(rRecord.maFormulaSource))
            return;
    }

    rDoc.SetString(aAddress, toLibreOfficeString(rRecord.maFormulaSource));
    ++rResult.mnFormulaCellsRealized;
}

} // namespace detail

[[nodiscard]] inline FormulaCellLifetimeResult realizeAdmittedFormulaCellLifetime(
    ScDocument& rDoc, const spreadsheetengine::detail::substrate::AdmittedFormulaCellLifetime& rStore)
{
    FormulaCellLifetimeResult aResult;

    std::vector<ScAddress> aLiveFormulaAddresses;
    if (!detail::collectAdmittedLiveFormulaAddresses(rDoc, aLiveFormulaAddresses, aResult.maReason))
        return aResult;

    detail::clearRemovedFormulaCells(rDoc, aLiveFormulaAddresses, rStore, aResult);
    for (const auto& rRecord : rStore.maFormulaCells)
    {
        detail::realizeFormulaCellRecord(rDoc, rRecord, aResult);
        if (!aResult.maReason.empty())
            return aResult;
    }

    aResult.meKind = FormulaCellLifetimeResultKind::Applied;
    return aResult;
}

} // namespace spreadsheetengine::compat::libreoffice::substrateformulalifetime

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
