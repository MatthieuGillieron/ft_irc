#include "../../header/Server.hpp"


static std::string numToString(size_t n)
{
	if (n == 0)
		return "0";

	std::string out;
	while (n > 0)
	{
		out.insert(out.begin(), static_cast<char>('0' + n % 10));
		n /= 10;
	}
	return out;
}

// -1 si ce n'est pas un entier positif exploitable
static long parseLimit(const std::string &s)
{
	if (s.empty() || s.size() > 9)
		return -1;

	long value = 0;
	for (size_t i = 0; i < s.size(); i++)
	{
		if (!std::isdigit(static_cast<unsigned char>(s[i])))
			return -1;
		value = value * 10 + (s[i] - '0');
	}
	return value;
}

// "+itk cle 10" : l'etat courant du salon, pour la reponse 324
static std::string currentModes(Channel *chan)
{
	std::string modes = "+";
	std::string args;

	if (chan->isInviteOnly())
		modes += "i";
	if (chan->isTopicRestricted())
		modes += "t";
	if (chan->hasKey())
	{
		modes += "k";
		args += " " + chan->getKey();
	}
	if (chan->hasLimit())
	{
		modes += "l";
		args += " " + numToString(chan->getLimit());
	}
	return modes + args;
}

// -k et -l ne prennent pas d'argument : les clients envoient "MODE #x -k" tout
// court, et l'exiger renverrait un 461 sur une commande normale.
static bool needsArg(char mode, bool adding)
{
	if (mode == 'o')
		return true;
	if (mode == 'k' || mode == 'l')
		return adding;
	return false;
}

bool Server::applyOneMode(Client &client, Channel *chan, char mode, bool adding, std::string &arg)
{
	if (mode == 'i')
	{
		chan->setInviteOnly(adding);
		return true;
	}
	if (mode == 't')
	{
		chan->setTopicRestricted(adding);
		return true;
	}
	if (mode == 'k')
	{
		if (adding)
			chan->setKey(arg);
		else
			chan->removeKey();
		return true;
	}
	if (mode == 'l')
	{
		if (!adding)
		{
			chan->removeLimit();
			return true;
		}
		long value = parseLimit(arg);
		if (value <= 0)
			return false;
		chan->setLimit(static_cast<size_t>(value));
		return true;
	}
	Client *target = findClientByNick(arg);
	if (target == NULL || !chan->isMember(target))
	{
		sendNumeric(client, 441, arg + " " + chan->getName(), "They aren't on that channel");
		return false;
	}

	arg = target->getNickName();
	if (adding)
		chan->addModo(target);
	else
		chan->removeModo(target);
	return true;
}

// Parcourt "+ok-l bob" : le signe courant s'applique jusqu'au suivant, et
// chaque flag consomme ou non un argument dans l'ordre d'arrivee. Seuls les
// changements reellement appliques entrent dans le message diffuse, pour ne pas
// annoncer une limite invalide qui a ete ignoree.
void Server::applyModes(Client &client, Channel *chan, const Message &msg)
{
	std::string flags = msg.param[1];
	size_t argIndex = 2;
	bool adding = true;

	std::string doneModes;
	std::string doneArgs;
	char lastSign = 0;

	for (size_t i = 0; i < flags.size(); i++)
	{
		char c = flags[i];

		if (c == '+' || c == '-')
		{
			adding = (c == '+');
			continue;
		}

		if (c != 'i' && c != 't' && c != 'k' && c != 'o' && c != 'l')
		{
			sendNumeric(client, 472, std::string(1, c), "is unknown mode char to me");
			continue;
		}
		std::string arg;
		if (needsArg(c, adding))
		{
			if (argIndex >= msg.param.size())
			{
				sendNumeric(client, 461, "MODE", "Not enough parameters");
				continue;
			}
			arg = msg.param[argIndex++];
		}
		if (!applyOneMode(client, chan, c, adding, arg))
			continue;

		char sign = adding ? '+' : '-';
		if (sign != lastSign)
		{
			doneModes += sign;
			lastSign = sign;
		}
		doneModes += c;
		if (!arg.empty())
			doneArgs += " " + arg;
	}
	if (doneModes.empty())
		return;
	
	chan->broadcast(":" + buildPrefix(client) + " MODE " + chan->getName()
		+ " " + doneModes + doneArgs);
}

// MODE <salon> [<flags> [<arguments>]]
// Aucun mode utilisateur n'est gere : MODE <pseudo> est ignore. L'appartenance
// est verifiee avant l'affichage car la reponse 324 revele la cle du salon.
void Server::handleMode(Client &client, const Message &msg)
{
	if (msg.param.empty())
	{
		sendNumeric(client, 461, "MODE", "Not enough parameters");
		return;
	}

	if (msg.param[0].empty() || msg.param[0][0] != '#')
		return;

	Channel *chan = findChannel(msg.param[0]);
	if (chan == NULL)
	{
		sendNumeric(client, 403, msg.param[0], "No such channel");
		return;
	}
	if (!chan->isMember(&client))
	{
		sendNumeric(client, 442, chan->getName(), "You're not on that channel");
		return;
	}

	if (msg.param.size() < 2)
	{
		sendNumeric(client, 324, chan->getName() + " " + currentModes(chan), "");
		return;
	}

	if (!chan->isModo(&client))
	{
		sendNumeric(client, 482, chan->getName(), "You're not channel operator");
		return;
	}

	applyModes(client, chan, msg);
}
