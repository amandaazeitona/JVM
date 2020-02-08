#ifndef JAVACLASSFILE_H
#define JAVACLASSFILE_H

typedef struct java_class java_class;

#include <stdio.h>
#include <stdint.h>
#include "constantpool.h"
#include "attributes.h"
#include "fields.h"
#include "methods.h"

enum access_flags_types {
    CLASS_ACCESS_TYPE,
    FIELD_ACCESS_TYPE,
    METHOD_ACCESS_TYPE,
    INNERCLASS_ACCESS_TYPE
};

enum access_flags {

    PUBLIC_ACCESS_FLAG          = 0x0001,
    PRIVATE_ACCESS_FLAG         = 0x0002,
    PROTECTED_ACCESS_FLAG       = 0x0004,
    STATIC_ACCESS_FLAG          = 0x0008,
    FINAL_ACCESS_FLAG           = 0x0010,
    SUPER_ACCESS_FLAG           = 0x0020,
    SYNCHRONIZED_ACCESS_FLAG    = 0x0020,
    BRIDGE_ACCESS_FLAG          = 0x0040,
    VOLATILE_ACCESS_FLAG        = 0x0040,
    TRANSIENT_ACCESS_FLAG       = 0x0080,
    VARARGS_ACCESS_FLAG         = 0x0080,
    NATIVE_ACCESS_FLAG          = 0x0100,
    INTERFACE_ACCESS_FLAG       = 0x0200,
    ABSTRACT_ACCESS_FLAG        = 0x0400,
    STRICT_ACCESS_FLAG          = 0x0800,
    SYNTHETIC_ACCESS_FLAG       = 0x1000,

    INV_CLASS_FLAG_MASK_ACCESS_FLAG = ~(PUBLIC_ACCESS_FLAG | FINAL_ACCESS_FLAG | SUPER_ACCESS_FLAG | INTERFACE_ACCESS_FLAG | ABSTRACT_ACCESS_FLAG),

    INV_FIELD_FLAG_MASK_ACCESS_FLAG = ~(PUBLIC_ACCESS_FLAG | PRIVATE_ACCESS_FLAG | PROTECTED_ACCESS_FLAG | STATIC_ACCESS_FLAG | FINAL_ACCESS_FLAG |
                                    VOLATILE_ACCESS_FLAG | TRANSIENT_ACCESS_FLAG),

    INV_METHOD_FLAG_MASK_ACCESS_FLAG = ~(PUBLIC_ACCESS_FLAG | PRIVATE_ACCESS_FLAG | PROTECTED_ACCESS_FLAG | STATIC_ACCESS_FLAG | FINAL_ACCESS_FLAG |
                                     SYNCHRONIZED_ACCESS_FLAG | NATIVE_ACCESS_FLAG | ABSTRACT_ACCESS_FLAG | STRICT_ACCESS_FLAG |
                                     BRIDGE_ACCESS_FLAG | VARARGS_ACCESS_FLAG | SYNTHETIC_ACCESS_FLAG),

    INV_INNERCLASS_FLAG_MASK_ACCESS_FLAG = ~(PUBLIC_ACCESS_FLAG | PRIVATE_ACCESS_FLAG | PROTECTED_ACCESS_FLAG | STATIC_ACCESS_FLAG |
                                         FINAL_ACCESS_FLAG | INTERFACE_ACCESS_FLAG | ABSTRACT_ACCESS_FLAG)

};

enum java_class_status {
    CLASS_STA_OK,
    CLASS_STA_UNSPTD_VER,
    CLASS_STA_FILE_CN_BE_OPENED,
    CLASS_STA_INV_SIGN,
    MEM_ALLOC_FAILED,
    INV_CP_COUNT,
    UNXPTD_EOF,
    UNXPTD_EOF_READING_CP,
    UNXPTD_EOF_READING_UTF8,
    UNXPTD_EOF_READING_INTERFACES,
    UNXPTD_EOF_READING_ATTR_INFO,
    INV_UTF8_BYTES,
    INV_CP_INDEX,
    UNKNOWN_CP_TAG,

    INV_ACCESS_FLAGS,
    USE_OF_RSVD_CLASS_ACCESS_FLAGS,
    USE_OF_RSVD_METHOD_ACCESS_FLAGS,
    USE_OF_RSVD_FIELD_ACCESS_FLAGS,

    INV_THIS_CLASS_IDX,
    INV_SUPER_CLASS_IDX,
    INV_INTERFACE_IDX,

    INV_FIELD_DESC_IDX,
    INV_METHOD_DESC_IDX,
    INV_NAME_IDX,
    INV_STRING_IDX,
    INV_CLASS_IDX,
    INV_NAME_AND_TYPE_IDX,
    INV_JAVA_IDENTIFIER,

    ATTR_LEN_MISMATCH,
    ATTR_INV_CONST_VALUE_IDX,
    ATTR_INV_SRC_FILE_IDX,
    ATTR_INV_INNERCLASS_IDXS,
    ATTR_INV_EXC_CLASS_IDX,
    ATTR_INV_CODE_LEN,

    FILE_CONTAINS_UNXPTD_DATA
};

struct java_class {
  
    FILE* file;
    enum java_class_status status;
    uint8_t class_name_mismatch;

    uint16_t minor_version, major_version;
    uint16_t constant_pool_count;
    constant_pool_info* constant_pool;
    uint16_t access_flags;
    uint16_t this_class;
    uint16_t super_class;
    uint16_t interface_count;
    uint16_t* interfaces;
    uint16_t field_count;
    field_info* fields;
    uint16_t method_count;
    method_info* methods;
    uint16_t attribute_count;
    attribute_info* attributes;

    uint16_t static_field_count;
    uint16_t instance_field_count;

    uint32_t total_bytes_read;
    uint8_t last_tag_read;
    int32_t constant_pool_entries_read;
    int32_t interface_entries_read;
    int32_t field_entries_read;
    int32_t method_entries_read;
    int32_t attribute_entries_read;
    int32_t validity_entries_checked;

};

void open_class_file(java_class *, const char *);
void close_class_file(java_class *);
const char* decode_java_class_status(enum java_class_status);
void decode_access_flags(uint16_t, char *, int32_t, enum access_flags_types);

void print_class_file_debug_info(java_class *);
void print_class_file_info(java_class *);
#endif
