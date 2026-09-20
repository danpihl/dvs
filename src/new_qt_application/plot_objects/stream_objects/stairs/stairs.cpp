#include "plot_objects/stream_objects/stairs/stairs.h"

#include "plot_objects/stream_objects/conversion_function.h"

StairsStream::StairsStream() {}

constexpr size_t kStreamBufferSize{500U};
constexpr size_t kTotalNumPoints{kStreamBufferSize * 2U - 1U};

StairsStream::StairsStream(const SubscribedStreamSettings& subscribed_stream_settings,
                           const ShaderCollection& shader_collection)
    : StreamObjectBase(subscribed_stream_settings, shader_collection)
{
    num_elements_to_draw_ = 0U;

    points_ptr_ = new TimeAndValue[kTotalNumPoints];

    const size_t num_bytes = kTotalNumPoints * 2U * sizeof(float);
    std::memset(points_ptr_, 0, num_bytes);

    glGenVertexArrays(1, &sp_vertex_buffer_array_);
    glBindVertexArray(sp_vertex_buffer_array_);

    glGenBuffers(1, &sp_vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, sp_vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, num_bytes, points_ptr_, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if (subscribed_stream_settings.color.has_value())
    {
        color_ = RGBTripletf(static_cast<float>(subscribed_stream_settings.color.value().red) / 255.0f,
                             static_cast<float>(subscribed_stream_settings.color.value().green) / 255.0f,
                             static_cast<float>(subscribed_stream_settings.color.value().blue) / 255.0f);
    }
    else
    {
        color_ = RGBTripletf(0.0f, 0.0f, 0.0f);
    }
}

void StairsStream::appendNewData(const std::vector<std::shared_ptr<objects::BaseObject>>& obj)
{
    for (size_t k = 0; k < obj.size(); k++)
    {
        appendNewData(obj[k]);
    }
}

void StairsStream::appendNewData(const std::shared_ptr<objects::BaseObject>& obj)
{
    num_elements_to_draw_ =
        (num_elements_to_draw_ + 2U) > kTotalNumPoints ? kTotalNumPoints : (num_elements_to_draw_ + 2U);

    const uint64_t current_timestamp = obj->timestamp();

    float dt;

    constexpr float kDtScalingFactor = 0.001f;  // The timestamp received from the device is in integer milliseconds

    if (current_timestamp >= previous_timestamp_)
    {
        dt = kDtScalingFactor * static_cast<float>(current_timestamp - previous_timestamp_);
    }
    else
    {
        // Overflow has occured
        constexpr uint64_t kMaxTimestamp = 0xFFFFFFFFU;  // The timestamp received from the device is 32-bit
        dt = kDtScalingFactor * static_cast<float>((kMaxTimestamp - previous_timestamp_) + current_timestamp);
    }

    int64_t idx = static_cast<int64_t>(kTotalNumPoints) - 1;

    for (size_t k = 0; k < (kTotalNumPoints - 2U); k++)
    {
        points_ptr_[idx].value = points_ptr_[idx - 2].value;
        points_ptr_[idx].t = points_ptr_[idx - 2].t + dt;

        idx -= 1;
    }

    const float value = getFloatValue(obj);

    points_ptr_[1].t = dt;
    points_ptr_[1].value = value;

    points_ptr_[0].t = 0.0f;
    points_ptr_[0].value = value;

    const size_t num_bytes_to_replace = kTotalNumPoints * 2U * sizeof(float);

    glBindBuffer(GL_ARRAY_BUFFER, sp_vertex_buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_bytes_to_replace, reinterpret_cast<float*>(points_ptr_));

    previous_timestamp_ = current_timestamp;
}

void StairsStream::render()
{
    shader_collection_.basic_plot_shader.use();

    shader_collection_.basic_plot_shader.base_uniform_handles.vertex_color.setColor(color_);

    glBindVertexArray(sp_vertex_buffer_array_);
    glDrawArrays(GL_LINE_STRIP, 0, num_elements_to_draw_);
    glBindVertexArray(0);
}

void StairsStream::clear()
{
    num_elements_to_draw_ = 0U;
}
