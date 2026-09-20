#ifndef MAIN_APPLICATION_PLOT_OBJECTS_PLOT_OBJECT_BASE_PLOT_OBJECT_BASE_H_
#define MAIN_APPLICATION_PLOT_OBJECTS_PLOT_OBJECT_BASE_PLOT_OBJECT_BASE_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "axes/legend_properties.h"
#include "color_picker.h"
#include "communication/received_data.h"
#include "lumos/plotting/enumerations.h"
#include "lumos/math.h"
#include "lumos/plotting/plot_properties.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"
#include "plot_objects/utils.h"
#include "user_supplied_properties.h"
#include "lumos/plotting/internal.h"
#include "shader.h"

using namespace lumos;
using namespace lumos::internal;
using namespace lumos::properties;

bool isPlotDataFunction(const Function fcn);

struct ConvertedDataBase
{
    Function function;

    virtual ~ConvertedDataBase() {}
    virtual std::pair<lumos::Vec3<double>, double> getClosestPoint(const Line3D<double>& line) const
    {
        std::cout << "Called base function!" << std::endl;
        return {Vec3<double>{0.0, 0.0, 0.0}, std::numeric_limits<double>::max()};
    }
};

struct PlotObjectAttributes
{
    Function function;
    DataType data_type;

    size_t num_bytes_per_element;
    size_t num_elements;
    size_t num_dimensions;

    ItemId id;

    bool has_color;
    bool has_point_sizes;

    uint64_t num_bytes_for_one_vec;

    Dimension2D dims;

    uint32_t num_vertices;
    uint32_t num_indices;

    uint8_t num_channels;

    uint32_t num_objects;

    PlotObjectAttributes() {}
    PlotObjectAttributes(const CommunicationHeader& hdr) : function{hdr.getFunction()}
    {
        num_dimensions = getNumDimensionsFromFunction(function);

        if (hdr.hasObjectWithType(CommunicationHeaderObjectType::NUM_ELEMENTS))
        {
            num_elements = hdr.get(CommunicationHeaderObjectType::NUM_ELEMENTS).as<uint32_t>();
        }

        if (hdr.hasObjectWithType(CommunicationHeaderObjectType::DATA_TYPE))
        {
            data_type = hdr.get(CommunicationHeaderObjectType::DATA_TYPE).as<DataType>();
            num_bytes_per_element = dataTypeToNumBytes(data_type);
            num_bytes_for_one_vec = num_bytes_per_element * num_elements;
        }

        if (hdr.hasObjectWithType(CommunicationHeaderObjectType::ITEM_ID))
        {
            id = hdr.get(CommunicationHeaderObjectType::ITEM_ID).as<ItemId>();
        }

        has_color = hdr.hasObjectWithType(CommunicationHeaderObjectType::HAS_COLOR);
        has_point_sizes = hdr.hasObjectWithType(CommunicationHeaderObjectType::HAS_POINT_SIZES);

        if (hdr.hasObjectWithType(CommunicationHeaderObjectType::DIMENSION_2D))
        {
            dims = hdr.get(CommunicationHeaderObjectType::DIMENSION_2D).as<internal::Dimension2D>();
        }

        if (hdr.hasObjectWithType(CommunicationHeaderObjectType::NUM_VERTICES))
        {
            num_vertices = hdr.get(CommunicationHeaderObjectType::NUM_VERTICES).as<uint32_t>();
        }

        if (hdr.hasObjectWithType(CommunicationHeaderObjectType::NUM_INDICES))
        {
            num_indices = hdr.get(CommunicationHeaderObjectType::NUM_INDICES).as<uint32_t>();
        }

        if (hdr.hasObjectWithType(CommunicationHeaderObjectType::NUM_CHANNELS))
        {
            num_channels = hdr.get(CommunicationHeaderObjectType::NUM_CHANNELS).as<uint8_t>();
        }

        if (hdr.hasObjectWithType(CommunicationHeaderObjectType::NUM_OBJECTS))
        {
            num_objects = hdr.get(CommunicationHeaderObjectType::NUM_OBJECTS).as<uint32_t>();
        }
    }
};

class PlotObjectBase
{
protected:
    ReceivedData received_data_;

    uint8_t* data_ptr_;

    size_t num_dimensions_;
    size_t num_bytes_per_element_;
    uint32_t num_elements_;
    uint64_t num_bytes_for_one_vec_;
    size_t num_added_elements_;

    Function function_;
    DataType data_type_;

    Vec3d min_vec_;
    Vec3d max_vec_;
    bool min_max_calculated_;
    ShaderCollection shader_collection_;

    // Properties
    std::string label_;

    RGBTripletf color_;
    bool has_color_;
    bool has_point_sizes_;

    RGBTripletf edge_color_;
    RGBTripletf face_color_;
    size_t buffer_size_;

    bool has_silhouette_;
    float silhouette_percentage_;
    RGBTripletf silhouette_;

    bool has_distance_from_;
    DistanceFrom distance_from_;

    bool has_custom_transform_;
    glm::mat4 custom_rotation_;
    glm::mat4 custom_translation_;
    glm::mat4 custom_scale_;

    float z_offset_;

    bool has_edge_color_;
    bool has_face_color_;

    ColorMap color_map_;
    bool has_color_map_;

    LineStyle line_style_;
    bool has_line_style_;

    ScatterStyle scatter_style_;

    float alpha_;
    float line_width_;
    float point_size_;

    bool is_persistent_;
    bool is_updateable_;
    GLenum dynamic_or_static_usage_;
    bool interpolate_colormap_;
    bool is_appendable_;

    bool has_label_;

    ItemId id_;

    void initializeProperties(const UserSuppliedProperties& user_supplied_properties, ColorPicker& color_picker);
    virtual void findMinMax() = 0;

    void postInitialize(ReceivedData& received_data,
                        const CommunicationHeader& hdr,
                        const UserSuppliedProperties& user_supplied_properties);
    void throwIfNotUpdateable() const;

public:
    size_t getNumDimensions() const;
    virtual ~PlotObjectBase();
    PlotObjectBase() = default;
    PlotObjectBase(ReceivedData& received_data,
                   const CommunicationHeader& hdr,
                   const PlotObjectAttributes& plot_object_attributes,
                   const UserSuppliedProperties& user_supplied_properties,
                   const ShaderCollection& shader_collection,
                   ColorPicker& color_picker);
    virtual void render() = 0;
    void preRender(const ShaderBase* const shader_to_use);
    bool affectsColormapMinMax() const
    {
        return has_color_map_;
    }

    virtual void appendNewData(ReceivedData& received_data,
                               const CommunicationHeader& hdr,
                               const std::shared_ptr<const ConvertedDataBase>& converted_data,
                               const UserSuppliedProperties& user_supplied_properties);

    bool isAppendable() const;
    bool isUpdateable() const;

    void setTransform(const FixedSizeMatrix<double, 3, 3>& rotation,
                      const Vec3<double>& translation,
                      const FixedSizeMatrix<double, 3, 3>& scale);

    ItemId getId() const
    {
        return id_;
    }

    std::pair<Vec3d, Vec3d> getMinMaxVectors();

    bool isPersistent() const;
    std::string getLabel() const;

    virtual LegendProperties getLegendProperties() const
    {
        LegendProperties lp;
        lp.label = label_;

        return lp;
    }

    virtual void updateWithNewData(ReceivedData& received_data,
                                   const CommunicationHeader& hdr,
                                   const std::shared_ptr<const ConvertedDataBase>& converted_data,
                                   const UserSuppliedProperties& user_supplied_properties);

    bool hasLabel()
    {
        return has_label_;
    }

    virtual void modifyShader();

    void updateProperties(const UserSuppliedProperties& user_supplied_properties);
};

#endif  // MAIN_APPLICATION_PLOT_OBJECTS_PLOT_OBJECT_BASE_PLOT_OBJECT_BASE_H_
