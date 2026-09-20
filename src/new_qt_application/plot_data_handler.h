#ifndef MAIN_APPLICATION_PLOT_DATA_HANDLER_H_
#define MAIN_APPLICATION_PLOT_DATA_HANDLER_H_

#include <string>
#include <vector>

#include "axes/legend_properties.h"
#include "color_picker.h"
#include "communication/received_data.h"
#include "lumos/plotting/internal.h"
#include "lumos/math.h"
#include "input_data.h"
#include "misc/rgb_triplet.h"

using namespace lumos;
using namespace lumos::internal;

class PlotObjectBase;
struct ShaderCollection;

class PlotDataHandler
{
private:
    bool pending_soft_clear_;
    ShaderCollection shader_collection_;
    bool isUpdatable(const Function fcn) const;
    ColorPicker color_picker_{};

public:
    std::pair<Vec3d, Vec3d> getMinMaxVectors() const;
    std::vector<PlotObjectBase*> plot_datas_;
    std::vector<PlotObjectBase*> old_plot_datas_;

    std::vector<UserSuppliedProperties> awaiting_user_supplied_properties_;
    PlotDataHandler(const ShaderCollection& shader_collection);
    ~PlotDataHandler();
    void clear();
    void softClear();
    void render();
    void addData(const CommunicationHeader& hdr,
                 const PlotObjectAttributes& plot_object_attributes,
                 const UserSuppliedProperties& user_supplied_properties,
                 ReceivedData& received_data,
                 const std::shared_ptr<const ConvertedDataBase>& converted_data);
    void setTransform(const ItemId id,
                      const FixedSizeMatrix<double, 3, 3>& rotation,
                      const Vec3<double>& translation,
                      const FixedSizeMatrix<double, 3, 3>& scale);
    std::vector<LegendProperties> getLegendStrings() const;
    void propertiesExtension(const ItemId id, const UserSuppliedProperties& user_supplied_properties);
    void propertiesExtensionMultiple(const ReceivedData& received_data);
    void deletePlotObject(const ItemId id);
};

#endif  // MAIN_APPLICATION_PLOT_DATA_HANDLER_H_
