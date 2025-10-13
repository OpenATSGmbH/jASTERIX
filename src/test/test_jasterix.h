#pragma once

#include <cmath>
#include <string>

extern std::string definition_path;
extern std::string data_path;

inline bool approximatelyEqual(double a, double b, double epsilon)
{
    return fabs(a - b) < epsilon;
    // return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}


