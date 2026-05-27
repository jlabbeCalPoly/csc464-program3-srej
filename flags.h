#ifndef FLAGS_H
#define FLAGS_H

#include <stdlib.h>
#include <stdint.h>

extern const uint8_t SETUP_FLAG; // Setup packet (rcopy to server) – optional
extern const uint8_t SETUP_RESPONSE_FLAG; // Setup response packet (server to rcopy) - optional
extern const uint8_t DATA_FLAG; // Data packet
extern const uint8_t RR_FLAG; // RR packet
extern const uint8_t SREJ_FLAG; // SREJ packet
extern const uint8_t FILENAME_FLAG; // Packet contains the file name/buffer-size/window-size (rcopy to server)
extern const uint8_t FILENAME_RESPONSE_FLAG; // Packet contains the file name/buffer-size/window-size (rcopy to server)

// Other flags I use
extern const uint8_t FILENAME_OK_FLAG; // Server to rcopy, toFilename is valid
extern const uint8_t FILENAME_BAD_FLAG; // Server to rcopy, toFilename is invalid

uint8_t getFlag(uint8_t *flagStart);

#endif
