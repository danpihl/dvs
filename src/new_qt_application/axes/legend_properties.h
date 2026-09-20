#ifndef MAIN_APPLICATION_AXES_LEGEND_PROPERTIES_H_
#define MAIN_APPLICATION_AXES_LEGEND_PROPERTIES_H_

#include <string>
#include <string_view>

#include "lumos/plotting/enumerations.h"
#include "lumos/plotting/plot_properties.h"
#include "misc/rgb_triplet.h"

// TODO: This should be filled in plot_object_base and the individual object types

enum class LegendType
{
    LINE,
    POLYGON,
    DOT
};

struct LegendProperties
{
    std::string_view label;
    LegendType type;
    RGBTripletf color;
    RGBTripletf edge_color;
    RGBTripletf face_color;
    lumos::properties::ColorMap color_map;
    bool has_color_map;

    float point_size;
    lumos::properties::ScatterStyle scatter_style;

    // lumos::properties::LineStyle line_style;
};

#endif  // MAIN_APPLICATION_AXES_LEGEND_PROPERTIES_H_
