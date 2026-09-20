#ifndef MAIN_APPLICATION_MISC_RGB_TRIPLET_H
#define MAIN_APPLICATION_MISC_RGB_TRIPLET_H

#include <stdlib.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <type_traits>
#include <vector>

#include "lumos/plotting/enumerations.h"
#include "lumos/logging.h"

template <typename T> struct RGBTriplet
{
    T red;
    T green;
    T blue;

    constexpr RGBTriplet() = default;
    constexpr RGBTriplet(const T red_, const T green_, const T blue_) : red(red_), green(green_), blue(blue_) {}

    RGBTriplet(const uint32_t hex_color_code)
    {
        red = static_cast<float>((hex_color_code >> 16) & 0xFF) / 255.0f;
        green = static_cast<float>((hex_color_code >> 8) & 0xFF) / 255.0f;
        blue = static_cast<float>(hex_color_code & 0xFF) / 255.0f;
    }

    bool operator==(const RGBTriplet& other) const
    {
        return (red == other.red) && (green == other.green) && (blue == other.blue);
    }

    bool operator!=(const RGBTriplet& other) const
    {
        return !(*this == other);
    }
};

template <typename T> std::ostream& operator<<(std::ostream& os, const RGBTriplet<T>& c)
{
    const std::string s =
        "{ " + std::to_string(c.red) + ", " + std::to_string(c.green) + ", " + std::to_string(c.blue) + " }";
    os << s;

    return os;
}

using RGBTripletf = RGBTriplet<float>;

#endif  // MAIN_APPLICATION_MISC_RGB_TRIPLET_H
