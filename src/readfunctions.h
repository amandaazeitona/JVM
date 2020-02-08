#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <stdint.h>
#include <stdio.h>
#include "javaclass.h"
#include "constantpool.h"

uint8_t read_4_byte_unsigned(struct java_class*, uint32_t*);
uint8_t read_2_byte_unsigned(struct java_class*, uint16_t*);
int32_t read_field_descriptor(uint8_t*, int32_t, char);
int32_t read_method_descriptor(uint8_t*, int32_t, char);
float get_float_from_uint32(uint32_t);
double get_double_from_uint64(uint64_t);

#endif

