#include <pch.h>

#include "jobject.h"

namespace ohl::jobject {

#pragma region String Conversion

static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wstring_converter;

std::wstring JString::to_wstr(void) {
    return std::wstring(this->str.data);
}
std::string JString::to_str(void) {
    return wstring_converter.to_bytes(this->to_wstr());
}

std::wstring JDictEntry::key_wstr(void) {
    return std::wstring(this->key.data);
}
std::string JDictEntry::key_str(void) {
    return wstring_converter.to_bytes(this->key_wstr());
}

#pragma endregion

#pragma region Casting

template <typename T>
struct JTypeMapping {
    typedef T type;
    static inline const JObjectType enum_type;
};

template <>
struct JTypeMapping<JString> {
    typedef JString type;
    static inline const JObjectType enum_type = JObjectType::STR;
};

template <>
struct JTypeMapping<JArray> {
    typedef JArray type;
    static inline const JObjectType enum_type = JObjectType::ARRAY;
};

template <>
struct JTypeMapping<JDict> {
    typedef JDict type;
    static inline const JObjectType enum_type = JObjectType::DICT;
};

template <typename T>
T* JObject::cast(void) {
    if (this->type != JTypeMapping<T>::enum_type) {
        throw std::runtime_error("JSON object was of unexpected type " +
                                 std::to_string((uint32_t)this->type));
    }
    return reinterpret_cast<T*>(this);
}

template JString* JObject::cast(void);
template JArray* JObject::cast(void);
template JDict* JObject::cast(void);

#pragma endregion

#pragma region Accessors

uint32_t JArray::count() {
    return this->entries.count;
}

template <typename T>
T* JArray::get(uint32_t idx) {
    if (idx > this->count()) {
        throw std::out_of_range("Array index out of range");
    }
    return this->entries.data[idx].obj->cast<T>();
}

template JString* JArray::get(uint32_t);
template JArray* JArray::get(uint32_t);
template JDict* JArray::get(uint32_t);

template <typename T>
T* JDict::get(std::wstring key) {
    for (auto i = 0; i < this->entries->count; i++) {
        auto entry = this->entries->data[i];
        if (entry.key_wstr() == key) {
            return entry.value->cast<T>();
        }
    }
    throw std::runtime_error("Couldn't find key " + wstring_converter.to_bytes(key));
}

template JString* JDict::get(std::wstring key);
template JArray* JDict::get(std::wstring key);
template JDict* JDict::get(std::wstring key);

#pragma endregion

}  // namespace ohl::jobject
