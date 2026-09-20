#ifndef MAIN_APPLICATION_PLOT_OBJECTS_SCREEN_SPACE_PRIMITIVE_SCREEN_SPACE_PRIMITIVE_H
#define MAIN_APPLICATION_PLOT_OBJECTS_SCREEN_SPACE_PRIMITIVE_SCREEN_SPACE_PRIMITIVE_H

#include <string>
#include <vector>

#include "lumos/math.h"
#include "main_application/plot_objects/utils.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"
#include "opengl_low_level/vertex_buffer.h"
#include "plot_objects/plot_object_base/plot_object_base.h"

class ScreenSpacePrimitive : public PlotObjectBase
{
public:
    ScreenSpacePrimitive();
    ScreenSpacePrimitive(const CommunicationHeader& hdr,
              ReceivedData& received_data,
              const std::shared_ptr<const ConvertedDataBase>& converted_data,
              const PlotObjectAttributes& plot_object_attributes,
              const UserSuppliedProperties& user_supplied_properties,
              const ShaderCollection& shader_collection,
              ColorPicker& color_picker);
    ~ScreenSpacePrimitive();

    void render() override;
    void modifyShader() override;

    static std::shared_ptr<const ConvertedDataBase> convertRawData(const CommunicationHeader& hdr,
                                                                   const PlotObjectAttributes& attributes,
                                                                   const UserSuppliedProperties& user_supplied_properties,
                                                                   const uint8_t* const data_ptr);


private:
    VertexBuffer vertex_buffer_;
    void findMinMax() override;
    float* points_ptr_;
};

#endif  // MAIN_APPLICATION_PLOT_OBJECTS_SCREEN_SPACE_PRIMITIVE_SCREEN_SPACE_PRIMITIVE_H
