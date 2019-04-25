#include <sockets.hpp>

#include <string>

#include <sys/socket.h>


namespace sockets {
    bool send(std::string message, int sock) {
        const char* buffer_ptr = message.c_str();
        size_t bytes_left = message.length();
        while (bytes_left > 0) {
            ssize_t bytes_sent = ::send(sock, buffer_ptr, bytes_left, 0);
            if (bytes_sent == -1) {
                return false;
            }
            buffer_ptr += bytes_sent;
            bytes_left -= bytes_sent;
        }
        return true;
    }
}
