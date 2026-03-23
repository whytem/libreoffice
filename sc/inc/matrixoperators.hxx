/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once


#include <functional>
#include <sal/types.h>
#include <vector>
#include "kahan.hxx"

#if defined(SPREADSHEETENGINE_DLLIMPLEMENTATION)
#define SC_MATRIXOPS_DLLPUBLIC SAL_DLLPUBLIC_EXPORT
#else
#define SC_MATRIXOPS_DLLPUBLIC SAL_DLLPUBLIC_IMPORT
#endif

namespace sc::op {


template<typename T, typename tRes>
struct Op_
{
    const double mInitVal;
    const T maOp;
    Op_(double InitVal, T aOp):
        mInitVal(InitVal), maOp(std::move(aOp))
    {
    }
    void operator()(tRes& rAccum, double fVal) const
    {
        maOp(rAccum, fVal);
    }
};

using Op = Op_<std::function<void(double&, double)>, double>;
using kOp = Op_<std::function<void(KahanSum&, double)>, KahanSum>;

SC_MATRIXOPS_DLLPUBLIC void fkOpSum(KahanSum& rAccum, double fVal);
SC_MATRIXOPS_DLLPUBLIC void fkOpSumSquare(KahanSum& rAccum, double fVal);

extern SC_MATRIXOPS_DLLPUBLIC kOp kOpSum;
extern SC_MATRIXOPS_DLLPUBLIC kOp kOpSumSquare;
extern SC_MATRIXOPS_DLLPUBLIC std::vector<kOp> kOpSumAndSumSquare;

struct SC_MATRIXOPS_DLLPUBLIC Sum
{
    static const double InitVal;
    void operator()(KahanSum& rAccum, double fVal) const;
};

struct SC_MATRIXOPS_DLLPUBLIC SumSquare
{
    static const double InitVal;
    void operator()(KahanSum& rAccum, double fVal) const;
};

struct SC_MATRIXOPS_DLLPUBLIC Product
{
    static const double InitVal;
    void operator()(double& rAccum, double fVal) const;
};

}

#undef SC_MATRIXOPS_DLLPUBLIC

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
