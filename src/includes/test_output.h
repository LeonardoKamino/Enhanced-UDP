/**
*   @file test_output.h
*   @brief This file contains helper functions for displaying performance test results.
*
*   @bug No known bugs.
*   @author Leo Kamino (LeonardoKamino)
*/

#include <stdio.h>
#include <sys/time.h>

/**
 * @def LINK_CAPACITY
 * Definition that represents the network link capacity that the sender will assume
 * for calculating bandwidth utilization
 * 
 */
#define LINK_CAPACITY 20971520

/**
* @brief Display performance test results
* 
* This function displays the performance test results including throughput, bandwidth utilization,
* and transfer duration.
* @param start The start time of the transfer
* @param end The end time of the transfer
* @param totalBytesSent The total number of bytes sent
* @return Void.
*/
void displayPerformance(struct timeval *start, struct timeval *end, unsigned long long int totalBytesSent){
    double duration = (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec) / 1000000.0;
    double throughput = (totalBytesSent * 8) / duration;
    double bandwidthUtilization =  (throughput / LINK_CAPACITY) * 100;
    
    printf("Throughput: %.2f Mb/s\n", throughput/1000000.0);
    printf("Bandwidth Utilization: %.2f%%\n", bandwidthUtilization);
    printf("Transfer:  %.2f\n", duration);
}
