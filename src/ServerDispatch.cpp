#include "../header/Server.hpp"
#include "../header/Message.hpp"

void Server::dispatcher(Client* client, Message msg)
{
	if(msg.command == "PASS")
	{
		handlePass(*client, msg);
	}
	else if(msg.command == "NICK")
		handleNick(*client, msg);
	else if(msg.command == "USER")
		handleUser(*client, msg);
	else if (msg.command == "PING")
		handlePing(*client, msg);
	else if (msg.command == "JOIN")
		handleJoin(*client, msg);

}

Channel* Server::findChannel(const std::string& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it == _channels.end())
		return NULL;
	return it->second;
}