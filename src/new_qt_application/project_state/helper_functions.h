#ifndef PROJECT_STATE_HELPER_FUNCTIONS_H
#define PROJECT_STATE_HELPER_FUNCTIONS_H

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "lumos/plotting/enumerations.h"
#include "lumos/logging.h"
#include "misc/rgb_triplet.h"

constexpr RGBTriplet<float> hexToRgbTripletf(const std::uint32_t hex_val)
{
    const std::uint8_t r = (hex_val & 0xFF0000) >> 16;
    const std::uint8_t g = (hex_val & 0x00FF00) >> 8;
    const std::uint8_t b = (hex_val & 0x0000FF);

    return RGBTriplet<float>{
        static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f};
}

inline lumos::GuiElementType parseGuiElementType(const nlohmann::json& j)
{
    const std::string type_string = j["type"];

    if (type_string == "BUTTON")
    {
        return lumos::GuiElementType::Button;
    }
    else if (type_string == "SLIDER")
    {
        return lumos::GuiElementType::Slider;
    }
    else if (type_string == "CHECKBOX")
    {
        return lumos::GuiElementType::Checkbox;
    }
    else if (type_string == "EDITABLE_TEXT")
    {
        return lumos::GuiElementType::EditableText;
    }
    else if (type_string == "DROPDOWN_MENU")
    {
        return lumos::GuiElementType::DropdownMenu;
    }
    else if (type_string == "LISTBOX")
    {
        return lumos::GuiElementType::ListBox;
    }
    else if (type_string == "RADIO_BUTTON_GROUP")
    {
        return lumos::GuiElementType::RadioButtonGroup;
    }
    else if (type_string == "TEXT_LABEL")
    {
        return lumos::GuiElementType::TextLabel;
    }
    else if (type_string == "STATIC_BOX")
    {
        return lumos::GuiElementType::StaticBox;
    }
    else if (type_string == "PLOT_PANE")
    {
        return lumos::GuiElementType::PlotPane;
    }
    else if (type_string == "SCROLLING_TEXT")
    {
        return lumos::GuiElementType::ScrollingText;
    }
    else
    {
        return lumos::GuiElementType::Unknown;
    }
}

inline std::string guiElementTypeToString(const lumos::GuiElementType& type)
{
    std::string res;

    switch (type)
    {
        case lumos::GuiElementType::Button: {
            res = "BUTTON";
            break;
        }
        case lumos::GuiElementType::Slider: {
            res = "SLIDER";
            break;
        }
        case lumos::GuiElementType::Checkbox: {
            res = "CHECKBOX";
            break;
        }
        case lumos::GuiElementType::EditableText: {
            res = "EDITABLE_TEXT";
            break;
        }
        case lumos::GuiElementType::DropdownMenu: {
            res = "DROPDOWN_MENU";
            break;
        }
        case lumos::GuiElementType::ListBox: {
            res = "LISTBOX";
            break;
        }
        case lumos::GuiElementType::RadioButtonGroup: {
            res = "RADIO_BUTTON_GROUP";
            break;
        }
        case lumos::GuiElementType::TextLabel: {
            res = "TEXT_LABEL";
            break;
        }
        case lumos::GuiElementType::StaticBox: {
            res = "STATIC_BOX";
            break;
        }
        case lumos::GuiElementType::PlotPane: {
            res = "PLOT_PANE";
            break;
        }
        case lumos::GuiElementType::ScrollingText: {
            res = "SCROLLING_TEXT";
            break;
        }
        default: {
            res = "UNKNOWN";
            break;
        }
    }

    return res;
}

inline void throwIfMissing(const nlohmann::json& j, const std::string& field_name, const std::string& exception_string)
{
    if (j.count(field_name) == 0)
    {
        throw std::runtime_error(exception_string);
    }
}

inline RGBTripletf jsonObjToColor(const nlohmann::json& j)
{
    return RGBTripletf{j["r"], j["g"], j["b"]};
}

inline nlohmann::json colorToJsonObj(const RGBTripletf& c)
{
    nlohmann::json j;
    j["r"] = c.red;
    j["g"] = c.green;
    j["b"] = c.blue;
    return j;
}

template <typename T>
void assignIfNotDefault(nlohmann::json& j, const std::string& key, const T& val, const T& default_value)
{
    if (val != default_value)
    {
        j[key] = val;
    }
}

template <typename T>
void assignIfNotDefault(
    nlohmann::json& j, const std::string& key, const std::string& sub_key, const T& val, const T& default_value)
{
    if (val != default_value)
    {
        j[key][sub_key] = val;
    }
}

template <typename T> T getOptionalValue(const nlohmann::json& j_data, const std::string& key, const T& default_value)
{
    if (j_data.contains(key))
    {
        return j_data[key];
    }
    else
    {
        return default_value;
    }
}

#endif  // PROJECT_STATE_HELPER_FUNCTIONS_H
