#include "main_application/plot_objects/plot_collection3/plot_collection3.h"

#include "outer_converter.h"

namespace
{
struct ConvertedData : ConvertedDataBase
{
    float* data_ptr;
    Vec3d min_vec;
    Vec3d max_vec;
    uint32_t num_points;

    ConvertedData() : data_ptr{nullptr}, min_vec{-1.0, -1.0, -1.0}, max_vec{1.0, 1.0, 1.0}, num_points{0U} {}

    ConvertedData(const ConvertedData& other) = delete;
    ConvertedData& operator=(const ConvertedData& other) = delete;
    ConvertedData(ConvertedData&& other) = delete;
    ConvertedData& operator=(ConvertedData&& other) = delete;

    ~ConvertedData() override
    {
        delete[] data_ptr;
    }
};

struct InputParams
{
    size_t num_objects;
    size_t num_bytes_per_element;
    size_t num_points;
    Vector<uint16_t> vector_lengths;

    InputParams() = default;
    InputParams(const size_t num_objects_,
                const size_t num_bytes_per_element_,
                const size_t num_points_,
                const Vector<uint16_t>& vector_lengths_)
        : num_objects{num_objects_},
          num_bytes_per_element{num_bytes_per_element_},
          num_points{num_points_},
          vector_lengths{vector_lengths_}
    {
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

template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data, const InputParams& input_params)
{
    const size_t total_num_bytes = input_params.num_points * 3 * input_params.num_bytes_per_element;
    const size_t num_bytes_per_collection = input_params.vector_lengths.sum() * input_params.num_bytes_per_element;

    const T* data_x = reinterpret_cast<const T*>(input_data);
    const T* data_y = reinterpret_cast<const T*>(input_data + num_bytes_per_collection);
    const T* data_z = reinterpret_cast<const T*>(input_data + 2 * num_bytes_per_collection);

    ConvertedData* converted_data = new ConvertedData;
    converted_data->data_ptr = new float[total_num_bytes];
    converted_data->num_points = input_params.num_points;

    size_t idx_offset = 0;
    size_t idx = 0;

    Vec3d min_vec, max_vec;

    min_vec.x = data_x[0];
    min_vec.y = data_y[0];
    min_vec.z = data_z[0];

    max_vec.x = data_x[0];
    max_vec.y = data_y[0];
    max_vec.z = data_z[0];

    for (size_t i = 0; i < input_params.num_objects; i++)
    {
        for (size_t k = 0; k < input_params.vector_lengths(i) - 1; k++)
        {
            const T x_val = data_x[idx_offset + k];
            const T y_val = data_y[idx_offset + k];
            const T z_val = data_z[idx_offset + k];

            min_vec.x = x_val < min_vec.x ? x_val : min_vec.x;
            min_vec.y = y_val < min_vec.y ? y_val : min_vec.y;
            min_vec.z = z_val < min_vec.z ? z_val : min_vec.z;

            max_vec.x = x_val > max_vec.x ? x_val : max_vec.x;
            max_vec.y = y_val > max_vec.y ? y_val : max_vec.y;
            max_vec.z = z_val > max_vec.z ? z_val : max_vec.z;

            converted_data->data_ptr[idx] = x_val;
            converted_data->data_ptr[idx + 1] = y_val;
            converted_data->data_ptr[idx + 2] = z_val;

            converted_data->data_ptr[idx + 3] = data_x[idx_offset + k + 1];
            converted_data->data_ptr[idx + 4] = data_y[idx_offset + k + 1];
            converted_data->data_ptr[idx + 5] = data_z[idx_offset + k + 1];

            idx += 6;
        }
        idx_offset += input_params.vector_lengths(i);
    }

    converted_data->min_vec = min_vec;
    converted_data->max_vec = max_vec;

    return std::shared_ptr<const ConvertedData>(converted_data);
}

}  // namespace

PlotCollection3D::PlotCollection3D(const CommunicationHeader& hdr,
                                   ReceivedData& received_data,
                                   const std::shared_ptr<const ConvertedDataBase>& converted_data,
                                   const PlotObjectAttributes& plot_object_attributes,
                                   const UserSuppliedProperties& user_supplied_properties,
                                   const ShaderCollection& shader_collection,
                                   ColorPicker& color_picker)
    : PlotObjectBase(received_data, hdr, plot_object_attributes, user_supplied_properties, shader_collection, color_picker),
      vertex_buffer_{OGLPrimitiveType::LINES}
{
    if (function_ != Function::PLOT_COLLECTION3)
    {
        throw std::runtime_error("Invalid function type for PlotCollection3D!");
    }

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    num_points_ = converted_data_local->num_points;

    min_vec_ = converted_data_local->min_vec;
    max_vec_ = converted_data_local->max_vec;

    vertex_buffer_.addBuffer(converted_data_local->data_ptr, num_points_, 3);
}

std::shared_ptr<const ConvertedDataBase> PlotCollection3D::convertRawData(const CommunicationHeader& hdr,
                                                                          const PlotObjectAttributes& attributes,
                                                                          const UserSuppliedProperties& user_supplied_properties,
                                                                          const uint8_t* const data_ptr)

{
    const uint8_t* data_ptr_local = data_ptr;

    Vector<uint16_t> vector_lengths(attributes.num_objects);

    std::memcpy(vector_lengths.data(), data_ptr, attributes.num_objects * sizeof(uint16_t));

    uint32_t num_points = 0U;

    for (size_t k = 0; k < attributes.num_objects; k++)
    {
        num_points += (vector_lengths(k) - 1) * 2;
    }

    // Advance pointer to account for first bytes where 'vector_lengths' are stored
    data_ptr_local += attributes.num_objects * sizeof(uint16_t);

    const InputParams input_params{
        attributes.num_objects, attributes.num_bytes_per_element, num_points, vector_lengths};

    std::shared_ptr<const ConvertedDataBase> converted_data_base{
        applyConverter<ConvertedData>(data_ptr_local, attributes.data_type, Converter{}, input_params)};

    return converted_data_base;
}

void PlotCollection3D::findMinMax()
{
    // Already calculated in constructor phase
}

void PlotCollection3D::render()
{
    shader_collection_.basic_plot_shader.use();
    vertex_buffer_.render(num_points_);
}

PlotCollection3D::~PlotCollection3D() {}
