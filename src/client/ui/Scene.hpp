#pragma once
#include <cstdint>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace perlin::ui {
    enum class Theme {
        None, Title, List, Options, Keybinds, Menu, Pause, Inventory, Creative, Chest, Death, Disconnect, Loading,
        Download, Credits, Bed, Form, Hud, Replay, Plugin, Server
    };

    struct SceneRun {
        std::string text;
        std::string colored;
        std::string color = "#ffffff";
        std::string weight = "normal";
        std::string style = "normal";
        std::string background = "transparent";
        std::string display = "inline";
        std::string width = "auto";
        std::string overflow = "visible";

        bool operator==(const SceneRun&) const = default;
    };

    struct SceneNode {
        enum Kind { Box = 0, Image = 1, Item = 2, Text = 3, Rect = 4, Gradient = 5, Link = 6, Label = 7, Button = 8, Slider = 9 };

        int kind = Box;
        std::string id;
        std::string cls;
        std::string snd;
        std::string vol;
        std::string pit;
        float x = 0, y = 0, w = 0, h = 0;
        float knob = 0;
        std::string z = "auto";
        bool inheritedClip = false;

        std::string src;
        std::string uv;
        std::string nine;
        std::string tiled;
        std::string tileScale;
        std::string clipDirection;
        std::string tint;
        std::string alpha;
        std::string keep;
        std::string baseSize;
        std::string glint;
        std::string key;
        std::string layer;

        std::string color = "#ffffffff";
        std::string decorator;

        std::string family = "minecraft";
        std::string align = "left";
        float size = 8.0f;
        float lineHeight = 9.0f;
        std::vector<SceneRun> runs;

        std::vector<SceneNode> children;

        bool operator==(const SceneNode&) const = default;
    };

    struct Scene {
        Theme theme = Theme::None;
        float panoSpan = 0;
        bool sprites = false;
        std::vector<SceneNode> nodes;
        std::map<std::string, std::vector<SceneNode>> lists;

        bool operator==(const Scene&) const = default;
    };

    struct SceneDoc {
        Scene scene;
    };

    bool SceneHasId(const SceneDoc& doc, const std::string& id, bool prefix = false);
    bool SceneHasText(const SceneDoc& doc, const std::string& needle);
    std::string DumpScene(const SceneDoc& doc);

    class SceneBuilder {
    public:
        static constexpr int kAutoZ = INT32_MIN;

        explicit SceneBuilder(std::vector<SceneNode>& root) {
            stack.push_back(&root);
        }

        SceneNode& Add(SceneNode node);
        SceneNode& Begin(SceneNode node);
        void End();
        int Depth() const {
            return static_cast<int>(stack.size()) - 1;
        }
        std::vector<SceneNode>& Current() { return *stack.back(); }

    private:
        std::vector<std::vector<SceneNode>*> stack;
    };

    std::string SceneNumber(float v);
    inline std::string SceneFlag(bool v) { return v ? "1" : ""; }
    std::vector<SceneRun> ParseFormatLines(const std::string& text, const std::string& baseColor);
    std::vector<SceneRun> ParseFormatRuns(const std::string& text, const std::string& baseColor,
                                          float alpha = 1.0f, bool breaks = false);
}
