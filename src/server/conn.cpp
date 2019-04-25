#include <iostream>

#include <server/conn.hpp>
#include <parsing.hpp>
#include <server/conf.hpp>
#include <sys/socket.h>

#include <grass.hpp>
#include <sockets.hpp>

//Calculates the location of the conf file
std::string getConfFilepath() {
    //Will update later with something that should be less hard code-y
    return "./grass.conf";
}

conn::conn(std::string currentDir, std::string baseDir, UserReadTable *urt,
 FileDeleteTable *fd, ActiveUserTable *at, long sock_fd)
{
    this->currentDir = currentDir;
    this->baseDir = baseDir;
    this->fileDeleteTable = fd;
    this->userReadTable = urt;
    this->loginStatus = -1;
    this->activeUserTable = at;
    this->sock_fd = sock_fd;
    this->output_buffer = (char *)malloc(SOCKET_BUFFER_SIZE);
    this->written_in_buffer = 0;
}

void conn::send_to_socket(std::string to_send) {
    try {
        sockets::send_all(to_send, this->sock_fd);
    } catch (sockets::SocketError& e) {
        std::cerr << "Couldn't send message to socket\n";
    }
}

std::string conn::getBase() {
    return this->baseDir;
}

void conn::send_error(std::string message) {
    this->send_message("Error: " + message);
}

void conn::send_message(std::string msg) {  
    std::string to_send = msg;
    if (!to_send.empty() && to_send.back() != '\n') {
        to_send += '\n';
    }
    to_send += sockets::end_of_transmission;
    this->send_to_socket(to_send);
}

void conn::send_message() {
    this->send_message("");
}

std::string  conn::getCurrentDir(std::string filepath) {
    if (this->currentDir == "") {
        if (filepath == "") {
            return this->baseDir;
        }
        return this->baseDir + Parsing::join_path + filepath;
    }
    if (filepath.empty() || filepath == "") {
        if (this->currentDir == "") {
            return this->baseDir;
        }
        return this->baseDir + Parsing::join_path + this->currentDir;
    }
    return (this->baseDir) + Parsing::join_path + (this->currentDir) + Parsing::join_path + filepath;
}

conn::~conn()
{
    free(this->output_buffer);
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

bool conn::isBeingAuthenticated() {
    return this->loginStatus == 0;
}

bool conn::isLoggedIn() {
    return this->loginStatus == 1;
}

void conn::setLoginStatus(int status) {
    this->loginStatus = status;
}

void conn::setUser(std::string user) {
        this->user = user;
}

void conn::clearRead() {
    UserReadTable *urt = this->userReadTable;
    urt->removeFile(this->currentDir, this->user);
}

void conn::clearLogin() {
    ActiveUserTable *userTable = this->activeUserTable;
    userTable->removeUser(this->user);
    this->user = "";
    this->setLoginStatus(AuthenticationMessages::notLoggedIn);
}

void conn::setLogin() {
    //updates the shared login table after a successful authentication
    ActiveUserTable *p = this->activeUserTable;
    p->AddUser(this->user);
}

std::string conn::getAllLoggedInUsers() {
    ActiveUserTable *p = this->activeUserTable;
    return p->getAllLoggedInUsers();
}
