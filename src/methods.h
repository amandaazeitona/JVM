#ifndef METHODS_H
#define METHODS_H

typedef struct method_info method_info;

#include <stdint.h>
#include "javaclass.h"
#include "attributes.h"

struct method_info {
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t access_flags;
    uint16_t attributes_count;
    attribute_info* attributes;
};

char read_method(java_class*, method_info*);
void free_method_attributes(method_info*);
void methods_print(java_class*);

method_info* get_matching_method(java_class*, const uint8_t*, int32_t,
        const uint8_t*, int32_t, uint16_t);

#endif
