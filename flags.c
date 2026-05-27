#include <string.h>
#include <stdint.h>

const uint8_t SETUP_FLAG = 1; // Setup packet (rcopy to server) – optional
const uint8_t SETUP_RESPONSE_FLAG = 2; // Setup response packet (server to rcopy) - optional
const uint8_t DATA_FLAG = 3; // Data packet
const uint8_t RR_FLAG = 5; // RR packet
const uint8_t SREJ_FLAG = 6; // SREJ packet
const uint8_t FILENAME_FLAG = 7; // Packet contains the file name/buffer-size/window-size (rcopy to server)
const uint8_t FILENAME_RESPONSE_FLAG = 8; // Packet contains the file name/buffer-size/window-size (rcopy to server)

// Other flags I use
const uint8_t FILENAME_OK_FLAG = 33; // Server to rcopy, toFilename is valid
const uint8_t FILENAME_BAD_FLAG = 34; // Server to rcopy, toFilename is invalid

/**
 * Retrieves the value of the flag from the payload, returns it
 * 
 * @param flagStart Pointer to where the flag starts
 */ 
uint8_t getFlag(uint8_t *flagStart) {
    uint8_t flag;
    memcpy(&flag, flagStart, 1);

    return flag;
}
