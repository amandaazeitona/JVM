#include "utf8.h"

#define SINGLE_BYTE_MASK  0x80
#define SINGLE_BYTE_VALUE 0
#define DOUBLE_BYTE_MASK  0xE0
#define DOUBLE_BYTE_VALUE 0xC0
#define TRIPLE_BYTE_MASK  0xF0
#define TRIPLE_BYTE_VALUE 0xE0
#define FOLLOW_BYTE_MASK  0xC0
#define FOLLOW_BYTE_VALUE 0x80

uint8_t next_char_utf8(const uint8_t* utf8_bytes, int32_t utf8_len, uint32_t* outCharacter)
{
    if (utf8_len <= 0)
        return 0;

    uint32_t utf8_char;
    uint8_t used_bytes;

    if ((*utf8_bytes & SINGLE_BYTE_MASK) == SINGLE_BYTE_VALUE)
    {
        utf8_char = *utf8_bytes;
        used_bytes = 1;
    }
    else if (utf8_len >= 2 && ((*utf8_bytes & DOUBLE_BYTE_MASK) == DOUBLE_BYTE_VALUE) && ((*(utf8_bytes + 1) & FOLLOW_BYTE_MASK) == FOLLOW_BYTE_VALUE))
    {
        utf8_char = ((*utf8_bytes & 0x1F) << 6) + (*(utf8_bytes + 1) & 0x3F);
        used_bytes = 2;
    }
    else if (utf8_len >= 3 && ((*utf8_bytes & TRIPLE_BYTE_MASK) == TRIPLE_BYTE_VALUE) && ((*(utf8_bytes + 1) & FOLLOW_BYTE_MASK) == FOLLOW_BYTE_VALUE) && ((*(utf8_bytes + 2) & FOLLOW_BYTE_MASK) == FOLLOW_BYTE_VALUE))
    {
        utf8_char = ((*utf8_bytes & 0xF) << 12) + ((*(utf8_bytes + 1) & 0x3F) << 6) + (*(utf8_bytes + 2) & 0x3F);
        used_bytes = 3;
    }
    else
    {
        return 0;
    }

    if (outCharacter)
        *outCharacter = utf8_char;

    return used_bytes;
}

char compare_ascii_utf8(const uint8_t* utf8_bytes, int32_t utf8_len, const uint8_t* ascii_bytes, int32_t ascii_len)
{
    uint32_t utf8_char;
    uint8_t bytes_used;

    while (utf8_len > 0 && ascii_len > 0)
    {
        bytes_used = next_char_utf8(utf8_bytes, utf8_len, &utf8_char);

        if (bytes_used == 0 || utf8_char > 127 || (uint8_t)utf8_char != *ascii_bytes)
            return 0;

        utf8_bytes += bytes_used;
        utf8_len -= bytes_used;
        ascii_bytes++;
        ascii_len--;
    }

    return ascii_len == utf8_len;
}

char compare_utf8(const uint8_t* utf8A_bytes, int32_t utf8A_len, const uint8_t* utf8B_bytes, int32_t utf8B_len)
{
    if (utf8A_len != utf8B_len)
        return 0;

    int32_t i;

    for (i = 0; i < utf8A_len; i++)
    {
        if (utf8A_bytes[i] != utf8B_bytes[i])
            return 0;
    }

    return 1;
}

char compare_filepath_utf8(const uint8_t* utf8A_bytes, int32_t utf8A_len, const uint8_t* utf8B_bytes, int32_t utf8B_len)
{
    uint32_t utf8_charA, utf8_charB;
    uint8_t bytes_used;

    while (utf8A_len > 0 && utf8B_len > 0)
    {

        bytes_used = next_char_utf8(utf8A_bytes, utf8A_len, &utf8_charA);

        if (bytes_used == 0)
            return 0;

        utf8A_bytes += bytes_used;
        utf8A_len -= bytes_used;

        bytes_used = next_char_utf8(utf8B_bytes, utf8B_len, &utf8_charB);

        if (bytes_used == 0)
            return 0;

        utf8B_bytes += bytes_used;
        utf8B_len -= bytes_used;

        if ( (utf8_charA == utf8_charB) || (utf8_charA == '/' && utf8_charB == '\\') || (utf8_charA == '\\' && utf8_charB == '/'))
        {
            if (utf8_charA == '/' || utf8_charA == '\\')
            {
                while (utf8A_len > 0)
                {
                    bytes_used = next_char_utf8(utf8A_bytes, utf8A_len, &utf8_charA);

                    if (bytes_used == 0)
                        return 0;

                    if (utf8_charA != '/' && utf8_charA != '\\')
                        break;

                    utf8A_bytes += bytes_used;
                    utf8A_len -= bytes_used;
                }
            }

            if (utf8_charB == '/' || utf8_charB == '\\')
            {
                while (utf8B_len > 0)
                {
                    bytes_used = next_char_utf8(utf8B_bytes, utf8B_len, &utf8_charB);

                    if (bytes_used == 0)
                        return 0;

                    if (utf8_charB != '/' && utf8_charB != '\\')
                        break;

                    utf8B_bytes += bytes_used;
                    utf8B_len -= bytes_used;
                }
            }

            continue;
        }

        return 0;
    }

    return utf8A_len == utf8B_len;
}

uint32_t translate_from_utf8_to_ascii(uint8_t* out_buffer, int32_t buffer_length, const uint8_t* utf8_bytes, int32_t utf8_len)
{
    uint32_t writter_chars = 0;
    uint32_t utf8_char;
    uint8_t bytes_used;

    while (buffer_length > 1 && utf8_len > 0)
    {
        bytes_used = next_char_utf8(utf8_bytes, utf8_len, &utf8_char);

        if (bytes_used == 0)
            break;

        *out_buffer++ = utf8_char > 127 ? '?' : (uint8_t)utf8_char;
        buffer_length--;
        utf8_bytes += bytes_used;
        utf8_len -= bytes_used;
        writter_chars++;
    }

    if (buffer_length > 0)
        *out_buffer = '\0';

    return writter_chars;
}

uint32_t utf8_string_length(const uint8_t* utf8_bytes, int32_t utf8_len)
{
    uint32_t length = 0;
    uint8_t bytes_used;

    while (utf8_len > 0)
    {
        bytes_used = next_char_utf8(utf8_bytes, utf8_len, 0);

        if (bytes_used == 0)
            break;

        length++;
        utf8_len -= bytes_used;
        utf8_bytes += bytes_used;
    }

    return length;
}
