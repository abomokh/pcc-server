#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdatomic.h>
#include <signal.h>

#define BUFFER_SIZE 1048576 // 1MB: Buffer size used for reading and sending data.
#define LOWER_BOUND 32      // Lower bound of printable ASCII characters.
#define UPPER_BOUND 126     // Upper bound of printable ASCII characters.
#define FAILURE_CODE -1     // Code returned on failure in operations.
#define SUCCESS_CODE 0      // Code returned on success in operations.

uint32_t charCount[95];             // Array to store the frequency count of printable characters.
int serverSocket, clientSocket;     // Server and client socket file descriptors.
atomic_int interruptReceived = 0;   // Flag to handle interrupt signal and gracefully terminate the server.

int recv_from_client(int socket, char *buffer, size_t bytes);
int send_to_client(int socket, char *buffer, size_t bytes);
void handle_client();
void signal_handler(int sig);
void display_stats();

int main(int argc, char *argv[])
{
    // assert num of arguments
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);       // get port number.
    struct sockaddr_in serverAddr;  // struct to hold server address.
    struct sigaction action;        // struct to define the signal handler.

    // Config the signal handler for SIGINT.
    action.sa_handler = signal_handler;
    action.sa_flags = SA_RESTART;
    sigfillset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    // create a listening socket.
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    // set the socket to allow address reuse.
    int reuseAddrOpt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddrOpt, sizeof(int));

    // config server address (IPv4, specified port, any available network interface (INADDR_ANY)).
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);              // convert port to network byte order.
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // accept connections on any available network interface (INADDR_ANY).

    // bind the listening socket to the server address.
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    // listen for incoming client connections, up to 10 connections.
    if (listen(serverSocket, 10) < 0)
    {
        perror("listen");
        exit(1);
    }

    // main loop.
    while (!interruptReceived)
    {
        // accept an incoming connection.
        clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket < 0)
        {
            // if interrupted, break the loop.
            if (interruptReceived) break;
            perror("accept");
            continue;           // continue to the next connection attempt.
        }
        handle_client();        // handle the connected client.
        close(clientSocket);    // close the client socket after handling.
    }

    // display the statistics.
    display_stats();
    close(serverSocket);        // close the server socket.
    return SUCCESS_CODE;
}

// function to receive data from the client.
int recv_from_client(int socket, char *buffer, size_t bytes)
{
    size_t totalReceived = 0;
    // Continue receiving until reaching the requested number of bytes.
    while (totalReceived < bytes)
    {
        ssize_t result = recv(socket, buffer + totalReceived, bytes - totalReceived, 0);
        if (result <= 0)
        {
            perror("recv");
            return FAILURE_CODE;    // return failure if no data or error occurs.
        }
        totalReceived += result;    // Update the total bytes received.
    }
    return totalReceived;
}

// function to send data to the client socket.
int send_to_client(int socket, char *buffer, size_t bytes)
{
    size_t totalSent = 0;
    // continue sending until reaching the requested number of bytes.
    while (totalSent < bytes)
    {
        ssize_t result = send(socket, buffer + totalSent, bytes - totalSent, 0);
        if (result <= 0)
        {
            perror("send");
            return FAILURE_CODE;    // return failure if no data or error occurs.
        }
        totalSent += result;        // update the total bytes sent.
    }
    return totalSent;
}

// function to handle communication with a single client.
void handle_client()
{
    uint32_t fileSizeNetworkByteOrder, fileSize;
    // receive the file size from the client in network byte order.
    if (recv_from_client(clientSocket, (char*)&fileSizeNetworkByteOrder, sizeof(fileSizeNetworkByteOrder)) < 0) return;
    fileSize = ntohl(fileSizeNetworkByteOrder); // convert file size to host byte order.

    char buffer[BUFFER_SIZE]; // buffer to hold data received from client.
    uint32_t printableCount = 0; // counter to track printable characters.
    while (fileSize > 0)
    {
        // determine the chunk size to read, ensuring it does not exceed the file size.
        size_t chunkSize = (fileSize > BUFFER_SIZE) ? BUFFER_SIZE : fileSize;
        int bytesReceived = recv_from_client(clientSocket, buffer, chunkSize);
        if (bytesReceived < 0) return; // exit if error.

        // Process each byte in the recv buffer.
        for (int i = 0; i < bytesReceived; ++i)
        {
            // Check if the byte is a printable.
            if (buffer[i] >= LOWER_BOUND && buffer[i] <= UPPER_BOUND)
            {
                charCount[buffer[i] - LOWER_BOUND]++;   // increment the count for the character.
                printableCount++;                       // Increment the total counter.
            }
        }
        fileSize -= bytesReceived;                      // decrease the remaining file size.
    }

    // send the count of printables back to the client.
    uint32_t printableCountNetworkByteOrder = htonl(printableCount);
    send_to_client(clientSocket, (char*)&printableCountNetworkByteOrder, sizeof(printableCountNetworkByteOrder));
}

// signal handler for SIGINT to shut down the server.
void signal_handler(int sig)
{
    interruptReceived = 1;  // set the interrupt flag.
    close(serverSocket);    // close the server socket.
}

// function to display the stats.
void display_stats()
{
    for (int i = 0; i < UPPER_BOUND - LOWER_BOUND + 1; i++)
        printf("Character '%c' occurred %u times\n", (char)(i + LOWER_BOUND), charCount[i]);
}
