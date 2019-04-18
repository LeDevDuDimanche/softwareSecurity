#include <iostream>
#include <conn.hpp>
#include <parsing.hpp>

conn::conn(std::string currentDir, std::string baseDir, UserReadTable *urt, FileDeleteTable *fd)
{
    this->currentDir = currentDir;
    this->baseDir = baseDir;
    this->fileDeleteTable = fd;
    this->userReadTable = urt;
}

std::string conn::getBase() {
    return this->baseDir;
};

void conn::send_error(std::string err) {
    std::vector<std::string> v = Parsing::split_string(err, '/');
    
    std::cout <<"ERROR: " << err << std::endl;
};

void conn::send_message(std::string msg) {
    std::cout << msg << std::endl;
}
std::string  conn::getCurrentDir(std::string filepath) {
    return (this->baseDir) + (this->currentDir) + filepath;
}

conn::~conn()
{
}

bool conn::isBeingRead(std::string filename) {
    UserReadTable *urt = this->userReadTable;
    return urt->isBeingRead(filename);
}
bool conn::isBeingDeleted(std::string filename) {
    FileDeleteTable *fd = this->fileDeleteTable;
    return fd->isBeingDeleted(filename);
}
void conn::addFileAsRead(std::string filename) {
    UserReadTable *urt = this->userReadTable;
    urt->addFile(filename, this->user);
}
void conn::addFileAsDeleted(std::string filename){
    FileDeleteTable *fd = this->fileDeleteTable;
    fd->setAsDeleted(filename);
}
void conn::removeFileAsRead(std::string filename) {
    UserReadTable *urt = this->userReadTable;
    urt->removeFile(filename, this->user);
}
void conn::removeFileAsDeleted(std::string filename) {
    FileDeleteTable *fd = this->fileDeleteTable;
    fd->removeAsDeleted(filename);
}

std::string conn::getUser() {
    return this->user;
}