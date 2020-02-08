#ifndef CONSTANTPOOL_H
#define CONSTANTPOOL_H

typedef struct constant_pool_info constant_pool_info;

#include <stdint.h>
#include "javaclass.h"

struct constant_pool_info {
    uint8_t tag;

    union {

        struct {
            uint16_t name_index;
        } Class;

        struct {
            uint16_t string_index;
        } String;

        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        } Fieldref;

        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        } Methodref;

        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        } InterfaceMethodref;

        struct {
            uint16_t name_index;
            uint16_t descriptor_index;
        } NameAndType;

        struct {
            uint32_t value;
        } Integer;

        struct {
            uint32_t bytes;
        } Float;

        struct {
            uint32_t high;
            uint32_t low;
        } Long;

        struct {
            uint32_t high;
            uint32_t low;
        } Double;

        struct {
            uint16_t length;
            uint8_t* bytes;
        } Utf8;

    };

};

enum constant_pool_tag {
    UTF8_CONST = 1,
    INT_CONST = 3,
    FLOAT_CONST = 4,
    LONG_CONST = 5,
    DOUBLE_CONST = 6,
    CLASS_CONST = 7,
    STRING_CONST = 8,
    FIELDREF_CONST = 9,
    METHODREF_CONST = 10,
    INTERFACEMETHODREF_CONST = 11,
    NAMEANDTYPE_CONST = 12,
    METHODHANDLE_CONST = 15,
    METHODTYPE_CONST = 16,
    INVOKEDYNAMIC_CONST = 18
};

const char* tag_decoding(uint8_t);
char constant_pool_entry_reading(java_class*, constant_pool_info*);
void constant_pool_print(java_class*);

#endif
