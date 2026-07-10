
#include "../header/Server.hpp"
#include "../header/Message.hpp"

Server::~Server()
{
	std::string msg_error = "ERROR : Server shutting down\r\n";
	for(size_t i = 0; i < _clients.size(); i++)
	{
		send(_clients[i]->getFd(), msg_error.c_str(), msg_error.size(), 0);
		close(_clients[i]->getFd());
		delete(_clients[i]);
	}
	close(_listenFd);
}

void Server::run()
{
	Server::setupSocket();

	struct pollfd pfd;
	pfd.fd = _listenFd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	_pollfds.push_back(pfd);

    while(!g_shutdown)
    {
		for(size_t y = 0; y < _pollfds.size(); y++)
		{
			if(_pollfds[y].fd == _listenFd)
				continue;
			Client* client = findClient(_pollfds[y].fd);
			if (client == NULL) 
				continue;
			if(client->getOutBuffer().empty())
				_pollfds[y].events = POLLIN;
			else
			_pollfds[y].events = POLLIN | POLLOUT;
		}
		int ret = poll(&_pollfds[0], _pollfds.size(), -1);
		if (ret < 0)
		{
    		if (errno == EINTR) continue;
    			std::cerr << "poll() error: " << strerror(errno) << std::endl;
    		break;
		}
		for (size_t i = 0; i < _pollfds.size(); i++)
		{
			if (_pollfds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
    			if (_pollfds[i].fd != _listenFd)
        			_toDisconnect.push_back(_pollfds[i].fd);
    			continue;
			}
			if (_pollfds[i].revents & POLLIN)
			{
				if(_pollfds[i].fd == _listenFd)
				{
					acceptClient();
				}
				else
				{
					handleClient(_pollfds[i].fd);
				}
			}
			if (_pollfds[i].revents & POLLOUT)
			{
    		flushClient(_pollfds[i].fd);
			}
		}
		for (size_t k = 0; k < _toDisconnect.size(); k++)
    		disconnectClient(_toDisconnect[k]);
		_toDisconnect.clear();
    }
	std::cout << "Server shutting down..." << std::endl;
}

void Server::acceptClient()
{
	sockaddr_in aclient;
	socklen_t clientLen = sizeof(aclient);

	int clientFd = (accept(_listenFd, (struct sockaddr*)&aclient, &clientLen));
	if(clientFd == -1)
	{
		std::cerr << "accept() error:" << strerror(errno) << std::endl;
		return;
	}
	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	struct pollfd acceptcl;
	acceptcl.fd = clientFd;
	acceptcl.events = POLLIN;
	acceptcl.revents = 0;

	_pollfds.push_back(acceptcl);
	_clients.push_back(new Client(clientFd));
	std::cout << "New connexion" << std::endl;
}

void Server::handleClient(int fd)
{
	char recvBuffer[512];
	int bytesReceived = recv(fd, recvBuffer,sizeof(recvBuffer), 0);
	if(bytesReceived < 0)
	{
		if(errno == EAGAIN)
			return;
		std::cerr << "recv() error:" << strerror(errno) << std::endl;
		_toDisconnect.push_back(fd);
		return;
	}
	else if(bytesReceived == 0)
	{
		_toDisconnect.push_back(fd);
		std::cout << "Client disconnected" << std::endl;
	}
	else if(bytesReceived > 0)
	{
		Client* client = findClient(fd);
		if (client == NULL)
			return;
		recvBuffer[bytesReceived] = '\0';
		client->appendToBuffer(recvBuffer);
		while(client->getInBuffer().find("\r\n") != std::string::npos)
		{
			size_t pos = client->getInBuffer().find("\r\n");
			std::string line = client->getInBuffer().substr(0, pos);
			client->eraseBuffer(0, pos + 2);
			Message msg = Message::parse(line);
			dispatcher(client, msg);
		}
	}
}

void Server::dispatcher(Client* client, Message msg)
{
	if(msg.command == "PASS")
	{
		handlePass(*client, msg);
	}
	else if(msg.command == "NICK")
		handleNick(*client, msg);
	else if(msg.command == "USER")
		handleUser(*client, msg);
	
}

void Server::disconnectClient(int fd)
{
	close(fd);
	for(size_t i = 0; i < _pollfds.size(); i++)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}
	for(size_t i = 0; i < _clients.size(); i++)
	{
		if(_clients[i]->getFd() == fd)
		{
			delete _clients[i];
			_clients.erase(_clients.begin() + i);
			break;
		}
	}
}

// https://www.geeksforgeeks.org/cpp/socket-programming-in-cpp/


void Server::setupSocket()
{
	_listenFd = socket(AF_INET, SOCK_STREAM, 0);

	if(_listenFd == -1)
	{
		std::cerr << "socket() error:" << strerror(errno) << std::endl;
		return;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(_port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	int opt = 1;
	setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(_listenFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1 )
	{
		std::cerr << "bind() error:" << strerror(errno) << std::endl;
		return;
	}
	if(listen(_listenFd, 5) == -1)
	{
		std::cerr << "listen() error:" << strerror(errno) << std::endl;
		return;
	}
	fcntl(_listenFd, F_SETFL, O_NONBLOCK);
}

void Server::flushClient(int fd)
{
	Client* client = findClient(fd);
	if (client == NULL) return;
	std::string out = client->getOutBuffer();
	int bytesSent = send(fd, out.c_str(), out.size(), 0);
	if(bytesSent < 0)
	{
		std::cerr << "Error: " << std::endl;
		return;
	}
	if(bytesSent > 0)
	{
		client->eraseOutBuffer(0, bytesSent);
	}
}

Client* Server::findClient(int fd)
{
	for(size_t i = 0; i < _clients.size(); i++)
	{
		if(_clients[i]->getFd() == fd)
		{
			return _clients[i];
		}
	}
	return NULL;
}
