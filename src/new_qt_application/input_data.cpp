#include "input_data.h"

InputData::InputData(ReceivedData& received_data)
    : received_data_{std::move(received_data)},
      function_{received_data_.getFunction()},
      converted_data_{nullptr},
      plot_object_attributes_{},
      user_supplied_properties_{}
{
}

InputData::InputData(ReceivedData& received_data,
                     const PlotObjectAttributes& plot_object_attributes,
                     const UserSuppliedProperties& user_supplied_properties)
    : received_data_{std::move(received_data)},
      function_{received_data_.getFunction()},
      converted_data_{nullptr},
      plot_object_attributes_{plot_object_attributes},
      user_supplied_properties_{user_supplied_properties}
{
}

InputData::InputData(ReceivedData& received_data,
                     const std::shared_ptr<const ConvertedDataBase>& converted_data,
                     const PlotObjectAttributes& plot_object_attributes,
                     const UserSuppliedProperties& user_supplied_properties)
    : received_data_{std::move(received_data)},
      function_{received_data_.getFunction()},
      converted_data_{converted_data},
      plot_object_attributes_{plot_object_attributes},
      user_supplied_properties_{user_supplied_properties}
{
}

internal::Function InputData::getFunction() const
{
    return function_;
}

std::tuple<ReceivedData, PlotObjectAttributes, UserSuppliedProperties, std::shared_ptr<const ConvertedDataBase>>
InputData::moveAllData()
{
    return {std::move(received_data_), plot_object_attributes_, user_supplied_properties_, converted_data_};
}

std::tuple<ReceivedData, PlotObjectAttributes, UserSuppliedProperties> InputData::moveAllDataButConvertedData()
{
    return {std::move(received_data_), plot_object_attributes_, user_supplied_properties_};
}