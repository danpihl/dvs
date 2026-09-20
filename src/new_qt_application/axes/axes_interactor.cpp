#include "axes/axes_interactor.h"

#include <cassert>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utility>

#include "axes/structures/grid_vectors.h"
#include "lumos/math.h"
#include "opengl_low_level/opengl_header.h"

AxesInteractor::AxesInteractor(const AxesSettings& axes_settings, const int window_height, const int window_width)
{
    const Vec3d min_vec = Vec3d(-1.0, -1.0, -1.0);
    const Vec3d max_vec = Vec3d(1.0, 1.0, 1.0);

    has_query_points_ = false;
    axes_limits_ = AxesLimits(min_vec, max_vec);
    default_axes_limits_ = axes_limits_;
    should_draw_zoom_rect_ = false;

    current_window_width = window_width;
    current_window_height = window_height;

    view_angles_ = ViewAngles(0.0 * M_PI / 180.0, 90.0 * M_PI / 180.0, 5.0 * M_PI / 180.0);
    default_view_angles_ = view_angles_;

    axes_settings_ = axes_settings;
    current_mouse_interaction_type_ = MouseInteractionType::ROTATE;
    overridden_mouse_interaction_type_ = MouseInteractionType::UNCHANGED;

    is_image_view_ = false;

    const size_t num_lines = axes_settings_.num_axes_ticks;
    inc0 = 0.9999999999 * (default_axes_limits_.getMax() - default_axes_limits_.getMin()) /
           static_cast<double>(num_lines - 1);
    mouse_pressed_ = false;
    show_legend_ = false;
}

void AxesInteractor::registerMousePressed(const Vec2f& mouse_pos)
{
    mouse_pressed_ = true;
    mouse_pos_at_press_ = mouse_pos;
}

void AxesInteractor::registerMouseReleased(const Vec2f& mouse_pos)
{
    if (should_draw_zoom_rect_)
    {
        const float sw = 3.0f;
        const glm::mat4 orth_projection_mat = glm::ortho(-sw, sw, -sw, sw, 0.1f, 100.0f);
        const glm::vec4 v_viewport = glm::vec4(-1, -1, 2, 2);
        const float az = std::pow(std::fabs(std::sin(view_angles_.getSnappedAzimuth() * 2.0f)), 0.6) * 0.7;
        const float el = std::pow(std::fabs(std::sin(view_angles_.getSnappedElevation() * 2.0f)), 0.7) * 0.5;
        const float f = 2.5;
        glm::mat4 window_scale_mat = glm::mat4(1.0f);

        window_scale_mat[0][0] = 2.7;
        window_scale_mat[1][1] = 2.7;
        window_scale_mat[2][2] = 2.7;

        window_scale_mat[0][0] = f - az - el;
        window_scale_mat[1][1] = f - az - el;
        window_scale_mat[2][2] = f - az - el;
        const Matrix<double> rot_mat =
            rotationMatrixZ(-view_angles_.getSnappedAzimuth()) * rotationMatrixX(-view_angles_.getSnappedElevation());

        glm::mat4 model_mat = glm::mat4(1.0f);

        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
            {
                model_mat[r][c] = rot_mat(r, c);
            }
        }

        const glm::mat4 view_mat = glm::lookAt(glm::vec3(0, -6.0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));

        const Vec2f mouse_pos_at_release = mouse_pos;
        const Vec2f window_xy(current_window_width, current_window_height);

        const Vec2f mouse_pos_at_press_norm = mouse_pos_at_press_;     // .elementWiseDivide(window_xy);
        const Vec2f mouse_pos_at_release_norm = mouse_pos_at_release;  // .elementWiseDivide(window_xy);

        const Vec2f mouse_pos_at_press_mod =
            2.0f * (mouse_pos_at_press_norm.elementWiseMultiply(Vec2f(1.0f, -1.0f)) + Vec2f(0.0f, 1.0f)) - 1.0f;
        const Vec2f current_mouse_pos_mod =
            2.0f * (mouse_pos_at_release_norm.elementWiseMultiply(Vec2f(1.0f, -1.0f)) + Vec2f(0.0f, 1.0f)) - 1.0f;

        const SnappingAxis snapping_axis = view_angles_.getSnappingAxis();

        const glm::vec3 mouse_pos_at_press_projected(mouse_pos_at_press_mod.x, mouse_pos_at_press_mod.y, 0);
        const glm::vec3 current_mouse_pos_projected(current_mouse_pos_mod.x, current_mouse_pos_mod.y, 0);

        const glm::mat4 view_model = view_mat * model_mat * window_scale_mat;

        const glm::vec3 current_mouse_pos_unprojected =
            glm::unProject(current_mouse_pos_projected, view_model, orth_projection_mat, v_viewport);
        const glm::vec3 mouse_pos_at_press_unprojected =
            glm::unProject(mouse_pos_at_press_projected, view_model, orth_projection_mat, v_viewport);

        const Vec3f scale_vec_div_2 = axes_limits_.getAxesScale() / 2.0;
        const Vec3f scale_vec = axes_limits_.getAxesScale();
        const Vec3f axes_center = axes_limits_.getAxesCenter();
        const Vec3f min_vec = axes_center - scale_vec_div_2;
        const Vec3f max_vec = axes_center + scale_vec_div_2;

        if (SnappingAxis::SA_X == snapping_axis)
        {
            const float x_start = mouse_pos_at_press_unprojected.y / 2.0f + 0.5f;
            const float y_start = mouse_pos_at_press_unprojected.z / 2.0f + 0.5f;

            const float x_end = current_mouse_pos_unprojected.y / 2.0f + 0.5f;
            const float y_end = current_mouse_pos_unprojected.z / 2.0f + 0.5f;

            // Min/max
            const float x_min = std::min(x_start, x_end);
            const float x_max = std::max(x_start, x_end);
            const float y_min = std::min(y_start, y_end);
            const float y_max = std::max(y_start, y_end);

            // Find new points from scale
            const float new_y_min = min_vec.y + x_min * scale_vec.y;
            const float new_y_max = min_vec.y + x_max * scale_vec.y;

            const float new_z_min = min_vec.z + y_min * scale_vec.z;
            const float new_z_max = min_vec.z + y_max * scale_vec.z;

            axes_limits_.setMin(Vec3f(min_vec.x, new_y_min, new_z_min));
            axes_limits_.setMax(Vec3f(max_vec.x, new_y_max, new_z_max));
        }
        else if (SnappingAxis::SA_Y == snapping_axis)
        {
            // Normalize to be in the interval [0.0, 1.0]
            const float x_start = mouse_pos_at_press_unprojected.x / 2.0f + 0.5f;
            const float y_start = mouse_pos_at_press_unprojected.z / 2.0f + 0.5f;

            const float x_end = current_mouse_pos_unprojected.x / 2.0f + 0.5f;
            const float y_end = current_mouse_pos_unprojected.z / 2.0f + 0.5f;

            // Min/max
            const float x_min = std::min(x_start, x_end);
            const float x_max = std::max(x_start, x_end);
            const float y_min = std::min(y_start, y_end);
            const float y_max = std::max(y_start, y_end);

            // Find new points from scale
            const float new_x_min = min_vec.x + x_min * scale_vec.x;
            const float new_x_max = min_vec.x + x_max * scale_vec.x;

            const float new_z_min = min_vec.z + y_min * scale_vec.z;
            const float new_z_max = min_vec.z + y_max * scale_vec.z;

            axes_limits_.setMin(Vec3f(new_x_min, min_vec.y, new_z_min));
            axes_limits_.setMax(Vec3f(new_x_max, max_vec.y, new_z_max));
        }
        else if (SnappingAxis::SA_Z == snapping_axis)
        {
            const float x_start = mouse_pos_at_press_unprojected.x / 2.0f + 0.5f;
            const float y_start = mouse_pos_at_press_unprojected.y / 2.0f + 0.5f;

            const float x_end = current_mouse_pos_unprojected.x / 2.0f + 0.5f;
            const float y_end = current_mouse_pos_unprojected.y / 2.0f + 0.5f;

            // Min/max
            const float x_min = std::min(x_start, x_end);
            const float x_max = std::max(x_start, x_end);
            const float y_min = std::min(y_start, y_end);
            const float y_max = std::max(y_start, y_end);

            // Find new points from scale
            const float new_x_min = min_vec.x + x_min * scale_vec.x;
            const float new_x_max = min_vec.x + x_max * scale_vec.x;

            const float new_y_min = min_vec.y + y_min * scale_vec.y;
            const float new_y_max = min_vec.y + y_max * scale_vec.y;

            axes_limits_.setMin(Vec3f(new_x_min, new_y_min, min_vec.z));
            axes_limits_.setMax(Vec3f(new_x_max, new_y_max, max_vec.z));
        }

        should_draw_zoom_rect_ = false;
    }
    mouse_pressed_ = false;
}

ViewAngles AxesInteractor::getViewAngles() const
{
    return view_angles_;
}

AxesLimits AxesInteractor::getAxesLimits() const
{
    return axes_limits_;
}

void AxesInteractor::setViewAnglesSnapAngle(const double snap_angle)
{
    view_angles_.setAngleLimit(snap_angle);
}

void AxesInteractor::showLegend(const bool show_legend)
{
    show_legend_ = show_legend;
}

void AxesInteractor::updateWindowSize(const int window_width, const int window_height)
{
    current_window_width = window_width;
    current_window_height = window_height;
}

void AxesInteractor::setMouseInteractionType(const MouseInteractionType interaction_type)
{
    current_mouse_interaction_type_ = interaction_type;
}

void AxesInteractor::setOverriddenMouseInteractionType(const MouseInteractionType overridden_mouse_interaction_type)
{
    overridden_mouse_interaction_type_ = overridden_mouse_interaction_type;
}

void AxesInteractor::resetView()
{
    view_angles_ = default_view_angles_;
    axes_limits_ = default_axes_limits_;
}

void AxesInteractor::registerMouseDragInput(
    const MouseInteractionAxis current_mouse_interaction_axis, const int x, const int y, const int dx, const int dy)
{
    const float dx_mod = 250.0f * static_cast<float>(dx) / current_window_width;
    const float dy_mod = 250.0f * static_cast<float>(dy) / current_window_height;
    should_draw_zoom_rect_ = false;
    has_query_points_ = false;

    const MouseInteractionType mit = (overridden_mouse_interaction_type_ != MouseInteractionType::UNCHANGED)
                                         ? overridden_mouse_interaction_type_
                                         : current_mouse_interaction_type_;

    switch (mit)
    {
        case MouseInteractionType::ROTATE:
            if (!is_image_view_)
            {
                changeRotation(
                    dx_mod * rotation_mouse_gain, dy_mod * rotation_mouse_gain, current_mouse_interaction_axis);
            }
            break;
        case MouseInteractionType::ZOOM:
            // if (SnappingAxis::SA_None == view_angles_.getSnappingAxis()) // TODO: Add back
            {
                changeZoom(dy_mod * zoom_mouse_gain, current_mouse_interaction_axis);
            }
            /*else
            {
                should_draw_zoom_rect_ = false;  // TODO: Temporary, should be true for it to work
            }*/
            break;
        case MouseInteractionType::PAN:
            changePan(dx_mod * pan_mouse_gain, dy_mod * pan_mouse_gain, current_mouse_interaction_axis);
            break;
        case MouseInteractionType::POINT_SELECTION:
            query_point_screen_x_ = x;
            query_point_screen_y_ = y;
            has_query_points_ = true;
            break;
        default:
            break;
    }
}

QueryPoint AxesInteractor::getQueryPoint() const
{
    return QueryPoint{has_query_points_, query_point_screen_x_, query_point_screen_y_};
}

void AxesInteractor::changeRotation(const double dx, const double dy, const MouseInteractionAxis mia)
{
    Vec2d sa(1.0, 1.0);
    if (mia == MouseInteractionAxis::X)
    {
        sa.y = 0.0;
    }
    else if (mia == MouseInteractionAxis::Y)
    {
        sa.x = 0.0;
    }
    else
    {
        sa.x = 1.0;
        sa.y = 1.0;
    }
    view_angles_.changeAnglesWithDelta(dx * sa.x, dy * sa.y);
}

double changeIncrement(const double scale, const double inc, const size_t num_lines)
{
    double new_inc = inc;
    if ((scale / inc) < static_cast<double>(num_lines) / 2.0)
    {
        new_inc = inc / 2.0;
    }
    else if ((scale / inc) > static_cast<double>(num_lines) * 1.0)
    {
        new_inc = inc * 2.0;
    }
    return new_inc;
}

void AxesInteractor::changeZoom(const double dy, const MouseInteractionAxis mia)
{
    const Vec3d s = axes_limits_.getAxesScale();
    const SnappingAxis snapping_axis = view_angles_.getSnappingAxis();
    Vec3d sa(1.0, 1.0, 1.0);

    if (SnappingAxis::SA_None == snapping_axis)  // TODO: Add back
    {
        if (mia == MouseInteractionAxis::X)
        {
            sa.y = 0.0;
            sa.z = 0.0;
        }
        else if (mia == MouseInteractionAxis::Y)
        {
            sa.x = 0.0;
            sa.z = 0.0;
        }
        else if (mia == MouseInteractionAxis::Z)
        {
            sa.x = 0.0;
            sa.y = 0.0;
        }
        else if (mia == MouseInteractionAxis::XY)
        {
            sa.z = 0.0;
        }
        else if (mia == MouseInteractionAxis::YZ)
        {
            sa.x = 0.0;
        }
        else if (mia == MouseInteractionAxis::XZ)
        {
            sa.y = 0.0;
        }
    }
    else
    {
        if (snapping_axis == SnappingAxis::SA_X)
        {
            sa.x = 0.0;
        }
        else if (snapping_axis == SnappingAxis::SA_Y)
        {
            sa.y = 0.0;
        }
        else if (snapping_axis == SnappingAxis::SA_Z)
        {
            sa.z = 0.0;
        }
    }
    const Vec3d inc_vec = Vec3d(dy * s.x * sa.x, dy * s.y * sa.y, dy * s.z * sa.z);

    axes_limits_.setMin(axes_limits_.getMin() - inc_vec);
    axes_limits_.setMax(axes_limits_.getMax() + inc_vec);

    const size_t num_lines = axes_settings_.num_axes_ticks;
    inc0.x = changeIncrement(s.x, inc0.x, num_lines);
    inc0.y = changeIncrement(s.y, inc0.y, num_lines);
    inc0.z = changeIncrement(s.z, inc0.z, num_lines);
}

void AxesInteractor::changePan(const double dx, const double dy, const MouseInteractionAxis mia)
{
    Vec3d sa(1.0, 1.0, 1.0);
    if (mia == MouseInteractionAxis::X)
    {
        sa.y = 0.0;
        sa.z = 0.0;
    }
    else if (mia == MouseInteractionAxis::Y)
    {
        sa.x = 0.0;
        sa.z = 0.0;
    }
    else if (mia == MouseInteractionAxis::Z)
    {
        sa.x = 0.0;
        sa.y = 0.0;
    }
    else if (mia == MouseInteractionAxis::XY)
    {
        sa.z = 0.0;
    }
    else if (mia == MouseInteractionAxis::YZ)
    {
        sa.x = 0.0;
    }
    else if (mia == MouseInteractionAxis::XZ)
    {
        sa.y = 0.0;
    }
    const Matrixd rotation_mat = view_angles_.getSnappedRotationMatrix();
    const Vec3d v = rotation_mat.getTranspose() * Vec3d(-dx, dy, 0.0);

    const Vec3d s = axes_limits_.getAxesScale();

    const Vec3d v_scaled = v.elementWiseMultiply(s).elementWiseMultiply(sa);

    axes_limits_.incrementMinMax(v_scaled);
}

void AxesInteractor::setViewAngles(const double azimuth, const double elevation)
{
    view_angles_.setAngles(azimuth, elevation);
    default_view_angles_.setAngles(azimuth, elevation);
}

void AxesInteractor::setAxesLimits(const Vec3d& min_vec, const Vec3d& max_vec)
{
    axes_limits_ = AxesLimits(min_vec, max_vec);
    default_axes_limits_ = axes_limits_;

    // TODO: Remove below?
    const size_t num_lines = axes_settings_.num_axes_ticks;
    inc0 = 0.9999999999 * (default_axes_limits_.getMax() - default_axes_limits_.getMin()) /
           static_cast<double>(num_lines - 1);
}

void AxesInteractor::setAxesLimits(const Vec2d& min_vec, const Vec2d& max_vec)
{
    axes_limits_ =
        AxesLimits({min_vec.x, min_vec.y, axes_limits_.getMin().z}, {max_vec.x, max_vec.y, axes_limits_.getMax().z});
    default_axes_limits_ = axes_limits_;

    const size_t num_lines = axes_settings_.num_axes_ticks;
    inc0 = 0.9999999999 * (default_axes_limits_.getMax() - default_axes_limits_.getMin()) /
           static_cast<double>(num_lines - 1);
}

void generateAxisVector(
    const double min_val, const double max_val, const double num_lines, const double offset, GridVector& ret_vec)
{
    const double d = max_val - min_val;

    const double d_inc = std::pow(2.0, std::floor(std::log2(d / num_lines)));
    double val = min_val - std::fmod(min_val, d_inc);

    std::vector<double> vec;
    vec.reserve(num_lines);

    int it = 0;

    while (val < max_val)
    {
        if (val > min_val)
        {
            vec.push_back(val - offset);
        }
        val = val + d_inc;
        it++;
        if (it > static_cast<int>(num_lines * 3))
        {
            std::cout << "ERROR: Number of lines grew a lot!" << std::endl;
            break;
        }
    }

    LUMOS_ASSERT(vec.size() <= GridVector::kMaxNumGridNumbers);
    LUMOS_ASSERT(vec.size() > 1U);

    ret_vec.grid_spacing = d_inc;
    ret_vec.min_value = vec[0];
    ret_vec.max_value = vec[vec.size() - 1U];
    ret_vec.range = ret_vec.max_value - ret_vec.min_value;
    ret_vec.num_valid_values = vec.size();

    for (size_t k = 0; k < vec.size(); k++)
    {
        ret_vec.data[k] = vec[k];
    }
}

GridVectors AxesInteractor::generateGridVectors()
{
    GridVectors gv;

    const Vec3d axes_center = axes_limits_.getAxesCenter();

    const Vec3d v_min = (axes_limits_.getMin() - axes_center) + axes_center;
    const Vec3d v_max = (axes_limits_.getMax() - axes_center) + axes_center;

    generateAxisVector(v_min.x, v_max.x, axes_settings_.num_axes_ticks, axes_center.x, gv.x);
    generateAxisVector(v_min.y, v_max.y, axes_settings_.num_axes_ticks, axes_center.y, gv.y);
    generateAxisVector(v_min.z, v_max.z, axes_settings_.num_axes_ticks, axes_center.z, gv.z);

    return gv;
}
