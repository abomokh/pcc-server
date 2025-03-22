# Printable Characters Counting (PCC) Server

## Overview: pcc-server
Client-server application for counting printable characters in a byte stream, showcasing network programming with TCP sockets in C.

## Features
- **Server (`pcc_server`)**
  - Accepts TCP connections from clients.
  - Receives a stream of bytes and counts printable characters (ASCII values 32-126).
  - Returns the count to the client.
  - Maintains statistics on the frequency of each printable character across all client sessions.
  - Handles `SIGINT` signal to print final statistics before termination.

- **Client (`pcc_client`)**
  - Connects to the server over TCP.
  - Reads a user-specified file and sends its contents to the server.
  - Receives and prints the count of printable characters.

## Compilation
To compile both the server and the client, run:
```sh
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_server.c -o pcc_server
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_client.c -o pcc_client
```

## Usage
### Start the Server
```sh
./pcc_server <port>
```
Example:
```sh
./pcc_server 8080
```

### Run the Client
```sh
./pcc_client <server_ip> <port> <file_path>
```
Example:
```sh
./pcc_client 127.0.0.1 8080 sample.txt
```

## Protocol
1. **Client to Server:**
   - Sends `N` (file size, 32-bit unsigned integer in network byte order).
   - Sends `N` bytes (file content).
2. **Server to Client:**
   - Sends `C` (count of printable characters, 32-bit unsigned integer in network byte order).

## Error Handling
- Invalid arguments result in an error message and exit code `1`.
- Network or file-related errors are printed to `stderr`.
- TCP connection failures are handled gracefully.

## Signal Handling
- On receiving `SIGINT`, the server:
  - Finishes processing any ongoing client request.
  - Prints statistics for all printable characters.
  - Exits cleanly.

## Notes
- The server uses `SO_REUSEADDR` to allow quick port reuse.
- The client only uses system calls for file handling (no `fopen()`).
- Memory allocations are limited to `1 MB` to handle large files efficiently.

## License
This project is for educational purposes and follows academic integrity guidelines.
