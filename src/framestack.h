#ifndef STACKFRAME_H
#define STACKFRAME_H

typedef struct frame frame;
typedef struct stack_frame stack_frame;

#include "operandstack.h"
#include "attributes.h"

struct frame {
    java_class* jc;
    uint8_t return_count;
    uint32_t PC;
    uint32_t bytecode_length;
    uint8_t* bytecode;
    stack_operand* operands;
    int32_t* local_vars;

#ifdef DEBUG
    uint16_t max_locals;
#endif
};

struct stack_frame {
    frame* fr;
    struct stack_frame* next;
};

frame* create_new_frame(java_class*, method_info*);
uint8_t push_to_stack_frame(stack_frame**, frame*);
uint8_t pop_from_stack_frame(stack_frame**, frame*);
void free_frame(frame*);
void free_stack_frame(stack_frame**);

#endif
