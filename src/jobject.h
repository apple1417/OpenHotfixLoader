#pragma once

#include <pch.h>

namespace ohl::jobject {

template <typename T>
struct TArray {
    T* data;
    uint32_t count;
    uint32_t max;
};

enum class JObjectType : uint32_t {
    STR = 2,
    ARRAY = 5,
    DICT = 6,
};

struct JObject {
    void* unknown1;
    enum JObjectType type;
    uint32_t unknown2;

    template <typename T>
    T* cast(void);
};

struct JString : JObject {
    TArray<wchar_t> str;

    std::wstring to_wstr(void);
    std::string to_str(void);
};

struct JArrayEntry {
    JObject* obj;
    void* unknown;
};

struct JArray : JObject {
    TArray<JArrayEntry> entries;
};

struct JDictEntry {
    TArray<wchar_t> key;
    JObject* value;
    void* unknown1;
    int32_t unknown2;
    uint32_t unknown3;

    std::wstring key_wstr(void);
    std::string key_str(void);
};

struct JDict : JObject {
    TArray<JDictEntry>* entries;

    JObject* get(std::wstring key);
};

}  // namespace ohl::jobject
