
#include <iostream>
#include <cstdlib>



int main(int ac, char **av)
{

	if (ac != 3)
	{
		std::cout << "Usage: ./irc <port> <password>" << std::endl;
		return 1;
	}


	int port = std::atoi(av[1]);
	if (port < 1024 && port > 65535)
	{
		std::cout << "Error of range port : 1024 - 65535 " << std::endl;
		return 1;
	}


	std::string password(av[2]);
	//check si mdp ok etcc ..



	std::cout << " Running ..." << std::endl;

	return 0;
}



/***
 * Phase 0 — Setup (30 min)

 Créer le repo + structure : srcs/, includes/, Makefile												 [OK]
 Makefile : all clean fclean re, flags -Wall -Wextra -Werror -std=c++98, exécutable ircserv			  [OK]
 main.cpp : parser argc/argv, valider port (1024–65535) et password non vide					    [OK]
 Vérifier que ça compile vide et que les mauvais args sont rejetés proprement					     [OK]





Phase 1 — Le tuyau réseau (le plus important)

 Classe Server : socket() → setsockopt(SO_REUSEADDR) → bind() → listen() → fcntl(O_NONBLOCK)		[]
 Boucle poll() minimale avec juste le fd d'écoute													[]
 accept() une connexion, l'ajouter au vecteur de pollfd, logger "client connecté"					[]
 Test : nc localhost <port> → ton serveur doit dire qu'un client s'est co							[]
 Gérer plusieurs nc en même temps (preuve que le multiplexage marche)								[]
 Gérer recv() == 0 → déconnexion propre, close, retrait du tableau									[]

👉 À ce stade, ne passe pas à la suite tant que 3 nc peuvent se connecter/déconnecter sans crash ni leak.





Phase 2 — Buffering + parsing (la fondation invisible)

 Classe Client : fd, inBuffer, outBuffer, état registered
 Sur POLLIN client : recv() → append dans inBuffer
 Extraire les lignes complètes \r\n une par une, garder le reste en buffer
 Parser une ligne en command + params (gérer le : du trailing)
 Dispatcher vers un handler (map ou if/else)
 Test : tape une commande caractère par caractère dans nc → ne doit pas casser






 Phase 3 — Enregistrement (premier vrai client)

 PASS → vérif mot de passe (464 si faux)
 NICK → unicité (433 si pris), validité (432)
 USER → username + realname
 Quand les 3 sont OK → envoyer 001 (+ 002 003 004)
 PING/PONG ← sans ça irssi te coupe
 Test : irssi → /connect localhost <port> <password> doit afficher le welcome







 Phase 4 — Le chat de base

 Classe Channel : nom, membres, opérateurs, topic
 JOIN (crée si inexistant, créateur = op) + réponses 353/366
 PRIVMSG vers un channel (broadcast à tous les membres sauf l'émetteur)
 PRIVMSG vers un user (message privé)
 PART, QUIT, changement de NICK
 Test : 2 irssi dans le même #channel qui se parlent








 Phase 5 — Commandes opérateur

 KICK (op only, 482 sinon)
 INVITE
 TOPIC (afficher / changer)
 MODE avec les 5 flags : i t k o l





 Phase 6 — Robustesse (avant l'éval)

 SIGINT (Ctrl-C) → shutdown propre, tous les fd fermés
 Pas de leak (valgrind / pas de fd zombie)
 Re-tester tous les edge cases : mauvais password, double JOIN, kick d'un non-membre, etc.
 Noter ton client de référence dans le README
 */