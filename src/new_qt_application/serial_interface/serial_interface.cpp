#include "serial_interface.h"

#include <iostream>

constexpr uint64_t kReadPeriodUs = 1000U * 10U;

SerialInterface::SerialInterface(const std::string& port_name, const int32_t baudrate)
    : serial_port_{port_name, baudrate},
      searching_for_start_of_frame_{true},
      searching_for_end_of_frame_{false},
      head_idx_{0U},
      tail_idx_{0U},
      start_of_frame_idx_{0U},
      buffer_{new uint8_t[kBufferSize]}
{
}

void SerialInterface::start()
{
    if (!serial_port_.isValid())
    {
        std::cout << "Error: Serial port is not valid!" << std::endl;
        return;
    }
    size_t num_bytes_to_read = serial_port_.numAvailableBytes();
    while (num_bytes_to_read > 0)
    {
        const ssize_t num_read_bytes = serial_port_.readBytes(buffer_, num_bytes_to_read);
        num_bytes_to_read -= num_read_bytes;
    }
    serial_receive_thread_ = new std::thread(&SerialInterface::readSerialDataThreadFunction, this);
}

void SerialInterface::readSerialDataThreadFunction()
{
    while (true)
    {
        readSerialData();
        usleep(kReadPeriodUs);
    }
}

void SerialInterface::readSerialData()
{
    const size_t num_bytes_to_read = serial_port_.numAvailableBytes();

    if (num_bytes_to_read > 0)
    {
        // TODO: How to detect if head_idx_ catches up to tail_idx_?

        if (head_idx_ + num_bytes_to_read > kBufferSize)
        {
            // Reading will wrap around the buffer_
            ssize_t num_read_bytes = serial_port_.readBytes(buffer_ + head_idx_, kBufferSize - head_idx_);
            head_idx_ = 0U;

            const size_t num_leftover_bytes_to_read = serial_port_.numAvailableBytes();

            if (num_leftover_bytes_to_read > 0)
            {
                if (head_idx_ + num_leftover_bytes_to_read > kBufferSize)
                {
                    std::cout << "Error: Buffer data that was leftover did not fit into buffer_!" << std::endl;
                }

                num_read_bytes = serial_port_.readBytes(buffer_ + head_idx_, num_leftover_bytes_to_read);
                head_idx_ += num_read_bytes;
            }
        }
        else
        {
            // Reading fits in the buffer_ without wrapping
            const ssize_t num_read_bytes = serial_port_.readBytes(buffer_ + head_idx_, num_bytes_to_read);
            head_idx_ += num_read_bytes;
        }
    }
}

std::vector<RawDataFrame> SerialInterface::extractRawDataFrames()
{
    std::vector<RawDataFrame> raw_data_frames;

    while (NUM_BYTES_AVAILABLE_TO_READ(tail_idx_, head_idx_, kBufferSize) > 0)
    {
        const size_t head_idx_at_read = head_idx_;

        if (searching_for_start_of_frame_)
        {
            while (NUM_BYTES_AVAILABLE_TO_READ(tail_idx_, head_idx_at_read, kBufferSize) > 0)
            {
                if (buffer_[tail_idx_] == kStartOfFrameByte)
                {
                    searching_for_start_of_frame_ = false;
                    searching_for_end_of_frame_ = true;

                    start_of_frame_idx_ = tail_idx_;

                    tail_idx_ = (tail_idx_ + 1U) % kBufferSize;

                    break;
                }

                tail_idx_ = (tail_idx_ + 1U) % kBufferSize;
            }
        }

        if (searching_for_end_of_frame_)
        {
            while (NUM_BYTES_AVAILABLE_TO_READ(tail_idx_, head_idx_at_read, kBufferSize) > 0)
            {
                if (buffer_[tail_idx_] == kStartOfFrameByte)
                {
                    std::cout << "Error: Found start of frame byte before end of frame byte! Skipping this frame."
                              << std::endl;
                    searching_for_end_of_frame_ = false;
                    searching_for_start_of_frame_ = true;
                    break;
                }
                else if (buffer_[tail_idx_] == kEndOfFrameByte)
                {
                    const size_t end_of_frame_idx = tail_idx_;

                    raw_data_frames.emplace_back(buffer_, kBufferSize, start_of_frame_idx_, end_of_frame_idx);

                    searching_for_end_of_frame_ = false;
                    searching_for_start_of_frame_ = true;

                    tail_idx_ = (tail_idx_ + 1U) % kBufferSize;

                    break;
                }

                tail_idx_ = (tail_idx_ + 1U) % kBufferSize;
            }
        }
    }

    return raw_data_frames;
}

void SerialInterface::publishData(const uint8_t* const data, const size_t size)
{
    serial_port_.writeBytes(data, size);
}
