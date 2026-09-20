#ifndef MAIN_APPLICATION_PLOT_OBJECTS_SCATTER_SCATTER_H_
#define MAIN_APPLICATION_PLOT_OBJECTS_SCATTER_SCATTER_H_

#include <string>
#include <vector>

#include "lumos/math.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"
#include "opengl_low_level/vertex_buffer.h"
#include "plot_objects/plot_object_base/plot_object_base.h"

class Scatter2D : public PlotObjectBase
{
public:
    Scatter2D();
    Scatter2D(const CommunicationHeader& hdr,
              ReceivedData& received_data,
              const std::shared_ptr<const ConvertedDataBase>& converted_data,

              const PlotObjectAttributes& plot_object_attributes,
              const UserSuppliedProperties& user_supplied_properties,
              const ShaderCollection& shader_collection,
              ColorPicker& color_picker);
    ~Scatter2D();

    void render() override;
    void modifyShader() override;
    LegendProperties getLegendProperties() const override;

    static std::shared_ptr<const ConvertedDataBase> convertRawData(const CommunicationHeader& hdr,
                                                                   const PlotObjectAttributes& attributes,
                                                                   const UserSuppliedProperties& user_supplied_properties,
                                                                   const uint8_t* const data_ptr);

    void appendNewData(ReceivedData& received_data,
                       const CommunicationHeader& hdr,
                       const std::shared_ptr<const ConvertedDataBase>& converted_data,
                       const UserSuppliedProperties& user_supplied_properties) override;

private:
    VertexBuffer vertex_buffer_;
    void findMinMax() override;
};

#endif  // MAIN_APPLICATION_PLOT_OBJECTS_SCATTER_SCATTER_H_
