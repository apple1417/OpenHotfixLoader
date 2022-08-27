#pragma once

namespace ohl::hooks {

void init(void);

void* malloc_raw(size_t count);
void* realloc_raw(void* original, size_t count);

template <typename T>
T* malloc(size_t count) {
    return reinterpret_cast<T*>(malloc_raw(count));
}

template <typename T>
T* realloc(void* original, size_t count) {
    return reinterpret_cast<T*>(realloc_raw(original, count));
}

void free(void* data);

}  // namespace ohl::hooks
