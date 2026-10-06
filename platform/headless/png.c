#include "png.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t crc_table[256];

static void crc_init(void)
{
    uint32_t n, k, c;
    for (n = 0; n < 256; n++) {
        c = n;
        for (k = 0; k < 8; k++)
            c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
}

static uint32_t crc_update(uint32_t c, const uint8_t *p, size_t n)
{
    while (n--)
        c = crc_table[(c ^ *p++) & 0xFF] ^ (c >> 8);
    return c;
}

static void be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static bool chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len)
{
    uint8_t hdr[8], tail[4];
    uint32_t c;
    be32(hdr, len);
    memcpy(hdr + 4, type, 4);
    c = crc_update(0xFFFFFFFFu, hdr + 4, 4);
    c = crc_update(c, data, len);
    be32(tail, ~c);
    return fwrite(hdr, 1, 8, f) == 8 && (len == 0 || fwrite(data, 1, len, f) == len) && fwrite(tail, 1, 4, f) == 4;
}

/* Raw scanlines: filter byte 0 + RGB triplets. */
static uint8_t *make_raw(const uint16_t *px, int w, int h, size_t *out_len)
{
    size_t stride = (size_t)w * 3 + 1;
    uint8_t *raw = malloc(stride * (size_t)h);
    int x, y;
    if (!raw)
        return NULL;
    for (y = 0; y < h; y++) {
        uint8_t *r = raw + stride * (size_t)y;
        r[0] = 0;
        for (x = 0; x < w; x++) {
            uint16_t c = px[y * w + x];
            r[1 + x * 3] = (uint8_t)(((c >> 11) & 31) * 255 / 31);
            r[2 + x * 3] = (uint8_t)(((c >> 5) & 63) * 255 / 63);
            r[3 + x * 3] = (uint8_t)((c & 31) * 255 / 31);
        }
    }
    *out_len = stride * (size_t)h;
    return raw;
}

/* zlib stream made of uncompressed deflate blocks (<= 65535 bytes each). */
static uint8_t *make_zlib(const uint8_t *raw, size_t n, size_t *out_len)
{
    size_t blocks = n / 65535 + 1, pos = 0, o = 0;
    uint8_t *z = malloc(2 + n + blocks * 5 + 4);
    uint32_t a = 1, b = 0;
    size_t i;
    if (!z)
        return NULL;
    z[o++] = 0x78;
    z[o++] = 0x01;
    do {
        size_t len = n - pos > 65535 ? 65535 : n - pos;
        z[o++] = (uint8_t)(pos + len >= n ? 1 : 0);
        z[o++] = (uint8_t)len;
        z[o++] = (uint8_t)(len >> 8);
        z[o++] = (uint8_t)~len;
        z[o++] = (uint8_t)(~len >> 8);
        memcpy(z + o, raw + pos, len);
        o += len;
        pos += len;
    } while (pos < n);
    for (i = 0; i < n; i++) {
        a = (a + raw[i]) % 65521;
        b = (b + a) % 65521;
    }
    be32(z + o, (b << 16) | a);
    *out_len = o + 4;
    return z;
}

bool png_write_rgb565(const char *path, const uint16_t *px, int w, int h)
{
    static const uint8_t sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    uint8_t ihdr[13] = {0};
    size_t raw_len = 0, z_len = 0;
    uint8_t *raw, *z;
    FILE *f;
    bool ok;

    crc_init();
    raw = make_raw(px, w, h, &raw_len);
    z = raw ? make_zlib(raw, raw_len, &z_len) : NULL;
    free(raw);
    if (!z || !(f = fopen(path, "wb"))) {
        free(z);
        return false;
    }
    be32(ihdr, (uint32_t)w);
    be32(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8; /* bit depth */
    ihdr[9] = 2; /* truecolour RGB */
    ok = fwrite(sig, 1, 8, f) == 8 && chunk(f, "IHDR", ihdr, 13)
      && chunk(f, "IDAT", z, (uint32_t)z_len) && chunk(f, "IEND", NULL, 0);
    ok = (fclose(f) == 0) && ok;
    free(z);
    return ok;
}
