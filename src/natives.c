#include "readfunctions.h"
#include "natives.h"
#include "jvm.h"
#include "utf8.h"
#include <string.h>
#include <inttypes.h>
#include <time.h>

#define HI_WORD(x) (((int32_t)(x >> 32)))
#define LO_WORD(x) (((int32_t)(x & 0xFFFFFFFFll)))

uint8_t native_println(interpreter_module* jvm, frame* fr, const uint8_t* descriptor_utf8, int32_t length_utf8)
{
    int64_t long_value = 0;
    int32_t high, low;

    if (length_utf8 <=1 )
    {
        DEBUG_REPORT_ERROR_INSTRUCTION
        return 0;
    }

    switch (descriptor_utf8[1])
    {
        case ')': break;

        case 'Z':
            pop_from_stack_operand(&fr->operands, &low, NULL);
            printf("%s", (int8_t)low ? "true" : "false");
            break;

        case 'B':
            pop_from_stack_operand(&fr->operands, &low, NULL);
            printf("%d", (int8_t)low);
            break;

        case 'C':
            pop_from_stack_operand(&fr->operands, &low, NULL);
            if (low <= 127)
                printf("%c", (char)low);
            else
                printf("%c%c", (int16_t)low >> 8, (int16_t)low & 0xFF);
            break;

        case 'D':
        case 'J':
            pop_from_stack_operand(&fr->operands, &low, NULL);
            pop_from_stack_operand(&fr->operands, &high, NULL);
            long_value = high;
            long_value = (long_value << 32) | (uint32_t)low;

            if (descriptor_utf8[1] == 'D')
                printf("%#f", get_double_from_uint64(long_value));
            else
                printf("%" PRId64"", long_value);

            break;

        case 'F':
            pop_from_stack_operand(&fr->operands, &low, NULL);
            printf("%#f", get_float_from_uint32(low));
            break;

        case 'I':
            pop_from_stack_operand(&fr->operands, &low, NULL);
            printf("%d", low);
            break;

        case 'L':
        {
            pop_from_stack_operand(&fr->operands, &low, NULL);
            reference* obj = (reference*)low;

            if (obj->type == REF_TYPE_STRING)
            {
                uint8_t* bytes = obj->str.utf8_bytes;
                int32_t len = obj->str.len;

                if (len > 0)
                    printf("%.*s", len, bytes);

            }
            else
                printf("0x%.8X", low);

            break;
        }

        case '[':
            pop_from_stack_operand(&fr->operands, &low, NULL);
            printf("0x%X", low);
            break;

        default:
            break;
    }

    printf("\n");

    pop_from_stack_operand(&fr->operands, NULL, NULL);

    return 1;
}

uint8_t native_currentTimeMillis(interpreter_module* jvm, frame* fr, const uint8_t* descriptor_utf8, int32_t length_utf8)
{
    int64_t seconds = (int64_t)time(NULL) * 1000;

    if (!push_to_stack_operand(&fr->operands, HI_WORD(seconds), LONG_OP) ||
        !push_to_stack_operand(&fr->operands, LO_WORD(seconds), LONG_OP))
    {
        jvm->status = OUT_OF_MEMORY;
        return 0;
    }

    fr->return_count = 2;
    return 1;
}

native_func get_native_func(const uint8_t* class_name, int32_t class_length, const uint8_t* method_name, int32_t method_length, const uint8_t* descriptor, int32_t descriptor_length)
{
    const struct {
        char* classname; int32_t classlength;
        char* methodname; int32_t methodlength;
        char* descriptor; int32_t descrlength;
        native_func func;
    } native_methods[] = {
        {"java/io/PrintStream", 19, "println", 7, NULL, 0, native_println},
        {"java/lang/System", 16, "currentTimeMillis", 17, NULL, 0, native_currentTimeMillis},
    };

    uint32_t idx;

    for (idx = 0; idx < sizeof(native_methods)/sizeof(*native_methods); idx++)
    {
        if (compare_utf8((uint8_t*)native_methods[idx].methodname, native_methods[idx].methodlength, method_name, method_length) &&
            compare_utf8((uint8_t*)native_methods[idx].classname, native_methods[idx].classlength, class_name, class_length))
        {
            if (!native_methods[idx].descriptor ||
                compare_utf8((uint8_t*)native_methods[idx].descriptor, native_methods[idx].descrlength, descriptor, descriptor_length))
            {
                return native_methods[idx].func;
            }
        }
    }

    return NULL;
}
