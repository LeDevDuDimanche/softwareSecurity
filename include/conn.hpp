#include <iostream>

#ifndef CONN_H
#define CONN_H
class conn
{
private:
    std::string baseDir;
public:
    std::string currentDir;
    conn(std::string CurrentDir, std::string baseDir);
    ~conn();
    std::string getBase();
    std::string getCurrentDir(std::string filepath);
    //Send an error message to the client 
    void send_error(std::string err);
    //This sends a message to the client that is not an error.
    void send_message(std::string msg);
    std::string get_file_location(std::string filepath);
};

#endif