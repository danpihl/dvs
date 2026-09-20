#include "communication/received_data.h"

#include "lumos/plotting/constants.h"

ReceivedData::ReceivedData()
    : hdr_{},
      function_{lumos::internal::Function::UNKNOWN},
      payload_data_{nullptr},
      raw_data_{nullptr},
      num_data_bytes_{0U},
      total_num_bytes_{0U}
{
}

ReceivedData::ReceivedData(const size_t size_to_allocate)
{
    raw_data_ = new uint8_t[size_to_allocate];
    total_num_bytes_ = size_to_allocate;
    payload_data_ = nullptr;
    num_data_bytes_ = 0U;
}

ReceivedData::ReceivedData(ReceivedData&& other)
    : hdr_{other.hdr_},
      function_{other.function_},
      payload_data_{other.payload_data_},
      raw_data_{other.raw_data_},
      num_data_bytes_{other.num_data_bytes_},
      total_num_bytes_{other.total_num_bytes_}
{
    other.function_ = lumos::internal::Function::UNKNOWN;
    other.payload_data_ = nullptr;
    other.num_data_bytes_ = 0U;
    other.raw_data_ = nullptr;
    other.total_num_bytes_ = 0U;
}

ReceivedData& ReceivedData::operator=(ReceivedData&& other)
{
    if (raw_data_ != nullptr)
    {
        delete[] raw_data_;
    }

    hdr_ = other.hdr_;
    function_ = other.function_;
    raw_data_ = other.raw_data_;
    payload_data_ = other.payload_data_;
    num_data_bytes_ = other.num_data_bytes_;
    total_num_bytes_ = other.total_num_bytes_;

    other.function_ = lumos::internal::Function::UNKNOWN;
    other.payload_data_ = nullptr;
    other.raw_data_ = nullptr;
    other.num_data_bytes_ = 0U;
    other.total_num_bytes_ = 0U;

    return *this;
}

ReceivedData::~ReceivedData()
{
    if (raw_data_ != nullptr)
    {
        delete[] raw_data_;
    }
}

void ReceivedData::parseHeader()
{
    const UInt8ArrayView array_view{raw_data_, total_num_bytes_};
    hdr_ = lumos::internal::CommunicationHeader{array_view};
    function_ = hdr_.getFunction();
    const uint64_t transmission_data_offset = hdr_.numBytes() + lumos::internal::kHeaderDataStartOffset;

    if (transmission_data_offset > array_view.size())
    {
        throw std::runtime_error("transmission_data_offset can't be bigger than num_received_bytes");
    }

    num_data_bytes_ = array_view.size() - transmission_data_offset;

    if (num_data_bytes_ == 0)
    {
        payload_data_ = nullptr;
    }
    else
    {
        payload_data_ = raw_data_ + transmission_data_offset;
    }
}

lumos::internal::Function ReceivedData::getFunction() const
{
    return function_;
}

uint8_t* ReceivedData::rawData() const
{
    return raw_data_;
}

uint8_t* ReceivedData::payloadData() const
{
    return payload_data_;
}

uint64_t ReceivedData::size() const
{
    return num_data_bytes_;
}

const lumos::internal::CommunicationHeader& ReceivedData::getCommunicationHeader() const
{
    return hdr_;
}
