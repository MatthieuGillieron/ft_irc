
#include "../header/Channel.hpp"


Channel::Channel(const std::string &name)
	: _name(name), _limit(0), _inviteOnly(false),
	  _topicRestricted(false), _hasKey(false), _hasLimit(false)
{
}




// === MEMBERS ===

void Channel::addMember(Client *client)
{
	if (isMember(client))
		return;
	_members.push_back(client);
}

void Channel::removeMember(Client *client)
{
	for (size_t i = 0; i < _members.size(); i++)
	{
		if (_members[i] == client)
		{
			_members.erase(_members.begin() + i);
			break;
		}
	}
	// un membre qui part ne doit plus etre modo
	removeModo(client);
}

bool Channel::isMember(Client *client) const
{
	for (size_t i = 0; i < _members.size(); i++)
	{
		if (_members[i] == client)
			return true;
	}
	return false;
}





// === MODERATOR ===

void Channel::addModo(Client *client)
{
	if (isModo(client))
		return;
	_modos.push_back(client);
}

void Channel::removeModo(Client *client)
{
	for (size_t i = 0; i < _modos.size(); i++)
	{
		if (_modos[i] == client)
		{
			_modos.erase(_modos.begin() + i);
			break;
		}
	}
}

bool Channel::isModo(Client *client) const
{
	for (size_t i = 0; i < _modos.size(); i++)
	{
		if (_modos[i] == client)
			return true;
	}
	return false;
}





// === INVITES ===

void Channel::addInvite(const std::string &nick)
{
	if (isInvited(nick))
		return;
	_invited.push_back(nick);
}

void Channel::removeInvite(const std::string &nick)
{
	for (size_t i = 0; i < _invited.size(); i++)
	{
		if (_invited[i] == nick)
		{
			_invited.erase(_invited.begin() + i);
			break;
		}
	}
}

bool Channel::isInvited(const std::string &nick) const
{
	for (size_t i = 0; i < _invited.size(); i++)
	{
		if (_invited[i] == nick)
			return true;
	}
	return false;
}





// === UTILS ===

void Channel::broadcast(const std::string &msg, Client *except)
{
	std::string complete = msg + "\r\n";
	for (size_t i = 0; i < _members.size(); i++)
	{
		if (_members[i] == except)
			continue;
		_members[i]->appendToOutBuffer(complete);
	}
}

std::string Channel::getNamesList() const
{
	std::string list;
	for (size_t i = 0; i < _members.size(); i++)
	{
		if (i > 0)
			list += " ";
		if (isModo(_members[i]))
			list += "@";
		list += _members[i]->getNickName();
	}
	return list;
}
