
#include "../header/Server.hpp"

// Le message d'adieu est envoye par flushAll() avant d'arriver ici :
// le destructeur ne fait plus aucune I/O, tout passe par poll().
Server::~Server()
{
	for(size_t i = 0; i < _clients.size(); i++)
	{
		close(_clients[i]->getFd());
		delete(_clients[i]);
	}
	for(std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;
	if (_listenFd != -1)
		close(_listenFd);
}


// Vide les buffers de sortie restants, en passant par poll() comme le reste.
// Borne a 10 tours de 100 ms : un client qui ne lit plus ne doit pas
// empecher le serveur de s'arreter.
void Server::flushAll()
{
	for (int round = 0; round < 10; round++)
	{
		std::vector<struct pollfd> pending;

		for (size_t i = 0; i < _clients.size(); i++)
		{
			if (_clients[i]->getOutBuffer().empty())
				continue;

			struct pollfd p;
			p.fd = _clients[i]->getFd();
			p.events = POLLOUT;
			p.revents = 0;
			pending.push_back(p);
		}
		if (pending.empty())
			return;

		if (poll(&pending[0], pending.size(), 100) <= 0)
			return;

		for (size_t i = 0; i < pending.size(); i++)
		{
			if (pending[i].revents & POLLOUT)
				flushClient(pending[i].fd);
		}
	}
}


bool Server::run()
{
	if (!setupSocket())
		return false;

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
        			markDisconnect(_pollfds[i].fd);
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
		// un client marque "quitting" part une fois sa derniere reponse envoyee
		for (size_t c = 0; c < _clients.size(); c++)
		{
			if (_clients[c]->isQuitting() && _clients[c]->getOutBuffer().empty())
				markDisconnect(_clients[c]->getFd());
		}

		for (size_t k = 0; k < _toDisconnect.size(); k++)
    		disconnectClient(_toDisconnect[k]);
		_toDisconnect.clear();
    }

	std::cout << "Server shutting down..." << std::endl;
	for (size_t i = 0; i < _clients.size(); i++)
		reply(*_clients[i], "ERROR :Server shutting down");
	flushAll();
	return true;
}


bool Server::setupSocket()
{
	_listenFd = socket(AF_INET, SOCK_STREAM, 0);

	if(_listenFd == -1)
	{
		std::cerr << "socket() error: " << strerror(errno) << std::endl;
		return false;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(_port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	int opt = 1;
	setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(_listenFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1 )
	{
		std::cerr << "bind() error: " << strerror(errno) << std::endl;
		close(_listenFd);
		_listenFd = -1;
		return false;
	}
	if(listen(_listenFd, 5) == -1)
	{
		std::cerr << "listen() error: " << strerror(errno) << std::endl;
		close(_listenFd);
		_listenFd = -1;
		return false;
	}
	fcntl(_listenFd, F_SETFL, O_NONBLOCK);
	return true;
}
