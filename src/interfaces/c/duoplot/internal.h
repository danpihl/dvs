#ifndef DUOPLOT_INTERNAL_H_
#define DUOPLOT_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>

#include "duoplot/communication_header.h"
#include "duoplot/constants.h"
#include "duoplot/internal.h"
#include "duoplot/math/math.h"
#include "duoplot/pp.h"
#include "duoplot/structures.h"
#include "duoplot/uint8_array.h"

DUOPLOT_WEAK uint8_t duoplot_internal_isBigEndian()
{
    const uint32_t x = 1;
    const uint8_t* const ptr = (uint8_t*)(&x);

    if (ptr[0] == '\x01')
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

DUOPLOT_WEAK uint64_t duoplot_internal_minVal(const uint64_t v0, const uint64_t v1)
{
    if (v0 < v1)
    {
        return v0;
    }
    else
    {
        return v1;
    }
}

DUOPLOT_WEAK int* duoplot_internal_getSocketFileDescriptor()
{
    static int sock_file_descr = -1;
    return &sock_file_descr;
}

DUOPLOT_WEAK bool* duoplot_internal_getIsInitialized()
{
    static bool is_initialized = false;
    return &is_initialized;
}

DUOPLOT_WEAK void duoplot_internal_initializeTcpSocket()
{
    int* const tcp_sockfd = duoplot_internal_getSocketFileDescriptor();
    struct sockaddr_in tcp_servaddr;

    *tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&tcp_servaddr, sizeof(tcp_servaddr));

    tcp_servaddr.sin_family = AF_INET;
    tcp_servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tcp_servaddr.sin_port = htons(DUOPLOT_INTERNAL_TCP_PORT_NUM);

    if (connect(*tcp_sockfd, (struct sockaddr*)&tcp_servaddr, sizeof(tcp_servaddr)) == (-1))
    {
        printf("Failed to connect! Is receiving application running?\n");
    }
}

DUOPLOT_WEAK void duoplot_internal_sendThroughTcpInterface(const uint8_t* const data_blob, const uint64_t num_bytes)
{
    if (!(*duoplot_internal_getIsInitialized()))
    {
        duoplot_internal_initializeTcpSocket();
        (*duoplot_internal_getIsInitialized()) = true;
    }

    int* const tcp_sockfd = duoplot_internal_getSocketFileDescriptor();

    const uint64_t num_bytes_to_send = num_bytes;

    socket_send(*tcp_sockfd, &num_bytes_to_send, sizeof(uint64_t));
    socket_send(*tcp_sockfd, data_blob, num_bytes);
}

#define DUOPLOT_INTERNAL_APPEND_PROPERTIES(__hdr, __first_prop)                                                        \
    {                                                                                                                  \
        va_list __args;                                                                                                \
        va_start(__args, __first_prop);                                                                                \
                                                                                                                       \
        duoplot_Property __prp = __first_prop;                                                                         \
        duoplot_internal_CommunicationHeaderObject* __obj_ptr = (duoplot_internal_CommunicationHeaderObject*)(&__prp); \
        while (__obj_ptr->type != DUOPLOT_INTERNAL_CHOT_UNKNOWN)                                                       \
        {                                                                                                              \
            duoplot_internal_appendProperty(&__hdr, __obj_ptr);                                                        \
            __prp = va_arg(__args, duoplot_Property);                                                                  \
        }                                                                                                              \
        va_end(__args);                                                                                                \
    }

#define DUOPLOT_INTERNAL_APPEND_OBJ(__hdr, __type, __val, __target_data_type)                                  \
    {                                                                                                          \
        duoplot_internal_CommunicationHeaderObject* const __current_obj = (__hdr)->objects + (__hdr)->obj_idx; \
        __current_obj->type = __type;                                                                          \
        __current_obj->num_bytes = sizeof(__target_data_type);                                                 \
        __target_data_type __tmp_val = __val;                                                                  \
        memcpy(__current_obj->data, &__tmp_val, sizeof(__target_data_type));                                   \
        duoplot_internal_appendObjectIndexToCommunicationHeaderObjectLookupTable(                              \
            &((__hdr)->objects_lut), __type, (__hdr)->obj_idx);                                                \
        (__hdr)->obj_idx += 1;                                                                                 \
    }

DUOPLOT_WEAK void duoplot_internal_appendDims(duoplot_internal_CommunicationHeader* hdr,
                                              const duoplot_internal_CommunicationHeaderObjectType type,
                                              const duoplot_internal_Dimension2D dims)
{
    duoplot_internal_CommunicationHeaderObject* const current_obj = hdr->objects + hdr->obj_idx;
    current_obj->type = type;
    current_obj->num_bytes = 2U * sizeof(uint32_t);
    memcpy(current_obj->data, &(dims.rows), sizeof(uint32_t));
    memcpy(current_obj->data + sizeof(uint32_t), &(dims.cols), sizeof(uint32_t));
    duoplot_internal_appendObjectIndexToCommunicationHeaderObjectLookupTable(&(hdr->objects_lut), type, hdr->obj_idx);
    hdr->obj_idx += 1;
}

typedef void (*SendFunction)(const uint8_t* const, const uint64_t);

DUOPLOT_WEAK SendFunction duoplot_internal_getSendFunction()
{
    return duoplot_internal_sendThroughTcpInterface;
}

#define COMMUNICATION_HEADER_OBJECT_TYPE_TRANSMISSION_TYPE uint16_t
#define COMMUNICATION_HEADER_OBJECT_NUM_BYTES_TRANSMISSION_TYPE uint8_t

DUOPLOT_WEAK int duoplot_internal_countNumHeaderBytes(const duoplot_internal_CommunicationHeader* const hdr)
{
    // 2 for first two bytes, that indicates how many objects and
    // props there will be in the buffer
    size_t s = 2;

    s += sizeof(uint8_t);

    // Object look up table
    s += DUOPLOT_INTERNAL_COMMUNICATION_HEADER_OBJECT_LOOKUP_TABLE_SIZE;

    // Properties look up table
    s += DUOPLOT_INTERNAL_PROPERTY_LOOKUP_TABLE_SIZE;

    const size_t base_size = sizeof(COMMUNICATION_HEADER_OBJECT_TYPE_TRANSMISSION_TYPE) +
                             sizeof(COMMUNICATION_HEADER_OBJECT_NUM_BYTES_TRANSMISSION_TYPE);

    for (size_t k = 0; k < hdr->obj_idx; k++)
    {
        s += base_size + hdr->objects[k].num_bytes;
    }

    for (size_t k = 0; k < hdr->prop_idx; k++)
    {
        s += base_size + hdr->props[k].num_bytes;
    }

    s += DUOPLOT_INTERNAL_NUM_FLAGS;

    return s;
}

DUOPLOT_WEAK void duoplot_internal_fillBufferWithHeader(const duoplot_internal_CommunicationHeader* const hdr,
                                                        uint8_t* const buffer)
{
    size_t idx = 0U;
    buffer[idx] = (uint8_t)(hdr->obj_idx);
    idx++;

    buffer[idx] = (uint8_t)(hdr->prop_idx);
    idx++;

    buffer[idx] = (uint8_t)(hdr->function);
    idx++;

    // Objects look up table
    memcpy(buffer + idx, hdr->objects_lut.data, DUOPLOT_INTERNAL_COMMUNICATION_HEADER_OBJECT_LOOKUP_TABLE_SIZE);
    idx += DUOPLOT_INTERNAL_COMMUNICATION_HEADER_OBJECT_LOOKUP_TABLE_SIZE;

    // Properties look up table
    memcpy(buffer + idx, hdr->props_lut.data, DUOPLOT_INTERNAL_PROPERTY_LOOKUP_TABLE_SIZE);
    idx += DUOPLOT_INTERNAL_PROPERTY_LOOKUP_TABLE_SIZE;

    // Objects
    const duoplot_internal_CommunicationHeaderObject* const objects = hdr->objects;

    for (size_t k = 0; k < hdr->obj_idx; k++)
    {
        memcpy(&(buffer[idx]), &(objects[k].type), sizeof(COMMUNICATION_HEADER_OBJECT_TYPE_TRANSMISSION_TYPE));
        idx += sizeof(COMMUNICATION_HEADER_OBJECT_TYPE_TRANSMISSION_TYPE);

        memcpy(
            &(buffer[idx]), &(objects[k].num_bytes), sizeof(COMMUNICATION_HEADER_OBJECT_NUM_BYTES_TRANSMISSION_TYPE));
        idx += sizeof(COMMUNICATION_HEADER_OBJECT_NUM_BYTES_TRANSMISSION_TYPE);

        memcpy(&(buffer[idx]), objects[k].data, objects[k].num_bytes);
        idx += objects[k].num_bytes;
    }

    // Properties
    const duoplot_internal_CommunicationHeaderObject* const props = hdr->props;

    for (size_t k = 0; k < hdr->prop_idx; k++)
    {
        memcpy(&(buffer[idx]), &(props[k].type), sizeof(COMMUNICATION_HEADER_OBJECT_TYPE_TRANSMISSION_TYPE));
        idx += sizeof(COMMUNICATION_HEADER_OBJECT_TYPE_TRANSMISSION_TYPE);

        memcpy(&(buffer[idx]), &(props[k].num_bytes), sizeof(COMMUNICATION_HEADER_OBJECT_NUM_BYTES_TRANSMISSION_TYPE));
        idx += sizeof(COMMUNICATION_HEADER_OBJECT_NUM_BYTES_TRANSMISSION_TYPE);

        memcpy(&(buffer[idx]), props[k].data, props[k].num_bytes);
        idx += props[k].num_bytes;
    }

    memcpy(buffer + idx, hdr->flags, DUOPLOT_INTERNAL_NUM_FLAGS);
}

DUOPLOT_WEAK void duoplot_internal_sendHeaderAndByteArray(SendFunction send_function,
                                                          const uint8_t* const array,
                                                          const size_t num_bytes_from_array,
                                                          duoplot_internal_CommunicationHeader* hdr)
{
    const uint64_t num_bytes_hdr = duoplot_internal_countNumHeaderBytes(hdr);

    // + 1 bytes for endianness byte
    // + 2 * sizeof(uint64_t) for magic number and number of bytes in transfer
    const uint64_t num_bytes = num_bytes_from_array + num_bytes_hdr + 1U + 2U * sizeof(uint64_t);

    uint8_t* const data_blob = malloc(num_bytes);

    uint64_t idx = 0;
    data_blob[idx] = duoplot_internal_isBigEndian();
    idx += 1;

    const uint64_t magic_num = DUOPLOT_INTERNAL_MAGIC_NUMBER;
    memcpy(data_blob + idx, &magic_num, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    memcpy(data_blob + idx, &num_bytes, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    duoplot_internal_fillBufferWithHeader(hdr, data_blob + idx);
    idx += num_bytes_hdr;

    memcpy(&(data_blob[idx]), array, num_bytes_from_array);
    idx += num_bytes_from_array;

    send_function(data_blob, num_bytes);

    free(data_blob);
}

DUOPLOT_WEAK void duoplot_internal_sendHeaderAndTwoByteArrays(SendFunction send_function,
                                                              const uint8_t* const array0,
                                                              const size_t num_bytes_from_array0,
                                                              const uint8_t* const array1,
                                                              const size_t num_bytes_from_array1,
                                                              duoplot_internal_CommunicationHeader* hdr)
{
    const uint64_t num_bytes_hdr = duoplot_internal_countNumHeaderBytes(hdr);

    // + 1 bytes for endianness byte
    // + 2 * sizeof(uint64_t) for magic number and number of bytes in transfer
    const uint64_t num_bytes =
        num_bytes_from_array0 + num_bytes_from_array1 + num_bytes_hdr + 1U + 2U * sizeof(uint64_t);

    uint8_t* const data_blob = malloc(num_bytes);

    uint64_t idx = 0;
    data_blob[idx] = duoplot_internal_isBigEndian();
    idx += 1;

    const uint64_t magic_num = DUOPLOT_INTERNAL_MAGIC_NUMBER;
    memcpy(data_blob + idx, &magic_num, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    memcpy(data_blob + idx, &num_bytes, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    duoplot_internal_fillBufferWithHeader(hdr, data_blob + idx);
    idx += num_bytes_hdr;

    memcpy(&(data_blob[idx]), array0, num_bytes_from_array0);
    idx += num_bytes_from_array0;

    memcpy(&(data_blob[idx]), array1, num_bytes_from_array1);
    idx += num_bytes_from_array1;

    send_function(data_blob, num_bytes);

    free(data_blob);
}

DUOPLOT_WEAK void duoplot_internal_sendHeaderAndTwoVectors(SendFunction send_function,
                                                           const duoplot_Vector* const x,
                                                           const duoplot_Vector* const y,
                                                           duoplot_internal_CommunicationHeader* hdr)
{
    const uint64_t num_bytes_hdr = duoplot_internal_countNumHeaderBytes(hdr);
    const uint64_t num_bytes_one_vector = x->num_elements * x->num_bytes_per_element;

    const uint64_t num_bytes_from_object = 2U * num_bytes_one_vector + num_bytes_hdr;

    // + 1 bytes for endianness byte
    // + 2 * sizeof(uint64_t) for magic number and number of bytes in transfer
    const uint64_t num_bytes = num_bytes_from_object + 1U + 2U * sizeof(uint64_t);

    uint8_t* const data_blob = malloc(num_bytes);

    uint64_t idx = 0;
    data_blob[idx] = duoplot_internal_isBigEndian();
    idx += 1;

    const uint64_t magic_num = DUOPLOT_INTERNAL_MAGIC_NUMBER;
    memcpy(data_blob + idx, &magic_num, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    memcpy(data_blob + idx, &num_bytes, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    duoplot_internal_fillBufferWithHeader(hdr, data_blob + idx);
    idx += num_bytes_hdr;

    memcpy(data_blob + idx, x->data, num_bytes_one_vector);
    idx += num_bytes_one_vector;

    memcpy(data_blob + idx, y->data, num_bytes_one_vector);
    idx += num_bytes_one_vector;

    send_function(data_blob, num_bytes);

    free(data_blob);
}

DUOPLOT_WEAK void duoplot_internal_sendHeaderAndThreeMatrices(SendFunction send_function,
                                                              const duoplot_Matrix* const x,
                                                              const duoplot_Matrix* const y,
                                                              const duoplot_Matrix* const z,
                                                              duoplot_internal_CommunicationHeader* hdr)
{
    const uint64_t num_bytes_hdr = duoplot_internal_countNumHeaderBytes(hdr);
    const uint64_t num_bytes_one_matrix = x->num_rows * x->num_cols * x->num_bytes_per_element;

    const uint64_t num_bytes_from_object = 3U * num_bytes_one_matrix + num_bytes_hdr;

    // + 1 bytes for endianness byte
    // + 2 * sizeof(uint64_t) for magic number and number of bytes in transfer
    const uint64_t num_bytes = num_bytes_from_object + 1U + 2U * sizeof(uint64_t);

    uint8_t* const data_blob = malloc(num_bytes);

    uint64_t idx = 0;
    data_blob[idx] = duoplot_internal_isBigEndian();
    idx += 1;

    const uint64_t magic_num = DUOPLOT_INTERNAL_MAGIC_NUMBER;
    memcpy(data_blob + idx, &magic_num, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    memcpy(data_blob + idx, &num_bytes, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    duoplot_internal_fillBufferWithHeader(hdr, data_blob + idx);
    idx += num_bytes_hdr;

    memcpy(&(data_blob[idx]), x->data, num_bytes_one_matrix);
    idx += num_bytes_one_matrix;

    memcpy(&(data_blob[idx]), y->data, num_bytes_one_matrix);
    idx += num_bytes_one_matrix;

    memcpy(&(data_blob[idx]), z->data, num_bytes_one_matrix);
    idx += num_bytes_one_matrix;

    send_function(data_blob, num_bytes);

    free(data_blob);
}

DUOPLOT_WEAK void duoplot_internal_sendHeaderAndThreeVectors(SendFunction send_function,
                                                             const duoplot_Vector* const x,
                                                             const duoplot_Vector* const y,
                                                             const duoplot_Vector* const z,
                                                             duoplot_internal_CommunicationHeader* hdr)
{
    const uint64_t num_bytes_hdr = duoplot_internal_countNumHeaderBytes(hdr);
    const uint64_t num_bytes_one_vector = x->num_elements * x->num_bytes_per_element;

    const uint64_t num_bytes_from_object = 3 * num_bytes_one_vector + num_bytes_hdr;

    // + 1 bytes for endianness byte
    // + 2 * sizeof(uint64_t) for magic number and number of bytes in transfer
    const uint64_t num_bytes = num_bytes_from_object + 1U + 2U * sizeof(uint64_t);

    uint8_t* const data_blob = malloc(num_bytes);

    uint64_t idx = 0;
    data_blob[idx] = duoplot_internal_isBigEndian();
    idx += 1;

    const uint64_t magic_num = DUOPLOT_INTERNAL_MAGIC_NUMBER;
    memcpy(data_blob + idx, &magic_num, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    memcpy(data_blob + idx, &num_bytes, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    duoplot_internal_fillBufferWithHeader(hdr, data_blob + idx);
    idx += num_bytes_hdr;

    memcpy(&(data_blob[idx]), x->data, num_bytes_one_vector);
    idx += num_bytes_one_vector;

    memcpy(&(data_blob[idx]), y->data, num_bytes_one_vector);
    idx += num_bytes_one_vector;

    memcpy(&(data_blob[idx]), z->data, num_bytes_one_vector);
    idx += num_bytes_one_vector;

    send_function(data_blob, num_bytes);

    free(data_blob);
}

DUOPLOT_WEAK void duoplot_internal_sendHeader(SendFunction send_function, duoplot_internal_CommunicationHeader* hdr)
{
    const uint64_t num_bytes_hdr = duoplot_internal_countNumHeaderBytes(hdr);

    // + 1 bytes for endianness byte
    // + 2 * sizeof(uint64_t) for magic number and number of bytes in transfer
    const uint64_t num_bytes = num_bytes_hdr + 1U + 2U * sizeof(uint64_t);

    uint8_t* const data_blob = malloc(num_bytes);

    uint64_t idx = 0;
    data_blob[idx] = duoplot_internal_isBigEndian();
    idx += 1;

    const uint64_t magic_num = DUOPLOT_INTERNAL_MAGIC_NUMBER;
    memcpy(data_blob + idx, &magic_num, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    memcpy(data_blob + idx, &num_bytes, sizeof(uint64_t));
    idx += sizeof(uint64_t);

    duoplot_internal_fillBufferWithHeader(hdr, data_blob + idx);
    idx += num_bytes_hdr;

    send_function(data_blob, num_bytes);

    free(data_blob);
}

DUOPLOT_WEAK bool duoplot_internal_isSubstringInString(const char* const substring, const char* const string)
{
    if (substring == NULL || string == NULL)
    {
        return false;
    }
    else if (substring[0] == '\0' || string[0] == '\0')
    {
        return false;
    }

    const size_t substring_len = strlen(substring);
    const size_t string_len = strlen(string);

    if (substring_len > string_len)
    {
        return false;
    }

    for (size_t k = 0; k < string_len - substring_len + 1; k++)
    {
        bool match = true;

        for (size_t l = 0; l < substring_len; l++)
        {
            if (substring[l] != string[k + l])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            return true;
        }
    }

    return false;
}

DUOPLOT_WEAK bool duoplot_internal_stringEndsWith(const char* const str, const char* const suffix)
{
    if (str == NULL || suffix == NULL)
    {
        return false;
    }
    else if (str[0] == '\0' || suffix[0] == '\0')
    {
        return false;
    }

    const size_t str_len = strlen(str);
    const size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
    {
        return false;
    }

    for (size_t k = 0; k < suffix_len; k++)
    {
        if (str[str_len - suffix_len + k] != suffix[k])
        {
            return false;
        }
    }

    return true;
}

DUOPLOT_WEAK bool duoplot_internal_isDuoplotRunning()
{
    char path[1035];

    FILE* const fp = popen("ps -ef | grep duoplot", "r");
    if (fp == NULL)
    {
        printf("Failed to run command\n");
        return false;
    }

    bool duoplot_running = false;

    while (fgets(path, sizeof(path), fp) != NULL)
    {
        if (path[0] == '\0')
        {
            continue;
        }
        else if (!duoplot_internal_isSubstringInString("duoplot", path) ||
                 duoplot_internal_isSubstringInString("grep", path))
        {
            continue;
        }
        else if (duoplot_internal_stringEndsWith(path, "duoplot\n") ||
                 duoplot_internal_stringEndsWith(path, "duoplot &\n"))
        {
            duoplot_running = true;
            break;
        }
    }

    pclose(fp);

    return duoplot_running;
}

#endif  // DUOPLOT_INTERNAL_H_
