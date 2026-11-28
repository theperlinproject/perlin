// Test-only. The bitmap harness reports the vector font unavailable; production links TenFont.cpp.
#include "client/ui/TenFont.hpp"
#include "client/systems/text/McFont.hpp"
namespace perlin::ui {
TenFont& TenFont::Instance() { static TenFont f; return f; }
void TenFont::EnsureLoaded() {}
bool TenFont::Load() { return false; }
float TenFont::Ascent(float s) const { return 7*s; }
float TenFont::Descent(float s) const { return 2*s; }
float TenFont::Advance(uint32_t cp,float s) const { return McFont::Instance().Get(cp).advance*s; }
float TenFont::Width(const std::string& str,float s) const { return McFont::Instance().Width(str,s); }
const TenFont::Page* TenFont::PageFor(int) { return nullptr; }
bool IsTenFontType(const std::string& type) { return type=="MinecraftTen" || type=="minecraft_ten"; }
}
