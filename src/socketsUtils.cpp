#include <socketsUtils.hpp>
#include <unistd.h>
 
for_socket_accept bind_to_port(long port, int *server_fd_ptr) { 

    //an int where we store options of the socket
    int opt = 1;

    //specifies a transport address and port for the Ipv4
    struct sockaddr_in *address = new sockaddr_in();

    int *addrlen = new int(sizeof(address));

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons( port ); 
    //create a socket file descriptor for connections using the IPv4 for two way connections sending ybyte streams to each others (TCP).
    if ((*server_fd_ptr = socket(AF_INET, SOCK_STREAM, IP_PROT)) == 0)
    {
        throw MySocketException("socket creation failed");
    }

        //setting use of the TCP. Also we are attaching socket to port PORT
    if (setsockopt(*server_fd_ptr, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        throw MySocketException("setsockopt");
        close(*server_fd_ptr);
    }


    if (bind(*server_fd_ptr, (struct sockaddr *)address,
                                 sizeof(*address))<0)
    {
        throw MySocketException("bind failed");
        close(*server_fd_ptr);
    }

    if (listen(*server_fd_ptr, SOCKET_QUEUE_LENGTH) < 0)
    {
        throw MySocketException("listen");
        close(*server_fd_ptr);
    }

    return {
        (struct sockaddr *) address,
        (socklen_t *) addrlen
    };

}