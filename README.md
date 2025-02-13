# Enhanced UDP: Reliable Data Transmission over UDP

## Overview
This project is an **Enhanced UDP implementation** designed to provide **reliable data transmission** over an inherently **unreliable UDP protocol**. It introduces key mechanisms such as:
- **Packet acknowledgment (ACK)**
- **Retransmissions upon timeout**
- **Sequence numbering**

These features ensure **data integrity and order**, making UDP a more robust transport mechanism.

## Features
- **Reliable Data Transfer**: Implements sequence numbers and ACKs to prevent data loss.
- **Timeouts & Retransmissions**: Automatically detects lost packets and retransmits them.
- **Packet Sequence Numbering**: Ensures packets arrive in the correct order.
- **Bandwidth Utilization Metrics**: Calculates throughput and network efficiency.
- **Customizable Buffer Size**: Allows adjustment of packet size for optimized performance.

## **Tech Stack**
- **Language**: C  
- **Networking**: UDP Sockets  
- **Testing Platform**: CloudLab  
- **OS Compatibility**: Linux (tested on Ubuntu)  
- **Build System**: `Makefile`  
- **Performance Metrics**: Bandwidth, Throughput Analysis  

## Running the Application
Clone the repository

```git clone https://github.com/maddypaulson/enhanced-udp.git```

Run the make file from the main project directory

```make```

Run the following command to start the receiver:

```./receiver <UDP Port> <filename.txt>```

Run the following command in a seperate terminal to start the sender:

```./sender <receiver hostname> <receiver port> <transfer filename.txt> <num bytes to transfer>```

## Design Decisions
### Buffer Size & Packet Header Design
- The buffer size controls the amount of data per packet.
- The packet header contains essential metadata for reliable transmission.

### Timeouts & Retransmissions
- Uses ACK timeouts (ACK_TIMEOUT_USEC) to detect lost packets.
- Limits retransmissions with MAX_RESEND_ATTEMPTS to prevent infinite loops.

### Bandwidth Utilization Calculation
- Measures throughput and bandwidth efficiency.
- Includes overhead and retransmissions in the calculation.

### Packet Sequence Numbering
- Ensures packets are processed in order.
- Helps identify missing or out-of-order packets.

### ACK Timeouts & Retransmissions
- Implements a reliability layer by resending packets if no acknowledgment is received.

### Closing Packet Mechanism
- Uses a special packet to signal the end of transmission.
- Ensures the receiver knows when all data has been sent.

### Known Limitations
- Does not use Go-Back-N or Selective Repeat pipelining.
- Vulnerable to small packet loss, which impacts performance.

## Testing
### **Throughput Calculation**
The throughput is calculated using the following formula:


$$
\text{Throughput} = \frac{(\text{totalBytesSent} + \text{sizeof(PacketHeader)} \times \text{sequenceNumber}) \times 8}{\text{duration}}
$$

where:
- **totalBytesSent**: The total number of bytes of the payload sent.
- **sizeof(PacketHeader)**: The size of the header of each packet.
- **sequenceNumber**: The total number of packets sent.
- **8**: Converts bytes to bits.
- **duration**: The total time taken for transmission in seconds.

### **Bandwidth Calculation**
The bandwidth is calculated using the following formula:

$$
\text{Bandwidth} = \frac{\text{throughput}}{\text{LINK CAPACITY}} \times 100
$$

where:
- **throughput** is bits sent per second
- **LINK_CAPACITY** is the maximum amount of data that can be transmitted over a communication channel within a given time frame.
- 100 converts the value to a bandwidth percentage. 

### Testing a Single Instance of Our Protocol
**Requirement:** the protocol must, in steady state (averaged over 10 seconds), utilize at least 70% of bandwidth when there is no competing traffic, and packets are not artificially dropped or reordered.

#### **Testing Process**
- The test was conducted using CloudLab to ensure isolated network conditions.
- The sender runs on Juno, while the receiver runs on Europa.
- A 20Mbit/s link capacity was enforced.
- The transmission duration was measured over 10 seconds.
- A 20.1 MB file (SampleVideo.mp4) was used for accurate results.

#### **Test Results**
| Metric                  | Result        |
|-------------------------|--------------|
| **Achieved Bandwidth**  | 83.32%       |
| **Throughput**          | 17.47 Mb/s   |
| **Buffer Size**         | 10,000 bytes per packet |
| **Link Capacity**       | 20 Mbit/s    |
| **Total Bytes Sent**    | 21,086,550 bytes |
| **Total Bytes Received** | 21,069,678 bytes |packet headers.