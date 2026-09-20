#ifndef CONVERSION_FUNCTION_H
#define CONVERSION_FUNCTION_H

#include "serial_interface/object_types.h"

inline float getFloatValue(const std::shared_ptr<objects::BaseObject>& obj)
{
    const NumberDataType ndt = static_cast<objects::Number*>(obj.get())->numberDataType();

    if (ndt == NumberDataType::kFloat)
    {
        return static_cast<objects::Float*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kDouble)
    {
        return static_cast<objects::Double*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kInt8)
    {
        return static_cast<objects::Int8*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kInt16)
    {
        return static_cast<objects::Int16*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kInt32)
    {
        return static_cast<objects::Int32*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kInt64)
    {
        return static_cast<objects::Int64*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kUInt8)
    {
        return static_cast<objects::UInt8*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kUInt16)
    {
        return static_cast<objects::UInt16*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kUInt32)
    {
        return static_cast<objects::UInt32*>(obj.get())->value();
    }
    else if (ndt == NumberDataType::kUInt64)
    {
        return static_cast<objects::UInt64*>(obj.get())->value();
    }
    return static_cast<objects::Float*>(obj.get())->value();
}

#endif
