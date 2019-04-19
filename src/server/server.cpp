#include <grass.hpp>
#include <ctype.h>
#include <server/parsing.hpp>

#define IP_PROT 0
#define SOCKET_QUEUE_LENGTH 3
#define forever for(;;)
#define server_failure(msg) \
    perror(msg); \
    exit(EXIT_FAILURE);
#define HELLO_WORLD "hello world"



static struct User **userlist;
static int numUsers;
static struct Command **cmdlist;
static int numCmds;

// Helper function to run commands in unix.
void run_command(const char* command, int sock){
    std::vector<std::string> v = Parsing::split_string("bjorn", '/');
}


/*
 * Send a file to the client as its own thread
 *
 * fp: file descriptor of file to send
 * sock: socket that has already been created.
 */
void send_file(int fp, int sock) {
}

/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to save to.
 * sock: socket that has already been created.
 * size: the size (in bytes) of the file to recv
 */
void recv_file(int fp, int sock, int size) {
}

// Server side REPL given a socket file descriptor
void *connection_handler(void* sockfd) {
}

/*
 * search all files in the current directory
 * and its subdirectory for the pattern
 *
 * pattern: an extended regular expressions.
 * Output: A line seperated list of matching files' addresses
 */
void search(char *pattern) {
    // TODO
}

// Parse the grass.conf file and fill in the global variables
void parse_grass() {
}

// TODO:
// Parse the rass.conf file
// Listen to the port and handle each connection
int main() {
    int server_fd, new_socket, valread;
    //specifies a transport address and port for the Ipv4    
    struct sockaddr_in address;

    int addrlen = sizeof(address);

    char buffer[SOCKET_BUFFER_SIZE] = {0};

    //an int where we store options of the socket
    int opt = 1;

    //create a socket file descriptor for connections using the IPv4 for two way connections sending ybyte streams to each others (TCP).
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IP_PROT)) == 0)
    {
        server_failure("socket creation failed");
    }

    //setting use of the TCP. Also we are attaching socket to port PORT 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        server_failure("setsockopt");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    //change byte order according to make it match the big endian TCP/IP network byte order.
    address.sin_port = htons( PORT );

   if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        server_failure("bind failed");
    }

    if (listen(server_fd, SOCKET_QUEUE_LENGTH) < 0)
    {
        server_failure("listen");
    }


    forever
    {
        printf("waiting for a connection\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t*)&addrlen)) < 0)
        {
            server_failure("accept");
        }

        valread = read(new_socket, buffer, SOCKET_BUFFER_SIZE);
        snprintf(buffer, SOCKET_BUFFER_SIZE, "%s\n");
        send(new_socket, HELLO_WORLD, sizeof HELLO_WORLD, 0);
        printf("hello message sent\n");
    }

    //should we clean up when a kill signal is received by this app like CTRL+C?
    return 0;
}
