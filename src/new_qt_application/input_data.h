#ifndef INPUT_DATA_H_
#define INPUT_DATA_H_

#include <tuple>

#include "lumos/plotting/communication_header.h"
#include "plot_objects/plot_object_base/plot_object_base.h"
#include "plot_objects/plot_objects.h"

class InputData
{
public:
    InputData() = delete;

    InputData(ReceivedData& received_data);
    InputData(ReceivedData& received_data,
              const PlotObjectAttributes& plot_object_attributes,
              const UserSuppliedProperties& user_supplied_properties);
    InputData(ReceivedData& received_data,
              const std::shared_ptr<const ConvertedDataBase>& converted_data,
              const PlotObjectAttributes& plot_object_attributes,
              const UserSuppliedProperties& user_supplied_properties);

    internal::Function getFunction() const;

    std::tuple<ReceivedData, PlotObjectAttributes, UserSuppliedProperties, std::shared_ptr<const ConvertedDataBase>>
    moveAllData();

    std::tuple<ReceivedData, PlotObjectAttributes, UserSuppliedProperties> moveAllDataButConvertedData();

private:
    ReceivedData received_data_;
    internal::Function function_;
    std::shared_ptr<const ConvertedDataBase> converted_data_;
    PlotObjectAttributes plot_object_attributes_;
    UserSuppliedProperties user_supplied_properties_;
};

#endif  // INPUT_DATA_H_
