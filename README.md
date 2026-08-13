*This project has been created as part of the 42 school curriculum by mtaramar, magillie.*

# ft_irc

An IRC server written in C++ 98, following RFC 1459. A single process, no threads,
no `fork`: every client is multiplexed through one `poll()` that covers reading,
writing and accepting connections.

---

## Description

IRC (Internet Relay Chat) is a text-based communication protocol running over TCP.
Clients connect to a server, authenticate, pick a nickname, join channels and
exchange messages — every exchange being a single line of text terminated by
`\r\n`.

This project implements the **server** side only. There is no client and no
server-to-server communication, as required by the subject. A real IRC client
(irssi) is used to connect to it.

The interesting part of the project is not the protocol but the architecture. The
server must handle many clients at once while never blocking, using a single
`poll()` call and non-blocking file descriptors. Since the server can never wait,
each client owns two buffers:

- **an input buffer** that accumulates incoming bytes until a complete line is
  available — TCP is a byte stream and does not preserve message boundaries, so a
  command may arrive split across several packets, or several commands may arrive
  in one;
- **an output buffer** that holds replies until `poll()` reports the socket is
  writable — `send()` may only write part of what it is given, and the remainder
  must survive until the next round.

### Features

- Authentication with a server password, nickname and username
- Channels with operators, topic and member list
- Private and channel messages
- The five mandatory channel modes: `i`, `t`, `k`, `o`, `l`
- Graceful shutdown on `SIGINT` / `SIGQUIT`, with no leaked memory

### Supported commands

| Command | Form | Description |
|---|---|---|
| `PASS` | `PASS <password>` | Server password, required first |
| `NICK` | `NICK <nickname>` | Set or change the nickname, broadcast to channels |
| `USER` | `USER <user> 0 * :<realname>` | Client identity |
| `PING` | `PING <token>` | The server replies `PONG` |
| `QUIT` | `QUIT [:<reason>]` | Disconnect, announced to channels |
| `JOIN` | `JOIN <channel> [<key>]` | Join a channel, creating it if needed |
| `PART` | `PART <channel>{,<channel>} [:<reason>]` | Leave one or more channels |
| `PRIVMSG` | `PRIVMSG <target>{,<target>} :<text>` | Message to a channel or a user |
| `NOTICE` | `NOTICE <target>{,<target>} :<text>` | Same, but never triggers an error reply |
| `TOPIC` | `TOPIC <channel> [:<topic>]` | View or change the topic |
| `KICK` | `KICK <channel> <nick>{,<nick>} [:<reason>]` | Eject a member — operators only |
| `INVITE` | `INVITE <nick> <channel>` | Invite someone to a channel |
| `MODE` | `MODE <channel> [<flags> [<args>]]` | View or change channel modes |

The user who creates a channel becomes its operator. A channel is destroyed once
its last member leaves, along with its modes and invite list.

### Channel modes

| Mode | Argument | Effect |
|---|---|---|
| `+i` / `-i` | — | Invite-only channel |
| `+t` / `-t` | — | Only operators may change the topic |
| `+k` / `-k` | key on `+` | Protect the channel with a key |
| `+o` / `-o` | nickname | Give or take operator privilege |
| `+l` / `-l` | number on `+` | Set or remove the member limit |

Flags are chained with their arguments consumed in order:

```
MODE #dev +ok-l bob secret
```

gives operator privilege to bob, sets the key `secret`, and removes the limit.
`MODE <channel>` with no flag shows the current modes (reply `324`), restricted to
channel members since it reveals the key.

---

## Instructions

### Build

```bash
make          # builds ./ircserv
make clean    # removes object files
make fclean   # also removes the binary
make re
```

Compiled with `c++ -Wall -Wextra -Werror -std=c++98`. No warnings, no unnecessary
relinking.

### Run

```bash
./ircserv <port> <password>
```

- `port` — listening port, between 1024 and 65535
- `password` — the password every client must provide through `PASS`

```bash
./ircserv 6667 mypassword
```

The server stops cleanly on `Ctrl-C` (SIGINT) or SIGQUIT: clients receive
`ERROR :Server shutting down` before their descriptors are closed.

### Reference client

**irssi** — the server was developed and tested against it, and it is the client
used during evaluation.

```bash
irssi -c 127.0.0.1 -p 6667 -w mypassword
```

`nc` also works for debugging. The server accepts both `\n` and `\r\n` as line
terminators, so `-C` is optional:

```bash
nc -C 127.0.0.1 6667
PASS mypassword
NICK bob
USER bob 0 * :Bob
JOIN #test
```

Commands may be sent in several parts — the server buffers incoming bytes until a
complete line is available:

```bash
printf 'PASS mypassword\r\nNICK a\r\nUSER a 0 * :A\r\n' | nc -C 127.0.0.1 6667
```

### Project layout

```
main.cpp            arguments, signals, startup
Server.cpp          poll() loop, listening socket, shutdown
ServerClient.cpp    accept, recv, flush, disconnection
ServerDispatch.cpp  command routing, lookups
Message.cpp         splits a line into command + parameters
Channel.cpp         members, operators, invites, modes, broadcast
commands/           one file per command family
```

---

## Resources

### Documentation

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459) —
  the reference for this project: message grammar, command semantics, numeric replies
- [RFC 2812 — IRC Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812) —
  a later revision, more precise on numeric reply formats
- [Modern IRC documentation](https://modern.ircdocs.horse/) — a readable
  consolidation of what current servers actually implement
- `man 2 poll`, `man 2 socket`, `man 2 recv`, `man 2 send`, `man 7 signal` —
  the primary source for every system call used here
- Beej's Guide to Network Programming — for the socket lifecycle and the
  `sockaddr_in` / `htons` / `bind` / `listen` / `accept` sequence

### Use of AI

An AI coding assistant was used on this project, in the following way.

**What it was used for**

- *Code review and bug hunting.* Reviewing the existing socket layer surfaced two
  crashes that were then reproduced deterministically: an off-by-one write in
  `handleClient` that overflowed the receive buffer on a 512-byte line, and a
  dangling `Client*` kept by `Channel` after disconnection.

- *Documentation.* This README, and the comments explaining non-obvious decisions.

**What it was not used for**

The original architecture — the socket setup, the `poll()` loop, the per-client
buffering, the message parser, the registration flow, etc ..
was designed and written by the team before any AI assistance.

**How the output was handled**

Every generated change was compiled, run and tested against a live server before
being kept; the behavioural claims in this README (no leaks, partial commands,
abrupt disconnections, the flood-while-suspended case) correspond to tests that
were actually executed. Nothing was merged that the team could not explain.

---

## Known limitations

- `JOIN` does not accept comma-separated channel lists
- Only `#` channels are supported, not `&`
- The *realname* parameter of `USER` is parsed but not stored
- No user modes: `MODE <nickname>` is ignored
- Bonus features (file transfer, bot) are not implemented
