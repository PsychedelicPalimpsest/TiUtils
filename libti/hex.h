#include <stdint.h>
#include <stddef.h>


extern const uint8_t HEX_TO_BIN_MAP[32];
extern const uint8_t BIN_TO_HEX_MAP[16];



/**
 * Converts a hex buffer to a binary buffer.
 * `length` is the amount of hex data.
 * 
 * Safety:
 *      `length` MUST be the amount of hex digits in the src.
 *      `dest` MUST be at least `length/2` bytes long.
 *      No safty checks are done, no null checking is done. 
 *      If you provide a large length it will barrel through
 *      lots of memory.
 */
static void hex2bin(uint8_t* dest, uint8_t* src, size_t length){
    uint8_t idx0, idx1;

    /* Amount of bytes to write */
    length >>= 1;

    
    while (length--){
        idx0 = (*(src++) & 0x1F) ^ 0x10;
        idx1 = (*(src++) & 0x1F) ^ 0x10;

        *(dest++) = (uint8_t)(HEX_TO_BIN_MAP[idx0] << 4) | HEX_TO_BIN_MAP[idx1];
    }
}

/**
 * Converts a raw buffer into a hex string.
 * `length` is the amount of binary data to convert.
 *
 * Safety:
 *      No null terminators are added
 *      `length` be equal to or less than the amount of data in src.   
 *      `dest` must be at least `length*2` bytes long.
 *      If you provide a large length it will barrel through
 *      lots of memory.
 */
static void bin2hex(uint8_t* dest, uint8_t* src, size_t length){
    while (length--){
        *(dest++) = BIN_TO_HEX_MAP[*src & 0xF0 >> 4];
        *(dest++) = BIN_TO_HEX_MAP[*src & 0x0F];
        src++;
    }
}

