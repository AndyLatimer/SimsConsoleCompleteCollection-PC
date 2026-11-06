# AGENTS.md - Los Sims 2 (Xbox) PC

Fase 3 de The Sims Console Complete Collection: el Sims 2 de **consola** (no el de PC), recompilado de
forma nativa, en castellano y con DualSense. El estado está en `docs/PROJECT_STATUS.md`; las regiones en
`docs/REGIONS.md`.

## Rutas
- Repo: `X:/projects/sims2-pc/sims2-recomp`, rama `pc-definitive` (también en el submódulo `sdk`).
- Disco PAL extraído: `X:/projects/sims2-pc/pal` (en `assets/` como hardlinks).
- Ports terminados, **solo lectura**: `X:/projects/sims-console-complete-collection-pc` (Sims 1),
  `X:/projects/bustinout-pc/upstream/pal`, `X:/projects/urbz-pc/urbz-recomp`.
- Logs: `out/build/release/logs/sims2*.log` (el más reciente por fecha, no `sims2.log`).

## Compilar y ejecutar
- `tools\devshell.bat cmake --preset release` y luego `tools\devshell.bat cmake --build out\build\release`.
- La región sale del SHA-1 de `assets/default.xbe`: PAL usa `config/game_pal.toml` y `src/game_pal/`.
- Ejecutar `out/build/release/Sims2.exe` (`assets` es una unión al `assets/` del repo).
- Pruebas: `SIMS2_STEP=<archivo>` para pulsaciones exactas. **No usar `SIMS2_AUTOSTART`**: en la pantalla
  de banderas cambia el idioma.

## Reglas
- No modificar los tres ports terminados.
- Nunca usar direcciones USA en PAL: las secciones XDK se enlazaron en otro orden.
- Si el log dice `add 0x... to [functions]`, comprobar en los bytes que es el inicio de una función y añadirlo a la configuración de la región.
- No editar `generated/`.
- No declarar nada probado sin haberlo ejecutado. El usuario juega y mira las ejecuciones: lanzar el juego solo cuando haga falta, y avisar antes.
