#include "project_state/project_settings.h"

constexpr RGBTriplet<float> kElementBackgroundColorDefault{hexToRgbTripletf(0x8EE6DA)};
constexpr RGBTriplet<float> kPlotBoxColorDefault{hexToRgbTripletf(0xFFFFFF)};
constexpr RGBTriplet<float> kGridColorDefault{0.7f, 0.7f, 0.7f};
constexpr RGBTriplet<float> kAxesNumbersColorDefault{0.0f, 0.0f, 0.0f};
constexpr RGBTriplet<float> kAxesLettersColorDefault{0.0f, 0.0f, 0.0f};
constexpr bool kGridOnDefault{true};
constexpr bool kPlotBoxOnDefault{true};
constexpr bool kAxesNumbersOnDefault{true};
constexpr bool kAxesLettersOnDefault{true};
constexpr bool kClippingOnDefault{true};
constexpr bool kSnapViewToAxesDefault{true};
constexpr float kPaneRadiusDefault{10.0f};

PlotPaneSettings::PlotPaneSettings()
    : ElementSettings{},
      title{"<NO-NAME>"},
      background_color{kElementBackgroundColorDefault},
      plot_box_color{kPlotBoxColorDefault},
      grid_color{kGridColorDefault},
      axes_numbers_color{kAxesNumbersColorDefault},
      axes_letters_color{kAxesLettersColorDefault},
      grid_on{kGridOnDefault},
      plot_box_on{kPlotBoxOnDefault},
      axes_numbers_on{kAxesNumbersOnDefault},
      axes_letters_on{kAxesLettersOnDefault},
      clipping_on{kClippingOnDefault},
      snap_view_to_axes{kSnapViewToAxesDefault},
      pane_radius{kPaneRadiusDefault},
      projection_mode{ProjectionMode::ORTHOGRAPHIC},
      subscribed_streams{}
{
    x = 0.0;
    y = 0.0;
    width = 0.4;
    height = 0.4;
    type = lumos::GuiElementType::PlotPane;
}

void PlotPaneSettings::defaultInitializeAllSettings()
{
    title = "";
    background_color = kElementBackgroundColorDefault;
    plot_box_color = kPlotBoxColorDefault;
    grid_color = kGridColorDefault;
    axes_numbers_color = kAxesNumbersColorDefault;
    axes_letters_color = kAxesLettersColorDefault;
    grid_on = kGridOnDefault;
    plot_box_on = kPlotBoxOnDefault;
    axes_numbers_on = kAxesNumbersOnDefault;
    axes_letters_on = kAxesLettersOnDefault;
    clipping_on = kClippingOnDefault;
    snap_view_to_axes = kSnapViewToAxesDefault;
    pane_radius = kPaneRadiusDefault;
    projection_mode = ProjectionMode::ORTHOGRAPHIC;
    subscribed_streams = {};
}

PlotPaneSettings::PlotPaneSettings(const nlohmann::json& j) : ElementSettings{j}
{
    defaultInitializeAllSettings();
    parsePlotPaneSettings(j);
}

bool PlotPaneSettings::operator==(const PlotPaneSettings& other) const
{
    return ElementSettings::operator==(other) && title == other.title && background_color == other.background_color &&
           plot_box_color == other.plot_box_color && grid_color == other.grid_color &&
           axes_numbers_color == other.axes_numbers_color && axes_letters_color == other.axes_letters_color &&
           grid_on == other.grid_on && plot_box_on == other.plot_box_on && axes_numbers_on == other.axes_numbers_on &&
           axes_letters_on == other.axes_letters_on && clipping_on == other.clipping_on &&
           snap_view_to_axes == other.snap_view_to_axes && pane_radius == other.pane_radius &&
           projection_mode == other.projection_mode && subscribed_streams == other.subscribed_streams;
}

bool PlotPaneSettings::operator!=(const PlotPaneSettings& other) const
{
    return !(*this == other);
}

void PlotPaneSettings::parsePlotPaneSettings(const nlohmann::json& j)
{
    if (!j.contains("element_specific_settings"))
    {
        return;
    }

    nlohmann::json j_ess = j["element_specific_settings"];

    background_color =
        j_ess.contains("background_color") ? jsonObjToColor(j_ess["background_color"]) : kElementBackgroundColorDefault;
    plot_box_color = j_ess.contains("plot_box_color") ? jsonObjToColor(j_ess["plot_box_color"]) : kPlotBoxColorDefault;
    grid_color = j_ess.contains("grid_color") ? jsonObjToColor(j_ess["grid_color"]) : kGridColorDefault;
    axes_numbers_color =
        j_ess.contains("axes_numbers_color") ? jsonObjToColor(j_ess["axes_numbers_color"]) : kAxesNumbersColorDefault;
    axes_letters_color =
        j_ess.contains("axes_letters_color") ? jsonObjToColor(j_ess["axes_letters_color"]) : kAxesLettersColorDefault;
    grid_on = j_ess.contains("grid_on") ? static_cast<bool>(j_ess["grid_on"]) : kGridOnDefault;
    plot_box_on = j_ess.contains("plot_box_on") ? static_cast<bool>(j_ess["plot_box_on"]) : kPlotBoxOnDefault;
    axes_numbers_on =
        j_ess.contains("axes_numbers_on") ? static_cast<bool>(j_ess["axes_numbers_on"]) : kAxesNumbersOnDefault;
    axes_letters_on =
        j_ess.contains("axes_letters_on") ? static_cast<bool>(j_ess["axes_letters_on"]) : kAxesLettersOnDefault;
    clipping_on = j_ess.contains("clipping_on") ? static_cast<bool>(j_ess["clipping_on"]) : kClippingOnDefault;
    snap_view_to_axes =
        j_ess.contains("snap_view_to_axes") ? static_cast<bool>(j_ess["snap_view_to_axes"]) : kSnapViewToAxesDefault;
    pane_radius = j_ess.contains("pane_radius") ? static_cast<float>(j_ess["pane_radius"]) : kPaneRadiusDefault;

    if (j_ess.contains("projection_mode"))
    {
        const std::string projection_type_str = j_ess["projection_mode"];
        if (projection_type_str == "orthographic")
        {
            projection_mode = ProjectionMode::ORTHOGRAPHIC;
        }
        else if (projection_type_str == "perspective")
        {
            projection_mode = ProjectionMode::PERSPECTIVE;
        }
        else
        {
            throw std::runtime_error("Invalid option for \"projection_mode\": \"" + projection_type_str + "\"");
        }
    }
    else
    {
        projection_mode = ProjectionMode::ORTHOGRAPHIC;
    }

    if (j_ess.contains("subscribed_streams"))
    {
        for (size_t k = 0; k < j_ess["subscribed_streams"].size(); k++)
        {
            subscribed_streams.push_back(SubscribedStreamSettings(j_ess["subscribed_streams"][k]));
        }
    }

    if (j_ess.contains("title"))
    {
        title = j_ess["title"];
    }
    else
    {
        title = "";
    }
}

nlohmann::json PlotPaneSettings::toJson() const
{
    nlohmann::json j = ElementSettings::toJson();

    if (title != "")
    {
        j["element_specific_settings"]["title"] = title;
    }

    if (background_color != kElementBackgroundColorDefault)
    {
        j["element_specific_settings"]["background_color"] = colorToJsonObj(background_color);
    }

    if (plot_box_color != kPlotBoxColorDefault)
    {
        j["element_specific_settings"]["plot_box_color"] = colorToJsonObj(plot_box_color);
    }

    if (grid_color != kGridColorDefault)
    {
        j["element_specific_settings"]["grid_color"] = colorToJsonObj(grid_color);
    }

    if (axes_numbers_color != kAxesNumbersColorDefault)
    {
        j["element_specific_settings"]["axes_numbers_color"] = colorToJsonObj(axes_numbers_color);
    }

    if (axes_letters_color != kAxesLettersColorDefault)
    {
        j["element_specific_settings"]["axes_letters_color"] = colorToJsonObj(axes_letters_color);
    }

    assignIfNotDefault(j, "element_specific_settings", "grid_on", grid_on, kGridOnDefault);
    assignIfNotDefault(j, "element_specific_settings", "plot_box_on", plot_box_on, kPlotBoxOnDefault);
    assignIfNotDefault(j, "element_specific_settings", "axes_numbers_on", axes_numbers_on, kAxesNumbersOnDefault);
    assignIfNotDefault(j, "element_specific_settings", "axes_letters_on", axes_letters_on, kAxesLettersOnDefault);
    assignIfNotDefault(j, "element_specific_settings", "clipping_on", clipping_on, kClippingOnDefault);
    assignIfNotDefault(j, "element_specific_settings", "snap_view_to_axes", snap_view_to_axes, kSnapViewToAxesDefault);
    assignIfNotDefault(j, "element_specific_settings", "pane_radius", pane_radius, kPaneRadiusDefault);

    if (projection_mode == ProjectionMode::PERSPECTIVE)
    {
        j["element_specific_settings"]["projection_mode"] = "perspective";
    }

    if (!subscribed_streams.empty())
    {
        for (const auto& ss : subscribed_streams)
        {
            j["element_specific_settings"]["subscribed_streams"].push_back(ss.toJson());
        }
    }

    return j;
}

SubscribedStreamSettings::SubscribedStreamSettings(const nlohmann::json& j)
{
    topic_id = j["topic_id"];

    const std::string stream_type_str = j["stream_type"];
    if (stream_type_str == "unknown")
    {
        stream_type = StreamType::UNKNOWN;
    }
    else if (stream_type_str == "plot")
    {
        stream_type = StreamType::PLOT;
    }
    else if (stream_type_str == "scatter")
    {
        stream_type = StreamType::SCATTER;
    }
    else if (stream_type_str == "plot3d")
    {
        stream_type = StreamType::PLOT3D;
    }
    else if (stream_type_str == "scatter3d")
    {
        stream_type = StreamType::SCATTER3D;
    }
    else if (stream_type_str == "stairs")
    {
        stream_type = StreamType::STAIRS;
    }
    else
    {
        throw std::runtime_error("Invalid option for \"stream_type\": \"" + stream_type_str + "\"");
    }

    if (j.contains("alpha"))
    {
        alpha = j["alpha"];
    }

    if (j.contains("line_width"))
    {
        line_width = j["line_width"];
    }

    if (j.contains("point_size"))
    {
        point_size = j["point_size"];
    }

    if (j.contains("color"))
    {
        const RGBTripletf rgbt{jsonObjToColor(j["color"])};
        color = lumos::properties::Color{static_cast<uint8_t>(rgbt.red * 255.0f),
                                           static_cast<uint8_t>(rgbt.green * 255.0f),
                                           static_cast<uint8_t>(rgbt.blue * 255.0f)};
    }

    if (j.contains("line_style"))
    {
        const std::string line_style_str = j["line_style"];

        if (line_style_str == "solid")
        {
            line_style = lumos::properties::LineStyle::SOLID;
        }
        else if (line_style_str == "dashed")
        {
            line_style = lumos::properties::LineStyle::DASHED;
        }
        else if (line_style_str == "short_dashed")
        {
            line_style = lumos::properties::LineStyle::SHORT_DASHED;
        }
        else if (line_style_str == "long_dashed")
        {
            line_style = lumos::properties::LineStyle::LONG_DASHED;
        }
        else
        {
            throw std::runtime_error("Invalid option for \"line_style\": \"" + line_style_str + "\"");
        }
    }

    if (j.contains("scatter_style"))
    {
        const std::string scatter_style_str = j["scatter_style"];

        if (scatter_style_str == "square")
        {
            scatter_style = lumos::properties::ScatterStyle::SQUARE;
        }
        else if (scatter_style_str == "circle")
        {
            scatter_style = lumos::properties::ScatterStyle::CIRCLE;
        }
        else if (scatter_style_str == "disc")
        {
            scatter_style = lumos::properties::ScatterStyle::DISC;
        }
        else if (scatter_style_str == "plus")
        {
            scatter_style = lumos::properties::ScatterStyle::PLUS;
        }
        else if (scatter_style_str == "cross")
        {
            scatter_style = lumos::properties::ScatterStyle::CROSS;
        }
        else
        {
            throw std::runtime_error("Invalid option for \"scatter_style\": \"" + scatter_style_str + "\"");
        }
    }

    if (j.contains("label"))
    {
        label = j["label"];
    }
}

nlohmann::json SubscribedStreamSettings::toJson() const
{
    nlohmann::json j;

    if (stream_type == StreamType::UNKNOWN)
    {
        j["stream_type"] = "unknown";
    }
    else if (stream_type == StreamType::PLOT)
    {
        j["stream_type"] = "plot";
    }
    else if (stream_type == StreamType::SCATTER)
    {
        j["stream_type"] = "scatter";
    }
    else if (stream_type == StreamType::PLOT3D)
    {
        j["stream_type"] = "plot3d";
    }
    else if (stream_type == StreamType::SCATTER3D)
    {
        j["stream_type"] = "scatter3d";
    }
    else if (stream_type == StreamType::STAIRS)
    {
        j["stream_type"] = "stairs";
    }
    else
    {
        throw std::runtime_error("Invalid option for \"stream_type\": \"" +
                                 std::to_string(static_cast<uint8_t>(stream_type)) + "\"");
    }

    if (line_style == lumos::properties::LineStyle::SOLID)
    {
        j["line_style"] = "solid";
    }
    else if (line_style == lumos::properties::LineStyle::DASHED)
    {
        j["line_style"] = "dashed";
    }
    else if (line_style == lumos::properties::LineStyle::SHORT_DASHED)
    {
        j["line_style"] = "short_dashed";
    }
    else if (line_style == lumos::properties::LineStyle::LONG_DASHED)
    {
        j["line_style"] = "LONG_DASHED";
    }

    else if (scatter_style == lumos::properties::ScatterStyle::SQUARE)
    {
        j["scatter_style"] = "square";
    }
    else if (scatter_style == lumos::properties::ScatterStyle::CIRCLE)
    {
        j["scatter_style"] = "circle";
    }
    else if (scatter_style == lumos::properties::ScatterStyle::DISC)
    {
        j["scatter_style"] = "disc";
    }
    else if (scatter_style == lumos::properties::ScatterStyle::PLUS)
    {
        j["scatter_style"] = "plus";
    }
    else if (scatter_style == lumos::properties::ScatterStyle::CROSS)
    {
        j["scatter_style"] = "cross";
    }

    j["topic_id"] = topic_id;
    j["alpha"] = alpha;
    j["line_width"] = line_width;
    j["point_size"] = point_size;
    j["color"] = colorToJsonObj(RGBTripletf{static_cast<float>(color.value().red) / 255.0f,
                                            static_cast<float>(color.value().green) / 255.0f,
                                            static_cast<float>(color.value().blue) / 255.0f});
    j["label"] = label;

    return j;
}

bool SubscribedStreamSettings::operator==(const SubscribedStreamSettings& other) const
{
    return other.topic_id == topic_id && other.stream_type == stream_type && other.alpha == alpha &&
           other.line_width == line_width && other.point_size == point_size && other.color == color &&
           other.line_style == line_style && other.scatter_style == scatter_style && other.label == label;
}
