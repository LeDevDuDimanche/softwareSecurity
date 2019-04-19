
#ifndef COMMANDS_H
#define COMMANDS_H

#include <iostream>
#include <conn.hpp>
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
    void cd(conn& conn, std::string dir);
    void ls(conn& conn);
    //Modify directory command
    void mkdir(conn& conn, std::string newDirName);
    void rm(conn& conn, std::string filename);
    //File specific commands
    void get(conn& conn, std::string filename);
    void put(conn& conn, std::string filename, unsigned int fileSize);
    //Misc commands
    void date(conn& conn);
    void grep(conn& conn, std::string pattern);
    void w(conn& conn);
    void whoami(conn& conn);
}
#endif