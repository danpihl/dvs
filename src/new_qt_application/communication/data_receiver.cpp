#include "communication/data_receiver.h"

#include "lumos/plotting/fillable_uint8_array.h"

DataReceiver::DataReceiver()
{
    tcp_sockfd_ = socket(AF_INET, SOCK_STREAM, 0);

    // Set reuse address that's already in use (probably by exited duoplot instance)
    int true_val = 1;
    setsockopt(tcp_sockfd_, SOL_SOCKET, SO_REUSEADDR, &true_val, sizeof(int));

    bzero(&tcp_servaddr_, sizeof(tcp_servaddr_));

    tcp_servaddr_.sin_family = AF_INET;
    tcp_servaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_servaddr_.sin_port = htons(lumos::internal::kTcpPortNum);

    if ((bind(tcp_sockfd_, (struct sockaddr*)&tcp_servaddr_, sizeof(tcp_servaddr_))) != 0)
    {
        throw std::runtime_error("Socket bind failed...");
    }

    tcp_len_ = sizeof(tcp_cli_);

    if ((listen(tcp_sockfd_, 5)) != 0)
    {
        throw std::runtime_error("Socket listen failed...");
    }

    is_connected_ = false;
}

ReceivedData DataReceiver::receiveAndGetDataFromTcp()
{
    if(!is_connected_)
    {
        tcp_connfd_ = accept(tcp_sockfd_, (struct sockaddr*)&tcp_cli_, &tcp_len_);
        is_connected_ = true;
    }

    if (tcp_connfd_ < 0)
    {
        throw std::runtime_error("Server accept failed...");
    }

    size_t num_expected_bytes = 0U;
    const ssize_t num_read_bytes = read(tcp_connfd_, &num_expected_bytes, sizeof(uint64_t));
    if(num_read_bytes == 0 || num_expected_bytes == 0)
    {
        is_connected_ = false;
        close(tcp_connfd_);

        return ReceivedData{};
    }

    ReceivedData received_data{num_expected_bytes};
    char* rec_buffer = reinterpret_cast<char*>(received_data.rawData());

    size_t total_num_received_bytes = 0U;
    size_t num_bytes_left = num_expected_bytes;

    while (true)
    {
        const ssize_t num_received_bytes = read(tcp_connfd_, rec_buffer + total_num_received_bytes, num_bytes_left);

        if(num_received_bytes == 0)
        {
            is_connected_ = false;
            close(tcp_connfd_);

            return ReceivedData{};
        }

        total_num_received_bytes += num_received_bytes;
        num_bytes_left -= static_cast<size_t>(num_received_bytes);

        if (total_num_received_bytes >= num_expected_bytes)
        {
            break;
        }
    }

    uint64_t received_magic_num;
    std::memcpy(&received_magic_num, rec_buffer + 1, sizeof(uint64_t));  // +1 because first byte is endianness

    if (received_magic_num != lumos::internal::kMagicNumber)
    {
        close(tcp_connfd_);
        throw std::runtime_error("Invalid magic number received!");
    }

    received_data.parseHeader();

    return received_data;
}

DataReceiver::~DataReceiver() {}
