#include <grass.hpp>

#include <mutex>
#include <vector>

#include <ctype.h>
#include <pthread.h>
#include <netinet/in.h>
#include <parsing.hpp>
#include <server/conf.hpp>
#include <vector>
#include <mutex>
#include <server/commands.hpp>
#include <server/commandParsing.hpp>
#include <unistd.h>
#include <socketsUtils.hpp>

#include <parsing.hpp>
#include <server/commands.hpp>
#include <server/commandParsing.hpp>
#include <server/conf.hpp>
#include <server/FileDeleteTable.hpp>
#include <server/ActiveUserTable.hpp>
#include <server/UserReadTable.hpp>

#define forever for(;;)

static std::string basedir;

static FileDeleteTable fileDeleteTable;
static UserReadTable userReadTable;
static ActiveUserTable activeUserTable;

static std::mutex client_handlers_mutex;
static std::vector<pthread_t> client_handlers = {};


// Server side REPL given a socket file descriptor
void *connection_handler(void* socket_id_void) {
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    int valread;
    long socket_id = (long)socket_id_void; //conversion from int to long because of -fnopermissive compilation flag
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

    conn* thread_conn = new conn("", basedir, &userReadTable, &fileDeleteTable, &activeUserTable, socket_id);

    bool exit = false;
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
            exit = command::run_command(thread_conn, command_line);
            std::cout << "Processed command: " << command_line << std::endl;
            if (exit) {
                break;
            }
        }

        if (exit) {
            delete thread_conn;
            break;
        }
    }

    std::cout << "Exiting thread\n";
    return NULL;
}

int makeBaseDir(std::string baseDir) {
    return system(("mkdir -p " + baseDir).c_str());
}

// Parse the rass.conf file
// Listen to the port and handle each connection
int main() {
    int server_fd;

    std::string conf_path = getConfFilepath();

    long server_port = getConfPort(conf_path);
    basedir = getConfBaseDir(conf_path);

    if (makeBaseDir(basedir) != 0) {
        server_failure("listen");
    }

    basedir = getConfBaseDir(conf_path);


    int ret_create = -1;
    pthread_t new_thread;
    long new_socket;

    for_socket_accept accept_args;
    try {
        accept_args = bind_to_port(server_port, &server_fd);
    } catch (const MySocketException& e) {
        server_failure(e.what());
    }

    forever
    {
        printf("waiting for a connection on port %ld\n", server_port);

        //a blocking call
        if ((new_socket = accept(server_fd, accept_args.address,
                           accept_args.addrlen_ptr)) < 0)
        {
            close(server_fd);
            server_failure("accept");
        }
        printf("handling new connection\n");

        if ((ret_create = pthread_create(&new_thread, NULL /*default attributes*/,
             connection_handler, (void *) new_socket))) {

            #define FIRST_PART_ERROR_MSG "unable to create thread, thread creation error number: "
            size_t err_msg_max_len = (sizeof FIRST_PART_ERROR_MSG) + 30;
            char *err_msg_buffer = (char *)malloc(err_msg_max_len);
            if (err_msg_buffer == NULL)
            {
                close(server_fd);
                server_failure("cannot allocate memory and cannot create a thread");
            }

            snprintf(err_msg_buffer, err_msg_max_len, FIRST_PART_ERROR_MSG " %d", ret_create);
            close(server_fd);
            server_failure(err_msg_buffer);
        }

        client_handlers_mutex.lock();
        client_handlers.push_back(new_thread);
        client_handlers_mutex.unlock();
    }

    return 0;
}
