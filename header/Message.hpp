#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include <string>
#include <cctype>

struct Message
{

	std::string command;
	std::vector<std::string> param;
	static Message parse(std::string line);

};

#endif