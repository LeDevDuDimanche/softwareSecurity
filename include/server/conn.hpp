
#ifndef CONN_H
#define CONN_H
#include <iostream>
#include <string>

#include <server/FileDeleteTable.hpp>
#include <server/UserReadTable.hpp>
#include <server/ActiveUserTable.hpp>

std::string getConfFilepath();

class conn
{
private:
    std::string baseDir;
    FileDeleteTable *fileDeleteTable;
    UserReadTable *userReadTable;
    ActiveUserTable *activeUserTable;
    std::string user;
    int loginStatus;
    long sock_fd;
    char *output_buffer;
    ssize_t written_in_buffer;
    void send_to_socket(std::string to_send);

public:
    std::string currentDir;
    conn(std::string CurrentDir, std::string baseDir,
        UserReadTable *urt, FileDeleteTable *fd, ActiveUserTable *at, long sock_fd);
    ~conn();
    std::string getBase();
    std::string getCurrentDir(std::string filepath);
    //Send an error message to the client
    void send_error(std::string err);
    //This sends a message to the client that is not an error.
    void send_message(std::string msg);
    //Sends an empty message. Sort of like an ACK.
    void send_message();
    std::string get_file_location(std::string filepath);
    bool isBeingRead(std::string filename);
    bool isBeingDeleted(std::string filename);
    void addFileAsRead(std::string filename);
    void addFileAsDeleted(std::string filename);
    void removeFileAsRead(std::string filename);
    void removeFileAsDeleted(std::string filename);
    std::string getUser();
    bool isLoggedIn();
    //Status must be a part of {-1, 0, 1}
    void setLoginStatus(int status);
    bool isBeingAuthenticated();
    void setUser(std::string user);
    void clearRead();
    void clearLogin();
    //setLogin updates the shared login table with the username
    void setLogin();
    std::string getAllLoggedInUsers();
};

#endif
