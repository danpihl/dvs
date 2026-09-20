#include "plot_box_grid.h"

#include <iostream>

void PlotBoxGrid::fillXYGrid(const GridVectors& gv, const AxesSideConfiguration& axes_side_configuration)
{
    const float z_val = axes_side_configuration.xy_plane_z_value * axes_scale_.z;

    for (size_t k = 0; k < gv.x.num_valid_values; k++)
    {
        grid_points_[idx_] = gv.x.data[k];
        grid_points_[idx_ + 1] = -axes_scale_.y;
        grid_points_[idx_ + 2] = z_val;

        idx_ += 3;

        grid_points_[idx_] = gv.x.data[k];
        grid_points_[idx_ + 1] = axes_scale_.y;
        grid_points_[idx_ + 2] = z_val;

        idx_ += 3;
    }

    for (size_t k = 0; k < gv.y.num_valid_values; k++)
    {
        grid_points_[idx_] = axes_scale_.x;
        grid_points_[idx_ + 1] = gv.y.data[k];
        grid_points_[idx_ + 2] = z_val;

        idx_ += 3;

        grid_points_[idx_] = -axes_scale_.x;
        grid_points_[idx_ + 1] = gv.y.data[k];
        grid_points_[idx_ + 2] = z_val;

        idx_ += 3;
    }
}

void PlotBoxGrid::fillXZGrid(const GridVectors& gv, const AxesSideConfiguration& axes_side_configuration)
{
    const float y_val = axes_side_configuration.xz_plane_y_value * axes_scale_.y;

    for (size_t k = 0; k < gv.x.num_valid_values; k++)
    {
        grid_points_[idx_] = gv.x.data[k];
        grid_points_[idx_ + 1] = y_val;
        grid_points_[idx_ + 2] = -axes_scale_.z;

        idx_ += 3;

        grid_points_[idx_] = gv.x.data[k];
        grid_points_[idx_ + 1] = y_val;
        grid_points_[idx_ + 2] = axes_scale_.z;

        idx_ += 3;
    }

    for (size_t k = 0; k < gv.z.num_valid_values; k++)
    {
        grid_points_[idx_] = axes_scale_.x;
        grid_points_[idx_ + 1] = y_val;
        grid_points_[idx_ + 2] = gv.z.data[k];

        idx_ += 3;

        grid_points_[idx_] = -axes_scale_.x;
        grid_points_[idx_ + 1] = y_val;
        grid_points_[idx_ + 2] = gv.z.data[k];

        idx_ += 3;
    }
}

void PlotBoxGrid::fillYZGrid(const GridVectors& gv, const AxesSideConfiguration& axes_side_configuration)
{
    const float x_val = axes_side_configuration.yz_plane_x_value * axes_scale_.x;

    for (size_t k = 0; k < gv.y.num_valid_values; k++)
    {
        grid_points_[idx_] = x_val;
        grid_points_[idx_ + 1] = gv.y.data[k];
        grid_points_[idx_ + 2] = -axes_scale_.z;

        idx_ += 3;

        grid_points_[idx_] = x_val;
        grid_points_[idx_ + 1] = gv.y.data[k];
        grid_points_[idx_ + 2] = axes_scale_.z;

        idx_ += 3;
    }

    for (size_t k = 0; k < gv.z.num_valid_values; k++)
    {
        grid_points_[idx_] = x_val;
        grid_points_[idx_ + 1] = axes_scale_.y;
        grid_points_[idx_ + 2] = gv.z.data[k];

        idx_ += 3;

        grid_points_[idx_] = x_val;
        grid_points_[idx_ + 1] = -axes_scale_.y;
        grid_points_[idx_ + 2] = gv.z.data[k];

        idx_ += 3;
    }
}

void PlotBoxGrid::render(const GridVectors& gv,
                         const AxesLimits& axes_limits,
                         const ViewAngles& view_angles,
                         const AxesSideConfiguration& axes_side_configuration)
{
    azimuth_ = view_angles.getSnappedAzimuth();
    elevation_ = view_angles.getSnappedElevation();

    idx_ = 0;

    axes_scale_ = axes_limits.getAxesScale() / 2.0;

    fillXYGrid(gv, axes_side_configuration);
    fillXZGrid(gv, axes_side_configuration);
    fillYZGrid(gv, axes_side_configuration);

    const size_t num_vertices_to_render = idx_ / 3;

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_vertices_to_render * 3 * sizeof(float), grid_points_);
    glBindVertexArray(vertex_buffer_array_);
    glDrawArrays(GL_LINES, 0, num_vertices_to_render);
    glBindVertexArray(0);
}

PlotBoxGrid::PlotBoxGrid()
{
    // TODO: Remove last ' * 2' for num_vertices_per_plane?
    const size_t num_vertices_per_plane = GridVector::kMaxNumGridNumbers * 2 * 2;
    const size_t num_vertices = num_vertices_per_plane * 3;
    const size_t num_array_elements = num_vertices * 3;
    const size_t num_bytes = num_array_elements * sizeof(float);

    grid_points_ = new float[num_array_elements];

    azimuth_ = 0.0f;
    elevation_ = 0.0f;

    idx_ = 0;

    glGenVertexArrays(1, &vertex_buffer_array_);
    glBindVertexArray(vertex_buffer_array_);

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, num_bytes, grid_points_, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

PlotBoxGrid::~PlotBoxGrid()
{
    delete[] grid_points_;
}