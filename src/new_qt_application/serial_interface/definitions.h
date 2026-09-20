#ifndef SERIAL_TEST_DEFINITIONS_H
#define SERIAL_TEST_DEFINITIONS_H

#ifndef TIOCINQ
#ifdef FIONREAD
#define TIOCINQ FIONREAD
#else
#define TIOCINQ 0x541B
#endif
#endif

constexpr size_t kBufferSize = 1024U * 1024U * 8U;

constexpr uint8_t kStartOfFrameByte = 0x7E;
constexpr uint8_t kEndOfFrameByte = 0x7F;
constexpr uint8_t kEscapeByte = 0x7D;
constexpr uint8_t kEscapeXOR = 0x20;

#define NUM_BYTES_AVAILABLE_TO_READ(__tail_idx, __head_idx, __buffer_size) \
    ((__tail_idx) <= (__head_idx)) ? ((__head_idx) - (__tail_idx)) : ((__head_idx) + (__buffer_size) - (__tail_idx))

enum class NumberDataType : uint8_t
{
    kFloat,
    kDouble,
    kInt8,
    kInt16,
    kInt32,
    kInt64,
    kUInt8,
    kUInt16,
    kUInt32,
    kUInt64
};

enum class ObjectType : uint8_t
{
    kNumber,
    kString,
    kArray,
    kFunction,
    kBlob
};

using TopicId = uint16_t;
constexpr TopicId kUnknownTopicId = 0xFFFFU;

#define READ_ESCAPED_BYTE(input_buffer, idx, output_data, cast_type)                   \
    {                                                                                  \
        if (input_buffer[idx] == kEscapeByte)                                          \
        {                                                                              \
            output_data = static_cast<cast_type>(input_buffer[idx + 1U] ^ kEscapeXOR); \
            idx += 2U;                                                                 \
        }                                                                              \
        else                                                                           \
        {                                                                              \
            output_data = static_cast<cast_type>(input_buffer[idx]);                   \
            idx++;                                                                     \
        }                                                                              \
    }

#define READ_ESCAPED_UINT16(input_buffer, idx, output_data)                        \
    {                                                                              \
        uint8_t* const output_data_ptr = reinterpret_cast<uint8_t*>(&output_data); \
                                                                                   \
        if (input_buffer[idx] == kEscapeByte)                                      \
        {                                                                          \
            output_data_ptr[0] = input_buffer[idx + 1U] ^ kEscapeXOR;              \
            idx += 2U;                                                             \
        }                                                                          \
        else                                                                       \
        {                                                                          \
            output_data_ptr[0] = input_buffer[idx];                                \
            idx++;                                                                 \
        }                                                                          \
                                                                                   \
        if (input_buffer[idx] == kEscapeByte)                                      \
        {                                                                          \
            output_data_ptr[1] = input_buffer[idx + 1U] ^ kEscapeXOR;              \
            idx += 2U;                                                             \
        }                                                                          \
        else                                                                       \
        {                                                                          \
            output_data_ptr[1] = input_buffer[idx];                                \
            idx++;                                                                 \
        }                                                                          \
    }

#define READ_ESCAPED_DATA(input_buffer, idx, output_data)                          \
    {                                                                              \
        uint8_t* const output_data_ptr = reinterpret_cast<uint8_t*>(&output_data); \
                                                                                   \
        for (size_t __k = 0U; __k < sizeof(output_data); __k++)                    \
        {                                                                          \
            if (input_buffer[idx] == kEscapeByte)                                  \
            {                                                                      \
                output_data_ptr[__k] = input_buffer[idx + 1U] ^ kEscapeXOR;        \
                idx += 2U;                                                         \
            }                                                                      \
            else                                                                   \
            {                                                                      \
                output_data_ptr[__k] = input_buffer[idx];                          \
                idx++;                                                             \
            }                                                                      \
        }                                                                          \
    }

#endif  // SERIAL_TEST_DEFINITIONS_H
