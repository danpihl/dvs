#include "main_application/plot_objects/scatter/scatter.h"

#include "outer_converter.h"

namespace
{
struct ConvertedData : ConvertedDataBase
{
    float* points_ptr;
    float* color_data;
    float* point_sizes_data;

    ConvertedData() : points_ptr{nullptr}, color_data{nullptr}, point_sizes_data{nullptr} {}

    ConvertedData(const ConvertedData& other) = delete;
    ConvertedData& operator=(const ConvertedData& other) = delete;
    ConvertedData(ConvertedData&& other) = delete;
    ConvertedData& operator=(ConvertedData&& other) = delete;

    ~ConvertedData() override
    {
        delete[] points_ptr;
        delete[] color_data;
        delete[] point_sizes_data;
    }
};

struct InputParams
{
    size_t num_elements;
    size_t num_bytes_per_element;
    size_t num_bytes_for_one_vec;
    bool has_color;
    bool has_point_sizes;
    float z_offset;

    InputParams() = default;
    InputParams(const size_t num_elements_,
                const size_t num_bytes_per_element_,
                const size_t num_bytes_for_one_vec_,
                const bool has_color_,
                const bool has_points_sizes_,
                float z_offset_)
        : num_elements{num_elements_},
          num_bytes_per_element{num_bytes_per_element_},
          num_bytes_for_one_vec{num_bytes_for_one_vec_},
          has_color{has_color_},
          has_point_sizes{has_points_sizes_},
          z_offset{z_offset_}
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

}  // namespace

Scatter2D::Scatter2D(const CommunicationHeader& hdr,
                     ReceivedData& received_data,
                     const std::shared_ptr<const ConvertedDataBase>& converted_data,
                     const PlotObjectAttributes& plot_object_attributes,
                     const UserSuppliedProperties& user_supplied_properties,
                     const ShaderCollection& shader_collection,
                     ColorPicker& color_picker)
    : PlotObjectBase(received_data, hdr, plot_object_attributes, user_supplied_properties, shader_collection, color_picker),
      vertex_buffer_{OGLPrimitiveType::POINTS}
{
    if (function_ != Function::SCATTER2)
    {
        throw std::runtime_error("Invalid function type for Scatter2D!");
    }

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    num_added_elements_ = 0;

    if (user_supplied_properties.is_appendable)
    {
        vertex_buffer_.addExpandableBuffer<float>(user_supplied_properties.buffer_size.value_or(kDefaultBufferSize), 3);

        vertex_buffer_.updateBufferData(0U, converted_data_local->points_ptr, num_elements_, 3U, num_added_elements_);

        if (has_color_)
        {
            vertex_buffer_.addExpandableBuffer<float>(user_supplied_properties.buffer_size.value_or(kDefaultBufferSize), 3);
            vertex_buffer_.updateBufferData(
                1U, converted_data_local->color_data, num_elements_, 3U, num_added_elements_);
        }

        num_added_elements_ += num_elements_;
    }
    else
    {
        vertex_buffer_.addBuffer(converted_data_local->points_ptr, num_elements_, 3);

        num_added_elements_ = num_elements_;

        if (has_color_)
        {
            vertex_buffer_.addBuffer(converted_data_local->color_data, num_elements_, 3);
        }

        if (has_point_sizes_)
        {
            vertex_buffer_.addBuffer(converted_data_local->point_sizes_data, num_elements_, 1, GL_STATIC_DRAW, 2);
        }
    }
}

void Scatter2D::appendNewData(ReceivedData& received_data,
                              const CommunicationHeader& hdr,
                              const std::shared_ptr<const ConvertedDataBase>& converted_data,
                              const UserSuppliedProperties& user_supplied_properties)
{
    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    num_elements_ = hdr.get(CommunicationHeaderObjectType::NUM_ELEMENTS).as<uint32_t>();

    if ((num_added_elements_ + num_elements_) > buffer_size_)
    {
        buffer_size_ = buffer_size_ * 2U;

        float* const points_ptr = new float[num_added_elements_ * 3];
        float* const color_ptr = new float[num_added_elements_ * 3];

        vertex_buffer_.getBufferData(0U, points_ptr, num_added_elements_, 3U);
        vertex_buffer_.getBufferData(1U, color_ptr, num_added_elements_, 3U);

        vertex_buffer_.clear();
        vertex_buffer_.init();

        vertex_buffer_.addExpandableBuffer<float>(buffer_size_, 3);

        vertex_buffer_.updateBufferData(0U, points_ptr, num_added_elements_, 3U, 0U);

        if (has_color_)
        {
            vertex_buffer_.addExpandableBuffer<float>(buffer_size_, 3);
            vertex_buffer_.updateBufferData(1U, color_ptr, num_added_elements_, 3U, 0U);
        }

        delete[] points_ptr;
        delete[] color_ptr;
    }

    vertex_buffer_.updateBufferData(0U, converted_data_local->points_ptr, num_elements_, 3U, num_added_elements_);

    if (has_color_)
    {
        vertex_buffer_.updateBufferData(1U, converted_data_local->color_data, num_elements_, 3U, num_added_elements_);
    }

    num_added_elements_ += num_elements_;
}

LegendProperties Scatter2D::getLegendProperties() const
{
    LegendProperties lp{PlotObjectBase::getLegendProperties()};
    lp.type = LegendType::DOT;
    lp.color = color_;
    lp.scatter_style = scatter_style_;
    lp.point_size = point_size_;

    return lp;
}

void Scatter2D::modifyShader()
{
    PlotObjectBase::modifyShader();
    shader_collection_.scatter_shader.use();
    shader_collection_.scatter_shader.base_uniform_handles.point_size.setFloat(point_size_);
    shader_collection_.scatter_shader.base_uniform_handles.scatter_mode.setInt(static_cast<int>(scatter_style_));
    if (has_point_sizes_)
    {
        shader_collection_.scatter_shader.base_uniform_handles.has_point_sizes_vec.setInt(1);
    }
    else
    {
        shader_collection_.scatter_shader.base_uniform_handles.has_point_sizes_vec.setInt(0);
    }

    if (has_silhouette_)
    {
        shader_collection_.scatter_shader.uniform_handles.has_silhouette.setInt(1);
        shader_collection_.scatter_shader.uniform_handles.silhouette_color.setColor(silhouette_);
        const float s = 1.0f - silhouette_percentage_;
        shader_collection_.scatter_shader.uniform_handles.squared_silhouette_percentage.setFloat(s * s);
    }
    else
    {
        shader_collection_.scatter_shader.uniform_handles.has_silhouette.setInt(0);
    }

    if (has_color_)
    {
        shader_collection_.scatter_shader.base_uniform_handles.has_color_vec.setInt(1);
    }
    else if (has_distance_from_)
    {
        shader_collection_.scatter_shader.uniform_handles.distance_from_point.setVec(distance_from_.getPoint());
        shader_collection_.scatter_shader.uniform_handles.min_dist.setFloat(distance_from_.getMinDist());
        shader_collection_.scatter_shader.uniform_handles.max_dist.setFloat(distance_from_.getMaxDist());
        shader_collection_.scatter_shader.uniform_handles.has_distance_from.setInt(1);
        shader_collection_.scatter_shader.uniform_handles.distance_from_type.setInt(
            static_cast<int>(distance_from_.getDistFromType()));
        shader_collection_.scatter_shader.base_uniform_handles.color_map_selection.setInt(static_cast<int>(color_map_) +
                                                                                          1);
        shader_collection_.scatter_shader.base_uniform_handles.has_color_vec.setInt(0);
    }
    else
    {
        shader_collection_.scatter_shader.base_uniform_handles.has_color_vec.setInt(0);
    }
    shader_collection_.basic_plot_shader.use();
}

void Scatter2D::findMinMax()
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

std::shared_ptr<const ConvertedDataBase> Scatter2D::convertRawData(const CommunicationHeader& hdr,
                                                                   const PlotObjectAttributes& attributes,
                                                                   const UserSuppliedProperties& user_supplied_properties,
                                                                   const uint8_t* const data_ptr)
{
    const InputParams input_params{attributes.num_elements,
                                   attributes.num_bytes_per_element,
                                   attributes.num_bytes_for_one_vec,
                                   attributes.has_color,
                                   attributes.has_point_sizes,
                                   user_supplied_properties.z_offset.value_or(kDefaultZOffset)};

    std::shared_ptr<const ConvertedDataBase> converted_data_base{
        applyConverter<ConvertedData>(data_ptr, attributes.data_type, Converter{}, input_params)};

    return converted_data_base;
}

void Scatter2D::render()
{
    shader_collection_.scatter_shader.use();
    vertex_buffer_.render(num_added_elements_);
}

Scatter2D::~Scatter2D() {}

namespace
{
template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data, const InputParams& input_params)
{
    ConvertedData* converted_data = new ConvertedData;
    converted_data->points_ptr = new float[3 * input_params.num_elements];

    size_t idx = 0U;

    const T* const input_data_dt_x = reinterpret_cast<const T* const>(input_data);
    const T* const input_data_dt_y = input_data_dt_x + input_params.num_elements;

    for (size_t k = 0; k < input_params.num_elements; k++)
    {
        converted_data->points_ptr[idx] = input_data_dt_x[k];
        converted_data->points_ptr[idx + 1] = input_data_dt_y[k];
        converted_data->points_ptr[idx + 2] = input_params.z_offset;
        idx += 3;
    }

    if (input_params.has_color)
    {
        const properties::Color* input_data_rgb;

        if (input_params.has_point_sizes)
        {
            input_data_rgb =
                reinterpret_cast<const properties::Color* const>(input_data_dt_x + 3U * input_params.num_elements);
        }
        else
        {
            input_data_rgb =
                reinterpret_cast<const properties::Color* const>(input_data_dt_x + 2U * input_params.num_elements);
        }
        converted_data->color_data = new float[3 * input_params.num_elements];

        idx = 0;

        for (size_t k = 0; k < input_params.num_elements; k++)
        {
            converted_data->color_data[idx] = static_cast<float>(input_data_rgb[k].red) / 255.0f;
            converted_data->color_data[idx + 1] = static_cast<float>(input_data_rgb[k].green) / 255.0f;
            converted_data->color_data[idx + 2] = static_cast<float>(input_data_rgb[k].blue) / 255.0f;

            idx += 3;
        }
    }

    if (input_params.has_point_sizes)
    {
        const T* input_data_point_sizes;

        input_data_point_sizes = reinterpret_cast<const T*>(input_data_dt_x + 2U * input_params.num_elements);
        converted_data->point_sizes_data = new float[input_params.num_elements];

        for (size_t k = 0; k < input_params.num_elements; k++)
        {
            converted_data->point_sizes_data[k] = input_data_point_sizes[k];
        }
    }

    return std::shared_ptr<const ConvertedData>{converted_data};
}

}  // namespace
