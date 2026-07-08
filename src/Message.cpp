#include "../header/Message.hpp"

Message Message::parse(std::string line)
{
    Message msg;
    size_t spacePos = line.find(' ');
    if(spacePos == std::string::npos)
    {
        msg.command = line;
        return msg;
    }
    msg.command = line.substr(0, spacePos);
    std::string rest = line.substr(spacePos + 1);
    while(!rest.empty())
    {
        if(rest[0] == ':')
        {
            msg.param.push_back(rest.substr(1));
            break;
        }
        else
        {
            size_t nextSpace = rest.find(' ');
            if(nextSpace == std::string::npos)
            {
                msg.param.push_back(rest);
                break;
            }
            else
            {
                msg.param.push_back(rest.substr(0, nextSpace));
                rest = rest.substr(nextSpace + 1);
            }
        }
    }
    return msg;
}
