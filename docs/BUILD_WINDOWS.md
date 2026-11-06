# Compilación Windows reproducible

Revisiones base y SDK en `PROJECT_STATUS.md`. Submódulo inicializado con
`git submodule update --init --recursive` (sin `--remote`).

Herramientas detectadas el 5-11-2026: Git 2.56.0.windows.2, Python 3.12,
Clang/LLVM 19.1.5 de Visual Studio Build Tools 2022, CMake 3.31.6-msvc6,
Ninja 1.12.1, MSVC 14.44.35207 y Windows SDK 10.0.22621.0 / 10.0.26100.0.
No se instala ni actualiza un SDK sin necesidad. El entorno usa `vswhere` y encuentra
LLVM/CMake/Ninja de Visual Studio, sin depender de `C:\Program Files\LLVM`.

Desde **cmd.exe** en la raíz:

```bat
call tools\env.bat
cmake --preset tools-debug
cmake --build --preset tools-debug
cmake --preset tools-release
cmake --build --preset tools-release
```

Estos presets no necesitan datos. Producen `xbe_recomp.exe` y `xbe_symbols.exe` en
`out/build/tools-debug/` y `out/build/tools-release/`. No producen el juego.
Dependencias de herramientas: Zydis v4.1.1 (con Zycore) y toml++ v3.4.0.
El runtime añade SDL3 release-3.4.16 e ImGui v1.91.9b, y enlaza D3D12/XAudio2 del SDK Windows.

Con datos propios de **Xbox**:

```bat
python tools\game_data.py inspect "RUTA\Los Sims (USA).iso"
python tools\game_data.py import "RUTA\Los Sims (USA).iso"
set CI=1
call .\build.bat debug
call .\build.bat release
```

`CI=1` evita las preguntas interactivas del script oficial. La importación admite una
carpeta extraída, crea `assets/` solo si no existe y mantiene los originales. Si ya hay
datos, inspeccionarlos antes: `python tools/game_data.py prepare` comprueba el conjunto y
extrae solo el XBE de una ISO en assets. Más de una ISO requiere selección explícita.

Se exigen SHA-1 USA y tamaños de los 14 archivos de `collection/game.json`. PAL se
inspecciona aparte, sin cambiar hooks ni sobrescribir configuración. CMake también
rechaza hashes incompatibles cuando se invoca sin `build.bat`.

Pruebas sin datos: `python -m unittest discover -s tests -v` después de `tools/env.bat`.
Son catorce pruebas; las tres de CMake se omiten si el entorno de compilación no está cargado.
`python tools/game_data.py check` devuelve JSON y código 2 mientras falten datos o ejecutable.
La compilación completa Debug/Release está pendiente de datos; no hay un ejecutable jugable
producido en esta sesión. Las advertencias de compatibilidad CMake de Zydis son heredadas.
