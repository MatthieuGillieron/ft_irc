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
		markDisconnect(fd);
		return;
	}
	else if(bytesReceived == 0)
	{
		markDisconnect(fd);
		std::cout << "Client disconnected" << std::endl;
	}
	else if(bytesReceived > 0)
	{
		Client* client = findClient(fd);
		if (client == NULL)
			return;
		// on construit la chaine a partir de la longueur lue, jamais d'un '\0'
		// ajoute a la main : recvBuffer[512] serait hors du tampon
		client->appendToBuffer(std::string(recvBuffer, bytesReceived));
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

// un meme fd peut etre signale deux fois dans le meme tour (POLLHUP + recv a 0) :
// sans ce garde-fou, le second close() porterait sur un descripteur deja reattribue
void Server::markDisconnect(int fd)
{
	for (size_t i = 0; i < _toDisconnect.size(); i++)
	{
		if (_toDisconnect[i] == fd)
			return;
	}
	_toDisconnect.push_back(fd);
}


void Server::disconnectClient(int fd)
{
	Client* client = findClient(fd);

	// il faut sortir le client de ses salons AVANT de le detruire :
	// sinon Channel::_members garde un pointeur vers de la memoire liberee,
	// et le prochain broadcast ecrit dedans
	if (client != NULL)
	{
		// une deconnexion brutale doit prevenir les salons, comme un QUIT
		if (!client->getNickName().empty() && client->getRegistred())
			broadcastToPeers(*client, ":" + buildPrefix(*client) + " QUIT :Connection reset by peer", false);

		std::vector<Channel*> chans = getChannelsOf(client);
		for (size_t i = 0; i < chans.size(); i++)
			leaveChannel(*client, chans[i]);
	}

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