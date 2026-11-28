#pragma once
#include "util/JsonValue.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace perlin::ui {
    struct DduiScreen {
        std::string screenId;
        uint32_t formId = 0;
        std::optional<uint32_t> dataInstanceId;
        std::string StoreName() const;
    };
    // A data store property is versioned separately from each scalar path.
    // The caller serializes network and render-thread accesses.
    class DduiState {
    public:
        struct Version { uint32_t property = 0, path = 0; };
        bool Change(std::string_view name, std::string_view property, uint32_t generation, json::Value value);
        bool Update(std::string_view name, std::string_view property, std::string_view path,
                    Version version, json::Value value);
        bool Remove(std::string_view name, std::string_view property);
        const json::Value* Read(std::string_view name, std::string_view property) const;
        std::optional<Version> Next(std::string_view name, std::string_view property, std::string_view path) const;
        uint64_t Revision() const { return revision; }
        void Clear();
        static const json::Value* At(const json::Value& root, std::string_view path);
    private:
        struct Property {
            json::Value value;
            uint32_t generation = 0;
            std::map<std::string, uint32_t, std::less<>> paths;
        };
        using Key = std::pair<std::string, std::string>;
        std::map<Key, Property> properties;
        uint64_t revision = 0;
    };
}
