
#ifndef CONN_H
#define CONN_H
#include <iostream>
#include <string>
#include <FileDeleteTable.hpp>
#include <UserReadTable.hpp>
class conn
{
private:
    std::string baseDir;
    FileDeleteTable *fileDeleteTable;
    UserReadTable *userReadTable;
    std::string user;
public:
    std::string currentDir;
    conn(std::string CurrentDir, std::string baseDir, UserReadTable *urt, FileDeleteTable *fd);
    ~conn();
    std::string getBase();
    std::string getCurrentDir(std::string filepath);
    //Send an error message to the client 
    void send_error(std::string err);
    //This sends a message to the client that is not an error.
    void send_message(std::string msg);
    std::string get_file_location(std::string filepath);
    bool isBeingRead(std::string filename);
    bool isBeingDeleted(std::string filename);
    void addFileAsRead(std::string filename);
    void addFileAsDeleted(std::string filename);
    void removeFileAsRead(std::string filename);
    void removeFileAsDeleted(std::string filename);
};

#endif