#include "client/ui/DduiState.hpp"
#include <iostream>
using namespace perlin;
using namespace perlin::ui;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "failed at line " << __LINE__ << ": " #expr "\n"; return 1; } } while (false)
int main() {
    DduiScreen screen{"minecraft:custom_form", 7, 42};
    CHECK(screen.StoreName() == "custom_form_data_42");
    DduiState state;
    json::Value form;
    std::string error;
    CHECK(json::Value::Parse(R"({"layout":{"0":{"toggled":false,"value":3}},"title":"Test","layout[1]":{"text":"a"}})", form, error));
    const auto name = screen.StoreName();
    CHECK(state.Change(name, "modalForm", 4, form));
    CHECK(state.Change(name, "other", 0, json::Value(true)));
    CHECK(!state.Change(name, "modalForm", 3, form));
    CHECK(state.Update(name, "modalForm", "layout[0].toggled", {4, 1}, json::Value(true)));
    CHECK(!state.Update(name, "modalForm", "layout[0].toggled", {4, 1}, json::Value(false)));
    CHECK(!state.Update(name, "modalForm", "layout[0].toggled", {3, 99}, json::Value(false)));
    CHECK(!state.Update(name, "modalForm", "layout[0].toggled", {4, 2}, json::Value("bad")));
    CHECK(!state.Update(name, "modalForm", "layout[0].unknown", {4, 1}, json::Value(true)));
    CHECK(state.Update(name, "modalForm", "layout[1].text", {4, 1}, json::Value("b")));
    CHECK(DduiState::At(*state.Read(name, "modalForm"), "layout[1].text")->string == "b");
    CHECK(DduiState::At(*state.Read(name, "modalForm"), "layout[0].toggled")->boolean);
    auto next = state.Next(name, "modalForm", "layout[0].toggled");
    CHECK(next && next->property == 4 && next->path == 2);
    CHECK(state.Change(name, "modalForm", 5, form));
    next = state.Next(name, "modalForm", "layout[0].toggled");
    CHECK(next && next->property == 5 && next->path == 1);
    CHECK(state.Remove(name, ""));
    CHECK(!state.Read(name, "modalForm") && !state.Read(name, "other"));
    std::cout << "DDUI data-store lifecycle and counters passed\n";
    return 0;
}
