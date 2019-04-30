#include <grass.hpp>
#include <ctype.h>
#include <pthread.h>
#include <vector>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>

#include <parsing.hpp>
#include <server/commands.hpp>
#include <server/commandParsing.hpp>
#include <server/conf.hpp>
#include <server/FileDeleteTable.hpp>
#include <server/ActiveUserTable.hpp>
#include <server/UserReadTable.hpp>

#define SOCKET_QUEUE_LENGTH 3
#define forever for(;;)


static std::string basedir;

static FileDeleteTable fileDeleteTable;
static UserReadTable userReadTable;
static ActiveUserTable activeUserTable;

static std::mutex client_handlers_mutex;
static std::vector<pthread_t> client_handlers = {};


/*
 * Send a file to the client as its own thread
 *
 * fp: file descriptor of file to send
 * sock: socket that has already been created.
 */
void send_file(int fp, int sock) {
    fp += sock;    // supress compiler warnings
    // TODO
}

/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to save to.
 * sock: socket that has already been created.
 * size: the size (in bytes) of the file to recv
 */
void recv_file(int fp, int sock, int size) {
    fp += sock * size;    // supress compiler warnings
    // TODO
}

// Server side REPL given a socket file descriptor
void *connection_handler(void* sockfd) {
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    int valread;
    long socket_id = (long)sockfd; //conversion from int to long because of -fnopermissive compilation flag
    pthread_t this_thread = pthread_self();
    printf("new thread id %ld, new socket_id %ld\n", this_thread, socket_id);

    bool found = false;
    client_handlers_mutex.lock();
    for (auto it = client_handlers.begin(); it != client_handlers.end(); )
    {
        if (*it == this_thread) {
            found = true;
            client_handlers.erase(it);
            break;
        } else {
            ++it;
        }
    }
    client_handlers_mutex.unlock();
    printf("found thread before exiting: %d\n", found);

    std::string to_process = "";
    std::vector<std::string> processed_lines;
    std::string read_str;
    int buffer_idx, end_last_copy;

    conn thread_conn = conn("", basedir, &userReadTable, &fileDeleteTable, &activeUserTable, socket_id);

    //TODO change the condition that we end this loop to the fact that we process a CTRL+C character
    while ((valread = read(socket_id, buffer, SOCKET_BUFFER_SIZE)) > 0 && valread < SOCKET_BUFFER_SIZE)
    {
        read_str = std::string(buffer, valread);
        buffer_idx = 0;
        end_last_copy = -1;
        processed_lines = {};

        #define push_inside_to_process \
            if (buffer_idx > end_last_copy + 1) { \
                to_process.append(read_str.substr(end_last_copy + 1, buffer_idx)); \
            }

        for (char cur_chr: read_str) {
            if (cur_chr == '\n') {
                push_inside_to_process
                end_last_copy = buffer_idx;
                if (to_process.size() > 0) {
                    processed_lines.push_back(to_process);
                }

                to_process = "";
            }
            buffer_idx++;
        }

        push_inside_to_process

        for (std::string command_line: processed_lines) {
            command::run_command(thread_conn, command_line);
            std::cout << "Processed command : " << command_line;
        }

    }

    pthread_exit(NULL/*a return value available to the thread doing join on this thread*/);

}

/*
 * search all files in the current directory
 * and its subdirectory for the pattern
 *
 * pattern: an extended regular expressions.
 * Output: A line seperated list of matching files' addresses
 */
void search(char *pattern) {
    pattern = pattern;    // supress compiler warnings
    // TODO
}


// TODO:
// Parse the rass.conf file
// Listen to the port and handle each connection
int main() {
    int server_fd;
    //specifies a transport address and port for the Ipv4
    struct sockaddr_in address;

    int addrlen = sizeof(address);

    //an int where we store options of the socket
    int opt = 1;

    //create a socket file descriptor for connections using the IPv4 for two way connections sending ybyte streams to each others (TCP).
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
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

    std::string conf_path = getConfFilepath();
    //change byte order according to make it match the big endian TCP/IP network byte order.


    long server_port = getConfPort(conf_path);
    basedir = getConfBaseDir(conf_path);
    address.sin_port = htons( server_port );

    if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0)
    {
        server_failure("bind failed");
    }

    if (listen(server_fd, SOCKET_QUEUE_LENGTH) < 0)
    {
        server_failure("listen");
    }

    int ret_create = -1;
    pthread_t new_thread;


    long new_socket;
    forever
    {
        printf("waiting for a connection on port %ld\n", server_port);

        //a blocking call
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t*)&addrlen)) < 0)
        {
            server_failure("accept");
        }
        printf("handling new connection\n");

        if ((ret_create = pthread_create(&new_thread, NULL /*default attributes*/,
             connection_handler, (void *) new_socket)))
        {

            #define FIRST_PART_ERROR_MSG "unable to create thread, thread creation error number: "
            size_t err_msg_max_len = (sizeof FIRST_PART_ERROR_MSG) + 30;
            char *err_msg_buffer = (char *)malloc(err_msg_max_len);
            if (err_msg_buffer == NULL)
            {
                server_failure("cannot allocate memory and cannot create a thread");
            }

            snprintf(err_msg_buffer, err_msg_max_len, FIRST_PART_ERROR_MSG " %d", ret_create);
            server_failure(err_msg_buffer);
        }

        client_handlers_mutex.lock();
        client_handlers.push_back(new_thread);
        client_handlers_mutex.unlock();
    }

    //should we clean up when a kill signal is received by this app like CTRL+C?
    return 0;
}
