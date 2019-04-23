#include <iostream>

#include <server/commands.hpp>
#include <server/pathvalidate.hpp>
#include <server/systemcmd.hpp>
#include <server/parsing.hpp>
#include <server/commandParsing.hpp>
#include <server/conf.hpp>

namespace command
{
    //Commands that do not require authentication
    void ping(conn& conn) {
        //TODO
    }
    void exit(conn& conn) {
        //TODO
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
        //TODO
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
        std::string lsOutput = SystemCommands::ls(cmd, conn.getCurrentDir(""));
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
        std::cout <<"removing file as read " << std::endl;
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
    //File specific commands
    void get(conn& conn, std::string filename) {

    }
    void put(conn& conn, std::string filename, unsigned int fileSize) {

    }
    //Misc commands
    void date(conn& conn) {

    }
    void grep(conn& conn, std::string pattern) {
        //TODO
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

    void run_command(conn& conn, std::string commandLine) {
        try {
            std::vector<std::string> splitBySpace = Parsing::split_string(commandLine, Parsing::space);
            if (splitBySpace.empty()) {
                conn.send_error("Could not parse the command");
                return;
            }
            std::string commandName = splitBySpace[0];
            bool hasRightArguments = Parsing::hasRightNumberOfArguments(splitBySpace);
            if (!hasRightArguments) {
                conn.send_error("Not the right argument total");
                return;
            }
            if (commandName == "rm") {
                rm(conn, splitBySpace[1]);
            }
            if (commandName == "cd") {
                cd(conn, splitBySpace[1]);
                return;
            }
            if (commandName == "ls") {
                ls(conn);
            }
            if (commandName == "mkdir") {
                mkdir(conn, splitBySpace[1]);
                return;
            }
            if (commandName == "get") {
                get(conn, splitBySpace[1]);
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
                ping(conn);
            }
            if (commandName == "ping") {
                ping(conn);
            } 
            if (commandName == "login") {
                login(conn, splitBySpace[1]);
            }
            if (commandName == "pass") {
                pass(conn, splitBySpace[1]);
            }
            if (commandName == "exit") {
                exit(conn);
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
    }
} // command
