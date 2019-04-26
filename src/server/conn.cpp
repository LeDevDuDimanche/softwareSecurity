#include <iostream>

#include <server/conn.hpp>
#include <parsing.hpp>
#include <server/conf.hpp>
#include <sys/socket.h>

#include <grass.hpp>

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
    #define SOCKET_SEND \
        if ((send_status = send(this->sock_fd, this->output_buffer, this->written_in_buffer, 0 /*flag*/)) != this->written_in_buffer) \
        { \
            std::stringstream ss; \
            ss << "cannot send message to socket\n status:" << send_status << "\nmessage:" << to_send; \
            server_failure(ss.str().c_str()); \
        } \
        this->written_in_buffer = this->written_in_buffer - send_status;


    ssize_t send_status = -1;
    for (char c: to_send) {
        if (this->written_in_buffer == SOCKET_BUFFER_SIZE) {
            SOCKET_SEND
        }
        (this->output_buffer)[this->written_in_buffer] = c;
        this->written_in_buffer += 1;
    }

    if (this->written_in_buffer > 0) {
        SOCKET_SEND
    }
}

std::string conn::getBase() {
    return this->baseDir;
}

void conn::send_error(std::string err) {
    std::string to_send = "Error: ";
    to_send.append(err);
    to_send.append("\n");
    this->send_to_socket(to_send);
}

void conn::send_message(std::string msg) {  
    std::string to_send = "";
    to_send.append(msg);
    to_send.append("\n");
    this->send_to_socket(to_send);
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
    this->user = "";
    this->setLoginStatus(AuthenticationMessages::notLoggedIn);
    //Clear the login table
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
