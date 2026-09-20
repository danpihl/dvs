#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include <stdint.h>

#include <vector>

#include "definitions.h"
#include "raw_data_frame.h"
#include "serial_port.h"

class SerialInterface
{
private:
    SerialPort serial_port_;
    bool searching_for_start_of_frame_;
    bool searching_for_end_of_frame_;
    size_t head_idx_;
    size_t tail_idx_;
    size_t start_of_frame_idx_;
    uint8_t* const buffer_;

    std::thread* serial_receive_thread_;

public:
    SerialInterface() = delete;
    SerialInterface(const std::string& port_name, const int32_t baudrate);

    void readSerialDataThreadFunction();
    void readSerialData();
    std::vector<RawDataFrame> extractRawDataFrames();
    void publishData(const uint8_t* const data, const size_t size);
    void start();
};

#endif  // SERIAL_INTERFACE_H
