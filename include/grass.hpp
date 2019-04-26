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

#define PORT_NUMBER_GET_KEYWORD "PORT_NUMBER_GET"

#include <stdlib.h>
#include <stdio.h>
#define SOCKET_BUFFER_SIZE 1024 
#define server_failure(msg) \
    perror(msg); \
    exit(EXIT_FAILURE);

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
