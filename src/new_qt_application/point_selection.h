#ifndef POINT_SELECTION_H
#define POINT_SELECTION_H

#include <memory>
#include <string>
#include <vector>

#include "axes/legend_properties.h"
#include "color_picker.h"
#include "communication/received_data.h"
#include "lumos/plotting/internal.h"
#include "lumos/math.h"
#include "lumos/math.h"
#include "input_data.h"
#include "misc/rgb_triplet.h"

class PointSelection
{
private:
    struct PlotData
    {
        PlotData() = default;
        PlotData(const CommunicationHeader& hdr_,
                 const PlotObjectAttributes& plot_object_attributes_,
                 const UserSuppliedProperties& user_supplied_properties_,
                 const std::shared_ptr<const ConvertedDataBase>& converted_data_)
            : hdr{hdr_},
              plot_object_attributes{plot_object_attributes_},
              user_supplied_properties{user_supplied_properties_},
              converted_data{converted_data_}
        {
        }

        CommunicationHeader hdr;
        PlotObjectAttributes plot_object_attributes;
        UserSuppliedProperties user_supplied_properties;
        std::shared_ptr<const ConvertedDataBase> converted_data;
    };

    bool pending_soft_clear_;

public:
    PointSelection();
    ~PointSelection();

    std::vector<PlotData> plot_datas_;
    std::vector<PlotData> old_plot_datas_;

    void deletePlotObject(const ItemId id);
    void clear();
    void softClear();

    void addData(const CommunicationHeader& hdr,
                 const PlotObjectAttributes& plot_object_attributes,
                 const UserSuppliedProperties& user_supplied_properties,
                 const std::shared_ptr<const ConvertedDataBase>& converted_data);
    std::pair<lumos::Vec3<double>, bool> getClosestPoint(const Line3D<double>& line, const bool has_query_line);
};

#endif  // POINT_SELECTION_H