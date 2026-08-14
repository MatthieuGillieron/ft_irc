#include "../header/Message.hpp"

// Format d'une ligne IRC :
//   [":" prefixe SPACE] commande [SPACE parametres] [SPACE ":" trailing]
//
// Le prefixe est ignore : c'est le socket qui identifie l'emetteur, s'y fier
// permettrait d'ecrire au nom d'un autre. La commande est mise en majuscules
// car le RFC la definit insensible a la casse. Le trailing commence a ':' et
// va jusqu'au bout : c'est le seul parametre pouvant contenir des espaces.
Message Message::parse(std::string line)
{
    Message msg;
    size_t i = 0;

    if (!line.empty() && line[0] == ':')
    {
        size_t space = line.find(' ');
        if (space == std::string::npos)
            return msg;
        i = space + 1;
    }
    while (i < line.size() && line[i] == ' ')
        i++;

    size_t start = i;
    while (i < line.size() && line[i] != ' ')
        i++;
    msg.command = line.substr(start, i - start);

    for (size_t k = 0; k < msg.command.size(); k++)
    {
        unsigned char c = static_cast<unsigned char>(msg.command[k]);
        msg.command[k] = static_cast<char>(std::toupper(c));
    }
    while (i < line.size())
    {
        while (i < line.size() && line[i] == ' ')
            i++;
        if (i >= line.size())
            break;

        if (line[i] == ':')
        {
            msg.param.push_back(line.substr(i + 1));
            break;
        }
        start = i;
        while (i < line.size() && line[i] != ' ')
            i++;
        msg.param.push_back(line.substr(start, i - start));
    }
    return msg;
}
