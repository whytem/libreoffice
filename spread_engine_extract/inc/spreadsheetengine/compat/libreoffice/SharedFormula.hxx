/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <formulacell.hxx>

#include <spreadsheetengine/api/SharedFormula.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::sharedformula::TokenCompareState toApiTokenCompareState(
    ScFormulaCell::CompareState eState)
{
    switch (eState)
    {
        case ScFormulaCell::NotEqual:
            return spreadsheetengine::api::sharedformula::TokenCompareState::NotEqual;
        case ScFormulaCell::EqualInvariant:
            return spreadsheetengine::api::sharedformula::TokenCompareState::EqualInvariant;
        case ScFormulaCell::EqualRelativeRef:
            return spreadsheetengine::api::sharedformula::TokenCompareState::EqualRelativeRef;
    }

    return spreadsheetengine::api::sharedformula::TokenCompareState::NotEqual;
}

inline spreadsheetengine::api::sharedformula::GroupSingleRefListenPlan makeGroupSingleRefListenPlan(
    const ScAddress& rAddress)
{
    return spreadsheetengine::api::sharedformula::makeGroupSingleRefListenPlan(
        toApiCellAddress(rAddress));
}

inline spreadsheetengine::api::sharedformula::GroupDoubleRefListenPlan makeGroupDoubleRefListenPlan(
    const ScRange& rRange, bool bRef1RowRelative, bool bRef2RowRelative, sal_Int32 nGroupLength)
{
    return spreadsheetengine::api::sharedformula::makeGroupDoubleRefListenPlan(
        toApiCellRange(rRange), bRef1RowRelative, bRef2RowRelative, nGroupLength);
}

inline ScRange toLibreOfficeListenedRange(
    const spreadsheetengine::api::sharedformula::GroupDoubleRefListenPlan& rPlan)
{
    return toLibreOfficeRange(rPlan.maListenedRange);
}

inline ScRange toLibreOfficeOriginalRange(
    const spreadsheetengine::api::sharedformula::GroupDoubleRefListenPlan& rPlan)
{
    return toLibreOfficeRange(rPlan.maOriginalRange);
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
