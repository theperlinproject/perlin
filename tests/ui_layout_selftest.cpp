#include "client/ui/UiLayout.hpp"
#include "client/ui/TenFont.hpp"
#include <cmath>
#include <iostream>
using namespace perlin;
using namespace perlin::ui;
// Layout-only test: font measurement is not under test and no game assets are loaded.
namespace perlin::ui {
    bool IsTenFontType(const std::string&) { return false; }
    TenFont& TenFont::Instance() { static TenFont font; return font; }
    void TenFont::EnsureLoaded() {}
}
class Data : public UiData { public: bool Global(const std::string&, json::Value&) const override { return false; } };
static std::unique_ptr<Node> Make(const char* id, const char* type, const char* props) {
    auto n = std::make_unique<Node>();
    n->name = id; n->path = id; n->type = type;
    std::string error;
    if (!json::Value::Parse(props, n->props, error)) std::abort();
    return n;
}
#define CHECK(expr) do { if (!(expr)) { std::cerr << "failed at line " << __LINE__ << ": " #expr "\n"; return 1; } } while (false)
static bool eq(float a,float b) { return std::abs(a-b)<0.001f; }
int main() {
    Data data;
    UiServices svc;
    auto root = Make("root", "screen", R"({"size":[50,50]})");
    root->children.push_back(Make("cross", "panel", R"({"size":["200%y + 5px",40],"anchor_from":"top_left","anchor_to":"top_left","offset":["50%x","-25%y"]})"));
    root->children.push_back(Make("other", "panel", R"({"size":["25%","50%x"],"anchor_from":"bottom_right","anchor_to":"bottom_right"})"));
    root->children.push_back(Make("default", "panel", R"({"size":["default","default"]})"));
    auto stack = Make("stack", "stack_panel", R"({"size":[100,100],"anchor_from":"top_left","anchor_to":"top_left"})");
    stack->children.push_back(Make("fixed", "panel", R"({"size":[20,20],"offset":[3,4]})"));
    stack->children.push_back(Make("fill1", "panel", R"({"size":[20,"fill"]})"));
    stack->children.push_back(Make("fill2", "panel", R"({"size":[20,"fill"]})"));
    root->children.push_back(std::move(stack));
    auto grid = Make("grid", "grid", R"({"size":["100%c","100%c"],"grid_dimensions":[3,2],"anchor_from":"top_left","anchor_to":"top_left"})");
    grid->children.push_back(Make("cell", "panel", R"({"size":[10,20],"grid_position":[1,2]})"));
    root->children.push_back(std::move(grid));
    auto vertical = Make("vertical", "grid", R"({"size":["100%c","100%c"],"grid_dimensions":[2,2],"grid_fill_direction":"vertical","anchor_from":"top_left","anchor_to":"top_left"})");
    for(int i=0;i<4;++i) vertical->children.push_back(Make("cell", "panel", R"({"size":[10,20]})"));
    root->children.push_back(std::move(vertical));
    auto n = EvaluateTree(*root,data,svc,320,180);
    CHECK(n && n->kids.size()==6);
    const auto& cross=*n->kids[0];
    CHECK(eq(cross.w,85) && eq(cross.h,40) && eq(cross.x,42.5f) && eq(cross.y,-10));
    const auto& other=*n->kids[1];
    CHECK(eq(other.w,80) && eq(other.h,40) && eq(other.x,240) && eq(other.y,140));
    CHECK(eq(n->kids[2]->w,320) && eq(n->kids[2]->h,180));
    const auto& s=*n->kids[3];
    CHECK(eq(s.kids[0]->x,3) && eq(s.kids[0]->y,4));
    CHECK(eq(s.kids[1]->h,40) && eq(s.kids[1]->y,20));
    CHECK(eq(s.kids[2]->h,40) && eq(s.kids[2]->y,60));
    const auto& g=*n->kids[4];
    CHECK(eq(g.w,30) && eq(g.h,40) && eq(g.kids[0]->x,20) && eq(g.kids[0]->y,20));
    const auto& v=*n->kids[5];
    CHECK(eq(v.kids[1]->x,0) && eq(v.kids[1]->y,20) && eq(v.kids[2]->x,10) && eq(v.kids[2]->y,0));
    std::cout << "JSON UI viewport, own-axis sizing, anchors, offsets, fill and grid regressions passed\n";
}
