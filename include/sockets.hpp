#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <stdexcept>
#include <string>


namespace sockets {
    // Send the full message and throw SocketError on error.
    void send(std::string command, int sock);

    // Try to receive the whole message.
    std::string receive_all(int sock);
    std::string receive_all(int sock, int timeout, int waitBetween);

    class SocketError: public virtual std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}

#endif
