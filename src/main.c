#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "javaclass.h"
#include "jvm.h"

int main(int argc, char* args[])
{
    if (argc <= 1)
    {
        printf("First parameter should be the path to a .class file.\n");
        printf("Following parameters could be:\n");
        printf(" -c \t Shows the content of the .class file\n");
        printf(" -e \t Execute the method 'main' from the class\n");
        printf(" -b \t Adds UTF-8 BOM to the output\n");
        return 0;
    }

    if (sizeof(int32_t*) > sizeof(int32_t))
    {
        printf("Warning: pointers cannot be stored in an int32_t.\n");
        printf("This is very likely to malfunction when handling references.\n");
        printf("Continue anyway? (Y/N)\n");

        int input;

        do {

            input = getchar();

            if (input == 'Y' || input == 'y')
                break;

            if (input == 'N' || input == 'n')
                exit(0);

            while (getchar() != '\n');

        } while (1);

    }

    uint8_t printClassContent = 0;
    uint8_t executeClassMain = 0;
    uint8_t includeBOM = 0;

    int argIndex;

    for (argIndex = 2; argIndex < argc; argIndex++)
    {
        if (!strcmp(args[argIndex], "-c"))
            printClassContent = 1;
        else if (!strcmp(args[argIndex], "-e"))
            executeClassMain = 1;
        else if (!strcmp(args[argIndex], "-b"))
            includeBOM = 1;
        else
            printf("Unknown argument #%d ('%s')\n", argIndex, args[argIndex]);
    }

    if (!printClassContent && !executeClassMain)
    {
        printf("Nothing to do with input.\n");
        printf("Ensure that at least one of the following options are included: \"-c\", \"-e\".\n");
    }

    if (includeBOM)
        printf("%c%c%c", 0xEF, 0xBB, 0xBF);

    if (printClassContent)
    {
        java_class jc;
        open_class_file(&jc, args[1]);

        if (jc.status != CLASS_STA_OK)
            print_class_file_debug_info(&jc);
        else if (printClassContent)
            print_class_file_info(&jc);

        close_class_file(&jc);
    }

    if (executeClassMain)
    {
        interpreter_module jvm;
        initialize_virtual_machine(&jvm);

        size_t inputLength = strlen(args[1]);

        if (inputLength > 6 && strcmp(args[1] + inputLength - 6, ".class") == 0)
        {
            inputLength -= 6;
            args[1][inputLength] = '\0';
        }

        loaded_classes* mainLoadedClass;

        set_class_path(&jvm, args[1]);

        if (class_handler(&jvm, (const uint8_t*)args[1], inputLength, &mainLoadedClass))
            interpret_cl(&jvm, mainLoadedClass);

        uint8_t printStatus = jvm.status != OK;


        if (printStatus)
        {
            printf("Execution finished. Status: %d\n", jvm.status);
            printf("Status message: %s.", get_general_status_msg(jvm.status));
        }

        deinitialize_virtual_machine(&jvm);
    }

    return 0;
}

/// @mainpage Java Virtual Machine
///
/// <pre>
/// Group:
///     Amanda Oliveira
///     Fillype Alves
///     Gabriel Bessa
///     Jader Martins
///     Victor Martorelli
/// </pre>

