#include <pch.h>

#include "unreal.h"

namespace ohl::unreal {

#pragma region Casting

template <typename T>
struct JTypeMapping {
    typedef T type;
    static inline const EJson enum_type;
};

template <>
struct JTypeMapping<FJsonValueString> {
    typedef FJsonValueString type;
    static inline const EJson enum_type = EJson::String;
};

template <>
struct JTypeMapping<FJsonValueArray> {
    typedef FJsonValueArray type;
    static inline const EJson enum_type = EJson::Array;
};

template <>
struct JTypeMapping<FJsonValueObject> {
    typedef FJsonValueObject type;
    static inline const EJson enum_type = EJson::Object;
};

template <typename T>
T* FJsonValue::cast(void) {
    if (this->type != JTypeMapping<T>::enum_type) {
        throw std::runtime_error("JSON object was of unexpected type " +
                                 std::to_string((uint32_t)this->type));
    }
    return reinterpret_cast<T*>(this);
}

#pragma endregion

#pragma region Accessors

std::wstring FString::to_wstr(void) {
    return std::wstring(this->data);
}

std::wstring FJsonValueString::to_wstr(void) {
    return this->str.to_wstr();
}

std::wstring FSparkRequest::get_url(void) {
    return this->url.to_wstr();
}

uint32_t FJsonValueArray::count() {
    return this->entries.count;
}

template <typename T>
T* FJsonValueArray::get(uint32_t idx) {
    if (idx > this->count()) {
        throw std::out_of_range("Array index out of range");
    }
    return this->entries.data[idx].obj->cast<T>();
}

template <typename T>
T* FJsonObject::get(std::wstring key) {
    for (auto i = 0; i < this->entries.count; i++) {
        auto entry = this->entries.data[i];
        if (entry.key.to_wstr() == key) {
            return entry.value.obj->cast<T>();
        }
    }
    throw std::runtime_error("Couldn't find key!");
}

FJsonObject* FJsonValueObject::to_obj(void) {
    return this->value.obj;
}

#pragma endregion

#pragma region Explict Template Instantiation

template FJsonValueString* FJsonValue::cast(void);
template FJsonValueArray* FJsonValue::cast(void);
template FJsonValueObject* FJsonValue::cast(void);

template FJsonValueString* FJsonValueArray::get(uint32_t);
template FJsonValueArray* FJsonValueArray::get(uint32_t);
template FJsonValueObject* FJsonValueArray::get(uint32_t);

template FJsonValueString* FJsonObject::get(std::wstring key);
template FJsonValueArray* FJsonObject::get(std::wstring key);
template FJsonValueObject* FJsonObject::get(std::wstring key);

#pragma endregion

}  // namespace ohl::unreal
