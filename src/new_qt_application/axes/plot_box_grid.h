#ifndef MAIN_APPLICATION_AXES_PLOT_BOX_GRID_H_
#define MAIN_APPLICATION_AXES_PLOT_BOX_GRID_H_

#include <stddef.h>

#include "axes/axes_side_configuration.h"
#include "axes/structures/axes_limits.h"
#include "axes/structures/axes_settings.h"
#include "axes/structures/grid_vectors.h"
#include "axes/structures/view_angles.h"
#include "lumos/math.h"
#include "opengl_low_level/opengl_header.h"

class PlotBoxGrid
{
private:
    GLuint vertex_buffer_, vertex_buffer_array_;
    float* grid_points_;
    size_t idx_;
    float azimuth_;
    float elevation_;

    Vec3d axes_scale_;

    void fillXYGrid(const GridVectors& gv, const AxesSideConfiguration& axes_side_configuration);
    void fillXZGrid(const GridVectors& gv, const AxesSideConfiguration& axes_side_configuration);
    void fillYZGrid(const GridVectors& gv, const AxesSideConfiguration& axes_side_configuration);

public:
    PlotBoxGrid();
    ~PlotBoxGrid();

    void render(const GridVectors& gv,
                const AxesLimits& axes_limits,
                const ViewAngles& view_angles,
                const AxesSideConfiguration& axes_side_configuration);
};

#endif  // MAIN_APPLICATION_AXES_PLOT_BOX_GRID_H_
