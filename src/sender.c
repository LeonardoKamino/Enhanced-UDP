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
#include <sys/time.h>

#include "includes/packet_header.h"
#include "includes/rtt_estimates.h"
#include "includes/test_output.h"

/**
 * @def BUFFER_SIZE
 * Definition to indicate size of the buffer.
 */
#define BUFFER_SIZE 10000

/**
* @def EXPECTED_RTT 
* Definition specifying the expected Round Trip Time (RTT) in milliseconds.
*/
#define EXPECTED_RTT 20

#define TIMEOUT_ACK 30000

/**
 * @def MAX_FINAL_PKT_RESEND_ATTEMPTS
 * Definition specifying the maximum number of resend attempts for a packet
 * if the acknowledgment (ACK) is not received within the timeout period.
 */
#define MAX_FINAL_PKT_RESEND_ATTEMPTS 5

/**
 * @brief Sends a closing packet to the specified destination address.
 * 
 * This function sends a closing packet containing the provided sequence number
 * to the specified destination address using the given socket descriptor. The
 * closing packet indicates the end of the data transmission. It retransmits the
 * closing packet up to a maximum number of times defined by MAX_FINAL_PKT_RESEND_ATTEMPTS
 * until an acknowledgment packet is received or the maximum attempts are
 * exhausted.
 * 
 * @param sockDescriptor The socket descriptor for sending the closing packet.
 * @param destAddr The destination address to send the closing packet.
 * @param sequenceNumber The sequence number of the closing packet.
 * @return Void.
 */
void sendClosingPacket(int sockDescriptor, struct sockaddr_in *destAddr, int sequenceNumber) {
    int resendAttempts = 0;
    int sentBytes;

    PacketHeader lastPacketHeader;
    lastPacketHeader.sequenceNumber = sequenceNumber;
    lastPacketHeader.flags = 0;
    lastPacketHeader.flags = setFlag(lastPacketHeader.flags, IS_LAST_PACKET);

    do {
        sentBytes = sendto(sockDescriptor, &lastPacketHeader, sizeof(lastPacketHeader), 0, (struct sockaddr *)destAddr, sizeof(struct sockaddr_in));
        if(sentBytes < 0) {
            perror("Error sending closing packet");
            break;
        }
       

        PacketHeader ack;
        ssize_t ackSize = recvfrom(sockDescriptor, &ack, sizeof(ack), 0, NULL, 0);
        if (ackSize > 0 && isFlagSet(ack.flags, IS_ACK) && isFlagSet(ack.flags, IS_LAST_PACKET) && ack.sequenceNumber == sequenceNumber){
            printf("ACK received for closing packet sequence number: %d\n", ack.sequenceNumber);
            break; // Exit the resend loop
        } else {
            printf("ACK timeout or error, resending closing packet, sequence number: %ld\n", sequenceNumber);
            resendAttempts++;
        }

    } while(resendAttempts < MAX_FINAL_PKT_RESEND_ATTEMPTS);
}

/**
 * @brief Sends a file over UDP to the specified destination.
 * 
 * This function sends a file over User Datagram Protocol (UDP) to the specified
 * destination hostname and port. It reads the file specified by the filename and
 * sends it in chunks (packets) until all bytes are transferred or until an error
 * occurs. It uses acknowledgments (ACKs) to ensure reliable delivery of packets
 * and handles retransmissions in case of timeouts or errors. The function also
 * measures the bandwidth during the transmission process.
 * 
 * @param hostname The hostname or IP address of the destination.
 * @param hostUDPport The hostname or IP address of the destination.
 * @param filename The name of the file to be sent.
 * @param bytesToTransfer The total number of bytes to transfer from the file.
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
    ssize_t readBytes, sentBytes;
    FILE *file;

    unsigned long long int totalBytesSent = 0;
    unsigned long long int totalValidBytesSent = 0;

    int sequenceNumber = 0;

    /*
    * Initialize variables for Timeout calculation
    */
    double estimatedRTT = EXPECTED_RTT;
    double deviationRTT = 0;
    struct timeval sendTime, receiveTime;

    /*
     * Resolve the hostname to support domain & ip addresses.
     */
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int status;
    printf("Hostname: %s\n", hostname);
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

    /*
    * Set socket timeout for ACKs
    */
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 2000 * EXPECTED_RTT;
    if (setsockopt(sockDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
        close(sockDescriptor);
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
    * Start timing for bandwidth calculation
    */
    struct timeval start, end;
    gettimeofday(&start, NULL);

    /*
     * Continue sending file in chunks (packets) until all bytes are transferred.
     */
    while(totalValidBytesSent < bytesToTransfer && (readBytes = fread(buffer + sizeof(PacketHeader), 1, BUFFER_SIZE - sizeof(PacketHeader), file)) > 0){
        PacketHeader header;
        header.sequenceNumber = sequenceNumber;
        header.flags = 0;

        memcpy(buffer, &header, sizeof(header));

        do {

            sentBytes = sendto(sockDescriptor, buffer, readBytes + sizeof(PacketHeader), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
            totalBytesSent += sentBytes;

            
            //printf("Sent %ld sequence\n", sequenceNumber);
            printf("Sent %ld sequence\n", sequenceNumber);

            PacketHeader ack;
            ssize_t ackSize = recvfrom(sockDescriptor, &ack, sizeof(ack), 0, NULL, 0);

            

            /*
            * If ACK is for the current packet, update sequence number and timeout, and exit the resend loop.
            */
            if (ackSize > 0 && isFlagSet(ack.flags, IS_ACK) && ack.sequenceNumber == header.sequenceNumber) {
                sequenceNumber++;
                totalValidBytesSent += sentBytes - sizeof(PacketHeader);
                printf("ACK received for sequence number: %d\n", ack.sequenceNumber);
                
                gettimeofday(&receiveTime, NULL);
                double rtt = calculateRTT(sendTime, receiveTime);
                updateTimeout(&estimatedRTT, &deviationRTT, rtt, &timeout);

                if (setsockopt(sockDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                    perror("Error setting socket timeout");
                    close(sockDescriptor);
                    exit(EXIT_FAILURE);
                }

                break; 
            } else if (errno == EAGAIN || errno == EWOULDBLOCK){
                printf("ACK TIMEOUT resending sequence number: %ld\n", sequenceNumber);
                /*
                * If the ACK timeout occurs, double current timeout.
                */
                doubleTimeOut(&timeout);

                if (setsockopt(sockDescriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                    perror("Error setting socket timeout");
                    close(sockDescriptor);
                    exit(EXIT_FAILURE);
                }

            }

        } while(1);
    }

    sendClosingPacket(sockDescriptor, &destAddr, sequenceNumber);

    gettimeofday(&end, NULL);

    displayPerformance(&start, &end, totalBytesSent);

    fclose(file);
    close(sockDescriptor);
}

/**
 * @brief Entry point for the UDP file receiver program.
 * 
 * This function parses command line arguments and initiates the file sending process
 * by calling the rsend function. The program expects exactly two arguments:
 * the UDP port to send data to, and the filename of the file to be sent.
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