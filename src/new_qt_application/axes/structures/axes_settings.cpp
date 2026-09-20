#include "axes/structures/axes_settings.h"

AxesSettings::AxesSettings()
{
    is_default = true;

    // Dark
    plot_pane_background_color = RGBTripletf(58.0f / 255.0f, 63.0f / 255.0f, 66.0f / 255.0f);
    // tab_background_color = RGBTripletf(35.0f / 255.0f, 42.0f / 255.0f, 48.0f / 255.0f);
    // tab_button_normal_color = RGBTripletf(25.0f / 255.0f, 32.0f / 255.0f, 38.0f / 255.0f);
    // tab_button_clicked_color = RGBTripletf(15.0f / 255.0f, 12.0f / 255.0f, 18.0f / 255.0f);
    // tab_button_selected_color = RGBTripletf(55.0f / 255.0f, 52.0f / 255.0f, 38.0f / 255.0f);
    // tab_button_text_color = RGBTripletf(1.0f, 1.0f, 1.0f);
    plot_box_wall_color = RGBTripletf(44.0f / 255.0f, 45.0f / 255.0f, 108.0f / 255.0f);
    grid_color = RGBTripletf(1.0f, 1.0f, 1.0f);
    axes_numbers_color = RGBTripletf(1.0f, 1.0f, 1.0f);
    axes_letters_color = RGBTripletf(1.0f, 1.0f, 1.0f);

    // Light
    // plot_pane_background_color = RGBTripletf(164.0f / 255.0f, 217.0f / 255.0f, 200.0f / 255.0f);
    // tab_button_normal_color = RGBTripletf(174.0f / 255.0f, 227.0f / 255.0f, 209.0f / 255.0f);
    // tab_button_color = RGBTripletf(184.0f / 255.0f, 237.0f / 255.0f, 219.0f / 255.0f);
    // tab_button_clicked_color = RGBTripletf(174.0f / 255.0f, 227.0f / 255.0f, 209.0f / 255.0f);
    // tab_button_selected_color = RGBTripletf(164.0f / 255.0f, 217.0f / 255.0f, 199.0f / 255.0f);
    // tab_button_text_color = RGBTripletf(0.0f, 0.0f, 0.0f);
    // plot_box_wall_color = RGBTripletf(1.0f, 1.0f, 1.0f);
    // grid_color = RGBTripletf(0.7f, 0.7f, 0.7f);
    // axes_numbers_color = RGBTripletf(0.7f, 0.7f, 0.7f);
    // axes_letters_color = RGBTripletf(0.7f, 0.7f, 0.7f);

    grid_on = true;
    plot_box_on = false;
    axes_numbers_on = false;
    axes_letters_on = false;
    clipping_on = true;

    x_label = "x";
    y_label = "y";
    z_label = "z";

    num_axes_ticks = 4;
}
