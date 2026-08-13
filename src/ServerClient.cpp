#include "../header/Server.hpp"
#include "../header/Message.hpp"

// Le RFC plafonne un message a 512 octets. Sans cette borne, un client qui
// enverrait un flux sans jamais de retour a la ligne ferait grossir le buffer
// d'entree jusqu'a epuiser la memoire.
#define MAX_LINE_LENGTH 8192


void Server::acceptClient()
{
	sockaddr_in aclient;
	socklen_t clientLen = sizeof(aclient);

	int clientFd = accept(_listenFd, (struct sockaddr*)&aclient, &clientLen);
	if (clientFd == -1)
	{
		std::cerr << C_ERR << "[!] accept() error: " << strerror(errno) << RESET << std::endl;
		return;
	}
	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	struct pollfd acceptcl;
	acceptcl.fd = clientFd;
	acceptcl.events = POLLIN;
	acceptcl.revents = 0;

	_pollfds.push_back(acceptcl);
	_clients.push_back(new Client(clientFd));
	std::cout << C_UP << "[+] New connexion" << RESET << std::endl;
}


// Le sujet interdit de consulter errno apres un recv : un retour <= 0 est donc
// traite indistinctement comme un depart, poll() ayant deja signale le fd pret.
// Le decoupage se fait sur '\n' et non sur "\r\n" car netcat sans -C n'envoie
// qu'un LF, et le sujet exige que nc fonctionne.
void Server::handleClient(int fd)
{
	char recvBuffer[512];
	int bytesReceived = recv(fd, recvBuffer, sizeof(recvBuffer), 0);

	if (bytesReceived <= 0)
	{
		markDisconnect(fd);
		return;
	}

	Client* client = findClient(fd);
	if (client == NULL)
		return;

	client->appendToBuffer(std::string(recvBuffer, bytesReceived));

	size_t pos;
	while ((pos = client->getInBuffer().find('\n')) != std::string::npos)
	{
		std::string line = client->getInBuffer().substr(0, pos);
		client->eraseBuffer(0, pos + 1);

		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		Message msg = Message::parse(line);
		dispatcher(client, msg);
	}

	if (client->getInBuffer().size() > MAX_LINE_LENGTH)
		markDisconnect(fd);
}


// send peut n'ecrire qu'une partie du buffer : on n'efface que ce qui est parti,
// le reste sera retente au prochain POLLOUT. Le sujet interdisant de consulter
// errno, un echec laisse le buffer intact ; un client mort est ramasse par
// POLLHUP ou POLLERR en tete de boucle.
void Server::flushClient(int fd)
{
	Client* client = findClient(fd);
	if (client == NULL)
		return;

	std::string out = client->getOutBuffer();
	if (out.empty())
		return;

	int bytesSent = send(fd, out.c_str(), out.size(), 0);
	if (bytesSent <= 0)
		return;

	client->eraseOutBuffer(0, bytesSent);
}


// Un meme fd peut etre signale deux fois dans le meme tour (POLLHUP + recv a 0).
// Sans ce garde-fou, le second close() porterait sur un descripteur deja
// reattribue a une autre connexion.
void Server::markDisconnect(int fd)
{
	for (size_t i = 0; i < _toDisconnect.size(); i++)
	{
		if (_toDisconnect[i] == fd)
			return;
	}
	_toDisconnect.push_back(fd);
}


// Le client doit sortir de ses salons AVANT d'etre detruit, sinon
// Channel::_members garde un pointeur vers de la memoire liberee et le prochain
// broadcast ecrit dedans. Une coupure brutale est annoncee comme un QUIT.
void Server::disconnectClient(int fd)
{
	Client* client = findClient(fd);

	if (client != NULL)
	{
		std::cout << C_DOWN << "[-] Client disconnected" << RESET;
		if (!client->getNickName().empty())
			std::cout << C_DETAIL << " (" << client->getNickName() << ")" << RESET;
		std::cout << std::endl;

		if (!client->getNickName().empty() && client->getRegistred())
			broadcastToPeers(*client, ":" + buildPrefix(*client) + " QUIT :Connection reset by peer", false);

		std::vector<Channel*> chans = getChannelsOf(client);
		for (size_t i = 0; i < chans.size(); i++)
			leaveChannel(*client, chans[i]);
	}

	close(fd);
	for (size_t i = 0; i < _pollfds.size(); i++)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds.erase(_pollfds.begin() + i);
			break;
		}
	}
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i]->getFd() == fd)
		{
			delete _clients[i];
			_clients.erase(_clients.begin() + i);
			break;
		}
	}
}


Client* Server::findClient(int fd)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i]->getFd() == fd)
			return _clients[i];
	}
	return NULL;
}
