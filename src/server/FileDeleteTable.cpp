#include <mutex>
#include <string>
#include <map>
#include <server/FileDeleteTable.hpp>

FileDeleteTable::FileDeleteTable(/* args */)
{
    //Do nothing
}

FileDeleteTable::~FileDeleteTable()
{
    //Should we empty the table? Tricky
}

bool FileDeleteTable::isBeingDeleted(std::string filename) {
    //Check if a file is in the map of files being deleted
    this->mtx.lock();
    bool found = this->filesBeingDeleted.find(filename) != this->filesBeingDeleted.end();
    this->mtx.unlock();
    return found;
}

//Add a file as a entry that is being deleted
void FileDeleteTable::setAsDeleted(std::string filename) {
    this->mtx.lock();
    this->filesBeingDeleted[filename] = true;
    this->mtx.unlock();
}

//Remove a file from the datastructure
void FileDeleteTable::removeAsDeleted(std::string filename) {
    this->mtx.lock();
    this->filesBeingDeleted.erase(filename);
    this->mtx.unlock();
}
