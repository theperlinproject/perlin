#include "client/ui/DduiState.hpp"
#include <charconv>
#include <cmath>

namespace perlin::ui {
    namespace {
        bool Newer(uint32_t a, uint32_t b) { return a != b && (a - b) < 0x80000000u; }
        template<class V> V* Locate(V& root, std::string_view path) {
            V* node = &root;
            while (!path.empty()) {
                // Native data stores can also contain literal keys such as layout[0].
                if (auto* exact = node->Find(path)) return exact;
                if (path.front() == '.') { path.remove_prefix(1); continue; }
                size_t end = 0;
                std::string_view key;
                if (path.front() == '[') {
                    end = path.find(']');
                    if (end == std::string_view::npos) return nullptr;
                    key = path.substr(1, end - 1);
                    ++end;
                } else {
                    end = path.find_first_of(".[");
                    if (end == std::string_view::npos) end = path.size();
                    key = path.substr(0, end);
                    // Prefer the longest existing literal segment at this level.
                    for (size_t k = end; k < path.size(); ++k) {
                        if (path[k] == '.' && node->Find(path.substr(0, k))) {
                            key = path.substr(0, k); end = k; break;
                        }
                    }
                }
                if (key.empty()) return nullptr;
                if (node->IsArray()) {
                    size_t i = 0;
                    const auto r = std::from_chars(key.data(), key.data() + key.size(), i);
                    if (r.ec != std::errc{} || r.ptr != key.data() + key.size() || i >= node->array.size()) return nullptr;
                    node = &node->array[i];
                } else node = node->Find(key);
                if (!node) return nullptr;
                path.remove_prefix(end);
            }
            return node;
        }
        bool Scalar(const json::Value& v) {
            return v.IsBool() || v.IsString() || (v.IsNumber() && std::isfinite(v.number));
        }
    }
    std::string DduiScreen::StoreName() const {
        const auto id = dataInstanceId.value_or(formId);
        if (screenId == "minecraft:custom_form") return "custom_form_data_" + std::to_string(id);
        if (screenId == "minecraft:message_box") return "message_box_data_" + std::to_string(id);
        return {};
    }
    bool DduiState::Change(std::string_view name, std::string_view property, uint32_t generation, json::Value value) {
        const Key key{std::string(name), std::string(property)};
        auto it = properties.find(key);
        if (it != properties.end() && !Newer(generation, it->second.generation)) return false;
        if (it == properties.end() && properties.size() >= 4096) return false;
        properties.insert_or_assign(key, Property{std::move(value), generation, {}});
        ++revision;
        return true;
    }
    bool DduiState::Update(std::string_view name, std::string_view property, std::string_view path, Version version, json::Value value) {
        auto it = properties.find({std::string(name), std::string(property)});
        if (it == properties.end() || it->second.generation != version.property || !Scalar(value)) return false;
        auto& p = it->second;
        const auto seen = p.paths.find(path);
        if (seen != p.paths.end() && !Newer(version.path, seen->second)) return false;
        auto* target = Locate(p.value, path);
        if (!target || (!target->IsNull() && target->type != value.type)) return false;
        if (seen == p.paths.end() && p.paths.size() >= 16384) return false;
        *target = std::move(value);
        p.paths[std::string(path)] = version.path;
        ++revision;
        return true;
    }
    bool DduiState::Remove(std::string_view name, std::string_view property) {
        if (property.empty()) {
            const auto before = properties.size();
            std::erase_if(properties, [&](const auto& item) { return item.first.first == name; });
            if (before == properties.size()) return false;
        } else if (!properties.erase({std::string(name), std::string(property)})) return false;
        ++revision;
        return true;
    }
    const json::Value* DduiState::Read(std::string_view name, std::string_view property) const {
        auto it = properties.find({std::string(name), std::string(property)});
        return it == properties.end() ? nullptr : &it->second.value;
    }
    std::optional<DduiState::Version> DduiState::Next(std::string_view name, std::string_view property, std::string_view path) const {
        auto it = properties.find({std::string(name), std::string(property)});
        if (it == properties.end()) return std::nullopt;
        const auto* target = Locate(it->second.value, path);
        if (!target || !Scalar(*target)) return std::nullopt;
        auto count = it->second.paths.find(path);
        return Version{it->second.generation, count == it->second.paths.end() ? 1u : count->second + 1u};
    }
    const json::Value* DduiState::At(const json::Value& root, std::string_view path) { return Locate(root, path); }
    void DduiState::Clear() { properties.clear(); ++revision; }
}
