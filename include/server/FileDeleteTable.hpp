#ifndef FILEDELETETABLE_HPP
#define FILEDELETETABLE_HPP

#include <mutex>
#include <string>
#include <map>

/*
This class is to keep state of which files are being deleted at any given time.
A file in this context can be anything. This is to make sure that if it takes a while to delete something then
a log of the things that are being deleted is kept such that a user can't access a directory or file and then it is deleted
mid way through.
*/
class FileDeleteTable
{
private:
    std::mutex mtx;
    std::map<std::string, bool> filesBeingDeleted;
public:
    bool isBeingDeleted(std::string filename);
    void setAsDeleted(std::string filename);
    void removeAsDeleted(std::string filename);
    FileDeleteTable(/* args */);
    ~FileDeleteTable();
};
#endif
