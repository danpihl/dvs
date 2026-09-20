#ifndef MAIN_APPLICATION_AXES_AXES_RENDERER_H_
#define MAIN_APPLICATION_AXES_AXES_RENDERER_H_

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <vector>

#include "axes/axes_interactor.h"
#include "axes/axes_side_configuration.h"
#include "axes/grid_numbers.h"
#include "axes/legend_properties.h"
#include "axes/legend_renderer.h"
#include "axes/plot_box_grid.h"
#include "axes/plot_box_silhouette.h"
#include "axes/plot_box_walls.h"
#include "axes/plot_pane_background.h"
#include "axes/point_selection_box.h"
#include "axes/structures/axes_limits.h"
#include "axes/structures/axes_settings.h"
#include "axes/structures/grid_vectors.h"
#include "axes/structures/view_angles.h"
#include "axes/text_rendering.h"
#include "axes/zoom_rect.h"
#include "opengl_low_level/opengl_header.h"
#include "project_state/project_settings.h"
#include "shader.h"

class AxesRenderer
{
private:
    TextRenderer text_renderer_;
    ShaderCollection shader_collection_;
    LegendRenderer legend_renderer_;
    PointSelectionBox point_selection_box_;

    ViewAngles view_angles_;
    AxesLimits axes_limits_;

    AxesSettings axes_settings_;

    PlotBoxWalls plot_box_walls_{};
    PlotBoxSilhouette plot_box_silhouette_{};
    PlotBoxGrid plot_box_grid_{};
    PlotPaneSettings plot_pane_settings_;
    PlotPaneBackground plot_pane_background_;
    RGBTripletf tab_background_color_;
    AxesSideConfiguration axes_side_configuration_;

    glm::mat4 orth_projection_mat_;
    glm::mat4 persp_projection_mat_;
    glm::mat4 projection_mat_;
    glm::mat4 view_mat_;
    glm::mat4 model_mat_;
    glm::mat4 scale_mat_;
    glm::mat4 window_scale_mat_;

    float width_, height_;
    Vec3d scale_for_window_;
    bool axes_square_;
    bool scale_on_rotation_;

    GridVectors grid_vectors_;
    bool use_perspective_proj_;
    Vec2f mouse_pos_at_press_;
    Vec2f current_mouse_pos_;
    ZoomRect zoom_rect_;
    MouseInteractionType mouse_interaction_type_;
    bool mouse_pressed_;
    bool render_zoom_rect_;
    bool render_legend_;
    bool global_illumination_active_;
    Vec3d light_pos_;
    std::vector<LegendProperties> legend_properties_;
    Vec3d scale_vector_;
    Vec3d orth_scale_vector_;
    Vec3d persp_scale_vector_;
    std::string name_;
    bool has_title_;
    std::string title_;
    MouseInteractionAxis current_mouse_interaction_axis_;

    Line3D<double> line_;

    void renderBackground();
    void renderPlotBox();
    void renderBoxGrid();
    void enableClipPlanes();
    void renderLegend();
    void renderHandle();
    void renderTitle();
    void renderInteractionLetter();
    void renderViewAngles();
    void setClipPlane(const GLuint clip_plane_uniform_handle,
                      const Point3d& p0,
                      const Point3d& p1,
                      const Point3d& p2,
                      const bool invert) const;

public:
    AxesRenderer(const ShaderCollection& shader_collection,
                 const PlotPaneSettings& plot_pane_settings,
                 const RGBTripletf& tab_background_color);

    void updateStates(const AxesLimits& axes_limits,
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
                      const MouseInteractionAxis current_mouse_interaction_axis);
    void render();
    void plotBegin();
    void plotEnd();
    void renderHelpPane();
    void activateGlobalIllumination(const Vec3d& light_pos);
    void resetGlobalIllumination();
    void setTitle(const std::string& title);
    Vec3d getAxesBoxScaleFactor() const;
    void setAxesBoxScaleFactor(const Vec3d& scale_vector);
    void setScaleOnRotation(const bool scale_on_rotation);
    void setAxesSquare(const bool axes_square);
    void renderPointSelection(const Point3d& closest_point);

    Line3D<double> getLine() const
    {
        return line_;
    }
};

#endif  // MAIN_APPLICATION_AXES_AXES_RENDERER_H_
