#ifndef MAIN_APPLICATION_AXES_AXES_SIDE_CONFIGURATION_H_
#define MAIN_APPLICATION_AXES_AXES_SIDE_CONFIGURATION_H_

#include <cmath>
#include <utility>
#include <vector>

#include "axes/structures/axes_limits.h"
#include "axes/structures/axes_settings.h"
#include "axes/structures/grid_vectors.h"
#include "axes/structures/view_angles.h"
#include "lumos/math.h"

using namespace lumos;

struct AxesSideConfiguration
{
    float xy_plane_z_value;
    float xz_plane_y_value;
    float yz_plane_x_value;

    float z_axes_numbers_x_value;
    float z_axes_numbers_y_value;

    float y_axes_numbers_x_value;
    float y_axes_numbers_z_value;

    bool is_snapped;

    AxesSideConfiguration() = default;
    AxesSideConfiguration(const ViewAngles& view_angles, const bool perspective_projection);
};

#endif  // MAIN_APPLICATION_AXES_AXES_SIDE_CONFIGURATION_H_
