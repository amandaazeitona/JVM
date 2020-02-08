#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

typedef struct attribute_info attribute_info;

#include <stdint.h>
#include "javaclass.h"

struct attribute_info {
    uint32_t length;
    uint16_t name_index;
    uint8_t attr_type;
    void* info;
};

enum attribute_type {
    ATTRIBUTE_Unknown = 0,
    ATTRIBUTE_ConstantValue,
    ATTRIBUTE_SourceFile,
    ATTRIBUTE_InnerClasses,
    ATTRIBUTE_Code,
    ATTRIBUTE_LineNumberTable,
    ATTRIBUTE_Exceptions,
    ATTRIBUTE_Deprecated
};

typedef struct {
    uint16_t sourcefile_index;
} attr_sourcefile_info;

typedef struct {
    uint16_t constantvalue_index;
} atrt_const_value_info;

typedef struct {
    uint16_t inner_class_index;
    uint16_t outer_class_index;
    uint16_t inner_class_name_index;
    uint16_t inner_class_access_flags;
} inner_class_info;

typedef struct {
    uint16_t number_of_classes;
    inner_class_info* inner_classes;
} attr_inner_classes_info;

typedef struct {
    uint16_t start_pc;
    uint16_t line_number;
} line_number_table_entry;

typedef struct {
    uint16_t line_number_table_length;
    line_number_table_entry* line_number_table;
} attr_line_number_table_info;

typedef struct {
    uint16_t start_pc;
    uint16_t end_pc;
    uint16_t handler_pc;
    uint16_t catch_type;
} exception_table_entry;

typedef struct {
    uint16_t max_stack;
    uint16_t max_locals;
    uint32_t code_length;
    uint8_t* code;
    uint16_t exception_table_length;
    exception_table_entry* exception_table;
    uint16_t attributes_count;
    attribute_info* attributes;
} attr_code_info;

typedef struct {
    uint16_t number_of_exceptions;
    uint16_t* exception_index_table;
} attr_exceptions_info;

char attribute_read(java_class *, attribute_info *);
void free_attribute_info(attribute_info *);
void attribute_print(java_class *, attribute_info *, int);
void attributes_print_all(java_class *);
attribute_info* get_attribute_using_type(attribute_info *, uint16_t,
                                         enum attribute_type);

#endif
