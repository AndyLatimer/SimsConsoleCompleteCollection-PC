# Regions: USA and Europe (PAL) discs

The build picks the region from the SHA-1 of `assets/default.xbe` at configure time
(`CMakeLists.txt`, override with `-DSIMS2_REGION=USA|PAL`):

| Region | XBE SHA-1 | Config | Address-bound hooks | Generated code |
| --- | --- | --- | --- | --- |
| USA | `16f9e41ce26c315a758f25d43ba1fa9d3809d1ad` | `config/game.toml` | `src/game/` | `generated/game` |
| Europe (En,Fr,De,Es,It,Nl,Sv) | `68d7eabdea89c9900e1a440c10bc3905e1afdf8b` | `config/game_pal.toml` | `src/game_pal/` | `generated/game_pal` |

Region-independent host code lives in `src/common/` (including `d3d_callback.cpp`, which binds by
symbol name, not address).

## How the two xbes differ

Same title ID `0x4541038B`, same XDK 5849 libraries and QFEs, same 124 kernel imports. The PAL `.text`
is 6,064 bytes larger, so game code addresses shift non-uniformly. The XDK library sections are
**relinked in a different function order** (USA→PAL deltas inside `D3D` are +0x17A0, +0x1F30,
+0x2820, +0x3F60, −0x8B0, −0x1FF0...): no USA address can be translated by a fixed offset.

`xbe_symbols` finds the same 132/240 functions and 11/12 data symbols on both. The four symbols the USA
config adds by hand were re-located on the PAL image by neighbour order plus byte fingerprints:

| Symbol | USA | PAL | PAL evidence |
| --- | --- | --- | --- |
| `D3DDevice_Reset` | 0x271570 | 0x2734A0 | between InsertFence and SetRenderTarget, both +0x1F30; stdcall `retn 4`, ~168 bytes, 2 `.text` callers |
| `D3DDevice_SetPalette` | 0x272020 | 0x273F50 | reads `[buf+4*stage+0xF98]` (CDevice_Palettes), pushes `0x41B20`; 7 `.text` callers (as USA) |
| `D3DDevice_SetVertexShaderConstant1Fast` | 0x272FD0 | 0x2757F0 | `rep movsd` into the fast-constant scratch, calls NotInlineFast 0x275710, `retn 4`; 2 callers |
| `D3DDevice_InsertCallback` | 0x275750 | 0x276EF0 | writes the callback token `0x81D8C` and fence words `0x40100`/`0x40110` |

## Disc contents

The Europe disc has 47 files: the same `DATA/` archives as USA except `quickdat.arc`
(14,441,494 vs 14,497,786 bytes), plus per-language `ea_games_*.xmv` and `_pal` cuts of the intro,
attract and ending movies.

## Language

The PAL xbe chooses its text from the console language (`XC_LANGUAGE`). The SDK answers that from
`language` in `settings.toml` (english, german, french, spanish, italian...); the port defaults to
`spanish` when the file names none (`src/common/game_info.cpp`). Verified 2026-11-06: logo "Los Sims 2",
"Todos los derechos reservados", save-select dialog in Spanish.

## Frame step (ported)

`src/game_pal/hooks.cpp` carries the USA hooks to PAL, found by code rather than offset:
quantiser `0x1C3BC0` (USA `0x1C2400`), tick size `0x3540D4`, n15 `0x33B930`, tick total `0x3B9B0C`,
pad repeat `0x1BE240` (USA `0x1BCA50`).

The PAL engine's tick is **0.02 s**, not the USA's 1/60 s: the ctor at `0x1C389A` copies it from
`0x317C40`. Unhooked, every 16.7 ms frame at 60 fps is stepped as 20 ms, so the game runs 1.2x fast
even at a 60 fps cap. The hook steps by the real elapsed time in 1 ms ticks. The pad repeat is rescaled
against 50 polls/s, the console's PAL pace. `SIMS2_NO_STEP=1` / `SIMS2_NO_REPEAT=1` turn them off for
comparison.

Verified 2026-11-06: the log shows the hook stepping the real elapsed time
(`elapsed 0.0748 s -> 75 ticks of 0.000997 s`), with no exceptions in a 25 s run. Not yet measured:
in-game clock, needs and animations against wall time at 60 fps and uncapped.
