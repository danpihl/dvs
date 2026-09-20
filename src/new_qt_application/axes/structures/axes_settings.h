#ifndef MAIN_APPLICATION_AXES_STRUCTURES_AXES_SETTINGS_H_
#define MAIN_APPLICATION_AXES_STRUCTURES_AXES_SETTINGS_H_

#include <unistd.h>

#include <string>

#include "lumos/math.h"
#include "misc/rgb_triplet.h"

using namespace lumos;

struct AxesSettings
{
    bool is_default;

    RGBTripletf plot_pane_background_color;
    RGBTripletf plot_box_wall_color;
    RGBTripletf grid_color;
    RGBTripletf axes_numbers_color;
    RGBTripletf axes_letters_color;

    bool grid_on;
    bool plot_box_on;
    bool axes_numbers_on;
    bool axes_letters_on;
    bool clipping_on;

    std::string x_label;
    std::string y_label;
    std::string z_label;

    size_t num_axes_ticks;

    AxesSettings();
};

#endif  // MAIN_APPLICATION_AXES_STRUCTURES_AXES_SETTINGS_H_
