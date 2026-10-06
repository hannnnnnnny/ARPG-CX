/*
 * save.h - Profile persistence (calculator: AshenDepths.sav.tns next to
 * the game). Explicit little-endian field-by-field format with magic,
 * version, length and CRC32; anything invalid loads as a fresh hero.
 * Writes go to a temp file first, then rename, so a crash cannot corrupt it.
 */
#ifndef AD_SAVE_H
#define AD_SAVE_H

#include "defs.h"

typedef enum { SAVE_OK, SAVE_MISSING, SAVE_CORRUPT, SAVE_IO_ERROR } SaveStatus;

#define SAVE_MAX_BYTES 16384

size_t     save_serialize(const Profile *p, uint8_t *buf, size_t cap);
SaveStatus save_deserialize(Profile *p, const uint8_t *buf, size_t len);
SaveStatus save_load(Profile *p, const char *path);
SaveStatus save_write(const Profile *p, const char *path);
uint32_t   save_crc32(const uint8_t *data, size_t n);

#endif
