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

// Taille au-dela de laquelle une ligne sans fin est consideree hostile.
// Le RFC plafonne un message a 512 octets ; sans cette borne, un client qui
// enverrait un flux sans jamais de retour a la ligne ferait grossir le buffer
// jusqu'a epuiser la memoire.
#define MAX_LINE_LENGTH 8192

void Server::handleClient(int fd)
{
	char recvBuffer[512];
	int bytesReceived = recv(fd, recvBuffer, sizeof(recvBuffer), 0);

	// poll() vient de signaler ce descripteur pret : un retour <= 0 est soit une
	// fin de flux, soit une erreur reelle. Le sujet interdit de consulter errno
	// apres un recv pour decider de la suite, donc les deux cas sont traites
	// de la meme facon : le client s'en va.
	if (bytesReceived <= 0)
	{
		markDisconnect(fd);
		return;
	}

	Client* client = findClient(fd);
	if (client == NULL)
		return;

	// on construit la chaine a partir de la longueur lue, jamais d'un '\0'
	// ajoute a la main : recvBuffer[512] serait hors du tampon
	client->appendToBuffer(std::string(recvBuffer, bytesReceived));

	// On decoupe sur '\n' et non sur "\r\n" : le protocole impose CRLF, mais
	// netcat sans -C n'envoie qu'un LF, et le sujet demande que nc fonctionne.
	// Le '\r' eventuel est retire juste apres.
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

	// ligne interminable : on coupe plutot que de laisser le buffer enfler
	if (client->getInBuffer().size() > MAX_LINE_LENGTH)
		markDisconnect(fd);
}

void Server::flushClient(int fd)
{
	Client* client = findClient(fd);
	if (client == NULL) return;

	std::string out = client->getOutBuffer();
	if (out.empty())
		return;

	int bytesSent = send(fd, out.c_str(), out.size(), 0);

	// Le sujet interdit de consulter errno apres un send. On ne peut donc pas
	// distinguer "tampon noyau plein" d'une erreur reelle : on ne touche pas au
	// buffer, l'envoi sera retente au prochain POLLOUT. Un client reellement
	// mort est ramasse par POLLHUP / POLLERR en tete de boucle.
	if (bytesSent <= 0)
		return;

	// on n'efface QUE ce qui est reellement parti : send peut etre partiel
	client->eraseOutBuffer(0, bytesSent);
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