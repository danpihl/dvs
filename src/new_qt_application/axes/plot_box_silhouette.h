#ifndef MAIN_APPLICATION_AXES_PLOT_BOX_SILHOUETTE_H_
#define MAIN_APPLICATION_AXES_PLOT_BOX_SILHOUETTE_H_

#include <stddef.h>

#include "axes/axes_side_configuration.h"
#include "opengl_low_level/opengl_header.h"

class PlotBoxSilhouette
{
private:
    const size_t num_vertices_;
    const size_t num_bytes_;
    float* const data_array_;

    GLuint vertex_buffer_, vertex_buffer_array_;

    void setIndices(const size_t first_vertex_idx,
                    const size_t last_vertex_idx,
                    const size_t dimension_idx,
                    const float val);

public:
    PlotBoxSilhouette();
    ~PlotBoxSilhouette();

    void render(const AxesSideConfiguration& axes_side_configuration, const float azimuth, const float elevation);
};

#endif  // MAIN_APPLICATION_AXES_PLOT_BOX_SILHOUETTE_H_
