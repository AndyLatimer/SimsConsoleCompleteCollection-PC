// PlayStation button glyphs: the D3D12 renderer extension that sees every texture level as it is
// decoded for upload (gpu/ext.h Extension::Decoded).
//
// The title's face-button icons are four 32x32 P8 textures, found with SIMS1_TEXDUMP below in the
// menus, Create a Sim and live mode: a black square, a metallic ring at radius ~13-16 and a coloured
// centre with the letter, stored bottom row first. Each is known by the FNV-1a hash of its decoded
// BGRA pixels. With button_glyphs = 1 (settings.toml, the default) the centre inside the ring is
// redrawn as a black DualSense button with its symbol, by position as SDL maps a DualSense to the
// console pad: A Cross, B Circle, X Square, Y Triangle. The ring and corners stay the title's own.
// The art is drawn here, at any size, 4x4 supersampled; nothing is loaded from disk.
//
// SIMS1_TEXDUMP=<dir>: writes each distinct small texture (level 0, at most 256x256) once, named
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
        case 0xe8e94d450337bad6ull: return Symbol::Cross;     // A, green
        case 0xf2c85edbadde345bull: return Symbol::Circle;    // B, red
        case 0x4eca81485b9f5f35ull: return Symbol::Square;    // X, blue
        case 0xe0f37bf4ace48c97ull: return Symbol::Triangle;  // Y, orange
        default: return Symbol::None;
    }
}

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
    const float cx = 16.0f, cy = 16.0f, t = 1.55f;  // half the stroke width
    switch (s) {
        case Symbol::Cross: {
            const float r = 5.6f;
            return SegDist(x, y, cx - r, cy - r, cx + r, cy + r) < t || SegDist(x, y, cx - r, cy + r, cx + r, cy - r) < t;
        }
        case Symbol::Circle: {
            const float d = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
            return std::fabs(d - 6.0f) < t;
        }
        case Symbol::Square: {
            const float h = 5.4f;
            const float dx = std::fabs(x - cx), dy = std::fabs(y - cy);
            return std::max(dx, dy) < h + t && std::max(dx, dy) > h - t;
        }
        case Symbol::Triangle: {
            const float r = 7.0f;  // circumradius, apex up, centred a little low so it sits optically centred
            const float ax = cx, ay = cy - r + 0.8f;
            const float bx = cx - r * 0.866f, by = cy + r * 0.5f + 0.8f;
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
            const float d = std::sqrt((ux - 16.0f) * (ux - 16.0f) + (uy - 16.0f) * (uy - 16.0f));
            const float inner = Clamp01(14.0f - d);  // 1 inside the ring, over the title's coloured rim too
            if (inner <= 0.0f) continue;
            float cover = 0.0f;
            for (int sy = 0; sy < 4; sy++)
                for (int sx = 0; sx < 4; sx++)
                    cover += OnSymbol(s, (float(x) + (sx + 0.5f) / 4.0f) * k, (float(y) + (sy + 0.5f) / 4.0f) * k) ? 1.0f : 0.0f;
            cover /= 16.0f;
            // a black glossy button: lighter at the top, a soft highlight top left
            const float shade = 46.0f - 30.0f * Clamp01(uy / 26.0f);
            const float hl = 38.0f * Clamp01(1.0f - std::sqrt((ux - 12.0f) * (ux - 12.0f) + (uy - 9.0f) * (uy - 9.0f)) / 7.0f);
            float r = shade + hl, g = shade + hl, b = shade + 4.0f + hl;
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

class GlyphExtension : public gpu::Extension {
public:
    GlyphExtension() {
        if (const char* d = std::getenv("SIMS1_TEXDUMP"); d && d[0]) {
            dump_ = d;
            std::error_code ec;
            std::filesystem::create_directories(dump_, ec);
            LOGI("glyphs: dumping small textures to %s", d);
        }
    }

    void Decoded(const gpu::TextureDesc& d, const gpu::FormatInfo& fi, uint32_t face, uint32_t level,
                 std::vector<uint8_t>& px, uint32_t pitch, uint8_t&) override {
        if (face || d.width > 256 || d.height > 256 || d.depth > 1 || px.empty()) return;
        const uint64_t h = Fnv(px.data(), px.size());
        if (fi.host == gpu::Host::BGRA8 && g_enabled) {
            // level 0 names the texture; its smaller levels follow it in the same upload
            if (level == 0) current_ = Known(h);
            if (current_ != Symbol::None) {
                const uint32_t w = std::max(1u, d.width >> level), hh = std::max(1u, d.height >> level);
                if (px.size() >= size_t(pitch) * hh) Paint(current_, w, hh, px.data(), pitch);
                if (level == 0) LOG_FIRST(8, "glyphs: %ux%u icon %016llx drawn as a PlayStation button", w, hh, (unsigned long long)h);
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
