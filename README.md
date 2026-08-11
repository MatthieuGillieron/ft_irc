# ft_irc

Serveur IRC écrit en C++98, conforme au RFC 1459. Un seul processus, aucun thread,
aucun `fork` : tous les clients sont multiplexés par un unique `poll()` qui couvre
la lecture, l'écriture et l'acceptation des connexions.

---

## Compilation

```bash
make          # produit ./ircserv
make clean    # supprime les objets
make fclean   # supprime aussi le binaire
make re
```

Compilé avec `c++ -Wall -Wextra -Werror -std=c++98`.

## Lancement

```bash
./ircserv <port> <password>
```

- `port` — port d'écoute, entre 1024 et 65535
- `password` — mot de passe que chaque client doit fournir via `PASS`

```bash
./ircserv 6667 monmotdepasse
```

Le serveur s'arrête proprement sur `Ctrl-C` (SIGINT) ou SIGQUIT : les clients
reçoivent `ERROR :Server shutting down` avant la fermeture des descripteurs.

## Client de référence

**irssi** — c'est avec lui que le serveur a été développé et testé.

```bash
irssi -c 127.0.0.1 -p 6667 -w monmotdepasse
```

Pour déboguer à la main, `nc` fonctionne aussi, mais attention : le protocole
exige `\r\n` en fin de ligne, or netcat n'envoie que `\n`.

```bash
printf 'PASS monmotdepasse\r\nNICK bob\r\nUSER bob 0 * :Bob\r\nJOIN #test\r\n' \
  | nc -c 127.0.0.1 6667
```

---

## Enregistrement

Un client doit fournir les trois informations, **`PASS` en premier**, avant de
pouvoir faire quoi que ce soit d'autre :

```
PASS <password>
NICK <pseudo>
USER <username> 0 * :<realname>
```

Les trois validées, le serveur envoie les réponses `001` à `004`. Un mot de passe
incorrect renvoie `464` puis ferme la connexion.

Les pseudos font au plus 9 caractères, commencent par une lettre ou l'un de
`` []\_^{}|` ``, et sont uniques sans distinction de casse : `Bob` et `bob` sont
le même pseudo.

## Commandes

| Commande | Forme | Description |
|---|---|---|
| `PASS` | `PASS <password>` | Mot de passe du serveur, obligatoire en premier |
| `NICK` | `NICK <pseudo>` | Choisit ou change le pseudo, diffusé aux salons |
| `USER` | `USER <user> 0 * :<realname>` | Identité du client |
| `PING` | `PING <token>` | Le serveur répond `PONG` |
| `QUIT` | `QUIT [:<raison>]` | Déconnexion, annoncée aux salons |
| `JOIN` | `JOIN <salon> [<clé>]` | Rejoint un salon, le crée s'il n'existe pas |
| `PART` | `PART <salon>{,<salon>} [:<raison>]` | Quitte un ou plusieurs salons |
| `PRIVMSG` | `PRIVMSG <cible>{,<cible>} :<texte>` | Message vers un salon ou un utilisateur |
| `NOTICE` | `NOTICE <cible>{,<cible>} :<texte>` | Idem, sans aucune réponse d'erreur |
| `TOPIC` | `TOPIC <salon> [:<sujet>]` | Consulte ou change le sujet |
| `KICK` | `KICK <salon> <pseudo>{,<pseudo>} [:<raison>]` | Exclut un membre — opérateur |
| `INVITE` | `INVITE <pseudo> <salon>` | Invite quelqu'un dans un salon |
| `MODE` | `MODE <salon> [<flags> [<args>]]` | Consulte ou change les modes |

Le créateur d'un salon en devient opérateur. Un salon vidé de tous ses membres
est détruit, avec ses modes et sa liste d'invitations.

## Modes de salon

| Mode | Argument | Effet |
|---|---|---|
| `+i` / `-i` | — | Salon sur invitation uniquement |
| `+t` / `-t` | — | Seuls les opérateurs changent le sujet |
| `+k` / `-k` | clé au `+` | Protège le salon par une clé |
| `+o` / `-o` | pseudo | Donne ou retire le statut d'opérateur |
| `+l` / `-l` | nombre au `+` | Limite le nombre de membres |

Les flags s'enchaînent avec leurs arguments dans l'ordre d'arrivée :

```
MODE #dev +ok-l bob secret
```

donne le statut d'opérateur à bob, pose la clé `secret`, et retire la limite.
`MODE <salon>` sans flag affiche les modes courants (réponse `324`), réservée
aux membres du salon puisqu'elle révèle la clé.

---

## Architecture

```
main.cpp            arguments, signaux, lancement
Server.cpp          boucle poll(), socket d'écoute, arrêt
ServerClient.cpp    accept, recv, flush, déconnexion
ServerDispatch.cpp  aiguillage des commandes, recherches
Message.cpp         découpage d'une ligne en commande + paramètres
Channel.cpp         membres, opérateurs, invités, modes, diffusion
commands/           un fichier par famille de commandes
```

Chaque client possède deux buffers. `_inBuffer` accumule les octets reçus
jusqu'à trouver un `\r\n` — une commande peut donc arriver en plusieurs
morceaux, ou plusieurs commandes dans un seul paquet. `_outBuffer` accumule
les réponses, envoyées seulement quand `poll()` signale `POLLOUT`. Aucune
lecture ni écriture n'a lieu en dehors de `poll()`.

## Limites connues

- `JOIN` ne gère pas encore les listes séparées par des virgules
- Les salons en `&` ne sont pas acceptés, seulement ceux en `#`
- Le *realname* de `USER` est reçu mais pas conservé
- Aucun mode utilisateur : `MODE <pseudo>` est ignoré
