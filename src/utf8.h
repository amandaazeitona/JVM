#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

#define PRINT_UTF8(x) x->Utf8.length, x->Utf8.bytes

#define UTF8(x) x->Utf8.bytes, x->Utf8.length

uint8_t next_char_utf8(const uint8_t*, int32_t, uint32_t*);
char compare_ascii_utf8(const uint8_t*, int32_t, const uint8_t*, int32_t);
char compare_utf8(const uint8_t*, int32_t, const uint8_t*, int32_t);
char compare_filepath_utf8(const uint8_t*, int32_t, const uint8_t*, int32_t);
uint32_t translate_from_utf8_to_ascii(uint8_t*, int32_t, const uint8_t*,
        int32_t);
uint32_t utf8_string_length(const uint8_t*, int32_t);

#endif
