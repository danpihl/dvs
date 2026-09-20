#include "main_application/plot_objects/screen_space_primitive/screen_space_primitive.h"

#include "outer_converter.h"

namespace
{

struct InputParams
{
    size_t num_elements;

    InputParams() = default;
    InputParams(const size_t num_elements_)
        : num_elements{num_elements_}
    {
    }
};

struct ConvertedData : ConvertedDataBase
{
    float* points_ptr;
    size_t num_elements;

    ConvertedData() : points_ptr{nullptr} {}

    ConvertedData(const ConvertedData& other) = delete;
    ConvertedData& operator=(const ConvertedData& other) = delete;
    ConvertedData(ConvertedData&& other) = delete;
    ConvertedData& operator=(ConvertedData&& other) = delete;

    ~ConvertedData() override
    {
        delete[] points_ptr;
    }
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

ScreenSpacePrimitive::ScreenSpacePrimitive(const CommunicationHeader& hdr,
                     ReceivedData& received_data,
                     const std::shared_ptr<const ConvertedDataBase>& converted_data,
                     const PlotObjectAttributes& plot_object_attributes,
                     const UserSuppliedProperties& user_supplied_properties,
                     const ShaderCollection& shader_collection,
                     ColorPicker& color_picker)
    : PlotObjectBase(received_data, hdr, plot_object_attributes, user_supplied_properties, shader_collection, color_picker),
      vertex_buffer_{OGLPrimitiveType::TRIANGLES}
{
    if (function_ != Function::SCREEN_SPACE_PRIMITIVE)
    {
        throw std::runtime_error("Invalid function type for ScreenSpacePrimitive!");
    }

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    vertex_buffer_.addBuffer(converted_data_local->points_ptr, num_elements_ * 3U, 2U);
}

std::shared_ptr<const ConvertedDataBase> ScreenSpacePrimitive::convertRawData(const CommunicationHeader& hdr,
                                                                   const PlotObjectAttributes& attributes,
                                                                   const UserSuppliedProperties& user_supplied_properties,
                                                                   const uint8_t* const data_ptr)
{
    const InputParams input_params{attributes.num_elements};

    std::shared_ptr<const ConvertedDataBase> converted_data_base{
        applyConverter<ConvertedData>(data_ptr, attributes.data_type, Converter{}, input_params)};

    return converted_data_base;
}

void ScreenSpacePrimitive::modifyShader()
{
    PlotObjectBase::modifyShader();
    shader_collection_.screen_space_shader.use();
    shader_collection_.screen_space_shader.base_uniform_handles.vertex_color.setColor(color_);
}

void ScreenSpacePrimitive::findMinMax()
{
    min_vec_ = Vec3d{-1.0, -1.0, -1.0};
    max_vec_ = Vec3d{1.0, 1.0, 1.0};
}

void ScreenSpacePrimitive::render()
{
    shader_collection_.screen_space_shader.use();
    vertex_buffer_.render(num_elements_ * 3U);
}

ScreenSpacePrimitive::~ScreenSpacePrimitive() {}

namespace
{
template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data, const InputParams& input_params)
{
    ConvertedData* converted_data = new ConvertedData;

    converted_data->points_ptr = new float[input_params.num_elements * 3U * 2U];
    converted_data->num_elements = input_params.num_elements;

    using Triangle = std::array<Point2f, 3U>;

    const Triangle* const tp = reinterpret_cast<const Triangle* const>(input_data);

    const VectorConstView<Triangle> triangles{reinterpret_cast<const Triangle* const>(input_data), input_params.num_elements};

    size_t points_idx{0U};
    for(size_t k = 0; k < input_params.num_elements; k++)
    {
        const Triangle& t = triangles(k);
        converted_data->points_ptr[points_idx] = t[0].x;
        converted_data->points_ptr[points_idx + 1] = t[0].y;

        converted_data->points_ptr[points_idx + 2] = t[1].x;
        converted_data->points_ptr[points_idx + 3] = t[1].y;

        converted_data->points_ptr[points_idx + 4] = t[2].x;
        converted_data->points_ptr[points_idx + 5] = t[2].y;

        points_idx += 6U;
    }

    // std::memcpy(converted_data->points_ptr, input_data, input_params.num_elements * 3U * 2U * sizeof(float));

    return std::shared_ptr<const ConvertedData>{converted_data};
}

}  // namespace
