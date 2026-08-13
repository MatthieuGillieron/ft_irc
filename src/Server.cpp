
#include "../header/Server.hpp"

// Le message d'adieu est envoye par la phase d'arret de run() avant d'arriver
// ici : le destructeur ne fait plus aucune I/O, tout passe par poll().
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


// Un SEUL poll() dans tout le projet, celui de cette boucle.
// L'extinction n'a donc pas sa propre boucle d'attente : elle bascule la meme
// dans un mode "closing" ou l'on ne fait plus qu'ecrire, jusqu'a ce que les
// buffers de sortie soient vides.
bool Server::run()
{
	if (!setupSocket())
		return false;

	struct pollfd pfd;
	pfd.fd = _listenFd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	_pollfds.push_back(pfd);

	bool closing = false;
	int closingRounds = 0;

	while (true)
	{
		// premier tour apres le signal : on empile le message d'adieu
		if (g_shutdown && !closing)
		{
			closing = true;
			std::cout << "Server shutting down..." << std::endl;
			for (size_t i = 0; i < _clients.size(); i++)
				reply(*_clients[i], "ERROR :Server shutting down");
		}

		bool pending = false;
		for (size_t y = 0; y < _pollfds.size(); y++)
		{
			if (_pollfds[y].fd == _listenFd)
			{
				// en phase d'arret on n'accepte plus personne
				_pollfds[y].events = closing ? 0 : POLLIN;
				continue;
			}

			Client* client = findClient(_pollfds[y].fd);
			if (client == NULL)
			{
				_pollfds[y].events = 0;
				continue;
			}

			bool hasOut = !client->getOutBuffer().empty();
			if (hasOut)
				pending = true;

			// on ne demande POLLOUT que si on a quelque chose a ecrire :
			// sinon poll reviendrait immediatement a chaque tour
			if (closing)
				_pollfds[y].events = hasOut ? POLLOUT : 0;
			else
				_pollfds[y].events = hasOut ? (POLLIN | POLLOUT) : POLLIN;
		}

		// plus rien a envoyer, ou un client qui ne lit plus : on sort.
		// La borne evite qu'un client bloque l'arret du serveur.
		if (closing && (!pending || ++closingRounds > 10))
			break;

		int ret = poll(&_pollfds[0], _pollfds.size(), closing ? 100 : -1);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
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

			if (closing)
			{
				// pendant l'extinction on n'ecoute plus, on ne fait que vider
				if (_pollfds[i].revents & POLLOUT)
					flushClient(_pollfds[i].fd);
				continue;
			}

			if (_pollfds[i].revents & POLLIN)
			{
				if (_pollfds[i].fd == _listenFd)
					acceptClient();
				else
					handleClient(_pollfds[i].fd);
			}
			if (_pollfds[i].revents & POLLOUT)
				flushClient(_pollfds[i].fd);
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
