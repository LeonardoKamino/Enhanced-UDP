/**
 * @file receiver.c
 * @brief Functions for receiving data over an enhanced UDP protocol.
 * 
 * This file contains the implementation of functions
 * responsible for receiving data over the enhanced UDP 
 * protocol. It includes key functions for setting up a server socket,
 * handling incoming packets, and assembling them into a complete file.
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

#include <pthread.h>
#include <errno.h>

#include "packet_header.h"

/**
 * @def BUFFER_SIZE
 * Definition to indicate size of the buffer.
 */
#define BUFFER_SIZE 10000

/**
 * @brief Prints the contents of the buffer.
 * 
 * This function is used for debugging purposes. It prints the
 * contents of the buffer up to the specified length.
 * 
 * @param buffer The buffer containing the data to be printed.
 * @param length The length of the data in the buffer.
 */
void printBufferContents(const char *buffer, ssize_t length) {
    printf("Buffer contents:\n");
    printf("ASCII: ");
    for (ssize_t i = 0; i < length; i++) {
        printf("%c", (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : '.');
    }
    printf("\n\n");
}

/**
 * @brief Sends a final acknowledgment (ACK) for the specified sequence number.
 * 
 * This function sends a final acknowledgment packet for the provided sequence number
 * to the specified destination address using the given socket descriptor. The acknowledgment
 * indicates the successful receipt of the last packet in the data transmission process.
 * 
 * @param sockDescriptor The socket descriptor for sending the acknowledgment.
 * @param destAddr The destination address to send the acknowledgment.
 * @param sequenceNumber The sequence number of the packet being acknowledged.
 * @return Void.
 */
void sendFinalAck(int sockDescriptor, struct sockaddr_in *destAddr, int sequenceNumber) {
    PacketHeader ack;
    ack.sequenceNumber = sequenceNumber;
    ack.flags = 0;
    ack.flags = setFlag(ack.flags, IS_ACK);
    ack.flags = setFlag(ack.flags, IS_LAST_PACKET);

    sendto(sockDescriptor, &ack, sizeof(ack), 0, (struct sockaddr *)destAddr, sizeof(struct sockaddr_in));
    printf("Sent final ack for sequence number: %d\n", sequenceNumber);
}

/**
 * @brief Receives a file over a network using a reliable UDP protocol.
 * 
 * This function sets up a UDP socket for the specified port, then enters a loop to receive packets.
 * Each packet is expected to have a header that the function checks to determine if it is the last packet.
 * Packet data, excluding the header, is written to the destination file specified.
 * For each received packet, an acknowledgment is sent back to the sender.
 * The function continues to receive packets until the last packet flag is encountered.
 * 
 * @param myUDPport The local UDP port to bind for listening to incoming packets.
 * @param destinationFile The path to the file where the incoming data should be written.
 * @param writeRate The rate at which the data should be written to the file.
 * 
 * @return Void.
 */
void rrecv(unsigned short int myUDPport, char* destinationFile, unsigned long long int writeRate) {
    
    int sockDescriptor;
    struct sockaddr_in myAddr, senderAddr;
    char buffer[BUFFER_SIZE];
    ssize_t receivedBytes;
    FILE *file;
    unsigned long long int bytesWritten = 0;
    int expectedSequenceNumber = 0;

    /*
     * Create UDP socket.
     */
    sockDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockDescriptor < 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(myUDPport);
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /*
     * Bind socket.
     */
    if(bind(sockDescriptor, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) {
        perror("Bind failed");
        close(sockDescriptor);
        exit(EXIT_FAILURE);
    }

    /*
     * Open destination file for writing.
     */
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
        
        /*
         * Extract the packet header from the received packet.
         */
        PacketHeader header;
        memcpy(&header, buffer, sizeof(header));

        if (isFlagSet(header.flags, IS_LAST_PACKET)) {
            printf("Last packet received. Sequence Number: %d\n", header.sequenceNumber);
            sendFinalAck(sockDescriptor, &senderAddr, header.sequenceNumber);
            break;
        } else if (header.sequenceNumber == expectedSequenceNumber) {
            printf("Received packet with sequence number: %d\n", header.sequenceNumber);
            /*
             * Write the received payload without the header to the file.
             */
            ssize_t payloadSize = receivedBytes - sizeof(PacketHeader);
            fwrite(buffer + sizeof(PacketHeader), 1, payloadSize, file);

            bytesWritten += payloadSize;

            /*
             * Prepare the acknowledgement packet to send.
             */
            PacketHeader ack;
            ack.sequenceNumber = header.sequenceNumber;
            ack.flags = 0;
            ack.flags = setFlag(ack.flags, IS_ACK);

            sendto(sockDescriptor, &ack, sizeof(ack), 0, (struct sockaddr *)&senderAddr, sizeof(senderAddr));

            expectedSequenceNumber++;
        }else if(header.sequenceNumber < expectedSequenceNumber){
            printf("Received duplicate packet with sequence number: %d Should be: %d\n", header.sequenceNumber, expectedSequenceNumber);
            PacketHeader ack;
            ack.sequenceNumber = header.sequenceNumber;
            ack.flags = 0;
            ack.flags = setFlag(ack.flags, IS_ACK);

            sendto(sockDescriptor, &ack, sizeof(ack), 0, (struct sockaddr *)&senderAddr, sizeof(senderAddr));
        }else{
            printf("Wrong packet received. Sequence Number: %d\n Expected: %d", header.sequenceNumber, expectedSequenceNumber);
        }
    }

    fclose(file);
    close(sockDescriptor);
}

/**
 * @brief Entry point for the UDP file receiver program.
 * 
 * This function parses command line arguments and initiates the file reception process
 * by calling the rrecv function. The program expects exactly two arguments:
 * the UDP port to listen on, and the filename to which the incoming data will be written.
 * 
 * @param argc The number of command line arguments.
 * @param argv The array of command line arguments.
 * @return int EXIT_SUCCESS returned on successful completion or error code 1 on failure.
 */
int main(int argc, char** argv) {
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
