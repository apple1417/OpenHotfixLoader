#include <pch.h>

#include "unreal.h"

using namespace ohl::unreal;

namespace ohl::processing {

void handle_discovery_from_json(ohl::unreal::FJsonObject** json) {
    auto services = (*json)->get<FJsonValueArray>(L"services");

    FJsonObject* micropatch = nullptr;

    for (auto i = 0; i < services->count(); i++) {
        auto service = services->get<FJsonValueObject>(i)->to_obj();
        auto service_name = service->get<FJsonValueString>(L"service_name")->to_wstr();
        if (service_name == L"Micropatch") {
            micropatch = service;
            break;
        }
    }

    // This happens during the first verify call so don't throw
    if (micropatch == nullptr) {
        return;
    }

    std::cout << "[OHL] Found Hotfixes:\n";

    auto params = micropatch->get<FJsonValueArray>(L"parameters");
    for (auto i = 0; i < params->count(); i++) {
        auto hotfix = params->get<FJsonValueObject>(i)->to_obj();
        auto key = hotfix->get<FJsonValueString>(L"key")->to_str();
        auto value = hotfix->get<FJsonValueString>(L"value")->to_str();

        std::cout << "[OHL] " << key << "," << value << "\n";
    }
}

}  // namespace ohl::processing
