#include <iostream>
#include <conn.hpp>
#include <parsing.hpp>

conn::conn(std::string currentDir, std::string baseDir)
{
    this->currentDir = currentDir;
    this->baseDir = baseDir;
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
