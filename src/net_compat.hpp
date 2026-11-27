#pragma once

#include <cstddef>
#include <cstdint>
#include <climits>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace net_compat {

#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket invalid_socket = INVALID_SOCKET;

inline bool initialize(std::string& error) {
    WSADATA data{};
    const int result = WSAStartup(MAKEWORD(2, 2), &data);
    if (result != 0) {
        error = "WSAStartup failed (code " + std::to_string(result) + ")";
        return false;
    }
    return true;
}

inline void cleanup() {
    WSACleanup();
}

inline void close_socket(Socket socket) {
    if (socket != invalid_socket) {
        closesocket(socket);
    }
}

inline bool shutdown_send(Socket socket) {
    return shutdown(socket, SD_SEND) == 0;
}

inline int last_socket_error() {
    return WSAGetLastError();
}

inline bool is_interrupted(int error) {
    return error == WSAEINTR;
}

inline std::int64_t socket_send(Socket socket, const char* data, std::size_t size) {
    const int chunk_size = size > static_cast<std::size_t>(INT_MAX)
        ? INT_MAX
        : static_cast<int>(size);
    return static_cast<std::int64_t>(send(socket, data, chunk_size, 0));
}

inline std::int64_t socket_recv(Socket socket, char* data, std::size_t size) {
    const int chunk_size = size > static_cast<std::size_t>(INT_MAX)
        ? INT_MAX
        : static_cast<int>(size);
    return static_cast<std::int64_t>(recv(socket, data, chunk_size, 0));
}
#else
using Socket = int;
constexpr Socket invalid_socket = -1;

inline bool initialize(std::string&) {
    return true;
}

inline void cleanup() {}

inline void close_socket(Socket socket) {
    if (socket != invalid_socket) {
        close(socket);
    }
}

inline bool shutdown_send(Socket socket) {
    return shutdown(socket, SHUT_WR) == 0;
}

inline int last_socket_error() {
    return errno;
}

inline bool is_interrupted(int error) {
    return error == EINTR;
}

inline std::int64_t socket_send(Socket socket, const char* data, std::size_t size) {
#ifdef MSG_NOSIGNAL
    constexpr int send_flags = MSG_NOSIGNAL;
#else
    constexpr int send_flags = 0;
#endif
    return static_cast<std::int64_t>(send(socket, data, size, send_flags));
}

inline std::int64_t socket_recv(Socket socket, char* data, std::size_t size) {
    return static_cast<std::int64_t>(recv(socket, data, size, 0));
}
#endif

inline std::string socket_error_message(const char* operation, int error) {
    return std::string(operation) + " failed (socket error " +
        std::to_string(error) + ")";
}

}  // namespace net_compat
