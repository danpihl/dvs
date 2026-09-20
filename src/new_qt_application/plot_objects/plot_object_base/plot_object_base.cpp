#include "main_application/plot_objects/plot_object_base/plot_object_base.h"

PlotObjectBase::PlotObjectBase(ReceivedData& received_data,
                               const CommunicationHeader& hdr,
                               const PlotObjectAttributes& plot_object_attributes,
                               const UserSuppliedProperties& user_supplied_properties,
                               const ShaderCollection& shader_collection,
                               ColorPicker& color_picker)
    : received_data_(std::move(received_data)), shader_collection_{shader_collection}
{
    data_ptr_ = received_data_.payloadData();

    function_ = hdr.getFunction();
    data_type_ = hdr.get(CommunicationHeaderObjectType::DATA_TYPE).as<DataType>();

    num_bytes_per_element_ = dataTypeToNumBytes(data_type_);
    num_elements_ = hdr.get(CommunicationHeaderObjectType::NUM_ELEMENTS).as<uint32_t>();

    num_dimensions_ = getNumDimensionsFromFunction(function_);

    if (hdr.hasObjectWithType(CommunicationHeaderObjectType::ITEM_ID))
    {
        id_ = hdr.get(CommunicationHeaderObjectType::ITEM_ID).as<ItemId>();
    }
    else
    {
        id_ = ItemId::UNKNOWN;
    }

    has_color_ = hdr.hasObjectWithType(CommunicationHeaderObjectType::HAS_COLOR);
    has_point_sizes_ = hdr.hasObjectWithType(CommunicationHeaderObjectType::HAS_POINT_SIZES);
    num_bytes_for_one_vec_ = num_bytes_per_element_ * num_elements_;

    min_max_calculated_ = false;
    has_custom_transform_ = false;
    z_offset_ = 0.0f;

    has_face_color_ = true;
    has_edge_color_ = true;
    has_silhouette_ = false;

    initializeProperties(user_supplied_properties, color_picker);
}

void PlotObjectBase::postInitialize(ReceivedData& received_data,
                                    const CommunicationHeader& hdr,
                                    const UserSuppliedProperties& user_supplied_properties)
{
    received_data_ = std::move(received_data);
    data_ptr_ = received_data_.payloadData();

    min_max_calculated_ = false;
    has_custom_transform_ = false;
    z_offset_ = 0.0f;

    function_ = hdr.getFunction();
    data_type_ = hdr.get(CommunicationHeaderObjectType::DATA_TYPE).as<DataType>();

    num_bytes_per_element_ = dataTypeToNumBytes(data_type_);
    num_elements_ = hdr.get(CommunicationHeaderObjectType::NUM_ELEMENTS).as<uint32_t>();

    num_dimensions_ = getNumDimensionsFromFunction(function_);

    if (hdr.hasObjectWithType(CommunicationHeaderObjectType::ITEM_ID))
    {
        id_ = hdr.get(CommunicationHeaderObjectType::ITEM_ID).as<ItemId>();
    }
    else
    {
        id_ = ItemId::UNKNOWN;
    }

    has_color_ = hdr.hasObjectWithType(CommunicationHeaderObjectType::HAS_COLOR);
    has_point_sizes_ = hdr.hasObjectWithType(CommunicationHeaderObjectType::HAS_POINT_SIZES);

    num_bytes_for_one_vec_ = num_bytes_per_element_ * num_elements_;

    updateProperties(user_supplied_properties);
}

void PlotObjectBase::updateProperties(const UserSuppliedProperties& user_supplied_properties)
{
    // Flags
    is_persistent_ = is_persistent_ || user_supplied_properties.is_persistent;
    interpolate_colormap_ = interpolate_colormap_ || user_supplied_properties.interpolate_colormap;
    is_updateable_ = is_updateable_ || user_supplied_properties.is_updateable;
    is_appendable_ = is_appendable_ || user_supplied_properties.is_appendable;

    dynamic_or_static_usage_ = is_updateable_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    // Properties
    if (user_supplied_properties.alpha.has_value())
    {
        alpha_ = user_supplied_properties.alpha.value();
    }

    if (user_supplied_properties.buffer_size.has_value())
    {
        buffer_size_ = user_supplied_properties.buffer_size.value();
    }

    if (user_supplied_properties.scatter_style.has_value())
    {
        scatter_style_ = user_supplied_properties.scatter_style.value();
    }

    if ((user_supplied_properties.line_style.has_value()) && (user_supplied_properties.line_style.value() != LineStyle::SOLID))
    {
        has_line_style_ = true;
        line_style_ = user_supplied_properties.line_style.value();
    }
    else
    {
        has_line_style_ = false;
    }

    if (user_supplied_properties.line_width.has_value())
    {
        line_width_ = user_supplied_properties.line_width.value();
    }

    if (user_supplied_properties.point_size.has_value())
    {
        point_size_ = user_supplied_properties.point_size.value();
    }

    if (user_supplied_properties.z_offset.has_value())
    {
        z_offset_ = user_supplied_properties.z_offset.value();
    }

    if (user_supplied_properties.custom_transform.has_value())
    {
        has_custom_transform_ = true;
        setTransform(user_supplied_properties.custom_transform.value().rotation,
                     user_supplied_properties.custom_transform.value().translation,
                     user_supplied_properties.custom_transform.value().scale);
    }

    if (user_supplied_properties.distance_from.has_value())
    {
        distance_from_ = user_supplied_properties.distance_from.value();
        has_distance_from_ = true;
    }

    if (user_supplied_properties.label.has_value())
    {
        label_ = user_supplied_properties.label.value();
        has_label_ = true;
    }

    if (user_supplied_properties.color.has_value())
    {
        color_ = user_supplied_properties.color.value();
    }

    if (user_supplied_properties.color_map.has_value())
    {
        color_map_ = user_supplied_properties.color_map.value();
        has_color_map_ = true;
    }

    if (user_supplied_properties.edge_color.has_value())
    {
        if (user_supplied_properties.no_edges)
        {
            has_edge_color_ = false;
        }
        else
        {
            edge_color_ = user_supplied_properties.edge_color.value();
            has_edge_color_ = true;
        }
    }

    if (user_supplied_properties.silhouette.has_value())
    {
        has_silhouette_ = user_supplied_properties.has_silhouette;
        silhouette_percentage_ = user_supplied_properties.silhouette_percentage;
        silhouette_ = user_supplied_properties.silhouette.value();
    }

    if (user_supplied_properties.face_color.has_value())
    {
        if (user_supplied_properties.no_faces)
        {
            has_face_color_ = false;
        }
        else
        {
            face_color_ = user_supplied_properties.face_color.value();
            has_face_color_ = true;
        }
    }
}

void PlotObjectBase::preRender(const ShaderBase* const shader_to_use)
{
    if (has_custom_transform_)
    {
        shader_to_use->base_uniform_handles.has_custom_transform.setInt(1);
        glUniformMatrix4fv(
            shader_to_use->base_uniform_handles.custom_translation_mat, 1, GL_FALSE, &custom_translation_[0][0]);
        glUniformMatrix4fv(
            shader_to_use->base_uniform_handles.custom_rotation_mat, 1, GL_FALSE, &custom_rotation_[0][0]);
        glUniformMatrix4fv(shader_to_use->base_uniform_handles.custom_scale_mat, 1, GL_FALSE, &custom_scale_[0][0]);
    }
    else
    {
        shader_to_use->base_uniform_handles.has_custom_transform.setInt(0);
    }
}

bool isPlotDataFunction(const Function fcn)
{
    return (fcn == Function::STAIRS) || (fcn == Function::PLOT2) || (fcn == Function::PLOT3) ||
           (fcn == Function::FAST_PLOT2) || (fcn == Function::LINE_COLLECTION2) ||
           (fcn == Function::LINE_COLLECTION3) || (fcn == Function::FAST_PLOT3) || (fcn == Function::STEM) ||
           (fcn == Function::SCATTER2) || (fcn == Function::SCATTER3) || (fcn == Function::SURF) ||
           (fcn == Function::IM_SHOW) || (fcn == Function::PLOT_COLLECTION2) || (fcn == Function::PLOT_COLLECTION3) ||
           (fcn == Function::DRAW_MESH_SEPARATE_VECTORS) || (fcn == Function::DRAW_MESH) ||
           (fcn == Function::REAL_TIME_PLOT) || (fcn == Function::SCREEN_SPACE_PRIMITIVE);
}

void PlotObjectBase::modifyShader()
{
    shader_collection_.basic_plot_shader.use();
    shader_collection_.basic_plot_shader.base_uniform_handles.vertex_color.setColor(color_);
    shader_collection_.scatter_shader.use();
    shader_collection_.scatter_shader.base_uniform_handles.vertex_color.setColor(color_);
    shader_collection_.plot_2d_shader.use();
    shader_collection_.plot_2d_shader.base_uniform_handles.vertex_color.setColor(color_);
    shader_collection_.plot_3d_shader.use();
    shader_collection_.plot_3d_shader.base_uniform_handles.vertex_color.setColor(color_);
    shader_collection_.basic_plot_shader.use();
}

bool PlotObjectBase::isPersistent() const
{
    return is_persistent_;
}

std::string PlotObjectBase::getLabel() const
{
    return label_;
}

std::pair<Vec3d, Vec3d> PlotObjectBase::getMinMaxVectors()
{
    if (!min_max_calculated_)
    {
        min_max_calculated_ = true;
        findMinMax();
    }
    return std::pair<Vec3d, Vec3d>(min_vec_, max_vec_);
}

size_t PlotObjectBase::getNumDimensions() const
{
    return num_dimensions_;
}

void PlotObjectBase::setTransform(const FixedSizeMatrix<double, 3, 3>& rotation,
                                  const Vec3<double>& translation,
                                  const FixedSizeMatrix<double, 3, 3>& scale)
{
    has_custom_transform_ = true;

    custom_scale_ = glm::mat4(1.0f);

    custom_translation_ = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, translation.z));

    custom_rotation_ = glm::mat4(1.0f);

    for (size_t r = 0; r < 3; r++)
    {
        for (size_t c = 0; c < 3; c++)
        {
            custom_rotation_[r][c] = rotation(r, c);
            custom_scale_[r][c] = scale(r, c);
        }
    }
}

void PlotObjectBase::initializeProperties(const UserSuppliedProperties& user_supplied_properties, ColorPicker& color_picker)
{
    // Flags
    is_persistent_ = user_supplied_properties.is_persistent;
    interpolate_colormap_ = user_supplied_properties.interpolate_colormap;
    is_updateable_ = user_supplied_properties.is_updateable;
    is_appendable_ = user_supplied_properties.is_appendable;

    dynamic_or_static_usage_ = user_supplied_properties.dynamic_or_static_usage;

    // Properties
    alpha_ = user_supplied_properties.alpha.value_or(kDefaultAlpha);
    buffer_size_ = user_supplied_properties.buffer_size.value_or(kDefaultBufferSize);
    scatter_style_ = user_supplied_properties.scatter_style.value_or(kDefaultScatterStyle);
    line_width_ = user_supplied_properties.line_width.value_or(kDefaultLineWidth);
    point_size_ = user_supplied_properties.point_size.value_or(kDefaultPointSize);

    z_offset_ = user_supplied_properties.z_offset.value_or(kDefaultZOffset);

    /*if ((user_supplied_properties.line_style.has_value()) && (user_supplied_properties.line_style.value() != LineStyle::SOLID))
    {
        has_line_style_ = true;
        line_style_ = user_supplied_properties.line_style.value();
    }
    else
    {
        has_line_style_ = false;
    }*/

    if (user_supplied_properties.custom_transform.has_value())
    {
        has_custom_transform_ = true;
        setTransform(user_supplied_properties.custom_transform.value().rotation,
                     user_supplied_properties.custom_transform.value().translation,
                     user_supplied_properties.custom_transform.value().scale);
    }
    else
    {
        has_custom_transform_ = false;
    }

    if (user_supplied_properties.distance_from.has_value())
    {
        has_distance_from_ = true;
        distance_from_ = user_supplied_properties.distance_from.value();
    }
    else
    {
        has_distance_from_ = false;
    }

    if (user_supplied_properties.label.has_value())
    {
        label_ = user_supplied_properties.label.value();
        has_label_ = true;
    }
    else
    {
        has_label_ = false;
    }

    if (user_supplied_properties.color.has_value())
    {
        color_ = user_supplied_properties.color.value();
    }
    else
    {
        color_ = color_picker.getNextColor();
    }

    if (user_supplied_properties.color_map.has_value())
    {
        color_map_ = user_supplied_properties.color_map.value();
        has_color_map_ = true;
    }
    else
    {
        has_color_map_ = false;
    }

    if (user_supplied_properties.edge_color.has_value())
    {
        if (user_supplied_properties.no_edges)
        {
            has_edge_color_ = false;
        }
        else
        {
            has_edge_color_ = true;
            edge_color_ = user_supplied_properties.edge_color.value();
        }
    }
    else
    {
        edge_color_ = RGBTripletf(0.0f, 0.0f, 0.0f);
        has_edge_color_ = true;
    }

    if (user_supplied_properties.face_color.has_value())
    {
        if (user_supplied_properties.no_faces)
        {
            has_face_color_ = false;
        }
        else
        {
            has_face_color_ = true;
            face_color_ = user_supplied_properties.face_color.value();
        }
    }
    else
    {
        face_color_ = color_picker.getNextFaceColor();
        has_face_color_ = true;
    }

    if (user_supplied_properties.silhouette.has_value())
    {
        has_silhouette_ = user_supplied_properties.has_silhouette;
        silhouette_percentage_ = user_supplied_properties.silhouette_percentage;
        silhouette_ = user_supplied_properties.silhouette.value();
    }
}

void PlotObjectBase::updateWithNewData(ReceivedData& received_data,
                                       const CommunicationHeader& hdr,
                                       const std::shared_ptr<const ConvertedDataBase>& converted_data,
                                       const UserSuppliedProperties& user_supplied_properties)
{
    static_cast<void>(received_data);
    static_cast<void>(hdr);
    static_cast<void>(user_supplied_properties);
}

void PlotObjectBase::throwIfNotUpdateable() const
{
    if (!is_updateable_)
    {
        throw std::runtime_error("Tried to update non updateable object!");
    }
}

void PlotObjectBase::appendNewData(ReceivedData& received_data,
                                   const CommunicationHeader& hdr,
                                   const std::shared_ptr<const ConvertedDataBase>& converted_data,
                                   const UserSuppliedProperties& user_supplied_properties)
{
    std::cout << "appendNewData not implemented for this object!" << std::endl;
}

bool PlotObjectBase::isUpdateable() const
{
    return is_updateable_;
}

bool PlotObjectBase::isAppendable() const
{
    return is_appendable_;
}

PlotObjectBase::~PlotObjectBase() {}
