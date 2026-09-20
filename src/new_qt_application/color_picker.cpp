#include "color_picker.h"

#include <array>

std::array<RGBTripletf, 6> colors = {RGBTripletf{218.0f / 255.0f, 83.0f / 255.0f, 29.0f / 255.0f},
                                     RGBTripletf{0.0f / 255.0f, 114.0f / 255.0f, 187.0f / 255.0f},
                                     RGBTripletf{238.0f / 255.0f, 177.0f / 255.0f, 49.0f / 255.0f},
                                     RGBTripletf{118.0f / 255.0f, 172.0f / 255.0f, 59.0f / 255.0f},
                                     RGBTripletf{29.0f / 255.0f, 218.0f / 255.0f, 186.0f / 255.0f},
                                     RGBTripletf{218.0f / 255.0f, 29.0f / 255.0f, 155.0f / 255.0f}};

ColorPicker::ColorPicker()
{
    color_idx_ = 0U;
    edge_color_idx_ = 0U;
    face_color_idx_ = 0U;
}

void ColorPicker::reset()
{
    color_idx_ = 0U;
    edge_color_idx_ = 0U;
    face_color_idx_ = 0U;
}

RGBTripletf ColorPicker::getNextColor()
{
    const RGBTripletf c = colors[color_idx_];
    color_idx_++;

    color_idx_ = color_idx_ >= colors.size() ? 0U : color_idx_;

    return c;
}

RGBTripletf ColorPicker::getNextFaceColor()
{
    const RGBTripletf c = colors[edge_color_idx_];
    edge_color_idx_++;

    edge_color_idx_ = edge_color_idx_ >= colors.size() ? 0U : edge_color_idx_;

    return c;
}

RGBTripletf ColorPicker::getNextEdgeColor()
{
    const RGBTripletf c = colors[face_color_idx_];
    face_color_idx_++;

    face_color_idx_ = face_color_idx_ >= colors.size() ? 0U : face_color_idx_;

    return c;
}
