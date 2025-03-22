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

#define MAXBUFFER 1048576   // 1MB buffer size
int sockId;                 // Socket descriptor
#define FAILURE_CODE -1     // Return code for failure
#define SUCCESS_CODE 0      // Return code for success

int SockReceive(char *buffer, size_t numBytes);
int SockWrite(char *buffer, size_t totalBytes);
int SockSendFile(int filedescriptor);
uint32_t getFileSize(char *fp);

int main(int argc, char *argv[])
{
    // assert number of agrs
    if (argc != 4)
    {
        fprintf(stderr, "Invalid number of arguments!\n");
        exit(1);
    }

    // extract args
    char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);
    char *path_file_send = argv[3];

    //open file to be sent
    int filedescriptor = open(path_file_send, O_RDONLY);
    if (filedescriptor < 0)
    {
        perror("Error opening file");
        exit(1);
    }

    //get file size
    uint32_t fileSize = getFileSize(path_file_send);
    uint32_t fileSizeNBO = htonl(fileSize);

    //create listening socket
    if ((sockId = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    {
        perror("Error creating socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serverPort);

    // convert the IP address to binary & store it in server_addr
    if (inet_pton(AF_INET, serverIP, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid server IP address");
        exit(1);
    }

    //connect to the server
    if (connect(sockId, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error connecting to server");
        exit(1);
    }

    //send the file size
    if (SockWrite((char *)&fileSizeNBO, sizeof(fileSizeNBO)) < 0)
        exit(1);

    // Send the file contents
    if (SockSendFile(filedescriptor) < 0)
        exit(1);

    uint32_t printableCharCountNBO;
    if (SockReceive((char *)&printableCharCountNBO, sizeof(printableCharCountNBO)) < 0)
        exit(1);

    uint32_t printableCharCount = ntohl(printableCharCountNBO);
    printf("# of printable characters: %u\n", printableCharCount);
    
    // close file and socket
    close(filedescriptor);
    close(sockId);
    return SUCCESS_CODE;
}

//function to recv data
int SockReceive(char *buffer, size_t numBytes)
{
    size_t totalReceived = 0;
    ssize_t res;
    
    while (totalReceived < numBytes)
    {
        res = recv(sockId, buffer + totalReceived, numBytes - totalReceived, 0);
        if (res <= 0)
        {
            perror("Error receiving data");
            return FAILURE_CODE;
        }
        totalReceived += res;
    }
    return SUCCESS_CODE;
}

// function to write data
int SockWrite(char *buffer, size_t totalBytes)
{
    ssize_t bytesSent;
    size_t totalSent = 0;
    
    while (totalSent < totalBytes)
    {
        bytesSent = send(sockId, buffer + totalSent, totalBytes - totalSent, 0);
        if (bytesSent <= 0)
        {
            perror("Error sending data");
            return FAILURE_CODE;
        }
        totalSent += bytesSent;
    }
    return SUCCESS_CODE;
}

// function to send the file's contents to the server
int SockSendFile(int filedescriptor)
{
    ssize_t res;
    char buffer[MAXBUFFER];
    
    while ((res = read(filedescriptor, buffer, MAXBUFFER)) > 0)
    {
        if (SockWrite(buffer, res) < 0)
        {
            return FAILURE_CODE;
        }
    }
    
    if (res < 0)
    {
        perror("Error reading file");
        return FAILURE_CODE;
    }
    
    return SUCCESS_CODE;
}

// function to get the size of the file
uint32_t getFileSize(char *fp)
{
    struct stat st;
    if (stat(fp, &st) != 0)
    {
        perror("Error getting file size");
        exit(1);
    }
    return (uint32_t)st.st_size;
}