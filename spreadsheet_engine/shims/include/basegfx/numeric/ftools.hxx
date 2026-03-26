#pragma once

namespace basegfx
{

inline double rad2deg(double fRadians)
{
    return fRadians * (180.0 / 3.14159265358979323846);
}

inline double deg2rad(double fDegrees)
{
    return fDegrees * (3.14159265358979323846 / 180.0);
}

} // namespace basegfx
