#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include<iostream>
#include <string>


namespace sockets {
    const char end_of_transmission = '\x04';

    // Send the full message and throw SocketError on error.
    void send_all(std::string command, int sock);

    // Try to receive the whole message and throw SocketError on error.

    void receive_all(const int sock, std::ostream *outputBuffer); 
    void receive_all(const int sock, int timeout, int waitBetween, std::ostream *outputBuffer); 

    void receive_N(const int sock, std::ostream *outputBuffer, unsigned int n);

    class SocketError: public virtual std::runtime_error { 
        using std::runtime_error::runtime_error;
    };
}

#endif
