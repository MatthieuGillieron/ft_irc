
#include "../../header/Server.hpp"


// PRIVMSG et NOTICE ne different que par le drapeau silent : le RFC interdit
// d'emettre la moindre reponse automatique en retour d'un NOTICE, sous peine de
// boucle infinie entre deux serveurs. Le texte est normalement le trailing,
// mais on accepte aussi "PRIVMSG #chan bonjour tout le monde" sans les ':'.
void Server::sendMessage(Client &client, const Message &msg, const std::string &cmd, bool silent)
{
	if (msg.param.empty())
	{
		if (!silent)
			sendNumeric(client, 411, "No recipient given (" + cmd + ")");
		return;
	}

	std::string text;
	for (size_t i = 1; i < msg.param.size(); i++)
	{
		if (i > 1)
			text += " ";
		text += msg.param[i];
	}

	if (text.empty())
	{
		if (!silent)
			sendNumeric(client, 412, "No text to send");
		return;
	}

	std::vector<std::string> targets = splitList(msg.param[0], ',');

	for (size_t i = 0; i < targets.size(); i++)
	{
		std::string target = targets[i];
		if (target.empty())
			continue;

		if (target[0] == '#')
		{
			Channel *chan = findChannel(target);
			if (chan == NULL)
			{
				if (!silent)
					sendNumeric(client, 403, target, "No such channel");
				continue;
			}
			if (!chan->isMember(&client))
			{
				if (!silent)
					sendNumeric(client, 404, target, "Cannot send to channel");
				continue;
			}
			chan->broadcast(":" + buildPrefix(client) + " " + cmd + " "
				+ chan->getName() + " :" + text, &client);
		}
		else
		{
			Client *dest = findClientByNick(target);
			if (dest == NULL)
			{
				if (!silent)
					sendNumeric(client, 401, target, "No such nick/channel");
				continue;
			}
			reply(*dest, ":" + buildPrefix(client) + " " + cmd + " "
				+ dest->getNickName() + " :" + text);
		}
	}
}


void Server::handlePrivmsg(Client &client, const Message &msg)
{
	sendMessage(client, msg, "PRIVMSG", false);
}


void Server::handleNotice(Client &client, const Message &msg)
{
	sendMessage(client, msg, "NOTICE", true);
}
