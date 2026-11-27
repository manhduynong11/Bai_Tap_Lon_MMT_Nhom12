#include "json.hpp"
#include "net_compat.hpp"
#include "protocol.hpp"

#include <iostream>
#include <string>
#include <thread>

namespace {

constexpr int short_frame_count = 50;
constexpr std::size_t large_frame_size = 16U * 1024U;

}  // namespace

int main() {
    std::string error;
    if (!net_compat::initialize(error)) {
        std::cerr << "FAIL: " << error << '\n';
        return 1;
    }

    int result = 1;
    net_compat::Socket listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == net_compat::invalid_socket) {
        error = net_compat::socket_error_message(
            "socket", net_compat::last_socket_error());
        std::cerr << "FAIL: " << error << '\n';
        net_compat::cleanup();
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (bind(listener, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) != 0 ||
        listen(listener, 1) != 0) {
        error = net_compat::socket_error_message(
            "bind/listen", net_compat::last_socket_error());
        std::cerr << "FAIL: " << error << '\n';
        net_compat::close_socket(listener);
        net_compat::cleanup();
        return 1;
    }

#ifdef _WIN32
    int address_length = sizeof(address);
#else
    socklen_t address_length = sizeof(address);
#endif
    if (getsockname(listener, reinterpret_cast<sockaddr*>(&address),
                    &address_length) != 0) {
        error = net_compat::socket_error_message(
            "getsockname", net_compat::last_socket_error());
        std::cerr << "FAIL: " << error << '\n';
        net_compat::close_socket(listener);
        net_compat::cleanup();
        return 1;
    }

    net_compat::Socket client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == net_compat::invalid_socket) {
        error = net_compat::socket_error_message(
            "socket", net_compat::last_socket_error());
        net_compat::close_socket(listener);
        std::cerr << "FAIL: " << error << '\n';
        net_compat::cleanup();
        return 1;
    }

    if (connect(client, reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) != 0) {
        error = net_compat::socket_error_message(
            "connect", net_compat::last_socket_error());
        net_compat::close_socket(client);
        net_compat::close_socket(listener);
        std::cerr << "FAIL: " << error << '\n';
        net_compat::cleanup();
        return 1;
    }

    bool server_ok = true;
    std::string server_error;
    std::thread server([&]() {
        net_compat::Socket connection = accept(listener, nullptr, nullptr);
        if (connection == net_compat::invalid_socket) {
            server_ok = false;
            server_error = net_compat::socket_error_message(
                "accept", net_compat::last_socket_error());
            return;
        }

        int frames_received = 0;
        while (true) {
            std::string payload;
            const protocol::ReceiveStatus status =
                protocol::recv_frame(connection, payload, &server_error);
            if (status == protocol::ReceiveStatus::CleanEof) {
                break;
            }
            if (status != protocol::ReceiveStatus::Ok) {
                server_ok = false;
                break;
            }
            ++frames_received;
            if (!protocol::send_frame(connection, payload, &server_error)) {
                server_ok = false;
                break;
            }
        }

        if (frames_received != short_frame_count + 1) {
            server_ok = false;
            if (server_error.empty()) {
                server_error = "server received an unexpected number of frames";
            }
        }
        net_compat::close_socket(connection);
    });

    bool client_ok = true;
    for (int i = 0; i < short_frame_count; ++i) {
        const std::string sent = nlohmann::json{
            {"type", "TEST"},
            {"msg_id", i},
            {"payload", "short"}
        }.dump();
        if (!protocol::send_frame(client, sent, &error)) {
            client_ok = false;
            break;
        }
    }

    for (int i = 0; client_ok && i < short_frame_count; ++i) {
        std::string echoed;
        if (protocol::recv_frame(client, echoed, &error) !=
            protocol::ReceiveStatus::Ok) {
            if (error.empty()) {
                error = "failed to receive a short JSON echo";
            }
            client_ok = false;
            break;
        }
        const std::string expected = nlohmann::json{
            {"type", "TEST"},
            {"msg_id", i},
            {"payload", "short"}
        }.dump();
        if (echoed != expected) {
            error = "short JSON frame did not echo byte-for-byte";
            client_ok = false;
        }
    }

    if (client_ok) {
        nlohmann::json large_message{
            {"type", "TEST"},
            {"msg_id", "large"},
            {"payload", ""}
        };
        const std::size_t fixed_json_size = large_message.dump().size();
        large_message["payload"] =
            std::string(large_frame_size - fixed_json_size, 'x');
        const std::string large_json = large_message.dump();
        if (large_json.size() != large_frame_size ||
            !protocol::send_frame(client, large_json, &error)) {
            if (error.empty()) {
                error = "failed to create or send the exact 16 KiB JSON frame";
            }
            client_ok = false;
        } else {
            std::string echoed;
            if (protocol::recv_frame(client, echoed, &error) !=
                    protocol::ReceiveStatus::Ok ||
                echoed != large_json) {
                if (error.empty()) {
                    error = "large JSON frame did not echo byte-for-byte";
                }
                client_ok = false;
            }
        }
    }

    if (client_ok && !net_compat::shutdown_send(client)) {
        error = net_compat::socket_error_message(
            "shutdown", net_compat::last_socket_error());
        client_ok = false;
    }
    net_compat::close_socket(client);
    net_compat::close_socket(listener);
    server.join();

    if (!client_ok) {
        std::cerr << "FAIL: " << error << '\n';
    } else if (!server_ok) {
        std::cerr << "FAIL: " << server_error << '\n';
    } else {
        std::cout << "PASS: 50 short JSON frames, 16 KiB JSON frame, "
                     "and clean TCP close\n";
        result = 0;
    }

    net_compat::cleanup();
    return result;
}
