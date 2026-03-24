/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace spreadsheetengine::standalone::test
{

inline bool almostEqual(double fLeft, double fRight)
{
    const double fScale = std::max({ 1.0, std::fabs(fLeft), std::fabs(fRight) });
    return std::fabs(fLeft - fRight) <= (1.0e-9 * fScale);
}

inline int fail(const char* pTestName, const char* pMessage)
{
    std::cerr << pTestName << ": " << pMessage << '\n';
    return EXIT_FAILURE;
}

} // namespace spreadsheetengine::standalone::test

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
