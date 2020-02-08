#include <string.h>
#include "jvm.h"
#include "utf8.h"
#include "natives.h"
#include "instructions.h"

const char* get_general_status_msg(enum general_status status)
{
  switch (status)
  {
    case OK: return "Ok";
    case NO_CLASS_LOADED: return "No class loaded";
    case MAIN_CLASS_RESOLUTION_FAILED: return "Main class resolution failed";
    case CLASS_RESOLUTION_FAILED: return "Class resolution failed";
    case METHOD_RESOLUTION_FAILED: return "Method resolution failed";
    case FIELD_RESOLUTION_FAILED: return "Field resolution failed";
    case UNKNOWN_INSTRUCTION: return "Unknown instruction";
    case OUT_OF_MEMORY: return "Out of memory";
    case MAIN_METHOD_NOT_FOUND: return "Main method not found";
    case INVALID_INSTRUCTION_PARAMETERS: return "Invalid instruction parameters";
  }

  return "Unknown status";
}

void initialize_virtual_machine(interpreter_module* virtual_machine)
{
    virtual_machine->status = OK;
    virtual_machine->frames = NULL;
    virtual_machine->classes = NULL;
    virtual_machine->objects = NULL;

    virtual_machine->class_path[0] = '\0';

    virtual_machine->sys_and_str_classes_simulation = 1;
}

void deinitialize_virtual_machine(interpreter_module* virtual_machine)
{
    free_stack_frame(&virtual_machine->frames);

    loaded_classes* classnode = virtual_machine->classes;
    loaded_classes* classtmp;

    while (classnode)
    {
        classtmp = classnode;
        classnode = classnode->next;
        close_class_file(classtmp->jc);
        free(classtmp->jc);

        if (classtmp->static_data)
            free(classtmp->static_data);

        free(classtmp);
    }

    reference_table* refnode = virtual_machine->objects;
    reference_table* reftmp;

    while (refnode)
    {
        reftmp = refnode;
        refnode = refnode->next;
        delete_reference(reftmp->obj);
        free(reftmp);
    }

    virtual_machine->objects = NULL;
    virtual_machine->classes = NULL;
}

void interpret_cl(interpreter_module* virtual_machine, loaded_classes* main_class)
{
    if (!main_class)
    {
        if (!virtual_machine->classes || !virtual_machine->classes->jc)
        {
            virtual_machine->status = NO_CLASS_LOADED;
            return;
        }

        main_class = virtual_machine->classes;
    }
    else if (!initialize_class(virtual_machine, main_class))
    {
        virtual_machine->status = MAIN_CLASS_RESOLUTION_FAILED;
        return;
    }

    const uint8_t name[] = "main";
    const uint8_t descriptor[] = "([Ljava/lang/String;)V";

    method_info* method = get_matching_method(main_class->jc, name, sizeof(name) - 1, descriptor, sizeof(descriptor) - 1, STATIC_ACCESS_FLAG);

    if (!method)
    {
        virtual_machine->status = MAIN_METHOD_NOT_FOUND;
        return;
    }

    if (!run_method(virtual_machine, main_class->jc, method, 0))
        return;
}

void set_class_path(interpreter_module* virtual_machine, const char* path)
{
    uint32_t index;
    uint32_t last_slash = 0;

    for (index = 0; path[index]; index++)
    {
        if (path[index] == '/' || path[index] == '\\')
            last_slash = index;
    }

    if (last_slash)
        memcpy(virtual_machine->class_path, path, last_slash + 1);
    else
        virtual_machine->class_path[0] = '\0';
}

uint8_t class_handler(interpreter_module* virtual_machine, const uint8_t* className_utf8_bytes, int32_t utf8_length, loaded_classes** output_class)
{
    java_class* jc;
    constant_pool_info* cpi;
    char path[1024];
    uint8_t success = 1;
    uint16_t u16;

    if (virtual_machine->sys_and_str_classes_simulation &&
        compare_utf8(className_utf8_bytes, utf8_length, (const uint8_t*)"java/lang/String", 16))
    {
        return 1;
    }

    if (utf8_length > 0 && *className_utf8_bytes == '[')
    {
        do {
            utf8_length--;
            className_utf8_bytes++;
        } while (utf8_length > 0 && *className_utf8_bytes == '[');

        if (utf8_length == 0)
        {
            return 0;
        }
        else if (*className_utf8_bytes != 'L')
        {
            if (output_class)
                *output_class = NULL;
            return 1;
        }
        else
        {
            return class_handler(virtual_machine, className_utf8_bytes + 1, utf8_length - 2, output_class);
        }
    }

    loaded_classes* loaded_class = class_is_already_loaded(virtual_machine, className_utf8_bytes, utf8_length);

    if (loaded_class)
    {
        if (output_class)
            *output_class = loaded_class;

        return 1;
    }


    snprintf(path, sizeof(path), "%.*s.class", utf8_length, className_utf8_bytes);

    jc = (java_class*)malloc(sizeof(java_class));
    open_class_file(jc, path);

    if (jc->status == CLASS_STA_FILE_CN_BE_OPENED && virtual_machine->class_path[0])
    {
        snprintf(path, sizeof(path), "%s%.*s.class", virtual_machine->class_path, utf8_length, className_utf8_bytes);
        open_class_file(jc, path);
    }

    if (jc->status != CLASS_STA_OK)
    {


        success = 0;
    }
    else
    {
        if (jc->super_class)
        {
            cpi = jc->constant_pool + jc->super_class - 1;
            cpi = jc->constant_pool + cpi->Class.name_index - 1;
            success = class_handler(virtual_machine, UTF8(cpi), &loaded_class);

            if (success)
            {
                jc->instance_field_count += loaded_class->jc->instance_field_count;

                for (u16 = 0; u16 < jc->field_count; u16++)
                    jc->fields[u16].offset += loaded_class->jc->instance_field_count;
            }

        }

        for (u16 = 0; success && u16 < jc->interface_count; u16++)
        {
            cpi = jc->constant_pool + jc->interfaces[u16] - 1;
            cpi = jc->constant_pool + cpi->Class.name_index - 1;
            success = class_handler(virtual_machine, UTF8(cpi), NULL);
        }
    }

    if (success)
    {
        loaded_class = add_class_to_loaded_classes(virtual_machine, jc);
        success = loaded_class != NULL;
    }

    if (success)
    {


        if (output_class)
            *output_class = loaded_class;
    }
    else
    {
        virtual_machine->status = CLASS_RESOLUTION_FAILED;
        close_class_file(jc);
        free(jc);
    }

    return success;
}

uint8_t method_handler(interpreter_module* virtual_machine, java_class* jc, constant_pool_info* cp_method, loaded_classes** output_class)
{

    constant_pool_info* cpi;

    cpi = jc->constant_pool + cp_method->Methodref.class_index - 1;
    cpi = jc->constant_pool + cpi->Class.name_index - 1;

    if (!class_handler(virtual_machine, UTF8(cpi), output_class))
        return 0;

    cpi = jc->constant_pool + cp_method->Methodref.name_and_type_index - 1;
    cpi = jc->constant_pool + cpi->NameAndType.descriptor_index - 1;

    uint8_t* descriptor_bytes = cpi->Utf8.bytes;
    int32_t descriptor_len = cpi->Utf8.length;
    int32_t length;

    while (descriptor_len > 0)
    {
        descriptor_bytes++;
        descriptor_len--;

        switch(*descriptor_bytes)
        {
            case 'L':

                length = -1;

                do {
                    descriptor_bytes++;
                    descriptor_len--;
                    length++;
                } while (*descriptor_bytes != ';');

                if (!class_handler(virtual_machine, descriptor_bytes - length, length, NULL))
                    return 0;

                break;

            default:
                break;
        }
    }

    return 1;
}

uint8_t field_handler(interpreter_module* virtual_machine, java_class* jc, constant_pool_info* cp_field, loaded_classes** output_class)
{


    constant_pool_info* cpi;

    cpi = jc->constant_pool + cp_field->Fieldref.class_index - 1;
    cpi = jc->constant_pool + cpi->Class.name_index - 1;

    if (!class_handler(virtual_machine, UTF8(cpi), output_class))
        return 0;

    cpi = jc->constant_pool + cp_field->Fieldref.name_and_type_index - 1;
    cpi = jc->constant_pool + cpi->NameAndType.descriptor_index - 1;

    uint8_t* descriptor_bytes = cpi->Utf8.bytes;
    int32_t descriptor_len = cpi->Utf8.length;

    while (*descriptor_bytes == '[')
    {
        descriptor_bytes++;
        descriptor_len--;
    }

    if (*descriptor_bytes == 'L')
    {
        if (!class_handler(virtual_machine, descriptor_bytes + 1, descriptor_len - 2, NULL))
            return 0;
    }

    return 1;
}

uint8_t run_method(interpreter_module* virtual_machine, java_class* jc, method_info* method, uint8_t parameters_amount)
{

    frame* caller_frame = virtual_machine->frames ? virtual_machine->frames->fr : NULL;
    frame* fr = create_new_frame(jc, method);


    if (!fr || !push_to_stack_frame(&virtual_machine->frames, fr))
    {
        virtual_machine->status = OUT_OF_MEMORY;
        return 0;
    }

    uint8_t parameterIndex;
    int32_t parameter;

    for (parameterIndex = 0; parameterIndex < parameters_amount; parameterIndex++)
    {
        pop_from_stack_operand(&caller_frame->operands, &parameter, NULL);
        fr->local_vars[parameters_amount - parameterIndex - 1] = parameter;
    }

    if (method->access_flags & NATIVE_ACCESS_FLAG)
    {
        constant_pool_info* className = jc->constant_pool + jc->this_class - 1;
        className = jc->constant_pool + className->Class.name_index - 1;

        constant_pool_info* methodName = jc->constant_pool + method->name_index - 1;
        constant_pool_info* descriptor = jc->constant_pool + method->descriptor_index - 1;

        native_func native = get_native_func(UTF8(className), UTF8(methodName), UTF8(descriptor));

        if (native)
            native(virtual_machine, fr, UTF8(descriptor));
    }
    else
    {
        instruction_fun function;

        while (fr->PC < fr->bytecode_length)
        {


            uint8_t opcode = *(fr->bytecode + fr->PC++);
            function = fetchOpcodeFunction(opcode);


            if (function == NULL)
            {


                virtual_machine->status = UNKNOWN_INSTRUCTION;
                break;
            }
            else if (!function(virtual_machine, fr))
            {
                return 0;
            }
        }
    }

    if (fr->return_count > 0 && caller_frame)
    {
        stack_operand parameters[2];
        uint8_t index;

        for (index = 0; index < fr->return_count; index++)
            pop_from_stack_operand(&fr->operands, &parameters[index].value, &parameters[index].type);

        while (fr->return_count-- > 0)
        {
            if (!push_to_stack_operand(&caller_frame->operands, parameters[fr->return_count].value, parameters[fr->return_count].type))
            {
                virtual_machine->status = OUT_OF_MEMORY;
                return 0;
            }
        }
    }

    pop_from_stack_frame(&virtual_machine->frames, NULL);
    free_frame(fr);


    return virtual_machine->status == OK;
}

uint8_t get_method_descriptor_param_cout(const uint8_t* descriptor_utf8, int32_t utf8_length)
{
    uint8_t parameterCount = 0;

    while (utf8_length > 0)
    {
        switch (*descriptor_utf8)
        {
            case '(': break;
            case ')': return parameterCount;

            case 'J': case 'D':
                parameterCount += 2;
                break;

            case 'L':

                parameterCount++;

                do {
                    utf8_length--;
                    descriptor_utf8++;
                } while (utf8_length > 0 && *descriptor_utf8 != ';');

                break;

            case '[':

                parameterCount++;

                do {
                    utf8_length--;
                    descriptor_utf8++;
                } while (utf8_length > 0 && *descriptor_utf8 == '[');

                if (utf8_length > 0 && *descriptor_utf8 == 'L')
                {
                    do {
                        utf8_length--;
                        descriptor_utf8++;
                    } while (utf8_length > 0 && *descriptor_utf8 != ';');
                }

                break;

            case 'F':
            case 'B':
            case 'C':
            case 'I':
            case 'S':
            case 'Z':
                parameterCount++;
                break;

            default:
                break;
        }

        descriptor_utf8++;
        utf8_length--;
    }

    return parameterCount;
}

loaded_classes* add_class_to_loaded_classes(interpreter_module* virtual_machine, java_class* jc)
{
    loaded_classes* node = (loaded_classes*)malloc(sizeof(loaded_classes));

    if (node)
    {
        node->jc = jc;
        node->static_data = NULL;
        node->needs_init = 1;
        node->next = virtual_machine->classes;

        virtual_machine->classes = node;
    }

    return node;
}

loaded_classes* class_is_already_loaded(interpreter_module* virtual_machine, const uint8_t* utf8_bytes, int32_t utf8_length)
{
    loaded_classes* classes = virtual_machine->classes;
    java_class* jc;
    constant_pool_info* cpi;

    while (classes)
    {
        jc = classes->jc;
        cpi = jc->constant_pool + jc->this_class - 1;
        cpi = jc->constant_pool + cpi->Class.name_index - 1;

        if (compare_utf8(UTF8(cpi), utf8_bytes, utf8_length))
            return classes;

        classes = classes->next;
    }

    return NULL;
}

java_class* get_super_class_of_given_class(interpreter_module* virtual_machine, java_class* jc)
{
    loaded_classes* lc;
    constant_pool_info* cp1;

    cp1 = jc->constant_pool + jc->super_class - 1;
    cp1 = jc->constant_pool + cp1->Class.name_index - 1;

    lc = class_is_already_loaded(virtual_machine, UTF8(cp1));

    return lc ? lc->jc : NULL;
}

uint8_t is_super_class_of_given_class(interpreter_module* virtual_machine, java_class* super, java_class* jc)
{
    constant_pool_info* cp1;
    constant_pool_info* cp2;
    loaded_classes* classes;

    cp2 = super->constant_pool + super->this_class - 1;
    cp2 = super->constant_pool + cp2->Class.name_index - 1;

    while (jc && jc->super_class)
    {
        cp1 = jc->constant_pool + jc->super_class - 1;
        cp1 = jc->constant_pool + cp1->Class.name_index - 1;

        if (compare_utf8(UTF8(cp1), UTF8(cp2)))
            return 1;

        classes = class_is_already_loaded(virtual_machine, UTF8(cp1));

        if (classes)
            jc = classes->jc;
        else
            break;
    }

    return 0;
}

uint8_t initialize_class(interpreter_module* virtual_machine, loaded_classes* lc)
{
    if (lc->needs_init)
        lc->needs_init = 0;
    else
        return 1;

    if (lc->jc->static_field_count > 0)
    {
        lc->static_data = (int32_t*)malloc(sizeof(int32_t) * lc->jc->static_field_count);

        if (!lc->static_data)
            return 0;

        uint16_t index;
        attribute_info* att;
        field_info* field;
        atrt_const_value_info* cv;
        constant_pool_info* cp;

        for (index = 0; index < lc->jc->field_count; index++)
        {
            field = lc->jc->fields + index;

            if (!(field->access_flags & STATIC_ACCESS_FLAG))
                continue;

            att = get_attribute_using_type(field->attributes, field->attributes_count, ATTRIBUTE_ConstantValue);

            if (!att)
                continue;

            cv = (atrt_const_value_info*)att->info;

            if (!cv)
                continue;

            cp = lc->jc->constant_pool + cv->constantvalue_index - 1;

            switch (cp->tag)
            {
                case INT_CONST: case FLOAT_CONST:
                    lc->static_data[field->offset] = cp->Integer.value;
                    break;

                case DOUBLE_CONST: case LONG_CONST:
                    lc->static_data[field->offset] = cp->Long.high;
                    lc->static_data[field->offset + 1] = cp->Long.low;
                    break;

                case STRING_CONST:
                    cp = lc->jc->constant_pool + cp->String.string_index - 1;
                    lc->static_data[field->offset] = (int32_t)create_new_string(virtual_machine, UTF8(cp));
                    break;

                default:
                    break;
            }
        }
    }

    method_info* clinit = get_matching_method(lc->jc, (uint8_t*)"<clinit>", 8, (uint8_t*)"()V", 3, STATIC_ACCESS_FLAG);

    if (clinit && !run_method(virtual_machine, lc->jc, clinit, 0))
        return 0;

    return 1;
}

reference* create_new_string(interpreter_module* virtual_machine, const uint8_t* str, int32_t strlen)
{
    reference* r = (reference*)malloc(sizeof(reference));
    reference_table* node = (reference_table*)malloc(sizeof(reference_table));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REF_TYPE_STRING;
    r->str.len = strlen;

    if (strlen)
    {
        r->str.utf8_bytes = (uint8_t*)malloc(strlen);

        if (!r->str.utf8_bytes)
        {
            free(r);
            free(node);
            return NULL;
        }

        memcpy(r->str.utf8_bytes, str, strlen);
    }
    else
    {
        r->str.utf8_bytes = NULL;
    }

    node->next = virtual_machine->objects;
    node->obj = r;
    virtual_machine->objects = node;


    return r;
}

reference* create_new_class_instance(interpreter_module* virtual_machine, loaded_classes* lc)
{
    if (!initialize_class(virtual_machine, lc))
        return 0;

    java_class* jc = lc->jc;
    reference* r = (reference*)malloc(sizeof(reference));
    reference_table* node = (reference_table*)malloc(sizeof(reference_table));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REF_TYPE_CLASSINSTANCE;
    r->ci.c = jc;

    if (jc->instance_field_count)
    {
        r->ci.data = (int32_t*)malloc(sizeof(int32_t) * jc->instance_field_count);

        if (!r->ci.data)
        {
            free(r);
            free(node);
            return NULL;
        }
    }
    else
    {
        r->ci.data = NULL;
    }

    node->next = virtual_machine->objects;
    node->obj = r;
    virtual_machine->objects = node;


    return r;
}

reference* create_new_array(interpreter_module* virtual_machine, uint32_t length, opcode_newarray_type type)
{
    size_t elementSize;

    switch (type)
    {
        case T_BOOLEAN:
        case T_BYTE:
            elementSize = sizeof(uint8_t);
            break;

        case T_SHORT:
        case T_CHAR:
            elementSize = sizeof(uint16_t);
            break;

        case T_FLOAT:
        case T_INT:
            elementSize = sizeof(uint32_t);
            break;

        case T_DOUBLE:
        case T_LONG:
            elementSize = sizeof(uint64_t);
            break;

        default:
            
            return NULL;
    }

    reference* r = (reference*)malloc(sizeof(reference));
    reference_table* node = (reference_table*)malloc(sizeof(reference_table));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REF_TYPE_ARRAY;
    r->arr.length = length;
    r->arr.type = type;

    if (length > 0)
    {
        r->arr.data = (uint8_t*)malloc(elementSize * length);

        if (!r->arr.data)
        {
            free(r);
            free(node);
            return NULL;
        }

        uint8_t* data = (uint8_t*)r->arr.data;

        while (length-- > 0)
            *data++ = 0;
    }
    else
    {
        r->arr.data = NULL;
    }

    node->next = virtual_machine->objects;
    node->obj = r;
    virtual_machine->objects = node;


    return r;
}

reference* create_new_object_array(interpreter_module* virtual_machine, uint32_t length, const uint8_t* utf8_className, int32_t utf8_length)
{
    if (utf8_length <= 1)
        return NULL;

    switch (utf8_className[1])
    {
        case 'J': return create_new_array(virtual_machine, length, T_LONG);
        case 'Z': return create_new_array(virtual_machine, length, T_BOOLEAN);
        case 'B': return create_new_array(virtual_machine, length, T_BYTE);
        case 'C': return create_new_array(virtual_machine, length, T_CHAR);
        case 'S': return create_new_array(virtual_machine, length, T_SHORT);
        case 'I': return create_new_array(virtual_machine, length, T_INT);
        case 'F': return create_new_array(virtual_machine, length, T_FLOAT);
        case 'D': return create_new_array(virtual_machine, length, T_DOUBLE);
        default:
            break;
    }

    reference* r = (reference*)malloc(sizeof(reference));
    reference_table* node = (reference_table*)malloc(sizeof(reference_table));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REF_TYPE_OBJECTARRAY;
    r->oar.length = length;
    r->oar.utf8_className = (uint8_t*)malloc(utf8_length);
    r->oar.utf8_len = utf8_length;

    if (!r->oar.utf8_className)
    {
        free(r);
        free(node);
        return NULL;
    }

    if (length > 0)
    {
        r->oar.elements = (reference**)malloc(length * sizeof(reference));

        if (!r->oar.elements)
        {
            free(r->oar.utf8_className);
            free(r);
            free(node);
            return NULL;
        }

        while (length-- > 0)
            r->oar.elements[length] = NULL;

    }
    else
    {
        r->oar.elements = NULL;
    }

    while (utf8_length-- > 0)
        r->oar.utf8_className[utf8_length] = utf8_className[utf8_length];

    node->next = virtual_machine->objects;
    node->obj = r;
    virtual_machine->objects = node;


    return r;
}

reference* create_new_object_multi_array(interpreter_module* virtual_machine, int32_t* dimensions, uint8_t dimensionsSize,
                               const uint8_t* utf8_className, int32_t utf8_length)
{
    if (dimensionsSize == 0)
        return NULL;

    if (dimensionsSize == 1)
        return create_new_object_array(virtual_machine, dimensions[0], utf8_className, utf8_length);

    if (utf8_length <= 0)
        return NULL;

    reference* r = (reference*)malloc(sizeof(reference));
    reference_table* node = (reference_table*)malloc(sizeof(reference_table));

    if (!node || !r)
    {
        if (node) free(node);
        if (r) free(r);

        return NULL;
    }

    r->type = REF_TYPE_OBJECTARRAY;
    r->oar.length = dimensions[0];
    r->oar.utf8_className = (uint8_t*)malloc(utf8_length);
    r->oar.utf8_len = utf8_length;

    if (!r->oar.utf8_className)
    {
        free(r);
        free(node);
        return NULL;
    }

    if (dimensions[0] > 0)
    {
        r->oar.elements = (reference**)malloc(dimensions[0] * sizeof(reference));

        if (!r->oar.elements)
        {
            free(r->oar.utf8_className);
            free(r);
            free(node);
            return NULL;
        }

        uint32_t dimensionLength = dimensions[0];

        while (dimensionLength-- > 0)
            r->oar.elements[dimensionLength] = create_new_object_multi_array(virtual_machine, dimensions + 1, dimensionsSize - 1, utf8_className + 1, utf8_length - 1);

    }
    else
    {
        r->oar.elements = NULL;
    }

    memcpy(r->oar.utf8_className, utf8_className, utf8_length);

    node->next = virtual_machine->objects;
    node->obj = r;
    virtual_machine->objects = node;


    return r;
}

void delete_reference(reference* obj)
{
    switch (obj->type)
    {
        case REF_TYPE_STRING:
            if (obj->str.utf8_bytes)
                free(obj->str.utf8_bytes);
            break;

        case REF_TYPE_ARRAY:
            if (obj->arr.data)
                free(obj->arr.data);
            break;

        case REF_TYPE_CLASSINSTANCE:
            if (obj->ci.data)
                free(obj->ci.data);
            break;

        case REF_TYPE_OBJECTARRAY:
        {
            free(obj->oar.utf8_className);

            if (obj->oar.elements)
                free(obj->oar.elements);
            break;
        }

        default:
            return;
    }

    free(obj);
}
