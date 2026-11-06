# Localización de Los Sims

Estado: **español de España pendiente**, sin cobertura comprobada en el juego.
La base se recompila desde el XBE USA. No hay archivos USA/PAL de Los Sims disponibles aún.
No se conocen aquí el formato de las tablas, su codificación ni la selección de idioma.

La solución de Los Urbz demuestra una técnica reutilizable (selección de idioma mediante
hook), pero sus direcciones, formatos y equivalencias de archivos no se extrapolan.
El SDK fijado de Los Sims tampoco tiene la misma API de idioma del SDK local de Los Urbz.

## Procedimiento al disponer de ambas copias

1. Ejecutar `python tools/game_data.py inspect "ruta" --hash-resources` para USA y PAL.
   Es lectura y permite inspeccionar un XBE no soportado. Conservar JSON en `out/audit/`.
2. Comparar SHA-1, región, título, XDK y punto de entrada del XBE; comparar inventarios,
   tamaños y SHA-256 de recursos. Coincidir el título o XDK no basta para reutilizar hooks.
3. Analizar el XBE PAL con `tools/xbe_info.py` y `xbe_symbols.exe`, escribiendo una
   configuración PAL aparte, nunca sobre `config/game.toml`.
4. Priorizar recompilación PAL; si no es viable, demostrar compatibilidad de recursos;
   si tampoco, estudiar interceptación de cadenas con fuentes y variables de formato.
5. Verificar dentro del juego tildes, ñ, signos, longitud, glifos y sustituciones dinámicas.

El importador de la base USA rechaza un XBE diferente o tamaños de recursos desconocidos.
No mezcla datos PAL/USA ni anuncia español basándose en la interfaz técnica.

Cobertura pendiente en todas las áreas: menú, Crear un Sim, Vivir, Comprar, Construir,
objetivos, opciones, diálogos, tutoriales, notificaciones, errores y guardar/cargar.
