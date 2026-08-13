

#include "../../header/Server.hpp"


void Server::handlePing(Client& client, const Message& msg)
{

	if (msg.param.empty())
	{
		sendNumeric(client, 409, "No origin specified");
		return;
	}

	reply(client, ":" SERVER_NAME " PONG " SERVER_NAME " :" + msg.param[0]);

}
