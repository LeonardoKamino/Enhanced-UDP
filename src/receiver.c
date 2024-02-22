#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <pthread.h>
#include <errno.h>

#define BUFFER_SIZE 1024

void printBufferContents(const char *buffer, ssize_t length) {
    printf("Buffer contents:\n");
    printf("ASCII: ");
    for (ssize_t i = 0; i < length; i++) {
        // Print as a character if it's printable, otherwise print '.'
        printf("%c", (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : '.');
    }
    printf("\nHEX: ");
    for (ssize_t i = 0; i < length; i++) {
        // Print each byte in hexadecimal format
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n\n");
}

void rrecv(unsigned short int myUDPport, char* destinationFile, unsigned long long int writeRate) {
    
    // initial parameters
    int sockDescriptor;
    struct sockaddr_in myAddr;
    char buffer[BUFFER_SIZE];
    ssize_t receivedBytes;
    FILE *file;
    unsigned long long int bytesWritten = 0;

    // create UDP socket
    sockDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockDescriptor < 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // setup
    memset(&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(myUDPport);
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    fprintf(stdout, "hello 1\n");

    // bind socket
    if(bind(sockDescriptor, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) {
        perror("Bind failed");
        close(sockDescriptor);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "hello 2\n");

    // open file
    file = fopen(destinationFile, "wb");
    if(file == NULL){
        perror("Failed to open destination file for writing.");
        close(sockDescriptor);
        exit(EXIT_FAILURE);
    }


    while(1){
        receivedBytes = recvfrom(sockDescriptor, buffer, BUFFER_SIZE, 0, NULL, NULL);

        if(receivedBytes < 0){
            perror("recvfrom failed");
            break;
        }

        fprintf(stdout, "%d\n", receivedBytes);
        printBufferContents(buffer, receivedBytes);
        fwrite(buffer, 1, receivedBytes, file);

        bytesWritten += receivedBytes;
        fclose(file);
        break;


        // implement logic for writerate > 0 here
    }

    fclose(file);
    close(sockDescriptor);
}

int main(int argc, char** argv) {
    // This is a skeleton of a main function.
    // You should implement this function more completely
    // so that one can invoke the file transfer from the
    // command line.

    unsigned short int udpPort;
    char* filename = NULL;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);
    filename = argv[2];

    rrecv(udpPort, filename, 0);
}
