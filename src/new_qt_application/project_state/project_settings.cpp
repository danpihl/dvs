#include "project_state/project_settings.h"

constexpr RGBTriplet<float> kTabBackgroundColorDefault{0.7765f, 0.9020f, 0.5569f};
constexpr RGBTriplet<float> kButtonNormalColorDefault{0.682f, 0.890f, 0.820f};
constexpr RGBTriplet<float> kButtonClickedColorDefault{0.682f, 0.890f, 0.820f};
constexpr RGBTriplet<float> kButtonSelectedColorDefault{0.643f, 0.851f, 0.780f};
constexpr RGBTriplet<float> kButtonTextColorDefault{0.0f, 0.0f, 0.0f};

RGBTriplet<float> kMainWindowBackgroundColor{0.7765f, 0.9020f, 0.5569f};

TabSettings::TabSettings()
    : name{""},
      background_color{kTabBackgroundColorDefault},
      button_normal_color{kButtonNormalColorDefault},
      button_clicked_color{kButtonClickedColorDefault},
      button_selected_color{kButtonSelectedColorDefault},
      button_text_color{kButtonTextColorDefault}
{
}

TabSettings::TabSettings(const nlohmann::json& j)
{
    name = j["name"];

    for (size_t k = 0; k < j["elements"].size(); k++)
    {
        const lumos::GuiElementType type{parseGuiElementType(j["elements"][k])};

        if (type == lumos::GuiElementType::PlotPane)
        {
            elements.emplace_back(std::make_shared<PlotPaneSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::Button)
        {
            elements.emplace_back(std::make_shared<ButtonSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::Slider)
        {
            elements.emplace_back(std::make_shared<SliderSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::Checkbox)
        {
            elements.emplace_back(std::make_shared<CheckboxSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::EditableText)
        {
            elements.emplace_back(std::make_shared<EditableTextSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::DropdownMenu)
        {
            elements.emplace_back(std::make_shared<DropdownMenuSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::ListBox)
        {
            elements.emplace_back(std::make_shared<ListBoxSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::RadioButtonGroup)
        {
            elements.emplace_back(std::make_shared<RadioButtonGroupSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::ScrollingText)
        {
            elements.emplace_back(std::make_shared<ScrollingTextSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::TextLabel)
        {
            elements.emplace_back(std::make_shared<TextLabelSettings>(j["elements"][k]));
        }
        else if (type == lumos::GuiElementType::StaticBox)
        {
            elements.emplace_back(std::make_shared<StaticBoxSettings>(j["elements"][k]));
        }
        else
        {
            throw std::runtime_error("Unknown element type: \"" + j["elements"][k]["type"].get<std::string>() + "\"");
        }
    }

    background_color =
        j.count("background_color") > 0 ? jsonObjToColor(j["background_color"]) : kTabBackgroundColorDefault;
    button_normal_color =
        j.count("button_normal_color") > 0 ? jsonObjToColor(j["button_normal_color"]) : kButtonNormalColorDefault;
    button_clicked_color =
        j.count("button_clicked_color") > 0 ? jsonObjToColor(j["button_clicked_color"]) : kButtonClickedColorDefault;
    button_selected_color =
        j.count("button_selected_color") > 0 ? jsonObjToColor(j["button_selected_color"]) : kButtonSelectedColorDefault;
    button_text_color =
        j.count("button_text_color") > 0 ? jsonObjToColor(j["button_text_color"]) : kButtonTextColorDefault;
}

nlohmann::json TabSettings::toJson() const
{
    nlohmann::json json_elements = nlohmann::json::array();

    for (const auto& e : elements)
    {
        json_elements.push_back(e->toJson());
    }

    nlohmann::json j;
    j["name"] = name;
    j["elements"] = json_elements;

    if (background_color != kTabBackgroundColorDefault)
    {
        j["background_color"] = colorToJsonObj(background_color);
    }

    if (background_color != kButtonNormalColorDefault)
    {
        j["button_normal_color"] = colorToJsonObj(button_normal_color);
    }

    if (background_color != kButtonClickedColorDefault)
    {
        j["button_clicked_color"] = colorToJsonObj(button_clicked_color);
    }

    if (background_color != kButtonSelectedColorDefault)
    {
        j["button_selected_color"] = colorToJsonObj(button_selected_color);
    }

    if (background_color != kButtonTextColorDefault)
    {
        j["button_text_color"] = colorToJsonObj(button_text_color);
    }

    return j;
}

bool TabSettings::hasElementWithHandleString(const std::string& handle_string) const
{
    return std::find_if(elements.begin(), elements.end(), [&](const std::shared_ptr<ElementSettings>& es) -> bool {
               return es->handle_string == handle_string;
           }) != elements.end();
}

std::shared_ptr<ElementSettings> TabSettings::getElementWithHandleString(const std::string& handle_string) const
{
    LUMOS_ASSERT(hasElementWithHandleString(handle_string));

    const auto q = std::find_if(
        elements.begin(), elements.end(), [&handle_string](const std::shared_ptr<ElementSettings>& e) -> bool {
            return e->handle_string == handle_string;
        });

    return *q;
}

bool areDerivedElementEqual(const std::shared_ptr<ElementSettings>& lhs, const std::shared_ptr<ElementSettings>& rhs)
{
    if (lhs->type != rhs->type)
    {
        return false;
    }

    switch (lhs->type)
    {
        case lumos::GuiElementType::PlotPane: {
            const std::shared_ptr<PlotPaneSettings> lhs_casted = std::dynamic_pointer_cast<PlotPaneSettings>(lhs);
            const std::shared_ptr<PlotPaneSettings> rhs_casted = std::dynamic_pointer_cast<PlotPaneSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::Button: {
            const std::shared_ptr<ButtonSettings> lhs_casted = std::dynamic_pointer_cast<ButtonSettings>(lhs);
            const std::shared_ptr<ButtonSettings> rhs_casted = std::dynamic_pointer_cast<ButtonSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::Slider: {
            const std::shared_ptr<SliderSettings> lhs_casted = std::dynamic_pointer_cast<SliderSettings>(lhs);
            const std::shared_ptr<SliderSettings> rhs_casted = std::dynamic_pointer_cast<SliderSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::Checkbox: {
            const std::shared_ptr<CheckboxSettings> lhs_casted = std::dynamic_pointer_cast<CheckboxSettings>(lhs);
            const std::shared_ptr<CheckboxSettings> rhs_casted = std::dynamic_pointer_cast<CheckboxSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::EditableText: {
            const std::shared_ptr<EditableTextSettings> lhs_casted =
                std::dynamic_pointer_cast<EditableTextSettings>(lhs);
            const std::shared_ptr<EditableTextSettings> rhs_casted =
                std::dynamic_pointer_cast<EditableTextSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::DropdownMenu: {
            const std::shared_ptr<DropdownMenuSettings> lhs_casted =
                std::dynamic_pointer_cast<DropdownMenuSettings>(lhs);
            const std::shared_ptr<DropdownMenuSettings> rhs_casted =
                std::dynamic_pointer_cast<DropdownMenuSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::ListBox: {
            const std::shared_ptr<ListBoxSettings> lhs_casted = std::dynamic_pointer_cast<ListBoxSettings>(lhs);
            const std::shared_ptr<ListBoxSettings> rhs_casted = std::dynamic_pointer_cast<ListBoxSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::RadioButtonGroup: {
            const std::shared_ptr<RadioButtonGroupSettings> lhs_casted =
                std::dynamic_pointer_cast<RadioButtonGroupSettings>(lhs);
            const std::shared_ptr<RadioButtonGroupSettings> rhs_casted =
                std::dynamic_pointer_cast<RadioButtonGroupSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::TextLabel: {
            const std::shared_ptr<TextLabelSettings> lhs_casted = std::dynamic_pointer_cast<TextLabelSettings>(lhs);
            const std::shared_ptr<TextLabelSettings> rhs_casted = std::dynamic_pointer_cast<TextLabelSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::StaticBox: {
            const std::shared_ptr<StaticBoxSettings> lhs_casted = std::dynamic_pointer_cast<StaticBoxSettings>(lhs);
            const std::shared_ptr<StaticBoxSettings> rhs_casted = std::dynamic_pointer_cast<StaticBoxSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        case lumos::GuiElementType::ScrollingText: {
            const std::shared_ptr<ScrollingTextSettings> lhs_casted =
                std::dynamic_pointer_cast<ScrollingTextSettings>(lhs);
            const std::shared_ptr<ScrollingTextSettings> rhs_casted =
                std::dynamic_pointer_cast<ScrollingTextSettings>(rhs);
            return *lhs_casted == *rhs_casted;
        }
        default: {
            throw std::runtime_error("Unknown element type");
        }
    }
}

bool TabSettings::operator==(const TabSettings& other) const
{
    if ((name != other.name) || (elements.size() != other.elements.size()))
    {
        return false;
    }

    if ((background_color != other.background_color) || (button_normal_color != other.button_normal_color) ||
        (button_clicked_color != other.button_clicked_color) ||
        (button_selected_color != other.button_selected_color) || (button_text_color != other.button_text_color))
    {
        return false;
    }

    for (size_t k = 0; k < elements.size(); k++)
    {
        if (other.hasElementWithHandleString(elements[k]->handle_string))
        {
            const std::shared_ptr<ElementSettings> other_element =
                other.getElementWithHandleString(elements[k]->handle_string);

            if (areDerivedElementEqual(other_element, elements[k]))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool TabSettings::operator!=(const TabSettings& other) const
{
    return !(*this == other);
}

WindowSettings::WindowSettings() : x{0.0f}, y{0.0f}, width{0.0f}, height{0.0f}, name{""} {}

WindowSettings::WindowSettings(const nlohmann::json& j)
{
    x = j["x"];
    y = j["y"];
    width = j["width"];
    height = j["height"];

    name = j["name"];

    for (size_t k = 0; k < j["tabs"].size(); k++)
    {
        tabs.emplace_back(j["tabs"][k]);
    }
}

nlohmann::json WindowSettings::toJson() const
{
    nlohmann::json json_tabs = nlohmann::json::array();

    for (const auto& t : tabs)
    {
        json_tabs.push_back(t.toJson());
    }

    nlohmann::json j;
    j["x"] = x;
    j["y"] = y;
    j["width"] = width;
    j["height"] = height;
    j["name"] = name;
    j["tabs"] = json_tabs;

    return j;
}

bool WindowSettings::hasTabWithName(const std::string& name) const
{
    return std::find_if(tabs.begin(), tabs.end(), [&](const TabSettings& ts) -> bool { return ts.name == name; }) !=
           tabs.end();
}

TabSettings WindowSettings::getTabWithName(const std::string& name) const
{
    LUMOS_ASSERT(hasTabWithName(name));
    TabSettings res{};

    // TODO: Use find_if?
    for (const TabSettings& t : tabs)
    {
        if (t.name == name)
        {
            res = t;
            break;
        }
    }
    return res;
}

bool WindowSettings::operator==(const WindowSettings& other) const
{
    if ((name != other.name) || (tabs.size() != other.tabs.size()))
    {
        return false;
    }

    if ((x != other.x) || (y != other.y) || (width != other.width) || (height != other.height))
    {
        return false;
    }

    for (size_t k = 0; k < tabs.size(); k++)
    {
        if (other.hasTabWithName(tabs[k].name))
        {
            const TabSettings other_element = other.getTabWithName(tabs[k].name);
            if (other_element != tabs[k])
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool WindowSettings::operator!=(const WindowSettings& other) const
{
    return !(*this == other);
}

ProjectSettings::ProjectSettings(const std::string& file_path)
{
    std::ifstream input_file(file_path);
    nlohmann::json j;
    try
    {
        input_file >> j;

        for (size_t k = 0; k < j["windows"].size(); k++)
        {
            windows_.emplace_back(j["windows"][k]);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception thrown when reading file " << file_path << ": " << e.what() << std::endl;
    }
}

void ProjectSettings::pushBackWindowSettings(const WindowSettings& ws)
{
    windows_.push_back(ws);
}

std::vector<WindowSettings> ProjectSettings::getWindows() const
{
    return windows_;
}

nlohmann::json ProjectSettings::toJson() const
{
    nlohmann::json j_to_save;
    j_to_save["windows"] = nlohmann::json::array();

    for (auto we : windows_)
    {
        j_to_save["windows"].push_back(we.toJson());
    }

    return j_to_save;
}

bool ProjectSettings::hasWindowWithName(const std::string& name) const
{
    return std::find_if(windows_.begin(), windows_.end(), [&](const WindowSettings& ws) -> bool {
               return ws.name == name;
           }) != windows_.end();
}

WindowSettings ProjectSettings::getWindowWithName(const std::string& name) const
{
    LUMOS_ASSERT(hasWindowWithName(name));
    WindowSettings res;
    for (const WindowSettings& ws : windows_)
    {
        if (ws.name == name)
        {
            res = ws;
            break;
        }
    }
    return res;
}

bool ProjectSettings::operator==(const ProjectSettings& other) const
{
    if (windows_.size() != other.windows_.size())
    {
        return false;
    }

    for (const WindowSettings& ws : windows_)
    {
        if (other.hasWindowWithName(ws.name))
        {
            if (other.getWindowWithName(ws.name) != ws)
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool ProjectSettings::operator!=(const ProjectSettings& other) const
{
    return !(*this == other);
}
