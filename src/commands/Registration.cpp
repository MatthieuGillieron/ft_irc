
#include "../../header/Server.hpp"



void Server::reply(Client &client, const std::string &msg)
{

	// revoir les retours protocole IRC
	std::string complete = msg + "\r\n";

	//changer avec pollout
	send(client.getFd(), complete.c_str(), complete.size(), 0);

}





void Server::handlePass(Client& client, const Message& msg)
{

	if (msg.param.empty())
	{
		reply(client, "461: Empty parameter");
		return;
	}

	if (client.getPass())
	{
		reply(client, "462: Already connected");
		return;
	}

	if (msg.param[0] == _password)
		client.setPass(true);
	else
		reply(client, "464: Password incorrect");

}






void Server::handleNick(Client &client, const Message &msg)
{
	if (msg.param.empty())
	{
		reply(client, "431: No nickname");
		return;
	}

	if ()


}
