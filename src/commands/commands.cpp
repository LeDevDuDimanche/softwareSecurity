#include <commands.hpp>
#include <pathvalidate.hpp>
#include <systemcmd.hpp>
#include <parsing.hpp>
#include <iostream>

namespace command
{
    //Commands that do not require authentication
    void ping(conn& conn);
    void exit(conn& conn);
    //Authentication commands
    void login(conn& conn, std::string username);
    void pass(conn& conn, std::string pw);
    void logout(conn& conn);
    //Directory traversal commands
    void cd(conn& conn, std::string dir) {
        std::string absoluteDir = conn.getCurrentDir(dir);
        std::string base = conn.getBase();
        std::string oldCurrentDir = conn.currentDir;
        try {
            std::string newPath = Parsing::resolve_path(base, base + conn.currentDir , dir);
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
        std::string currDir = conn.getCurrentDir("");
        std::string cmd = CommandConstants::ls;
        std::string lsOutput = SystemCommands::ls(cmd, conn.getCurrentDir(""));
        conn.send_message(lsOutput);
    }
    //Modify directory command
    void mkdir(conn& conn, std::string newDirName) {
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
    void put(conn& conn, std::string filename, unsigned int fileSize);
    //Misc commands
    void date(conn& conn);
    void grep(conn& conn, std::string pattern);
    void w(conn& conn);
    void whoami(conn& conn);
} // command
