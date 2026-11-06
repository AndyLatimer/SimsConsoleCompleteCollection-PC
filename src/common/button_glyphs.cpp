// PlayStation button glyphs: the D3D12 renderer extension that sees every texture level as it is
// decoded for upload (gpu/ext.h Extension::Decoded).
//
// The title's face-button icons are four 32x32 P8 textures, found with SIMS2_TEXDUMP below in the
// menus, Create a Sim and live mode: a black square, a metallic ring at radius ~13-16 and a coloured
// centre with the letter, stored bottom row first. Each is known by the FNV-1a hash of its decoded
// BGRA pixels. With button_glyphs = 1 (settings.toml, the default) the centre inside the ring is
// redrawn as a black DualSense button with its symbol, by position as SDL maps a DualSense to the
// console pad: A Cross, B Circle, X Square, Y Triangle. The ring and corners stay the title's own.
// The art is drawn here, at any size, 4x4 supersampled; nothing is loaded from disk.
//
// SIMS2_TEXDUMP=<dir>: writes each distinct small texture (level 0, at most 256x256) once, named
// by size, format and a hash of its decoded pixels. BGRA textures as 32-bit BMP, DXT ones as DDS.
#if defined(_WIN32)
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/log.h"
#include "gpu/ext.h"

namespace glyphs {

uint32_t g_enabled = 1;  // settings.toml button_glyphs: 1 PlayStation, 0 the title's own

uint64_t Fnv(const uint8_t* p, size_t n) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}

namespace {

void Put16(std::vector<uint8_t>& b, uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8)); }
void Put32(std::vector<uint8_t>& b, uint32_t v) { for (int i = 0; i < 4; i++) b.push_back(uint8_t(v >> (8 * i))); }

void WriteFile(const std::filesystem::path& p, const std::vector<uint8_t>& b) {
    if (FILE* f = _wfopen(p.c_str(), L"wb")) {
        std::fwrite(b.data(), 1, b.size(), f);
        std::fclose(f);
    }
}

// 32-bit top-down BMP with alpha
void WriteBmp(const std::filesystem::path& p, uint32_t w, uint32_t h, const uint8_t* px, uint32_t pitch) {
    std::vector<uint8_t> b;
    uint32_t data = w * h * 4;
    b.push_back('B'); b.push_back('M');
    Put32(b, 14 + 40 + data); Put32(b, 0); Put32(b, 14 + 40);
    Put32(b, 40); Put32(b, w); Put32(b, uint32_t(-int32_t(h))); Put16(b, 1); Put16(b, 32);
    Put32(b, 0); Put32(b, data); Put32(b, 2835); Put32(b, 2835); Put32(b, 0); Put32(b, 0);
    for (uint32_t y = 0; y < h; y++) b.insert(b.end(), px + size_t(y) * pitch, px + size_t(y) * pitch + w * 4);
    WriteFile(p, b);
}

void WriteDds(const std::filesystem::path& p, uint32_t w, uint32_t h, const char* fourcc, const std::vector<uint8_t>& px) {
    std::vector<uint8_t> b = {'D', 'D', 'S', ' '};
    Put32(b, 124); Put32(b, 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000); Put32(b, h); Put32(b, w);
    Put32(b, uint32_t(px.size())); Put32(b, 0); Put32(b, 1);
    for (int i = 0; i < 11; i++) Put32(b, 0);
    Put32(b, 32); Put32(b, 0x4); b.insert(b.end(), fourcc, fourcc + 4);
    for (int i = 0; i < 5; i++) Put32(b, 0);
    Put32(b, 0x1000); for (int i = 0; i < 4; i++) Put32(b, 0);
    b.insert(b.end(), px.begin(), px.end());
    WriteFile(p, b);
}

enum class Symbol { None, Cross, Circle, Square, Triangle };

// the title's icons, by the hash of level 0 decoded (texdump names)
Symbol Known(uint64_t h) {
    switch (h) {
        // Sims 2 (PAL), from a SIMS2_TEXDUMP of a play session (2026-11-06): 32x32 DXT3, one level,
        // a green ball with a black A filling the whole texture. B, X, Y and the other buttons were
        // not loaded as textures in that session.
        case 0xdd9aa5936b90f73eull: return Symbol::Cross;  // A
        default: return Symbol::None;
    }
}

// the ball in icon units (32 per icon, y down), and the symbol scale against the Sims 1 layout (radius 14).
// Sims 2's ball fills its texture: centre (15.5, 15.5), radius 15.5 with its dark outline.
constexpr float kCx = 15.5f, kCy = 15.5f, kR = 15.5f, kS = kR / 14.0f;

float Clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

// distance from p to the segment ab
float SegDist(float px, float py, float ax, float ay, float bx, float by) {
    float vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
    float t = Clamp01((wx * vx + wy * vy) / (vx * vx + vy * vy));
    float dx = wx - t * vx, dy = wy - t * vy;
    return std::sqrt(dx * dx + dy * dy);
}

// is the point (in units of a 32-pixel icon, centre 16,16, y down) on the symbol's stroke
bool OnSymbol(Symbol s, float x, float y) {
    const float cx = kCx, cy = kCy, t = 1.55f * kS;  // half the stroke width
    switch (s) {
        case Symbol::Cross: {
            const float r = 5.6f * kS;
            return SegDist(x, y, cx - r, cy - r, cx + r, cy + r) < t || SegDist(x, y, cx - r, cy + r, cx + r, cy - r) < t;
        }
        case Symbol::Circle: {
            const float d = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
            return std::fabs(d - 6.0f * kS) < t;
        }
        case Symbol::Square: {
            const float h = 5.4f * kS;
            const float dx = std::fabs(x - cx), dy = std::fabs(y - cy);
            return std::max(dx, dy) < h + t && std::max(dx, dy) > h - t;
        }
        case Symbol::Triangle: {
            const float r = 7.0f * kS;  // circumradius, apex up, centred a little low so it sits optically centred
            const float ax = cx, ay = cy - r + 0.8f * kS;
            const float bx = cx - r * 0.866f, by = cy + r * 0.5f + 0.8f * kS;
            const float qx = cx + r * 0.866f, qy = by;
            return SegDist(x, y, ax, ay, bx, by) < t || SegDist(x, y, bx, by, qx, qy) < t || SegDist(x, y, qx, qy, ax, ay) < t;
        }
        default: return false;
    }
}

// the DualSense symbol colours, BGR
void SymbolColour(Symbol s, float& r, float& g, float& b) {
    switch (s) {
        case Symbol::Cross: r = 124; g = 178; b = 232; break;
        case Symbol::Circle: r = 255; g = 102; b = 102; break;
        case Symbol::Square: r = 231; g = 155; b = 213; break;
        case Symbol::Triangle: r = 64; g = 226; b = 160; break;
        default: r = g = b = 255; break;
    }
}

// redraws the centre of one level (w x h BGRA, bottom row first) inside the ring
void Paint(Symbol s, uint32_t w, uint32_t h, uint8_t* px, uint32_t pitch) {
    const float k = 32.0f / float(std::min(w, h));  // pixel -> icon units
    float sr, sg, sb;
    SymbolColour(s, sr, sg, sb);
    for (uint32_t row = 0; row < h; row++) {
        const uint32_t y = h - 1 - row;  // stored bottom row first
        for (uint32_t x = 0; x < w; x++) {
            const float ux = (float(x) + 0.5f) * k, uy = (float(y) + 0.5f) * k;
            const float d = std::sqrt((ux - kCx) * (ux - kCx) + (uy - kCy) * (uy - kCy));
            const float inner = Clamp01(kR + 0.5f - d);  // the whole ball, edge antialiased
            if (inner <= 0.0f) continue;
            float cover = 0.0f;
            for (int sy = 0; sy < 4; sy++)
                for (int sx = 0; sx < 4; sx++)
                    cover += OnSymbol(s, (float(x) + (sx + 0.5f) / 4.0f) * k, (float(y) + (sy + 0.5f) / 4.0f) * k) ? 1.0f : 0.0f;
            cover /= 16.0f;
            // a black glossy button: lighter at the top, a soft highlight top left, a thin grey bezel
            const float shade = 46.0f - 30.0f * Clamp01((uy - kCy + kR) / (2.0f * kR));
            const float hl = 38.0f * Clamp01(1.0f - std::sqrt((ux - kCx + 4.0f) * (ux - kCx + 4.0f) + (uy - kCy + 7.0f) * (uy - kCy + 7.0f)) / 7.5f);
            const float bezel = 110.0f * Clamp01(1.0f - std::fabs(d - (kR - 0.9f)) / 1.1f);
            float r = shade + hl + bezel, g = shade + hl + bezel, b = shade + 4.0f + hl + bezel;
            r += (sr - r) * cover;
            g += (sg - g) * cover;
            b += (sb - b) * cover;
            uint8_t* p = px + size_t(row) * pitch + size_t(x) * 4;
            p[0] = uint8_t(p[0] + (b - p[0]) * inner);
            p[1] = uint8_t(p[1] + (g - p[1]) * inner);
            p[2] = uint8_t(p[2] + (r - p[2]) * inner);
        }
    }
}

// DXT3 (BC2) blocks <-> BGRA, for icons the title stores compressed: the level is decoded, painted
// and encoded again in place, so the upload keeps its size and host format.
void Rgb565(uint16_t v, float c[3]) {
    c[0] = float((v >> 11) & 31) * 255.0f / 31.0f;  // r
    c[1] = float((v >> 5) & 63) * 255.0f / 63.0f;   // g
    c[2] = float(v & 31) * 255.0f / 31.0f;          // b
}

uint16_t To565(const float c[3]) {
    auto q = [](float v, int m) { return uint16_t(std::clamp(int(v * m / 255.0f + 0.5f), 0, m)); };
    return uint16_t(q(c[0], 31) << 11 | q(c[1], 63) << 5 | q(c[2], 31));
}

void Palette(uint16_t c0, uint16_t c1, float pal[4][3]) {
    Rgb565(c0, pal[0]);
    Rgb565(c1, pal[1]);
    for (int k = 0; k < 3; k++) {
        pal[2][k] = (2 * pal[0][k] + pal[1][k]) / 3;
        pal[3][k] = (pal[0][k] + 2 * pal[1][k]) / 3;
    }
}

// w x h BGRA (rows of w*4 bytes) from BC2 blocks
void DecodeBc2(const uint8_t* blocks, uint32_t w, uint32_t h, std::vector<uint8_t>& out) {
    out.assign(size_t(w) * h * 4, 0);
    const uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
    for (uint32_t by = 0; by < bh; by++)
        for (uint32_t bx = 0; bx < bw; bx++) {
            const uint8_t* b = blocks + (size_t(by) * bw + bx) * 16;
            uint16_t c0, c1;
            uint32_t bits;
            std::memcpy(&c0, b + 8, 2);
            std::memcpy(&c1, b + 10, 2);
            std::memcpy(&bits, b + 12, 4);
            float pal[4][3];
            Palette(c0, c1, pal);
            for (uint32_t i = 0; i < 16; i++) {
                const uint32_t x = bx * 4 + i % 4, y = by * 4 + i / 4;
                if (x >= w || y >= h) continue;
                const float* c = pal[(bits >> (2 * i)) & 3];
                const uint8_t a4 = (b[i / 2] >> (4 * (i & 1))) & 15;
                uint8_t* p = out.data() + (size_t(y) * w + x) * 4;
                p[0] = uint8_t(c[2]);
                p[1] = uint8_t(c[1]);
                p[2] = uint8_t(c[0]);
                p[3] = uint8_t(a4 * 17);
            }
        }
}

// BC2 blocks from w x h BGRA: explicit 4-bit alpha; colour endpoints at the block's extreme
// projections on its channel-range axis, each texel given the nearest of the four colours
void EncodeBc2(const std::vector<uint8_t>& in, uint32_t w, uint32_t h, uint8_t* blocks) {
    const uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
    for (uint32_t by = 0; by < bh; by++)
        for (uint32_t bx = 0; bx < bw; bx++) {
            uint8_t* b = blocks + (size_t(by) * bw + bx) * 16;
            float px[16][3];
            uint8_t al[16];
            for (uint32_t i = 0; i < 16; i++) {
                const uint32_t x = std::min(bx * 4 + i % 4, w - 1), y = std::min(by * 4 + i / 4, h - 1);
                const uint8_t* p = in.data() + (size_t(y) * w + x) * 4;
                px[i][0] = p[2];
                px[i][1] = p[1];
                px[i][2] = p[0];
                al[i] = p[3];
            }
            std::memset(b, 0, 8);
            for (uint32_t i = 0; i < 16; i++) b[i / 2] |= uint8_t(((al[i] * 15 + 127) / 255) << (4 * (i & 1)));
            float lo[3] = {255, 255, 255}, hi[3] = {0, 0, 0};
            for (auto& c : px)
                for (int k = 0; k < 3; k++) lo[k] = std::min(lo[k], c[k]), hi[k] = std::max(hi[k], c[k]);
            const float ax[3] = {hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]};
            int imin = 0, imax = 0;
            float pmin = 1e9f, pmax = -1e9f;
            for (int i = 0; i < 16; i++) {
                const float pr = px[i][0] * ax[0] + px[i][1] * ax[1] + px[i][2] * ax[2];
                if (pr < pmin) pmin = pr, imin = i;
                if (pr > pmax) pmax = pr, imax = i;
            }
            uint16_t c0 = To565(px[imax]), c1 = To565(px[imin]);
            if (c0 < c1) std::swap(c0, c1);
            float pal[4][3];
            Palette(c0, c1, pal);
            uint32_t bits = 0;
            for (int i = 0; i < 16; i++) {
                int best = 0;
                float bd = 1e18f;
                for (int k = 0; k < 4; k++) {
                    const float dr = px[i][0] - pal[k][0], dg = px[i][1] - pal[k][1], db = px[i][2] - pal[k][2];
                    const float dd = dr * dr * 0.30f + dg * dg * 0.59f + db * db * 0.11f;
                    if (dd < bd) bd = dd, best = k;
                }
                bits |= uint32_t(best) << (2 * i);
            }
            std::memcpy(b + 8, &c0, 2);
            std::memcpy(b + 10, &c1, 2);
            std::memcpy(b + 12, &bits, 4);
        }
}

class GlyphExtension : public gpu::Extension {
public:
    GlyphExtension() {
        if (const char* d = std::getenv("SIMS2_TEXDUMP"); d && d[0]) {
            dump_ = d;
            std::error_code ec;
            std::filesystem::create_directories(dump_, ec);
            LOGI("glyphs: dumping small textures to %s", d);
        }
    }

    void Decoded(const gpu::TextureDesc& d, const gpu::FormatInfo& fi, uint32_t face, uint32_t level,
                 std::vector<uint8_t>& px, uint32_t pitch, uint8_t&) override {
        if (face || d.width > 1024 || d.height > 1024 || d.depth > 1 || px.empty()) return;
        const uint64_t h = Fnv(px.data(), px.size());
        if ((fi.host == gpu::Host::BGRA8 || fi.host == gpu::Host::BC2) && g_enabled) {
            // level 0 names the texture; its smaller levels follow it in the same upload
            if (level == 0) current_ = Known(h);
            if (current_ != Symbol::None) {
                const uint32_t w = std::max(1u, d.width >> level), hh = std::max(1u, d.height >> level);
                if (fi.host == gpu::Host::BGRA8) {
                    if (px.size() >= size_t(pitch) * hh) Paint(current_, w, hh, px.data(), pitch);
                } else if (w >= 4 && hh >= 4 && px.size() >= size_t((w + 3) / 4) * ((hh + 3) / 4) * 16) {
                    // a compressed level is decoded, painted and encoded again; levels under 4x4 stay as made
                    std::vector<uint8_t> bgra;
                    DecodeBc2(px.data(), w, hh, bgra);
                    Paint(current_, w, hh, bgra.data(), w * 4);
                    EncodeBc2(bgra, w, hh, px.data());
                }
                if (level == 0) LOG_FIRST(8, "glyphs: %ux%u %s icon %016llx drawn as a PlayStation button", w, hh, fi.name, (unsigned long long)h);
            }
        }
        if (dump_.empty() || level) return;
        if (!seen_.insert(h).second) return;
        char name[96];
        std::snprintf(name, sizeof name, "%ux%u_%s_%016llx", d.width, d.height, fi.name, (unsigned long long)h);
        std::filesystem::path out = dump_ / name;
        switch (fi.host) {
            case gpu::Host::BGRA8: WriteBmp(out.replace_extension(".bmp"), d.width, d.height, px.data(), pitch); break;
            case gpu::Host::BC1: WriteDds(out.replace_extension(".dds"), d.width, d.height, "DXT1", px); break;
            case gpu::Host::BC2: WriteDds(out.replace_extension(".dds"), d.width, d.height, "DXT3", px); break;
            case gpu::Host::BC3: WriteDds(out.replace_extension(".dds"), d.width, d.height, "DXT5", px); break;
            default: break;
        }
    }

private:
    std::filesystem::path dump_;
    std::unordered_set<uint64_t> seen_;  // render thread only
    Symbol current_ = Symbol::None;      // the texture whose levels are being uploaded
};

}  // namespace

void Install() { gpu::SetExtension(std::make_unique<GlyphExtension>()); }

}  // namespace glyphs
#else
#include <cstdint>
namespace glyphs {
uint32_t g_enabled = 1;  // the Vulkan backend has no extension: the title's own icons
void Install() {}
}  // namespace glyphs
#endif
