#ifndef USERREADTABLE_HPP
#define USERREADTABLE_HPP
#include <map>
#include <set>
#include <iostream>
#include <mutex>

bool hasKey(std::map<std::string,std::set<std::string>> m, std::string key);

//Here we put our data structures that are used for concurrent access control
class UserReadTable
{
private:
    std::map<std::string, std::set<std::string>> FilesBeingRead;
    std::mutex mtx;
public:
    void addFile(std::string filename, std::string user);
    void removeFile(std::string filename, std::string user);
    bool isBeingRead(std::string filename);
    UserReadTable();
    ~UserReadTable();
};    

#endif