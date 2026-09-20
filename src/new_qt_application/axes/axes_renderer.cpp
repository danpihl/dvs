#include "axes_renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "lumos/math.h"
#include "misc/misc.h"

using namespace lumos;

AxesRenderer::AxesRenderer(const ShaderCollection& shader_collection,
                           const PlotPaneSettings& plot_pane_settings,
                           const RGBTripletf& tab_background_color)
    : shader_collection_{shader_collection},
      legend_renderer_{text_renderer_, shader_collection_},
      point_selection_box_{text_renderer_, shader_collection_},
      plot_pane_settings_{plot_pane_settings},
      plot_pane_background_{plot_pane_settings_.pane_radius},
      tab_background_color_{tab_background_color}
{
    axes_square_ = false;
    scale_on_rotation_ = true;
    glEnable(GL_DEPTH_CLAMP);

    text_renderer_.init();

    if (plot_pane_settings_.clipping_on)
    {
        shader_collection_.basic_plot_shader.use();
        shader_collection_.basic_plot_shader.base_uniform_handles.use_clip_plane.setInt(1);
        shader_collection_.img_plot_shader.use();
        shader_collection_.img_plot_shader.base_uniform_handles.use_clip_plane.setInt(1);
        shader_collection_.draw_mesh_shader.use();
        shader_collection_.draw_mesh_shader.base_uniform_handles.use_clip_plane.setInt(1);
        shader_collection_.scatter_shader.use();
        shader_collection_.scatter_shader.base_uniform_handles.use_clip_plane.setInt(1);
        shader_collection_.plot_2d_shader.use();
        shader_collection_.plot_2d_shader.base_uniform_handles.use_clip_plane.setInt(1);
        shader_collection_.plot_3d_shader.use();
        shader_collection_.plot_3d_shader.base_uniform_handles.use_clip_plane.setInt(1);
    }
    else
    {
        shader_collection_.basic_plot_shader.use();
        shader_collection_.basic_plot_shader.base_uniform_handles.use_clip_plane.setInt(0);
        shader_collection_.img_plot_shader.use();
        shader_collection_.img_plot_shader.base_uniform_handles.use_clip_plane.setInt(0);
        shader_collection_.draw_mesh_shader.use();
        shader_collection_.draw_mesh_shader.base_uniform_handles.use_clip_plane.setInt(0);
        shader_collection_.scatter_shader.use();
        shader_collection_.scatter_shader.base_uniform_handles.use_clip_plane.setInt(0);
        shader_collection_.plot_2d_shader.use();
        shader_collection_.plot_2d_shader.base_uniform_handles.use_clip_plane.setInt(0);
        shader_collection_.plot_3d_shader.use();
        shader_collection_.plot_3d_shader.base_uniform_handles.use_clip_plane.setInt(0);
    }

    const float sw = 3.0f;
    orth_projection_mat_ = glm::ortho(-sw, sw, -sw, sw, 0.1f, 100.0f);
    persp_projection_mat_ = glm::perspective(glm::radians(75.0f), 1.0f, 0.1f, 100.0f);

    use_perspective_proj_ = false;
    current_mouse_interaction_axis_ = MouseInteractionAxis::ALL;

    projection_mat_ = use_perspective_proj_ ? persp_projection_mat_ : orth_projection_mat_;

    // Camera matrix
    view_mat_ = glm::lookAt(glm::vec3(0, -6.0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
    model_mat_ = glm::mat4(1.0f);
    scale_mat_ = glm::mat4(1.0f);

    window_scale_mat_ = glm::mat4(1.0f);
    scale_vector_ = Vec3d(2.5, 2.5, 2.5);

    global_illumination_active_ = false;

    if (plot_pane_settings_.title != "")
    {
        setTitle(plot_pane_settings_.title);
    }
}

void AxesRenderer::enableClipPlanes()
{
    if (plot_pane_settings_.clipping_on)
    {
        const double f = 0.1;

        const Vec3d axes_center = axes_limits_.getAxesCenter();

        const Vec3d scale = axes_limits_.getAxesScale() / 2.0;

        // clang-format off
        const Vector<Point3d> points_x0{VectorInitializer{Point3d(scale.x - axes_center.x, f, f),
                                        Point3d(scale.x - axes_center.x, -f, f),
                                        Point3d(scale.x - axes_center.x, f, -f)}};
        const Vector<Point3d> points_x1{VectorInitializer{(Point3d(scale.x + axes_center.x, f, f)),
                                        Point3d(scale.x + axes_center.x, -f, f),
                                        Point3d(scale.x + axes_center.x, f, -f)}};
        const Vector<Point3d> points_y0{VectorInitializer{Point3d(-f, scale.y - axes_center.y, f),
                                        Point3d(f, scale.y - axes_center.y, f),
                                        Point3d(f, scale.y - axes_center.y, -f)}};
        const Vector<Point3d> points_y1{VectorInitializer{Point3d(-f, scale.y + axes_center.y, f),
                                        Point3d(f, scale.y + axes_center.y, f),
                                        Point3d(f, scale.y + axes_center.y, -f)}};
        const Vector<Point3d> points_z0{VectorInitializer{Point3d(-f, f, -(axes_center.z + scale.z)),
                                        Point3d(f, -f, -(axes_center.z + scale.z)),
                                        Point3d(-f, -f, -(axes_center.z + scale.z))}};
        const Vector<Point3d> points_z1{VectorInitializer{Point3d(-f, f, -(scale.z - axes_center.z)),
                                        Point3d(f, -f, -(scale.z - axes_center.z)),
                                        Point3d(-f, -f, -(scale.z - axes_center.z))}};

        setClipPlane(shader_collection_.basic_plot_shader.base_uniform_handles.clip_plane0, points_x0(0), points_x0(1), points_x0(2), true);
        setClipPlane(shader_collection_.basic_plot_shader.base_uniform_handles.clip_plane1, points_x1(0), points_x1(1), points_x1(2), false);
        setClipPlane(shader_collection_.basic_plot_shader.base_uniform_handles.clip_plane2, points_y0(0), points_y0(1), points_y0(2), true);
        setClipPlane(shader_collection_.basic_plot_shader.base_uniform_handles.clip_plane3, points_y1(0), points_y1(1), points_y1(2), false);
        setClipPlane(shader_collection_.basic_plot_shader.base_uniform_handles.clip_plane4, points_z0(0), points_z0(1), points_z0(2), true);
        setClipPlane(shader_collection_.basic_plot_shader.base_uniform_handles.clip_plane5, points_z1(0), points_z1(1), points_z1(2), false);

        shader_collection_.img_plot_shader.use();
        setClipPlane(shader_collection_.img_plot_shader.base_uniform_handles.clip_plane0, points_x0(0), points_x0(1), points_x0(2), true);
        setClipPlane(shader_collection_.img_plot_shader.base_uniform_handles.clip_plane1, points_x1(0), points_x1(1), points_x1(2), false);
        setClipPlane(shader_collection_.img_plot_shader.base_uniform_handles.clip_plane2, points_y0(0), points_y0(1), points_y0(2), true);
        setClipPlane(shader_collection_.img_plot_shader.base_uniform_handles.clip_plane3, points_y1(0), points_y1(1), points_y1(2), false);
        setClipPlane(shader_collection_.img_plot_shader.base_uniform_handles.clip_plane4, points_z0(0), points_z0(1), points_z0(2), true);
        setClipPlane(shader_collection_.img_plot_shader.base_uniform_handles.clip_plane5, points_z1(0), points_z1(1), points_z1(2), false);

        shader_collection_.draw_mesh_shader.use();
        setClipPlane(shader_collection_.draw_mesh_shader.base_uniform_handles.clip_plane0, points_x0(0), points_x0(1), points_x0(2), true);
        setClipPlane(shader_collection_.draw_mesh_shader.base_uniform_handles.clip_plane1, points_x1(0), points_x1(1), points_x1(2), false);
        setClipPlane(shader_collection_.draw_mesh_shader.base_uniform_handles.clip_plane2, points_y0(0), points_y0(1), points_y0(2), true);
        setClipPlane(shader_collection_.draw_mesh_shader.base_uniform_handles.clip_plane3, points_y1(0), points_y1(1), points_y1(2), false);
        setClipPlane(shader_collection_.draw_mesh_shader.base_uniform_handles.clip_plane4, points_z0(0), points_z0(1), points_z0(2), true);
        setClipPlane(shader_collection_.draw_mesh_shader.base_uniform_handles.clip_plane5, points_z1(0), points_z1(1), points_z1(2), false);

        shader_collection_.scatter_shader.use();
        setClipPlane(shader_collection_.scatter_shader.base_uniform_handles.clip_plane0, points_x0(0), points_x0(1), points_x0(2), true);
        setClipPlane(shader_collection_.scatter_shader.base_uniform_handles.clip_plane1, points_x1(0), points_x1(1), points_x1(2), false);
        setClipPlane(shader_collection_.scatter_shader.base_uniform_handles.clip_plane2, points_y0(0), points_y0(1), points_y0(2), true);
        setClipPlane(shader_collection_.scatter_shader.base_uniform_handles.clip_plane3, points_y1(0), points_y1(1), points_y1(2), false);
        setClipPlane(shader_collection_.scatter_shader.base_uniform_handles.clip_plane4, points_z0(0), points_z0(1), points_z0(2), true);
        setClipPlane(shader_collection_.scatter_shader.base_uniform_handles.clip_plane5, points_z1(0), points_z1(1), points_z1(2), false);

        shader_collection_.plot_2d_shader.use();
        setClipPlane(shader_collection_.plot_2d_shader.base_uniform_handles.clip_plane0, points_x0(0), points_x0(1), points_x0(2), true);
        setClipPlane(shader_collection_.plot_2d_shader.base_uniform_handles.clip_plane1, points_x1(0), points_x1(1), points_x1(2), false);
        setClipPlane(shader_collection_.plot_2d_shader.base_uniform_handles.clip_plane2, points_y0(0), points_y0(1), points_y0(2), true);
        setClipPlane(shader_collection_.plot_2d_shader.base_uniform_handles.clip_plane3, points_y1(0), points_y1(1), points_y1(2), false);
        setClipPlane(shader_collection_.plot_2d_shader.base_uniform_handles.clip_plane4, points_z0(0), points_z0(1), points_z0(2), true);
        setClipPlane(shader_collection_.plot_2d_shader.base_uniform_handles.clip_plane5, points_z1(0), points_z1(1), points_z1(2), false);

        shader_collection_.plot_3d_shader.use();
        setClipPlane(shader_collection_.plot_3d_shader.base_uniform_handles.clip_plane0, points_x0(0), points_x0(1), points_x0(2), true);
        setClipPlane(shader_collection_.plot_3d_shader.base_uniform_handles.clip_plane1, points_x1(0), points_x1(1), points_x1(2), false);
        setClipPlane(shader_collection_.plot_3d_shader.base_uniform_handles.clip_plane2, points_y0(0), points_y0(1), points_y0(2), true);
        setClipPlane(shader_collection_.plot_3d_shader.base_uniform_handles.clip_plane3, points_y1(0), points_y1(1), points_y1(2), false);
        setClipPlane(shader_collection_.plot_3d_shader.base_uniform_handles.clip_plane4, points_z0(0), points_z0(1), points_z0(2), true);
        setClipPlane(shader_collection_.plot_3d_shader.base_uniform_handles.clip_plane5, points_z1(0), points_z1(1), points_z1(2), false);
        // clang-format on
    }

    shader_collection_.basic_plot_shader.use();
}

void AxesRenderer::setClipPlane(const GLuint clip_plane_uniform_handle,
                                const Point3d& p0,
                                const Point3d& p1,
                                const Point3d& p2,
                                const bool invert) const
{
    // Fit plane
    const Planed fp = planeFromThreePoints(p0, p1, p2);

    // Invert
    const Planed plane = invert ? Planed(-fp.a, -fp.b, -fp.c, fp.d) : fp;

    glUniform4f(clip_plane_uniform_handle, plane.a, plane.b, plane.c, plane.d);
}

void AxesRenderer::renderHandle()
{
    return;  // TODO: For demo, handle is not used
    shader_collection_.text_shader.use();

    glUniform3f(shader_collection_.text_shader.uniform_handles.text_color, 0.0, 0.0, 0.0);

    const float x_coord = -1.0f + 5.0f / width_;
    const float y_coord = 1.0f - 20.0f / height_;
    text_renderer_.renderTextFromLeftCenter(name_, x_coord, y_coord, 0.0005f, width_, height_);
}

void AxesRenderer::renderTitle()
{
    text_renderer_.setColor(0.0f, 0.0f, 0.0f);

    const float x_coord = 5.0f / width_;
    const float y_coord = 1.0f - 20.0f / height_;
    text_renderer_.renderTextFromCenter(title_, x_coord, y_coord, 0.0005f, width_, height_);
}

void AxesRenderer::renderInteractionLetter()
{
    return;  // TODO: For demo, interaction letter is not used
    shader_collection_.text_shader.use();

    glUniform3f(shader_collection_.text_shader.uniform_handles.text_color, 0.0, 0.0, 0.0);

    std::string interaction_string;
    if (mouse_interaction_type_ == MouseInteractionType::PAN)
    {
        interaction_string = "Pan";
    }
    else if (mouse_interaction_type_ == MouseInteractionType::ZOOM)
    {
        interaction_string = "Zoom";
    }
    else if (mouse_interaction_type_ == MouseInteractionType::ROTATE)
    {
        interaction_string = "Rotate";
        if (current_mouse_interaction_axis_ == MouseInteractionAxis::X)
        {
            interaction_string += " [azimuth]";
        }
        else if (current_mouse_interaction_axis_ == MouseInteractionAxis::Y)
        {
            interaction_string += " [elevation]";
        }
    }
    else
    {
        interaction_string = "U";
    }

    if (mouse_interaction_type_ != MouseInteractionType::ROTATE)
    {
        if (current_mouse_interaction_axis_ == MouseInteractionAxis::X)
        {
            interaction_string += " [x]";
        }
        if (current_mouse_interaction_axis_ == MouseInteractionAxis::Y)
        {
            interaction_string += " [y]";
        }
        if (current_mouse_interaction_axis_ == MouseInteractionAxis::Z)
        {
            interaction_string += " [z]";
        }
        if (current_mouse_interaction_axis_ == MouseInteractionAxis::XY)
        {
            interaction_string += " [xy]";
        }
        if (current_mouse_interaction_axis_ == MouseInteractionAxis::XZ)
        {
            interaction_string += " [xz]";
        }
        if (current_mouse_interaction_axis_ == MouseInteractionAxis::YZ)
        {
            interaction_string += " [yz]";
        }
    }

    const float y_coord = -1.0f + 17.0f / height_;
    const float x_coord = -1.0f + 10.0f / width_;
    text_renderer_.renderTextFromLeftCenter(interaction_string, x_coord, y_coord, 0.0003f, width_, height_);
}

void AxesRenderer::renderViewAngles()
{
    // return;  // TODO: For demo, rendering view angles is not used
    text_renderer_.setColor(0.0f, 0.0f, 0.0f);

    const int az = static_cast<int>(view_angles_.getSnappedAzimuth() * 180.0 / M_PI);
    const int el = static_cast<int>(view_angles_.getSnappedElevation() * 180.0 / M_PI);

    const std::string view_angle_str = "[ " + std::to_string(az) + ", " + std::to_string(el) + " ]";
    const float y_coord = -1.0f + 20.0f / height_;
    const float x_coord = 1.0f - 110.0f / width_;
    text_renderer_.renderTextFromLeftCenter(view_angle_str, x_coord, y_coord, 0.0003f, width_, height_);
}

void AxesRenderer::render()
{
    renderHandle();
    if (has_title_)
    {
        renderTitle();
    }
    renderInteractionLetter();
    if (mouse_pressed_)
    {
        renderViewAngles();
    }
    // shader_collection_.pane_background_shader.use();
    // renderBackground();
    shader_collection_.plot_box_shader.use();
    if (plot_pane_settings_.plot_box_on)
    {
        renderPlotBox();
    }
    if (render_zoom_rect_)
    {
        model_mat_[3][0] = 0.0;
        model_mat_[3][1] = 0.0;
        model_mat_[3][2] = 0.0;

        const glm::mat4 mvp = projection_mat_ * view_mat_ * model_mat_ * window_scale_mat_;

        shader_collection_.plot_box_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

        zoom_rect_.render(mouse_pos_at_press_,
                          current_mouse_pos_,
                          view_angles_.getSnappingAxis(),
                          view_mat_,
                          model_mat_ * window_scale_mat_,
                          projection_mat_);
    }

    if (plot_pane_settings_.grid_on)
    {
        renderBoxGrid();
    }
    if (plot_pane_settings_.plot_box_on)
    {
        // Render the silhouette after the box grid because of reasons...

        scale_mat_[0][0] = 1.0;
        scale_mat_[1][1] = 1.0;
        scale_mat_[2][2] = 1.0;
        scale_mat_[3][3] = 1.0;
        const glm::mat4 mvp = projection_mat_ * view_mat_ * model_mat_ * scale_mat_ * window_scale_mat_;

        shader_collection_.plot_box_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);
        shader_collection_.plot_box_shader.base_uniform_handles.vertex_color.setColor({0.0f, 0.0f, 0.0f});
        plot_box_silhouette_.render(
            axes_side_configuration_, view_angles_.getSnappedAzimuth(), view_angles_.getSnappedElevation());
    }

    drawGridNumbers(text_renderer_,
                    axes_limits_,
                    view_angles_,
                    plot_pane_settings_,
                    view_mat_,
                    model_mat_ * window_scale_mat_,
                    projection_mat_,
                    width_,
                    height_,
                    grid_vectors_,
                    axes_side_configuration_,
                    use_perspective_proj_);
    text_renderer_.flush();
}

void AxesRenderer::renderLegend()
{
    const glm::mat4 mvp = orth_projection_mat_ * view_mat_;

    shader_collection_.legend_shader.use();
    shader_collection_.legend_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    shader_collection_.plot_box_shader.use();
    shader_collection_.plot_box_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);
    legend_renderer_.render(legend_properties_, width_, height_);
}

void AxesRenderer::renderBoxGrid()
{
    const Vec3d scale = axes_limits_.getAxesScale() / 2.0;

    scale_mat_[0][0] = 1.0 / scale.x;
    scale_mat_[1][1] = 1.0 / scale.y;
    scale_mat_[2][2] = 1.0 / scale.z;
    scale_mat_[3][3] = 1.0;

    model_mat_[3][0] = 0.0;
    model_mat_[3][1] = 0.0;
    model_mat_[3][2] = 0.0;

    const glm::mat4 mvp = projection_mat_ * view_mat_ * model_mat_ * scale_mat_ * window_scale_mat_;

    shader_collection_.plot_box_shader.base_uniform_handles.vertex_color.setColor(plot_pane_settings_.grid_color);
    shader_collection_.plot_box_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    plot_box_grid_.render(grid_vectors_, axes_limits_, view_angles_, axes_side_configuration_);
}

void AxesRenderer::renderPointSelection(const Point3d& closest_point)
{
    const glm::mat4 mvp = orth_projection_mat_ * view_mat_;
    shader_collection_.plot_box_shader.use();
    shader_collection_.plot_box_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    shader_collection_.plot_box_shader.base_uniform_handles.vertex_color.setColor({0.0f, 0.0f, 0.0f});

    const Vec3d axes_center = axes_limits_.getAxesCenter();

    glm::mat4 t_mat = glm::translate(glm::mat4(1.0f), glm::vec3(-axes_center.x, -axes_center.y, -axes_center.z));

    const Vec3d scale = axes_limits_.getAxesScale() / 2.0;

    glm::mat4 local_scale_mat = glm::mat4(1.0f);

    local_scale_mat[0][0] = 1.0 / scale.x;
    local_scale_mat[1][1] = 1.0 / scale.y;
    local_scale_mat[2][2] = 1.0 / scale.z;
    local_scale_mat[3][3] = 1.0;

    model_mat_[3][0] = 0.0;
    model_mat_[3][1] = 0.0;
    model_mat_[3][2] = 0.0;

    const glm::mat4 local_mvp = view_mat_ * model_mat_ * local_scale_mat * window_scale_mat_ * t_mat;

    const glm::vec4 v_viewport = glm::vec4(0.0, 0.0, width_, height_);

    const glm::vec3 data_point(closest_point.x, closest_point.y, closest_point.z);

    const glm::vec3 projected_point = glm::project(data_point, local_mvp, projection_mat_, v_viewport);

    const Vec2d res_point(((projected_point.x / width_) - 0.5) * 6.0, ((projected_point.y / height_) - 0.5) * 6.0);

    const bool draw_up = current_mouse_pos_.y < 0.5;
    point_selection_box_.render(res_point, draw_up);
}

void AxesRenderer::plotBegin()
{
    glEnable(GL_BLEND);

    const Vec3d axes_center = axes_limits_.getAxesCenter();

    const Vec3d scale = axes_limits_.getAxesScale() / 2.0;

    glm::mat4 t_mat = glm::translate(glm::mat4(1.0f), glm::vec3(-axes_center.x, -axes_center.y, -axes_center.z));

    scale_mat_[0][0] = 1.0 / scale.x;
    scale_mat_[1][1] = 1.0 / scale.y;
    scale_mat_[2][2] = 1.0 / scale.z;
    scale_mat_[3][3] = 1.0;

    const glm::mat4 mvp = projection_mat_ * view_mat_ * model_mat_ * scale_mat_ * window_scale_mat_ * t_mat;
    const glm::mat4 i_mvp = glm::inverse(mvp);

    shader_collection_.basic_plot_shader.use();
    shader_collection_.basic_plot_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    shader_collection_.draw_mesh_shader.use();
    shader_collection_.draw_mesh_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    shader_collection_.draw_mesh_shader.uniform_handles.rotation_mat.setMat4x4(model_mat_);
    shader_collection_.draw_mesh_shader.uniform_handles.global_illumination_active.setInt(global_illumination_active_);
    shader_collection_.draw_mesh_shader.uniform_handles.light_pos.setVec(light_pos_);

    shader_collection_.scatter_shader.use();
    shader_collection_.scatter_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    shader_collection_.plot_2d_shader.use();
    shader_collection_.plot_2d_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    shader_collection_.plot_2d_shader.uniform_handles.inverse_model_view_proj_mat.setMat4x4(i_mvp);
    shader_collection_.plot_2d_shader.base_uniform_handles.axes_width.setFloat(width_);
    shader_collection_.plot_2d_shader.base_uniform_handles.axes_height.setFloat(height_);

    shader_collection_.plot_3d_shader.use();

    shader_collection_.plot_3d_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);
    shader_collection_.plot_3d_shader.uniform_handles.inverse_model_view_proj_mat.setMat4x4(i_mvp);
    shader_collection_.plot_3d_shader.base_uniform_handles.axes_width.setFloat(width_);
    shader_collection_.plot_3d_shader.base_uniform_handles.axes_height.setFloat(height_);

    shader_collection_.basic_plot_shader.use();
    enableClipPlanes();

    shader_collection_.img_plot_shader.use();
    shader_collection_.img_plot_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    shader_collection_.basic_plot_shader.use();
}

void AxesRenderer::plotEnd()
{
    glDisable(GL_BLEND);

    if (render_legend_)
    {
        renderLegend();
    }
    renderBackground();
}

void AxesRenderer::activateGlobalIllumination(const Vec3d& light_pos)
{
    global_illumination_active_ = true;
    light_pos_ = light_pos;
}

void AxesRenderer::resetGlobalIllumination()
{
    global_illumination_active_ = false;
}

void AxesRenderer::renderBackground()
{
    shader_collection_.pane_background_shader.use();
    shader_collection_.pane_background_shader.base_uniform_handles.axes_width.setFloat(width_);
    shader_collection_.pane_background_shader.base_uniform_handles.axes_height.setFloat(height_);
    shader_collection_.pane_background_shader.base_uniform_handles.radius.setFloat(plot_pane_settings_.pane_radius);

    model_mat_[3][0] = 0.0;
    model_mat_[3][1] = 0.0;
    model_mat_[3][2] = 0.0;

    scale_mat_[0][0] = 1.0;
    scale_mat_[1][1] = 1.0;
    scale_mat_[2][2] = 1.0;
    scale_mat_[3][3] = 1.0;

    const glm::mat4 rotation_mat = glm::rotate(static_cast<float>(M_PI / 2.0), glm::vec3(1.0, 0.0, 0.0));
    const glm::mat4 mvp = orth_projection_mat_ * view_mat_ * rotation_mat;

    shader_collection_.pane_background_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);

    shader_collection_.pane_background_shader.base_uniform_handles.vertex_color.setColor(tab_background_color_);
    plot_pane_background_.render(width_, height_);
}

void AxesRenderer::renderPlotBox()
{
    model_mat_[3][0] = 0.0;
    model_mat_[3][1] = 0.0;
    model_mat_[3][2] = 0.0;

    scale_mat_[0][0] = 1.0;
    scale_mat_[1][1] = 1.0;
    scale_mat_[2][2] = 1.0;
    scale_mat_[3][3] = 1.0;

    const glm::mat4 mvp = projection_mat_ * view_mat_ * model_mat_ * window_scale_mat_;

    shader_collection_.plot_box_shader.base_uniform_handles.model_view_proj_mat.setMat4x4(mvp);
    shader_collection_.plot_box_shader.base_uniform_handles.vertex_color.setColor(plot_pane_settings_.plot_box_color);

    plot_box_walls_.render(axes_side_configuration_);
}

void AxesRenderer::setAxesBoxScaleFactor(const Vec3d& scale_vector)
{
    scale_vector_ = scale_vector;
}

Vec3d AxesRenderer::getAxesBoxScaleFactor() const
{
    return scale_vector_;
}

void AxesRenderer::setScaleOnRotation(const bool scale_on_rotation)
{
    scale_on_rotation_ = scale_on_rotation;
}

void AxesRenderer::setTitle(const std::string& title)
{
    if (title == "")
    {
        has_title_ = false;
    }
    else
    {
        has_title_ = true;
        title_ = title;
    }
}

void AxesRenderer::setAxesSquare(const bool axes_square)
{
    axes_square_ = axes_square;
}

void AxesRenderer::updateStates(const AxesLimits& axes_limits,
                                const ViewAngles& view_angles,
                                const QueryPoint& query_point,
                                const GridVectors& gv,
                                const AxesSideConfiguration& axes_side_configuration,
                                const bool use_perspective_proj,
                                const float width,
                                const float height,
                                const Vec2f mouse_pos_at_press,
                                const Vec2f current_mouse_pos,
                                const MouseInteractionType mouse_interaction_type,
                                const bool mouse_pressed,
                                const bool render_zoom_rect,
                                const bool render_legend,
                                const float legend_scale_factor,
                                const std::vector<LegendProperties>& legend_properties,
                                const std::string& name,
                                const MouseInteractionAxis current_mouse_interaction_axis)
{
    axes_limits_ = axes_limits;
    view_angles_ = view_angles;
    grid_vectors_ = gv;
    use_perspective_proj_ = use_perspective_proj;
    width_ = width;
    height_ = height;
    mouse_pos_at_press_ = mouse_pos_at_press;
    current_mouse_pos_ = current_mouse_pos;
    mouse_interaction_type_ = mouse_interaction_type;
    mouse_pressed_ = mouse_pressed;
    render_zoom_rect_ = render_zoom_rect;
    render_legend_ = render_legend;
    legend_properties_ = legend_properties;
    legend_renderer_.setLegendScaleFactor(legend_scale_factor);
    name_ = name;
    current_mouse_interaction_axis_ = current_mouse_interaction_axis;
    axes_side_configuration_ = axes_side_configuration;

    const Matrix<double> rot_mat = rotationMatrixZ(-view_angles_.getSnappedAzimuth()) * rotationMatrixX(-view_angles_.getSnappedElevation());

    for (int r = 0; r < 3; r++)
    {
        for (int c = 0; c < 3; c++)
        {
            model_mat_[r][c] = rot_mat(r, c);
        }
    }

    if (axes_square_)
    {
        float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);
        const float s = 3.0f;

        float left = -aspect_ratio * s;
        float right = aspect_ratio * s;

        float bottom = -1.0f * s;
        float top = 1.0f * s;

        float fov = glm::radians(75.0f);

        if (aspect_ratio < 1.0)
        {
            left = -1.0f * s;
            right = 1.0f * s;

            bottom = -1.0f / aspect_ratio * s;
            top = 1.0f / aspect_ratio * s;

            fov = 2.0f * std::atan(std::tan(fov / 2.0f) * (1.0f / aspect_ratio));
        }

        orth_projection_mat_ = glm::ortho(left, right, bottom, top, 0.1f, 100.0f);
        persp_projection_mat_ = glm::perspective(fov, aspect_ratio, 0.1f, 100.0f);
    }

    projection_mat_ = use_perspective_proj_ ? persp_projection_mat_ : orth_projection_mat_;

    float s = 0.2f;

    if (scale_on_rotation_ && (!use_perspective_proj_))
    {
        const float az = std::pow(std::fabs(std::sin(view_angles_.getSnappedAzimuth() * 2.0f)), 0.6) * 0.7;
        const float el = std::pow(std::fabs(std::sin(view_angles_.getSnappedElevation() * 2.0f)), 0.7) * 0.5;
        s = std::sqrt(az * az + el * el);
    }

    window_scale_mat_ = glm::mat4(1.0f);

    window_scale_mat_[0][0] = scale_vector_.x - s;
    window_scale_mat_[1][1] = scale_vector_.y - s;
    window_scale_mat_[2][2] = scale_vector_.z - s;

    if (query_point.has_query_point)
    {
        const Vec3d axes_center = axes_limits_.getAxesCenter();

        glm::mat4 t_mat = glm::translate(glm::mat4(1.0f), glm::vec3(-axes_center.x, -axes_center.y, -axes_center.z));

        const Vec3d scale = axes_limits_.getAxesScale() / 2.0;

        glm::mat4 local_scale_mat = glm::mat4(1.0f);

        local_scale_mat[0][0] = 1.0 / scale.x;
        local_scale_mat[1][1] = 1.0 / scale.y;
        local_scale_mat[2][2] = 1.0 / scale.z;
        local_scale_mat[3][3] = 1.0;

        model_mat_[3][0] = 0.0;
        model_mat_[3][1] = 0.0;
        model_mat_[3][2] = 0.0;

        const glm::mat4 local_mvp = view_mat_ * model_mat_ * local_scale_mat * window_scale_mat_ * t_mat;

        const glm::vec4 v_viewport = glm::vec4(0.0, 0.0, width_, height_);

        const double sceen_y = (height_ - 1.0) - query_point.query_point_screen_y;

        const glm::vec3 v_screen(query_point.query_point_screen_x, sceen_y, 0.0);
        const glm::vec3 unprojected_point = glm::unProject(v_screen, local_mvp, projection_mat_, v_viewport);

        const glm::vec3 v_in_screen(query_point.query_point_screen_x, sceen_y, -0.1);
        const glm::vec3 v_in_screen_unprojected = glm::unProject(v_in_screen, local_mvp, projection_mat_, v_viewport);

        line_ = Line3D<double>::fromTwoPoints(
            Vec3d{unprojected_point.x, unprojected_point.y, unprojected_point.z},
            Vec3d{v_in_screen_unprojected.x, v_in_screen_unprojected.y, v_in_screen_unprojected.z});
    }
}
