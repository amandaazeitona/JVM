#include <locale.h>
#include <wctype.h>
#include <ctype.h>
#include "validity.h"
#include "constantpool.h"
#include "utf8.h"
#include "readfunctions.h"

char check_access_flags_method(java_class* jc, uint16_t access_flags)
{
    if (access_flags & INV_METHOD_FLAG_MASK_ACCESS_FLAG)
    {
        jc->status = USE_OF_RSVD_METHOD_ACCESS_FLAGS;
        return 0;
    }

    if ((access_flags & ABSTRACT_ACCESS_FLAG) && (access_flags & (FINAL_ACCESS_FLAG | NATIVE_ACCESS_FLAG | PRIVATE_ACCESS_FLAG | STATIC_ACCESS_FLAG | STRICT_ACCESS_FLAG | SYNCHRONIZED_ACCESS_FLAG)))
    {
        jc->status = INV_ACCESS_FLAGS;
        return 0;
    }

    uint8_t count_access_modifier = 0;

    if (access_flags & PUBLIC_ACCESS_FLAG)
        count_access_modifier++;

    if (access_flags & PRIVATE_ACCESS_FLAG)
        count_access_modifier++;

    if (access_flags & PROTECTED_ACCESS_FLAG)
        count_access_modifier++;

    if (count_access_modifier > 1)
    {
        jc->status = INV_ACCESS_FLAGS;
        return 0;
    }

    return 1;
}

char check_access_flags_field(java_class* jc, uint16_t access_flags)
{
    if (access_flags & INV_FIELD_FLAG_MASK_ACCESS_FLAG)
    {
        jc->status = USE_OF_RSVD_FIELD_ACCESS_FLAGS;
        return 0;
    }

    if ((access_flags & ABSTRACT_ACCESS_FLAG) && (access_flags & FINAL_ACCESS_FLAG))
    {
        jc->status = INV_ACCESS_FLAGS;
        return 0;
    }

    uint16_t bit_mask_is_required = PUBLIC_ACCESS_FLAG | STATIC_ACCESS_FLAG | FINAL_ACCESS_FLAG;

    if ((jc->access_flags & INTERFACE_ACCESS_FLAG) &&
        ((access_flags & bit_mask_is_required) != bit_mask_is_required))
    {
        jc->status = INV_ACCESS_FLAGS;
        return 0;
    }

    uint8_t count_access_modifier = 0;

    if (access_flags & PUBLIC_ACCESS_FLAG)
        count_access_modifier++;

    if (access_flags & PRIVATE_ACCESS_FLAG)
        count_access_modifier++;

    if (access_flags & PROTECTED_ACCESS_FLAG)
        count_access_modifier++;

    if (count_access_modifier > 1)
    {
        jc->status = INV_ACCESS_FLAGS;
        return 0;
    }

    return 1;
}

char check_access_flags_and_class_idx(java_class* jc)
{
    if (jc->access_flags & INV_CLASS_FLAG_MASK_ACCESS_FLAG)
    {
        jc->status = USE_OF_RSVD_CLASS_ACCESS_FLAGS;
        return 0;
    }

    if (jc->access_flags & INTERFACE_ACCESS_FLAG)
    {
        if ((jc->access_flags & ABSTRACT_ACCESS_FLAG) == 0 ||
            (jc->access_flags & (FINAL_ACCESS_FLAG | SUPER_ACCESS_FLAG)))
        {
            jc->status = INV_ACCESS_FLAGS;
            return 0;
        }
    }

    if (!jc->this_class || jc->this_class >= jc->constant_pool_count ||
        jc->constant_pool[jc->this_class - 1].tag != CLASS_CONST)
    {
        jc->status = INV_THIS_CLASS_IDX;
        return 0;
    }

    if (jc->super_class >= jc->constant_pool_count ||
        (jc->super_class && jc->constant_pool[jc->super_class - 1].tag != CLASS_CONST))
    {
        jc->status = INV_SUPER_CLASS_IDX;
        return 0;
    }

    return 1;
}

char check_class_name_mathcing_with_file(java_class* jc, const char* class_file_path)
{
    int32_t i, begin = 0, end;
    constant_pool_info* cpi;

    for (i = 0; class_file_path[i] != '\0'; i++)
    {
        if (class_file_path[i] == '/' || class_file_path[i] == '\\')
            begin = i + 1;
        else if (class_file_path[i] == '.')
            break;
    }

    end = i;

    cpi = jc->constant_pool + jc->this_class - 1;
    cpi = jc->constant_pool + cpi->Class.name_index - 1;

    for (i = 0; i < cpi->Utf8.length; i++)
    {
        if (*(cpi->Utf8.bytes + i) == '/')
        {
            if (begin == 0)
                break;

            while (--begin > 0 && (class_file_path[begin - 1] != '/' || class_file_path[begin - 1] != '\\'));
        }
    }

    return compare_filepath_utf8(cpi->Utf8.bytes, cpi->Utf8.length, (uint8_t*)class_file_path + begin, end - begin);
}

char java_identifier_is_valid(uint8_t* utf8_bytes, int32_t utf8_length, uint8_t id_class_identifier)
{
    uint32_t utf8_char;
    uint8_t used_bytes;
    uint8_t first_character = 1;
    char is_valid = 1;

    if (*utf8_bytes == '[')
        return read_field_descriptor(utf8_bytes, utf8_length, 1) == utf8_length;

    while (utf8_length > 0)
    {
        used_bytes = next_char_utf8(utf8_bytes, utf8_length, &utf8_char);

        if (used_bytes == 0)
        {
            is_valid = 0;
            break;
        }

        if (isalpha(utf8_char) || utf8_char == '_' || utf8_char == '$' || (isdigit(utf8_char) && !first_character) || iswalpha(utf8_char) || (utf8_char == '/' && !first_character && id_class_identifier))
        {
            first_character = utf8_char == '/';
            utf8_length -= used_bytes;
            utf8_bytes += used_bytes;
        }
        else
        {
            is_valid = 0;
            break;
        }
    }

    return is_valid;
}

char is_valid_idx_utf8(java_class* jc, uint16_t index)
{
    return (jc->constant_pool + index - 1)->tag == UTF8_CONST;
}


char name_idx_is_valid(java_class* jc, uint16_t name_index, uint8_t id_class_identifier)
{
    constant_pool_info* entry = jc->constant_pool + name_index - 1;
    return entry->tag == UTF8_CONST && java_identifier_is_valid(entry->Utf8.bytes, entry->Utf8.length, id_class_identifier);
}

char method_name_idx_is_valid(java_class* jc, uint16_t name_index)
{
    constant_pool_info* entry = jc->constant_pool + name_index - 1;

    if (entry->tag != UTF8_CONST)
        return 0;

    if (compare_ascii_utf8(entry->Utf8.bytes, entry->Utf8.length, (uint8_t*)"<init>", 6) || compare_ascii_utf8(entry->Utf8.bytes, entry->Utf8.length, (uint8_t*)"<clinit>", 8))
        return 1;

    return java_identifier_is_valid(entry->Utf8.bytes, entry->Utf8.length, 0);
}

char check_class_idx(java_class* jc, uint16_t class_index)
{
    constant_pool_info* entry = jc->constant_pool + class_index - 1;

    if (entry->tag != CLASS_CONST)
    {
        jc->status = INV_CLASS_IDX;
        return 0;
    }

    return 1;
}

char check_idx_field_name_and_type(java_class* jc, uint16_t name_and_type_index)
{
    constant_pool_info* entry = jc->constant_pool + name_and_type_index - 1;

    if (entry->tag != NAMEANDTYPE_CONST || !name_idx_is_valid(jc, entry->NameAndType.name_index, 0))
    {
        jc->status = INV_NAME_IDX;
        return 0;
    }

    entry = jc->constant_pool + entry->NameAndType.descriptor_index - 1;

    if (entry->tag != UTF8_CONST || entry->Utf8.length == 0)
    {
        jc->status = INV_FIELD_DESC_IDX;
        return 0;
    }

    if (entry->Utf8.length != read_field_descriptor(entry->Utf8.bytes, entry->Utf8.length, 1))
    {
        jc->status = INV_FIELD_DESC_IDX;
        return 0;
    }

    return 1;
}

char check_idx_method_name_and_type(java_class* jc, uint16_t name_and_type_index)
{
    constant_pool_info* entry = jc->constant_pool + name_and_type_index - 1;

    if (entry->tag != NAMEANDTYPE_CONST || !method_name_idx_is_valid(jc, entry->NameAndType.name_index))
    {
        jc->status = INV_NAME_IDX;
        return 0;
    }

    entry = jc->constant_pool + entry->NameAndType.descriptor_index - 1;

    if (entry->tag != UTF8_CONST || entry->Utf8.length == 0)
    {
        jc->status = INV_METHOD_DESC_IDX;
        return 0;
    }

    if (entry->Utf8.length != read_method_descriptor(entry->Utf8.bytes, entry->Utf8.length, 1))
    {
        jc->status = INV_METHOD_DESC_IDX;
        return 0;
    }

    return 1;
}


char check_constant_pool_is_valid(java_class* jc)
{
    uint16_t i;
    char success = 1;
    char* prev_locale = setlocale(LC_CTYPE, "pt_BR.UTF-8");

    for (i = 0; success && i < jc->constant_pool_count - 1; i++)
    {
        constant_pool_info* entry = jc->constant_pool + i;

        switch(entry->tag)
        {
            case CLASS_CONST:

                if (!name_idx_is_valid(jc, entry->Class.name_index, 1))
                {
                    jc->status = INV_NAME_IDX;
                    success = 0;
                }

                break;

            case STRING_CONST:

                if (!is_valid_idx_utf8(jc, entry->String.string_index))
                {
                    jc->status = INV_STRING_IDX;
                    success = 0;
                }

                break;

            case METHODREF_CONST:
            case INTERFACEMETHODREF_CONST:

                if (!check_class_idx(jc, entry->Methodref.class_index) ||
                    !check_idx_method_name_and_type(jc, entry->Methodref.name_and_type_index))
                {
                    success = 0;
                }

                break;

            case FIELDREF_CONST:

                if (!check_class_idx(jc, entry->Fieldref.class_index) ||
                    !check_idx_field_name_and_type(jc, entry->Fieldref.name_and_type_index))
                {
                    success = 0;
                }

                break;

            case NAMEANDTYPE_CONST:

                if (!is_valid_idx_utf8(jc, entry->NameAndType.name_index) ||
                    !is_valid_idx_utf8(jc, entry->NameAndType.descriptor_index))
                {
                    jc->status = INV_NAME_AND_TYPE_IDX;
                    success = 0;
                }

                break;

            case DOUBLE_CONST:
            case LONG_CONST:
                i++;
                break;

            case FLOAT_CONST:
            case INT_CONST:
            case UTF8_CONST:
                break;

            default:
                break;
        }

        jc->validity_entries_checked = i + 1;
    }

    setlocale(LC_CTYPE, prev_locale);

    return success;
}
