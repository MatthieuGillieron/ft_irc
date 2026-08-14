#include "../../header/Server.hpp"

// Les trois informations reunies font basculer le client dans l'etat enregistre
// et declenchent les reponses 001 a 004. La 004 n'a pas de trailing :
// <serveur> <version> <modes user> <modes salon>.
void Server::checkRegister(Client &client)
{
	if (!client.getPass() || client.getNickName().empty() || client.getUsrName().empty())
		return;

	if (client.getRegistred())
		return;

	client.setRegistred(true);

	sendNumeric(client, 1, "Welcome to the Internet Relay Network " + buildPrefix(client));
	sendNumeric(client, 2, "Your host is " SERVER_NAME ", running version 1.0");
	sendNumeric(client, 3, "This server was created recently");
	sendNumeric(client, 4, SERVER_NAME " 1.0 o itkol", "");
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
	if (nickName.empty() || nickName.length() > 9)
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

// Un mot de passe faux recoit sa reponse, puis la connexion se ferme une fois
// le buffer de sortie parti.
void Server::handlePass(Client& client, const Message& msg)
{
	if (client.getRegistred())
	{
		sendNumeric(client, 462, "You may not reregister");
		return;
	}

	if (msg.param.empty())
	{
		sendNumeric(client, 461, "PASS", "Not enough parameters");
		return;
	}
	if (msg.param[0] != _password)
	{
		sendNumeric(client, 464, "Password incorrect");
		reply(client, "ERROR :Closing link (bad password)");
		client.setQuitting(true);
		return;
	}
	client.setPass(true);
	checkRegister(client);
}

// PASS doit avoir ete fourni en premier. L'unicite est insensible a la casse.
// Un changement en cours de session est diffuse a tous les salons du client, le
// message portant l'ancien prefixe puisqu'il annonce qui change ; les
// invitations en attente, nominatives, suivent le nouveau pseudo.
void Server::handleNick(Client &client, const Message &msg)
{
	if (!client.getPass())
	{
		sendNumeric(client, 451, "You have not registered");
		return;
	}
	if (msg.param.empty())
	{
		sendNumeric(client, 431, "No nickname given");
		return;
	}

	std::string nickName = msg.param[0];

	if (!isValidNick(nickName))
	{
		sendNumeric(client, 432, nickName, "Erroneous nickname");
		return;
	}
	Client *other = findClientByNick(nickName);
	if (other != NULL && other != &client)
	{
		sendNumeric(client, 433, nickName, "Nickname is already in use");
		return;
	}
	std::string oldNick = client.getNickName();
	if (oldNick == nickName)
		return;
	if (client.getRegistred())
	{
		std::string line = ":" + buildPrefix(client) + " NICK :" + nickName;

		std::vector<Channel*> chans = getChannelsOf(&client);
		for (size_t i = 0; i < chans.size(); i++)
		{
			if (chans[i]->isInvited(oldNick))
			{
				chans[i]->removeInvite(oldNick);
				chans[i]->addInvite(nickName);
			}
		}
		client.setNickName(nickName);
		broadcastToPeers(client, line, true);
		return;
	}
	client.setNickName(nickName);
	checkRegister(client);
}

void Server::handleUser(Client& client, const Message& msg)
{
	if (!client.getPass())
	{
		sendNumeric(client, 451, "You have not registered");
		return;
	}
	if (client.getRegistred())
	{
		sendNumeric(client, 462, "You may not reregister");
		return;
	}
	if (msg.param.size() < 4)
	{
		sendNumeric(client, 461, "USER", "Not enough parameters");
		return;
	}
	client.setUserName(msg.param[0]);
	checkRegister(client);
}