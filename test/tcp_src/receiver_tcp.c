/**
 * @file receiver_tcp.c
 * @brief Functions for receiving data over TCP Protocol.
 * 
 * This file contains the implementation of functions
 * responsible for receiving data over the  TCP 
 * protocol. It includes key functions for handling
 * incoming data and compiling the output file.
 * 
 * @author Maddy Paulson (maddypaulson)
 * @author Leo Kamino (LeoKamino)
 * @bug No known bugs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

/**
 * @brief Receives a file over a TCP connection and writes it to the specified file.
 * 
 * This function sets up a TCP socket to listen on the specified port. It accepts a connection,
 * then reads data from the connection and writes it to the specified destination file.
 * The function continues to receive data until the connection is closed by the sender.
 * 
 * @param myTCPport The local TCP port to bind for listening to incoming connections.
 * @param destinationFile The path to the file where the incoming data should be written.
 */
void trecv(unsigned short int myTCPport, char* destinationFile) {
    int listenfd, connfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    ssize_t readBytes;
    FILE *file;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(myTCPport);

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 10) < 0) {
        perror("Listen failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", myTCPport);

    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
    if (connfd < 0) {
        perror("Accept failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    file = fopen(destinationFile, "wb");
    if (file == NULL) {
        perror("Failed to open destination file for writing");
        close(connfd);
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    while ((readBytes = read(connfd, buffer, BUFFER_SIZE)) > 0) {
        fwrite(buffer, sizeof(char), readBytes, file);
    }

    printf("File transfer complete.\n");

    fclose(file);
    close(connfd);
    close(listenfd);
}

/**
 * @brief Entry point for the TCP file receiver program.
 * 
 * This function parses command line arguments and initiates the file reception process
 * by calling the trecv function. The program expects exactly two arguments:
 * the TCP port to listen on, and the filename to which the incoming data will be written.
 * 
 * @param argc The number of command line arguments.
 * @param argv The array of command line arguments.
 * @return int EXIT_SUCCESS on successful completion or EXIT_FAILURE on error.
 */
int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s TCP_port filename_to_write\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    unsigned short int tcpPort = (unsigned short int)atoi(argv[1]);
    char* filename = argv[2];

    trecv(tcpPort, filename);

    return EXIT_SUCCESS;
}
