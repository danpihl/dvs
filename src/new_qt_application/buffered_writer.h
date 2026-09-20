#ifndef BUFFERED_WRITER_H
#define BUFFERED_WRITER_H

#include <cstring>

class BufferedWriter
{
private:
    uint8_t* const buffer_;
    size_t idx_;
    size_t size_;

public:
    BufferedWriter() = delete;

    BufferedWriter(uint8_t* const buffer, const size_t buffer_size) : buffer_{buffer}, idx_{0U}, size_{buffer_size} {}

    BufferedWriter(const BufferedWriter& other) = delete;
    BufferedWriter(BufferedWriter&& other) = delete;
    BufferedWriter& operator=(const BufferedWriter& other) = delete;
    BufferedWriter& operator=(BufferedWriter&& other) = delete;

    ~BufferedWriter() {}

    template <typename T> void write(const T& value)
    {
        if (idx_ + sizeof(T) > size_)
        {
            throw std::runtime_error("BufferedWriter: Buffer overflow");
        }
        std::memcpy(buffer_ + idx_, &value, sizeof(T));
        idx_ += sizeof(T);
    }

    void advance(const size_t n_bytes)
    {
        idx_ += n_bytes;
    }

    /*bool hasWrittenToEnd() const
    {
        return idx_ == size_;
    }*/

    uint8_t* data() const
    {
        return buffer_;
    }

    size_t idx() const
    {
        return idx_;
    }
};

#endif  // BUFFERED_WRITER_H