#ifndef MAIN_APPLICATION_COMMUNICATION_RECEIVED_DATA_H_
#define MAIN_APPLICATION_COMMUNICATION_RECEIVED_DATA_H_

#include "lumos/plotting/internal.h"
#include "lumos/math.h"
#include "lumos/plotting/fillable_uint8_array.h"

class ReceivedData
{
private:
    lumos::internal::CommunicationHeader hdr_;
    lumos::internal::Function function_;
    uint8_t* payload_data_;
    uint8_t* raw_data_;
    uint64_t num_data_bytes_;
    uint64_t total_num_bytes_;

public:
    ReceivedData();
    ReceivedData(const ReceivedData& other) = delete;
    ReceivedData(ReceivedData&& other);
    ReceivedData& operator=(const ReceivedData& other) = delete;
    ReceivedData& operator=(ReceivedData&& other);

    ReceivedData(const size_t size_to_allocate);
    ~ReceivedData();

    void parseHeader();

    uint8_t* payloadData() const;
    uint8_t* rawData() const;
    uint64_t size() const;
    lumos::internal::Function getFunction() const;
    const lumos::internal::CommunicationHeader& getCommunicationHeader() const;
};

#endif  // MAIN_APPLICATION_COMMUNICATION_RECEIVED_DATA_H_
