# webserv

|command|description|
|----|----|
| `execve`				| Executes a program, replacing the current process. |
| `dup`					| Duplicates a file descriptor to the lowest available number. |
| `dup2`				| Duplicates a file descriptor to a specific number. |
| `pipe`				| Creates a unidirectional data channel for interprocess communication. |
| `strerror`			| Returns a string describing an error code. |
| `gai_strerror`		| Returns a string describing getaddrinfo errors. |
| `errno`				| Global variable storing the last error code. |
| `fork`				| Creates a new child process by duplicating the parent. |
| `socketpair`			| Creates a pair of connected sockets for IPC. |
| `htons`				| Converts a 16-bit integer from host to network byte order. |
| `htonl`				| Converts a 32-bit integer from host to network byte order. |
| `ntohs`				| Converts a 16-bit integer from network to host byte order. |
| `ntohl`				| Converts a 32-bit integer from network to host byte order. |
| `select`				| Monitors multiple file descriptors for readiness. |
| `poll`				| Monitors multiple file descriptors with a timeout. |
| `epoll_create`		| Creates an epoll instance for event monitoring. |
| `epoll_ctl`			| Controls interest in file descriptor events for epoll. |
| `epoll_wait`			| Waits for events on an epoll instance. |
| `kqueue`				| Creates an event queue for monitoring file descriptors. |
| `kevent`				| Modifies and retrieves events from a kqueue. |
| `socket`				| Creates an endpoint for network communication. |
| `accept`				| Accepts a connection on a socket. |
| `listen`				| Marks a socket as passive for incoming connections. |
| `send`				| Sends data through a socket. |
| `recv`				| Receives data from a socket. |
| `chdir`				| Changes the current working directory. |
| `bind`				| Assigns an address to a socket. |
| `connect`				| Establishes a connection to a remote socket. |
| `getaddrinfo`			| Resolves hostnames and service names into addresses. |
| `freeaddrinfo`		| Frees memory allocated by getaddrinfo. |
| `setsockopt`			| Sets socket options. |
| `getsockname`			| Retrieves the local address of a socket. |
| `getprotobyname`		| Retrieves protocol information by name. |
| `fcntl`				| Manipulates file descriptor properties. |
| `close`				| Closes a file descriptor. |
| `read`				| Reads data from a file descriptor. |
| `write`				| Writes data to a file descriptor. |
| `waitpid`				| Waits for a child process to change state. |
| `kill`				| Sends a signal to a process. |
| `signal`				| Registers a handler for signals. |
| `access`				| Checks file accessibility and permissions. |
| `stat`				| Retrieves file status information. |
| `open`				| Opens or creates a file. |
| `opendir`				| Opens a directory for reading. |
| `readdir`				| Reads an entry from a directory. |
| `closedir`			| Closes a directory stream. |
