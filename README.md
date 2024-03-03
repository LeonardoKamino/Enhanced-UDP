@mainpage ELEC 331 Programming Assignment 1

@author Maddy Paulson (maddypaulson) @author Leo Kamino (LeoKamino)

@section sec1 1.0 Introduction

Our enhanced UDP implementation aims to provide reliable data transmission over an inherently unreliable UDP protocol. It introduces mechanisms such as packet acknowledgment (ACK), retransmissions upon timeout, and sequence numbering to ensure data integrity and order.

@section sec2 2.0 Usage 
First run the receiver with the following command, changing the arguments as required 
\code{.sh} 
./receiver UDP_port filename 
\endcode 
Then run the sender with the following command, changing the arguments as required 
\code{.sh} 
./sender hostname UDP_port filename bytesToSend 
\endcode

@section sec3 3.0 Design Decisions 
@subsection sub1 3.1 Buffer Size & Packet Structure
- The choice of buffer size and the design of the packet header were crucial. The buffer size determines how much data can be sent per packet, while the packet header structure carries necessary metadata for reliable transmission. 
@subsection sub2 3.2 Timeouts & Retransmissions
- The ACK timeout (ACK_TIMEOUT_USEC) and maximum resend attempts (MAX_RESEND_ATTEMPTS) are designed to balance between efficiency and reliability, preventing endless retransmissions. 
@subsection sub3 3.3 Bandwidth Utilization Calculation
- A calculation function for throughput and bandwidth utilization was implemented to provide feedback on the efficiency of the data transmission process, considering the actual data sent, including overhead and retransmissions.
@section sec4 4.0 Special Policies 
@subsection sub4 4.1 Packet Sequence Numbering
- Ensures packets are processed in the correct order and aids in identifying missing or out-of-order packets. 
@subsection sub5 4.2 ACK Timeouts and Retransmissions
- Defines the reliability layer on top of UDP by ensuring packets are retransmitted if an acknowledgment is not received within a certain timeframe. 
@subsection sub6 4.3 Closing Packet
- A special packet indicating the end of transmission, ensuring the receiver is aware when all data has been sent.
@section sec5 5.0 Known Bugs & Limitations 
@subsection sub7 5.1 TCP Bandwidth & Throughput Calculation
- The calculation for the throughput and bandwidth was giving us a value of 102%, however we believe this is likely due to the speed at which the ...
@section sec6 6.0 Testing 
See @subpage main_test_page

@section sec7 7.0 Project Comments 
The project was very difficult.