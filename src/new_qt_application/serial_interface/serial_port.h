#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <errno.h>
#include <fcntl.h>
#ifdef PLATFORM_APPLE_M
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#include <paths.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sysexits.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <sstream>
#include <thread>

class SerialPort
{
private:
    int32_t fd_;

    void reconfigurePort(const int32_t baudrate);

public:
    SerialPort() = delete;
    SerialPort(const std::string& port_name, const int32_t baudrate);
    ~SerialPort();

    size_t numAvailableBytes();
    ssize_t readBytes(uint8_t* buf, const size_t size);
    void writeBytes(const uint8_t* const buf, const size_t size);
    void flushPort();
    bool isValid() const
    {
        return fd_ != -1;
    }
};

#endif  // SERIAL_PORT_H
