
#include "../header/Server.hpp"

#include <iostream>
#include <cstdlib>



int main(int ac, char **av)
{

	if (ac != 3)
	{
		std::cout << "Usage: ./irc <port> <password>" << std::endl;
		return 1;
	}


	unsigned int port = std::atoi(av[1]);
	if (port < 1024 || port > 65535)
	{
		std::cout << "Error of range port : 1024 - 65535 " << std::endl;
		return 1;
	}


	std::string password(av[2]);
	//check si mdp ok etcc ..



	std::cout << " Running ..." << std::endl;

	Server server(port, password);
	server.run();

	return 0;
}


/**
 * # ft_irc — Répartition des tâches

## Coéquipier 1 — Couche réseau & cycle de vie des connexions

### Phase 2 — Buffering & Parsing
- Buffer par client (`inBuffer` dans `Client`)
- Accumulation des données jusqu'à `\r\n`
- Parser générique : extraire `commande + paramètres + trailing`
- Dispatcher vers les handlers

### Phase 6 — Robustesse
- SIGINT (Ctrl-C) → shutdown propre, fermeture de tous les fd
- Vérification des leaks (valgrind)
- Edge cases : déconnexion brutale, données fragmentées, gros volumes

---

## Coéquipier 2 — Logique IRC & protocole

### Phase 3 — Enregistrement
- `PASS` → vérification mot de passe (erreur 464)
- `NICK` → unicité (433), validité (432)
- `USER` → username + realname
- Message de bienvenue `001` quand les 3 sont OK
- `PING/PONG` → sans ça irssi coupe la connexion

### Phase 4 — Chat de base
- Classe `Channel` (nom, membres, opérateurs, topic)
- `JOIN` → créer si inexistant, créateur = opérateur, réponses 353/366
- `PRIVMSG` → channel (broadcast) et user (privé)
- `PART`, `QUIT`, changement de `NICK`

### Phase 5 — Commandes opérateur
- `KICK` (op only, erreur 482 sinon)
- `INVITE`
- `TOPIC` (afficher / changer)
- `MODE` avec les flags : `i` `t` `k` `o` `l`

---
 */