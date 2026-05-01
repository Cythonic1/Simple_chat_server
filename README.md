# 💬 Simple Chat Server

> A multi-client TCP chat server built from scratch in C using POSIX sockets and pthreads.

---

## Overview

Simple Chat Server is a terminal-based group chat application where multiple clients can connect simultaneously and broadcast messages to each other in real time. Each client is handled in its own thread, and a mutex-protected client queue ensures safe concurrent access.

---

## Features

- **Multi-client support** — up to 10 simultaneous connections
- **Per-client threads** — each connection gets a dedicated `pthread` for handling messages
- **Username registration** — clients choose a display name on connect
- **Message broadcasting** — messages are forwarded to all other connected clients
- **Join/leave notifications** — the chat room announces when users arrive or disconnect
- **Thread-safe client queue** — mutex locks protect shared client state across threads
- **Dynamic port binding** — binds to port 0 and reports the assigned port at startup

---

## Tech Stack

- **C** (C99)
- **POSIX sockets** (`sys/socket.h`, `netinet/in.h`)
- **pthreads** — one thread per client + mutex for shared state
- **arpa/inet** — address conversion utilities

---

## How It Works

```
Server starts → binds & listens on a random port
        │
Client connects → accept()
        │
malloc client_t → assign UID → queue_add()
        │
pthread_create() → handle_client()
        │
    ┌───┴────────────────────────────┐
    │  Prompt for username           │
    │  Notify all: "X has joined"    │
    │  Loop:                         │
    │    recv() message from client  │
    │    send_to_all() broadcast     │
    │  On disconnect:                │
    │    Notify all, queue_remove()  │
    └────────────────────────────────┘
```

---

## Project Structure

```
Simple_chat_server/
├── mainServer.c    # Full server implementation (single file)
└── README.md
```

### Key Functions

| Function | Description |
|----------|-------------|
| `queue_add()` | Thread-safely adds a new client to the pool |
| `queue_remove()` | Removes and frees a disconnected client |
| `send_to_all()` | Broadcasts a message to every client except the sender |
| `send_to_client()` | Sends a message to a specific client by UID |
| `userName_Handller()` | Prompts the client for a username on first connect |
| `handle_client()` | Main thread function — recv/send loop per client |

---

## Building & Running

```bash
# Clone the repo
git clone https://github.com/Cythonic1/Simple_chat_server.git
cd Simple_chat_server

# Compile
gcc mainServer.c -o chat_server -lpthread

# Run
./chat_server
# Output: The server is listening at 0.0.0.0, and port XXXXX
```

### Connecting a Client

Use `netcat` or any TCP client:

```bash
nc 127.0.0.1 <PORT>
```

---

## Limitations

- Max 10 concurrent clients (hardcoded array size)
- No encryption or authentication
- Terminal/raw TCP interface only — no GUI client

---

## Author

**Cythonic1** — [GitHub](https://github.com/Cythonic1)
