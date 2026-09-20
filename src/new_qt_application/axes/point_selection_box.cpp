#include "axes/point_selection_box.h"

constexpr size_t kNumVertices{6U};

void PointSelectionBox::render(const Point2d& closest_point, const bool draw_up)
{
    const float x_len{0.2f};
    const float z_len{draw_up ? -0.2f : 0.2f};

    // First triangle
    pane_vertices_(0) = 0.0 + closest_point.x;
    pane_vertices_(2) = 0.0 + closest_point.y;

    pane_vertices_(3) = x_len + closest_point.x;
    pane_vertices_(5) = 0.0 + closest_point.y;

    pane_vertices_(6) = x_len + closest_point.x;
    pane_vertices_(8) = z_len + closest_point.y;

    // Second triangle
    pane_vertices_(9) = 0.0 + closest_point.x;
    pane_vertices_(11) = 0.0 + closest_point.y;

    pane_vertices_(12) = 0.0 + closest_point.x;
    pane_vertices_(14) = z_len + closest_point.y;

    pane_vertices_(15) = x_len + closest_point.x;
    pane_vertices_(17) = z_len + closest_point.y;

    pane_vao_.updateBufferData(0, pane_vertices_.data(), kNumVertices, 3);
    pane_vao_.render(kNumVertices);
}

PointSelectionBox::PointSelectionBox(const TextRenderer& text_renderer, const ShaderCollection& shader_collection)
    : text_renderer_{text_renderer},
      shader_collection_{shader_collection},
      pane_vao_{OGLPrimitiveType::TRIANGLES},
      pane_vertices_{kNumVertices * 3U}
{
    shader_collection.plot_box_shader.use();

    pane_vertices_.fill(-5.0f);

    const float x_len{0.1f};
    const float z_len{0.1f};

    // First triangle
    pane_vertices_(0) = -x_len;
    pane_vertices_(2) = -z_len;

    pane_vertices_(3) = x_len;
    pane_vertices_(5) = -z_len;

    pane_vertices_(6) = x_len;
    pane_vertices_(8) = z_len;

    // Second triangle
    pane_vertices_(9) = -x_len;
    pane_vertices_(11) = -z_len;

    pane_vertices_(12) = -x_len;
    pane_vertices_(14) = z_len;

    pane_vertices_(15) = x_len;
    pane_vertices_(17) = z_len;

    pane_vao_.addBuffer(pane_vertices_.data(), kNumVertices, 3, GL_DYNAMIC_DRAW);
}

PointSelectionBox::~PointSelectionBox() {}
