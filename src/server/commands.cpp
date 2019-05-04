#include <iostream>
#include <vector>

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
#define MIN_FREE_PORT 4000
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
        try {
            std::string pingRetValue = SystemCommands::ping(host);
            if (pingRetValue.empty()) {
                std::string ret = "ping: " + host + ": Name or service not known";
                conn.send_message(ret);
            }
            else {
                conn.send_to_socket(pingRetValue);
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
            return;
        }
        conn.setUser(username);
        conn.setLoginStatus(AuthenticationMessages::authenticatingStatus);
        conn.send_message("Almost logged in send your password\n");
    }
    void pass(conn& conn, std::string pw) {
        std::string confPath = getConfFilepath();
        bool isBeingAuthenticated = conn.isBeingAuthenticated();
        if (!isBeingAuthenticated) {
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
    }
    void logout(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        if (!isLoggedIn) {
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        conn.clearRead();
        conn.clearLogin();
        conn.currentDir = "";
        conn.send_message(AuthenticationMessages::logutMessage);
    }
    //Directory traversal commands
    void cd(conn& conn, std::string dir) {
        bool isLoggedIn = conn.isLoggedIn();
        if (!isLoggedIn) {
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
                conn.send_message(relativePath);
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
        if (!isLoggedIn) {
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string currDir = conn.getCurrentDir("");
        std::string cmd = CommandConstants::ls;
        std::string lsOutput = SystemCommands::command_with_output(cmd, conn.getCurrentDir(""));
        conn.send_message(lsOutput);
    }
    //Modify directory command
    void mkdir(conn& conn, std::string newDirName) {
        bool isLoggedIn = conn.isLoggedIn();
        if (!isLoggedIn) {
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string base = conn.getBase();
        std::string currentDir = conn.getCurrentDir("");
        std::string resolved;
        try {
            resolved = Parsing::resolve_path(base, currentDir, newDirName);
            bool isBeingDeleted = conn.isBeingDeleted(resolved);
            if (isBeingDeleted) {
                conn.send_error(Parsing::entryDoesNotExist);
                return;
            }
            conn.addFileAsRead(resolved);
            SystemCommands::mkdir(CommandConstants::mkdir, resolved);
        }
        catch(Parsing::BadPathException e) {
            conn.send_error(e.getDesc());
        }
        conn.removeFileAsRead(resolved);
    }
    void rm(conn& conn, std::string filename) {
        bool isLoggedIn = conn.isLoggedIn();
        if (!isLoggedIn) {
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string base = conn.getBase();
        std::string currentDir = conn.getCurrentDir("");
        std::string resolved;
        try {
            resolved = Parsing::resolve_path(base, currentDir, filename);
            bool isBeingRead = conn.isBeingRead(resolved);
            if (isBeingRead) {
                conn.send_error(Parsing::entryInUse);
                return;
            }
            conn.addFileAsDeleted(resolved);
            SystemCommands::rm(CommandConstants::rm, resolved);
        }
        catch(Parsing::BadPathException e) {
            conn.send_error(e.getDesc());
        }
        //The entry should have been deleted and is now removed from the synch data structure
        conn.removeFileAsDeleted(resolved);
    }
    struct get_handler_args {
        conn *c;
        std::string filename; 
    };


    // doing most of the work to process the get command inside the thread
    void *get_handler(void *uncast_params) {
        std::cout << "are we here?\n"<<std::flush;
        get_handler_args* handler_params = (get_handler_args *) uncast_params;
        /*
        there could be many threads executing get_handler at the same time.
        those multiple threads each must at first copy the connection object.
        when they are done copying the connection thread they must signal the main thread
        that they are done. When no thread is copying the main thread is free to go continue 
        doing what is was doing earlier.
        unlock mutex when done copying handler args like the filename and the connection object
        */
        conn *c = handler_params->c; 
        std::string filename = std::string(handler_params->filename);

        copy_get_args_mutex.lock();
        copying_get_args --;
        copy_get_args_mutex.unlock();
        
        bool isLoggedIn = c->isLoggedIn();
        std::cout << "is logged in " << isLoggedIn <<  "\n" << std::flush;
        if (!isLoggedIn) {
            c->send_error(AuthenticationMessages::mustBeLoggedIn);
            return NULL;
        }


        std::string file_location = c->getCurrentDir(filename);
        if (!pathvalidate::isFile(file_location)) {
            c->send_error("this file doesn't exist");
            return NULL;
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
            } catch (const MySocketException e) {
                c->send_error("unable to create a socket for the get command");
                std::cerr << e.what() << std::endl;
                return NULL;
            }
            break;
        } 


        long get_socket = -1; 
        //we accept only one socket connection
        if ((get_socket = accept(server_fd, accept_args.address,
                        accept_args.addrlen_ptr)) < 0)
        { 
            std::cerr << "cannot accept connection socket for get with port" << port << std::endl;
            c->send_error("cannot open a socket for you to receive the file sorry");
            pthread_exit(NULL);
            close(server_fd);
        }
        //need to find a good port from a list of available ports.
        std::string to_send = PORT_NUMBER_GET_KEYWORD;

        c->send_message(to_send.append(" 66666")); 

/*
        free port 
        destroy thread 
        remove port from recorded used ports*/
            
        return NULL;
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

        get_handler_args args = {
            c,
            std::string(filename)
        }; 
        
        std::cout << "Is logged in outside thread" << c->isLoggedIn();

        copy_get_args_mutex.lock();
        copying_get_args++;
        copy_get_args_mutex.unlock();

        if ((ret_create = pthread_create(&get_thread, NULL /*default attributes*/,
                    get_handler, (void *) &args))) {
            copy_get_args_mutex.lock();
            copying_get_args--;
            copy_get_args_mutex.unlock();
        }  

        copy_get_args_mutex.lock();
        std::cout << "waiting for copying get args to be 0, current value = "<<copying_get_args << std::flush;
        while (copying_get_args != 0) {
            copy_get_args_mutex.unlock();
            sleep(0.1); 
            copy_get_args_mutex.lock();
        }
        copy_get_args_mutex.unlock();
        //wait until copying_get_args reaches 0    
        

    }

    
    void put(conn& conn, std::string filename, unsigned int fileSize) {
        conn = conn;    // supress compiler warnings
        filename = filename;    // supress compiler warnings
        fileSize = fileSize;    // supress compiler warnings
        // TODO
    }

    //Misc commands
    void date(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        if (!isLoggedIn) {
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        std::string cmd = CommandConstants::date;
        //Pass in the empty string since date is not used on a directory
        std::string dateOutput = SystemCommands::command_with_output(cmd, "");
        std::cout << "date output " << dateOutput << std::flush;
        conn.send_to_socket(dateOutput);

    }
    void grep(conn& conn, std::string pattern) {
        std::string base = conn.getBase();
        std::string currentDir = conn.getCurrentDir("");
        std::string resolved = Parsing::resolve_path(base, currentDir, "");
        std::vector<std::string> files = FileFetching::fetch_all_files_from_dir(resolved);
        std::vector<std::string> candidateFiles;
        for (std::string file: files) {
            bool match = SystemCommands::grep(file, pattern);
            if (match) {
                candidateFiles.push_back(file);
            }
        }
        std::string ret = Parsing::join_vector(candidateFiles, Parsing::new_line);
        conn.send_message(ret);
    }
    void w(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        if (!isLoggedIn) {
            conn.send_error(AuthenticationMessages::mustBeLoggedIn);
            return;
        }
        conn.send_message(conn.getAllLoggedInUsers());
    }
    void whoami(conn& conn) {
        bool isLoggedIn = conn.isLoggedIn();
        if (!isLoggedIn) {
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
            }
            if (commandName == "cd") {
                cd(conn, splitBySpace[1]);
                return false;
            }
            if (commandName == "ls") {
                ls(conn);
            }
            if (commandName == "mkdir") {
                mkdir(conn, splitBySpace[1]);
                return false;
            }
            if (commandName == "get") {
                get(conn_ptr, splitBySpace[1]);
            }
            if (commandName == "put") {

            }
            if (commandName == "w") {
                w(conn);
            }
            if (commandName == "whoami") {
                whoami(conn);
            }
            if (commandName == "date") {
                date(conn);
            }
            if (commandName == "ping") {
                ping(conn, splitBySpace[1]);
            }
            if (commandName == "login") {
                login(conn, splitBySpace[1]);
            }
            if (commandName == "pass") {
                pass(conn, splitBySpace[1]);
            }
            if (commandName == "exit") {
                return true;
            }
            if (commandName == "logout") {
                logout(conn);
            }
            if (commandName == "grep") {
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
