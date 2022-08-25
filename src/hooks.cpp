#include <pch.h>

typedef bool(__thiscall* discovery_from_json)(void* this_service, void* json);

discovery_from_json original_discovery_from_json = nullptr;

bool __thiscall detour_discovery_from_json(void* this_service, void* json) {
  return original_discovery_from_json(this_service, json);
}
