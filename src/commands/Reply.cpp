
#include "../../header/Server.hpp"
#include <cctype>


// === UTILS ===

// les codes numeriques IRC s'ecrivent toujours sur 3 chiffres : 1 -> "001"
static std::string codeToString(int code)
{
	std::string out;

	out += static_cast<char>('0' + (code / 100) % 10);
	out += static_cast<char>('0' + (code / 10) % 10);
	out += static_cast<char>('0' + code % 10);
	return out;
}


std::string buildPrefix(const Client &client)
{
	return client.getNickName() + "!" + client.getUsrName() + "@localhost";
}


std::vector<std::string> splitList(const std::string &line, char sep)
{
	std::vector<std::string> out;
	size_t start = 0;

	while (true)
	{
		size_t pos = line.find(sep, start);
		if (pos == std::string::npos)
		{
			out.push_back(line.substr(start));
			break;
		}
		out.push_back(line.substr(start, pos - start));
		start = pos + 1;
	}
	return out;
}


bool ircEqual(const std::string &a, const std::string &b)
{
	if (a.size() != b.size())
		return false;

	for (size_t i = 0; i < a.size(); i++)
	{
		unsigned char ca = static_cast<unsigned char>(a[i]);
		unsigned char cb = static_cast<unsigned char>(b[i]);

		if (std::tolower(ca) != std::tolower(cb))
			return false;
	}
	return true;
}



// === ENVOI ===

void Server::reply(Client &client, const std::string &msg)
{
	client.appendToOutBuffer(msg + "\r\n");
}


// ":ircserv <code> <nick> <params> :<trailing>"
// tant que le client n'a pas de pseudo, le destinataire s'ecrit "*"
void Server::sendNumeric(Client &client, int code, const std::string &params, const std::string &trailing)
{
	std::string line = ":" SERVER_NAME " " + codeToString(code) + " ";

	if (client.getNickName().empty())
		line += "*";
	else
		line += client.getNickName();

	if (!params.empty())
		line += " " + params;
	if (!trailing.empty())
		line += " :" + trailing;

	reply(client, line);
}


void Server::sendNumeric(Client &client, int code, const std::string &trailing)
{
	sendNumeric(client, code, "", trailing);
}
