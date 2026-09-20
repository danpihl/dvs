#ifndef MAIN_APPLICATION_OUTER_CONVERTER_H_
#define MAIN_APPLICATION_OUTER_CONVERTER_H_

#include <stdint.h>

#include <utility>

#include "lumos/plotting/enumerations.h"

template <typename O, typename I, typename C>
std::shared_ptr<const O> applyConverter(const uint8_t* const input_data,
                                        const lumos::internal::DataType data_type,
                                        C&& converter,
                                        const I& input_params)
{
    std::shared_ptr<const O> output_data;

    if (data_type == lumos::internal::DataType::FLOAT)
    {
        output_data = std::forward<C>(converter).template convert<float>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::DOUBLE)
    {
        output_data = std::forward<C>(converter).template convert<double>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::INT8)
    {
        output_data = std::forward<C>(converter).template convert<int8_t>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::INT16)
    {
        output_data = std::forward<C>(converter).template convert<int16_t>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::INT32)
    {
        output_data = std::forward<C>(converter).template convert<int32_t>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::INT64)
    {
        output_data = std::forward<C>(converter).template convert<int64_t>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::UINT8)
    {
        output_data = std::forward<C>(converter).template convert<uint8_t>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::UINT16)
    {
        output_data = std::forward<C>(converter).template convert<uint16_t>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::UINT32)
    {
        output_data = std::forward<C>(converter).template convert<uint32_t>(input_data, input_params);
    }
    else if (data_type == lumos::internal::DataType::UINT64)
    {
        output_data = std::forward<C>(converter).template convert<uint64_t>(input_data, input_params);
    }
    else
    {
        throw std::runtime_error("Invalid data type!");
    }

    return output_data;
}

#endif  // MAIN_APPLICATION_OUTER_CONVERTER_H_
