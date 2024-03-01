#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <pthread.h>
#include <errno.h>

#include "packet_header.h"


#define BUFFER_SIZE 1024

void rsend(char* hostname, 
            unsigned short int hostUDPport, 
            char* filename, 
            unsigned long long int bytesToTransfer) 
{
    // initial parameters
    int sockDescriptor;
    struct sockaddr_in destAddr;
    char buffer[BUFFER_SIZE];
    char ackBuffer[BUFFER_SIZE];
    ssize_t readBytes, sentBytes;
    FILE *file;
    unsigned long long int totalBytesSent = 0;
    int sequenceNumber = 0;

    // resolve the hostname to support domain & ip addresses
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int status;
    if((status = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0){
        perror("Address translation failed.");
        exit(EXIT_FAILURE);
    }

    // create UDP socket
    sockDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockDescriptor < 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(hostUDPport);
    memcpy(&destAddr.sin_addr, &((struct sockaddr_in *)servinfo->ai_addr)->sin_addr, sizeof(struct in_addr));

    freeaddrinfo(servinfo);

    if((file = fopen(filename, "rb")) == NULL) {
        perror("Opening file failed");
        close(sockDescriptor);
        exit(EXIT_FAILURE);
    }

    while(totalBytesSent < bytesToTransfer && (readBytes = fread(buffer + sizeof(PacketHeader), 1, BUFFER_SIZE - sizeof(PacketHeader), file)) > 0){
        PacketHeader header;
        header.sequenceNumber = sequenceNumber;
        header.flags = 0;

        memcpy(buffer, &header, sizeof(header));

        do {
            sentBytes = sendto(sockDescriptor, buffer, readBytes + sizeof(PacketHeader), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
            if(sentBytes < 0) {
                perror("Error sending file");
                break;
            }
            printf("Sent %ld sequence\n", sequenceNumber);
            totalBytesSent += sentBytes - sizeof(PacketHeader);

            PacketHeader ack;
            ssize_t ackSize = recvfrom(sockDescriptor, &ack, sizeof(ack), 0, NULL, 0);
            if(ackSize == sizeof(ack) && isFlagSet(ack.flags, IS_ACK) && ack.sequenceNumber == header.sequenceNumber ) {
                // Correct ACK received
                printf("ACK received for sequence number: %d\n", ack.sequenceNumber);
                sequenceNumber++;
                break; // Exit the resend loop
            }

        } while(1);

        
    }

    // Send the last packet
    PacketHeader lastPacketHeader;
    lastPacketHeader.sequenceNumber = sequenceNumber++;
    lastPacketHeader.flags = 0;
    lastPacketHeader.flags = setFlag(lastPacketHeader.flags, IS_LAST_PACKET);

    sendto(sockDescriptor, &lastPacketHeader, sizeof(lastPacketHeader), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

    fclose(file);
    close(sockDescriptor);
}

int main(int argc, char** argv) {
    // This is a skeleton of a main function.
    // You should implement this function more completely
    // so that one can invoke the file transfer from the
    // command line.
    int hostUDPport;
    unsigned long long int bytesToTransfer;
    char* hostname = NULL;
    char* filename = NULL;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    hostUDPport = (unsigned short int) atoi(argv[2]);
    hostname = argv[1];
    bytesToTransfer = atoll(argv[4]);
    filename = argv[3];

    rsend(hostname, hostUDPport, filename, bytesToTransfer);

    return (EXIT_SUCCESS); 
}