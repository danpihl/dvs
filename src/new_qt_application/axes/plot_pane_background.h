#ifndef MAIN_APPLICATION_AXES_PLOT_PANE_BACKGROUND_H_
#define MAIN_APPLICATION_AXES_PLOT_PANE_BACKGROUND_H_

#include <stddef.h>

#include "opengl_low_level/opengl_header.h"

class PlotPaneBackground
{
private:
    size_t num_vertices_;
    size_t num_corner_segments_;
    size_t num_corner_vertices_;
    size_t num_bytes_;
    float* data_array_;
    float radius_;

    GLuint vertex_buffer_, vertex_buffer_array_;

public:
    PlotPaneBackground(const float radius);
    ~PlotPaneBackground();

    void render(const float pane_width, const float pane_height);
};

#endif  // MAIN_APPLICATION_AXES_PLOT_PANE_BACKGROUND_H_
