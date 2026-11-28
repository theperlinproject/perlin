#pragma once
#include "util/JsonValue.hpp"
#include <algorithm>
#include <cmath>
namespace perlin::ui {
    struct TextureSidecar {
        bool present = false;
        bool nine = false;
        float slice[4] = {0, 0, 0, 0};
        int baseW = 0, baseH = 0;
    };

    inline bool ReadNineSlice(const json::Value* value, float (&slice)[4]) {
        if (!value) return false;
        if (value->IsNumber()) {
            for (float& v : slice) v = static_cast<float>(value->number);
        } else if (value->IsArray() && value->array.size() == 2) {
            slice[0] = slice[2] = static_cast<float>(value->array[0].NumberOr(0));
            slice[1] = slice[3] = static_cast<float>(value->array[1].NumberOr(0));
        } else if (value->IsArray() && value->array.size() == 4) {
            for (int i=0; i<4; ++i) slice[i] = static_cast<float>(value->array[i].NumberOr(0));
        } else return false;
        bool any = false;
        for (float& v : slice) { v = std::isfinite(v) ? std::max(0.0f,v) : 0.0f; any |= v > 0; }
        return any;
    }
    inline TextureSidecar ParseTextureSidecar(const json::Value& value) {
        TextureSidecar out;
        if (!value.IsObject()) return out;
        out.present = true;
        out.nine = ReadNineSlice(value.Find("nineslice_size"), out.slice);
        if (const auto* size = value.Find("base_size"); size && size->IsArray() && size->array.size() == 2) {
            auto dim = [](double v) { return std::isfinite(v) && v > 0 && v <= 1048576 ? static_cast<int>(v) : 0; };
            out.baseW = dim(size->array[0].NumberOr(0));
            out.baseH = dim(size->array[1].NumberOr(0));
        }
        return out;
    }
}
