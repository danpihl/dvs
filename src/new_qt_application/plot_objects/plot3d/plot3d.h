#ifndef MAIN_APPLICATION_PLOT_OBJECTS_PLOT3D_PLOT3D_H_
#define MAIN_APPLICATION_PLOT_OBJECTS_PLOT3D_PLOT3D_H_

#include <string>
#include <vector>

#include "lumos/math.h"
#include "main_application/plot_objects/utils.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"
#include "opengl_low_level/vertex_buffer.h"
#include "plot_objects/plot_object_base/plot_object_base.h"

class Plot3D : public PlotObjectBase
{
public:
    Plot3D();
    Plot3D(const CommunicationHeader& hdr,
           ReceivedData& received_data,
           const std::shared_ptr<const ConvertedDataBase>& converted_data,

           const PlotObjectAttributes& plot_object_attributes,
           const UserSuppliedProperties& user_supplied_properties,
           const ShaderCollection& shader_collection,
           ColorPicker& color_picker);
    ~Plot3D();

    static std::shared_ptr<const ConvertedDataBase> convertRawData(const CommunicationHeader& hdr,
                                                                   const PlotObjectAttributes& attributes,
                                                                   const UserSuppliedProperties& user_supplied_properties,
                                                                   const uint8_t* const data_ptr);

    void render() override;

private:
    VertexBuffer vertex_buffer_;

    size_t num_points_;
    bool is_valid_;

    void findMinMax() override;
};

#endif  // MAIN_APPLICATION_PLOT_OBJECTS_PLOT3D_PLOT3D_H_
