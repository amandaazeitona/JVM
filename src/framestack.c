#include "framestack.h"

frame* create_new_frame(java_class* jc, method_info* method) {
    frame* fr = (frame*)malloc(sizeof(frame));

    if (fr)
    {
        attribute_info* codeAttribute = get_attribute_using_type(method->attributes, method->attributes_count, ATTRIBUTE_Code);
        attr_code_info* code;

        if (codeAttribute)
        {
            code = (attr_code_info*)codeAttribute->info;
            fr->bytecode = code->code;
            fr->bytecode_length = code->code_length;

            if (code->max_locals > 0)
                fr->local_vars = (int32_t*)malloc(code->max_locals * sizeof(int32_t));
            else
                fr->local_vars = NULL;

#ifdef DEBUG
            fr->max_locals = code->max_locals;
#endif

        }
        else
        {
            fr->bytecode = NULL;
            fr->bytecode_length = 0;
            fr->local_vars = NULL;
        }

        fr->operands = NULL;
        fr->jc = jc;
        fr->PC = 0;
    }

    return fr;
}

void free_frame(frame* fr) {
    if (fr->local_vars)
        free(fr->local_vars);

    if (fr->operands)
        free_stack_operand(&fr->operands);

    free(fr);
}

uint8_t push_to_stack_frame(stack_frame** fs, frame* fr) {
    stack_frame* node = (stack_frame*)malloc(sizeof(stack_frame));

    if (node)
    {
        node->fr = fr;
        node->next = *fs;
        *fs = node;
    }

    return node != NULL;
}

uint8_t pop_from_stack_frame(stack_frame** fs, frame* outPtr) {
    stack_frame* node = *fs;

    if (node)
    {
        if (outPtr)
            *outPtr = *node->fr;

        *fs = node->next;
        free(node);
    }

    return node != NULL;
}

void free_stack_frame(stack_frame** fs) {
    stack_frame* node = *fs;
    stack_frame* tmp;

    while (node)
    {
        tmp = node;
        node = node->next;
        free_frame(tmp->fr);
        free(tmp);
    }

    *fs = NULL;
}
