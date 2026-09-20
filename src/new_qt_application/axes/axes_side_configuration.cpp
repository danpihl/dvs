#include "axes/axes_side_configuration.h"

AxesSideConfiguration::AxesSideConfiguration(const ViewAngles& view_angles, const bool perspective_projection)
{
    const float azimuth = view_angles.getSnappedAzimuth();
    const float elevation = view_angles.getSnappedElevation();
    const float azimuth_abs = std::abs(azimuth);
    const float elevation_abs = std::abs(elevation);
    const float angle_limit = view_angles.getAngleLimit();

    is_snapped = view_angles.isSnappedLookingAlongPositiveX() || view_angles.isSnappedLookingAlongNegativeX() ||
                 view_angles.isSnappedLookingAlongPositiveY() || view_angles.isSnappedLookingAlongNegativeY() ||
                 view_angles.isSnappedLookingAlongPositiveZ() || view_angles.isSnappedLookingAlongNegativeZ();

    // XY
    xy_plane_z_value = (elevation >= 0.0f) ? -1.0f : 1.0f;

    // XZ
    xz_plane_y_value = (((-M_PI / 2.0f) <= azimuth) && (azimuth <= (M_PI / 2.0f))) ? 1.0f : -1.0f;

    // YZ
    yz_plane_x_value = (azimuth >= 0.0f) ? 1.0f : -1.0f;

    z_axes_numbers_y_value = azimuth > 0.0f ? 1.0f : -1.0f;
    if ((azimuth < M_PI_2) && (azimuth >= -M_PI_2))
    {
        z_axes_numbers_x_value = -1.0f;
    }
    else
    {
        z_axes_numbers_x_value = 1.0f;
    }

    /*if (view_angles.isSnappedLookingAlongPositiveX())
    {
        xz_plane_y_value = 1.0f;
        z_axes_numbers_x_value = -1.0f;
    }
    else if (view_angles.isSnappedLookingAlongNegativeX())
    {
        xz_plane_y_value = -1.0f;
    }
    if (view_angles.isSnappedLookingAlongPositiveY())
    {
        yz_plane_x_value = -1.0f;
    }
    else if (view_angles.isSnappedLookingAlongNegativeY())
    {
        z_axes_numbers_y_value = 1.0f;
        yz_plane_x_value = 1.0f;
    }
    if (view_angles.isSnappedLookingAlongPositiveZ())
    {
        if (azimuth_abs < angle_limit)  // 0 deg
        {
            yz_plane_x_value = -1.0f;
        }
        else if (std::abs(M_PI_2 - azimuth) < angle_limit)  // 90 deg
        {
            xz_plane_y_value = 1.0f;
            z_axes_numbers_x_value = -1.0f;
        }
        else if (std::abs(M_PI - azimuth_abs) < angle_limit)  // -+180 deg
        {
            z_axes_numbers_y_value = 1.0f;
            yz_plane_x_value = 1.0f;
        }
        else if (std::abs(M_PI + azimuth_abs) < angle_limit)  // -90 deg
        {
            xz_plane_y_value = -1.0f;
        }
    }
    else if (view_angles.isSnappedLookingAlongNegativeZ())
    {
        if (azimuth_abs < angle_limit)  // 0 deg
        {
            yz_plane_x_value = -1.0f;
        }
        else if (std::abs(M_PI_2 - azimuth) < angle_limit)  // 90 deg
        {
            xz_plane_y_value = 1.0f;
            z_axes_numbers_x_value = -1.0f;
        }
        else if (std::abs(M_PI - azimuth_abs) < angle_limit)  // -+180 deg
        {
            z_axes_numbers_y_value = 1.0f;
            yz_plane_x_value = 1.0f;
        }
        else if (std::abs(M_PI + azimuth_abs) < angle_limit)  // -90 deg
        {
            xz_plane_y_value = -1.0f;
        }
    }*/

    y_axes_numbers_x_value = yz_plane_x_value;
    y_axes_numbers_z_value = xy_plane_z_value;

    /*if (!perspective_projection)
    {
        if (view_angles.isSnappedLookingAlongNegativeZ() && (azimuth_abs < angle_limit))
        {
            y_axes_numbers_x_value = 1.0f;
            y_axes_numbers_z_value = 1.0f;
        }
        else if (view_angles.isSnappedLookingAlongPositiveZ() && (azimuth_abs < angle_limit))
        {
            y_axes_numbers_x_value = 1.0f;
            y_axes_numbers_z_value = -1.0f;
        }
    }*/
}
