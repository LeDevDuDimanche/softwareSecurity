#include <sockets.hpp>

#include <string>

#include <poll.h>
#include <sys/socket.h>


namespace sockets {
    const int defaultTimeout = 1500;    // ms
    const int defaultWait = 1500;    // ms

    void send_all(std::string message, int sock) {
        const char* buffer_ptr = message.c_str();
        size_t bytes_left = message.length();
        while (bytes_left > 0) {
            ssize_t bytes_sent = send(sock, buffer_ptr, bytes_left, 0);
            if (bytes_sent == -1) {
                throw SocketError("Couldn't send to socket");
            }
            buffer_ptr += bytes_sent;
            bytes_left -= bytes_sent;
        }
    }

    std::string receive_all(int sock) {
        return receive_all(sock, defaultTimeout, defaultWait);
    }

    std::string receive_all(int sock, int timeout, int waitBetween) {
        char buffer[4096];
        struct pollfd pollFds[1];
        pollFds[0].fd = sock;
        pollFds[0].events = POLLIN;

        std::string message = "";
        int wait = timeout;    // initial value
        for (;;) {
            int pollVal = poll(pollFds, 1, wait);

            if (pollVal < 0) {
                throw SocketError("Couldn't poll socket");
            }

            if (pollVal == 0) {
                // Timed out.
                throw SocketError("Receiving message timed out");
            }

            if (pollFds[0].revents != POLLIN) {
                throw SocketError("Couldn't receive from socket");
            }

            while (true) {
                int recvVal = recv(sock, buffer, sizeof buffer, 0);
                if (recvVal < 0) {
                    if (errno == EWOULDBLOCK) {
                        break;
                    }
                }
                if (recvVal == 0) {
                    throw SocketError("Socket connection closed");
                }
                message.append(buffer, recvVal);
                if (buffer[recvVal - 1] == end_of_transmission) {
                    message.pop_back();    // Remove end-of-transmission byte
                    return message;
                }
            }

            wait = waitBetween;
        }
    }
}
