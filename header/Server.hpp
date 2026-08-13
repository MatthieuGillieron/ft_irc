#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Message.hpp"
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

extern volatile sig_atomic_t g_shutdown;

// nom du serveur, utilise comme prefixe de toutes les reponses
#define SERVER_NAME "ircserv"

// prefixe d'un message emis au nom d'un client : "nick!user@localhost"
std::string buildPrefix(const Client &client);

// comparaison de pseudos / de noms de salon : IRC est insensible a la casse
bool ircEqual(const std::string &a, const std::string &b);

// "#a,#b,#c" -> ["#a", "#b", "#c"] : listes de cibles de JOIN, PART et PRIVMSG
std::vector<std::string> splitList(const std::string &line, char sep);

class Server
{
    public:
    Server(unsigned int port, std::string password) : _port(port), _password(password), _listenFd(-1) {};
    ~Server();

    bool run(); // poll ; false si le serveur n'a pas pu demarrer

	bool setupSocket(); // socket ecoute
	void acceptClient();

	void handleClient(int fd);
	void handlePass(Client& client, const Message& msg);
	void handleNick(Client &client, const Message &msg);
	void handleUser(Client& client, const Message& msg);
	void handlePing(Client& client, const Message& msg);
	void handleJoin(Client& client, const Message& msg);
	bool checkJoin(Client& client, Channel* chan, const std::string& key);
	void joinReplies(Client& client, Channel* chan);

	void handlePrivmsg(Client& client, const Message& msg);
	void handleNotice(Client& client, const Message& msg);
	// coeur commun de PRIVMSG et NOTICE : NOTICE n'emet jamais d'erreur
	void sendMessage(Client& client, const Message& msg, const std::string& cmd, bool silent);

	void handlePart(Client& client, const Message& msg);
	void handleQuit(Client& client, const Message& msg);

	// envoie a tous ceux qui partagent un salon avec ce client, une seule fois chacun
	void broadcastToPeers(Client& client, const std::string& msg, bool includeSelf);
	// retire le client d'un salon et detruit celui-ci s'il devient vide
	void leaveChannel(Client& client, Channel* chan);

	void handleTopic(Client& client, const Message& msg);
	void handleKick(Client& client, const Message& msg);
	void handleInvite(Client& client, const Message& msg);

	void handleMode(Client& client, const Message& msg);
	void applyModes(Client& client, Channel* chan, const Message& msg);
	// applique un seul flag ; renvoie false si le changement n'a pas eu lieu
	bool applyOneMode(Client& client, Channel* chan, char mode, bool adding, std::string& arg);

	void disconnectClient(int fd);
	void markDisconnect(int fd); // met un fd en file de deconnexion, sans doublon

	// === ENVOI ===
	// ligne brute, le CRLF est ajoute
	void reply(Client &client, const std::string &msg);
	// ":ircserv <code> <nick> <params> :<trailing>"
	void sendNumeric(Client &client, int code, const std::string &params, const std::string &trailing);
	void sendNumeric(Client &client, int code, const std::string &trailing);

	void checkRegister(Client &client);


	void dispatcher(Client* client, Message msg);

	void flushClient(int fd);
	Client* findClient(int fd);
	Client* findClientByNick(const std::string& nick);
	Channel* findChannel(const std::string& name);
	std::vector<Channel*> getChannelsOf(Client* client);

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