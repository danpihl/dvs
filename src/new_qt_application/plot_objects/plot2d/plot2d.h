#ifndef MAIN_APPLICATION_PLOT_OBJECTS_PLOT2D_PLOT2D_H_
#define MAIN_APPLICATION_PLOT_OBJECTS_PLOT2D_PLOT2D_H_

#include <string>
#include <vector>

#include "lumos/math.h"
#include "misc/rgb_triplet.h"
#include "opengl_low_level/opengl_header.h"
#include "opengl_low_level/vertex_buffer.h"
#include "plot_objects/plot_object_base/plot_object_base.h"

class Plot2D : public PlotObjectBase
{
public:
    Plot2D();
    Plot2D(const CommunicationHeader& hdr,
           ReceivedData& received_data,
           const std::shared_ptr<const ConvertedDataBase>& converted_data,

           const PlotObjectAttributes& plot_object_attributes,
           const UserSuppliedProperties& user_supplied_properties,
           const ShaderCollection& shader_collection,
           ColorPicker& color_picker);
    ~Plot2D();

    LegendProperties getLegendProperties() const override;
    void updateWithNewData(ReceivedData& received_data,
                           const CommunicationHeader& hdr,
                           const std::shared_ptr<const ConvertedDataBase>& converted_data,
                           const UserSuppliedProperties& user_supplied_properties) override;

    void render() override;

    static std::shared_ptr<const ConvertedDataBase> convertRawData(const CommunicationHeader& hdr,
                                                                   const PlotObjectAttributes& attributes,
                                                                   const UserSuppliedProperties& user_supplied_properties,
                                                                   const uint8_t* const data_ptr);

private:
    VertexBuffer vertex_buffer_;
    float gap_size_, dash_size_;

    float first_length_;
    Vec3f first_point_;
    Vec3f second_point_;

    bool is_valid_;

    size_t num_points_;

    void findMinMax() override;
};

#endif  // MAIN_APPLICATION_PLOT_OBJECTS_PLOT2D_PLOT2D_H_
