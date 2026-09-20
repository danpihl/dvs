#ifndef SERIAL_RAW_DATA_FRAME_H
#define SERIAL_RAW_DATA_FRAME_H

#include <stdint.h>

#include <cstddef>

#include "definitions.h"

class RawDataFrame
{
private:
    uint8_t* data_;
    uint64_t size_;

public:
    RawDataFrame() = delete;
    RawDataFrame(uint8_t* const buffer,
                 const size_t buffer_size,
                 const size_t start_of_frame_idx,
                 const size_t end_of_frame_idx);

    RawDataFrame(const RawDataFrame& other) = delete;
    RawDataFrame(RawDataFrame&& other);

    RawDataFrame& operator=(const RawDataFrame& other) = delete;
    RawDataFrame& operator=(RawDataFrame&& other) = delete;

    ~RawDataFrame();

    size_t size() const
    {
        return size_;
    }

    const uint8_t* data() const
    {
        return data_;
    }
};

class BufferedReader
{
private:
    const uint8_t* const buffer_;
    size_t idx_;
    size_t size_;

public:
    BufferedReader() = delete;

    BufferedReader(const RawDataFrame& raw_data_frame)
        : buffer_{raw_data_frame.data()}, idx_{0U}, size_{raw_data_frame.size()}
    {
    }

    BufferedReader(const BufferedReader& other) = delete;
    BufferedReader(BufferedReader&& other) = delete;
    BufferedReader& operator=(const BufferedReader& other) = delete;
    BufferedReader& operator=(BufferedReader&& other) = delete;

    ~BufferedReader() {}

    template <typename T> void read(T& value)
    {
        READ_ESCAPED_DATA(buffer_, idx_, value);
    }

    template <typename T> void readByte(T& value)
    {
        READ_ESCAPED_BYTE(buffer_, idx_, value, T);
    }

    void readUInt16(uint16_t& value)
    {
        READ_ESCAPED_UINT16(buffer_, idx_, value);
    }

    bool hasReadToEnd() const
    {
        return idx_ == size_;
    }

    const uint8_t* data() const
    {
        return buffer_;
    }

    size_t idx() const
    {
        return idx_;
    }

    void advance(const size_t num_bytes)
    {
        idx_ += num_bytes;
    }
};

#endif  // SERIAL_RAW_DATA_FRAME_H
