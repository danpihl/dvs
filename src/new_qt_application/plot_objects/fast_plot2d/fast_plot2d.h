#ifndef MAIN_APPLICATION_PLOT_OBJECTS_FAST_PLOT2D_FAST_PLOT2D_H_
#define MAIN_APPLICATION_PLOT_OBJECTS_FAST_PLOT2D_FAST_PLOT2D_H_

#include <string>
#include <vector>

#include "lumos/math.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"
#include "opengl_low_level/vertex_buffer.h"
#include "plot_objects/plot_object_base/plot_object_base.h"

class FastPlot2D : public PlotObjectBase
{
public:
    FastPlot2D();
    FastPlot2D(const CommunicationHeader& hdr,
               ReceivedData& received_data,
               const std::shared_ptr<const ConvertedDataBase>& converted_data,
               const PlotObjectAttributes& plot_object_attributes,
               const UserSuppliedProperties& user_supplied_properties,
               const ShaderCollection& shader_collection,
               ColorPicker& color_picker);
    ~FastPlot2D();

    LegendProperties getLegendProperties() const override;

    void render() override;

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

#endif  // MAIN_APPLICATION_PLOT_OBJECTS_FAST_PLOT2D_FAST_PLOT2D_H_
