#include <grass.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <cstdlib>

#include <parsing.hpp>
#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <parsing.hpp>
#include <sockets.hpp>
#include <regex>



static bool watching_for_port_number = false;

#define SPACES "[ \t]*"

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
 



int create_socket(const char *server_ip, const char *server_port, long *ret_sock) {

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;

    if(inet_aton(server_ip, &(server_addr.sin_addr)) == 0) {
        std::cerr << "Invalid address " << '"' << server_ip << '"' << "\n";
        return EXIT_FAILURE;
    }

    char* end;
    long port = std::strtol(server_port, &end, 10);

    if (errno != 0 || *end != '\0' || port <= 0 || port >= 0x10000) {
        std::cerr << "Invalid port " << '"' << port << '"' << "\n";
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons(port);

    *ret_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*ret_sock == -1) {
        std::cerr << "Socket creation error\n";
        return EXIT_FAILURE;
    }

    struct sockaddr* connect_addr = (struct sockaddr*) &server_addr;
    if (connect(*ret_sock, connect_addr, sizeof *connect_addr) != 0) {
        std::cerr << "Couldn't connect to server\n";
        return EXIT_FAILURE;
    }

    if (fcntl(*ret_sock, F_SETFL, O_NONBLOCK) == -1) {
        std::cerr << "Error setting socket non-blocking\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;

}

struct get_handler_args {
    const char *server_ip;
    const std::string *server_port;
    const long *filesize_ptr;
    std::string* filename_ptr;
};

void *get_handler(void *handler_args) { 
    std::flush(std::cout);
    get_handler_args *args = (get_handler_args *) handler_args; 
    #define GET_HANDLER_DEFAULT_EXIT \
        delete args->server_port; \
        delete args -> filesize_ptr; \
        delete args -> filename_ptr; \
        return NULL;
    
    long ret_socket;
    if (create_socket(args->server_ip, args->server_port->c_str(), &ret_socket) == EXIT_FAILURE) {
        std::cerr << "could not create socket in get handler of the client" << std::flush;
        GET_HANDLER_DEFAULT_EXIT
    } 

    std::ofstream *out_file = new std::ofstream(*(args -> filename_ptr));

    sockets::receive_all(ret_socket, out_file);

    out_file->close();

    GET_HANDLER_DEFAULT_EXIT
}

int main(int argc, const char* argv[]) {
    if (argc != 3 && argc != 5) {
        print_usage(argc, argv);
        return EXIT_FAILURE;
    }


    const char *server_ip = argv[1];
    const char *server_port = argv[2]; 
    long sock;

    if (create_socket(server_ip, server_port, &sock) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }




    //TODO close socket connection on every error (EXIT FAILURE). Have to do it on server_fault in server source code too.
    // delete the printer thread too in case of error. 
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

    #define NUMBER_PATTERN "(\\d+)"
    std::regex PORT_NUMBER_GET_REGEX (SPACES PORT_NUMBER_GET_KEYWORD SPACES NUMBER_PATTERN SPACES GET_SIZE_KEYWORD NUMBER_PATTERN SPACES);
    std::string command;
    std::string response;
    std::string *get_file_name = NULL;
 
    std::ostringstream *output_buffer = new std::ostringstream();
    while (std::getline(*in, command)) {
        output_buffer->clear();
        output_buffer->str("");

        std::vector<std::string> parts = Parsing::split_string(command, ' ');
        if (parts.size() == 0) {
            continue;
        }
        std::string command_name = parts[0];


        try {
            sockets::send_all(command + '\n', sock);
        } catch (sockets::SocketError& e) {
            std::cerr << "Couldn't send command to server\n";
            return EXIT_FAILURE;
        }

        if (command_name == "get") {
            watching_for_port_number = true;
            get_file_name = new std::string();
            (*get_file_name).append(parts[1]);
        }

        if (command_name == "put") {
            // TODO
        }

        try {
            sockets::receive_all(sock, output_buffer);
        } catch (sockets::SocketError& e) {
            std::cerr << "Couldn't receive server response\n";
            return EXIT_FAILURE;
        }  
        
        response = output_buffer->str();
        *out << response << std::flush;
         
        if (watching_for_port_number) {
            std::smatch port_matches;
            if (std::regex_search(response, port_matches, PORT_NUMBER_GET_REGEX)) {
                std::string *get_port_number = new std::string();
                (*get_port_number).append(port_matches.str(1)); 

                long *filesize_ptr = new long(std::stol(port_matches.str(2)));
                watching_for_port_number = false; 
 
                pthread_t get_thread;
                long ret_create;

                get_handler_args *args = new get_handler_args {
                    server_ip, 
                    get_port_number,
                    filesize_ptr,
                    get_file_name
                };
                //TODO delete filesize in the get_handler
                if ((ret_create = pthread_create(&get_thread, NULL /*default attributes*/,
                            get_handler, (void *) args))) {
                    std::cerr << "cannot create thread for get command response handler\n";
                    delete filesize_ptr;
                    delete get_port_number;
                    return EXIT_FAILURE;
                }   
            }
        }



        if (command_name == "exit" && parts.size() == 1) {
            break;
        }


    }

    return EXIT_SUCCESS;
}
