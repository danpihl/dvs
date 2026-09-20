#ifndef MAIN_APPLICATION_PROJECT_STATE_PROJECT_SETTINGS_H_
#define MAIN_APPLICATION_PROJECT_STATE_PROJECT_SETTINGS_H_

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>

#include "lumos/plotting/enumerations.h"
#include "lumos/logging.h"
#include "lumos/plotting/plot_properties.h"
#include "misc/rgb_triplet.h"
#include "project_state/helper_functions.h"
#include "serial_interface/definitions.h"

extern RGBTriplet<float> kMainWindowBackgroundColor;

struct ElementSettings
{
    float x;
    float y;
    float width;
    float height;

    std::string handle_string;

    int z_order;

    lumos::GuiElementType type;

    ElementSettings();
    explicit ElementSettings(const nlohmann::json& j);
    virtual ~ElementSettings() {}

    void parseSettings(const nlohmann::json& j);

    virtual nlohmann::json toJson() const;

    bool operator==(const ElementSettings& other) const;
    bool operator!=(const ElementSettings& other) const;
};

bool areDerivedElementEqual(const std::shared_ptr<ElementSettings>& lhs, const std::shared_ptr<ElementSettings>& rhs);

enum class StreamType : uint8_t
{
    PLOT,
    PLOT3D,
    SCATTER,
    SCATTER3D,
    STAIRS,
    UNKNOWN
};

struct SubscribedTextStreamSettings
{
    TopicId topic_id{kUnknownTopicId};

    std::optional<RGBTripletf> text_color{std::nullopt};

    SubscribedTextStreamSettings();
    explicit SubscribedTextStreamSettings(const nlohmann::json& j);

    bool operator==(const SubscribedTextStreamSettings& other) const;
    bool operator!=(const SubscribedTextStreamSettings& other) const;

    nlohmann::json toJson() const;
};

struct ScrollingTextSettings : ElementSettings
{
    ScrollingTextSettings();
    explicit ScrollingTextSettings(const nlohmann::json& j);

    std::string title;
    bool print_timestamp;
    bool print_topic_id;

    std::vector<SubscribedTextStreamSettings> subscribed_streams;

    nlohmann::json toJson() const override;

    bool operator==(const ScrollingTextSettings& other) const;
    bool operator!=(const ScrollingTextSettings& other) const;
};

struct SubscribedStreamSettings
{
    TopicId topic_id{kUnknownTopicId};
    StreamType stream_type{StreamType::UNKNOWN};
    float alpha{1.0f};
    uint8_t line_width{1U};
    uint8_t point_size{1U};
    std::optional<lumos::properties::Color> color{std::nullopt};
    lumos::properties::LineStyle line_style{lumos::properties::LineStyle::SOLID};
    lumos::properties::ScatterStyle scatter_style{lumos::properties::ScatterStyle::CIRCLE};
    std::string label{};

    SubscribedStreamSettings() = default;
    explicit SubscribedStreamSettings(const nlohmann::json& j);

    bool operator==(const SubscribedStreamSettings& other) const;

    nlohmann::json toJson() const;
};

struct PlotPaneSettings : ElementSettings
{
    PlotPaneSettings();
    explicit PlotPaneSettings(const nlohmann::json& j);

    std::string title;

    RGBTripletf background_color;
    RGBTripletf plot_box_color;
    RGBTripletf grid_color;
    RGBTripletf axes_numbers_color;
    RGBTripletf axes_letters_color;

    bool grid_on;
    bool plot_box_on;
    bool axes_numbers_on;
    bool axes_letters_on;
    bool clipping_on;
    bool snap_view_to_axes;

    float pane_radius;

    enum class ProjectionMode
    {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };

    ProjectionMode projection_mode;
    std::vector<SubscribedStreamSettings> subscribed_streams;

    void parsePlotPaneSettings(const nlohmann::json& j);

    nlohmann::json toJson() const override;

    void defaultInitializeAllSettings();

    bool operator==(const PlotPaneSettings& other) const;
    bool operator!=(const PlotPaneSettings& other) const;
};

using GuiElementId = uint16_t;

struct GuiElementSettings : public ElementSettings
{
    bool publish_to_local{true};
    bool publish_to_serial{false};
    GuiElementId id{0U};

    GuiElementSettings();
    explicit GuiElementSettings(const nlohmann::json& j);

    nlohmann::json toJson() const override;

    bool operator==(const GuiElementSettings& other) const;
    bool operator!=(const GuiElementSettings& other) const;
};

struct ButtonSettings : public GuiElementSettings
{
    std::string label;

    ButtonSettings();
    explicit ButtonSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const ButtonSettings& other) const;
    bool operator!=(const ButtonSettings& other) const;
};

struct CheckboxSettings : public GuiElementSettings
{
    std::string label;

    CheckboxSettings();
    explicit CheckboxSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const CheckboxSettings& other) const;
    bool operator!=(const CheckboxSettings& other) const;
};

struct EditableTextSettings : public GuiElementSettings
{
    std::string init_value;

    EditableTextSettings();
    explicit EditableTextSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const EditableTextSettings& other) const;
    bool operator!=(const EditableTextSettings& other) const;
};

struct DropdownMenuSettings : public GuiElementSettings
{
    std::string initially_selected_item;
    std::vector<std::string> elements;

    DropdownMenuSettings();
    explicit DropdownMenuSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const DropdownMenuSettings& other) const;
    bool operator!=(const DropdownMenuSettings& other) const;
};

struct ListBoxSettings : public GuiElementSettings
{
    std::vector<std::string> elements;

    ListBoxSettings();
    explicit ListBoxSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const ListBoxSettings& other) const;
    bool operator!=(const ListBoxSettings& other) const;
};

struct RadioButtonSettings
{
    std::string label;

    RadioButtonSettings();
    explicit RadioButtonSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const;

    bool operator==(const RadioButtonSettings& other) const;
    bool operator!=(const RadioButtonSettings& other) const;
};

struct RadioButtonGroupSettings : public GuiElementSettings
{
    std::string label;
    std::vector<RadioButtonSettings> radio_buttons;

    RadioButtonGroupSettings();
    explicit RadioButtonGroupSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const RadioButtonGroupSettings& other) const;
    bool operator!=(const RadioButtonGroupSettings& other) const;
};

struct TextLabelSettings : public GuiElementSettings
{
    std::string label;

    TextLabelSettings();
    explicit TextLabelSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const TextLabelSettings& other) const;
    bool operator!=(const TextLabelSettings& other) const;
};

struct SliderSettings : public GuiElementSettings
{
    int min_value;
    int max_value;
    int init_value;
    int step_size;
    bool is_horizontal;

    SliderSettings();
    explicit SliderSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const SliderSettings& other) const;
    bool operator!=(const SliderSettings& other) const;
};

struct StaticBoxSettings : public ElementSettings
{
    std::string label;
    std::vector<std::shared_ptr<ElementSettings>> elements;

    StaticBoxSettings();
    explicit StaticBoxSettings(const nlohmann::json& j_data);

    nlohmann::json toJson() const override;

    bool operator==(const StaticBoxSettings& other) const;
    bool operator!=(const StaticBoxSettings& other) const;
};

struct TabSettings
{
    std::string name;
    RGBTripletf background_color;
    RGBTripletf button_normal_color;
    RGBTripletf button_clicked_color;
    RGBTripletf button_selected_color;
    RGBTripletf button_text_color;
    std::vector<std::shared_ptr<ElementSettings>> elements;

    TabSettings();
    TabSettings(const nlohmann::json& j);
    nlohmann::json toJson() const;

    bool hasElementWithHandleString(const std::string& handle_string) const;
    std::shared_ptr<ElementSettings> getElementWithHandleString(const std::string& handle_string) const;

    bool operator==(const TabSettings& other) const;
    bool operator!=(const TabSettings& other) const;
};

struct WindowSettings
{
    float x;
    float y;
    float width;
    float height;
    std::string name;
    std::vector<TabSettings> tabs;

    WindowSettings();
    WindowSettings(const nlohmann::json& j);

    nlohmann::json toJson() const;

    bool hasTabWithName(const std::string& name) const;

    TabSettings getTabWithName(const std::string& name) const;

    bool operator==(const WindowSettings& other) const;
    bool operator!=(const WindowSettings& other) const;
};

class ProjectSettings
{
private:
    std::vector<WindowSettings> windows_;

public:
    ProjectSettings() = default;
    ProjectSettings(const std::string& file_path);

    void pushBackWindowSettings(const WindowSettings& ws);

    std::vector<WindowSettings> getWindows() const;

    nlohmann::json toJson() const;

    bool hasWindowWithName(const std::string& name) const;
    WindowSettings getWindowWithName(const std::string& name) const;

    bool operator==(const ProjectSettings& other) const;
    bool operator!=(const ProjectSettings& other) const;
};

#endif  // MAIN_APPLICATION_PROJECT_STATE_PROJECT_SETTINGS_H_
