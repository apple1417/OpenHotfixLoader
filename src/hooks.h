#pragma once

namespace ohl::hooks {

/**
 * @brief Initalizes the hooks module.
 */
void init(void);

/**
 * @brief Calls unreal's malloc function.
 * @note Throws a runtime error if the call fails.
 *
 * @param count How many bytes to allocate.
 * @return A pointer to the allocated memory.
 */
void* malloc_raw(size_t count);

/**
 * @brief Calls unreal's realloc function.
 * @note Throws a runtime error if the call fails.
 *
 * @param original The original memory to re-allocate.
 * @param count How many bytes to allocate.
 * @return A pointer to the re-allocated memory.
 */
void* realloc_raw(void* original, size_t count);

/**
 * @brief Wrapper around `malloc_raw` casts to the relevant type.
 *
 * @tparam T The type to cast to.
 * @param count How many bytes to allocate.
 * @return A pointer to the allocated memory.
 */
template <typename T>
T* malloc(size_t count) {
    return reinterpret_cast<T*>(malloc_raw(count));
}

/**
 * @brief Wrapper around `realloc_raw` casts to the relevant type.
 *
 * @tparam T The type to cast to.
 * @param original The original memory to re-allocate.
 * @param count How many bytes to allocate.
 * @return A pointer to the re-allocated memory.
 */
template <typename T>
T* realloc(void* original, size_t count) {
    return reinterpret_cast<T*>(realloc_raw(original, count));
}

/**
 * @brief Call's unreal's free function.
 *
 * @param data The data to free.
 */
void free(void* data);

}  // namespace ohl::hooks
