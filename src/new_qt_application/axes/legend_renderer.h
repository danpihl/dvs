#ifndef MAIN_APPLICATION_AXES_LEGEND_RENDERER_H_
#define MAIN_APPLICATION_AXES_LEGEND_RENDERER_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "axes/legend_properties.h"
#include "axes/text_rendering.h"
#include "lumos/math.h"
#include "opengl_low_level/opengl_header.h"
#include "opengl_low_level/vertex_buffer.h"
#include "shader.h"

class LegendRenderer
{
private:
    size_t num_vertices_edge_;
    size_t num_vertices_inner_;
    float scale_factor_;
    VertexBuffer edge_vao_;
    VertexBuffer inner_vao_;
    VertexBuffer legend_shape_;

    TextRenderer text_renderer_;
    ShaderCollection shader_collection_;

    lumos::Vector<float> points_;
    lumos::Vector<float> colors_;

    lumos::Vector<float> legend_inner_vertices_;
    lumos::Vector<float> legend_edge_vertices_;
    void renderColorMapLegend(const size_t num_segments,
                              const RGBTripletf& edge_color,
                              const float xc,
                              const float yc,
                              const float r,
                              const float axes_width,
                              const float axes_height);
    void setVertexAtIdx(const float x, const float y, const float z, const size_t idx);
    void setColorAtIdx(const float r, const float g, const float b, const size_t idx);
    void setBoxValues(const float new_x_min, const float new_x_max, const float new_z_min, const float new_z_max);

public:
    LegendRenderer(const TextRenderer& text_renderer_, const ShaderCollection& shader_collection_);

    void render(const std::vector<LegendProperties>& legend_properties,
                const float axes_width,
                const float axes_height);
    void setLegendScaleFactor(const float new_scale_factor);
};

#endif  // MAIN_APPLICATION_AXES_LEGEND_RENDERER_H_
