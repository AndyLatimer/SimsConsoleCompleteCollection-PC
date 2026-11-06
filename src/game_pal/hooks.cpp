// Frame-rate / game-speed decoupling for the Europe (PAL) xbe, SHA-1 68d7eabd...
//
// The same two hooks as the USA port (src/game/hooks.cpp, docs/analysis-frame-step.md), at the PAL
// addresses. Each was found by its code, not by an offset from USA (docs/REGIONS.md):
//
//   USA        PAL        what
//   0x1C2400   0x1C3BC0   frame-step quantiser: `mov cl,[eax+5]`, two paths each doing
//                         fld elapsed / fdiv [tick] / fadd 0.5 / _ftol / clamp 1..15, retn 4.
//                         Only caller 0x1C3D5F (USA 0x1C259F), in the frame tick.
//   0x3526D4   0x3540D4   seconds per tick, written once by the app ctor at 0x1C389A
//   0x339F40   0x33B930   n15, the frame's tick count
//   0x3B810C   0x3B9B0C   running tick total
//   0x1BCA50   0x1BE240   pad auto-repeat, pad in eax; callers 0x1BE235 (tail of 0x1BE200, which first
//                         snapshots the tick total 0x3B9B0C) and 0x1C2EA7. Delay/rate bytes at
//                         pad+0x1D8/+0x1D9, set to 10/3 by the pad ctor at 0x1BE06D/0x1BE074.
//
// The PAL difference that matters: the ctor copies 0.02 s (0x317C40), not 1/60 s, into the tick. The
// engine rounds each frame to whole 20 ms ticks, so at 60 fps every 16.7 ms frame is stepped as 20 ms
// and the whole game runs 1.2x fast; uncapped it runs faster still. As in the USA port, the step is
// replaced by a 1 ms quantiser that advances by the time that really passed.
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>

#include "core/log.h"
#include "cpu/x86_ops.h"
#include "xdk/hle.h"

namespace {
constexpr double kConsoleRate = 50.0;  // polls per second the PAL engine was paced for (its 20 ms tick)
std::atomic<double> g_dt{1.0 / kConsoleRate};

bool HookOff(const char* name) {
    const char* v = std::getenv(name);
    return v && v[0] && std::strcmp(v, "0") != 0;
}
}  // namespace

// sub_001C3BC0(this in eax, float elapsed) -> ticks, retn 4. Writes the tick size, n15 and the tick
// total exactly as the original does, with ticks of 1 ms instead of 20 ms, so ticks * tick size is the
// real elapsed time and the frame tick derives the engine delta from it as before.
X86_FUNC(__imp__sub_001C3BC0);
X86_FUNC(sub_001C3BC0) {
    if (HookOff("SIMS2_NO_STEP")) { __imp__sub_001C3BC0(ctx, base); return; }
    constexpr double kTick = 0.001;  // seconds per tick, 0.02 before this
    constexpr int kMaxTicks = 300;   // 0.3 s, the engine's own ceiling of 15 ticks of 20 ms
    const float elapsed = X86_MEMF32(ctx.esp + 0x4u);
    // not (elapsed > 0): a NaN or a negative reading takes the smallest step, as the clamp did
    const double dt = elapsed > 0.0f ? std::min(double(elapsed), kMaxTicks * kTick) : kTick;
    const int ticks = std::clamp(int(dt / kTick + 0.5), 1, kMaxTicks);
    X86_MEMF32(0x3540D4u) = float(dt / ticks);  // ticks * this == the time that really passed
    X86_MEM32(0x33B930u) = uint32_t(ticks);     // n15
    X86_MEM32(0x3B9B0Cu) = X86_MEM32(0x3B9B0Cu) + uint32_t(ticks);
    ctx.eax = uint32_t(ticks);
    g_dt.store(dt, std::memory_order_relaxed);
    LOG_FIRST(3, "frame step: elapsed %.4f s -> %d ticks of %.6f s (console tick was %.4f s)", double(elapsed), ticks, dt / ticks, 0.02);
    ctx.esp += 8;  // ret 4
}

// sub_001BE240(pad in eax): button auto-repeat counted in calls, one poll per frame. The console made
// 50 per second, so the delay/rate bytes are rescaled to the frame that really passed for the call and
// restored after, as in the USA port (which scales against 60).
X86_FUNC(__imp__sub_001BE240);
X86_FUNC(sub_001BE240) {
    if (HookOff("SIMS2_NO_REPEAT")) { __imp__sub_001BE240(ctx, base); return; }
    const uint32_t pad = ctx.eax;
    if (pad < 0x10000u) { __imp__sub_001BE240(ctx, base); return; }
    const double scale = 1.0 / (kConsoleRate * g_dt.load(std::memory_order_relaxed));
    const uint8_t delay = X86_MEM8(pad + 0x1D8u), rate = X86_MEM8(pad + 0x1D9u);
    X86_MEM8(pad + 0x1D8u) = uint8_t(std::clamp(int(delay * scale + 0.5), 1, 255));
    X86_MEM8(pad + 0x1D9u) = uint8_t(std::clamp(int(rate * scale + 0.5), 1, 255));
    __imp__sub_001BE240(ctx, base);
    X86_MEM8(pad + 0x1D8u) = delay;
    X86_MEM8(pad + 0x1D9u) = rate;
}
