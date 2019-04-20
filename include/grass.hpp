#ifndef GRASS_HPP
#define GRASS_HPP

#define DEBUG true

// The following headers were included in the template, it should be safe to
// uncomment them if needed.
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <netdb.h>
// #include <fcntl.h>
// #include <stdbool.h>


#define SOCKET_BUFFER_SIZE 1024
#define PORT 31337


struct User {
    const char* uname;
    const char* pass;

    bool isLoggedIn;
};

struct Command {
    const char* cname;
    const char* cmd;
    const char* params;
};

void hijack_flow();

#endif
