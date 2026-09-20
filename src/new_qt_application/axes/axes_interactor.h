#ifndef MAIN_APPLICATION_AXES_AXES_INTERACTOR_H_
#define MAIN_APPLICATION_AXES_AXES_INTERACTOR_H_

#include <cmath>
#include <utility>
#include <vector>

#include "axes/structures/axes_limits.h"
#include "axes/structures/axes_settings.h"
#include "axes/structures/grid_vectors.h"
#include "axes/structures/view_angles.h"
#include "lumos/math.h"

using namespace lumos;

double findFirstPointInInterval(const double min_point,
                                const double max_point,
                                const double start_point,
                                const double inc);
double findFirstPointBeforeMin(const double min_point, const double start_point, const double inc);

enum class MouseInteractionAxis
{
    X,
    Y,
    Z,
    XY,
    YZ,
    XZ,
    ALL
};

struct QueryPoint
{
    bool has_query_point;
    double query_point_screen_x;
    double query_point_screen_y;
};

class AxesInteractor
{
private:
    // Variables
    const double rotation_mouse_gain = 0.01;
    const double pan_mouse_gain = 0.005;
    const double zoom_mouse_gain = 0.005;

    double current_window_width;
    double current_window_height;

    MouseInteractionType current_mouse_interaction_type_;
    MouseInteractionType overridden_mouse_interaction_type_;
    bool mouse_pressed_;
    bool should_draw_zoom_rect_;
    bool is_image_view_;

    ViewAngles view_angles_;
    ViewAngles default_view_angles_;

    AxesLimits axes_limits_;
    AxesLimits default_axes_limits_;
    Vec2f mouse_pos_at_press_;

    AxesSettings axes_settings_;
    bool show_legend_;
    bool has_query_points_;
    double query_point_screen_x_;
    double query_point_screen_y_;

    Vec3d inc0;

    // Functions
    void changePan(const double dx, const double dy, const MouseInteractionAxis mia);
    void changeZoom(const double dy, const MouseInteractionAxis mia);
    void changeRotation(const double dx, const double dy, const MouseInteractionAxis mia);

public:
    QueryPoint getQueryPoint() const;
    AxesInteractor(const AxesSettings& axes_settings, const int window_height, const int window_width);

    void updateWindowSize(const int window_width, const int window_height);
    void setOverriddenMouseInteractionType(const MouseInteractionType overridden_mouse_interaction_type);
    void setMouseInteractionType(const MouseInteractionType interaction_type);
    void resetView();
    void setViewAngles(const double azimuth, const double elevation);
    void setAxesLimits(const Vec3d& min_vec, const Vec3d& max_vec);
    void setAxesLimits(const Vec2d& min_vec, const Vec2d& max_vec);

    void setViewAnglesSnapAngle(const double snap_angle);
    bool shouldDrawZoomRect() const
    {
        return should_draw_zoom_rect_;
    }

    void setIsImageView(const bool val)
    {
        is_image_view_ = val;

        if (is_image_view_)
        {
            view_angles_.setAngles(0.0, -90.0 * M_PI / 180.0);
        }
    }

    void registerMouseDragInput(const MouseInteractionAxis current_mouse_interaction_axis,
                                const int x,
                                const int y,
                                const int dx,
                                const int dy);
    void registerMousePressed(const Vec2f& mouse_pos);
    void registerMouseReleased(const Vec2f& mouse_pos);

    MouseInteractionType getMouseInteractionType() const
    {
        return current_mouse_interaction_type_;
    }

    bool getShowLegend() const
    {
        return show_legend_;
    }

    GridVectors generateGridVectors();
    ViewAngles getViewAngles() const;
    AxesLimits getAxesLimits() const;
    void showLegend(const bool show_legend);
};

#endif  // MAIN_APPLICATION_AXES_AXES_INTERACTOR_H_
