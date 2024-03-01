typedef struct {
    int sequenceNumber;
    int flags;
} PacketHeader;

#define IS_LAST_PACKET 0
#define IS_ACK 1

// Function to set a flag
int setFlag(int flags, int bit) {
    return flags | (1 << bit);
}

// Function to check a flag
int isFlagSet(int flags, int bit) {
    return (flags & (1 << bit)) != 0;
}