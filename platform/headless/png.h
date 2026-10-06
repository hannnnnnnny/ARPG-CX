/* png.h - minimal dependency-free PNG writer (stored deflate blocks). Used
 * by the headless runner to save screenshots of the RGB565 framebuffer. */
#ifndef BR_PNG_H
#define BR_PNG_H

#include <stdint.h>
#include <stdbool.h>

bool png_write_rgb565(const char *path, const uint16_t *px, int w, int h);

#endif
