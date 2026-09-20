#include "main_application/plot_objects/stem/stem.h"

#include "outer_converter.h"

namespace
{
struct ConvertedData : ConvertedDataBase
{
    float* lines_data;
    float* points_data;

    ConvertedData() : lines_data{nullptr}, points_data{nullptr} {}

    ConvertedData(const ConvertedData& other) = delete;
    ConvertedData& operator=(const ConvertedData& other) = delete;
    ConvertedData(ConvertedData&& other) = delete;
    ConvertedData& operator=(ConvertedData&& other) = delete;

    ~ConvertedData() override
    {
        delete[] lines_data;
        delete[] points_data;
    }
};

struct InputParams
{
    size_t num_elements;

    InputParams() = default;
    InputParams(const size_t num_elements_) : num_elements{num_elements_} {}
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

Stem::Stem(const CommunicationHeader& hdr,
           ReceivedData& received_data,
           const std::shared_ptr<const ConvertedDataBase>& converted_data,
           const PlotObjectAttributes& plot_object_attributes,
           const UserSuppliedProperties& user_supplied_properties,
           const ShaderCollection& shader_collection,
           ColorPicker& color_picker)
    : PlotObjectBase(received_data, hdr, plot_object_attributes, user_supplied_properties, shader_collection, color_picker),
      vertex_buffer_lines_{OGLPrimitiveType::LINES},
      vertex_buffer_points_{OGLPrimitiveType::POINTS}
{
    if (function_ != Function::STEM)
    {
        throw std::runtime_error("Invalid function type for Stem!");
    }

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    vertex_buffer_lines_.addBuffer(converted_data_local->lines_data, num_elements_ * 2, 2);
    vertex_buffer_points_.addBuffer(converted_data_local->points_data, num_elements_, 2);
}

void Stem::findMinMax()
{
    Vec2d min_vec_2d, max_vec_2d;
    std::tie<Vec2d, Vec2d>(min_vec_2d, max_vec_2d) =
        findMinMaxFromTwoVectors(data_ptr_, num_elements_, num_bytes_for_one_vec_, data_type_);

    min_vec_.x = min_vec_2d.x;
    min_vec_.y = min_vec_2d.y;
    min_vec_.z = -1.0;

    max_vec_.x = max_vec_2d.x;
    max_vec_.y = max_vec_2d.y;
    max_vec_.z = 1.0;
}

void Stem::render()
{
    shader_collection_.basic_plot_shader.use();
    vertex_buffer_lines_.render(num_elements_ * 2);

    shader_collection_.scatter_shader.use();
    vertex_buffer_points_.render(num_elements_);
}

void Stem::modifyShader()
{
    PlotObjectBase::modifyShader();
    shader_collection_.scatter_shader.use();
    shader_collection_.scatter_shader.base_uniform_handles.point_size.setFloat(10.0f);
    shader_collection_.scatter_shader.base_uniform_handles.scatter_mode.setInt(2);
}

std::shared_ptr<const ConvertedDataBase> Stem::convertRawData(const CommunicationHeader& hdr,
                                                              const PlotObjectAttributes& attributes,
                                                              const UserSuppliedProperties& user_supplied_properties,
                                                              const uint8_t* const data_ptr)
{
    const InputParams input_params{attributes.num_elements};

    std::shared_ptr<const ConvertedDataBase> converted_data_base{
        applyConverter<ConvertedData>(data_ptr, attributes.data_type, Converter{}, input_params)};

    return converted_data_base;
}

Stem::~Stem() {}

LegendProperties Stem::getLegendProperties() const
{
    LegendProperties lp{PlotObjectBase::getLegendProperties()};
    lp.type = LegendType::LINE;
    lp.color = color_;

    return lp;
}

namespace
{
template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data, const InputParams& input_params)
{
    ConvertedData* converted_data = new ConvertedData;

    converted_data->lines_data = new float[4 * input_params.num_elements];
    converted_data->points_data = new float[2 * input_params.num_elements];

    size_t lines_idx = 0, points_idx = 0;

    const T* const input_data_dt_x = reinterpret_cast<const T* const>(input_data);
    const T* const input_data_dt_y = input_data_dt_x + input_params.num_elements;

    for (size_t k = 0; k < input_params.num_elements; k++)
    {
        const T data_x = input_data_dt_x[k];
        const T data_y = input_data_dt_y[k];

        converted_data->lines_data[lines_idx] = data_x;
        converted_data->lines_data[lines_idx + 1] = 0.0f;
        converted_data->lines_data[lines_idx + 2] = data_x;
        converted_data->lines_data[lines_idx + 3] = data_y;

        converted_data->points_data[points_idx] = data_x;
        converted_data->points_data[points_idx + 1] = data_y;

        lines_idx += 4;
        points_idx += 2;
    }

    return std::shared_ptr<const ConvertedData>{converted_data};
}

}  // namespace