
#include "../../header/Server.hpp"


// === KICK ===

// KICK <salon> <pseudo>{,<pseudo>} [:<raison>]
void Server::handleKick(Client &client, const Message &msg)
{
	if (msg.param.size() < 2)
	{
		sendNumeric(client, 461, "KICK", "Not enough parameters");
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
	if (!chan->isModo(&client))
	{
		sendNumeric(client, 482, chan->getName(), "You're not channel operator");
		return;
	}

	std::string reason = (msg.param.size() > 2) ? msg.param[2] : client.getNickName();
	std::vector<std::string> nicks = splitList(msg.param[1], ',');

	for (size_t i = 0; i < nicks.size(); i++)
	{
		if (nicks[i].empty())
			continue;

		Client *target = findClientByNick(nicks[i]);
		if (target == NULL || !chan->isMember(target))
		{
			sendNumeric(client, 441, nicks[i] + " " + chan->getName(), "They aren't on that channel");
			continue;
		}

		// diffuser avant de retirer : l'exclu doit recevoir sa propre exclusion,
		// sinon son client reste affiche dans le salon
		chan->broadcast(":" + buildPrefix(client) + " KICK " + chan->getName()
			+ " " + target->getNickName() + " :" + reason);

		leaveChannel(*target, chan);

		// le salon a pu etre detruit si l'exclu etait le dernier membre
		if (findChannel(msg.param[0]) == NULL)
			return;
	}
}



// === INVITE ===

// INVITE <pseudo> <salon>
void Server::handleInvite(Client &client, const Message &msg)
{
	if (msg.param.size() < 2)
	{
		sendNumeric(client, 461, "INVITE", "Not enough parameters");
		return;
	}

	Client *target = findClientByNick(msg.param[0]);
	if (target == NULL)
	{
		sendNumeric(client, 401, msg.param[0], "No such nick/channel");
		return;
	}

	Channel *chan = findChannel(msg.param[1]);
	if (chan == NULL)
	{
		sendNumeric(client, 403, msg.param[1], "No such channel");
		return;
	}
	if (!chan->isMember(&client))
	{
		sendNumeric(client, 442, chan->getName(), "You're not on that channel");
		return;
	}
	// sur un salon +i, seuls les operateurs peuvent inviter
	if (chan->isInviteOnly() && !chan->isModo(&client))
	{
		sendNumeric(client, 482, chan->getName(), "You're not channel operator");
		return;
	}
	if (chan->isMember(target))
	{
		sendNumeric(client, 443, target->getNickName() + " " + chan->getName(), "is already on channel");
		return;
	}

	chan->addInvite(target->getNickName());

	// 341 n'a pas de trailing : <pseudo> <salon>
	sendNumeric(client, 341, target->getNickName() + " " + chan->getName(), "");
	reply(*target, ":" + buildPrefix(client) + " INVITE " + target->getNickName()
		+ " :" + chan->getName());
}
