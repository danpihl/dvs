#include "plot_box_walls.h"

#include <iostream>

// The value '0.0f' corresponds to the dimension that will be changed
float walls_vertices[] = {
    -1.0f, -1.0f, 0.0f,  1.0f,  -1.0f, 0.0f, -1.0f, 1.0f, 0.0f,

    1.0f,  1.0f,  0.0f,  1.0f,  -1.0f, 0.0f, -1.0f, 1.0f, 0.0f,

    0.0f,  -1.0f, -1.0f, 0.0f,  -1.0f, 1.0f, 0.0f,  1.0f, -1.0f,

    0.0f,  1.0f,  1.0f,  0.0f,  -1.0f, 1.0f, 0.0f,  1.0f, -1.0f,

    -1.0f, 0.0f,  -1.0f, -1.0f, 0.0f,  1.0f, 1.0f,  0.0f, -1.0f,

    1.0f,  0.0f,  1.0f,  -1.0f, 0.0f,  1.0f, 1.0f,  0.0f, -1.0f,
};

void PlotBoxWalls::setIndices(const size_t first_vertex_idx,
                              const size_t last_vertex_idx,
                              const size_t dimension_idx,
                              const float val)
{
    for (size_t k = first_vertex_idx; k < last_vertex_idx; k++)
    {
        data_array_[k * 3 + dimension_idx] = val;
    }
}

void PlotBoxWalls::render(const AxesSideConfiguration axes_side_configuration)
{
    setIndices(kXYFirstIdx, kXYLastIdx, kXYChangeDimension, axes_side_configuration.xy_plane_z_value);

    setIndices(kYZFirstIdx, kYZLastIdx, kYZChangeDimension, axes_side_configuration.yz_plane_x_value);

    setIndices(kXZFirstIdx, kXZLastIdx, kXZChangeDimension, axes_side_configuration.xz_plane_y_value);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_bytes_, data_array_);
    glBindVertexArray(vertex_buffer_array_);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices_);
    glBindVertexArray(0);
}

PlotBoxWalls::PlotBoxWalls()
    : num_vertices_{18U}, num_bytes_{sizeof(float) * num_vertices_ * 3U}, data_array_{new float[num_vertices_ * 3]}
{
    std::memcpy(data_array_, walls_vertices, num_bytes_);

    glGenVertexArrays(1, &vertex_buffer_array_);
    glBindVertexArray(vertex_buffer_array_);

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, num_bytes_, data_array_, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

PlotBoxWalls::~PlotBoxWalls()
{
    delete[] data_array_;
}
