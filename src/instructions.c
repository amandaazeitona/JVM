#include "instructions.h"
#include "utf8.h"
#include "jvm.h"
#include "natives.h"
#include <math.h>

#define NEXT_BYTE (*(fr->bytecode + fr->PC++))
#define HIWORD(x) ((int32_t)(x >> 32))
#define LOWORD(x) ((int32_t)(x & 0xFFFFFFFFll))

uint8_t instfunc_nop(interpreter_module* jvm, frame* fr)
{
    return 1;
}

uint8_t instfunc_aconst_null(interpreter_module* jvm, frame* fr)
{
    if (!push_to_stack_operand(&fr->operands, 0, REF_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

#define DECLR_CONST_CAT_1_FAMILY(instructionprefix, value, type) \
    uint8_t instfunc_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        if (!push_to_stack_operand(&fr->operands, value, type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

#define DECLR_CONST_CAT_2_FAMILY(instructionprefix, highvalue, lowvalue, type) \
    uint8_t instfunc_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        if (!push_to_stack_operand(&fr->operands, highvalue, type) || \
            !push_to_stack_operand(&fr->operands, lowvalue,  type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_CONST_CAT_1_FAMILY(iconst_m1, -1, INT_OP)
DECLR_CONST_CAT_1_FAMILY(iconst_0, 0, INT_OP)
DECLR_CONST_CAT_1_FAMILY(iconst_1, 1, INT_OP)
DECLR_CONST_CAT_1_FAMILY(iconst_2, 2, INT_OP)
DECLR_CONST_CAT_1_FAMILY(iconst_3, 3, INT_OP)
DECLR_CONST_CAT_1_FAMILY(iconst_4, 4, INT_OP)
DECLR_CONST_CAT_1_FAMILY(iconst_5, 5, INT_OP)

DECLR_CONST_CAT_2_FAMILY(lconst_0, 0, 0, LONG_OP)
DECLR_CONST_CAT_2_FAMILY(lconst_1, 0, 1, LONG_OP)

DECLR_CONST_CAT_1_FAMILY(fconst_0, 0x00000000, FLOAT_OP)
DECLR_CONST_CAT_1_FAMILY(fconst_1, 0x3F800000, FLOAT_OP)
DECLR_CONST_CAT_1_FAMILY(fconst_2, 0x40000000, FLOAT_OP)

DECLR_CONST_CAT_2_FAMILY(dconst_0, 0x00000000, 0x00000000, DOUBLE_OP)
DECLR_CONST_CAT_2_FAMILY(dconst_1, 0x3FF00000, 0x00000000, DOUBLE_OP)


uint8_t instfunc_bipush(interpreter_module* jvm, frame* fr)
{
    if (!push_to_stack_operand(&fr->operands, (int8_t)NEXT_BYTE, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_sipush(interpreter_module* jvm, frame* fr)
{
    int16_t immediate = (int16_t)NEXT_BYTE;
    immediate <<= 8;
    immediate |= NEXT_BYTE;

    if (!push_to_stack_operand(&fr->operands, immediate, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ldc(interpreter_module* jvm, frame* fr)
{
    uint32_t value = (uint32_t)NEXT_BYTE;

    enum operand_type type;

    constant_pool_info* cpi = fr->jc->constant_pool + value - 1;

    switch (cpi->tag)
    {
        case FLOAT_CONST:
            value = cpi->Float.bytes;
            type = FLOAT_OP;
            break;

        case INT_CONST:
            value = cpi->Integer.value;
            type = INT_OP;
            break;

        case STRING_CONST:
        {
            cpi = fr->jc->constant_pool + cpi->String.string_index - 1;

            reference* str = create_new_string(jvm, UTF8(cpi));

            if (!str)
            {
                jvm->status = OUT_OF_MEMORY;
                return 0;
            }

            value = (int32_t)str;
            type = REF_OP;
            break;
        }

        case CLASS_CONST:
        {
            cpi = fr->jc->constant_pool + cpi->Class.name_index - 1;

            loaded_classes* loadedClass;

            if (!class_handler(jvm, UTF8(cpi), &loadedClass))
            {
                return 0;
            }

            reference* obj = create_new_class_instance(jvm, loadedClass);

            if (!obj)
            {
                jvm->status = OUT_OF_MEMORY;
                return 0;
            }

            value = (int32_t)obj;
            type = REF_OP;
            break;
        }

        case METHODREF_CONST:

            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;

        default:
            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;
    }

    if (!push_to_stack_operand(&fr->operands, value, type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ldc_w(interpreter_module* jvm, frame* fr)
{
    uint32_t value = (uint32_t)NEXT_BYTE;
    value <<= 8;
    value |= NEXT_BYTE;

    enum operand_type type;

    constant_pool_info* cpi = fr->jc->constant_pool + value - 1;

    switch (cpi->tag)
    {
        case FLOAT_CONST:
            value = cpi->Float.bytes;
            type = FLOAT_OP;
            break;

        case INT_CONST:
            value = cpi->Integer.value;
            type = INT_OP;
            break;

        case STRING_CONST:
        {
            cpi = fr->jc->constant_pool + cpi->String.string_index - 1;

            reference* str = create_new_string(jvm, UTF8(cpi));

            if (!str)
            {
                jvm->status = OUT_OF_MEMORY;
                return 0;
            }

            value = (int32_t)str;
            type = REF_OP;
            break;
        }

        case CLASS_CONST:
        {
            cpi = fr->jc->constant_pool + cpi->Class.name_index - 1;

            loaded_classes* loadedClass;

            if (!class_handler(jvm, UTF8(cpi), &loadedClass))
            {
                return 0;
            }

            reference* obj = create_new_class_instance(jvm, loadedClass);

            if (!obj)
            {
                jvm->status = OUT_OF_MEMORY;
                return 0;
            }

            value = (int32_t)obj;
            type = REF_OP;
            break;
        }

        case METHODREF_CONST:

            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;

        default:
            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;
    }

    if (!push_to_stack_operand(&fr->operands, (int32_t)value, type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ldc2_w(interpreter_module* jvm, frame* fr)
{
    uint32_t highvalue;
    uint32_t lowvalue = (uint32_t)NEXT_BYTE;
    lowvalue <<= 8;
    lowvalue |= NEXT_BYTE;

    enum operand_type type;

    constant_pool_info* cpi = fr->jc->constant_pool + lowvalue - 1;

    switch (cpi->tag)
    {
        case LONG_CONST:
            highvalue = cpi->Long.high;
            lowvalue = cpi->Long.low;
            type = LONG_OP;
            break;

        case DOUBLE_CONST:
            highvalue = cpi->Double.high;
            lowvalue = cpi->Double.low;
            type = DOUBLE_OP;
            break;

        default:
            return 0;
    }

    if (!push_to_stack_operand(&fr->operands, highvalue, type) ||
        !push_to_stack_operand(&fr->operands, lowvalue,  type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

#define DECLR_LOAD_CAT_1_FAMILY(instructionprefix, type) \
    uint8_t instfunc_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        if (!push_to_stack_operand(&fr->operands, *(fr->local_vars + NEXT_BYTE), type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

#define DECLR_LOAD_CAT_2_FAMILY(instructionprefix, type) \
    uint8_t instfunc_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        uint8_t index = NEXT_BYTE; \
        if (!push_to_stack_operand(&fr->operands, *(fr->local_vars + index), type) || \
            !push_to_stack_operand(&fr->operands, *(fr->local_vars + index + 1), type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

#define DECLR_WIDE_LOAD_CAT_1_FAMILY(instructionprefix, type) \
    uint8_t instfunc_wide_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        uint16_t index = NEXT_BYTE; \
        index = (index << 8) | NEXT_BYTE; \
        if (!push_to_stack_operand(&fr->operands, *(fr->local_vars + index), type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

#define DECLR_WIDE_LOAD_CAT_2_FAMILY(instructionprefix, type) \
    uint8_t instfunc_wide_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        uint16_t index = NEXT_BYTE; \
        index = (index << 8) | NEXT_BYTE; \
        if (!push_to_stack_operand(&fr->operands, *(fr->local_vars + index), type) || \
            !push_to_stack_operand(&fr->operands, *(fr->local_vars + index + 1), type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_LOAD_CAT_1_FAMILY(iload, INT_OP)
DECLR_LOAD_CAT_2_FAMILY(lload, LONG_OP)
DECLR_LOAD_CAT_1_FAMILY(fload, FLOAT_OP)
DECLR_LOAD_CAT_2_FAMILY(dload, DOUBLE_OP)
DECLR_LOAD_CAT_1_FAMILY(aload, REF_OP)

DECLR_WIDE_LOAD_CAT_1_FAMILY(iload, INT_OP)
DECLR_WIDE_LOAD_CAT_2_FAMILY(lload, LONG_OP)
DECLR_WIDE_LOAD_CAT_1_FAMILY(fload, FLOAT_OP)
DECLR_WIDE_LOAD_CAT_2_FAMILY(dload, DOUBLE_OP)
DECLR_WIDE_LOAD_CAT_1_FAMILY(aload, REF_OP)

#define DECLR_CAT_1_LOAD_N_FAMILY(instructionprefix, value, type) \
    uint8_t instfunc_##instructionprefix##_##value(interpreter_module* jvm, frame* fr) \
    { \
        if (!push_to_stack_operand(&fr->operands, *(fr->local_vars + value), type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

#define DECLR_CAT_2_LOAD_N_FAMILY(instructionprefix, value, type) \
    uint8_t instfunc_##instructionprefix##_##value(interpreter_module* jvm, frame* fr) \
    { \
        if (!push_to_stack_operand(&fr->operands, *(fr->local_vars + value), type) || \
            !push_to_stack_operand(&fr->operands, *(fr->local_vars + value + 1), type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_CAT_1_LOAD_N_FAMILY(iload, 0, INT_OP)
DECLR_CAT_1_LOAD_N_FAMILY(iload, 1, INT_OP)
DECLR_CAT_1_LOAD_N_FAMILY(iload, 2, INT_OP)
DECLR_CAT_1_LOAD_N_FAMILY(iload, 3, INT_OP)

DECLR_CAT_2_LOAD_N_FAMILY(lload, 0, LONG_OP)
DECLR_CAT_2_LOAD_N_FAMILY(lload, 1, LONG_OP)
DECLR_CAT_2_LOAD_N_FAMILY(lload, 2, LONG_OP)
DECLR_CAT_2_LOAD_N_FAMILY(lload, 3, LONG_OP)

DECLR_CAT_1_LOAD_N_FAMILY(fload, 0, FLOAT_OP)
DECLR_CAT_1_LOAD_N_FAMILY(fload, 1, FLOAT_OP)
DECLR_CAT_1_LOAD_N_FAMILY(fload, 2, FLOAT_OP)
DECLR_CAT_1_LOAD_N_FAMILY(fload, 3, FLOAT_OP)

DECLR_CAT_2_LOAD_N_FAMILY(dload, 0, DOUBLE_OP)
DECLR_CAT_2_LOAD_N_FAMILY(dload, 1, DOUBLE_OP)
DECLR_CAT_2_LOAD_N_FAMILY(dload, 2, DOUBLE_OP)
DECLR_CAT_2_LOAD_N_FAMILY(dload, 3, DOUBLE_OP)

DECLR_CAT_1_LOAD_N_FAMILY(aload, 0, REF_OP)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 1, REF_OP)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 2, REF_OP)
DECLR_CAT_1_LOAD_N_FAMILY(aload, 3, REF_OP)

#define DECLR_ALOAD_CAT_1_FAMILY(instructionname, type, op_type) \
    uint8_t instfunc_##instructionname(interpreter_module* jvm, frame* fr) \
    { \
        int32_t index; \
        int32_t arrayref; \
        reference* obj; \
        pop_from_stack_operand(&fr->operands, &index, NULL); \
        pop_from_stack_operand(&fr->operands, &arrayref, NULL); \
        obj = (reference*)arrayref; \
        if (obj == NULL) \
        { \
            \
            DEBUG_REPORT_ERROR_INSTRUCTION \
            return 0; \
        } \
        if (index < 0 || (uint32_t)index >= obj->arr.length) \
        { \
            \
            DEBUG_REPORT_ERROR_INSTRUCTION \
            return 0; \
        } \
        type* ptr = (type*)obj->arr.data; \
        if (!push_to_stack_operand(&fr->operands, ptr[index], op_type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

#define DECLR_ALOAD_CAT_2_FAMILY(instructionname, type, op_type) \
    uint8_t instfunc_##instructionname(interpreter_module* jvm, frame* fr) \
    { \
        int32_t index; \
        int32_t arrayref; \
        reference* obj; \
        pop_from_stack_operand(&fr->operands, &index, NULL); \
        pop_from_stack_operand(&fr->operands, &arrayref, NULL); \
        obj = (reference*)arrayref; \
        if (obj == NULL) \
        { \
            \
            DEBUG_REPORT_ERROR_INSTRUCTION \
            return 0; \
        } \
        if (index < 0 || (uint32_t)index >= obj->arr.length) \
        { \
            \
            DEBUG_REPORT_ERROR_INSTRUCTION \
            return 0; \
        } \
        type* ptr = (type*)obj->arr.data; \
        if (!push_to_stack_operand(&fr->operands, HIWORD(ptr[index]), op_type) || \
            !push_to_stack_operand(&fr->operands, LOWORD(ptr[index]), op_type)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_ALOAD_CAT_1_FAMILY(iaload, int32_t, INT_OP)
DECLR_ALOAD_CAT_2_FAMILY(laload, int64_t, INT_OP)
DECLR_ALOAD_CAT_1_FAMILY(faload, int32_t, FLOAT_OP)
DECLR_ALOAD_CAT_2_FAMILY(daload, int64_t, INT_OP)
DECLR_ALOAD_CAT_1_FAMILY(baload, int8_t, INT_OP)
DECLR_ALOAD_CAT_1_FAMILY(saload, int16_t, INT_OP)
DECLR_ALOAD_CAT_1_FAMILY(caload, int16_t, INT_OP)

uint8_t instfunc_aaload(interpreter_module* jvm, frame* fr)
{
    int32_t index;
    int32_t arrayref;
    reference* obj;

    pop_from_stack_operand(&fr->operands, &index, NULL);
    pop_from_stack_operand(&fr->operands, &arrayref, NULL);

    obj = (reference*)arrayref;

    if (obj == NULL)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    if (index < 0 || (uint32_t)index >= obj->oar.length)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    reference** ptr = (reference**)obj->oar.elements;

    if (!push_to_stack_operand(&fr->operands, (int32_t)ptr[index], REF_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

#define DECLR_STORE_CAT_1_FAMILY(instructionprefix) \
    uint8_t instfunc_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        int32_t operand; \
        pop_from_stack_operand(&fr->operands, &operand, NULL); \
        *(fr->local_vars + NEXT_BYTE) = operand; \
        return 1; \
    }

#define DECLR_WIDE_STORE_CAT_1_FAMILY(instructionprefix) \
    uint8_t instfunc_wide_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        uint16_t index = NEXT_BYTE; \
        index = (index << 8) | NEXT_BYTE; \
        int32_t operand; \
        pop_from_stack_operand(&fr->operands, &operand, NULL); \
        *(fr->local_vars + index) = operand; \
        return 1; \
    }

#define DECLR_STORE_CAT_2_FAMILY(instructionprefix) \
    uint8_t instfunc_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        uint8_t index = NEXT_BYTE; \
        int32_t highoperand; \
        int32_t lowoperand; \
        pop_from_stack_operand(&fr->operands, &lowoperand, NULL); \
        pop_from_stack_operand(&fr->operands, &highoperand, NULL); \
        *(fr->local_vars + index) = highoperand; \
        *(fr->local_vars + index + 1) = lowoperand; \
        return 1; \
    }

#define DECLR_WIDE_STORE_CAT_2_FAMILY(instructionprefix) \
    uint8_t instfunc_wide_##instructionprefix(interpreter_module* jvm, frame* fr) \
    { \
        uint16_t index = NEXT_BYTE; \
        index = (index << 8) | NEXT_BYTE; \
        int32_t highoperand; \
        int32_t lowoperand; \
        pop_from_stack_operand(&fr->operands, &lowoperand, NULL); \
        pop_from_stack_operand(&fr->operands, &highoperand, NULL); \
        *(fr->local_vars + index) = highoperand; \
        *(fr->local_vars + index + 1) = lowoperand; \
        return 1; \
    }

DECLR_STORE_CAT_1_FAMILY(istore)
DECLR_STORE_CAT_2_FAMILY(lstore)
DECLR_STORE_CAT_1_FAMILY(fstore)
DECLR_STORE_CAT_2_FAMILY(dstore)
DECLR_STORE_CAT_1_FAMILY(astore)

DECLR_WIDE_STORE_CAT_1_FAMILY(istore)
DECLR_WIDE_STORE_CAT_2_FAMILY(lstore)
DECLR_WIDE_STORE_CAT_1_FAMILY(fstore)
DECLR_WIDE_STORE_CAT_2_FAMILY(dstore)
DECLR_WIDE_STORE_CAT_1_FAMILY(astore)

#define DECLR_STORE_N_CAT_1_FAMILY(instructionprefix, N) \
    uint8_t instfunc_##instructionprefix##_##N(interpreter_module* jvm, frame* fr) \
    { \
        int32_t operand; \
        pop_from_stack_operand(&fr->operands, &operand, NULL); \
        *(fr->local_vars + N) = operand; \
        return 1; \
    }

#define DECLR_STORE_N_CAT_2_FAMILY(instructionprefix, N) \
    uint8_t instfunc_##instructionprefix##_##N(interpreter_module* jvm, frame* fr) \
    { \
        int32_t highoperand; \
        int32_t lowoperand; \
        pop_from_stack_operand(&fr->operands, &lowoperand, NULL); \
        pop_from_stack_operand(&fr->operands, &highoperand, NULL); \
        *(fr->local_vars + N) = highoperand; \
        *(fr->local_vars + N + 1) = lowoperand; \
        return 1; \
    }

DECLR_STORE_N_CAT_1_FAMILY(istore, 0)
DECLR_STORE_N_CAT_1_FAMILY(istore, 1)
DECLR_STORE_N_CAT_1_FAMILY(istore, 2)
DECLR_STORE_N_CAT_1_FAMILY(istore, 3)

DECLR_STORE_N_CAT_2_FAMILY(lstore, 0)
DECLR_STORE_N_CAT_2_FAMILY(lstore, 1)
DECLR_STORE_N_CAT_2_FAMILY(lstore, 2)
DECLR_STORE_N_CAT_2_FAMILY(lstore, 3)

DECLR_STORE_N_CAT_1_FAMILY(fstore, 0)
DECLR_STORE_N_CAT_1_FAMILY(fstore, 1)
DECLR_STORE_N_CAT_1_FAMILY(fstore, 2)
DECLR_STORE_N_CAT_1_FAMILY(fstore, 3)

DECLR_STORE_N_CAT_2_FAMILY(dstore, 0)
DECLR_STORE_N_CAT_2_FAMILY(dstore, 1)
DECLR_STORE_N_CAT_2_FAMILY(dstore, 2)
DECLR_STORE_N_CAT_2_FAMILY(dstore, 3)

DECLR_STORE_N_CAT_1_FAMILY(astore, 0)
DECLR_STORE_N_CAT_1_FAMILY(astore, 1)
DECLR_STORE_N_CAT_1_FAMILY(astore, 2)
DECLR_STORE_N_CAT_1_FAMILY(astore, 3)

#define DECLR_ASTORE_CAT_1_FAMILY(instructionname, type) \
    uint8_t instfunc_##instructionname(interpreter_module* jvm, frame* fr) \
    { \
        int32_t operand; \
        int32_t index; \
        int32_t arrayref; \
        reference* obj; \
        pop_from_stack_operand(&fr->operands, &operand, NULL); \
        pop_from_stack_operand(&fr->operands, &index, NULL); \
        pop_from_stack_operand(&fr->operands, &arrayref, NULL); \
        obj = (reference*)arrayref; \
        if (obj == NULL) \
        { \
            \
            DEBUG_REPORT_ERROR_INSTRUCTION \
            return 0; \
        } \
        if (index < 0 || (uint32_t)index >= obj->arr.length) \
        { \
            \
            DEBUG_REPORT_ERROR_INSTRUCTION \
            return 0; \
        } \
        type* ptr = (type*)obj->arr.data; \
        ptr[index] = (type)operand; \
        return 1; \
    }

DECLR_ASTORE_CAT_1_FAMILY(bastore, int8_t)
DECLR_ASTORE_CAT_1_FAMILY(sastore, int16_t)
DECLR_ASTORE_CAT_1_FAMILY(castore, int16_t)
DECLR_ASTORE_CAT_1_FAMILY(iastore, int32_t)
DECLR_ASTORE_CAT_1_FAMILY(fastore, int32_t)

#define DECLR_ASTORE_CAT_2_FAMILY(instructionname) \
    uint8_t instfunc_##instructionname(interpreter_module* jvm, frame* fr) \
    { \
        int32_t highoperand; \
        int32_t lowoperand; \
        int32_t index; \
        int32_t arrayref; \
        reference* obj; \
        pop_from_stack_operand(&fr->operands, &lowoperand, NULL); \
        pop_from_stack_operand(&fr->operands, &highoperand, NULL); \
        pop_from_stack_operand(&fr->operands, &index, NULL); \
        pop_from_stack_operand(&fr->operands, &arrayref, NULL); \
        obj = (reference*)arrayref; \
        if (obj == NULL) \
        { \
            \
            DEBUG_REPORT_ERROR_INSTRUCTION \
            return 0; \
        } \
        if (index < 0 || (uint32_t)index >= obj->arr.length) \
        { \
            \
            DEBUG_REPORT_ERROR_INSTRUCTION \
            return 0; \
        } \
        int64_t* ptr = (int64_t*)obj->arr.data; \
        ptr[index] = ((int64_t)highoperand << 32) | (uint32_t)lowoperand; \
        return 1; \
    }

DECLR_ASTORE_CAT_2_FAMILY(dastore)
DECLR_ASTORE_CAT_2_FAMILY(lastore)

uint8_t instfunc_aastore(interpreter_module* jvm, frame* fr)
{
    int32_t operand;
    int32_t index;
    int32_t arrayref;

    reference* arrayobj;
    reference* element;

    pop_from_stack_operand(&fr->operands, &operand, NULL);
    pop_from_stack_operand(&fr->operands, &index, NULL);
    pop_from_stack_operand(&fr->operands, &arrayref, NULL);

    arrayobj = (reference*)arrayref;
    element = (reference*)operand;

    if (arrayobj == NULL)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0; \
    }

    if (index < 0 || (uint32_t)index >= arrayobj->oar.length)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    arrayobj->oar.elements[index] = element;
    return 1;
}

uint8_t instfunc_pop(interpreter_module* jvm, frame* fr)
{
    pop_from_stack_operand(&fr->operands, NULL, NULL);
    return 1;
}

uint8_t instfunc_pop2(interpreter_module* jvm, frame* fr)
{
    pop_from_stack_operand(&fr->operands, NULL, NULL);
    pop_from_stack_operand(&fr->operands, NULL, NULL);
    return 1;
}

uint8_t instfunc_dup(interpreter_module* jvm, frame* fr)
{
    if (!push_to_stack_operand(&fr->operands, fr->operands->value, fr->operands->type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }
    return 1;
}

uint8_t instfunc_dup_x1(interpreter_module* jvm, frame* fr)
{
    if (!push_to_stack_operand(&fr->operands, fr->operands->value, fr->operands->type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    stack_operand* nodeA = fr->operands;
    stack_operand* nodeC = nodeA->next;
    stack_operand* nodeB = nodeC->next;

    nodeA->next = nodeB;
    nodeC->next = nodeB->next;
    nodeB->next = nodeC;

    return 1;
}

uint8_t instfunc_dup_x2(interpreter_module* jvm, frame* fr)
{
    if (!push_to_stack_operand(&fr->operands, fr->operands->value, fr->operands->type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    stack_operand* nodeA = fr->operands;
    stack_operand* nodeD = nodeA->next;
    stack_operand* nodeB = nodeD->next;
    stack_operand* nodeC = nodeB->next;

    nodeA->next = nodeB;
    nodeD->next = nodeC->next;
    nodeC->next = nodeD;

    return 1;
}

uint8_t instfunc_dup2(interpreter_module* jvm, frame* fr)
{
    stack_operand* node1 = fr->operands;
    stack_operand* node2 = node1->next;

    if (!push_to_stack_operand(&fr->operands, node2->value, node2->type) ||
        !push_to_stack_operand(&fr->operands, node1->value, node1->type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dup2_x1(interpreter_module* jvm, frame* fr)
{
    int32_t operand1, operand2, operand3;
    operand_type type1, type2, type3;

    pop_from_stack_operand(&fr->operands, &operand1, &type1);
    pop_from_stack_operand(&fr->operands, &operand2, &type2);
    pop_from_stack_operand(&fr->operands, &operand3, &type3);

    if (!push_to_stack_operand(&fr->operands, operand2, type2) ||
        !push_to_stack_operand(&fr->operands, operand1, type1) ||
        !push_to_stack_operand(&fr->operands, operand3, type3) ||
        !push_to_stack_operand(&fr->operands, operand2, type2) ||
        !push_to_stack_operand(&fr->operands, operand1, type1))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dup2_x2(interpreter_module* jvm, frame* fr)
{
    int32_t operand1, operand2, operand3, operand4;
    operand_type type1, type2, type3, type4;

    pop_from_stack_operand(&fr->operands, &operand1, &type1);
    pop_from_stack_operand(&fr->operands, &operand2, &type2);
    pop_from_stack_operand(&fr->operands, &operand3, &type3);
    pop_from_stack_operand(&fr->operands, &operand4, &type4);

    if (!push_to_stack_operand(&fr->operands, operand2, type2) ||
        !push_to_stack_operand(&fr->operands, operand1, type1) ||
        !push_to_stack_operand(&fr->operands, operand4, type4) ||
        !push_to_stack_operand(&fr->operands, operand3, type3) ||
        !push_to_stack_operand(&fr->operands, operand2, type2) ||
        !push_to_stack_operand(&fr->operands, operand1, type1))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_swap(interpreter_module* jvm, frame* fr)
{
    stack_operand* node1 = fr->operands;
    stack_operand* node2 = node1->next;

    fr->operands = node2;
    node1->next = node2->next;
    node2->next = node1;
    return 1;
}

#define DECLR_INTEGER_MATH_OP(instruction, op) \
    uint8_t instfunc_##instruction(interpreter_module* jvm, frame* fr) \
    { \
        int32_t value1, value2; \
        pop_from_stack_operand(&fr->operands, &value2, NULL); \
        pop_from_stack_operand(&fr->operands, &value1, NULL); \
        if (!push_to_stack_operand(&fr->operands, value1 op value2, INT_OP)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_INTEGER_MATH_OP(iadd, +)
DECLR_INTEGER_MATH_OP(isub, -)
DECLR_INTEGER_MATH_OP(imul, *)
DECLR_INTEGER_MATH_OP(idiv, /)
DECLR_INTEGER_MATH_OP(irem, %)
DECLR_INTEGER_MATH_OP(iand, &)
DECLR_INTEGER_MATH_OP(ior, |)
DECLR_INTEGER_MATH_OP(ixor, ^)

uint8_t instfunc_ishl(interpreter_module* jvm, frame* fr)
{
    int32_t value1, value2;

    pop_from_stack_operand(&fr->operands, &value2, NULL);
    pop_from_stack_operand(&fr->operands, &value1, NULL);

    if (!push_to_stack_operand(&fr->operands, value1 << (value2 & 0x1F), INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ishr(interpreter_module* jvm, frame* fr)
{
    int32_t value1, value2;

    pop_from_stack_operand(&fr->operands, &value2, NULL);
    pop_from_stack_operand(&fr->operands, &value1, NULL);

    if (!push_to_stack_operand(&fr->operands, value1 >> (value2 & 0x1F), INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_iushr(interpreter_module* jvm, frame* fr)
{
    uint32_t value1, value2;

    pop_from_stack_operand(&fr->operands, (int32_t*)&value2, NULL);
    pop_from_stack_operand(&fr->operands, (int32_t*)&value1, NULL);

    if (!push_to_stack_operand(&fr->operands, value1 >> (value2 & 0x1F), INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

#define DECLR_LONG_MATH_OP(instruction, op) \
    uint8_t instfunc_##instruction(interpreter_module* jvm, frame* fr) \
    { \
        int64_t value1, value2; \
        int32_t high, low; \
        pop_from_stack_operand(&fr->operands, &low, NULL); \
        pop_from_stack_operand(&fr->operands, &high, NULL); \
        value2 = high; \
        value2 = value2 << 32 | (uint32_t)low; \
        pop_from_stack_operand(&fr->operands, &low, NULL); \
        pop_from_stack_operand(&fr->operands, &high, NULL); \
        value1 = high; \
        value1 = value1 << 32 | (uint32_t)low; \
        value1 = value1 op value2; \
        if (!push_to_stack_operand(&fr->operands, HIWORD(value1), LONG_OP) || \
            !push_to_stack_operand(&fr->operands, LOWORD(value1), LONG_OP)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_LONG_MATH_OP(ladd, +)
DECLR_LONG_MATH_OP(lsub, -)
DECLR_LONG_MATH_OP(lmul, *)
DECLR_LONG_MATH_OP(ldiv, /)
DECLR_LONG_MATH_OP(lrem, %)
DECLR_LONG_MATH_OP(land, &)
DECLR_LONG_MATH_OP(lor, |)
DECLR_LONG_MATH_OP(lxor, ^)

uint8_t instfunc_lshl(interpreter_module* jvm, frame* fr)
{
    int64_t value1;
    int32_t value2;
    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &value2, NULL);
    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value1 = high;
    value1 = (value1 << 32) | (uint32_t)low;

    value1 = value1 << (value2 & 0x3F);

    if (!push_to_stack_operand(&fr->operands, HIWORD(value1), LONG_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(value1), LONG_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }
    return 1;
}

uint8_t instfunc_lshr(interpreter_module* jvm, frame* fr)
{
    int64_t value1;
    int32_t value2;
    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &value2, NULL);
    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value1 = high;
    value1 = (value1 << 32) | (uint32_t)low;

    value1 = value1 >> (value2 & 0x3F);

    if (!push_to_stack_operand(&fr->operands, HIWORD(value1), LONG_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(value1), LONG_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }
    return 1;
}

uint8_t instfunc_lushr(interpreter_module* jvm, frame* fr)
{
    uint64_t value1;
    uint32_t value2;
    uint32_t high, low;

    pop_from_stack_operand(&fr->operands, (int32_t*)&value2, NULL);
    pop_from_stack_operand(&fr->operands, (int32_t*)&low, NULL);
    pop_from_stack_operand(&fr->operands, (int32_t*)&high, NULL);

    value1 = high;
    value1 = (value1 << 32) | (uint32_t)low;

    value1 = value1 >> (value2 & 0x3F);

    if (!push_to_stack_operand(&fr->operands, HIWORD(value1), LONG_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(value1), LONG_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }
    return 1;
}

#define DECLR_FLOAT_MATH_OP(instruction, op) \
    uint8_t instfunc_##instruction(interpreter_module* jvm, frame* fr) \
    { \
        union { \
            float f; \
            int32_t i; \
        } value1, value2; \
        pop_from_stack_operand(&fr->operands, &value2.i, NULL); \
        pop_from_stack_operand(&fr->operands, &value1.i, NULL); \
        value1.f = value1.f op value2.f; \
        if (!push_to_stack_operand(&fr->operands, value1.i, FLOAT_OP)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_FLOAT_MATH_OP(fadd, +)
DECLR_FLOAT_MATH_OP(fsub, -)
DECLR_FLOAT_MATH_OP(fmul, *)
DECLR_FLOAT_MATH_OP(fdiv, /)

#define DECLR_DOUBLE_MATH_OP(instruction, op) \
    uint8_t instfunc_##instruction(interpreter_module* jvm, frame* fr) \
    { \
        union { \
            double d; \
            int64_t i; \
        } value1, value2; \
        int32_t high, low; \
        pop_from_stack_operand(&fr->operands, &low, NULL); \
        pop_from_stack_operand(&fr->operands, &high, NULL); \
        value2.i = high; \
        value2.i = (value2.i << 32) | (uint32_t)low; \
        pop_from_stack_operand(&fr->operands, &low, NULL); \
        pop_from_stack_operand(&fr->operands, &high, NULL); \
        value1.i = high; \
        value1.i = (value1.i << 32) | (uint32_t)low; \
        value1.d = value1.d op value2.d; \
        if (!push_to_stack_operand(&fr->operands, HIWORD(value1.i), DOUBLE_OP) || \
            !push_to_stack_operand(&fr->operands, LOWORD(value1.i), DOUBLE_OP)) \
        { \
            jvm->status = OUT_OF_MEMORY; \
            return 0; \
        } \
        return 1; \
    }

DECLR_DOUBLE_MATH_OP(dadd, +)
DECLR_DOUBLE_MATH_OP(dsub, -)
DECLR_DOUBLE_MATH_OP(dmul, *)
DECLR_DOUBLE_MATH_OP(ddiv, /)

uint8_t instfunc_frem(interpreter_module* jvm, frame* fr)
{
    union {
        float f;
        int32_t i;
    } value1, value2;

    pop_from_stack_operand(&fr->operands, &value2.i, NULL);
    pop_from_stack_operand(&fr->operands, &value1.i, NULL);

    if (!(value1.f != INFINITY && value1.f != -INFINITY && (value2.f == INFINITY || value2.f == -INFINITY)))
        value1.f = fmodf(value1.f, value2.f);

    if (!push_to_stack_operand(&fr->operands, value1.i, FLOAT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_drem(interpreter_module* jvm, frame* fr)
{
    union {
        double d;
        int64_t i;
    } value1, value2;

    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value2.i = high;
    value2.i = (value2.i << 32) | (uint32_t)low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value1.i = high;
    value1.i = (value1.i << 32) | (uint32_t)low;

    if (!(value1.d != INFINITY && value1.d != -INFINITY && (value2.d == INFINITY || value2.d == -INFINITY)))
        value1.d = fmod(value1.d, value2.d);

    if (!push_to_stack_operand(&fr->operands, HIWORD(value1.i), DOUBLE_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(value1.i), DOUBLE_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_ineg(interpreter_module* jvm, frame* fr)
{
    int32_t value;
    pop_from_stack_operand(&fr->operands, &value, NULL);
    value = -value;

    if (!push_to_stack_operand(&fr->operands, value, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_lneg(interpreter_module* jvm, frame* fr)
{
    int64_t value;
    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value = high;
    value = (value << 32) | (uint32_t)low;
    value = -value;

    if (!push_to_stack_operand(&fr->operands, HIWORD(value), LONG_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(value), LONG_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_fneg(interpreter_module* jvm, frame* fr)
{
    union {
        float f;
        int32_t i;
    } value;

    pop_from_stack_operand(&fr->operands, &value.i, NULL);

    value.f = -value.f;

    if (!push_to_stack_operand(&fr->operands, value.i, FLOAT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dneg(interpreter_module* jvm, frame* fr)
{
    union {
        double d;
        int64_t i;
    } value;

    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value.i = high;
    value.i = (value.i << 32) | (uint32_t)low;
    value.d = -value.d;

    if (!push_to_stack_operand(&fr->operands, HIWORD(value.i), DOUBLE_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(value.i), DOUBLE_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_iinc(interpreter_module* jvm, frame* fr)
{
    uint8_t index = NEXT_BYTE;
    int8_t immediate = (int8_t)NEXT_BYTE;
    fr->local_vars[index] += (int32_t)immediate;
    return 1;
}

uint8_t instfunc_wide_iinc(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;
    int16_t immediate = NEXT_BYTE;
    immediate = (immediate << 8) | NEXT_BYTE;
    fr->local_vars[index] += (int32_t)immediate;
    return 1;
}

uint8_t instfunc_i2l(interpreter_module* jvm, frame* fr)
{
    int64_t value;
    int32_t temp;

    pop_from_stack_operand(&fr->operands, &temp, NULL);

    value = temp;

    if (!push_to_stack_operand(&fr->operands, HIWORD(value), LONG_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(value), LONG_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2f(interpreter_module* jvm, frame* fr)
{
    union {
        float f;
        int32_t i;
    } value;

    pop_from_stack_operand(&fr->operands, &value.i, NULL);
    value.f = (float)value.i;

    if (!push_to_stack_operand(&fr->operands, value.i, FLOAT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2d(interpreter_module* jvm, frame* fr)
{
    union {
        double d;
        int64_t i;
    } value;

    int32_t temp;

    pop_from_stack_operand(&fr->operands, &temp, NULL);
    value.d = (double)temp;

    if (!push_to_stack_operand(&fr->operands, HIWORD(value.i), DOUBLE_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(value.i), DOUBLE_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_l2i(interpreter_module* jvm, frame* fr)
{
    int32_t temp;

    pop_from_stack_operand(&fr->operands, &temp, NULL);
    pop_from_stack_operand(&fr->operands, NULL, NULL);

    if (!push_to_stack_operand(&fr->operands, temp, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_l2f(interpreter_module* jvm, frame* fr)
{
    int64_t lval;

    union {
        float f;
        int32_t i;
    } temp;

    int32_t low, high;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    lval = high;
    lval = (lval << 32) | (uint32_t)low;
    temp.f = (float)lval;

    if (!push_to_stack_operand(&fr->operands, temp.i, FLOAT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_l2d(interpreter_module* jvm, frame* fr)
{
    union {
        double d;
        int64_t i;
    } val;

    int32_t temp;

    pop_from_stack_operand(&fr->operands, &temp, NULL);

    val.i = temp;

    pop_from_stack_operand(&fr->operands, &temp, NULL);

    val.i = (val.i << 32) | (uint32_t)temp;
    val.d = (double)val.i;

    if (!push_to_stack_operand(&fr->operands, HIWORD(val.i), DOUBLE_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(val.i), DOUBLE_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_f2i(interpreter_module* jvm, frame* fr)
{
    union {
        float f;
        int32_t i;
    } value;

    pop_from_stack_operand(&fr->operands, &value.i, NULL);
    value.i = (int32_t)value.f;

    if (!push_to_stack_operand(&fr->operands, value.i, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_f2l(interpreter_module* jvm, frame* fr)
{
    int64_t lval;

    union {
        float f;
        int32_t i;
    } temp;

    pop_from_stack_operand(&fr->operands, &temp.i, NULL);

    lval = (int64_t)temp.f;

    if (!push_to_stack_operand(&fr->operands, HIWORD(lval), LONG_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(lval), LONG_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_f2d(interpreter_module* jvm, frame* fr)
{
    union {
        double d;
        int64_t i;
    } dval;

    union {
        float f;
        int32_t i;
    } temp;

    pop_from_stack_operand(&fr->operands, &temp.i, NULL);

    dval.d = (double)temp.f;

    if (!push_to_stack_operand(&fr->operands, HIWORD(dval.i), DOUBLE_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(dval.i), DOUBLE_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_d2i(interpreter_module* jvm, frame* fr)
{
    union {
        double d;
        int64_t i;
    } dval;

    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    dval.i = high;
    dval.i = (dval.i << 32) | (uint32_t)low;

    low = (int32_t)dval.d;

    if (!push_to_stack_operand(&fr->operands, low, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_d2l(interpreter_module* jvm, frame* fr)
{
    union {
        double d;
        int64_t i;
    } dval;

    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    dval.i = high;
    dval.i = (dval.i << 32) | (uint32_t)low;
    dval.i = (int64_t)dval.d;

    if (!push_to_stack_operand(&fr->operands, HIWORD(dval.i), LONG_OP) ||
        !push_to_stack_operand(&fr->operands, LOWORD(dval.i), LONG_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_d2f(interpreter_module* jvm, frame* fr)
{
    union {
        double d;
        int64_t i;
    } dval;

    union {
        float f;
        int32_t i;
    } temp;

    int32_t low, high;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    dval.i = high;
    dval.i = (dval.i << 32) | (uint32_t)low;

    temp.f = (float)dval.d;

    if (!push_to_stack_operand(&fr->operands, temp.i, FLOAT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2b(interpreter_module* jvm, frame* fr)
{
    int32_t value;
    int8_t byte;

    pop_from_stack_operand(&fr->operands, &value, NULL);

    byte = (int8_t)value;

    if (!push_to_stack_operand(&fr->operands, (int32_t)byte, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2c(interpreter_module* jvm, frame* fr)
{
    int32_t value;
    uint16_t character;

    pop_from_stack_operand(&fr->operands, &value, NULL);

    character = (uint16_t)value;

    if (!push_to_stack_operand(&fr->operands, (int32_t)character, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_i2s(interpreter_module* jvm, frame* fr)
{
    int32_t value;
    int16_t sval;

    pop_from_stack_operand(&fr->operands, &value, NULL);

    sval = (int16_t)value;

    if (!push_to_stack_operand(&fr->operands, (int32_t)sval, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_lcmp(interpreter_module* jvm, frame* fr)
{
    int32_t high, low;
    int64_t value1, value2;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value2 = high;
    value2 = (value2 << 32) | (uint32_t)low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value1 = high;
    value1 = (value1 << 32) | (uint32_t)low;

    if (value1 > value2)
        high = 1;
    else if (value1 == value2)
        high = 0;
    else
        high = -1;

    if (!push_to_stack_operand(&fr->operands, high, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_fcmpl(interpreter_module* jvm, frame* fr)
{
    union {
        int32_t i;
        float f;
    } value1, value2;

    pop_from_stack_operand(&fr->operands, &value2.i, NULL);
    pop_from_stack_operand(&fr->operands, &value1.i, NULL);

    if (value1.f < value2.f || value1.f == NAN || value2.f == NAN)
        value1.i = -1;
    else if (value1.f == value2.f)
        value1.i = 0;
    else
        value1.i = 1;

    if (!push_to_stack_operand(&fr->operands, value1.i, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_fcmpg(interpreter_module* jvm, frame* fr)
{
    union {
        int32_t i;
        float f;
    } value1, value2;

    pop_from_stack_operand(&fr->operands, &value2.i, NULL);
    pop_from_stack_operand(&fr->operands, &value1.i, NULL);

    if (value1.f > value2.f || value1.f == NAN || value2.f == NAN)
        value1.i = 1;
    else if (value1.f == value2.f)
        value1.i = 0;
    else
        value1.i = -1;

    if (!push_to_stack_operand(&fr->operands, value1.i, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dcmpl(interpreter_module* jvm, frame* fr)
{
    union {
        int64_t i;
        double d;
    } value1, value2;

    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value2.i = high;
    value2.i = (value2.i << 32) | (uint32_t)low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value1.i = high;
    value1.i = (value1.i << 32) | (uint32_t)low;

    if (value1.d < value2.d || value1.d == NAN || value2.d == NAN)
        value1.i = -1;
    else if (value1.d == value2.d)
        value1.i = 0;
    else
        value1.i = 1;

    if (!push_to_stack_operand(&fr->operands, value1.i, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_dcmpg(interpreter_module* jvm, frame* fr)
{
    union {
        int64_t i;
        double d;
    } value1, value2;

    int32_t high, low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value2.i = high;
    value2.i = (value2.i << 32) | (uint32_t)low;

    pop_from_stack_operand(&fr->operands, &low, NULL);
    pop_from_stack_operand(&fr->operands, &high, NULL);

    value1.i = high;
    value1.i = (value1.i << 32) | (uint32_t)low;

    if (value1.d > value2.d || value1.d == NAN || value2.d == NAN)
        value1.i = 1;
    else if (value1.d == value2.d)
        value1.i = 0;
    else
        value1.i = -1;

    if (!push_to_stack_operand(&fr->operands, value1.i, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

#define DECLR_IF_FAMILY(inst, op) \
    uint8_t instfunc_##inst(interpreter_module* jvm, frame* fr) \
    { \
        int32_t value; \
        int16_t offset = NEXT_BYTE; \
        offset = (offset << 8) | NEXT_BYTE; \
        pop_from_stack_operand(&fr->operands, &value, NULL); \
        if (value op 0) \
            fr->PC += offset - 3; \
        return 1; \
    }

DECLR_IF_FAMILY(ifeq, ==)
DECLR_IF_FAMILY(ifne, !=)
DECLR_IF_FAMILY(iflt, <)
DECLR_IF_FAMILY(ifle, <=)
DECLR_IF_FAMILY(ifgt, >)
DECLR_IF_FAMILY(ifge, >=)

#define DECLR_IF_ICMP_FAMILY(inst, op) \
    uint8_t instfunc_##inst(interpreter_module* jvm, frame* fr) \
    { \
        int32_t value1, value2; \
        int16_t offset = NEXT_BYTE; \
        offset = (offset << 8) | NEXT_BYTE; \
        pop_from_stack_operand(&fr->operands, &value2, NULL); \
        pop_from_stack_operand(&fr->operands, &value1, NULL); \
        if (value1 op value2) \
            fr->PC += offset - 3; \
        return 1; \
    }

DECLR_IF_ICMP_FAMILY(if_icmpeq, ==)
DECLR_IF_ICMP_FAMILY(if_icmpne, !=)
DECLR_IF_ICMP_FAMILY(if_icmplt, <)
DECLR_IF_ICMP_FAMILY(if_icmple, <=)
DECLR_IF_ICMP_FAMILY(if_icmpgt, >)
DECLR_IF_ICMP_FAMILY(if_icmpge, >=)

DECLR_IF_ICMP_FAMILY(if_acmpeq, ==)
DECLR_IF_ICMP_FAMILY(if_acmpne, !=)

uint8_t instfunc_goto(interpreter_module* jvm, frame* fr)
{
    int16_t offset = NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    fr->PC += offset - 3;
    return 1;
}

uint8_t instfunc_jsr(interpreter_module* jvm, frame* fr)
{
    int16_t offset = NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;

    if (!push_to_stack_operand(&fr->operands, (int32_t)fr->PC, RETADD_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    fr->PC += offset - 3;
    return 1;
}

uint8_t instfunc_ret(interpreter_module* jvm, frame* fr)
{
    uint8_t index = NEXT_BYTE;
    fr->PC = (uint32_t)fr->local_vars[index];
    return 1;
}

uint8_t instfunc_wide_ret(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;
    fr->PC = (uint32_t)fr->local_vars[index];
    return 1;
}

uint8_t instfunc_tableswitch(interpreter_module* jvm, frame* fr)
{
    uint32_t base = fr->PC - 1;

    fr->PC += 4 - (fr->PC % 4);

    int32_t defaultValue = NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;

    int32_t lowValue = NEXT_BYTE;
    lowValue = (lowValue << 8) | NEXT_BYTE;
    lowValue = (lowValue << 8) | NEXT_BYTE;
    lowValue = (lowValue << 8) | NEXT_BYTE;

    int32_t highValue = NEXT_BYTE;
    highValue = (highValue << 8) | NEXT_BYTE;
    highValue = (highValue << 8) | NEXT_BYTE;
    highValue = (highValue << 8) | NEXT_BYTE;

    int32_t index;
    pop_from_stack_operand(&fr->operands, &index, NULL);

    if (index >= lowValue && index <= highValue)
    {
        fr->PC += 4 * (index - lowValue);
        defaultValue = NEXT_BYTE;
        defaultValue = (defaultValue << 8) | NEXT_BYTE;
        defaultValue = (defaultValue << 8) | NEXT_BYTE;
        defaultValue = (defaultValue << 8) | NEXT_BYTE;
    }

    fr->PC = base + defaultValue;

    return 1;
}

uint8_t instfunc_lookupswitch(interpreter_module* jvm, frame* fr)
{
    uint32_t base = fr->PC - 1;

    fr->PC += 4 - (fr->PC % 4);

    int32_t defaultValue = NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;
    defaultValue = (defaultValue << 8) | NEXT_BYTE;

    int32_t npairs = NEXT_BYTE;
    npairs = (npairs << 8) | NEXT_BYTE;
    npairs = (npairs << 8) | NEXT_BYTE;
    npairs = (npairs << 8) | NEXT_BYTE;

    int32_t key, match;
    pop_from_stack_operand(&fr->operands, &key, NULL);

    while (npairs-- > 0)
    {
        match = NEXT_BYTE;
        match = (match << 8) | NEXT_BYTE;
        match = (match << 8) | NEXT_BYTE;
        match = (match << 8) | NEXT_BYTE;

        if (key == match)
        {
            defaultValue = NEXT_BYTE;
            defaultValue = (defaultValue << 8) | NEXT_BYTE;
            defaultValue = (defaultValue << 8) | NEXT_BYTE;
            defaultValue = (defaultValue << 8) | NEXT_BYTE;
            break;
        }
        else
        {
            fr->PC += 4;
        }
    }

    fr->PC = base + defaultValue;

    return 1;
}

#define DECLR_RETURN_FAMILY(instname, retcount) \
    uint8_t instfunc_##instname(interpreter_module* jvm, frame* fr) \
    { \
        \
        fr->PC = fr->bytecode_length; \
        \
        fr->return_count = retcount; \
        return 1;\
    }

DECLR_RETURN_FAMILY(ireturn, 1)
DECLR_RETURN_FAMILY(lreturn, 2)
DECLR_RETURN_FAMILY(freturn, 1)
DECLR_RETURN_FAMILY(dreturn, 2)
DECLR_RETURN_FAMILY(areturn, 1)
DECLR_RETURN_FAMILY(return, 0)

uint8_t instfunc_getstatic(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    constant_pool_info* field = fr->jc->constant_pool + index - 1;
    constant_pool_info* cpi1, *cpi2;

    if (jvm->sys_and_str_classes_simulation)
    {
        cpi1 = fr->jc->constant_pool + field->Fieldref.class_index - 1;
        cpi1 = fr->jc->constant_pool + cpi1->Class.name_index - 1;

        if (compare_utf8(UTF8(cpi1), (const uint8_t*)"java/lang/System", 16))
        {
            if (!push_to_stack_operand(&fr->operands, 0, NULL_OP))
            {
                jvm->status = OUT_OF_MEMORY;
                return 0;
            }

            return 1;
        }
    }

    loaded_classes* fieldLoadedClass;

    if (!field_handler(jvm, fr->jc, field, &fieldLoadedClass) ||
        !fieldLoadedClass || !initialize_class(jvm, fieldLoadedClass))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cpi2 = fr->jc->constant_pool + field->Fieldref.name_and_type_index - 1;
    cpi1 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;
    cpi2 = fr->jc->constant_pool + cpi2->NameAndType.descriptor_index - 1;

    field_info* fi = get_maching_field(fieldLoadedClass->jc, UTF8(cpi1), UTF8(cpi2), 0);

    if (!fi)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    operand_type type;

    switch (*cpi2->Utf8.bytes)
    {
        case 'J': type = LONG_OP; break;
        case 'D': type = DOUBLE_OP; break;
        case 'F': type = FLOAT_OP; break;

        case 'L':
        case '[':
            type = REF_OP;
            break;

        case 'B':
        case 'C':
        case 'I':
        case 'S':
        case 'Z':
            type = INT_OP;
            break;

        default:
            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;
    }

    if (!push_to_stack_operand(&fr->operands, fieldLoadedClass->static_data[fi->offset], type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    if (type == LONG_OP || type == DOUBLE_OP)
    {
        if (!push_to_stack_operand(&fr->operands, fieldLoadedClass->static_data[fi->offset + 1], type))
        {
            jvm->status = OUT_OF_MEMORY;
            return 0;
        }
    }

    return 1;
}

uint8_t instfunc_putstatic(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    constant_pool_info* field = fr->jc->constant_pool + index - 1;
    constant_pool_info* cpi1, *cpi2;

    loaded_classes* fieldLoadedClass;

    if (!field_handler(jvm, fr->jc, field, &fieldLoadedClass) ||
        !fieldLoadedClass || !initialize_class(jvm, fieldLoadedClass))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cpi2 = fr->jc->constant_pool + field->Fieldref.name_and_type_index - 1;
    cpi1 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;
    cpi2 = fr->jc->constant_pool + cpi2->NameAndType.descriptor_index - 1;

    field_info* fi = get_maching_field(fieldLoadedClass->jc, UTF8(cpi1), UTF8(cpi2), 0);

    if (!fi)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    operand_type type;

    switch (*cpi2->Utf8.bytes)
    {
        case 'J': type = LONG_OP; break;
        case 'D': type = DOUBLE_OP; break;
        case 'F': type = FLOAT_OP; break;

        case 'L':
        case '[':
            type = REF_OP;
            break;

        case 'B':
        case 'C':
        case 'I':
        case 'S':
        case 'Z':
            type = INT_OP;
            break;

        default:
            
            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;
    }

    int32_t operand;

    pop_from_stack_operand(&fr->operands, &operand, NULL);

    if (type == LONG_OP || type == DOUBLE_OP)
    {
        fieldLoadedClass->static_data[fi->offset + 1] = operand;
        pop_from_stack_operand(&fr->operands, &operand, NULL);
        fieldLoadedClass->static_data[fi->offset] = operand;
    }
    else
    {
        fieldLoadedClass->static_data[fi->offset] = operand;
    }

    return 1;
}

uint8_t instfunc_getfield(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    constant_pool_info* field = fr->jc->constant_pool + index - 1;
    constant_pool_info* cpi1, *cpi2;

    loaded_classes* fieldLoadedClass;

    if (!field_handler(jvm, fr->jc, field, &fieldLoadedClass) ||
        !fieldLoadedClass || !initialize_class(jvm, fieldLoadedClass))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cpi2 = fr->jc->constant_pool + field->Fieldref.name_and_type_index - 1;
    cpi1 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;
    cpi2 = fr->jc->constant_pool + cpi2->NameAndType.descriptor_index - 1;

    field_info* fi = get_maching_field(fieldLoadedClass->jc, UTF8(cpi1), UTF8(cpi2), 0);

    if (!fi)
    {
        java_class* super = get_super_class_of_given_class(jvm, fieldLoadedClass->jc);

        while (super)
        {
            fi = get_maching_field(super, UTF8(cpi1), UTF8(cpi2), 0);

            if (fi)
                break;

            super = get_super_class_of_given_class(jvm, super);
        }
    }

    if (!fi)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    operand_type type;

    switch (*cpi2->Utf8.bytes)
    {
        case 'J': type = LONG_OP; break;
        case 'D': type = DOUBLE_OP; break;
        case 'F': type = FLOAT_OP; break;

        case 'L':
        case '[':
            type = REF_OP;
            break;

        case 'B':
        case 'C':
        case 'I':
        case 'S':
        case 'Z':
            type = INT_OP;
            break;

        default:
            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;
    }

    reference* object;
    int32_t object_address;

    pop_from_stack_operand(&fr->operands, &object_address, NULL);
    object = (reference*)object_address;

    if (!object)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    if (!push_to_stack_operand(&fr->operands, object->ci.data[fi->offset], type))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    if (type == LONG_OP || type == DOUBLE_OP)
    {
        if (!push_to_stack_operand(&fr->operands, object->ci.data[fi->offset + 1], type))
        {
            jvm->status = OUT_OF_MEMORY;
            return 0;
        }
    }

    return 1;
}

uint8_t instfunc_putfield(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    constant_pool_info* field = fr->jc->constant_pool + index - 1;
    constant_pool_info* cpi1, *cpi2;

    loaded_classes* fieldLoadedClass;

    if (!field_handler(jvm, fr->jc, field, &fieldLoadedClass) ||
        !fieldLoadedClass || !initialize_class(jvm, fieldLoadedClass))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cpi2 = fr->jc->constant_pool + field->Methodref.name_and_type_index - 1;
    cpi1 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;
    cpi2 = fr->jc->constant_pool + cpi2->NameAndType.descriptor_index - 1;

    field_info* fi = get_maching_field(fieldLoadedClass->jc, UTF8(cpi1), UTF8(cpi2), 0);

    if (!fi)
    {
        java_class* super = get_super_class_of_given_class(jvm, fieldLoadedClass->jc);

        while (super)
        {
            fi = get_maching_field(super, UTF8(cpi1), UTF8(cpi2), 0);

            if (fi)
                break;

            super = get_super_class_of_given_class(jvm, super);
        }
    }

    if (!fi)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    operand_type type;

    switch (*cpi2->Utf8.bytes)
    {
        case 'J': type = LONG_OP; break;
        case 'D': type = DOUBLE_OP; break;
        case 'F': type = FLOAT_OP; break;

        case 'L':
        case '[':
            type = REF_OP;
            break;

        case 'B':
        case 'C':
        case 'I':
        case 'S':
        case 'Z':
            type = INT_OP;
            break;

        default:
            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;
    }

    reference* object;
    int32_t lo_operand;
    int32_t hi_operand;
    int32_t object_address;

    pop_from_stack_operand(&fr->operands, &lo_operand, NULL);

    if (type == LONG_OP || type == DOUBLE_OP)
        pop_from_stack_operand(&fr->operands, &hi_operand, NULL);

    pop_from_stack_operand(&fr->operands, &object_address, NULL);
    object = (reference*)object_address;

    if (!object)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    if (type == LONG_OP || type == DOUBLE_OP)
    {
        object->ci.data[fi->offset] = hi_operand;
        object->ci.data[fi->offset + 1] = lo_operand;
    }
    else
    {
        object->ci.data[fi->offset] = lo_operand;
   }

    return 1;
}

uint8_t instfunc_invokevirtual(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    constant_pool_info* method = fr->jc->constant_pool + index - 1;
    constant_pool_info* cpi1, *cpi2, *cpi3;
    method_info* mi = NULL;

    if (jvm->sys_and_str_classes_simulation)
    {
        cpi1 = fr->jc->constant_pool + method->Methodref.class_index - 1;
        cpi1 = fr->jc->constant_pool + cpi1->Class.name_index - 1;

        cpi2 = fr->jc->constant_pool + method->Methodref.name_and_type_index - 1;
        cpi2 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;

        cpi3 = fr->jc->constant_pool + method->Methodref.name_and_type_index - 1;
        cpi3 = fr->jc->constant_pool + cpi3->NameAndType.descriptor_index - 1;

        native_func nativeFunc = get_native_func(UTF8(cpi1), UTF8(cpi2), UTF8(cpi3));

        if (nativeFunc)
            return nativeFunc(jvm, fr, UTF8(cpi3));
    }

    loaded_classes* methodLoadedClass;

    if (!method_handler(jvm, fr->jc, method, &methodLoadedClass) ||
        !methodLoadedClass || !initialize_class(jvm, methodLoadedClass))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cpi2 = fr->jc->constant_pool + method->Methodref.name_and_type_index - 1;
    cpi1 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;
    cpi2 = fr->jc->constant_pool + cpi2->NameAndType.descriptor_index - 1;

    uint8_t operandIndex;
    uint8_t parameterCount = get_method_descriptor_param_cout(UTF8(cpi2));
    stack_operand* node = fr->operands;

    for (operandIndex = 0; operandIndex < parameterCount; operandIndex++)
        node = node->next;

    reference* object = (reference*)node->value;
    java_class* jc = object->ci.c;

    if (object)
    {
        while (jc)
        {
            mi = get_matching_method(jc, UTF8(cpi1), UTF8(cpi2), 0);

            if (mi)
                break;

            jc = get_super_class_of_given_class(jvm, jc);
        }
    }
    else
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    if (!mi)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    return run_method(jvm, jc, mi, 1 + parameterCount);
}

uint8_t instfunc_invokespecial(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    constant_pool_info* method = fr->jc->constant_pool + index - 1;
    constant_pool_info* cpi1, *cpi2;
    method_info* mi = NULL;

    loaded_classes* methodLoadedClass;

    if (!method_handler(jvm, fr->jc, method, &methodLoadedClass) ||
        !methodLoadedClass || !initialize_class(jvm, methodLoadedClass))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cpi2 = fr->jc->constant_pool + method->Methodref.name_and_type_index - 1;
    cpi1 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;
    cpi2 = fr->jc->constant_pool + cpi2->NameAndType.descriptor_index - 1;

    if (!compare_utf8(UTF8(cpi1), (const uint8_t*)"<init>", 6) &&
        (fr->jc->access_flags & SUPER_ACCESS_FLAG) && is_super_class_of_given_class(jvm, methodLoadedClass->jc, fr->jc))
    {
        java_class* super = get_super_class_of_given_class(jvm, fr->jc);

        while (super)
        {
            mi = get_matching_method(super, UTF8(cpi1), UTF8(cpi2), 0);

            if (mi)
            {
                methodLoadedClass->jc = super;
                break;
            }

            super = get_super_class_of_given_class(jvm, super);
        }

        if (!mi)
        {
            DEBUG_REPORT_ERROR_INSTRUCTION
            return 0;
        }
    }
    else
    {
        mi = get_matching_method(methodLoadedClass->jc, UTF8(cpi1), UTF8(cpi2), 0);
    }

    if (!mi)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    uint8_t parameterCount = 1 + get_method_descriptor_param_cout(UTF8(cpi2));

    return run_method(jvm, methodLoadedClass->jc, mi, parameterCount);
}

uint8_t instfunc_invokestatic(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    constant_pool_info* method = fr->jc->constant_pool + index - 1;
    constant_pool_info* cpi1, *cpi2, *cpi3;

    if (jvm->sys_and_str_classes_simulation)
    {
        cpi1 = fr->jc->constant_pool + method->Methodref.class_index - 1;
        cpi1 = fr->jc->constant_pool + cpi1->Class.name_index - 1;

        cpi2 = fr->jc->constant_pool + method->Methodref.name_and_type_index - 1;
        cpi2 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;

        cpi3 = fr->jc->constant_pool + method->Methodref.name_and_type_index - 1;
        cpi3 = fr->jc->constant_pool + cpi3->NameAndType.descriptor_index - 1;

        native_func nativeFunc = get_native_func(UTF8(cpi1), UTF8(cpi2), UTF8(cpi3));

        if (nativeFunc)
            return nativeFunc(jvm, fr, UTF8(cpi3));
    }

    loaded_classes* methodLoadedClass;

    if (!method_handler(jvm, fr->jc, method, &methodLoadedClass) ||
        !methodLoadedClass || !initialize_class(jvm, methodLoadedClass))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cpi2 = fr->jc->constant_pool + method->Methodref.name_and_type_index - 1;
    cpi1 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;
    cpi2 = fr->jc->constant_pool + cpi2->NameAndType.descriptor_index - 1;

    method_info* mi = get_matching_method(methodLoadedClass->jc, UTF8(cpi1), UTF8(cpi2), 0);

    if (!mi)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    return run_method(jvm, methodLoadedClass->jc, mi, get_method_descriptor_param_cout(UTF8(cpi2)));
}

uint8_t instfunc_invokeinterface(interpreter_module* jvm, frame* fr)
{
    uint16_t index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    fr->PC += 2;

    constant_pool_info* method = fr->jc->constant_pool + index - 1;
    constant_pool_info* cpi1, *cpi2;
    method_info* mi = NULL;

    loaded_classes* methodLoadedClass;

    if (!method_handler(jvm, fr->jc, method, &methodLoadedClass) ||
        !methodLoadedClass || !initialize_class(jvm, methodLoadedClass))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cpi2 = fr->jc->constant_pool + method->Methodref.name_and_type_index - 1;
    cpi1 = fr->jc->constant_pool + cpi2->NameAndType.name_index - 1;
    cpi2 = fr->jc->constant_pool + cpi2->NameAndType.descriptor_index - 1;

    uint8_t operandIndex;
    uint8_t parameterCount = get_method_descriptor_param_cout(UTF8(cpi2));
    stack_operand* node = fr->operands;

    for (operandIndex = 0; operandIndex < parameterCount; operandIndex++)
        node = node->next;

    reference* object = (reference*)node->value;
    java_class* jc = object->ci.c;

    if (object)
    {

        while (jc)
        {
            mi = get_matching_method(jc, UTF8(cpi1), UTF8(cpi2), 0);

            if (mi)
                break;

            jc = get_super_class_of_given_class(jvm, jc);
        }
    }
    else
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    if (!mi || (mi->access_flags & ABSTRACT_ACCESS_FLAG))
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    if ((mi->access_flags & PUBLIC_ACCESS_FLAG) == 0)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    return run_method(jvm, jc, mi, 1 + parameterCount);
}

uint8_t instfunc_invokedynamic(interpreter_module* jvm, frame* fr)
{
    DEBUG_REPORT_ERROR_INSTRUCTION
    return 0;
}

uint8_t instfunc_new(interpreter_module* jvm, frame* fr)
{
    uint16_t index;
    constant_pool_info* cp;
    loaded_classes* instanceLoadedClass;

    index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    cp = fr->jc->constant_pool + index - 1;
    cp = fr->jc->constant_pool + cp->Class.name_index - 1;

    if (!class_handler(jvm, UTF8(cp), &instanceLoadedClass) || !instanceLoadedClass)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    reference* instance = create_new_class_instance(jvm, instanceLoadedClass);

    if (!instance || !push_to_stack_operand(&fr->operands, (int32_t)instance, REF_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_newarray(interpreter_module* jvm, frame* fr)
{
    uint8_t type = NEXT_BYTE;
    int32_t count;

    pop_from_stack_operand(&fr->operands, &count, NULL);

    if (count < 0)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    reference* arrayref = create_new_array(jvm, (uint32_t)count, (opcode_newarray_type)type);

    if (!arrayref || !push_to_stack_operand(&fr->operands, (int32_t)arrayref, REF_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_anewarray(interpreter_module* jvm, frame* fr)
{
    uint16_t index;
    int32_t count;
    constant_pool_info* cp;

    index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    pop_from_stack_operand(&fr->operands, &count, NULL);

    if (count < 0)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    cp = fr->jc->constant_pool + index - 1;
    cp = fr->jc->constant_pool + cp->Class.name_index - 1;

    loaded_classes* loadedClass;

    if (!class_handler(jvm, UTF8(cp), &loadedClass))
    {
        return 0;
    }

    reference* aarray = create_new_object_array(jvm, count, UTF8(cp));

    if (!aarray || !push_to_stack_operand(&fr->operands, (int32_t)aarray, REF_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_arraylength(interpreter_module* jvm, frame* fr)
{
    int32_t operand;
    reference* object;

    pop_from_stack_operand(&fr->operands, &operand, NULL);

    object = (reference*)operand;

    if (!object)
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    if (object->type == REF_TYPE_ARRAY)
    {
        operand = object->arr.length;
    }
    else if (object->type == REF_TYPE_OBJECTARRAY)
    {
        operand = object->oar.length;
    }
    else
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    if (!push_to_stack_operand(&fr->operands, operand, INT_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    return 1;
}

uint8_t instfunc_athrow(interpreter_module* jvm, frame* fr)
{
    DEBUG_REPORT_ERROR_INSTRUCTION
    return 0;
}

uint8_t instfunc_checkcast(interpreter_module* jvm, frame* fr)
{
    DEBUG_REPORT_ERROR_INSTRUCTION
    return 0;
}

uint8_t instfunc_instanceof(interpreter_module* jvm, frame* fr)
{
    DEBUG_REPORT_ERROR_INSTRUCTION
    return 0;
}

uint8_t instfunc_monitorenter(interpreter_module* jvm, frame* fr)
{
    return 1;
}

uint8_t instfunc_monitorexit(interpreter_module* jvm, frame* fr)
{
    return 1;
}

uint8_t instfunc_wide(interpreter_module* jvm, frame* fr)
{
    uint8_t opcode = NEXT_BYTE;
    uint8_t index;

    if (opcode >= opcode_iload && opcode <= opcode_lload)
        index = opcode - opcode_iload;
    else if (opcode >= opcode_istore || opcode <= opcode_astore)
        index = 5 + opcode - opcode_istore;
    else if (opcode == opcode_ret)
        index = 10;
    else if (opcode == opcode_iinc)
        index = 11;
    else
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    const instruction_fun funcs[] = {
        instfunc_wide_iload, instfunc_wide_lload, instfunc_wide_fload,
        instfunc_wide_dload, instfunc_wide_aload, instfunc_wide_istore,
        instfunc_wide_lstore, instfunc_wide_fstore, instfunc_wide_dstore,
        instfunc_wide_dstore, instfunc_wide_ret, instfunc_wide_iinc
    };

    return funcs[index](jvm, fr);
}

uint8_t instfunc_multianewarray(interpreter_module* jvm, frame* fr)
{
    uint16_t index;
    uint8_t numberOfDimensions;
    int32_t* dimensions;
    constant_pool_info* cp;

    index = NEXT_BYTE;
    index = (index << 8) | NEXT_BYTE;

    numberOfDimensions = NEXT_BYTE;
    dimensions = (int32_t*)malloc(sizeof(int32_t) * numberOfDimensions);

    if (!dimensions)
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    uint8_t dimensionIndex = numberOfDimensions;

    while (dimensionIndex--)
    {
        pop_from_stack_operand(&fr->operands, dimensions + dimensionIndex, NULL);

        if (dimensions[dimensionIndex] < 0)
        {
            DEBUG_REPORT_ERROR_INSTRUCTION
            free(dimensions);
            return 0;
        }
        else if (dimensions[dimensionIndex] == 0)
        {
            DEBUG_REPORT_ERROR_INSTRUCTION
            free(dimensions);
            return 0;

        }
    }

    cp = fr->jc->constant_pool + index - 1;
    cp = fr->jc->constant_pool + cp->Class.name_index - 1;

    if (!class_handler(jvm, UTF8(cp), NULL))
    {
        free(dimensions);
        return 0;
    }

    reference* aarray = create_new_object_multi_array(jvm, dimensions, numberOfDimensions, UTF8(cp));

    if (!aarray || !push_to_stack_operand(&fr->operands, (int32_t)aarray, REF_OP))
    {
        free(dimensions);
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    free(dimensions);
    return 1;
}

uint8_t instfunc_ifnull(interpreter_module* jvm, frame* fr)
{
    int16_t branch = NEXT_BYTE;
    branch = (branch << 8) | NEXT_BYTE;

    int32_t address;

    pop_from_stack_operand(&fr->operands, &address, NULL);

    if (!address)
        fr->PC += branch - 3;

    return 1;
}

uint8_t instfunc_ifnonnull(interpreter_module* jvm, frame* fr)
{
    int16_t branch = NEXT_BYTE;
    branch = (branch << 8) | NEXT_BYTE;

    int32_t address;

    pop_from_stack_operand(&fr->operands, &address, NULL);

    if (address)
        fr->PC += branch - 3;

    return 1;
}

uint8_t instfunc_goto_w(interpreter_module* jvm, frame* fr)
{
    int32_t offset = NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    fr->PC += offset - 5;
    return 1;
}

uint8_t instfunc_jsr_w(interpreter_module* jvm, frame* fr)
{
    int32_t offset = NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;
    offset = (offset << 8) | NEXT_BYTE;

    if (!push_to_stack_operand(&fr->operands, (int32_t)fr->PC, RETADD_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    fr->PC += offset - 5;
    return 1;
}

instruction_fun fetchOpcodeFunction(uint8_t opcode)
{
    const instruction_fun opcodeFunctions[202] = {
        instfunc_nop, instfunc_aconst_null, instfunc_iconst_m1,
        instfunc_iconst_0, instfunc_iconst_1, instfunc_iconst_2,
        instfunc_iconst_3, instfunc_iconst_4, instfunc_iconst_5,
        instfunc_lconst_0, instfunc_lconst_1, instfunc_fconst_0,
        instfunc_fconst_1, instfunc_fconst_2, instfunc_dconst_0,
        instfunc_dconst_1, instfunc_bipush, instfunc_sipush,
        instfunc_ldc, instfunc_ldc_w, instfunc_ldc2_w,
        instfunc_iload, instfunc_lload, instfunc_fload,
        instfunc_dload, instfunc_aload, instfunc_iload_0,
        instfunc_iload_1, instfunc_iload_2, instfunc_iload_3,
        instfunc_lload_0, instfunc_lload_1, instfunc_lload_2,
        instfunc_lload_3, instfunc_fload_0, instfunc_fload_1,
        instfunc_fload_2, instfunc_fload_3, instfunc_dload_0,
        instfunc_dload_1, instfunc_dload_2, instfunc_dload_3,
        instfunc_aload_0, instfunc_aload_1, instfunc_aload_2,
        instfunc_aload_3, instfunc_iaload, instfunc_laload,
        instfunc_faload, instfunc_daload, instfunc_aaload,
        instfunc_baload, instfunc_caload, instfunc_saload,
        instfunc_istore, instfunc_lstore, instfunc_fstore,
        instfunc_dstore, instfunc_astore, instfunc_istore_0,
        instfunc_istore_1, instfunc_istore_2, instfunc_istore_3,
        instfunc_lstore_0, instfunc_lstore_1, instfunc_lstore_2,
        instfunc_lstore_3, instfunc_fstore_0, instfunc_fstore_1,
        instfunc_fstore_2, instfunc_fstore_3, instfunc_dstore_0,
        instfunc_dstore_1, instfunc_dstore_2, instfunc_dstore_3,
        instfunc_astore_0, instfunc_astore_1, instfunc_astore_2,
        instfunc_astore_3, instfunc_iastore, instfunc_lastore,
        instfunc_fastore, instfunc_dastore, instfunc_aastore,
        instfunc_bastore, instfunc_castore, instfunc_sastore,
        instfunc_pop, instfunc_pop2, instfunc_dup,
        instfunc_dup_x1, instfunc_dup_x2, instfunc_dup2,
        instfunc_dup2_x1, instfunc_dup2_x2, instfunc_swap,
        instfunc_iadd, instfunc_ladd, instfunc_fadd,
        instfunc_dadd, instfunc_isub, instfunc_lsub,
        instfunc_fsub, instfunc_dsub, instfunc_imul,
        instfunc_lmul, instfunc_fmul, instfunc_dmul,
        instfunc_idiv, instfunc_ldiv, instfunc_fdiv,
        instfunc_ddiv, instfunc_irem, instfunc_lrem,
        instfunc_frem, instfunc_drem, instfunc_ineg,
        instfunc_lneg, instfunc_fneg, instfunc_dneg,
        instfunc_ishl, instfunc_lshl, instfunc_ishr,
        instfunc_lshr, instfunc_iushr, instfunc_lushr,
        instfunc_iand, instfunc_land, instfunc_ior,
        instfunc_lor, instfunc_ixor, instfunc_lxor,
        instfunc_iinc, instfunc_i2l, instfunc_i2f,
        instfunc_i2d, instfunc_l2i, instfunc_l2f,
        instfunc_l2d, instfunc_f2i, instfunc_f2l,
        instfunc_f2d, instfunc_d2i, instfunc_d2l,
        instfunc_d2f, instfunc_i2b, instfunc_i2c,
        instfunc_i2s, instfunc_lcmp, instfunc_fcmpl,
        instfunc_fcmpg, instfunc_dcmpl, instfunc_dcmpg,
        instfunc_ifeq, instfunc_ifne, instfunc_iflt,
        instfunc_ifge, instfunc_ifgt, instfunc_ifle,
        instfunc_if_icmpeq, instfunc_if_icmpne, instfunc_if_icmplt,
        instfunc_if_icmpge, instfunc_if_icmpgt, instfunc_if_icmple,
        instfunc_if_acmpeq, instfunc_if_acmpne, instfunc_goto,
        instfunc_jsr, instfunc_ret, instfunc_tableswitch,
        instfunc_lookupswitch, instfunc_ireturn, instfunc_lreturn,
        instfunc_freturn, instfunc_dreturn, instfunc_areturn,
        instfunc_return, instfunc_getstatic, instfunc_putstatic,
        instfunc_getfield, instfunc_putfield, instfunc_invokevirtual,
        instfunc_invokespecial, instfunc_invokestatic, instfunc_invokeinterface,
        instfunc_invokedynamic, instfunc_new, instfunc_newarray,
        instfunc_anewarray, instfunc_arraylength, instfunc_athrow,
        instfunc_checkcast, instfunc_instanceof, instfunc_monitorenter,
        instfunc_monitorexit, instfunc_wide, instfunc_multianewarray,
        instfunc_ifnull, instfunc_ifnonnull, instfunc_goto_w,
        instfunc_jsr_w
    };

    if (opcode > 201)
        return NULL;

    return opcodeFunctions[opcode];
}
