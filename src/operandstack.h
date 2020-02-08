#ifndef STACK_OPERAND
#define STACK_OPERAND

typedef struct stack_operand stack_operand;

#include <stdint.h>
#include <stdlib.h>

typedef enum operand_type {
    INT_OP,
    FLOAT_OP,
    LONG_OP,
    DOUBLE_OP,
    NULL_OP,
    REF_OP,
    RETADD_OP
} operand_type;

struct stack_operand {
    int32_t value;
    operand_type type;
    stack_operand* next;
};

uint8_t push_to_stack_operand(stack_operand**, int32_t, operand_type);
uint8_t pop_from_stack_operand(stack_operand**, int32_t*, operand_type*);
void free_stack_operand(stack_operand**);

#endif
