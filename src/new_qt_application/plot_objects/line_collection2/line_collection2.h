#ifndef MAIN_APPLICATION_PLOT_OBJECTS_LINE_COLLECTION2_LINE_COLLECTION2_H_
#define MAIN_APPLICATION_PLOT_OBJECTS_LINE_COLLECTION2_LINE_COLLECTION2_H_

#include <string>
#include <vector>

#include "lumos/math.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"
#include "opengl_low_level/vertex_buffer.h"
#include "plot_objects/plot_object_base/plot_object_base.h"

class LineCollection2D : public PlotObjectBase
{
public:
    LineCollection2D();
    LineCollection2D(const CommunicationHeader& hdr,
                     ReceivedData& received_data,
                     const std::shared_ptr<const ConvertedDataBase>& converted_data,

                     const PlotObjectAttributes& plot_object_attributes,
                     const UserSuppliedProperties& user_supplied_properties,
                     const ShaderCollection& shader_collection,
                     ColorPicker& color_picker);
    ~LineCollection2D();

    LegendProperties getLegendProperties() const override;

    void render() override;

    static std::shared_ptr<const ConvertedDataBase> convertRawData(const CommunicationHeader& hdr,
                                                                   const PlotObjectAttributes& attributes,
                                                                   const UserSuppliedProperties& user_supplied_properties,
                                                                   const uint8_t* const data_ptr);

private:
    VertexBuffer vertex_buffer_;

    void findMinMax() override;
};

#endif  // MAIN_APPLICATION_PLOT_OBJECTS_LINE_COLLECTION2_LINE_COLLECTION2_H_
