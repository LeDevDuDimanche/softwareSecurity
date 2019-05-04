#include <exception>
#include <iostream>
#include <vector>

#include <server/commands.hpp>
#include <server/pathvalidate.hpp>
#include <server/systemcmd.hpp>
#include <parsing.hpp>
#include <server/commandParsing.hpp>
#include <server/conf.hpp>
#include <server/fileFetching.hpp>

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
            resolved = Parsing::format(resolved);
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

    //File specific commands
    void get(conn& conn, std::string filename) {
        conn = conn;    // supress compiler warnings
        filename = filename;    // supress compiler warnings
        conn.send_message();
        // TODO
    }
    void put(conn& conn, std::string filename, unsigned int fileSize) {
        conn = conn;    // supress compiler warnings
        filename = filename;    // supress compiler warnings
        fileSize = fileSize;    // supress compiler warnings
        conn.send_message();
        // TODO
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
        std::string formattedPattern = Parsing::format(pattern);
        for (std::string file: files) {
            std::string formatFile = Parsing::format(file);
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
    bool run_command(conn& conn, std::string commandLine) {
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
            } else if (commandName == "get") {
                get(conn, splitBySpace[1]);
            } else if (commandName == "put") {
                get(conn, splitBySpace[1]);
            } else if (commandName == "w") {
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
