# ft_irc — Cheatsheets

Aide-mémoire du serveur. Tout ce qui est listé ici est réellement implémenté et
testé : rien de générique, rien d'aspirationnel.

- [1. Démarrage](#1-démarrage)
- [2. Enregistrement](#2-enregistrement)
- [3. Commandes de base](#3-commandes-de-base)
- [4. Commandes opérateur](#4-commandes-opérateur)
- [5. Modes de salon](#5-modes-de-salon)
- [6. Gérer les grades](#6-gérer-les-grades)
- [7. irssi et nc](#7-irssi-et-nc)
- [8. Codes numériques](#8-codes-numériques)
- [9. Recettes](#9-recettes)
- [10. Dépannage](#10-dépannage)

---

## 1. Démarrage

| Quoi | Commande |
|---|---|
| Compiler | `make` · `make re` · `make fclean` |
| Lancer | `./ircserv <port> <password>` |
| Exemple | `./ircserv 6667 pass` |
| Arrêter | `Ctrl-C` (SIGINT) ou `Ctrl-\` (SIGQUIT) |

**Contraintes** : port entre 1024 et 65535, mot de passe non vide. Sinon code de
sortie `1`.

### Se connecter

| Client | Commande |
|---|---|
| nc | `nc 127.0.0.1 6667` |
| nc (CRLF forcés) | `nc -C 127.0.0.1 6667` |
| irssi | `irssi -c 127.0.0.1 -p 6667 -w pass -n alice` |
| irssi (déjà lancé) | `/connect 127.0.0.1 6667 pass alice` |

Le serveur accepte les fins de ligne `\n` **et** `\r\n`, donc `-C` est optionnel.

### Logs du serveur

| Trace | Sens |
|---|---|
| `[*] ircserv listening on port N` | Démarrage réussi |
| `[+] New connexion` | Un client vient d'être accepté |
| `[-] Client disconnected (nick)` | Départ, pseudo affiché s'il est connu |
| `[!] bind() error: ...` | Port déjà occupé, le serveur s'arrête |
| `[*] Server shutting down...` | Signal reçu, extinction en cours |

---

## 2. Enregistrement

**Ordre obligatoire.** `PASS` doit venir en premier, sinon `451`.

| Commande | Syntaxe | Effet |
|---|---|---|
| `PASS` | `PASS <password>` | Mot de passe du serveur. Faux → `464` puis fermeture |
| `NICK` | `NICK <pseudo>` | Définit le pseudo. Unicité insensible à la casse |
| `USER` | `USER <user> 0 * :<realname>` | Identité. 4 paramètres exigés |

Les trois validés → le serveur envoie `001` `002` `003` `004`, le client est
enregistré.

### Règles du pseudo

| Règle | Détail |
|---|---|
| Longueur | 1 à 9 caractères — au-delà `432` |
| Premier caractère | Une lettre, ou l'un de `[` `]` `\` `_` `^` `{` `}` `\|` `` ` `` |
| Suivants | Lettres, chiffres, les mêmes spéciaux, et `-` |
| Unicité | Insensible à la casse : `Bob` == `bob` → `433` |

### Commandes autorisées avant l'enregistrement

| Commande | Note |
|---|---|
| `PASS` `NICK` `USER` | Les trois de l'enregistrement |
| `PING` | Certains clients vérifient le serveur avant de s'authentifier |
| `QUIT` | On doit toujours pouvoir partir |
| `CAP` | Ignorée silencieusement, aucune extension négociée |

**Toute autre commande avant l'enregistrement → `451`.**

---

## 3. Commandes de base

| Commande | Syntaxe | Effet |
|---|---|---|
| `JOIN` | `JOIN <#salon> [<clé>]` | Rejoint ou crée. Le créateur devient opérateur |
| `PART` | `PART <#salon>{,<#salon>} [:<raison>]` | Quitte un ou plusieurs salons |
| `PRIVMSG` | `PRIVMSG <cible>{,<cible>} :<texte>` | Message à un salon ou à un utilisateur |
| `NOTICE` | `NOTICE <cible>{,<cible>} :<texte>` | Idem, mais **n'émet jamais d'erreur** |
| `QUIT` | `QUIT [:<raison>]` | Déconnexion, annoncée aux salons |
| `PING` | `PING <token>` | Le serveur répond `PONG` |
| `NICK` | `NICK <nouveau>` | Change le pseudo, diffusé à tous les salons |

### Ce qui accepte les listes séparées par virgules

| Commande | Listes ? |
|---|---|
| `PART #a,#b` | ✅ oui |
| `PRIVMSG bob,carol :salut` | ✅ oui |
| `KICK #dev bob,carol` | ✅ sur les pseudos, **un seul salon** |
| `JOIN #a,#b` | ❌ non — un salon à la fois |

### Qui reçoit quoi

| Action | L'émetteur | Les autres membres |
|---|---|---|
| `JOIN #dev` | Reçoit son propre JOIN, puis `353` + `366` | Reçoivent le JOIN |
| `PRIVMSG #dev :x` | **Ne reçoit rien** | Reçoivent le message |
| `PRIVMSG bob :x` | **Ne reçoit rien** | Seul bob reçoit |
| `PART #dev` | Reçoit son propre PART | Reçoivent le PART |
| `QUIT` | Reçoit `ERROR :Closing link` | Reçoivent le QUIT, **une seule fois** |
| `NICK bobby` | Reçoit son propre NICK | Reçoivent le NICK |
| `TOPIC #dev :x` | Reçoit le TOPIC | Reçoivent le TOPIC |
| `KICK #dev bob` | Reçoit le KICK | Reçoivent le KICK, **bob compris** |

Salon détruit dès que son dernier membre part, avec ses modes et sa liste
d'invités.

---

## 4. Commandes opérateur

| Commande | Syntaxe | Opérateur requis ? |
|---|---|---|
| `TOPIC` | `TOPIC <#salon>` | Non — affiche le sujet |
| `TOPIC` | `TOPIC <#salon> :<sujet>` | Seulement si mode `+t` |
| `TOPIC` | `TOPIC <#salon> :` | Efface le sujet |
| `KICK` | `KICK <#salon> <pseudo>{,<pseudo>} [:<raison>]` | **Oui** — sinon `482` |
| `INVITE` | `INVITE <pseudo> <#salon>` | Seulement si mode `+i` |
| `MODE` | `MODE <#salon>` | Non — affiche les modes (`324`) |
| `MODE` | `MODE <#salon> <flags> [args]` | **Oui** — sinon `482` |

**`MODE <#salon>` exige d'être membre** : la réponse `324` révèle la clé du salon.

---

## 5. Modes de salon

| Mode | Argument au `+` | Argument au `-` | Effet |
|---|---|---|---|
| `i` | — | — | Salon sur invitation seulement |
| `t` | — | — | Seuls les opérateurs changent le sujet |
| `k` | la clé | — | Protège l'entrée par une clé |
| `o` | le pseudo | le pseudo | Donne / retire le grade opérateur |
| `l` | le nombre | — | Limite le nombre de membres |

### Exemples

| Commande | Effet |
|---|---|
| `MODE #dev +i` | Invitation obligatoire |
| `MODE #dev -i` | Salon rouvert à tous |
| `MODE #dev +t` | Sujet réservé aux opérateurs |
| `MODE #dev +k secret` | Clé `secret` posée |
| `MODE #dev -k` | Clé retirée — pas d'argument |
| `MODE #dev +l 10` | 10 membres maximum |
| `MODE #dev -l` | Limite retirée — pas d'argument |
| `MODE #dev +o bob` | bob devient opérateur |
| `MODE #dev -o bob` | bob redevient membre simple |
| `MODE #dev +ok-l bob secret` | Enchaîné : `+o bob`, `+k secret`, `-l` |
| `MODE #dev` | Affiche par ex. `+itk secret` |

### Règles du parsing

| Point | Comportement |
|---|---|
| Signe courant | S'applique jusqu'au signe suivant |
| Arguments | Consommés dans l'ordre d'arrivée des flags qui en demandent |
| Flag inconnu | `472`, les autres flags de la ligne sont quand même appliqués |
| Argument manquant | `461`, le flag est ignoré |
| `+l abc` ou `+l 0` | Ignoré silencieusement, rien n'est diffusé |
| Diffusion | Seuls les changements **réellement appliqués** sont annoncés |

---

## 6. Gérer les grades

Il n'y a qu'un seul grade : **opérateur de salon**, marqué `@` devant le pseudo.

| Objectif | Commande | irssi |
|---|---|---|
| Devenir opérateur | Créer le salon : `JOIN #neuf` | `/join #neuf` |
| Promouvoir quelqu'un | `MODE #dev +o bob` | `/mode #dev +o bob` ou `/op bob` |
| Rétrograder quelqu'un | `MODE #dev -o bob` | `/mode #dev -o bob` ou `/deop bob` |
| Promouvoir et poser une clé | `MODE #dev +ok bob secret` | `/mode #dev +ok bob secret` |
| Exclure quelqu'un | `KICK #dev bob :raison` | `/kick bob raison` |
| Exclure plusieurs | `KICK #dev bob,carol :raison` | — |
| Inviter sur un salon `+i` | `INVITE dave #dev` | `/invite dave #dev` |
| Voir qui est opérateur | Rejoindre le salon, lire le `353` | `/join #dev` |

### Ce qu'un opérateur peut faire, un membre non

| Action | Membre | Opérateur |
|---|---|---|
| Parler, `PART`, `TOPIC` en lecture | ✅ | ✅ |
| `TOPIC` en écriture si `+t` | ❌ `482` | ✅ |
| `KICK` | ❌ `482` | ✅ |
| `MODE` en écriture | ❌ `482` | ✅ |
| `INVITE` sur un salon `+i` | ❌ `482` | ✅ |
| `INVITE` sur un salon normal | ✅ | ✅ |

### Points à connaître

| Situation | Comportement |
|---|---|
| Créateur du salon | Devient opérateur automatiquement |
| L'opérateur part | Le salon **n'a plus d'opérateur**, aucune promotion automatique |
| Dernier membre part | Salon détruit ; le prochain à le créer sera opérateur |
| Cible absente du salon | `441` |
| Cible inconnue du serveur | `441` sur `MODE +o`, `401` sur `KICK` |
| Aucun grade serveur | Pas d'`OPER`, pas d'admin global — hors sujet |

---

## 7. irssi et nc

Dans `nc` on tape les commandes **brutes**, sans `/`. irssi les fabrique.

| Objectif | irssi | nc |
|---|---|---|
| Se connecter | `/connect 127.0.0.1 6667 pass` | `nc 127.0.0.1 6667` puis `PASS pass` |
| Pseudo | `/nick alice` | `NICK alice` |
| Identité | *(automatique)* | `USER alice 0 * :Alice` |
| Rejoindre | `/join #dev` | `JOIN #dev` |
| Rejoindre avec clé | `/join #dev secret` | `JOIN #dev secret` |
| Parler au salon | *(taper le texte)* | `PRIVMSG #dev :salut` |
| Message privé | `/msg bob salut` | `PRIVMSG bob :salut` |
| Notice | `/notice #dev avis` | `NOTICE #dev :avis` |
| Voir le sujet | `/topic` | `TOPIC #dev` |
| Changer le sujet | `/topic Nouveau` | `TOPIC #dev :Nouveau` |
| Quitter le salon | `/part` | `PART #dev` |
| Exclure | `/kick bob raison` | `KICK #dev bob :raison` |
| Inviter | `/invite dave` | `INVITE dave #dev` |
| Promouvoir | `/op bob` | `MODE #dev +o bob` |
| Rétrograder | `/deop bob` | `MODE #dev -o bob` |
| Voir les modes | `/mode #dev` | `MODE #dev` |
| Se déconnecter | `/quit :bye` | `QUIT :bye` |
| Envoyer du brut | `/quote <ligne>` | *(c'est déjà du brut)* |

### Non implémentées — renvoient `421`

`NAMES` · `WHOIS` · `LIST` · `WHO` · `AWAY` · `OPER` · `MOTD`

irssi les propose, le sujet ne les exige pas. Ne pas les taper par réflexe.

### Navigation irssi

| Action | Touche |
|---|---|
| Fenêtre 1 à 10 | `Alt+1` … `Alt+0` |
| Suivante / précédente | `Ctrl+N` / `Ctrl+P` |
| Où ça bouge | `Alt+A` |
| Par commande | `/window 2` |
| Fermer la fenêtre | `/window close` |
| Défiler | `PgUp` / `PgDn` |
| Quitter irssi | `/quit` — **pas `Ctrl+C`** |

### Réglages irssi à faire une fois

| Commande | Effet |
|---|---|
| `/set autocreate_own_query ON` | Ouvre une fenêtre quand **tu** envoies un privé |
| `/set autocreate_query_level MSGS` | Ouvre une fenêtre quand **tu reçois** un privé |
| `/rawlog open /tmp/raw.log` | Journalise tout le protocole brut |
| `/save` | Rend les réglages permanents |

---

## 8. Codes numériques

Format : `:ircserv <code> <destinataire> [params] :<texte>`

### Bienvenue — envoyés une fois l'enregistrement terminé

| Code | Nom | Texte |
|---|---|---|
| `001` | RPL_WELCOME | `Welcome to the Internet Relay Network <nick>!<user>@localhost` |
| `002` | RPL_YOURHOST | `Your host is ircserv, running version 1.0` |
| `003` | RPL_CREATED | `This server was created recently` |
| `004` | RPL_MYINFO | `ircserv 1.0 o itkol` — pas de trailing |

### Réponses de commande

| Code | Nom | Quand |
|---|---|---|
| `324` | RPL_CHANNELMODEIS | `MODE <#salon>` sans flag |
| `331` | RPL_NOTOPIC | `TOPIC` alors qu'aucun sujet n'est défini |
| `332` | RPL_TOPIC | `TOPIC` en lecture, ou à l'entrée dans le salon |
| `341` | RPL_INVITING | Confirmation à celui qui invite — pas de trailing |
| `353` | RPL_NAMREPLY | Liste des membres après un `JOIN`, `@` devant les opérateurs |
| `366` | RPL_ENDOFNAMES | Fin de la liste |

### Erreurs — cibles et paramètres

| Code | Nom | Quand | Émis par |
|---|---|---|---|
| `401` | ERR_NOSUCHNICK | Pseudo inconnu | `PRIVMSG` `INVITE` |
| `403` | ERR_NOSUCHCHANNEL | Salon inexistant, ou nom sans `#` | `JOIN` `PART` `PRIVMSG` `TOPIC` `KICK` `MODE` |
| `404` | ERR_CANNOTSENDTOCHAN | Écrire dans un salon non rejoint | `PRIVMSG` |
| `409` | ERR_NOORIGIN | `PING` sans argument | `PING` |
| `411` | ERR_NORECIPIENT | `PRIVMSG` sans cible | `PRIVMSG` |
| `412` | ERR_NOTEXTTOSEND | `PRIVMSG` sans texte | `PRIVMSG` |
| `421` | ERR_UNKNOWNCOMMAND | Commande non gérée | dispatcher |
| `461` | ERR_NEEDMOREPARAMS | Paramètres insuffisants | `PASS` `USER` `JOIN` `PART` `TOPIC` `KICK` `INVITE` `MODE` |

### Erreurs — pseudo et enregistrement

| Code | Nom | Quand |
|---|---|---|
| `431` | ERR_NONICKNAMEGIVEN | `NICK` sans argument |
| `432` | ERR_ERRONEUSNICKNAME | Caractères invalides, ou plus de 9 caractères |
| `433` | ERR_NICKNAMEINUSE | Pseudo déjà pris, casse ignorée |
| `451` | ERR_NOTREGISTERED | Commande envoyée avant d'être enregistré |
| `462` | ERR_ALREADYREGISTRED | `PASS` ou `USER` après enregistrement |
| `464` | ERR_PASSWDMISMATCH | Mot de passe faux — la connexion se ferme ensuite |

### Erreurs — salons et droits

| Code | Nom | Quand |
|---|---|---|
| `441` | ERR_USERNOTINCHANNEL | La cible n'est pas dans le salon (`KICK`, `MODE +o`) |
| `442` | ERR_NOTONCHANNEL | **Toi** tu n'es pas dans le salon |
| `443` | ERR_USERONCHANNEL | `INVITE` sur quelqu'un déjà membre |
| `471` | ERR_CHANNELISFULL | `JOIN` refusé par `+l` |
| `472` | ERR_UNKNOWNMODE | Flag de mode inconnu |
| `473` | ERR_INVITEONLYCHAN | `JOIN` refusé par `+i` |
| `475` | ERR_BADCHANNELKEY | `JOIN` refusé par `+k`, clé absente ou fausse |
| `482` | ERR_CHANOPRIVSNEEDED | Action réservée aux opérateurs |

### Messages non numériques

| Message | Quand |
|---|---|
| `PONG` | Réponse à `PING` |
| `ERROR :Closing link (bad password)` | Après un `464` |
| `ERROR :Closing link (Quit: ...)` | Après un `QUIT` |
| `ERROR :Server shutting down` | À l'arrêt du serveur |

---

## 9. Recettes

### Session complète en nc

```
nc 127.0.0.1 6667
PASS pass
NICK bob
USER bob 0 * :Bob
JOIN #dev
PRIVMSG #dev :salut tout le monde
PART #dev :j'y retourne
QUIT :au revoir
```

### Verrouiller un salon

```
JOIN #prive                    → tu es opérateur
MODE #prive +i                 → invitation obligatoire
MODE #prive +k secret          → clé posée
MODE #prive +l 5               → 5 personnes maximum
MODE #prive +t                 → sujet réservé aux opérateurs
INVITE dave #prive             → dave peut entrer
```

### Monter une équipe de modération

```
MODE #dev +o bob               → bob promu
MODE #dev +o carol             → carol promue
MODE #dev -o bob               → bob rétrogradé
KICK #dev spammeur :spam       → exclusion
```

### Provoquer chaque erreur — utile en démo

```
PASS                           → 461
BLAHBLAH                       → 421
PRIVMSG                        → 411
PRIVMSG bob                    → 412
PRIVMSG fantome :x             → 401
JOIN salon-sans-diese          → 403
MODE #dev +z                   → 472
KICK #dev absent               → 441
```

### Tester la robustesse

| Test | Commande |
|---|---|
| Commande fragmentée | `printf 'PASS pa' ; sleep 1 ; printf 'ss\r\n'` |
| Ligne de 512 octets | `python3 -c "import socket;s=socket.create_connection(('127.0.0.1',6667));s.sendall(b'NICK '+b'z'*505+b'\r\n')"` |
| Client suspendu | `Ctrl-Z` dans irssi, inonder le salon, puis `fg` |
| Coupure brutale | `kill -9 <pid du client>` |
| Fuites | `valgrind --leak-check=full ./ircserv 6667 pass` |

---

## 10. Dépannage

| Symptôme | Cause | Solution |
|---|---|---|
| Le serveur ne démarre pas, `bind() error` | Port déjà occupé | Attendre, ou changer de port |
| Tout renvoie `451` | `PASS` non envoyé, ou pas en premier | Recommencer par `PASS` |
| `nc` ne répond pas | Enregistrement incomplet | Envoyer `PASS`, `NICK` **et** `USER` |
| Message privé invisible dans irssi | Pas de fenêtre auto | `/set autocreate_query_level MSGS` |
| `433` dès le second client | Même pseudo | Lancer irssi avec `-n <autre pseudo>` |
| irssi se reconnecte en boucle | Mot de passe refusé | `/disconnect` |
| `404` en écrivant dans un salon | Salon non rejoint | `JOIN` d'abord |
| `482` sur `KICK` ou `MODE` | Tu n'es pas opérateur | Se faire promouvoir, ou créer son salon |
| Le texte est tronqué après le premier mot | `:` manquant | `PRIVMSG #dev :mon texte` |
| `/names` ou `/whois` → `421` | Non implémentées | Normal, hors sujet |

### Limites connues

| Limite | Détail |
|---|---|
| `JOIN #a,#b` | Listes non gérées, un salon à la fois |
| Salons `&` | Seuls les `#` sont acceptés |
| *realname* | Reçu dans `USER`, non conservé |
| Modes utilisateur | `MODE <pseudo>` ignoré |
| Bonus | Ni transfert de fichiers, ni bot |
