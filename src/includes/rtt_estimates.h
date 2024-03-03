/**
*   @file rtt_estimates.h
*   @brief This file contains the function declarations for calculating RTT and updating timeout values.

*   This file contains functions to calculate the Round Trip Time (RTT) and update the timeout value.
*   The RTT is calculated using the start and end time of a packet transmission. The timeout can be updated
*   via the function updateTimeout using the current sample RTT, estimated RTT and the deviation based on 
*   Jacobson/Karels Algorithm. The timeout value is also doubled vis the function doubleTimeOut.
*
*   @bug No known bugs.
*   @author Leo Kamino (LeonardoKamino)
*/

#include <math.h>
#include <sys/time.h>

/**
 * @brief Calculates the Round Trip Time (RTT) using the start and end time of a packet transmission.
 * 
 * @param startTime The time at which the packet was sent.
 * @param endTime The time at which the packet was received.
 * @return The RTT in milliseconds.
 */
double calculateRTT(struct timeval startTime, struct timeval endTime) {
    return (endTime.tv_sec - startTime.tv_sec) * 1000 + (endTime.tv_usec - startTime.tv_usec) / 1000.0;
}

/**
 * @brief Helper function to set timeout value from the given time in milliseconds.
 * 
 * @param timeout The timeout struct to be updated.
 * @param ms The time in milliseconds.
 */
void setTimeoutFromMs(struct timeval *timeout, int ms) {
    timeout->tv_sec = (ms / 1000); // Seconds part
    timeout->tv_usec = ((ms - timeout->tv_sec * 1000) * 1000); // Microseconds part
}

/**
 * @brief Updates the timeout value using the current sample RTT, estimated RTT, and deviation.
 * 
 * This function updates the timeout value using the current sample RTT, estimated RTT, and deviation.
 * The logic behind these calculations is based on the Jacobson/Karels Algorithm to decide when to timeout
 * and retransmit a packet. We added a constraint on the minimum timeout value of 5ms to avoid extreme cases.
 * 
 * @param estimatedRTT The estimated RTT value in ms.
 * @param deviation The deviation value in ms.
 * @param sampleRTT The current sample RTT value in ms.
 * @param timeout The timeout struct to be updated.
 */
void updateTimeout(double *estimatedRTT, double *deviation, double sampleRTT, struct timeval *timeout) {
    double alpha = 0.125;
    double beta = 0.25;

    double difference = sampleRTT - *estimatedRTT;
    *estimatedRTT = *estimatedRTT * (1-alpha) + alpha * sampleRTT;
    *deviation = *deviation * (1-beta) + beta * (fabs(difference));

    double timeout_msec = *estimatedRTT  + 4 * *deviation;
    timeout_msec = timeout_msec < 5 ? 5 : timeout_msec;
    timeout_msec = timeout_msec > 300 ? 300 : timeout_msec;

    setTimeoutFromMs(timeout, timeout_msec);
}


/**
 * @brief Doubles the timeout value.
 * Function is used in order to increase timeout value when multiple timeouts occur.
 * In order to avoid retransmission of packets in case of network congestion.
 * 
 * @param timeout The timeout struct to be updated.
 */
void doubleTimeOut(struct timeval *timeout){
    double timeout_msec = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    timeout_msec *= 2;

    setTimeoutFromMs(timeout, timeout_msec);
}