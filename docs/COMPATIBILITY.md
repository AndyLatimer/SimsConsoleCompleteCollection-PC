# Compatibilidad y protocolo de validación

Todos los resultados siguientes son **pendientes localmente**. El estado heredado está
separado en `PROJECT_STATUS.md`, `status.md` y `playtest.md`.

| Área | Prueba y evidencia mínima |
| --- | --- |
| Arranque | Inicio limpio, ajustes existentes/corruptos, datos ausentes e incompatibles; código de salida y log |
| Vídeos | EA, Maxis, introducción, attract y transición; imagen completa sin recorte de 24 filas |
| Menús/CAS | Texto, cámara, nombre con teclado virtual original y confirmación con mando |
| Primera casa | Movimiento, cámara, tutorial, música y acciones; log y captura |
| Guardados | Guardar, cerrar normalmente, abrir, cargar y comparar hogar, dinero, hora y relaciones |
| Errores de guardado | Copia de seguridad previa; espacio insuficiente y archivo dañado en un perfil de prueba |
| Get a Life | Nueva campaña, tutorial, objetivos, progresión de viviendas, eventos y final completo |
| Play the Sims | Crear/editar hogares, autonomía, rutas, necesidades, relaciones y trabajo |
| Comprar/construir | Colocar, retirar, rotar y modificar; guardar y recargar los cambios |
| Mandos | USB antes/después de arrancar, desconectar/reconectar, dos mandos y entrada independiente |
| Rendimiento | Escena fija y tiempo lógico a 30/60 FPS, audio sincronizado, sesiones prolongadas |
| Resoluciones | 720p/1080p/1440p/4K, ventana/sin bordes/completa, HUD y relaciones de aspecto |

Errores abiertos heredados: vídeos afectados por el recorte global, panorámico sin analizar,
gatillos DualSense sin zona muerta, segundo mando sin probar. El guardado original continúa
en `hdd/` junto al ejecutable; no se ha migrado ni alterado ningún guardado de Los Urbz.

Para la prueba manual: registrar revisión, hash XBE, hardware, ajustes, pasos, resultado y
archivo de evidencia por fila. F1 configura; F2 muestra tiempos; `XBE_CAPTURE_DIR` y F3
permiten captura de cuadro. Nunca publicar capturas de datos, partidas o contenido comercial
como parte del repositorio de fuentes. La campaña completa requiere una sesión interactiva.
