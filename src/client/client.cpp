#include <grass.hpp>

#include <fstream>
#include <iostream>
#include <string>

#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sockets.hpp>


/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to send
 * d_port: destination port
 */
void send_file(int fp, int d_port) {
    // TODO
    fp += d_port; // just to get rid of warnings
}

/*
 * Recv a file from the server as its own thread
 *
 * fp: file descriptor of file to save to.
 * d_port: destination port
 * size: the size (in bytes) of the file to recv
 */
void recv_file(int fp, int d_port, int size) {
    // TODO
    fp += d_port * size; // just to get rid of warnings
}

void print_usage(int argc, const char* argv[]) {
    std::string program_name = "client";
    if (argc > 0 && argv[0][0] != '\0') {
        program_name = argv[0];
    }
    std::cerr << "Usage: " << program_name << " server_ip server_port "
                 "[in_file out_file]\n";
}

struct printer_handler_params {
    int sockfd;
    std::ostream *output_stream_ptr;
};

void* printer_handler(void* params) {
    printer_handler_params *handler_params = (printer_handler_params*) params;
    int valread;
    char buffer[SOCKET_BUFFER_SIZE] = {0};


    while ((valread = read(handler_params->sockfd, buffer, SOCKET_BUFFER_SIZE)) > 0 && valread < SOCKET_BUFFER_SIZE) {
        std::string input_copy = std::string(buffer, valread);
        (* (handler_params->output_stream_ptr)) << input_copy;
    }
    //TODO handle errors

    return nullptr;
}

int main(int argc, const char* argv[]) {
    if (argc != 3 && argc != 5) {
        print_usage(argc, argv);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;

    if(inet_aton(argv[1], &(server_addr.sin_addr)) == 0) {
        std::cerr << "Invalid address " << '"' << argv[1] << '"' << "\n";
        return EXIT_FAILURE;
    }

    char* end;
    long port = std::strtol(argv[2], &end, 10);
    if (errno != 0 || *end != '\0' || port <= 0 || port >= 0x10000) {
        std::cerr << "Invalid port " << '"' << argv[2] << '"' << "\n";
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons(port);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Socket creation error\n";
        return EXIT_FAILURE;
    }

    struct sockaddr* connect_addr = (struct sockaddr*) &server_addr;
    if (connect(sock, connect_addr, sizeof *connect_addr) != 0) {
        std::cerr << "Couldn't connect to server\n";
        return EXIT_FAILURE;
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "Error setting socket non-blocking\n";
        return EXIT_FAILURE;
    }

    std::istream* in;
    std::ostream* out;
    std::ifstream in_file;
    std::ofstream out_file;
    if (argc == 5) {
        in_file = std::ifstream(argv[3]);
        if (!in_file) {
            std::cerr << "Couldn't open in_file\n";
            return EXIT_FAILURE;
        }
        in = &in_file;

        out_file = std::ofstream(argv[4]);
        if (!out_file) {
            std::cerr << "Couldn't open out_file\n";
            return EXIT_FAILURE;
        }
        out = &out_file;
    } else {
        in = &std::cin;
        out = &std::cout;
    }

    std::string command;
    std::string response;
    while (std::getline(*in, command)) {
        try {
            sockets::send(command + '\n', sock);
        } catch (sockets::SocketError& e) {
            std::cerr << "Couldn't send command to server\n";
            return EXIT_FAILURE;
        }

        try {
            response = sockets::receive(sock);
        } catch (sockets::SocketError& e) {
            std::cerr << "Couldn't receive server response\n";
            return EXIT_FAILURE;
        }
        *out << response << std::flush;

        // TODO: handle the special cases of a get and put command
    }

    // TODO: did we break successfully or was it a failure?
    // TODO: close files properly
    return EXIT_SUCCESS;
}
