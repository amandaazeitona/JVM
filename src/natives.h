#ifndef NATIVES_H
#define NATIVES_H

#include <stdint.h>
#include "jvm.h"

typedef uint8_t(*native_func)(interpreter_module*, frame*, const uint8_t*,
        int32_t);

native_func get_native_func(const uint8_t*, int32_t, const uint8_t*,
                            int32_t, const uint8_t*, int32_t);

#endif
