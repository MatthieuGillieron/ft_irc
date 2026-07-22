
#include "../../header/Server.hpp"


static std::string buildPrefix(Client &client)
{
	return client.getNickName() + "!" + client.getUsrName() + "@localhost";
}


bool Server::checkJoin(Client &client, Channel *chan, const std::string &key)
{
	std::string nick = client.getNickName();
	std::string chanName = chan->getName();

	if (chan->isInviteOnly() && !chan->isInvited(nick))
	{
		reply(client, ":ircserv 473 " + nick + " " + chanName + " :Cannot join channel (+i)");
		return false;
	}
	if (chan->hasKey() && key != chan->getKey())
	{
		reply(client, ":ircserv 475 " + nick + " " + chanName + " :Cannot join channel (+k)");
		return false;
	}
	if (chan->hasLimit() && chan->getMemberCount() >= chan->getLimit())
	{
		reply(client, ":ircserv 471 " + nick + " " + chanName + " :Cannot join channel (+l)");
		return false;
	}
	return true;
}




void Server::joinReplies(Client &client, Channel *chan)
{
	std::string nick = client.getNickName();
	std::string chanName = chan->getName();

	if (!chan->getTopic().empty())
		reply(client, ":ircserv 332 " + nick + " " + chanName + " :" + chan->getTopic());

	reply(client, ":ircserv 353 " + nick + " = " + chanName + " :" + chan->getNamesList());
	reply(client, ":ircserv 366 " + nick + " " + chanName + " :End of /NAMES list");
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

	if (!created && !checkJoin(client, chan, key))
		return;


	chan->addMember(&client);
	if (created)
		chan->addModo(&client);
	chan->removeInvite(client.getNickName());

	chan->broadcast(":" + buildPrefix(client) + " JOIN " + chanName);
	joinReplies(client, chan);
}
