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
        try {
            std::string newPath = Parsing::resolve_path(base, base + conn.currentDir , dir);
            std::string relativePath = Parsing::get_relative_path(base, newPath);
            if (pathvalidate::isDir(newPath)) {
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
            SystemCommands::mkdir(CommandConstants::mkdir, resolved);
        }
        catch(Parsing::BadPathException e) {
            conn.send_error(e.getDesc());
        }
    }
    void rm(conn& conn, std::string filename) {
        std::string base = conn.getBase();
        std::string currentDir = conn.getCurrentDir("");
        std::string resolved;
        try {
            resolved = Parsing::resolve_path(base, currentDir, filename);
            SystemCommands::rm(CommandConstants::rm, resolved);
        }
        catch(Parsing::BadPathException e) {
            conn.send_error(e.getDesc());
        }
    }
    //File specific commands
    void get(conn& conn, std::string filename);
    void put(conn& conn, std::string filename, unsigned int fileSize);
    //Misc commands
    void date(conn& conn);
    void grep(conn& conn, std::string pattern);
    void w(conn& conn);
    void whoami(conn& conn);
} // command
