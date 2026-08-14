#include "../../header/Server.hpp"

bool Server::checkJoin(Client &client, Channel *chan, const std::string &key)
{
	std::string nick = client.getNickName();
	std::string chanName = chan->getName();

	if (chan->isInviteOnly() && !chan->isInvited(nick))
	{
		sendNumeric(client, 473, chanName, "Cannot join channel (+i)");
		return false;
	}
	if (chan->hasKey() && key != chan->getKey())
	{
		sendNumeric(client, 475, chanName, "Cannot join channel (+k)");
		return false;
	}
	if (chan->hasLimit() && chan->getMemberCount() >= chan->getLimit())
	{
		sendNumeric(client, 471, chanName, "Cannot join channel (+l)");
		return false;
	}
	return true;
}

void Server::joinReplies(Client &client, Channel *chan)
{
	std::string nick = client.getNickName();
	std::string chanName = chan->getName();

	if (!chan->getTopic().empty())
		sendNumeric(client, 332, chanName, chan->getTopic());

	sendNumeric(client, 353, "= " + chanName, chan->getNamesList());
	sendNumeric(client, 366, chanName, "End of /NAMES list");
}

void Server::handleJoin(Client &client, const Message &msg)
{
	if (msg.param.empty())
	{
		sendNumeric(client, 461, "JOIN", "Not enough parameters");
		return;
	}

	std::string chanName = msg.param[0];
	std::string key = (msg.param.size() > 1) ? msg.param[1] : "";

	if (chanName.empty() || chanName[0] != '#')
	{
		sendNumeric(client, 403, chanName, "No such channel");
		return;
	}

	Channel *chan = findChannel(chanName);
	bool created = false;
	if (chan == NULL)
	{
		chan = new Channel(chanName);
		_channels[chanName] = chan;
		created = true;
	}

	if (chan->isMember(&client))
		return;

	if (!created && !checkJoin(client, chan, key))
		return;


	chan->addMember(&client);
	if (created)
		chan->addModo(&client);
	chan->removeInvite(client.getNickName());

	chan->broadcast(":" + buildPrefix(client) + " JOIN " + chanName);
	joinReplies(client, chan);
}