#include "main_application/plot_objects/im_show/im_show.h"

#include "outer_converter.h"

using namespace lumos::internal;

namespace
{

struct ConvertedData : ConvertedDataBase
{
    uint8_t* image_data;

    ConvertedData() : image_data{nullptr} {}

    ConvertedData(const ConvertedData& other) = delete;
    ConvertedData& operator=(const ConvertedData& other) = delete;
    ConvertedData(ConvertedData&& other) = delete;
    ConvertedData& operator=(ConvertedData&& other) = delete;

    ~ConvertedData() override
    {
        delete[] image_data;
    }
};

struct InputParams
{
    size_t num_channels;
    Dimension2D dims;
    float z_offset;
    DataType data_type;
    float multiplier;
    size_t num_bytes_per_element;

    InputParams() {}
    InputParams(const size_t num_channels_,
                const Dimension2D& dims_,
                const float z_offset_,
                const DataType data_type_,
                const size_t num_bytes_per_element_)
        : num_channels{num_channels_},
          dims{dims_},
          z_offset{z_offset_},
          data_type{data_type_},
          num_bytes_per_element{num_bytes_per_element_}
    {
        // Multiplier for FLOAT and DOUBLE is 1.0f, so that's left as the default case
        multiplier = 1.0f;
        multiplier = (data_type_ == DataType::INT8) ? (1.0f / 127.0f) : multiplier;
        multiplier = (data_type_ == DataType::INT16) ? (1.0f / 32767.0f) : multiplier;
        multiplier = (data_type_ == DataType::INT32) ? (1.0f / 2147483647.0f) : multiplier;
        multiplier = (data_type_ == DataType::INT64) ? (1.0f / 9223372036854775807.0f) : multiplier;
        multiplier = (data_type_ == DataType::UINT8) ? (1.0f / 255.0f) : multiplier;
        multiplier = (data_type_ == DataType::UINT16) ? (1.0f / 65535.0f) : multiplier;
        multiplier = (data_type_ == DataType::UINT32) ? (1.0f / 4294967295.0f) : multiplier;
        multiplier = (data_type_ == DataType::UINT64) ? (1.0f / 18446744073709551615.0f) : multiplier;
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

void ImShow::findMinMax()
{
    min_vec_.x = 0;
    min_vec_.y = 0;
    min_vec_.z = -1.0;

    max_vec_.x = width_;
    max_vec_.y = height_;
    max_vec_.z = 1.0;
}

ImShow::ImShow(const CommunicationHeader& hdr,
               ReceivedData& received_data,
               const std::shared_ptr<const ConvertedDataBase>& converted_data,
               const PlotObjectAttributes& plot_object_attributes,
               const UserSuppliedProperties& user_supplied_properties,
               const ShaderCollection& shader_collection,
               ColorPicker& color_picker)
    : PlotObjectBase(received_data, hdr, plot_object_attributes, user_supplied_properties, shader_collection, color_picker)
{
    if (function_ != Function::IM_SHOW)
    {
        throw std::runtime_error("Invalid function type for ImShow!");
    }

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    dims_ = hdr.get(CommunicationHeaderObjectType::DIMENSION_2D).as<Dimension2D>();
    num_channels_ = hdr.get(CommunicationHeaderObjectType::NUM_CHANNELS).as<uint8_t>();

    width_ = dims_.cols;
    height_ = dims_.rows;

    // clang-format off
    indices_ = VectorInitializer<unsigned int>{0, 2, 3, 0, 2, 1};
    vertices_ = VectorInitializer<float>{
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    // clang-format on

    vertices_(0) = width_;
    vertices_(1) = height_;
    vertices_(5) = width_;
    vertices_(16) = height_;

    vertices_(2) = z_offset_;
    vertices_(7) = z_offset_;
    vertices_(12) = z_offset_;
    vertices_(17) = z_offset_;

    glGenVertexArrays(1, &vertex_array_object_);
    glGenBuffers(1, &vertex_buffer_object_);
    glGenBuffers(1, &extended_buffer_object_);

    glBindVertexArray(vertex_array_object_);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.numBytes(), vertices_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, extended_buffer_object_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.numBytes(), indices_.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &texture_handle_);
    glBindTexture(GL_TEXTURE_2D, texture_handle_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum gl_data_type;

    if ((plot_object_attributes.data_type == DataType::FLOAT) || (plot_object_attributes.data_type == DataType::DOUBLE))
    {
        gl_data_type = GL_FLOAT;
    }
    else if (plot_object_attributes.data_type == DataType::UINT8)
    {
        gl_data_type = GL_UNSIGNED_BYTE;
    }
    else
    {
        throw std::runtime_error("Invalid data type for ImShow!");
    }

    GLenum gl_channel_format;

    if ((num_channels_ == 1) || (num_channels_ == 3))
    {
        gl_channel_format = GL_RGB;
    }
    else if ((num_channels_ == 2) || (num_channels_ == 4))
    {
        gl_channel_format = GL_RGBA;
        shader_collection_.img_plot_shader.uniform_handles.use_global_alpha.setInt(0);
    }
    else
    {
        throw std::runtime_error("Invalid number of channels for ImShow!");
    }

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 gl_channel_format,
                 width_,
                 height_,
                 0,
                 gl_channel_format,
                 gl_data_type,
                 converted_data_local->image_data);

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    findMinMax();
}

void ImShow::render()
{
    shader_collection_.img_plot_shader.use();

    preRender(&shader_collection_.img_plot_shader);

    if ((num_channels_ == 4) || (num_channels_ == 2))
    {
        shader_collection_.img_plot_shader.uniform_handles.use_global_alpha.setInt(0);
    }
    else
    {
        shader_collection_.img_plot_shader.uniform_handles.use_global_alpha.setInt(1);
        shader_collection_.img_plot_shader.uniform_handles.global_alpha.setFloat(alpha_);
    }

    glBindTexture(GL_TEXTURE_2D, texture_handle_);

    glBindVertexArray(vertex_array_object_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

ImShow::~ImShow()
{
    glDeleteVertexArrays(1, &vertex_array_object_);
    glDeleteBuffers(1, &vertex_buffer_object_);
    glDeleteBuffers(1, &extended_buffer_object_);
}

std::shared_ptr<const ConvertedDataBase> ImShow::convertRawData(const CommunicationHeader& hdr,
                                                                const PlotObjectAttributes& attributes,
                                                                const UserSuppliedProperties& user_supplied_properties,
                                                                const uint8_t* const data_ptr)
{
    const InputParams input_params{attributes.num_channels,
                                   attributes.dims,
                                   user_supplied_properties.z_offset.value_or(kDefaultZOffset),
                                   attributes.data_type,
                                   attributes.num_bytes_per_element};

    std::shared_ptr<const ConvertedDataBase> converted_data_base{
        applyConverter<ConvertedData>(data_ptr, attributes.data_type, Converter{}, input_params)};

    return converted_data_base;
}

namespace
{
template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data, const InputParams& input_params)
{
    ConvertedData* converted_data = new ConvertedData;

    const size_t num_elements_per_channel = input_params.dims.cols * input_params.dims.rows;
    const size_t num_elements_per_channel_2 = num_elements_per_channel * 2U;
    const size_t num_elements_per_channel_3 = num_elements_per_channel * 3U;
    const size_t num_cols = input_params.dims.cols;

    const size_t num_bytes_for_one_channel =
        input_params.dims.rows * input_params.dims.cols * input_params.num_bytes_per_element;

    const size_t num_output_channels = ((input_params.num_channels == 4) || (input_params.num_channels == 2)) ? 4 : 3;

    converted_data->image_data = new uint8_t[num_bytes_for_one_channel * num_output_channels];

    size_t idx = 0;

    if (input_params.data_type == DataType::DOUBLE)
    {
        const double* const data_ptr = reinterpret_cast<const double* const>(input_data);
        float* new_data_ptr = reinterpret_cast<float*>(converted_data->image_data);

        if (input_params.num_channels == 4)
        {
            for (size_t r = 0; r < input_params.dims.rows; r++)
            {
                const size_t outer_idx_r = r * num_cols;
                const size_t outer_idx_g = outer_idx_r + num_elements_per_channel;
                const size_t outer_idx_b = outer_idx_r + num_elements_per_channel_2;
                const size_t outer_idx_a = outer_idx_r + num_elements_per_channel_3;

                for (size_t c = 0; c < input_params.dims.cols; c++)
                {
                    new_data_ptr[idx] = data_ptr[outer_idx_r + c];
                    new_data_ptr[idx + 1] = data_ptr[outer_idx_g + c];
                    new_data_ptr[idx + 2] = data_ptr[outer_idx_b + c];
                    new_data_ptr[idx + 3] = data_ptr[outer_idx_a + c];

                    idx += 4;
                }
            }
        }
        else if (input_params.num_channels == 2)
        {
            for (size_t r = 0; r < input_params.dims.rows; r++)
            {
                const size_t outer_idx_gray = r * num_cols;
                const size_t outer_idx_alpha = outer_idx_gray + num_elements_per_channel;
                for (size_t c = 0; c < input_params.dims.cols; c++)
                {
                    const T gray_pxl_data = data_ptr[outer_idx_gray + c];
                    new_data_ptr[idx] = gray_pxl_data;
                    new_data_ptr[idx + 1] = gray_pxl_data;
                    new_data_ptr[idx + 2] = gray_pxl_data;
                    new_data_ptr[idx + 3] = data_ptr[outer_idx_alpha + c];

                    idx += 4;
                }
            }
        }
        else if (input_params.num_channels == 3)
        {
            for (size_t r = 0; r < input_params.dims.rows; r++)
            {
                const size_t outer_idx_r = r * num_cols;
                const size_t outer_idx_g = outer_idx_r + num_elements_per_channel;
                const size_t outer_idx_b = outer_idx_r + num_elements_per_channel_2;
                for (size_t c = 0; c < input_params.dims.cols; c++)
                {
                    new_data_ptr[idx] = data_ptr[outer_idx_r + c];
                    new_data_ptr[idx + 1] = data_ptr[outer_idx_g + c];
                    new_data_ptr[idx + 2] = data_ptr[outer_idx_b + c];

                    idx += 3;
                }
            }
        }
        else if (input_params.num_channels == 1)
        {
            for (size_t r = 0; r < input_params.dims.rows; r++)
            {
                const size_t outer_idx = r * num_cols;
                for (size_t c = 0; c < input_params.dims.cols; c++)
                {
                    const T pxl_data = data_ptr[outer_idx + c];

                    new_data_ptr[idx] = pxl_data;
                    new_data_ptr[idx + 1] = pxl_data;
                    new_data_ptr[idx + 2] = pxl_data;

                    idx += 3;
                }
            }
        }
        else
        {
            std::cout << "Invalid number of channels: " << input_params.num_channels << std::endl;
        }
    }
    else
    {
        const T* const data_ptr = reinterpret_cast<const T* const>(input_data);
        T* new_data_ptr = reinterpret_cast<T*>(converted_data->image_data);

        if (input_params.num_channels == 4)
        {
            for (size_t r = 0; r < input_params.dims.rows; r++)
            {
                const size_t outer_idx_r = r * num_cols;
                const size_t outer_idx_g = outer_idx_r + num_elements_per_channel;
                const size_t outer_idx_b = outer_idx_r + num_elements_per_channel_2;
                const size_t outer_idx_a = outer_idx_r + num_elements_per_channel_3;

                for (size_t c = 0; c < input_params.dims.cols; c++)
                {
                    new_data_ptr[idx] = data_ptr[outer_idx_r + c];
                    new_data_ptr[idx + 1] = data_ptr[outer_idx_g + c];
                    new_data_ptr[idx + 2] = data_ptr[outer_idx_b + c];
                    new_data_ptr[idx + 3] = data_ptr[outer_idx_a + c];

                    idx += 4;
                }
            }
        }
        else if (input_params.num_channels == 2)
        {
            for (size_t r = 0; r < input_params.dims.rows; r++)
            {
                const size_t outer_idx_gray = r * num_cols;
                const size_t outer_idx_alpha = outer_idx_gray + num_elements_per_channel;
                for (size_t c = 0; c < input_params.dims.cols; c++)
                {
                    const T gray_pxl_data = data_ptr[outer_idx_gray + c];
                    new_data_ptr[idx] = gray_pxl_data;
                    new_data_ptr[idx + 1] = gray_pxl_data;
                    new_data_ptr[idx + 2] = gray_pxl_data;
                    new_data_ptr[idx + 3] = data_ptr[outer_idx_alpha + c];

                    idx += 4;
                }
            }
        }
        else if (input_params.num_channels == 3)
        {
            for (size_t r = 0; r < input_params.dims.rows; r++)
            {
                const size_t outer_idx_r = r * num_cols;
                const size_t outer_idx_g = outer_idx_r + num_elements_per_channel;
                const size_t outer_idx_b = outer_idx_r + num_elements_per_channel_2;
                for (size_t c = 0; c < input_params.dims.cols; c++)
                {
                    new_data_ptr[idx] = data_ptr[outer_idx_r + c];
                    new_data_ptr[idx + 1] = data_ptr[outer_idx_g + c];
                    new_data_ptr[idx + 2] = data_ptr[outer_idx_b + c];

                    idx += 3;
                }
            }
        }
        else if (input_params.num_channels == 1)
        {
            for (size_t r = 0; r < input_params.dims.rows; r++)
            {
                const size_t outer_idx = r * num_cols;
                for (size_t c = 0; c < input_params.dims.cols; c++)
                {
                    const T pxl_data = data_ptr[outer_idx + c];

                    new_data_ptr[idx] = pxl_data;
                    new_data_ptr[idx + 1] = pxl_data;
                    new_data_ptr[idx + 2] = pxl_data;

                    idx += 3;
                }
            }
        }
        else
        {
            std::cout << "Invalid number of channels: " << input_params.num_channels << std::endl;
        }
    }

    return std::shared_ptr<const ConvertedData>(converted_data);
}
}  // namespace
