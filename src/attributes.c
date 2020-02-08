#include "attributes.h"
#include "readfunctions.h"
#include "constantpool.h"
#include "utf8.h"
#include "opcodes.h"
#include <inttypes.h>
#include <stdlib.h>

#define DECLARE_ATTRIBUTE_FUNCTIONS(attribute) \
    uint8_t read_attribute_##attribute(java_class* jc, attribute_info* entry); \
    void print_attribute_##attribute(java_class* jc, attribute_info* entry, int ident_level); \
    void free_attribute_##attribute(attribute_info* entry);

DECLARE_ATTRIBUTE_FUNCTIONS(SourceFile)
DECLARE_ATTRIBUTE_FUNCTIONS(InnerClasses)
DECLARE_ATTRIBUTE_FUNCTIONS(LineNumberTable)
DECLARE_ATTRIBUTE_FUNCTIONS(ConstantValue)
DECLARE_ATTRIBUTE_FUNCTIONS(Code)
DECLARE_ATTRIBUTE_FUNCTIONS(Deprecated)
DECLARE_ATTRIBUTE_FUNCTIONS(Exceptions)

char attribute_read(java_class* jc, attribute_info* entry)
{
    entry->info = NULL;

    if (!read_2_byte_unsigned(jc, &entry->name_index) ||
        !read_4_byte_unsigned(jc, &entry->length))
    {
        jc->status = UNXPTD_EOF;
        return 0;
    }

    constant_pool_info* cp = jc->constant_pool + entry->name_index - 1;

    if (entry->name_index == 0 ||
        entry->name_index >= jc->constant_pool_count ||
        cp->tag != UTF8_CONST)
    {
        jc->status = INV_NAME_IDX;
        return 0;
    }

    #define IF_ATTRIBUTE_CHECK(name) \
        if (compare_ascii_utf8(cp->Utf8.bytes, cp->Utf8.length, (uint8_t*)#name, sizeof(#name) - 1)) { \
            entry->attr_type = ATTRIBUTE_##name; \
            result = read_attribute_##name(jc, entry); \
        }

    uint32_t total_bytes_read = jc->total_bytes_read;
    char result;

    IF_ATTRIBUTE_CHECK(ConstantValue)
    else IF_ATTRIBUTE_CHECK(SourceFile)
    else IF_ATTRIBUTE_CHECK(InnerClasses)
    else IF_ATTRIBUTE_CHECK(Code)
    else IF_ATTRIBUTE_CHECK(LineNumberTable)
    else IF_ATTRIBUTE_CHECK(Deprecated)
    else IF_ATTRIBUTE_CHECK(Exceptions)
    else
    {
        uint32_t u32;

        for (u32 = 0; u32 < entry->length; u32++)
        {
            if (fgetc(jc->file) == EOF)
            {
                jc->status = UNXPTD_EOF_READING_ATTR_INFO;
                return 0;
            }

            jc->total_bytes_read++;
        }

        result = 1;
        entry->attr_type = ATTRIBUTE_Unknown;
    }

    if (jc->total_bytes_read - total_bytes_read != entry->length)
    {
        jc->status = ATTR_LEN_MISMATCH;
        return 0;
    }

    #undef ATTRIBUTE_CHECK
    return result;
}

void ident(int level)
{
    while (level-- > 0)
        printf("\t");
}

uint8_t read_attribute_Deprecated(java_class* jc, attribute_info* entry)
{
    entry->info = NULL;
    return 1;
}

void print_attribute_Deprecated(java_class* jc, attribute_info* entry, int ident_level)
{
    ident(ident_level);
    printf("This element is marked as deprecated and should no longer be used.");
}

void free_attribute_Deprecated(attribute_info* entry)
{

}

uint8_t read_attribute_ConstantValue(java_class* jc, attribute_info* entry)
{
    atrt_const_value_info* info = (atrt_const_value_info*)malloc(sizeof(atrt_const_value_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    if (!read_2_byte_unsigned(jc, &info->constantvalue_index))
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    if (info->constantvalue_index == 0 ||
        info->constantvalue_index >= jc->constant_pool_count)
    {
        jc->status = ATTR_INV_CONST_VALUE_IDX;
        return 0;
    }

    constant_pool_info* cp = jc->constant_pool + info->constantvalue_index - 1;

    if (cp->tag != STRING_CONST && cp->tag != FLOAT_CONST &&
        cp->tag != DOUBLE_CONST && cp->tag != LONG_CONST &&
        cp->tag != INT_CONST)
    {
        jc->status = ATTR_INV_CONST_VALUE_IDX;
        return 0;
    }

    return 1;
}

void print_attribute_ConstantValue(java_class* jc, attribute_info* entry, int ident_level)
{
    char buffer[48];

    atrt_const_value_info* info = (atrt_const_value_info*)entry->info;
    constant_pool_info* cp = jc->constant_pool + info->constantvalue_index - 1;

    ident(ident_level);
    printf("constantvalue_index: #%u <", info->constantvalue_index);

    switch (cp->tag)
    {
        case INT_CONST:
            printf("%d", (int32_t)cp->Integer.value);
            break;

        case LONG_CONST:
            printf("%" PRId64"", ((int64_t)cp->Long.high << 32) | cp->Long.low);
            break;

        case FLOAT_CONST:
            printf("%e", get_float_from_uint32(cp->Float.bytes));
            break;

        case DOUBLE_CONST:
            printf("%e", get_double_from_uint64((uint64_t)cp->Double.high << 32 | cp->Double.low));
            break;

        case STRING_CONST:
            cp = jc->constant_pool + cp->String.string_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("%s", buffer);
            break;

        default:
            printf(" - unknown constant tag - ");
            break;
    }

    printf(">");
}

void free_attribute_ConstantValue(attribute_info* entry)
{
    if (entry->info)
        free(entry->info);

    entry->info = NULL;
}

uint8_t read_attribute_SourceFile(java_class* jc, attribute_info* entry)
{
    attr_sourcefile_info* info = (attr_sourcefile_info*)malloc(sizeof(attr_sourcefile_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    if (!read_2_byte_unsigned(jc, &info->sourcefile_index))
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    if (info->sourcefile_index == 0 ||
        info->sourcefile_index >= jc->constant_pool_count ||
        jc->constant_pool[info->sourcefile_index - 1].tag != UTF8_CONST)
    {
        jc->status = ATTR_INV_SRC_FILE_IDX;
        return 0;
    }

    return 1;
}

void print_attribute_SourceFile(java_class* jc, attribute_info* entry, int ident_level)
{
    char buffer[48];
    attr_sourcefile_info* info = (attr_sourcefile_info*)entry->info;
    constant_pool_info* cp = jc->constant_pool + info->sourcefile_index - 1;

    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);

    ident(ident_level);
    printf("sourcefile_index: #%u <%s>", info->sourcefile_index, buffer);
}

void free_attribute_SourceFile(attribute_info* entry)
{
    if (entry->info)
        free(entry->info);

    entry->info = NULL;
}

uint8_t read_attribute_InnerClasses(java_class* jc, attribute_info* entry)
{
    attr_inner_classes_info* info = (attr_inner_classes_info*)malloc(sizeof(attr_inner_classes_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    info->inner_classes = NULL;

    if (!read_2_byte_unsigned(jc, &info->number_of_classes))
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    info->inner_classes = (inner_class_info*)malloc(info->number_of_classes * sizeof(inner_class_info));

    if (!info->inner_classes)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    uint16_t u16;
    inner_class_info* icf = info->inner_classes;

    for (u16 = 0; u16 < info->number_of_classes; u16++, icf++)
    {
        if (!read_2_byte_unsigned(jc, &icf->inner_class_index) ||
            !read_2_byte_unsigned(jc, &icf->outer_class_index) ||
            !read_2_byte_unsigned(jc, &icf->inner_class_name_index) ||
            !read_2_byte_unsigned(jc, &icf->inner_class_access_flags))
        {
            jc->status = UNXPTD_EOF_READING_ATTR_INFO;
            return 0;
        }

        if (icf->inner_class_index == 0 ||
            icf->inner_class_index >= jc->constant_pool_count ||
            jc->constant_pool[icf->inner_class_index - 1].tag != CLASS_CONST ||
            icf->outer_class_index >= jc->constant_pool_count ||
            (icf->outer_class_index > 0 && jc->constant_pool[icf->outer_class_index - 1].tag != CLASS_CONST) ||
            icf->inner_class_name_index >= jc->constant_pool_count ||
            (icf->inner_class_name_index > 0 && jc->constant_pool[icf->inner_class_name_index - 1].tag != UTF8_CONST))
        {
            jc->status = ATTR_INV_INNERCLASS_IDXS;
            return 0;
        }
    }

    return 1;
}

void print_attribute_InnerClasses(java_class* jc, attribute_info* entry, int ident_level)
{
    attr_inner_classes_info* info = (attr_inner_classes_info*)entry->info;
    constant_pool_info* cp;
    char buffer[48];
    uint16_t index;
    inner_class_info* innerclass = info->inner_classes;

    ident(ident_level);
    printf("number_of_classes: %u", info->number_of_classes);

    for (index = 0; index < info->number_of_classes; index++, innerclass++)
    {
        printf("\n\n");
        ident(ident_level);
        printf("Inner Class #%u:\n\n", index + 1);

        cp = jc->constant_pool + innerclass->inner_class_index - 1;
        cp = jc->constant_pool + cp->Class.name_index - 1;
        translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
        ident(ident_level + 1);
        printf("inner_class_info_index:   #%u <%s>\n", innerclass->inner_class_index, buffer);

        ident(ident_level + 1);
        printf("outer_class_info_index:   #%u ", innerclass->outer_class_index);

        if (innerclass->outer_class_index == 0)
        {
            printf("(no outer class)\n");
        }
        else
        {
            cp = jc->constant_pool + innerclass->outer_class_index - 1;
            cp = jc->constant_pool + cp->Class.name_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("<%s>\n", buffer);
        }

        ident(ident_level + 1);
        printf("inner_name_index:         #%u ", innerclass->inner_class_name_index);

        if (innerclass->inner_class_name_index == 0)
        {
            printf("(anonymous class)\n");
        }
        else
        {
            cp = jc->constant_pool + innerclass->inner_class_name_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);
            printf("<%s>\n", buffer);
        }

        decode_access_flags(innerclass->inner_class_access_flags, buffer, sizeof(buffer), INNERCLASS_ACCESS_TYPE);
        ident(ident_level + 1);
        printf("inner_class_access_flags: 0x%.4X [%s]", innerclass->inner_class_access_flags, buffer);
    }
}

void free_attribute_InnerClasses(attribute_info* entry)
{
    attr_inner_classes_info* info = (attr_inner_classes_info*)entry->info;

    if (info)
    {
        if (info->inner_classes)
            free(info->inner_classes);

        free(info);
    }

    entry->info = NULL;
}

uint8_t read_attribute_LineNumberTable(java_class* jc, attribute_info* entry)
{
    attr_line_number_table_info* info = (attr_line_number_table_info*)malloc(sizeof(attr_line_number_table_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    info->line_number_table = NULL;

    if (!read_2_byte_unsigned(jc, &info->line_number_table_length))
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    info->line_number_table = (line_number_table_entry*)malloc(info->line_number_table_length * sizeof(line_number_table_entry));

    if (!info->line_number_table)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    uint16_t u16;
    line_number_table_entry* lnte = info->line_number_table;

    for (u16 = 0; u16 < info->line_number_table_length; u16++, lnte++)
    {
        if (!read_2_byte_unsigned(jc, &lnte->start_pc) ||
            !read_2_byte_unsigned(jc, &lnte->line_number))
        {
            jc->status = UNXPTD_EOF_READING_ATTR_INFO;
            return 0;
        }
    }

    return 1;
}

void print_attribute_LineNumberTable(java_class* jc, attribute_info* entry, int ident_level)
{
    attr_line_number_table_info* info = (attr_line_number_table_info*)entry->info;
    line_number_table_entry* lnte = info->line_number_table;
    uint16_t index;

    printf("\n");
    ident(ident_level);
    printf("line_number_table_length: %u\n\n", info->line_number_table_length);
    ident(ident_level);
    printf("Table:\tIndex\tline_number\tstart_pc");

    for (index = 0; index < info->line_number_table_length; index++, lnte++)
    {
        printf("\n");
        ident(ident_level);
        printf("\t%u\t%u\t\t%u", index + 1, lnte->line_number, lnte->start_pc);
    }
}

void free_attribute_LineNumberTable(attribute_info* entry)
{
    attr_line_number_table_info* info = (attr_line_number_table_info*)entry->info;

    if (info)
    {
        if (info->line_number_table)
            free(info->line_number_table);

        free(info);
        entry->info = NULL;
    }
}

uint8_t read_attribute_Code(java_class* jc, attribute_info* entry)
{
    attr_code_info* info = (attr_code_info*)malloc(sizeof(attr_code_info));
    entry->info = (void*)info;
    uint32_t u32;

    if (!info)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    info->code = NULL;
    info->exception_table = NULL;

    if (!read_2_byte_unsigned(jc, &info->max_stack) ||
        !read_2_byte_unsigned(jc, &info->max_locals) ||
        !read_4_byte_unsigned(jc, &info->code_length))
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    if (info->code_length == 0 || info->code_length >= 65536)
    {
        jc->status = ATTR_INV_CODE_LEN;
        return 0;
    }

    info->code = (uint8_t*)malloc(info->code_length);

    if (!info->code)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    if (fread(info->code, sizeof(uint8_t), info->code_length, jc->file) != info->code_length)
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    jc->total_bytes_read += info->code_length;

    // TODO: check if all instructions are valid and have correct parameters.

    if (!read_2_byte_unsigned(jc, &info->exception_table_length))
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    info->exception_table = (exception_table_entry*)malloc(info->exception_table_length * sizeof(exception_table_entry));

    if (!info->exception_table)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    exception_table_entry* except = info->exception_table;

    for (u32 = 0; u32 < info->exception_table_length; u32++)
    {
        if (!read_2_byte_unsigned(jc, &except->start_pc) ||
            !read_2_byte_unsigned(jc, &except->end_pc) ||
            !read_2_byte_unsigned(jc, &except->handler_pc) ||
            !read_2_byte_unsigned(jc, &except->catch_type))
        {
            jc->status = UNXPTD_EOF_READING_ATTR_INFO;
            return 0;
        }

        except++;
    }

    if (!read_2_byte_unsigned(jc, &info->attributes_count))
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    info->attributes = (attribute_info*)malloc(info->attributes_count * sizeof(attribute_info));

    if (!info->attributes)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    for (u32 = 0; u32 < info->attributes_count; u32++)
    {
        if (!attribute_read(jc, info->attributes + u32))
        {
            info->attributes_count = u32 + 1;
            return 0;
        }
    }

    return 1;
}

void print_attribute_Code(java_class* jc, attribute_info* entry, int ident_level)
{
    attr_code_info* info = (attr_code_info*)entry->info;
    uint32_t code_offset;

    printf("\n");
    ident(ident_level);
    printf("max_stack: %u, max_locals: %u, code_length: %u\n", info->max_stack, info->max_locals, info->code_length);
    ident(ident_level);
    printf("exception_table_length: %u, attribute_count: %u\n\n", info->exception_table_length, info->attributes_count);
    ident(ident_level);
    printf("Code:\tOffset\tMnemonic\tParameters");

    ident_level++;

    char buffer[48];
    uint8_t opcode;
    uint32_t u32;
    constant_pool_info* cpi;

    for (code_offset = 0; code_offset < info->code_length; code_offset++)
    {
        opcode = *(info->code + code_offset);

        printf("\n");
        ident(ident_level);
        printf("%u\t%s", code_offset, get_opcode_mnemonic(opcode));

        #define OPCODE_CHECK_INTERVAL(begin, end) (opcode >= opcode_##begin && opcode <= opcode_##end)

        if (OPCODE_CHECK_INTERVAL(nop, dconst_1) || OPCODE_CHECK_INTERVAL(iload_0, saload) ||
            OPCODE_CHECK_INTERVAL(istore_0, lxor) || OPCODE_CHECK_INTERVAL(i2l, dcmpg) ||
            OPCODE_CHECK_INTERVAL(ireturn, return) || OPCODE_CHECK_INTERVAL(arraylength, athrow) ||
            OPCODE_CHECK_INTERVAL(monitorenter, monitorexit))
        {
            continue;
        }

        #undef OPCODE_CHECK_INTERVAL

        #define NEXT_BYTE (*(info->code + ++code_offset))

        switch (opcode)
        {
            case opcode_iload:
            case opcode_fload:
            case opcode_dload:
            case opcode_lload:
            case opcode_aload:
            case opcode_istore:
            case opcode_lstore:
            case opcode_fstore:
            case opcode_dstore:
            case opcode_astore:
            case opcode_ret:

                printf("\t\t%u", NEXT_BYTE);
                break;

            case opcode_newarray:

                u32 = NEXT_BYTE;
                printf("\t%u (array of %s)", u32, decode_opcode_newarraytype((uint8_t)u32));
                break;

            case opcode_bipush:

                printf("\t\t%d", (int8_t)NEXT_BYTE);
                break;

            case opcode_getfield:
            case opcode_getstatic:
            case opcode_putfield:
            case opcode_putstatic:
            case opcode_invokevirtual:
            case opcode_invokespecial:
            case opcode_invokestatic:

                u32 = NEXT_BYTE;
                u32 = (u32 << 8) | NEXT_BYTE;

                printf("\t#%u ", u32);
                cpi = jc->constant_pool + u32 - 1;

                if ((opcode <  opcode_invokevirtual && cpi->tag == FIELDREF_CONST) ||
                    (opcode >= opcode_invokevirtual && cpi->tag == METHODREF_CONST))
                {
                    cpi = jc->constant_pool + cpi->Fieldref.class_index - 1;
                    cpi = jc->constant_pool + cpi->Class.name_index - 1;
                    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("(%s %s.", opcode < opcode_invokevirtual ? "Field" : "Method", buffer);

                    cpi = jc->constant_pool + u32 - 1;
                    cpi = jc->constant_pool + cpi->Fieldref.name_and_type_index - 1;
                    u32 = cpi->NameAndType.descriptor_index;
                    cpi = jc->constant_pool + cpi->NameAndType.name_index - 1;
                    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("%s, descriptor: ", buffer);

                    cpi = jc->constant_pool + u32 - 1;
                    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("%s)", buffer);
                }
                else
                {
                    printf("(%s, invalid - not a %s)", tag_decoding(cpi->tag), opcode < opcode_invokevirtual ? "Field" : "Method");
                }

                break;

            case opcode_invokedynamic:

                u32 = NEXT_BYTE;
                u32 = (u32 << 8) | NEXT_BYTE;

                printf("\t#%u - INVOKEDYNAMIC_CONST not implemented -", u32);

                u32 = NEXT_BYTE;

                if (u32 != 0)
                {
                    printf("\n");
                    ident(ident_level);
                    printf("%u\t- expected a zero byte in this offset due to invokedynamic, found 0x%.2X instead -", code_offset, u32);
                }

                u32 = NEXT_BYTE;

                if (u32 != 0)
                {
                    printf("\n");
                    ident(ident_level);
                    printf("%u\t- expected a zero byte in this offset due to invokedynamic, found 0x%.2X instead -", code_offset, u32);
                }

                break;

            case opcode_invokeinterface:

                u32 = NEXT_BYTE;
                u32 = (u32 << 8) | NEXT_BYTE;

                printf("\t#%u, count: %u ", u32, NEXT_BYTE);

                cpi = jc->constant_pool + u32 - 1;

                if (cpi->tag == INTERFACEMETHODREF_CONST)
                {
                    cpi = jc->constant_pool + cpi->InterfaceMethodref.class_index - 1;
                    cpi = jc->constant_pool + cpi->Class.name_index - 1;
                    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("(InterfaceMethod %s.", buffer);

                    cpi = jc->constant_pool + u32 - 1;
                    cpi = jc->constant_pool + cpi->InterfaceMethodref.name_and_type_index - 1;
                    u32 = cpi->NameAndType.descriptor_index;
                    cpi = jc->constant_pool + cpi->NameAndType.name_index - 1;
                    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("%s, descriptor: ", buffer);

                    cpi = jc->constant_pool + u32 - 1;
                    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf("%s)", buffer);
                }
                else
                {
                    printf("(%s, invalid - not a InterfaceMethod)", tag_decoding(cpi->tag));
                }

                u32 = NEXT_BYTE;

                if (u32 != 0)
                {
                    printf("\n");
                    ident(ident_level);
                    printf("%u\t- expected a zero byte in this offset due to invokeinterface, found 0x%.2X instead -", code_offset, u32);
                }

                break;

            case opcode_ifeq:
            case opcode_ifne:
            case opcode_iflt:
            case opcode_ifge:
            case opcode_ifgt:
            case opcode_ifle:
            case opcode_goto:
            case opcode_jsr:
            case opcode_sipush:
            case opcode_ifnull:

                printf("\t");

            case opcode_if_icmpeq:
            case opcode_if_icmpne:
            case opcode_if_icmplt:
            case opcode_if_icmpge:
            case opcode_if_icmpgt:
            case opcode_if_icmple:
            case opcode_if_acmpeq:
            case opcode_if_acmpne:
            case opcode_ifnonnull:

                u32 = (uint16_t)NEXT_BYTE << 8;
                u32 |= NEXT_BYTE;

                printf("\tpc + %d = address %d", (int16_t)u32, (int16_t)u32 + code_offset - 2);
                break;

            case opcode_goto_w:
            case opcode_jsr_w:

                u32 = NEXT_BYTE;
                u32 = (u32 << 8) | NEXT_BYTE;
                u32 = (u32 << 8) | NEXT_BYTE;
                u32 = (u32 << 8) | NEXT_BYTE;
                printf("\t\t%d", (int32_t)u32);
                break;

            case opcode_iinc:

                printf("\t\t%u,", NEXT_BYTE);
                printf(" %d", (int8_t)NEXT_BYTE);
                break;

            case opcode_new:
            case opcode_anewarray:
            case opcode_checkcast:
            case opcode_instanceof:
            case opcode_multianewarray:

                u32 = NEXT_BYTE;
                u32 = (u32 << 8) | NEXT_BYTE;

                if (opcode == opcode_new)
                    printf("\t");

                printf("\t#%u", u32);

                if (opcode == opcode_multianewarray)
                    printf(", dimension %u", NEXT_BYTE);

                cpi = jc->constant_pool + u32 - 1;

                if (cpi->tag == CLASS_CONST)
                {
                    cpi = jc->constant_pool + cpi->Class.name_index - 1;
                    translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                    printf(" (class: %s)", buffer);
                }
                else
                {
                    printf(" (%s, invalid - not a class)", tag_decoding(cpi->tag));
                }

                break;

            case opcode_ldc:
            case opcode_ldc_w:
            case opcode_ldc2_w:

                u32 = NEXT_BYTE;

                if (opcode == opcode_ldc_w || opcode == opcode_ldc2_w)
                    u32 = (u32 << 8) | NEXT_BYTE;

                printf("\t\t#%u", u32);

                cpi = jc->constant_pool + u32 - 1;

                if (opcode == opcode_ldc2_w)
                {
                    if (cpi->tag == LONG_CONST)
                        printf(" (long: %" PRId64")", ((int64_t)cpi->Long.high << 32) | cpi->Long.low);
                    else if (cpi->tag == DOUBLE_CONST)
                        printf(" (double: %e)", get_double_from_uint64((uint64_t)cpi->Double.high << 32 | cpi->Double.low));
                    else
                        printf(" (%s, invalid)", decode_opcode_newarraytype(cpi->tag));
                }
                else
                {
                    if (cpi->tag == CLASS_CONST)
                    {
                        cpi = jc->constant_pool + cpi->Class.name_index - 1;
                        translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                        printf(" (class: %s)", buffer);
                    }
                    else if (cpi->tag == STRING_CONST)
                    {
                        cpi = jc->constant_pool + cpi->String.string_index - 1;
                        translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                        printf(" (string: %s)", buffer);
                    }
                    else if (cpi->tag == INT_CONST)
                    {
                        printf(" (integer: %d)", (int32_t)cpi->Integer.value);
                    }
                    else if (cpi->tag == FLOAT_CONST)
                    {
                        printf(" (float: %e)", get_float_from_uint32(cpi->Float.bytes));
                    }
                    else
                    {
                        printf(" (%s, invalid)", tag_decoding(cpi->tag));
                    }
                }

                break;

            case opcode_wide:

                opcode = NEXT_BYTE;

                switch (opcode)
                {
                    case opcode_iload:
                    case opcode_fload:
                    case opcode_dload:
                    case opcode_lload:
                    case opcode_aload:
                    case opcode_istore:
                    case opcode_lstore:
                    case opcode_fstore:
                    case opcode_dstore:
                    case opcode_astore:
                    case opcode_ret:

                        u32 = NEXT_BYTE;
                        u32 = (u32 << 8) | NEXT_BYTE;

                        printf(" %s\t%u", get_opcode_mnemonic(opcode), u32);

                        break;

                    case opcode_iinc:

                        u32 = NEXT_BYTE;
                        u32 = (u32 << 8) | NEXT_BYTE;

                        printf(" iinc\t%u,", u32);

                        u32 = NEXT_BYTE;
                        u32 = (u32 << 8) | NEXT_BYTE;

                        printf(" %d", (int16_t)u32);

                        break;

                    default:
                        printf(" %s (invalid following instruction, can't continue)", get_opcode_mnemonic(opcode));
                        code_offset = info->code_length;
                        break;
                }

                break;

            case opcode_tableswitch:
            {
                uint32_t base_address = code_offset;

                while ((code_offset + 1) % 4)
                    u32 = NEXT_BYTE;

                int32_t default_value = NEXT_BYTE;
                default_value = (default_value << 8) | NEXT_BYTE;
                default_value = (default_value << 8) | NEXT_BYTE;
                default_value = (default_value << 8) | NEXT_BYTE;

                int32_t low_value = NEXT_BYTE;
                low_value = (low_value << 8) | NEXT_BYTE;
                low_value = (low_value << 8) | NEXT_BYTE;
                low_value = (low_value << 8) | NEXT_BYTE;

                int32_t high_value = NEXT_BYTE;
                high_value = (high_value << 8) | NEXT_BYTE;
                high_value = (high_value << 8) | NEXT_BYTE;
                high_value = (high_value << 8) | NEXT_BYTE;

                if (low_value > high_value)
                {
                    printf("\tinvalid operands - low_value (%d) is greater than high_value (%d)\n", low_value, high_value);
                    ident(ident_level);
                    printf("- can't continue -");
                    code_offset = info->code_length;
                    break;
                }
                else
                {
                    printf("\tlow = %d, high = %d", low_value, high_value);
                }

                int32_t offset;

                for (u32 = 0; u32 < (uint32_t)(high_value - low_value + 1); u32++)
                {
                    printf("\n");
                    ident(ident_level);
                    printf("%u\t", code_offset);

                    offset = NEXT_BYTE;
                    offset = (offset << 8) | NEXT_BYTE;
                    offset = (offset << 8) | NEXT_BYTE;
                    offset = (offset << 8) | NEXT_BYTE;

                    printf("  case %d: pc + %d = address %d", low_value + u32, offset, offset + base_address);
                }

                printf("\n");
                ident(ident_level);
                printf("-\t  default: pc + %d = address %d", default_value, default_value + base_address);

                break;
            }

            case opcode_lookupswitch:
            {
                uint32_t base_address = code_offset;

                while ((code_offset + 1) % 4)
                    u32 = NEXT_BYTE;

                int32_t default_value = NEXT_BYTE;
                default_value = (default_value << 8) | NEXT_BYTE;
                default_value = (default_value << 8) | NEXT_BYTE;
                default_value = (default_value << 8) | NEXT_BYTE;

                int32_t npairs = NEXT_BYTE;
                npairs = (npairs << 8) | NEXT_BYTE;
                npairs = (npairs << 8) | NEXT_BYTE;
                npairs = (npairs << 8) | NEXT_BYTE;

                if (npairs < 0)
                {
                    printf("\tinvalid operand - npairs (%d) should be greater than or equal to 0\n", npairs);
                    ident(ident_level);
                    printf("- can't continue -");
                    code_offset = info->code_length;
                    break;
                }
                else
                {
                    printf("\tnpairs = %d", npairs);
                }

                int32_t match, offset;

                while (npairs-- > 0)
                {
                    printf("\n");
                    ident(ident_level);
                    printf("%u\t", code_offset);

                    match = NEXT_BYTE;
                    match = (match << 8) | NEXT_BYTE;
                    match = (match << 8) | NEXT_BYTE;
                    match = (match << 8) | NEXT_BYTE;

                    offset = NEXT_BYTE;
                    offset = (offset << 8) | NEXT_BYTE;
                    offset = (offset << 8) | NEXT_BYTE;
                    offset = (offset << 8) | NEXT_BYTE;

                    printf("  case %d: pc + %d = address %d", match, offset, offset + base_address);
                }

                printf("\n");
                ident(ident_level);
                printf("-\t  default: pc + %d = address %d", default_value, default_value + base_address);

                break;
            }


            default:
                printf("\n");
                ident(ident_level);
                printf("- last instruction was not recognized, can't continue -");
                code_offset = info->code_length;
                break;
        }
    }

    printf("\n");
    ident_level--;

    if (info->exception_table_length > 0)
    {
        printf("\n");
        ident(ident_level);
        printf("Exception Table:\n");
        ident(ident_level);
        printf("Index\tstart_pc\tend_pc\thandler_pc\tcatch_type");

        exception_table_entry* except = info->exception_table;

        for (u32 = 0; u32 < info->exception_table_length; u32++)
        {
            printf("\n");
            ident(ident_level);
            printf("%u\t%u\t\t%u\t%u\t\t%u", u32 + 1, except->start_pc, except->end_pc, except->handler_pc, except->catch_type);

            if (except->catch_type > 0)
            {
                cpi = jc->constant_pool + except->catch_type - 1;
                cpi = jc->constant_pool + cpi->Class.name_index - 1;
                translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);
                printf(" <%s>", buffer);
            }

            except++;
        }

        printf("\n");
    }

    if (info->attributes_count > 0)
    {
        for (u32 = 0; u32 < info->attributes_count; u32++)
        {
            attribute_info* atti = info->attributes + u32;
            cpi = jc->constant_pool + atti->name_index - 1;
            translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

            printf("\n");
            ident(ident_level);
            printf("Code Attribute #%u - %s:\n", u32 + 1, buffer);
            attribute_print(jc, atti, ident_level + 1);
        }
    }
}

void free_attribute_Code(attribute_info* entry)
{
    attr_code_info* info = (attr_code_info*)entry->info;

    if (info)
    {
        if (info->code)
            free(info->code);

        if (info->exception_table)
            free(info->exception_table);

        if (info->attributes)
        {
            uint16_t u16;

            for (u16 = 0; u16 < info->attributes_count; u16++)
                free_attribute_info(info->attributes + u16);

            free(info->attributes);
        }

        free(info);
        entry->info = NULL;
    }
}

uint8_t read_attribute_Exceptions(java_class* jc, attribute_info* entry)
{
    attr_exceptions_info* info = (attr_exceptions_info*)malloc(sizeof(attr_exceptions_info));
    entry->info = (void*)info;

    if (!info)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    info->exception_index_table = NULL;

    if (!read_2_byte_unsigned(jc, &info->number_of_exceptions))
    {
        jc->status = UNXPTD_EOF_READING_ATTR_INFO;
        return 0;
    }

    info->exception_index_table = (uint16_t*)malloc(info->number_of_exceptions * sizeof(uint16_t));

    if (!info->exception_index_table)
    {
        jc->status = MEM_ALLOC_FAILED;
        return 0;
    }

    uint16_t u16;
    uint16_t* exception_index = info->exception_index_table;

    for (u16 = 0; u16 < info->number_of_exceptions; u16++, exception_index++)
    {
        if (!read_2_byte_unsigned(jc, exception_index))
        {
            jc->status = UNXPTD_EOF_READING_ATTR_INFO;
            return 0;
        }

        if (*exception_index == 0 ||
            *exception_index >= jc->constant_pool_count ||
            jc->constant_pool[*exception_index - 1].tag != CLASS_CONST)
        {
            jc->status = ATTR_INV_EXC_CLASS_IDX;
            return 0;
        }
    }

    return 1;
}

void print_attribute_Exceptions(java_class* jc, attribute_info* entry, int ident_level)
{
    attr_exceptions_info* info = (attr_exceptions_info*)entry->info;
    uint16_t* exception_index = info->exception_index_table;
    uint16_t index;
    char buffer[48];
    constant_pool_info* cpi;

    printf("\n");
    ident(ident_level);
    printf("number_of_exceptions: %u", info->number_of_exceptions);

    for (index = 0; index < info->number_of_exceptions; index++, exception_index++)
    {
        cpi = jc->constant_pool + *exception_index - 1;
        cpi = jc->constant_pool + cpi->Class.name_index - 1;
        translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cpi->Utf8.bytes, cpi->Utf8.length);

        printf("\n\n");
        ident(ident_level + 1);
        printf("Exception #%u: #%u <%s>\n", index + 1, *exception_index, buffer);
    }
}

void free_attribute_Exceptions(attribute_info* entry)
{
    attr_exceptions_info* info = (attr_exceptions_info*)entry->info;

    if (info)
    {
        if (info->exception_index_table)
            free(info->exception_index_table);

        free(info);
        entry->info = NULL;
    }
}

void free_attribute_info(attribute_info* entry)
{
    #define ATTRIBUTE_CASE(attribute) case ATTRIBUTE_##attribute: free_attribute_##attribute(entry); return;

    switch (entry->attr_type)
    {
        ATTRIBUTE_CASE(Code)
        ATTRIBUTE_CASE(LineNumberTable)
        ATTRIBUTE_CASE(SourceFile)
        ATTRIBUTE_CASE(InnerClasses)
        ATTRIBUTE_CASE(ConstantValue)
        ATTRIBUTE_CASE(Deprecated)
        ATTRIBUTE_CASE(Exceptions)
        default:
            break;
    }

    #undef ATTRIBUTE_CASE
}

void attribute_print(java_class* jc, attribute_info* entry, int ident_level)
{
    #define ATTRIBUTE_CASE(attribute) case ATTRIBUTE_##attribute: print_attribute_##attribute(jc, entry, ident_level); break;

    switch (entry->attr_type)
    {
        ATTRIBUTE_CASE(Code)
        ATTRIBUTE_CASE(ConstantValue)
        ATTRIBUTE_CASE(InnerClasses)
        ATTRIBUTE_CASE(SourceFile)
        ATTRIBUTE_CASE(LineNumberTable)
        ATTRIBUTE_CASE(Deprecated)
        ATTRIBUTE_CASE(Exceptions)
        default:
            ident(ident_level);
            printf("Attribute not implemented and ignored.");
            break;
    }

    printf("\n");

    #undef ATTRIBUTE_CASE
}

void attributes_print_all(java_class* jc)
{
    if (jc->attribute_count == 0)
        return;

    uint16_t u16;
    char buffer[48];
    constant_pool_info* cp;
    attribute_info* atti;

    printf("\n---- Class Attributes ----");

    for (u16 = 0; u16 < jc->attribute_count; u16++)
    {
        atti = jc->attributes + u16;
        cp = jc->constant_pool + atti->name_index - 1;
        translate_from_utf8_to_ascii((uint8_t*)buffer, sizeof(buffer), cp->Utf8.bytes, cp->Utf8.length);

        printf("\n\n\tAttribute #%u - %s:\n\n", u16 + 1, buffer);
        attribute_print(jc, atti, 2);
    }
}

attribute_info* get_attribute_using_type(attribute_info* attributes, uint16_t attributes_length, enum attribute_type type)
{
    while (attributes_length-- > 0)
    {
        if (attributes->attr_type == type)
            return attributes;

        attributes++;
    }

    return NULL;
}
