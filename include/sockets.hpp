#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <string>


namespace sockets {
    // Send a whole string and return true on success.
    bool send(std::string command, int sock);
}

#endif
