#pragma once

#include <pch.h>

namespace ohl::unreal {

template <typename T>
struct TArray {
    T* data;
    uint32_t count;
    uint32_t max;
};

struct FString : TArray<wchar_t> {
    std::wstring to_wstr(void);
    std::string to_str(void);
};

template <typename T>
struct TSharedPtr {
    T* obj;
    void* ref_count;
};

enum class EJson { None = 0, Null = 1, String = 2, Number = 3, Boolean = 4, Array = 5, Object = 6 };

struct FJsonValue {
   private:
    virtual void dummy(void);

   public:
    enum EJson type;

    template <typename T>
    T* cast(void);
};

template <typename K, typename V>
struct KeyValuePair {
    K key;
    V value;
    int32_t hash_next_id;
    int32_t hash_idx;
};

template <typename K, typename V>
struct TMap {
    TArray<KeyValuePair<K, V>> entries;
    void* allocation_flags;
    int32_t first_free_idx;
    int32_t num_free_idxes;
    void* hash;
    int32_t hash_size;
};

struct FJsonObject {
    TMap<FString, TSharedPtr<FJsonValue>> values;

    template <typename T>
    T* get(std::wstring key);
};

struct FJsonValueString : FJsonValue {
    FString str;

    std::wstring to_wstr(void);
    std::string to_str(void);
};

struct FJsonValueArray : FJsonValue {
    TArray<TSharedPtr<FJsonValue>> entries;

    uint32_t count(void);

    template <typename T>
    T* get(uint32_t i);
};

struct FJsonValueObject : FJsonValue {
    TSharedPtr<FJsonObject> value;

    FJsonObject* to_obj(void);
};

}  // namespace ohl::unreal
