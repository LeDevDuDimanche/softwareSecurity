
#include <netinet/in.h>
#include <exception>
#include <stdlib.h>
#include <string>

#define IP_PROT 0
#define SOCKET_QUEUE_LENGTH 10
/*
* contains informations that are parameters for the accept method of the socket library
*/
struct for_socket_accept {
    struct sockaddr *address;
    socklen_t *addrlen_ptr;
};
 

class MySocketException: public std::exception
{
  private: 
    std::string msg;
  public:
    MySocketException(std::string msg_param) {
        this->msg = "";
        (this->msg).append("Socket error: ");
        (this -> msg).append(msg_param);
    } 
    virtual const char* what() const throw() {
        return this->msg.c_str();
    } 
};

for_socket_accept bind_to_port(long port, int *server_fd_ptr);