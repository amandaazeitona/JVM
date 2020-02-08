#ifndef VALIDITY_H
#define VALIDITY_H

#include "javaclass.h"

char check_access_flags_and_class_idx(java_class*);
char check_access_flags_field(java_class*, uint16_t);
char check_access_flags_method(java_class*, uint16_t);
char check_class_name_mathcing_with_file(java_class*, const char*);
char check_constant_pool_is_valid(java_class*);
char method_name_idx_is_valid(java_class*, uint16_t);
char name_idx_is_valid(java_class*, uint16_t, uint8_t);
char java_identifier_is_valid(uint8_t*, int32_t, uint8_t);

#endif
