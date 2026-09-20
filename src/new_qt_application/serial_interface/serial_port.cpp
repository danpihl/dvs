#include "serial_port.h"

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

#include "definitions.h"

SerialPort::SerialPort(const std::string& port_name, const int32_t baudrate)
{
    fd_ = ::open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ == -1)
    {
        return;
    }

    reconfigurePort(baudrate);
}

SerialPort::~SerialPort()
{
    ::close(fd_);
}

void SerialPort::writeBytes(const uint8_t* const buf, const size_t size)
{
    ::write(fd_, buf, size);
}

void SerialPort::reconfigurePort(const int32_t baudrate)
{
    struct termios options;

    options.c_cflag |= (tcflag_t)(CLOCAL | CREAD);
    options.c_lflag &= (tcflag_t) ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);

    options.c_oflag &= (tcflag_t) ~(OPOST);
    options.c_iflag &= (tcflag_t) ~(INLCR | IGNCR | ICRNL | IGNBRK);

    options.c_iflag &= (tcflag_t)~PARMRK;

    speed_t baud = baudrate;

    ::cfsetispeed(&options, baud);
    ::cfsetospeed(&options, baud);

    options.c_iflag &= (tcflag_t) ~(INPCK | ISTRIP);
    options.c_cflag &= (tcflag_t) ~(PARENB | PARODD);

    options.c_cflag &= (tcflag_t) ~(CSTOPB);

    options.c_cflag &= (tcflag_t)~CSIZE;

    options.c_cflag |= CS8;

    options.c_cflag &= ~CRTSCTS;

    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    options.c_oflag &= ~OPOST;

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 20;

    ::tcsetattr(fd_, TCSANOW, &options);
}

size_t SerialPort::numAvailableBytes()
{
    int32_t count = 0;

    if (-1 == ioctl(fd_, TIOCINQ, &count))
    {
        return 0;
    }
    else
    {
        return static_cast<size_t>(count);
    }
}

ssize_t SerialPort::readBytes(uint8_t* buf, const size_t size)
{
    return ::read(fd_, buf, size);
}

void SerialPort::flushPort()
{
    tcflush(fd_, TCIOFLUSH);
}
