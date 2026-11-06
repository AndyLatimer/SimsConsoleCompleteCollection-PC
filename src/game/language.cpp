// The game's language, from the settings file (language = 0..4) instead of always English.
//
// The USA and European discs carry the same data: all thirteen data files (datasets.arc,
// quickdat.arc, movies.arc, ...) have the same SHA-256 on both; only default.xbe differs
// (out/audit, docs/LOCALIZATION.md). So the USA disc already holds the French, German,
// Italian and Spanish text.
//
// The USA image also keeps the code that reads the language: the global at 0x323C20 has seven
// references, the same seven, in the same order and shape, as the European image's 0x323C80
// (writes from the options byte at 0x2A2B4, 0x6408A, 0x64143, 0xB78F6; the read at 0x13D580;
// the reset at 0x18B1F9). What it lacks is the start-up translation of the console language.
// The European start-up function stores it (PAL 0x11391: mov [0x323C80], eax, eax from
// XGetLanguage at 0x11353 through the table at 0x1168C), where the USA twin stores zero
// (0x1135B: mov [0x323C20], ebx with ebx = 0). The European table maps XC_LANGUAGE to the
// game's numbering: English (and Japanese) 0, French 1, German 2, Italian 3, Spanish 4.
//
// This hook runs after that store and writes the language the settings ask for. 0 is the stock
// USA run exactly.
#include <cstdint>

#include "core/log.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"

namespace sims1 {
// settings.toml `language`: 0 English, 1 French, 2 German, 3 Italian, 4 Spanish (the default)
uint32_t g_language = 4;
}  // namespace sims1

namespace {
constexpr uint32_t kGameLanguage = 0x323C20;
const char* const kNames[] = {"English", "French", "German", "Italian", "Spanish"};
}  // namespace

// [[hook]] at 0x1135B, after: the start-up store of the game language (config/game.toml)
void Sims1SetLanguage(X86Context&, uint8_t*) {
    uint32_t lang = sims1::g_language;
    if (lang > 4) lang = 0;
    kernel::W32(kGameLanguage, lang);
    LOGI("language: game language %u (%s)", lang, kNames[lang]);
}
