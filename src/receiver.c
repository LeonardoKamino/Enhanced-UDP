
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

#include "packet_header.h"

#define BUFFER_SIZE 1024


void printBufferContents(const char *buffer, ssize_t length) {
    printf("Buffer contents:\n");
    printf("ASCII: ");
    for (ssize_t i = 0; i < length; i++) {
        printf("%c", (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : '.');
    }
    printf("\n\n");
}

void sendFinalAck(int sockDescriptor, struct sockaddr_in *destAddr, int sequenceNumber) {
    PacketHeader ack;
    ack.sequenceNumber = sequenceNumber;
    ack.flags = 0;
    ack.flags = setFlag(ack.flags, IS_ACK);
    ack.flags = setFlag(ack.flags, IS_LAST_PACKET);

    sendto(sockDescriptor, &ack, sizeof(ack), 0, (struct sockaddr *)destAddr, sizeof(struct sockaddr_in));
    printf("Sent final ack for sequence number: %d\n", sequenceNumber);
}

void rrecv(unsigned short int myUDPport, char* destinationFile, unsigned long long int writeRate) {
    
    // initial parameters
    int sockDescriptor;
    struct sockaddr_in myAddr, senderAddr;
    char buffer[BUFFER_SIZE];
    ssize_t receivedBytes;
    FILE *file;
    unsigned long long int bytesWritten = 0;
    int expectedSequenceNumber = 0;

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

    // bind socket
    if(bind(sockDescriptor, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) {
        perror("Bind failed");
        close(sockDescriptor);
        exit(EXIT_FAILURE);
    }

    // open file
    file = fopen(destinationFile, "wb");
    if(file == NULL){
        perror("Failed to open destination file for writing.");
        close(sockDescriptor);
        exit(EXIT_FAILURE);
    }


    while(1){
        socklen_t senderAddrLen = sizeof(senderAddr);
        receivedBytes = recvfrom(sockDescriptor, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&senderAddr, &senderAddrLen);

        if(receivedBytes < 0){
            perror("recvfrom failed");
            break;
        }

        PacketHeader header;
        memcpy(&header, buffer, sizeof(header));

        if (isFlagSet(header.flags, IS_LAST_PACKET)) {
            printf("Last packet received. Sequence Number: %d\n", header.sequenceNumber);
            sendFinalAck(sockDescriptor, &senderAddr, header.sequenceNumber);
            break;
        } else if (header.sequenceNumber == expectedSequenceNumber) {
            printf("Packet received. Sequence Number: %d\n", header.sequenceNumber);
            // fprintf(stdout, "%d\n", receivedBytes);
            // printBufferContents(buffer, receivedBytes);

            ssize_t payloadSize = receivedBytes - sizeof(PacketHeader);
            fwrite(buffer + sizeof(PacketHeader), 1, payloadSize, file);

            bytesWritten += payloadSize;

            PacketHeader ack;
            ack.sequenceNumber = header.sequenceNumber;
            ack.flags = 0;
            ack.flags = setFlag(ack.flags, IS_ACK);

            sendto(sockDescriptor, &ack, sizeof(ack), 0, (struct sockaddr *)&senderAddr, sizeof(senderAddr));
            expectedSequenceNumber++;
        }else{
            printf("Wrong packet received. Sequence Number: %d\n Expected: %d", header.sequenceNumber, expectedSequenceNumber);
        }

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
