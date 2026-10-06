/* save_legacy.h - converts save files of format versions 1-3 (see .c). */
#ifndef AD_SAVE_LEGACY_H
#define AD_SAVE_LEGACY_H

#include "save.h"

/* buf: the whole file (header + body + CRC), already validated. */
SaveStatus save_legacy_read(Profile *p, const uint8_t *buf, size_t len, int version);

#endif
