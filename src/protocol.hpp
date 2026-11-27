#pragma once

#include "net_compat.hpp"

#include <cstdint>
#include <string>

namespace protocol {

constexpr std::uint32_t max_frame_size = 1024U * 1024U;

enum class ReceiveStatus {
    Ok,
    CleanEof,
    Error
};

inline bool send_all(net_compat::Socket socket, const void* data,
                     std::size_t size, std::string* error = nullptr) {
    const char* bytes = static_cast<const char*>(data);
    std::size_t sent = 0;

    while (sent < size) {
        const std::int64_t result =
            net_compat::socket_send(socket, bytes + sent, size - sent);
        if (result > 0) {
            sent += static_cast<std::size_t>(result);
            continue;
        }
        if (result == 0) {
            if (error != nullptr) {
                *error = "socket closed while sending";
            }
            return false;
        }

        const int socket_error = net_compat::last_socket_error();
        if (net_compat::is_interrupted(socket_error)) {
            continue;
        }
        if (error != nullptr) {
            *error = net_compat::socket_error_message("send", socket_error);
        }
        return false;
    }
    return true;
}

inline ReceiveStatus recv_all(net_compat::Socket socket, void* data,
                              std::size_t size, std::string* error = nullptr,
                              bool allow_clean_eof = false) {
    char* bytes = static_cast<char*>(data);
    std::size_t received = 0;

    while (received < size) {
        const std::int64_t result =
            net_compat::socket_recv(socket, bytes + received, size - received);
        if (result > 0) {
            received += static_cast<std::size_t>(result);
            continue;
        }
        if (result == 0) {
            if (received == 0 && allow_clean_eof) {
                return ReceiveStatus::CleanEof;
            }
            if (error != nullptr) {
                *error = "connection closed before all requested bytes arrived";
            }
            return ReceiveStatus::Error;
        }

        const int socket_error = net_compat::last_socket_error();
        if (net_compat::is_interrupted(socket_error)) {
            continue;
        }
        if (error != nullptr) {
            *error = net_compat::socket_error_message("recv", socket_error);
        }
        return ReceiveStatus::Error;
    }
    return ReceiveStatus::Ok;
}

inline bool send_frame(net_compat::Socket socket, const std::string& payload,
                       std::string* error = nullptr) {
    if (payload.empty() || payload.size() > max_frame_size) {
        if (error != nullptr) {
            *error = "frame payload size must be between 1 byte and 1 MiB";
        }
        return false;
    }

    const std::uint32_t network_length =
        htonl(static_cast<std::uint32_t>(payload.size()));
    if (!send_all(socket, &network_length, sizeof(network_length), error)) {
        return false;
    }
    return send_all(socket, payload.data(), payload.size(), error);
}

inline ReceiveStatus recv_frame(net_compat::Socket socket, std::string& payload,
                                std::string* error = nullptr) {
    std::uint32_t network_length = 0;
    const ReceiveStatus header_status =
        recv_all(socket, &network_length, sizeof(network_length), error, true);
    if (header_status != ReceiveStatus::Ok) {
        return header_status;
    }

    const std::uint32_t payload_length = ntohl(network_length);
    if (payload_length == 0 || payload_length > max_frame_size) {
        if (error != nullptr) {
            *error = "received frame size is outside the 1-byte to 1-MiB limit";
        }
        return ReceiveStatus::Error;
    }

    std::string received_payload(payload_length, '\0');
    const ReceiveStatus payload_status =
        recv_all(socket, &received_payload[0], payload_length, error);
    if (payload_status != ReceiveStatus::Ok) {
        return ReceiveStatus::Error;
    }

    payload.swap(received_payload);
    return ReceiveStatus::Ok;
}

}  // namespace protocol
