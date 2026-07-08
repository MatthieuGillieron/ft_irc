
#include "../../header/Server.hpp"
#include <cctype>


void Server::reply(Client &client, const std::string &msg)
{

	// revoir les retours protocole IRC
	std::string complete = msg + "\r\n";

	//changer avec pollout
	send(client.getFd(), complete.c_str(), complete.size(), 0);

}



bool specialChar(char c)
{

std::string allowed = "[]\\_^{}|`";

	for (size_t i = 0; i < allowed.length(); i++)
	{
		if (allowed[i] == c)
			return true;
	}
	return false;
}


bool isValidNick(const std::string &nickName)
{

	if (nickName.empty())
		return false;


	char first = nickName[0];


	if (!isalpha(first) && !specialChar(first))
		return false;


	for (size_t i = 0; i < nickName.length(); i++)
	{
		if (!isdigit(nickName[i]) && !isalpha(nickName[i]) && !specialChar(nickName[i]) && nickName[i] != '-')
			return false;
	}
	return true;

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

	std::string nickName = msg.param[0];

	if (!isValidNick(nickName))
	{
		reply(client, "432 " + nickName + " :Erroneous nickname");
		return;
	}


	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i]->getNickName() == nickName && _clients[i] != &client)
		{
			reply(client, "433 " + nickName + " :Nickname is already in use");
			return;
		}
	}

	client.setNickName(nickName);
}



void Server::handleUser(Client& client, const Message& msg)
{
	if (client.getRegistred())
	{
		reply(client, "462 :Already registred");
		return;
	}

	if (msg.param.size() < 4)
	{
		reply(client, "461 USER :Not enough parameters");
		return;
	}

	client.setUserName(msg.param[0]);
}



