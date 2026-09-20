#ifndef MAIN_APPLICATION_COLOR_PICKER_H_
#define MAIN_APPLICATION_COLOR_PICKER_H_

#include "misc/rgb_triplet.h"

class ColorPicker
{
private:
    size_t color_idx_;
    size_t edge_color_idx_;
    size_t face_color_idx_;

public:
    ColorPicker();

    RGBTripletf getNextColor();
    RGBTripletf getNextFaceColor();
    RGBTripletf getNextEdgeColor();

    void reset();
};

#endif
