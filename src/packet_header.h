/**
 * @file packet_header.h
 * @brief Definitions and utility functions for UDP protocol flag handling.
 * 
 * This header file defines constants for packet flags and includes utility functions
 * for setting and checking these flags within the PacketHeader structure.
 * These are used to manage and interpret the state of packets in a custom UDP protocol.
 * 
 * @author Maddy Paulson (maddypaulson)
 * @author Leo Kamino (LeonardoKamino)
 * @bug No known bugs.
 */

/**
 * @def IS_LAST_PACKET
 * Flag to indicate the last packet in a sequence of UDP transmissions.
 */
#define IS_LAST_PACKET 0

/**
 * @def IS_ACK
 * Flag to indicate an acknowledgment packet.
 */
#define IS_ACK 1

/**
 * @struct PacketHeader
 * @brief Header structure for packets in the enhanced UDP protocol.
 * 
 * This structure is used to hold the packet sequence number and flags for
 * control information such as the last packet and acknowledgment indicators.
 */
typedef struct {
    int sequenceNumber;
    int flags;
} PacketHeader;

/**
 * @brief Sets the specified bit in the flags.
 * 
 * This function sets a specified bit in the flags integer to indicate the status
 * of the packet, such as IS_LAST_PACKET or IS_ACK.
 * 
 * @param flags The current flags value.
 * @param bit The bit to set in the flags.
 * @return int The new value of flags after setting the specified bit.
 */
int setFlag(int flags, int bit) {
    return flags | (1 << bit);
}

/**
 * @brief Checks if the specified bit in the flags is set.
 * 
 * This function checks whether a specified bit in the flags integer is set,
 * which can indicate whether the packet is the last one or an acknowledgment.
 * 
 * @param flags The flags value to check.
 * @param bit The bit to check in the flags.
 * @return int Non-zero if the bit is set; otherwise, zero.
 */
int isFlagSet(int flags, int bit) {
    return (flags & (1 << bit)) != 0;
}