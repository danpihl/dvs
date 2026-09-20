#include "point_selection.h"

using namespace lumos;
using namespace lumos::internal;

PointSelection::~PointSelection() {}

PointSelection::PointSelection()
{
    pending_soft_clear_ = false;
}

void PointSelection::addData(const CommunicationHeader& hdr,
                             const PlotObjectAttributes& plot_object_attributes,
                             const UserSuppliedProperties& user_supplied_properties,
                             const std::shared_ptr<const ConvertedDataBase>& converted_data)
{
    if (user_supplied_properties.exclude_from_selection)
    {
        return;
    }

    if (pending_soft_clear_)
    {
        old_plot_datas_.clear();
        pending_soft_clear_ = false;
    }

    plot_datas_.emplace_back(hdr, plot_object_attributes, user_supplied_properties, converted_data);
}

void PointSelection::deletePlotObject(const ItemId id)
{
    const auto q = std::find_if(plot_datas_.begin(), plot_datas_.end(), [&id](const PlotData& pd) -> bool {
        return pd.plot_object_attributes.id == id;
    });

    if (q != plot_datas_.end())
    {
        plot_datas_.erase(q);
    }
}

void PointSelection::clear()
{
    plot_datas_.clear();
    old_plot_datas_.clear();
    pending_soft_clear_ = false;
}

std::pair<lumos::Vec3<double>, bool> PointSelection::getClosestPoint(const Line3D<double>& line,
                                                                   const bool has_query_line)
{
    double min_dist = std::numeric_limits<double>::max();
    lumos::Vec3<double> closest_point;
    bool has_closest_point = false;

    if (has_query_line)
    {
        for (size_t k = 0; k < plot_datas_.size(); k++)
        {
            const std::pair<lumos::Vec3<double>, double> res = plot_datas_[k].converted_data->getClosestPoint(line);
            if (res.second < min_dist)
            {
                min_dist = res.second;
                closest_point = res.first;
                has_closest_point = true;
            }
        }
    }

    return {closest_point, has_closest_point};
}

void PointSelection::softClear()
{
    pending_soft_clear_ = true;

    std::vector<PlotData> new_plot_datas;
    for (size_t k = 0; k < plot_datas_.size(); k++)
    {
        if (plot_datas_[k].user_supplied_properties.is_persistent)
        {
            new_plot_datas.push_back(plot_datas_[k]);
        }
        else
        {
            old_plot_datas_.push_back(plot_datas_[k]);
        }
    }
    plot_datas_ = new_plot_datas;
}
