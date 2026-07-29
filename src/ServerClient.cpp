#include "../header/Server.hpp"
#include "../header/Message.hpp"


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