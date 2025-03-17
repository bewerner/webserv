> ℹ️ **Info**: This project is currently in active development. Not all the listed features have been implemented yet.

# Webserv

Webserv is a custom HTTP server implementation developed in C++17 inspired by nginx, created as part of the 42 school core curriculum. This project involves building a web server from scratch that can handle HTTP requests, serve static websites, and execute CGI scripts.

## Features

### Core Functionality
- HTTP/1.1 compliant web server
- Non-blocking I/O operations
- Multiple host/port listening capability
- Limited nginx compatible server configuration

### HTTP Support
- Implements GET, POST, and DELETE methods
- Accurate HTTP response status codes
- Default error pages
- HTTP redirections
- Directory listing
- File uploads

### CGI Support
- CGI script execution based on file extensions
- Support for both GET and POST methods with CGI
- Proper CGI environment setup
- Chunked response transfer encoding

## Configuration

The server configuration file format is compatible with but more limietd than nginx.

The configuration allows you to configure:

- Port and host for each server
- Server names
- Error pages
- Client body size
- Accepted HTTP methods (limited to GET, POST and DELETE)
- Root directory (relative or absolute)
- Default index
- Directory listing
- Locations with additional configurations:
  - HTTP redirections (alias)
  - CGI execution

## Installation

```bash
git clone https://github.com/yourusername/webserv.git
cd webserv
make
```

## Usage

```bash
./webserv [configuration_file]
```

If no configuration file is provided, the server will use `webserver.conf`

## Testing

As a bonus and to help during development, I implemented a custom tester that compares webserv responses with nginx responses.

The tester allows you to:

- Set up test cases with specific HTTP requests
- Configure server config and files for each test
- Automatically compare responses between webserv and nginx
- Identify discrepancies in behavior or output

To run the tester, nginx needs to be installed on your system:

```bash
./turtle_tester/turtle_tester.sh
```

Since we are developing on an old macOS version, the tester might not be perfectly compatible with other platforms out of the box.
