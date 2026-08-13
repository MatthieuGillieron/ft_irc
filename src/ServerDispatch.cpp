#include "../header/Server.hpp"
#include "../header/Message.hpp"

// Aiguillage d'une commande deja parsee.
// Trois etages : les commandes d'enregistrement, celles autorisees avant
// l'enregistrement, puis tout le reste qui exige un client enregistre.
void Server::dispatcher(Client* client, Message msg)
{
	if (msg.command.empty())
		return;

	// irssi envoie CAP a la connexion : on ne negocie aucune extension
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
	// un client peut avoir besoin de PING avant d'etre enregistre
	if (msg.command == "PING")
	{
		handlePing(*client, msg);
		return;
	}
	// et il doit toujours pouvoir partir
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


// === RECHERCHE ===

// recherche insensible a la casse : "#Test" et "#test" sont le meme salon.
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


Client* Server::findClientByNick(const std::string& nick)
{
	// sinon un pseudo vide correspondrait a tout client pas encore nomme
	if (nick.empty())
		return NULL;

	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (ircEqual(_clients[i]->getNickName(), nick))
			return _clients[i];
	}
	return NULL;
}


// tous les salons dont ce client est membre
// utilise par QUIT, par le changement de pseudo et par la deconnexion
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
