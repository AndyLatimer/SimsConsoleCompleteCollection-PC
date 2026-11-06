# Rendimiento

No hay mediciones locales del juego: faltan los datos originales y `Sims1.exe`.
GPU detectada: NVIDIA GeForce RTX 3060 Ti. No implica rendimiento validado.

Se conserva la corrección temporal de `src/game/hooks.cpp`. Las pruebas heredadas en
`frame-rate.md` son de Linux y documentan tiempo lógico correcto a distintas tasas, así
como mejoras del SDK para los despertares entre hilos. No prueban necesidades, animación,
audio ni acciones sociales durante una campaña completa a tasas altas.

Configuración heredada: límite 60 FPS y VSync activado; tasas superiores siguen sin
validación local. La selección de resolución, escalado y FPS está en el panel F1.

Primera medición: mismo guardado, escena y cámara a 30 y 60 FPS; `XBE_PERF=1`, sin
`XBE_MUTE`, sin desactivar hooks temporales. Registrar tiempo simulado/real, FPS, ms de
cuadro, percentiles p50/p95/p99, memoria, audio y necesidades durante al menos 60 segundos.
Repetir en 1080p, 1440p y 4K según capacidad. Después probar VSync desactivado y sesiones
largas. Tasas 120/144/sin límite requieren validación adicional de consumidores por cuadro.

No hay optimizaciones nuevas ni comparaciones antes/después que declarar.
