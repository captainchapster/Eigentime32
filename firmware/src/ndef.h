/* NFC Type 5 tag content: our status URL out, a phone's time sync in. Portable C. */
#ifndef NDEF_H
#define NDEF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Builds the capability container, an NDEF TLV holding one URI record
 * ("https://" + url) and a terminator TLV. Returns bytes written, 0 if it doesn't fit. */
size_t ndef_build_uri(uint8_t *buf, size_t cap, uint16_t tag_bytes, const char *url);

/* Looks for a text record "ET32 T=<unix ms> Z=<utc offset minutes>" written by
 * the sync page. Returns true and fills the outputs when one is found. */
bool ndef_find_sync(const uint8_t *mem, size_t len, int64_t *unix_ms, int16_t *tz_min);

#endif
