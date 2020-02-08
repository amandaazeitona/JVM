#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>
#include "jvm.h"
#include "framestack.h"

typedef uint8_t (*instruction_fun)(interpreter_module *, frame *);

instruction_fun fetchOpcodeFunction(uint8_t);
#endif
