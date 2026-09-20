#ifndef MAIN_APPLICATION_AXES_STRUCTURES_GRID_VECTORS_H_
#define MAIN_APPLICATION_AXES_STRUCTURES_GRID_VECTORS_H_

#include <array>
#include <cstdint>

#include "lumos/math.h"

using namespace lumos;

enum class MouseInteractionType
{
    ROTATE,
    PAN,
    ZOOM,
    POINT_SELECTION,
    UNCHANGED,
    UNKNOWN
};

struct GridVector
{
    static constexpr std::uint8_t kMaxNumGridNumbers = 30;
    std::array<double, kMaxNumGridNumbers> data;
    size_t num_valid_values;
    double min_value;
    double max_value;
    double grid_spacing;
    double range;
};

struct GridVectors
{
    GridVector x;
    GridVector y;
    GridVector z;
};

#endif  // MAIN_APPLICATION_AXES_STRUCTURES_GRID_VECTORS_H_
