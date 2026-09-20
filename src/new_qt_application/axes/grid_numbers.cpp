#include "grid_numbers.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <iostream>

#include "lumos/math.h"
#include "misc/misc.h"
#include "opengl_low_level/opengl_header.h"
#include "text_rendering.h"

const float kTextScale{0.00045f};

void drawXLetter(TextRenderer& text_renderer,
                 const glm::mat4& view_model,
                 const glm::vec4& v_viewport,
                 const glm::mat4& projection,
                 const float width,
                 const float height,
                 const float y,
                 const float z,
                 const float z_value_multiplier)
{
    const glm::vec3 v(0.0f, y * 1.1, z * z_value_multiplier);

    const glm::vec3 v_projected = glm::project(v, view_model, projection, v_viewport);

    text_renderer.renderTextFromRightCenter("X", v_projected[0], v_projected[1], 0.0004f, width, height);
}

void drawYLetter(TextRenderer& text_renderer,
                 const glm::mat4& view_model,
                 const glm::vec4& v_viewport,
                 const glm::mat4& projection,
                 const float width,
                 const float height,
                 const float x,
                 const float z,
                 const float z_value_multiplier)
{
    const glm::vec3 v(x * 1.1, 0.0f, z * z_value_multiplier);

    const glm::vec3 v_projected = glm::project(v, view_model, projection, v_viewport);

    text_renderer.renderTextFromRightCenter("Y", v_projected[0], v_projected[1], 0.0004f, width, height);
}

void drawZLetter(TextRenderer& text_renderer,
                 const glm::mat4& view_model,
                 const glm::vec4& v_viewport,
                 const glm::mat4& projection,
                 const float width,
                 const float height,
                 const float x,
                 const float y,
                 const float x_val_multiplier,
                 const float y_val_multiplier)
{
    const glm::vec3 v(x * x_val_multiplier, y * y_val_multiplier, 0.0f);

    const glm::vec3 v_projected = glm::project(v, view_model, projection, v_viewport);

    text_renderer.renderTextFromRightCenter("Z", v_projected[0], v_projected[1], 0.0004f, width, height);
}

void drawXAxisNumbers(TextRenderer& text_renderer,
                      const PlotPaneSettings& plot_pane_settings,
                      const glm::mat4& view_model,
                      const glm::vec4& v_viewport,
                      const glm::mat4& projection,
                      const double azimuth,
                      const double elevation,
                      const float width,
                      const float height,
                      const SnappingAxis snapping_axis,
                      const Vec3d& axes_center,
                      const GridVectors& gv,
                      const AxesSideConfiguration& axes_side_configuration,
                      const float distance_multiplier,
                      const bool perspective_projection)
{
    const bool cond = (azimuth > (M_PI / 2.0)) || (azimuth < (-M_PI / 2.0));
    const float z_value_distance_multiplier = perspective_projection ? 1.0f : distance_multiplier;

    const double y = -axes_side_configuration.xz_plane_y_value * distance_multiplier;
    const double z = axes_side_configuration.xy_plane_z_value * z_value_distance_multiplier;

    if (plot_pane_settings.axes_letters_on)
    {
        const auto c = plot_pane_settings.axes_letters_color;
        text_renderer.setColor(c.red, c.green, c.blue);
        drawXLetter(
            text_renderer, view_model, v_viewport, projection, width, height, y, z, z_value_distance_multiplier);
    }

    if (!plot_pane_settings.axes_numbers_on)
    {
        return;
    }
    const auto c = plot_pane_settings.axes_numbers_color;
    text_renderer.setColor(c.red, c.green, c.blue);

    const bool cond2 =
        ((azimuth <= 0) && (azimuth >= (-M_PI / 2.0))) || ((azimuth >= (M_PI / 2.0)) && (azimuth <= (M_PI)));

    for (size_t k = 0; k < gv.x.num_valid_values; k++)
    {
        const double x = gv.x.data[k];
        const glm::vec3 v3(x, y, z);

        const glm::vec3 v_projected = glm::project(v3, view_model, projection, v_viewport);
        const std::string val = formatNumber(gv.x.data[k] + axes_center.x, 3);
        text_renderer.renderTextFromCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
        /*if (cond2)
        {
            text_renderer.renderTextFromRightCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
        }
        else
        {
            text_renderer.renderTextFromLeftCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
        }*/
    }
}

void drawYAxisNumbers(TextRenderer& text_renderer,
                      const PlotPaneSettings& plot_pane_settings,
                      const glm::mat4& view_model,
                      const glm::vec4& v_viewport,
                      const glm::mat4& projection,
                      const ViewAngles& view_angles,
                      const double azimuth,
                      const double elevation,
                      const float width,
                      const float height,
                      const SnappingAxis snapping_axis,
                      const Vec3d& axes_center,
                      const GridVectors& gv,
                      const AxesSideConfiguration& axes_side_configuration,
                      const float distance_multiplier,
                      const bool perspective_projection)
{
    const float z_value_distance_multiplier = perspective_projection ? 1.0f : distance_multiplier;

    const double x = -axes_side_configuration.y_axes_numbers_x_value * distance_multiplier;
    const double z = axes_side_configuration.y_axes_numbers_z_value * z_value_distance_multiplier;

    const bool cond2 =
        ((azimuth <= 0) && (azimuth >= (-M_PI / 2.0))) || ((azimuth >= (M_PI / 2.0)) && (azimuth <= (M_PI)));

    if (plot_pane_settings.axes_letters_on)
    {
        const auto c = plot_pane_settings.axes_letters_color;
        text_renderer.setColor(c.red, c.green, c.blue);
        drawYLetter(
            text_renderer, view_model, v_viewport, projection, width, height, x, z, z_value_distance_multiplier);
    }

    if (!plot_pane_settings.axes_numbers_on)
    {
        return;
    }
    const auto c = plot_pane_settings.axes_numbers_color;
    text_renderer.setColor(c.red, c.green, c.blue);

    for (size_t k = 0; k < gv.y.num_valid_values; k++)
    {
        const double y = gv.y.data[k];
        const glm::vec3 v3(x, y, z);

        const glm::vec3 v_projected = glm::project(v3, view_model, projection, v_viewport);
        const std::string val = formatNumber(gv.y.data[k] + axes_center.y, 3);
        /*if (elevation == M_PI / 2.0)
        {
            text_renderer.renderTextFromRightCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
        }*/
        // if (view_an)
        if (view_angles.isSnappedLookingAlongNegativeY())
        {
            text_renderer.renderTextFromLeftCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
        }
        else
        {
            if (cond2)
            {
                text_renderer.renderTextFromLeftCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
            }
            else
            {
                text_renderer.renderTextFromRightCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
            }
        }
    }
}

void drawZAxisNumbers(TextRenderer& text_renderer,
                      const PlotPaneSettings& plot_pane_settings,
                      const glm::mat4& view_model,
                      const glm::vec4& v_viewport,
                      const glm::mat4& projection,
                      const ViewAngles& view_angles,
                      const double azimuth,
                      const double elevation,
                      const float width,
                      const float height,
                      const Vec3d& axes_center,
                      const GridVectors& gv,
                      const AxesSideConfiguration& axes_side_configuration,
                      const float distance_multiplier,
                      const bool perspective_projection)
{
    float x_value_distance_multiplier = distance_multiplier;
    float y_value_distance_multiplier = distance_multiplier;

    if (perspective_projection)
    {
        if ((azimuth >= 0.0f) && (azimuth <= M_PI_2))
        {
            y_value_distance_multiplier = 1.0f;
        }
        else if ((azimuth >= M_PI_2) && (azimuth <= M_PI))
        {
            x_value_distance_multiplier = 1.0f;
        }
        else if ((azimuth >= (-M_PI_2)) && (azimuth <= 0.0f))
        {
            x_value_distance_multiplier = 1.0f;
        }
        else if ((azimuth >= (-M_PI)) && (azimuth <= (-M_PI_2)))
        {
            y_value_distance_multiplier = 1.0f;
        }
    }

    const double x = axes_side_configuration.z_axes_numbers_x_value * x_value_distance_multiplier;
    const double y = axes_side_configuration.z_axes_numbers_y_value * y_value_distance_multiplier;

    if (plot_pane_settings.axes_letters_on)
    {
        const auto c = plot_pane_settings.axes_letters_color;
        text_renderer.setColor(c.red, c.green, c.blue);
        drawZLetter(text_renderer,
                    view_model,
                    v_viewport,
                    projection,
                    width,
                    height,
                    x,
                    y,
                    x_value_distance_multiplier,
                    y_value_distance_multiplier);
    }

    if (!plot_pane_settings.axes_numbers_on)
    {
        return;
    }
    const auto c = plot_pane_settings.axes_numbers_color;
    text_renderer.setColor(c.red, c.green, c.blue);

    for (size_t k = 0; k < gv.z.num_valid_values; k++)
    {
        const double z = gv.z.data[k];
        const glm::vec3 v3(x, y, z);

        const glm::vec3 v_projected = glm::project(v3, view_model, projection, v_viewport);
        const std::string val = formatNumber(gv.z.data[k] + axes_center.z, 3);

        if (view_angles.isSnappedLookingAlongNegativeY())
        {
            text_renderer.renderTextFromLeftCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
        }
        else
        {
            text_renderer.renderTextFromRightCenter(val, v_projected[0], v_projected[1], kTextScale, width, height);
        }
    }
}

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
                     const bool perspective_projection)
{

    glm::mat4 model_mat_local = model_mat;

    // AxesLimits
    const Vec3d axes_center = axes_limits.getAxesCenter();

    // Scales
    const Vec3d scale = axes_limits.getAxesScale() / 2.0;

    const float distance_multiplier = perspective_projection ? 1.1f : 1.05f;

    model_mat_local[3][0] = 0.0;
    model_mat_local[3][1] = 0.0;
    model_mat_local[3][2] = 0.0;

    const glm::vec4 v_viewport = glm::vec4(-1, -1, 2, 2);

    const double azimuth = view_angles.getSnappedAzimuth();
    const double elevation = view_angles.getSnappedElevation();

    glm::mat4 scale_mat = glm::mat4(1.0f);

    scale_mat[0][0] = 1.0 / scale.x;
    scale_mat[1][1] = 1.0;
    scale_mat[2][2] = 1.0;
    scale_mat[3][3] = 1.0;

    const glm::mat4 view_model_x = view_mat * model_mat_local * scale_mat;

    scale_mat[0][0] = 1.0;
    scale_mat[1][1] = 1.0 / scale.y;
    const glm::mat4 view_model_y = view_mat * model_mat_local * scale_mat;

    scale_mat[1][1] = 1.0;
    scale_mat[2][2] = 1.0 / scale.z;
    const glm::mat4 view_model_z = view_mat * model_mat_local * scale_mat;

    if ((!view_angles.isSnappedAlongX()) || perspective_projection)
    {
        drawXAxisNumbers(text_renderer,
                         plot_pane_settings,
                         view_model_x,
                         v_viewport,
                         projection_mat,
                         azimuth,
                         elevation,
                         width,
                         height,
                         view_angles.getSnappingAxis(),
                         axes_center,
                         gv,
                         axes_side_configuration,
                         distance_multiplier,
                         perspective_projection);
    }
    if ((!view_angles.isSnappedAlongY()) || perspective_projection)
    {
        drawYAxisNumbers(text_renderer,
                         plot_pane_settings,
                         view_model_y,
                         v_viewport,
                         projection_mat,
                         view_angles,
                         azimuth,
                         elevation,
                         width,
                         height,
                         view_angles.getSnappingAxis(),
                         axes_center,
                         gv,
                         axes_side_configuration,
                         distance_multiplier,
                         perspective_projection);
    }
    if ((!view_angles.isSnappedAlongZ()) || perspective_projection)
    {
        drawZAxisNumbers(text_renderer,
                         plot_pane_settings,
                         view_model_z,
                         v_viewport,
                         projection_mat,
                         view_angles,
                         azimuth,
                         elevation,
                         width,
                         height,
                         axes_center,
                         gv,
                         axes_side_configuration,
                         distance_multiplier,
                         perspective_projection);
    }
}