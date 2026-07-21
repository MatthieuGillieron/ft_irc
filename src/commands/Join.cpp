
#include "../../header/Server.hpp"



// prefix + info sur le join
static std::string buildPrefix(Client &client)
{
	return client.getNickName() + "!" + client.getUsrName() + "@localhost";
}


void Server::handleJoin(Client &client, const Message &msg)
{

	if (!client.getRegistred())
	{
		reply(client, ":ircserv 451 * :You have not registered");
		return;
	}


	if (msg.param.empty())
	{
		reply(client, ":ircserv 461 " + client.getNickName() + " JOIN :Not enough parameters");
		return;
	}

	std::string chanName = msg.param[0];

	std::string key = (msg.param.size() > 1) ? msg.param[1] : "";


	if (chanName.empty() || chanName[0] != '#')
	{
		reply(client, ":ircserv 403 " + client.getNickName() + " " + chanName + " :No such channel");
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


	if (!created)
	{

		if (chan->isInviteOnly() && !chan->isInvited(client.getNickName()))
		{
			reply(client, ":ircserv 473 " + client.getNickName() + " " + chanName + " :Cannot join channel (+i)");
			return;
		}

		if (chan->hasKey() && key != chan->getKey())
		{
			reply(client, ":ircserv 475 " + client.getNickName() + " " + chanName + " :Cannot join channel (+k)");
			return;
		}

		if (chan->hasLimit() && chan->getMemberCount() >= chan->getLimit())
		{
			reply(client, ":ircserv 471 " + client.getNickName() + " " + chanName + " :Cannot join channel (+l)");
			return;
		}
	}

	// modo si createur
	chan->addMember(&client);
	if (created)
		chan->addModo(&client);

	chan->removeInvite(client.getNickName());


	chan->broadcast(":" + buildPrefix(client) + " JOIN " + chanName);

	if (!chan->getTopic().empty())
		reply(client, ":ircserv 332 " + client.getNickName() + " " + chanName + " :" + chan->getTopic());

	reply(client, ":ircserv 353 " + client.getNickName() + " = " + chanName + " :" + chan->getNamesList());
	reply(client, ":ircserv 366 " + client.getNickName() + " " + chanName + " :End of /NAMES list");
}
