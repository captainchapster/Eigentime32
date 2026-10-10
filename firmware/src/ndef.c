#include "ndef.h"

#include <string.h>

size_t ndef_build_uri(uint8_t *buf, size_t cap, uint16_t tag_bytes, const char *url)
{
    size_t ulen = strlen(url);
    size_t payload = 1 + ulen;           /* URI identifier code + rest */
    size_t rec = 3 + 1 + payload;        /* header, type len, payload len (SR), type 'U' */
    size_t need = 4 + 2 + rec + 1;
    if (payload > 255 || rec > 254 || need > cap)
        return 0;
    size_t i = 0;
    buf[i++] = 0xE1;                     /* CC: magic, version 1.0, read/write */
    buf[i++] = 0x40;
    buf[i++] = (uint8_t)(tag_bytes / 8);
    buf[i++] = 0x00;
    buf[i++] = 0x03;                     /* NDEF message TLV */
    buf[i++] = (uint8_t)rec;
    buf[i++] = 0xD1;                     /* MB ME SR, TNF well-known */
    buf[i++] = 1;
    buf[i++] = (uint8_t)payload;
    buf[i++] = 'U';
    buf[i++] = 0x04;                     /* "https://" */
    memcpy(&buf[i], url, ulen);
    i += ulen;
    buf[i++] = 0xFE;                     /* terminator TLV */
    return i;
}

static bool parse_int(const char **p, const char *end, int64_t *out)
{
    const char *s = *p;
    bool neg = false;
    if (s < end && (*s == '-' || *s == '+')) {
        neg = *s == '-';
        s++;
    }
    if (s >= end || *s < '0' || *s > '9')
        return false;
    int64_t v = 0;
    while (s < end && *s >= '0' && *s <= '9')
        v = v * 10 + (*s++ - '0');
    *out = neg ? -v : v;
    *p = s;
    return true;
}

static bool parse_sync_text(const char *t, size_t n, int64_t *ms, int16_t *tz)
{
    const char *end = t + n;
    if (n < 5 || memcmp(t, "ET32 ", 5) != 0)
        return false;
    bool have_t = false, have_z = false;
    const char *p = t + 5;
    while (p < end) {
        if (*p == ' ') {
            p++;
            continue;
        }
        if (end - p >= 2 && p[1] == '=') {
            char key = p[0];
            int64_t v;
            p += 2;
            if (!parse_int(&p, end, &v))
                return false;
            if (key == 'T') {
                *ms = v;
                have_t = true;
            } else if (key == 'Z') {
                if (v < -14 * 60 || v > 14 * 60)
                    return false;
                *tz = (int16_t)v;
                have_z = true;
            }
        } else {
            p++;
        }
    }
    return have_t && have_z;
}

bool ndef_find_sync(const uint8_t *mem, size_t len, int64_t *unix_ms, int16_t *tz_min)
{
    if (len < 8 || (mem[0] != 0xE1 && mem[0] != 0xE2))
        return false;
    size_t i = mem[2] ? 4 : 8;           /* 8-byte CC when the size byte is 0 */
    while (i + 2 <= len) {
        uint8_t tag = mem[i];
        if (tag == 0x00) {               /* NULL TLV */
            i++;
            continue;
        }
        if (tag == 0xFE)
            return false;
        size_t l = mem[i + 1], hdr = 2;
        if (l == 0xFF) {
            if (i + 4 > len)
                return false;
            l = (size_t)mem[i + 2] << 8 | mem[i + 3];
            hdr = 4;
        }
        size_t start = i + hdr, stop = start + l;
        if (stop > len)
            return false;
        if (tag == 0x03) {
            size_t r = start;
            while (r + 3 <= stop) {
                uint8_t h = mem[r];
                bool sr = h & 0x10, il = h & 0x08;
                uint8_t tnf = h & 0x07;
                size_t q = r + 1;
                uint8_t tlen = mem[q++];
                size_t plen;
                if (sr) {
                    plen = mem[q++];
                } else {
                    if (q + 4 > stop)
                        return false;
                    plen = (size_t)mem[q] << 24 | (size_t)mem[q + 1] << 16 |
                           (size_t)mem[q + 2] << 8 | mem[q + 3];
                    q += 4;
                }
                size_t idlen = il ? mem[q++] : 0;
                const uint8_t *type = &mem[q];
                q += tlen + idlen;
                if (q + plen > stop)
                    return false;
                const uint8_t *pl = &mem[q];
                if (tnf == 1 && tlen == 1 && type[0] == 'T' && plen >= 1) {
                    size_t lang = pl[0] & 0x3F;
                    if (1 + lang <= plen &&
                        parse_sync_text((const char *)pl + 1 + lang, plen - 1 - lang, unix_ms, tz_min))
                        return true;
                }
                if (h & 0x40)            /* ME: last record */
                    break;
                r = q + plen;
            }
        }
        i = stop;
    }
    return false;
}
