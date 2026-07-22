
#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include "Client.hpp"

class Channel
{
	public:
		Channel(const std::string &name);
		~Channel() {};

		// === GETTERS ===
		std::string getName() const { return _name; }
		std::string getTopic() const { return _topic; }
		std::string getKey() const { return _key; }
		size_t getLimit() const { return _limit; }

		bool isInviteOnly() const { return _inviteOnly; }
		bool isTopicRestricted() const { return _topicRestricted; }
		bool hasKey() const { return _hasKey; }
		bool hasLimit() const { return _hasLimit; }


		// === SETTERS / MODES ===
		void setTopic(const std::string &topic) { _topic = topic; }

		void setInviteOnly(bool value) { _inviteOnly = value; }
		void setTopicRestricted(bool value) { _topicRestricted = value; }
		void setKey(const std::string &key) { _key = key; _hasKey = true; }
		void removeKey() { _key.clear(); _hasKey = false; }
		void setLimit(size_t limit) { _limit = limit; _hasLimit = true; }
		void removeLimit() { _limit = 0; _hasLimit = false; }


		// === MEMBERS ===
		void addMember(Client *client);
		void removeMember(Client *client);
		bool isMember(Client *client) const;
		size_t getMemberCount() const { return _members.size(); }
		const std::vector<Client*> &getMembers() const { return _members; }
		bool isEmpty() const { return _members.empty(); }


		// === MODOS ===
		void addModo(Client *client);
		void removeModo(Client *client);
		bool isModo(Client *client) const;


		// === INVITES (par nickname) ===
		void addInvite(const std::string &nick);
		void removeInvite(const std::string &nick);
		bool isInvited(const std::string &nick) const;


		// === UTILS ===
		void broadcast(const std::string &msg, Client *except = NULL);
		std::string getNamesList() const;



	private:
		std::string _name;
		std::string _topic;
		std::string _key;
		size_t _limit;

		bool _inviteOnly;
		bool _topicRestricted;
		bool _hasKey;
		bool _hasLimit;

		std::vector<Client*> _members;
		std::vector<Client*> _modos;
		std::vector<std::string> _invited;
};

#endif
