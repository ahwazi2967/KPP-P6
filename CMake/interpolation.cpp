#include "interpolation.h"
#include <algorithm>

int interpolate(double angle) {
    angle = std::clamp(angle, -180.0, 180.0);
    double digital_unit = (angle + 180) * 1023.0 / 360.0;
    return static_cast<int>(digital_unit);
}

std::vector<int> interpolate_list(const std::vector<double>& angles) {
    std::vector<int> digital_units;
    digital_units.reserve(angles.size());
    for (double angle : angles) {
        digital_units.push_back(interpolate(angle));
    }
    return digital_units;
}
