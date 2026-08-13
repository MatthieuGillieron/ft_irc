
#include "../../header/Server.hpp"


// Un salon vide ne doit pas survivre : il garderait ses modes, sa cle et sa
// liste d'invites pour le prochain qui le recree. removeMember retire aussi le
// statut d'operateur.
void Server::leaveChannel(Client &client, Channel *chan)
{
	chan->removeMember(&client);
	chan->removeInvite(client.getNickName());

	if (chan->isEmpty())
	{
		_channels.erase(chan->getName());
		delete chan;
	}
}


// Envoie a tous ceux qui partagent au moins un salon avec ce client. Le
// dedoublonnage est indispensable : sans lui, quelqu'un present dans trois
// salons communs recevrait le meme QUIT trois fois. Un client sans aucun salon
// doit tout de meme se voir notifie quand includeSelf est vrai.
void Server::broadcastToPeers(Client &client, const std::string &msg, bool includeSelf)
{
	std::vector<Channel*> chans = getChannelsOf(&client);
	std::vector<Client*> done;

	for (size_t i = 0; i < chans.size(); i++)
	{
		const std::vector<Client*> &members = chans[i]->getMembers();

		for (size_t k = 0; k < members.size(); k++)
		{
			if (members[k] == &client && !includeSelf)
				continue;

			bool seen = false;
			for (size_t d = 0; d < done.size(); d++)
			{
				if (done[d] == members[k])
				{
					seen = true;
					break;
				}
			}
			if (seen)
				continue;

			done.push_back(members[k]);
			reply(*members[k], msg);
		}
	}

	if (includeSelf)
	{
		for (size_t d = 0; d < done.size(); d++)
		{
			if (done[d] == &client)
				return;
		}
		reply(client, msg);
	}
}


// PART <salon>{,<salon>} [:<raison>]
// La diffusion precede le retrait du membre : sinon l'emetteur ne voit pas son
// propre depart et son client le croit toujours dans le salon.
void Server::handlePart(Client &client, const Message &msg)
{
	if (msg.param.empty())
	{
		sendNumeric(client, 461, "PART", "Not enough parameters");
		return;
	}

	std::string reason = (msg.param.size() > 1) ? msg.param[1] : client.getNickName();
	std::vector<std::string> names = splitList(msg.param[0], ',');

	for (size_t i = 0; i < names.size(); i++)
	{
		Channel *chan = findChannel(names[i]);
		if (chan == NULL)
		{
			sendNumeric(client, 403, names[i], "No such channel");
			continue;
		}
		if (!chan->isMember(&client))
		{
			sendNumeric(client, 442, chan->getName(), "You're not on that channel");
			continue;
		}

		chan->broadcast(":" + buildPrefix(client) + " PART " + chan->getName() + " :" + reason);
		leaveChannel(client, chan);
	}
}


// QUIT [:<raison>]
// L'emetteur recoit ERROR, pas son propre QUIT, puis part une fois son buffer
// de sortie vide.
void Server::handleQuit(Client &client, const Message &msg)
{
	std::string reason = msg.param.empty() ? "Client Quit" : msg.param[0];

	broadcastToPeers(client, ":" + buildPrefix(client) + " QUIT :Quit: " + reason, false);

	std::vector<Channel*> chans = getChannelsOf(&client);
	for (size_t i = 0; i < chans.size(); i++)
		leaveChannel(client, chans[i]);

	reply(client, "ERROR :Closing link (Quit: " + reason + ")");
	client.setQuitting(true);
}
