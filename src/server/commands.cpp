#include <exception>
#include <iostream>
#include <ios>
#include <vector>

#include <sockets.hpp>
#include <server/commands.hpp>
#include <server/pathvalidate.hpp>
#include <server/systemcmd.hpp>
#include <parsing.hpp>
#include <server/commandParsing.hpp>
#include <server/conf.hpp>
#include <server/fileFetching.hpp>
#include <exception>
#include <grass.hpp>
#include <server/commandParsing.hpp>
#include <server/conf.hpp>
#define MIN_FREE_PORT 5000
#define MAX_FREE_PORT 65535

#include <socketsUtils.hpp>
#include <server/conn.hpp>
#include <mutex>

static std::mutex unavailable_ports_mutex;
static std::set<long> unavailable_ports = {};

static std::mutex copy_get_args_mutex;
static int copying_get_args = 0;


namespace command
{
    //Commands that do not require authentication
    void ping(conn& conn, std::string host) {
        char placeHolder[600];
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
        }
        try {
            host = Parsing::cleanDir(host);
            std::string pingRetValue = SystemCommands::ping(Parsing::format(host));
            if (pingRetValue.empty()) {
                sprintf(placeHolder, "ping: %s: Name or service not known", host.c_str());
                printf("\n");
                std::string ret{placeHolder};
                conn.send_message(ret);
            }
            else {
                conn.send_message(pingRetValue);
            }
        }
        catch(std::runtime_error& e) {
            conn.send_error(e.what());
        }
    }
    //Authentication commands
    void login(conn& conn, std::string username) {
        std::string confPath = getConfFilepath();
        bool userExists = checkIfUserExists(username, confPath);
        if (!userExists) {
            conn.send_error(AuthenticationMessages::userDoesNotExist);
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            return;
        }
        bool isLoggedIn = conn.isLoggedIn();
        if (isLoggedIn) {
            conn.clearRead();
            conn.clearLogin();
            if (conn.currentDir != "") {

            }
        }
        conn.setUser(username);
        conn.setLoginStatus(AuthenticationMessages::authenticatingStatus);
        conn.send_message();
    }
    void pass(conn& conn, std::string pw) {
        std::string confPath = getConfFilepath();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::incorrectCommandSequence);
            return;
        }
        std::string username = conn.getUser();
        bool correctPasswordForUser = checkIfUserPasswordExists(username, pw, confPath);
        if (!correctPasswordForUser) {
            conn.send_error(AuthenticationMessages::incorrectPassword);
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.setUser("");
            return;
        }
        //The authentication was sucessful if we make it this far.
        conn.setLoginStatus(AuthenticationMessages::loggedIn);
        conn.setLogin();
        conn.send_message();
    }
    void logout(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        conn.clearRead();
        conn.clearLogin();
        conn.currentDir = "";
        conn.send_message();
        std::cout << AuthenticationMessages::logutMessage << '\n';
    }
    //Directory traversal commands
    void cd(conn& conn, std::string dir) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string absoluteDir = conn.getCurrentDir(dir);
        //std::cout << "Absolutedir: " << absoluteDir << std::endl;
        std::string base = conn.getBase();
        std::string oldCurrentDir = conn.currentDir;
        try {
            std::string newPath = Parsing::resolve_path(base, conn.getCurrentDir("") , dir);
            std::string relativePath = Parsing::get_relative_path(base, newPath);
            if (pathvalidate::isDir(newPath)) {
                bool isBeingDeleted = conn.isBeingDeleted(newPath);
                if (isBeingDeleted) {
                    conn.send_error(Parsing::entryDoesNotExist);
                    return;
                }
                //Only update the tables if everything was successful, i.e. it is a valid directory
                conn.removeFileAsRead(oldCurrentDir);
                conn.addFileAsRead(relativePath);
                conn.currentDir = relativePath;
                conn.send_message();
            }
            else {
                if (pathvalidate::exists(newPath)) {
                    conn.send_error("This is not a directory");
                }
                else {
                    conn.send_error(relativePath + " does not exist");
                }
            }
        }
        catch(Parsing::BadPathException e) {
            conn.send_error(e.getDesc());
        }
    }
    void ls(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string currDir = conn.getCurrentDir("");
        std::string cmd = CommandConstants::ls;
        std::string escaped = Parsing::format(currDir);
        std::string lsOutput = SystemCommands::command_with_output(cmd, escaped);
        conn.send_message(lsOutput);
    }
    //Modify directory command
    void mkdir(conn& conn, std::string newDirName) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            std::string mustBeLoggedIn = Parsing::bufferToString(AuthenticationMessages::mustBeLoggedIn.c_str());
            conn.send_error(mustBeLoggedIn);
            return;
        }
        std::string base = conn.getBase();
        std::string currentDir = conn.getCurrentDir("");
        std::string resolved;
        try {
            resolved = Parsing::resolve_path(base, currentDir, newDirName);
            if (Parsing::exceedsMaxLength(base, resolved)) {
                std::string entryTooLong = Parsing::bufferToString(Parsing::entryTooLong.c_str());
                conn.send_error(entryTooLong);
                return;
            }
            bool isBeingDeleted = conn.isBeingDeleted(resolved);
            if (isBeingDeleted) {
                std::string entryDoesNotExist = Parsing::bufferToString(Parsing::entryDoesNotExist.c_str());
                conn.send_error(entryDoesNotExist);
                return;
            }
            conn.addFileAsRead(resolved);
            resolved = Parsing::format(Parsing::cleanDir(resolved));
            SystemCommands::mkdir(CommandConstants::mkdir, resolved);
            conn.send_message();
        }
        catch(Parsing::BadPathException e) {
            conn.send_error(e.getDesc());
        }
        conn.removeFileAsRead(resolved);
    }
    void rm(conn& conn, std::string filename) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string base = conn.getBase();
        std::string currentDir = conn.getCurrentDir("");
        std::string resolved;
        try {
            resolved = Parsing::resolve_path(base, currentDir, filename);
            bool isBeingRead = conn.isBeingRead(resolved);
            bool canDelete = resolved.empty() || resolved != currentDir;
            if (!canDelete) {
                conn.send_error(Parsing::entryCantBeDeleted);
                return;
            }
            if (isBeingRead) {
                conn.send_error(Parsing::entryInUse);
                return;
            }
            conn.addFileAsDeleted(resolved);
            resolved = Parsing::format(resolved);
            SystemCommands::rm(CommandConstants::rm, resolved);
            conn.send_message();
        }
        catch(Parsing::BadPathException e) {
            conn.send_error(e.getDesc());
        }
        //The entry should have been deleted and is now removed from the synch data structure
        conn.removeFileAsDeleted(resolved);
    }
    struct get_handler_args {
        conn *c;
        std::string *filename;
    };

    struct put_handler_args {
        conn *c;
        std::string *filename;
        unsigned int filesize;
    };

    void *put_handler(void *uncast_params) {
        put_handler_args* handler_params = (put_handler_args *) uncast_params;
        conn *c = handler_params->c;
        #define GET_HANDLER_EXIT \
            delete handler_params->filename; \
            delete handler_params; \
            return NULL;
        long port = MIN_FREE_PORT; //arbitrary
        for_socket_accept accept_args;
        int server_fd;

        for (; port < MAX_FREE_PORT; port++) {
            unavailable_ports_mutex.lock();
            bool already_used = unavailable_ports.find(port) != unavailable_ports.end();
            unavailable_ports_mutex.unlock();
            if (already_used) {
                continue;
            }

            unavailable_ports_mutex.lock();
            unavailable_ports.insert(port);
            unavailable_ports_mutex.unlock();
            try {
                accept_args = bind_to_port(port, &server_fd);
            } catch (const MySocketException& e) {
                c->send_error("unable to create a socket for the get command");
                std::cerr << e.what() << std::endl;
                GET_HANDLER_EXIT
            }
            break;
        }

        std::string to_send = PORT_NUMBER_PUT_KEYWORD;
        std::stringstream strm;



        strm << PORT_NUMBER_PUT_KEYWORD << " " << port;
        c->send_message(strm.str());
        long put_socket = -1;
        if ((put_socket = accept(server_fd, accept_args.address,
                        accept_args.addrlen_ptr)) < 0)
        {

            std::cerr << "cannot accept connection socket for get with port " << port << " Errno = " << errno << std::flush;
            c->send_error("cannot open a socket for you to receive the file sorry");
            GET_HANDLER_EXIT
        }

        std::ofstream *out_file = new std::ofstream(*(handler_params -> filename));

        sockets::receive_N(put_socket, out_file, handler_params->filesize);
        *out_file << std::flush;
        return NULL;
    }
    // doing most of the work to process the get command inside the thread
    void *get_handler(void *uncast_params) {
        get_handler_args* handler_params = (get_handler_args *) uncast_params;
        /*
        there could be many threads executing get_handler at the same time.
        those multiple threads each must at first copy the connection object.
        when they are done copying the connection thread they must signal the main thread
        that they are done. When no thread is copying the main thread is free to go continue
        doing what is was doing earlier.
        unlock mutex when done copying handler args like the filename and the connection object
        */
        copy_get_args_mutex.lock();
        copying_get_args --;
        copy_get_args_mutex.unlock();
        conn *c = handler_params->c;

        #define GET_HANDLER_EXIT \
            delete handler_params->filename; delete handler_params; return NULL;

        bool isLoggedIn = c->isLoggedIn();
        if (!isLoggedIn) {
            c->send_error(AuthenticationMessages::mustBeLoggedIn);
            GET_HANDLER_EXIT
        }


        std::string file_location = c->getCurrentDir(*(handler_params->filename));
        if (!pathvalidate::isFile(file_location)) {
            c->send_error("this file doesn't exist");
            GET_HANDLER_EXIT
        }


        long port = MIN_FREE_PORT; //arbitrary
        for_socket_accept accept_args;
        int server_fd;

        for (; port < MAX_FREE_PORT; port++) {
            unavailable_ports_mutex.lock();
            bool already_used = unavailable_ports.find(port) != unavailable_ports.end();
            unavailable_ports_mutex.unlock();
            if (already_used) {
                continue;
            }
            try {
                accept_args = bind_to_port(port, &server_fd);
            } catch (const MySocketException& e) {
                c->send_error("unable to create a socket for the get command");
                std::cerr << e.what() << std::endl;
                GET_HANDLER_EXIT
            }
            break;
        }



        //need to find a good port from a list of available ports.
        std::string to_send = PORT_NUMBER_GET_KEYWORD;
        std::stringstream strm;

        std::ifstream infile;
        infile.open(file_location);
        std::string line;

        std::vector<unsigned char> infile_buffer(GET_BUFFER_SIZE, 0);
        infile.seekg (0, infile.end);
        int file_length = infile.tellg();
        infile.seekg (0, infile.beg);


        strm << PORT_NUMBER_GET_KEYWORD << " " << port << " " << GET_SIZE_KEYWORD << file_length;
        c->send_message(strm.str());

        long get_socket = -1;
        //we accept only one socket connection
        if ((get_socket = accept(server_fd, accept_args.address,
                        accept_args.addrlen_ptr)) < 0)
        {

            std::cerr << "cannot accept connection socket for get with port " << port << " Errno = " << errno << std::flush;
            c->send_error("cannot open a socket for you to receive the file sorry");
            GET_HANDLER_EXIT
        }



        // https://stackoverflow.com/questions/25625115/cpp-byte-file-reading


        int total_left_to_read = file_length;
        do {

            int to_read = std::min(total_left_to_read , GET_BUFFER_SIZE);
            infile.read((char*)&infile_buffer[0], to_read);
            total_left_to_read -= to_read;

            std::ios_base::iostate state = infile.rdstate();
            if (state == std::ios_base::badbit) {
                c->send_error("Error while reading file");
                break;
            }
            if (infile.gcount() == 0 || state == std::ios_base::eofbit) {
                break;
            }



            send(get_socket, &infile_buffer[0], infile.gcount(), 0);

        } while (true);
        char EOT[] = {sockets::end_of_transmission};
        send(get_socket, EOT, 1, 0);



        unavailable_ports_mutex.lock();
        unavailable_ports.erase(port);
        unavailable_ports_mutex.unlock();

        close(server_fd);
        GET_HANDLER_EXIT

    }

    //File specific commands
    void get(conn *c, std::string filename) {
        //first check if the client is logged in
        //if he is not logged then throw an error
        // then check if a file with the name filename exists
        // if it is a directory or doesnt exist throw an error
        // if it exists send a message to the client with the port number he has to connect to to receive the file
        //
        long ret_create;
        pthread_t get_thread;

        std::string *copied_filename = new std::string();
        copied_filename->append(filename);
        get_handler_args* args = new get_handler_args {
            c,
            copied_filename
        };

        copy_get_args_mutex.lock();
        copying_get_args++;
        copy_get_args_mutex.unlock();


        if ((ret_create = pthread_create(&get_thread, NULL /*default attributes*/,
                    get_handler, (void *) args))) {
            copy_get_args_mutex.lock();
            copying_get_args--;
            copy_get_args_mutex.unlock();
        }

        copy_get_args_mutex.lock();
        while (copying_get_args > 0) {
            copy_get_args_mutex.unlock();
            sleep(0.1);
            copy_get_args_mutex.lock();
        }
        copy_get_args_mutex.unlock();
        //wait until copying_get_args reaches 0


    }


    void put(conn *c, std::string filename, unsigned int fileSize) {
        long ret_create;
        pthread_t put_thread;
        std::string filePath = c->getCurrentDir(filename);
        bool alreadyExists = pathvalidate::exists(filePath);
        if (alreadyExists) {
            c->send_error(Parsing::entryExists);
            return;
        }
        bool isBeingDeleted = c->isBeingDeleted(filePath);
        if (isBeingDeleted) {
            c->send_error(Parsing::entryDoesNotExist);
            return;
        }
        std::string *copied_filename = new std::string();
        copied_filename->append(filePath);
        put_handler_args* args = new put_handler_args {
            c,
            copied_filename,
            fileSize,
        };
        if ((ret_create = pthread_create(&put_thread, NULL /*default attributes*/,
                    put_handler, (void *) args))) {
            copy_get_args_mutex.lock();
            copying_get_args--;
            copy_get_args_mutex.unlock();
        }
    }

    //Misc commands
    void date(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string cmd = CommandConstants::date;
        //Pass in the empty string since date is not used on a directory
        std::string dateOutput = SystemCommands::command_with_output(cmd, "");
        conn.send_message(dateOutput);
    }
    void grep(conn& conn, std::string pattern) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string base = conn.getBase();
        std::string currentDir = conn.getCurrentDir("");
        std::string resolved = Parsing::resolve_path(base, currentDir, "");
        std::vector<std::string> files = FileFetching::fetch_all_files_from_dir(resolved);
        std::vector<std::string> candidateFiles;
        std::string formattedPattern = Parsing::format(Parsing::cleanDir(pattern));
        for (std::string file: files) {
            std::string formatFile = Parsing::format(Parsing::cleanDir(file));
            bool match = SystemCommands::grep(formatFile, formattedPattern);
            if (match) {
                candidateFiles.push_back(file);
            }
        }
        std::string ret = Parsing::join_vector(candidateFiles, Parsing::new_line);
        conn.send_message(ret);
    }
    void w(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        conn.send_message(conn.getAllLoggedInUsers());
    }
    void whoami(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isLoggedIn || isBeingAuthenticated) {
            conn.setUser("");
            conn.setLoginStatus(AuthenticationMessages::notLoggedIn);
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        //This is really stupid but I did it for consistency
        conn.send_message(conn.getUser());
    }

    // return true on exit
    bool run_command(conn *conn_ptr, std::string commandLine) {
        conn &conn  = *conn_ptr;
        try {
            std::vector<std::string> splitBySpace = Parsing::split_string(commandLine, Parsing::space);
            if (splitBySpace.empty()) {
                conn.send_error("Could not parse the command");
                return false;
            }
            std::string commandName = splitBySpace[0];
            bool hasRightArguments = Parsing::hasRightNumberOfArguments(splitBySpace);
            if (!hasRightArguments) {
                conn.send_error("Not the right argument total");
                return false;
            }
            if (commandName == "rm") {
                rm(conn, splitBySpace[1]);
            } else if (commandName == "cd") {
                cd(conn, splitBySpace[1]);
            } else if (commandName == "ls") {
                ls(conn);
            } else if (commandName == "mkdir") {
                mkdir(conn, splitBySpace[1]);
                return false;
            }
            if (commandName == "get") {
                get(conn_ptr, splitBySpace[1]);
            }
            if (commandName == "put") {
                long len;
                char* end;
                errno = 0;
                len = std::strtol(splitBySpace[2].c_str(), &end, 10);
                if (errno != 0 || *end != '\0' || len <= 0 || len >= 123456) {
                    std::cerr << "Invalid len " << '"' << len << '"' << *end << '"' << errno << '"' << "\n";
                    std::cerr << "Invalid len " << '"' << splitBySpace[2] << '"' << "\n";
                    conn.send_error("Invalid len");
                } else {
                    put(conn_ptr, splitBySpace[1], len);
                }
            }
            if (commandName == "w") {
                w(conn);
            } else if (commandName == "whoami") {
                whoami(conn);
            } else if (commandName == "date") {
                date(conn);
            } else if (commandName == "ping") {
                ping(conn, splitBySpace[1]);
            } else if (commandName == "login") {
                login(conn, splitBySpace[1]);
            } else if (commandName == "pass") {
                pass(conn, splitBySpace[1]);
            } else if (commandName == "exit") {
                conn.send_message();
                return true;
            } else if (commandName == "logout") {
                logout(conn);
            } else if (commandName == "grep") {
                grep(conn, splitBySpace[1]);
            }
        }
        catch(Parsing::CommandArgumentsException e) {
            conn.send_error(e.getDesc());
        }
        catch(Parsing::CommandNotFoundException e) {
            conn.send_error(e.getDesc());
        }

        return false;
    }
} // command
