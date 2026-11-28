#pragma once
#include <cstdint>

namespace perlin {
    struct UiVertex {
        float x, y, z;
        uint32_t abgr;
        float u, v;
    };
    static_assert(sizeof(UiVertex) == 24);

    // HudDoll: the HUD paper doll, composited right after the HUD (Gui) as part of it. Debug: the F3 debug
    // screen, drawn over the HUD and its paper doll. Doll (the inventory player preview) stays above the GUI
    // it belongs to.
    enum class UiLayer : uint8_t { World, WorldLate, Hand, Gui, Doll, Top, Export, HudDoll, Debug };

}
