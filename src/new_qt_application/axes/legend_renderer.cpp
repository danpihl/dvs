#include "axes/legend_renderer.h"

#include <iostream>

#include "misc/rgb_triplet.h"

constexpr int kMaxNumPoints{100};
constexpr float kYOffset{-5.1f};

void LegendRenderer::setBoxValues(const float new_x_min,
                                  const float new_x_max,
                                  const float new_z_min,
                                  const float new_z_max)
{
    legend_inner_vertices_(0) = new_x_min;
    legend_inner_vertices_(2) = new_z_min;
    legend_inner_vertices_(3) = new_x_max;
    legend_inner_vertices_(5) = new_z_min;
    legend_inner_vertices_(6) = new_x_max;
    legend_inner_vertices_(8) = new_z_max;

    legend_inner_vertices_(9) = new_x_min;
    legend_inner_vertices_(11) = new_z_min;
    legend_inner_vertices_(12) = new_x_min;
    legend_inner_vertices_(14) = new_z_max;
    legend_inner_vertices_(15) = new_x_max;
    legend_inner_vertices_(17) = new_z_max;

    legend_edge_vertices_(0) = new_x_min;
    legend_edge_vertices_(2) = new_z_min;
    legend_edge_vertices_(3) = new_x_max;
    legend_edge_vertices_(5) = new_z_min;
    legend_edge_vertices_(6) = new_x_max;
    legend_edge_vertices_(8) = new_z_max;
    legend_edge_vertices_(9) = new_x_min;
    legend_edge_vertices_(11) = new_z_max;
    legend_edge_vertices_(12) = new_x_min;
    legend_edge_vertices_(14) = new_z_min;
}

void LegendRenderer::setVertexAtIdx(const float x, const float y, const float z, const size_t idx)
{
    points_.data()[idx] = x;
    points_.data()[idx + 1] = y;
    points_.data()[idx + 2] = z;
}

void LegendRenderer::setColorAtIdx(const float r, const float g, const float b, const size_t idx)
{
    colors_.data()[idx] = r;
    colors_.data()[idx + 1] = g;
    colors_.data()[idx + 2] = b;
}

void LegendRenderer::renderColorMapLegend(const size_t num_segments,
                                          const RGBTripletf& edge_color,
                                          const float xc,
                                          const float yc,
                                          const float r,
                                          const float axes_width,
                                          const float axes_height)
{
    const float delta_phi = static_cast<float>(M_PI) * 2.0f / static_cast<float>(num_segments);
    float angle = 0.0f;
    const float mul = 400.0f;  // Empirically found
    int idx = 0;

    for (size_t k = 0; k < num_segments; k++)
    {
        const float kf = k;
        const float a_val = kf / static_cast<float>(num_segments - 1U);

        setVertexAtIdx(xc, kYOffset, yc, idx);
        setColorAtIdx(a_val, 0.0f, 0.0f, idx);
        idx += 3;

        setVertexAtIdx(r * cos(angle) * mul / axes_width + xc, kYOffset, r * sin(angle) * mul / axes_height + yc, idx);
        setColorAtIdx(a_val, 0.0f, 0.0f, idx);
        idx += 3;
        angle += delta_phi;

        setVertexAtIdx(r * cos(angle) * mul / axes_width + xc, kYOffset, r * sin(angle) * mul / axes_height + yc, idx);
        setColorAtIdx(a_val, 0.0f, 0.0f, idx);
        idx += 3;
    }

    angle = 0.0f;
    for (size_t k = 0; k < num_segments; k++)
    {
        setVertexAtIdx(xc, kYOffset, yc, idx);
        setColorAtIdx(edge_color.red, edge_color.green, edge_color.blue, idx);
        idx += 3;

        setVertexAtIdx(r * cos(angle) * mul / axes_width + xc, kYOffset, r * sin(angle) * mul / axes_height + yc, idx);
        setColorAtIdx(edge_color.red, edge_color.green, edge_color.blue, idx);
        idx += 3;
        angle += delta_phi;

        setVertexAtIdx(r * cos(angle) * mul / axes_width + xc, kYOffset, r * sin(angle) * mul / axes_height + yc, idx);
        setColorAtIdx(edge_color.red, edge_color.green, edge_color.blue, idx);
        idx += 3;

        setVertexAtIdx(xc, kYOffset, yc, idx);
        setColorAtIdx(edge_color.red, edge_color.green, edge_color.blue, idx);
        idx += 3;
    }
}

void LegendRenderer::setLegendScaleFactor(const float new_scale_factor)
{
    scale_factor_ = new_scale_factor > 0.1f ? new_scale_factor : 1.0f;
}

void LegendRenderer::render(const std::vector<LegendProperties>& legend_properties,
                            const float axes_width,
                            const float axes_height)
{
    float max_width = 0.0f;
    for (size_t k = 0; k < legend_properties.size(); k++)
    {
        const lumos::Vec2f sz = text_renderer_.calculateStringSize(legend_properties[k].label, 0.0005f, axes_width, axes_height);
        const float current_width = sz.x;
        max_width = std::max(max_width, current_width);
    }

    const float text_x_offset = 150.0f / axes_width;  // 150.0f is "empirically" found
    const float text_x_margin = 150.0f / axes_width;
    const float z_offset = 100.0f / axes_height;  // 100.0f is "empirically" found
    const float x_offset = 100.0f / axes_width;
    const float legend_element_z_delta = 140.0f / axes_height;
    // Multiply by 3.0f because the text coordinates has a scale of 1/3 relative to the box coords.
    const float box_width = max_width * 3.0f + text_x_offset + text_x_margin;

    const float x_max = 3.0f - x_offset;  // * scale_factor_;
    const float x_min = x_max - box_width * scale_factor_;

    const float z_max = 3.0f - z_offset;  //  * scale_factor_;
    // std::max in case legend_properties.size() == 0
    const float z_min =
        z_max -
        (2.0f * z_offset + legend_element_z_delta * (std::max(legend_properties.size(), 1UL) - 1)) * scale_factor_;

    setBoxValues(x_min, x_max, z_min, z_max);

    shader_collection_.plot_box_shader.use();

    shader_collection_.plot_box_shader.base_uniform_handles.vertex_color.setColor({0.0f, 0.0f, 0.0f});

    edge_vao_.updateBufferData(0, legend_edge_vertices_.data(), num_vertices_edge_, 3);
    edge_vao_.render(num_vertices_edge_);

    shader_collection_.plot_box_shader.base_uniform_handles.vertex_color.setColor(
        {253.0f / 255.0f, 246.0f / 255.0f, 228.0f / 255.0f});
    inner_vao_.updateBufferData(0, legend_inner_vertices_.data(), num_vertices_inner_, 3);
    inner_vao_.render(num_vertices_inner_);

    shader_collection_.legend_shader.use();

    float* const legend_shape_vertices = points_.data();
    float* const legend_shape_colors = colors_.data();

    const float x_center = x_min + scale_factor_ * text_x_offset / 2.0f;
    const float legend_symbol_width = 70.0f / axes_width;
    const float legend_symbol_height = 70.0f / axes_height;

    for (size_t k = 0; k < legend_properties.size(); k++)
    {
        const float kf = k;
        const float z_center = z_max - scale_factor_ * (z_offset + legend_element_z_delta * kf);

        shader_collection_.legend_shader.base_uniform_handles.scatter_mode.setInt(-1);
        shader_collection_.legend_shader.base_uniform_handles.color_map_selection.setInt(0);
        if (legend_properties[k].type == LegendType::LINE)
        {
            const RGBTripletf col = legend_properties[k].color;
            legend_shape_vertices[0] = x_center - scale_factor_ * legend_symbol_width / 2.0f;
            legend_shape_vertices[1] = kYOffset;
            legend_shape_vertices[2] = z_center;

            legend_shape_vertices[3] = x_center + scale_factor_ * legend_symbol_width / 2.0;
            legend_shape_vertices[4] = kYOffset;
            legend_shape_vertices[5] = z_center;

            legend_shape_colors[0] = col.red;
            legend_shape_colors[1] = col.green;
            legend_shape_colors[2] = col.blue;

            legend_shape_colors[3] = col.red;
            legend_shape_colors[4] = col.green;
            legend_shape_colors[5] = col.blue;

            legend_shape_.updateBufferData(0, legend_shape_vertices, 2, 3);
            legend_shape_.updateBufferData(1, legend_shape_colors, 2, 3);
            legend_shape_.render(2, OGLPrimitiveType::LINE_STRIP);
        }
        else if (legend_properties[k].type == LegendType::DOT)
        {
            shader_collection_.legend_shader.base_uniform_handles.point_size.setFloat(legend_properties[k].point_size);
            shader_collection_.legend_shader.base_uniform_handles.scatter_mode.setInt(
                static_cast<int>(legend_properties[k].scatter_style));

            const RGBTripletf col = legend_properties[k].color;
            legend_shape_vertices[0] = x_center;
            legend_shape_vertices[1] = kYOffset;
            legend_shape_vertices[2] = z_center;

            legend_shape_colors[0] = col.red;
            legend_shape_colors[1] = col.green;
            legend_shape_colors[2] = col.blue;

            legend_shape_.updateBufferData(0, legend_shape_vertices, 1, 3);
            legend_shape_.updateBufferData(1, legend_shape_colors, 1, 3);
            legend_shape_.render(1, OGLPrimitiveType::POINTS);
        }
        else if (legend_properties[k].type == LegendType::POLYGON)
        {
            if (legend_properties[k].has_color_map)
            {
                shader_collection_.legend_shader.base_uniform_handles.color_map_selection.setInt(
                    static_cast<int>(legend_properties[k].color_map) + 1);
                const size_t num_segments = 6;
                const float radius = scale_factor_ * 0.1;
                renderColorMapLegend(
                    num_segments, legend_properties[k].edge_color, x_center, z_center, radius, axes_width, axes_height);
                legend_shape_.updateBufferData(0, legend_shape_vertices, num_segments * 7, 3);
                legend_shape_.updateBufferData(1, legend_shape_colors, num_segments * 7, 3);
                legend_shape_.render(num_segments * 3, OGLPrimitiveType::TRIANGLES);

                shader_collection_.legend_shader.base_uniform_handles.color_map_selection.setInt(0);

                legend_shape_.render(num_segments * 4, num_segments * 3, OGLPrimitiveType::LINE_STRIP);
            }
            else
            {
                const RGBTripletf edge_color = legend_properties[k].edge_color;
                const RGBTripletf face_color = legend_properties[k].face_color;
                const float dxc = scale_factor_ * legend_symbol_width / 2.0;
                const float dzc = scale_factor_ * legend_symbol_height / 2.0;

                // Face 0
                legend_shape_vertices[0] = x_center - dxc;
                legend_shape_vertices[1] = kYOffset;
                legend_shape_vertices[2] = z_center - dzc;

                legend_shape_vertices[3] = x_center + dxc;
                legend_shape_vertices[4] = kYOffset;
                legend_shape_vertices[5] = z_center - dzc;

                legend_shape_vertices[6] = x_center + dxc;
                legend_shape_vertices[7] = kYOffset;
                legend_shape_vertices[8] = z_center + dzc;

                legend_shape_vertices[9] = x_center - dxc;
                legend_shape_vertices[10] = kYOffset;
                legend_shape_vertices[11] = z_center - dzc;

                legend_shape_colors[0] = face_color.red;
                legend_shape_colors[1] = face_color.green;
                legend_shape_colors[2] = face_color.blue;

                legend_shape_colors[3] = face_color.red;
                legend_shape_colors[4] = face_color.green;
                legend_shape_colors[5] = face_color.blue;

                legend_shape_colors[6] = face_color.red;
                legend_shape_colors[7] = face_color.green;
                legend_shape_colors[8] = face_color.blue;

                legend_shape_.updateBufferData(0, legend_shape_vertices, 3, 3);
                legend_shape_.updateBufferData(1, legend_shape_colors, 3, 3);
                legend_shape_.render(3, OGLPrimitiveType::TRIANGLES);

                // Face 1
                legend_shape_vertices[3] = x_center - dxc;
                legend_shape_vertices[4] = kYOffset;
                legend_shape_vertices[5] = z_center + dzc;

                legend_shape_.updateBufferData(0, legend_shape_vertices, 3, 3);
                legend_shape_.updateBufferData(1, legend_shape_colors, 3, 3);
                legend_shape_.render(3, OGLPrimitiveType::TRIANGLES);

                // Edge 0
                legend_shape_colors[0] = edge_color.red;
                legend_shape_colors[1] = edge_color.green;
                legend_shape_colors[2] = edge_color.blue;

                legend_shape_colors[3] = edge_color.red;
                legend_shape_colors[4] = edge_color.green;
                legend_shape_colors[5] = edge_color.blue;

                legend_shape_colors[6] = edge_color.red;
                legend_shape_colors[7] = edge_color.green;
                legend_shape_colors[8] = edge_color.blue;

                legend_shape_colors[9] = edge_color.red;
                legend_shape_colors[10] = edge_color.green;
                legend_shape_colors[11] = edge_color.blue;

                legend_shape_.updateBufferData(0, legend_shape_vertices, 4, 3);
                legend_shape_.updateBufferData(1, legend_shape_colors, 4, 3);
                legend_shape_.render(3, OGLPrimitiveType::LINE_STRIP);

                // Edge 1
                legend_shape_vertices[3] = x_center + dxc;
                legend_shape_vertices[4] = kYOffset;
                legend_shape_vertices[5] = z_center - dzc;

                legend_shape_.updateBufferData(0, legend_shape_vertices, 4, 3);
                legend_shape_.updateBufferData(1, legend_shape_colors, 4, 3);
                legend_shape_.render(3, OGLPrimitiveType::LINE_STRIP);
            }
        }
    }

    text_renderer_.setColor(0.0f, 0.0f, 0.0f);

    for (size_t k = 0; k < legend_properties.size(); k++)
    {
        const float kf = k;

        // Divided by three because the legend pane is in the coordinate system that spans [-3.0, 3.0]
        const float xp = (x_min + scale_factor_ * text_x_offset) / 3.0f;
        const float zp = (z_max - scale_factor_ * (z_offset + legend_element_z_delta * kf)) / 3.0f;
        text_renderer_.renderTextFromLeftCenter(
            legend_properties[k].label, xp, zp, scale_factor_ * 0.0005f, axes_width, axes_height);
    }
    text_renderer_.flush();
}

LegendRenderer::LegendRenderer(const TextRenderer& text_renderer, const ShaderCollection& shader_collection)
    : text_renderer_{text_renderer},
      shader_collection_{shader_collection},
      points_(kMaxNumPoints * 3),
      colors_(kMaxNumPoints * 3),
      legend_inner_vertices_(18),
      legend_edge_vertices_(15),
      edge_vao_{OGLPrimitiveType::LINE_STRIP},
      inner_vao_{OGLPrimitiveType::TRIANGLES},
      legend_shape_{OGLPrimitiveType::TRIANGLES}
{
    scale_factor_ = 1.0f;
    num_vertices_edge_ = 5;
    num_vertices_inner_ = 6;

    legend_inner_vertices_.fill(-5.0f);
    legend_edge_vertices_.fill(-5.0f);

    edge_vao_.addBuffer(legend_edge_vertices_.data(), num_vertices_edge_, 3, GL_DYNAMIC_DRAW);
    inner_vao_.addBuffer(legend_inner_vertices_.data(), num_vertices_inner_, 3, GL_DYNAMIC_DRAW);
    legend_shape_.addBuffer(points_.data(), kMaxNumPoints, 3, GL_DYNAMIC_DRAW);
    legend_shape_.addBuffer(colors_.data(), kMaxNumPoints, 3, GL_DYNAMIC_DRAW);
}
