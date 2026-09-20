#ifndef MAIN_APPLICATION_AXES_GRID_NUMBERS_H_
#define MAIN_APPLICATION_AXES_GRID_NUMBERS_H_

#include <stddef.h>

#include <glm/mat4x4.hpp>

#include "axes/axes_side_configuration.h"
#include "axes/structures/axes_limits.h"
#include "axes/structures/axes_settings.h"
#include "axes/structures/grid_vectors.h"
#include "axes/structures/view_angles.h"
#include "axes/text_rendering.h"
#include "lumos/math.h"
#include "opengl_low_level/opengl_header.h"
#include "project_state/project_settings.h"

void drawGridNumbers(TextRenderer& text_renderer,
                     const AxesLimits& axes_limits,
                     const ViewAngles& view_angles,
                     const PlotPaneSettings& plot_pane_settings,
                     const glm::mat4& view_mat,
                     const glm::mat4& model_mat,
                     const glm::mat4& projection_mat,
                     const float width,
                     const float height,
                     const GridVectors& gv,
                     const AxesSideConfiguration& axes_side_configuration,
                     const bool perspective_projection);

#endif  // MAIN_APPLICATION_AXES_GRID_NUMBERS_H_
