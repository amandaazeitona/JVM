#ifndef INTERPRETERMODULE_H
#define INTERPRETERMODULE_H

typedef struct interpreter_module interpreter_module;
typedef struct reference reference;

#include <stdint.h>
#include "javaclass.h"
#include "opcodes.h"
#include "framestack.h"

enum general_status {
    OK,
    NO_CLASS_LOADED,
    MAIN_CLASS_RESOLUTION_FAILED,
    CLASS_RESOLUTION_FAILED,
    METHOD_RESOLUTION_FAILED,
    FIELD_RESOLUTION_FAILED,
    UNKNOWN_INSTRUCTION,
    OUT_OF_MEMORY,
    MAIN_METHOD_NOT_FOUND,
    INVALID_INSTRUCTION_PARAMETERS
};

const char* get_general_status_msg(enum general_status status);

typedef struct class_instance
{
    java_class* c;
    int32_t* data;
} class_instance;

typedef struct String
{
    uint8_t* utf8_bytes;
    uint32_t len;
} String;

typedef struct Array
{
    uint32_t length;
    uint8_t* data;
    opcode_newarray_type type;
} Array;

typedef struct ObjectArray
{
    uint32_t length;
    uint8_t* utf8_className;
    int32_t utf8_len;
    reference** elements;
} ObjectArray;

typedef enum reference_type {
     REF_TYPE_ARRAY,
     REF_TYPE_CLASSINSTANCE,
     REF_TYPE_OBJECTARRAY,
     REF_TYPE_STRING
} reference_type;

struct reference
{
    reference_type type;

    union {
        class_instance ci;
        Array arr;
        ObjectArray oar;
        String str;
    };
};

typedef struct reference_table
{
    reference* obj;
    struct reference_table* next;
} reference_table;

typedef struct loaded_classes
{
    java_class* jc;
    uint8_t needs_init;
    int32_t* static_data;
    struct loaded_classes* next;
} loaded_classes;

struct interpreter_module
{
    uint8_t status;
    uint8_t sys_and_str_classes_simulation;
    reference_table* objects;
    stack_frame* frames;
    loaded_classes* classes;
    char class_path[256];
};

void initialize_virtual_machine(interpreter_module*);
void deinitialize_virtual_machine(interpreter_module*);
void interpret_cl(interpreter_module*, loaded_classes*);
void set_class_path(interpreter_module*, const char*);
uint8_t class_handler(interpreter_module*, const uint8_t*, int32_t,
        loaded_classes**);
uint8_t method_handler(interpreter_module*, java_class*, constant_pool_info*,
        loaded_classes**);
uint8_t field_handler(interpreter_module*, java_class*, constant_pool_info*,
        loaded_classes**);
uint8_t run_method(interpreter_module*, java_class*, method_info*, uint8_t);
uint8_t get_method_descriptor_param_cout(const uint8_t*, int32_t);
loaded_classes* add_class_to_loaded_classes(interpreter_module*, java_class*);
loaded_classes* class_is_already_loaded(interpreter_module*, const uint8_t*,
        int32_t);
java_class* get_super_class_of_given_class(interpreter_module*, java_class*);
uint8_t is_super_class_of_given_class(interpreter_module*, java_class*,
        java_class*);
uint8_t initialize_class(interpreter_module*, loaded_classes*);
reference* create_new_string(interpreter_module*, const uint8_t*, int32_t);
reference* create_new_class_instance(interpreter_module*, loaded_classes*);
reference* create_new_array(interpreter_module*, uint32_t,
        opcode_newarray_type);
reference* create_new_object_array(interpreter_module*, uint32_t,
        const uint8_t*, int32_t);
reference* create_new_object_multi_array(interpreter_module*, int32_t*,
        uint8_t, const uint8_t*, int32_t);

void delete_reference(reference* obj);

 #define DEBUG_REPORT_ERROR_INSTRUCTION \
    printf("\nAbortion request by instruction at %s:%u.\n", __FILE__, __LINE__); \
    printf("Check at the source file what the cause could be.\n"); \
    printf("It could be an exception that was supposed to be thrown or an unsupported feature.\n"); \
    printf("Execution will proceed, but others instruction will surely request abortion.\n\n");

#endif
