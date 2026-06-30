# ft_irc — Documentation complète

> Construire un serveur IRC en C++98 capable de gérer plusieurs clients en simultané, sans fork, avec **un seul** `poll()` pour tout.

---

## 1. C'est quoi, au juste

IRC (Internet Relay Chat) est un protocole de chat **texte**, basé sur **TCP**, qui date de 1988. Le principe :

- Un **serveur** central écoute sur un port.
- Des **clients** s'y connectent, s'authentifient, choisissent un pseudo, rejoignent des **channels** (salons), et s'envoient des messages.
- Tout passe par des **messages texte terminés par `\r\n`** (CRLF).

Ton job : coder le **serveur**. Tu ne codes PAS de client — tu utilises un vrai client IRC existant (irssi, HexChat, WeeChat, ou `nc` pour débugger) pour parler à ton serveur. C'est ça qui rend le projet vivant : à l'éval, l'examinateur ouvre un vrai client et discute avec ton serveur.

**Les deux RFC de référence :**
- **RFC 1459** — le protocole IRC original (le plus pertinent pour 42).
- **RFC 2812** — version mise à jour, plus précise sur les réponses numériques.

Tu n'as pas à tout implémenter. Le sujet définit un sous-ensemble. Mais quand tu as un doute sur le format exact d'une réponse, la RFC tranche.

---

## 2. Les règles du projet (le cadre 42)

### Contraintes de base
- **C++98 strict** : `c++ -Wall -Wextra -Werror -std=c++98`
- **Makefile** avec les règles : `all`, `clean`, `fclean`, `re` (et pas de relink inutile)
- **Exécutable** : `ircserv`
- **Lancement** : `./ircserv <port> <password>`
  - `port` : le port d'écoute
  - `password` : mot de passe que les clients doivent fournir pour se connecter

### Ce qui est INTERDIT
- **Aucun fork.** Tout en mono-process, mono-thread (le multi-client se fait via le multiplexage I/O, pas via des process).
- Pas de blocage : **tous les fd doivent être non-bloquants**.
- Pas de busy-wait / lecture-écriture en dehors du `poll()`.

### La règle qui peut te coûter 0 à l'éval
> Tu dois utiliser **un seul `poll()`** (ou équivalent : `select()`, `epoll()`, `kqueue()`) pour **toutes** les opérations I/O : lire, écrire, **et** écouter les nouvelles connexions.

Traduction concrète : tu ne fais **jamais** un `recv()` ou un `send()` "à l'aveugle". Tu attends que `poll()` te dise que le fd est prêt (`POLLIN` pour lire, `POLLOUT` pour écrire). Si l'examinateur voit un `read`/`write`/`recv`/`send` qui n'est pas gardé par `poll()`, c'est sanctionné.

### Le piège `fcntl` sur macOS
Sur macOS, le seul moyen de rendre un fd non-bloquant autorisé par le sujet est :

```cpp
fcntl(fd, F_SETFL, O_NONBLOCK);
```

**Aucun autre flag, aucun autre usage de `fcntl` n'est autorisé.** Sur Linux tu pourrais passer `O_NONBLOCK` directement à `accept4`, mais reste sur la forme ci-dessus pour être portable et conforme.

### Fonctions autorisées
`socket`, `close`, `setsockopt`, `getsockname`, `getprotobyname`, `gethostbyname`, `getaddrinfo`, `freeaddrinfo`, `bind`, `connect`, `listen`, `accept`, `htons`, `htonl`, `ntohs`, `ntohl`, `inet_addr`, `inet_ntoa`, `send`, `recv`, `signal`, `sigaction`, `lseek`, `fstat`, `fcntl`, `poll` (ou son équivalent).

### Le client de référence
Tu **choisis un client de référence** (irssi est le choix classique) et tu t'assures que ton serveur fonctionne parfaitement avec lui. À l'éval, c'est avec **ce client-là** que l'examinateur testera. Note bien lequel dans ton README.

---

## 3. Les fonctionnalités à livrer (partie obligatoire)

Le serveur doit, au minimum :

1. **Authentifier** un client avec le mot de passe.
2. Permettre de **définir un nickname et un username**.
3. Permettre de **rejoindre un channel**.
4. Envoyer et recevoir des **messages privés** (PRIVMSG).
5. **Diffuser** les messages d'un channel à tous ses membres.
6. Gérer deux types d'utilisateurs : **opérateurs** et **utilisateurs normaux**.
7. Implémenter les **commandes opérateur** :
   - `KICK` — éjecter un client d'un channel
   - `INVITE` — inviter un client dans un channel
   - `TOPIC` — voir / changer le sujet du channel
   - `MODE` — changer les modes du channel (voir §7)

---

## 4. Architecture réseau : les sockets

C'est le socle. Voici la séquence côté serveur, dans l'ordre.

```
socket()      → crée le fd d'écoute (point d'entrée TCP)
setsockopt()  → SO_REUSEADDR (relancer le serveur sans attendre le TIME_WAIT)
bind()        → associe le socket au port/adresse
listen()      → passe le socket en mode écoute (file d'attente des connexions)
fcntl()       → rend le socket non-bloquant
... puis dans la boucle ...
accept()      → accepte une nouvelle connexion → nouveau fd client
```

```cpp
int server_fd = socket(AF_INET, SOCK_STREAM, 0);

int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = INADDR_ANY;    // écoute sur toutes les interfaces
addr.sin_port = htons(port);          // /!\ htons : byte order réseau

bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
listen(server_fd, SOMAXCONN);
fcntl(server_fd, F_SETFL, O_NONBLOCK);
```

**Concepts clés à maîtriser pour l'oral :**
- **`htons` / `htonl`** : le réseau utilise le *big-endian* (network byte order). Ta machine est probablement *little-endian*. Ces fonctions convertissent. Oublie-les et ton serveur écoute sur le mauvais port.
- **`SO_REUSEADDR`** : sans ça, après un crash tu dois attendre ~1 min (TIME_WAIT) avant de réutiliser le port.
- **Le fd d'écoute n'est pas un fd de communication.** `accept()` te rend un **nouveau** fd, dédié à ce client. Le fd d'écoute, lui, ne sert qu'à accepter.

---

## 5. La boucle d'événements : le cœur du projet

Tout le projet tourne autour d'**un seul `poll()`**. Tu maintiens un tableau de `struct pollfd`, un par fd actif (le fd d'écoute + un par client). À chaque tour de boucle, `poll()` te dit quels fds sont prêts.

```
struct pollfd {
    int   fd;        // le file descriptor à surveiller
    short events;    // ce que JE veux surveiller (POLLIN | POLLOUT)
    short revents;   // ce qui s'est RÉELLEMENT passé (rempli par poll)
};
```

### Schéma de la boucle

```
┌─────────────────────────────────────────────────────────┐
│  poll(&fds[0], nfds, -1)   ← bloque jusqu'à un événement  │
└─────────────────────────────────────────────────────────┘
                          │
          ┌───────────────┼────────────────┐
          ▼               ▼                 ▼
   fd d'écoute      fd client           fd client
   POLLIN ?         POLLIN ?            POLLOUT ?
      │                │                    │
      ▼                ▼                    ▼
   accept()         recv()              send()
   → nouveau fd     → bufferise         → flush le
   → ajoute aux     → extrait les         buffer de
     fds              lignes \r\n          sortie
                    → parse + exécute
                          │
                  ┌───────┴────────┐
                  ▼                ▼
            recv() == 0       recv() > 0
            → déconnexion     → traite les commandes
            → close + retire
              des fds
```

### Squelette

```cpp
std::vector<struct pollfd> fds;
// fds[0] = le socket d'écoute, events = POLLIN

while (running)
{
    int ready = poll(fds.data(), fds.size(), -1);   // -1 = bloque indéfiniment
    if (ready < 0) { /* gérer EINTR / signal */ continue; }

    for (size_t i = 0; i < fds.size(); ++i)
    {
        if (fds[i].revents & POLLIN)
        {
            if (fds[i].fd == server_fd)
                acceptNewClient();      // nouvelle connexion
            else
                receiveFromClient(fds[i].fd);  // données entrantes
        }
        if (fds[i].revents & POLLOUT)
            flushClientOutput(fds[i].fd);  // données à envoyer
    }
}
```

### Pourquoi `POLLOUT` ?
Quand tu veux envoyer un message à un client, tu ne fais pas `send()` direct. Le socket peut être plein (le client lit lentement). Tu mets le message dans un **buffer de sortie** propre à ce client, et tu actives `POLLOUT` dans ses `events`. Quand `poll()` te dit `POLLOUT`, tu fais `send()`. Quand le buffer est vidé, tu désactives `POLLOUT`.

> En pratique, beaucoup d'implémentations 42 envoient directement avec un `send()` gardé par le test que le fd est prêt. C'est tolérable pour le mandatory si les messages restent courts, mais la version "buffer + POLLOUT" est la propre et celle qui résiste aux questions de l'examinateur. Choisis et assume.

---

## 6. Le protocole IRC : format des messages

C'est LA partie que tout le monde sous-estime. Un message IRC, c'est une ligne de texte structurée, terminée par `\r\n`.

### Grammaire (simplifiée)

```
[ ":" <prefix> SPACE ] <command> [ <params> ] "\r\n"
```

- **`prefix`** (optionnel, commence par `:`) — l'expéditeur. Le **client n'en envoie jamais** ; c'est le **serveur** qui en ajoute un dans ses réponses pour dire "ce message vient de tel utilisateur".
- **`command`** — soit un mot (`NICK`, `JOIN`, `PRIVMSG`), soit un code numérique à 3 chiffres (`001`, `433`).
- **`params`** — jusqu'à 15 paramètres séparés par des espaces. Le **dernier** peut être un *trailing* : il commence par `:` et **peut contenir des espaces** (utile pour les messages).

### Exemples client → serveur

```
PASS motdepasse\r\n
NICK magillie\r\n
USER mag 0 * :Matthieu Gillieron\r\n
JOIN #42lausanne\r\n
PRIVMSG #42lausanne :Salut tout le monde !\r\n
PRIVMSG bob :message privé direct\r\n
```

### Exemples serveur → client (avec prefix)

```
:magillie!mag@localhost JOIN #42lausanne\r\n
:magillie!mag@localhost PRIVMSG #42lausanne :Salut tout le monde !\r\n
:irc.42.fr 001 magillie :Welcome to the IRC network\r\n
```

Le prefix `magillie!mag@localhost` suit le format `nick!user@host`. C'est comme ça que le client sait qui parle.

### Le parsing, étape par étape
1. Tu reçois des octets via `recv()` et tu les **accumules** dans le buffer du client.
2. Tu cherches `\r\n` dans le buffer. Tant qu'il y en a un, tu **extrais une ligne complète** et tu la traites.
3. Pour chaque ligne : tu sépares `command` et `params` (en gérant le `:` du trailing).
4. Tu dispatches vers le bon handler (`NICK`, `JOIN`, etc.).

> **Sois tolérant sur la fin de ligne.** Certains clients/`nc` envoient `\n` seul au lieu de `\r\n`. Accepte les deux pour ne pas galérer en test.

---

## 7. Les commandes à implémenter (détail)

### Phase d'enregistrement (registration)
Avant qu'un client soit "enregistré" et puisse tout faire, il doit fournir, dans l'ordre logique :

| Commande | Rôle | Erreur si manquant/faux |
|---|---|---|
| `PASS <password>` | Mot de passe du serveur | `464 ERR_PASSWDMISMATCH` |
| `NICK <nickname>` | Pseudo unique | `431`, `432`, `433` |
| `USER <user> <mode> <unused> :<realname>` | Identité | `461 ERR_NEEDMOREPARAMS` |

Tant que `PASS` + `NICK` + `USER` ne sont pas tous validés, le client n'est **pas enregistré** → la plupart des autres commandes répondent `451 ERR_NOTREGISTERED`. Une fois les trois OK, tu envoies la séquence de bienvenue (`001`, `002`, `003`, `004`).

### Commandes principales

| Commande | Syntaxe | Effet |
|---|---|---|
| `NICK` | `NICK <nick>` | Change/définit le pseudo. Vérifie l'unicité. |
| `USER` | `USER <u> 0 * :<realname>` | Définit username + realname. |
| `JOIN` | `JOIN <#channel> [key]` | Rejoint (ou crée) un channel. Le créateur devient opérateur. |
| `PART` | `PART <#channel> [:raison]` | Quitte un channel. |
| `PRIVMSG` | `PRIVMSG <cible> :<texte>` | Message à un user ou un channel. |
| `NOTICE` | `NOTICE <cible> :<texte>` | Comme PRIVMSG mais ne génère **jamais** de réponse d'erreur auto. |
| `QUIT` | `QUIT [:raison]` | Déconnexion propre. |
| `PING` / `PONG` | `PING <token>` | Keep-alive. Le serveur doit répondre `PONG`. **À ne pas oublier** : irssi envoie des PING, si tu ne réponds pas il coupe. |

### Commandes opérateur (obligatoires)

| Commande | Syntaxe | Effet |
|---|---|---|
| `KICK` | `KICK <#chan> <user> [:raison]` | Éjecte un user du channel. Op only. |
| `INVITE` | `INVITE <user> <#chan>` | Invite un user (nécessaire si le channel est en mode `+i`). |
| `TOPIC` | `TOPIC <#chan> [:sujet]` | Sans argument → affiche le topic. Avec → le change (selon mode `+t`). |
| `MODE` | `MODE <#chan> <flags> [args]` | Change les modes du channel (voir ci-dessous). |

### Les modes de channel (via `MODE`)
Tu dois gérer **ces cinq modes**, activables avec `+` et désactivables avec `-` :

| Mode | Activé par | Effet |
|---|---|---|
| `i` | `MODE #chan +i` | **Invite-only** : on ne peut JOIN que si invité. |
| `t` | `MODE #chan +t` | **Topic restreint** : seuls les ops peuvent changer le topic. |
| `k` | `MODE #chan +k <clé>` | **Clé** (mot de passe) du channel. JOIN requiert la clé. |
| `o` | `MODE #chan +o <user>` | Donne/retire le **statut opérateur** à un user. |
| `l` | `MODE #chan +l <n>` | **Limite** d'utilisateurs dans le channel. |

Exemple : `MODE #42lausanne +itk supersecret` → invite-only + topic restreint + clé "supersecret".

---

## 8. Les réponses numériques essentielles

Le serveur répond souvent par des **codes à 3 chiffres**. Format :
`:<serveur> <code> <nick> <params> [:message]\r\n`

**Réponses de succès (RPL_) :**

| Code | Nom | Quand |
|---|---|---|
| 001 | RPL_WELCOME | Fin de l'enregistrement |
| 332 | RPL_TOPIC | Topic d'un channel |
| 331 | RPL_NOTOPIC | Channel sans topic |
| 353 | RPL_NAMREPLY | Liste des users d'un channel (après JOIN) |
| 366 | RPL_ENDOFNAMES | Fin de la liste |
| 324 | RPL_CHANNELMODEIS | Modes actuels du channel |
| 341 | RPL_INVITING | Confirmation d'invitation |

**Erreurs (ERR_) — les plus fréquentes à l'éval :**

| Code | Nom | Quand |
|---|---|---|
| 401 | ERR_NOSUCHNICK | Pseudo/cible inconnu |
| 403 | ERR_NOSUCHCHANNEL | Channel inexistant |
| 433 | ERR_NICKNAMEINUSE | Pseudo déjà pris |
| 451 | ERR_NOTREGISTERED | Pas encore enregistré |
| 461 | ERR_NEEDMOREPARAMS | Pas assez d'arguments |
| 462 | ERR_ALREADYREGISTERED | Déjà enregistré (re-PASS/USER) |
| 464 | ERR_PASSWDMISMATCH | Mauvais mot de passe |
| 471 | ERR_CHANNELISFULL | Channel plein (mode `l`) |
| 473 | ERR_INVITEONLYCHAN | Channel en `+i`, pas invité |
| 475 | ERR_BADCHANNELKEY | Mauvaise clé (`+k`) |
| 482 | ERR_CHANOPRIVSNEEDED | Pas opérateur, action refusée |

> Tu n'as pas besoin de TOUS les implémenter parfaitement, mais les plus courants (433, 461, 464, 482, 473, 475) seront testés. Récupère les chaînes exactes dans la RFC 2812.

---

## 9. Le piège #1 : les données partielles

**TCP est un flux d'octets, pas un flux de messages.** C'est l'erreur qui plante 80% des implémentations naïves.

Un seul `recv()` peut te rendre :
- une demi-commande (`PRIVMSG #ch`)
- une commande et demie (`NICK bob\r\nPRIVMSG #ch :sa`)
- exactement une commande
- trois commandes d'un coup

**La règle :** chaque client a son **propre buffer de réception**. Tu y ajoutes ce que `recv()` rend, puis tu n'extrais et ne traites que les lignes **complètes** terminées par `\r\n`. Ce qui reste (commande incomplète) attend le prochain `recv()`.

```cpp
void Server::receiveFromClient(int fd)
{
    char buf[512];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);

    if (n <= 0) { disconnectClient(fd); return; }   // 0 = déconnexion

    Client& c = clients[fd];
    c.inBuffer.append(buf, n);

    size_t pos;
    while ((pos = c.inBuffer.find("\r\n")) != std::string::npos)
    {
        std::string line = c.inBuffer.substr(0, pos);
        c.inBuffer.erase(0, pos + 2);   // +2 pour retirer \r\n
        executeCommand(c, line);        // ligne complète garantie
    }
    // ce qui reste dans inBuffer est une commande partielle → on attend
}
```

> Pour tester ce comportement : envoie une commande caractère par caractère avec `nc` (en tapant lentement), ou coupe une commande en deux paquets. Si ton serveur traite ça correctement, tu es blindé.

---

## 10. Architecture en classes (proposition)

Pas imposé, mais une structure propre rend l'oral facile et le code maintenable.

```
Server
 ├── _serverFd            (socket d'écoute)
 ├── _password
 ├── _pollFds             (std::vector<pollfd>)
 ├── _clients             (std::map<int, Client>)   ← clé = fd
 ├── _channels            (std::map<std::string, Channel>)
 ├── run()                (la boucle poll)
 ├── acceptNewClient()
 ├── receiveFromClient(fd)
 └── executeCommand(...)

Client
 ├── _fd
 ├── _nickname, _username, _hostname, _realname
 ├── _inBuffer, _outBuffer
 ├── _registered          (PASS+NICK+USER OK ?)
 ├── _passOk
 └── _channels            (channels rejoints)

Channel
 ├── _name, _topic, _key
 ├── _members             (set ou map de Client*)
 ├── _operators
 ├── _invited
 ├── _userLimit
 └── _modes               (i, t, k, l actifs ?)
```

**Pour le dispatch des commandes**, une `std::map<std::string, void (Server::*)(Client&, params)>` (pointeurs sur méthodes) est élégante, mais un gros `if/else if` reste parfaitement valide en C++98 et plus simple à débugger.

---

## 11. Plan d'attaque (ordre de dev recommandé)

Ne code pas tout en bloc. Va incrémental, teste à chaque étape avec `nc` puis irssi.

1. **Parsing des arguments** (`port`, `password`) + Makefile + structure de classes vides.
2. **Socket + bind + listen + accept** : accepter une connexion, l'afficher dans les logs. Teste avec `nc localhost <port>`.
3. **La boucle `poll()`** : gérer plusieurs connexions, lire les octets, les logger.
4. **Buffering + parsing** des lignes `\r\n` → §9. C'est ta fondation, soigne-la.
5. **Enregistrement** : `PASS`, `NICK`, `USER` + réponse `001`. À partir de là, irssi se connecte vraiment.
6. **`PING`/`PONG`** : sinon irssi te coupe au bout de quelques secondes.
7. **`JOIN`** + `PRIVMSG` vers un channel → le chat de base fonctionne.
8. **`PRIVMSG`** vers un user (messages privés).
9. **`PART`, `QUIT`, `NICK`** (changement), gestion propre des déconnexions.
10. **Commandes opérateur** : `KICK`, `INVITE`, `TOPIC`, `MODE` avec les 5 modes.
11. **Robustesse** : signaux (`SIGINT` propre), données partielles, edge cases, pas de leaks.

---

## 12. Tests & éval

### Tester sans client lourd
```bash
# Connexion brute
nc -C localhost 6667        # -C force l'envoi de \r\n

# Puis tape :
PASS motdepasse
NICK testuser
USER test 0 * :Test User
JOIN #test
PRIVMSG #test :hello
```

### Tester avec un vrai client (irssi)
```bash
irssi
/connect localhost 6667 motdepasse
/join #test
/msg #test salut
```

### Ce que l'examinateur vérifiera
- Compilation `-Wall -Wextra -Werror -std=c++98`, sans relink.
- **Aucun fork**, tout est non-bloquant, **un seul poll**.
- Plusieurs clients en simultané, messages routés vers les bons channels.
- Authentification par mot de passe (bon ET mauvais).
- Nickname/username, JOIN, PRIVMSG (channel + privé).
- Op vs non-op : un non-op se fait refuser `KICK`/`MODE`/etc. (`482`).
- Les 5 modes (`i`, `t`, `k`, `o`, `l`) testés un par un.
- **Test des données partielles** : ils couperont une commande en plusieurs `recv()`. Ton serveur ne doit pas casser.
- Pas de crash, pas de fd qui fuit, gestion propre du Ctrl-C.

---

## 13. Pièges classiques (qui coûtent des points)

- **Oublier `PING`/`PONG`** → le client se déconnecte tout seul, ça donne l'impression que ton serveur bug.
- **Lire/écrire hors du `poll()`** → 0 direct si repéré.
- **Ne pas bufferiser** → casse au moindre message coupé.
- **Ne pas gérer `recv() == 0`** (déconnexion brutale) → fd zombie, voire boucle infinie.
- **Mauvais byte order** (`htons` oublié) → le serveur écoute sur le mauvais port.
- **Modifier le tableau de `pollfds` pendant l'itération** sans précaution → comportement indéfini. Marque les fds à supprimer et nettoie après la boucle.
- **Réponses numériques mal formatées** → certains clients ignorent les réponses non conformes. Respecte `:<serveur> <code> <nick> ...`.
- **Pseudo non unique / case sensitivity** → IRC est insensible à la casse pour les pseudos et channels (`#Test` == `#test`). À gérer.
- **Leaks** → chaque `Client`/`Channel` doit être proprement libéré à la déconnexion.

---

## 14. Bonus (si le mandatory est nickel)

À ne tenter **que** si la partie obligatoire est parfaite — sinon le bonus n'est même pas évalué.

- **Transfert de fichiers** entre clients (DCC — Direct Client-to-Client).
- **Un bot** : un client automatique que ton serveur héberge (réponses auto, commandes custom, etc.).

---

## 15. Ressources

- **RFC 1459** — https://datatracker.ietf.org/doc/html/rfc1459 (le protocole original)
- **RFC 2812** — https://datatracker.ietf.org/doc/html/rfc2812 (réponses numériques précises)
- **Modern IRC docs** — https://modern.ircdocs.horse/ (lecture beaucoup plus digeste que les RFC)
- `man poll`, `man socket`, `man recv`, `man getaddrinfo`

---

### TL;DR
ft_irc = un serveur TCP mono-process qui multiplexe N clients avec **un seul `poll()`**, parse des lignes `\r\n` bufferisées par client, et implémente le sous-ensemble IRC : auth, nick/user, channels, PRIVMSG, et les commandes op (KICK/INVITE/TOPIC/MODE + 5 modes). Les trois fondations à blinder en premier : **la boucle poll**, **le buffering des données partielles**, et **PING/PONG**.
