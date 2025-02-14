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



| Content-Type | Erklärung |
|--------------|-----------|
| text/plain | Einfache Textdatei ohne spezielle Formatierung |
| text/html | Webseiten im HTML-Format für Browser |
| text/css | Stylesheet-Dateien für das Styling von Webseiten |
| text/javascript | JavaScript-Dateien für Webfunktionalitäten |
| image/jpeg | Komprimierte Bildformate für Fotos und Grafiken |
| image/png | Verlustfreies Bildformat mit Transparenzunterstützung |
| audio/mpeg | Komprimierte Audiodateien wie MP3 |
| video/mp4 | Gängiges Format für Videos im Web |
| application/json | Datenaustauschformat für strukturierte Informationen |
| application/pdf | Portable Document Format für plattformunabhängige Dokumente |
| application/zip | Komprimierte Archivdateien |
| multipart/form-data | Für Dateiuploads und Formularübermittlungen |
| text/csv | Tabellarische Daten im CSV-Format |
| application/xml | Strukturierte Daten im XML-Format |
| image/svg+xml | Vektorgrafiken im SVG-Format |


| Header | Function | Example Values |
|--------|----------|----------------|
| Accept | Specifies which content types are acceptable | `text/html, application/json, image/*, application/xml;q=0.9` |
| Accept-Encoding | Acceptable encoding methods for response | `gzip, deflate, br, compress` |
| Accept-Language | Preferred natural languages | `en-US,en;q=0.9,de-DE;q=0.8,de;q=0.7` |
| Authorization | Authentication credentials | `Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...`, `Basic dXNlcjpwYXNzd29yZA==` |
| Cache-Control | Directives for caching mechanisms | `no-cache, no-store, must-revalidate, max-age=3600, public` |
| Connection | Control options for current connection | `keep-alive, close, upgrade` |
| Content-Length | Size of the response body in bytes | `2048, 1024, 512` |
| Content-Type | Media type of the resource | `application/json; charset=utf-8`, `multipart/form-data; boundary=something` |
| Content-Encoding | Encoding methods applied to the data | `gzip, deflate, br` |
| Content-Language | Natural language of the content | `en-US, fr-CA, es-ES` |
| Content-Disposition | Controls how content should be displayed or downloaded | `attachment; filename="report.pdf"`, `inline; filename="preview.jpg"`, `form-data; name="field1"; filename="upload.txt"` |
| Cookie | HTTP cookies previously sent by the server | `sessionId=abc123; user=john_doe; theme=light` |
| Date | Date and time of the message | `Wed, 15 Jan 2025 15:30:00 GMT` |
| ETag | Version identifier for a resource | `"33a64df551425fcc55e4d42a148795d9f25f89d4"` |
| Expires | Date/time after which response is stale | `Wed, 15 Jan 2025 15:30:00 GMT` |
| Host | Domain name of the server | `api.example.com:443, subdomain.website.com` |
| If-Modified-Since | Conditional request timestamp | `Wed, 15 Jan 2025 15:30:00 GMT` |
| Last-Modified | Last modification date of resource | `Wed, 15 Jan 2025 15:30:00 GMT` |
| Location | URL for redirection | `https://api.example.com/v2/users, /new-page.html` |
| Origin | Origin initiating the request | `https://www.example.com, https://admin.site.org` |
| Pragma | Implementation-specific directives | `no-cache, x-custom-header=value` |
| Referer | Address of previous web page | `https://www.example.com/page1?q=search` |
| Server | Server software information | `Apache/2.4.41 (Unix) OpenSSL/1.1.1d PHP/7.4.1` |
| Set-Cookie | Set a cookie in the client | `sessionId=abc123; Path=/; HttpOnly; Secure; SameSite=Strict` |
| User-Agent | Client application information | `Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36` |
| X-Forwarded-For | Original client IP address | `203.0.113.195, 70.41.3.18, 150.172.238.178` |
| X-Frame-Options | Control frame embedding | `DENY, SAMEORIGIN, ALLOW-FROM https://example.com` |
| X-XSS-Protection | Cross-site scripting filter | `1; mode=block, 0, 1; report=https://example.com/xss` |
| Access-Control-Allow-Origin | CORS permissions | `*, https://trusted-site.com, https://api.example.com` |
| Strict-Transport-Security | HTTPS enforcement policy | `max-age=31536000; includeSubDomains; preload` |
| Transfer-Encoding | Data transfer encoding method | `chunked, compress, deflate, gzip` |
| Vary | How to match future request headers | `User-Agent, Accept-Encoding, Accept-Language` |
| WWW-Authenticate | Authentication method required | `Basic realm="login", Bearer realm="token_required", Digest realm="restricted"` |

