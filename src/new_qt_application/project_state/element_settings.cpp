#include "project_state/project_settings.h"

constexpr int kZOrderDefault{-1};

ElementSettings::ElementSettings()
    : x{0.0f},
      y{0.0f},
      width{100.0f},
      height{100.0f},
      handle_string{"<NO-NAME>"},
      z_order{kZOrderDefault},
      type{lumos::GuiElementType::Unknown}
{
}

ElementSettings::ElementSettings(const nlohmann::json& j) : ElementSettings{}
{
    parseSettings(j);
}

void ElementSettings::parseSettings(const nlohmann::json& j)
{
    handle_string = j["handle_string"];

    x = j["x"];
    y = j["y"];
    width = j["width"];
    height = j["height"];

    width = std::min(std::max(width, 0.01f), 1.0f);
    height = std::min(std::max(height, 0.01f), 1.0f);

    x = std::max(std::min(x, 0.99f), 0.0f);
    y = std::max(std::min(y, 0.99f), 0.0f);

    if (j.count("z_order") > 0)
    {
        z_order = static_cast<int>(j["z_order"]);
    }

    type = parseGuiElementType(j);
}

nlohmann::json ElementSettings::toJson() const
{
    nlohmann::json j;

    j["handle_string"] = handle_string;

    j["x"] = x;
    j["y"] = y;
    j["width"] = width;
    j["height"] = height;
    j["type"] = guiElementTypeToString(type);

    assignIfNotDefault(j, "z_order", z_order, kZOrderDefault);

    return j;
}

bool ElementSettings::operator==(const ElementSettings& other) const
{
    return x == other.x && y == other.y && width == other.width && height == other.height &&
           handle_string == other.handle_string && z_order == other.z_order && type == other.type;
}

bool ElementSettings::operator!=(const ElementSettings& other) const
{
    return !(*this == other);
}
