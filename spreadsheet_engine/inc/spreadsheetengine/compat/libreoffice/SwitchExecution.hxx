/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <rtl/math.hxx>
#include <svl/sharedstring.hxx>

namespace spreadsheetengine::compat::libreoffice::switchexecution
{

struct SwitchValue
{
    bool mbNumeric = false;
    double mfNumeric = 0.0;
    svl::SharedString maText;
};

[[nodiscard]] inline SwitchValue makeNumericSwitchValue(double fValue)
{
    SwitchValue aResult;
    aResult.mbNumeric = true;
    aResult.mfNumeric = fValue;
    return aResult;
}

[[nodiscard]] inline SwitchValue makeTextSwitchValue(const svl::SharedString& rValue)
{
    SwitchValue aResult;
    aResult.mbNumeric = false;
    aResult.maText = rValue;
    return aResult;
}

[[nodiscard]] inline bool matchesSwitchCase(
    const SwitchValue& rReference, const SwitchValue& rCandidate)
{
    if (rReference.mbNumeric != rCandidate.mbNumeric)
        return false;

    if (rReference.mbNumeric)
        return rtl::math::approxEqual(rReference.mfNumeric, rCandidate.mfNumeric);

    return rReference.maText.getDataIgnoreCase() == rCandidate.maText.getDataIgnoreCase();
}

} // namespace spreadsheetengine::compat::libreoffice::switchexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
