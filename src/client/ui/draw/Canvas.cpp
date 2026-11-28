#include "client/ui/draw/Canvas.hpp"

#include "client/systems/sprites/UiAssets.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace perlin::ui::draw {
    namespace {
        uint32_t Packed(float r, float g, float b, float a) {
            const auto byte = [](float v) { return std::clamp(static_cast<int>(std::lround(v * 255.0f)), 0, 255); };
            const uint32_t A = static_cast<uint32_t>(byte(a));
            return (A << 24) | (static_cast<uint32_t>(byte(b * a)) << 16) | (static_cast<uint32_t>(byte(g * a)) << 8) |
                   static_cast<uint32_t>(byte(r * a));
        }
    }

    Canvas::Canvas(TextureSource& textures, std::vector<Batch>& out)
        : tex(textures), out(out), device(std::max(1, textures.Device())) {
        scissors.push_back({false, {0, 0, 0, 0}});
    }

    Batch& Canvas::Current(uint32_t texture, bool glint) {
        const Scissor& sc = scissors.back();
        if (!out.empty()) {
            Batch& b = out.back();
            if (b.texture == texture && b.glint == glint && b.layer == layer && b.scissor == sc.on &&
                b.shift == shift &&
                (!sc.on || (b.rect[0] == sc.r[0] && b.rect[1] == sc.r[1] && b.rect[2] == sc.r[2] &&
                            b.rect[3] == sc.r[3])) &&
                b.vertices.size() < 24000)
                return b;
        }
        Batch b;
        b.texture = texture;
        b.glint = glint;
        b.layer = layer;
        b.scissor = sc.on;
        b.shift = shift;
        for (int i = 0; i < 4; i++)
            b.rect[i] = sc.r[i];
        out.push_back(std::move(b));
        return out.back();
    }

    void Canvas::Append(Batch& b, const std::array<float, 8>& xy, const std::array<float, 8>& uv, uint32_t color) {
        const uint32_t base = static_cast<uint32_t>(b.vertices.size());
        for (int i = 0; i < 4; i++)
            b.vertices.push_back({xy[i * 2], xy[i * 2 + 1], 0.0f, color, uv[i * 2], uv[i * 2 + 1]});
        for (const uint32_t i : {0u, 2u, 1u, 0u, 3u, 2u})
            b.indices.push_back(base + i);
    }

    void Canvas::Quad(uint32_t texture, float x, float y, float w, float h, float u0, float v0, float u1, float v1,
                      uint32_t color) {
        if (!texture)
            return;
        Append(Current(texture, false), {x, y, x + w, y, x + w, y + h, x, y + h}, {u0, v0, u1, v0, u1, v1, u0, v1},
               color);
    }

    void Canvas::Skewed(uint32_t texture, const float xy[8], const float uv[8], uint32_t color) {
        if (!texture)
            return;
        Append(Current(texture, false), {xy[0], xy[1], xy[2], xy[3], xy[4], xy[5], xy[6], xy[7]},
               {uv[0], uv[1], uv[2], uv[3], uv[4], uv[5], uv[6], uv[7]}, color);
    }

    void Canvas::Solid(float x, float y, float w, float h, Color c) {
        if (c.a == 0 || w <= 0 || h <= 0)
            return;
        const TexInfo white = tex.Get("mc:white");
        if (!white.handle)
            return;
        const float x0 = Snap(x), y0 = Snap(y), x1 = Snap(x + w), y1 = Snap(y + h);
        if (x1 <= x0 || y1 <= y0)
            return;
        Append(Current(white.handle, false), {x0, y0, x1, y0, x1, y1, x0, y1}, {0, 0, 1, 0, 1, 1, 0, 1},
               PackPremultiplied(c));
    }

    void Canvas::Gradient(float x, float y, float w, float h, bool vertical, Color a, Color b) {
        const TexInfo white = tex.Get("mc:white");
        if (!white.handle || w <= 0 || h <= 0)
            return;
        const float x0 = Snap(x), y0 = Snap(y), x1 = Snap(x + w), y1 = Snap(y + h);
        if (x1 <= x0 || y1 <= y0)
            return;
        Batch& batch = Current(white.handle, false);
        const uint32_t c0 = PackPremultiplied(a), c1 = PackPremultiplied(b);
        const uint32_t base = static_cast<uint32_t>(batch.vertices.size());
        const uint32_t tr = vertical ? c0 : c1, bl = vertical ? c1 : c0;
        batch.vertices.push_back({x0, y0, 0.0f, c0, 0, 0});
        batch.vertices.push_back({x1, y0, 0.0f, tr, 1, 0});
        batch.vertices.push_back({x1, y1, 0.0f, c1, 1, 1});
        batch.vertices.push_back({x0, y1, 0.0f, bl, 0, 1});
        for (const uint32_t i : {0u, 2u, 1u, 0u, 3u, 2u})
            batch.indices.push_back(base + i);
    }

    float Canvas::Text(const Face& face, const std::string& text, float x, float baseline, Color color) {
        if (color.a == 0)
            return 0;
        return EmitText(face, text, Snap(x), Snap(baseline), color, *this);
    }

    void Canvas::Item(const std::string& key, float x, float y, float w, float h, float alpha, bool glint) {
        if (key.empty() || w <= 0 || h <= 0)
            return;
        const TexInfo t = tex.Get("mcicon:" + key);
        if (!t.handle) {
            starved = true;
            return;
        }
        if (t.w == ui::kOverflowIconPx && t.h == ui::kOverflowIconPx) {
            // A model icon with a margin around its slot (see UiAssets): grow the rect by that margin.
            const float mx = w * static_cast<float>(ui::kOverflowIconPx - ui::kOverflowIconSlotPx) /
                             (2.0f * static_cast<float>(ui::kOverflowIconSlotPx));
            const float my = h * static_cast<float>(ui::kOverflowIconPx - ui::kOverflowIconSlotPx) /
                             (2.0f * static_cast<float>(ui::kOverflowIconSlotPx));
            x -= mx;
            y -= my;
            w += 2.0f * mx;
            h += 2.0f * my;
        }
        const float x0 = Snap(x), y0 = Snap(y), x1 = Snap(x + w), y1 = Snap(y + h);
        const uint32_t color = Packed(1, 1, 1, alpha);
        Append(Current(t.handle, false), {x0, y0, x1, y0, x1, y1, x0, y1}, {0, 0, 1, 0, 1, 1, 0, 1}, color);
        if (glint)
            Append(Current(t.handle, true), {x0, y0, x1, y0, x1, y1, x0, y1}, {0, 0, 1, 0, 1, 1, 0, 1}, color);
    }

    void Canvas::PushClip(float x0f, float y0f, float x1f, float y1f) {
        const Scissor& cur = scissors.back();
        int x0 = static_cast<int>(std::lround(x0f * static_cast<float>(device)));
        int y0 = static_cast<int>(std::lround(y0f * static_cast<float>(device)));
        int x1 = static_cast<int>(std::lround(x1f * static_cast<float>(device)));
        int y1 = static_cast<int>(std::lround(y1f * static_cast<float>(device)));
        if (cur.on) {
            x0 = std::max(x0, cur.r[0]);
            y0 = std::max(y0, cur.r[1]);
            x1 = std::min(x1, cur.r[0] + cur.r[2]);
            y1 = std::min(y1, cur.r[1] + cur.r[3]);
        }
        scissors.push_back({true, {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)}});
    }

    void Canvas::PopClip() {
        if (scissors.size() > 1)
            scissors.pop_back();
    }

    bool Canvas::ClipEmpty() const {
        const Scissor& s = scissors.back();
        return s.on && (s.r[2] <= 0 || s.r[3] <= 0);
    }

    namespace {
        std::vector<float> Floats(const std::string& text) {
            std::vector<float> out;
            std::istringstream in(text);
            float v;
            while (in >> v)
                out.push_back(v);
            return out;
        }
    }

    void Canvas::Image(const ImageSpec& e) {
        if (e.src.empty() || e.w <= 0 || e.h <= 0)
            return;
        const TexInfo t = tex.Get(e.src);
        if (!t.handle || t.w <= 0 || t.h <= 0)
            return;
        const float tw = static_cast<float>(t.w), th = static_cast<float>(t.h);
        const uint32_t color = Packed(e.tint.r / 255.0f, e.tint.g / 255.0f, e.tint.b / 255.0f, e.alpha);

        float su = 0, sv = 0, sw = tw, sh = th;
        if (e.hasUv) {
            su = e.uv[0];
            sv = e.uv[1];
            sw = tw - su;
            sh = th - sv;
            if (e.uv[2] >= 0)
                sw = e.uv[2];
            if (e.uv[3] >= 0)
                sh = e.uv[3];
        }

        if (!std::isfinite(su) || !std::isfinite(sv) || !std::isfinite(sw) || !std::isfinite(sh) || sw <= 0 || sh <= 0)
            return;
        float dx = 0, dy = 0, dw = e.w, dh = e.h;
        const TextureSidecar sidecar = tex.Sidecar(e.src);
        const float baseW = e.baseW > 0 ? e.baseW : sidecar.baseW > 0 ? static_cast<float>(sidecar.baseW) : sw;
        const float baseH = e.baseH > 0 ? e.baseH : sidecar.baseH > 0 ? static_cast<float>(sidecar.baseH) : sh;
        bool nine = e.nine || sidecar.nine;
        float cu0 = 0, cu1 = 1, cv0 = 0, cv1 = 1;
        bool clipped = false;
        if (!e.clip.empty()) {
            std::istringstream in(e.clip);
            std::string dir, precision;
            float visible = 1;
            in >> dir >> visible >> precision;
            visible = std::clamp(visible, 0.0f, 1.0f);
            float cutX = 1 - visible, cutY = cutX;
            if (precision == "pixelperfect") {
                // A small epsilon counters rounding error for ratios exactly on a texel edge.
                cutX = std::floor(cutX * sw + 1e-5f) / sw;
                cutY = std::floor(cutY * sh + 1e-5f) / sh;
            }
            if (dir == "left") cu1 = 1 - cutX;
            else if (dir == "right") cu0 = cutX;
            else if (dir == "up") cv1 = 1 - cutY;
            else if (dir == "down") cv0 = cutY;
            else if (dir == "center") {
                cu0 = cutX * .5f; cu1 = 1 - cu0;
                cv0 = cutY * .5f; cv1 = 1 - cv0;
            }
            clipped = cu0 != 0 || cv0 != 0 || cu1 != 1 || cv1 != 1;
        }
        if (!nine && !e.tiled && !clipped) {
            const float ratio = sw / sh;
            if (e.cover) {
                if (dw / dh > ratio) { const float h = sw * dh / dw; sv += (sh - h) * .5f; sh = h; }
                else { const float w = sh * dw / dh; su += (sw - w) * .5f; sw = w; }
            } else if (e.keep) {
                if (dw / dh > ratio) { const float w = dh * ratio; dx = (dw - w) * .5f; dw = w; }
                else { const float h = dw / ratio; dy = (dh - h) * .5f; dh = h; }
            }
        }

        const auto quad = [&](float x, float y, float w, float h, float u0, float v0, float u1, float v1) {
            if (w <= 0 || h <= 0)
                return;
            const float vx0 = dx + dw * cu0, vx1 = dx + dw * cu1, vy0 = dy + dh * cv0, vy1 = dy + dh * cv1;
            const float x0 = std::max(x, vx0), x1 = std::min(x + w, vx1);
            const float y0 = std::max(y, vy0), y1 = std::min(y + h, vy1);
            if (x1 <= x0 || y1 <= y0)
                return;
            const float ku = (u1 - u0) / w, kv = (v1 - v0) / h;
            const float fu0 = (u0 + (x0 - x) * ku) / tw, fv0 = (v0 + (y0 - y) * kv) / th;
            const float fu1 = (u0 + (x1 - x) * ku) / tw, fv1 = (v0 + (y1 - y) * kv) / th;
            const float ax = Snap(e.x + x0), ay = Snap(e.y + y0), bx = Snap(e.x + x1), by = Snap(e.y + y1);
            if (bx <= ax || by <= ay) return;
            Append(Current(t.handle, false), {ax, ay, bx, ay, bx, by, ax, by}, {fu0, fv0, fu1, fv0, fu1, fv1, fu0, fv1}, color);
        };

        // Tile only along selected axes; partial edge tiles keep source density.
        // Degenerate one-texel axes can be stretched without changing pixels.
        int tilesDrawn = 0;
        const auto repeat = [&](float x, float y, float w, float h, float u, float v,
                                float uw, float vh, bool tx, bool ty) {
            if (w <= 0 || h <= 0) return;
            tx = tx && uw > 1; ty = ty && vh > 1;
            const float stepX = tx ? uw * e.tileScale : w;
            const float stepY = ty ? vh * e.tileScaleY : h;
            if (!std::isfinite(stepX) || !std::isfinite(stepY) || stepX < 0.001f || stepY < 0.001f) {
                quad(x, y, w, h, u, v, u + uw, v + vh); return;
            }
            for (float yy = 0; yy < h && tilesDrawn < 8192; yy += stepY)
                for (float xx = 0; xx < w && tilesDrawn < 8192; xx += stepX, ++tilesDrawn) {
                    const float cw = std::min(stepX, w - xx), ch = std::min(stepY, h - yy);
                    quad(x + xx, y + yy, cw, ch, u, v, u + uw * cw / stepX, v + vh * ch / stepY);
                }
        };
        const bool tx = e.tiled == 1 || e.tiled == 2, ty = e.tiled == 1 || e.tiled == 3;
        if (nine) {
            const float* slice = e.nine ? e.slice : sidecar.slice;
            float dl = slice[0], dt = slice[1], dr = slice[2], db = slice[3];
            const auto fit = [](float& a, float& b, float available) {
                a = std::max(0.0f, a); b = std::max(0.0f, b);
                if (a + b > available) { const float scale = available / (a + b); a *= scale; b *= scale; }
            };
            // Preserve overlapping source insets; destination insets cannot overlap.
            const float l = std::clamp(dl * sw / baseW, 0.0f, sw), r = std::clamp(dr * sw / baseW, 0.0f, sw);
            const float tt = std::clamp(dt * sh / baseH, 0.0f, sh), bb = std::clamp(db * sh / baseH, 0.0f, sh);
            fit(dl, dr, dw); fit(dt, db, dh);
            const float xs[4] = {dx, dx + dl, dx + dw - dr, dx + dw};
            const float ys[4] = {dy, dy + dt, dy + dh - db, dy + dh};
            const float us[4] = {su, su + l, su + sw - r, su + sw};
            const float vs[4] = {sv, sv + tt, sv + sh - bb, sv + sh};
            const auto span = [](float& a, float& b, float origin, float size) {
                if (a == b) { a = std::clamp(a - .5f, origin, origin + std::max(0.0f, size - 1)); b = std::min(a + 1, origin + size); }
            };
            for (int j = 0; j < 3; ++j) for (int i = 0; i < 3; ++i) {
                float u0 = us[i], u1 = us[i+1], v0 = vs[j], v1 = vs[j+1];
                span(u0, u1, su, sw); span(v0, v1, sv, sh);
                const float w = xs[i+1] - xs[i], h = ys[j+1] - ys[j];
                const bool repeatX = tx && (i == 1 || (ty && j == 1));
                const bool repeatY = ty && (j == 1 || (tx && i == 1));
                if ((repeatX || repeatY) && u1 > u0 && v1 > v0)
                    repeat(xs[i], ys[j], w, h, u0, v0, u1-u0, v1-v0, repeatX, repeatY);
                else quad(xs[i], ys[j], w, h, u0, v0, u1, v1);
            }
        } else if (e.tiled) {
            repeat(dx, dy, dw, dh, su, sv, sw, sh, tx, ty);
        } else {
            quad(dx, dy, dw, dh, su, sv, su + sw, sv + sh);
        }
    }
}
