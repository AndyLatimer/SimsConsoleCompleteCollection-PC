# Cambios

## 0.1.0-dev — 2026-11-05

- Clon de sims1-recomp con historial, rama local y SDK original fijado, sin tocar Los Urbz.
- Compilación nativa de herramientas Xbox en Debug y Release.
- Detección del entorno Visual Studio mediante vswhere; presets de herramientas sin datos.
- `build.bat` acepta Debug/Release/RelWithDebInfo y valida los datos antes de recompilar.
- CMake rechaza hashes XBE ajenos a las direcciones USA y exige el SDK local.
- Inspección e importación de datos con diagnóstico JSON, copia temporal, conservación
  de originales, rechazo de sobrescritura y hashes opcionales de recursos.
- Catorce pruebas: importación con fixtures sintéticos y barreras CMake, sin datos comerciales.
- Manifiesto inicial para launcher y auditoría con cobertura y limitaciones explícitas.

Pendiente de los datos de Xbox: compilación del juego, primera ejecución y análisis PAL.
