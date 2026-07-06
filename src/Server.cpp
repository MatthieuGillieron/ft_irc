
#include "../header/Server.hpp"

void Server::run()
{
	Server::setupSocket();

	struct pollfd pfd;
	pfd.fd = _listenFd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	_pollfds.push_back(pfd);

    while(true)
    {
		poll(&_pollfds[0], _pollfds.size(), -1);
		for (size_t i = 0; i < _pollfds.size(); i++)
		{
			if (_pollfds[i].revents == POLLIN)
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
		}
    }
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
		std::cerr << "recv() error:" << strerror(errno) << std::endl;
		return;
	}
	else if(bytesReceived == 0)
	{
		disconnectClient(fd);
		std::cout << "Client disconnected" << std::endl;
	}
	else if(bytesReceived > 0)
	{
		for(size_t i = 0; i < _clients.size(); i++)
		{
			if(_clients[i]->getFd() == fd)
			{
				recvBuffer[bytesReceived] = '\0';
				_clients[i]->appendToBuffer(recvBuffer);
				while(_clients[i]->getInBuffer().find("\r\n") != std::string::npos)
				{
					size_t pos = _clients[i]->getInBuffer().find("\r\n");
					std::string line = _clients[i]->getInBuffer().substr(0, pos);
					_clients[i]->eraseBuffer(0, pos + 2);
					std::cout << "Line: " << line << std::endl;
				}
			}
		}
	}
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
