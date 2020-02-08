#include "readfunctions.h"
#include "javaclass.h"
#include "constantpool.h"
#include "utf8.h"
#include "validity.h"
#include <stdlib.h>

void open_class_file(java_class* jc, const char* path) {
    if (!jc)
        return;

    uint32_t u32;
    uint16_t u16;

    jc->file = fopen(path, "rb");
    jc->minor_version = jc->major_version = jc->constant_pool_count = 0;
    jc->constant_pool = NULL;
    jc->interfaces = NULL;
    jc->fields = NULL;
    jc->methods = NULL;
    jc->attributes = NULL;
    jc->status = CLASS_STA_OK;
    jc->class_name_mismatch = 0;

    jc->this_class = jc->super_class = jc->access_flags = 0;
    jc->attribute_count = jc->field_count = jc->method_count = jc->constant_pool_count = jc->interface_count = 0;

    jc->static_field_count = 0;
    jc->instance_field_count = 0;

    jc->last_tag_read = 0;
    jc->total_bytes_read = 0;
    jc->constant_pool_entries_read = 0;
    jc->attribute_entries_read = 0;
    jc->constant_pool_entries_read = 0;
    jc->interface_entries_read = 0;
    jc->field_entries_read = 0;
    jc->method_entries_read = 0;
    jc->validity_entries_checked = 0;

    if (!jc->file)
    {
        jc->status = CLASS_STA_FILE_CN_BE_OPENED;
        return;
    }

    if (!read_4_byte_unsigned(jc, &u32) || u32 != 0xCAFEBABE)
    {
        jc->status = CLASS_STA_INV_SIGN;
        return;
    }

    if (!read_2_byte_unsigned(jc, &jc->minor_version) ||
        !read_2_byte_unsigned(jc, &jc->major_version) ||
        !read_2_byte_unsigned(jc, &jc->constant_pool_count))
    {
        jc->status = UNXPTD_EOF;
        return;
    }

    if (jc->major_version < 45 || jc->major_version > 52)
    {
        jc->status = CLASS_STA_UNSPTD_VER;
        return;
    }

    if (jc->constant_pool_count == 0)
    {
        jc->status = INV_CP_COUNT;
        return;
    }

    if (jc->constant_pool_count > 1)
    {
        jc->constant_pool = (constant_pool_info*)malloc(sizeof(constant_pool_info) * (jc->constant_pool_count - 1));

        if (!jc->constant_pool)
        {
            jc->status = MEM_ALLOC_FAILED;
            return;
        }

        for (u16 = 0; u16 < jc->constant_pool_count - 1; u16++)
        {
            if (!constant_pool_entry_reading(jc, jc->constant_pool + u16))
            {
                jc->constant_pool_count = u16 + 1;
                return;
            }

            if (jc->constant_pool[u16].tag == DOUBLE_CONST ||
                jc->constant_pool[u16].tag == LONG_CONST)
            {
                u16++;
            }

            jc->constant_pool_entries_read++;
        }

        if (!check_constant_pool_is_valid(jc))
            return;
    }

    if (!read_2_byte_unsigned(jc, &jc->access_flags) ||
        !read_2_byte_unsigned(jc, &jc->this_class) ||
        !read_2_byte_unsigned(jc, &jc->super_class))
    {
        jc->status = UNXPTD_EOF;
        return;
    }

    if (!check_access_flags_and_class_idx(jc))
        return;

    if (!check_class_name_mathcing_with_file(jc, path))
        jc->class_name_mismatch = 1;

    if (!read_2_byte_unsigned(jc, &jc->interface_count))
    {
        jc->status = UNXPTD_EOF;
        return;
    }

    if (jc->interface_count > 0)
    {
        jc->interfaces = (uint16_t*)malloc(sizeof(uint16_t) * jc->interface_count);

        if (!jc->interfaces)
        {
            jc->status = MEM_ALLOC_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->interface_count; u32++)
        {
            if (!read_2_byte_unsigned(jc, &u16))
            {
                jc->status = UNXPTD_EOF_READING_INTERFACES;
                return;
            }

            if (u16 == 0 || jc->constant_pool[u16 - 1].tag != CLASS_CONST)
            {
                jc->status = INV_INTERFACE_IDX;
                return;
            }

            *(jc->interfaces + u32) = u16;
            jc->interface_entries_read++;
        }
    }

    if (!read_2_byte_unsigned(jc, &jc->field_count))
    {
        jc->status = UNXPTD_EOF;
        return;
    }

    if (jc->field_count > 0)
    {
        jc->fields = (field_info*)malloc(sizeof(field_info) * jc->field_count);

        if (!jc->fields)
        {
            jc->status = MEM_ALLOC_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->field_count; u32++)
        {
            field_info* field = jc->fields + u32;
            uint8_t isCat2;

            if (!fieald_read(jc, field))
            {
                jc->field_count = u32 + 1;
                return;
            }

            isCat2 = *jc->constant_pool[field->descriptor_index - 1].Utf8.bytes;
            isCat2 = isCat2 == 'J' || isCat2 == 'D';

            if (field->access_flags & STATIC_ACCESS_FLAG)
            {
                field->offset = jc->static_field_count++;
                jc->static_field_count += isCat2;
            }
            else
            {
                field->offset = jc->instance_field_count++;
                jc->instance_field_count += isCat2;
            }


            jc->field_entries_read++;
        }
    }

    if (!read_2_byte_unsigned(jc, &jc->method_count))
    {
        jc->status = UNXPTD_EOF;
        return;
    }

    if (jc->method_count > 0)
    {
        jc->methods = (method_info*)malloc(sizeof(method_info) * jc->method_count);

        if (!jc->methods)
        {
            jc->status = MEM_ALLOC_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->method_count; u32++)
        {
            if (!read_method(jc, jc->methods + u32))
            {
                jc->method_count = u32 + 1;
                return;
            }

            jc->method_entries_read++;
        }
    }

    if (!read_2_byte_unsigned(jc, &jc->attribute_count))
    {
        jc->status = UNXPTD_EOF;
        return;
    }

    if (jc->attribute_count > 0)
    {
        jc->attribute_entries_read = 0;
        jc->attributes = (attribute_info*)malloc(sizeof(attribute_info) * jc->attribute_count);

        if (!jc->attributes)
        {
            jc->status = MEM_ALLOC_FAILED;
            return;
        }

        for (u32 = 0; u32 < jc->attribute_count; u32++)
        {
            if (!attribute_read(jc, jc->attributes + u32))
            {
                jc->attribute_count = u32 + 1;
                return;
            }

            jc->attribute_entries_read++;
        }
    }

    if (fgetc(jc->file) != EOF)
    {
        jc->status = FILE_CONTAINS_UNXPTD_DATA;
    }
    else
    {
        fclose(jc->file);
        jc->file = NULL;
    }

}

void close_class_file(java_class* jc) {
    if (!jc)
        return;

    uint16_t i;

    if (jc->file)
    {
        fclose(jc->file);
        jc->file = NULL;
    }

    if (jc->interfaces)
    {
        free(jc->interfaces);
        jc->interfaces = NULL;
        jc->interface_count = 0;
    }

    if (jc->constant_pool)
    {
        for (i = 0; i < jc->constant_pool_count - 1; i++)
        {
            if (jc->constant_pool[i].tag == UTF8_CONST && jc->constant_pool[i].Utf8.bytes)
                free(jc->constant_pool[i].Utf8.bytes);
        }

        free(jc->constant_pool);
        jc->constant_pool = NULL;
        jc->constant_pool_count = 0;
    }

    if (jc->methods)
    {
        for (i = 0; i < jc->method_count; i++)
            free_method_attributes(jc->methods + i);

        free(jc->methods);
        jc->method_count = 0;
    }

    if (jc->fields)
    {
        for (i = 0; i < jc->field_count; i++)
            free_field_attributes(jc->fields + i);

        free(jc->fields);
        jc->field_count = 0;
    }

    if (jc->attributes)
    {
        for (i = 0; i < jc->attribute_count; i++)
            free_attribute_info(jc->attributes + i);

        free(jc->attributes);
        jc->attribute_count = 0;
    }
}

const char* decode_java_class_status(enum java_class_status status) {
    switch (status)
    {
        case CLASS_STA_OK: return "file is ok";
        case CLASS_STA_UNSPTD_VER: return "class file version isn't supported";
        case CLASS_STA_FILE_CN_BE_OPENED: return "class file couldn't be opened";
        case CLASS_STA_INV_SIGN: return "signature (0xCAFEBABE) mismatch";
        case MEM_ALLOC_FAILED: return "not enough memory";
        case INV_CP_COUNT: return "constant pool count should be at least 1";
        case UNXPTD_EOF: return "end of file found too soon";
        case UNXPTD_EOF_READING_CP: return "unexpected end of file while reading constant pool";
        case UNXPTD_EOF_READING_UTF8: return "unexpected end of file while reading UTF-8 stream";
        case UNXPTD_EOF_READING_INTERFACES: return "unexpected end of file while reading interfaces' index";
        case UNXPTD_EOF_READING_ATTR_INFO: return "unexpected end of file while reading attribute information";
        case INV_UTF8_BYTES: return "invalid UTF-8 encoding";
        case INV_CP_INDEX: return "invalid index for constant pool entry";
        case UNKNOWN_CP_TAG: return "unknown constant pool entry tag";
        case INV_ACCESS_FLAGS: return "invalid combination of access flags";

        case USE_OF_RSVD_CLASS_ACCESS_FLAGS: return "class access flags contains bits that should be zero";
        case USE_OF_RSVD_METHOD_ACCESS_FLAGS: return "method access flags contains bits that should be zero";
        case USE_OF_RSVD_FIELD_ACCESS_FLAGS: return "field access flags contains bits that should be zero";

        case INV_THIS_CLASS_IDX: return "\"this Class\" field doesn't point to valid class index";
        case INV_SUPER_CLASS_IDX: return "\"super class\" field doesn't point to valid class index";
        case INV_INTERFACE_IDX: return "interface index doesn't point to a valid class index";
        case INV_FIELD_DESC_IDX: return "field descriptor isn't valid";
        case INV_METHOD_DESC_IDX: return "method descriptor isn't valid";
        case INV_NAME_IDX: return "name_index doesn't point to a valid name";
        case INV_STRING_IDX: return "string_index doesn't point to a valid UTF-8 stream";
        case INV_CLASS_IDX: return "class_index doesn't point to a valid class name";
        case INV_NAME_AND_TYPE_IDX: return "(NameAndType) name_index or descriptor_index doesn't point to a valid UTF-8 stream";
        case INV_JAVA_IDENTIFIER: return "UTF-8 stream isn't a valid Java identifier";

        case ATTR_LEN_MISMATCH: return "Attribute length is different than expected";
        case ATTR_INV_CONST_VALUE_IDX: return "constantvalue_index isn't a valid constant value index";
        case ATTR_INV_SRC_FILE_IDX: return "sourcefile_index isn't a valid source file index";
        case ATTR_INV_INNERCLASS_IDXS: return "InnerClass has at least one invalid index";
        case ATTR_INV_EXC_CLASS_IDX: return "Exceptions has an index that doesn't point to a valid class";
        case ATTR_INV_CODE_LEN: return "Attribute code must have a length greater than 0 and less than 65536 bytes";

        case FILE_CONTAINS_UNXPTD_DATA: return "class file contains more data than expected, which wasn't processed";

        default:
            break;
    }

    return "unknown status";
}

void decode_access_flags(uint16_t flags, char* buffer, int32_t buffer_len, enum access_flags_types acctype) {
    uint32_t bytes = 0;
    char* buffer_ptr = buffer;
    const char* comma = ", ";
    const char* empty = "";

    #define DECODE_FLAG(flag, name) if (flags & flag) { \
        bytes = snprintf(buffer, buffer_len, "%s%s", bytes ? comma : empty, name); \
        buffer += bytes; \
        buffer_len -= bytes; }

    if (acctype == CLASS_ACCESS_TYPE)
    {
        DECODE_FLAG(SUPER_ACCESS_FLAG, "super")
    }

    if (acctype == CLASS_ACCESS_TYPE || acctype == METHOD_ACCESS_TYPE || acctype == INNERCLASS_ACCESS_TYPE)
    {
        DECODE_FLAG(ABSTRACT_ACCESS_FLAG, "abstract")
    }

    if (acctype == METHOD_ACCESS_TYPE)
    {
        DECODE_FLAG(SYNCHRONIZED_ACCESS_FLAG, "synchronized")
        DECODE_FLAG(NATIVE_ACCESS_FLAG, "native")
        DECODE_FLAG(STRICT_ACCESS_FLAG, "strict")
    }

    if (acctype == FIELD_ACCESS_TYPE)
    {
        DECODE_FLAG(TRANSIENT_ACCESS_FLAG, "transient")
        DECODE_FLAG(VOLATILE_ACCESS_FLAG, "volatile")
    }

    DECODE_FLAG(PUBLIC_ACCESS_FLAG, "public")

    if (acctype == FIELD_ACCESS_TYPE || acctype == METHOD_ACCESS_TYPE || acctype == INNERCLASS_ACCESS_TYPE)
    {
        DECODE_FLAG(PRIVATE_ACCESS_FLAG, "private")
        DECODE_FLAG(PROTECTED_ACCESS_FLAG, "protected")
    }

    if (acctype == FIELD_ACCESS_TYPE || acctype == METHOD_ACCESS_TYPE || acctype == INNERCLASS_ACCESS_TYPE)
    {
        DECODE_FLAG(STATIC_ACCESS_FLAG, "static")
    }

    DECODE_FLAG(FINAL_ACCESS_FLAG, "final")

    if (acctype == CLASS_ACCESS_TYPE || acctype == INNERCLASS_ACCESS_TYPE)
    {
        DECODE_FLAG(INTERFACE_ACCESS_FLAG, "interface")
    }

    if (buffer == buffer_ptr)
        snprintf(buffer, buffer_len, "no flags");

    #undef DECODE_FLAG
}

void print_class_file_debug_info(java_class* jc) {
    if (jc->class_name_mismatch)
        printf("Warning: class name and file path don't match.\n");

    if (jc->constant_pool_entries_read != jc->constant_pool_count)
    {
        printf("Failed to read constant pool entry at index #%d\n", jc->constant_pool_entries_read);
    }
    else if (jc->validity_entries_checked != jc->constant_pool_count)
    {
        printf("Failed to check constant pool validity. Entry at index #%d is not valid.\n", jc->validity_entries_checked);
    }
    else if (jc->interface_entries_read != jc->interface_count)
    {
        printf("Failed to read interface at index #%d\n", jc->interface_entries_read);
    }
    else if (jc->field_entries_read != jc->field_count)
    {
        printf("Failed to read field at index #%d\n", jc->field_entries_read);

        if (jc->attribute_entries_read != -1)
            printf("Failed to read its attribute at index %d.\n", jc->attribute_entries_read);
    }
    else if (jc->method_entries_read != jc->method_count)
    {
        printf("Failed to read method at index #%d\n", jc->method_entries_read);

        if (jc->attribute_entries_read != -1)
            printf("Failed to read its attribute at index %d.\n", jc->attribute_entries_read);
    }
    else if (jc->attribute_entries_read != jc->attribute_count)
    {
        printf("Failed to read attribute at index #%d\n", jc->attribute_entries_read);

    }

    printf("File status code: %d\nStatus description: %s.\n", jc->status, decode_java_class_status(jc->status));
    printf("Number of bytes read: %d\n", jc->total_bytes_read);
}

void print_class_file_info(java_class* jc) {
    char buffer[256];
    constant_pool_info* cp;
    uint16_t u16;

    if (jc->class_name_mismatch)
        printf("---- Warning ----\n\nClass name and file path don't match.\nReading will proceed anyway.\n\n");

    printf("---- General Information ----\n\n");

    printf("Version:\t\t%u.%u (Major.minor)", jc->major_version, jc->minor_version);

    if (jc->major_version >= 45 && jc->major_version <= 52)
        printf(" [jdk version 1.%d]", jc->major_version - 44);

    printf("\nConstant pool count:\t%u\n", jc->constant_pool_count);

    decode_access_flags(jc->access_flags, buffer, sizeof(buffer), CLASS_ACCESS_TYPE);
    printf("Access flags:\t\t0x%.4X [%s]\n", jc->access_flags, buffer);

    cp = jc->constant_pool + jc->this_class - 1;
    cp = jc->constant_pool + cp->Class.name_index - 1;
    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
    printf("This class:\t\tcp index #%u <%s>\n", jc->this_class, buffer);

    cp = jc->constant_pool + jc->super_class - 1;
    cp = jc->constant_pool + cp->Class.name_index - 1;
    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
    printf("Super class:\t\tcp index #%u <%s>\n", jc->super_class, buffer);

    printf("Interfaces count:\t%u\n", jc->interface_count);
    printf("Fields count:\t\t%u\n", jc->field_count);
    printf("Methods count:\t\t%u\n", jc->method_count);
    printf("Attributes count:\t%u\n", jc->attribute_count);

    constant_pool_print(jc);

    if (jc->interface_count > 0)
    {
        printf("\n---- Interfaces implemented by the class ----\n\n");

        for (u16 = 0; u16 < jc->interface_count; u16++)
        {
            cp = jc->constant_pool + *(jc->interfaces + u16) - 1;
            cp = jc->constant_pool + cp->Class.name_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("\tInterface #%u: #%u <%s>\n", u16 + 1, *(jc->interfaces + u16), buffer);
        }
    }

    print_all_fields(jc);
    methods_print(jc);
    attributes_print_all(jc);
}
