#include "main_application/plot_objects/fast_plot3d/fast_plot3d.h"

#include "outer_converter.h"

namespace
{
struct ConvertedData : ConvertedDataBase
{
    float* points_ptr;

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

FastPlot3D::FastPlot3D(const CommunicationHeader& hdr,
                       ReceivedData& received_data,
                       const std::shared_ptr<const ConvertedDataBase>& converted_data,
                       const PlotObjectAttributes& plot_object_attributes,
                       const UserSuppliedProperties& user_supplied_properties,
                       const ShaderCollection& shader_collection,
                       ColorPicker& color_picker)
    : PlotObjectBase(received_data, hdr, plot_object_attributes, user_supplied_properties, shader_collection, color_picker),
      vertex_buffer_{OGLPrimitiveType::LINE_STRIP}
{
    if (function_ != Function::FAST_PLOT3)
    {
        throw std::runtime_error("Invalid function type for FastPlot3D!");
    }

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    vertex_buffer_.addBuffer(converted_data_local->points_ptr, num_elements_, 3);
}

std::shared_ptr<const ConvertedDataBase> FastPlot3D::convertRawData(const CommunicationHeader& hdr,
                                                                    const PlotObjectAttributes& attributes,
                                                                    const UserSuppliedProperties& user_supplied_properties,
                                                                    const uint8_t* const data_ptr)
{
    const InputParams input_params{attributes.num_elements};

    std::shared_ptr<const ConvertedDataBase> converted_data_base{
        applyConverter<ConvertedData>(data_ptr, attributes.data_type, Converter{}, input_params)};

    return converted_data_base;
}

void FastPlot3D::findMinMax()
{
    std::tie<Vec3d, Vec3d>(min_vec_, max_vec_) =
        findMinMaxFromThreeVectors(data_ptr_, num_elements_, num_bytes_for_one_vec_, data_type_);
}

void FastPlot3D::render()
{
    shader_collection_.basic_plot_shader.use();
    vertex_buffer_.render(num_elements_);
}

FastPlot3D::~FastPlot3D() {}

namespace
{
template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data, const InputParams& input_params)
{
    ConvertedData* converted_data = new ConvertedData;
    converted_data->points_ptr = new float[3 * input_params.num_elements];

    size_t idx = 0;

    const T* const x_data = reinterpret_cast<const T* const>(input_data);
    const T* const y_data = reinterpret_cast<const T* const>(input_data) + input_params.num_elements;
    const T* const z_data = reinterpret_cast<const T* const>(input_data) + 2 * input_params.num_elements;

    for (size_t k = 0; k < input_params.num_elements; k++)
    {
        converted_data->points_ptr[idx] = x_data[k];
        converted_data->points_ptr[idx + 1] = y_data[k];
        converted_data->points_ptr[idx + 2] = z_data[k];

        idx += 3;
    }

    return std::shared_ptr<const ConvertedData>(converted_data);
}

}  // namespace
