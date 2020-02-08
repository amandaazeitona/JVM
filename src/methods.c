#include "methods.h"
#include "readfunctions.h"
#include "validity.h"
#include "utf8.h"
#include "string.h"
#include <stdlib.h>

char read_method(java_class* jc, method_info* entry)
{
    entry->attributes = NULL;
    jc->attribute_entries_read = -1;

    if (!read_2_byte_unsigned(jc, &entry->access_flags) || !read_2_byte_unsigned(jc, &entry->name_index) || !read_2_byte_unsigned(jc, &entry->descriptor_index) || !read_2_byte_unsigned(jc, &entry->attributes_count))
    {
        jc->status = UNXPTD_EOF;
        return 0;
    }

    if (!check_access_flags_method(jc, entry->access_flags))
        return 0;

    if (entry->name_index == 0 ||
        entry->name_index >= jc->constant_pool_count ||
        !method_name_idx_is_valid(jc, entry->name_index))
    {
        jc->status = INV_NAME_IDX;
        return 0;
    }

    constant_pool_info* cpi = jc->constant_pool + entry->descriptor_index - 1;

    if (entry->descriptor_index == 0 ||
        entry->descriptor_index >= jc->constant_pool_count ||
        cpi->tag != UTF8_CONST ||
        cpi->Utf8.length != read_method_descriptor(cpi->Utf8.bytes, cpi->Utf8.length, 1))
    {
        jc->status = INV_FIELD_DESC_IDX;
        return 0;
    }

    if (entry->attributes_count > 0)
    {
        entry->attributes = (attribute_info*)malloc(sizeof(attribute_info) * entry->attributes_count);

        if (!entry->attributes)
        {
            jc->status = MEM_ALLOC_FAILED;
            return 0;
        }

        uint16_t i;

        jc->attribute_entries_read = 0;

        for (i = 0; i < entry->attributes_count; i++)
        {
            if (!attribute_read(jc, entry->attributes + i))
            {
                entry->attributes_count = i + 1;
                return 0;
            }

            jc->attribute_entries_read++;
        }
    }

    return 1;
}

void free_method_attributes(method_info* entry)
{
    uint32_t i;

    if (entry->attributes != NULL)
    {
        for (i = 0; i < entry->attributes_count; i++)
            free_attribute_info(entry->attributes + i);

        free(entry->attributes);

        entry->attributes_count = 0;
        entry->attributes = NULL;
    }
}

void methods_print(java_class* jc)
{

    uint16_t u16, att_index;
    char buffer[48];
    constant_pool_info* cpi;
    method_info* mi;
    attribute_info* atti;

    if (jc->method_count > 0)
    {
        printf("\n---- Methods ----\n");

        for (u16 = 0; u16 < jc->method_count; u16++)
        {
            mi = jc->methods + u16;

            cpi = jc->constant_pool + mi->name_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

            printf("\n\tMethod #%u:\n\n", u16 + 1);
            printf("\t\tname_index:        #%u <%s>\n", mi->name_index, buffer);

            cpi = jc->constant_pool + mi->descriptor_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
            printf("\t\tdescriptor_index:  #%u <%s>\n", mi->descriptor_index, buffer);

            decode_access_flags(mi->access_flags, buffer, sizeof(buffer), METHOD_ACCESS_TYPE);
            printf("\t\taccess_flags:      0x%.4X [%s]\n", mi->access_flags, buffer);

            printf("\t\tattribute_count:   %u\n", mi->attributes_count);

            if (mi->attributes_count > 0)
            {
                for (att_index = 0; att_index < mi->attributes_count; att_index++)
                {
                    atti = mi->attributes + att_index;
                    cpi = jc->constant_pool + atti->name_index - 1;
                    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

                    printf("\n\t\tMethod Attribute #%u - %s:\n", att_index + 1, buffer);
                    attribute_print(jc, atti, 3);
                }
            }

        }
    }
}

method_info* get_matching_method(java_class* jc, const uint8_t* name, int32_t name_len, const uint8_t* descriptor, int32_t descriptor_len, uint16_t flag_mask)
{
    method_info* method = jc->methods;
    constant_pool_info* cpi;
    uint16_t index;

    for (index = jc->method_count; index > 0; index--, method++)
    {
        if ((method->access_flags & flag_mask) != flag_mask)
            continue;

        cpi = jc->constant_pool + method->name_index - 1;
        if (!compare_ascii_utf8(cpi->Utf8.bytes, cpi->Utf8.length, name, name_len))
            continue;

        cpi = jc->constant_pool + method->descriptor_index - 1;
        
        if (!compare_ascii_utf8(cpi->Utf8.bytes, cpi->Utf8.length, descriptor, descriptor_len))
            continue;


        return method;
    }

    return NULL;
}
