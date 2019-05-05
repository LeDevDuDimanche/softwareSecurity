#include <sockets.hpp>

#include <string>

#include <poll.h>
#include <fstream>
#include <sys/socket.h>


namespace sockets {
    const int defaultTimeout = 1500;    // ms
    const int defaultWait = 1500;    // ms

    void receive_N(const int sock, std::ostream *outputBuffer, unsigned int n) {
        char buffer[4096];
        struct pollfd pollFds[1];
        pollFds[0].fd = sock;
        pollFds[0].events = POLLIN;
 
        int wait = defaultTimeout;    // initial value
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
                int min = sizeof buffer < n ? sizeof buffer : n;
                int recvVal = recv(sock, buffer, min, 0);
                if (recvVal < 0) {
                    if (errno == EWOULDBLOCK) {
                        break;
                    }
                }
 
                if (recvVal == 0) {
                    throw SocketError("Socket connection closed");
                } 

                outputBuffer->write(buffer, recvVal);
                n -= recvVal;
                if (n == 0) {
                    break;
                }
            }
            if (n == 0) {
                break;
            }

            wait = defaultWait;
        }
    }

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

    void receive_all(const int sock, std::ostream *outputBuffer) {
        receive_all(sock, defaultTimeout, defaultWait, outputBuffer);
    }

/*
 *   I could add an optional value indicating the expected number of bytes to receive.
 *   I could make it so that it prints to a stream instead
*/
    void receive_all(const int sock, int timeout, int waitBetween, std::ostream *outputBuffer) {
        char buffer[4096];
        struct pollfd pollFds[1];
        pollFds[0].fd = sock;
        pollFds[0].events = POLLIN;
 
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

                outputBuffer->write(buffer, recvVal - 1);
                if (buffer[recvVal - 1] == end_of_transmission) { 
                    return;
                } else {
                    outputBuffer->write(buffer + recvVal - 1, 1);
                } 
            }

            wait = waitBetween;
        }
    }
}
