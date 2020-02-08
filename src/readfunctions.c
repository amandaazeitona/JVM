#include <math.h>
#include "readfunctions.h"
#include "utf8.h"
#include "validity.h"

static const union {
    uint16_t value;
    uint8_t bytes[2];
} byte_order = {1};
#define LITTLE_ENDIAN_CONVENTION byte_order.bytes[1]

uint8_t read_4_byte_unsigned(java_class* jc, uint32_t* resultant_value)
{
    uint32_t u4 = 0;

    if (fread(&u4, sizeof(uint32_t), 1, jc->file) != 1)
        return 0;

    jc->total_bytes_read += 4;

    if (!LITTLE_ENDIAN_CONVENTION)
        u4 = (u4 >> 24) | ((u4 & 0xFF0000) >> 8) | ((u4 & 0xFF00) << 8) | ((u4 & 0xFF) << 24);

    if (resultant_value)
        *resultant_value = u4;

    return 1;
}

uint8_t read_2_byte_unsigned(java_class* jc, uint16_t* resultant_value)
{
    uint16_t u2 = 0;

    if (fread(&u2, sizeof(uint16_t), 1, jc->file) != 1)
        return 0;

    jc->total_bytes_read += 2;

    if (!LITTLE_ENDIAN_CONVENTION)
        u2 = (u2 >> 8) | ((u2 & 0xFF) << 8);

    if (resultant_value)
        *resultant_value = u2;

    return 1;
}

int32_t read_field_descriptor(uint8_t* utf8_bytes, int32_t utf8_length, char check_valid_identifier_for_class)
{
    int32_t total_bytes_read = 0;
    uint32_t utf8_char;
    uint8_t used_bytes;

    do
    {
        used_bytes = next_char_utf8(utf8_bytes, utf8_length, &utf8_char);

        if (used_bytes == 0)
            return 0;

        utf8_bytes += used_bytes;
        utf8_length -= used_bytes;
        total_bytes_read += used_bytes;

    } while (utf8_char == '[');

    switch (utf8_char)
    {
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z':
        break;
        case 'L':
        {
            uint8_t* id_start = utf8_bytes;
            int32_t id_length = 0;

            do {
                used_bytes = next_char_utf8(utf8_bytes, utf8_length, &utf8_char);

                if (used_bytes == 0)
                    return 0;

                utf8_bytes += used_bytes;
                utf8_length -= used_bytes;
                total_bytes_read += used_bytes;
                id_length += used_bytes;

            } while (utf8_char != ';');

            if (check_valid_identifier_for_class && !java_identifier_is_valid(id_start, id_length - 1, 1))
                return 0;

            break;
        }

        default:
            return 0;
    }

    return total_bytes_read;
}

int32_t read_method_descriptor(uint8_t* utf8_bytes, int32_t utf8_length, char check_valid_identifier_for_class)
{
    int32_t processed_bytes = 0;
    uint32_t utf8_char;
    uint8_t used_bytes;

    used_bytes = next_char_utf8(utf8_bytes, utf8_length, &utf8_char);

    if (used_bytes == 0 || utf8_char != '(')
        return 0;

    utf8_bytes += used_bytes;
    utf8_length -= used_bytes;
    processed_bytes += used_bytes;

    int32_t field_descriptor_length;

    do
    {
        field_descriptor_length = read_field_descriptor(utf8_bytes, utf8_length, check_valid_identifier_for_class);

        if (field_descriptor_length == 0)
        {
            used_bytes = next_char_utf8(utf8_bytes, utf8_length, &utf8_char);

            if (used_bytes == 0 || utf8_char != ')')
                return 0;

            utf8_bytes += used_bytes;
            utf8_length -= used_bytes;
            processed_bytes += used_bytes;

            break;
        }

        utf8_bytes += field_descriptor_length;
        utf8_length -= field_descriptor_length;
        processed_bytes += field_descriptor_length;

    } while (1);

    field_descriptor_length = read_field_descriptor(utf8_bytes, utf8_length, 1);

    if (field_descriptor_length == 0)
    {
        used_bytes = next_char_utf8(utf8_bytes, utf8_length, &utf8_char);

        if (used_bytes == 0 || utf8_char != 'V')
            return 0;

        utf8_length -= used_bytes;
        processed_bytes += used_bytes;
    }
    else
    {
        utf8_length -= field_descriptor_length;
        processed_bytes += field_descriptor_length;
    }

    return utf8_length == 0 ? processed_bytes : 0;
}

float get_float_from_uint32(uint32_t value)
{
    if (value == 0x7F800000UL)
        return INFINITY;

    if (value == 0xFF800000UL)
        return -INFINITY;

    if ((value >= 0x7F800000UL && value <= 0x7FFFFFFFUL) ||
        (value >= 0xFF800000UL && value <= 0xFFFFFFFFUL))
    {
        return NAN;
    }

    int32_t s = (value >> 31) == 0 ? 1 : -1;
    int32_t e = (value >> 23) & 0xFF;
    int32_t m = (e == 0) ?
        (value & 0x7FFFFFUL) << 1 :
        (value & 0x7FFFFFUL) | 0x800000;

    return s * (float)ldexp((double)m, e - 150);
}

double get_double_from_uint64(uint64_t value)
{
    if (value == 0x7FF0000000000000ULL)
        return INFINITY;

    if (value == 0xFFF0000000000000ULL)
        return -INFINITY;

    if ((value >= 0x7FF0000000000001ULL && value <= 0x7FFFFFFFFFFFFFFFULL) ||
        (value >= 0xFFF0000000000001ULL && value <= 0xFFFFFFFFFFFFFFFFULL))
    {
        return NAN;
    }

    int32_t s = (value >> 63) == 0 ? 1 : -1;
    int32_t e = (value >> 52) & 0x7FF;
    int64_t m = (e == 0) ?
        (value & 0xFFFFFFFFFFFFFULL) << 1 :
        (value & 0xFFFFFFFFFFFFFULL) | 0x10000000000000ULL;

    return s * ldexp((double)m, e - 1075);
}
