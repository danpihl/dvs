#ifndef SERIAL_OBJECT_TYPES_H
#define SERIAL_OBJECT_TYPES_H

#include <stdint.h>

#include <iostream>

#include "definitions.h"
#include "raw_data_frame.h"

namespace objects
{
class BaseObject
{
protected:
    TopicId topic_id_;
    ObjectType object_type_;
    uint64_t timestamp_;

public:
    BaseObject() = delete;
    BaseObject(const ObjectType object_type, const TopicId topic_id, const uint64_t timestamp)
        : topic_id_{topic_id}, object_type_{object_type}, timestamp_{timestamp}
    {
    }

    ~BaseObject() = default;

    TopicId topicId() const
    {
        return topic_id_;
    }
    ObjectType objectType() const
    {
        return object_type_;
    }

    uint64_t timestamp() const
    {
        return timestamp_;
    }
};

class Number : public BaseObject
{
protected:
    NumberDataType number_data_type_;

public:
    Number() = delete;
    Number(const NumberDataType number_data_type, const TopicId topic_id, const uint64_t timestamp)
        : BaseObject{ObjectType::kNumber, topic_id, timestamp}, number_data_type_{number_data_type}
    {
    }

    ~Number() {}

    NumberDataType numberDataType() const
    {
        return number_data_type_;
    }
};

class Float : public Number
{
private:
    float value_;

public:
    Float() = delete;
    Float(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kFloat, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    ~Float() {}

    float value() const
    {
        return value_;
    }
};

class Double : public Number
{
private:
    double value_;

public:
    Double() = delete;
    Double(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kDouble, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    double value() const
    {
        return value_;
    }
};

class UInt8 : public Number
{
private:
    uint8_t value_;

public:
    UInt8() = delete;
    UInt8(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kUInt8, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    uint8_t value() const
    {
        return value_;
    }
};

class UInt16 : public Number
{
private:
    uint16_t value_;

public:
    UInt16() = delete;
    UInt16(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kUInt16, topic_id, timestamp)
    {
        buffered_reader.readUInt16(value_);
    }

    uint16_t value() const
    {
        return value_;
    }
};

class UInt32 : public Number
{
private:
    uint32_t value_;

public:
    UInt32() = delete;
    UInt32(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kUInt32, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    uint32_t value() const
    {
        return value_;
    }
};

class UInt64 : public Number
{
private:
    uint64_t value_;

public:
    UInt64() = delete;
    UInt64(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kUInt64, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    uint64_t value() const
    {
        return value_;
    }
};

class Int8 : public Number
{
private:
    int8_t value_;

public:
    Int8() = delete;
    Int8(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kInt8, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    int8_t value() const
    {
        return value_;
    }
};

class Int16 : public Number
{
private:
    int16_t value_;

public:
    Int16() = delete;
    Int16(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kInt16, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    int16_t value() const
    {
        return value_;
    }
};

class Int32 : public Number
{
private:
    int32_t value_;

public:
    Int32() = delete;
    Int32(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kInt32, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    int32_t value() const
    {
        return value_;
    }
};

class Int64 : public Number
{
private:
    int64_t value_;

public:
    Int64() = delete;
    Int64(BufferedReader& buffered_reader, const TopicId topic_id, const uint64_t timestamp)
        : Number(NumberDataType::kInt64, topic_id, timestamp)
    {
        buffered_reader.read(value_);
    }

    int64_t value() const
    {
        return value_;
    }
};

inline void printObjectData(const std::shared_ptr<objects::BaseObject>& obj)
{
    const NumberDataType number_data_type = static_cast<objects::Number*>(obj.get())->numberDataType();

    if (number_data_type == NumberDataType::kFloat)
    {
        const float value = static_cast<objects::Float*>(obj.get())->value();

        std::cout << "Received float: " << value << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kDouble)
    {
        const double value = static_cast<objects::Double*>(obj.get())->value();

        std::cout << "Received double: " << value << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kInt8)
    {
        const int8_t value = static_cast<objects::Int8*>(obj.get())->value();

        std::cout << "Received int8: " << static_cast<int>(value) << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kInt16)
    {
        const int16_t value = static_cast<objects::Int16*>(obj.get())->value();

        std::cout << "Received int16: " << value << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kInt32)
    {
        const int32_t value = static_cast<objects::Int32*>(obj.get())->value();

        std::cout << "Received int32: " << value << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kInt64)
    {
        const int64_t value = static_cast<objects::Int64*>(obj.get())->value();

        std::cout << "Received int64: " << value << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kUInt8)
    {
        const uint8_t value = static_cast<objects::UInt8*>(obj.get())->value();

        std::cout << "Received uint8: " << static_cast<uint16_t>(value) << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kUInt16)
    {
        const uint16_t value = static_cast<objects::UInt16*>(obj.get())->value();

        std::cout << "Received uint16: " << value << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kUInt32)
    {
        const uint32_t value = static_cast<objects::UInt32*>(obj.get())->value();

        std::cout << "Received uint32: " << value << " with id: " << obj->topicId() << std::endl;
    }
    else if (number_data_type == NumberDataType::kUInt64)
    {
        const uint64_t value = static_cast<objects::UInt64*>(obj.get())->value();

        std::cout << "Received uint64: " << value << " with id: " << obj->topicId() << std::endl;
    }
}

}  // namespace objects

#endif  // SERIAL_OBJECT_TYPES_H
