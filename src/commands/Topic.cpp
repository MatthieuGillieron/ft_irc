
#include "../../header/Server.hpp"


// TOPIC <salon> consulte le sujet, TOPIC <salon> :<sujet> le change et
// TOPIC <salon> : l'efface. La distinction tient au nombre de parametres : le
// parseur produit un parametre vide pour un trailing vide, donc "TOPIC #x :" en
// a bien deux. Le mode +t reserve la modification aux operateurs.
void Server::handleTopic(Client &client, const Message &msg)
{
	if (msg.param.empty())
	{
		sendNumeric(client, 461, "TOPIC", "Not enough parameters");
		return;
	}

	Channel *chan = findChannel(msg.param[0]);
	if (chan == NULL)
	{
		sendNumeric(client, 403, msg.param[0], "No such channel");
		return;
	}
	if (!chan->isMember(&client))
	{
		sendNumeric(client, 442, chan->getName(), "You're not on that channel");
		return;
	}

	if (msg.param.size() < 2)
	{
		if (chan->getTopic().empty())
			sendNumeric(client, 331, chan->getName(), "No topic is set");
		else
			sendNumeric(client, 332, chan->getName(), chan->getTopic());
		return;
	}

	if (chan->isTopicRestricted() && !chan->isModo(&client))
	{
		sendNumeric(client, 482, chan->getName(), "You're not channel operator");
		return;
	}

	chan->setTopic(msg.param[1]);
	chan->broadcast(":" + buildPrefix(client) + " TOPIC " + chan->getName() + " :" + msg.param[1]);
}
