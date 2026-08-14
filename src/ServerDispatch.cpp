#include "../header/Server.hpp"
#include "../header/Message.hpp"

// Aiguillage en trois etages : les commandes d'enregistrement, celles autorisees
// avant d'etre enregistre, puis tout le reste derriere une barriere 451 unique.
// CAP est ignore sans reponse : irssi l'envoie a la connexion pour negocier des
// extensions, et nous n'en gerons aucune.
void Server::dispatcher(Client* client, Message msg)
{
	if (msg.command.empty())
		return;
	if (msg.command == "CAP")
		return;
	if (msg.command == "PASS")
	{
		handlePass(*client, msg);
		return;
	}
	if (msg.command == "NICK")
	{
		handleNick(*client, msg);
		return;
	}
	if (msg.command == "USER")
	{
		handleUser(*client, msg);
		return;
	}
	if (msg.command == "PING")
	{
		handlePing(*client, msg);
		return;
	}
	if (msg.command == "QUIT")
	{
		handleQuit(*client, msg);
		return;
	}
	if (!client->getRegistred())
	{
		sendNumeric(*client, 451, "You have not registered");
		return;
	}
	if (msg.command == "JOIN")
		handleJoin(*client, msg);
	else if (msg.command == "PRIVMSG")
		handlePrivmsg(*client, msg);
	else if (msg.command == "NOTICE")
		handleNotice(*client, msg);
	else if (msg.command == "PART")
		handlePart(*client, msg);
	else if (msg.command == "TOPIC")
		handleTopic(*client, msg);
	else if (msg.command == "KICK")
		handleKick(*client, msg);
	else if (msg.command == "INVITE")
		handleInvite(*client, msg);
	else if (msg.command == "MODE")
		handleMode(*client, msg);
	else
		sendNumeric(*client, 421, msg.command, "Unknown command");
}

// Recherche insensible a la casse : "#Test" et "#test" designent le meme salon.
// C'est aussi ce qui empeche JOIN d'en creer deux variantes.
Channel* Server::findChannel(const std::string& name)
{
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (ircEqual(it->first, name))
			return it->second;
	}
	return NULL;
}

// Le nom vide est refuse : il correspondrait a tout client pas encore nomme.
Client* Server::findClientByNick(const std::string& nick)
{
	if (nick.empty())
		return NULL;

	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (ircEqual(_clients[i]->getNickName(), nick))
			return _clients[i];
	}
	return NULL;
}

// Utilise par QUIT, par le changement de pseudo et par la deconnexion.
std::vector<Channel*> Server::getChannelsOf(Client* client)
{
	std::vector<Channel*> out;

	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->second->isMember(client))
			out.push_back(it->second);
	}
	return out;
}