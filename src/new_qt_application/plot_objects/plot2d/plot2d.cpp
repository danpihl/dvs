#include "main_application/plot_objects/plot2d/plot2d.h"

#include <functional>

#include "outer_converter.h"

namespace
{

struct InputParams
{
    size_t num_elements;
    size_t num_bytes_per_element;
    size_t num_bytes_for_one_vec;
    bool has_color;

    InputParams() = default;
    InputParams(const size_t num_elements_,
                const size_t num_bytes_per_element_,
                const size_t num_bytes_for_one_vec_,
                const bool has_color_)
        : num_elements{num_elements_},
          num_bytes_per_element{num_bytes_per_element_},
          num_bytes_for_one_vec{num_bytes_for_one_vec_},
          has_color{has_color_}
    {
    }
};

struct ConvertedData : ConvertedDataBase
{
    float* p0;
    float* p1;
    float* p2;
    float* length_along;
    float first_length;
    Vec3f first_point;
    Vec3f second_point;
    int32_t* idx_data;
    float* color_data;
    size_t num_points;
    size_t num_elements;

    ConvertedData()
        : p0{nullptr},
          p1{nullptr},
          p2{nullptr},
          length_along{nullptr},
          idx_data{nullptr},
          color_data{nullptr},
          num_points{0U}
    {
    }

    ConvertedData(const ConvertedData& other) = delete;
    ConvertedData& operator=(const ConvertedData& other) = delete;
    ConvertedData(ConvertedData&& other) = delete;
    ConvertedData& operator=(ConvertedData&& other) = delete;

    ~ConvertedData() override
    {
        delete[] p0;
        delete[] p1;
        delete[] p2;
        delete[] length_along;
        delete[] idx_data;
        delete[] color_data;
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

Plot2D::Plot2D(const CommunicationHeader& hdr,
               ReceivedData& received_data,
               const std::shared_ptr<const ConvertedDataBase>& converted_data,
               const PlotObjectAttributes& plot_object_attributes,
               const UserSuppliedProperties& user_supplied_properties,
               const ShaderCollection& shader_collection,
               ColorPicker& color_picker)
    : PlotObjectBase(received_data, hdr, plot_object_attributes, user_supplied_properties, shader_collection, color_picker),
      vertex_buffer_{OGLPrimitiveType::TRIANGLES}
{
    if (function_ != Function::PLOT2)
    {
        throw std::runtime_error("Invalid function type for Plot2D!");
    }

    is_valid_ = true;

    if (converted_data == nullptr)
    {
        is_valid_ = false;
        return;
    }

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    num_points_ = converted_data_local->num_points;

    first_length_ = converted_data_local->first_length;
    first_point_ = converted_data_local->first_point;
    second_point_ = converted_data_local->second_point;
    if (has_line_style_)
    {
        if (line_style_ == properties::LineStyle::DASHED)
        {
            gap_size_ = 0.05f;
            dash_size_ = 0.05f;
        }
        else if (line_style_ == properties::LineStyle::SHORT_DASHED)
        {
            gap_size_ = 0.01f;
            dash_size_ = 0.01f;
        }
        else if (line_style_ == properties::LineStyle::LONG_DASHED)
        {
            gap_size_ = 0.1f;
            dash_size_ = 0.1f;
        }
    }

    vertex_buffer_.addBuffer(converted_data_local->p0, num_points_, 2, dynamic_or_static_usage_);
    vertex_buffer_.addBuffer(converted_data_local->p1, num_points_, 2, dynamic_or_static_usage_);
    vertex_buffer_.addBuffer(converted_data_local->p2, num_points_, 2, dynamic_or_static_usage_);

    vertex_buffer_.addBuffer(converted_data_local->idx_data, num_points_, 1, dynamic_or_static_usage_);
    vertex_buffer_.addBuffer(converted_data_local->length_along, num_points_, 1, dynamic_or_static_usage_);

    if (has_color_)
    {
        vertex_buffer_.addBuffer(converted_data_local->color_data, num_points_, 3);
    }
}

void Plot2D::findMinMax()
{
    if (!is_valid_)
    {
        min_vec_.x = -1.0;
        min_vec_.y = -1.0;
        min_vec_.z = -1.0;

        max_vec_.x = 1.0;
        max_vec_.y = 1.0;
        max_vec_.z = 1.0;
        return;
    }

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

void Plot2D::render()
{
    if (!is_valid_)
    {
        return;
    }

    shader_collection_.plot_2d_shader.use();
    preRender(&shader_collection_.plot_2d_shader);

    shader_collection_.plot_2d_shader.uniform_handles.half_line_width.setFloat(line_width_ / 3.0f);
    shader_collection_.plot_2d_shader.uniform_handles.z_offset.setFloat(z_offset_);
    shader_collection_.plot_2d_shader.uniform_handles.use_dash.setInt(static_cast<int>(has_line_style_));
    shader_collection_.plot_2d_shader.base_uniform_handles.alpha.setFloat(alpha_);
    shader_collection_.plot_2d_shader.uniform_handles.first_length.setFloat(first_length_);
    shader_collection_.plot_2d_shader.uniform_handles.first_point.setVec(first_point_);
    shader_collection_.plot_2d_shader.uniform_handles.second_point.setVec(second_point_);
    shader_collection_.plot_2d_shader.base_uniform_handles.gap_size.setFloat(gap_size_);
    shader_collection_.plot_2d_shader.base_uniform_handles.dash_size.setFloat(dash_size_);

    if (has_color_)
    {
        shader_collection_.plot_2d_shader.base_uniform_handles.has_color_vec.setInt(1);
    }
    else
    {
        shader_collection_.plot_2d_shader.base_uniform_handles.has_color_vec.setInt(0);
    }

    vertex_buffer_.render(num_points_);
}

std::shared_ptr<const ConvertedDataBase> Plot2D::convertRawData(const CommunicationHeader& hdr,
                                                                const PlotObjectAttributes& attributes,
                                                                const UserSuppliedProperties& user_supplied_properties,
                                                                const uint8_t* const data_ptr)
{
    const InputParams input_params{attributes.num_elements,
                                   attributes.num_bytes_per_element,
                                   attributes.num_bytes_for_one_vec,
                                   attributes.has_color};

    std::shared_ptr<const ConvertedDataBase> converted_data_base{
        applyConverter<ConvertedData>(data_ptr, attributes.data_type, Converter{}, input_params)};

    return converted_data_base;
}

void Plot2D::updateWithNewData(ReceivedData& received_data,
                               const CommunicationHeader& hdr,
                               const std::shared_ptr<const ConvertedDataBase>& converted_data,
                               const UserSuppliedProperties& user_supplied_properties)
{
    if (!is_valid_)
    {
        return;
    }
    throwIfNotUpdateable();

    postInitialize(received_data, hdr, user_supplied_properties);

    const ConvertedData* const converted_data_local = static_cast<const ConvertedData* const>(converted_data.get());

    num_points_ = converted_data_local->num_points;

    vertex_buffer_.updateBufferData(0, converted_data_local->p0, num_points_, 2);
    vertex_buffer_.updateBufferData(1, converted_data_local->p1, num_points_, 2);
    vertex_buffer_.updateBufferData(2, converted_data_local->p2, num_points_, 2);
    vertex_buffer_.updateBufferData(3, converted_data_local->idx_data, num_points_, 1);
    vertex_buffer_.updateBufferData(4, converted_data_local->length_along, num_points_, 1);
}

Plot2D::~Plot2D() {}

LegendProperties Plot2D::getLegendProperties() const
{
    LegendProperties lp{PlotObjectBase::getLegendProperties()};
    lp.type = LegendType::LINE;
    lp.color = color_;

    return lp;
}

namespace
{
template <typename T>
std::shared_ptr<const ConvertedData> convertData(const uint8_t* const input_data,
                                                 const InputParams& input_params_original)
{
    const T* const input_data_dt = reinterpret_cast<const T* const>(input_data);

    float* sanitized_input_x = new float[input_params_original.num_elements];
    float* sanitized_input_y = new float[input_params_original.num_elements];

    size_t total_num_points = 1U;

    float x_last_valid_point = static_cast<float>(input_data_dt[0]);
    float y_last_valid_point = static_cast<float>(input_data_dt[input_params_original.num_elements]);

    sanitized_input_x[0U] = x_last_valid_point;
    sanitized_input_y[0U] = y_last_valid_point;

    properties::Color* sanitized_color_input{nullptr};

    if (input_params_original.has_color)
    {
        sanitized_color_input = new properties::Color[input_params_original.num_elements];
        const properties::Color* const input_data_rgb =
            reinterpret_cast<const properties::Color* const>(input_data_dt + 2U * input_params_original.num_elements);

        sanitized_color_input[0U] = input_data_rgb[0U];

        for (size_t k = 1U; k < input_params_original.num_elements; k++)
        {
            const float xk = static_cast<float>(input_data_dt[k]);
            const float yk = static_cast<float>(input_data_dt[input_params_original.num_elements + k]);
            const properties::Color color_k = input_data_rgb[k];

            if (x_last_valid_point == xk && y_last_valid_point == yk)
            {
                continue;
            }

            sanitized_input_x[total_num_points] = xk;
            sanitized_input_y[total_num_points] = yk;
            sanitized_color_input[total_num_points] = color_k;

            x_last_valid_point = xk;
            y_last_valid_point = yk;

            total_num_points++;
        }
    }
    else
    {
        for (size_t k = 1U; k < input_params_original.num_elements; k++)
        {
            const float xk = static_cast<float>(input_data_dt[k]);
            const float yk = static_cast<float>(input_data_dt[input_params_original.num_elements + k]);

            if (x_last_valid_point == xk && y_last_valid_point == yk)
            {
                continue;
            }

            sanitized_input_x[total_num_points] = xk;
            sanitized_input_y[total_num_points] = yk;

            x_last_valid_point = xk;
            y_last_valid_point = yk;

            total_num_points++;
        }
    }

    if (total_num_points == 1U)
    {
        delete[] sanitized_input_x;
        delete[] sanitized_input_y;
        if (input_params_original.has_color)
        {
            delete[] sanitized_color_input;
        }
        return nullptr;
    }

    InputParams input_params{input_params_original};
    input_params.num_elements = total_num_points;

    const size_t num_segments = input_params.num_elements - 1U;
    const size_t num_triangles = num_segments * 2U + (num_segments - 1U) * 2U;
    const size_t num_points = num_triangles * 3U;

    ConvertedData* converted_data = new ConvertedData;
    converted_data->num_points = num_points;
    converted_data->p0 = new float[2 * num_points];
    converted_data->p1 = new float[2 * num_points];
    converted_data->p2 = new float[2 * num_points];
    converted_data->length_along = new float[num_points];
    converted_data->idx_data = new int32_t[num_points];

    float* length_along_tmp = new float[input_params.num_elements];

    std::memset(converted_data->p0, 0, 2 * num_points * sizeof(float));
    std::memset(converted_data->p1, 0, 2 * num_points * sizeof(float));
    std::memset(converted_data->p2, 0, 2 * num_points * sizeof(float));

    std::memset(converted_data->idx_data, 0, num_points * sizeof(int32_t));

    struct Points
    {
        Vec2<float> p0;
        Vec2<float> p1;
        Vec2<float> p2;
    };
    std::vector<Points> pts;
    pts.resize(input_params.num_elements);

    length_along_tmp[0] = 0.0f;

    for (size_t k = 1; k < input_params.num_elements; k++)
    {
        // Segments inbetween
        const float p0x = sanitized_input_x[k - 1U];
        const float p0y = sanitized_input_y[k - 1U];

        const float p1x = sanitized_input_x[k];
        const float p1y = sanitized_input_y[k];

        const float x = static_cast<float>(p0x - p1x);
        const float y = static_cast<float>(p0y - p1y);

        const float d = std::sqrt(x * x + y * y);

        length_along_tmp[k] = length_along_tmp[k - 1U] + d;
    }

    {
        // First segment
        const float p1x = sanitized_input_x[0U];
        const float p1y = sanitized_input_y[0U];

        const float p2x = sanitized_input_x[1U];
        const float p2y = sanitized_input_y[1U];

        const T vx = p2x - p1x;
        const T vy = p2y - p1y;

        pts[0].p0.x = p1x - vx;
        pts[0].p0.y = p1y - vy;

        pts[0].p1.x = p1x;
        pts[0].p1.y = p1y;

        pts[0].p2.x = p2x;
        pts[0].p2.y = p2y;

        converted_data->first_length = std::sqrt(vx * vx + vy * vy);
        converted_data->first_point = Vec3f(p1x, p1y, 0.0f);
        converted_data->second_point = Vec3f(p2x, p2y, 0.0f);
    }

    {
        // Last segment
        const float p0x = sanitized_input_x[input_params.num_elements - 2];
        const float p0y = sanitized_input_y[input_params.num_elements - 2];

        const float p1x = sanitized_input_x[input_params.num_elements - 1];
        const float p1y = sanitized_input_y[input_params.num_elements - 1];

        const T vx = p1x - p0x;
        const T vy = p1y - p0y;

        pts[input_params.num_elements - 1].p0.x = p0x;
        pts[input_params.num_elements - 1].p0.y = p0y;

        pts[input_params.num_elements - 1].p1.x = p1x;
        pts[input_params.num_elements - 1].p1.y = p1y;

        pts[input_params.num_elements - 1].p2.x = p1x + vx;
        pts[input_params.num_elements - 1].p2.y = p1y + vy;
    }

    // Segments inbetween
    for (size_t k = 1; k < (input_params.num_elements - 1); k++)
    {
        const float p0x = sanitized_input_x[k - 1];
        const float p0y = sanitized_input_y[k - 1];

        const float p1x = sanitized_input_x[k];
        const float p1y = sanitized_input_y[k];

        const float p2x = sanitized_input_x[k + 1];
        const float p2y = sanitized_input_y[k + 1];

        pts[k].p0.x = p0x;
        pts[k].p0.y = p0y;

        pts[k].p1.x = p1x;
        pts[k].p1.y = p1y;

        pts[k].p2.x = p2x;
        pts[k].p2.y = p2y;
    }

    size_t idx = 0;
    size_t idx_idx = 0;
    size_t length_along_idx = 1;

    for (size_t k = 1; k < (input_params.num_elements - 1U); k++)
    {
        const Points& pt_1 = pts[k - 1];
        const Points& pt = pts[k];

        // 1st triangle
        converted_data->p0[idx] = pt_1.p0.x;
        converted_data->p0[idx + 1] = pt_1.p0.y;

        converted_data->p0[idx + 2] = pt.p0.x;
        converted_data->p0[idx + 3] = pt.p0.y;

        converted_data->p0[idx + 4] = pt.p0.x;
        converted_data->p0[idx + 5] = pt.p0.y;

        // 2nd triangle
        converted_data->p0[idx + 6] = pt_1.p0.x;
        converted_data->p0[idx + 7] = pt_1.p0.y;

        converted_data->p0[idx + 8] = pt.p0.x;
        converted_data->p0[idx + 9] = pt.p0.y;

        converted_data->p0[idx + 10] = pt_1.p0.x;
        converted_data->p0[idx + 11] = pt_1.p0.y;

        // 3nd triangle
        converted_data->p0[idx + 12] = pt.p0.x;
        converted_data->p0[idx + 13] = pt.p0.y;

        converted_data->p0[idx + 14] = pt.p0.x;
        converted_data->p0[idx + 15] = pt.p0.y;

        converted_data->p0[idx + 16] = pt.p0.x;
        converted_data->p0[idx + 17] = pt.p0.y;

        // 4th triangle
        converted_data->p0[idx + 18] = pt.p0.x;
        converted_data->p0[idx + 19] = pt.p0.y;

        converted_data->p0[idx + 20] = pt.p0.x;
        converted_data->p0[idx + 21] = pt.p0.y;

        converted_data->p0[idx + 22] = pt.p0.x;
        converted_data->p0[idx + 23] = pt.p0.y;

        // 1st triangle
        converted_data->p1[idx] = pt_1.p1.x;
        converted_data->p1[idx + 1] = pt_1.p1.y;

        converted_data->p1[idx + 2] = pt.p1.x;
        converted_data->p1[idx + 3] = pt.p1.y;

        converted_data->p1[idx + 4] = pt.p1.x;
        converted_data->p1[idx + 5] = pt.p1.y;

        // 2nd triangle
        converted_data->p1[idx + 6] = pt_1.p1.x;
        converted_data->p1[idx + 7] = pt_1.p1.y;

        converted_data->p1[idx + 8] = pt.p1.x;
        converted_data->p1[idx + 9] = pt.p1.y;

        converted_data->p1[idx + 10] = pt_1.p1.x;
        converted_data->p1[idx + 11] = pt_1.p1.y;

        // 3nd triangle
        converted_data->p1[idx + 12] = pt.p1.x;
        converted_data->p1[idx + 13] = pt.p1.y;

        converted_data->p1[idx + 14] = pt.p1.x;
        converted_data->p1[idx + 15] = pt.p1.y;

        converted_data->p1[idx + 16] = pt.p1.x;
        converted_data->p1[idx + 17] = pt.p1.y;

        // 4th triangle
        converted_data->p1[idx + 18] = pt.p1.x;
        converted_data->p1[idx + 19] = pt.p1.y;

        converted_data->p1[idx + 20] = pt.p1.x;
        converted_data->p1[idx + 21] = pt.p1.y;

        converted_data->p1[idx + 22] = pt.p1.x;
        converted_data->p1[idx + 23] = pt.p1.y;

        // 1st triangle
        converted_data->p2[idx] = pt_1.p2.x;
        converted_data->p2[idx + 1] = pt_1.p2.y;

        converted_data->p2[idx + 2] = pt.p2.x;
        converted_data->p2[idx + 3] = pt.p2.y;

        converted_data->p2[idx + 4] = pt.p2.x;
        converted_data->p2[idx + 5] = pt.p2.y;

        // 2nd triangle
        converted_data->p2[idx + 6] = pt_1.p2.x;
        converted_data->p2[idx + 7] = pt_1.p2.y;

        converted_data->p2[idx + 8] = pt.p2.x;
        converted_data->p2[idx + 9] = pt.p2.y;

        converted_data->p2[idx + 10] = pt_1.p2.x;
        converted_data->p2[idx + 11] = pt_1.p2.y;

        // 3nd triangle
        converted_data->p2[idx + 12] = pt.p2.x;
        converted_data->p2[idx + 13] = pt.p2.y;

        converted_data->p2[idx + 14] = pt.p2.x;
        converted_data->p2[idx + 15] = pt.p2.y;

        converted_data->p2[idx + 16] = pt.p2.x;
        converted_data->p2[idx + 17] = pt.p2.y;

        // 4th triangle
        converted_data->p2[idx + 18] = pt.p2.x;
        converted_data->p2[idx + 19] = pt.p2.y;

        converted_data->p2[idx + 20] = pt.p2.x;
        converted_data->p2[idx + 21] = pt.p2.y;

        converted_data->p2[idx + 22] = pt.p2.x;
        converted_data->p2[idx + 23] = pt.p2.y;

        // TODO: Currently broken, fix
        const float la_1 = length_along_tmp[k - 1];
        const float la = length_along_tmp[k];

        converted_data->length_along[length_along_idx] = la_1;
        converted_data->length_along[length_along_idx + 1] = la_1;
        converted_data->length_along[length_along_idx + 2] = la_1;

        converted_data->length_along[length_along_idx + 3] = la_1;
        converted_data->length_along[length_along_idx + 4] = la_1;
        converted_data->length_along[length_along_idx + 5] = la_1;

        converted_data->length_along[length_along_idx + 6] = la;
        converted_data->length_along[length_along_idx + 7] = la;
        converted_data->length_along[length_along_idx + 8] = la;

        converted_data->length_along[length_along_idx + 9] = la;
        converted_data->length_along[length_along_idx + 10] = la;
        converted_data->length_along[length_along_idx + 11] = la;

        converted_data->idx_data[idx_idx] = 0;
        converted_data->idx_data[idx_idx + 1] = 1;
        converted_data->idx_data[idx_idx + 2] = 2;
        converted_data->idx_data[idx_idx + 3] = 3;
        converted_data->idx_data[idx_idx + 4] = 4;
        converted_data->idx_data[idx_idx + 5] = 5;
        converted_data->idx_data[idx_idx + 6] = 6;
        converted_data->idx_data[idx_idx + 7] = 7;
        converted_data->idx_data[idx_idx + 8] = 8;
        converted_data->idx_data[idx_idx + 9] = 9;
        converted_data->idx_data[idx_idx + 10] = 10;
        converted_data->idx_data[idx_idx + 11] = 11;

        idx += 24;
        idx_idx += 12;
        length_along_idx += 12;
    }

    // Last segment will not have the two "Inbetween triangles"
    const Points& pt_1 = pts[input_params.num_elements - 2];
    const Points& pt = pts[input_params.num_elements - 1];

    // 1st triangle
    converted_data->p0[idx] = pt_1.p0.x;
    converted_data->p0[idx + 1] = pt_1.p0.y;

    converted_data->p0[idx + 2] = pt.p0.x;
    converted_data->p0[idx + 3] = pt.p0.y;

    converted_data->p0[idx + 4] = pt.p0.x;
    converted_data->p0[idx + 5] = pt.p0.y;

    // 2nd triangle
    converted_data->p0[idx + 6] = pt_1.p0.x;
    converted_data->p0[idx + 7] = pt_1.p0.y;

    converted_data->p0[idx + 8] = pt.p0.x;
    converted_data->p0[idx + 9] = pt.p0.y;

    converted_data->p0[idx + 10] = pt_1.p0.x;
    converted_data->p0[idx + 11] = pt_1.p0.y;

    // 1st triangle
    converted_data->p1[idx] = pt_1.p1.x;
    converted_data->p1[idx + 1] = pt_1.p1.y;

    converted_data->p1[idx + 2] = pt.p1.x;
    converted_data->p1[idx + 3] = pt.p1.y;

    converted_data->p1[idx + 4] = pt.p1.x;
    converted_data->p1[idx + 5] = pt.p1.y;

    // 2nd triangle
    converted_data->p1[idx + 6] = pt_1.p1.x;
    converted_data->p1[idx + 7] = pt_1.p1.y;

    converted_data->p1[idx + 8] = pt.p1.x;
    converted_data->p1[idx + 9] = pt.p1.y;

    converted_data->p1[idx + 10] = pt_1.p1.x;
    converted_data->p1[idx + 11] = pt_1.p1.y;

    // 1st triangle
    converted_data->p2[idx] = pt_1.p2.x;
    converted_data->p2[idx + 1] = pt_1.p2.y;

    converted_data->p2[idx + 2] = pt.p2.x;
    converted_data->p2[idx + 3] = pt.p2.y;

    converted_data->p2[idx + 4] = pt.p2.x;
    converted_data->p2[idx + 5] = pt.p2.y;

    // 2nd triangle
    converted_data->p2[idx + 6] = pt_1.p2.x;
    converted_data->p2[idx + 7] = pt_1.p2.y;

    converted_data->p2[idx + 8] = pt.p2.x;
    converted_data->p2[idx + 9] = pt.p2.y;

    converted_data->p2[idx + 10] = pt_1.p2.x;
    converted_data->p2[idx + 11] = pt_1.p2.y;

    converted_data->idx_data[idx_idx] = 0;
    converted_data->idx_data[idx_idx + 1] = 1;
    converted_data->idx_data[idx_idx + 2] = 2;
    converted_data->idx_data[idx_idx + 3] = 3;
    converted_data->idx_data[idx_idx + 4] = 4;
    converted_data->idx_data[idx_idx + 5] = 5;

    // TODO: Currently broken, fix
    const float la_1 = length_along_tmp[input_params.num_elements - 2];
    const float la = length_along_tmp[input_params.num_elements - 1];

    converted_data->length_along[length_along_idx] = la_1;
    converted_data->length_along[length_along_idx + 1] = la_1;
    converted_data->length_along[length_along_idx + 2] = la_1;

    converted_data->length_along[length_along_idx + 3] = la_1;
    converted_data->length_along[length_along_idx + 4] = la_1;
    converted_data->length_along[length_along_idx + 5] = la_1;

    if (input_params.has_color)
    {
        converted_data->color_data = new float[3 * num_points];

        idx = 0;

        for (size_t k = 1; k < input_params.num_elements - 1; k++)
        {
            const RGBTripletf color_k{static_cast<float>(sanitized_color_input[k].red) / 255.0f,
                                      static_cast<float>(sanitized_color_input[k].green) / 255.0f,
                                      static_cast<float>(sanitized_color_input[k].blue) / 255.0f};

            const RGBTripletf color_k_1{static_cast<float>(sanitized_color_input[k - 1].red) / 255.0f,
                                        static_cast<float>(sanitized_color_input[k - 1].green) / 255.0f,
                                        static_cast<float>(sanitized_color_input[k - 1].blue) / 255.0f};
            // v0
            converted_data->color_data[idx] = color_k_1.red;
            converted_data->color_data[idx + 1] = color_k_1.green;
            converted_data->color_data[idx + 2] = color_k_1.blue;

            // v1
            converted_data->color_data[idx + 3] = color_k.red;
            converted_data->color_data[idx + 4] = color_k.green;
            converted_data->color_data[idx + 5] = color_k.blue;

            // v2
            converted_data->color_data[idx + 6] = color_k.red;
            converted_data->color_data[idx + 7] = color_k.green;
            converted_data->color_data[idx + 8] = color_k.blue;

            // v3
            converted_data->color_data[idx + 9] = color_k_1.red;
            converted_data->color_data[idx + 10] = color_k_1.green;
            converted_data->color_data[idx + 11] = color_k_1.blue;

            // v4
            converted_data->color_data[idx + 12] = color_k.red;
            converted_data->color_data[idx + 13] = color_k.green;
            converted_data->color_data[idx + 14] = color_k.blue;

            // v5
            converted_data->color_data[idx + 15] = color_k_1.red;
            converted_data->color_data[idx + 16] = color_k_1.green;
            converted_data->color_data[idx + 17] = color_k_1.blue;

            // v6
            converted_data->color_data[idx + 18] = color_k.red;
            converted_data->color_data[idx + 19] = color_k.green;
            converted_data->color_data[idx + 20] = color_k.blue;

            // v7
            converted_data->color_data[idx + 21] = color_k.red;
            converted_data->color_data[idx + 22] = color_k.green;
            converted_data->color_data[idx + 23] = color_k.blue;

            // v8
            converted_data->color_data[idx + 24] = color_k.red;
            converted_data->color_data[idx + 25] = color_k.green;
            converted_data->color_data[idx + 26] = color_k.blue;

            // v9
            converted_data->color_data[idx + 27] = color_k.red;
            converted_data->color_data[idx + 28] = color_k.green;
            converted_data->color_data[idx + 29] = color_k.blue;

            // v10
            converted_data->color_data[idx + 30] = color_k.red;
            converted_data->color_data[idx + 31] = color_k.green;
            converted_data->color_data[idx + 32] = color_k.blue;

            // v11
            converted_data->color_data[idx + 33] = color_k.red;
            converted_data->color_data[idx + 34] = color_k.green;
            converted_data->color_data[idx + 35] = color_k.blue;

            idx += 36;
        }

        const RGBTripletf color_k{
            static_cast<float>(sanitized_color_input[input_params.num_elements - 1].red) / 255.0f,
            static_cast<float>(sanitized_color_input[input_params.num_elements - 1].green) / 255.0f,
            static_cast<float>(sanitized_color_input[input_params.num_elements - 1].blue) / 255.0f};

        const RGBTripletf color_k_1{
            static_cast<float>(sanitized_color_input[input_params.num_elements - 2].red) / 255.0f,
            static_cast<float>(sanitized_color_input[input_params.num_elements - 2].green) / 255.0f,
            static_cast<float>(sanitized_color_input[input_params.num_elements - 2].blue) / 255.0f};
        // v0
        converted_data->color_data[idx] = color_k_1.red;
        converted_data->color_data[idx + 1] = color_k_1.green;
        converted_data->color_data[idx + 2] = color_k_1.blue;

        // v1
        converted_data->color_data[idx + 3] = color_k.red;
        converted_data->color_data[idx + 4] = color_k.green;
        converted_data->color_data[idx + 5] = color_k.blue;

        // v2
        converted_data->color_data[idx + 6] = color_k.red;
        converted_data->color_data[idx + 7] = color_k.green;
        converted_data->color_data[idx + 8] = color_k.blue;

        // v3
        converted_data->color_data[idx + 9] = color_k_1.red;
        converted_data->color_data[idx + 10] = color_k_1.green;
        converted_data->color_data[idx + 11] = color_k_1.blue;

        // v4
        converted_data->color_data[idx + 12] = color_k.red;
        converted_data->color_data[idx + 13] = color_k.green;
        converted_data->color_data[idx + 14] = color_k.blue;

        // v5
        converted_data->color_data[idx + 15] = color_k_1.red;
        converted_data->color_data[idx + 16] = color_k_1.green;
        converted_data->color_data[idx + 17] = color_k_1.blue;

        delete[] sanitized_color_input;
    }

    delete[] sanitized_input_x;
    delete[] sanitized_input_y;
    delete[] length_along_tmp;

    return std::shared_ptr<const ConvertedData>{converted_data};
}

}  // namespace
