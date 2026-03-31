#pragma once

// Shim: redirect to the engine's FloatingPoint module for divAllowZero.
#include <spreadsheetengine/runtime/FloatingPoint.hxx>

#include <type_traits>

namespace o3tl
{

inline double div_allow_zero(double a, double b)
{
    return spreadsheetengine::core::fp::divAllowZero(a, b);
}

template <typename T> inline void untaint_for_overrun([[maybe_unused]] T& rValue) {}

} // namespace o3tl
