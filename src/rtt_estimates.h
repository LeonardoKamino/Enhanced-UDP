#include <math.h>
#include <sys/time.h>


double calculateRTT(struct timeval startTime, struct timeval endTime) {
    return (endTime.tv_sec - startTime.tv_sec) * 1000 + (endTime.tv_usec - startTime.tv_usec) / 1000.0;
}

void updateTimeout(double *estimatedRTT, double *deviation, double sampleRTT, struct timeval *timeout) {
    double alpha = 0.125;
    double beta = 0.25;

    double difference = sampleRTT - *estimatedRTT;
    *estimatedRTT = *estimatedRTT * (1-alpha) + alpha * sampleRTT;
    *deviation = *deviation * (1-beta) + beta * (fabs(difference));
    double timeout_msec = *estimatedRTT  + 4 * *deviation;
    timeout_msec = timeout_msec < 5 ? 5 : timeout_msec;
    timeout->tv_sec = (timeout_msec / 1000); // Seconds part
    timeout->tv_usec = ((timeout_msec - timeout->tv_sec * 1000) * 1000); 


}

void    doubleTimeOut(struct timeval *timeout){
    printf("Timeout: %ld %ld\n", timeout->tv_sec, timeout->tv_usec);

    double timeout_msec = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    timeout_msec *= 2;
    timeout->tv_sec = (timeout_msec / 1000); // Seconds part
    timeout->tv_usec = ((timeout_msec - timeout->tv_sec * 1000) * 1000); 
    printf("Timeout: %ld %ld\n", timeout->tv_sec, timeout->tv_usec);

}