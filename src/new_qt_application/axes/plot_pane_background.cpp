#include "plot_pane_background.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

void PlotPaneBackground::render(const float pane_width, const float pane_height)
{
    const float rx = 3.0f * radius_ / pane_width;
    const float ry = 3.0f * radius_ / pane_height;

    std::vector<float> offsets_x = {-1.0f, 1.0f, 1.0f, -1.0f};
    std::vector<float> offsets_y = {1.0f, 1.0f, -1.0f, -1.0f};

    size_t idx = 0U;

    const float z_val = 3.0f;

    for (size_t k = 0; k < 4; k++)
    {
        const float ox = offsets_x[k] * 3.0f - offsets_x[k] * rx;
        const float oy = offsets_y[k] * 3.0f - offsets_y[k] * ry;

        data_array_[idx + 0] = -rx + ox;
        data_array_[idx + 1] = -ry + oy;
        data_array_[idx + 2] = z_val;

        data_array_[idx + 3] = rx + ox;
        data_array_[idx + 4] = -ry + oy;
        data_array_[idx + 5] = z_val;

        data_array_[idx + 6] = -rx + ox;
        data_array_[idx + 7] = ry + oy;
        data_array_[idx + 8] = z_val;

        data_array_[idx + 9] = -rx + ox;
        data_array_[idx + 10] = ry + oy;
        data_array_[idx + 11] = z_val;

        data_array_[idx + 12] = rx + ox;
        data_array_[idx + 13] = ry + oy;
        data_array_[idx + 14] = z_val;

        data_array_[idx + 15] = rx + ox;
        data_array_[idx + 16] = -ry + oy;
        data_array_[idx + 17] = z_val;

        idx += 18U;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_bytes_, data_array_);
    glBindVertexArray(vertex_buffer_array_);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices_);
    glBindVertexArray(0);
}

PlotPaneBackground::PlotPaneBackground(const float radius)
{
    radius_ = radius;
    const size_t num_rectangles = 4U;
    const size_t num_triangles = num_rectangles * 2U;
    num_vertices_ = num_triangles * 3U;
    num_bytes_ = sizeof(float) * num_vertices_ * 3U;
    data_array_ = new float[num_vertices_ * 3U];

    std::memset(data_array_, 0, num_bytes_);

    glGenVertexArrays(1, &vertex_buffer_array_);
    glBindVertexArray(vertex_buffer_array_);

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, num_bytes_, data_array_, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

PlotPaneBackground::~PlotPaneBackground()
{
    delete[] data_array_;
}
