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

struct FReferenceControllerBase {
    void* vf_table;
    int32_t ref_count;
    int32_t weak_ref_count;
    void* obj;
};

template <typename T>
struct TSharedPtr {
    T* obj;
    FReferenceControllerBase* ref_controller;
};

enum class EJson { None = 0, Null = 1, String = 2, Number = 3, Boolean = 4, Array = 5, Object = 6 };

struct FJsonValue {
    void* vf_table;
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

using JSONObjectEntry = KeyValuePair<FString, TSharedPtr<FJsonValue>>;

struct FJsonObject {
    TArray<JSONObjectEntry> entries;
    uint32_t pattern[16];

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
