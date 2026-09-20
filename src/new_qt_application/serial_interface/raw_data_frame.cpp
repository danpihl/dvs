#include "raw_data_frame.h"
#include "definitions.h"

#include <cstring>
#include <iostream>

RawDataFrame::RawDataFrame(uint8_t* const buffer, const size_t buffer_size, const size_t start_of_frame_idx, const size_t end_of_frame_idx) : data_{nullptr}, size_{0U}
{
    if(start_of_frame_idx == end_of_frame_idx)
    {
        return;
    }

    size_ = (start_of_frame_idx < end_of_frame_idx) ? ((end_of_frame_idx - start_of_frame_idx) - 1U) : (((end_of_frame_idx + buffer_size) - start_of_frame_idx) - 1U);

    if(0U == size_)
    {
        return;
    }

    data_ = new uint8_t[size_];

    if(start_of_frame_idx < end_of_frame_idx)
    {
        std::memcpy(data_, buffer + start_of_frame_idx + 1U, size_);
    }
    else
    {
        const size_t num_bytes_to_end_of_buffer = (buffer_size - start_of_frame_idx) - 1U;
        std::memcpy(data_, buffer + start_of_frame_idx + 1U, num_bytes_to_end_of_buffer);

        const size_t num_bytes_to_read_from_start_of_buffer = end_of_frame_idx;
        std::memcpy(data_ + num_bytes_to_end_of_buffer, buffer, num_bytes_to_read_from_start_of_buffer);
    }
}

RawDataFrame::RawDataFrame(RawDataFrame&& other)
{
    data_ = other.data_;
    size_ = other.size_;

    other.data_ = nullptr;
    other.size_ = 0U;
}

RawDataFrame::~RawDataFrame()
{
    delete[] data_;
}
