#include "main_application/plot_objects/scrolling_plot2d/scrolling_plot2d.h"

#include "outer_converter.h"

namespace
{
struct ConvertedData : ConvertedDataBase
{
    float dt;
    float x;

    ConvertedData() : dt{0.0f}, x{0.0f} {}

    ConvertedData(const ConvertedData& other) = delete;
    ConvertedData& operator=(const ConvertedData& other) = delete;
    ConvertedData(ConvertedData&& other) = delete;
    ConvertedData& operator=(ConvertedData&& other) = delete;

    ~ConvertedData() override {}
};

struct InputParams
{
    InputParams() = default;
};

template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data, const InputParams& input_params);

struct Converter
{
    template <class T>
    std::shared_ptr<const ConvertedData> convert(const uint8_t* const input_data, const InputParams& input_params) const
    {
        return convertData<T>(input_data, input_params);
    }
};

}  // namespace

ScrollingPlot2D::ScrollingPlot2D(const CommunicationHeader& hdr,
                                 ReceivedData& received_data,
                                 const std::shared_ptr<const ConvertedDataBase>& converted_data,

                                 const PlotObjectAttributes& plot_object_attributes,
                                 const UserSuppliedProperties& user_supplied_properties,
                                 const ShaderCollection& shader_collection,
                                 ColorPicker& color_picker)
    : PlotObjectBase(received_data, hdr, plot_object_attributes, user_supplied_properties, shader_collection, color_picker)
{
    if (function_ != Function::REAL_TIME_PLOT)
    {
        throw std::runtime_error("Invalid function type for ScrollingPlot2D!");
    }

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    num_elements_to_draw_ = 1U;

    points_ptr_ = new float[buffer_size_ * 2U];
    dt_vec_ = new float[buffer_size_];

    const size_t num_bytes = buffer_size_ * 2U * sizeof(float);
    std::memset(points_ptr_, 0, num_bytes);
    std::memset(dt_vec_, 0, num_bytes / 2U);
    previous_buffer_size_ = buffer_size_;

    points_ptr_[1U] = converted_data_local->x;

    glGenVertexArrays(1, &sp_vertex_buffer_array_);
    glBindVertexArray(sp_vertex_buffer_array_);

    glGenBuffers(1, &sp_vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, sp_vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, num_bytes, points_ptr_, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    is_updateable_ = true;  // TODO: Temporary hack
}

void ScrollingPlot2D::findMinMax()
{
    min_vec_.x = -1.0;
    min_vec_.y = -1.0;
    min_vec_.z = -1.0;

    max_vec_.x = 1.0;
    max_vec_.y = 1.0;
    max_vec_.z = 1.0;
}

void ScrollingPlot2D::render()
{
    // preRender(&shader_collection_.basic_plot_shader);
    shader_collection_.basic_plot_shader.use();
    glBindVertexArray(sp_vertex_buffer_array_);
    glDrawArrays(GL_LINE_STRIP, 0, num_elements_to_draw_);
    glBindVertexArray(0);
}

ScrollingPlot2D::~ScrollingPlot2D()
{
    delete[] points_ptr_;
    delete[] dt_vec_;

    glDeleteBuffers(1, &sp_vertex_buffer_);
    glDeleteVertexArrays(1, &sp_vertex_buffer_array_);
}

LegendProperties ScrollingPlot2D::getLegendProperties() const
{
    LegendProperties lp{PlotObjectBase::getLegendProperties()};
    lp.type = LegendType::LINE;
    lp.color = color_;

    return lp;
}

std::shared_ptr<const ConvertedDataBase> ScrollingPlot2D::convertRawData(const CommunicationHeader& hdr,
                                                                         const PlotObjectAttributes& attributes,
                                                                         const UserSuppliedProperties& user_supplied_properties,
                                                                         const uint8_t* const data_ptr)
{
    const InputParams input_params{};

    std::shared_ptr<const ConvertedDataBase> converted_data_base{
        applyConverter<ConvertedData>(data_ptr, attributes.data_type, Converter{}, input_params)};

    return converted_data_base;
}

void ScrollingPlot2D::updateWithNewData(ReceivedData& received_data,
                                        const CommunicationHeader& hdr,
                                        const std::shared_ptr<const ConvertedDataBase>& converted_data,
                                        const UserSuppliedProperties& user_supplied_properties)
{
    static_cast<void>(hdr);

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    if (user_supplied_properties.hasProperties())
    {
        updateProperties(user_supplied_properties);
    }

    if (previous_buffer_size_ != buffer_size_)
    {
        float* points_ptr_tmp = new float[buffer_size_ * 2U];
        float* dt_vec_tmp = new float[buffer_size_];

        const size_t num_bytes = buffer_size_ * 2U * sizeof(float);

        std::memset(points_ptr_tmp, 0, num_bytes);
        std::memset(dt_vec_tmp, 0, num_bytes / 2U);

        std::memcpy(points_ptr_tmp, points_ptr_, sizeof(float) * buffer_size_ * 2U);
        std::memcpy(dt_vec_tmp, dt_vec_, sizeof(float) * buffer_size_);

        delete[] points_ptr_;
        delete[] dt_vec_;

        points_ptr_ = points_ptr_tmp;
        dt_vec_ = dt_vec_tmp;

        glDeleteBuffers(1, &sp_vertex_buffer_);
        glDeleteVertexArrays(1, &sp_vertex_buffer_array_);

        glGenVertexArrays(1, &sp_vertex_buffer_array_);
        glBindVertexArray(sp_vertex_buffer_array_);

        glGenBuffers(1, &sp_vertex_buffer_);
        glBindBuffer(GL_ARRAY_BUFFER, sp_vertex_buffer_);
        glBufferData(GL_ARRAY_BUFFER, num_bytes, points_ptr_, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        previous_buffer_size_ = buffer_size_;
    }

    data_ptr_ = received_data.payloadData();

    num_elements_to_draw_ = (num_elements_to_draw_ + 1U) > buffer_size_ ? buffer_size_ : (num_elements_to_draw_ + 1U);
    int idx = 0;
    const int num_samples_2 = buffer_size_ * 2;
    const int num_samples = buffer_size_;
    for (size_t k = 0; k < (buffer_size_ - 1U); k++)
    {
        points_ptr_[num_samples_2 - idx - 1] = points_ptr_[num_samples_2 - idx - 3];
        idx += 2;
    }

    idx = 0;
    for (size_t k = 0; k < (buffer_size_ - 1U); k++)
    {
        dt_vec_[num_samples - 1 - idx] = dt_vec_[num_samples - 2 - idx];
        idx += 1;
    }

    dt_vec_[0U] = converted_data_local->dt;

    idx = 0;
    for (size_t k = 0; k < buffer_size_; k++)
    {
        points_ptr_[idx + 2] = points_ptr_[idx] + dt_vec_[k];
        idx += 2;
    }

    points_ptr_[1U] = converted_data_local->x;

    const size_t num_bytes_to_replace = buffer_size_ * 2U * sizeof(float);

    glBindBuffer(GL_ARRAY_BUFFER, sp_vertex_buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_bytes_to_replace, points_ptr_);
    is_updateable_ = true;  // TODO: Temporary hack
}

namespace
{

template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data, const InputParams& input_params)
{
    static_cast<void>(input_params);

    T dt, x;
    uint8_t* dt_ptr = reinterpret_cast<uint8_t*>(&dt);
    uint8_t* x_ptr = reinterpret_cast<uint8_t*>(&x);

    std::memcpy(dt_ptr, input_data, sizeof(T));
    std::memcpy(x_ptr, input_data + sizeof(T), sizeof(T));

    ConvertedData* converted_data = new ConvertedData;
    converted_data->dt = dt;
    converted_data->x = x;

    return std::shared_ptr<const ConvertedData>(converted_data);
}

}  // namespace
