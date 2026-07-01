#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"

# include <iostream>   // std::cout, std::cerr
# include <string>
# include <vector>
# include <map>
# include <cstring>    // memset, strerror
# include <cstdlib>    // atoi, exit
# include <csignal>    // signal, sigaction, sigemptyset...
# include <cerrno>      // errno
# include <unistd.h>    // close, read, write, lseek
# include <fcntl.h>      // fcntl (O_NONBLOCK)
# include <poll.h>       // poll, struct pollfd
# include <sys/types.h>
# include <sys/socket.h> // socket, bind, listen, accept, setsockopt
# include <netinet/in.h> // sockaddr_in, htons, htonl, ntohs, ntohl
# include <arpa/inet.h>  // inet_addr, inet_ntoa, inet_ntop
# include <netdb.h>      // getaddrinfo, freeaddrinfo, gethostbyname






class server
{
    public:

    private:

};

















#endif