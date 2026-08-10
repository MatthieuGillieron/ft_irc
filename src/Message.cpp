#include "../header/Message.hpp"
#include <cctype>

// Format d'une ligne IRC :
//   [":" prefixe SPACE] commande [SPACE parametres] [SPACE ":" trailing]
// Le serveur ignore le prefixe : c'est le socket qui identifie l'emetteur.
// La commande est insensible a la casse ("join" == "JOIN").
Message Message::parse(std::string line)
{
    Message msg;
    size_t i = 0;

    // prefixe optionnel -> on le saute
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
        // plusieurs espaces d'affilee ne doivent pas creer de parametre vide
        while (i < line.size() && line[i] == ' ')
            i++;
        if (i >= line.size())
            break;

        // le trailing commence par ':' et va jusqu'a la fin de la ligne
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
