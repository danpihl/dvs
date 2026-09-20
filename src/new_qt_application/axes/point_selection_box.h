#ifndef POINT_SELECTRION_BOX_H
#define POINT_SELECTRION_BOX_H

#include <stddef.h>

#include "axes/axes_side_configuration.h"
#include "axes/legend_properties.h"
#include "axes/text_rendering.h"
#include "lumos/math.h"
#include "opengl_low_level/opengl_header.h"
#include "opengl_low_level/vertex_buffer.h"
#include "shader.h"

class PointSelectionBox
{
private:
    TextRenderer text_renderer_;
    ShaderCollection shader_collection_;
    VertexBuffer pane_vao_;
    lumos::Vector<float> pane_vertices_;

public:
    PointSelectionBox(const TextRenderer& text_renderer, const ShaderCollection& shader_collection);
    ~PointSelectionBox();

    void render(const Point2d& closest_point, const bool draw_up);
};

#endif  // POINT_SELECTRION_BOX_H