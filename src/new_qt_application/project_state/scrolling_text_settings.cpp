#include "project_state/project_settings.h"

ScrollingTextSettings::ScrollingTextSettings() : ElementSettings{}, title{"<NO-NAME>"}, subscribed_streams{}
{
    x = 0.0;
    y = 0.0;
    width = 0.4;
    height = 0.4;
    type = lumos::GuiElementType::ScrollingText;
}

ScrollingTextSettings::ScrollingTextSettings(const nlohmann::json& j) : ElementSettings{j}
{
    if (!j.contains("element_specific_settings"))
    {
        return;
    }

    nlohmann::json j_ess = j["element_specific_settings"];

    if (j_ess.contains("subscribed_streams"))
    {
        for (size_t k = 0; k < j_ess["subscribed_streams"].size(); k++)
        {
            subscribed_streams.push_back(SubscribedTextStreamSettings(j_ess["subscribed_streams"][k]));
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

    if (j_ess.contains("print_timestamp"))
    {
        print_timestamp = j_ess["print_timestamp"];
    }
    else
    {
        print_timestamp = true;
    }

    if (j_ess.contains("print_topic_id"))
    {
        print_topic_id = j_ess["print_topic_id"];
    }
    else
    {
        print_topic_id = true;
    }
}

bool ScrollingTextSettings::operator==(const ScrollingTextSettings& other) const
{
    return ElementSettings::operator==(other) && title == other.title && subscribed_streams == other.subscribed_streams;
}

bool ScrollingTextSettings::operator!=(const ScrollingTextSettings& other) const
{
    return !(*this == other);
}

nlohmann::json ScrollingTextSettings::toJson() const
{
    nlohmann::json j;

    if (!subscribed_streams.empty())
    {
        for (const auto& ss : subscribed_streams)
        {
            j["element_specific_settings"]["subscribed_streams"].push_back(ss.toJson());
        }
    }

    if (!print_timestamp)
    {
        j["element_specific_settings"]["print_timestamp"] = print_timestamp;
    }

    if (!print_topic_id)
    {
        j["element_specific_settings"]["print_topic_id"] = print_topic_id;
    }

    if (title != "")
    {
        j["title"] = title;
    }

    return j;
}

/****************************************************************************
 *********************** SubscribedTextStreamSettings ***********************
 ****************************************************************************/

SubscribedTextStreamSettings::SubscribedTextStreamSettings() {}

SubscribedTextStreamSettings::SubscribedTextStreamSettings(const nlohmann::json& j)
{
    if (j.contains("topic_id"))
    {
        topic_id = j["topic_id"];
    }

    if (j.contains("text_color"))
    {
        text_color = jsonObjToColor(j["text_color"]);
    }
}

nlohmann::json SubscribedTextStreamSettings::toJson() const
{
    nlohmann::json j;

    j["topic_id"] = topic_id;
    if (text_color.has_value())
    {
        j["text_color"] = colorToJsonObj(text_color.value());
    }

    return j;
}

bool SubscribedTextStreamSettings::operator==(const SubscribedTextStreamSettings& other) const
{
    bool color_is_same = true;

    if (text_color.has_value() && other.text_color.has_value())
    {
        // Both have a value
        color_is_same = text_color.value() == other.text_color.value();
    }
    else if (text_color.has_value() != other.text_color.has_value())
    {
        // One has a value, the other doesn't
        color_is_same = false;
    }
    else
    {
        // Neither have a value
        color_is_same = true;
    }

    return color_is_same && topic_id == other.topic_id;
}

bool SubscribedTextStreamSettings::operator!=(const SubscribedTextStreamSettings& other) const
{
    return !(*this == other);
}
