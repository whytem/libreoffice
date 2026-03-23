#pragma once

#include <cmath>
#include <limits>
#include <type_traits>

namespace o3tl
{

inline double div_allow_zero(double a, double b)
{
    if (b == 0.0)
    {
        if (std::isfinite(a) && a != 0.0)
            return std::signbit(a) ? -std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::infinity();
        return std::numeric_limits<double>::quiet_NaN();
    }
    return a / b;
}

template <typename T> inline void untaint_for_overrun([[maybe_unused]] T& rValue) {}

} // namespace o3tl
