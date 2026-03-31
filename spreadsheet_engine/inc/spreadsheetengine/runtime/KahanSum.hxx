/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>

namespace spreadsheetengine::core::fp
{

class KahanSum
{
public:
    constexpr KahanSum() = default;
    constexpr KahanSum(double fInitial)
        : mfSum(fInitial)
    {
    }

    void add(double fValue)
    {
        const double fAdjusted = fValue - mfCompensation;
        const double fNext = mfSum + fAdjusted;
        mfCompensation = (fNext - mfSum) - fAdjusted;
        mfSum = fNext;
    }

    void subtract(double fValue) { add(-fValue); }

    double get() const { return mfSum; }

    KahanSum& operator=(double fValue)
    {
        mfSum = fValue;
        mfCompensation = 0.0;
        return *this;
    }

    void operator+=(double fValue) { add(fValue); }

    void operator+=(const KahanSum& rOther) { add(rOther.get()); }

    void operator-=(double fValue) { subtract(fValue); }

    void operator-=(const KahanSum& rOther) { subtract(rOther.get()); }

    void operator*=(double fFactor)
    {
        mfSum *= fFactor;
        mfCompensation *= fFactor;
    }

    KahanSum operator-(double fValue) const
    {
        KahanSum aCopy(*this);
        aCopy -= fValue;
        return aCopy;
    }

    KahanSum operator-(const KahanSum& rOther) const
    {
        KahanSum aCopy(*this);
        aCopy -= rOther;
        return aCopy;
    }

private:
    double mfSum = 0.0;
    double mfCompensation = 0.0;
};

} // namespace spreadsheetengine::core::fp

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
