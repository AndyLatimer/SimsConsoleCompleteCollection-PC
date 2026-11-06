# Los Sims 2 (Xbox) - PC: estado del proyecto

Estados: **Probado** (verificado jugando o con capturas) · **Sin probar** · **Pendiente** · **Bloqueado**.

## Base

| Elemento | Valor |
| --- | --- |
| Repositorio | `abilliontokens/sims2-recomp`, `8ce155d`; rama de trabajo `pc-definitive` |
| SDK | `XboxXBERecomp` `adc40ab` más los commits genéricos del fork de Urbz y Bustin' Out, rama local `pc-definitive` del submódulo |
| Disco del usuario | **solo Europa (PAL)**: `X:/projects/sims-console-complete-collection-pc/isos/Sims 2, The (Europe) (En,Fr,De,Es,It,Nl,Sv).xiso.iso` |
| XBE PAL | SHA-1 `68d7eabd...` (el USA `16f9e41c...` no está disponible) |
| Archivos extraídos | `X:/projects/sims2-pc/pal/`, enlazados (hardlinks) en `assets/` |
| Equipo | Ryzen 7 5700G, RTX 3060 Ti, monitor 2560x1080, Windows 11 |

## Hecho

| Sistema | Estado | Evidencia |
| --- | --- | --- |
| Compilación PAL (región detectada por el hash del XBE) | **Probado** | `docs/REGIONS.md`; 74 s de recompilación |
| Arranque, vídeos, menús y entrada en partida | **Probado** | partidas del usuario del 2026-11-06 |
| Castellano por defecto | **Probado** | logotipo "Los Sims 2", menú de partidas en castellano |
| Pantalla de banderas en cada arranque | **Probado** | sale España preseleccionada; no usar `SIMS2_AUTOSTART` (elige otro idioma a ciegas) |
| Cierre por llamada indirecta a `0x20BF0` | **Sin probar** | añadida en `[functions]`; el juego se cerraba a los 37,9 s de jugar |
| A → ✕ Cruz (DXT3 `dd9aa593...`) | **Probado** | visto en el volcado repintado; el usuario coge objetos con ✕ |
| X → □ Cuadrado (DXT3 `bb44466a...`) | **Sin probar** | |
| Paso temporal PAL (tick de 20 ms → tiempo real) | **Probado** (el hook se ejecuta) | log `frame step`; falta medir el reloj del juego |
| Caché de shaders en disco, nombres de mando y barra de luz (SDK) | **Sin probar** en Sims 2 | importado del SDK de Urbz |

## Pendiente (por prioridad)

1. Más iconos de botón: B, Y, la bola plateada (¿Blanco/L1?), "D" e "I" (`X:/projects/sims2-pc/button-assets/README.md`). Hacen falta formas L1/R1 en `button_glyphs.cpp`.
2. Paso temporal: medir el reloj del juego y las necesidades frente al tiempo real a 60 FPS y sin límite, comparando con `SIMS2_NO_STEP=1`.
3. Guardar, cerrar y cargar: matriz real de pruebas.
4. Mando: DualSense en el menú F1 (ajustes `pad_*` de Urbz), conexión y desconexión en caliente, jugador 2.
5. Rendimiento: medir con `framelog` y `framestats` (Urbz).
6. Pantalla dividida, 16:9 y empaquetado (`release.bat`, `manifest.json`).

## Volcado de texturas

`SIMS2_TEXDUMP=X:\projects\sims2-pc\texdump`: el usuario juega y cierra el juego; luego se revisan las
texturas que no son vídeo (los 640x480 lineales son fotogramas de vídeo).
