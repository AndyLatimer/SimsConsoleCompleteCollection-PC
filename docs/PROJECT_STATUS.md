# Estado de la colección — 5 de noviembre de 2026

Proyecto local en `sims-console-complete-collection-pc`, rama `collection/sims1-windows`.
Base: [abilliontokens/sims1-recomp](https://github.com/abilliontokens/sims1-recomp),
commit `aa665356bc893a702cdfeea58e2ee20ed466a16a`.
SDK submódulo: `2c5c84496eaabf794d820b420c9b84ec09134f54`, sin modificaciones.

## Comprobado aquí

- Clon con historial y submódulo original inicializado.
- Detectado Los Urbz en `../urbz-pc/urbz-recomp`; solo lectura, sin compilar ni modificar.
  Sus ajustes de idioma y mando sirven como referencia de arquitectura, nunca como
  prueba de compatibilidad de recursos o direcciones de Los Sims.
- Recompilador `xbe_recomp.exe` y analizador `xbe_symbols.exe` compilados para Windows x64
  en Debug y Release. Esto **no** es una compilación del juego.
- Herramienta `tools/game_data.py`: inspección Xbox, inventario, SHA-1 del XBE,
  hashes SHA-256 opcionales de recursos, importación sin sobrescribir y diagnóstico JSON.
- Catorce pruebas automatizadas: once del importador con XBE/XISO sintéticos sin contenido
  comercial y tres de las barreras de configuración CMake.
- Configuración CMake rechaza un XBE distinto del USA antes de generar código; desaparece
  la descarga implícita de un SDK `main` diferente.

## Bloqueo actual

Faltan los datos originales de **Los Sims (2003), Xbox**. El usuario está localizándolos.
Se necesitan la ISO o los 14 archivos documentados en `iso-extract.md`; el XBE base es
`40ab6f97e8ee3bf4102a5a7820dcb8047fc30097` (USA, XDK 5233).
Para investigar español hacen falta además datos PAL con español. No se ha supuesto que
el XBE o los archivos PAL sean compatibles. Las copias PS2 localizadas no sirven para esta base.

## Evidencia heredada, no repetida aquí

`status.md` documenta arranque, audio y un DualSense en Windows real, y escenas jugables
mediante automatización en Linux. `playtest.md` aclara que se rechazó guardar en ese recorrido:
no prueba guardado/carga, interacciones, comprar/construir ni campaña completa.
`frame-rate.md` mide tiempo lógico en Linux; las cifras no son mediciones de este PC.
`.recomp.json` y fragmentos de `toolchain.md` contienen afirmaciones antiguas que contradicen
la actualización de `status.md`; se conserva su procedencia y prevalece la evidencia fechada.

## Hitos

| Hito | Estado |
| --- | --- |
| 1. Base estable | Debug y Release compilados el 6-nov-2026 con el XBE USA aportado; primera ejecución pendiente |
| 2. Español de España | Pendiente de análisis USA/PAL y comprobación dentro del juego |
| 3. DualSense | Soporte heredado, sin validación local |
| 4. 60 FPS | Corrección temporal heredada conservada; sin medición local |
| 5. Gráficos | Recorte de vídeos y panorámico pendientes; sin cambios especulativos |
| 6. Compatibilidad completa | Campaña, modo libre, guardados y multijugador pendientes |
| 7. Distribución | Contrato inicial; paquete y licencia del código específico pendientes |

Actualización 6-nov-2026: datos USA importados (variante en `USA_DISC_VARIANT.md`).
`build.bat debug` y `build.bat release` terminan sin errores: `out/build/debug/Sims1.exe`
y `out/build/release/Sims1.exe` (registros `build-game-debug-20261106-run2.log` y
`build-game-release-20261106.log`). Compilar no prueba jugabilidad.

Primera ejecución del usuario (6-nov): inglés, "sin memoria para guardar" y imagen cortada en
2560x1080. Corregido (registro `out/build/release/logs/sims1_002.log`):
- Guardado: la comprobación de DVD de XAPI `0xCA891` fallaba sin unidad y el arranque volvía antes
  de montar T:/U:. Hook en `src/game/hooks.cpp` (`SIMS1_NO_MEDIACHECK=1` lo desactiva). Ahora se
  crean los enlaces T:/U: y `hdd/E/UDATA/4541002f/`.
- Español: los 13 archivos de datos USA y PAL tienen SHA-256 idéntico; solo difiere el XBE. La USA
  guarda 0 en `[0x323C20]` (`0x1135B`) donde la PAL guarda el idioma de consola (español = 4).
  `src/game/language.cpp` + `[[hook]]` en `config/game.toml`; `language` en `settings.toml`.
- Imagen: el juego solo proyecta 4:3; `auto` pasa a 4:3 con bandas laterales. Cambio de una línea
  en `sdk/runtime/app/settings.cpp` (el submódulo SDK ya no está sin modificaciones).
Confirmado por el usuario: español, guardado y 4:3.

Iconos PlayStation (6-nov): los botones A/B/X/Y son cuatro texturas P8 32x32 (halladas con
`SIMS1_TEXDUMP`). `src/common/button_glyphs.cpp`, extensión D3D12, redibuja su centro como ✕ ○ □ △
(A, B, X, Y por posición); `button_glyphs = 0` en `settings.toml` deja los de Xbox. Requiere que
`Extension::Decoded` reciba los píxeles sin `const` (`sdk/runtime/gpu/ext.h`, segundo cambio al SDK).
Comprobado ✕ y ○ en ejecución; □ y △ solo por la geometría. Gatillos y Black/White no se vieron.

Próxima acción: registrar arranque hasta casa y ciclo guardar/cargar.
No se afirma que el juego sea jugable o esté traducido.

Registros locales en `out/audit/`: `data-tests.log`, `readiness.json`,
`configure-game-debug.log`, `configure-game-release.log` y `build-game-debug.log`.
Las configuraciones del juego fallan expresamente por datos ausentes; no son builds aprobadas.
