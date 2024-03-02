/**
 * @file sender.c
 * @brief Functions for sending data over an enhanced UDP protocol.
 * 
 * This file contains the implementation of functions
 * responsible for sending data over the enhanced UDP 
 * protocol. It includes a key function rsend which 
 * handles sending packets and handling acknowledgments. 
 * 
 * @author Maddy Paulson (maddypaulson)
 * @author Leo Kamino (LeonardoKamino)
 * @bug No known bugs.
 */

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

/**
 * @def BUFFER_SIZE
 * Definition to indicate size of the buffer.
 */
#define BUFFER_SIZE 1024

/**
 * @brief Sends a file over a network using a reliable UDP protocol.
 * 
 * This function rsend is designed to send a file specified by filename to a given hostname and UDP port. 
 * It handles the creation of a UDP socket, resolving the hostname, and setting up the destination address.
 * The file is read and sent in chunks, with each chunk being prefixed by a custom packet header.
 * After each packet is sent, the function waits for an acknowledgment before proceeding to the next packet.
 * If an acknowledgment is not received, the packet is resent. This continues until the entire file is sent.
 * The last packet is flagged as the final packet to inform the receiver no more packets will follow.
 * 
 * @param hostname The destination hostname or IP address to send the file to.
 * @param hostUDPport The destination port number on the receiver's side.
 * @param filename The name of the file to be sent.
 * @param bytesToTransfer The total number of bytes of the file to be transferred.
 * @return Void.
 */
void rsend(char* hostname, 
            unsigned short int hostUDPport, 
            char* filename, 
            unsigned long long int bytesToTransfer) 
{
    int sockDescriptor;
    struct sockaddr_in destAddr;
    char buffer[BUFFER_SIZE];
    char ackBuffer[BUFFER_SIZE];
    ssize_t readBytes, sentBytes;
    FILE *file;
    unsigned long long int totalBytesSent = 0;
    int sequenceNumber = 0;

    /*
     * Resolve the hostname to support domain & ip addresses.
     */
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int status;
    if((status = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0){
        perror("Address translation failed.");
        exit(EXIT_FAILURE);
    }

    /*
     * Create UDP socket.
     */
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

    /*
     * Open file for reading.
     */
    if((file = fopen(filename, "rb")) == NULL) {
        perror("Opening file failed");
        close(sockDescriptor);
        exit(EXIT_FAILURE);
    }

    /*
     * Continue sending file in chunks (packets) until all bytes are transferred.
     */
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
                /*
                 * Correct acknowlodgement received.
                 */
                printf("ACK received for sequence number: %d\n", ack.sequenceNumber);
                sequenceNumber++;
                /*
                 * Exit the resend loop.
                 */
                break;
            }

        } while(1);

        
    }

    /*
     * Send the last packet.
     */
    PacketHeader lastPacketHeader;
    lastPacketHeader.sequenceNumber = sequenceNumber++;
    lastPacketHeader.flags = 0;
    lastPacketHeader.flags = setFlag(lastPacketHeader.flags, IS_LAST_PACKET);

    sendto(sockDescriptor, &lastPacketHeader, sizeof(lastPacketHeader), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

    fclose(file);
    close(sockDescriptor);
}

/**
 * @brief Entry point for the UDP file sender program.
 * 
 * This function parses command line arguments and initiates the file transfer process
 * by calling the rsend function. The program expects exactly four arguments:
 * the receiver's hostname, the receiver's UDP port, the filename to transfer,
 * and the number of bytes to transfer.
 * 
 * @param argc The number of command line arguments.
 * @param argv The array of command line arguments.
 * @return int EXIT_SUCCESS returned on successful completion or error code 1 on failure.
 */
int main(int argc, char** argv) {
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