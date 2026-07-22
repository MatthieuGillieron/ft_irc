#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channel.hpp"

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

#include "Message.hpp"

extern sig_atomic_t g_shutdown;

class Server
{
    public:
    Server(unsigned int port, std::string password) : _port(port), _password(password) {};
    ~Server();

	std::string getPassword() const;
	std::string getPort() const;
	std::string getListenFd() const;

    void run(); // poll

	void setupSocket(); // socket ecoute
	void acceptClient();

	void handleClient(int fd);
	void handlePass(Client& client, const Message& msg);
	void handleNick(Client &client, const Message &msg);
	void handleUser(Client& client, const Message& msg);
	void handlePing(Client& client, const Message& msg);
	void handleJoin(Client& client, const Message& msg);
	bool checkJoin(Client& client, Channel* chan, const std::string& key);
	void joinReplies(Client& client, Channel* chan);

	void disconnectClient(int fd);

	void reply(Client &client, const std::string &msg);
	void checkRegister(Client &client);


	void dispatcher(Client* client, Message msg);

	void flushClient(int fd);
	Client* findClient(int fd);
	Channel* findChannel(const std::string& name);

    private:
		unsigned int _port;
		std::string _password;
		int _listenFd;
		std::vector<struct pollfd> _pollfds;
		std::vector<Client*> _clients;
		std::map<std::string, Channel*> _channels;
		std::vector<int> _toDisconnect;

};



















#endif