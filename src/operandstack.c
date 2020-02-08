#include "operandstack.h"

uint8_t push_to_stack_operand(stack_operand** os, int32_t value, enum operand_type type) {
    stack_operand* node = (stack_operand*)malloc(sizeof(stack_operand));

    if (node) {
        node->value = value;
        node->type = type;
        node->next = *os;
        *os = node;
    }

    return node != NULL;
}

uint8_t pop_from_stack_operand(stack_operand** os, int32_t* outPtr, enum operand_type* outType) {
    stack_operand* node = *os;

    if (node) {
        if (outPtr)
            *outPtr = node->value;

        if (outType)
            *outType = node->type;

        *os = node->next;
        free(node);
    }

    return node != NULL;
}

void free_stack_operand(stack_operand** os) {
    stack_operand* node = *os;
    stack_operand* tmp;

    while (node) {
        tmp = node;
        node = node->next;
        free(tmp);
    }

    *os = NULL;
}
