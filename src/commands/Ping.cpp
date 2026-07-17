

#include "../../header/Server.hpp"



void Server::handlePing(Client& client, const Message& msg)
{

	if (msg.param.empty())
	{
		reply(client, "409: No target for ping");
		return;
	}

	std::string value = msg.param[0];
	reply(client, "PONG :" + value);

}
