#ifndef FIELDS_H
#define FIELDS_H

typedef struct field_info field_info;

#include <stdint.h>
#include "javaclass.h"
#include "attributes.h"

struct field_info {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    attribute_info* attributes;
    uint16_t offset;
};

char fieald_read(java_class*, field_info*);
void free_field_attributes(field_info*);
void print_all_fields(java_class*);
field_info* get_maching_field(java_class*, const uint8_t*, int32_t,
        const uint8_t*, int32_t, uint16_t);

#endif
