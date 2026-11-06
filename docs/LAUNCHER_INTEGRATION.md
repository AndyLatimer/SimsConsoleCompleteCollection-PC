# Contrato inicial de integración

`collection/game.json`, esquema 1: ID `the-sims-1-console`, nombre `Los Sims`, ejecutable
objetivo `Sims1.exe` y datos en `assets/`. Las rutas del manifiesto se interpretan respecto
al directorio de instalación. No se ha construido un launcher.

La API de diagnóstico disponible ahora es:

```bat
python tools/game_data.py check "RUTA\assets" --executable "RUTA\Sims1.exe"
```

Código 0: archivos esperados y ejecutable presentes; código 2: falta algo o los datos no
son compatibles. Devuelve `data_ready`, `executable_found` cuando se pudo inspeccionar,
`can_start` y diagnóstico. `can_start` es una comprobación de disponibilidad, **no una
prueba de ejecución**. `gameplay_verified` es falso. La utilidad requiere Python y el
árbol de fuentes; no se presenta como un verificador ya empaquetado dentro de Sims1.exe.

No hay argumentos públicos de lanzamiento de la colección implementados; el manifiesto
declara una lista vacía. El runtime heredado resuelve recursos desde la ruta del ejecutable
y sus carpetas ascendentes, no desde el directorio de trabajo. Falta comprobarlo con un
ejecutable real en una instalación independiente.

Guardados actuales: `hdd/` junto al ejecutable. La ruta futura bajo LocalAppData, perfiles
y migración con copia de seguridad no están implementados. No anunciar esa ruta como
disponible ni mover partidas existentes. Cada juego conserva sus recursos y proceso.

Español, segundo mando, panorámico y rendimiento local figuran como pendientes.
No distribuir todavía: falta verificar el juego y resolver la licencia del código
específico de sims1-recomp, que carece de un LICENSE en su raíz.
