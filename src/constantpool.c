#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include "readfunctions.h"
#include "constantpool.h"
#include "utf8.h"

char read_constant_pool_class(java_class* jc, constant_pool_info* entry)
{
    if (!read_2_byte_unsigned(jc, &entry->Class.name_index))
    {
        jc->status = UNXPTD_EOF_READING_CP;
        return 0;
    }

    if (entry->Class.name_index == 0 ||
        entry->Class.name_index >= jc->constant_pool_count)
    {
        jc->status = INV_CP_INDEX;
        return 0;
    }

    return 1;
}

char read_constant_pool_fieldref(java_class* jc, constant_pool_info* entry)
{
    if (!read_2_byte_unsigned(jc, &entry->Fieldref.class_index))
    {
        jc->status = UNXPTD_EOF_READING_CP;
        return 0;
    }

    if (entry->Fieldref.class_index == 0 ||
        entry->Fieldref.class_index >= jc->constant_pool_count)
    {
        jc->status = INV_CP_INDEX;
        return 0;
    }

    if (!read_2_byte_unsigned(jc, &entry->Fieldref.name_and_type_index))
    {
        jc->status = UNXPTD_EOF_READING_CP;
        return 0;
    }

    if (entry->Fieldref.name_and_type_index == 0 ||
        entry->Fieldref.name_and_type_index >= jc->constant_pool_count)
    {
        jc->status = INV_CP_INDEX;
        return 0;
    }

    return 1;
}

char read_constant_pool_int(java_class* jc, constant_pool_info* entry)
{
    if (!read_4_byte_unsigned(jc, &entry->Integer.value))
    {
        jc->status = UNXPTD_EOF_READING_CP;
        return 0;
    }

    return 1;
}

char read_constant_pool_long(java_class* jc, constant_pool_info* entry)
{
    if (!read_4_byte_unsigned(jc, &entry->Long.high))
    {
        jc->status = UNXPTD_EOF_READING_CP;
        return 0;
    }

    if (!read_4_byte_unsigned(jc, &entry->Long.low))
    {
        jc->status = UNXPTD_EOF_READING_CP;
        return 0;
    }

    return 1;
}

char read_constant_pool_utf8(java_class* jc, constant_pool_info* entry)
{
    if (!read_2_byte_unsigned(jc, &entry->Utf8.length))
    {
        jc->status = UNXPTD_EOF_READING_CP;
        return 0;
    }

    if (entry->Utf8.length > 0)
    {
        entry->Utf8.bytes = (uint8_t*)malloc(entry->Utf8.length);

        if (!entry->Utf8.bytes)
        {
            jc->status = MEM_ALLOC_FAILED;
            return 0;
        }

        uint16_t i;
        uint8_t* bytes = entry->Utf8.bytes;

        for (i = 0; i < entry->Utf8.length; i++)
        {
            int byte = fgetc(jc->file);

            if (byte == EOF)
            {
                jc->status = UNXPTD_EOF_READING_UTF8;
                return 0;
            }

            jc->total_bytes_read++;

            if (byte == 0 || (byte >= 0xF0))
            {
                jc->status = INV_UTF8_BYTES;
                return 0;
            }

            *bytes++ = (uint8_t)byte;
        }
    }
    else
    {
        entry->Utf8.bytes = NULL;
    }

    return 1;
}

char constant_pool_entry_reading(java_class* jc, constant_pool_info* entry)
{
    int byte = fgetc(jc->file);

    if (byte == EOF)
    {
        jc->status = UNXPTD_EOF_READING_CP;
        entry->tag = 0xFF;
        return 0;
    }

    entry->tag = (uint8_t)byte;

    jc->total_bytes_read++;
    jc->last_tag_read = entry->tag;

    switch(entry->tag)
    {
        case METHODTYPE_CONST:
        case CLASS_CONST:
        case STRING_CONST:
            return read_constant_pool_class(jc, entry);

        case UTF8_CONST:
            return read_constant_pool_utf8(jc, entry);

        case INVOKEDYNAMIC_CONST:
        case FIELDREF_CONST:
        case METHODREF_CONST:
        case INTERFACEMETHODREF_CONST:
        case NAMEANDTYPE_CONST:
            return read_constant_pool_fieldref(jc, entry);

        case INT_CONST:
        case FLOAT_CONST:
            return read_constant_pool_int(jc, entry);

        case LONG_CONST:
        case DOUBLE_CONST:
            return read_constant_pool_long(jc, entry);

        case METHODHANDLE_CONST:

            if (!read_2_byte_unsigned(jc, NULL) || fgetc(jc->file) == EOF)
            {
                jc->status = UNXPTD_EOF_READING_CP;
                return 0;
            }

            jc->total_bytes_read++;

            break;

        default:
            jc->status = UNKNOWN_CP_TAG;
            break;
    }

    return 0;
}

const char* tag_decoding(uint8_t tag)
{
    switch(tag)
    {
        case CLASS_CONST: return "Class";
        case DOUBLE_CONST: return "Double";
        case FIELDREF_CONST: return "Fieldref";
        case FLOAT_CONST: return "Float";
        case INT_CONST: return "Integer";
        case INTERFACEMETHODREF_CONST: return "InterfaceMethodref";
        case LONG_CONST: return "Long";
        case METHODREF_CONST: return "Methodref";
        case NAMEANDTYPE_CONST: return "NameAndType";
        case STRING_CONST: return "String";
        case UTF8_CONST: return "Utf8";
        default:
            break;
    }

    return "Unknown Tag";
}

void print_constant_pool_entry(java_class* jc, constant_pool_info* entry)
{
    char buffer[48];
    uint32_t u32;
    constant_pool_info* cpi;

    switch(entry->tag)
    {

        case UTF8_CONST:

            u32 = translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), entry->Utf8.bytes, entry->Utf8.length);
            printf("\t\tByte Count: %u\n", entry->Utf8.length);
            printf("\t\tLength: %u\n", u32);
            printf("\t\tASCII Characters: %s", buffer);

            if (u32 != entry->Utf8.length)
                printf("\n\t\tUTF-8 Characters: %.*s", (int)entry->Utf8.length, entry->Utf8.bytes);

            break;

        case STRING_CONST:
            cpi = jc->constant_pool + entry->String.string_index - 1;
            u32 = translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tstring_index: #%u\n", entry->String.string_index);
            printf("\t\tString Length: %u\n", u32);
            printf("\t\tASCII: %s", buffer);

            if (u32 != cpi->Utf8.length)
                printf("\n\t\tUTF-8: %.*s", (int)cpi->Utf8.length, cpi->Utf8.bytes);

            break;

        case FIELDREF_CONST:
        case METHODREF_CONST:
        case INTERFACEMETHODREF_CONST:

            cpi = jc->constant_pool + entry->Fieldref.class_index - 1;
            cpi = jc->constant_pool + cpi->Class.name_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tclass_index: #%u <%s>\n", entry->Fieldref.class_index, buffer);
            printf("\t\tname_and_type_index: #%u\n", entry->Fieldref.name_and_type_index);

            cpi = jc->constant_pool + entry->Fieldref.name_and_type_index - 1;
            u32 = cpi->NameAndType.name_index;
            cpi = jc->constant_pool + u32 - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tname_and_type.name_index: #%u <%s>\n", u32, buffer);

            cpi = jc->constant_pool + entry->Fieldref.name_and_type_index - 1;
            u32 = cpi->NameAndType.descriptor_index;
            cpi = jc->constant_pool + u32 - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tname_and_type.descriptor_index: #%u <%s>", u32, buffer);

            break;

        case CLASS_CONST:

            cpi = jc->constant_pool + entry->Class.name_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tname_index: #%u <%s>", entry->Class.name_index, buffer);
            break;

        case NAMEANDTYPE_CONST:

            cpi = jc->constant_pool + entry->NameAndType.name_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tname_index: #%u <%s>\n", entry->NameAndType.name_index, buffer);

            cpi = jc->constant_pool + entry->NameAndType.descriptor_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tdescriptor_index: #%u <%s>", entry->NameAndType.descriptor_index, buffer);
            break;

        case INT_CONST:
            printf("\t\tBytes: 0x%08X\n", entry->Integer.value);
            printf("\t\tValue: %d", (int32_t)entry->Integer.value);
            break;

        case LONG_CONST:
            printf("\t\tHigh Bytes: 0x%08X\n", entry->Long.high);
            printf("\t\tLow  Bytes: 0x%08X\n", entry->Long.low);
            printf("\t\tLong Value: %" PRId64"", ((int64_t)entry->Long.high << 32) | entry->Long.low);
            break;

        case FLOAT_CONST:
            printf("\t\tBytes: 0x%08X\n", entry->Float.bytes);
            printf("\t\tValue: %e", get_float_from_uint32(entry->Float.bytes));
            break;

        case DOUBLE_CONST:
            printf("\t\tHigh Bytes:   0x%08X\n", entry->Double.high);
            printf("\t\tLow  Bytes:   0x%08X\n", entry->Double.low);
            printf("\t\tDouble Value: %e", get_double_from_uint64((uint64_t)entry->Double.high << 32 | entry->Double.low));
            break;

        default:
            break;
    }

    printf("\n");
}

void constant_pool_print(java_class* jc)
{
    uint16_t u16;
    constant_pool_info* cp;

    if (jc->constant_pool_count > 1)
    {
        printf("\n---- Constant Pool ----\n");

        for (u16 = 0; u16 < jc->constant_pool_count - 1; u16++)
        {
            cp = jc->constant_pool + u16;
            printf("\n\t#%u: %s\n", u16 + 1, tag_decoding(cp->tag));
            print_constant_pool_entry(jc, cp);

            if (cp->tag == DOUBLE_CONST || cp->tag == LONG_CONST)
                u16++;
        }
    }
}
